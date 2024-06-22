#include <Wire.h> // The wire library allows us to communicate with I2C/TWI devices. The MAX30102 sensors that we used are one of the many I2C serial devices.
#include <MAX30105.h> // The MAX 30105 library was used after several trial and error libraries like MAX30100.h and MAX30102.h. It gives the best results required for the project.
#define MULT_ADD 0x70 // Multiplexer was required for this project and it's value is defined here as 0x70 or 70H

int sensorData1 = 0; // Initializing the data of Sensor 1 to be printed on-screen serially
int sensorData2 = 0; // Initializing the data of Sensor 2 to be printed
int sensorData3 = 0;// Initializing the data of Sensor 3 to be printed

MAX30105 sensor1; // sensor1, sensor2 and sensor3 are instances of MAX30105
MAX30105 sensor2; // This class declares address as 0x57 and has several functionalities
MAX30105 sensor3; // Important onesx are getRed, getIR and getGreen

void selectSens(byte ch); // Sensor Selecting Function declared at the end

void setup() 
{
  Serial.begin(115200);     // Begin Serial Communication at 115200 baud
  byte avgsampling = 16;    // Set the avgsampling at 16 (out of the available range of 1-32)
  int fs = 400;             // Set fs = 1/Ts (sampling Rate) at 400
  int PW = 411;             // Set the Pulse Width at 411 (out of some limited options)0
  byte Mode = 3;            // Set the LED to mode 3 which is Red + Green + IR
  byte LED = 0x1F;          // Set LED to 50 ma mode
  int adcRange = 8192;      // Set the adcRange to 8192 (2^13)

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
}

void loop() 
{


  selectSens(0); // Select Sensor 1
  sensorData1 = sensor1.getIR(); // Read IR data of Sensor 1
  Serial.print(sensorData1); // Serially Print the IR data acquired
  Serial.print("  "); // Print two spaces

  selectSens(1); // Select Sensor 2
  sensorData2 = sensor2.getIR(); // Read IR data of Sensor 2
  Serial.print(sensorData2); // Serially Print the IR data acquired
  Serial.print("  "); // Print two spaces

  selectSens(2); // Select Sensor 3
  sensorData3 = sensor3.getIR(); // Read IR data of Sensor 3
  Serial.print(sensorData3); // Print data
  Serial.print("  "); // spaces

  Serial.println(); // Carriage Return. Thus, the code prints the data of all sensors after each loop.
}

void selectSens(byte ch) 
{
  Wire.beginTransmission(MULT_ADD); // Begin Wire Transmission for Multiplexer Address
  Wire.write(1 << ch); // Bit-shift operator, shifts 1 by 'ch' bits
  Wire.endTransmission(); // End transmission of wire
  delay(10); // Delay for switch settling
}
