import { initializeApp } from "https://www.gstatic.com/firebasejs/10.8.1/firebase-app.js";
import { getDatabase, ref, onValue, update, set } from "https://www.gstatic.com/firebasejs/10.8.1/firebase-database.js";

// Firebase configuration from earlier
const firebaseConfig = {
    projectId: "pressure-control-17b6e",
    appId: "1:21523282778:web:cbc87f1ef72460c42a5163",
    databaseURL: "https://pressure-control-17b6e-default-rtdb.firebaseio.com",
    storageBucket: "pressure-control-17b6e.firebasestorage.app",
    apiKey: "AIzaSyDPPqJp0Gk3BGlV6-WkATQ6nrQfswRozVQ",
    authDomain: "pressure-control-17b6e.firebaseapp.com",
    messagingSenderId: "21523282778",
    measurementId: "G-T7Q1Y1WFMM"
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);
const db = getDatabase(app);

// Database References
const stateRef = ref(db, 'devices/esp32_controller_1/state');
const settingsRef = ref(db, 'devices/esp32_controller_1/settings');
const commandsRef = ref(db, 'devices/esp32_controller_1/commands');
const connectionRef = ref(db, '.info/connected');

// DOM Elements - Status
const elConnectionStatus = document.getElementById('connectionStatus');
const elPingDot = document.getElementById('pingDot');
const elPingPing = document.getElementById('pingPing');
const elSystemActiveBadge = document.getElementById('systemActiveBadge');
const elStaticWarning = document.getElementById('staticWarning');
const elLastUpdated = document.getElementById('lastUpdated');
let lastSeenTimestamp = 0; // Track device heartbeat

// DOM Elements - Telemetry Values
const valPressure = document.getElementById('valPressure');
const valSetpoint = document.getElementById('valSetpoint');
const valVoltage = document.getElementById('valVoltage');
const valPid = document.getElementById('valPid');
const barPid = document.getElementById('barPid');
const barPressure = document.getElementById('barPressure');
const barVoltage = document.getElementById('barVoltage');
const valAirVolume = document.getElementById('valAirVolume');
const valCurrent = document.getElementById('valCurrent');
const barCurrent = document.getElementById('barCurrent');

// Chart Setup
const ctx = document.getElementById('pressureChart').getContext('2d');
const pressureChart = new Chart(ctx, {
    type: 'line',
    data: {
        labels: [],
        datasets: [{
            label: 'Pressure (BAR)',
            data: [],
            borderColor: '#0ea5e9',
            backgroundColor: 'rgba(14, 165, 233, 0.1)',
            borderWidth: 3,
            fill: true,
            tension: 0.4,
            pointRadius: 0
        }, {
            label: 'Working Max',
            data: [],
            borderColor: '#ef4444',
            borderDash: [5, 5],
            borderWidth: 1.5,
            pointRadius: 0,
            fill: false
        }]
    },
    options: {
        responsive: true,
        plugins: { legend: { display: false } },
        scales: {
            x: { display: false },
            y: {
                beginAtZero: true,
                max: 12,
                grid: { color: '#f1f5f9' },
                ticks: { color: '#64748b' }
            }
        },
        animation: false
    }
});

// DOM Elements - Settings Form
const inputKp = document.getElementById('kp');
const inputKi = document.getElementById('ki');
const inputKd = document.getElementById('kd');
const inputSetpoint = document.getElementById('setpoint');
const inputSafetyAllowance = document.getElementById('safetyAllowance');
const inputMinVoltage = document.getElementById('minVoltage');
const inputMaxVoltage = document.getElementById('maxVoltage');
const inputSensorMinV = document.getElementById('sensorMinV');
const inputSensorMaxV = document.getElementById('sensorMaxV');
const inputSensorMaxBar = document.getElementById('sensorMaxBar');
const inputWorkingMaxBar = document.getElementById('workingMaxBar');
const inputAccuracyMinV = document.getElementById('accuracyMinV');
const inputAccuracyMaxV = document.getElementById('accuracyMaxV');
const syncConfigBadge = document.getElementById('syncConfigBadge');
const btnSaveConfig = document.getElementById('btnSaveConfig');
const savingOverlay = document.getElementById('savingOverlay');

// DOM Elements - Controls
const btnStart = document.getElementById('btnStart');
const btnStop = document.getElementById('btnStop');
const btnToggleSolenoid = document.getElementById('btnToggleSolenoid');
const solenoidIndicator = document.getElementById('solenoidIndicator');
const btnRefreshSettings = document.getElementById('btnRefreshSettings');

