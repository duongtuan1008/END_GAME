#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <AccelStepper.h>
#include <Wire.h>
#include <SPIFFS.h>
#include <Keypad.h>
#include <EEPROM.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char *ssid = "mypc";
const char *password_wifi = "11111111";

const char *server_rfid_url = "http://192.168.137.88/api/rfid.php";
const char *server_pass_url = "http://192.168.137.88/api/save_password.php";
String apiKeyValue = "tPmAT5Ab3j7F9";

bool screenUpdated = false;
unsigned char id = 0;
unsigned char id_rf = 0;
unsigned char index_t = 0;
unsigned char error_in = 0;
unsigned char in_num = 0, error_pass = 0, isMode = 0;
byte firstScanTag[4] = {0}; // Biến lưu dữ liệu UID của thẻ quét lần 1

// 🛠 Keypad 4x4
const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
byte rowPins[4] = {13, 12, 14, 27}; // ✅ Hàng - Sử dụng GPIO hợp lệ
byte colPins[4] = {16, 17, 4, 15};  // ✅ Cột - Đảm bảo GPIO hợp lệ

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// 🛠 Stepper Motor 28BYJ-48 (ULN2003)
#define IN1 32
#define IN2 33
#define IN3 25
#define IN4 26
AccelStepper stepper(AccelStepper::HALF4WIRE, IN1, IN3, IN2, IN4);
const byte RFID_SIZE = 4;
int addr = 0;
char password[6] = "11111";
char pass_def[6] = "12395";
char mode_changePass[6] = "*#01#";
char mode_resetPass[6] = "*#02#";
char mode_hardReset[6] = "*#03#";
char mode_addRFID[6] = "*101#";
char mode_delRFID[6] = "*102#";
char mode_delAllRFID[6] = "*103#";
char data_input[6];
char new_pass1[6];
char new_pass2[6];
// 🛠 SPI cho GC9A01(VSPI)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define SCREEN_ADDRESS 0x3C // Địa chỉ I2C mặc định của OLED
// 📌 Khai báo địa chỉ I2C của OLED (thường là 0x3C)
#define OLED_ADDRESS 0x3C

// 📌 Khởi tạo màn hình OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// 🛠 SPI cho RFID (HSPI)
// ⚡ Chuyển Module RFID RC522 sang HSPI
#define SS_PIN 5
#define RST_PIN 2
MFRC522 rfid(SS_PIN, RST_PIN); // Chân SS và RST của RFID

MFRC522::MIFARE_Key key;
byte nuidPICC[4];
typedef enum
{
  MODE_ID_RFID_ADD,
  MODE_ID_RFID_FIRST,
  MODE_ID_RFID_SECOND,
} MODE_ID_RFID_E;

// unsigned char MODE = MODE_ID_FINGER_ADD; // Mode = 3
unsigned char MODE_RFID = MODE_ID_RFID_ADD;
void writeEpprom(char data[])
{
  for (unsigned char i = 0; i < 5; i++)
  {
    EEPROM.write(i, data[i]);
  }
  EEPROM.commit();
}

// 📌 Đọc mật khẩu từ EEPROM
void readEpprom()
{
  for (unsigned char i = 0; i < 5; i++)
  {
    password[i] = EEPROM.read(i);
  }
}

// 📌 Xóa dữ liệu nhập
void clear_data_input() // xoa gia tri nhap vao hien tai
{
  int i = 0;
  for (i = 0; i < 6; i++)
  {
    data_input[i] = '\0';
  }
}
void displayMessage(String message, uint16_t color)
{
  display.clearDisplay();
  display.setTextSize(1);              // OLED sử dụng setTextSize(1) là đủ
  display.setTextColor(SSD1306_WHITE); // OLED chỉ có đơn sắc
  display.setCursor(0, 10);
  display.println(message);
  display.display(); // Cập nhật màn hình
}

bool isBufferdata(char data[])
{
  for (unsigned char i = 0; i < 5; i++)
  {
    if (data[i] == '\0')
    {
      return false;
    }
  }
  return true;
}

// 📌 So sánh hai buffer
bool compareData(char data1[], char data2[]) // Kiem tra 2 cai buffer co giong nhau hay khong
{
  unsigned char i = 0;
  for (i = 0; i < 5; i++)
  {
    if (data1[i] != data2[i])
    {
      return false;
    }
  }
  return true;
}

