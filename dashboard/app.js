// MQTT Configuration (hardcoded for convenience)
let client = null;
let config = {
    host: '17d1b4dd181a4a5b9c95b5f674c2a672.s1.eu.hivemq.cloud',
    user: 'esp32',
    pass: 'PlantWater26'
};

// Topics
const TOPIC_PUMP_SET = 'plant/pump1/set';
const TOPIC_PUMP_STATUS = 'plant/pump1/status';
const TOPIC_STATUS = 'plant/status';

// Watering animation state
let wateringTimeout = null;
let thankYouTimeout = null;

// Initialize - auto-connect with hardcoded credentials
document.addEventListener('DOMContentLoaded', () => {
    connectMQTT();
});

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
        
        if (topic === TOPIC_STATUS) {
            updateDeviceStatus(msg);
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

// Update connection status display (dashboard connection)
function updateStatus(state, text) {
    const statusEl = document.getElementById('status');
    const textEl = document.getElementById('status-text');
    
    statusEl.className = 'status ' + state;
    textEl.textContent = text;
}

// Update ESP32 device status
function updateDeviceStatus(status) {
    const statusEl = document.getElementById('status');
    const textEl = document.getElementById('status-text');
    
    if (status === 'online') {
        statusEl.className = 'status online';
        textEl.textContent = 'ESP32 Online';
    } else {
        statusEl.className = 'status offline';
        textEl.textContent = 'ESP32 Offline';
    }
}

// Update pump status display
function updatePumpStatus(status) {
    const el = document.getElementById('pump1-status');
    const card = document.getElementById('pump1-card');
    const waterFlow = document.getElementById('water-flow');
    const plantPots = document.querySelectorAll('.plant-pot');
    
    if (status === 'on') {
        el.textContent = 'ON';
        el.classList.add('on');
        card.classList.add('active');
        
        // Start water flow animation
        startWateringAnimation();
    } else {
        el.textContent = 'OFF';
        el.classList.remove('on');
        card.classList.remove('active');
        
        // Stop animations
        stopWateringAnimation();
    }
}

// Start the watering animation sequence
function startWateringAnimation() {
    const waterFlow = document.getElementById('water-flow');
    const plantPots = document.querySelectorAll('.plant-pot');
    
    // Clear any existing timeouts
    clearTimeout(wateringTimeout);
    clearTimeout(thankYouTimeout);
    
    // Reset states
    waterFlow.style.width = '0%';
    waterFlow.classList.remove('flowing');
    plantPots.forEach(pot => {
        pot.classList.remove('watering', 'happy');
    });
    
    // Start water flow
    setTimeout(() => {
        waterFlow.classList.add('flowing');
    }, 100);
    
    // Trigger each plant's drip animation as water reaches them
    const delays = [500, 1000, 1500, 2000]; // Water reaches each plant
    
    plantPots.forEach((pot, index) => {
        setTimeout(() => {
            pot.classList.add('watering');
        }, delays[index]);
    });
    
}

// Stop watering animation
function stopWateringAnimation() {
    const waterFlow = document.getElementById('water-flow');
    const plantPots = document.querySelectorAll('.plant-pot');
    
    clearTimeout(wateringTimeout);
    clearTimeout(thankYouTimeout);
    
    // Stop dripping
    plantPots.forEach(pot => {
        pot.classList.remove('watering');
    });
    
    // Reset water flow
    waterFlow.classList.remove('flowing');
    waterFlow.style.width = '0%';
    
    // Show thank you messages when stopping
    showThankYouMessages();
}

// Show thank you messages from plants
function showThankYouMessages() {
    const plantPots = document.querySelectorAll('.plant-pot');
    
    // Stop dripping first
    plantPots.forEach(pot => {
        pot.classList.remove('watering');
    });
    
    // Show thank you with staggered timing
    plantPots.forEach((pot, index) => {
        setTimeout(() => {
            pot.classList.add('happy');
        }, index * 300);
    });
    
    // Hide thank you messages after 3 seconds
    thankYouTimeout = setTimeout(() => {
        plantPots.forEach(pot => {
            pot.classList.remove('happy');
        });
    }, 5000);
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
