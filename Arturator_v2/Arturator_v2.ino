#include <LiquidCrystal_I2C.h>
#include <IRremote.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <UbiProtocol.h>
#include <UbiConstants.h>
#include <UbiTcp.h>
#include <Ubidots.h>
#include <UbiTypes.h>
#include <UbiBuilder.h>
#include <UbiProtocolHandler.h>
#include <UbiUdp.h>
#include <UbiUtils.h>
#include <UbiHttp.h>

#define ONE_WIRE_BUS 2
#define pinkettlesour 3
#define pinBoilPump 0
#define pinMashPump 12
#define pinhlt 13
#define pinboil 15

// internet connection

const char* UBIDOTS_TOKEN = "BBUS-U9PzCLeCBwstFMTwMFbdJ5IDQ9BiEw";  // Put here your Ubidots TOKEN
const char* WIFI_SSID = "ADROIVA"; // Put here your Wi-Fi SSID
const char* WIFI_PASS = "Los5locos"; // Put here your Wi-Fi password



//variables
const int receiver = D5; // Signal Pin of IR receiver to Arduino Digital Pin 10
int hlt = 0;
int boil = 1;
String MESSAGE = "Arturator 3.0";
int power = 2000;
float hltTemp = 80;
bool hltON = false;
bool boilON = false;
bool mashPumpON = false;
bool boilPumpON = false;
bool preparation = false;
bool kettlesour = false;
long timeing;
int maxpower = 5500;
byte kettlesourtemp = 35;
String BOIL_MESSAGE = "boil power";
int printpower = power;
const int intervalON = 5500;
uint32_t nextTime;
float loweringTemps[10] = {88,89,90,91,92,93,94,95,96,97};
float *loweringtemp = loweringTemps;
uint32_t currentTime;

//open ubidots connection with TCP protocol

Ubidots ubidots(UBIDOTS_TOKEN, UBI_TCP);

OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 20 chars and 4 line display

IRrecv irrecv(receiver);     // create instance of 'irrecv'



//variable

void setup()
{
    // Start up the library
  sensors.begin();
  lcd.begin();
  lcd.backlight();
  lcd.print("Connecting to WIFI ...");
  ubidots.wifiConnect(WIFI_SSID, WIFI_PASS);  // connect to ubidots
  lcd.print("Connected");
  pinMode(pinhlt, OUTPUT);
  pinMode(pinboil, OUTPUT);
  pinMode(pinMashPump, OUTPUT);
  pinMode(pinBoilPump, OUTPUT);
  pinMode(pinkettlesour, OUTPUT);
  digitalWrite(pinhlt, HIGH);
  digitalWrite(pinboil, LOW);
  digitalWrite(pinMashPump, HIGH);
  digitalWrite(pinBoilPump, HIGH);
  digitalWrite(pinkettlesour,HIGH);
  irrecv.enableIRIn(); // Start the receiver
  

}