// 📌 Gán buffer từ data2 sang data1
void insertData(char data1[], char data2[])
{
  for (unsigned char i = 0; i < 5; i++)
  {
    data1[i] = data2[i];
  }
}
void displayPassword()
{
  display.clearDisplay();              // Xóa màn hình trước khi vẽ nội dung mới
  display.setTextSize(1);              // OLED nhỏ, text size = 1 là đủ
  display.setTextColor(SSD1306_WHITE); // Màu trắng trên nền đen
  display.setCursor(10, 10);
  display.print("ENTER PASSWORD:");

  // Hiển thị dấu '*' đại diện cho mật khẩu đã nhập
  display.setCursor(30, 30);
  for (byte i = 0; i < in_num; i++)
  {
    display.print("*");
  }

  display.display(); // Cập nhật màn hình
}

void openDoor()
{
  // Hiển thị thông báo mở cửa trên OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.print("OPENING DOOR...");
  display.display();

  // Cấu hình Stepper Motor
  stepper.setMaxSpeed(500);
  stepper.setAcceleration(300);
  stepper.move(4096); // Quay 360 độ để mở cửa
  while (stepper.distanceToGo() != 0)
    stepper.run();

  delay(3000); // Giữ cửa mở trong 3 giây

  // Hiển thị thông báo đóng cửa trên OLED
  display.clearDisplay();
  display.setCursor(10, 10);
  display.print("DOOR CLOSING...");
  display.display();

  stepper.move(-4096); // Quay ngược lại để đóng cửa
  while (stepper.distanceToGo() != 0)
    stepper.run();

  index_t = 0;
}

// 🔑 Nhận dữ liệu từ bàn phím và hiển thị trên TFT
void getData()
{
  char key = keypad.getKey(); // Đọc giá trị bàn phím
  if (key)
  {
    Serial.print("Phím bấm: ");
    Serial.println(key);

    if (in_num < 5)
    {
      data_input[in_num] = key; // Lưu vào mảng nhập liệu

      // Hiển thị số vừa nhập trước
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(20 + in_num * 12, 20); // Dịch từng ký tự sang phải
      display.print(key);
      display.display();

      delay(500); // Chờ 500ms trước khi thay bằng *

      // Cập nhật lại chỉ **số đó** thành dấu *
      display.setCursor(20 + in_num * 12, 20);
      display.print("*");
      display.display();

      in_num++;

      if (in_num == 5)
      { // Khi nhập đủ 5 số
        Serial.print("Mật khẩu nhập: ");
        Serial.println(data_input);
        in_num = 0; // Reset bộ đếm
      }
    }
  }
}

void checkPass()
{
  getData();

  if (isBufferdata(data_input))
  {
    Serial.print("🔎 Data Input: ");
    Serial.println(data_input);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 10);

    if (compareData(data_input, password)) // Nếu đúng mật khẩu
    {
      display.print("ACCESS GRANTED");
      index_t = 3;
      Serial.println("🔄 index_t SET TO 3 - READY TO OPEN DOOR");
    }
    else if (compareData(data_input, mode_changePass)) // Đổi mật khẩu
    {
      display.print("CHANGE PASS MODE");
      index_t = 1;
    }
    else if (compareData(data_input, mode_resetPass)) // Reset mật khẩu
    {
      display.print("RESET PASS");
      index_t = 2;
    }
    else if (compareData(data_input, mode_hardReset)) // Hard Reset
    {
      display.print("HARD RESET");
      writeEpprom(pass_def);
      insertData(password, pass_def);
      delay(2000);

      display.clearDisplay();
      display.setCursor(10, 10);
      display.print("SYSTEM RESET");
      index_t = 0;
    }
    else if (compareData(data_input, mode_addRFID)) // Thêm thẻ RFID
    {
      display.setTextSize(1);
      display.print("ADD RFID MODE");
      index_t = 8;
    }
    else if (compareData(data_input, mode_delRFID)) // Xóa RFID
    {
      display.setTextSize(1);
      display.print("DELETE RFID");
      index_t = 9;
    }
    else if (compareData(data_input, mode_delAllRFID)) // Xóa tất cả RFID
    {
      display.setTextSize(1);
      display.print("DELETE ALL RFID");
      index_t = 10;
    }
    else // Mật khẩu sai
    {
      if (error_pass >= 2)
      {
        display.setTextSize(1);
        display.print("ACCESS BLOCKED");
        delay(3000);
        index_t = 4;
      }
      else
      {
        display.setTextSize(1);
        display.print("WRONG PASSWORD");
        error_pass++;
        delay(1000);
        display.setTextSize(1);
        display.clearDisplay();
        display.setCursor(10, 10);
        display.print("ENTER PASSWORD");
      }
    }

    display.display(); // Cập nhật màn hình OLED

    // Chỉ xóa dữ liệu nhập khi đã kiểm tra xong
    clear_data_input();
  }
}

