#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <BH1750.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

// 🛜 Cấu hình WiFi
const char *ssid = "mypc";
const char *password = "11111111";
const char *serverName = "http://192.168.137.88/post-esp-data.php";
String apiKeyValue = "tPmAT5Ab3j7F9";

// 🛠 Cấu hình cảm biến
Adafruit_AHTX0 aht10;
BH1750 lightMeter;

#define GAS_SENSOR_A0 35
#define FLAME_SENSOR_D0 32

// 🛠 Cấu hình thiết bị điều khiển
#define BUZZER_PIN 15
#define FAN_PIN 19

// 🔹 Biến lưu dữ liệu
float temperature = 0.0, humidity = 0.0, lightLevel = 0.0;
int gasLevel = 0, flameStatus = 0;

// 🔧 Cấu hình ban đầu
void setup()
{
    Serial.begin(115200);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(FAN_PIN, OUTPUT);
    pinMode(FLAME_SENSOR_D0, INPUT);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(FAN_PIN, LOW);

    // 🔄 Kết nối WiFi
    WiFi.begin(ssid, password);
    Serial.print("🔄 Đang kết nối WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n✅ WiFi đã kết nối!");
    Serial.print("📡 Địa chỉ IP: ");
    Serial.println(WiFi.localIP());

    // 🚀 Khởi động màn hình TFT
    tft.init();
    tft.setRotation(1); // Điều chỉnh hướng màn hình nếu cần
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);

    tft.setCursor(10, 20);
    tft.print("WiFi OK!");

    tft.setCursor(10, 50);
    tft.print("IP: ");
    tft.print(WiFi.localIP());

    delay(2000);

    // 📡 Khởi động cảm biến
    if (!aht10.begin())
    {
        Serial.println("⚠️ Lỗi AHT10!");
    }
    if (!lightMeter.begin())
    {
        Serial.println("⚠️ Lỗi BH1750!");
    }
}

// 🛠 Đọc dữ liệu cảm biến
void readSensors()
{
    sensors_event_t humidityEvent, tempEvent;
    aht10.getEvent(&humidityEvent, &tempEvent);
    temperature = tempEvent.temperature;
    humidity = humidityEvent.relative_humidity;
    lightLevel = lightMeter.readLightLevel();
    gasLevel = analogRead(GAS_SENSOR_A0);
    flameStatus = digitalRead(FLAME_SENSOR_D0);
}

// 📡 Gửi dữ liệu lên server
void sendSensorData()
{
    readSensors();

    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        http.begin(serverName);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        String postData = "api_key=" + apiKeyValue +
                          "&temperature=" + String(temperature) +
                          "&humidity=" + String(humidity) +
                          "&gas=" + String(gasLevel) +
                          "&flame=" + String(flameStatus) +
                          "&light=" + String(lightLevel);

        Serial.println("📤 Gửi dữ liệu: " + postData);
        int httpResponseCode = http.POST(postData);

        Serial.println("📩 Mã phản hồi: " + String(httpResponseCode));
        Serial.println("📥 Phản hồi từ server: " + http.getString());

        http.end();
    }
    else
    {
        Serial.println("⚠️ Mất kết nối WiFi! Đang thử lại...");
        WiFi.reconnect();
    }
}

// 🔔 Kiểm tra và điều khiển còi, quạt
void checkAndControlDevices()
{
    if (flameStatus == LOW && gasLevel > 600)
    {
        digitalWrite(BUZZER_PIN, HIGH);
        Serial.println("🚨 Báo động: Phát hiện lửa hoặc khí gas cao!");
    }
    else
    {
        digitalWrite(BUZZER_PIN, LOW);
    }

    if (temperature > 30 || gasLevel > 300)
    {
        digitalWrite(FAN_PIN, HIGH);
        Serial.println("🌀 Quạt bật do nhiệt độ/gas cao!");
    }
    else
    {
        digitalWrite(FAN_PIN, LOW);
    }
}

// 🖥 Hiển thị dữ liệu lên màn hình OLED
void displayData()
{
    tft.fillScreen(TFT_BLACK); // Xóa màn hình trước khi hiển thị dữ liệu mới

    tft.setCursor(10, 20);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.print("Nhiet do: ");
    tft.print(temperature);
    tft.println(" C");

    tft.setCursor(10, 50);
    tft.print("Do am: ");
    tft.print(humidity);
    tft.println(" %");

    tft.setCursor(10, 80);
    tft.print("Gas: ");
    tft.print(gasLevel);

    tft.setCursor(10, 110);
    tft.print("Anh sang: ");
    tft.print(lightLevel);
}

// 🔁 Loop
void loop()
{
    sendSensorData();
    checkAndControlDevices();
    displayData();
    delay(5000);
}
