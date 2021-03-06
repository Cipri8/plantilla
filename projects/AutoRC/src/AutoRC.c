/* Copyright 2019, Sebastian Mateos
 * Copyright 2017, Esteban Volentini - Facet UNT, Fi UNER
 * Copyright 2014, 2015 Mariano Cerdeiro
 * Copyright 2014, Pablo Ridolfi
 * Copyright 2014, Juan Cecconi
 * Copyright 2014, Gustavo Muro
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/** @file blinking.c
 **
 ** @brief Ejemplo de un led parpadeando
 **
 ** Ejemplo de un led parpadeando utilizando la capa de abstraccion de 
 ** hardware y sin sistemas operativos.
 ** 
 ** | RV | YYYY.MM.DD | Autor       | Descripción de los cambios              |
 ** |----|------------|-------------|-----------------------------------------|
 ** |  2 | 2017.10.16 | evolentini  | Correción en el formato del archivo     |
 ** |  1 | 2017.09.21 | evolentini  | Version inicial del archivo             |
 ** 
 ** @defgroup ejemplos Proyectos de ejemplo
 ** @brief Proyectos de ejemplo de la Especialización en Sistemas Embebidos
 ** @{ 
 */

/* === Inclusiones de cabeceras ============================================ */
#include "FreeRTOS.h"
#include "task.h"
#include "unt.h"
#include "soc.h"
#include "event_groups.h"
#include "queue.h"
#include "led.h"
#include "switch.h"
#include "gpio.h"
#include "uart.h"
#include "pwm_sct.h"
#include "servo.h"
#include <string.h>

/* === Definicion y Macros ================================================= */
#define EVENTO_CARACTER   (1 << 0)
#define EVENTO_CADENA   (1 << 1)

#define PIN_ADELANTE T_FIL2
#define PIN_ATRAS    T_COL0
#define PIN_BOCINA   LCD2

#define CTOUT1 1
#define CTOUT1_PORT 4
#define CTOUT1_PIN  1
#define CTOUT1_FUNC FUNC1

#define CTOUT3 3
#define CTOUT3_PORT 4
#define CTOUT3_PIN  3
#define CTOUT3_FUNC FUNC1

#define INIT_FREC 50

#define ADELANTE 0xF1
#define ATRAS 0xF2
#define FRENAR 0xF3
#define CABEZAL 0xF4
#define APAGAR_TODO 0xF5
//#define ANGULO_CERO 90
#define ANGULO_CERO 150

#define LUCES_DELANTERAS (1<<7)
#define LUCES_TRASERAS (1<<6)
#define BOCINA (1<<5)
#define A (1<<4)
#define B (1<<3)
#define C (1<<2)
#define D (1<<1)
#define E (1<<0)

/* === Declaraciones de tipos de datos internos ============================ */
/** @brief Estructura de datos para enviar y recibir datos
 **
 ** Estructura que contiene los datos necesarios para que la interrupción
 ** pueda continuar el envio o recepcion de datos
 */
typedef struct
{
   uint8_t comando[4];   /** < Bloque de datos */
   uint8_t posicion;     /** < Posicion del puntero */
} cola_t;

/* === Declaraciones de funciones internas ================================= */
/** @brief Función que decodifica un dato recibido por UART y realiza una accion
 **
 ** @parameter[in] parametros Puntero a una cadena que contiene el led a prender
 */
void Decodificar(void * parametros);

/** @brief Función que decodifica un dato recibido por UART y realiza una accion
 ** 
 ** @parameter[in] parametros Puntero a una cadena que contiene el led a prender
 */
void RecibirComando(void * parametros);

/** @brief Función que implementa la funcion de interrupcion de recepcion de la UART
 */
void IntRecepcion(void);

/** @brief Función que lee las teclas y activa la decodificacion con la tecla 1
 */ 
void Teclado(void * parametros);

/** @brief Función que cambia el estado del motor de traccion
 */
void MotorTraccion(uint8_t sentido, uint8_t velocidad);

/** @brief Función que cambia el estado del motor de direccion
 */
void MotorDireccion(uint8_t angulo);

/** @brief Función que cambia el estado de los accesorios del auto
 */
void EstadoAccesorios(uint8_t estado);

/** @brief Función que apaga ambos motores, luces y demas funciones
 */
void ApagarTodo(void);

/* === Definiciones de variables internas ================================== */
/** @brief Variable para la inicializacion del servo de la direccion */
//	servo_t servo = SERVO1;

/** @brief Variable para la inicializacion control por PWM de la traccion */
//	pwm_out_t pwm = CTOUT3;