// 🔒 Xử lý nhập sai 3 lần - khóa hệ thống 30 giây
void error()
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(20, 10);
  display.println("WRONG 3 TIMES");
  display.display(); // Cập nhật màn hình OLED

  delay(2000);

  display.clearDisplay();
  display.setCursor(20, 10);
  display.println("WAIT 30 SEC");
  display.display(); // Cập nhật màn hình OLED

  unsigned char i = 30;
  while (i > 0)
  {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(10, 20);
    display.println("WAIT:");

    display.setCursor(50, 50);
    display.setTextSize(3);
    display.print(i);
    display.print("s");

    display.display(); // Cập nhật màn hình OLED
    i--;
    delay(1000);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(20, 10);
  display.println("ENTER PASSWORD");
  display.display(); // Cập nhật màn hình OLED

  index_t = 0;
}

void changePass() // Thay đổi mật khẩu
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(10, 10);
  display.println("CHANGE PASSWORD");
  display.display();
  delay(3000);

  display.clearDisplay();
  display.setCursor(10, 10);
  display.println("ENTER NEW PASS");
  display.display();

  while (1)
  {
    getData();
    if (isBufferdata(data_input))
    {
      insertData(new_pass1, data_input);
      clear_data_input();
      break;
    }
  }

  display.clearDisplay();
  display.setCursor(10, 10);
  display.println("ENTER AGAIN");
  display.display();

  while (1)
  {
    getData();
    if (isBufferdata(data_input))
    {
      insertData(new_pass2, data_input);
      clear_data_input();
      break;
    }
  }

  delay(1000);
  if (compareData(new_pass1, new_pass2)) // Nếu mật khẩu trùng khớp
  {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 10);
    display.println("PASSWORD SAVED");
    display.display();

    delay(1000);
    writeEpprom(new_pass2);
    insertData(password, new_pass2);

    display.clearDisplay();
    display.display();
    index_t = 0;
  }
  else // Nếu mật khẩu không trùng
  {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 10);
    display.println("MISMATCHED!");
    display.display();

    delay(1000);
    display.clearDisplay();
    display.display();
    index_t = 0;
  }
}

void resetPass()
{
  unsigned char choise = 0;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(10, 10);
  display.println("RESET PASSWORD");
  display.display();

  getData();
  if (isBufferdata(data_input))
  {
    if (compareData(data_input, password))
    {
      display.clearDisplay();
      clear_data_input();

      while (1)
      {
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(2);
        display.setCursor(10, 10);
        display.println("RESET PASSWORD");

        if (choise == 0)
        {
          display.setTextColor(SSD1306_WHITE);
          display.setCursor(20, 40);
          display.println("> YES");

          display.setTextColor(SSD1306_WHITE);
          display.setCursor(20, 60);
          display.println("  NO");
        }
        else
        {
          display.setTextColor(SSD1306_WHITE);
          display.setCursor(20, 40);
          display.println("  YES");

          display.setTextColor(SSD1306_WHITE);
          display.setCursor(20, 60);
          display.println("> NO");
        }

        display.display();

        char key = keypad.getKey();
        if (key == '*')
        {
          choise = (choise == 1) ? 0 : 1;
        }

        if (key == '#' && choise == 0) // Chọn YES - Reset mật khẩu
        {
          display.clearDisplay();
          delay(1000);
          writeEpprom(pass_def);
          insertData(password, pass_def);

          display.setTextColor(SSD1306_WHITE);
          display.setCursor(10, 30);
          display.println("RESET DONE");
          display.display();

          delay(1000);
          display.clearDisplay();
          display.display();
          break;
        }

        if (key == '#' && choise == 1) // Chọn NO - Thoát
        {
          display.clearDisplay();
          display.display();
          break;
        }
      }
      index_t = 0;
    }
    else
    {
      index_t = 0;
      display.clearDisplay();
      display.display();
    }
  }
}

