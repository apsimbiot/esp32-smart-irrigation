// MQTT Configuration
let client = null;
let config = {
    host: '',
    user: '',
    pass: ''
};

// Topics
const TOPIC_PUMP_SET = 'plant/pump1/set';
const TOPIC_PUMP_STATUS = 'plant/pump1/status';
const TOPIC_STATUS = 'plant/status';

// Initialize
document.addEventListener('DOMContentLoaded', () => {
    loadConfig();
    if (config.host && config.user && config.pass) {
        connectMQTT();
    } else {
        showConfigModal();
    }
});

// Load config from localStorage
function loadConfig() {
    const saved = localStorage.getItem('mqtt_config');
    if (saved) {
        config = JSON.parse(saved);
    }
}

// Save config to localStorage
function saveConfig() {
    config.host = document.getElementById('mqtt-host').value;
    config.user = document.getElementById('mqtt-user').value;
    config.pass = document.getElementById('mqtt-pass').value;
    
    localStorage.setItem('mqtt_config', JSON.stringify(config));
    hideConfigModal();
    connectMQTT();
}

// Show/hide config modal
function showConfigModal() {
    document.getElementById('config-modal').classList.add('show');
    document.getElementById('mqtt-host').value = config.host || '';
    document.getElementById('mqtt-user').value = config.user || '';
    document.getElementById('mqtt-pass').value = config.pass || '';
}

function hideConfigModal() {
    document.getElementById('config-modal').classList.remove('show');
}

// Connect to MQTT broker
function connectMQTT() {
    updateStatus('connecting', 'Connecting...');
    
    const url = `wss://${config.host}:8884/mqtt`;
    
    client = mqtt.connect(url, {
        username: config.user,
        password: config.pass,
        clientId: 'dashboard_' + Math.random().toString(16).substr(2, 8),
        clean: true,
        reconnectPeriod: 5000
    });
    
    client.on('connect', () => {
        console.log('Connected to MQTT');
        updateStatus('online', 'Connected');
        
        // Subscribe to topics
        client.subscribe(TOPIC_PUMP_STATUS);
        client.subscribe(TOPIC_STATUS);
    });
    
    client.on('message', (topic, message) => {
        const msg = message.toString();
        console.log(`${topic}: ${msg}`);
        
        if (topic === TOPIC_PUMP_STATUS) {
            updatePumpStatus(msg);
        }
        
        updateLastUpdate();
    });
    
    client.on('error', (err) => {
        console.error('MQTT Error:', err);
        updateStatus('offline', 'Error');
    });
    
    client.on('offline', () => {
        updateStatus('offline', 'Offline');
    });
    
    client.on('reconnect', () => {
        updateStatus('connecting', 'Reconnecting...');
    });
}

// Update connection status display
function updateStatus(state, text) {
    const statusEl = document.getElementById('status');
    const textEl = document.getElementById('status-text');
    
    statusEl.className = 'status ' + state;
    textEl.textContent = text;
}

// Update pump status display
function updatePumpStatus(status) {
    const el = document.getElementById('pump1-status');
    const card = document.getElementById('pump1-card');
    
    if (status === 'on') {
        el.textContent = 'ON';
        el.classList.add('on');
        card.classList.add('active');
    } else {
        el.textContent = 'OFF';
        el.classList.remove('on');
        card.classList.remove('active');
    }
}

// Set pump state
function setPump(state) {
    if (client && client.connected) {
        client.publish(TOPIC_PUMP_SET, state);
        console.log(`Pump → ${state}`);
    }
}

// Update last update timestamp
function updateLastUpdate() {
    const el = document.getElementById('last-update');
    const now = new Date();
    el.textContent = `Last update: ${now.toLocaleTimeString()}`;
}
