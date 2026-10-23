#include <Arduino.h>
#include "hal_motor.h"

void setup() {
    Serial.begin(115200);

    motorInit();

    Serial.println("Proyecto 1: Banda transportadora");
    Serial.println("PWM configurado. Motor detenido.");
}

void loop() {
    // Aqui agregaremos la maquina de estados.
}