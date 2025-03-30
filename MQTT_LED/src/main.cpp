#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ✅ Cấu hình WiFi
const char *ssid = "mypc";
const char *password = "11111111";

// ✅ IP tĩnh cho ESP32
IPAddress local_IP(192, 168, 137, 50);
IPAddress gateway(192, 168, 137, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

// ✅ Cấu hình GPIO cho từng thiết bị
struct Device
{
  const char *name;
  const char *api;
  int relayPin;
  int touchPin;
  bool state;
};

// ✅ Danh sách 6 thiết bị
Device devices[] = {
    {"LivingRoom_Light", "livingroom", 2, 15, false},
    {"Desk_Lamp", "desklamp", 4, 16, false},
    {"Fan", "fan", 5, 17, false},
    {"Kitchen_Light", "kitchen", 18, 19, false},
    {"Bedroom_Light", "bedroom", 21, 22, false},
    {"Garage_Door", "garage", 23, 25, false}};

// ✅ Lưu trạng thái cảm biến chạm
static bool lastTouchState[6] = {LOW, LOW, LOW, LOW, LOW, LOW};

const char *serverName = "http://192.168.137.88/log.php";
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void notifyClients(String json)
{
  ws.textAll(json);
}
// ✅ Hàm mã hóa URL để tránh lỗi ký tự đặc biệt
String urlencode(String str)
{
  String encodedString = "";
  char c;
  char buf[4];
  for (int i = 0; i < str.length(); i++)
  {
    c = str.charAt(i);
    if (isalnum(c))
    {
      encodedString += c;
    }
    else if (c == ' ')
    {
      encodedString += "%20";
    }
    else
    {
      sprintf(buf, "%%%02X", c);
      encodedString += buf;
    }
  }
  return encodedString;
}

// ✅ Hàm gửi log lên MySQL
void logToDatabase(String device, String action, String status)
{
  HTTPClient http;
  String url = String(serverName) + "?device_name=" +
               urlencode(device) + "&activity=" +
               urlencode(action) + "&status=" +
               urlencode(status);

  Serial.print("[LOG] Gửi dữ liệu lên MySQL: ");
  Serial.println(url);

  http.begin(url);
  int httpResponseCode = http.GET();
  Serial.print("[LOG] HTTP Response Code: ");
  Serial.println(httpResponseCode);
  http.end();
}

void handleCors(AsyncWebServerRequest *request)
{
  AsyncWebServerResponse *response = request->beginResponse(204);
  response->addHeader("Access-Control-Allow-Origin", "*");
  response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  response->addHeader("Access-Control-Allow-Headers", "*");
  request->send(response);
}
void updateRelayState(int index, bool newState)
{
  devices[index].state = newState;
  digitalWrite(devices[index].relayPin, newState ? HIGH : LOW);

  // ✅ Gửi dữ liệu qua WebSocket
  String json = "{\"device\":\"" + String(devices[index].api) + "\", \"status\":\"" + (newState ? "ON" : "OFF") + "\"}";
  notifyClients(json);

  Serial.printf("[UPDATE] %s -> %s | WebSocket: %s\n", devices[index].name, newState ? "ON" : "OFF", json.c_str());
}

void setup()
{
  Serial.begin(115200);

  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
  {
    Serial.println("[WiFi] Lỗi cấu hình IP tĩnh!");
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n[WiFi] Kết nối thành công!");
  Serial.print("[WiFi] Địa chỉ IP của ESP32: ");
  Serial.println(WiFi.localIP());

  // ✅ Cấu hình GPIO
  for (int i = 0; i < 6; i++)
  {
    pinMode(devices[i].relayPin, OUTPUT);
    digitalWrite(devices[i].relayPin, LOW);
    pinMode(devices[i].touchPin, INPUT);
  }

  // ✅ Cho phép CORS
  server.on("/status", HTTP_OPTIONS, handleCors);
  for (int i = 0; i < 6; i++)
  {
    server.on(("/toggle/" + String(devices[i].api)).c_str(), HTTP_OPTIONS, handleCors);
  }

  // ✅ API lấy trạng thái hiện tại
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        StaticJsonDocument<300> jsonDoc;
        for (int i = 0; i < 6; i++) {
            jsonDoc[devices[i].api] = devices[i].state ? "ON" : "OFF";
        }
        String jsonResponse;
        serializeJson(jsonDoc, jsonResponse);

        Serial.println("[DEBUG] JSON Response: " + jsonResponse);
        
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", jsonResponse);
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response); });

  // ✅ API điều khiển từ Web
  for (int i = 0; i < 6; i++)
  {
    server.on(("/toggle/" + String(devices[i].api)).c_str(), HTTP_GET, [i](AsyncWebServerRequest *request)
              {
            devices[i].state = !devices[i].state;
            digitalWrite(devices[i].relayPin, devices[i].state ? HIGH : LOW);
            logToDatabase(devices[i].name, "Web Control", devices[i].state ? "ON" : "OFF");

            Serial.printf("[TOGGLE] %s -> %s\n", devices[i].name, devices[i].state ? "ON" : "OFF");
            
            AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", String(devices[i].name) + " toggled");
            response->addHeader("Access-Control-Allow-Origin", "*");
            request->send(response); });
  }

  server.begin();
}

void loop()
{
  for (int i = 0; i < 6; i++)
  {
    bool touchState = digitalRead(devices[i].touchPin);

    if (touchState == HIGH && lastTouchState[i] == LOW)
    {
      devices[i].state = !devices[i].state;
      digitalWrite(devices[i].relayPin, devices[i].state ? HIGH : LOW);
      logToDatabase(devices[i].name, "Touch Sensor", devices[i].state ? "ON" : "OFF");

      // ✅ Gửi WebSocket thông báo trạng thái thay đổi
      String json = "{\"device\":\"" + String(devices[i].api) + "\", \"status\":\"" + (devices[i].state ? "ON" : "OFF") + "\"}";
      ws.textAll(json);

      Serial.printf("[TOUCH] %s đã thay đổi trạng thái!\n", devices[i].name);
    }
    lastTouchState[i] = touchState;
  }

  delay(300);
}