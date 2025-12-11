// Blynk Config
#define BLYNK_TEMPLATE_ID "TMPL2dpuYuv8l"
#define BLYNK_TEMPLATE_NAME "PlantBuddy"
#define BLYNK_AUTH_TOKEN "token"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>

#include "nvs.h"
#include "nvs_flash.h"

// Buffers for WiFi credentials stored in ESP32 NVS
char ssid[50];
char pass[50];

// Sensor objects
Adafruit_AHTX0 aht;
const int SOIL_PIN = 32; 
const int PUMP_PIN = 26; 

BlynkTimer timer;

// Soil raw calibration values
const int SOIL_DRY_RAW = 2400;
const int SOIL_WET_RAW = 1200;

// Percent thresholds for labeling
const int SOIL_DRY_PERCENT = 20; // 20 percent and below -> DRY
const int SOIL_WET_PERCENT = 80; // 80 percent and above -> WET

// Alert when raw reading is high, which means dry soil
const int ALERT_RAW_THRESHOLD = 2000;                         // raw ADC value
const unsigned long ALERT_INTERVAL_MS = 30UL * 60UL * 1000UL; // 30 minutes
unsigned long lastAlertMillis = 0;

// Track pump state and last stable moisture reading
bool pumpOn = false;
int lastSoilPct = 0;
int lastSoilRaw = 0;


void loadWiFiFromNVS()
{
  Serial.println("[LOG] Initializing NVS...");

  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    Serial.println("[LOG] NVS full or new version, erasing...");
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  Serial.println("[LOG] Opening NVS namespace 'storage'...");
  nvs_handle_t handle;
  err = nvs_open("storage", NVS_READWRITE, &handle);
  if (err != ESP_OK)
  {
    Serial.printf("[ERROR] Failed to open NVS: %s\n", esp_err_to_name(err));
    return;
  }

  size_t ssid_len = sizeof(ssid);
  size_t pass_len = sizeof(pass);

  err = nvs_get_str(handle, "ssid", ssid, &ssid_len);
  err |= nvs_get_str(handle, "pass", pass, &pass_len);

  if (err == ESP_OK)
  {
    Serial.println("[LOG] Loaded WiFi credentials:");
    Serial.printf("SSID: %s\n", ssid);
  }
  else
  {
    Serial.println("[ERROR] Could not read WiFi credentials from NVS.");
  }

  nvs_close(handle);
}


int computeSoilPercent(int raw)
{
  int dry = SOIL_DRY_RAW; // driest end of calibration
  int wet = SOIL_WET_RAW; // wettest end of calibration

  if (dry < wet)
  {
    int temp = dry;
    dry = wet;
    wet = temp;
  }

  int pct = map(raw, dry, wet, 20, 100);
  pct = constrain(pct, 0, 100);

  return pct;
}

const char *soilLabel(int pct)
{
  if (pct <= SOIL_DRY_PERCENT)
    return "DRY";
  if (pct >= SOIL_WET_PERCENT)
    return "WET";
  return "OK";
}


BLYNK_WRITE(V3)
{
  int value = param.asInt();

  if (value == 1)
  {
    Serial.println("[LOG] Blynk button V3 -> PUMP ON");
    pumpOn = true;
    digitalWrite(PUMP_PIN, HIGH); 
  }
  else
  {
    Serial.println("[LOG] Blynk button V3 -> PUMP OFF");
    pumpOn = false;
    digitalWrite(PUMP_PIN, LOW);
  }
}

void sendSensorReadings()
{
  Serial.println("======= SENSOR READING =======");

  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);

  float t = temp.temperature;
  float h = humidity.relative_humidity;

  int soil_raw;
  int soil_pct;

  if (pumpOn)
  {
    // While pump is running, reuse last stable values
    soil_raw = lastSoilRaw;
    soil_pct = lastSoilPct;
    Serial.println("[LOG] Pump is ON, using last recorded soil values");
  }
  else
  {
    // Pump is off, take a fresh reading and update last stable values
    soil_raw = analogRead(SOIL_PIN);
    soil_pct = computeSoilPercent(soil_raw);
    lastSoilRaw = soil_raw;
    lastSoilPct = soil_pct;
  }

  Serial.printf("[LOG] Temp: %.2f C\n", t);
  Serial.printf("[LOG] Hum : %.2f %%\n", h);
  Serial.printf("[LOG] Soil raw: %d\n", soil_raw);
  Serial.printf("[LOG] Soil percent: %d %% (%s)\n",
                soil_pct, soilLabel(soil_pct));

  // Send to Blynk
  Blynk.virtualWrite(V1, t);        // Temperature
  Blynk.virtualWrite(V2, h);        // Humidity
  Blynk.virtualWrite(V0, soil_pct); // Soil moisture %

  // Alert logic based on raw sensor value, only when pump is off
  if (!pumpOn)
  {
    unsigned long now = millis();
    if (soil_raw > ALERT_RAW_THRESHOLD)
    {
      if (now - lastAlertMillis > ALERT_INTERVAL_MS)
      {
        Serial.println("[LOG] Soil too dry based on raw reading, sending alert event to Blynk");
        Blynk.logEvent("low_moisture", "Soil moisture is low. Open the app to water your plant.");
        lastAlertMillis = now;
      }
      else
      {
        Serial.println("[LOG] Soil dry but alert already sent recently");
      }
    }
  }
  else
  {
    Serial.println("[LOG] Skipping dry alert while pump is running");
  }

  Serial.println("================================\n");
}


void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("[LOG] Booting PlantBuddy...");

  loadWiFiFromNVS();

  Serial.printf("[LOG] Connecting to WiFi: %s\n", ssid);
  WiFi.begin(ssid, pass);

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20)
  {
    delay(500);
    Serial.print(".");
    retries++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("[LOG] WiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress());
  }
  else
  {
    Serial.println("[ERROR] WiFi failed to connect.");
  }

  Wire.begin(21, 22);

  if (!aht.begin())
  {
    Serial.println("[ERROR] Could not find DHT20 sensor!");
    while (1)
      delay(1000);
  }

  Serial.println("[LOG] DHT20 initialized.");

  analogReadResolution(12);
  pinMode(SOIL_PIN, INPUT);

  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW); 
  pumpOn = false;

  Serial.println("[LOG] Connecting to Blynk...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Read and send every 2 seconds
  timer.setInterval(2000L, sendSensorReadings);
}

void loop()
{
  Blynk.run();
  timer.run();
}