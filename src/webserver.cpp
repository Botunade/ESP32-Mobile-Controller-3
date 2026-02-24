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
        .container { max-width: 800px; margin: auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }
        .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin-top: 20px; }
        .card { padding: 15px; border: 1px solid #ddd; border-radius: 8px; }
        input { width: 80px; padding: 5px; margin: 5px; }
        button { padding: 10px 20px; background: #007bff; color: white; border: none; border-radius: 5px; cursor: pointer; }
        button:hover { background: #0056b3; }
        .warning { color: red; font-weight: bold; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Pressure Control System</h1>
        <div id="static-warning" class="warning" style="display:none;">PID output not changing – check scaling</div>
        <div class="grid">
            <div class="card">
                <h3>Live Status [<span id="status">IDLE</span>]</h3>
                <p>Pressure: <span id="pressure">0.0</span> PSI</p>
                <p>Control Voltage: <span id="voltage">0.0</span> V</p>
                <p>Setpoint: <span id="setpoint-display">0.0</span> PSI</p>
            </div>
            <div class="card">
                <canvas id="pressureChart"></canvas>
            </div>
        </div>
        <div class="card" style="margin-top:20px;">
            <h3>Settings</h3>
            <div>
                Kp: <input type="number" id="kp" step="0.1">
                Ki: <input type="number" id="ki" step="0.1">
                Kd: <input type="number" id="kd" step="0.1">
            </div>
            <div>
                Setpoint: <input type="number" id="setpoint" step="1">
                Min V: <input type="number" id="minV" step="0.1">
                Max V: <input type="number" id="maxV" step="0.1">
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
                        label: 'Pressure (PSI)',
                        data: dataHistory,
                        borderColor: '#007bff',
                        tension: 0.1
                    }]
                },
                options: { animation: false, scales: { y: { beginAtZero: true } } }
            });
        }

        async function fetchData() {
            try {
                const response = await fetch('/data');
                const data = await response.json();
                document.getElementById('pressure').innerText = data.pressure.toFixed(2);
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
                    document.getElementById('setpoint').value = data.setpoint;
                    document.getElementById('minV').value = data.minV;
                    document.getElementById('maxV').value = data.maxV;
                }
            } catch (e) { console.error(e); }
        }

        async function updateSettings() {
            const params = new URLSearchParams({
                kp: document.getElementById('kp').value,
                ki: document.getElementById('ki').value,
                kd: document.getElementById('kd').value,
                setpoint: document.getElementById('setpoint').value,
                minV: document.getElementById('minV').value,
                maxV: document.getElementById('maxV').value
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
    doc["setpoint"] = _settings->setpoint;
    doc["minV"] = _settings->minVoltage;
    doc["maxV"] = _settings->maxVoltage;

    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void PressureWebServer::handleUpdate() {
    if (_server.hasArg("kp")) _settings->kp = _server.arg("kp").toFloat();
    if (_server.hasArg("ki")) _settings->ki = _server.arg("ki").toFloat();
    if (_server.hasArg("kd")) _settings->kd = _server.arg("kd").toFloat();
    if (_server.hasArg("setpoint")) _settings->setpoint = _server.arg("setpoint").toFloat();
    if (_server.hasArg("minV")) _settings->minVoltage = _server.arg("minV").toFloat();
    if (_server.hasArg("maxV")) _settings->maxVoltage = _server.arg("maxV").toFloat();

    _server.send(200, "text/plain", "OK");
}

void PressureWebServer::handleNotFound() {
    // Redirect to root for captive portal
    _server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
    _server.send(302, "text/plain", "");
}
