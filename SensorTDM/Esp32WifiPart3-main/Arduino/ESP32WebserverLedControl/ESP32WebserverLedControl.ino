// ---------------------------------------------------------------------------------------
//
// Code for a webserver on the ESP32 to control LEDs (device used for tests: ESP32-WROOM-32D).
// The code allows user to switch between three LEDs and set the intensity of the LED selected
//
// For installation, the following libraries need to be installed:
// * Websockets by Markus Sattler (can be tricky to find -> search for "Arduino Websockets"
// * ArduinoJson by Benoit Blanchon
//
// Written by mo thunderz (last update: 19.11.2021)
//
// ---------------------------------------------------------------------------------------

#include <WiFi.h>                                     // needed to connect to WiFi
#include <ESPAsyncWebServer.h>// needed to create a simple webserver (make sure tools -> board is set to ESP32, otherwise you will get a "WebServer.h: No such file or directory" error)
#include <WebSocketsServer.h>                         // needed for instant communication between client and server through Websockets
#include <ArduinoJson.h>                              // needed for JSON encapsulation (send multiple variables with one string)
#include <SPIFFS.h>
#include <Wire.h> // The wire library allows us to communicate with I2C/TWI devices. The MAX30102 sensors that we used are one of the many I2C serial devices.
#include <MAX30105.h> // The MAX 30105 library was used after several trial and error libraries like MAX30100.h and MAX30102.h. It gives the best results required for the project.
#define MULT_ADD 0x70 

 // Important onesx are getRed, getIR and getGreen

// SSID and password of Wifi connection:
const char* ssid = "LAPTOP-QPT3U3E2 5263";
const char* password = "h099|1N6";

//IPAddress local_IP(192,168,1,1);
//IPAddress gateway(192,168,1,2);
//IPAddress subnet(255,255,255,0);
void selectSens(byte ch); // Sensor Selecting Function declared at the end
// The String below "webpage" contains the complete HTML code that is sent to the client whenever someone connects to the webserver
// NOTE 27.08.2022: I updated in the webpage "slider.addEventListener('click', slider_changed);" to "slider.addEventListener('change', slider_changed);" -> the "change" did not work on my phone.
String webpage = "<!DOCTYPE html><html><head><title>Page Title</title></head><body style='background-color: #EEEEEE;'><span style='color: #003366;'><h1>LED Controller</h1><form> <p>Select LED:</p> <div> <input type='radio' id='ID_LED_0' name='operation_mode'> <label for='ID_LED_0'>LED 0</label> <input type='radio' id='ID_LED_1' name='operation_mode'> <label for='ID_LED_1'>LED 1</label> <input type='radio' id='ID_LED_2' name='operation_mode'> <label for='ID_LED_2'>LED 2</label> </div></form><br>Set intensity level: <br><input type='range' min='1' max='100' value='50' class='slider' id='LED_INTENSITY'>Value: <span id='LED_VALUE'>-</span><br></span></body><script> document.getElementById('ID_LED_0').addEventListener('click', led_changed); document.getElementById('ID_LED_1').addEventListener('click', led_changed); document.getElementById('ID_LED_2').addEventListener('click', led_changed); var slider = document.getElementById('LED_INTENSITY'); var output = document.getElementById('LED_VALUE'); slider.addEventListener('change', slider_changed); var Socket; function init() { Socket = new WebSocket('ws://' + window.location.hostname + ':81/'); Socket.onmessage = function(event) { processCommand(event); }; } function led_changed() {var l_LED_selected = 0;if(document.getElementById('ID_LED_1').checked == true) { l_LED_selected = 1;} else if(document.getElementById('ID_LED_2').checked == true) { l_LED_selected = 2;}console.log(l_LED_selected); var msg = { type: 'LED_selected', value: l_LED_selected};Socket.send(JSON.stringify(msg)); } function slider_changed () { var l_LED_intensity = slider.value;console.log(l_LED_intensity);var msg = { type: 'LED_intensity', value: l_LED_intensity};Socket.send(JSON.stringify(msg)); } function processCommand(event) {var obj = JSON.parse(event.data); var type = obj.type;if (type.localeCompare(\"LED_intensity\") == 0) { var l_LED_intensity = parseInt(obj.value); console.log(l_LED_intensity); slider.value = l_LED_intensity; output.innerHTML = l_LED_intensity;}else if(type.localeCompare(\"LED_selected\") == 0) { var l_LED_selected = parseInt(obj.value); console.log(l_LED_selected); if(l_LED_selected == 0) { document.getElementById('ID_LED_0').checked = true; } else if (l_LED_selected == 1) { document.getElementById('ID_LED_1').checked = true; } else if (l_LED_selected == 2) { document.getElementById('ID_LED_2').checked = true; }} } window.onload = function(event) { init(); }</script></html>";

