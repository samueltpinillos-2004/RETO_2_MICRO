# Reto 2 - Sistema de Control de Acceso

## Descripción

Este proyecto corresponde al Reto 2 de Microcontroladores.

El sistema implementa un control de acceso mediante un teclado matricial 4x4 y una matriz LED 8x8, utilizando un microcontrolador STM32F407VET6.

El sistema permite ingresar una contraseña de 4 dígitos mediante el teclado. La contraseña ingresada es comparada con una contraseña almacenada en memoria y, dependiendo del resultado, se muestra en la matriz LED una indicación de acceso correcto o incorrecto.

El proyecto fue desarrollado utilizando programación Bare-Metal en lenguaje C, mediante acceso directo a los registros del microcontrolador.

## Hardware utilizado

- Microcontrolador STM32F407VET6
- Teclado matricial 4x4
- Matriz LED 8x8 1088AS
- Cables de conexión
- Resistencias de 220 ohm 

## Funcionalidad del sistema

El funcionamiento general del sistema es el siguiente:

1. El sistema inicia en el estado de espera.
2. La matriz LED muestra la imagen principal.
3. Cuando se presiona una tecla, el sistema detecta la entrada.
4. Se aplica un proceso de antirrebote mediante una máquina de estados.
5. Las teclas válidas se almacenan para formar una contraseña de 4 teclas.
6. Mientras se está ingresando la contraseña, la matriz LED muestra la imagen correspondiente al ingreso.
7. Al ingresar los cuatro caracteres, el sistema valida la contraseña.
8. Si la contraseña es correcta, se muestra la imagen de acceso correcto.
9. Si la contraseña es incorrecta, se muestra la imagen de error.
10. Después de aproximadamente 3 segundos, el sistema vuelve al estado de espera.

## Arquitectura del software

El programa está organizado mediante varias máquinas de estados que trabajan de manera cooperativa.

Las principales partes del sistema son:

- FSM del teclado.
- FSM de antirrebote.
- FSM de contraseña.
- FSM del sistema.
- FSM Master.
- Multiplexación de la matriz LED.
- SysTick como base de tiempo.

## Máquina de estados principal

La máquina de estados principal utiliza los siguientes estados:

- ESPERA
- INGRESANDO
- VALIDANDO
- ACCESO
- ERROR

El flujo general del sistema es:

ESPERA →- INGRESANDO →- VALIDANDO →- ACCESO

o

ESPERA -→ INGRESANDO →- VALIDANDO →- ERROR

Después de aproximadamente 3 segundos, los estados ACCESO y ERROR regresan a ESPERA.

## Teclado matricial 4x4

El teclado se conecta al puerto GPIOA del STM32F407VET6.

| Señal | GPIO |
|---|---|
| Fila 1 | PA0 |
| Fila 2 | PA1 |
| Fila 3 | PA2 |
| Fila 4 | PA3 |
| Columna 1 | PA4 |
| Columna 2 | PA5 |
| Columna 3 | PA6 |
| Columna 4 | PA7 |

Las filas se utilizan como salidas y las columnas como entradas con resistencia de pull-up.

El mapa del teclado utilizado es:

| 1 | 2 | 3 | A |
| 4 | 5 | 6 | B |
| 7 | 8 | 9 | C |
| * | 0 | # | D |

## Matriz LED 8x8

La matriz LED utilizada es una 1088AS.

La conexión utilizada es:

| Señal | GPIO |
|---|---|
| Fila 1 | PD0 |
| Fila 2 | PD1 |
| Fila 3 | PD2 |
| Fila 4 | PD3 |
| Fila 5 | PD4 |
| Fila 6 | PD5 |
| Fila 7 | PD6 |
| Fila 8 | PD7 |
| Columna 1 | PD8 |
| Columna 2 | PD9 |
| Columna 3 | PD10 |
| Columna 4 | PD11 |
| Columna 5 | PD12 |
| Columna 6 | PD13 |
| Columna 7 | PD14 |
| Columna 8 | PD15 |

Para la matriz utilizada:

- La fila seleccionada se coloca en 0.
- Las demás filas se mantienen en 1.
- La columna que debe encenderse se coloca en 1.
- Las demás columnas se mantienen en 0.

La matriz utiliza multiplexación de filas para mostrar las diferentes imágenesa una velocidad que no es posible visualizar y se ve como estatica

## Antirrebote del teclado

El teclado utiliza una máquina de estados para evitar múltiples detecciones de una misma pulsación.

Los estados utilizados son:

LIBRE →> DEBOUNCE →> PRESIONADA →> LIBRE

De esta manera, una tecla mantenida presionada no genera múltiples eventos de pulsación.

## Contraseña

La contraseña utilizada por el sistema tiene cuatro caracteres. (1, 2, 3, 4)

Los caracteres ingresados se almacenan temporalmente y, cuando se reciben los cuatro caracteres, se comparan con la contraseña almacenada.

El resultado genera uno de los siguientes eventos:

- CONTRASEÑA_OK
- CONTRASEÑA_ERROR

## Comunicación entre máquinas de estados

Las máquinas de estados se comunican por eventos y variables compartidas.

Cuando el teclado detecta una pulsación válida se genera el evento TECLA_PRESIONADA junto con el valor de la tecla detectada.

La FSM de contraseña utiliza este evento para almacenar el carácter correspondiente.

Después de ingresar los cuatro caracteres, genera CONTRASEÑA_OK o CONTRASEÑA_ERROR.

La FSM del sistema utiliza estos eventos para determinar el resultado del proceso.

## Base de tiempo

El proyecto utiliza el SysTick del STM32 como base de tiempo.

El contador utilizado es: volatile uint32_t tick = 0;

Este contador se incrementa mediante la interrupción del SysTick.

La base de tiempo permite implementar:

- Antirrebote del teclado.
- Tiempo de visualización del resultado.
- Funcionamiento sin retardos bloqueantes.

El programa no utiliza funciones de delay para evitar errores y por requisito del profesor

## Programación Bare-Metal

El proyecto fue desarrollado utilizando programación Bare-Metal.

Se accede directamente a los registros del microcontrolador, por ejemplo:

GPIOA->MODER

GPIOA->IDR

GPIOA->ODR

GPIOA->BSRR

GPIOD->MODER

GPIOD->ODR

SysTick->LOAD

SysTick->CTRL

## Estructura del proyecto

La estructura del proyecto es:

Reto-2-Microcontroladores/

├── README.md

├── src/
│   └── Reto2_SamuelTangarife.c

└── docs/
    └── FSM_Conexiónes.pdf

README.md: explicacion del proyecto

src/main.c: código fuente principal del reto

docs/FSM_Conexiónes.pdf: documento que contiene los diagramas, máquinas de estados y tablas desarrolladas para el proyecto.

## Documentación de diagramas

La documentación gráfica completa del proyecto se encuentra en el archivo:

docs/FSM_Conexiónes.pdf

## Tecnologías utilizadas

- Lenguaje C
- STM32F407VET6
- ARM Cortex-M4
- Programación Bare-Metal
- GPIO
- SysTick
- Máquinas de estados finitos (FSM)
- Multiplexación de matriz LED

## Autor
Samuel Tangarife Pinillos

**Reto 2 - Sistema de Control de Acceso**