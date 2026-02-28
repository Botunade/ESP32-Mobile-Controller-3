#include "webserver.h"

const char* DASHBOARD_HTML = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Pressure Control System</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet">
    <style>
        :root {
            --bg-body: #f8fafc;
            --bg-card: #ffffff;
            --border: #e2e8f0;
            --text-main: #0f172a;
            --text-muted: #64748b;
            --primary: #0ea5e9;
            --primary-hover: #0284c7;
            --success: #10b981;
            --danger: #ef4444;
            --shadow-sm: 0 1px 3px rgba(0,0,0,0.05);
            --shadow-md: 0 4px 6px -1px rgba(0,0,0,0.1), 0 2px 4px -1px rgba(0,0,0,0.06);
            --radius-md: 12px;
            --radius-lg: 16px;
        }

        * { margin: 0; padding: 0; box-sizing: border-box; font-family: 'Inter', sans-serif; }
        
        body {
            background-color: var(--bg-body);
            color: var(--text-main);
            padding: 2rem 1rem;
            line-height: 1.5;
            -webkit-font-smoothing: antialiased;
        }

        .container {
            max-width: 1000px;
            margin: 0 auto;
            display: flex;
            flex-direction: column;
            gap: 1.5rem;
        }

        /* Header Area */
        .header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            background: var(--bg-card);
            padding: 1.5rem 2rem;
            border-radius: var(--radius-lg);
            box-shadow: var(--shadow-sm);
            border: 1px solid var(--border);
        }

        h1 { font-size: 1.5rem; font-weight: 700; letter-spacing: -0.025em; }

        .status-badge {
            display: inline-flex;
            align-items: center;
            gap: 0.5rem;
            padding: 0.5rem 1rem;
            border-radius: 9999px;
            font-size: 0.875rem;
            font-weight: 600;
            background: #f1f5f9;
            color: var(--text-muted);
            transition: all 0.3s ease;
        }

        .status-badge.running {
            background: #dcfce7;
            color: #166534;
        }

        .status-badge.running::before {
            content: '';
            width: 8px;
            height: 8px;
            border-radius: 50%;
            background: var(--success);
            box-shadow: 0 0 0 rgba(16, 185, 129, 0.4);
            animation: pulse-green 2s infinite;
        }

        /* Error Banner */
        .error-banner {
            display: none;
            background: #fef2f2;
            color: #991b1b;
            padding: 1rem;
            border-radius: var(--radius-md);
            border-left: 4px solid var(--danger);
            font-weight: 500;
            font-size: 0.9rem;
            box-shadow: var(--shadow-sm);
        }

        /* Grid Layouts */
        .grid-top {
            display: grid;
            grid-template-columns: 1fr 2fr;
            gap: 1.5rem;
        }

        .card {
            background: var(--bg-card);
            border: 1px solid var(--border);
            border-radius: var(--radius-lg);
            padding: 1.5rem;
            box-shadow: var(--shadow-sm);
            transition: box-shadow 0.3s ease;
        }
        .card:hover { box-shadow: var(--shadow-md); }

        .card-header {
            font-size: 0.875rem;
            font-weight: 600;
            color: var(--text-muted);
            text-transform: uppercase;
            letter-spacing: 0.05em;
            margin-bottom: 1.25rem;
        }

        /* Metrics List */
        .metrics-list { display: flex; flex-direction: column; gap: 1.25rem; }
        
        .metric-item {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding-bottom: 0.75rem;
            border-bottom: 1px solid #f1f5f9;
        }
        .metric-item:last-child { border-bottom: none; padding-bottom: 0; }

        .metric-label { font-size: 0.9rem; color: var(--text-muted); font-weight: 500;}
        .metric-value { font-size: 1.25rem; font-weight: 700; font-variant-numeric: tabular-nums; }
        .metric-unit { font-size: 0.875rem; color: var(--text-muted); font-weight: 500; margin-left: 2px;}

        /* Chart Area */
        .chart-container {
            position: relative;
            height: 300px;
            width: 100%;
        }

        /* Settings Grid */
        .settings-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 1.5rem;
        }

        .input-group {
            display: flex;
            flex-direction: column;
            gap: 0.5rem;
        }
        
        .input-group label {
            font-size: 0.875rem;
            font-weight: 500;
            color: var(--text-muted);
        }

        input[type="number"], input[type="text"], input[type="password"] {
            width: 100%;
            padding: 0.75rem 1rem;
            border: 1px solid var(--border);
            border-radius: var(--radius-md);
            font-size: 1rem;
            color: var(--text-main);
            background: #f8fafc;
            transition: all 0.2s ease;
            outline: none;
        }
        input[type="number"]:focus, input[type="text"]:focus, input[type="password"]:focus {
            background: var(--bg-card);
            border-color: var(--primary);
            box-shadow: 0 0 0 3px rgba(14, 165, 233, 0.1);
        }

        .btn-update {
            margin-top: 1.5rem;
            width: 100%;
            padding: 1rem;
            background: var(--primary);
            color: white;
            border: none;
            border-radius: var(--radius-md);
            font-size: 1rem;
            font-weight: 600;
            cursor: pointer;
            transition: background 0.2s ease, transform 0.1s ease;
        }
        .btn-update:hover { background: var(--primary-hover); }
        .btn-update:active { transform: scale(0.98); }

        /* Modal Styles */
        .modal-overlay {
            display: none;
            position: fixed;
            top: 0; left: 0; width: 100%; height: 100%;
            background: rgba(15, 23, 42, 0.8);
            backdrop-filter: blur(4px);
            z-index: 1000;
            align-items: center;
            justify-content: center;
        }
        .modal {
            background: white;
            padding: 2.5rem;
            border-radius: var(--radius-lg);
            text-align: center;
            max-width: 400px;
            width: 90%;
            box-shadow: var(--shadow-md);
            animation: modal-pop 0.3s ease;
        }
        @keyframes modal-pop {
            from { opacity: 0; transform: scale(0.9); }
            to { opacity: 1; transform: scale(1); }
        }

        .loader {
            width: 48px; height: 48px;
            border: 5px solid #f3f3f3;
            border-top: 5px solid var(--primary);
            border-radius: 50%;
            animation: spin 1s linear infinite;
            margin: 0 auto 1.5rem;
        }
        @keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }

        /* Status Grid Updates */
        .config-summary {
            padding: 1rem;
            background: #f1f5f9;
            border-radius: var(--radius-md);
            font-size: 0.85rem;
            margin-bottom: 1rem;
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 0.5rem;
        }
        .config-label { font-weight: 600; color: var(--text-muted); }

        @keyframes pulse-green {
            0% { box-shadow: 0 0 0 0 rgba(16, 185, 129, 0.4); }
            70% { box-shadow: 0 0 0 6px rgba(16, 185, 129, 0); }
            100% { box-shadow: 0 0 0 0 rgba(16, 185, 129, 0); }
        }

        /* Responsive */
        @media (max-width: 768px) {
            .grid-top { grid-template-columns: 1fr; }
            .header { flex-direction: column; gap: 1rem; text-align: center; }
        }
    </style>
