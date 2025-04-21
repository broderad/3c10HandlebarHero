#define PORTRAIT  0
#define LANDSCAPE 1
#define USE_XPT2046   0
#define USE_LOCAL_KBV 1
#define TOUCH_ORIENTATION  PORTRAIT

// Including the relevant libraries ... including the MCUFriend fork
#include <SPI.h>
#include <WiFiS3.h>
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>  

// WIFI Stuff ---------------------------------------------------------------------------------------
int val1 = 0;
int prev_val1 = 0;
float val2 = 0;
float prev_val2 = 0;
int val3 = 0;
int prev_val3 = 0;

const char* ssid = "Bike Computer";
const char* password = "password";
WiFiServer server(8080);
WiFiServer server2(9090);

//--------------------------------------------------------------------------------------------------
//Initialising the display and touchscreen
MCUFRIEND_kbv tft;

#if defined(ESP32)
int XP = 27, YP = 4, XM = 15, YM = 14;
#else
int XP = 7, YP = A2, XM = A1, YM = 6;
#endif
TouchScreen ts(XP, YP, XM, YM, 300);
TSPoint tp;

// Colour declaration
#define WHITE 0xFFFF
#define RED   0xF800
#define BLUE  0x001F
#define GREEN 0x07E0
#define BLACK 0x0000
#define YELLOW 0xFFE0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define GREY 0x7BEF
#define LIGHT_GREY 0xD3D3D3 

int tot_width = 320, tot_height = 480; 
int screen_number = 0;  
int convert_M2M = 0;  

long lastTime = 0;

// Conversion for meters to miles from the web
const float metersToMiles = 0.00062137119223;

// Function to convert meters to miles
float changeM2M(float meters) {
    return meters * metersToMiles;
}

// Used to return a time 
String get_time() {     
   return "03:15 PM";
}

// Drawing the top bar
void topbar() {
    // Draw from origin across the screen width with a depth of 30 in colour light grey
    tft.fillRect(0, 0, tot_width, 30, LIGHT_GREY);
    String time = get_time();
    tft.setTextColor(BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 10);  
    tft.print(time);
    
    // Displaying location within the bar
    String location = "Dublin";
    int location_ = 200; 
    tft.setTextColor(BLACK);
    tft.setTextSize(2);
    tft.setCursor(location_, 10); 
    tft.print(location);

    // Draw the battery icon...
    tft.setTextColor(BLACK);  
    tft.drawRect(290, 10, 20, 10, BLACK);
    tft.fillRect(290 + 22, 10 + 2, 5, 6, BLACK);  
    tft.fillRect(290 + 2, 10 + 2, 16, 6, BLACK); 
}

// Conversion button 
void setbut() {
    int but_w = 160;
    int but_h = 40;
    int but_x = (tot_width - but_w) / 2;  // Fixed typo here, `buth` should be `but_w`
    int but_y = tot_height - 40;
    
    // Background
    tft.fillRect(but_x, but_y, but_w, but_h, LIGHT_GREY);  
    // Bordered
    tft.drawRect(but_x, but_y, but_w, but_h, BLACK);    
    // Text colour  
    tft.setTextColor(BLACK);
    tft.setTextSize(2);
    String text;
         if (convert_M2M == 0) {
            text = "m -> miles";
        } else {
            text = "miles -> m";
        } 
    tft.setCursor(but_x + 10, but_y + 10);  
    tft.print(text);
}

// Input box
void inputbox(const char* writing, float input, int y_value, const char* measured_unit) {

    tft.setTextColor(WHITE);
    tft.setTextSize(3);  
    int16_t x1, y1;
    uint16_t w, h;

    // Bounding writing
    tft.getTextBounds(writing, 0, 0, &x1, &y1, &w, &h);
    int writing_w = w;
    int writing_x = (tot_width - writing_w) / 2;
    tft.setCursor(writing_x, y_value);  
    tft.print(writing);

    // Box dimensions
    int box_x = (tot_width - 120) / 2;  
    int box_y = y_value + 40;
    int box_w = 120;  
    int box_h = 50;  

    // Create box
    tft.drawRect(box_x, box_y, box_w, box_h, GREY);  
    tft.setTextColor(WHITE);
    tft.setTextSize(3);  
    tft.setCursor(box_x + 10, box_y + 10);  
    tft.print(input);

    // Displaying measured value within the box
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    int unit_xval = box_x + box_w + 10;  
    tft.setCursor(unit_xval, box_y + 20);
    tft.print(measured_unit);
}
 //Function to overwrite the value displayed on screen.
 //Draws a black box over the current value and then writes the new value
