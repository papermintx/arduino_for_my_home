#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <TaskScheduler.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

class SensorPintu {
public:
    int pinSensorPintu;
    int pinBuzzer;
    int pinTombol; 

    bool alarmAktif = false;
    bool statusTombol = false;
    unsigned long waktuTombolTerakhir = 0;
    const unsigned long debounceDelay = 200; 

    SensorPintu(int _pinSensorPintu, int _pinBuzzer, int _pinTombol) {
        pinSensorPintu = _pinSensorPintu;
        pinBuzzer = _pinBuzzer;
        pinTombol = _pinTombol;
    }

    void init() {
        pinMode(pinSensorPintu, INPUT);
        pinMode(pinBuzzer, OUTPUT);
        pinMode(pinTombol, INPUT_PULLUP); 
    }

    void cekPintu() {
        int statePintu = digitalRead(pinSensorPintu);
        int currentTombolState = digitalRead(pinTombol);
        unsigned long currentMillis = millis();

        if (currentTombolState == LOW && (currentMillis - waktuTombolTerakhir >= debounceDelay)) {
            alarmAktif = !alarmAktif; 
            waktuTombolTerakhir = currentMillis;
            Serial.print("Alarm ");
            Serial.println(alarmAktif ? "Aktif" : "Nonaktif");
        }

        if (alarmAktif) {
            if (statePintu == HIGH) { 
                Serial.println("Pintu Terbuka! Alarm Aktif.");
                digitalWrite(pinBuzzer, HIGH);
            } else { 
                digitalWrite(pinBuzzer, LOW); 
            }
        } else {
            digitalWrite(pinBuzzer, LOW); 
        }
    }
};

class Keamanan {
public:
    String namaSensor;
    int pinSensorGerak;
    int pinBuzzerPerubahan;
    int batasPerubahan;

    int intervalBunyi = 200;
    int jumlahBunyi = 4;

    unsigned long waktuBunyiTerakhir;
    int counterBunyi;
    int counterSensorGerak;

    int stateSensorGerak = LOW;
    int lastStateSensorGerak;

    Keamanan(String _namaSensor, int _pinSensorGerak, int _pinBuzzerPerubahan, int _batasPerubahan) {
        pinSensorGerak = _pinSensorGerak;
        namaSensor = _namaSensor;
        pinBuzzerPerubahan = _pinBuzzerPerubahan;
        batasPerubahan = _batasPerubahan;
    }

    void init() {
        pinMode(pinSensorGerak, INPUT);
        pinMode(pinBuzzerPerubahan, OUTPUT);
    }

    void cekSensorGerak() {
        stateSensorGerak = digitalRead(pinSensorGerak);

        if (stateSensorGerak != lastStateSensorGerak) {
            if (stateSensorGerak == HIGH && lastStateSensorGerak == LOW) {
                counterSensorGerak++;
                Serial.println(namaSensor + " - Sensor Gerak: HIGH");
                Serial.println(namaSensor + " - Counter: " + String(counterSensorGerak));
                bunyikanBuzzerCepat();
            }
            lastStateSensorGerak = stateSensorGerak;
        }
    }

    void bunyikanBuzzerCepat() {
        unsigned long waktuSekarang = millis();
        if (waktuSekarang - waktuBunyiTerakhir >= intervalBunyi && counterBunyi < jumlahBunyi) {
            digitalWrite(pinBuzzerPerubahan, HIGH);
            waktuBunyiTerakhir = waktuSekarang;
            counterBunyi++;
        }

        if (waktuSekarang - waktuBunyiTerakhir >= 70) {
            digitalWrite(pinBuzzerPerubahan, LOW);
        }

        if (counterBunyi >= jumlahBunyi) {
            counterBunyi = 0;
            digitalWrite(pinBuzzerPerubahan, LOW);
        }
    }
};

class SensorLDR {
public:
    int nilaiLDR;
    int pinLDR;
    int pinLampu;
    int ambangBatas = 840; 

    bool lampuMenyala = false;
    bool statusGelap = false;
    unsigned long waktuPerubahanTerakhir = 0;

    SensorLDR(int _pinLDR, int _pinLampu) {
        pinLDR = _pinLDR;
        pinLampu = _pinLampu;
    }

    void init() {
        pinMode(pinLDR, INPUT);
        pinMode(pinLampu, OUTPUT);
    }

    void read() {
        nilaiLDR = analogRead(pinLDR);
        bool kondisiGelapSaatIni = (nilaiLDR < ambangBatas);

        if (kondisiGelapSaatIni != statusGelap) {
            waktuPerubahanTerakhir = millis();
            statusGelap = kondisiGelapSaatIni;
        }

        if (millis() - waktuPerubahanTerakhir >= 8000) {
            if (statusGelap && !lampuMenyala) {
                digitalWrite(pinLampu, HIGH);
                lampuMenyala = true;
                Serial.println("Kondisi Gelap, Lampu Menyala");
            } else if (!statusGelap && lampuMenyala) {
                digitalWrite(pinLampu, LOW);
                lampuMenyala = false;
                Serial.println("Kondisi Terang, Lampu Mati");
            }
        }
    }
};

#define pinSensorGerak1 4 
#define pinSensorGerak2 3
#define pinSensorPintu 5
#define pinBuzzerPerubahan 8
#define pinLampuPintu 9
#define pinLdr 14
#define pinLampuLdr 11
#define pinTombol 7 

SensorLDR sensorLampu(pinLdr, pinLampuLdr);
Keamanan sensorKeamanan("Sensor 1", pinSensorGerak1, pinBuzzerPerubahan, 8);
Keamanan sensorKeamanan2("Sensor 2", pinSensorGerak2, pinBuzzerPerubahan, 8);
SensorPintu sensorPintu(pinSensorPintu, pinBuzzerPerubahan, pinTombol);

Scheduler runner;

void setup() {
    Serial.begin(9600);
    sensorKeamanan.init();
    sensorKeamanan2.init();
    sensorLampu.init();
    sensorPintu.init();
    lcd.init();
    lcd.backlight();
}

void loop() {
    runner.execute();

    sensorKeamanan.cekSensorGerak();
    sensorKeamanan2.cekSensorGerak();
    sensorLampu.read();
    sensorPintu.cekPintu();
    showDisplay();
}

void showDisplay() {
    lcd.setCursor(0, 0);
    lcd.print("S1:"); 
    lcd.print(sensorKeamanan.stateSensorGerak == HIGH ? "1" : "0");
    lcd.setCursor(5, 0); 
    lcd.print("S2:"); 
    lcd.print(sensorKeamanan2.stateSensorGerak == HIGH ? "1" : "0");
    lcd.setCursor(10, 0); 
    lcd.print("P:"); 
    lcd.print(sensorPintu.alarmAktif ? "1" : "0");
    lcd.setCursor(0, 1); 
    lcd.print("C:"); 
    lcd.print(sensorKeamanan.counterSensorGerak);
    lcd.setCursor(8, 1);
    lcd.print("L:"); 
    lcd.print(sensorLampu.lampuMenyala ? "1" : "0");
}