unsigned char numberInput()
{
  char number[5] = {0};
  char count_i = 0;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(10, 10);
  display.println("ENTER NUMBER:");
  display.display();

  while (count_i < 2)
  {
    char key = keypad.getKey();
    if (key && key != 'A' && key != 'B' && key != 'C' && key != 'D' && key != '*' && key != '#')
    {
      delay(100);

      // Hiển thị số nhập vào
      display.setCursor(30 + (count_i * 15), 40);
      display.setTextSize(3);
      display.setTextColor(SSD1306_WHITE);
      display.print(key);
      display.display();

      number[count_i] = key;
      count_i++;
    }
  }

  return (number[0] - '0') * 10 + (number[1] - '0'); // Chuyển chuỗi thành số nguyên
}

void checkEEPROM()
{
  Serial.println("🔍 Kiểm tra EEPROM lưu thẻ RFID...");

  for (int i = 10; i < 50; i += 4) // Duyệt qua từng thẻ RFID trong EEPROM
  {
    Serial.print("📌 ID ");
    Serial.print((i - 10) / 4 + 1); // Tính số thứ tự thẻ RFID
    Serial.print(": ");

    bool empty = true; // Biến kiểm tra nếu tất cả giá trị = 0

    for (int j = 0; j < 4; j++)
    {
      byte data = EEPROM.read(i + j);
      Serial.print(data, HEX);
      Serial.print(" ");

      if (data != 0xFF && data != 0x00) // Nếu có dữ liệu hợp lệ
      {
        empty = false;
      }
    }

    if (empty)
    {
      Serial.print(" -> (Trống)");
    }

    Serial.println();
  }
}

bool isAllowedRFIDTag(byte tag[])
{
  int count = 0;
  for (int i = 10; i < 512; i += 4)
  {
    Serial.print("EEPROM: ");
    for (int j = 0; j < 4; j++)
    {
      Serial.print(EEPROM.read(i + j), HEX);
      if (tag[j] == EEPROM.read(i + j))
      {
        count++;
      }
    }
    Serial.println();
    if (count == 4)
    {
      return true; // Thẻ đã tồn tại
    }
    count = 0;
  }
  return false;
}

