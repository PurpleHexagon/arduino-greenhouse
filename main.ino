// include the library code:
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// set up a constant for the tilt switch pin
const int switchPin = 6;
// named constant for the pin the sensor is connected to
const int sensorPin = A0;
// room temperature in Celsius
const float baselineTemp = 20.0;
float temperatureReadingsSet[100];
int temperatureIndexPosition = 0;
int backLight = 13;    // pin 13 will control the backlight

// variable to hold the value of the switch pin
int switchState = 0;
// variable to hold previous value of the switch pin
int prevSwitchState = 0;

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
}

void loop() {
  int i = 0;
  int n = 99;
  float sum = 0.0, averageTemperature;

  // check the status of the switch
  switchState = digitalRead(switchPin);

  // read the value on AnalogIn pin 0 and store it in a variable
  int sensorVal = analogRead(sensorPin);

  // send the 10-bit sensor value out the serial port
  Serial.print("sensor Value: ");
  Serial.print(sensorVal);

  // convert the ADC reading to voltage
  float voltage = (sensorVal / 1024.0) * 5.0;

  // Send the voltage level out the Serial port
  Serial.print(", Volts: ");
  Serial.print(voltage);

  // convert the voltage to temperature in degrees C
  // the sensor changes 10 mV per degree
  // the datasheet says there's a 500 mV offset
  // ((voltage - 500 mV) times 100)
  Serial.print(", degrees C: ");
  float temperature = (voltage - .5) * 100;
  temperatureReadingsSet[temperatureIndexPosition] = temperature;
  Serial.println(temperature);
  Serial.println(temperatureIndexPosition);

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
  }

  if (switchState == 1) {
    digitalWrite(backLight, HIGH);
  } else {
    digitalWrite(backLight, LOW);
  }

  // save the current switch state as the last state
  prevSwitchState = switchState;
  temperatureIndexPosition++;
  delay(100);
}