const inputTankVolume = document.getElementById('tankVolume');

// Format helpers
const _f = (num, decimals = 2) => Number(num).toFixed(decimals);

// Connection Status Monitor
onValue(connectionRef, (snap) => {
    if (snap.val() === true) {
        elConnectionStatus.innerHTML = `
            <span class="relative flex h-3 w-3">
                <span class="animate-ping absolute inline-flex h-full w-full rounded-full bg-emerald-400 opacity-75"></span>
                <span class="relative inline-flex rounded-full h-3 w-3 bg-emerald-500"></span>
            </span>
            <span class="text-emerald-700">Connected to Cloud</span>
        `;
        elConnectionStatus.className = 'flex items-center gap-2 px-3 py-1.5 rounded-full bg-emerald-50 text-sm font-semibold transition-colors duration-300 border border-emerald-100 shadow-sm';
    } else {
        elConnectionStatus.innerHTML = `
            <span class="relative flex h-3 w-3">
                <span class="relative inline-flex rounded-full h-3 w-3 bg-rose-500"></span>
            </span>
            <span class="text-rose-700">Disconnected</span>
        `;
        elConnectionStatus.className = 'flex items-center gap-2 px-3 py-1.5 rounded-full bg-rose-50 text-sm font-semibold transition-colors duration-300 border border-rose-100 shadow-sm';
    }
});

// Telemetry Monitor
onValue(stateRef, (snapshot) => {
    const data = snapshot.val();
    if (!data) return;

    // Time stamp
    const now = new Date();
    elLastUpdated.innerText = now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });

    // --- CRITICAL STATUS UPDATES (Moved up for reliability) ---
    if (data.systemActive !== undefined) {
        if (data.systemActive) {
            elSystemActiveBadge.innerText = 'SYSTEM RUNNING';
            elSystemActiveBadge.className = 'px-4 py-2 rounded-lg text-sm font-bold tracking-wide bg-emerald-100 text-emerald-700 border border-emerald-200 shadow-inner transition-colors duration-300';
        } else {
            elSystemActiveBadge.innerText = 'SYSTEM IDLE';
            elSystemActiveBadge.className = 'px-4 py-2 rounded-lg text-sm font-bold tracking-wide bg-slate-100 text-slate-500 border border-slate-200 shadow-inner transition-colors duration-300';
        }
    }

    if (data.solenoidState !== undefined) {
        if (data.solenoidState) {
            btnToggleSolenoid.innerHTML = `<div id="solenoidIndicator" class="w-3 h-3 rounded-full bg-emerald-500 shadow-[0_0_8px_rgba(16,185,129,0.5)]"></div> SOLENOID: ON`;
            btnToggleSolenoid.className = 'px-6 py-2 bg-emerald-50 hover:bg-emerald-100 text-emerald-700 font-bold rounded-lg shadow-sm border border-emerald-200 transition-all flex items-center gap-2';
            btnToggleSolenoid.setAttribute('data-state', 'on');
        } else {
            btnToggleSolenoid.innerHTML = `<div id="solenoidIndicator" class="w-3 h-3 rounded-full bg-slate-400"></div> SOLENOID: OFF`;
            btnToggleSolenoid.className = 'px-6 py-2 bg-slate-100 hover:bg-slate-200 text-slate-600 font-bold rounded-lg shadow-sm border border-slate-200 transition-all flex items-center gap-2';
            btnToggleSolenoid.setAttribute('data-state', 'off');
        }
    }
    // --------------------------------------------------------

    if (data.pressure !== undefined) {
        if (valPressure) valPressure.innerText = _f(data.pressure, 2);
        if (document.getElementById('valPressurePercent')) {
            document.getElementById('valPressurePercent').innerText = _f(data.pressurePercent || 0, 1);
        }
        if (barPressure) {
            barPressure.style.width = `${Math.min(100, Math.max(0, data.pressurePercent || 0))}%`;
        }

        // Update Chart
        const nowStr = new Date().toLocaleTimeString();
        pressureChart.data.labels.push(nowStr);
        pressureChart.data.datasets[0].data.push(data.pressure);
        pressureChart.data.datasets[1].data.push(data.workingMaxBar || 0);
        if (pressureChart.data.labels.length > 50) {
            pressureChart.data.labels.shift();
            pressureChart.data.datasets[0].data.shift();
            pressureChart.data.datasets[1].data.shift();
        }
        pressureChart.update();
    }

    if (data.airVolume !== undefined) {
        valAirVolume.innerText = _f(data.airVolume, 2);
    }

    if (data.sensorVoltage !== undefined) {
        if (document.getElementById('liveSensorV')) document.getElementById('liveSensorV').innerText = _f(data.sensorVoltage, 2);
    }
    if (data.scaledTo3v3 !== undefined) {
        if (document.getElementById('liveScaledV')) document.getElementById('liveScaledV').innerText = _f(data.scaledTo3v3, 2);
    }

    if (data.controlVoltage !== undefined) {
        valVoltage.innerText = _f(data.controlVoltage, 2);
        // Voltage bar (0-3.3v assumed scale)
        barVoltage.style.width = `${Math.min(100, Math.max(0, (data.controlVoltage / 3.3) * 100))}%`;
    }

    if (data.pidOutput !== undefined) {
        if (valPid) valPid.innerText = _f(data.pidOutput, 1);
        if (barPid) {
            barPid.style.width = `${Math.min(100, Math.max(0, data.pidOutput))}%`;
        }
    }

    if (data.currentMA !== undefined) {
        if (valCurrent) valCurrent.innerText = _f(data.currentMA, 2);
        if (barCurrent) {
            // Scale 4-20mA to 0-100%
            const currentPercent = ((data.currentMA - 4) / 16) * 100;
            barCurrent.style.width = `${Math.min(100, Math.max(0, currentPercent))}%`;
        }
    }

    // Update Setpoint
    if (data.setpoint !== undefined) {
        valSetpoint.innerText = _f(data.setpoint, 2);
    }

    // Update Setpoint Percent
    if (data.setpointPercent !== undefined) {
        if (document.getElementById('valSetpointPercent')) {
            document.getElementById('valSetpointPercent').innerText = _f(data.setpointPercent, 1);
        }
    }

    // --- DYNAMIC SYNC MONITOR ---
    // This constantly compares the real hardware state (from ESP32) 
    // vs the cloud settings to verify they are perfectly merged.
    const cloudSetpoint = parseFloat(inputSetpoint.value) || 0;
    const hardwareSetpoint = data.setpoint || 0;
    
    // Check if they match within 0.01 margin
    if (Math.abs(cloudSetpoint - hardwareSetpoint) < 0.01) {
        syncConfigBadge.innerText = "✓ SYNCED WITH LCD";
        syncConfigBadge.className = "text-xs font-bold text-emerald-600 bg-emerald-50 px-2 py-1 rounded-md border border-emerald-200 shadow-sm";
    } else {
        syncConfigBadge.innerText = "⚠ SYNC PENDING...";
        syncConfigBadge.className = "text-xs font-bold text-amber-600 bg-amber-50 px-2 py-1 rounded-md border border-amber-200 animate-pulse";
    }

    // Warnings
    if (data.isStatic) {
        elStaticWarning.classList.remove('hidden');
    } else {
        elStaticWarning.classList.add('hidden');
    }
});

