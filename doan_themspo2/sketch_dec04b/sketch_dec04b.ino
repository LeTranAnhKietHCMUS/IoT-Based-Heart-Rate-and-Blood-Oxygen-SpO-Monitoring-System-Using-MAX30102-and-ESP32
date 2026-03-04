// Thêm các define này TRƯỚC khi include Blynk
#define BLYNK_TEMPLATE_ID "TMPL6UmVBnRWj"
#define BLYNK_TEMPLATE_NAME "Blood Oxygen and Heart Rate"
#define BLYNK_AUTH_TOKEN "hUcG2WmRX2QA9hSMTEjTCCZkRe_OfRDH"
#define BLYNK_PRINT Serial

// THÊM các hằng số mới đầu file:
#define MINIMUM_SPO2 90 // SPO2 < 90% -> Báo động
#define MAXIMUM_SPO2 100 // SPO2 tối đa 100% 
#define MINIMUM_BPM 40 // Nhịp tim < 40 -> Báo động
#define MAXIMUM_BPM 100 // Nhịp tim > 100 -> Báo động
#define FINGER_THRESHOLD 20000  // Giảm ngưỡng phát hiện ngón tay

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// --- THIẾT BỊ: RGB LED + BUZZER ---
const int PIN_R = 15;
const int PIN_G = 2;
const int PIN_B = 4;
const int PIN_BUZZER = 16;

// Biến trạng thái
bool wifiFailed = false;
bool alarmActive = false;
unsigned long lastColorChange = 0;
unsigned long lastWifiBlink = 0;
unsigned long lastAlarmBeep = 0;
int colorIndex = 0;
int alarmStep = 0;
bool wifiBlinkState = false;
bool startupPhase = true;
unsigned long startupTime = 0;

// Define sensor, rates array, and beat variables
MAX30105 particleSensor;
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int beatAvg = 0;

int32_t spo2;
int8_t validSPO2;
int sp02Avg = 0;

// SpO2 sampling config
#define SAMPLE_SIZE 100
long redBuffer[SAMPLE_SIZE];
long irBuffer[SAMPLE_SIZE];
int sampleIndex = 0;
unsigned long lastSampleTimeSpO2 = 0;
const unsigned long sampleIntervalMs = 10;

const byte BPM_FILTER_SIZE = 8;
byte bpmBuffer[BPM_FILTER_SIZE];
byte bpmIndex = 0;

// ThingSpeak config
String apiKey = "LNNR8ZYXVA10GMG4";
const char *ssid = "Vy";
const char *pass = "vvvvvvvv";
const char *server = "api.thingspeak.com";
WiFiClient client;

unsigned long lastUploadTime = 0;
const unsigned long uploadInterval = 10000;

unsigned long tsLastReport = 0;
const unsigned long REPORTING_PERIOD_MS = 5000;

// OLED display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Bitmap data for heart icons
static const unsigned char PROGMEM beat1_bmp[] = {
  0x03, 0xC0, 0xF0, 0x06, 0x71, 0x8C, 0x0C, 0x1B, 0x06, 0x18, 0x0E, 0x02, 0x10, 0x0C, 0x03, 0x10,
  0x04, 0x01, 0x10, 0x04, 0x01, 0x10, 0x40, 0x01, 0x10, 0x40, 0x01, 0x10, 0xC0, 0x03, 0x08, 0x88,
  0x02, 0x08, 0xB8, 0x04, 0xFF, 0x37, 0x08, 0x01, 0x30, 0x18, 0x01, 0x90, 0x30, 0x00, 0xC0, 0x60,
  0x00, 0x60, 0xC0, 0x00, 0x31, 0x80, 0x00, 0x1B, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x04, 0x00
};

static const unsigned char PROGMEM beat2_bmp[] = {
  0x01, 0xF0, 0x0F, 0x80, 0x06, 0x1C, 0x38, 0x60, 0x18, 0x06, 0x60, 0x18, 0x10, 0x01, 0x80, 0x08,
  0x20, 0x01, 0x80, 0x04, 0x40, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x02, 0xC0, 0x00, 0x08, 0x03,
  0x80, 0x00, 0x08, 0x01, 0x80, 0x00, 0x18, 0x01, 0x80, 0x00, 0x1C, 0x01, 0x80, 0x00, 0x14, 0x00,
  0x80, 0x00, 0x14, 0x00, 0x80, 0x00, 0x14, 0x00, 0x40, 0x10, 0x12, 0x00, 0x40, 0x10, 0x12, 0x00,
  0x7E, 0x1F, 0x23, 0xFE, 0x03, 0x31, 0xA0, 0x04, 0x01, 0xA0, 0xA0, 0x0C, 0x00, 0xA0, 0xA0, 0x08,
  0x00, 0x60, 0xE0, 0x10, 0x00, 0x20, 0x60, 0x20, 0x06, 0x00, 0x40, 0x60, 0x03, 0x00, 0x40, 0xC0,
  0x01, 0x80, 0x01, 0x80, 0x00, 0xC0, 0x03, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0x30, 0x0C, 0x00,
  0x00, 0x08, 0x10, 0x00, 0x00, 0x06, 0x60, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x01, 0x80, 0x00
};

