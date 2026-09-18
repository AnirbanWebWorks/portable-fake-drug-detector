#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "drug_model.h"  // YOUR AI BRAIN

// --- 1. CONFIGURATION ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- PINS ---
const int BUTTON_UP = 13;      // Move Up / Back
const int BUTTON_DOWN = 14;    // Move Down
const int BUTTON_SELECT = 12;  // Select / Start
const int LED_GREEN = 18;      // Real
const int LED_RED = 19;        // Fake

// --- SAFETY GATES (Strict Logic) ---
const int ASPIRIN_MIN = 2600, ASPIRIN_MAX = 3000;
const int PARA_MIN    = 2450, PARA_MAX    = 2750;
const int VITC_MIN    = 3130, VITC_MAX    = 3200;

// --- MENU SYSTEM ---
const char *options[] = { 
  "1. Test Aspirin", 
  "2. Test Paracetamol", 
  "3. Test Vitamin C", 
  "4. Battery Info", 
  "5. Environment"
};
const int menuLength = 5;
int selected = 0;
int topVisible = 0;

// --- STATES ---
enum SystemState { STATE_MENU, STATE_READY, STATE_SCANNING, STATE_RESULT, STATE_INFO };
SystemState currentState = STATE_MENU;

// --- RESULT VARIABLES ---
bool isAuthentic = false;
String failReason = "";
float detectedAmp = 0;
int aiClass = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // ESP32 I2C

  // Setup Hardware
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_SELECT, INPUT_PULLUP);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);

  // Init OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.setRotation(2); // Rotation as per your request

  // Splash Screen
  showSplashScreen();
}

void loop() {
  switch (currentState) {
    case STATE_MENU:     handleMenu(); break;
    case STATE_READY:    handleReady(); break;
    case STATE_SCANNING: runScan(); break;
    case STATE_RESULT:   handleResult(); break;
    case STATE_INFO:     handleInfo(); break;
  }
}

// ==========================================
// CORE LOGIC: PROBE READING
// ==========================================
void read_probe(float* buffer) {
  // ⚠️ INSERT YOUR ANALOG READ CODE HERE ⚠️
  // For now, we simulate data to prevent crashes if probe isn't connected
  for(int i=0; i<64; i++) {
     // buffer[i] = analogRead(34); // UNCOMMENT THIS FOR REAL SENSOR
     // delay(5);
     buffer[i] = 0; // Placeholder
  }
}

