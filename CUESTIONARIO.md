
# Cuestionario de investigación

## Proyecto 1: Firmware controlador de banda transportadora

### 1. Contexto de una tarea y contexto de una interrupción

Una tarea de FreeRTOS se ejecuta bajo el control del planificador y puede pasar temporalmente al estado bloqueado mientras espera un evento o un intervalo de tiempo. Por ejemplo, vTaskDelay() permite suspender una tarea durante una cantidad determinada de ticks.

Una rutina de servicio de interrupción (ISR) atiende un evento que interrumpe la ejecución normal del programa. Debe realizar un trabajo breve y regresar rápidamente; el procesamiento más extenso debe dejarse para una tarea.

No se debe llamar a vTaskDelay() desde una ISR, porque es una operación bloqueante diseñada para tareas. Una ISR no puede suspenderse como si fuera una tarea del planificador. Deben utilizarse únicamente las operaciones permitidas para el contexto de interrupción. [1]

### 2. Diferencia entre mutex y spinlock

Un mutex permite que una tarea tenga acceso exclusivo a un recurso compartido. Si el recurso está ocupado, otra tarea puede quedar bloqueada hasta que el propietario lo libere.

Un spinlock protege una sección crítica mediante un mecanismo de exclusión entre los núcleos. Si otro núcleo tiene el bloqueo, el núcleo que lo solicita espera activamente hasta adquirirlo, en lugar de bloquear una tarea mediante el planificador. Por eso, la sección protegida debe ser muy breve.

En este proyecto, el profesor solicita utilizar un spinlock SMP de tipo portMUX_TYPE para proteger las variables compartidas entre loop() y la ISR. En la interrupción se emplean portENTER_CRITICAL_ISR() y portEXIT_CRITICAL_ISR(); en la tarea, las variantes correspondientes para contexto de tarea, utilizando el mismo spinlock.

Un mutex de tareas no es apropiado para esta comunicación porque no debe adquirirse desde una ISR. La sección crítica debe limitarse a leer o actualizar los datos compartidos, sin imprimir mensajes ni controlar procesos largos dentro de ella. [1], [2]

### 3. Inversión de prioridad y herencia de prioridad

La inversión de prioridad ocurre cuando una tarea de alta prioridad espera un recurso ocupado por una tarea de baja prioridad. El problema aumenta si una tarea de prioridad intermedia interrumpe a la de baja prioridad, porque retrasa la liberación del recurso y, de manera indirecta, a la tarea de alta prioridad.

Los mutex de FreeRTOS incorporan herencia de prioridad. Cuando una tarea de alta prioridad espera un mutex, la tarea que lo posee recibe temporalmente esa prioridad. Así puede avanzar sin que las tareas de prioridad intermedia la retrasen y liberar el recurso.

En el caso sencillo de un único mutex, después de liberarlo la tarea recupera su prioridad original. Este mecanismo reduce la inversión de prioridad, pero no evita por sí solo los deadlocks. [3].

### 4. Condiciones de Coffman y bloqueo entre una tarea y una ISR

Para que exista un deadlock o bloqueo mutuo deben cumplirse simultáneamente cuatro condiciones necesarias:

1. Exclusión mutua: al menos un recurso solo puede ser utilizado por una tarea o un proceso a la vez.

2. Retención y espera: una tarea conserva un recurso mientras espera obtener otro.

3. Ausencia de expropiación: el recurso no se retira por la fuerza; quien lo posee debe liberarlo.

4. Espera circular: existe una cadena en la que cada participante espera un recurso o una condición que depende del siguiente.

En el caso planteado por el profesor, la secuencia problemática sería:

1. La tarea que ejecuta loop() adquiere un mutex.

2. Una ISR interrumpe esa tarea en el mismo núcleo.

3. La ISR intenta adquirir el mismo mutex y espera su liberación.

4. La tarea no puede continuar para liberarlo hasta que termine la ISR.

5. La ISR no termina porque sigue esperando el mutex.

Se forma una dependencia circular: la ISR espera el recurso y la tarea necesita recuperar la ejecución para liberarlo.

Esta secuencia es un análisis del bloqueo que produciría esa espera. FreeRTOS no permite utilizar los mutex de esta manera desde una ISR; una llamada incorrecta también puede producir un fallo antes de llegar al bloqueo descrito. [2], [3], [4]

### 5. Caché de Flash e IRAM_ATTR

El ESP32 puede ejecutar instrucciones almacenadas en la memoria Flash externa mediante una caché. Durante operaciones de escritura o borrado de Flash, como las utilizadas por NVS o una actualización OTA, el acceso mediante esa caché puede quedar deshabilitado temporalmente.

Si una ISR se ejecuta durante ese intervalo y necesita instrucciones almacenadas en Flash, puede provocar un fallo al intentar acceder a ellas.

IRAM_ATTR permite colocar el código de la ISR en la memoria interna de instrucciones, llamada IRAM. De esta manera, la rutina no depende de recuperar sus propias instrucciones desde la Flash.

