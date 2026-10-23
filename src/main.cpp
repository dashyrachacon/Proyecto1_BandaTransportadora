#include <Arduino.h>

#include "hal_motor.h"
#include "entrada_emergencia.h"
#include "fsm_control.h"

namespace {
// Duracion elegida para demostrar el evento TIMEOUT.
constexpr uint32_t TIEMPO_MARCHA_MS = 10000UL;

uint32_t inicioMarcha = 0;
Estado estadoAnterior = Estado::IDLE;

void mostrarEstado(Estado estado) {
    constexpr const char* nombres[] = {
        "IDLE",
        "RUNNING",
        "FAULT"
    };

    Serial.print("Estado: ");
    Serial.println(nombres[static_cast<uint8_t>(estado)]);
}

Evento leerComando() {
    if (Serial.available() <= 0) {
        return Evento::NONE;
    }

    const char comando = static_cast<char>(Serial.read());

    switch (comando) {
        case 'a':
        case 'A':
            return Evento::START;

        case 'p':
        case 'P':
            return Evento::STOP;

        case 'r':
        case 'R':
            return Evento::RESET;

        default:
            return Evento::NONE;
    }
}
}

void setup() {
    Serial.begin(115200);

    motorInit();
    emergenciaInit();
    fsmInit();

    estadoAnterior = fsmEstadoActual();

    Serial.println("Proyecto 1: Banda transportadora");
    Serial.println("A: arrancar | P: parar | R: restablecer");
    Serial.println("Parada automatica tras 10 segundos de marcha.");

    mostrarEstado(estadoAnterior);
}

void loop() {
    uint32_t totalPulsaciones = 0;
    Evento evento = Evento::NONE;

    const bool hayEmergencia =
        emergenciaConsumirEvento(totalPulsaciones);

    // La emergencia tiene prioridad sobre los comandos.
    if (hayEmergencia || emergenciaActiva()) {
        evento = Evento::BTN_PRESS;
    } else {
        evento = leerComando();

        if (fsmEstadoActual() == Estado::RUNNING &&
            static_cast<uint32_t>(millis() - inicioMarcha)
                >= TIEMPO_MARCHA_MS) {
            evento = Evento::TIMEOUT;
        }
    }

    // Selecciona y ejecuta el handler mediante la matriz.
    fsmProcesar(evento);

    const Estado estadoActual = fsmEstadoActual();

    if (estadoActual != estadoAnterior) {
        if (estadoActual == Estado::RUNNING) {
            inicioMarcha = millis();
        }

        estadoAnterior = estadoActual;
        mostrarEstado(estadoActual);
    }

    if (hayEmergencia) {
        Serial.print("Pulsaciones aceptadas: ");
        Serial.println(totalPulsaciones);
    }
}