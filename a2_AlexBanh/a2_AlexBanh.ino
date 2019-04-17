// Alex Banh, ajb9702 HCDE 440 sp19 a2
// adafruitIO url: https://io.adafruit.com/wow1881/dashboards/hcde-440-a2

//MPL115A2 required libraries
#include <Wire.h>
#include <Adafruit_MPL115A2.h>
//OLED SSD1306 128x32 i2c required libraries
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//Libraries for API calls with the ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h> //provides the ability to parse and construct JSON objects
#include <Arduino.h>

#include "config.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     13 // Reset pin # (or -1 if sharing Arduino reset pin)


#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

typedef struct { //A new data type to hold information from our International Space Station API
  String ln; //Current longitude of the ISS
  String lt; //Current latitude of the ISS
  int numberPpl; //Current number of people on the ISS
  String people[6]; //String array holding the names of people currently on the ISS
} ISSData;

//Instantiating our data type
ISSData ISSinfo;

// Initialize MPL115A2 sensor
Adafruit_MPL115A2 mpl115a2;

// Initialize the OLED display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Initialize the adafruitIO feed for temperature
AdafruitIO_Feed *switchIO = io.feed("switch");

AdafruitIO_Feed *pressure = io.feed("pressure");


void setup() {
  // put your setup code here, to run once:
    Serial.begin(115200);

    // Prints file name and compile time
  Serial.print("This board is running: ");
  Serial.println(F(__FILE__));
  Serial.print("Compiled: ");
  Serial.println(F(__DATE__ " " __TIME__));
  
    mpl115a2.begin();

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
      Serial.println(F("SSD1306 allocation failed"));
      for(;;); // Don't proceed, loop forever
    }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
  
  // Code to connect to the wifi network to allow for API calls
  Serial.print("Connecting to "); 
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(); Serial.println("WiFi connected"); Serial.println();

    // connect to io.adafruit.com
  Serial.print("Connecting to Adafruit IO");
  io.connect();

  // wait for a connection
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());

  switchIO->onMessage(handleMessage);
}

void loop() {

  // io.run() keeps the client connected to
  // io.adafruit.com and processes any incoming data.
  io.run();

  // ======== START MPL115A2 =========

  // Variables initalized to store the current pressure and temperature
  float pressureKPA = 0, temperatureC = 0;    

  // Gets and sets both the pressure and temperature variables with data from the MPL115A2 sensor
  mpl115a2.getPT(&pressureKPA,&temperatureC);

  // Prints the currently stored values for pressure and temperature
  Serial.print("Pressure (kPa): "); Serial.print(pressureKPA, 4); Serial.print(" kPa  ");
  Serial.print("Temp (*C): "); Serial.print(temperatureC, 1); Serial.println(" *C both measured together");

  // Sends the current pressure value to the "pressure" feed on adafruitIO
  pressure->save(pressureKPA);
  
  // ======== END MPL115A2 =========
  delay(2000);

}
// handleMessage executes when a message is received from our adafruitIO dashboard
void handleMessage(AdafruitIO_Data *data) {
  //Method call to get information on the current position of the ISS
  getPeople();
  
  // Clears the current display
  display.clearDisplay();

  // Sets some display options
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  // Prints the current number of people in space to the OLED screen
  display.print("There are currently ");
  display.print((String)ISSinfo.numberPpl + " people in space.");
  display.print("The pressure up there is about one trillionth (or less) of the current pressure here.");

  // Tells the display to display what is buffered 
  display.display();
}

// getPeople populates our ISS data type with information on the number of people currently in space and who they are
void getPeople() {
  HTTPClient theClient;
  theClient.begin("http://api.open-notify.org/astros.json"); //return current ISS people data as a JSON object
  int httpCode = theClient.GET();
  if (httpCode > 0) {
    if (httpCode == 200) {
      DynamicJsonBuffer jsonBuffer;
      String payload = theClient.getString();
      JsonObject& root = jsonBuffer.parse(payload);
      // Test if parsing succeeds.
      if (!root.success()) {
        Serial.println("parseObject() failed");
        Serial.println(payload);
        return;
      }
      //Populate some of the values inside our ISSData object. We refer to the object by it's instantiated name (ISSinfo)
      ISSinfo.numberPpl = root["number"];

      //For the number of people currently in space, iterate over the JSON object to retrieve their names
      for (int i = 0; i < ISSinfo.numberPpl; i++) {
        ISSinfo.people[i] = root["people"][i]["name"].as<String>();
      }
    } else {
      Serial.println("Something went wrong with connecting to the endpoint.");
    }
  }
}
