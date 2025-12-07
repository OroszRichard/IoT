#include <Arduino.h>
#include <LiquidCrystal.h> //LCD
#include <DHT.h> //DHT
#include <WiFi.h>  //WIFI
#include <PubSubClient.h> //MQTT
#include <WiFiClientSecure.h> //TLS

#define PIN_PIR        26  // PIR érzékelő
#define PIN_DHT        4   // DHT11 adatláb
#define PIN_LIGHT      35  // Fényérzékelő (ADC bemenet - Analóg PIN)
#define PIN_BUZZER     0   // Csipogó
#define PIN_BTN1       32  // GOMB1
#define PIN_BTN2       33  // GOMB2
#define PIN_LCD_RS     22  //LCD PIN
#define PIN_LCD_E      21  //LCD PIN
#define PIN_LCD_D4      5  //LCD PIN
#define PIN_LCD_D5     18  //LCD PIN
#define PIN_LCD_D6     23  //LCD PIN
#define PIN_LCD_D7     19  //LCD PIN

LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_E, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7); 

// --- DHT11 definiálás, változói ---
#define DHTTYPE DHT11
DHT dht(PIN_DHT, DHTTYPE); 
unsigned long lastDhtMs = 0; //Utolsó adatbekérés óta eltelt idő
const unsigned long DHT_INTERVAL = 2000; // 2másodpercenként pollingoljuk a DHT szenzort
float lastT = NAN; //Utolsó hőmérséklet
float lastH = NAN; //Utolsó pára
float lastHI = NAN; //Utolsó hőindex

// --- Menü / kijelző aktuális menü ---
uint8_t currentMenu = 1; // 1 = DHT, 2 = Fény, 3 = Mozgás, 4 = ESP adatok

// Gomb-debounce, inicializálás
bool lastBtn1State = HIGH; //Úgy nézzük, hogy kezdetben a gomb lenyomott volt
bool lastBtn2State = HIGH; //Úgy nézzük, hogy kezdetben a gomb lenyomott volt
unsigned long lastBtnMs = 0; //Számláló az utolsó gomb leütése óta
const unsigned long DEBOUNCE_MS = 200; //100ms debounce time

// Belső változó az almenü eléréshez (abba legyünk-e vagy sem)
bool AltMenu = false;

// Serial kiírás időzítése, illetve felesleges sor repeater elkerülése
unsigned long lastSerialMs = 0; //Utoljára közölt adat a serialra
const unsigned long SERIAL_INTERVAL = 5000; // 5 másodpercenként lökjük ki serialra az infót.
float lastSerialT = NAN; //Utolsó Serialra írt Páratartalom
float lastSerialH = NAN; //Utolsó Serialra írt Hőmérséklet
int lastSerialLight = -1; //Utolsó fényérték KEZDŐ! Ilyen érték tuti nincs ADC-n! 0....4095!

//WIFI csatihoz szükséges adatok
const char* ssid = "SSID"; //SSID
const char* password = "JELSZO"; //Jelszó

//ESP adatok MAC+IP
String espMac = ""; //Kezdő Mac üres
String espIP = ""; //Kezdő IP üres

//MQTT brokerem
unsigned long lastMqttMs = 0; //Utolsó MQTT adatküldés óta eltelt idő
const char* mqtt_broker = "86a4864e38404fa0ac3975d7ee622fb3.s1.eu.hivemq.cloud"; //BélAI Mqtt brokere.
const int mqtt_port = 8883; //Mqtt port.
const char* mqtt_username = "28562F4A74A8"; //HiveMQ-n létrehozott speckó user ennek az espnek
const char* mqtt_password = "Nyiregyhaziegyetem1"; //user jelszava
const char* mqtt_topic = "esp32/28562F4A74A8"; //mqtt topik
const unsigned long MQTT_INTERVAL = 120000;  // 2 percenként lepjük meg adattal 2*60*1000
WiFiClientSecure espClient; //TLS/SSL kell a HiveMQ Cloud csatlakozáshoz.
PubSubClient mqttClient(espClient); //MQTT kliens objektum

// Wifi init
void init_wifi() {
  WiFi.begin(ssid, password);
  int num_tries = 0; //Próbák száma, 20 több mint 20 próba után, 10000msec nincs wifi, akkor engedje el a csatlakozást.
  while (WiFi.status() != WL_CONNECTED && num_tries < 20) {
    delay(500);
    num_tries++;
  }
}

//MQTT init
void init_mqtt() {
  espClient.setInsecure(); //letiltjuk a cert ellenőrzést espnél /most nem érdekel a man in the middle lehetőség
  mqttClient.setServer(mqtt_broker, mqtt_port); //mqttklienst belőjük a feljebb paraméterezett broker ip + jelszóval.
}

