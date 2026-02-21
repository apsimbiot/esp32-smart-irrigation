#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// WiFi Configuration
// ============================================
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"

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

#endif // CONFIG_H
