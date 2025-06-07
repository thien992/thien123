#include <ESP8266WiFi.h>              // Thư viện điều khiển kết nối WiFi cho ESP8266
#include <DHT.h>                      // Thư viện đọc cảm biến DHT (nhiệt độ và độ ẩm)
#include <Wire.h>                     // Thư viện giao tiếp I2C (dùng cho màn hình OLED)
#include <Adafruit_GFX.h>             // Thư viện hỗ trợ đồ họa chung (Adafruit)
#include <Adafruit_SSD1306.h>         // Thư viện điều khiển màn hình OLED SSD1306

// --- Cấu hình WiFi và ThingSpeak ---
String apiKey = "5F8NIR1N22PYD5WG";   // API Key để gửi dữ liệu lên ThingSpeak
const char* ssid = "Galaxy A23 AE62"; // Tên mạng WiFi để ESP8266 kết nối
const char* password = "agrq7058";    // Mật khẩu WiFi để ESP8266 kết nối
const char* server = "api.thingspeak.com"; // Địa chỉ server ThingSpeak để gửi dữ liệu

WiFiClient client;                   // Tạo đối tượng client để gửi HTTP request tới server

// --- Cấu hình DHT11 ---
#define DHTPIN 14                    // Chân GPIO14 trên ESP8266 kết nối với DHT11
#define DHTTYPE DHT11                // Xác định loại cảm biến là DHT11
DHT dht(DHTPIN, DHTTYPE);            // Khởi tạo đối tượng cảm biến DHT11

// --- Cấu hình OLED ---
#define SCREEN_WIDTH 128             // Chiều rộng màn hình OLED (128 pixel)
#define SCREEN_HEIGHT 64             // Chiều cao màn hình OLED (64 pixel)
#define OLED_RESET -1                // Không dùng chân RESET (SSD1306 hỗ trợ phần mềm)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Khai báo màn hình OLED

bool oled_ok = false;                // Biến lưu trạng thái OLED hoạt động tốt hay không
bool dht_found = false;              // Biến lưu trạng thái DHT11 có phản hồi hay không
bool dht_read_ok = false;            // Biến lưu trạng thái dữ liệu DHT11 có đọc được không

float temperature = 0.0;             // Biến lưu giá trị nhiệt độ đọc được
float humidity = 0.0;                // Biến lưu giá trị độ ẩm đọc được
int dht_error_count = 0;             // Đếm số lần liên tiếp đọc lỗi DHT11

// --- Hàm hiển thị màn hình splash ---
void showSplashScreen() {
  display.clearDisplay();            // Xóa toàn bộ màn hình OLED
  display.setTextSize(1);            // Cỡ chữ nhỏ nhất (1)
  display.setTextColor(SSD1306_WHITE); // Chọn màu chữ trắng
  display.setCursor(20, 20);         // Đặt con trỏ tại tọa độ (20,20)
  display.println("Dang kiem tra");  // In ra màn hình dòng chữ
  display.setCursor(20, 30);         // Đặt con trỏ xuống dòng tiếp theo
  display.println("phan cung...");   // In tiếp dòng chữ
  display.display();                 // Hiển thị toàn bộ nội dung vừa vẽ ra màn hình
  delay(2000);                       // Tạm dừng 2 giây để xem splash
}

// --- Hàm kiểm tra OLED ---
void testOLED() {
  Serial.print("Kiem tra OLED... "); // In ra Serial để debug
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Khởi động OLED ở địa chỉ I2C 0x3C
    oled_ok = true;                  // Đánh dấu OLED hoạt động tốt
    display.clearDisplay();          // Xóa màn hình
    display.setTextSize(1);          // Kích thước chữ
    display.setTextColor(SSD1306_WHITE); // Màu chữ trắng
    display.setCursor(0, 0);         // Đặt con trỏ ở góc trên cùng bên trái
    display.println("OLED hoat dong OK"); // Thông báo OLED hoạt động tốt
    display.display();               // Hiển thị lên OLED
    Serial.println("OK");            // In ra Serial
  } else {
    oled_ok = false;                 // OLED không hoạt động
    Serial.println("LOI! Khong tim thay OLED"); // Thông báo lỗi OLED
  }
}

// --- Hàm đọc dữ liệu từ DHT11 ---
void testDHT() {
  float h = dht.readHumidity();      // Đọc độ ẩm từ DHT11
  float t = dht.readTemperature();   // Đọc nhiệt độ từ DHT11

  if (isnan(h) || isnan(t)) {        // Nếu giá trị trả về không hợp lệ (NaN)
    dht_found = false;               // Cảm biến không phản hồi
    dht_read_ok = false;             // Đọc lỗi
    dht_error_count++;               // Tăng bộ đếm lỗi
    Serial.print("Doc loi DHT11! Loi lien tiep: ");
    Serial.println(dht_error_count); // In số lỗi liên tiếp
    if (dht_error_count >= 3) {      // Nếu lỗi >=3 lần
      Serial.println("Thu reset DHT11..."); // Thử reset cảm biến
      dht.begin();                   // Gọi lại hàm khởi động cảm biến
      dht_error_count = 0;           // Reset bộ đếm lỗi
    }
  } else {                           // Nếu dữ liệu hợp lệ
    dht_found = true;                // Cảm biến có phản hồi
    dht_read_ok = true;              // Dữ liệu hợp lệ
    dht_error_count = 0;             // Reset bộ đếm lỗi
    temperature = t;                 // Lưu nhiệt độ
    humidity = h;                    // Lưu độ ẩm
    Serial.print("Nhiet do: "); 
    Serial.print(t); 
    Serial.println(" C");            // In nhiệt độ ra Serial
    Serial.print("Do am: "); 
    Serial.print(h); 
    Serial.println(" %");            // In độ ẩm ra Serial
  }
}

