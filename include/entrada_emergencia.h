#pragma once

#include <Arduino.h>

// Configura IO33 y su interrupcion.
void emergenciaInit();

// Lee y limpia el evento pendiente de forma protegida.
// Tambien entrega el total de pulsaciones aceptadas.
bool emergenciaConsumirEvento(uint32_t& totalPulsaciones);

// Indica si el microinterruptor permanece presionado.
bool emergenciaActiva();