</head>
<body>
    <div class="container">
        <!-- Reboot Modal -->
        <div id="save-modal" class="modal-overlay">
            <div class="modal">
                <div class="loader"></div>
                <h2 style="margin-bottom: 0.5rem;">Settings Saved!</h2>
                <p id="reboot-msg" style="color: var(--text-muted);">The system is rebooting to apply changes. Please wait <span id="countdown">5</span> seconds...</p>
            </div>
        </div>

        <!-- Header -->
        <header class="header">
            <div>
                <h1>Pressure Control</h1>
            </div>
            <div id="status" class="status-badge">SYSTEM IDLE</div>
        </header>

        <!-- Warnings -->
        <div id="static-warning" class="error-banner">
            ⚠️ WARNING: PID output is not changing. Please check scaling or sensor connection.
        </div>

        <!-- Utility Buttons -->
        <div class="card" style="margin-bottom: 1.5rem; display: flex; justify-content: space-between; align-items: center;">
            <div>
                <h3 style="margin-bottom: 0.25rem;">Solenoid Valve Override</h3>
                <p style="font-size: 0.875rem; color: var(--text-muted);">Manually trigger the relay to open/close the valve.</p>
            </div>
            <button id="btn-solenoid" class="btn-solenoid-toggle" data-state="off" style="width: auto; background: var(--border); color: var(--text-main); margin-top: 0; padding: 0.75rem 1.5rem; border-radius: 8px; border: 1px solid var(--border); cursor: pointer; font-weight: 600;" onclick="toggleSolenoid()">
                VALVE OFF
            </button>
        </div>

        <!-- Top Grid -->
        <div class="grid-top">
            <!-- Live Metrics -->
            <div class="card">
                <div class="card-header">Telemetry</div>
                <div class="metrics-list">
                    <div class="metric-item">
                        <span class="metric-label">Pressure</span>
                        <div>
                            <span id="pressure" class="metric-value" style="color: var(--primary);">0.0</span>
                            <span class="metric-unit">BAR</span>
                        </div>
                    </div>
                    <div class="metric-item">
                        <span class="metric-label">Target Setpoint</span>
                        <div>
                            <span id="setpoint-display" class="metric-value">0.0</span>
                            <span class="metric-unit">BAR</span>
                        </div>
                    </div>
                    <div class="metric-item">
                        <span class="metric-label">Air Volume</span>
                        <div>
                            <span id="airVolume" class="metric-value" style="color: #6366f1;">0.0</span>
                            <span class="metric-unit">L</span>
                        </div>
                    </div>
                    <div class="metric-item">
                        <span class="metric-label">Control Voltage</span>
                        <div>
                            <span id="voltage" class="metric-value" style="color: var(--text-muted);">0.00</span>
                            <span class="metric-unit">V</span>
                        </div>
                    </div>
                </div>
            </div>

            <!-- Diagnostics -->
            <div class="card">
                <div class="card-header">Advanced Diagnostics</div>
                <div class="metrics-list">
                    <div class="metric-item">
                        <span class="metric-label">Filtered ADC</span>
                        <div>
                            <span id="rawADC" class="metric-value" style="color: var(--text-muted); font-size: 1rem;">0</span>
                            <span class="metric-unit">Units</span>
                        </div>
                    </div>
                    <div class="metric-item">
                        <span class="metric-label">Sensor Raw</span>
                        <div>
                            <span id="sensorV" class="metric-value" style="color: var(--text-muted); font-size: 1rem;">0.00</span>
                            <span class="metric-unit">V</span>
                        </div>
                    </div>
                    <div class="metric-item">
                        <span class="metric-label">DAC / PWM</span>
                        <div>
                            <span id="dacValue" class="metric-value" style="color: var(--text-muted); font-size: 1rem;">0</span>
                            <span class="metric-unit">/ 1023</span>
                        </div>
                    </div>
                    <div class="metric-item">
                        <span class="metric-label">PID Raw Output</span>
                        <div>
                            <span id="pidOut" class="metric-value" style="color: var(--text-muted); font-size: 1rem;">0.00</span>
                            <span class="metric-unit">V</span>
                        </div>
                    </div>
                    <div class="metric-item">
                        <span class="metric-label">Resampled Scale</span>
                        <div>
                            <span id="scaled3v3" class="metric-value" style="color: #10b981; font-weight: bold; font-size: 1rem;">0.00</span>
                            <span class="metric-unit">V (0.66-3.3)</span>
                        </div>
                    </div>
                </div>
            </div>

            <!-- Chart -->
            <div class="card">
                <div class="card-header">Live Pressure Trend</div>
                <div class="chart-container">
                    <canvas id="pressureChart"></canvas>
                </div>
            </div>
        </div>

        <!-- Logic Rules Card -->
        <div class="card" style="background: #fafafa; border-style: dashed;">
            <div class="card-header" style="color: var(--primary);">System Calibration Rules</div>
            <div style="font-size: 0.85rem; display: grid; grid-template-columns: 1fr 1fr; gap: 1rem;">
                <div>
                    <p><strong>ADC Logic:</strong> V = (ADC/4095) * 3.3</p>
                    <p><strong>BAR Mapping:</strong> (V - Vmin) * (MaxBAR / (Vmax - Vmin))</p>
                </div>
                <div>
                    <p><strong>Safety:</strong> Bypass Open at Setpoint + 0.5 BAR</p>
                    <p><strong>Limit:</strong> Tank max 8L / Sensor max 12 BAR</p>
                </div>
            </div>
        </div>

        <!-- Settings Grid -->
        <div class="card">
            <div class="card-header">System Configuration</div>
            
            <div class="config-summary">
                <div><span class="config-label">Active SSID:</span> <span id="active-ssid">-</span></div>
                <div><span class="config-label">Tank Size:</span> <span id="active-tank">-</span></div>
            </div>

            <div class="settings-grid">
                <div class="input-group">
                    <label for="tankVol">Tank Size (L, Max 8)</label>
                    <input type="number" id="tankVol" step="0.1" max="8">
                </div>
                <div class="input-group">
                    <label for="kp">Proportional (Kp)</label>

                    <input type="number" id="kp" step="0.1">
                </div>
                <div class="input-group">
                    <label for="ki">Integral (Ki)</label>
                    <input type="number" id="ki" step="0.1">
                </div>
                <div class="input-group">
                    <label for="kd">Derivative (Kd)</label>
                    <input type="number" id="kd" step="0.01">
                </div>
                <div class="input-group">
                    <label for="setpoint">Setpoint (BAR)</label>
                    <input type="number" id="setpoint" step="0.1" max="12">
                </div>
                <div class="input-group">
                    <label for="safeAllow">Safety Allowance (BAR)</label>
                    <input type="number" id="safeAllow" step="0.1" value="0.5">
                </div>
                <div class="input-group">
                    <label for="minV">DAC Min (V)</label>
                    <input type="number" id="minV" step="0.1">
                </div>
                <div class="input-group">
                    <label for="maxV">DAC Max (V)</label>
                    <input type="number" id="maxV" step="0.1">
                </div>
            </div>

            <hr style="margin: 1.5rem 0; border: none; border-top: 1px solid var(--border);">
            <div class="card-header">Sensor Scaling (4-20mA / 117Ω)</div>
            <div class="settings-grid">
                <div class="input-group">
                    <label for="sMinV">Sensor Min (V)</label>
                    <input type="number" id="sMinV" step="0.001">
                </div>
                <div class="input-group">
                    <label for="sMaxV">Sensor Max (V)</label>
                    <input type="number" id="sMaxV" step="0.001">
                </div>
                <div class="input-group">
                    <label for="sMaxP">Sensor Full Scale (BAR)</label>
                    <input type="number" id="sMaxP" step="0.1">
                </div>
                <div class="input-group">
                    <label for="wMaxP">Working Max (BAR)</label>
                    <input type="number" id="wMaxP" step="0.1">
                </div>
                <div class="input-group">
                    <label for="aMinV">Acc. Scale Min (V)</label>
                    <input type="number" id="aMinV" step="0.01">
                </div>
                <div class="input-group">
                    <label for="aMaxV">Acc. Scale Max (V)</label>
                    <input type="number" id="aMaxV" step="0.01">
                </div>
            </div>
            <div style="margin-top: 1rem; padding: 0.75rem; background: var(--bg); border-radius: 8px; border: 1px solid var(--border); display: flex; justify-content: space-around; font-size: 0.9rem;">
                <div><span style="color: var(--text-muted);">Live Raw:</span> <span id="liveSensorV" style="font-weight: bold; color: var(--primary);">0.00</span> V</div>
                <div><span style="color: var(--text-muted);">Live Acc:</span> <span id="liveScaledV" style="font-weight: bold; color: #10b981;">0.00</span> V</div>
            </div>
            
            <hr style="margin: 1.5rem 0; border: none; border-top: 1px solid var(--border);">
            <div class="card-header">Wi-Fi Configuration (For Cloud Mode)</div>
            <div class="settings-grid">
                <div class="input-group">
                    <label for="wifiSSID">WiFi SSID</label>
                    <input type="text" id="wifiSSID" placeholder="Network Name">
                </div>
                <div class="input-group">
                    <label for="wifiPassword">WiFi Password</label>
                    <input type="password" id="wifiPassword" placeholder="**********">
                </div>
            </div>
            
            <button id="btn-save-settings" class="btn-update" onclick="updateSettings()">Apply Configuration</button>
        </div>
    </div>

    <script>
        let chart;
        const maxDataPoints = 60; // 30 seconds at 500ms intervals
        const dataHistory = Array(maxDataPoints).fill(null);
        const setpointHistory = Array(maxDataPoints).fill(null);
        const workingMaxHistory = Array(maxDataPoints).fill(null);
        const labels = Array(maxDataPoints).fill('');

        function initChart() {
            const ctx = document.getElementById('pressureChart').getContext('2d');
            
            // Create Gradient
            const gradient = ctx.createLinearGradient(0, 0, 0, 300);
            gradient.addColorStop(0, 'rgba(14, 165, 233, 0.2)');
            gradient.addColorStop(1, 'rgba(14, 165, 233, 0)');

            chart = new Chart(ctx, {
                type: 'line',
                data: {
                    labels: labels,
                    datasets: [
                        {
                            label: 'Pressure',
                            data: dataHistory,
                            borderColor: '#0ea5e9',
                            backgroundColor: gradient,
                            borderWidth: 2,
                            fill: true,
                            tension: 0.4,
                            pointRadius: 0,
                            pointHitRadius: 10
                        },
                        {
                            label: 'Setpoint',
                            data: setpointHistory,
                            borderColor: '#cbd5e1',
                            borderWidth: 2,
                            borderDash: [5, 5],
                            fill: false,
                            tension: 0,
                            pointRadius: 0
                        },
                        {
                            label: 'Working Max',
                            data: workingMaxHistory,
                            borderColor: '#ef4444',
                            borderDash: [5, 5],
                            borderWidth: 1.5,
                            fill: false,
                            tension: 0,
                            pointRadius: 0
                        }
                    ]
                },
                options: { 
                    responsive: true,
                    maintainAspectRatio: false,
                    animation: false,
                    interaction: {
                        mode: 'index',
                        intersect: false,
                    },
                    plugins: {
                        legend: { display: false },
                        tooltip: {
                            backgroundColor: 'rgba(15, 23, 42, 0.9)',
                            titleFont: { family: 'Inter', size: 13 },
                            bodyFont: { family: 'Inter', size: 13 },
                            padding: 10,
                            cornerRadius: 8,
                            displayColors: true
                        }
                    },
                    scales: { 
                        y: { 
                            beginAtZero: true,
                            grid: { color: '#f1f5f9', drawBorder: false },
                            ticks: { font: { family: 'Inter' }, color: '#64748b' }
                        },
                        x: {
                            grid: { display: false, drawBorder: false },
                            ticks: { display: false }
                        }
                    } 
                }
            });
        }

        async function fetchData() {
            try {
                const response = await fetch('/data');
                const data = await response.json();
                
                // Update text elements
                document.getElementById('pressure').innerText = data.pressure.toFixed(2);
                document.getElementById('voltage').innerText = data.voltage.toFixed(2);
                document.getElementById('setpoint-display').innerText = data.setpoint.toFixed(2);
                document.getElementById('airVolume').innerText = (data.airVolume || 0).toFixed(2);
                
                // Diagnostics
                document.getElementById('rawADC').innerText = Math.round(data.rawADC);
                document.getElementById('sensorV').innerText = data.sensorV.toFixed(2);
                document.getElementById('dacValue').innerText = data.dacValue;
                document.getElementById('pidOut').innerText = data.pidOut.toFixed(2);
                document.getElementById('scaled3v3').innerText = (data.scaled3v3 || 0).toFixed(2);
                if (document.getElementById('liveSensorV')) document.getElementById('liveSensorV').innerText = data.sensorV.toFixed(2);
                if (document.getElementById('liveScaledV')) document.getElementById('liveScaledV').innerText = (data.scaled3v3 || 0).toFixed(2);
                
                // Update Status Badge
                const statusEl = document.getElementById('status');
                if(data.active) {
                    statusEl.innerText = 'SYSTEM RUNNING';
                    statusEl.className = 'status-badge running';
                } else {
                    statusEl.innerText = 'SYSTEM IDLE';
                    statusEl.className = 'status-badge';
                }

                // Solenoid Button State Update
                if(!isUpdatingSolenoid) {
                    const solBtn = document.getElementById('btn-solenoid');
                    if(data.solenoid) {
                        solBtn.innerText = 'VALVE ON';
                        solBtn.style.background = 'var(--primary)';
                        solBtn.style.color = 'white';
                        solBtn.setAttribute('data-state', 'on');
                    } else {
                        solBtn.innerText = 'VALVE OFF';
                        solBtn.style.background = 'var(--border)';
                        solBtn.style.color = 'var(--text-main)';
                        solBtn.setAttribute('data-state', 'off');
                    }
                }

                // Warnings
                document.getElementById('static-warning').style.display = data.isStatic ? 'block' : 'none';

                // Update Chart Arrays
                dataHistory.push(data.pressure);
                setpointHistory.push(data.setpoint);
                workingMaxHistory.push(data.wMaxP || 0);
                labels.push('');
                
                if (dataHistory.length > maxDataPoints) {
                    dataHistory.shift();
                    setpointHistory.shift();
                    workingMaxHistory.shift();
                    labels.shift();
                }
                chart.update('none');

                // Populate Form (only if empty to not overwrite user typing)
                if (!document.getElementById('tankVol').value) {
                    document.getElementById('tankVol').value = data.tankVol;
                    document.getElementById('kp').value = data.kp;
                    document.getElementById('ki').value = data.ki;
                    document.getElementById('kd').value = data.kd;
                    document.getElementById('setpoint').value = data.setpoint;
                    document.getElementById('safeAllow').value = data.safeAllow;
                    document.getElementById('minV').value = data.minV;
                    document.getElementById('maxV').value = data.maxV;
                    document.getElementById('sMinV').value = data.sMinV;
                    document.getElementById('sMaxV').value = data.sMaxV;
                    document.getElementById('sMaxP').value = data.sMaxP;
                    document.getElementById('wMaxP').value = data.wMaxP;
                    document.getElementById('aMinV').value = data.aMinV;
                    document.getElementById('aMaxV').value = data.aMaxV;
                    document.getElementById('wifiSSID').value = data.wifiSSID || '';
                    document.getElementById('wifiPassword').value = data.wifiPassword || '';
                }

                // Update configuration summary
                document.getElementById('active-ssid').innerText = data.wifiSSID || 'NONE';
                document.getElementById('active-tank').innerText = data.tankVol + " L";
            } catch (e) { console.error('Data fetch failed', e); }
        }

        let isUpdatingSolenoid = false; // Flag to prevent jitter during fetch
        async function toggleSolenoid() {
            const btn = document.getElementById('btn-solenoid');
            const state = btn.getAttribute('data-state');
            const turnOn = (state !== 'on');
            
            // 1. Instant UI Feedback (Optimistic)
            isUpdatingSolenoid = true;
            if(turnOn) {
                btn.innerText = 'VALVE ON';
                btn.style.background = 'var(--primary)';
                btn.style.color = 'white';
                btn.setAttribute('data-state', 'on');
            } else {
                btn.innerText = 'VALVE OFF';
                btn.style.background = 'var(--border)';
                btn.style.color = 'var(--text-main)';
                btn.setAttribute('data-state', 'off');
            }

            try {
                await fetch('/update?solenoid=' + (turnOn ? '1' : '0'));
                // Briefly wait before allowing fetchData to overwrite UI
                setTimeout(() => { isUpdatingSolenoid = false; }, 1500);
            } catch(e) { 
                console.error('Failed to toggle solenoid', e); 
                isUpdatingSolenoid = false;
            }
        }

        async function updateSettings() {
            const btn = document.getElementById('btn-save-settings');
            const originalText = btn.innerText;
            btn.innerText = 'Saving...';
            btn.style.opacity = '0.7';

            try {
                const params = new URLSearchParams({
                    tankVol: document.getElementById('tankVol').value,
                    kp: document.getElementById('kp').value,
                    ki: document.getElementById('ki').value,
                    kd: document.getElementById('kd').value,
                    setpoint: document.getElementById('setpoint').value,
                    safeAllow: document.getElementById('safeAllow').value,
                    minV: document.getElementById('minV').value,
                    maxV: document.getElementById('maxV').value,
                    sMinV: document.getElementById('sMinV').value,
                    sMaxV: document.getElementById('sMaxV').value,
                    sMaxP: document.getElementById('sMaxP').value,
                    wMaxP: document.getElementById('wMaxP').value,
                    aMinV: document.getElementById('aMinV').value,
                    aMaxV: document.getElementById('aMaxV').value,
                    wifiSSID: document.getElementById('wifiSSID').value,
                    wifiPassword: document.getElementById('wifiPassword').value
                });
                await fetch('/update?' + params.toString());
                
                // Show Reboot Modal
                document.getElementById('save-modal').style.display = 'flex';
                let counts = 5;
                const timer = setInterval(async () => {
                    counts--;
                    document.getElementById('countdown').innerText = counts;
                    if (counts <= 0) {
                        clearInterval(timer);
                        document.getElementById('reboot-msg').innerText = "Rebooting now... Reconnecting in a moment.";
                        await fetch('/restart');
                    }
                }, 1000);

            } catch (e) {
                btn.innerText = 'Error Saving';
                btn.style.background = 'var(--danger)';
                setTimeout(() => {
                    btn.innerText = originalText;
                    btn.style.background = '';
                }, 2000);
            }
        }

        initChart();
        setInterval(fetchData, 500);
    </script>