// --- Hàm vẽ icon nhiệt độ (hình tròn + cột nhiệt kế) ---
void drawTemperatureIcon(int x, int y) {
  display.drawCircle(x + 6, y + 6, 5, SSD1306_WHITE);  // Vẽ hình tròn nhỏ
  display.drawRect(x + 5, y, 2, 12, SSD1306_WHITE);    // Vẽ hình chữ nhật đứng (cột nhiệt kế)
}

// --- Hàm vẽ icon độ ẩm (hình giọt nước) ---
void drawHumidityIcon(int x, int y) {
  display.drawCircle(x + 6, y + 6, 5, SSD1306_WHITE);  // Vẽ hình giọt nước
  display.drawLine(x + 6, y + 1, x + 6, y + 11, SSD1306_WHITE); // Vẽ đường dọc ở giữa giọt nước
}

// --- Hàm hiển thị kết quả đọc lên OLED ---
void displayTestResult() {
  if (!oled_ok) return;             // Nếu OLED không hoạt động thì không hiển thị gì

  display.clearDisplay();           // Xóa màn hình
  display.setTextSize(1);           // Kích thước chữ nhỏ
  display.setTextColor(SSD1306_WHITE); // Màu chữ trắng

  int y = 0;                        // Tọa độ y ban đầu
  display.setCursor(0, y);
  display.println("KET QUA DOC:");  // Tiêu đề

  y += 10;                          // Xuống 1 dòng
  display.setCursor(0, y);
  display.print("OLED: ");
  display.println(oled_ok ? "OK" : "LOI"); // Hiển thị trạng thái OLED

  y += 10;
  display.setCursor(0, y);
  display.print("DHT11: ");
  if (!dht_found) {                 // Nếu không tìm thấy cảm biến
    display.println("KHONG PHAN HOI");
  } else if (!dht_read_ok) {        // Nếu cảm biến có phản hồi nhưng lỗi đọc
    display.println("DOC LOI");
  } else {                          // Nếu mọi thứ OK
    display.println("OK");

    y += 10;
    drawTemperatureIcon(0, y);      // Vẽ icon nhiệt độ
    display.setCursor(20, y);
    display.print(temperature);     // In nhiệt độ
    display.println(" C");

    y += 15;
    drawHumidityIcon(0, y);         // Vẽ icon độ ẩm
    display.setCursor(20, y);
    display.print(humidity);        // In độ ẩm
    display.println(" %");
  }

  display.display();                // Hiển thị nội dung lên màn hình
}

// --- Hàm gửi dữ liệu lên ThingSpeak ---
void sendDataToThingSpeak() {
  if (!dht_read_ok) return; // Nếu không đọc được dữ liệu từ DHT11 thì thoát luôn, tránh gửi dữ liệu rác

  if (client.connect(server, 80)) { // Kết nối TCP đến server ThingSpeak trên cổng 80 (HTTP)
    String postStr = apiKey;          // Chuỗi dữ liệu POST: bắt đầu với API key
    postStr += "&field1=";            // Thêm field1 cho nhiệt độ
    postStr += String(temperature);   // Thêm giá trị nhiệt độ
    postStr += "&field2=";            // Thêm field2 cho độ ẩm
    postStr += String(humidity);      // Thêm giá trị độ ẩm
    postStr += "\r\n\r\n";            // Kết thúc chuỗi dữ liệu bằng dấu xuống dòng (HTTP yêu cầu)

    // Gửi HTTP header và body tuần tự:
    client.print("POST /update HTTP/1.1\n");                  // Yêu cầu POST đến endpoint /update
    client.print("Host: api.thingspeak.com\n");               // Header bắt buộc trong HTTP/1.1
    client.print("Connection: close\n");                      // Đóng kết nối sau khi gửi xong
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");     // Header thêm API key (phòng khi ThingSpeak yêu cầu)
    client.print("Content-Type: application/x-www-form-urlencoded\n"); // Loại dữ liệu gửi đi
    client.print("Content-Length: ");                         // Header báo độ dài dữ liệu
    client.print(postStr.length());                           // In độ dài dữ liệu
    client.print("\n\n");                                     // Kết thúc header bằng dòng trắng

    client.print(postStr); // Gửi phần body dữ liệu (nhiệt độ và độ ẩm)

    Serial.println("Da gui du lieu len ThingSpeak."); // Ghi log ra Serial Monitor để kiểm tra
  }
  client.stop(); // Đóng kết nối TCP để giải phóng tài nguyên
}


// --- Hàm setup chạy 1 lần ---
void setup() {
  Serial.begin(115200);             // Khởi động Serial để debug
  delay(1000);                      // Delay 1 giây

  oled_ok = display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Khởi động OLED
  if (oled_ok) showSplashScreen(); // Nếu OLED hoạt động thì hiển thị splash screen
  testOLED();                       // Kiểm tra OLED

  dht.begin();                      // Khởi động cảm biến DHT11
  testDHT();                        // Đọc dữ liệu ban đầu từ DHT11
  displayTestResult();              // Hiển thị kết quả lên OLED

  Serial.print("Dang ket noi WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);       // Kết nối WiFi
  while (WiFi.status() != WL_CONNECTED) { // Chờ kết nối thành công
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nDa ket noi WiFi!"); // Thông báo đã kết nối
}

// --- Vòng lặp chính chạy liên tục ---
void loop() {
  testDHT();                        // Đọc dữ liệu DHT11
  displayTestResult();              // Hiển thị dữ liệu lên OLED
  sendDataToThingSpeak();           // Gửi dữ liệu lên ThingSpeak
  delay(15000);                     // Chờ 15 giây trước khi lặp lại
}
