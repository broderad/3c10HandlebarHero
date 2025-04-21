#include <SPI.h>
#include <WiFiNINA.h>
#include "LSM6DS3.h"
#include "Wire.h"

//Variables for recording speed
long currentTime = 0;
long speedTime = 0;

//Create a instance of class LSM6DS3
LSM6DS3 myIMU(I2C_MODE, 0x6A);    //I2C device address 0x6A
const char* ssid = "Bike Computer";
const char* pass = "password";
const char* serverIP = "192.168.4.1";
const int port = 8080;
//Setting up wifi stuff
WiFiClient client;
int val1 = 0;
int val2 = 0;

const int INTERRUPT_PIN = 9;
volatile int no_rotations = 0;
//Interuppt function
void countRotation() {
  no_rotations++;
}

void setup() {
  Serial.begin(9600);
  // Initialize WiFI
  WiFi.begin(ssid, pass);
  Serial.println("\nConnected to WiFi");
    if (myIMU.begin() != 0) {
        Serial.println("Device error");
    } else {
        Serial.println("Device OK!");
    }
    //Attach interrupt
    pinMode(INTERRUPT_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), countRotation, RISING);
}

//Function for calculating speed
float calc_speed() {
  long elapsed_time = millis() - speedTime; //Find time since last measurement
  // Serial.println(elapsed_time);
  float speed = (no_rotations * 2.2) / (elapsed_time/1000); //Calculate speed using circumfrence of bike wheel
  speedTime = millis();
  no_rotations = 0; //Reset number of rotations
  return speed;
}


void loop() {
    if(!client.connected()) {
      client.connect(serverIP, port); //If not connect to central unit then connect
    }

    // While the central is connected, keep updating the characteristic
    if (client.connected() && millis() > (currentTime+3500)) {
      currentTime = millis();
      //Get measurements from accelerometer
  float X = myIMU.readFloatAccelX();
    float Y = myIMU.readFloatAccelY();
    float Z = myIMU.readFloatAccelZ();
    float Xm = X * 9.433;
    float Ym = Y * 9.433; //Calibartion coefficient to transform to m/s^2
    float Zm = Z * 9.433;
    float angle = atan(Y/Z) * 180/M_PI; //Calculating the angle
    angle = -1 * angle; //Fliping to maintain orientation
    Serial.print("Current Angle: ");
    Serial.println(angle);
    float gradient = ((angle / 90) * 100); //Gradient percentage is how close we are to vertical
    Serial.print("Current Gradient:");
    Serial.println(gradient, 0);
    Serial.print("Number of Rotations: ");
    Serial.println(no_rotations);
    float speed = calc_speed();
    Serial.print("Speed: ");
    Serial.println(speed);
    String message = "NANO1," + String(gradient) + "," + String(speed) + "\n"; //Send formatted string over WiFi
    client.print(message);
    Serial.print("Sent: ");
    Serial.println(message);
    }
  }