// Hàm điều khiển LED RGB
void setRGB(int r, int g, int b) {
  analogWrite(PIN_R, r);
  analogWrite(PIN_G, g);
  analogWrite(PIN_B, b);
}

// Hàm tạo màu rainbow
void getRainbowColor(int index, int &r, int &g, int &b) {
  int step = index % 360;
  if (step < 60) {
    r = 255; g = step * 255 / 60; b = 0;
  } else if (step < 120) {
    r = 255 - (step - 60) * 255 / 60; g = 255; b = 0;
  } else if (step < 180) {
    r = 0; g = 255; b = (step - 120) * 255 / 60;
  } else if (step < 240) {
    r = 0; g = 255 - (step - 180) * 255 / 60; b = 255;
  } else if (step < 300) {
    r = (step - 240) * 255 / 60; g = 0; b = 255;
  } else {
    r = 255; g = 0; b = 255 - (step - 300) * 255 / 60;
  }
}

// Hàm cập nhật LED và Buzzer
void updateLEDAndBuzzer() {
  unsigned long currentMillis = millis();
  
  // Ưu tiên 1: WiFi Failed
  if (wifiFailed) {
    // Nhấp nháy vàng chu kỳ 2s
    if (currentMillis - lastWifiBlink >= 2000) {
      lastWifiBlink = currentMillis;
      wifiBlinkState = !wifiBlinkState;
      
      if (wifiBlinkState) {
        setRGB(255, 255, 0); // Vàng
        tone(PIN_BUZZER, 2000, 100); // Bíp ngắn
      } else {
        setRGB(0, 0, 0); // Tắt
      }
    }
    return;
  }
  
  // Ưu tiên 2: Startup phase (2 giây đầu)
  if (startupPhase) {
    if (currentMillis - startupTime < 2000) {
      setRGB(0, 255, 255); // Cyan
      return;
    } else {
      startupPhase = false;
    }
  }
  
  // Ưu tiên 3: Alarm (BPM ngoài [40,120] hoặc SpO2 < 80)
  bool alarmCondition = false;
  if (beatAvg > 0) {
    if (beatAvg < MINIMUM_BPM || beatAvg > MAXIMUM_BPM) {
      alarmCondition = true;
    }
  }
  if (validSPO2 && spo2 < MINIMUM_SPO2) {
    alarmCondition = true;
  }
  
  if (alarmCondition) {
    setRGB(255, 0, 0); // Đỏ
    
    // Pattern 3 ngắn 1 dài
    if (currentMillis - lastAlarmBeep >= 200) {
      lastAlarmBeep = currentMillis;
      
      if (alarmStep == 0 || alarmStep == 1 || alarmStep == 2) {
        tone(PIN_BUZZER, 2000, 150); // Bíp ngắn
        alarmStep++;
      } else if (alarmStep == 3) {
        tone(PIN_BUZZER, 1500, 500); // Bíp dài
        alarmStep = 0;
      }
    }
    return;
  } else {
    alarmStep = 0;
    noTone(PIN_BUZZER);
  }
  
  // Ưu tiên 4: Đang đo - chạy rainbow
  if (currentMillis - lastColorChange >= 20) {
    lastColorChange = currentMillis;
    int r, g, b;
    getRainbowColor(colorIndex, r, g, b);
    setRGB(r, g, b);
    colorIndex = (colorIndex + 2) % 360;
  }
}

