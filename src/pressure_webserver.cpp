#include "pressure_webserver.h"

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

        /* Keypad Reference Card */
        .keypad-grid {
            display: grid;
            grid-template-columns: repeat(4, 1fr);
            gap: 0.5rem;
            margin-bottom: 1.25rem;
        }
        .key-cell {
            display: flex;
            flex-direction: column;
            align-items: center;
            gap: 0.3rem;
            padding: 0.75rem 0.5rem;
            border-radius: 10px;
            background: #f8fafc;
            border: 1px solid var(--border);
            transition: background 0.2s;
        }
        .key-cell.active-key {
            background: #dbeafe;
            border-color: #93c5fd;
            animation: key-flash 0.4s ease;
        }
        @keyframes key-flash {
            0% { background: #bfdbfe; transform: scale(1.06); }
            100% { background: #dbeafe; transform: scale(1); }
        }
        .key-badge {
            display: inline-flex;
            align-items: center;
            justify-content: center;
            width: 34px;
            height: 34px;
            border-radius: 8px;
            background: var(--text-main);
            color: white;
            font-weight: 700;
            font-size: 1rem;
            letter-spacing: 0;
            box-shadow: 0 2px 4px rgba(0,0,0,0.18);
        }
        .key-badge.action  { background: var(--primary); }
        .key-badge.danger  { background: var(--danger); }
        .key-badge.success { background: var(--success); }
        .key-badge.warn    { background: #f59e0b; }
        .key-badge.muted   { background: #94a3b8; }
        .key-desc {
            font-size: 0.7rem;
            color: var(--text-muted);
            font-weight: 600;
            text-align: center;
            line-height: 1.2;
        }
        .activity-log {
            list-style: none;
            display: flex;
            flex-direction: column;
            gap: 0.4rem;
            min-height: 60px;
        }
        .activity-log li {
            display: flex;
            align-items: center;
            gap: 0.6rem;
            font-size: 0.82rem;
            padding: 0.4rem 0.6rem;
            border-radius: 6px;
            background: #f8fafc;
            border-left: 3px solid var(--border);
            animation: slide-in 0.3s ease;
        }
        .activity-log li .act-key {
            font-weight: 700;
            font-size: 0.9rem;
            min-width: 22px;
            text-align: center;
        }
        .activity-log li .act-time {
            margin-left: auto;
            font-size: 0.72rem;
            color: var(--text-muted);
        }
        @keyframes slide-in {
            from { opacity: 0; transform: translateY(-6px); }
            to   { opacity: 1; transform: translateY(0); }
        }

        /* Responsive */
        @media (max-width: 768px) {
            .grid-top { grid-template-columns: 1fr; }
            .header { flex-direction: column; gap: 1rem; text-align: center; }
            .keypad-grid { grid-template-columns: repeat(2, 1fr); }
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

        <!-- Master Control -->
        <div class="card" style="margin-bottom: 1.5rem; background: #f0f9ff; border: 1px solid #bae6fd;">
            <div style="display: flex; justify-content: space-between; align-items: center;">
                <div>
                    <h3 style="margin-bottom: 0.25rem; color: #0369a1;">System Master Control</h3>
                    <p style="font-size: 0.875rem; color: #0c4a6e;">Activate or deactivate the PID pressure regulation loop.</p>
                </div>
                <div style="display: flex; gap: 0.75rem;">
                    <button id="btn-stop" class="btn-update" style="margin-top:0; background: var(--danger); width: auto; padding: 0.75rem 1.5rem;" onclick="toggleSystem(0)">STOP PID</button>
                    <button id="btn-start" class="btn-update" style="margin-top:0; background: var(--success); width: auto; padding: 0.75rem 1.5rem;" onclick="toggleSystem(1)">START PID</button>
                </div>
            </div>
        </div>

        <!-- Warnings -->
        <div id="static-warning" class="error-banner">
            ⚠️ WARNING: PID output is not changing. Please check scaling or sensor connection.
        </div>
        <div id="disconnect-warning" class="error-banner" style="display: none; background: #fff1f2; color: #e11d48; border-left: 5px solid #e11d48; margin-top: 1rem;">
            🛑 CRITICAL: PRESSURE SENSOR DISCONNECTED! Wire broken or sensor unplugged. System Stopped.
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
                        <span class="metric-label">User Pressure</span>
                        <div>
                            <span id="pressure" class="metric-value" style="color: var(--primary);">0.0</span>
                            <span class="metric-unit">BAR</span>
                        </div>
                    </div>
                    <div class="metric-item">
                        <span class="metric-label">System Pressure</span>
                        <div>
                            <span id="rawPressure" class="metric-value" style="color: var(--text-muted); font-size: 1rem;">0.00</span>
                            <span class="metric-unit">BAR</span>
                        </div>
                    </div>
                    <div class="metric-item">
                        <span class="metric-label">PV Percentage</span>
                        <div>
                            <span id="pressure-percent" class="metric-value" style="color: var(--primary);">0</span>
                            <span class="metric-unit">%</span>
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
                        <span class="metric-label">SP Percentage</span>
                        <div>
                            <span id="setpoint-percent" class="metric-value">0</span>
                            <span class="metric-unit">%</span>
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
                    <div class="metric-item">
                        <span class="metric-label">Loop Current</span>
                        <div>
                            <span id="currentMA" class="metric-value" style="color: #f59e0b;">4.00</span>
                            <span class="metric-unit">mA</span>
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
                        <span class="metric-label">Valve Duty Cycle</span>
                        <div>
                            <span id="pidOut" class="metric-value" style="color: var(--text-muted); font-size: 1rem;">10.00</span>
                            <span class="metric-unit">%</span>
                        </div>
                    </div>
                    <div class="metric-item">
                        <span class="metric-label">PID (P | I | D)</span>
                        <div style="font-size: 0.9rem; font-weight: 600;">
                            <span id="pidP" style="color: #ef4444;">0.0</span> | 
                            <span id="pidI" style="color: #f59e0b;">0.0</span> | 
                            <span id="pidD" style="color: #0ea5e9;">0.0</span>
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

        <!-- Keypad Reference Card -->
        <div class="card">
            <div class="card-header">&#9109; Keypad Reference &amp; Recent Activity</div>
            <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 1.5rem;">
                <!-- Key Map -->
                <div>
                    <p style="font-size:0.78rem; font-weight:600; color:var(--text-muted); text-transform:uppercase; letter-spacing:0.05em; margin-bottom:0.75rem;">Key Layout</p>
                    <div class="keypad-grid" id="keypad-visual">
                        <div class="key-cell" id="kc-1"><span class="key-badge action">1</span><span class="key-desc">PID<br>Tuning</span></div>
                        <div class="key-cell" id="kc-2"><span class="key-badge action">2</span><span class="key-desc">Navigate<br>UP</span></div>
                        <div class="key-cell" id="kc-3"><span class="key-badge muted">3</span><span class="key-desc">&mdash;</span></div>
                        <div class="key-cell" id="kc-A"><span class="key-badge action">A</span><span class="key-desc">Home<br>Screen</span></div>
                        <div class="key-cell" id="kc-4"><span class="key-badge warn">4</span><span class="key-desc">Decrement<br>&minus;Value</span></div>
                        <div class="key-cell" id="kc-5"><span class="key-badge warn">5</span><span class="key-desc">Manual<br>Calibrate</span></div>
                        <div class="key-cell" id="kc-6"><span class="key-badge warn">6</span><span class="key-desc">Increment<br>+Value</span></div>
                        <div class="key-cell" id="kc-B"><span class="key-badge action">B</span><span class="key-desc">Vessel<br>Size</span></div>
                        <div class="key-cell" id="kc-7"><span class="key-badge muted">7</span><span class="key-desc">&mdash;</span></div>
                        <div class="key-cell" id="kc-8"><span class="key-badge action">8</span><span class="key-desc">Navigate<br>DOWN</span></div>
                        <div class="key-cell" id="kc-9"><span class="key-badge action">9</span><span class="key-desc">Keypad<br>Test</span></div>
                        <div class="key-cell" id="kc-C"><span class="key-badge action">C</span><span class="key-desc">PID<br>Tuning</span></div>
                        <div class="key-cell" id="kc-star"><span class="key-badge success">&#42;</span><span class="key-desc">START<br>System</span></div>
                        <div class="key-cell" id="kc-0"><span class="key-badge action">0</span><span class="key-desc">Keypad<br>Test</span></div>
                        <div class="key-cell" id="kc-hash"><span class="key-badge danger">&#35;</span><span class="key-desc">STOP<br>System</span></div>
                        <div class="key-cell" id="kc-D"><span class="key-badge action">D</span><span class="key-desc">Calibration<br>Menu</span></div>
                    </div>
                </div>
                <!-- Activity Feed -->
                <div>
                    <p style="font-size:0.78rem; font-weight:600; color:var(--text-muted); text-transform:uppercase; letter-spacing:0.05em; margin-bottom:0.75rem;">Recent Activity <span id="last-key-badge" style="display:inline-block; padding:0.1rem 0.5rem; border-radius:9999px; background:#e0f2fe; color:#0369a1; font-size:0.75rem; font-weight:700; margin-left:0.5rem;">—</span></p>
                    <ul class="activity-log" id="activity-log">
                        <li style="color:var(--text-muted); font-style:italic; background:none; border:none;">No keypresses yet&hellip;</li>
                    </ul>
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
                    <label for="bandPerc">Control Band (%)</label>
                    <input type="number" id="bandPerc" step="1" value="20">
                </div>
                <div class="input-group">
                    <label for="minOn">Min Supply (ms)</label>
                    <input type="number" id="minOn" step="100" value="2000">
                </div>
                <div class="input-group">
                    <label for="minOff">Exh. Pause (ms)</label>
                    <input type="number" id="minOff" step="100" value="1000">
                </div>
                <div class="input-group">
                    <label for="exBurst">Exh. Burst (ms)</label>
                    <input type="number" id="exBurst" step="100" value="500">
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
            <div style="margin-top: 1rem; padding: 0.75rem; background: #f8fafc; border-radius: 8px; border: 1px solid var(--border); display: flex; flex-direction: column; gap: 0.75rem;">
                <div style="display: flex; justify-content: space-around; font-size: 0.9rem;">
                    <div><span style="color: var(--text-muted);">Live Raw:</span> <span id="liveSensorV" style="font-weight: bold; color: var(--primary);">0.00</span> V</div>
                    <div><span style="color: var(--text-muted);">Live Acc:</span> <span id="liveScaledV" style="font-weight: bold; color: #10b981;">0.00</span> V</div>
                </div>
                <button onclick="setZero()" style="background: #64748b; color: white; border: none; padding: 0.5rem; border-radius: 6px; cursor: pointer; font-size: 0.8rem; font-weight: 600;">CALIBRATE: Set Current as 0 BAR</button>
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

        // Keypad activity log state
        const keyLabels = {
            '1':'PID Tuning','2':'Navigate UP','3':'—','4':'Decrement','5':'Manual Calib.',
            '6':'Increment','7':'—','8':'Navigate DOWN','9':'Keypad Test','0':'Keypad Test',
            'A':'Home Screen','B':'Vessel Size','C':'PID Tuning','D':'Calibration Menu',
            '*':'START System','#':'STOP System'
        };
        const keyColors = {
            '*':'var(--success)','#':'var(--danger)','A':'var(--primary)','B':'var(--primary)',
            'C':'var(--primary)','D':'var(--primary)','2':'var(--primary)','8':'var(--primary)',
            '4':'#f59e0b','5':'#f59e0b','6':'#f59e0b','1':'var(--primary)'
        };
        const keyCellMap = {
            '1':'kc-1','2':'kc-2','3':'kc-3','4':'kc-4','5':'kc-5','6':'kc-6',
            '7':'kc-7','8':'kc-8','9':'kc-9','0':'kc-0',
            'A':'kc-A','B':'kc-B','C':'kc-C','D':'kc-D','*':'kc-star','#':'kc-hash'
        };
        let lastKeyTracked = null;
        const activityFeed = [];

        function padTime(n) { return String(n).padStart(2,'0'); }
        function nowStr() {
            const d = new Date();
            return padTime(d.getHours())+':'+padTime(d.getMinutes())+':'+padTime(d.getSeconds());
        }

        function updateKeypadActivity(key) {
            if (!key || key === lastKeyTracked) return;
            lastKeyTracked = key;

            // Flash the visual key cell
            const cellId = keyCellMap[key];
            if (cellId) {
                const cell = document.getElementById(cellId);
                if (cell) {
                    cell.classList.add('active-key');
                    setTimeout(() => cell.classList.remove('active-key'), 600);
                }
            }

            // Update last key badge
            const badge = document.getElementById('last-key-badge');
            badge.innerText = '[' + key + '] ' + (keyLabels[key] || '?');
            badge.style.background = '#e0f2fe';

            // Add to activity feed
            const label = keyLabels[key] || 'Unknown';
            const color = keyColors[key] || '#64748b';
            activityFeed.unshift({ key, label, time: nowStr(), color });
            if (activityFeed.length > 5) activityFeed.pop();

            const ul = document.getElementById('activity-log');
            ul.innerHTML = activityFeed.map(e =>
                `<li style="border-left-color:${e.color}">
                    <span class="act-key" style="color:${e.color}">${e.key}</span>
                    <span>${e.label}</span>
                    <span class="act-time">${e.time}</span>
                </li>`
            ).join('');
        }

        async function fetchData() {
            try {
                const response = await fetch('/data');
                const data = await response.json();
                
                // Update text elements — PV and SP to 1 decimal place
                document.getElementById('pressure').innerText = data.displayP.toFixed(1);
                document.getElementById('rawPressure').innerText = data.pressure.toFixed(1);
                document.getElementById('pressure-percent').innerText = (data.pressurePercent || 0).toFixed(1);
                document.getElementById('voltage').innerText = data.voltage.toFixed(2);
                document.getElementById('setpoint-display').innerText = data.setpoint.toFixed(1);
                document.getElementById('setpoint-percent').innerText = (data.setpointPercent || 0).toFixed(1);
                document.getElementById('airVolume').innerText = (data.airVolume || 0).toFixed(1);
                document.getElementById('currentMA').innerText = (data.currentMA || 4.00).toFixed(2);
                
                // Diagnostics
                document.getElementById('rawADC').innerText = Math.round(data.rawADC);
                document.getElementById('sensorV').innerText = data.sensorV.toFixed(2);
                document.getElementById('dacValue').innerText = data.dacValue;
                document.getElementById('pidOut').innerText = data.pidOut.toFixed(1);
                document.getElementById('scaled3v3') && (document.getElementById('scaled3v3').innerText = (data.scaled3v3 || 0).toFixed(2));
                if (document.getElementById('liveSensorV')) document.getElementById('liveSensorV').innerText = data.sensorV.toFixed(2);
                if (document.getElementById('liveScaledV')) document.getElementById('liveScaledV').innerText = (data.scaled3v3 || 0).toFixed(2);
                
                // PID Breakdown
                document.getElementById('pidP').innerText = data.pidP.toFixed(1);
                document.getElementById('pidI').innerText = data.pidI.toFixed(1);
                document.getElementById('pidD').innerText = data.pidD.toFixed(1);

                // Keypad activity
                if (data.lastKey) updateKeypadActivity(data.lastKey);
                
                // Update Status Badge and Master Buttons
                const statusEl = document.getElementById('status');
                const btnStart = document.getElementById('btn-start');
                const btnStop = document.getElementById('btn-stop');
                
                if(data.active) {
                    const states = ["IDLE", "SUPPLYING", "EXHAUSTING"];
                    statusEl.innerText = states[data.cState] || "RUNNING";
                    statusEl.className = 'status-badge running';
                    btnStart.style.opacity = '0.5';
                    btnStart.style.cursor = 'not-allowed';
                    btnStart.disabled = true;
                    btnStop.style.opacity = '1';
                    btnStop.style.cursor = 'pointer';
                    btnStop.disabled = false;
                } else {
                    statusEl.innerText = 'SYSTEM IDLE';
                    statusEl.className = 'status-badge';
                    btnStart.style.opacity = '1';
                    btnStart.style.cursor = 'pointer';
                    btnStart.disabled = false;
                    btnStop.style.opacity = '0.5';
                    btnStop.style.cursor = 'not-allowed';
                    btnStop.disabled = true;
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
                document.getElementById('disconnect-warning').style.display = !data.sensorConnected ? 'block' : 'none';
                if (!data.sensorConnected) {
                    document.getElementById('pressure').innerText = "DISC";
                }

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
                    document.getElementById('bandPerc').value = data.bandPerc;
                    document.getElementById('minOn').value = data.minOn;
                    document.getElementById('minOff').value = data.minOff;
                    document.getElementById('exBurst').value = data.exBurst || 500;
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

        async function toggleSystem(active) {
            try {
                await fetch('/update?active=' + active);
                fetchData(); // Immediate refresh
            } catch(e) { console.error('Failed to toggle system', e); }
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
                    bandPerc: document.getElementById('bandPerc').value,
                    minOn: document.getElementById('minOn').value,
                    minOff: document.getElementById('minOff').value,
                    exBurst: document.getElementById('exBurst').value,
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

        async function setZero() {
            const currentV = document.getElementById('liveSensorV').innerText;
            if(confirm("Are you sure you want to set " + currentV + "V as the new 0 BAR baseline? \n\nEnsure the vessel is vented to atmosphere first.")) {
                try {
                    await fetch('/update?setZero=1');
                    alert("Calibration updated. Device is rebooting.");
                    location.reload();
                } catch(e) { alert("Failed to calibrate"); }
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
    doc["sensorConnected"] = _state->sensorConnected;
    doc["active"] = _state->systemActive;
    doc["solenoid"] = _state->solenoidState;
    doc["airVolume"] = _state->airVolume;
    doc["scaled3v3"] = _state->scaledTo3v3;
    doc["currentMA"] = _state->controlCurrent;
    doc["displayP"] = _state->displayPressure;
    doc["cState"] = _state->controlState;
    
    // PID Components Synchronization
    doc["pidP"] = _state->pTerm;
    doc["pidI"] = _state->iTerm;
    doc["pidD"] = _state->dTerm;

    doc["tankVol"] = _settings->tankVolume;
    doc["bandPerc"] = _settings->controlBandPercent;
    doc["minOn"] = _settings->minOnTimeMS;
    doc["minOff"] = _settings->minOffTimeMS;
    doc["exBurst"] = _settings->exhaustBurstMS;
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

    doc["setpointPercent"] = _state->setpointPercent;
    doc["pressurePercent"] = _state->pressurePercent;
    
    // Send lastKey as a single-char string for the dashboard keypad activity panel
    char keyStr[2] = { _state->lastKeyPressed, '\0' };
    if (_state->lastKeyPressed == '\0' || _state->lastKeyPressed == ' ') {
        doc["lastKey"] = (char*)nullptr; // No key pressed yet
    } else {
        doc["lastKey"] = keyStr;
    }

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
    if (_server.hasArg("active")) {
        _state->systemActive = (_server.arg("active") == "1");
        Serial.print("Local System Command: ");
        Serial.println(_state->systemActive ? "START" : "STOP");
    }
    if (_server.hasArg("bandPerc")) _settings->controlBandPercent = _server.arg("bandPerc").toFloat();
    if (_server.hasArg("minOn")) _settings->minOnTimeMS = _server.arg("minOn").toInt();
    if (_server.hasArg("minOff")) _settings->minOffTimeMS = _server.arg("minOff").toInt();
    if (_server.hasArg("exBurst")) _settings->exhaustBurstMS = _server.arg("exBurst").toInt();
    if (_server.hasArg("setpoint")) {
        _settings->setpoint = _server.arg("setpoint").toFloat();
        Serial.printf("[WEB] Setpoint updated: %.2f BAR\n", _settings->setpoint);
    }
    if (_server.hasArg("safeAllow")) _settings->safetyAllowance = _server.arg("safeAllow").toFloat();
    if (_server.hasArg("minV")) _settings->minVoltage = _server.arg("minV").toFloat();
    if (_server.hasArg("maxV")) _settings->maxVoltage = _server.arg("maxV").toFloat();
    
    if (_server.hasArg("setZero")) {
        _settings->sensorMinV = _state->sensorVoltage;
        Serial.printf("[CALIB] Software Zero Set: %.3fV\n", _settings->sensorMinV);
    }

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
