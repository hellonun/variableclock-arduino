#include <WiFiNINA.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

const char* ssid     = " ";
const char* password = " ";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // Adjust the time offset as needed

void setup() {
  Serial.begin(115200);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.print(" ...");
    WiFi.begin(ssid, password);
    delay(5000);
  }
  Serial.println("Connected to WiFi");
  
  timeClient.begin();
}

void loop() {
  timeClient.update();
  Serial.println(timeClient.getFormattedTime());
  delay(1000); // Update every second
}
