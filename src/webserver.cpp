#include "webserver.h"

const char* DASHBOARD_HTML = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Sanni | Dual-Track Controller</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <link href="https://fonts.googleapis.com/css2?family=Plus+Jakarta+Sans:wght@400;500;600;700;800&display=swap" rel="stylesheet">
    <style>
        :root {
            --accent: #06b6d4;
            --accent-glow: rgba(6, 182, 212, 0.4);
            --bg: #020617;
            --card-bg: rgba(30, 41, 59, 0.4);
            --glass-border: rgba(255, 255, 255, 0.08);
            --text: #f8fafc;
            --text-muted: #94a3b8;
            --success: #10b981;
            --danger: #ef4444;
            --radius: 20px;
        }

        * { margin: 0; padding: 0; box-sizing: border-box; font-family: 'Plus Jakarta Sans', sans-serif; }
        
        body {
            background: var(--bg);
            background-image: 
                radial-gradient(at 0% 0%, rgba(6, 182, 212, 0.1) 0px, transparent 50%),
                radial-gradient(at 100% 0%, rgba(99, 102, 241, 0.1) 0px, transparent 50%);
            color: var(--text);
            min-height: 100vh;
            padding: 2rem 1rem;
        }

        .container {
            max-width: 1100px;
            margin: 0 auto;
            display: flex;
            flex-direction: column;
            gap: 2rem;
        }

        /* Glassmorphism Classes */
        .glass {
            background: var(--card-bg);
            backdrop-filter: blur(12px);
            -webkit-backdrop-filter: blur(12px);
            border: 1px solid var(--glass-border);
            border-radius: var(--radius);
            box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.37);
        }

        /* Header */
        header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 1.5rem 2.5rem;
        }

        h1 { font-size: 1.25rem; font-weight: 800; letter-spacing: -0.025em; color: var(--accent); }

        .status-badge {
            display: flex;
            align-items: center;
            gap: 0.75rem;
            padding: 0.5rem 1.25rem;
            border-radius: 999px;
            font-size: 0.75rem;
            font-weight: 700;
            text-transform: uppercase;
            letter-spacing: 0.05em;
            background: rgba(0,0,0,0.2);
        }

        .status-dot { width: 8px; height: 8px; border-radius: 50%; background: #475569; }
        .running .status-dot { background: var(--success); box-shadow: 0 0 12px var(--success); animation: pulse 2s infinite; }

        @keyframes pulse {
            0% { opacity: 1; } 50% { opacity: 0.4; } 100% { opacity: 1; }
        }

        /* Main Dashboard Grid */
        .dashboard-grid {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 1.5rem;
        }

        .metric-card {
            padding: 2rem;
            text-align: center;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
        }

        .metric-label { font-size: 0.875rem; font-weight: 600; color: var(--text-muted); margin-bottom: 0.5rem; text-transform: uppercase; }
        .metric-value-large { font-size: 3.5rem; font-weight: 800; line-height: 1; letter-spacing: -0.05em; margin-bottom: 0.25rem; }
        .metric-unit-large { font-size: 1rem; font-weight: 600; color: var(--accent); }

        /* Sliders & Controls */
        .controls-card {
            grid-column: span 3;
            padding: 2.5rem;
            display: grid;
            grid-template-columns: 2fr 1fr;
            gap: 3rem;
        }

        .sp-slider-group { display: flex; flex-direction: column; gap: 1.5rem; }
        .sp-header { display: flex; justify-content: space-between; align-items: flex-end; }
        .sp-title { font-size: 1.1rem; font-weight: 700; }
        .sp-val { font-size: 2.5rem; font-weight: 800; color: var(--accent); }

        input[type="range"] {
            -webkit-appearance: none;
            width: 100%;
            height: 8px;
            background: rgba(255,255,255,0.1);
            border-radius: 4px;
            outline: none;
        }

        input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 24px;
            height: 24px;
            background: var(--accent);
            border-radius: 50%;
            cursor: pointer;
            box-shadow: 0 0 15px var(--accent-glow);
            border: 4px solid var(--bg);
        }

        .btn-toggle {
            width: 100%;
            height: 100%;
            border: none;
            border-radius: var(--radius);
            font-size: 1.25rem;
            font-weight: 800;
            cursor: pointer;
            transition: all 0.3s ease;
            text-transform: uppercase;
        }

        .btn-toggle.off { background: rgba(0,0,0,0.3); color: var(--text-muted); border: 1px solid var(--glass-border); }
        .btn-toggle.on { background: var(--accent); color: var(--bg); box-shadow: 0 0 30px var(--accent-glow); }

        /* Chart */
        .chart-card { grid-column: span 3; padding: 2rem; min-height: 350px; }

        /* Detailed Config */
        .config-section { grid-column: span 3; }
        .config-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 1.5rem;
            padding: 2rem;
        }

        .input-group { display: flex; flex-direction: column; gap: 0.5rem; }
        .input-group label { font-size: 0.8rem; font-weight: 600; color: var(--text-muted); }
        
        input[type="number"], input[type="text"], input[type="password"] {
            background: rgba(0,0,0,0.2);
            border: 1px solid var(--glass-border);
            border-radius: 10px;
            padding: 0.75rem 1rem;
            color: white;
            font-size: 0.9rem;
            outline: none;
            transition: border-color 0.2s;
        }
        input:focus { border-color: var(--accent); }

        .btn-save {
            grid-column: span 3;
            padding: 1rem;
            background: var(--accent);
            color: var(--bg);
            border: none;
            border-radius: 12px;
            font-weight: 700;
            cursor: pointer;
            margin-top: 1rem;
        }

        /* Responsive */
        @media (max-width: 900px) {
            .dashboard-grid, .controls-card { grid-template-columns: 1fr; }
            .metric-card, .controls-card, .chart-card { grid-column: span 1; }
        }

        /* Overlay */
        .overlay {
            display: none;
            position: fixed;
            inset: 0;
            background: rgba(0,0,0,0.8);
            backdrop-filter: blur(8px);
            z-index: 1000;
            align-items: center;
            justify-content: center;
            text-align: center;
        }
    </style>