</body>
</html>
)=====";

PressureWebServer::PressureWebServer() : _server(80), _settings(nullptr), _state(nullptr) {}

void PressureWebServer::begin(SystemSettings* settings, SystemState* state) {
    _settings = settings;
    _state = state;

    const int WIFI_LED_PIN = 2; // ESP32 built-in blue LED
    pinMode(WIFI_LED_PIN, OUTPUT);
    digitalWrite(WIFI_LED_PIN, LOW);

    if (strlen(_settings->wifiSSID) > 0) {
        WiFi.mode(WIFI_AP_STA);
        WiFi.begin(_settings->wifiSSID, _settings->wifiPassword);
        Serial.print("Connecting to WiFi: ");
        Serial.println(_settings->wifiSSID);
    } else {
        WiFi.mode(WIFI_AP);
    }

    IPAddress local_IP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(local_IP, gateway, subnet);
    WiFi.softAP("PressureControl_AP");
    IPAddress IP = WiFi.softAPIP();

    _dnsServer.start(53, "*", IP);

    setupRoutes();
    _server.begin();
}

void PressureWebServer::handle() {
    _dnsServer.processNextRequest();
    _server.handleClient();

    // Non-blocking WiFi LED Status Logic (GPIO 2)
    static unsigned long lastBlink = 0;
    static bool ledState = false;

    if (_settings && strlen(_settings->wifiSSID) > 0) {
        if (WiFi.status() == WL_CONNECTED) {
            digitalWrite(2, HIGH); // Solid ON when connected
        } else {
            // Blink every 500ms while connecting / disconnected
            if (millis() - lastBlink >= 500) {
                lastBlink = millis();
                ledState = !ledState;
                digitalWrite(2, ledState ? HIGH : LOW);
            }
        }
    } else {
        digitalWrite(2, LOW); // LED OFF if no WiFi configured
    }
}

