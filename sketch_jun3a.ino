#include <ESP8266WiFi.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Cấu hình WiFi và ThingSpeak ---
String apiKey = "XXVLQW2ILG8W9AZ8"; // Thay bằng API Key của bạn
const char* ssid = "Nha Tro 4";     // Tên WiFi
const char* password = "nguyennam"; // Mật khẩu WiFi
const char* server = "api.thingspeak.com";

WiFiClient client;

// --- Cấu hình DHT11 ---
#define DHTPIN 14     // Chân kết nối cảm biến DHT11 (GPIO14)
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// --- Cấu hình OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool oled_ok = false;
bool dht_found = false;
bool dht_read_ok = false;

float temperature = 0.0;
float humidity = 0.0;
int dht_error_count = 0;

// --- Hàm hiển thị màn hình splash ---
void showSplashScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 20);
  display.println("Dang kiem tra");
  display.setCursor(20, 30);
  display.println("phan cung...");
  display.display();
  delay(2000);
}

// --- Kiểm tra OLED ---
void testOLED() {
  Serial.print("Kiem tra OLED... ");
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oled_ok = true;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("OLED hoat dong OK");
    display.display();
    Serial.println("OK");
  } else {
    oled_ok = false;
    Serial.println("LOI! Khong tim thay OLED");
  }
}

// --- Đọc dữ liệu DHT11 ---
void testDHT() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    dht_found = false;
    dht_read_ok = false;
    dht_error_count++;
    Serial.print("Doc loi DHT11! Loi lien tiep: ");
    Serial.println(dht_error_count);
    if (dht_error_count >= 3) {
      Serial.println("Thu reset DHT11...");
      dht.begin();
      dht_error_count = 0;
    }
  } else {
    dht_found = true;
    dht_read_ok = true;
    dht_error_count = 0;
    temperature = t;
    humidity = h;
    Serial.print("Nhiet do: ");
    Serial.print(t);
    Serial.println(" C");
    Serial.print("Do am: ");
    Serial.print(h);
    Serial.println(" %");
  }
}

// --- Vẽ icon nhiệt độ ---
void drawTemperatureIcon(int x, int y) {
  display.drawCircle(x + 6, y + 6, 5, SSD1306_WHITE);
  display.drawRect(x + 5, y, 2, 12, SSD1306_WHITE);
}

// --- Vẽ icon độ ẩm ---
void drawHumidityIcon(int x, int y) {
  display.drawCircle(x + 6, y + 6, 5, SSD1306_WHITE);
  display.drawLine(x + 6, y + 1, x + 6, y + 11, SSD1306_WHITE);
}

// --- Hiển thị dữ liệu trên OLED ---
void displayTestResult() {
  if (!oled_ok) return;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  int y = 0;
  display.setCursor(0, y);
  display.println("KET QUA DOC:");

  y += 10;
  display.setCursor(0, y);
  display.print("OLED: ");
  display.println(oled_ok ? "OK" : "LOI");

  y += 10;
  display.setCursor(0, y);
  display.print("DHT11: ");
  if (!dht_found) {
    display.println("KHONG PHAN HOI");
  } else if (!dht_read_ok) {
    display.println("DOC LOI");
  } else {
    display.println("OK");

    y += 10;
    drawTemperatureIcon(0, y);
    display.setCursor(20, y);
    display.print(temperature);
    display.println(" C");

    y += 15;
    drawHumidityIcon(0, y);
    display.setCursor(20, y);
    display.print(humidity);
    display.println(" %");
  }

  display.display();
}

// --- Gửi dữ liệu lên ThingSpeak ---
void sendDataToThingSpeak() {
  if (!dht_read_ok) return; // Nếu không đọc được thì không gửi

  if (client.connect(server, 80)) {
    String postStr = apiKey;
    postStr += "&field1=";
    postStr += String(temperature);
    postStr += "&field2=";
    postStr += String(humidity);
    postStr += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);

    Serial.println("Da gui du lieu len ThingSpeak.");
  }
  client.stop();
}

// --- Hàm setup ---
void setup() {
  Serial.begin(115200);
  delay(1000);

  // OLED
  oled_ok = display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (oled_ok) showSplashScreen();
  testOLED();

  // DHT11
  dht.begin();
  testDHT();
  displayTestResult();

  // WiFi
  Serial.print("Dang ket noi WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nDa ket noi WiFi!");
}

// --- Vòng lặp chính ---
void loop() {
  testDHT();           // Đọc DHT11
  displayTestResult(); // Hiển thị OLED
  sendDataToThingSpeak(); // Gửi dữ liệu

  delay(20000); // 20 giây (giảm tải ThingSpeak)
}
