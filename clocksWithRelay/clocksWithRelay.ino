#include <WiFiNINA.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TM1637TinyDisplay6.h>  // Include 6-Digit Display Class Header


//////////////////////// RELAY /////////////////////////////
// Variables for outputs
int outputPins[] = { 14, 15, 16, 17 };
bool outputState[] = { LOW, LOW, LOW, LOW };  // Initial output states (LOW)
int lastY[4] = { -1, -1, -1, -1 };            // Last tracked y values for each clock
////////////////////////////////////////////////////////////


// Define Digital Pins for the TM1637 displays
#define CLK1 6  // green
#define DIO1 7  // blue
#define CLK2 8
#define DIO2 9
#define CLK3 10
#define DIO3 11
#define CLK4 4
#define DIO4 5

const int clockBrightness = 4;

const char* ssid = " ";
const char* password = " ";

// NTP client setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);  // UTC time, updates every 60 seconds

// Easing algorithms
float linear(float t) {
  return t;
}
float squareWave(float t) {
  return (t < 0.5) ? 0 : (t - 0.5) * 2;
}
float quadraticOut(float t) {
  return t * (2 - t);
}
float quadraticInOut(float t) {
  return (t < 0.5) ? (2 * t * t) : (-1 + (4 - 2 * t) * t);
}

// Clock algorithms
typedef float (*EaseFunc)(float);
EaseFunc clockAlgos[] = { linear, squareWave, quadraticOut, quadraticInOut };
const char* clockAlgoNames[] = { "linear", "squareWave", "quadraticOut", "quadraticInOut" };

// Declare 4 display objects
TM1637TinyDisplay6 display1(CLK1, DIO1);
TM1637TinyDisplay6 display2(CLK2, DIO2);
TM1637TinyDisplay6 display3(CLK3, DIO3);
TM1637TinyDisplay6 display4(CLK4, DIO4);

// Variables to track time
unsigned long lastMillis = 0;  // Last recorded millis()
int lastSec = -1;              // Last recorded second


void setup() {
  // Set brightness for all displays
  display1.setBrightness(clockBrightness);
  display2.setBrightness(clockBrightness);
  display3.setBrightness(clockBrightness);
  display4.setBrightness(clockBrightness);

  Serial.begin(9600);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }

  // Initialize NTP client
  timeClient.begin();

  //////////////////////// RELAY /////////////////////////////
  // Initialize output pins
  for (int i = 0; i < 4; i++) {
    pinMode(outputPins[i], OUTPUT);
    digitalWrite(outputPins[i], outputState[i]);
  }
  ////////////////////////////////////////////////////////////
}


void updateDisplay(TM1637TinyDisplay6& display, int hours, int mins, float microSecs, float (*algo)(float)) {
  // Normalize seconds using the easing function
  float t01 = microSecs / 60.0;      // Normalize to 0-1
  if (t01 > 1) t01 -= 1;             // Wrap around
  float easedSecs = algo(t01) * 60;  // Apply easing and scale to seconds

  // Create time representation in HHMMSS format
  int easedTime = (hours * 10000) + (mins * 100) + int(easedSecs);
  uint8_t dots = 0b01010000;  // Dots between HH.MM.SS
  // uint8_t dots = 0b00000000; // no dots

  display.showNumberDec(easedTime, dots, true);
}


void loop() {
  // Update time
  timeClient.update();

  // Get current time components from NTP (in UTC)
  int hours = timeClient.getHours();
  int mins = timeClient.getMinutes();
  int secs = timeClient.getSeconds();

  // Adjust for Eastern Standard Time (EST), which is UTC - 5
  int timeZoneOffset = -5;
  hours += timeZoneOffset;
  if (hours < 0) hours += 24;  // Handle wraparound for negative hours
  else if (hours > 23) hours -= 24;

  // Calculate milliseconds since the last second change
  unsigned long currentMillis = millis();
  unsigned long mills = 0;
  if (secs != lastSec) {
    lastSec = secs;
    lastMillis = currentMillis;  // Reset the millisecond counter
  } else {
    mills = currentMillis - lastMillis;
  }

  // Combine seconds and milliseconds into a single value
  float microSecs = secs + mills / 1000.0;

  // Update each display with corresponding easing function
  updateDisplay(display1, hours, mins, microSecs, linear);
  updateDisplay(display2, hours, mins, microSecs, squareWave);
  updateDisplay(display3, hours, mins, microSecs, quadraticOut);
  updateDisplay(display4, hours, mins, microSecs, quadraticInOut);


  //////////////////////// RELAY /////////////////////////////

  // Format and print current time
  char buffer[64];
  sprintf(buffer, "Current Time (HH:MM:SS.mmm): %02d:%02d:%02d.%03lu", hours, mins, secs, mills);
  Serial.println(buffer);


  // Compute eased values and print
  for (int i = 0; i < 4; i++) {
    float t01 = microSecs / 60.0;    // Normalize to 0-1
    if (t01 > 1) t01 -= 1;           // Wrap around
    float y01 = clockAlgos[i](t01);  // Apply easing
    int y = int(y01 * 60);           // Scale to seconds
    sprintf(buffer, "%s: %02d", clockAlgoNames[i], y);
    // Serial.println(buffer);

    // Check if the y value has changed
    if (y != lastY[i]) {
      if (lastY[i] == 59) {
        // If the last value was 59, change the output state to LOW
        outputState[i] = HIGH;
        digitalWrite(outputPins[i], outputState[i]);  // Set the output pin state to LOW
      } else {
        // Toggle output state for this clock
        outputState[i] = (outputState[i] == LOW) ? HIGH : LOW;
        digitalWrite(outputPins[i], outputState[i]);  // Set the output pin state
      }

      // Update the last y value for this clock
      lastY[i] = y;
    }
  }
  ////////////////////////////////////////////////////////////

  delay(10);
}
