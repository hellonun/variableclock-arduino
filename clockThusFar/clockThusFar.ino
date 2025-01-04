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
#define CLK1 2  // green
#define DIO1 3  // blue
#define CLK2 4
#define DIO2 5
#define CLK3 6
#define DIO3 7
#define CLK4 8
#define DIO4 9

const int clockBrightness = 7;

const char* ssid = "   ";
const char* password = "   ";

// NTP client setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);  // UTC time, updates every 60 seconds
float microSecs;

// Easing algorithms
float linear(float t) {
  return t;
}
float squareWave(float t) {
  return (t < 0.5) ? 0 : (t - 0.5) * 2;
}
// float quadraticOut(float t) {
//   float midpointAdjustment = 0.6;  // Adjust this value to control how much the midpoint is lowered
//   return (t * (2 - t)) * midpointAdjustment + (1 - midpointAdjustment) * t;
// }

// float quadraticInOut(float t) {
//   float midpointAdjustment = 0.6;  // Adjust this value to control the midpoint lowering
//   float original = (t < 0.5) ? (2 * t * t) : (-1 + (4 - 2 * t) * t);
//   return original * midpointAdjustment + (1 - midpointAdjustment) * t;
// }


float triangleWave(float t) {
  float triInc = t * PI / 2;
  float triVal = sin(triInc);
  float mappedtriVal = triVal * 60;
  float triCount = mappedtriVal / 60;
  return triCount;
}

float sineWave(float t) {
  float sinInc = t * (PI * 2);
  float sinVal = sin(sinInc);                       // Compute sine of the increment
  float mappedSinVal = 5.0 - (sinVal + 1.0) * 5.0;  // Map -1 to 1 range to 5 to -5
  float sincount = (microSecs + mappedSinVal) / 60;
  return sincount;
}



// Clock algorithms
typedef float (*EaseFunc)(float);
EaseFunc clockAlgos[] = { linear, squareWave, triangleWave, sineWave };
const char* clockAlgoNames[] = { "linear", "squareWave", "triangleWave", "sineWave" };

// Declare 4 display objects
TM1637TinyDisplay6 display1(CLK1, DIO1);
TM1637TinyDisplay6 display2(CLK2, DIO2);
TM1637TinyDisplay6 display3(CLK3, DIO3);
TM1637TinyDisplay6 display4(CLK4, DIO4);

// Variables to track time
unsigned long lastMillis = 0;  // Last recorded millis()
int lastSec = -1;              // Last recorded second
int lastMin = -1;              // Last recorded minute

// Display order for algorithms
int algoOrder[] = { 0, 1, 2, 3 };

void shuffleArray(int* array, int size) {
  for (int i = size - 1; i > 0; i--) {
    int j = random(0, i + 1);  // Pick a random index
    int temp = array[i];
    array[i] = array[j];
    array[j] = temp;
  }
}

void setup() {
  // Set brightness for all displays
  display1.setBrightness(clockBrightness);
  display2.setBrightness(clockBrightness);
  display3.setBrightness(clockBrightness);
  display4.setBrightness(clockBrightness);

  Serial.begin(9600);

  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

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

  // Convert to 12-hour format
  bool isPM = (hours >= 12);
  hours = hours % 12;
  if (hours == 0) hours = 12;  // 12 AM or 12 PM


  // Shuffle the algorithms if the minute has changed
  if (mins != lastMin) {
    lastMin = mins;
    shuffleArray(algoOrder, 4);  // Shuffle the order of algorithms
    Serial.println("Minute changed! Algorithms shuffled:");
    for (int i = 0; i < 4; i++) {
      Serial.println(clockAlgoNames[algoOrder[i]]);
    }
  }

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
  microSecs = secs + mills / 1000.0;

  // Update each display with the shuffled algorithm order
  updateDisplay(display1, hours, mins, microSecs, clockAlgos[algoOrder[0]]);
  updateDisplay(display2, hours, mins, microSecs, clockAlgos[algoOrder[1]]);
  updateDisplay(display3, hours, mins, microSecs, clockAlgos[algoOrder[2]]);
  updateDisplay(display4, hours, mins, microSecs, clockAlgos[algoOrder[3]]);


  char buffer[64];
  sprintf(buffer, "Current Time (HH:MM:SS.mmm): %02d:%02d:%02d.%03lu %s",

          hours, mins, secs, mills, isPM ? "PM" : "AM");
  // Serial.println(buffer);


  //////////////////////// RELAY /////////////////////////////

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

  delay(5); 
}
