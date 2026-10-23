#include <Arduino.h>
#include "hal_motor.h"
#include "entrada_emergencia.h"

void setup() {
    Serial.begin(115200);

    motorInit();
    emergenciaInit();

    Serial.println("Proyecto 1: Banda transportadora");
    Serial.println("Motor detenido. Entrada de emergencia preparada.");
}

void loop() {
    uint32_t totalPulsaciones = 0;

    if (emergenciaConsumirEvento(totalPulsaciones)) {
        motorBrake();

        Serial.print("Emergencia detectada. Pulsaciones: ");
        Serial.println(totalPulsaciones);
    }
}