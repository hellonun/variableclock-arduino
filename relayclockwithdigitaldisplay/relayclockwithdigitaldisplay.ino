// 5 digit 7 segment display TM 1637
// p5.js version https://editor.p5js.org/nun.noc21/full/bWWMc8fB8
// algorithm plotting https://editor.p5js.org/icm-nun/sketches/Fr9G2DBMW 

#include <WiFiNINA.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TM1637TinyDisplay6.h>  // Include 6-Digit Display Class Header

// Define Digital Pins for the TM1637 display
#define CLK 7
#define DIO 8

const char* ssid     = " ";
const char* password = " ";

// NTP client setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // UTC time, updates every 60 seconds

// Easing algorithms
float linear(float t) { return t; }
float squareWave(float t) { 
  if (t < 0.5) return 0; 
  return (t - 0.5) * 2; // This maps t from 0.5-1.0 to 0-1
}
float quadraticOut(float t) { return t * (2 - t); }
float quadraticInOut(float t) {
  if (t < 0.5) return 2 * t * t;
  return -1 + (4 - 2 * t) * t;
}

// Clock algorithms
typedef float (*EaseFunc)(float);
EaseFunc clockAlgos[] = {linear, squareWave, quadraticOut, quadraticInOut};
const char* clockAlgoNames[] = {"linear", "squareWave", "quadraticOut", "quadraticInOut"};

// Variables to track time
unsigned long lastMillis = 0;  // Last recorded millis()
int lastSec = -1;              // Last recorded second

// Variables for outputs
int outputPins[] = {2, 3, 4, 5};
bool outputState[] = {LOW, LOW, LOW, LOW}; // Initial output states (LOW)
int lastY[4] = {-1, -1, -1, -1}; // Last tracked y values for each clock

// Declare the display object
TM1637TinyDisplay6 display(CLK, DIO);

void setup() {
  display.setBrightness(LOW);

  Serial.begin(9600);
  while (!Serial); // Wait for Serial monitor

  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected!");

  // Initialize NTP client
  timeClient.begin();

  // Initialize output pins
  for (int i = 0; i < 4; i++) {
    pinMode(outputPins[i], OUTPUT);
    digitalWrite(outputPins[i], outputState[i]);
  }
}

void loop() {
  // Update time
  timeClient.update();

// Get current time components from NTP (in UTC)
int hours = timeClient.getHours();
int mins = timeClient.getMinutes();
int secs = timeClient.getSeconds();

// Adjust for Eastern Standard Time (EST), which is UTC - 5
int timeZoneOffset = -5;  // UTC - 5 for EST
hours += timeZoneOffset;

// Handle potential rollover for hours (e.g., if hours becomes negative or exceeds 23)
if (hours < 0) {
  hours += 24; // Wrap around if negative (e.g., -1 becomes 23)
} else if (hours > 23) {
  hours -= 24; // Wrap around if greater than 23 (e.g., 24 becomes 0)
}

// Ensure hours are in a two-digit format (00-23 for EST)
if (hours < 10) {
  hours = 0;  // Set to 00 if single digit (for proper HHMMSS format)
}

// Create a single number representing HHMMSS
int timeNumber = (hours * 10000) + (mins * 100) + secs;

// Define the dots bitmask for HH.MM.SS format without the last dot
uint8_t dots = 0b01010000; // Dots between HH, MM, but not after SS

// Display the time with dots
display.showNumberDec(timeNumber, dots, true);


  // Calculate millis since the last second change
  unsigned long currentMillis = millis();
  unsigned long mills = 0;
  if (secs != lastSec) {
    lastSec = secs;
    lastMillis = currentMillis; // Reset the millisecond counter
  } else {
    mills = currentMillis - lastMillis;
  }

  // Combine seconds and milliseconds into a single value
  float microSecs = secs + mills / 1000.0;

  // Format and print current time
  char buffer[64];
  sprintf(buffer, "Current Time (HH:MM:SS.mmm): %02d:%02d:%02d.%03lu", hours, mins, secs, mills);
  Serial.println(buffer);

  // Compute eased values and print
  for (int i = 0; i < 4; i++) {
    float t01 = microSecs / 60.0; // Normalize to 0-1
    if (t01 > 1) t01 -= 1;       // Wrap around
    float y01 = clockAlgos[i](t01); // Apply easing
    int y = int(y01 * 60);          // Scale to seconds
    sprintf(buffer, "%s: %02d", clockAlgoNames[i], y);
    // Serial.println(buffer);

    // Check if the y value has changed
    if (y != lastY[i]) {
      if (lastY[i] == 59) {
        // If the last value was 59, change the output state to LOW
        outputState[i] = HIGH;
        digitalWrite(outputPins[i], outputState[i]); // Set the output pin state to LOW
      } else {
        // Toggle output state for this clock
        outputState[i] = (outputState[i] == LOW) ? HIGH : LOW;
        digitalWrite(outputPins[i], outputState[i]); // Set the output pin state
      }

      // Update the last y value for this clock
      lastY[i] = y;
    }
  }

  // Serial.println("-----------------------------");
  delay(100); 
} 