void loop()
{
  currentTime = millis(); //Keep internal clock
  // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
  sensors.requestTemperatures();
  lcd.clear();
  lcd.print("HLT ");
  lcd.print(sensors.getTempCByIndex(hlt));
  lcd.print("C") ;
  lcd.setCursor(0, 1);
  lcd.print(ubidotsRec());
  lcd.print(sensors.getTempCByIndex(boil));
  lcd.print("C") ;
  lcd.setCursor(0, 2);
  lcd.print(MESSAGE);
  if (kettlesour) {
    lcd.print("T=");
    lcd.print(kettlesourtemp);
    lcd.print(" C");
  }
  lcd.setCursor(0, 3);
  lcd.print(BOIL_MESSAGE);
  lcd.print(" ");
  lcd.print(printpower);
  lcd.print(" W");
  if (irrecv.decode()) // have we received an IR signal?
  {
    switch (irrecv.decodedIRData.decodedRawData) // Encender o apagar los programas en función de la tecla apretada.
    {
      case 0xF30CFF00: if (boilON) {
          boilON = false;
          MESSAGE = "hervido terminado";
        }
        else {
          boilON = true;
          MESSAGE = "hirviendo...";
        }  break;
      case 0xE718FF00: (*loweringtemp == 97) ? loweringtemp = loweringTemps : loweringtemp+=1 ; 
      MESSAGE = "Max pow till " + String(*loweringtemp) + "C"; break;
      case 0xA15EFF00: hltON = true ; MESSAGE = "HLT on"; break;
      case 0xF708FF00: hltON = false ; MESSAGE = "HLT off"; break;
      case 0xE31CFF00: mashPumpON = true; MESSAGE = "mash pump on"; break;
      case 0xA55AFF00: mashPumpON = false; MESSAGE = "mash pump off"; break;
      case 0xBD42FF00: boilPumpON = true; MESSAGE = "boil pump on"; break;
      case 0xAD52FF00: boilPumpON = false; MESSAGE = "boil pump off"; break;
      case 0xB54AFF00: preparation = true;
        MESSAGE = "calentando agua";
        boilON = hltON = mashPumpON = boilPumpON = false; break; // apaga todo el resto, este programa corre solo.
      case 0xE916FF00: boilON = hltON = mashPumpON = boilPumpON = preparation = kettlesour = false; MESSAGE = "todo apagado"; break; //Apaga todo
      case 0xE619FF00: if (kettlesour) {
          kettlesour = false;
          MESSAGE = "kettlesour OFF";
        }
        else {
          kettlesour = true;
          MESSAGE = "kettlesour ON ";
        }  break;
      case 0xB847FF00: if (kettlesourtemp < 50) kettlesourtemp += 1; MESSAGE = "kettletemp " ; break;
      case 0xBA45FF00: if (kettlesourtemp > 20) kettlesourtemp -= 1; MESSAGE = "kettletemp " ; break;
      case 0xF807FF00: if (hltTemp > 30) hltTemp -= 1; MESSAGE = "hltTemp " + String(hltTemp, 0) ; break;
      case 0xF609FF00: if (hltTemp < 90) hltTemp += 1; MESSAGE = "hltTemp " + String(hltTemp, 0) ; break;
      case 0xBC43FF00: if (BOIL_MESSAGE != "boil power") BOIL_MESSAGE = "boil power"; else if (power < 5500) power += 500; printpower = power; BOIL_MESSAGE = "boil power"; break;
      case 0xBB44FF00: if (BOIL_MESSAGE != "boil power") BOIL_MESSAGE = "boil power"; else if (power > 0) power -= 500; printpower = power; BOIL_MESSAGE = "boil power"; break;
      case 0xB946FF00: if (BOIL_MESSAGE != "max power") BOIL_MESSAGE = "max power"; else if (maxpower < 5500) maxpower += 500; printpower = maxpower; BOIL_MESSAGE = "max power"; break;
      case 0xEA15FF00: if (BOIL_MESSAGE != "max power") BOIL_MESSAGE = "max power"; else if (maxpower > 0) maxpower -= 500; printpower = maxpower; BOIL_MESSAGE = "max power"; break;

    }
     if (mashPumpON) {
      digitalWrite(pinMashPump, LOW);
    }
    if (!mashPumpON) {
      digitalWrite(pinMashPump, HIGH);
    }
    if (boilPumpON) {
      digitalWrite(pinBoilPump, LOW);
    }
    if (!boilPumpON) {
      digitalWrite(pinBoilPump, HIGH);
    }
    irrecv.resume();

  }
  if (boilON) {
      boilHeating(*loweringtemp, pinboil);
    }
    if (!boilON) {
      digitalWrite(pinboil, LOW);
    }
    if (hltON) {
      hltHeating(hltTemp, pinhlt);
    }
    if (!hltON) {
      digitalWrite(pinhlt, HIGH);
    }
    if (preparation) {
      waterHeating();
    }
    if (kettlesour) {
      kettlesouring();
    }
    if (!kettlesour) {
      digitalWrite(pinkettlesour, HIGH);
    ubidotsSend();
    }
}

// función para mantener temperatura a determinado valor del HLT


void hltHeating(float temp, int pin) {
  if (sensors.getTempCByIndex(hlt) <= temp) {
    digitalWrite(pin, LOW);
  }
  else {
    digitalWrite(pin, HIGH);
  }
}

// función para hervir. Hierve a maxima potencia hasta llegar a 98 grados, despues baja.


void boilHeating(float temp, int pin) {
  if (sensors.getTempCByIndex(boil) <= temp) {
    resistanceSwitch(maxpower);
  }
  else {
    resistanceSwitch(power);
  }
}

void waterHeating() {
  if (sensors.getTempCByIndex(boil) < hltTemp) {
    // have we received an IR signal?
    sensors.requestTemperatures();
    boilHeating(85, pinboil);
    MESSAGE = "Calentando agua";
  }
  else {
    digitalWrite(pinboil, LOW); // apaga hervor
    digitalWrite(pinBoilPump, LOW); // prende bomba
    delay(300000); // espera a que llene el tanque hasta que cubra la resistencia
    hltHeating(80, pinhlt); // enciende el hlt
    delay(300000); // espera a que cargue toda el agua
    digitalWrite(pinBoilPump, HIGH); // apaga la bomba
    preparation = false; // termina el ciclo
    MESSAGE = "agua lista"; // imprime mensaje
    hltON = true; // deja HLT prendido
    for (int i = 0 ; i < 5 ; i++) { // 5 pitidos de 1 s cada
      analogWrite(4, 50); // buzzer
      delay(1000);
      digitalWrite(4, LOW);
      delay(500);
    }
  }
}

void resistanceSwitch(int pw) {
  if (currentTime > nextTime) {
    if (digitalRead(pinboil)) {
      digitalWrite(pinboil, LOW);
      if (pw == 5500) {
        digitalWrite(pinboil, HIGH);
      }
      nextTime = currentTime + 0.5 * (5500 - pw);
    } else {
      digitalWrite(pinboil, HIGH);
      nextTime = currentTime + 0.5 * pw;
    }
  }
}

void kettlesouring() {
  if (kettlesourtemp < sensors.getTempCByIndex(boil)) {
    digitalWrite(pinkettlesour, HIGH);
  }
  else {
    digitalWrite(pinkettlesour, LOW);

  }
}
void ubidotsSend(){
  ubidots.add("BOIL_temp", sensors.getTempCByIndex(boil));
  bool bufferSent = false;
  bufferSent = ubidots.send();
}

float ubidotsRec(){
    float value = ubidots.get("bcddc27db775", "variable_name_two");
    return value;
}