void updateValues() {
    int box_x = (tot_width - 120) / 2;  
    int box_y = 140;
    int box_w = 120;  
    int box_h = 50;  
    tft.fillRect(box_x, box_y, box_w, box_h, BLACK);
        if (screen_number == 0) {
            //Display Heart Rate
        inputbox("Heart Rate:", val3, 100, "bmp");

    } 
    // Ascent screen display
    else if (screen_number == 1) {
        float ascent_in = val1;  // gradient value
        if (convert_M2M == 1) {
            ascent_in = ascent_in;  
        }
        String acsent_uni;
        if (convert_M2M == 0) {
            acsent_uni = "%";
        } else {
            acsent_uni = "%";
        }
        inputbox("Ascent:", ascent_in, 100, acsent_uni.c_str());
    }
    // Speed screen display
    else if (screen_number == 2) {
        float speed_in = val2;  // Speed value
        if (convert_M2M == 1) {
            speed_in = changeM2M(speed_in);  
        }
        String speed_uni;
        if (convert_M2M == 0) {
            speed_uni = "m";
        } else {
            speed_uni = "miles";
        }
        inputbox("Speed:", speed_in, 100, speed_uni.c_str());
    }
    }

// Button press to change conversion
void buttonpress_change() {
    convert_M2M = !convert_M2M;
}

//draw the heart
void heart(int x_value, int y_value) {
    int shape_size = 52;
    int circle_size = shape_size - 8;  
    tft.fillCircle(x_value - circle_size, y_value, circle_size, RED); 
    tft.fillCircle(x_value + circle_size, y_value, circle_size, RED);  
    int tri_h = shape_size * 2;  
    int tri_y_value = y_value + circle_size - 35;  
    tft.fillTriangle(x_value - shape_size * 1.64, tri_y_value, x_value + shape_size * 1.66, tri_y_value, x_value, tri_y_value + tri_h, RED);  
}

//drawing thr right anle triangle
void right_angletriangle(int x_value, int y_value) {
    int shape_size = 85;
    tft.fillTriangle(x_value - 10, y_value, x_value + shape_size * 1.75, y_value, x_value, y_value - shape_size,GREEN);                              
}

//draw arrow
void arrow(int x_value, int y_value) {  
    int shape_size = 20;
    tft.fillRect(x_value - shape_size / 8, y_value - shape_size * 2, shape_size / 2, shape_size * 2, WHITE);  
    tft.fillTriangle(x_value - shape_size, y_value - shape_size * 2, x_value + shape_size, y_value - shape_size * 2,               
                     x_value, y_value - shape_size * 4, WHITE);                                                       
}

// Speed graphic
void speedgraphic(int x_value, int y_value ) {
  int circle_size = 15;
    int spacing = 40;  
    uint16_t circleColors[] = {GREEN, GREEN, YELLOW, YELLOW, YELLOW, RED, RED};

    for (int i = 0; i < 7; i++) {
        uint16_t circleColor = circleColors[i];
        tft.fillCircle(x_value + i * spacing, y_value, circle_size, circleColor);
        tft.drawCircle(x_value + i * spacing, y_value, circle_size, WHITE);
    }
}

// create bike icon
void bike_icon(int x_value, int y_value) {
    uint16_t white = WHITE;
    uint16_t black = BLACK;
    int wheel_size = 20;

    int bike_frame = y_value + wheel_size + 10;  
    tft.fillCircle(x_value, y_value, wheel_size, black);  
    tft.drawCircle(x_value, y_value, wheel_size, white);  
    //draw wheels 
    tft.fillCircle(x_value + 2 * wheel_size + 20, y_value, wheel_size, black);  
    tft.drawCircle(x_value + 2 * wheel_size + 20, y_value, wheel_size, white);  
    //draw frame
    tft.drawLine(x_value, y_value, x_value + 2 * wheel_size + 20, y_value, white);
    tft.drawLine(x_value, y_value, x_value + wheel_size + 10, bike_frame - 60, white); 
    tft.drawLine(x_value + 2 * wheel_size + 20, y_value, x_value + wheel_size + 10, bike_frame - 60, white);  

    //draw seat and handle
    tft.fillRect(x_value + wheel_size - 5, bike_frame - 60, 20, 10, white); 
    tft.drawLine(x_value + 2 * wheel_size + 20, y_value, x_value + 2 * wheel_size + 20, y_value - 40, white); 

    tft.drawLine(x_value + 2 * wheel_size + 20, y_value - 40, x_value + 2 * wheel_size + 40, y_value - 50, white);  
    tft.drawLine(x_value + 2 * wheel_size + 20, y_value - 40, x_value + 2 * wheel_size, y_value - 50, white);  

}