void mqtt_reconnect() { //Csatlakozzon az MQTT-hez, Serialra írja ki hogy MQTT OK, vagy valami Hiba van.
  if (WiFi.status() != WL_CONNECTED) return;
  while (!mqttClient.connected()) {
    Serial.print("MQTT...");
    if (mqttClient.connect("ESP32_28562F4A74A8", mqtt_username, mqtt_password)) {
      Serial.println("OK");
    } else {
      Serial.print("Hiba: "); Serial.println(mqttClient.state());
      delay(5000); //5000msecet várjon a próbák közt.
    }
  }
}
void sendMqttData() { //Adatküldés MQTT brókerre
  if (WiFi.status() != WL_CONNECTED) return;
  if (!mqttClient.connected()) mqtt_reconnect();
  //data -> amit küldök MQTT-re
  String data = "{";
  data += "\"ho\":" + String(lastT, 1) + ",";
  data += "\"para\":" + String(lastH, 0) + ",";
  data += "\"hoindex\":" + String(lastHI, 1) + ",";
  data += "\"feny\":" + String(ReadLight()) + ",";
  data += "\"uptime(sec)\":" + String(millis()/1000) + "}";
  
  mqttClient.publish(mqtt_topic, data.c_str()); //Ami a data stringben összeépül, azt toljuk fel az mwtt-nek.
  Serial.println("MQTT: " + data);  //írjuk ki serialra is mi megy ki az mqtt-re.!
}
//Uptime formázó 
void Uptime(const char* prefix = "", bool LCD = false) { //Futasido Prefix csak azért, hogy ha akarunk valamit írni még a Serialhoz. LCD-nél be se raktam, nehogy kimenjen a 16karakterből.
  unsigned long totalSeconds = millis() / 1000;
  int hours = totalSeconds / 3600;
  int minutes = (totalSeconds / 60) % 60;
  int seconds = totalSeconds % 60;
  //ez a 4sor adja magát, csak a miliszekeket osztogatom.
  char timeStr[12];  // "HH:MM:SS" alakú tömb.
  
  if (hours > 0) {
    sprintf(timeStr, "%d:%02d:%02d", hours, minutes, seconds);
  } else {
    sprintf(timeStr, "%02d:%02d", minutes, seconds);
  }

  if (LCD) {
    lcd.print(timeStr); //ez írja az időt ki az LCD-re.
  } else { //Ezek mennek Serialba
    Serial.print(prefix);
    Serial.print(timeStr);
  }
}

// Csipogó csipcsipper
void beepBuzzer(){
  digitalWrite(PIN_BUZZER, HIGH);
  delay(50);                 // 50 ms időtartamú csippanás
  digitalWrite(PIN_BUZZER, LOW);
}

// DHT kezelő - frissítő eljárás LOOPba is behívva!
void handleDHT(){
  unsigned long now = millis(); //ESP szerinti MOST idő
  if (now - lastDhtMs < DHT_INTERVAL) return; //Ha a mostani időből leveszem az utolós DHT lekérést, ami kisebb, mint a DHT_INTERVAl változóba megadott érték, pihenünk egy kört.
  lastDhtMs = now; //Az utolsó DHT lekérdezés idejét MOST-ra tesszük.
  float h = dht.readHumidity(); //Pára leolvasó
  float t = dht.readTemperature(); // Hőmérséklet kiolvasó

  if (isnan(h) || isnan(t)) {
    // Ha bármely érték nem szalad be a szenzorról, akkor nem csinálunk semmit - hagyjuk a régi értéket benne.
    return;
  }

  lastH = h; //Az utolsó párát belőjük arra, ami a szenzorról jött
  lastT = t; //Az utolsó hőt belőjük arra, ami a szenzorból jött
  lastHI = dht.computeHeatIndex(t, h, false); // hő és párából kiszámolom a hőérzetet. FALSE = Celsiusba adja ki nem pedig Fahenreitbe.
}

//Analóg beolvasó fényérzékelőről 0-4095 érték szalad be analógról. 4096 már a membe nem fér oda - azért 4025
int ReadLight(){
  int val = analogRead(PIN_LIGHT);
  return val;
}

//PIR állapot olvasó
bool readPir() {
  int val = digitalRead(PIN_PIR);
  return val;
}

// ESP hálózati adatok beolvasása
void EspInfo() {
  if (espMac == "") {
    espMac = WiFi.macAddress(); espMac.replace(":", ""); // :-okat kiszedem, mert így belefér a cucc a 16karakteres LCD egy sorába.
    if (espMac.length() > 12) espMac = espMac.substring(0, 12); //Ha valami gixer van, akkor is 12 karakternél lecsapjuk. Azért nem 16, mert "MAC:"" már 4 karaktert foglal.
  }
  if (WiFi.status() == WL_CONNECTED) { //Wifi csati sikeres
    espIP = WiFi.localIP().toString(); //akkor az IP címet ledobjuk
    if (espIP.length() > 12) espIP = espIP.substring(0, 12); //Ugyanúgy lecsapjuk 12 karakternél ha van valami, bár "IP:" csak 3 karakter.
  } else espIP = "NINCS WIFI!"; //ha csati meghisul, akkor nincs wifi.
}


