/**
 * ESP32 Smart Drip Irrigation System
 * 
 * Features:
 * - Dual pump control via MQTT
 * - Scheduling with NTP time sync
 * - TLS encrypted MQTT (HiveMQ Cloud)
 * - Auto-reconnect WiFi & MQTT
 * - Safety timeout for pumps
 * 
 * Required Libraries (install via Library Manager):
 * - PubSubClient by Nick O'Leary
 * - ArduinoJson by Benoit Blanchon
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <Preferences.h>

// ============================================
// WiFi Configuration
// ============================================
#define WIFI_SSID "Airtel_anur_5235"
#define WIFI_PASS "Air@51429"

// ============================================
// MQTT Configuration (HiveMQ Cloud)
// ============================================
#define MQTT_HOST "17d1b4dd181a4a5b9c95b5f674c2a672.s1.eu.hivemq.cloud"
#define MQTT_PORT 8883
#define MQTT_USER "esp32"
#define MQTT_PASS "PlantWater26"
#define MQTT_CLIENT_ID "esp32-irrigation"

// ============================================
// MQTT Topics
// ============================================
#define TOPIC_PUMP1_SET      "plant/pump1/set"
#define TOPIC_PUMP2_SET      "plant/pump2/set"
#define TOPIC_PUMP1_STATUS   "plant/pump1/status"
#define TOPIC_PUMP2_STATUS   "plant/pump2/status"
#define TOPIC_PUMP1_SCHEDULE "plant/pump1/schedule"
#define TOPIC_PUMP2_SCHEDULE "plant/pump2/schedule"
#define TOPIC_STATUS         "plant/status"

// ============================================
// GPIO Pins
// ============================================
#define RELAY_PIN_1 26  // Pump 1 relay
#define RELAY_PIN_2 27  // Pump 2 relay
#define LED_PIN     2   // Built-in LED for status

// ============================================
// Pump Settings
// ============================================
#define DEFAULT_PUMP_DURATION_MS 30000  // 30 seconds default run time
#define MAX_PUMP_DURATION_MS 300000     // 5 minutes max safety limit

// ============================================
// Schedule Settings
// ============================================
#define SCHEDULE_CHECK_INTERVAL_MS 60000  // Check schedule every minute
#define NTP_SERVER "pool.ntp.org"
#define TIME_ZONE_OFFSET 19800  // IST = UTC+5:30 = 5.5 * 3600

// ============================================
// Relay Logic
// ============================================
// Set to true if relay is active-LOW (most common)
#define RELAY_ACTIVE_LOW true

// ============================================
// Global Objects
// ============================================
WiFiClientSecure espClient;
PubSubClient mqtt(espClient);
Preferences preferences;

// ============================================
// Pump State
// ============================================
struct PumpState {
    bool isOn;
    unsigned long startTime;
    unsigned long duration;
    int relayPin;
    const char* setTopic;
    const char* statusTopic;
    const char* scheduleTopic;
};

PumpState pump1 = {false, 0, DEFAULT_PUMP_DURATION_MS, RELAY_PIN_1, 
                   TOPIC_PUMP1_SET, TOPIC_PUMP1_STATUS, TOPIC_PUMP1_SCHEDULE};
PumpState pump2 = {false, 0, DEFAULT_PUMP_DURATION_MS, RELAY_PIN_2, 
                   TOPIC_PUMP2_SET, TOPIC_PUMP2_STATUS, TOPIC_PUMP2_SCHEDULE};

// ============================================
// Schedule Structure
// ============================================
struct Schedule {
    bool enabled;
    int hour;
    int minute;
    int intervalDays;  // 1 = daily, 2 = every 2 days, etc.
    unsigned long durationMs;
    unsigned long lastRunDay;  // Day number since epoch
};

Schedule schedule1 = {false, 7, 0, 1, 30000, 0};  // Default: 7:00 AM daily, 30s
Schedule schedule2 = {false, 7, 0, 3, 30000, 0};  // Default: 7:00 AM every 3 days, 30s

// ============================================
// Timing
// ============================================
unsigned long lastScheduleCheck = 0;
unsigned long lastMqttReconnect = 0;
unsigned long lastWifiCheck = 0;

// ============================================
// HiveMQ Cloud Root CA Certificate
// ============================================
const char* hivemq_root_ca = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

// ============================================
// Function Declarations
// ============================================
void setupWiFi();
void setupMQTT();
void setupTime();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void reconnectMQTT();
void publishStatus(PumpState& pump);
void publishSchedule(int pumpNum, Schedule& schedule);
void setPump(PumpState& pump, bool state);
void checkPumpTimeout(PumpState& pump);
void checkSchedule();
void handlePumpCommand(PumpState& pump, const char* payload);
void handleScheduleUpdate(int pumpNum, Schedule& schedule, const char* payload);
void saveSchedules();
void loadSchedules();
unsigned long getCurrentDay();

// ============================================
// Setup
// ============================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n\n=== ESP32 Smart Irrigation System ===");
    
    // Initialize GPIO
    pinMode(RELAY_PIN_1, OUTPUT);
    pinMode(RELAY_PIN_2, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    
    // Start with pumps OFF
    digitalWrite(RELAY_PIN_1, RELAY_ACTIVE_LOW ? HIGH : LOW);
    digitalWrite(RELAY_PIN_2, RELAY_ACTIVE_LOW ? HIGH : LOW);
    digitalWrite(LED_PIN, LOW);
    
    // Load saved schedules
    preferences.begin("irrigation", false);
    loadSchedules();
    
    // Connect to WiFi
    setupWiFi();
    
    // Sync time via NTP
    setupTime();
    
    // Setup MQTT
    setupMQTT();
    
    Serial.println("Setup complete!");
}

// ============================================
// Main Loop
// ============================================
void loop() {
    // Maintain WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        if (millis() - lastWifiCheck > 10000) {
            lastWifiCheck = millis();
            Serial.println("WiFi disconnected, reconnecting...");
            setupWiFi();
        }
    }
    
    // Maintain MQTT connection
    if (!mqtt.connected()) {
        if (millis() - lastMqttReconnect > 5000) {
            lastMqttReconnect = millis();
            reconnectMQTT();
        }
    }
    mqtt.loop();
    
    // Check pump safety timeouts
    checkPumpTimeout(pump1);
    checkPumpTimeout(pump2);
    
    // Check schedules
    if (millis() - lastScheduleCheck > SCHEDULE_CHECK_INTERVAL_MS) {
        lastScheduleCheck = millis();
        checkSchedule();
    }
    
    // Status LED - blink if connected
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 1000) {
        lastBlink = millis();
        if (mqtt.connected()) {
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        } else {
            digitalWrite(LED_PIN, LOW);
        }
    }
}

// ============================================
// WiFi Setup
// ============================================
void setupWiFi() {
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nWiFi connection failed!");
    }
}

// ============================================
// Time Setup (NTP)
// ============================================
void setupTime() {
    Serial.println("Syncing time via NTP...");
    configTime(TIME_ZONE_OFFSET, 0, NTP_SERVER);
    
    struct tm timeinfo;
    int attempts = 0;
    while (!getLocalTime(&timeinfo) && attempts < 10) {
        delay(500);
        attempts++;
    }
    
    if (getLocalTime(&timeinfo)) {
        Serial.printf("Time synced: %02d:%02d:%02d\n", 
                      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    } else {
        Serial.println("Failed to sync time!");
    }
}

// ============================================
// MQTT Setup
// ============================================
void setupMQTT() {
    espClient.setCACert(hivemq_root_ca);
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setCallback(mqttCallback);
    mqtt.setBufferSize(1024);
}

// ============================================
// MQTT Reconnect
// ============================================
void reconnectMQTT() {
    Serial.print("Connecting to MQTT...");
    
    // Set Last Will Testament
    if (mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS, 
                     TOPIC_STATUS, 1, true, "offline")) {
        Serial.println("connected!");
        
        // Publish online status
        mqtt.publish(TOPIC_STATUS, "online", true);
        
        // Subscribe to control topics
        mqtt.subscribe(TOPIC_PUMP1_SET);
        mqtt.subscribe(TOPIC_PUMP2_SET);
        mqtt.subscribe(TOPIC_PUMP1_SCHEDULE);
        mqtt.subscribe(TOPIC_PUMP2_SCHEDULE);
        
        // Publish current states
        publishStatus(pump1);
        publishStatus(pump2);
        publishSchedule(1, schedule1);
        publishSchedule(2, schedule2);
        
    } else {
        Serial.print("failed, rc=");
        Serial.println(mqtt.state());
    }
}

// ============================================
// MQTT Callback
// ============================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // Null-terminate payload
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    
    Serial.printf("MQTT [%s]: %s\n", topic, message);
    
    if (strcmp(topic, TOPIC_PUMP1_SET) == 0) {
        handlePumpCommand(pump1, message);
    } else if (strcmp(topic, TOPIC_PUMP2_SET) == 0) {
        handlePumpCommand(pump2, message);
    } else if (strcmp(topic, TOPIC_PUMP1_SCHEDULE) == 0) {
        handleScheduleUpdate(1, schedule1, message);
    } else if (strcmp(topic, TOPIC_PUMP2_SCHEDULE) == 0) {
        handleScheduleUpdate(2, schedule2, message);
    }
}

// ============================================
// Handle Pump Command
// ============================================
void handlePumpCommand(PumpState& pump, const char* payload) {
    if (strcmp(payload, "on") == 0) {
        setPump(pump, true);
    } else if (strcmp(payload, "off") == 0) {
        setPump(pump, false);
    } else {
        // Try to parse JSON with duration
        StaticJsonDocument<128> doc;
        if (deserializeJson(doc, payload) == DeserializationError::Ok) {
            if (doc.containsKey("state")) {
                bool state = doc["state"].as<bool>() || 
                            strcmp(doc["state"].as<const char*>(), "on") == 0;
                if (doc.containsKey("duration")) {
                    pump.duration = min((unsigned long)doc["duration"].as<int>(), 
                                       MAX_PUMP_DURATION_MS);
                }
                setPump(pump, state);
            }
        }
    }
}

// ============================================
// Handle Schedule Update
// ============================================
void handleScheduleUpdate(int pumpNum, Schedule& schedule, const char* payload) {
    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, payload) != DeserializationError::Ok) {
        Serial.println("Failed to parse schedule JSON");
        return;
    }
    
    if (doc.containsKey("enabled")) schedule.enabled = doc["enabled"];
    if (doc.containsKey("hour")) schedule.hour = doc["hour"];
    if (doc.containsKey("minute")) schedule.minute = doc["minute"];
    if (doc.containsKey("intervalDays")) schedule.intervalDays = doc["intervalDays"];
    if (doc.containsKey("durationMs")) schedule.durationMs = doc["durationMs"];
    
    saveSchedules();
    publishSchedule(pumpNum, schedule);
    
    Serial.printf("Schedule %d updated: %s at %02d:%02d every %d days\n",
                  pumpNum, schedule.enabled ? "ON" : "OFF",
                  schedule.hour, schedule.minute, schedule.intervalDays);
}

// ============================================
// Set Pump State
// ============================================
void setPump(PumpState& pump, bool state) {
    pump.isOn = state;
    
    if (state) {
        pump.startTime = millis();
        digitalWrite(pump.relayPin, RELAY_ACTIVE_LOW ? LOW : HIGH);
        Serial.printf("Pump ON (duration: %lu ms)\n", pump.duration);
    } else {
        digitalWrite(pump.relayPin, RELAY_ACTIVE_LOW ? HIGH : LOW);
        pump.duration = DEFAULT_PUMP_DURATION_MS;  // Reset to default
        Serial.println("Pump OFF");
    }
    
    publishStatus(pump);
}

// ============================================
// Publish Pump Status
// ============================================
void publishStatus(PumpState& pump) {
    mqtt.publish(pump.statusTopic, pump.isOn ? "on" : "off", true);
}

// ============================================
// Publish Schedule
// ============================================
void publishSchedule(int pumpNum, Schedule& schedule) {
    StaticJsonDocument<256> doc;
    doc["enabled"] = schedule.enabled;
    doc["hour"] = schedule.hour;
    doc["minute"] = schedule.minute;
    doc["intervalDays"] = schedule.intervalDays;
    doc["durationMs"] = schedule.durationMs;
    doc["lastRunDay"] = schedule.lastRunDay;
    
    char buffer[256];
    serializeJson(doc, buffer);
    
    const char* topic = (pumpNum == 1) ? TOPIC_PUMP1_SCHEDULE : TOPIC_PUMP2_SCHEDULE;
    mqtt.publish(topic, buffer, true);
}

// ============================================
// Check Pump Timeout
// ============================================
void checkPumpTimeout(PumpState& pump) {
    if (pump.isOn && (millis() - pump.startTime > pump.duration)) {
        Serial.println("Pump timeout - turning off");
        setPump(pump, false);
    }
}

// ============================================
// Check Schedule
// ============================================
void checkSchedule() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return;
    }
    
    unsigned long currentDay = getCurrentDay();
    int currentHour = timeinfo.tm_hour;
    int currentMinute = timeinfo.tm_min;
    
    // Check schedule 1
    if (schedule1.enabled && !pump1.isOn) {
        if (currentHour == schedule1.hour && currentMinute == schedule1.minute) {
            if ((currentDay - schedule1.lastRunDay) >= (unsigned long)schedule1.intervalDays) {
                Serial.println("Schedule 1 triggered!");
                pump1.duration = schedule1.durationMs;
                setPump(pump1, true);
                schedule1.lastRunDay = currentDay;
                saveSchedules();
            }
        }
    }
    
    // Check schedule 2
    if (schedule2.enabled && !pump2.isOn) {
        if (currentHour == schedule2.hour && currentMinute == schedule2.minute) {
            if ((currentDay - schedule2.lastRunDay) >= (unsigned long)schedule2.intervalDays) {
                Serial.println("Schedule 2 triggered!");
                pump2.duration = schedule2.durationMs;
                setPump(pump2, true);
                schedule2.lastRunDay = currentDay;
                saveSchedules();
            }
        }
    }
}

// ============================================
// Get Current Day (days since epoch)
// ============================================
unsigned long getCurrentDay() {
    time_t now;
    time(&now);
    return now / 86400;
}

// ============================================
// Save Schedules to Flash
// ============================================
void saveSchedules() {
    preferences.putBool("s1_enabled", schedule1.enabled);
    preferences.putInt("s1_hour", schedule1.hour);
    preferences.putInt("s1_minute", schedule1.minute);
    preferences.putInt("s1_interval", schedule1.intervalDays);
    preferences.putULong("s1_duration", schedule1.durationMs);
    preferences.putULong("s1_lastrun", schedule1.lastRunDay);
    
    preferences.putBool("s2_enabled", schedule2.enabled);
    preferences.putInt("s2_hour", schedule2.hour);
    preferences.putInt("s2_minute", schedule2.minute);
    preferences.putInt("s2_interval", schedule2.intervalDays);
    preferences.putULong("s2_duration", schedule2.durationMs);
    preferences.putULong("s2_lastrun", schedule2.lastRunDay);
}

// ============================================
// Load Schedules from Flash
// ============================================
void loadSchedules() {
    schedule1.enabled = preferences.getBool("s1_enabled", false);
    schedule1.hour = preferences.getInt("s1_hour", 7);
    schedule1.minute = preferences.getInt("s1_minute", 0);
    schedule1.intervalDays = preferences.getInt("s1_interval", 1);
    schedule1.durationMs = preferences.getULong("s1_duration", 30000);
    schedule1.lastRunDay = preferences.getULong("s1_lastrun", 0);
    
    schedule2.enabled = preferences.getBool("s2_enabled", false);
    schedule2.hour = preferences.getInt("s2_hour", 7);
    schedule2.minute = preferences.getInt("s2_minute", 0);
    schedule2.intervalDays = preferences.getInt("s2_interval", 3);
    schedule2.durationMs = preferences.getULong("s2_duration", 30000);
    schedule2.lastRunDay = preferences.getULong("s2_lastrun", 0);
    
    Serial.println("Schedules loaded from flash");
}