// Settings Monitor (One-time load or manual refresh)
function fetchSettings() {
    syncConfigBadge.innerText = "Loading...";
    syncConfigBadge.className = "text-xs font-semibold text-amber-600 bg-amber-50 px-2 py-1 rounded-md border border-amber-200";

    // Use onValue just once for a specific snapshot
    onValue(settingsRef, (snapshot) => {
        const data = snapshot.val();
        if (data) {
            inputKp.value = data.kp || '';
            inputKi.value = data.ki || '';
            inputKd.value = data.kd || '';
            inputSetpoint.value = data.setpoint || '';
            inputSafetyAllowance.value = data.safetyAllowance || '0.5';
            inputMinVoltage.value = data.minVoltage || '';
            inputMaxVoltage.value = data.maxVoltage || '';
            inputTankVolume.value = data.tankVolume || '';
            inputSensorMinV.value = data.sensorMinV || '';
            inputSensorMaxV.value = data.sensorMaxV || '';
            inputSensorMaxBar.value = data.sensorMaxBar || '';
            inputWorkingMaxBar.value = data.workingMaxBar || '';
            inputAccuracyMinV.value = data.accuracyMinV || '';
            inputAccuracyMaxV.value = data.accuracyMaxV || '';

            // Also update the active setpoint display
            if (data.setpoint !== undefined) valSetpoint.innerText = _f(data.setpoint, 2);

            syncConfigBadge.innerText = "Synced";
            syncConfigBadge.className = "text-xs font-semibold text-emerald-600 bg-emerald-50 px-2 py-1 rounded-md border border-emerald-200";
        } else {
            syncConfigBadge.innerText = "Defaults";
            syncConfigBadge.className = "text-xs font-semibold text-slate-500 bg-slate-100 px-2 py-1 rounded-md";
        }
    }, { onlyOnce: true });
}

