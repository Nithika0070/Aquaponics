#include <WiFi.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// WiFi credentials
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ThingSpeak settings
const char* serverName = "http://api.thingspeak.com/update";
String apiKey = "CSJMICOUESWTW8T4";

// Sensor pins
#define TEMPSENSORPIN 32  // Analog pin for temperature sensor (potentiometer)
#define PHSENSORPIN 34    // Analog pin for pH sensor (potentiometer)
#define TURBIDITYPIN 33   // Analog pin for turbidity sensor (potentiometer)
#define AMMONIASENSORPIN 35 // Analog pin for ammonia sensor (potentiometer)
#define LEDPIN 13         // Digital pin for LED
#define LCD_ADDR 0x27   

// Ultrasonic pins
const int trigPin = 5;   // Updated trig pin for ESP32
const int echoPin = 18;  // Updated echo pin for ESP32

// LCD
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// Safe limits
const float SAFE_TEMP_MIN = 0;
const float SAFE_TEMP_MAX = 100;
const float SAFE_PH_MIN = 0;
const float SAFE_PH_MAX = 10;
const float SAFE_TURBIDITY_MAX = 80;
const float SAFE_AMMONIA_MAX = 2.0;  // Example safe limit for ammonia concentration (adjust as needed)

void setup() {
  Serial.begin(115200);

  // WiFi connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  // Initialize pins
  pinMode(TEMPSENSORPIN, INPUT);
  pinMode(PHSENSORPIN, INPUT);
  pinMode(TURBIDITYPIN, INPUT);
  pinMode(AMMONIASENSORPIN, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(LEDPIN, OUTPUT);

  // LCD setup
  lcd.init();
  lcd.backlight();
  lcd.print("System Initialized");
  delay(2000);
  lcd.clear();

}

void checkLimits(float temperature, float pH, float turbidity, float ammonia, long waterLevel) {
  bool alert = false;
  
  lcd.clear();
  lcd.setCursor(0, 0);

  if (temperature < SAFE_TEMP_MIN || temperature > SAFE_TEMP_MAX) {
    Serial.println("Temperature Alert!");
    lcd.print("Temp Alert!");
    alert = true;
  }
  if (pH < SAFE_PH_MIN || pH > SAFE_PH_MAX) {
    Serial.println("pH Alert!");
    lcd.print("pH Alert!");
    alert = true;
  }
  if (turbidity > SAFE_TURBIDITY_MAX) {
    Serial.println("Turbidity Alert!");
    lcd.print("Turb Alert!");
    alert = true;
  }
  if (ammonia > SAFE_AMMONIA_MAX) {
    Serial.println("Ammonia Alert!");
    lcd.print("Ammonia Alert!");
    alert = true;
  }

  if (alert) {
    digitalWrite(LEDPIN, HIGH); // Turn on LED
    lcd.setCursor(0, 1);
    lcd.print("Check System!");
  } else {
    digitalWrite(LEDPIN, LOW); // Turn off LED
    lcd.setCursor(0, 0);
    lcd.print("All OK!");
  }
}

void loop() {
  // Read temperature (simulated)
  int tempValueRaw = analogRead(TEMPSENSORPIN);
  float temperature = map(tempValueRaw, 0, 4095, -40, 125);

  // Read pH
  int pHValueRaw = analogRead(PHSENSORPIN);
  float pHValue = map(pHValueRaw, 0, 4095, 0, 14);

  // Read turbidity
  int turbidityValue = analogRead(TURBIDITYPIN);
  float turbidity = map(turbidityValue, 0, 4095, 0, 100);

  // Read ammonia
  int ammoniaValueRaw = analogRead(AMMONIASENSORPIN);
  float ammonia = map(ammoniaValueRaw, 0, 4095, 0, 5); // Adjust range for your sensor

  // Ultrasonic sensor for water level
  long duration, distance;
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration / 2) / 29.1;

  // Display and serial print
  Serial.print("Temp: "); Serial.print(temperature);
  Serial.print(", pH: "); Serial.print(pHValue);
  Serial.print(", Turbidity: "); Serial.print(turbidity);
  Serial.print(", Ammonia: "); Serial.print(ammonia);
  Serial.print(", Water Level: "); Serial.println(distance);

  checkLimits(temperature, pHValue, turbidity, ammonia, distance);

  // Send to ThingSpeak
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(serverName) + "?api_key=" + apiKey + "&field1=" + String(temperature) + "&field2=" + String(pHValue) + "&field3=" + String(turbidity) + "&field4=" + String(ammonia) + "&field5=" + String(distance);
    http.begin(url);
    
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      Serial.println("Data sent to ThingSpeak");
    } else {
      Serial.print("Error sending: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }

  delay(2000); // Delay for next reading
}

