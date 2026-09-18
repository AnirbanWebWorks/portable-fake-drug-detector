#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// ==================================================
//               USER SETTINGS (EDIT THESE)
// ==================================================
const char* ssid     = "SAHA_5G";      // <--- CHANGE THIS
const char* password = "9007508462";  // <--- CHANGE THIS
 
// Paste the GOOGLE SCRIPT ID here (From Phase 1, Step 4)
String GAS_ID = "AKfycbyNpmibyFGALr4DnWEZyL-gDwk9xpVVLgf7ljZnIYptWzQKusYxwxw23iXpJaGTUZYP";  // <--- CHANGE THIS

// ==================================================
//               HARDWARE SETTINGS
// ==================================================
#define PIN_SOURCE 25
#define PIN_SENSE  34
#define LED_POWER  5 
#define LED_GREEN  18
#define LED_RED    19

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(115200); // Keep this fast for debugging
  
  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_POWER, HIGH); // System On

  lcd.init(); lcd.backlight();
  
  // --- WIFI CONNECTION ---
  lcd.setCursor(0,0);
  lcd.print("Connecting WiFi");
  
  WiFi.begin(ssid, password);
  int dots = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lcd.setCursor(0,1);
    for(int i=0; i<dots; i++) lcd.print(".");
    dots++;
    if(dots > 16) { dots=0; lcd.setCursor(0,1); lcd.print("                "); }
  }
  
  lcd.clear();
  lcd.print("WiFi Connected!");
  Serial.println("\nWiFi Connected.");
  delay(1000);
}

void loop() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Ready to Scan...");
  
  // Wait a bit before starting (simulates the "Dip Probe" pause)
  delay(2000);

  lcd.setCursor(0,0);
  lcd.print("Scanning...     ");
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);

  String dataString = ""; // This will hold "12,45,99,200..."
  
  // --- PHASE 1: FORWARD SCAN (0V -> 3.3V) ---
  // Exact same logic as your Python logger
  for (int i = 0; i <= 255; i += 8) { 
    dacWrite(PIN_SOURCE, i);
    delay(20); 
    int val = analogRead(PIN_SENSE);
    dataString += String(val) + ","; // Add comma
  }

  // --- PHASE 2: REVERSE SCAN (3.3V -> 0V) ---
  for (int i = 255; i >= 0; i -= 8) { 
    dacWrite(PIN_SOURCE, i);
    delay(20); 
    int val = analogRead(PIN_SENSE);
    
    // Logic to avoid trailing comma at the very end
    if (i > 0) {
      dataString += String(val) + ",";
    } else {
      dataString += String(val); // Last number, no comma
    }
  }
  
  // Turn off probe
  dacWrite(PIN_SOURCE, 0);

  // --- UPLOAD TO GOOGLE SHEETS ---
  lcd.setCursor(0,1);
  lcd.print("Uploading...");
  Serial.println("Uploading Data...");
  
  // Note: Since we have no keyboard, we label everything "Unknown"
  // You will rename them in the Sheet later.
  sendDataToGoogle("Unknown_Sample", dataString);
  
  lcd.clear();
  lcd.print("Upload Done!");
  digitalWrite(LED_GREEN, HIGH); // Success Blink
  delay(1000);
  digitalWrite(LED_GREEN, LOW);
  
  // Wait 5 seconds before next scan
  delay(5000);
}

//Function to send data to Google Sheets
void sendDataToGoogle(String label, String value) {
  if(WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    
    // Construct the URL
    // Format: https://script.google.com/.../exec?label=X&value=Y
    String url = "https://script.google.com/macros/s/" + GAS_ID + "/exec?label=" + label + "&value=" + value;
    
    Serial.println("Sending Request: " + url);

    // IMPORTANT: Google redirects to a temporary URL, we must follow it
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    http.begin(url);
    int httpCode = http.GET(); // Send the request
    
    if (httpCode > 0) {
      Serial.print("Google Response Code: "); Serial.println(httpCode);
      String payload = http.getString();
      Serial.println("Response: " + payload);
    } else {
      Serial.print("Error on HTTP request: "); Serial.println(http.errorToString(httpCode).c_str());
      lcd.clear();
      lcd.print("Net Error!");
      digitalWrite(LED_RED, HIGH);
    }
    
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
    lcd.clear();
    lcd.print("WiFi Lost!");
  }
}