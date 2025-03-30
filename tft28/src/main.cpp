#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <TFT_eSPI.h>

// WiFi Config
const char *ssid = "mypc";
const char *password = "11111111";
const char *api_url = "http://192.168.137.88/api/tft_sensor.php";

// Khởi tạo màn hình TFT
TFT_eSPI tft = TFT_eSPI();
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Dữ liệu lưu trữ vẽ biểu đồ
#define MAX_POINTS 10
float tempData[MAX_POINTS] = {0};
float humidData[MAX_POINTS] = {0};

// Khai báo trước các hàm
void drawUI();
void displayData(float temp, float humid, int light, int gas, int flame);
void updateGraph();

void setup()
{
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    tft.init();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);

    Serial.print("Đang kết nối WiFi...");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi đã kết nối!");

    drawUI(); // Vẽ giao diện lần đầu
}

void loop()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        http.begin(api_url);
        int httpResponseCode = http.GET();

        if (httpResponseCode == 200)
        {
            String payload = http.getString();
            Serial.println(payload);

            // Parse JSON
            DynamicJsonDocument doc(2048);
            deserializeJson(doc, payload);
            JsonArray sensors = doc["sensors"];

            // Lấy 10 giá trị mới nhất
            int count = sensors.size();
            for (int i = 0; i < MAX_POINTS; i++)
            {
                int index = count - MAX_POINTS + i;
                if (index >= 0)
                {
                    JsonObject data = sensors[index];
                    tempData[i] = data["temperature"];
                    humidData[i] = data["humidity"];
                }
            }

            // Hiển thị dữ liệu cuối cùng
            JsonObject latest = sensors[count - 1];
            float temperature = latest["temperature"];
            float humidity = latest["humidity"];
            int light = latest["light"];
            int gas = latest["gas"];
            int flame = latest["flame_sensor"];

            // Cập nhật hiển thị
            displayData(temperature, humidity, light, gas, flame);
            updateGraph();
        }
        else
        {
            Serial.println("Lỗi khi lấy dữ liệu từ API!");
        }
        http.end();
    }

    delay(5000); // Cập nhật mỗi 5 giây
}

// Vẽ giao diện UI với bố cục chuẩn 320x240
void drawUI()
{
    tft.fillScreen(TFT_BLACK);

    // Thanh trạng thái trên cùng
    tft.fillRect(0, 0, SCREEN_WIDTH, 30, TFT_NAVY);
    tft.setTextFont(2);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(40, 10);
    tft.print("Light | Gas | Flame");

    // // Khung nhiệt độ
    // tft.fillRect(10, 40, 140, 80, TFT_DARKGREY);
    // tft.setTextFont(4);
    // tft.setTextColor(TFT_ORANGE);
    // tft.setCursor(20, 50);
    // tft.print("Temp");

    // // Khung độ ẩm
    // tft.fillRect(170, 40, 140, 80, TFT_DARKGREY);
    // tft.setTextFont(4);
    // tft.setTextColor(TFT_CYAN);
    // tft.setCursor(180, 50);
    // tft.print("Humi");

    // Biểu đồ
    tft.fillRect(10, 130, SCREEN_WIDTH - 20, 100, TFT_BLACK);
    tft.drawRect(10, 130, SCREEN_WIDTH - 20, 100, TFT_WHITE);
}