</head>
<body>
    <div id="reboot-overlay" class="overlay">
        <div class="glass" style="padding: 3rem;">
            <h2 style="margin-bottom: 1rem;">Saving System State</h2>
            <p style="color: var(--text-muted);">Rebooting in <span id="timer">5</span>...</p>
        </div>
    </div>

    <div class="container">
        <header class="glass">
            <h1>SANNI | DUAL-TRACK</h1>
            <div id="status-badge" class="status-badge">
                <div class="status-dot"></div>
                <span id="status-text">OFFLINE</span>
            </div>
        </header>

        <main class="dashboard-grid">
            <!-- Telemetry -->
            <div class="metric-card glass">
                <span class="metric-label">Actual (PV)</span>
                <span id="pv-perc" class="metric-value-large">0</span>
                <span class="metric-unit-large">% TOTAL SCALE</span>
            </div>

            <div class="metric-card glass">
                <span class="metric-label">Simulated (OUT)</span>
                <span id="out-perc" class="metric-value-large">0</span>
                <span class="metric-unit-large">% OUTPUT POWER</span>
            </div>

            <div class="metric-card glass">
                <span class="metric-label">Pneumatic Stat</span>
                <span id="pv-bar" class="metric-value-large" style="font-size: 2.5rem;">0.0</span>
                <span id="pv-liters" class="metric-unit-large">0.0 L AIR</span>
            </div>

            <!-- Main Control -->
            <div class="controls-card glass">
                <div class="sp-slider-group">
                    <div class="sp-header">
                        <span class="sp-title">SETPOINT TARGET (%)</span>
                        <span id="sp-val-display" class="sp-val">0%</span>
                    </div>
                    <input type="range" id="sp-slider" min="0" max="100" step="1" oninput="updateSPSlider(this.value)" onchange="sendSetpoint(this.value)">
                    <div style="display: flex; justify-content: space-between; color: var(--text-muted); font-size: 0.75rem;">
                        <span>0% (0 Bar)</span>
                        <span>50% (3 Bar)</span>
                        <span>100% (6 Bar)</span>
                    </div>
                </div>
                <div>
                    <button id="system-toggle" class="btn-toggle off" onclick="toggleSystem()">SYSTEM OFF</button>
                    <button id="solenoid-toggle" class="glass" onclick="toggleSolenoid()" style="width:100%; border:none; padding:10px; margin-top:10px; cursor:pointer; font-size:0.7rem; color:var(--text-muted); font-weight:bold;">SOLENOID OVERRIDE</button>
                </div>
            </div>

            <!-- Visualization -->
            <div class="chart-card glass">
                <canvas id="mainChart"></canvas>
            </div>

            <!-- Advanced Configuration -->
            <div class="config-section glass">
                <div style="padding: 1.5rem 2rem; border-bottom: 1px solid var(--glass-border); font-weight: 800; font-size: 0.8rem; letter-spacing: 0.1em;">
                    HARDWARE CALIBRATION & NETWORK
                </div>
                <div class="config-grid">
                    <div class="input-group">
                        <label>Target Vessel (L)</label>
                        <input type="number" id="f-tankVol">
                    </div>
                    <div class="input-group">
                        <label>Track 2 Kp</label>
                        <input type="number" id="f-kp" step="0.1">
                    </div>
                    <div class="input-group">
                        <label>Track 2 Ki</label>
                        <input type="number" id="f-ki" step="0.1">
                    </div>
                    <div class="input-group">
                        <label>Track 1 Deadband (Bar)</label>
                        <input type="number" id="f-dband" step="0.1">
                    </div>
                    <div class="input-group">
                        <label>Safety Limit (Bar)</label>
                        <input type="number" id="f-safeAllow" step="0.1">
                    </div>
                    <div class="input-group">
                        <label>Working Max (Bar)</label>
                        <input type="number" id="f-wMaxP" step="0.1">
                    </div>
                    <div class="input-group">
                        <label>WiFi SSID</label>
                        <input type="text" id="f-wifiSSID">
                    </div>
                    <div class="input-group">
                        <label>WiFi Pass</label>
                        <input type="password" id="f-wifiPassword">
                    </div>
                    <button class="btn-save" onclick="saveSettings()">COMMIT TO NVS</button>
                </div>
            </div>
        </main>
    </div>

    <script>
        let chart;
        const historySize = 50;
        let pvData = Array(historySize).fill(0);
        let spData = Array(historySize).fill(0);

        function initChart() {
            const ctx = document.getElementById('mainChart').getContext('2d');
            chart = new Chart(ctx, {
                type: 'line',
                data: {
                    labels: Array(historySize).fill(''),
                    datasets: [
                        { label: 'PV', data: pvData, borderColor: '#06b6d4', borderWidth: 3, tension: 0.4, pointRadius: 0, fill: true, backgroundColor: 'rgba(6, 182, 212, 0.1)' },
                        { label: 'SP', data: spData, borderColor: 'rgba(255,255,255,0.2)', borderWidth: 1, borderDash: [5,5], pointRadius: 0 }
                    ]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    animation: false,
                    scales: {
                        y: { min: 0, max: 100, border: {display: false}, grid: { color: 'rgba(255,255,255,0.05)' } },
                        x: { display: false }
                    },
                    plugins: { legend: { display: false } }
                }
            });
        }

        async function sync() {
            try {
                const res = await fetch('/data');
                const d = await res.json();

                // Telemetry
                document.getElementById('pv-perc').innerText = Math.round(d.pvPerc);
                document.getElementById('out-perc').innerText = Math.round(d.outPerc);
                document.getElementById('pv-bar').innerText = d.pressure.toFixed(1);
                document.getElementById('pv-liters').innerText = d.airVolume.toFixed(1) + ' L AIR';
                
                // Status
                const badge = document.getElementById('status-badge');
                if (d.active) {
                    badge.classList.add('running');
                    document.getElementById('status-text').innerText = 'System Active';
                    document.getElementById('system-toggle').innerText = 'STOP SYSTEM';
                    document.getElementById('system-toggle').className = 'btn-toggle on';
                } else {
                    badge.classList.remove('running');
                    document.getElementById('status-text').innerText = 'System Idle';
                    document.getElementById('system-toggle').innerText = 'START SYSTEM';
                    document.getElementById('system-toggle').className = 'btn-toggle off';
                }

                const solBtn = document.getElementById('solenoid-toggle');
                solBtn.style.color = d.solenoid ? '#ef4444' : '#94a3b8';
                solBtn.innerText = d.solenoid ? 'SAFETY BYPASS OPEN' : 'SOLENOID READY';

                // Chart
                pvData.push(d.pvPerc);
                spData.push(d.spPerc);
                if (pvData.length > historySize) { pvData.shift(); spData.shift(); }
                chart.update('none');

                // Initial Form Load
                if (!document.getElementById('f-tankVol').value) {
                    document.getElementById('f-tankVol').value = d.tankVol;
                    document.getElementById('f-kp').value = d.kp;
                    document.getElementById('f-ki').value = d.ki;
                    document.getElementById('f-dband').value = d.dband;
                    document.getElementById('f-safeAllow').value = d.safeAllow;
                    document.getElementById('f-wMaxP').value = d.wMaxP;
                    document.getElementById('sp-slider').value = d.spPerc;
                    document.getElementById('sp-val-display').innerText = Math.round(d.spPerc) + '%';
                }

            } catch(e) {}
        }

        function updateSPSlider(v) {
            document.getElementById('sp-val-display').innerText = v + '%';
        }

        async function sendSetpoint(v) {
            await fetch(`/update?spPerc=${v}`);
        }

        async function toggleSystem() {
            const btn = document.getElementById('system-toggle');
            const turnOn = btn.classList.contains('off');
            await fetch(`/update?active=${turnOn ? '1' : '0'}`);
        }

        async function toggleSolenoid() {
            const res = await fetch('/data');
            const d = await res.json();
            await fetch(`/update?solenoid=${d.solenoid ? '0' : '1'}`);
        }

        async function saveSettings() {
            const params = new URLSearchParams({
                tankVol: document.getElementById('f-tankVol').value,
                kp: document.getElementById('f-kp').value,
                ki: document.getElementById('f-ki').value,
                dband: document.getElementById('f-dband').value,
                safeAllow: document.getElementById('f-safeAllow').value,
                wMaxP: document.getElementById('f-wMaxP').value,
                wifiSSID: document.getElementById('f-wifiSSID').value,
                wifiPassword: document.getElementById('f-wifiPassword').value
            });
            await fetch('/update?' + params.toString());
            
            document.getElementById('reboot-overlay').style.display = 'flex';
            let c = 5;
            const t = setInterval(async () => {
                c--;
                document.getElementById('timer').innerText = c;
                if (c <= 0) {
                    clearInterval(t);
                    await fetch('/restart');
                }
            }, 1000);
        }

        initChart();
        setInterval(sync, 500);
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
    doc["active"] = _state->systemActive;
    doc["solenoid"] = _state->solenoidState;
    doc["airVolume"] = _state->airVolume;
    doc["scaled3v3"] = _state->scaledTo3v3;
    doc["spPerc"] = _settings->spPercent;
    doc["pvPerc"] = _state->displayPV;
    doc["outPerc"] = _state->displayOUT;
    doc["dband"] = _settings->deadband;
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
    if (_server.hasArg("spPerc")) _settings->spPercent = _server.arg("spPerc").toFloat();
    if (_server.hasArg("dband")) _settings->deadband = _server.arg("dband").toFloat();
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
