#include "main.h"

#include <Arduino.h>
#include <SD.h>

// Pins
const uint8_t kPinChargeDischarge = 2;      // Pin controlling relay connecting Cap to battery or coils
const uint8_t kPinCoilSwitch = 6;           // Pin controlling relay connecting coil 1 or 2
const uint8_t kPinCapacitor = A7;           // Pin measuring capacitor voltage
const uint8_t kPinSensingCoil = A5;         // Pin measuring sensing coil voltage

// Settings
const float kCountsToVolts = 0.0032;        // Constant for translating ADC counts to Volts; Vref/1024
const float kVmax = 3.0;                    // Maximum capacitor voltage
const uint16_t kDischargeTime = 1000;       // Discharge for a total of [ms]
const uint16_t kSensorDelay = 1200;         // time to wait before measuring sensor volts; [us]
const uint16_t kSensorInterval = 1;         // Interval between measurements; [ms]
const uint16_t kResultsArrayLength = 30;    // Number of measurements
const uint16_t kMaxChargeCycles = 10000;    // Max number of charging cycles before giving up
const uint16_t kDischargeCycleDelay = 100;  // Wait for [ms] after a measurement cycle
const uint16_t kTransientDelay = 10;        // Amount of [ms] to wait for transients to subside
const uint16_t kChargingInterval = 10000;   // Charge for [ms] before measuring voltage

bool coil1_active = true;                   // Specifies which coil should be used for discharge

void setup() {
    Serial.begin(9600);
    pinMode(kPinChargeDischarge, OUTPUT);
    pinMode(kPinCoilSwitch, OUTPUT);
    pinMode(kPinCapacitor, INPUT);
    pinMode(kPinSensingCoil, INPUT);
    // analogReference(EXTERNAL);  // reference is 3.3V
}

void loop() {
    ChargeCapacitor(kVmax, kChargingInterval);
    float* results = Measure(coil1_active, kSensorDelay, kSensorInterval, kDischargeTime);
    // WriteResultsToSD(results);
    delay(kDischargeCycleDelay);
    delete[] results;
    coil1_active = !coil1_active;
}

/**
 * Charge capacitor until it exceeds vmin
 *
 * @param vmin Capacitor must exceed this voltage to stop charging.
 * @param interval Charge capacitor for [ms] before measuring again.
 * @return float Final capacitor voltage.
 */
float ChargeCapacitor(const float vmin, const unsigned int interval) {
    float cap_volts = 0;
    Serial.print("Charging capacitor\n");
    for (uint16_t i = 0; i < kMaxChargeCycles; i++) {
        digitalWrite(kPinChargeDischarge, HIGH);
        delay(kTransientDelay);
        cap_volts = analogRead(kPinCapacitor) * kCountsToVolts;
        digitalWrite(kPinChargeDischarge, LOW);

        Serial.print(cap_volts);
        Serial.print(" V\n");

        if (cap_volts > vmin) {
            Serial.print("Capacitor charged\n");
            return cap_volts;
        }

        delay(interval);
    }
    Serial.print("kMaxChargeCycles reached! Aborting charging at ");
    Serial.print(cap_volts);
    Serial.print(" V.");
    return cap_volts;
}

/**
 * Discharge capacitor into a coil and measure voltage on sensing coil
 *
 * @param coil1_active Whether to energize coil 1.
 * @param initial_delay Wait for [us] before starting measurements.
 * @param interval Interval [us] between measurements.
 * @param total_time Total discharge time.
 * @return float* An array of the measured voltages.
 */
float* Measure(const bool coil1_active, const unsigned int initial_delay,
               const unsigned int interval, const unsigned int total_time) {
    float* results = new float[kResultsArrayLength];

    Serial.print("Preparing to measure\n");
    if (coil1_active == 1) {
        Serial.print("Coil 1 energized\n");
    } else {
        digitalWrite(kPinCoilSwitch, HIGH);
        Serial.print("Coil 2 energized\n");
    }
    delay(kTransientDelay);

    Serial.print("Discharging capacitor\n");
    digitalWrite(kPinChargeDischarge, HIGH);
    delayMicroseconds(initial_delay);

    unsigned long tstart = millis();
    for (uint16_t j = 0; j < kResultsArrayLength; j++) {
        float sensor_volts = analogRead(kPinSensingCoil) * kCountsToVolts;
        results[j] = sensor_volts;
        delayMicroseconds(interval);
    }
    unsigned long tcheck = millis() - tstart;

    Serial.print("Measurement finished, discharging up to specified time\n");
    // finish up the discharge (to make sure all the ferrofluid is on one side?)
    delay(kDischargeTime - tcheck);
    digitalWrite(kPinChargeDischarge, LOW);
    digitalWrite(kPinCoilSwitch, LOW);  // always turn off relays to reduce current (56mA)

    Serial.print("Results: ");
    for (uint16_t j = 0; j < kResultsArrayLength; j++) {
        Serial.print(results[j]);
        Serial.print(" ");
    }
    Serial.print("\n");

    return results;
}