Para que toda la ISR sea segura durante ese intervalo, las funciones que llama y los datos que utiliza también deben estar disponibles sin depender de la Flash. Colocar únicamente la función principal en IRAM no garantiza por sí solo esa condición.

La ausencia de IRAM_ATTR no produce necesariamente un fallo en cada interrupción: el problema aparece cuando una rutina dependiente de Flash se ejecuta mientras esa memoria no está disponible mediante la caché. [1], [2]

### 6. Lecturas fragmentadas de un contador de 64 bits

El ESP32 del proyecto trabaja con registros de propósito general de 32 bits. La lectura ordinaria de una variable de 64 bits puede requerir acceder por separado a sus dos mitades de 32 bits. Por ello, no debe suponerse que esa lectura sea una operación indivisible.

Si la ISR modifica el contador entre ambas lecturas, loop() puede combinar una mitad del valor anterior con una mitad del valor nuevo. El resultado no representa una lectura coherente del contador.

Ejemplo propio, expresado en hexadecimal:

- Valor anterior: 0x00000000FFFFFFFF.

- Valor después de incrementar: 0x0000000100000000.

- loop() lee primero la mitad inferior anterior: FFFFFFFF.

- La ISR incrementa el contador.

- loop() lee la mitad superior nueva: 00000001.

- El valor combinado resulta 0x00000001FFFFFFFF.

Ese resultado no corresponde a ninguno de los dos valores completos. Para evitarlo, la lectura y la actualización deben coordinarse con el mismo mecanismo de protección. volatile, por sí solo, no convierte una lectura de 64 bits en atómica. [2]

### 7. La palabra clave volatile

El compilador puede optimizar el acceso a una variable conservando un valor en un registro o evitando lecturas repetidas cuando considera que el valor no cambia dentro del flujo normal del programa.

Si una bandera es modificada por una ISR y el compilador no contempla ese cambio externo, loop() podría seguir utilizando un valor previamente leído en lugar de consultar nuevamente la variable.

Declarar la bandera como volatile indica que sus accesos deben conservarse conforme a las reglas del compilador para objetos volátiles. Esto permite expresar que su contenido puede cambiar por una causa externa al flujo que se está ejecutando.

No significa que necesariamente se utilice el registro a2: el registro y las instrucciones concretas dependen del compilador, sus opciones y el código generado. El nivel -O2 tampoco garantiza una única transformación específica.

Además, volatile no proporciona exclusión mutua ni hace atómicas las operaciones compuestas, como incrementar un contador. Por eso, el proyecto también exige proteger los accesos compartidos mediante secciones críticas. [2]

### 8. Rebote mecánico del microinterruptor

Cuando se acciona un microinterruptor, sus contactos metálicos no siempre quedan unidos de manera estable desde el primer instante. La elasticidad, el impacto y las irregularidades de las superficies pueden provocar contactos y separaciones sucesivas antes de estabilizarse.

El microcontrolador puede detectar esas variaciones rápidas como varios flancos eléctricos, aunque la persona haya realizado una sola pulsación. Si cada flanco genera una interrupción, una pulsación puede registrarse varias veces.

La guía menciona ráfagas de 20 a 50 cambios. Esa cantidad describe el escenario propuesto; no es un valor fijo para todos los interruptores ni una consecuencia directa de trabajar a 160 MHz. Depende del dispositivo y de las condiciones de accionamiento.

El filtrado por software busca aceptar un evento válido y descartar los cambios que ocurran demasiado cerca de él. [2]

### 9. Depuración por ciclos mediante CCOUNT

delay() introduce una espera y prolonga innecesariamente la atención de la interrupción. Esto contradice el requisito de mantener la ISR breve.

millis() no debe confundirse con una función bloqueante. Su uso dentro de una ISR depende de cómo esté implementada y de las restricciones de esa plataforma. Para este proyecto, el profesor solicita utilizar directamente el contador de ciclos.

XTHAL_GET_CCOUNT() permite obtener el valor de CCOUNT y usarlo como marca de tiempo. La lectura no requiere esperar a que transcurra el intervalo de filtrado: se toma el valor actual y se compara con la marca anterior.

A una frecuencia de CPU de 160 MHz:

25 ms = 0,025 s

160 000 000 ciclos/s × 0,025 s = 4 000 000 ciclos

Por tanto, la ISR puede aceptar un evento cuando hayan transcurrido al menos 4 000 000 de ciclos desde el evento válido anterior.

La lectura del registro es una operación directa; no debe afirmarse que toda la ISR o todo el proceso de filtrado se ejecutan en un solo ciclo. [1], [2]

### 10. Aritmética modular y desbordamiento de CCOUNT

Un contador de 32 bits sin signo representa valores entre 0 y 2^32 − 1. Al superar el máximo, vuelve a cero.

Definimos:

M = 2^32 = 4 294 967 296

Si la marca anterior es a y transcurren d ciclos, la lectura actual será:

actual = (a + d) módulo M

Al realizar la resta con aritmética sin signo de 32 bits:

transcurrido = (actual − a) módulo M

Sustituyendo:

transcurrido = ((a + d) − a) módulo M

transcurrido = d módulo M

