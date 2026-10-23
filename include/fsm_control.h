#pragma once

#include <Arduino.h>

enum class Estado : uint8_t {
    IDLE,
    RUNNING,
    FAULT,
    COUNT
};

enum class Evento : uint8_t {
    NONE,
    BTN_PRESS,
    TIMEOUT,
    START,
    STOP,
    RESET,
    COUNT
};

// Inicializa la maquina de estados.
void fsmInit();

// Atiende un evento mediante la matriz de transiciones.
void fsmProcesar(Evento evento);

// Permite consultar el estado actual.
Estado fsmEstadoActual();