// Event Listeners - Start/Stop Commands
btnStart.addEventListener('click', async () => {
    btnStart.classList.add('opacity-75', 'cursor-wait');
    await set(ref(db, 'devices/esp32_controller_1/commands/start_system'), true);
    setTimeout(() => btnStart.classList.remove('opacity-75', 'cursor-wait'), 500);
});

btnStop.addEventListener('click', async () => {
    btnStop.classList.add('opacity-75', 'cursor-wait');
    await set(ref(db, 'devices/esp32_controller_1/commands/stop_system'), true);
    setTimeout(() => btnStop.classList.remove('opacity-75', 'cursor-wait'), 500);
});

btnToggleSolenoid.addEventListener('click', async () => {
    btnToggleSolenoid.classList.add('opacity-75', 'cursor-wait');
    await set(ref(db, 'devices/esp32_controller_1/commands/toggle_solenoid'), true);
    setTimeout(() => btnToggleSolenoid.classList.remove('opacity-75', 'cursor-wait'), 500);
});

// Event Listeners - Settings Push
btnSaveConfig.addEventListener('click', async () => {
    // Show saving overlay
    savingOverlay.classList.remove('hidden');

    const newSettings = {
        kp: parseFloat(inputKp.value) || 0,
        ki: parseFloat(inputKi.value) || 0,
        kd: parseFloat(inputKd.value) || 0,
        setpoint: parseFloat(inputSetpoint.value) || 0,
        safetyAllowance: parseFloat(inputSafetyAllowance.value) || 0.5,
        minVoltage: parseFloat(inputMinVoltage.value) || 0,
        maxVoltage: parseFloat(inputMaxVoltage.value) || 0,
        tankVolume: parseFloat(inputTankVolume.value) || 0,
        sensorMinV: parseFloat(inputSensorMinV.value) || 0,
        sensorMaxV: parseFloat(inputSensorMaxV.value) || 0,
        sensorMaxBar: parseFloat(inputSensorMaxBar.value) || 0,
        workingMaxBar: parseFloat(inputWorkingMaxBar.value) || 0,
        accuracyMinV: parseFloat(inputAccuracyMinV.value) || 0,
        accuracyMaxV: parseFloat(inputAccuracyMaxV.value) || 0
    };

    try {
        // Write the settings
        await update(settingsRef, newSettings);
        // Trigger the update command for ESP32 to pull
        await set(ref(db, 'devices/esp32_controller_1/commands/update_settings'), true);

        // Update local display
        valSetpoint.innerText = _f(newSettings.setpoint, 2);

        syncConfigBadge.innerText = "Synced";
        syncConfigBadge.className = "text-xs font-semibold text-emerald-600 bg-emerald-50 px-2 py-1 rounded-md border border-emerald-200";
    } catch (e) {
        console.error("Save Error", e);
        syncConfigBadge.innerText = "Sync Failed";
        syncConfigBadge.className = "text-xs font-semibold text-rose-600 bg-rose-50 px-2 py-1 rounded-md border border-rose-200";
    } finally {
        setTimeout(() => savingOverlay.classList.add('hidden'), 500);
    }
});

btnRefreshSettings.addEventListener('click', fetchSettings);

// Initial Load
fetchSettings();

// --- OFFLINE WATCHDOG ---
// Checks if the device has sent a heartbeat recently
setInterval(() => {
    if (lastSeenTimestamp === 0) return; // No data received yet

    const now = Date.now();
    const drift = now - lastSeenTimestamp;
    
    if (drift > 15000) { // 15 seconds timeout
        elConnectionStatus.innerHTML = `
            <span class="relative flex h-3 w-3">
                <span class="relative inline-flex rounded-full h-3 w-3 bg-rose-500"></span>
            </span>
            <span class="text-rose-700">OFFLINE (POWER OFF)</span>
        `;
        elConnectionStatus.className = 'flex items-center gap-2 px-3 py-1.5 rounded-full bg-rose-50 text-sm font-semibold border border-rose-100 shadow-sm transition-all animate-pulse';
        
        // Zero out readings to indicate stale state
        valPressure.innerText = "0.00";
        valPressure.className = "text-4xl font-black text-slate-300 tracking-tighter tabular-nums";
        barPressure.style.width = "0%";
    } else {
        // Restore style if back online
        valPressure.className = "text-4xl font-black text-slate-900 tracking-tighter tabular-nums";
    }
}, 5000);