int sensorData1 = 0; // Initializing the data of Sensor 1 to be printed.
int sensorData2 = 0; // Initializing the data of Sensor 2 to be printed.
int sensorData3 = 0;// Initializing the data of Sensor 3 to be printed.

MAX30105 sensor1; // sensor1, sensor2 and sensor3 are instances of MAX30105
MAX30105 sensor2; // This class declares address as 0x57 and has several functionalities
MAX30105 sensor3;

// global variables of the LED selected and the intensity of that LED
int LED_selected = 0;
int LED_intensity = 50;

const int sens_len = 200;
int sensor_vals[sens_len];

int sensor_vals1[sens_len];

int sensor_vals2[sens_len];

int interval = 50;
unsigned long previousMillis = 0;
// Initialization of webserver and websocket
AsyncWebServer server(80);                                 // the server uses port 80 (standard port for websites
WebSocketsServer webSocket = WebSocketsServer(81);    // the websocket uses port 81 (standard port for websockets

void setup() {
  Serial.begin(115200);  
  byte avgsampling = 16;    // Set the avgsampling at 16 (out of the available range of 1-32)
  int fs = 400;             // Set fs = 1/Ts (sampling Rate) at 400
  int PW = 411;             // Set the Pulse Width at 411 (out of some limited options)0
  byte Mode = 3;            // Set the LED to mode 3 which is Red + Green + IR
  byte LED = 0x1F;          // Set LED to 50 ma mode
  int adcRange = 8192;  

  Wire.begin(); // Begin Wire Transmission to setup the sensors
  selectSens(0); // Select Sensor 1
  byte SensAddr1 = 0x57; // MAX30102 is a sensor that has an address of 57H. Thus, bytes are created with address 57H
  if(!sensor1.begin(Wire,I2C_SPEED_FAST)) // If sensor is faulty, or connection are faulty
  {
    Serial.println("Sensor not detected! Check connections of the first sensor's connections "); // Raise error
    while (1); // Halt (run an infinite loop) because we aren't going to movew forward now
  }
  sensor1.setup(LED, avgsampling, Mode, fs, PW, adcRange); // If sensor 1 detected, setup the sensor with the modes declared

  selectSens(1); // Select Sensor 2
  byte SensAddr2 = 0x57; // Address 57H
  if (!sensor2.begin(Wire,I2C_SPEED_FAST)) // Same as before
  {
    Serial.println("Sensor not detected! Check connections of the second sensor's connections"); // Raise Error
    while (1); // Halt
  }
  sensor2.setup(LED, avgsampling, Mode, fs, PW, adcRange); // Setup Sensor 2


  selectSens(2); // Select Sensor 3
  byte SensAddr3 = 0x57; // Address 57H
  if (!sensor3.begin()) // If not detected
  {
    Serial.println("Sensor not detected! Check connections of the third sensor's connections"); // Raise Error
    while (1); // Infinite Loop
  }
  sensor3.setup(LED, avgsampling, Mode, fs, PW, adcRange); //Setup Sensor 3

  if(!SPIFFS.begin())
  {
    Serial.println("SPIFFS could not be initialised");
  }
 
  WiFi.begin(ssid, password);                         // start WiFi interface
  Serial.println("Establishing connection to WiFi with SSID: " + String(ssid));     // print SSID to the serial interface for debugging
 
  while (WiFi.status() != WL_CONNECTED) {             // wait until WiFi is connected
    delay(1000);
    Serial.print(".");
  }
  Serial.print("Connected to network with IP address: ");
  Serial.println(WiFi.localIP());                     // show IP address that the ESP32 has received from router
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {                               // define here wat the webserver needs to do
    request->send(SPIFFS, "/sensortest.html", "text/html");           //    -> it needs to send out the HTML string "webpage" to the client
  });       

  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404,"text/plain","File not found");
  });
  server.serveStatic("/", SPIFFS, "/");
  webSocket.begin();                                  // start websocket
  webSocket.onEvent(webSocketEvent);                  // define a callback function -> what does the ESP32 need to do when an event from the websocket is received? -> run function "webSocketEvent()"

  server.begin();  
}