// Serialra kiírós eljárások sorozata
void printSerialLine() {
  int light = ReadLight(); //Fény adc értéket beolvassuk.
  
  // Csak ha HŐ vagy PÁRA VÁLTOZOTT, VAGY FÉNY >=100 eltérés akkor írunk ki serialra bármit is. isnan = ne legyen már üres az új érték, azt nem szeretjük. Tehát üres érték és utolsó nem egyenló az utolsó kiírttal, akkor...
  bool tempChanged = (!isnan(lastT) && lastT != lastSerialT); //Hőváltozás
  bool humChanged = (!isnan(lastH) && lastH != lastSerialH); //Páraváltozás
  bool lightChanged = (abs(light - lastSerialLight) >= 100); //Fénynél a toleranciám adc érték esetén 100 változásnál nézelődik, ez bárhová állítható, fény függvénye.
  
  if (!tempChanged && !humChanged && !lightChanged) {
    return;  // Ha nincs változás, serialt ne basztassuk feleslegesen.
  }
  
  // Frissítjük régi értéket a jelenlegivel!
  lastSerialT = lastT;
  lastSerialH = lastH;
  lastSerialLight = light;
  
  //sorba.. hő...pára...hőindex.. és nézzük üres-e a változó, ha nem, akkor írjuk! egyébként gatya - sensor hibába tesszük
  Serial.print("Homerseklet: ");
  if (!isnan(lastT)){
    Serial.print(lastT, 1);
  } 
  else{
    Serial.print("Sensor hiba!");
  } 
  Serial.print(", Paratartalom: ");
  if (!isnan(lastH)) {
  Serial.print(lastH, 0); Serial.print("%");
  }
  else{ Serial.print("Sensor hiba!");
  }
  Serial.print(", Hoindex: ");
  if (!isnan(lastHI)){
    Serial.print(lastHI, 1);
      } 
  else{
     Serial.print("Sensor hiba!");
  }

  Serial.print(", Feny (ADC): "); Serial.print(light);
  Serial.print(", FutasIdo: "); Uptime("", false);
  Serial.println(); //sort töröm le a kövi serial sornak.
}


//LCD kitöltő.
void showLCD() {
  lcd.clear();
  if (currentMenu == 1) {
    // 1. oldal: DHT11
    if (!AltMenu) {
      // Alap nézet: Hőmérséklet + Páratartalom
      lcd.setCursor(0, 0);
      lcd.print("Homers.:");
      if (!isnan(lastT)) {
        lcd.print(lastT, 1);lcd.print((char)223);lcd.print("C");
      } else {
        lcd.print("Sensor hiba!");
      }
      lcd.setCursor(0, 1);
      lcd.print("Paratart.:");
      if (!isnan(lastH)) {
        lcd.print(lastH, 1); lcd.print("%");
      } else {
        lcd.print("Sensor hiba!");
      }
    } else {
      // AltMenu: Hőérzet + Fényerő
      int light = ReadLight();
      lcd.setCursor(0, 0);
      lcd.print("Hoerzet:");
      if (!isnan(lastHI)) {
        lcd.print(lastHI, 1);lcd.print((char)223);lcd.print("C");
      } else {
        lcd.print("Sensor hiba!");
      }
      lcd.setCursor(0, 1);
      lcd.print("Fenyero: ");
      //Itt nem ADC erteket irok be, hanem az ADC ertekek szerint nezzuk hogy sotet/kozepes/vilagos!
      if (light < 1000) {
        lcd.print("Sotet");
      } else if (light < 3000) {
        lcd.print("Kozepes");
      } else {
        lcd.print("Vilagos");
      }
    }

  } else if (currentMenu == 2) {
    // 2. oldal: Fényérzékelő
    int light = ReadLight();
    lcd.setCursor(0, 0);
    lcd.print("Fenyerzekelo");
    lcd.setCursor(0, 1);
    if (!AltMenu) {
      // Analog ADC erteket irunk
      lcd.print("ADC ertek:");
      lcd.print(light);
    } else {
      // Szövegesre alakitott, mint az lcd-s info esetében
      lcd.print("Feny: ");
      if (light < 1000) {
        lcd.print("sotet");
      } else if (light < 3000) {
        lcd.print("kozepes");
      } else {
        lcd.print("vilagos");
      }
    }

  } else if (currentMenu == 3) {
    // 3. oldal: Mozgás (PIR)
    bool pir = readPir(); //Ha mozgunk, akkor pir true - ha nem akkor bool false szoval offos a dolog.
    lcd.setCursor(0, 0);
    lcd.print("Mozgas erzekeles");
    lcd.setCursor(0, 1);
    if (pir) {
      lcd.print("Mozgas ESZLELVE");
    } else {
      lcd.print("Mozgas NINCS");
    }
  } else if (currentMenu == 4) {
    EspInfo(); // Frissítjük a hálózati infót
    if (!AltMenu) {
    lcd.setCursor(0, 0);
    lcd.print("MAC:");
    lcd.print(espMac);
    lcd.setCursor(0, 1);
    lcd.print("IP:");
    lcd.print(espIP);
    } else {
    lcd.setCursor(0, 0);
    lcd.print("Start ota eltelt");
    lcd.setCursor(0, 1);
    Uptime("ido: ", true); 
  }
}
}

