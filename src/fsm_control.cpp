#include "fsm_control.h"
#include "hal_motor.h"
#include "entrada_emergencia.h"

namespace {
constexpr uint16_t VELOCIDAD = 600;

Estado estadoActual = Estado::IDLE;

void sinAccion() {
}

void entrarFalla() {
    motorBrake();
    estadoActual = Estado::FAULT;
}

void iniciarMotor() {
    // No permite arrancar con el interruptor presionado.
    if (emergenciaActiva()) {
        entrarFalla();
        return;
    }

    motorForward(VELOCIDAD);
    estadoActual = Estado::RUNNING;
}

void detenerMotor() {
    motorCoast();
    estadoActual = Estado::IDLE;
}

void restablecer() {
    // Restablecer no arranca el motor.
    if (!emergenciaActiva()) {
        motorCoast();
        estadoActual = Estado::IDLE;
    }
}

using Handler = void (*)();

constexpr uint8_t NUM_ESTADOS =
    static_cast<uint8_t>(Estado::COUNT);

constexpr uint8_t NUM_EVENTOS =
    static_cast<uint8_t>(Evento::COUNT);

// Columnas: NONE, BTN_PRESS, TIMEOUT, START, STOP, RESET.
constexpr Handler FSM[NUM_ESTADOS][NUM_EVENTOS] = {
    // IDLE
    {
        sinAccion, entrarFalla, sinAccion,
        iniciarMotor, sinAccion, sinAccion
    },

    // RUNNING
    {
        sinAccion, entrarFalla, detenerMotor,
        sinAccion, detenerMotor, sinAccion
    },

    // FAULT
    {
        sinAccion, sinAccion, sinAccion,
        sinAccion, sinAccion, restablecer
    }
};
}

void fsmInit() {
    motorCoast();
    estadoActual = Estado::IDLE;

    if (emergenciaActiva()) {
        entrarFalla();
    }
}

void fsmProcesar(Evento evento) {
    const uint8_t fila = static_cast<uint8_t>(estadoActual);
    const uint8_t columna = static_cast<uint8_t>(evento);

    // Comprueba los indices antes de consultar la matriz.
    if (fila >= NUM_ESTADOS || columna >= NUM_EVENTOS) {
        entrarFalla();
        return;
    }

    FSM[fila][columna]();
}

Estado fsmEstadoActual() {
    return estadoActual;
}