#include <stdint.h>
#include "stm32f4xx.h"


/* =========================================================
   MATRIZ LED 8x8
   ========================================================= */

//VARIABLE DE LA MATRIZ QUE CONTIENE LA IMAGEN

uint8_t imagen_ingresando[8] =
{
		0b00111100,
		0b01000010,
		0b10000001,
		0b10011001,
		0b10011001,
		0b10000001,
		0b01000010,
		0b00111100
};

uint8_t imagen_principal[8] =
{
		0b11111111,
		0b11000011,
		0b10111101,
		0b10100101,
		0b10100101,
		0b10111101,
		0b11000011,
		0b11111111
};

uint8_t imagen_correcta[8] =
{
		0b00000000,
		0b01000010,
		0b11100111,
		0b01000010,
		0b00000000,
		0b10000001,
		0b01000010,
		0b00111100
};

uint8_t imagen_incorrecta[8]=
{
		0b00000000,
		0b01000010,
		0b11100111,
		0b01000010,
		0b00000000,
		0b00111100,
		0b01000010,
		0b10000001
};
//puntero que indica cual de ls imagenes se esta mostrando,
//solo cambiamos el puntero a la hora de solicitar una imagen
uint8_t * imagen_actual = imagen_correcta;	//la imagen que actualmente se muestra

uint8_t fila_actual = 0;		//Indica fila que estamos mostrando actualmente

void LED_Init(void)
{
    // Habilitar reloj de GPIOD
    RCC->AHB1ENR |= (1 << 3);

    // PD0-PD15 como salidas 01
    GPIOD->MODER &= ~(0xFFFFFFFF);		//limpiala configuracion de todos los pines
    GPIOD->MODER |=  0x55555555;		//colocamos en 01 en cada grupo de 2 bits

    // Push-Pull
    GPIOD->OTYPER &= ~(0xFFFF);		//ponemos en 0

    // Sin Pull-Up / Pull-Down
    GPIOD->PUPDR &= ~(0xFFFFFFFF);		//ponemos en 00 sin resistencias internas

    // Todo apagado inicialmente
    GPIOD->ODR = 0x0000;
}


void Led_MostrarFila(void)		//mostrar una fila
{
	uint16_t filas;
	uint16_t columnas;

	GPIOD->ODR = 0x0000;	//Apagar toda la matriz antes de cambiar de fila y asi evitamos leds encendidos en los cambios de fila

	filas = 0x00FF;		//Todas las filas en HIGH, desactivas = 1

	filas &= ~(1 << fila_actual);	//la fila actual se pone en LOW y AND las otras en 1

	columnas = ((uint16_t)imagen_actual[fila_actual]) <<8;	//tomar los bits de la imagen actual y colocarlos en las columnas están en PD8-PD15

	GPIOD->ODR = filas | columnas;	//combinamos los bits altos de las columnas y bajos de las filas con OR
}

//Multiplexacion de la matriz

void Led_Multiplexacion(void)
{
	fila_actual++;		//pasar a la siguiente fila

	if (fila_actual >= 8)	//si llegamos a la ultimo fila, volver a la primera
	{
		fila_actual = 0;
	}

	Led_MostrarFila();		//mostramos solo una fila
}

/* =========================================================
   TECLADO 4x4
   ========================================================= */

// PA0-PA3 → Filas
// PA4-PA7 → Columnas

void Keypad_Init(void)
{
    // Habilitar reloj de GPIOA
    RCC->AHB1ENR |= (1 << 0);

    // PA0-PA3 como salidas
    GPIOA->MODER &= ~(0x000000FF);		//corresponden a las 4 filas del teclado
    GPIOA->MODER |=  0x00000055;

    // PA4-PA7 como entradas
    GPIOA->MODER &= ~(0x0000FF00);		//corresponden a las 4 columnas del teclado

    // Pull-Up en PA4-PA7
    GPIOA->PUPDR &= ~(0x0000FF00);		//limpiamos la configuracion
    GPIOA->PUPDR |=  0x00005500;		//activamos el pull-up 1, cuando la fila esta en cero la columna puede llevarse a cero

    // Todas las filas inicialmente HIGH
    GPIOA->BSRR = 0x000F;		//iniciamos las 4 filas en 1
}


/* =========================================================
   SYSTICK
   ========================================================= */

volatile uint32_t tick = 0;		//controlador del tiempo en el sistema


void SysTick_Handler(void)
{
    tick++;		//cada interrupcion aumenta el contador cada 1ms
}

void SysTick_Init(void)
{
    SysTick->LOAD = 16000 - 1;	//16Mhz -> 1ms (valor de recarga)
    SysTick->VAL = 0;			//reiniciar contador
    SysTick->CTRL = 7;			//enable + interrupcion + reloj de procesador
}


/* =========================================================
   LECTURA DEL TECLADO
   ========================================================= */
char tecla_fisica = '\0';		//permite saber si en ese instnte hay una tecla presionada

