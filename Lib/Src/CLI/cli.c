/*
 * cli.c
 *
 *  Created on: 6 Aug 2021
 *      Author: Christodoulos Sotiriou
 */

/********************************
 * Includes
 ********************************/
#include "CLI/cli.h"

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "FreeRTOS_CLI.h"

#include "main.h"
#include "stm32wlxx_hal.h"

#include <string.h>

/********************************
 * Defines
 ********************************/
#define CLI_BUF_SIZE 64
#define CLI_QUEUE_SIZE 5

/********************************
 * External Variables
 ********************************/
extern UART_HandleTypeDef huart2;

/********************************
 * Function Prototypes
 ********************************/
static BaseType_t commandTestCallback( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);

/********************************
 * Static Variables
 ********************************/

/* Command Definition */
static const CLI_Command_Definition_t commandTest = {
    "test",
    "test: Hopefully prints out \"Hello World\"\r\n",
    commandTestCallback,
    0
};

/* UART Receive */
uint8_t cliByteRecved;
static uint8_t recvBuf[CLI_BUF_SIZE] = {0};
static uint32_t recvBufSize = 0;
static uint8_t responseSent = 0;

/* FreeRTOS Threads */
static osThreadId_t cliThread;
static osThreadAttr_t cliThreadAttr = {
		.name = "cliThread",
		.stack_size = 512,
		.priority = osPriorityAboveNormal
};

/* FreeRTOS Queues */
static osMessageQueueId_t cliQueue;
static osMessageQueueAttr_t cliQueueAttr = {
		.name = "cliQueue"
};

/********************************
 * Static Functions
 ********************************/
static BaseType_t commandTestCallback( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
	memset(pcWriteBuffer, 0, xWriteBufferLen);

	strcpy(pcWriteBuffer, "Hello World\r\n");

	return pdFALSE;
}

static void cliTask (void *argument) {
	static char rxdata[CLI_BUF_SIZE];
	static char txdata[CLI_BUF_SIZE];

	HAL_UART_Receive_IT(&huart2, &cliByteRecved, 1);

	for (;;) {
		if (osMessageQueueGet(cliQueue, rxdata, 0, osWaitForever) == osOK) {
			/* Received Command */
			static uint8_t moreData;

			do {
				moreData = FreeRTOS_CLIProcessCommand(rxdata, txdata, CLI_BUF_SIZE);

				responseSent = 0;
				HAL_UART_Transmit_IT(&huart2, (uint8_t *) txdata, strlen(txdata));

				while (!responseSent);
			} while (moreData != pdFALSE);

		}

		osDelay(10);
	}
}

/********************************
 * UART Callback
 ********************************/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);

	if (huart != &huart2) {
		return;
	}

	/* Add Byte to long buffer */
	recvBuf[recvBufSize] = cliByteRecved;
	recvBufSize++;

	/* Check if we got \r\n */
	if (recvBufSize != 0 && recvBuf[recvBufSize - 2] == '\r' && recvBuf[recvBufSize - 1] == '\n') {
		/* Add Message to Queue */
		recvBuf[recvBufSize - 1] = 0;
		recvBuf[recvBufSize - 2] = 0;
		osMessageQueuePut(cliQueue, &recvBuf, 0, 0);
		memset(recvBuf, 0, CLI_BUF_SIZE);
		recvBufSize = 0;
	}

	/* Buffer run out */
	if (recvBufSize == CLI_BUF_SIZE) {
		memset(recvBuf, 0, CLI_BUF_SIZE);
		recvBufSize = 0;
	}

	HAL_UART_Receive_IT(&huart2, &cliByteRecved, 1);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	responseSent = 1;
}


/********************************
 * Interface Functions
 ********************************/
void commandLineInit(void) {
	/* Register Commands */
	FreeRTOS_CLIRegisterCommand(&commandTest);

	/* Create Threads */
	cliThread = osThreadNew(cliTask, NULL, &cliThreadAttr);
	if (cliThread == NULL) {
		return;
	}

	/* Create Queues */
	cliQueue = osMessageQueueNew(CLI_QUEUE_SIZE, CLI_BUF_SIZE, &cliQueueAttr);
	if (cliQueue == NULL) {
		osThreadTerminate(cliThread);
		return;
	}
}
