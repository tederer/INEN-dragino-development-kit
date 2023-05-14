#include <SPI.h>
#include <RH_RF95.h>

#define SENSOR_ID                     "arduino"

#define DATA_PIN                      A0
#define SENSOR_DOES_NOT_SET_PIN_LOW   1
#define SENSOR_DOES_NOT_SET_PIN_HIGH  2
#define INVALID_CHECKSUM              3
#define OK                            0
#define FREQUENCY                     433.1

RH_RF95 rf95;
char    rawSensorData[5];

void assertOk(bool ok, char* message) {
  if (!ok) {
    Serial.println(message);
  }
}

void initTemperatureSensor()
{
  pinMode(DATA_PIN,OUTPUT);
  digitalWrite(DATA_PIN,HIGH);
}

int ReadSensorValues()
{ 
    byte dataPinValue;
    byte i;
    
    pinMode(DATA_PIN,OUTPUT);
    digitalWrite(DATA_PIN,LOW);
    delay(30);
    digitalWrite(DATA_PIN,HIGH);
    
    unsigned long start = micros();
    unsigned long end   = start + 40;

    pinMode(DATA_PIN,INPUT);
    dataPinValue = digitalRead(DATA_PIN);
    while ((dataPinValue == 1) && (micros() < end)) {
      dataPinValue = digitalRead(DATA_PIN);
    }
    
    if(dataPinValue != 0) {
      return SENSOR_DOES_NOT_SET_PIN_LOW;
    }

    start = micros();
    end   = start + 80;

    dataPinValue = digitalRead(DATA_PIN);
    while ((dataPinValue == 0) && (micros() < end)) {
      dataPinValue = digitalRead(DATA_PIN);
    }
    
    if(dataPinValue == 0) {
      return SENSOR_DOES_NOT_SET_PIN_HIGH;
    }

    delayMicroseconds(80);
    
    for (i=0; i<5; i++) {
      rawSensorData[i] = readByte();
    }

    byte dht_check_sum = rawSensorData[0] + rawSensorData[1] + rawSensorData[2] + rawSensorData[3];
    return (rawSensorData[4] == dht_check_sum) ? OK : INVALID_CHECKSUM;
};

byte readByte(){
  byte i = 0;
  byte result=0;
  for(i=0; i< 8; i++) {
    while(digitalRead(DATA_PIN) == HIGH);
    while(digitalRead(DATA_PIN) == LOW);
    delayMicroseconds(40);
    if (digitalRead(DATA_PIN) == HIGH) {
      result |= 1 << (7 - i);
    }
  }
  return result;
}

void setup() 
{
  Serial.begin(9600);
  Serial.println("init temperature sensor DHT11");
  initTemperatureSensor();
  Serial.println("Start LoRa Client");
  if (!rf95.init())
    Serial.println("init failed");
  rf95.setFrequency(FREQUENCY);
  rf95.setTxPower(13);
  rf95.setSpreadingFactor(7);
  rf95.setSignalBandwidth(125000);
  rf95.setCodingRate4(5);
}

void loop()
{
  int status = ReadSensorValues();
  
  if (status == OK) {
    char format[] = "%s,%d.%d";
    int messageLength = snprintf(NULL, 0, format, SENSOR_ID, rawSensorData[2], rawSensorData[3]) + 1;
    char message[messageLength];
    sprintf(message, format, SENSOR_ID, rawSensorData[2], rawSensorData[3]);
    message[messageLength - 1] = 0;
    Serial.print("sending \"");
    Serial.print(message);
    Serial.println("\"");
    assertOk(rf95.send(message, strlen(message)), "ERROR: rf95.send failed");
    assertOk(rf95.waitPacketSent(), "ERROR: rf95.waitPacketSent failed");
  } else {
    Serial.print("Failed to read sensor values (status = ");
    Serial.print(status);
    Serial.println(")");
  }

  delay(2000);
}