void PressureWebServer::setupRoutes() {
    _server.on("/", HTTP_GET, std::bind(&PressureWebServer::handleRoot, this));
    _server.on("/data", HTTP_GET, std::bind(&PressureWebServer::handleData, this));
    _server.on("/update", HTTP_GET, std::bind(&PressureWebServer::handleUpdate, this));
    _server.on("/restart", HTTP_GET, [this]() {
        _server.send(200, "text/plain", "OK");
        delay(500);
        ESP.restart();
    });
    
    // Captive Portal Probes
    _server.on("/generate_204", std::bind(&PressureWebServer::handleRoot, this));  // Android
    _server.on("/success.txt", std::bind(&PressureWebServer::handleRoot, this));   // Apple
    _server.on("/hotspot-detect.html", std::bind(&PressureWebServer::handleRoot, this)); // Apple
    _server.on("/canonical.html", std::bind(&PressureWebServer::handleRoot, this)); // Chrome/Android
    _server.on("/connecttest.txt", std::bind(&PressureWebServer::handleRoot, this)); // Windows
    _server.on("/favicon.ico", []() {}); // Silence favicon requests
    
    _server.onNotFound(std::bind(&PressureWebServer::handleNotFound, this));
}

void PressureWebServer::handleRoot() {
    _server.send(200, "text/html", DASHBOARD_HTML);
}