/** @brief Información para la recepcion de datos por la uart */
cola_t comando_recibido;

/** @brief Descriptor del grupo de eventos */
EventGroupHandle_t eventos;
QueueHandle_t cola;

uint32_t ticks_per_cycle;

/* === Definiciones de variables externas ================================== */

/* === Definiciones de funciones internas ================================== */

void Decodificar(void * parametros)
{
	uint8_t i;
	EventBits_t bits;
	cola_t * comando_recibido = parametros;
	uint8_t comando_anterior[4] = {FRENAR, 0, ANGULO_CERO, 0};
//	vTaskSuspend(NULL);
	while(1)
	{
//		strcpy(comando_anterior, comando_recibido->comando);
		for(i=0 ; i<4 ; i++)
			comando_anterior[i] = comando_recibido->comando[i];
		vTaskResume(xTaskGetHandle("RecibirComando"));
		bits = xEventGroupWaitBits(eventos, EVENTO_CADENA, TRUE, FALSE, pdMS_TO_TICKS(30000));

		if(bits == EVENTO_CADENA)
		{
			if(comando_recibido->comando[0] != comando_anterior[0] || comando_recibido->comando[1] != comando_anterior[1])
			{
				if(comando_recibido->comando[0] <= FRENAR)
					MotorTraccion(comando_recibido->comando[0], comando_recibido->comando[1]);
				else if(comando_recibido->comando[0] == CABEZAL)
				{

				}
				else
					ApagarTodo();
			}
				if(comando_recibido->comando[2] != comando_anterior[2])
					MotorDireccion(comando_recibido->comando[2]);
				if(comando_recibido->comando[3] != comando_anterior[3])
					EstadoAccesorios(comando_recibido->comando[3]);
//			}
		}
		else
			ApagarTodo();
	}
}

