// include the library code:
#include <LiquidCrystal.h>
#include <PubSubClient.h>
#include <Ethernet2.h>
#include <SPI.h>

EthernetClient ethClient;
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(9, 8, 5, 4, 3, 2);

// set up a constant for the tilt switch pin
const int switchPin = 6;
// named constant for the pin the sensor is connected to
const int sensorPin = A0;
// room temperature in Celsius
const float baselineTemp = 20.0;
float temperatureReadingsSet[100];
int temperatureIndexPosition = 0;
int backLight = 7;    // pin 13 will control the backlight

// variable to hold the value of the switch pin
int switchState = 0;
// variable to hold previous value of the switch pin
int prevSwitchState = 0;

byte mac[]    = {  0xA8, 0x60, 0xB6, 0x2C, 0x4A, 0x22 };
IPAddress server(192, 168, 0, 8);
const char *mqtt_server = "192.168.0.8";
const int mqtt_port = 11883;
const char *mqtt_client_name = "greenhouseClient1"; 

PubSubClient client(ethClient);  //instanciates client object
 
void setup() {
  // open a serial connection to display values
  Serial.begin(9600);

  // set up the switch pin as an input
  pinMode(switchPin, INPUT);
  pinMode(backLight, OUTPUT);

  // set up the number of columns and rows on the LCD
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("Read Room Temp:");

  lcd.setCursor(0, 1);
  
  client.setServer(mqtt_server,  mqtt_port);  
  Ethernet.begin(mac);

  // Allow the hardware to sort itself out
  delay(1500);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Attempt to connect
    if (client.connect(mqtt_client_name)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic","hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/**
 * Send the temperature reading to the MQTT Server and also display it on the LCD screen
 */
void loop() {

  if (!client.connected()) {
    reconnect();
    Serial.println("CONNNNNNNECT");
  }
  
  client.loop();
  
  int i = 0;
  int n = 99;
  float sum = 0.0, averageTemperature;

  // check the status of the switch
  switchState = digitalRead(switchPin);

  // read the value on AnalogIn pin 0 and store it in a variable
  int sensorVal = analogRead(sensorPin);
  
  // These constants, define values needed for the LDR readings and ADC
  #define LDR_PIN                   A5
  #define MAX_ADC_READING           1023
  #define ADC_REF_VOLTAGE           10.0
  #define REF_RESISTANCE            5030  // measure this for best results
  #define LUX_CALC_SCALAR           12518931
  #define LUX_CALC_EXPONENT         -1.405

  int   ldrRawData;
  float resistorVoltage, ldrVoltage;
  float ldrResistance;
  float ldrLux;
  
  // Perform the analog to digital conversion  
  ldrRawData = analogRead(LDR_PIN);
  
  // RESISTOR VOLTAGE_CONVERSION
  // Convert the raw digital data back to the voltage that was measured on the analog pin
  resistorVoltage = (float)ldrRawData / MAX_ADC_READING * ADC_REF_VOLTAGE;

  // voltage across the LDR is the 5V supply minus the 5k resistor voltage
  ldrVoltage = ADC_REF_VOLTAGE - resistorVoltage;
  
  // LDR_RESISTANCE_CONVERSION
  // resistance that the LDR would have for that voltage  
  ldrResistance = ldrVoltage/resistorVoltage * REF_RESISTANCE;
  
  // LDR_LUX
  ldrLux = LUX_CALC_SCALAR * pow(ldrResistance, LUX_CALC_EXPONENT);
 
  // print out the results
  // TODO: Review math for Lux values and send these values to MQTT Server
  Serial.print("LDR Raw Data   : "); Serial.println(ldrRawData);
  Serial.print("LDR Voltage    : "); Serial.print(ldrVoltage); Serial.println(" volts");
  Serial.print("LDR Resistance : "); Serial.print(ldrResistance); Serial.println(" Ohms");
  Serial.print("LDR Illuminance: "); Serial.print(ldrLux); Serial.println(" lux");

  // send the 10-bit sensor value out the serial port

  // convert the ADC reading to voltage
  float voltage = sensorVal * (5 / 1024.0);

  // Send the voltage level out the Serial port

  // convert the voltage to temperature in degrees C
  // the sensor changes 10 mV per degree
  // the datasheet says there's a 500 mV offset
  // ((voltage - 500 mV) times 100)
  float temperature = (voltage - .5) * 100;

  temperatureReadingsSet[temperatureIndexPosition] = temperature;
  
  if (temperatureIndexPosition == n) {
    for (i = 0; i < n; ++i) {
      sum += temperatureReadingsSet[i];
    }

    averageTemperature = sum / n;
    static char averageTemperatureAsString[15];
    dtostrf(averageTemperature, 2, 0, averageTemperatureAsString);
    char temperatureLcdOutput[10] = "";

    sprintf(temperatureLcdOutput, "%s Degrees", averageTemperatureAsString);
    memset(temperatureReadingsSet, 0, sizeof(temperatureReadingsSet));
    temperatureIndexPosition = 0;

    lcd.setCursor(0, 1);
    lcd.print(temperatureLcdOutput);
    client.publish("topic", temperatureLcdOutput);
  }

  if (switchState == 1) {
    digitalWrite(backLight, HIGH);
  } else {
    digitalWrite(backLight, LOW);
  }

  // save the current switch state as the last state
  prevSwitchState = switchState;
  temperatureIndexPosition++;
  delay(10);
}