void PressureWebServer::handleData() {
    JsonDocument doc;
    doc["pressure"] = _state->pressure;
    doc["voltage"] = _state->controlVoltage;
    doc["sensorV"] = _state->sensorVoltage;
    doc["rawADC"] = _state->rawADC;
    doc["dacValue"] = _state->dacValue;
    doc["pidOut"] = _state->pidOutput;
    doc["isStatic"] = _state->isStatic;
    doc["active"] = _state->systemActive;
    doc["solenoid"] = _state->solenoidState;
    doc["airVolume"] = _state->airVolume;
    doc["scaled3v3"] = _state->scaledTo3v3;
    doc["tankVol"] = _settings->tankVolume;
    doc["kp"] = _settings->kp;
    doc["ki"] = _settings->ki;
    doc["kd"] = _settings->kd;
    doc["setpoint"] = _settings->setpoint;
    doc["safeAllow"] = _settings->safetyAllowance;
    doc["minV"] = _settings->minVoltage;    
    doc["maxV"] = _settings->maxVoltage;
    doc["sMinV"] = _settings->sensorMinV;
    doc["sMaxV"] = _settings->sensorMaxV;
    doc["sMaxP"] = _settings->sensorMaxBar;
    doc["wMaxP"] = _settings->workingMaxBar;
    doc["aMinV"] = _settings->accuracyMinV;
    doc["aMaxV"] = _settings->accuracyMaxV;
    doc["wifiSSID"] = _settings->wifiSSID;

    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void PressureWebServer::handleUpdate() {
    if (_server.hasArg("tankVol")) _settings->tankVolume = _server.arg("tankVol").toInt();
    if (_server.hasArg("sMinV")) _settings->sensorMinV = _server.arg("sMinV").toFloat();
    if (_server.hasArg("sMaxV")) _settings->sensorMaxV = _server.arg("sMaxV").toFloat();
    if (_server.hasArg("sMaxP")) _settings->sensorMaxBar = _server.arg("sMaxP").toFloat();
    if (_server.hasArg("wMaxP")) _settings->workingMaxBar = _server.arg("wMaxP").toFloat();
    if (_server.hasArg("aMinV")) _settings->accuracyMinV = _server.arg("aMinV").toFloat();
    if (_server.hasArg("aMaxV")) _settings->accuracyMaxV = _server.arg("aMaxV").toFloat();

    if (_server.hasArg("solenoid")) {
        _state->solenoidState = (_server.arg("solenoid") == "1");
        Serial.print("Local Solenoid Command: ");
        Serial.println(_state->solenoidState ? "OPEN" : "CLOSED");
    }
    if (_server.hasArg("kp")) _settings->kp = _server.arg("kp").toFloat();
    if (_server.hasArg("ki")) _settings->ki = _server.arg("ki").toFloat();
    if (_server.hasArg("kd")) _settings->kd = _server.arg("kd").toFloat();
    if (_server.hasArg("setpoint")) _settings->setpoint = _server.arg("setpoint").toFloat();
    if (_server.hasArg("safeAllow")) _settings->safetyAllowance = _server.arg("safeAllow").toFloat();
    if (_server.hasArg("minV")) _settings->minVoltage = _server.arg("minV").toFloat();
    if (_server.hasArg("maxV")) _settings->maxVoltage = _server.arg("maxV").toFloat();
    
    if (_server.hasArg("wifiSSID")) {
        String ssid = _server.arg("wifiSSID");
        strncpy(_settings->wifiSSID, ssid.c_str(), sizeof(_settings->wifiSSID) - 1);
        _settings->wifiSSID[sizeof(_settings->wifiSSID) - 1] = '\0';
    }
    if (_server.hasArg("wifiPassword")) {
        String pass = _server.arg("wifiPassword");
        strncpy(_settings->wifiPassword, pass.c_str(), sizeof(_settings->wifiPassword) - 1);
        _settings->wifiPassword[sizeof(_settings->wifiPassword) - 1] = '\0';
    }

    // PERSISTENCE FIX: Save immediately when web dashboard updates settings
    extern void saveSettings();
    saveSettings();

    _server.send(200, "text/plain", "OK");
}

void PressureWebServer::handleNotFound() {
    String host = _server.hostHeader();
    IPAddress ip = WiFi.softAPIP();
    
    // If it's not the ESP's IP, redirect to the root (Captive Portal trigger)
    if (host != ip.toString()) {
        Serial.println("Captive Portal Redirect: " + _server.uri());
        _server.sendHeader("Location", String("http://") + ip.toString(), true);
        _server.send(302, "text/plain", "");
        return;
    }

    _server.send(404, "text/plain", "Not Found");
}