// Function to compute SpO2 from buffers
void computeSpO2FromBuffers() {
  long redMin = LONG_MAX, redMax = LONG_MIN;
  long irMin = LONG_MAX, irMax = LONG_MIN;
  long redSum = 0, irSum = 0;

  for (int i = 0; i < SAMPLE_SIZE; i++) {
    long r = redBuffer[i];
    long ir = irBuffer[i];

    if (r < redMin) redMin = r;
    if (r > redMax) redMax = r;
    if (ir < irMin) irMin = ir;
    if (ir > irMax) irMax = ir;

    redSum += r;
    irSum += ir;
  }

  float redDC = redSum / (float)SAMPLE_SIZE;
  float irDC = irSum / (float)SAMPLE_SIZE;
 
  // Kiểm tra DC hợp lệ
  if (redDC < 10000 || irDC < 10000) {
    validSPO2 = 0;
    return;
  }
  
  // Bước 2: Tính AC RMS (Root Mean Square) - CHÍNH XÁC HƠN
  float redACSum = 0, irACSum = 0;
  for (int i = 0; i < SAMPLE_SIZE; i++) {
    float redDiff = redBuffer[i] - redDC;
    float irDiff = irBuffer[i] - irDC;
    redACSum += redDiff * redDiff;
    irACSum += irDiff * irDiff;
  }
  
  float redAC = sqrt(redACSum / SAMPLE_SIZE);
  float irAC = sqrt(irACSum / SAMPLE_SIZE);
  
  // Kiểm tra AC hợp lệ
  if (redAC < 100 || irAC < 100) {
    validSPO2 = 0;
    return;
  }

  float R = (redAC / redDC) / (irAC / irDC);
  
  if (R <= 0.4 || R >= 3.5) {
    validSPO2 = 0;
    return;
  }
  
  float spo2f = 104.0 - 17.0 * R;
  
  if (spo2f > MAXIMUM_SPO2) spo2f = MAXIMUM_SPO2;
  if (spo2f < MINIMUM_SPO2) spo2f = MINIMUM_SPO2;
  
  if (spo2f < MINIMUM_SPO2 || spo2f > MAXIMUM_SPO2) {
    validSPO2 = 0;
  } else {
    validSPO2 = 1;
    spo2 = (int32_t)(spo2f + 0.5);
  }
}

// Function to send data to ThingSpeak
void sendToThingSpeak() {
  if (millis() - lastUploadTime < uploadInterval) {
    return;
  }

  if (beatAvg == 0) {
    return;
  }

  lastUploadTime = millis();

  if (client.connect(server, 80)) {
    Serial.println("Connecting to ThingSpeak...");

    String postStr = "field1=" + String(beatAvg);
    postStr += "&field2=" + String(spo2);

    client.println("POST /update HTTP/1.1");
    client.println("Host: api.thingspeak.com");
    client.println("Connection: close");
    client.println("X-THINGSPEAKAPIKEY: " + apiKey);
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: ");
    client.println(postStr.length());
    client.println();
    client.print(postStr);

    Serial.println("Data sent to ThingSpeak:");
    Serial.print("BPM: ");
    Serial.print(beatAvg);
    Serial.print(", SpO2: ");
    Serial.println(spo2);

    delay(500);
    client.stop();
  } else {
    Serial.println("Connection to ThingSpeak failed");
  }
}

// Function to send data to Blynk
void sendToBlynk() {
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    if (validSPO2) {
      sp02Avg = spo2;
    }
    
    Blynk.virtualWrite(V3, beatAvg);
    Blynk.virtualWrite(V4, sp02Avg);
    
    Serial.println("Data sent to Blynk:");
    Serial.print("V3 (BPM): ");
    Serial.print(beatAvg);
    Serial.print(", V4 (SpO2): ");
    Serial.println(sp02Avg);
    
    tsLastReport = millis();
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);

  // Khởi tạo LED RGB và Buzzer
  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  
  startupTime = millis();
  startupPhase = true;
  setRGB(0, 255, 255); // Cyan khi khởi động

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Initializing...");
  display.display();
  delay(1000);

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 was not found. Please check wiring/power.");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Sensor Error!");
    display.display();
    while (1);
  }

  Serial.println("Place your index finger on the sensor with steady pressure.");
  
  // CẤU HÌNH TỐI ƯU CHO MAX30105
  byte ledBrightness = 60;     // Giảm xuống 60 (từ 255) để tránh bão hòa
  byte sampleAverage = 4;      // Trung bình 4 mẫu
  byte ledMode = 2;            // Red + IR mode
  int sampleRate = 400;        // 400 samples/giây
  int pulseWidth = 411;        // 411µs = 18-bit resolution
  int adcRange = 4096;         // ADC range 4096

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
  // GIẢM CƯỜNG ĐỘ LED để tránh bão hòa
  particleSensor.setPulseAmplitudeRed(0x1F);   // 31 (thay vì 255)
  particleSensor.setPulseAmplitudeIR(0x1F);    // 31 (thay vì 255)
  particleSensor.setPulseAmplitudeGreen(0);

  sampleIndex = 0;
  lastSampleTimeSpO2 = millis();
  validSPO2 = 0;
  spo2 = 0;

  // Connect to WiFi
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Connecting WiFi");
  display.display();

  WiFi.begin(ssid, pass);
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    delay(500);
    Serial.print(".");
    wifiAttempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi Connected!");
    display.display();
    delay(1000);

    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
    
    if (Blynk.connected()) {
      Serial.println("Connected to Blynk!");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Blynk Connected!");
      display.display();
      delay(1000);
    } else {
      Serial.println("Blynk connection failed");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Blynk Failed!");
      display.println("Continuing...");
      display.display();
      delay(2000);
    }
    
  } else {
    Serial.println("\nWiFi connection failed");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi Failed!");
    display.println("Continuing...");
    display.display();
    delay(2000);
    wifiFailed = true;
    lastWifiBlink = millis();
  }

  lastUploadTime = millis();
}

