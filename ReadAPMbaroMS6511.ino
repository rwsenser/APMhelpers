// ReadAPMbaroMS6511
// RW Senser, 2019-06-03
// 
// based on concepts in APM HAL AVR baro code and
// readRegister() and writeRegister from
//     Acceleromter_Gyro_shield_test_Mega2560.ino from
//        accelerometer_gyro_test_mega.zip from
//            https://www.robogaia.com/6-axis-accelerometer-gyro-arduino-shield.html
//
// those above did the real work, I just merged code and debugged! :)
// /* and read the datasheet several times! */
//
// output is correct for the Front Range of Colorado, on a June day
//
// driving goal:
//
// given that the MPU6000 is working (for me),
// try same approach with MS5611
//

#include <SPI.h>

uint16_t _C1;
uint16_t _C2;
uint16_t _C3;
uint16_t _C4;
uint16_t _C5;
uint16_t _C6;  

const int MPUpin = 53;
const int MS5611pin = 40;
const int chipSelectPin = MS5611pin;

// from our friend APM "HAL":
#define CMD_MS5611_RESET 0x1E
#define CMD_MS5611_PROM_Setup 0xA0
#define CMD_MS5611_PROM_C1 0xA2
#define CMD_MS5611_PROM_C2 0xA4
#define CMD_MS5611_PROM_C3 0xA6
#define CMD_MS5611_PROM_C4 0xA8
#define CMD_MS5611_PROM_C5 0xAA
#define CMD_MS5611_PROM_C6 0xAC
#define CMD_MS5611_PROM_CRC 0xAE
#define CMD_CONVERT_D1_OSR4096 0x48   // Maximum resolution (oversampling)
#define CMD_CONVERT_D2_OSR4096 0x58   // Maximum resolution (oversampling)
#define CMD_CONVERT_D2 0x50

//Read from or write to register
//*****************************************************
unsigned int readRegister(byte thisRegister) 
//*****************************************************
{
  unsigned int result = 0;   
  byte addr = thisRegister; 
  // take the chip select low to select the device:
  digitalWrite(chipSelectPin, LOW);
  // send the device the register you want to read:
  SPI.transfer(addr);
  // send a value of 0 to read the first byte returned:
  result = SPI.transfer(0x00);
  // take the chip select high to de-select:
  digitalWrite(chipSelectPin, HIGH);
  return(result);
}

//Read from register 2 bytes (based on readRegister)
//*****************************************************
unsigned int readRegister16(byte thisRegister) 
//*****************************************************
{
  unsigned int result = 0;  
  byte addr = thisRegister; 
  // take the chip select low to select the device:
  digitalWrite(chipSelectPin, LOW);
  // send the device the register you want to read:
  SPI.transfer(addr);
  // send values of 0 to read the bytes returned:
  result = SPI.transfer(0x00);
  result = result * 256;
  result = result + SPI.transfer(0x00);
  // take the chip select high to de-select:
  digitalWrite(chipSelectPin, HIGH);
  return(result);
}

//Read from register 4 bytes, use only 3 (based on readRegister)
//*****************************************************
unsigned long readRegister32t24(byte thisRegister) 
//*****************************************************
{
  unsigned long result = 0;   // result to return
  unsigned long temp;
  // byte addr = thisRegister;
 int addr = thisRegister;  
  // take the chip select low to select the device:
  digitalWrite(chipSelectPin, LOW);
  // send the device the register you want to read:
  SPI.transfer(addr);
  // take the chip select high to de-select:   
  digitalWrite(chipSelectPin, HIGH); 
  // and we wait for over-sampling to occur 
  delayMicroseconds(9040);   
  // take the chip select low to select the device:  
  digitalWrite(chipSelectPin, LOW);  
  // send values of 0 to read the bytes returned:
  SPI.transfer(0x00);  // ignore first byte
  result = SPI.transfer(0x00);  
  result = result * 256;
  temp = SPI.transfer(0x00);
  result = result + temp; 
  result = result * 256;
  temp = SPI.transfer(0x00);  
  result = result + temp; 
  // take the chip select high to de-select: 
  digitalWrite(chipSelectPin, HIGH);
  return(result);
}
//Sends a write command
//*****************************************************
void writeRegister(byte thisRegister, byte thisValue) 
//*****************************************************
{
  thisRegister = thisRegister ;
  // now combine the register address and the command into one byte:
  byte dataToSend = thisRegister;
  // take the chip select low to select the device:
  digitalWrite(chipSelectPin, LOW);
  SPI.transfer(dataToSend); //Send register location
  SPI.transfer(thisValue);  //Send value to record into register
  // take the chip select high to de-select:
  digitalWrite(chipSelectPin, HIGH);
}

void setup() {
    Serial.begin(9600);
    Serial.println("APM Barometer Test");
    // since is APM, make sure MPU6000 is not selected
    pinMode(MPUpin, OUTPUT);
    pinMode(MS5611pin, OUTPUT);
    digitalWrite(MPUpin, HIGH);
    digitalWrite(MS5611pin, HIGH);

    // SPI initialization
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV16);      // SPI at 1Mhz (on 16Mhz clock)
    delay(10);
    readRegister(CMD_MS5611_RESET);  // lazy reset...
    delay(10);

    Serial.println("read config regs");
    _C1 = readRegister16(CMD_MS5611_PROM_C1);
    _C2 = readRegister16(CMD_MS5611_PROM_C2);
    _C3 = readRegister16(CMD_MS5611_PROM_C3);
    _C4 = readRegister16(CMD_MS5611_PROM_C4);
    _C5 = readRegister16(CMD_MS5611_PROM_C5);
    _C6 = readRegister16(CMD_MS5611_PROM_C6);
    // in production version, verify CDC...

    Serial.println("Values read:");    
    Serial.print("C1: "); Serial.println(_C1);
    Serial.print("C2: "); Serial.println(_C2);
    Serial.print("C3: "); Serial.println(_C3);
    Serial.print("C4: "); Serial.println(_C4);
    Serial.print("C5: "); Serial.println(_C5);
    Serial.print("C6: "); Serial.println(_C6);  
      
    unsigned long rawT;
    rawT = readRegister32t24(CMD_CONVERT_D2_OSR4096);
    // rawT = readRegister24(CMD_CONVERT_D2);
    Serial.print("Raw Temp: ");    
    Serial.println(rawT);

    unsigned long rawP;
    rawP = readRegister32t24(CMD_CONVERT_D1_OSR4096);
    Serial.print("Raw Pres: ");    
    Serial.println(rawP);  

    double dT=rawT - (_C5*pow(2,8) );
    double T=(2000 + (dT * _C6 / pow(2,23) ) * 0.01f );    // approach from AP HAL AVR  
    // not corrected for < -15C  :)
    Serial.print("Calc Temp: ");    
    Serial.println(T / 100.0);

    double P;
    // use dt from above
    double OFF=( _C2 * pow(2,17)) + ((dT * _C4) / pow(2,6));
    double SENS=(_C1 * pow(2,16)) + (dT *_C3  / pow(2,7));
    P=( ((rawP * SENS) / pow(2,21) - OFF) / pow(2,15) ) * 0.01f ;
    Serial.print("Calc Press: ");    
    Serial.println(P);         
}

void loop() {
    delay(1000);   
}

// end of code