void loop() {
  //server.handleClient();                              // Needed for the webserver to handle all clients
  webSocket.loop();    
  unsigned long now = millis();
  if((unsigned long)(now-previousMillis) > interval)
  {
    previousMillis = now;

    selectSens(0); // Select Sensor 1
    sensorData1 = sensor1.getIR(); // Read IR data of Sensor 1
    //Serial.print(sensorData1); // Serially Print the IR data acquired
    //Serial.print("  "); // Print two spaces
     for(int i=0;i<sens_len-1;i++)
    {
      sensor_vals[i] = sensor_vals[i+1];
    }
    sensor_vals[sens_len-1] = sensorData1;
    sendJsonArray("graph_update", sensor_vals);

    
    selectSens(1); // Select Sensor 2
    sensorData2 = sensor2.getIR(); // Read IR data of Sensor 2
    //Serial.print(sensorData2); // Serially Print the IR data acquired
    //Serial.print("  "); // Print two spaces
    for(int i=0;i<sens_len-1;i++)
    {
      sensor_vals1[i] = sensor_vals1[i+1];
    }
    sensor_vals1[sens_len-1] = sensorData2;
    sendJsonArray("graph_update", sensor_vals1);

    selectSens(2); // Select Sensor 3
    sensorData3 = sensor3.getIR(); // Read IR data of Sensor 3
    //Serial.print(sensorData3); // Print data
    //Serial.print("  "); // spaces
    for(int i=0;i<sens_len-1;i++)
    {
      sensor_vals2[i] = sensor_vals2[i+1];
    }
    sensor_vals2[sens_len-1] = sensorData3;

    Serial.println(sensor_vals2[sens_len-1]);
    sendJsonArray("graph_update", sensor_vals2);
    
  }
}

void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {      // the parameters of this callback function are always the same -> num: id of the client who send the event, type: type of message, payload: actual data sent and length: length of payload
  switch (type) {                                     // switch on the type of information sent
    case WStype_DISCONNECTED:                         // if a client is disconnected, then type == WStype_DISCONNECTED
      Serial.println("Client " + String(num) + " disconnected");
      break;
    case WStype_CONNECTED:                            // if a client is connected, then type == WStype_CONNECTED
      Serial.println("Client " + String(num) + " connected");
      
      // send LED_intensity and LED_select to clients -> as optimization step one could send it just to the new client "num", but for simplicity I left that out here
      sendJson("LED_intensity", String(LED_intensity));
      sendJsonArray("graph_update",sensor_vals);
      break;
    case WStype_TEXT:                                 // if a client has sent data, then type == WStype_TEXT
      // try to decipher the JSON string received
      StaticJsonDocument<200> doc;                    // create JSON container 
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      else {
        // JSON string was received correctly, so information can be retrieved:
        const char* l_type = doc["type"];
        const int l_value = doc["value"];
        Serial.println("Type: " + String(l_type));
        Serial.println("Value: " + String(l_value));

        // if LED_intensity value is received -> update and write to LED
        if(String(l_type) == "LED_intensity") {
          LED_intensity = int(l_value);
          sendJson("LED_intensity", String(l_value));
        }
      }
      Serial.println("");
      break;
  }
}

// Simple function to send information to the web clients
void sendJson(String l_type, String l_value) {
    String jsonString = "";                           // create a JSON string for sending data to the client
    StaticJsonDocument<200> doc;                      // create JSON container
    JsonObject object = doc.to<JsonObject>();         // create a JSON Object
    object["type"] = l_type;                          // write data into the JSON object -> I used "type" to identify if LED_selected or LED_intensity is sent and "value" for the actual value
    object["value"] = l_value;
    serializeJson(doc, jsonString);                // convert JSON object to string
    webSocket.broadcastTXT(jsonString);               // send JSON string to all clients
}


void sendJsonArray(String l_type, int array_values[]) {
    String jsonString = ""; // create a JSON string for sending data to the client
    const size_t CAPACITY = JSON_ARRAY_SIZE(sens_len)+100;
    StaticJsonDocument<CAPACITY> doc;                      // create JSON container
    JsonObject object = doc.to<JsonObject>();         // create a JSON Object
    object["type"] = l_type; // write data into the JSON object -> I used "type" to identify if LED_selected or LED_intensity is sent and "value"for the actual value
    JsonArray value = object.createNestedArray("value");
    for(int i=0;i<sens_len;i++)
    {
      value.add(array_values[i]);
    }
//    JsonArray value1 = object.createNestedArray("value1");
//    for(int i=0;i<sens_len;i++)
//    {
//      value1.add(array_values1[i]);
//    }
//    JsonArray value2 = object.createNestedArray("value2");
//    for(int i=0;i<sens_len;i++)
//    {
//      value2.add(array_values2[i]);
//    }
    serializeJson(doc, jsonString);                // convert JSON object to string
    webSocket.broadcastTXT(jsonString);               // send JSON string to all clients
}

void selectSens(byte ch) 
{
  Wire.beginTransmission(MULT_ADD); // Begin Wire Transmission for Multiplexer Address
  Wire.write(1 << ch); // Bit-shift operator, shifts 1 by 'ch' bits
  Wire.endTransmission(); // End transmission of wire
  delay(10); // Delay for switch settling
}