void loop() {
  Blynk.run();
  
  // Cập nhật LED và Buzzer
  updateLEDAndBuzzer();
  
  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();

  // Debug mỗi 2 giây
  static unsigned long lastDebugTime = 0;
  if (millis() - lastDebugTime > 2000) {
    lastDebugTime = millis();
    Serial.print("IR: "); 
    Serial.print(irValue);
    Serial.print(" | Red: "); 
    Serial.print(redValue);
    Serial.print(" | Threshold: ");
    Serial.println(FINGER_THRESHOLD);
  }

  // Phát hiện ngón tay với ngưỡng thấp hơn
  // Kiểm tra bão hòa: nếu IR hoặc Red = 262143 thì có vấn đề
  if (irValue >= 262143 || redValue >= 262143) {
    // Bão hòa - có thể không có ngón tay hoặc LED quá sáng
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(10, 10);
    display.println("SENSOR SATURATED");
    display.setCursor(10, 25);
    display.println("Remove finger");
    display.setCursor(10, 40);
    display.println("then place again");
    display.display();
    
    sampleIndex = 0;
    lastSampleTimeSpO2 = millis();
    validSPO2 = 0;
    beatAvg = 0;
    return;
  }

  if (irValue > FINGER_THRESHOLD && irValue < 200000) {

    if (millis() - lastSampleTimeSpO2 >= sampleIntervalMs) {
      lastSampleTimeSpO2 = millis();

      irBuffer[sampleIndex] = irValue;
      redBuffer[sampleIndex] = redValue;
      sampleIndex++;

      if (sampleIndex >= SAMPLE_SIZE) {
        sampleIndex = 0;
        computeSpO2FromBuffers();
        if (validSPO2) {
          Serial.print("SpO2: ");
          Serial.print(spo2);
          Serial.println("%");
        } else {
          Serial.println("SpO2: invalid");
        }
      }
    }

    if (checkForBeat(irValue) == true) {

      display.clearDisplay();
      display.drawBitmap(23, 15, beat1_bmp, 24, 21, WHITE);
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(60, 10);
      display.println("BPM");
      display.setCursor(60, 20);
      display.println(beatAvg);
      display.setCursor(60, 30);
      display.print("SpO2:");
      display.setCursor(60, 40);
      display.print(spo2);
      display.println("%");
      display.display();

      long delta = millis() - lastBeat;
      lastBeat = millis();
      beatsPerMinute = 60 / (delta / 1000.0);

      if (beatsPerMinute < 255 && beatsPerMinute > 20) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;

        beatAvg = 0;
        for (byte x = 0; x < RATE_SIZE; x++)
          beatAvg += rates[x];
        beatAvg /= RATE_SIZE;

        delay(100);

        display.clearDisplay();
        display.drawBitmap(18, 10, beat2_bmp, 32, 32, WHITE);
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(60, 10);
        display.println("BPM");
        display.setCursor(60, 20);
        display.println(beatAvg);
        display.setTextSize(1);
        display.setCursor(60, 30);
        display.print("SpO2:");
        if (validSPO2) {
          display.setCursor(60, 40);
          display.print(spo2);
          display.println("%");
        } else {
          display.setCursor(60, 40);
          display.println("--");
        }
        display.display();
      }

      Serial.print("IR=");
      Serial.print(irValue);
      Serial.print(", BPM=");
      Serial.print(beatsPerMinute);
      Serial.print(", SPO2=");
      Serial.print(spo2);
      Serial.print("%, Avg BPM=");
      Serial.println(beatAvg);
    }

    sendToThingSpeak();
    sendToBlynk();

  } else {
    sampleIndex = 0;
    lastSampleTimeSpO2 = millis();
    validSPO2 = 0;
    beatAvg = 0;

    Serial.println("Place your index finger on the sensor with steady pressure.");

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(30, 10);
    display.println("Please place");
    display.setCursor(30, 25);
    display.println("your finger");
    display.setCursor(30, 40);
    display.println("and wait...");
    display.setCursor(30, 55);
    display.print("IR: ");
    display.print(irValue);
    display.display();
  }
  
  delay(5);
}