// display on the screen function ... 
void display(int screen_num) {
    // Clear screen and draw the top bar
    tft.fillScreen(BLACK);
    topbar();

    // Heart rate screen display
    if (screen_num == 0) {
        // 32 represents the input and can be changed
        // as a float or something here
        inputbox("Heart Rate:", val3, 100, "bmp");
        heart(160, 270);  

    } 
    // Ascent screen display
    else if (screen_num == 1) {
        float ascent_in = val1;  // Example value for ascent
        if (convert_M2M == 1) {
            ascent_in = ascent_in;  
        }
        String acsent_uni;
        if (convert_M2M == 0) {
            acsent_uni = "%";
        } else {
            acsent_uni = "%";
        }
        inputbox("Ascent:", ascent_in, 100, acsent_uni.c_str());
        right_angletriangle(85, 320);   
        arrow(240, 320);
    }
    // Speed screen display
    else if (screen_num == 2) {
        float speed_in = val2;  // Example speed value (in meters per second)
        if (convert_M2M == 1) {
            speed_in = changeM2M(speed_in);  
        }
        String speed_uni;
        if (convert_M2M == 0) {
            speed_uni = "m";
        } else {
            speed_uni = "miles";
        }
        inputbox("Speed:", speed_in, 100, speed_uni.c_str());
        speedgraphic(40, 250);
        bike_icon(130, 360); 
    }

    setbut();
}


void setup() {
    uint16_t ID = tft.readID();
    if (ID == 0xD3D3) ID = 0x9486;
    tft.begin(ID);
    Serial.begin(9600);
    while (!Serial);
    tft.setRotation(TOUCH_ORIENTATION);
    tft.fillScreen(BLACK);
    display(screen_number);

    //Doing WiFi stuff
    WiFi.beginAP(ssid, password);

    server.begin();
    server2.begin();
    Serial.print("Server IP: ");
    Serial.println(WiFi.localIP());
}

// Function to check if the touchscreen is pressed
bool ISPRESSED() {
    tp = ts.getPoint();
    pinMode(YP, OUTPUT);
    pinMode(XM, OUTPUT);
    return tp.z > 200;
}
void loop() {
    //Connect the the two perhiperal devices
    WiFiClient client = server.available();
    WiFiClient client2 = server2.available();
    //If data is available then handle it
    if (client || client2) {
        Serial.println("Client connected");
    
    if (client.connected() || client2.connected()) {
      if (client.available() || client2.available()) {
        //Capturing data from the strings
        String data = client.readStringUntil('\n');
        String data2 = client2.readStringUntil('\n');
        data.trim();
        data2.trim();
        
        // Parse integers
        int commaIndex = data.indexOf(',');
        String clientID = data.substring(0, commaIndex);
        String values = data.substring(commaIndex + 1);
        if (clientID == "NANO1") {
          int valuesComma = values.indexOf(',');
          String valone = values.substring(0, valuesComma);
          val1 = valone.toInt();
          Serial.println(val1);
          String valtwo = values.substring(valuesComma + 1);
          Serial.println(val2);
          val2 = valtwo.toFloat();
          
          Serial.print("Received: ");
          Serial.print(val1);
          Serial.print(", ");
          Serial.println(val2);
        } else {
          val3 = data2.toInt();
          Serial.print("Nano2: ");
          Serial.println(val3);
        }
      }
    }
  }
    if (ISPRESSED()) {
        int press_x = tp.x;
        int press_y = tp.y;

        if (TOUCH_ORIENTATION == PORTRAIT) {
          //portrait mapping
            press_x = map(press_x, 120, 900, 0, tot_width);  
            press_y = map(press_y, 120, 900, 0, tot_height); 
        } else {
            press_x = map(press_x, 120, 900, 0, tot_height);
            press_y = map(press_y, 120, 900, 0, tot_width);
        }

        // is a touch is detected in the botton of the screen, change units M2M
        if (press_x > 9 * tot_width / 10) {
            buttonpress_change();
            display(screen_number);
        }

        // change screen with touch
        if (press_x >= 0 && press_x < tot_width && press_y >= 0 && press_y < tot_height) {
            screen_number = (screen_number + 1) % 3;
            display(screen_number); 
        }

        delay(400); //avoiding multiple screen changes
    }
    if(millis() > lastTime + 2000) {
        //Update values every 2 seconds
        updateValues();
        lastTime = millis();
    }
}
