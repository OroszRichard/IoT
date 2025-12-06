# ESP32 alapú IoT környezetfigyelő és jelenlétérzékelő állomás

A projekt célja egy ESP32 mikrokontrollerrel vezérelt, önműködő IoT rendszer megvalósítása, amely
- méri a környezeti hőmérsékletet és páratartalmat,
- mozgást érzékel,
- vizuális (LED, LCD) és hangjelzéssel (buzzer) ad visszajelzést,
- infravörös távirányítóval vezérelhető.

A rendszer alkalmas egyszerű okosotthon-szerű funkciók demonstrálására (riasztás, állapotjelzés, környezetfigyelés).

Megvalósítási terv:
![IoT](circuit_image.png)


---

## Tartalom

- [ESP32 alapú IoT környezetfigyelő és jelenlétérzékelő állomás](#esp32-alapú-iot-környezetfigyelő-és-jelenlétérzékelő-állomás)
  - [Tartalom](#tartalom)
  - [Készítők](#Készítők)
    - [Követelmények](#követelmények)
  - [Megvalósítás](#megvalósítás)
    - [Szimulációs környezet](#szimulációs-környezet)
    - [Fizikai eszközök](#fizikai-eszközök)
      - [Bill of Materials (BOM / Anyagjegyzék)](#bill-of-materials-bom-anyagjegyzék)
      - [Áramellátás](#áramellátás)
      - [Szenzorok](#szenzorok)
      - [Kijelző és kezelőfelület](#kijelző-és-kezelőfelület)
      - [Jelzőeszközök](#jelzőeszközök)
  - [Programozás](#programozás)
    - [A kód](#a-kód)
    - [A működés rövid leírása](#a-működés-rövid-leírása)
    - [A működés folyamatábrája](#a-működés-folyamatábrája)
- [About](#about)


---

## Készítők

A projekt megvalósítói:  

- Kulcsár Béla 
- Orosz Richárd 
- Kocserha Patrik

### Követelmények

A projekt feladatnak mindenképpen tartalmaznia kell:

- egy mikrokontrollert,
- egy vagy több érzékelőt,
- egy kijelzőt,
- egy beavatkozót (aktuátort),
- futtatható kódot,
- GitHub oldalt, ahol az eredmények elérhetők,
- dokumentációt (leírás, képek, folyamatábra stb.).

---

## Megvalósítás

### Szimulációs környezet

A projekt fejlesztése során (opcionálisan) használható volt online szimulációs környezet, például a WOKWI,
ahol az ESP32 és a kapcsolt perifériák működése előzetesen tesztelhető.  
A fizikai megvalósítás a szimuláció után egy breadboardon történt, valódi ESP32 fejlesztőpanellel.


### Fizikai eszközök

A projekt kivitelezéséhez a következő eszközök kerültek felhasználásra:

- ESP-WROOM-32-ESP32S alappanel
- 1602A LCD kijelző (16×2)
- DHT11 hőmérséklet–páratartalom szenzor
- PIR mozgásérzékelő szenzor
- 3.3V–5V aktív buzzer
- IR vevőmodul (kínai, piros NYÁK, pl. HX1838)
- 17 gombos IR távirányító (NEC protokoll)
- Piros LED
- Zöld LED
- ~220 Ω-os ellenállások
- 10 kΩ-os ellenállás
- B5K lineáris potenciométer
- 400 pin-es breadboard (tesztpanel)
- Jumper kábelek (male–female)
- Breadboard átkötő huzalok
- microUSB kábel (tápellátás, programozás)

#### Bill of Materials (BOM / Anyagjegyzék)

| #  | Megnevezés                             | Típus / Leírás                          | Mennyiség | Megjegyzés                            |
|----|----------------------------------------|-----------------------------------------|-----------|---------------------------------------|
| 1  | Piros LED                              | Általános célú LED                      | 1 db      | Jelzés / visszajelzés                |
| 2  | Zöld LED                               | Általános célú LED                      | 1 db      | Jelzés / visszajelzés                |
| 3  | Ellenállás ~220 Ω                      | Fix ellenállás                          | 3 db      | LED áramkorlátozáshoz                |
| 4  | Ellenállás 10 kΩ                       | Fix ellenállás                          | 1 db      | Pl. bemenet felhúzásához             |
| 5  | DHT11 hőmérséklet–páratartalom szenzor | Digitális szenzor                       | 1 db      | Hőmérséklet és páratartalom mérés    |
| 6  | PIR mozgásérzékelő szenzor             | Passzív infravörös szenzor              | 1 db      | Jelenlét / mozgás detektálás         |
| 7  | Buzzer 3,3–5 V                         | Aktív hangjelző                         | 1 db      | Hangjelzés                            |
| 8  | IR vevőmodul                           | Kínai, piros NYÁK (pl. HX1838)          | 1 db      | Távirányító jeleinek vétele          |
| 9  | IR távirányító                         | 17 gombos, NEC protokoll                | 1 db      | Felhasználói vezérlés                |
| 10 | Jumper kábel                           | Male–Female                             | 16 db     | Modulok és ESP32 összekötése         |
| 11 | Breadboard átkötő huzal                | Rigid vezeték                           | 10 db     | Kapcsolások kialakítása breadboardon |
| 12 | Tesztpanel (breadboard)                | 400 pin-es                              | 1 db      | Próbaáramkör építéséhez              |
| 13 | 1602A LCD kijelző                      | 16×2 karakteres                         | 1 db      | Adatok kijelzése                     |
| 14 | B5K lineáris potenciométer             | 5 kΩ, lineáris                          | 1 db      | Kontraszt / beállítás / analóg bemenet |
| 15 | ESP-WROOM-32-ESP32S alappanel         | ESP32 fejlesztőpanel                    | 1 db      | Fő vezérlő, Wi-Fi / logika           |
| 16 | microUSB kábel                         | Tápellátás + programozás                | 1 db      | ESP32 tápja és kommunikáció          |

---

#### Áramellátás

- A rendszer központi eleme az **ESP32 fejlesztőpanel**, amelyet egy **microUSB kábellel** látunk el 5 V tápfeszültséggel.  
- A szükséges 3,3 V-os és GND vonalakat az ESP32 panel biztosítja a breadboard felé.  
- Minden szenzor és modul a breadboard pozitív (+3,3 V vagy 5 V) és negatív (GND) sínjére van kötve.

---

#### Szenzorok

**DHT11 (hőmérséklet–páratartalom szenzor)**  
- VCC → 3,3 V vagy 5 V  
- GND → GND  
- DATA → ESP32 digitális bemenet (pl. **GPIO12**)  
- A DATA láb és VCC közé 10 kΩ-os felhúzó ellenállás köthető (ha a modul nem tartalmazza).

**PIR mozgásérzékelő**  
- VCC → 5 V (vagy a modul specifikációja szerint)  
- GND → GND  
- OUT → ESP32 digitális bemenet (pl. GPIOxx – a kódból egyértelműen következik)

**IR vevőmodul (pl. HX1838)**  
- VCC → 3,3 V vagy 5 V (modul függő)  
- GND → GND  
- OUT → ESP32 digitális bemenet (pl. egy megszakításra alkalmas GPIO)

---

#### Kijelző és kezelőfelület

**1602A LCD kijelző (16×2)**  
- VCC, GND → breadboard táp sínek  
- Az adat- és vezérlő lábak az ESP32 megfelelő GPIO-lábaira vannak kötve  
  (a konkrét kiosztást a forráskódban dokumentáljuk).  
- A **B5K lineáris potenciométer** a kontraszt beállítására szolgál (középső kivezetés → LCD VO láb, két szélső → VCC és GND).

---

#### Jelzőeszközök

**LED-ek**

- Piros LED – tipikusan hiba / riasztás jelzésére.  
- Zöld LED – normál működés / OK állapot jelzésére.  
- Mindkettőnél:
  - Anód (+) → ESP32 digitális kimenet egy ~220 Ω-os ellenálláson keresztül  
  - Katód (–) → GND

**Buzzer (3,3–5 V aktív)**

- + → ESP32 digitális kimenet  
- – → GND  
- A buzzer megszólaltatásával hangjelzés adható (pl. riasztáskor vagy gombnyomás jelzésére).

---

## Programozás

A projekt szoftveres megvalósítása **Arduino IDE** segítségével történt, az ESP32-hez szükséges kiegészítő csomag telepítése után.

Felhasznált könyvtárak (példa):

- `DHT` – DHT11 hőmérséklet–páratartalom szenzor kezelése  
- `IRremote` vagy ESP32-kompatibilis IR könyvtár – IR távirányító jeleinek dekódolására  
- `LiquidCrystal` vagy `LiquidCrystal_I2C` – 1602 LCD vezérlésére (a használt kijelzőtől függ)  

### A kód

A teljes kód a repository megfelelő forrásfájljában található:

- `src/iot_allomas.ino` *(vagy a valódi fájlnév, pl. `beadando.ino`)*

A kód fő részei:

- inicializálás (`setup()`):  
  - soros port, szenzorok, LCD, LED-ek, buzzer, IR vevő beállítása,  
- fő ciklus (`loop()`):  
  - szenzoradatok beolvasása (DHT11, PIR),  
  - IR távirányító jeleinek figyelése,  
  - kijelzés frissítése az LCD-n,  
  - LED-ek és buzzer vezérlése az aktuális állapotnak megfelelően.

*(Ide akár egy rövid kódrészletet is bemásolhatsz, ha szeretnéd.)*

---

### A működés rövid leírása

- A rendszer folyamatosan méri a **hőmérsékletet és a páratartalmat**, és ezeket az adatokat a **16×2 LCD kijelzőn** megjeleníti.  
- A **PIR szenzor** figyeli a mozgást a kijelölt térrészben.  
  - Ha mozgást érzékel, a rendszer:
    - aktiválhatja a buzzert (riasztó hangjelzés),  
    - átválthat egy „RIASZTÁS” kijelzésre az LCD-n,  
    - a piros LED villogtatásával jelezheti a riasztást.  
- A **zöld LED** a normál, nyugalmi működést jelzi.  
- A **17 gombos IR távirányítóval** különböző üzemmódok válthatók:
  - például: riasztó be/ki, hangos / néma mód, csak mérés és kijelzés, stb.  
- A rendszer egy egyszerű, jól átlátható IoT demonstrátor, amely több szenzor és aktuátor együttműködését mutatja be.

---

### A működés folyamatábrája

A rendszer logikáját az alábbi folyamatábra szemlélteti:

1. Rendszer indul → inicializálás (szenzorok, kijelző, változók)  
2. Fő ciklus:
   - DHT11 adatainak beolvasása  
   - PIR szenzor ellenőrzése  
   - IR távirányító jelének feldolgozása (üzemmódváltás stb.)  
   - LCD frissítése az aktuális adatokkal és állapotüzenettel  
   - LED-ek és buzzer vezérlése az aktuális üzemmód szerint  
3. Visszatérés a fő ciklus elejére

Folyamatábra: 

### A működés folyamatábrája

```mermaid
flowchart TD
    A([Indítás / reset]) --> B[setup()]
    B --> C[Hardver inicializálás<br>LCD, DHT11, PIR, IR, gomb, LED-ek, buzzer]
    C --> D[WiFi és idő beállítása<br>connectWiFiAndTime()]
    D --> E[showMenu()<br>alap menü kiírása]
    E --> F[[loop() – fő ciklus]]

    F --> G[handleIR()]
    G --> H{Érvényes<br>IR kód?}
    H -->|Nem| I[IR: nincs érdemi esemény]
    H -->|Igen| J[Menü / debug mód<br>állítása, LCD frissítés,<br>buzzer jelzés]

    I --> K[handleButton()]
    K --> L{Gomb<br>lenyomva?}
    L -->|Igen| M[Menü léptetése<br>currentMenu változik,<br>showMenuFunction(), buzzer]
    L -->|Nem| N[Button: nincs esemény]

    J --> O
    M --> O
    N --> O[handleDHT()]
    O --> P{Eltelt<br>2 másodperc?}
    P -->|Nem| Q
    P -->|Igen| R[DHT11 olvasás<br>lastH, lastT, lastHI frissítése]

    Q --> S
    R --> S[handlePIR()]

    S --> T[PIR állapot olvasása<br>pirActive frissítése,<br>LED_ACT / LED_NACT állítása]
    T --> U[handleScroll()]
    U --> V[Ha scrollingActive igaz<br>szöveg görgetése az LCD-n]
    V --> F
```




---

## About

Ez a repository a **Nyíregyházi Egyetem** IoT alapok tantárgyához készült.  
A projekt célja az volt, hogy egy **valós, működő IoT rendszeren keresztül** mutassa be a
szenzorok, aktuátorok, mikrokontroller és felhasználói interfész (LCD, távirányító) együttműködését.
