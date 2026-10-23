# Proyecto 1: Firmware controlador de banda transportadora

**Estudiante:** Dashyra Priscila Chacón Carvajal  
**Curso:** Programación II  
**Universidad:** Universidad Técnica Nacional

## Descripción

Proyecto desarrollado para la IdeaBoard de CRCibernética, basada en ESP32. El firmware organiza el control de un motor mediante una máquina de estados orientada a eventos.

Incluye PWM por hardware mediante LEDC, una interrupción para el microinterruptor de emergencia y un filtro de rebotes basado en el contador de ciclos CCOUNT. Las variables compartidas entre la interrupción y el programa principal se protegen mediante un spinlock.

## Organización del proyecto

| Archivo | Función |
|---|---|
| platformio.ini | Configuración de la placa, framework, CPU y monitor serial. |
| include/hal_motor.h | Declaraciones de las funciones del motor. |
| src/hal_motor.cpp | Configuración LEDC y funciones de avance y parada. |
| include/entrada_emergencia.h | Interfaz del módulo de emergencia. |
| src/entrada_emergencia.cpp | Interrupción, filtro de rebotes y lectura protegida de eventos. |
| include/fsm_control.h | Definición de estados, eventos e interfaz de la FSM. |
| src/fsm_control.cpp | Matriz de transiciones y funciones controladoras. |
| src/main.cpp | Inicialización, recepción de comandos y despacho de eventos. |
| CUESTIONARIO.md | Respuestas teóricas y referencias. |

## Configuración

- Placa de PlatformIO: esp32dev.
- Framework: Arduino.
- Frecuencia de CPU: 160 MHz.
- Monitor serial: 115200 baudios.
- Temporizador PWM: LEDC_TIMER_0.
- Resolución PWM: 10 bits.
- Frecuencia PWM: 1000 Hz.
- Canales utilizados: LEDC_CHANNEL_0 y LEDC_CHANNEL_1.
- Filtro de rebotes: 4 000 000 de ciclos, equivalentes a 25 ms a 160 MHz.

## Conexiones previstas

Las conexiones corresponden a la configuración indicada en la guía del profesor.

| Elemento | Conexión o configuración |
|---|---|
| Motor DC DG01D | Salida del controlador L9110S asociada a IO12 e IO14. |
| Microinterruptor | Entre IO33 y GND. |
| Entrada IO33 | INPUT_PULLUP e interrupción FALLING. |
| Jumper de alimentación | SELECT-Vin. |
| Alimentación del motor | Fuente de 5 V/3 A mediante el conector de barril. |

Este montaje todavía no se ha realizado ni verificado físicamente.

## Funcionamiento de la máquina de estados

La FSM utiliza una matriz bidimensional constexpr de punteros a funciones. El estado actual selecciona la fila y el evento selecciona la columna.

| Estado | Comportamiento previsto |
|---|---|
| IDLE | Motor sin orden de avance; espera una orden de arranque. |
| RUNNING | Motor accionado hacia adelante. |
| FAULT | Frenado solicitado; arranque bloqueado hasta restablecer. |

Los eventos son NONE, BTN_PRESS, TIMEOUT, START, STOP y RESET.

### Transiciones principales

| Estado actual | Evento | Resultado |
|---|---|---|
| IDLE | START, con interruptor liberado | RUNNING. |
| IDLE | BTN_PRESS | FAULT. |
| RUNNING | BTN_PRESS | FAULT y solicitud de frenado. |
| RUNNING | STOP | IDLE y solicitud de parada libre. |
| RUNNING | TIMEOUT | IDLE y solicitud de parada libre. |
| FAULT | RESET, con interruptor liberado | IDLE, sin arrancar el motor. |
| FAULT | START | Permanece en FAULT. |

Las combinaciones restantes conservan el estado. Además, la función de arranque comprueba el nivel del microinterruptor antes de accionar el motor.

## Comandos por monitor serial

| Letra | Acción |
|---|---|
| A o a | Solicita el arranque. |
| P o p | Solicita la parada normal. |
| R o r | Solicita el restablecimiento de una falla. |

Liberar el microinterruptor no reinicia automáticamente el motor. Se necesita restablecer con R y después arrancar con A.

El sistema da prioridad a la emergencia sobre la lectura de comandos.

## Decisiones de implementación

- Se utiliza un ciclo de trabajo de 600 sobre el intervalo de 0 a 1023 para la marcha.
- Se genera TIMEOUT después de 10 segundos de marcha.
- Los comandos START, STOP y RESET se reciben mediante letras por el monitor serial.
- La velocidad inicial y los 10 segundos son decisiones de esta implementación, no valores exigidos por la guía. Su adecuación al motor está pendiente de prueba.
- Las funciones de frenado y parada libre siguen las combinaciones utilizadas en el ejemplo de motores del repositorio de CRCibernética. Su comportamiento físico en la tarjeta utilizada debe verificarse.

## Interrupción y concurrencia

La ISR utiliza IRAM_ATTR y lee XTHAL_GET_CCOUNT(). Su trabajo se limita al filtrado por ciclos y a actualizar una bandera de evento y un contador de pulsaciones.

No se utilizan delay(), vTaskDelay() ni impresiones seriales dentro de la ISR.

La ISR y el programa principal utilizan el mismo portMUX_TYPE para proteger los datos compartidos. La lectura y limpieza de la bandera se realizan dentro de una sección crítica.

El programa principal recoge el evento y lo entrega a la FSM. El control del motor y las impresiones se realizan fuera de la ISR.

El filtro se rearma desde loop() después del intervalo de 25 ms. Esta estrategia requiere que loop() continúe ejecutándose regularmente.

## Cómo compilar

1. Abrir la carpeta del proyecto en Visual Studio Code.
2. Tener instalada la extensión PlatformIO IDE.
3. Ejecutar PlatformIO: Build desde la paleta de comandos.
4. Comprobar que el proceso termine con SUCCESS.

También se puede ejecutar `pio run` desde una terminal con PlatformIO disponible.

La compilación genera el firmware, pero no lo carga en una tarjeta ni demuestra su funcionamiento físico.

## Verificación realizada

El proyecto compiló correctamente en PlatformIO después de integrar la HAL, el módulo de emergencia y la máquina de estados.

También se verificó una compilación posterior a la corrección que coloca la lectura de CCOUNT dentro de la sección crítica de emergenciaConsumirEvento().

Se realizaron commits y envíos incrementales al repositorio de GitHub.

## Material de referencia

- Guía del profesor: Proyecto 1, Firmware Controlador Banda Transportadora.
- Bibliografía del cuestionario: consultar CUESTIONARIO.md.
- Plantilla del profesor:
  https://github.com/arnoldoalcides/project1-draft
- Documentación y ejemplos de la IdeaBoard:
  https://github.com/CRCibernetica/espressif-ideaboard
- Documentación de PlatformIO:
  https://docs.platformio.org/en/latest/

## Repositorio

https://github.com/dashyrachacon/Proyecto1_BandaTransportadora.git
