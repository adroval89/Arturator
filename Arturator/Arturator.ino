
#include <IRremote.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 2
#define pinhlt 8
#define pinboil 12
#define pinMashPump 7
#define pinBoilPump 6
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire ref"erence to Dallas Temperature.
DallasTemperature sensors(&oneWire);
LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 16 chars and 2 line display

int receiver = 10; // Signal Pin of IR receiver to Arduino Digital Pin 10
IRrecv irrecv(receiver);     // create instance of 'irrecv'
decode_results results;      // create instance of 'decode_results'

//constants
int hlt = 0;
int boil = 1;
String message;
int power = 2000;
bool hltON = false;
bool boilON = false;
bool mashPumpON = false;
bool boilPumpON = false;
bool preparation = false;
long timeing;
int maxpower = 5500;
String boilmessage = "boil power";
int printpower = power;

//variable

void setup()
{
  Serial.begin(9600);
  Serial.println("Dallas Temperature IC Control Library Demo");
  // Start up the library
  sensors.begin();
  lcd.begin();
  pinMode(pinhlt, OUTPUT);
  pinMode(pinboil, OUTPUT);
  pinMode(pinMashPump, OUTPUT);
  pinMode(pinBoilPump, OUTPUT);
  irrecv.enableIRIn(); // Start the receiver

}

void loop()
{
  // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
  sensors.requestTemperatures();
  lcd.clear();
  lcd.print("HLT ");
  lcd.print(sensors.getTempCByIndex(hlt));
  lcd.print(" C") ;
  lcd.setCursor(0, 1);
  lcd.print("BOIL ");
  lcd.print(sensors.getTempCByIndex(boil));
  lcd.print(" C") ;
  lcd.setCursor(0, 2);
  lcd.print(message);
  lcd.setCursor(0, 3);
  lcd.print(boilmessage);
  lcd.print(" ");
  lcd.print(printpower);
  lcd.print(" W");
  if (irrecv.decode(&results)) // have we received an IR signal?
  {
    switch (translateIR()) // Encender o apagar los programas en función de la tecla apretada.
    {
      case 1: boilON = true ; message = "hirviendo..."; break;
      case 2: boilON = false; message = "hervido terminado"; break;
      case 3: hltON = true ; message = "HLT on"; break;
      case 4: hltON = false ; message = "HLT off"; break;
      case 5: mashPumpON = true; message = "mash pump on"; break;
      case 6: mashPumpON = false; message = "mash pump off"; break;
      case 7: boilPumpON = true; message = "boil pump on"; break;
      case 8: boilPumpON = false; message = "boil pump off"; break;
      case 9: preparation = true;
        message = "calentando agua";
        boilON = hltON = mashPumpON = boilPumpON = false; break; // apaga todo el resto, este programa corre solo.
      case 0: boilON = hltON = mashPumpON = boilPumpON = preparation = false; message = "todo apagado"; break; //Apaga todo
      case 120: if (boilmessage != "boil power") boilmessage = "boil power"; else if (power < 5500) power = power + 500; printpower = power; boilmessage = "boil power"; break;
      case 170: if (boilmessage != "boil power") boilmessage = "boil power"; else if (power > 0) power = power - 500; printpower = power; boilmessage = "boil power"; break;
      case 150: if (boilmessage != "max power") boilmessage = "max power"; else if (maxpower < 5500) maxpower = maxpower + 500; printpower = maxpower; boilmessage = "max power"; break;
      case 130: if (boilmessage != "max power") boilmessage = "max power"; else if (maxpower > 0) maxpower = maxpower - 500; printpower = maxpower; boilmessage = "max power"; break;

    }

    irrecv.resume();

  }
  if (boilON) {
    boilHeating(30, pinboil);
  }
  if (!boilON) {
    digitalWrite(pinboil, LOW);
  }
  if (hltON) {
    hltHeating(80, pinhlt);
  }
  if (!hltON) {
    digitalWrite(pinhlt, HIGH);
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
  if (preparation) {
    waterHeating();
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
    analogWrite(pin, maxpower / 21.569);
  }
  else {
    analogWrite(pin, power / 21.569);
  }
}

void waterHeating() {
  while (sensors.getTempCByIndex(boil) < 31) {
    sensors.requestTemperatures();
    boilHeating(30, pinboil);
    lcd.clear();
    lcd.print("Calentando agua ");
    lcd.setCursor(0, 1);
    lcd.print("temperatura: ");
    lcd.print(sensors.getTempCByIndex(boil));
    lcd.print(" C");
    delay(2000); // cada 2 segundos verifica para que el programa no esté como loco.
  }
  digitalWrite(pinboil, LOW); // apaga hervor
  digitalWrite(pinBoilPump, LOW); // prende bomba
  delay(3000); // espera a que llene el tanque hasta que cubra la resistencia
  hltHeating(80, pinhlt); // enciende el hlt
  delay(3000); // espera a que cargue toda el agua
  digitalWrite(pinBoilPump, HIGH); // apaga la bomba
  preparation = false; // termina el ciclo
  message = "agua lista"; // imprime mensaje
  hltON = true; // deja HLT prendido
  for (int i = 0 ; i < 5 ; i++) { // 5 pitidos de 1 s cada
    analogWrite(4, 50); // buzzer
    delay(1000);
    digitalWrite(4, LOW);
    delay(500);
  }
}

int translateIR() // takes action based on IR code received

// describing Remote IR codes

{
  switch (results.value)

  {
    case 0xFFA25D: return 100; break; //"POWER"
    case 0xFFE21D: return 110; break; //"FUNC/STOP"
    case 0xFF629D: return 120; break; //"VOL+"
    case 0xFF22DD: return 130;   break; //"FAST BACK"
    case 0xFF02FD: return 140;   break; //"PAUSE"
    case 0xFFC23D: return 150;   break; //"FAST FORWARD"
    case 0xFFE01F: return 160;   break; //"DOWN"
    case 0xFFA857: return 170;   break; //"VOL-"
    case 0xFF906F: return 180;   break; //"UP"
    case 0xFF9867: return 190;   break; //"EQ"
    case 0xFFB04F: return 200;   break; //"ST/REPT"
    case 0xFF6897: return 0;   break; //"0"
    case 0xFF30CF: return 1;  break; //"1"
    case 0xFF18E7: return 2;  break; //"2"
    case 0xFF7A85: return 3;  break; //"3"
    case 0xFF10EF: return 4;  break; //"4"
    case 0xFF38C7: return 5;  break; //"5";
    case 0xFF5AA5: return 6;  break; //"6"
    case 0xFF42BD: return 7;  break; //"7"
    case 0xFF4AB5: return 8;  break; //"8"
    case 0xFF52AD: return 9;  break; // "9"
    case 0xFFFFFFFF: return 300; break;  //" REPEAT"

    default:
      return (301); //error o otro boton

  }// End Case

  delay(200); // Do not get immediate repeat


} //END translateIR