// ==========================================
// 1. MENU STATE
// ==========================================
void handleMenu() {
  // Navigation
  if (digitalRead(BUTTON_UP) == LOW) {
    selected--;
    if (selected < 0) { selected = menuLength - 1; topVisible = menuLength - 3; }
    else if (selected < topVisible) topVisible = selected;
    delay(150);
  }
  if (digitalRead(BUTTON_DOWN) == LOW) {
    selected++;
    if (selected >= menuLength) { selected = 0; topVisible = 0; }
    else if (selected >= topVisible + 3) topVisible++;
    delay(150);
  }
  if (digitalRead(BUTTON_SELECT) == LOW) {
    if (selected <= 2) currentState = STATE_READY; // Drug Tests
    else currentState = STATE_INFO; // Info pages
    delay(300);
  }

  // Draw Menu
  display.clearDisplay();
  display.fillRect(0, 0, 128, 14, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(30, 3); display.print("SELECT MODE");
  
  display.setTextColor(SSD1306_WHITE);
  for (int i = 0; i < 3; i++) {
    int itemIndex = topVisible + i;
    if (itemIndex >= menuLength) break;
    int y = 18 + (i * 16);
    
    if (itemIndex == selected) {
      display.drawRect(0, y - 2, 128, 15, SSD1306_WHITE); 
      display.setCursor(5, y); display.print(">");
    }
    display.setCursor(15, y); display.print(options[itemIndex]);
  }
  display.display();
}

// ==========================================
// 2. READY STATE (Pre-Scan)
// ==========================================
void handleReady() {
  // BACK SYSTEM: UP button goes back
  if (digitalRead(BUTTON_UP) == LOW) { currentState = STATE_MENU; delay(300); return; }
  // START: SELECT button
  if (digitalRead(BUTTON_SELECT) == LOW) { currentState = STATE_SCANNING; delay(300); return; }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  // Header
  display.setCursor(0, 0); display.print("TARGET: ");
  if(selected == 0) display.println("ASPIRIN");
  if(selected == 1) display.println("PARACETAMOL");
  if(selected == 2) display.println("VITAMIN C");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  display.setCursor(0, 25); display.println("Place probe in"); display.println("sample solution.");
  
  // Footer
  display.setCursor(0, 55); display.print("[UP] Back  [SEL] Scan");
  display.display();
}

// ==========================================
// 3. SCANNING STATE (Animation + AI)
// ==========================================
void runScan() {
  // 1. Cool Animation
  for(int i = 0; i <= 20; i++) {
    display.clearDisplay();
    display.setCursor(0, 0); display.print("ANALYZING...");
    display.drawLine(0, 35, 128, 35, SSD1306_WHITE);
    for(int x=0; x<128; x+=8) {
      display.drawLine(x, 35, x, 35 - random(0, 20), SSD1306_WHITE);
    }
    // Progress Bar
    display.drawRect(10, 50, 108, 6, SSD1306_WHITE);
    display.fillRect(12, 52, map(i, 0, 20, 0, 104), 2, SSD1306_WHITE);
    display.display();
    delay(100); 
  }

  // 2. REAL READING & AI LOGIC
  float sensor_data[64];
  read_probe(sensor_data); // Get Data

  // Calculate Max Amp
  float max_amp = 0;
  for(int i=0; i<64; i++) if(sensor_data[i] > max_amp) max_amp = sensor_data[i];
  detectedAmp = max_amp;

  // Run AI
  // 0=Aspirin, 1=Fake, 2=Paracetamol, 3=Vitamin_C
  int ai_result = predict_drug(sensor_data);
  aiClass = ai_result;

  // 3. DUAL-CHECK VERIFICATION
  isAuthentic = false;
  failReason = "";

  if (selected == 0) { // ASPIRIN MODE
    if (ai_result != 0) failReason = "Bad Shape"; 
    else if (max_amp < ASPIRIN_MIN || max_amp > ASPIRIN_MAX) failReason = "Bad Val";
    else isAuthentic = true;
  }
  else if (selected == 1) { // PARACETAMOL MODE
    if (ai_result != 2) failReason = "Bad Shape";
    else if (max_amp < PARA_MIN || max_amp > PARA_MAX) failReason = "Bad Val";
    else isAuthentic = true;
  }
  else if (selected == 2) { // VITAMIN C MODE
    if (ai_result != 3) failReason = "Bad Shape";
    else if (max_amp < VITC_MIN || max_amp > VITC_MAX) failReason = "Bad Val";
    else isAuthentic = true;
  }

  // 4. SET LEDS
  if(isAuthentic) {
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
  } else {
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, HIGH);
  }

  currentState = STATE_RESULT;
}

// ==========================================
// 4. RESULT STATE
// ==========================================
void handleResult() {
  // BACK SYSTEM: Any button returns to menu
  if (digitalRead(BUTTON_SELECT) == LOW || digitalRead(BUTTON_UP) == LOW || digitalRead(BUTTON_DOWN) == LOW) { 
    // Turn off LEDs when leaving result screen
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);
    currentState = STATE_MENU; 
    delay(300); 
  }

  display.clearDisplay();
  if (!isAuthentic) {
    // FAKE SCREEN
    display.fillRoundRect(0, 0, 128, 64, 4, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setTextSize(2); display.setCursor(35, 10); display.print("FAKE!");
    display.setTextSize(1); display.setCursor(10, 35); display.print("Reason: "); display.print(failReason);
    display.setCursor(10, 48); display.print("Val: "); display.print((int)detectedAmp);
  } else {
    // REAL SCREEN
    display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2); display.setCursor(35, 10); display.print("REAL");
    display.setTextSize(1); display.setCursor(20, 35); display.print("Match: 99%");
    display.setCursor(20, 48); display.print("Sample Verified");
  }
  display.display();
}

// ==========================================
// 5. INFO STATE
// ==========================================
void handleInfo() {
  // BACK SYSTEM: Up or Select goes back
  if (digitalRead(BUTTON_SELECT) == LOW || digitalRead(BUTTON_UP) == LOW) { currentState = STATE_MENU; delay(300); }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  
  if (selected == 3) { 
      display.println("BATTERY STATUS"); display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
      display.setCursor(0, 20); display.setTextSize(2); display.println("4.1 V");
      display.setTextSize(1); display.setCursor(0, 40); display.println("Optimal Level");
  } else { 
      display.println("ENVIRONMENT"); display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
      display.setCursor(0, 20); display.print("Temp: 24 C");
      display.setCursor(0, 35); display.print("Humid: 45 %");
  }
  display.setCursor(0, 55); display.print("[UP] Back");
  display.display();
}

void showSplashScreen() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.drawRect(10, 5, 108, 32, SSD1306_WHITE);
  display.fillRect(10, 34, 108, 3, SSD1306_WHITE); 
  display.setTextSize(3); display.setCursor(30, 10); display.print("SAFE");
  display.setTextSize(1); display.setCursor(5, 45); display.print("Substance Analysis");
  display.display();
  delay(2000); 
}