// Hiển thị dữ liệu cảm biến với font chữ cân đối
void displayData(float temp, float humid, int light, int gas, int flame)
{
    // **Kích thước & vị trí hai ô**
    int boxWidth = 140;  // Chiều rộng ô
    int boxHeight = 80;  // Chiều cao ô
    int boxSpacing = 20; // Khoảng cách giữa hai ô

    int tempBoxX = 10;                               // Vị trí X của ô nhiệt độ
    int humiBoxX = tempBoxX + boxWidth + boxSpacing; // Vị trí X của ô độ ẩm
    int boxY = 40;                                   // Vị trí Y của cả hai ô

    // **Vẽ ô nhiệt độ**
    tft.fillRect(tempBoxX, boxY, boxWidth, boxHeight, TFT_DARKGREY);
    tft.drawRect(tempBoxX, boxY, boxWidth, boxHeight, TFT_WHITE);

    tft.setTextFont(4);
    tft.setTextColor(TFT_ORANGE);
    tft.setCursor(tempBoxX + 20, boxY + 10);
    tft.print("Temp");

    // **Hiển thị giá trị nhiệt độ**
    tft.fillRect(tempBoxX + 10, boxY + 40, boxWidth - 20, 30, TFT_DARKGREY);
    tft.setTextFont(4);
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(tempBoxX + 30, boxY + 45);
    tft.print(temp);
    tft.print(" C");

    // **Vẽ ô độ ẩm**
    tft.fillRect(humiBoxX, boxY, boxWidth, boxHeight, TFT_DARKGREY);
    tft.drawRect(humiBoxX, boxY, boxWidth, boxHeight, TFT_WHITE);

    tft.setTextFont(4);
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(humiBoxX + 20, boxY + 10);
    tft.print("Humi");

    // **Hiển thị giá trị độ ẩm**
    tft.fillRect(humiBoxX + 10, boxY + 40, boxWidth - 20, 30, TFT_DARKGREY);
    tft.setTextFont(4);
    tft.setTextColor(TFT_BLUE);
    tft.setCursor(humiBoxX + 30, boxY + 45);
    tft.print(humid);
    tft.print(" %");

    // **Thanh trạng thái trên cùng (ánh sáng, gas, lửa)**
    tft.fillRect(10, 5, SCREEN_WIDTH - 20, 25, TFT_NAVY);
    tft.setTextFont(2);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(20, 10);

    // **Hiển thị ánh sáng**
    tft.print("☀ Light: ");
    tft.setTextColor(TFT_YELLOW);
    tft.print(light);

    // **Hiển thị khí gas**
    tft.setTextColor(TFT_WHITE);
    tft.print(" | 🌫 Gas: ");
    tft.setTextColor(TFT_RED);
    tft.print(gas);

    // **Hiển thị cảnh báo lửa**
    tft.setTextColor(TFT_WHITE);
    tft.print(" | 🔥 Flame: ");
    if (flame == 1)
    {
        tft.setTextColor(TFT_RED);
        tft.print("ALERT!");
    }
    else
    {
        tft.setTextColor(TFT_GREEN);
        tft.print("SAFE");
    }
}