char Keypad_Read(void)
{
    const char keys[4][4] =		//mapa fisico de las teclas
    {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}
    };

    for (int row = 0; row < 4; row++)		//explorar las 4 filas 1x1
    {
        // Todas las filas en HIGH = 1
        GPIOA->BSRR = 0x000F;

        // Fila actual en LOW = 0, los bits 16-31 son para reset
        GPIOA->BSRR = (1 << (row + 16));

        // Leer columnas PA4-PA7, desplaza los bits 0-3 a 4-7 y elimina el resto de bits
        uint16_t cols = (GPIOA->IDR >> 4) & 0x0F;

        // Con Pull-Up, tecla presionada = 0

        if (!(cols & 0x01))		//! para detectar un cero
            return keys[row][0];

        if (!(cols & 0x02))
            return keys[row][1];

        if (!(cols & 0x04))
            return keys[row][2];

        if (!(cols & 0x08))
            return keys[row][3];
    }

    return '\0';		//caracter nulo, no hay tecla, devolvemos
}


/* =========================================================
   MÁQUINA DE ESTADOS - ANTIRREBOTE
   ========================================================= */

typedef enum
{
    LIBRE,
    DEBOUNCE,
    PRESIONADA
} Keypad_State;

char Keypad_Scan(void)
{
    static Keypad_State estado = LIBRE;		//static nos permite conservar los valores
    static char tecla_detectada = '\0';
    static uint32_t tiempo_debounce = 0;

    char tecla = Keypad_Read();		//leemos el estado actual de la tecla
    tecla_fisica = tecla;
    switch (estado)
    {
        case LIBRE:

            // No había ninguna tecla.
            // Si aparece una, comienza el antirrebote.
            if (tecla != '\0')
            {
                tecla_detectada = tecla;
                tiempo_debounce = tick;		//guardamos el momento en el que se presiono
                estado = DEBOUNCE;
            }

            break;


        case DEBOUNCE:

            // Si durante el antirrebote la tecla desaparece,
            // se considera un rebote y volvemos a LIBRE.
            if (tecla == '\0')
            {
                estado = LIBRE;
            }

            // Después de 20 ms comprobamos nuevamente.
            else if ((tick - tiempo_debounce) >= 20)
            {
                // La tecla sigue presionada.
                // Se genera el evento UNA sola vez.
                if (tecla == tecla_detectada)
                {
                    estado = PRESIONADA;		//la pulsacion es avlida
                    return tecla_detectada;		//generamos la tecla una sola vez
                }

                // Si cambió la tecla, reiniciamos el proceso.
                else
                {
                    tecla_detectada = tecla;
                    tiempo_debounce = tick;
                }
            }

            break;


        case PRESIONADA:

            // Mientras la tecla siga presionada,
            // NO se genera ningún nuevo evento.
            if (tecla == '\0')
            {
                // La tecla fue liberada.
                estado = LIBRE;
                tecla_detectada = '\0';
            }

            break;
    }

    return '\0';		//si no hubo evento, devolvemos caracter nulo
}

//EVENTOS DEL TECLADO para evitar que la FSM cntraseña conozca como funciona internamente el teclado
typedef enum
{
	Key_ninguna,
	Key_presionada
}Keyevent;

Keyevent Key_event = Key_ninguna;		//evento generado por el teclado una sola vez por el antirrebote, para ingresar cada digito
char Key_evaluada = '\0';		//valor de la tecla que genero el evento


//FSM DEL TECLADO

void Teclado_FSM (void)
{
	char tecla;
	tecla = Keypad_Scan();		//obtener una tecla ya detectada por debounce
	Key_event = Key_ninguna;	//por defecrto no exite evento

	if (tecla != '\0')		//generamos el evento y guardamos la tecla
	{
		Key_evaluada = tecla;
		Key_event = Key_presionada;
	}

}

// CONTRASEÑA

const char contraseña_correcta[4] =		//contraseña 1234
{
		'1',
		'2',
		'3',
		'4'
};

char contraseña_ingresada[4];		//buffer donde guardamos la contraseña introducida
uint8_t Contraseña_index = 0;		//cantidad de digitos ingresados

//EVENTOS DE CONTRASEÑA
typedef enum
{
	ninguna_contraseña,
	contraseña_ok,
	contraseña_error

}contraseñaevent;

contraseñaevent contraseña_event = ninguna_contraseña;		//evento generado despues de comparar la contraseña


typedef enum
{
	esperando_contraseña,
	contraseña_1,
	contraseña_2,
	contraseña_3,
}estado_contraseña;



