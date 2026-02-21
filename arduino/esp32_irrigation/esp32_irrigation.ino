/**
 * ESP32 Smart Drip Irrigation System (Simple Version)
 * 
 * Features:
 * - Single pump manual ON/OFF control via MQTT
 * - TLS encrypted MQTT (HiveMQ Cloud)
 * - Auto-reconnect WiFi & MQTT
 * 
 * Required Libraries (install via Library Manager):
 * - PubSubClient by Nick O'Leary
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

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
#define TOPIC_PUMP_SET    "plant/pump1/set"
#define TOPIC_PUMP_STATUS "plant/pump1/status"
#define TOPIC_STATUS      "plant/status"

// ============================================
// GPIO Pins
// ============================================
#define RELAY_PIN 26  // Pump relay
#define LED_PIN   2   // Built-in LED for status

// ============================================
// Relay Logic (true = active LOW, most common)
// ============================================
#define RELAY_ACTIVE_LOW true

// ============================================
// Global Objects
// ============================================
WiFiClientSecure espClient;
PubSubClient mqtt(espClient);

bool pumpIsOn = false;
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
// Setup
// ============================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n\n=== ESP32 Irrigation (Simple) ===");
    
    // Initialize GPIO
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    
    // Start with pump OFF
    digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);
    digitalWrite(LED_PIN, LOW);
    
    // Connect to WiFi
    setupWiFi();
    
    // Setup MQTT
    espClient.setCACert(hivemq_root_ca);
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setCallback(mqttCallback);
    
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
// MQTT Reconnect
// ============================================
void reconnectMQTT() {
    Serial.print("Connecting to MQTT...");
    
    if (mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS, 
                     TOPIC_STATUS, 1, true, "offline")) {
        Serial.println("connected!");
        
        // Publish online status
        mqtt.publish(TOPIC_STATUS, "online", true);
        
        // Subscribe to pump control
        mqtt.subscribe(TOPIC_PUMP_SET);
        
        // Publish current pump state
        mqtt.publish(TOPIC_PUMP_STATUS, pumpIsOn ? "on" : "off", true);
        
    } else {
        Serial.print("failed, rc=");
        Serial.println(mqtt.state());
    }
}

// ============================================
// MQTT Callback
// ============================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    
    Serial.printf("MQTT [%s]: %s\n", topic, message);
    
    if (strcmp(topic, TOPIC_PUMP_SET) == 0) {
        if (strcmp(message, "on") == 0) {
            setPump(true);
        } else if (strcmp(message, "off") == 0) {
            setPump(false);
        }
    }
}

// ============================================
// Set Pump State
// ============================================
void setPump(bool state) {
    pumpIsOn = state;
    
    if (state) {
        digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? LOW : HIGH);
        Serial.println(">>> PUMP ON");
    } else {
        digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);
        Serial.println(">>> PUMP OFF");
    }
    
    mqtt.publish(TOPIC_PUMP_STATUS, pumpIsOn ? "on" : "off", true);
}
