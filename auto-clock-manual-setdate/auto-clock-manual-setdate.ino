// 5 digit 7 segment display TM 1637
#include <RTCZero.h>
#include <Arduino.h>
#include <TM1637TinyDisplay6.h>  // Include 6-Digit Display Class Header
#include <WiFiNINA.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Define Digital Pins for the TM1637 display
#define CLK 7
#define DIO 8

TM1637TinyDisplay6 display(CLK, DIO);  // 6-Digit Display Class

// Create an rtc object
RTCZero rtc;

// WiFi credentials
const char* ssid     = " ";
const char* password = " ";

// Eastern Time offset in seconds (UTC-5 hours)
const long utcOffsetInSeconds = -5 * 3600;
const long dstOffsetInSeconds = 3600;  // Additional offset for DST (1 hour)

// NTP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds, 60000); // Adjust the time offset as needed

void setup() {
  Serial.begin(9600);
  
  // Initialize WiFi and connect
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.print(" ...");
    WiFi.begin(ssid, password);
    delay(5000);
  }
  Serial.println("Connected to WiFi");
  
  // Initialize NTP Client
  timeClient.begin();
  timeClient.update();

  // Initialize RTC
  rtc.begin();  

  // Set RTC time to June 27, 2024
  setRTCTime();

  display.setBrightness(BRIGHT_HIGH);
}

void loop() {
  // Update NTP time periodically
  timeClient.update();
  
  // Set RTC time from NTP every minute
  if (rtc.getSeconds() == 0) {
    syncRTCWithNTP();
  }

  // Read current time from RTC
  int hours = rtc.getHours();
  int minutes = rtc.getMinutes();
  int seconds = rtc.getSeconds();

  // Create a single number representing HHMMSS
  int timeNumber = (hours * 10000) + (minutes * 100) + seconds;

  // Define the dots bitmask for HH.MM.SS format without the last dot
  uint8_t dots = 0b01010000; // Dots between HH, MM, but not after SS

  // Display the time with dots
  display.showNumberDec(timeNumber, dots, true);

  // Optional: Print to serial for debugging
  Serial.print(hours < 10 ? "0" : ""); Serial.print(hours); Serial.print(".");
  Serial.print(minutes < 10 ? "0" : ""); Serial.print(minutes); Serial.print(".");
  Serial.print(seconds < 10 ? "0" : ""); Serial.println(seconds);

  delay(1000); // Update display every second
}

void syncRTCWithNTP() {
  long offset = utcOffsetInSeconds;

  // Check if DST is in effect and adjust offset
  if (isDST()) {
    offset += dstOffsetInSeconds;
    Serial.println("DST is in effect. Adjusting time by 1 hour.");
  } else {
    Serial.println("DST is not in effect.");
  }

  timeClient.setTimeOffset(offset);
  timeClient.update();

  rtc.setHours(timeClient.getHours());
  rtc.setMinutes(timeClient.getMinutes());
  rtc.setSeconds(timeClient.getSeconds());
}

bool isDST() {
  // Simple DST rules for North America:
  // DST starts on the second Sunday in March at 2:00 AM
  // DST ends on the first Sunday in November at 2:00 AM

  int month = rtc.getMonth();
  int day = rtc.getDay();
  int year = rtc.getYear() + 2000;

  Serial.print("Checking DST for date: ");
  Serial.print(year);
  Serial.print("-");
  Serial.print(month);
  Serial.print("-");
  Serial.println(day);

  if (month < 3 || month > 11) {
    return false;  // Not DST
  }
  if (month > 3 && month < 11) {
    return true;   // DST
  }

  int dayOfWeek = calculateDayOfWeek(day, month, year);

  if (month == 3) {
    return (day - dayOfWeek >= 7); // Second Sunday or later in March
  }
  if (month == 11) {
    return (day - dayOfWeek < 0);  // Before the first Sunday in November
  }

  return false;
}

// Zeller's Congruence algorithm to calculate the day of the week
int calculateDayOfWeek(int day, int month, int year) {
  if (month < 3) {
    month += 12;
    year -= 1;
  }
  int K = year % 100;
  int J = year / 100;
  int dayOfWeek = (day + (13 * (month + 1)) / 5 + K + K / 4 + J / 4 + 5 * J) % 7;
  return (dayOfWeek + 5) % 7 + 1; // Adjust to make Sunday = 1
}

void setRTCTime() {
  // Set date and time to June 27, 2024, 00:00:00
  rtc.setTime(0, 0, 0);  // Set time to 00:00:00
  rtc.setDate(27, 6, 24);  // Set date to 27 June 2024 (year is last two digits)
}
