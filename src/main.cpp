#include <Arduino.h>
#include <SD.h>

const int charge_d2 = 2;               // capacitor charge on/off
const int discharge_d4 = 4;            // capacitor discharge on/off
const int L1_ON_d6 = 6;                // L1 on/L2 off on/off
const int capacitor_voltage_a7 = A7;    // capacitor voltage measured at gpio a7
const int sensor_voltage_a5 = 5;       // sensor voltage aplified by 7
const int pb_d8 = 8;                   // skip switch
const float counts_to_volts = 0.0032;  // 0.00459; ADC Vref 3.3V
const float vmax = 3.0;                // This is the maximum capacitor voltage desired

const float discharge_time_ms = 1000;
const float sensor_time_delay_us = 1200;  // time to wait before measuring sensor volts
const int sensor_interval_us = 1;         // was sensorTime_ms/sensorArrayLength

#define sensorArrayLength 30

#define max_charge_cycles 10000

#define discharge_cycles 0

#define discharge_cycle_delay 100  // ms

// variables will change:
int pbState = 0;  // variable for reading the pushbutton status

int L1_ON = 1;  // chooses L1 on, not L2

void setup() {
    Serial.begin(9600);
    pinMode(pb_d8, INPUT);
    pinMode(charge_d2, OUTPUT);
    pinMode(discharge_d4, OUTPUT);
    // analogReference(EXTERNAL);  // reference is 3.3V
}

void loop() {
    float cap_volts = 0;
    float sensor_volts = 0;
    int capcounts = 0;
    int tstart = 0;
    int tcheck = 0;
    float sensorArray[sensorArrayLength];
    cap_volts = analogRead(capacitor_voltage_a7) * counts_to_volts;
    Serial.print("analog read speed check 10,000 times\n");
    tstart = millis();
    for (int i = 0; i < 10; i++) {
        capcounts = analogRead(capacitor_voltage_a7);
        delayMicroseconds(1);
    }
    tcheck = millis() - tstart;
    Serial.print("analog read speed check complete for 1 msecs = ");
    Serial.print(tcheck);
    Serial.print("\n");
    for (int i = 0; i < max_charge_cycles; i++) {
        // terminate charging at vmax
        Serial.print(i);
        if (cap_volts < vmax) {
            digitalWrite(charge_d2, HIGH);  // turn on d2  to charge:
            delay(100);                     // let transient subside
        }
        cap_volts = analogRead(capacitor_voltage_a7) * counts_to_volts;
        Serial.print(" cap_volts ");
        Serial.print(cap_volts);
        delay(50);
        digitalWrite(charge_d2, LOW);
        delay(10);  // let transient subside
        Serial.print("  ");
        cap_volts = analogRead(capacitor_voltage_a7) * counts_to_volts;
        Serial.print(cap_volts);
        Serial.print("\n");

        pbState = digitalRead(pb_d8);
        if (pbState == HIGH) {
            Serial.print("pb pressed - skipping to discharge in 5s\n");
            delay(5000);
            break;
        }
        delay(10);
    }
    Serial.print("Discharging..\n");
    delay(5000);
    for (int i = 0; i < discharge_cycles; i++) {
        // see if we need to skip out
        pbState = digitalRead(pb_d8);
        if (pbState == HIGH) {
            Serial.print("pb pressed - exit\n");
            exit(0);
        }
        // choose the coil to power
        if (L1_ON == 1) {
            digitalWrite(L1_ON_d6, HIGH);
            Serial.print("L1 ");
        } else {
            Serial.print("L2 ");
        }

        delay(10);  // to give relay K3-K4-d6 time to turn on

        digitalWrite(discharge_d4, HIGH);
        delayMicroseconds(sensor_time_delay_us);
        // read the voltages into sensor array
        tstart = millis();
        for (int j = 0; j < sensorArrayLength; j++) {
            sensor_volts = analogRead(sensor_voltage_a5) * counts_to_volts;
            sensorArray[j] = sensor_volts;
            delayMicroseconds(sensor_interval_us);
        }
        tcheck = millis() - tstart;
        // finish up the discharge
        delay(discharge_time_ms - tcheck);
        digitalWrite(discharge_d4, LOW);
        digitalWrite(L1_ON_d6, LOW);  // always turn off d6 to reduce current (56mA)

        for (int j = 0; j < sensorArrayLength; j++) {
            Serial.print(sensorArray[j]);
            Serial.print(" ");
        }
        Serial.print("\n");

        delay(discharge_cycle_delay);
        // switch coils
        if (L1_ON == 1)
            L1_ON = 1;  // always turn on L1
        else
            L1_ON = 1;
    }

    Serial.print("sensor read time in msecs ");
    Serial.print(tcheck);
    Serial.print("\ncap_volts ");
    cap_volts = analogRead(capacitor_voltage_a7) * counts_to_volts;
    Serial.print(cap_volts);
    Serial.print("\n");
    Serial.print("Done\n");
    delay(1000);  // give the serial thread time to finish  output
    exit(0);
}