Por tanto, si 0 ≤ d < M, el resultado coincide con los ciclos transcurridos, incluso cuando el contador pasa por cero.

Ejemplo propio:

- Marca anterior: 4 294 967 000.

- Ciclos transcurridos: 500.

- Después del desbordamiento, lectura actual: 204.

- Resta sin signo: (204 − 4 294 967 000) módulo 4 294 967 296 = 500.

A 160 MHz, el periodo de desbordamiento es:

4 294 967 296 / 160 000 000 = 26,8435456 s

La condición importante es que el intervalo real entre las marcas sea menor que un periodo completo. La resta no permite distinguir 500 ciclos de 500 ciclos más una o varias vueltas completas.

En consecuencia, si el programa puede permanecer más de ese tiempo sin un evento, la estrategia de filtrado debe considerar esa limitación. La resta modular no permite conocer cualquier intervalo arbitrariamente largo. [2]

### 11. Configuración del temporizador y canales LEDC

El temporizador LEDC establece la frecuencia y la resolución de la señal PWM. Los canales utilizan la base de tiempo del temporizador seleccionado y aplican el ciclo de trabajo correspondiente a cada salida.

En este proyecto se solicita configurar LEDC_TIMER_0 con una resolución de 10 bits y una frecuencia de 1000 Hz, antes de configurar los canales asociados a los pines IO12 e IO14.

Una resolución de 10 bits divide el periodo en 1024 pasos. Para los valores ordinarios de trabajo del proyecto se utiliza el intervalo de 0 a 1023, respetando los límites de la API empleada.

Configurar primero el temporizador permite que los canales se asocien a una base de tiempo ya definida. También evita que frecuencia y resolución queden establecidas de manera accidental o incompatible.

La frecuencia de 1000 Hz y la resolución de 10 bits son requisitos de esta actividad; no se presentan como la única configuración posible para cualquier controlador L9110S. [2], [5]

### 12. Diferencia entre IO33 e IO34 y resistencia pull-up

El proyecto conecta el microinterruptor entre IO33 y GND y utiliza INPUT_PULLUP para mantener un nivel alto cuando el contacto está abierto. Al cerrar el contacto, la entrada pasa a nivel bajo.

IO34 pertenece al grupo de pines de entrada que no dispone de resistencias internas pull-up ni pull-down. Por eso, activar INPUT_PULLUP por software no proporciona en ese pin la resistencia que necesita el circuito.

Si se trasladara el interruptor a IO34 sin una resistencia externa, la entrada podría quedar flotante mientras el contacto está abierto y producir lecturas o interrupciones no deseadas.

Para utilizar ese pin con el mismo principio de funcionamiento se necesita una resistencia pull-up externa hacia 3,3 V. El interruptor seguiría conectándose entre la entrada y GND.

El problema no significa que IO34 sea inutilizable como entrada; significa que no dispone de la polarización interna utilizada en el circuito original. [2], [5]

### 13. Matriz de transición y complejidad O(1)

Una matriz de transición utiliza el estado actual como índice de fila y el evento como índice de columna. Con ambos índices válidos se consulta directamente la entrada que contiene la transición o la función que debe ejecutarse.

No es necesario recorrer todas las combinaciones para encontrar la correspondiente. Por ello, la selección mediante acceso directo al arreglo tiene complejidad O(1).

Esto se refiere a la selección de la transición. El tiempo que tarde la función ejecutada depende del trabajo que realice; una función extensa no se vuelve constante ni breve solamente por estar dentro de la matriz.

La matriz también facilita revisar que cada combinación de estado y evento tenga un comportamiento definido.

Sin embargo, no puede afirmarse que siempre sea más rápida que cualquier estructura switch-case. El compilador puede transformar un switch en una tabla de saltos, y las llamadas indirectas mediante punteros también tienen un costo. El resultado depende del procesador, del compilador y del código generado.

En este proyecto, la ventaja demostrable es la consulta directa y una organización explícita de las transiciones. Para afirmar una mejora concreta en predicción de bifurcaciones sería necesario revisar el ensamblador o realizar mediciones. [2]

## Referencias

[1] N. Kolban. Kolban’s Book on ESP32. Septiembre de 2018. PDF proporcionado por el profesor.

[2] Guía del curso. Proyecto 1: Firmware Controlador Banda Transportadora. Ingeniería Electrónica, Programación II, UTN. PDF proporcionado por el profesor.

[3] FreeRTOS. “Mutexes”. Documentación oficial. Fuente complementaria para las preguntas 3 y 4.

https://freertos.org/Documentation/02-Kernel/02-Kernel-features/02-Queues-mutexes-and-semaphores/04-Mutexes

[4] J. Bell. “Operating Systems: Deadlocks”, apartado 7.2.1. University of Illinois Chicago. Fuente académica complementaria para la pregunta 4.

https://www.cs.uic.edu/~jbell/CourseNotes/OperatingSystems/7_Deadlocks.html

[5] Espressif Systems. ESP32 Technical Reference Manual. Versión 5.8. Capítulos 6 y 28. PDF proporcionado por el profesor.