// --- Gombok kezelése (lapozás és AltMenu változók mókázása) ---
void handleButtons() {
  unsigned long now = millis();
  if (now - lastBtnMs < DEBOUNCE_MS) return; //Debounce teszter, megnézzük hogy a mostból kivonjuk az utolsó gomb interakció óta eltelt időt, az kisebb-emint a debounce, ha igen akkor nem megyünk beolvasásig
  bool btn1 = digitalRead(PIN_BTN1); //Nézzük hogy le van-e nyomva vagy nem a BTN1
  bool btn2 = digitalRead(PIN_BTN2);  //Nézzük hogy le van-e nyomva vagy nem a BTN1

  // GOMB1 BAL: oldal váltás 1-2-3...1-2-3...
  if (btn1 == LOW && lastBtn1State == HIGH) {
    lastBtnMs = now; //Megfrissítem az utolsó gomb interkació óta eltelt idő változóját mostra
    beepBuzzer();                   // minden gombnyomásra csippanjon
    currentMenu++; //az aktuális menühöz dobok egyet
    if (currentMenu > 4) currentMenu = 1; //Ha elértem a menük végére, akkor vissza az elsőre
    showLCD();
  }

  // GOMB2 JOBB: AltMenu váltogatása (második sor tartalma)
  if (btn2 == LOW && lastBtn2State == HIGH) { //Ha a gomb2 nincs benyomva és az utolsó gomb2 állapota eltárolva magas
    lastBtnMs = now; //akkor az utolsó interkació idjeét befrissítem
    beepBuzzer();                   // minden gombnyomásra csippanjon
    AltMenu = !AltMenu; //az AltMenu változót ellentétes értékre állítom (így ugrál a főmenü és az altmenü között)
    showLCD(); //és kirakom az LCD-re.
  }

  lastBtn1State = btn1; //Gomb1 állapotot resetelem
  lastBtn2State = btn2; //Gomb2 állapotot resetelem
}

void setup() {
  Serial.begin(9600); //soros kijelzőt initelem 9600bauddal
  analogSetAttenuation(ADC_11db); //150mV -tól 3100mV ig nézzük
  WiFi.mode(WIFI_STA); //Station modeba kenjük a wifit
  init_wifi(); //inicializáljuk wifi
  init_mqtt(); // inici mqtt
//Az LCD-t inicializálom
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Inicializalas...");
  
  // GPIO PIN módok beállítása
  pinMode(PIN_PIR, INPUT); //Mozgásérzékelő
  pinMode(PIN_LIGHT, INPUT);   // ADC!!
  pinMode(PIN_BUZZER, OUTPUT); //csipcsip
  digitalWrite(PIN_BUZZER, LOW);  //egyből 0-ba tesszük, ne őrítsen meg, ha valamiért high a pin.
  pinMode(PIN_BTN1, INPUT_PULLUP);  //gomb1
  pinMode(PIN_BTN2, INPUT_PULLUP); //gomb2

  // DHT indítása
  dht.begin();
  delay(2000); //2s után induljon meg az LCD. - Legyen értékünk kezdéskor is!
  showLCD();
}

void loop() {
  handleDHT(); //Folyamat tekerje a DHT szenzort
  handleButtons(); //Gombkezelőt meghívjük Loopra.
  mqttClient.loop();
  // LCD frissítés toljuk neki 1000msecenként
  static unsigned long lastLcdMs = 0;
  unsigned long now = millis();
  if (now - lastLcdMs > 1000) {
    lastLcdMs = now;
    showLCD();
  }

  // Serial sor kiírás 5 mp-enként - fentebb megadva az 5mp
  if (now - lastSerialMs > SERIAL_INTERVAL) {
    lastSerialMs = now;
    printSerialLine();
  }
  if (now - lastMqttMs > MQTT_INTERVAL) {  //ugyan az, mint a serial, interval fentebb van megadva. ~120s 2perc
    lastMqttMs = now; 
    sendMqttData(); 
    }
}