#pragma once

#include <Arduino.h>

// Configura el PWM y deja el motor detenido.
void motorInit();

// Acciona el motor hacia adelante: velocidad de 0 a 1023.
void motorForward(uint16_t duty);

// Aplica el frenado dinamico.
void motorBrake();

// Deja el motor en parada libre.
void motorCoast();