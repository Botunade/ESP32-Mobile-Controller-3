#include "webserver.h"

const char* DASHBOARD_HTML = R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>Pressure Control Dashboard</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        body { font-family: Arial; text-align: center; margin: 0; padding: 20px; background: #f4f4f4; }
        .container { max-width: 900px; margin: auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }
        .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin-top: 20px; }
        .card { padding: 15px; border: 1px solid #ddd; border-radius: 8px; }
        .settings-grid { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 10px; text-align: left; }
        .settings-section { margin-bottom: 15px; border-bottom: 1px solid #eee; padding-bottom: 10px; }
        input { width: 100%; box-sizing: border-box; padding: 5px; margin-top: 3px; }
        button { padding: 10px 20px; background: #007bff; color: white; border: none; border-radius: 5px; cursor: pointer; margin-top: 10px; }
        button:hover { background: #0056b3; }
        .warning { color: red; font-weight: bold; }
        label { font-size: 0.9em; color: #555; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Pressure Control System</h1>
        <div id="static-warning" class="warning" style="display:none;">PID output not changing – check scaling</div>
        <div class="grid">
            <div class="card">
                <h3>Live Status [<span id="status">IDLE</span>]</h3>
                <p>Pressure: <span id="pressure">0.0</span> <span id="unit-label">%</span></p>
                <p>Control Voltage: <span id="voltage">0.0</span> V</p>
                <p>Setpoint: <span id="setpoint-display">0.0</span> <span id="unit-label-sp">%</span></p>
            </div>
            <div class="card">
                <canvas id="pressureChart"></canvas>
            </div>
        </div>
        <div class="card" style="margin-top:20px;">
            <h3>Settings</h3>
            <div class="settings-section">
                <h4>PID & Timing</h4>
                <div class="settings-grid">
                    <div><label>Kp</label><input type="number" id="kp" step="0.1"></div>
                    <div><label>Ki</label><input type="number" id="ki" step="0.1"></div>
                    <div><label>Kd</label><input type="number" id="kd" step="0.1"></div>
                    <div><label>Sample Time (ms)</label><input type="number" id="sampleTime"></div>
                </div>
            </div>
            <div class="settings-section">
                <h4>Pressure Control</h4>
                <div class="settings-grid">
                    <div><label>Setpoint</label><input type="number" id="setpoint" step="0.1"></div>
                    <div><label>Max Pressure</label><input type="number" id="maxPressure" step="1"></div>
                    <div><label>Units (0:%, 1:PSI, 2:BAR)</label><input type="number" id="units" min="0" max="2"></div>
                </div>
            </div>
            <div class="settings-section">
                <h4>Output & Tank</h4>
                <div class="settings-grid">
                    <div><label>Min Voltage</label><input type="number" id="minV" step="0.1"></div>
                    <div><label>Max Voltage</label><input type="number" id="maxV" step="0.1"></div>
                    <div><label>Ramp Rate</label><input type="number" id="rampRate" step="1"></div>
                    <div><label>Tank Vol (L)</label><input type="number" id="tankVol"></div>
                </div>
            </div>
            <div class="settings-section">
                <h4>Sensor Calibration</h4>
                <div class="settings-grid">
                    <div><label>Low V (0%)</label><input type="number" id="lowV" step="0.001"></div>
                    <div><label>High V (100%)</label><input type="number" id="highV" step="0.001"></div>
                    <div><label>Cal Factor</label><input type="number" id="calFactor" step="0.01"></div>
                </div>
            </div>
            <button onclick="updateSettings()">Apply Settings</button>
        </div>
    </div>

    <script>
        let chart;
        const dataHistory = [];
        const labels = [];

        function initChart() {
            const ctx = document.getElementById('pressureChart').getContext('2d');
            chart = new Chart(ctx, {
                type: 'line',
                data: {
                    labels: labels,
                    datasets: [{
                        label: 'Pressure',
                        data: dataHistory,
                        borderColor: '#007bff',
                        tension: 0.1
                    }]
                },
                options: { animation: false, scales: { y: { beginAtZero: true, max: 100 } } }
            });
        }

        async function fetchData() {
            try {
                const response = await fetch('/data');
                const data = await response.json();

                const unitStr = data.units == 1 ? 'PSI' : (data.units == 2 ? 'BAR' : '%');
                document.getElementById('pressure').innerText = data.pressure.toFixed(2);
                document.getElementById('unit-label').innerText = unitStr;
                document.getElementById('unit-label-sp').innerText = unitStr;

                document.getElementById('voltage').innerText = data.voltage.toFixed(2);
                document.getElementById('setpoint-display').innerText = data.setpoint.toFixed(2);
                document.getElementById('status').innerText = data.active ? 'RUNNING' : 'IDLE';
                document.getElementById('static-warning').style.display = data.isStatic ? 'block' : 'none';

                dataHistory.push(data.pressure);
                labels.push('');
                if (dataHistory.length > 50) {
                    dataHistory.shift();
                    labels.shift();
                }
                chart.update();

                // Fill inputs if first time
                if (!document.getElementById('kp').value) {
                    document.getElementById('kp').value = data.kp;
                    document.getElementById('ki').value = data.ki;
                    document.getElementById('kd').value = data.kd;
                    document.getElementById('sampleTime').value = data.sampleTime;
                    document.getElementById('setpoint').value = data.setpoint;
                    document.getElementById('maxPressure').value = data.maxPressure;
                    document.getElementById('units').value = data.units;
                    document.getElementById('minV').value = data.minV;
                    document.getElementById('maxV').value = data.maxV;
                    document.getElementById('rampRate').value = data.rampRate;
                    document.getElementById('tankVol').value = data.tankVol;
                    document.getElementById('lowV').value = data.lowV;
                    document.getElementById('highV').value = data.highV;
                    document.getElementById('calFactor').value = data.calFactor;
                }
            } catch (e) { console.error(e); }
        }

        async function updateSettings() {
            const params = new URLSearchParams({
                kp: document.getElementById('kp').value,
                ki: document.getElementById('ki').value,
                kd: document.getElementById('kd').value,
                sampleTime: document.getElementById('sampleTime').value,
                setpoint: document.getElementById('setpoint').value,
                maxPressure: document.getElementById('maxPressure').value,
                units: document.getElementById('units').value,
                minV: document.getElementById('minV').value,
                maxV: document.getElementById('maxV').value,
                rampRate: document.getElementById('rampRate').value,
                tankVol: document.getElementById('tankVol').value,
                lowV: document.getElementById('lowV').value,
                highV: document.getElementById('highV').value,
                calFactor: document.getElementById('calFactor').value
            });
            await fetch('/update?' + params.toString());
            alert('Settings updated');
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
    _server.onNotFound(std::bind(&PressureWebServer::handleNotFound, this));
}

void PressureWebServer::handleRoot() {
    _server.send(200, "text/html", DASHBOARD_HTML);
}

void PressureWebServer::handleData() {
    JsonDocument doc;
    doc["pressure"] = _state->pressure;
    doc["voltage"] = _state->controlVoltage;
    doc["isStatic"] = _state->isStatic;
    doc["active"] = _state->systemActive;

    doc["kp"] = _settings->kp;
    doc["ki"] = _settings->ki;
    doc["kd"] = _settings->kd;
    doc["sampleTime"] = _settings->sampleTime;

    doc["setpoint"] = _settings->setpoint;
    doc["maxPressure"] = _settings->maxPressure;
    doc["units"] = _settings->units;

    doc["minV"] = _settings->minVoltage;
    doc["maxV"] = _settings->maxVoltage;
    doc["rampRate"] = _settings->rampRate;
    doc["tankVol"] = _settings->tankVolume;

    doc["lowV"] = _settings->lowVoltage;
    doc["highV"] = _settings->highVoltage;
    doc["calFactor"] = _settings->calibrationFactor;

    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void PressureWebServer::handleUpdate() {
    if (_server.hasArg("kp")) _settings->kp = _server.arg("kp").toFloat();
    if (_server.hasArg("ki")) _settings->ki = _server.arg("ki").toFloat();
    if (_server.hasArg("kd")) _settings->kd = _server.arg("kd").toFloat();
    if (_server.hasArg("sampleTime")) _settings->sampleTime = _server.arg("sampleTime").toInt();

    if (_server.hasArg("setpoint")) _settings->setpoint = _server.arg("setpoint").toFloat();
    if (_server.hasArg("maxPressure")) _settings->maxPressure = _server.arg("maxPressure").toFloat();
    if (_server.hasArg("units")) _settings->units = _server.arg("units").toInt();

    if (_server.hasArg("minV")) _settings->minVoltage = _server.arg("minV").toFloat();
    if (_server.hasArg("maxV")) _settings->maxVoltage = _server.arg("maxV").toFloat();
    if (_server.hasArg("rampRate")) _settings->rampRate = _server.arg("rampRate").toFloat();
    if (_server.hasArg("tankVol")) _settings->tankVolume = _server.arg("tankVol").toInt();

    if (_server.hasArg("lowV")) _settings->lowVoltage = _server.arg("lowV").toFloat();
    if (_server.hasArg("highV")) _settings->highVoltage = _server.arg("highV").toFloat();
    if (_server.hasArg("calFactor")) _settings->calibrationFactor = _server.arg("calFactor").toFloat();

    _server.send(200, "text/plain", "OK");
}

void PressureWebServer::handleNotFound() {
    _server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
    _server.send(302, "text/plain", "");
}
