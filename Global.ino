/****************************************************** Bibliothèques **********************************************************/
#include <MKRWAN.h>
#include <Wire.h>
#include <DHT.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include "HX711.h"
#include "DFRobot_INA219.h"
#include "ArduinoLowPower.h"
/************************************************* Déclarations de pins **********************************************************/
#define brocheBranchementDHT1 5   // Si la ligne de données du DHT22 est branchée sur la pin D6 de votre Arduino, par exemple
#define brocheBranchementDHT2 7
#define typeDeDHT DHT22          // Si votre DHT utilisé est un DHT22 (mais cela pourrait aussi bien être un DHT11 ou DHT21, par exemple)
#define analogPin A3
#define ONE_WIRE_BUS 4
const int LOADCELL_DOUT_PIN = 1;
const int LOADCELL_SCK_PIN = 0;
/************************************************** Déclarations de variables ****************************************************/
float ina219Reading_mA = 1000;
float extMeterReading_mA = 1000;

DHT dht1(brocheBranchementDHT1, typeDeDHT);
DHT dht2(brocheBranchementDHT2, typeDeDHT);

OneWire oneWire(ONE_WIRE_BUS); 

DallasTemperature sensors(&oneWire);

HX711 scale;

DFRobot_INA219_IIC     ina219(&Wire, INA219_I2C_ADDRESS4);

LoRaModem modem;
/************************************************** Connexion à la carte ****************************************************/
//#include "arduino_secrets.h" 
String appEui = "9100000000000000";
String appKey = "08FB556EB3FD0C65E8C1D899AB94E83C";
bool connected;
int err_count;
int con; 
/************************************************** Setup ****************************************************/
void setup() { 
  pinMode(LED_BUILTIN, OUTPUT);   //LED_BUILTIN as an output.
  Serial.begin(115200);
  
  //while (!Serial);
  Serial.println("Welcome to MKR WAN 1300/1310 first configuration sketch");
  Serial.println("Register to your favourite LoRa network and we are ready to go!");
  modem.begin(EU868);
  delay(1000);      // apparently the murata dislike if this tempo is removed...
  connected=false;
  err_count=0;
  con =0;  
  
  dht1.begin();
  dht2.begin();
  
  sensors.begin(); 
  digitalWrite(LED_BUILTIN, HIGH);    // turn the LED on (HIGH is the voltage level)
  delay(3000);     // wait for 3 seconds
  digitalWrite(LED_BUILTIN, LOW);


  
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  //scale.set_scale(23955.f);                      // this value is obtained by calibrating the scale with known weights; see the README for details
  scale.set_scale(23450);                      // this value is obtained by calibrating the scale with known weights; see the README for details

  scale.tare();               // reset the scale to 0
  long zero_factor = 13100;
  scale.set_offset(zero_factor);

  while(ina219.begin() != true) {
        Serial.println("INA219 begin failed");
        delay(2000);
  }
  ina219.linearCalibrate(ina219Reading_mA, extMeterReading_mA);
}
/************************************************** Loop ****************************************************/
void loop() {
  char msg[2] = {0,1};
  
  // Lecture des données
  float tauxHumidite1 = dht1.readHumidity();              // Lecture du taux d'humidité (en %)
  float temperatureEnCelsius1 = dht1.readTemperature();   // Lecture de la température, exprimée en degrés Celsius
  
  float tauxHumidite2 = dht2.readHumidity();              // Lecture du taux d'humidité (en %)
  float temperatureEnCelsius2 = dht2.readTemperature();   // Lecture de la température, exprimée en degrés Celsius

  sensors.requestTemperatures();

  float poids = scale.get_units();   // Lecture de la température, exprimée en degrés Celsius

  float val = 0;  // variable to store the value read
  float resultat= 0; // le resultat en volt
  short pourcentage=0; //la charge de la batterie en pourcentage
  val = analogRead(analogPin);  // read the input pin
  resultat = (val*3/931);  //(val*3.3/1024) 10 bits avec analogRead 937
  pourcentage =(short)((resultat*100/3));

  float BusVoltage = ina219.getBusVoltage_V();
  float ShuntVoltage = ina219.getShuntVoltage_mV();
  float Current = ina219.getCurrent_mA(); 
  float Power = ina219.getPower_mW();

  if ( !connected ) {
    int ret=modem.joinOTAA(appEui, appKey);
    if ( ret ) {
      connected=true;
      modem.minPollInterval(60);
      Serial.println("Connected");
      modem.dataRate(5);   // switch to SF7
      delay(100);          // because ... more stable
      err_count=0;
    }
  }
  con++; 
  Serial.print("Join test : ");
  Serial.println(con);
  
  if ( connected ) {
  // Affichage des valeurs
  Serial.print("Humidité DHT intérieure = "); Serial.print(tauxHumidite1); Serial.println(" %");
  Serial.print("Température DHT intérieure = "); Serial.print(temperatureEnCelsius1); Serial.println(" °C");

  Serial.print("Humidité DHT extérieure = "); Serial.print(tauxHumidite2); Serial.println(" %");
  Serial.print("Température DHT extérieure = "); Serial.print(temperatureEnCelsius2); Serial.println(" °C");

  Serial.print("TempératureSonde1 = "); Serial.print(sensors.getTempCByIndex(0)); Serial.println(" °C");
  Serial.print("TempératureSonde2 = "); Serial.print(sensors.getTempCByIndex(1)); Serial.println(" °C");

  Serial.print("Poids = "); Serial.print(poids); Serial.println("Kg");

  Serial.print("val : ");  Serial.println(val);
  Serial.print("Pourcentage batterie : ");  Serial.println(pourcentage);

  Serial.print("BusVoltage:   "); Serial.print(BusVoltage, 2); Serial.println("V");
  Serial.print("ShuntVoltage: "); Serial.print(ShuntVoltage, 3); Serial.println("mV");
  Serial.print("Current:      "); Serial.print(Current, 1);Serial.println("mA");
  Serial.print("Power:        "); Serial.print(Power, 1);Serial.println("mW");

  Serial.println();
    short temp1 = (short)(temperatureEnCelsius1*100);
    short hum1 = (short)(tauxHumidite1*100);

    short temp2 = (short)(temperatureEnCelsius2*100);
    short hum2 = (short)(tauxHumidite2*100);

    short tempo1 = (short)(sensors.getTempCByIndex(0)*100);
    short tempo2 = (short)(sensors.getTempCByIndex(1)*100);

    short po = (short)(poids*100);

    short bat= (short)(pourcentage*100);

    short powe= (short)(Power*100);
    
    int err=0;
    modem.beginPacket();
    
   // modem.write(msg,2);
    modem.write(temp1);
    modem.write(hum1);

    modem.write(temp2);
    modem.write(hum2);

    modem.write(tempo1);
    modem.write(tempo2);

    modem.write(po);

    modem.write(bat);

    modem.write(powe);
    
    err = modem.endPacket(true);
    if ( err <= 0 ) {
      // Confirmation not received - jam or coverage fault
      err_count++;
      if ( err_count > 50 ) {
        connected = false;
      }
      // wait for 2min for duty cycle with SF12 - 1.5s frame
      for ( int i = 0 ; i < 120 ; i++ ) {
        delay(1000);
      }
    } else {
      err_count = 0;
      // wait for 20s for duty cycle with SF7 - 55ms frame
      delay(100);
      LowPower.deepSleep(599900);
    }
  }
}