// Vẽ biểu đồ nhiệt độ & độ ẩm với bố cục giống ảnh
void updateGraph()
{
    int graphX = 20, graphY = 140, graphWidth = 280, graphHeight = 80; // Kích thước biểu đồ

    // Xóa & vẽ khung biểu đồ
    tft.fillRect(graphX, graphY, graphWidth, graphHeight, TFT_BLACK);
    tft.drawRect(graphX, graphY, graphWidth, graphHeight, TFT_WHITE);

    // Tìm giá trị min/max động với giới hạn rộng hơn để tránh bị tràn
    float minTemp = 100, maxTemp = -100, minHumid = 100, maxHumid = -100;
    for (int i = 0; i < MAX_POINTS; i++)
    {
        if (tempData[i] < minTemp)
            minTemp = tempData[i];
        if (tempData[i] > maxTemp)
            maxTemp = tempData[i];
        if (humidData[i] < minHumid)
            minHumid = humidData[i];
        if (humidData[i] > maxHumid)
            maxHumid = humidData[i];
    }

    // Mở rộng giới hạn nếu dữ liệu dao động quá nhỏ
    minTemp -= 3;
    maxTemp += 3;
    minHumid -= 5;
    maxHumid += 5;

    // Vẽ lưới ngang (màu xám nhạt)
    for (int i = 0; i <= 4; i++)
    {
        int y = graphY + (i * graphHeight / 5);
        tft.drawLine(graphX, y, graphX + graphWidth, y, TFT_DARKGREY);
    }

    // Vẽ lưới dọc (cho 10 giá trị)
    for (int i = 0; i < MAX_POINTS; i++)
    {
        int x = graphX + (i * (graphWidth / (MAX_POINTS - 1)));
        tft.drawLine(x, graphY, x, graphY + graphHeight, TFT_DARKGREY);
    }

    // Hiển thị giá trị trên trục Y (Màu xám nhạt)
    tft.setTextFont(2);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(graphX - 20, graphY);
    tft.print((int)maxTemp);
    tft.setCursor(graphX - 20, graphY + graphHeight - 10);
    tft.print((int)minTemp);

    tft.setCursor(graphX + graphWidth - 5, graphY);
    tft.print((int)maxHumid);
    tft.setCursor(graphX + graphWidth - 5, graphY + graphHeight - 10);
    tft.print((int)minHumid);

    // Hiển thị giá trị trên trục X (Màu trắng)
    for (int i = 0; i < MAX_POINTS; i++)
    {
        int x = graphX + (i * (graphWidth / (MAX_POINTS - 1))) - 5;
        tft.setCursor(x, graphY + graphHeight + 5);
        tft.print(i + 1);
    }

    // Làm mượt dữ liệu để đường không bị gấp khúc
    float smoothedTemp[MAX_POINTS] = {0};
    float smoothedHumid[MAX_POINTS] = {0};

    for (int i = 1; i < MAX_POINTS - 1; i++)
    {
        smoothedTemp[i] = (tempData[i - 1] + tempData[i] + tempData[i + 1]) / 3.0;
        smoothedHumid[i] = (humidData[i - 1] + humidData[i] + humidData[i + 1]) / 3.0;
    }

    // Vẽ dải màu gradient
    int prev_x = graphX;
    int prev_y_temp = graphY + graphHeight - map(smoothedTemp[0], minTemp, maxTemp, 0, graphHeight);
    int prev_y_humid = graphY + graphHeight - map(smoothedHumid[0], minHumid, maxHumid, 0, graphHeight);

    for (int i = 1; i < MAX_POINTS; i++)
    {
        int x = graphX + (i * (graphWidth / (MAX_POINTS - 1)));

        int y_temp = graphY + graphHeight - map(smoothedTemp[i], minTemp, maxTemp - 3, 0, graphHeight - 10);
        int y_humid = graphY + graphHeight - map(smoothedHumid[i], minHumid, maxHumid + 3, 0, graphHeight - 10);

        // Giới hạn tọa độ Y không vẽ ra ngoài vùng
        y_temp = constrain(y_temp, graphY, graphY + graphHeight);
        y_humid = constrain(y_humid, graphY, graphY + graphHeight);

        // 🌈 Vẽ dải màu gradient nhiệt độ 🌈
        for (int j = 0; j < 6; j++) // Tạo hiệu ứng nhạt dần 6 pixel phía dưới
        {
            uint16_t fadeColor = tft.color565(255, 255 - j * 40, 0); // Vàng nhạt dần
            tft.drawLine(prev_x, prev_y_temp + j, x, y_temp + j, fadeColor);
        }

        // 🌊 Vẽ dải màu gradient độ ẩm 🌊
        for (int j = 0; j < 6; j++) // Tạo hiệu ứng nhạt dần 6 pixel phía dưới
        {
            uint16_t fadeColor = tft.color565(0, 255 - j * 40, 255); // Xanh nhạt dần
            tft.drawLine(prev_x, prev_y_humid + j, x, y_humid + j, fadeColor);
        }

        // Vẽ đường nhiệt độ (màu vàng)
        tft.drawLine(prev_x, prev_y_temp, x, y_temp, TFT_YELLOW);

        // Vẽ đường độ ẩm (màu xanh)
        tft.drawLine(prev_x, prev_y_humid, x, y_humid, TFT_CYAN);

        prev_x = x;
        prev_y_temp = y_temp;
        prev_y_humid = y_humid;
    }
}