void rfidCheck()
{
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
  {
    byte rfidTag[4];
    Serial.print("RFID TAG: ");
    for (byte i = 0; i < rfid.uid.size; i++)
    {
      rfidTag[i] = rfid.uid.uidByte[i];
      Serial.print(rfidTag[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(10, 10);

    if (isAllowedRFIDTag(rfidTag))
    {
      display.println("RFID ACCESS");
      display.setCursor(20, 40);
      display.println("GRANTED");
      index_t = 3;
    }
    else
    {
      if (error_pass == 2)
      {
        display.println("SYSTEM");
        display.setCursor(20, 40);
        display.println("LOCKED");
        index_t = 4;
      }
      else
      {
        Serial.println("Error");
        display.println("WRONG RFID");
        error_pass++;
        delay(1000);
      }
    }

    display.display(); // Cập nhật màn hình OLED

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
}

void handleWrongRFID()
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(10, 10);

  if (error_pass == 2)
  {
    display.println("SYSTEM");
    display.setCursor(20, 40);
    display.println("LOCKED");
    index_t = 4;
  }
  else
  {
    Serial.println("Error: Wrong RFID");
    display.println("WRONG RFID");
    error_pass++;
  }

  display.display(); // Cập nhật màn hình OLED
  delay(1000);
}

// Khai báo biến toàn cục để lưu thẻ quét lần 1

void addRFID()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(25, 0);
  display.println("ADD NEW RFID");
  display.display();

  Serial.println("📌 ADD_RFID MODE");

  switch (MODE_RFID)
  {
  case MODE_ID_RFID_ADD:
  {
    Serial.println("📌 Nhập ID thẻ...");

    display.setCursor(0, 20);
    display.println("Input ID:");
    display.display();

    id_rf = numberInput();
    Serial.println(id_rf);

    if (id_rf == 0)
    {
      display.clearDisplay();
      display.setCursor(20, 30);
      display.println("ID ERROR");
      display.display();
      delay(2000);
    }
    else
    {
      MODE_RFID = MODE_ID_RFID_FIRST;
    }
  }
  break;

  case MODE_ID_RFID_FIRST:
  {
    Serial.println("🔄 Chờ quét thẻ lần 1...");
    display.clearDisplay();
    display.setCursor(20, 20);
    display.println("Put RFID");
    display.display();

    unsigned long timeout = millis() + 10000;
    while (!rfid.PICC_IsNewCardPresent() && millis() < timeout)
    {
      Serial.println("⏳ Đang quét... chưa thấy thẻ.");
      delay(500);
    }

    if (millis() >= timeout)
    {
      Serial.println("⚠ Không phát hiện thẻ!");
      display.clearDisplay();
      display.setCursor(20, 30);
      display.println("NO CARD DETECTED");
      display.display();
      delay(2000);
      MODE_RFID = MODE_ID_RFID_ADD;
      return;
    }

    if (rfid.PICC_ReadCardSerial())
    {
      Serial.println("✅ Đã phát hiện thẻ! Đọc UID...");
      Serial.print("🆔 RFID TAG: ");

      for (byte i = 0; i < 4; i++)
      {
        firstScanTag[i] = rfid.uid.uidByte[i];
        Serial.print(firstScanTag[i], HEX);
        Serial.print(" ");
      }
      Serial.println();

      if (isAllowedRFIDTag(firstScanTag))
      {
        display.clearDisplay();
        display.setCursor(10, 30);
        display.println("RFID EXISTS");
        display.display();
        delay(2000);
        MODE_RFID = MODE_ID_RFID_ADD;
      }
      else
      {
        MODE_RFID = MODE_ID_RFID_SECOND;
      }
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
  break;

  case MODE_ID_RFID_SECOND:
  {
    Serial.println("🔄 Chờ quét thẻ lần 2...");
    display.clearDisplay();
    display.setCursor(10, 20);
    display.println("Put Again");
    display.display();
    delay(1000);

    unsigned long timeout = millis() + 10000;
    while (!rfid.PICC_IsNewCardPresent() && millis() < timeout)
    {
      Serial.println("⏳ Đang quét lần 2...");
      delay(500);
    }

    if (millis() >= timeout)
    {
      Serial.println("⚠ Không phát hiện thẻ lần 2!");
      display.clearDisplay();
      display.setCursor(20, 30);
      display.println("NO CARD DETECTED");
      display.display();
      delay(2000);
      MODE_RFID = MODE_ID_RFID_ADD;
      return;
    }

    if (rfid.PICC_ReadCardSerial())
    {
      byte secondScanTag[4];
      Serial.print("🔍 RFID TAG (Lần 2): ");
      for (byte i = 0; i < 4; i++)
      {
        secondScanTag[i] = rfid.uid.uidByte[i];
        Serial.print(secondScanTag[i], HEX);
        Serial.print(" ");
      }
      Serial.println();

      if (memcmp(firstScanTag, secondScanTag, 4) != 0)
      {
        Serial.println("⚠️ Thẻ không khớp!");
        display.clearDisplay();
        display.setCursor(10, 30);
        display.println("MISMATCHED RFID");
        display.display();
        delay(2000);
        MODE_RFID = MODE_ID_RFID_ADD;
        return;
      }

      // ✅ Lưu vào EEPROM
      Serial.println("💾 Ghi EEPROM...");
      for (int i = 0; i < 4; i++)
      {
        EEPROM.write(10 + (id_rf - 1) * 4 + i, secondScanTag[i]);
      }
      EEPROM.commit();

      // ✅ Gửi dữ liệu lên server bằng GET
      if (WiFi.status() == WL_CONNECTED)
      {
        HTTPClient http;

        String url = String(server_rfid_url) + "?action=add"; // SỬA Ở ĐÂY
        url += "&id=" + String(id_rf);
        url += "&uid1=" + String(secondScanTag[0], HEX);
        url += "&uid2=" + String(secondScanTag[1], HEX);
        url += "&uid3=" + String(secondScanTag[2], HEX);
        url += "&uid4=" + String(secondScanTag[3], HEX);

        url.toUpperCase(); // Tùy chọn: cho UID in hoa
        printf("%s\n", url.c_str());
        Serial.println("🔗 Gửi URL: " + url); // BẮT BUỘC để kiểm tra

        http.begin(url);
        int code = http.GET();
        String response = http.getString();

        Serial.print("📡 Mã phản hồi: ");
        Serial.println(code);
        Serial.println("📤 Server Response: " + response);
        http.end();
      }
      // ✅ Hiển thị thông báo thành công
      display.clearDisplay();
      display.setCursor(10, 30);
      display.println("Add RFID Done");
      display.display();
      delay(2000);

      // ✅ QUAN TRỌNG: Reset trạng thái
      MODE_RFID = MODE_ID_RFID_ADD;
      id_rf = 0;
      Serial.println("✅ ADD_OUT");
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
  break;
  }
}

void delRFID()
{
  char buffDisp[20];

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(30, 0);
  display.println("DELETE RFID");
  display.display();

  Serial.println("📌 DEL_IN");

  display.setCursor(10, 20);
  display.println("Input ID:");
  display.display();

  id_rf = numberInput(); // Nhận ID từ bàn phím
  if (id_rf == 0)        // ID #0 không hợp lệ
  {
    display.clearDisplay();
    display.setCursor(30, 30);
    display.setTextColor(SSD1306_WHITE);
    display.println("ID ERROR");
    display.display();

    delay(2000);
  }
  else
  {
    for (int i = 0; i < 4; i++)
    {
      EEPROM.write(10 + (id_rf - 1) * 4 + i, '\0'); // Xóa dữ liệu trong EEPROM
    }
    EEPROM.commit();
    if (WiFi.status() == WL_CONNECTED)
    {
      HTTPClient http;
      http.begin(server_rfid_url);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      String postData = "action=delete&id=" + String(id_rf);
      int httpCode = http.POST(postData);
      String response = http.getString();

      Serial.println("🗑️ Delete RFID SQL: " + response);
      http.end();
    }
    sprintf(buffDisp, "Clear ID: %d Done", id_rf);

    display.clearDisplay();
    display.setCursor(10, 30);
    display.setTextColor(SSD1306_WHITE);
    display.println(buffDisp);
    display.display();

    Serial.println("✅ DEL_OUT");
    delay(2000);
    display.clearDisplay();
    index_t = 0;
  }
}

void delAllRFID()
{
  char key = keypad.getKey();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 0);
  display.println("CLEAR ALL RFID?");
  display.display();

  if (key == '*')
  {
    isMode = 0; // Chọn YES
  }
  if (key == '#')
  {
    isMode = 1; // Chọn NO
  }

  if (isMode == 0) // Hiển thị lựa chọn YES
  {
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 20);
    display.println("> YES");

    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 30);
    display.println("  NO");
  }
  if (isMode == 1) // Hiển thị lựa chọn NO
  {
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 20);
    display.println("  YES");

    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 30);
    display.println("> NO");
  }
  display.display(); // Cập nhật màn hình OLED

  if (key == '0' && isMode == 0) // Xác nhận xóa toàn bộ RFID
  {
    for (int i = 10; i < 512; i++)
    {
      EEPROM.write(i, '\0');
    }
    EEPROM.commit();
    // Gửi lệnh xóa toàn bộ lên server
    if (WiFi.status() == WL_CONNECTED)
    {
      HTTPClient http;
      http.begin(server_rfid_url);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      String postData = "action=delete_all";
      int httpCode = http.POST(postData);
      String response = http.getString();

      Serial.println("🧹 Delete ALL RFID SQL: " + response);
      http.end();
    }
    Serial.println("✅ Tất cả RFID đã bị xóa!");

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(15, 20);
    display.println("CLEAR DONE");
    display.display();

    delay(2000);
    display.clearDisplay();
    index_t = 0;
  }

  if (key == '0' && isMode == 1) // Thoát mà không xóa
  {
    display.clearDisplay();
    index_t = 0;
  }
}
void syncFromServer()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("⚠ Không có WiFi, bỏ qua sync.");
    return;
  }

  HTTPClient http;
  http.begin(server_rfid_url); // Địa chỉ server PHP
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpCode = http.POST("action=sync");

  if (httpCode == 200)
  {
    String payload = http.getString();
    Serial.println("📥 JSON từ server:");
    Serial.println(payload);

    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
      Serial.println("❌ Lỗi phân tích JSON");
      return;
    }

    // 🔐 Đồng bộ mật khẩu
    const char *pw_server = doc["password"];
    bool pass_diff = false;
    for (int i = 0; i < 5; i++)
    {
      if (EEPROM.read(i) != pw_server[i])
      {
        pass_diff = true;
        break;
      }
    }

    if (pass_diff)
    {
      Serial.println("🔁 Mật khẩu khác, cập nhật EEPROM");
      for (int i = 0; i < 5; i++)
      {
        EEPROM.write(i, pw_server[i]);
        password[i] = pw_server[i];
      }
    }
    else
    {
      Serial.println("✅ Mật khẩu trùng, giữ nguyên");
    }

    // 🔁 Đồng bộ RFID
    JsonArray list = doc["rfid_list"];
    for (JsonObject rfid : list)
    {
      int id = atoi(rfid["id"]);
      int addr = 10 + (id - 1) * 4;
      byte uid_server[4];

      for (int i = 0; i < 4; i++)
      {
        String key = "uid" + String(i + 1);
        const char *hexStr = rfid[key]; // Chuỗi hex như "A3"
        uid_server[i] = (byte)strtol(hexStr, NULL, 16);

        Serial.print("📦 UID[");
        Serial.print(i);
        Serial.print("] = ");
        Serial.println(uid_server[i], HEX);
      }

      bool rfid_diff = false;
      for (int i = 0; i < 4; i++)
      {
        if (EEPROM.read(addr + i) != uid_server[i])
        {
          rfid_diff = true;
          break;
        }
      }

      if (rfid_diff)
      {
        Serial.print("🔁 Ghi lại RFID ID ");
        Serial.println(id);
        for (int i = 0; i < 4; i++)
        {
          EEPROM.write(addr + i, uid_server[i]);
          Serial.print("📝 EEPROM[");
          Serial.print(addr + i);
          Serial.print("] = ");
          Serial.println(uid_server[i], HEX);
        }
      }
      else
      {
        Serial.print("✅ RFID ID ");
        Serial.print(id);
        Serial.println(" trùng khớp");
      }
    }

    EEPROM.commit();
    Serial.println("✅ Đồng bộ hoàn tất!");
    checkEEPROM(); // Gợi ý: In lại EEPROM sau khi sync
  }
  else
  {
    Serial.print("❌ Kết nối thất bại, mã lỗi: ");
    Serial.println(httpCode);
  }

  http.end();
}

