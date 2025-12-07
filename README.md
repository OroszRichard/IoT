# ESP32 alapú IoT környezetfigyelő / bébiőr állomás (LCD + MQTT)

A projekt célja egy ESP32 mikrokontrollerrel vezérelt, önállóan működő IoT állomás megvalósítása, amely
- méri a környezeti **hőmérsékletet**, **páratartalmat** és **hőérzetet** (DHT11),
- figyeli a **fényviszonyokat** (LDR / fényérzékelő),
- érzékeli a **mozgást** (PIR szenzor),
- az adatokat **16×2 LCD kijelzőn** jeleníti meg menürendszerben,
- két gombbal vezérelhető (oldalváltás, alternatív nézet),
- az összegyűjtött adatokat **TLS-sel védett MQTT kapcsolaton** keresztül továbbítja egy felhőalapú broker felé.

A rendszer alkalmas általános **környezetfigyelésre**, illetve **bébiőr / szobafigyelő prototípusként** való bemutatásra.

*Ide kerülhet egy fotó a kész eszközről:*

`![IoT környezetfigyelő](pics/device.jpg)`

---

## Tartalom

- [ESP32 alapú IoT környezetfigyelő / bébiőr állomás (LCD + MQTT)](#esp32-alapú-iot-környezetfigyelő--bébiőr-állomás-lcd--mqtt)
  - [Tartalom](#tartalom)
  - [Feladat](#feladat)
    - [Készítők](#készítők)
    - [Követelmények](#követelmények)
  - [Megvalósítás](#megvalósítás)
    - [Fizikai eszközök](#fizikai-eszközök)
      - [Bill of Materials (BOM)](#bill-of-materials-bom)
      - [Kapcsolási áttekintés](#kapcsolási-áttekintés)
    - [Programozás](#programozás)
      - [Főbb modulok](#főbb-modulok)
      - [Menürendszer és gombok](#menürendszer-és-gombok)
    - [MQTT kommunikáció](#mqtt-kommunikáció)
    - [A működés rövid áttekintése](#a-működés-rövid-áttekintése)
  - [Lehetséges továbbfejlesztés – bébiőr mód](#lehetséges-továbbfejlesztés--bébiőr-mód)
  - [About](#about)

---

## Feladat

A Nyíregyházi Egyetem **IoT alapok** tantárgyának keretében az volt a cél, hogy egy
**valós, működő IoT rendszert** tervezzünk és valósítsunk meg, amely egy mikrokontrollerre épül,
szenzorokból gyűjt adatokat, ezeket lokálisan kijelzi, valamint hálózaton keresztül továbbítja.

### Készítők

- *Név 1*
- *Név 2*
- *Név 3*

*(A neveket a beadandóhoz tetszés szerint ki lehet tölteni.)*

### Követelmények

A feladatnak tartalmaznia kellett:

- legalább egy **mikrokontrollert** (ESP32),
- több különböző **szenzort** (DHT11, fényérzékelő, PIR),
- **kijelzőt** (16×2 LCD),
- **aktuátort** (buzzer),
- gombos/menüs **felhasználói felületet**,
- **hálózati kommunikációt** (WiFi + MQTT),
- a projekthez tartozó **forráskódot** és **dokumentációt** GitHub repó formájában.

---

## Megvalósítás

### Fizikai eszközök

A projekt központi eleme egy **ESP32 fejlesztőpanel**, amelyre szenzorok és kijelző csatlakoznak.
Az eszközök breadboardon vagy egy próbapanelen keresztül kerültek összekötésre.

#### Bill of Materials (BOM)

| #  | Megnevezés                         | Típus / leírás                            | Mennyiség |
|----|------------------------------------|-------------------------------------------|-----------|
| 1  | ESP32 fejlesztőpanel               | ESP-WROOM-32 alapú DevKit                 | 1 db      |
| 2  | 16×2 LCD kijelző                   | Párhuzamos üzem, 4 bites mód              | 1 db      |
| 3  | DHT11 szenzor                      | Hőmérséklet + páratartalom mérés          | 1 db      |
| 4  | PIR mozgásérzékelő                 | 3,3–5 V, digitális kimenet                | 1 db      |
| 5  | Fényérzékelő (LDR / modul)         | ADC-re kötött fényérzékelő                | 1 db      |
| 6  | Buzzer                             | Aktív/passzív hangjelző                   | 1 db      |
| 7  | Nyomógomb                          | Momentary push button                     | 2 db      |
| 8  | Ellenállások, vezetékek, breadboard| Gombokhoz, LDR-hez, bekötéshez            | n db      |
| 9  | microUSB kábel / 5 V tápegység     | ESP32 tápellátás                          | 1 db      |

**Használt ESP32 lábak:**

- PIR: `PIN_PIR        = 26`
- DHT11 adat: `PIN_DHT  = 4`
- Fényérzékelő (ADC): `PIN_LIGHT = 35`
- Buzzer: `PIN_BUZZER  = 0`
- Gomb1 (bal, lapozás): `PIN_BTN1 = 32` (INPUT_PULLUP)
- Gomb2 (jobb, AltMenu): `PIN_BTN2 = 33` (INPUT_PULLUP)
- LCD:
  - RS: `PIN_LCD_RS = 22`
  - E:  `PIN_LCD_E  = 21`
  - D4: `PIN_LCD_D4 = 5`
  - D5: `PIN_LCD_D5 = 18`
  - D6: `PIN_LCD_D6 = 23`
  - D7: `PIN_LCD_D7 = 19`

#### Kapcsolási áttekintés

- A **DHT11** szenzor adatlába a GPIO4-re (`PIN_DHT`) csatlakozik, tápja 3,3 V / 5 V.
- A **PIR** szenzor kimenete a GPIO26-on (`PIN_PIR`) érhető el, logikai HIGH jelzi a mozgást.
- A **fényérzékelő** LDR/ellenállás osztóval az ADC35 lábra (`PIN_LIGHT`) ad 0–4095 közötti analóg értéket.
- A **buzzer** a GPIO0-ra (`PIN_BUZZER`) csatlakozik, magas szintre kapcsolva rövid csippanással jelez.
- A két **nyomógomb** (oldalváltó és AltMenu váltó) a GPIO32 és GPIO33 lábakra kerül, belső felhúzó ellenállással.
- A **16×2 LCD** 4 bites módban működik, az ESP32 külön digitális lábaira kötve (RS, E, D4–D7).

---

### Programozás

A program **Arduino keretrendszerben** készült ESP32-re, több modulra bontva.

#### Főbb modulok

- **Szenzor-kezelés**
  - `handleDHT()` – 2 másodpercenként olvassa a DHT11-et, frissíti a változókat:
    - `lastT` – hőmérséklet (°C),
    - `lastH` – relatív páratartalom (%),
    - `lastHI` – hőérzet (heat index, °C).
  - `ReadLight()` – visszaadja az ADC értéket a fényérzékelőről (0–4095).
  - `readPir()` – PIR állapot olvasása, logikai érték formájában.

- **Időzítések**
  - DHT lekérdezés: `DHT_INTERVAL = 2000 ms`
  - Soros kimenet frissítés: `SERIAL_INTERVAL = 5000 ms`
  - MQTT adatküldés: `MQTT_INTERVAL = 120000 ms` (2 perc)
  - Gomb-debounce: `DEBOUNCE_MS = 200 ms`

- **WiFi + MQTT**
  - `init_wifi()` – felcsatlakozás a megadott WiFi hálózatra.
  - `init_mqtt()` – MQTT kliens beállítása TLS-sel (`WiFiClientSecure` + `PubSubClient`).
  - `mqtt_reconnect()` – automatikus újracsatlakozás hibakezeléssel.
  - `sendMqttData()` – JSON objektum küldése a megadott topicra.

- **Kijelzés**
  - `showLCD()` – az aktuális menü (`currentMenu`) és az `AltMenu` állapot alapján frissíti a 16×2 LCD mindkét során megjelenő tartalmat.
  - `Uptime()` – futásidőt formáz `HH:MM:SS` alakban soros monitorra vagy LCD-re.

- **Felhasználói interakció**
  - `handleButtons()` – a két gomb kezelése (lapozás a menük között, AltMenu váltása), csipogás hívása (`beepBuzzer()`).
  - `beepBuzzer()` – 50 ms hosszúságú hangjelzés a buzzerrel.

- **Debug / logolás**
  - `printSerialLine()` – soros monitorra írja ki a legfontosabb adatokat, de csak akkor, ha:
    - hőmérséklet vagy páratartalom megváltozott, vagy
    - a fényérték legalább 100 ADC egységet változott.

#### Menürendszer és gombok

Az aktuális menüpontot a `currentMenu` változó tárolja:

- `1` – **DHT11 adatok**
- `2` – **Fényérzékelő**
- `3` – **Mozgásérzékelő (PIR)**
- `4` – **ESP hálózati információk**

Az `AltMenu` logikai változó jelzi, hogy a fő nézet vagy az alternatív nézet legyen-e aktív.

**GOMB1 – lapozás (PIN_BTN1):**

- Menük sorrendje: `1 → 2 → 3 → 4 → 1 → ...`
- Minden lenyomáskor:
  - rövid csipogás,
  - `currentMenu` növelése,
  - `showLCD()` újrarajzolja az állapotot.

**GOMB2 – Alt menü (PIN_BTN2):**

- Az adott menühöz tartozó alternatív nézetre vált:
  - `AltMenu = !AltMenu`,
  - csipogás,
  - `showLCD()` frissítés.

**Menük tartalma röviden:**

- **Menü 1 – DHT11**
  - Alap:
    - Sor1: `Homers.:<T>°C`
    - Sor2: `Paratart.:<H>%`
  - Alt:
    - Sor1: `Hoerzet:<HI>°C`
    - Sor2: `Fenyero: Sotet/Kozepes/Vilagos` (ADC határértékek alapján)

- **Menü 2 – Fény**
  - Alap: nyers ADC érték (`ADC ertek: 0..4095`)
  - Alt: szöveges szint: `sotet / kozepes / vilagos`

- **Menü 3 – PIR**
  - Sor1: `Mozgas erzekeles`
  - Sor2:
    - `Mozgas ESZLELVE` vagy
    - `Mozgas NINCS`

- **Menü 4 – ESP info**
  - Alap:
    - Sor1: `MAC:<espMac>`
    - Sor2: `IP:<espIP>` vagy `IP:NINCS WIFI!`
  - Alt:
    - Sor1: `Start ota eltelt`
    - Sor2: futásidő `MM:SS` vagy `H:MM:SS` formában.

---

### MQTT kommunikáció

Az ESP32 az adatokat TLS-sel védett kapcsolaton keresztül küldi a **HiveMQ Cloud** szerverére (vagy bármely más MQTT brokerre, megfelelő beállítás esetén).

A kódban megadott paraméterek:

```cpp
const char* mqtt_broker   = "86a4864e38404fa0ac3975d7ee622fb3.s1.eu.hivemq.cloud";
const int   mqtt_port     = 8883;
const char* mqtt_username = "28562F4A74A8";
const char* mqtt_password = "Nyiregyhaziegyetem1";
const char* mqtt_topic    = "esp32/28562F4A74A8";
