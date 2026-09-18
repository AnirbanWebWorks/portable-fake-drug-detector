#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- 1. HARDWARE PINS ---
const int PIN_SOURCE = 25; // DAC Output (Fixed at 3.3V)
const int PIN_SENSE  = 34; // ADC Input (Reads signal)

const int BUTTON_UP = 13;      
const int BUTTON_DOWN = 14;    
const int BUTTON_SELECT = 12;  
const int LED_GREEN = 18;     
const int LED_RED = 19;       

// --- 2. CALIBRATION RANGES ---
// Note: Since we are using Average now instead of Peak, 
// you may need to slighty tweak these values based on testing.
const int ASPIRIN_MIN = 1900;
const int ASPIRIN_MAX = 2200;

const int PARA_MIN    = 2100;
const int PARA_MAX    = 2350;

const int VITC_MIN    = 2500;
const int VITC_MAX    = 2700;

// Screen Setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Menu Data
const char *options[] = { "1. Aspirin", "2. Paracetamol", "3. Vitamin C" };
int selected = 0;
bool isTesting = false;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); 

  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_SELECT, INPUT_PULLUP);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(PIN_SENSE, INPUT); 

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.setRotation(2); 
  
  // Show Intro
  display.clearDisplay();
  display.setTextSize(2); display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 20); display.println("READY");
  display.display();
  delay(1000);
}

void loop() {
  if (!isTesting) {
    handleMenu();
  } else {
    runAverageScan();
  }
}

void handleMenu() {
  // Navigation
  if (digitalRead(BUTTON_UP) == LOW) { selected--; if(selected < 0) selected = 2; delay(200); }
  if (digitalRead(BUTTON_DOWN) == LOW) { selected++; if(selected > 2) selected = 0; delay(200); }
  if (digitalRead(BUTTON_SELECT) == LOW) { isTesting = true; delay(300); }

  // Draw UI
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0); display.println("SELECT TARGET:");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  
  for(int i=0; i<3; i++) {
    int y = 20 + (i*15);
    if(i == selected) { display.setCursor(0, y); display.print(">"); }
    display.setCursor(15, y); display.println(options[i]);
  }
  display.display();
}

void runAverageScan() {
  Serial.println("\n--- STARTING 10s AVERAGE SCAN ---");

  // 1. Set Voltage to Max (3.3V)
  dacWrite(PIN_SOURCE, 255); 
  delay(100); // Allow voltage to stabilize

  long totalSum = 0;
  int sampleCount = 50; 
  
  // 2. Sample Loop (50 times over 10 seconds)
  for (int i = 1; i <= sampleCount; i++) {
    int reading = analogRead(PIN_SENSE);
    totalSum += reading;
    
    // Serial Log for debugging
    Serial.print("Sample "); Serial.print(i);
    Serial.print(": "); Serial.println(reading);

    // Update Display with Countdown
    display.clearDisplay();
    display.setTextSize(2); 
    display.setCursor(0, 0); display.println("SAMPLING...");
    
    display.setTextSize(1);
    display.setCursor(0, 30); display.print("Sample: "); 
    display.print(i); display.print("/50");
    
    // Progress Bar
    int barWidth = map(i, 0, 50, 0, 128);
    display.fillRect(0, 50, barWidth, 10, SSD1306_WHITE);
    display.display();

    // Delay to make total time approx 10 seconds
    // 10000ms / 50 samples = 200ms delay per sample
    delay(200); 
  }

  // 3. Calculate Average
  int averageVal = totalSum / sampleCount;
  
  // Turn off probe
  dacWrite(PIN_SOURCE, 0);

  Serial.print(">>> AVERAGE VALUE: "); Serial.println(averageVal);

  // --- CHECK AGAINST SELECTED RANGE ---
  bool isAuthentic = false;

  // 0 = Aspirin, 1 = Paracetamol, 2 = Vitamin C
  if (selected == 0) {
      if (averageVal >= ASPIRIN_MIN && averageVal <= ASPIRIN_MAX) isAuthentic = true;
  }
  else if (selected == 1) {
      if (averageVal >= PARA_MIN && averageVal <= PARA_MAX) isAuthentic = true;
  }
  else if (selected == 2) {
      if (averageVal >= VITC_MIN && averageVal <= VITC_MAX) isAuthentic = true;
  }

  // --- DISPLAY RESULT ---
  display.clearDisplay();
  
  if (isAuthentic) {
    // PASS
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
    
    display.setTextSize(2); display.setCursor(10, 10); display.println("PASSED");
    display.setTextSize(1); display.setCursor(10, 35); display.println("Authentic");
    display.setCursor(10, 50); display.print("Avg: "); display.println(averageVal);
  } else {
    // FAIL
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, HIGH);
    
    display.setTextSize(2); display.setCursor(10, 10); display.println("FAKE!");
    display.setTextSize(1); display.setCursor(10, 35); display.println("Range Mismatch");
    display.setCursor(10, 50); display.print("Avg: "); display.println(averageVal);
  }
  display.display();

  // Wait for button to reset
  delay(1000);
  while(digitalRead(BUTTON_SELECT) == HIGH); 
  
  isTesting = false;
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  delay(500);
}