void contraseña_FSM(void)
{
	static estado_contraseña estado = esperando_contraseña;

	switch (estado)
	{
	//ESPERA
	case esperando_contraseña:

		if (Key_event == Key_presionada)		//esperamos el primer digito
		{
			contraseña_ingresada[0] = Key_evaluada;
			Contraseña_index = 1;

			estado = contraseña_1;
		}
	break;


case contraseña_1:

	if (Key_event == Key_presionada)		//recibimos el segundo digito
	{
		contraseña_ingresada[1] = Key_evaluada;
		Contraseña_index = 2;

		estado = contraseña_2;
	}
	break;


case contraseña_2:

	if (Key_event == Key_presionada)		//recibimos el tercer diigito
	{
		contraseña_ingresada[2] = Key_evaluada;
		Contraseña_index = 3;

		estado = contraseña_3;
	}
	break;


case contraseña_3:

	if (Key_event == Key_presionada)		//recibimos el cuarto digito
	{
		contraseña_ingresada[3] = Key_evaluada;
		Contraseña_index = 4;

		uint8_t correcta = 1;		//correcta 1 incorrecta 0
			for (int i = 0; i < 4; i++)		//comparamos los 4 digitos almacenados con los ingresados
			{
				if (contraseña_ingresada[i] != contraseña_correcta[i])
				{
					correcta= 0;
				}
			}

			//GENERAMOS EVENTO PARA LA FSM DEL SISTEMA

			if (correcta)
			{
				contraseña_event = contraseña_ok;
			}
			else
			{
				contraseña_event = contraseña_error;

			}

			//PREPARAMOS LA FSM PARA UNA NUEVA CONTRASEÑA

			Contraseña_index = 0;		//reiniciamos contador para una nueva
			estado = esperando_contraseña;
			}
				break;
		}
	}

//FSM DEL SISTEMA

typedef enum
{
	espera,
	ingresando,
	validando,
	acceso,
	error

}estadosistema;

estadosistema estado_sistema = espera;		//estado actual del sistema
uint32_t tiempo_resultado = 0;		//guarda el momento donde aparecen las imagenes

//FSM DEL SISTEMA

void FSM_sistema (void)
{
	switch (estado_sistema)
	{

	case espera:
		imagen_actual = imagen_principal;		//mostrar figura principal

		if(Key_event == Key_presionada)		//si se presiono una tecla, comienza el ingreso
		{
			imagen_actual = imagen_ingresando;
			estado_sistema = ingresando;
		}
		break;

	case ingresando:					//esperamos FSM contraseña termine de comparar
		if (tecla_fisica != '\0')
		{
			imagen_actual = imagen_ingresando;
		}
		else
		{
			imagen_actual = imagen_principal;
		}

		if (contraseña_event == contraseña_ok||		//esperamos el ok - error
			contraseña_event == contraseña_error)
		{
			estado_sistema = validando;		//ya se ingreso, validamos
		}
		break;

	case validando:
		if (contraseña_event == contraseña_ok)		//FSM de contraseña nos dio el resultado por medio de un evento
		{
			imagen_actual = imagen_correcta;		//contraseña correcta - imagen
			tiempo_resultado = tick;			//guardamos instante de la imagen
			estado_sistema = acceso;			//pasamos a estado acceso
			contraseña_event = ninguna_contraseña;		//consumimos el evento
		}
		else if (contraseña_event == contraseña_error)		//contraseña incorrecta
		{
			imagen_actual = imagen_incorrecta;		//contraseña incorrecta - imagen
			tiempo_resultado = tick;			//guardamos el momento de la imagen
			estado_sistema = error;				//pasamos a estado error
			contraseña_event = ninguna_contraseña;		//consumimos el evento
		}
		break;

	case acceso:
		if ((tick - tiempo_resultado) >= 3000)		//mantenemos imagen durante 3 seg
		{
			imagen_actual = imagen_principal;		//volvemos a la imagen principal
			estado_sistema = espera;

		}
		break;

	case error:
		if ((tick - tiempo_resultado) >= 3000)		//mantenemos imagen durante 3 seg
		{
			imagen_actual = imagen_principal;		//volvemos a la imagen principal
			estado_sistema = espera;

		}
		break;
	}
}

//MAQUINA MAESTRA

void FSM_maestra (void)
{
	Teclado_FSM();		//leer teclado y generar evento
	contraseña_FSM();	//procesar evento de la FSM contraseña
	FSM_sistema();		//coordinar el estado general del sistema
}
/* =========================================================
   MAIN
   ========================================================= */

int main(void)
//iniciar perisferiscos

{
    LED_Init();		//inicia matriz
    Keypad_Init();		//inicia teclado
    SysTick_Init();	//inicia systick

	imagen_actual  = imagen_principal;		//solo mostramos imagen principal
	Led_MostrarFila();

	//bucle cooperativo, para ejecutar continuamente el sistema
    while (1)
    {
    	//FSM_sistema();
    	//Teclado_FSM();
    	//contraseña_FSM();
    	FSM_maestra();		//ejecutamos las diferentes FSM
    	Led_Multiplexacion();		//actualizamos la siguiente fila de la matriz
    }								//se ejecuta continuamente para la multiplexacion rapida y evitar parpadeos visisbles
}
