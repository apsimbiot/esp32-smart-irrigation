/**
 * ESP32 Smart Irrigation - Web Dashboard
 * Connects to HiveMQ Cloud via WebSockets
 */

// ============================================
// MQTT Configuration
// ============================================
const CONFIG_KEY = 'irrigation_mqtt_config';
let client = null;
let isConnected = false;

// Topics
const TOPICS = {
    PUMP1_SET: 'plant/pump1/set',
    PUMP2_SET: 'plant/pump2/set',
    PUMP1_STATUS: 'plant/pump1/status',
    PUMP2_STATUS: 'plant/pump2/status',
    PUMP1_SCHEDULE: 'plant/pump1/schedule',
    PUMP2_SCHEDULE: 'plant/pump2/schedule',
    STATUS: 'plant/status'
};

// ============================================
// Initialization
// ============================================
document.addEventListener('DOMContentLoaded', () => {
    const config = loadConfig();
    if (config && config.host && config.user && config.pass) {
        document.getElementById('mqtt-host').value = config.host;
        document.getElementById('mqtt-user').value = config.user;
        document.getElementById('mqtt-pass').value = config.pass;
        connectMQTT(config);
    } else {
        showConfigModal();
    }
});

// ============================================
// Config Management
// ============================================
function loadConfig() {
    const saved = localStorage.getItem(CONFIG_KEY);
    return saved ? JSON.parse(saved) : null;
}

function saveConfig() {
    const config = {
        host: document.getElementById('mqtt-host').value.trim(),
        user: document.getElementById('mqtt-user').value.trim(),
        pass: document.getElementById('mqtt-pass').value.trim()
    };
    
    if (!config.host || !config.user || !config.pass) {
        alert('Please fill all fields');
        return;
    }
    
    localStorage.setItem(CONFIG_KEY, JSON.stringify(config));
    hideConfigModal();
    connectMQTT(config);
}

function showConfigModal() {
    document.getElementById('config-modal').classList.add('show');
}

function hideConfigModal() {
    document.getElementById('config-modal').classList.remove('show');
}

// ============================================
// MQTT Connection
// ============================================
function connectMQTT(config) {
    updateStatus('Connecting...', false);
    
    // HiveMQ Cloud WebSocket URL
    const url = `wss://${config.host}:8884/mqtt`;
    
    const options = {
        username: config.user,
        password: config.pass,
        clientId: 'web-dashboard-' + Math.random().toString(16).substr(2, 8),
        clean: true,
        reconnectPeriod: 5000,
        connectTimeout: 30000
    };
    
    client = mqtt.connect(url, options);
    
    client.on('connect', () => {
        console.log('Connected to MQTT');
        isConnected = true;
        updateStatus('Connected', true);
        
        // Subscribe to all topics
        Object.values(TOPICS).forEach(topic => {
            client.subscribe(topic);
        });
    });
    
    client.on('message', (topic, message) => {
        const payload = message.toString();
        console.log(`[${topic}] ${payload}`);
        handleMessage(topic, payload);
    });
    
    client.on('error', (err) => {
        console.error('MQTT Error:', err);
        updateStatus('Error: ' + err.message, false);
    });
    
    client.on('close', () => {
        console.log('MQTT Disconnected');
        isConnected = false;
        updateStatus('Disconnected', false);
    });
    
    client.on('reconnect', () => {
        updateStatus('Reconnecting...', false);
    });
}

// ============================================
// Message Handling
// ============================================
function handleMessage(topic, payload) {
    updateLastUpdate();
    
    switch (topic) {
        case TOPICS.PUMP1_STATUS:
            updatePumpStatus(1, payload);
            break;
        case TOPICS.PUMP2_STATUS:
            updatePumpStatus(2, payload);
            break;
        case TOPICS.PUMP1_SCHEDULE:
            updateScheduleUI(1, JSON.parse(payload));
            break;
        case TOPICS.PUMP2_SCHEDULE:
            updateScheduleUI(2, JSON.parse(payload));
            break;
        case TOPICS.STATUS:
            updateESP32Status(payload);
            break;
    }
}

// ============================================
// UI Updates
// ============================================
function updateStatus(text, online) {
    document.getElementById('status-text').textContent = text;
    const statusEl = document.getElementById('status');
    statusEl.className = 'status ' + (online ? 'online' : 'offline');
}

function updatePumpStatus(pumpNum, status) {
    const statusEl = document.getElementById(`pump${pumpNum}-status`);
    const cardEl = document.getElementById(`pump${pumpNum}-card`);
    const isOn = status === 'on';
    
    statusEl.textContent = isOn ? 'ON' : 'OFF';
    statusEl.className = 'pump-status' + (isOn ? ' on' : '');
    cardEl.className = 'pump-card' + (isOn ? ' active' : '');
}

function updateScheduleUI(pumpNum, schedule) {
    const prefix = `pump${pumpNum}-schedule`;
    
    document.getElementById(`${prefix}-enabled`).checked = schedule.enabled;
    document.getElementById(`${prefix}-interval`).value = schedule.intervalDays;
    document.getElementById(`${prefix}-duration`).value = schedule.durationMs;
    
    // Format time
    const hour = String(schedule.hour).padStart(2, '0');
    const minute = String(schedule.minute).padStart(2, '0');
    document.getElementById(`${prefix}-time`).value = `${hour}:${minute}`;
}

function updateESP32Status(status) {
    if (status === 'offline') {
        updateStatus('ESP32 Offline', false);
    }
}

function updateLastUpdate() {
    const now = new Date().toLocaleTimeString();
    document.getElementById('last-update').textContent = `Last update: ${now}`;
}

// ============================================
// Pump Controls
// ============================================
function setPump(pumpNum, state) {
    if (!isConnected) {
        alert('Not connected to MQTT');
        return;
    }
    
    const topic = pumpNum === 1 ? TOPICS.PUMP1_SET : TOPICS.PUMP2_SET;
    client.publish(topic, state);
}

function setPumpTimed(pumpNum) {
    if (!isConnected) {
        alert('Not connected to MQTT');
        return;
    }
    
    const duration = parseInt(document.getElementById(`pump${pumpNum}-duration`).value);
    const topic = pumpNum === 1 ? TOPICS.PUMP1_SET : TOPICS.PUMP2_SET;
    const payload = JSON.stringify({ state: 'on', duration: duration });
    
    client.publish(topic, payload);
}

// ============================================
// Schedule Controls
// ============================================
function updateSchedule(pumpNum) {
    if (!isConnected) {
        return;
    }
    
    const prefix = `pump${pumpNum}-schedule`;
    const timeValue = document.getElementById(`${prefix}-time`).value;
    const [hour, minute] = timeValue.split(':').map(Number);
    
    const schedule = {
        enabled: document.getElementById(`${prefix}-enabled`).checked,
        hour: hour,
        minute: minute,
        intervalDays: parseInt(document.getElementById(`${prefix}-interval`).value),
        durationMs: parseInt(document.getElementById(`${prefix}-duration`).value)
    };
    
    const topic = pumpNum === 1 ? TOPICS.PUMP1_SCHEDULE : TOPICS.PUMP2_SCHEDULE;
    client.publish(topic, JSON.stringify(schedule));
}