void RecibirComando(void * parametros)
{
	char dato;
	bool completando_comando;
	cola_t * comando_recibido = parametros;
	while(1)
	{
		vTaskSuspend(NULL);
		UARTIntEnable(UART_RS232,  UART_RBRINT);
		comando_recibido->posicion = 255;
		completando_comando = true;
		do
		{
			comando_recibido->posicion++;
			xQueueReceive(cola, &dato, pdMS_TO_TICKS(30000));
			comando_recibido->comando[comando_recibido->posicion] = dato;
			if(comando_recibido->comando[0]<0xf1 || comando_recibido->comando[0]>0xf5)
				comando_recibido->posicion = 255;
			if(comando_recibido->posicion == 3)
				completando_comando = FALSE;
		}
		while(completando_comando);
		UARTIntDisable(UART_RS232, UART_RBRINT);
		xEventGroupSetBits(eventos, EVENTO_CADENA);
	}
}
void IntRecepcion(void)
{
	char dato;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	UARTReadByte(UART_RS232, &dato);
	xQueueSendFromISR(cola, &dato, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
void Teclado(void * parametros)
{
	uint8_t tecla;
	uint8_t anterior = 0;

	while(1)
	{
		tecla = SwitchesRead();
		if (tecla != anterior)
		{
			switch(tecla)
			{
				case SWITCH_1:
//					vTaskResume(xTaskGetHandle("Decodificar"));
					break;
				case SWITCH_2:
					break;
				case SWITCH_3:
					break;
				case SWITCH_4:
					break;
			}
			anterior = tecla;
		}
		vTaskDelay(100/ portTICK_PERIOD_MS);
		LedToggle(LED_RGB_B);
	}
}

void MotorTraccion(uint8_t sentido, uint8_t velocidad)
{
	uint32_t ticks = (ticks_per_cycle*velocidad)/255;
	if(sentido == ADELANTE)
	{
		//	PWMSetDutyCycle(CTOUT3, velocidad);
		Chip_SCTPWM_SetDutyCycle(LPC_SCT, CTOUT3+1, ticks);
		GPIOOn(PIN_ADELANTE);
		GPIOOff(PIN_ATRAS);
		LedsMask(LED_3);
	}
	if(sentido == ATRAS)
	{
		//	PWMSetDutyCycle(CTOUT3, velocidad);
		Chip_SCTPWM_SetDutyCycle(LPC_SCT, CTOUT3+1, ticks);
		GPIOOn(PIN_ATRAS);
		GPIOOff(PIN_ADELANTE);
		LedsMask(LED_1);
	}
	if(sentido == FRENAR)
	{
		//	PWMSetDutyCycle(CTOUT3, 0);
		Chip_SCTPWM_SetDutyCycle(LPC_SCT, CTOUT3+1, 0);
		GPIOOff(PIN_ADELANTE);
		GPIOOff(PIN_ATRAS);
		LedsMask(LED_2);
	}

}

void MotorDireccion(uint8_t angulo)
{
	uint32_t ticks = (ticks_per_cycle*10)/angulo;
	Chip_SCTPWM_SetDutyCycle(LPC_SCT, CTOUT1+1, ticks);
//	ServoAngle(&servo, angulo); //Cambiar en APP
	if(angulo < ANGULO_CERO)
	{
		LedOn(LED_RGB_G);
		LedOff(LED_RGB_R);
		LedOff(LED_RGB_B);
	}
	else if(angulo > ANGULO_CERO)
	{
		LedOn(LED_RGB_R);
		LedOff(LED_RGB_G);
		LedOff(LED_RGB_B);
	}
	else
	{
		LedOn(LED_RGB_B);
		LedOff(LED_RGB_R);
		LedOff(LED_RGB_G);
	}
}

void EstadoAccesorios(uint8_t estado)
{
	GPIOState(PIN_BOCINA, (estado&BOCINA));
}

void ApagarTodo(void)
{
//	ServoAngle(&servo, ANGULO_CERO);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT, CTOUT1+1, (ticks_per_cycle*10)/ANGULO_CERO);
//	PWMSetDutyCycle(CTOUT3, 0);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT, CTOUT3+1, 0);
	GPIOOn(PIN_ATRAS);
	GPIOOn(PIN_ADELANTE);
	LedsOffAll();
}

/* === Definiciones de funciones externas ================================== */

/** @brief Función principal del programa
 **
 ** @returns 0 La función nunca debería termina
 **
 ** @remarks En un sistema embebido la función main() nunca debe terminar.
 **          El valor de retorno 0 es para evitar un error en el compilador.
 */
int main(void)
{
	/* Inicializaciones y configuraciones de dispositivos */
	LedsInit();
	SwitchesInit();
	GPIOInit(PIN_ADELANTE, GPIO_OUTPUT);
	GPIOInit(PIN_ATRAS, GPIO_OUTPUT);
	GPIOInit(PIN_BOCINA, GPIO_OUTPUT);
	UARTInit(UART_RS232);
	UARTActivInt(UART_RS232, IntRecepcion);
//	ServoInit(&servo, 1);
//	ServoAngle(&servo, ANGULO_CERO);
//	PWMInit(&pwm, 1, PWM_FREC);
//	PWMSetDutyCycle(CTOUT3, 0);
	Chip_SCTPWM_Init(LPC_SCT);
	Chip_SCTPWM_SetRate(LPC_SCT, INIT_FREC);
	Chip_SCU_PinMux(CTOUT1_PORT , CTOUT1_PIN , SCU_MODE_INACT , CTOUT1_FUNC);
	Chip_SCU_PinMux(CTOUT3_PORT , CTOUT3_PIN , SCU_MODE_INACT , CTOUT3_FUNC);
	Chip_SCTPWM_SetOutPin(LPC_SCT, CTOUT1+1 , CTOUT1);
	Chip_SCTPWM_SetOutPin(LPC_SCT, CTOUT3+1 , CTOUT3);
	ticks_per_cycle = Chip_SCTPWM_GetTicksPerCycle(LPC_SCT);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT, CTOUT1+1, (ticks_per_cycle/15));
	Chip_SCTPWM_SetDutyCycle(LPC_SCT, CTOUT3+1, 0);
	Chip_SCTPWM_Start(LPC_SCT);

	/* Creación del grupo de eventos */
	eventos = xEventGroupCreate();
	cola = xQueueCreate(4, sizeof(char));

	/* Creación de las tareas */
	xTaskCreate(Decodificar, "Decodificar", configMINIMAL_STACK_SIZE, &comando_recibido, tskIDLE_PRIORITY + 2, NULL);
	xTaskCreate(RecibirComando, "RecibirComando", configMINIMAL_STACK_SIZE, &comando_recibido, tskIDLE_PRIORITY + 3, NULL);
	xTaskCreate(Teclado, "Teclado", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

	/* Arranque del sistema operativo */
	SisTick_Init();
	vTaskStartScheduler();
   
	/* vTaskStartScheduler solo retorna si se detiene el sistema operativo */
	while(1);

	/* El valor de retorno es solo para evitar errores en el compilador*/
	return 0;
}
/* === Ciere de documentacion ============================================== */
/** @} Final de la definición del modulo para doxygen */
