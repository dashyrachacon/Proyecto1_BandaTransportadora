#include "entrada_emergencia.h"

#include <freertos/FreeRTOS.h>
#include <xtensa/core-macros.h>

namespace {
constexpr uint8_t PIN_EMERGENCIA = 33;

// 25 ms con la CPU configurada a 160 MHz.
constexpr uint32_t CICLOS_REBOTE = 4000000UL;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

volatile bool eventoPendiente = false;
volatile uint32_t pulsaciones = 0;
volatile uint32_t ultimoCiclo = 0;
volatile bool filtroActivo = false;

void IRAM_ATTR interrupcionEmergencia() {
    const uint32_t ahora = XTHAL_GET_CCOUNT();

    portENTER_CRITICAL_ISR(&mux);

    const uint32_t transcurridos = ahora - ultimoCiclo;

    if (!filtroActivo || transcurridos >= CICLOS_REBOTE) {
        ultimoCiclo = ahora;
        filtroActivo = true;
        eventoPendiente = true;
        pulsaciones++;
    }

    portEXIT_CRITICAL_ISR(&mux);
}
}

void emergenciaInit() {
    pinMode(PIN_EMERGENCIA, INPUT_PULLUP);

    portENTER_CRITICAL(&mux);
    eventoPendiente = false;
    pulsaciones = 0;
    ultimoCiclo = 0;
    filtroActivo = false;
    portEXIT_CRITICAL(&mux);

    attachInterrupt(
        digitalPinToInterrupt(PIN_EMERGENCIA),
        interrupcionEmergencia,
        FALLING
    );
}

bool emergenciaConsumirEvento(uint32_t& totalPulsaciones) {
    const uint32_t ahora = XTHAL_GET_CCOUNT();

    portENTER_CRITICAL(&mux);

    const bool hayEvento = eventoPendiente;
    eventoPendiente = false;
    totalPulsaciones = pulsaciones;

    // Rearma el filtro desde loop() al terminar los 25 ms.
    // Evita conservar una marca antigua durante largas pausas.
    if (filtroActivo &&
        static_cast<uint32_t>(ahora - ultimoCiclo) >= CICLOS_REBOTE) {
        filtroActivo = false;
    }

    portEXIT_CRITICAL(&mux);

    return hayEvento;
}

bool emergenciaActiva() {
    return digitalRead(PIN_EMERGENCIA) == LOW;
}