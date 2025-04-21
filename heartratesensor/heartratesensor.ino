#include <SPI.h>
#include <WiFiNINA.h>

// Variable to broadcast
int myVariable = 0;


long currentTime = millis();

const char* ssid = "Bike Computer";
const char* pass = "password";
const char* serverIP = "192.168.4.1";
const int port = 9090;

WiFiClient client;

// Pulse Sensor BPM Monitor
const int pulsePin = A7;  // Pulse sensor connected to analog pin A7

// Variables
int signal1;               // Raw analog signal1 from sensor
int threshold;            
unsigned long lastBeatTime = 0;
int beatCount = 0;
bool beatDetected = false;
int bpm = 0;
const int sampleWindow = 5000; // 5 second sample window for BPM calculation

void setup() {
  Serial.begin(9600);
  threshold = 800; //Set to 800 for good value
  WiFi.begin(ssid, pass);
  Serial.println("\nConnected to WiFi");
}

void loop() {
  unsigned long startTime = millis();
  unsigned long sampleStart = millis();
  
  // Sample for 10 seconds to calculate BPM
  while (millis() - sampleStart < sampleWindow) {
    signal1 = analogRead(pulsePin);
    
    // Detect heartbeat - signal1 crosses threshold
    if (signal1 > threshold && !beatDetected) {
      beatDetected = true;
      unsigned long currentTime = millis();
      
      // Only count if it's not noise (minimum 200ms between beats)
      if (lastBeatTime == 0 || (currentTime - lastBeatTime > 200)) {
        beatCount++;
        lastBeatTime = currentTime;
      }
    }
    
    // Reset detection when signal1 falls below threshold
    if (signal1 < threshold && beatDetected) {
      beatDetected = false;
    }
    
    delay(10);
  }
  
  // Calculate BPM (beats per minute)
  if (beatCount > 1) { // Need at least 2 beats to calculate interval
    unsigned long elapsedTime = millis() - startTime;
    bpm = (beatCount * 60000) / elapsedTime;
    
    Serial.print("BPM: ");
    Serial.println(bpm/2);
  } else {
    Serial.println("No pulse detected or not enough beats");
  }
  
  // Reset counters for next sample window
  beatCount = 0;
  lastBeatTime = 0;
  // Wait to connect to central
  if(!client.connected()) {
    client.connect(serverIP, port);
  }

    // While the central is connected, keep updating the data
    if (client.connected() && millis() > (currentTime+5000)) {
      currentTime = millis();
        
        String message = String((bpm/2)) + "\n";
      client.print(message); //Send data to the central screen as a string
      // Print the value to the Serial Monitor for debugging
      Serial.print("Broadcasting value: ");
      Serial.println(message);
    }
}