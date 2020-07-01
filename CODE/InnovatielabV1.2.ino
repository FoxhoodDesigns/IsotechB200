//InovatieLab Arduino ADC demonstratie Code
//Geschreven: Kwint van den Berg. 2020

#include <SPI.h>
#include <SD.h>
File myFile;

// MCP3102
#define DAT  12  // SPI MISO Pin
#define CLK  13  // SPI Clock Pin
#define CS   8   // SPI SS Pin (Chip Select)

// ADG406 Multiplexer pins 
#define PIN_MULPLEX_EN A2
#define PIN_MULPLEX_S0 7
#define PIN_MULPLEX_S1 A1
#define PIN_MULPLEX_S2 A3
#define PIN_MULPLEX_S3 A0

//Weerstand PWM pins
#define PIN_PWM1    3
#define PIN_PWM2    5
#define PIN_PWM3    6

//Weerstand GAIN pins
#define PIN_GAIN1   A5
#define PIN_GAIN2   A4
#define PIN_STATE   2

//Analog Reference voltage in Millivolts (5000 for Arduino 5V, 3300 for 3.3V or any own value if using external Reference generation. 
#define ADCREF   5000

//________________________________________________________Configuration________________________________________________________________

#define PRINT  0      // text = 1 plot = 0
#define SHOWCHANNELS  0x01FF        // 0x0000 .. 0x01FF


#define PWM1  10     // 1..100 % Weerstand
#define PWM2  10     // 1..100 % Weerstand
#define PWM3  10     // 1..100 % Weerstand

#define GAIN1  LOW    // 0 of 1
#define GAIN2  LOW    // 0 of 1
#define STATE  1      // 0 of 1   0 = manual 1 = Software 

//___________________________________________________________________________________________________________________________________

void setup()
{
  int val;
  Serial.begin(115200);

  //Check if SD present, Else do not continue
  if (PRINT) Serial.print("Initializing SD card...");
  if (!SD.begin(4))
  {
    if (PRINT) Serial.println("initialization failed!");
    while (1);
  }
  if (PRINT) Serial.println(" initialization done.");

  
  myFile = SD.open("output.csv", FILE_WRITE); // Generate File
  if (myFile)
  {
    if (PRINT) Serial.print("Writing to: output.csv");
    delay(250);
  }
  else
  {
    // if the file didn't open, print an error:
    if (PRINT) Serial.println("error opening output.csv");
  }

   //Intialize SPI bus for ADC
  SPI.beginTransaction(SPISettings(1500000, MSBFIRST, SPI_MODE0));
  SPI.begin();

  pinMode(DAT, INPUT);
  pinMode(CS, OUTPUT);

  digitalWrite(CS, LOW);  // Cycle CS pin to ensure PWR ON state is high.
  digitalWrite(CS, HIGH);
  digitalWrite(CLK, LOW);  // Turn clck low

  //Define Multiplexer outputs as such.
  pinMode(PIN_MULPLEX_EN, OUTPUT);      // sets the digital pin  as output
  pinMode(PIN_MULPLEX_S0, OUTPUT);      // sets the digital pin  as output
  pinMode(PIN_MULPLEX_S1, OUTPUT);      // sets the digital pin  as output
  pinMode(PIN_MULPLEX_S2, OUTPUT);      // sets the digital pin  as output
  pinMode(PIN_MULPLEX_S3, OUTPUT);      // sets the digital pin  as output

  setMultiplexer(0);                    // Set multiplex to 0

  // set PWM (remaps percentage to 8-bit value)
  val = map(PWM1, 0, 100, 0, 255);
  analogWrite(PIN_PWM1, val);
  val = map(PWM2, 0, 100, 0, 255);
  analogWrite(PIN_PWM2, val);
  val = map(PWM3, 0, 100, 0, 255);
  analogWrite(PIN_PWM3, val);

  // setting state and gain
  pinMode(PIN_GAIN1, OUTPUT);
  pinMode(PIN_GAIN2, OUTPUT);
  pinMode(PIN_STATE, OUTPUT);
  digitalWrite(PIN_GAIN1, GAIN1);
  digitalWrite(PIN_GAIN2, GAIN2);
  digitalWrite(PIN_STATE, STATE);
}

//Sets all relevant output pins for multiplexer according to address given
void setMultiplexer(byte input)
{
  digitalWrite(PIN_MULPLEX_EN, 0);  // Disable
  delayMicroseconds(1);

  digitalWrite(PIN_MULPLEX_S0, input & 0x01);  // sets the S0
  digitalWrite(PIN_MULPLEX_S1, input & 0x02);  // sets the S0
  digitalWrite(PIN_MULPLEX_S2, input & 0x04);  // sets the S0
  digitalWrite(PIN_MULPLEX_S3, input & 0x08);  // sets the S0

  digitalWrite(PIN_MULPLEX_EN, 1);  //  Enable
  delayMicroseconds(10);
}

float readoutMP3201(void)
{
  unsigned int reading = 0;      // To be measured reading

  digitalWrite(CS, LOW);         //Select MCP3102 for reading
  reading = SPI.transfer16(0x0000); // Copy latest reading from ADC
  digitalWrite(CS, HIGH);        //Unselect MCP3102

  //Trim the received 16 bits to gain 12-bit value. The top 3 MSB and the bottom LSB are not of interest.
  reading = reading << 3;   //Remove the top 3 MSB (unused) by shifting upwards
  reading = reading >> 4;   //Shift back down plus an extra in order to remove the lowest bit. Leaving the actual value

  //Convert reading into a Voltage (for displaying)
  float voltage = reading * (5.000 / ADCREF); // Using.
  //Return Voltage
  return (voltage);
}

void loop()
{
  float measure;
  char buf[50];
  int cnt;
  unsigned long currentMillis = millis();
  int channel;

  //Write current time to File
  myFile.print(currentMillis);
  myFile.print("\t");
  if (PRINT)
  {
    Serial.print(currentMillis);
    Serial.print("\t");
  }
  //Go through each analog channel, Writing the measurement to SD and to Terminal.
  for (cnt = 0, channel = 0x0001; cnt < 9; cnt++, channel = channel << 1)
  {
    setMultiplexer(cnt);

    measure = readoutMP3201();

    myFile.print(measure, 3);
    myFile.print("\t");

    if (SHOWCHANNELS & channel)
    {
      Serial.print(measure, 3);
      Serial.print("\t");
    }
  }

  myFile.print("\n");
  myFile.flush();
  Serial.print("\n");
  //If a new number is received. Update all PWM for demonstration:
  byte new_pwm;
  if (Serial.available()) {
    //Read the new value
    new_pwm = Serial.read();
    //Remap from percentage to byte
    new_pwm = map(new_pwm, 0, 100, 0, 255);
    analogWrite(PIN_PWM1, new_pwm);
    analogWrite(PIN_PWM2, new_pwm);
    analogWrite(PIN_PWM3, new_pwm);
  }
  
}