void setup()
{
  Serial.begin(115200);

  // 🔄 Kết nối WiFi
  WiFi.begin(ssid, password_wifi);
  Serial.print("🔄 Đang kết nối WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi đã kết nối!");
  Serial.print("📡 Địa chỉ IP: ");
  Serial.println(WiFi.localIP());
  syncFromServer(); // <=== GỌI ĐỒNG BỘ DỮ LIỆU TỪ SQL
  EEPROM.begin(512);
  EEPROM.commit();
  checkEEPROM();
  // Khởi tạo Stepper Motor
  stepper.setMaxSpeed(500);     // Tốc độ tối đa (step/s)
  stepper.setAcceleration(300); // Gia tốc

  // Khởi tạo SPI và RFID
  SPI.begin();
  rfid.PCD_Init();

  // Khởi tạo màn hình OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("⚠️ Lỗi khởi tạo OLED!"));
    for (;;)
      ; // Dừng chương trình nếu không tìm thấy màn hình
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.println("SYSTEM INIT...");
  display.display();

  readEpprom(); // Đọc mật khẩu từ EEPROM
  delay(2000);

  // Xóa màn hình sau khi hiển thị thông báo khởi động
  display.clearDisplay();
  display.display();

  Serial.print("🔑 PASSWORD: ");
  Serial.println(password);

  // Nếu mật khẩu EEPROM chưa được thiết lập, ghi lại mật khẩu mặc định
  if (password[0] == 0xFF)
  {
    writeEpprom(pass_def);
    insertData(password, pass_def);
    Serial.print("🔑 PASSWORD (Mới): ");
    Serial.println(password);
  }
}

void loop()
{
  display.clearDisplay(); // Xóa màn hình
  display.setTextSize(1); // Cỡ chữ nhỏ hơn do OLED có độ phân giải cao hơn LCD
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  display.println("Enter Password:");
  display.display(); // Cập nhật nội dung lên OLED
  checkPass();       // Kiểm tra mật khẩu nhập vào
  rfidCheck();       // Kiểm tra RFID

  // Các trạng thái khác của hệ thống
  while (index_t == 1)
  {
    changePass();
  }

  while (index_t == 2)
  {
    resetPass();
  }

  while (index_t == 3)
  {
    openDoor();
    error_pass = 0;
  }

  while (index_t == 4)
  {
    error();
    error_pass = 0;
  }

  while (index_t == 8)
  {
    addRFID();
  }

  while (index_t == 9)
  {
    delRFID();
  }

  while (index_t == 10)
  {
    delAllRFID();
  }
}
