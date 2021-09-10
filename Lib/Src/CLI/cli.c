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

#include "subghz_phy_app.h"

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "FreeRTOS_CLI.h"

#include "main.h"
#include "stm32wlxx_hal.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/********************************
 * Defines
 ********************************/
#define CLI_BUF_SIZE 128
#define CLI_QUEUE_SIZE 5

/********************************
 * External Variables
 ********************************/
extern UART_HandleTypeDef huart2;

/********************************
 * Function Prototypes
 ********************************/
static BaseType_t commandClearCallback(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t commandFreqCallback(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t commandFreqDeviationCallback(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t commandPowerCallback(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t commandDatarateCallback(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t commandPreambleCallback(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t commandCRCCallback(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t commandSyncwordCallback(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t commandTransmitCallback(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t commandTransmitContinuousCallback(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);

/********************************
 * Static Variables
 ********************************/

/* Command Definition */
static const CLI_Command_Definition_t commandClear = {
    "clear",
    "clear: Clears the terminal\r\n",
    commandClearCallback,
    0
};

static const CLI_Command_Definition_t commandFreq = {
    "freq",
    "freq [Hz]: Get/Set the current transmitting frequency\r\n",
    commandFreqCallback,
    -1
};

static const CLI_Command_Definition_t commandFreqDeviation = {
    "freqDeviation",
    "freqDeviation [Hz]: Get/Set the frequency deviation\r\n",
    commandFreqDeviationCallback,
    -1
};


static const CLI_Command_Definition_t commandPower = {
    "power",
    "power [dBm]: Get/Set the current transmitting power\r\n",
    commandPowerCallback,
    -1
};

static const CLI_Command_Definition_t commandDatarate = {
    "datarate",
    "datarate [bps]: Get/Set the current transmitting datarate\r\n",
    commandDatarateCallback,
    -1
};

static const CLI_Command_Definition_t commandPreamble = {
    "preamble",
    "preamble [byte_count]: Get/Set the preamble length\r\n",
    commandPreambleCallback,
    -1
};

static const CLI_Command_Definition_t commandCRC = {
    "crc",
    "crc [on|off]: Get/Set the if a CRC is transmitted\r\n",
    commandCRCCallback,
    -1
};

static const CLI_Command_Definition_t commandSyncword = {
    "syncword",
    "syncword [length] [word]: Set a Syncword for trnsmission before the message\r\n",
    commandSyncwordCallback,
    -1
};

static const CLI_Command_Definition_t commandTransmit = {
    "transmit",
    "transmit [msg]: Transmits a message using the SUBGHZ peripheral\r\n",
    commandTransmitCallback,
    -1
};

static const CLI_Command_Definition_t commandTransmitContinuous = {
    "transmitContinuous",
    "transmitContinuous [ms] [msg]: Transmits a message using the SUBGHZ peripheral every ms. Pass 0ms to stop\r\n",
    commandTransmitContinuousCallback,
    -1
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
		.stack_size = 1024,
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
static BaseType_t commandClearCallback(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
	memset(pcWriteBuffer, 0, xWriteBufferLen);

	/* Move Cursor to the start and clear the screen */
	strcpy(pcWriteBuffer, "\x1b[H\x1b[2J");

	return pdFALSE;
}

static BaseType_t commandFreqCallback(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
	memset(pcWriteBuffer, 0, xWriteBufferLen);

	BaseType_t paramLen;
	const char *param = FreeRTOS_CLIGetParameter(pcCommandString, 1, &paramLen);

	if (param == NULL) { /* No arguments */
		snprintf(pcWriteBuffer, xWriteBufferLen, "Frequency = %.3f MHz\r\n", SubghzApp_GetFreq() / 1.0e6);
	} else {
		uint32_t newFreq = atoll(param);

		if (newFreq >= 1e6 && newFreq < 1e9) {
			SubghzApp_SetFreq(newFreq);
			strcpy(pcWriteBuffer, "Frequency Set Successfully\r\n");
		} else {
			strcpy(pcWriteBuffer, "Invalid Frequency\r\n");
		}
	}


	return pdFALSE;
}

static BaseType_t commandFreqDeviationCallback(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
	memset(pcWriteBuffer, 0, xWriteBufferLen);

	BaseType_t paramLen;
	const char *param = FreeRTOS_CLIGetParameter(pcCommandString, 1, &paramLen);

	if (param == NULL) { /* No arguments */
		snprintf(pcWriteBuffer, xWriteBufferLen, "Frequency Deviation = %.3f kHz\r\n", SubghzApp_GetFreqDeviation() / 1.0e3);
	} else {
		uint32_t newFreqDeviation = atoll(param);

		if (newFreqDeviation >= 100 && newFreqDeviation <= 100e3) {
			SubghzApp_SetFreqDeviation(newFreqDeviation);
			strcpy(pcWriteBuffer, "Frequency Deviation Set Successfully\r\n");
		} else {
			strcpy(pcWriteBuffer, "Invalid Frequency Deviation\r\n");
		}
	}


	return pdFALSE;
}

static BaseType_t commandPowerCallback(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
	memset(pcWriteBuffer, 0, xWriteBufferLen);

	BaseType_t paramLen;
	const char *param = FreeRTOS_CLIGetParameter(pcCommandString, 1, &paramLen);

	if (param == NULL) { /* No arguments */
		snprintf(pcWriteBuffer, xWriteBufferLen, "Power = %lu dBm\r\n", SubghzApp_GetPower());
	} else {
		uint32_t newPower = atoll(param);

		if (newPower > 0 && newPower <= 22) {
			SubghzApp_SetPower(newPower);
			strcpy(pcWriteBuffer, "Power Set Successfully\r\n");
		} else {
			strcpy(pcWriteBuffer, "Invalid Power\r\n");
		}
	}


	return pdFALSE;
}

static BaseType_t commandDatarateCallback(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
	memset(pcWriteBuffer, 0, xWriteBufferLen);

	BaseType_t paramLen;
	const char *param = FreeRTOS_CLIGetParameter(pcCommandString, 1, &paramLen);

	if (param == NULL) { /* No arguments */
		snprintf(pcWriteBuffer, xWriteBufferLen, "Datarate = %lu bps\r\n", SubghzApp_GetDatarate());
	} else {
		uint32_t newDatarate = atoll(param);

		if (newDatarate > 0 && newDatarate <= 500e3) {
			SubghzApp_SetDatarate(newDatarate);
			strcpy(pcWriteBuffer, "Datarate Set Successfully\r\n");
		} else {
			strcpy(pcWriteBuffer, "Invalid Datarate\r\n");
		}
	}

	return pdFALSE;
}

static BaseType_t commandPreambleCallback(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
	memset(pcWriteBuffer, 0, xWriteBufferLen);

	BaseType_t paramLen;
	const char *param = FreeRTOS_CLIGetParameter(pcCommandString, 1, &paramLen);

	if (param == NULL) { /* No arguments */
		uint32_t len = SubghzApp_GetPreambleLength();
		snprintf(pcWriteBuffer, xWriteBufferLen, "Preamble Length = %lu byte%s\r\n", len, (len == 0 || len > 1) ? "s" : "");
	} else {
		uint32_t newPreamble = atoll(param);

		if (newPreamble > 0 && newPreamble <= 30) {
			SubghzApp_SetPreambleLength(newPreamble);
			strcpy(pcWriteBuffer, "Preamble Length Set Successfully\r\n");
		} else {
			strcpy(pcWriteBuffer, "Invalid Preamble Length\r\n");
		}
	}

	return pdFALSE;
}

static BaseType_t commandCRCCallback(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
	memset(pcWriteBuffer, 0, xWriteBufferLen);

	BaseType_t paramLen;
	const char *param = FreeRTOS_CLIGetParameter(pcCommandString, 1, &paramLen);

	if (param == NULL) { /* No arguments */
		snprintf(pcWriteBuffer, xWriteBufferLen, "CRC is %s\r\n", SubghzApp_GetCRC() ? "On" : "Off");
	} else {
		if (!strncmp(param, "on", paramLen)) {
			strcpy(pcWriteBuffer, "Turned CRC On\r\n");
			SubghzApp_SetCRC(1);
		} else if (!strncmp(param, "off", paramLen)) {
			strcpy(pcWriteBuffer, "Turned CRC Off\r\n");
			SubghzApp_SetCRC(0);
		} else {
			strcpy(pcWriteBuffer, "Invalid Argument\r\n");
		}
	}


	return pdFALSE;
}

static BaseType_t commandSyncwordCallback(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
	memset(pcWriteBuffer, 0, xWriteBufferLen);

	BaseType_t paramLen;
	const char *param = FreeRTOS_CLIGetParameter(pcCommandString, 1, &paramLen);

	if (param == NULL) {
		char word[8];
		uint32_t len = SubghzApp_GetSyncword(word);

		word[len] = '\x00';
		snprintf(pcWriteBuffer, xWriteBufferLen, "Syncword of size %ld: %s\r\n", len, word);

		return pdFALSE;
	}

	uint32_t len = atoll(param);

	if (len >= 0 && len <= 8) {
		param = FreeRTOS_CLIGetParameter(pcCommandString, 2, &paramLen);
		uint32_t param_len = param == NULL ? 0 : strlen(param);

		SubghzApp_SetSyncword(param_len < len ? param_len : len, param);

		strcpy(pcWriteBuffer, "Syncword Set Successfully\r\n");
	} else {
		strcpy(pcWriteBuffer, "Invalid Argument\r\n");
	}

	return pdFALSE;
}

static BaseType_t commandTransmitCallback(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
	memset(pcWriteBuffer, 0, xWriteBufferLen);

	BaseType_t paramLen;
	const char *param = FreeRTOS_CLIGetParameter(pcCommandString, 1, &paramLen);

	if (param != NULL && strlen(param) < 64) {
		SubghzApp_Sent((char *) param, strlen(param));
		strcpy(pcWriteBuffer, "Successful Transmission\r\n");
	} else {
		strcpy(pcWriteBuffer, "Invalid Arguments\r\n");
	}

	return pdFALSE;
}

static BaseType_t commandTransmitContinuousCallback(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
	memset(pcWriteBuffer, 0, xWriteBufferLen);

	BaseType_t paramLen;
	const char *param = FreeRTOS_CLIGetParameter(pcCommandString, 1, &paramLen);

	if (param == NULL) {
		strcpy(pcWriteBuffer, "Invalid Arguments\r\n");
		return pdFALSE;
	}

	uint32_t ms = atoll(param);

	if (ms == 0) {
		SubghzApp_StopContinuous();
		strcpy(pcWriteBuffer, "Continuous Mode Stopped\r\n");
		return pdFALSE;
	}

	param = FreeRTOS_CLIGetParameter(pcCommandString, 2, &paramLen);

	if (param != NULL && strlen(param) < 64) {
		SubghzApp_StartContinuous((char *) param, strlen(param), ms);
		strcpy(pcWriteBuffer, "Continuous Transmission Enabled\r\n");
	} else {
		strcpy(pcWriteBuffer, "Continuous Mode Stopped\r\n");
	}

	return pdFALSE;
}



static void cliTask (void *argument) {
	static char rxdata[CLI_BUF_SIZE];
	static char txdata[CLI_BUF_SIZE];

	HAL_UART_Transmit(&huart2, (uint8_t *) "> ", 2, 10);
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

			HAL_UART_Transmit(&huart2, (uint8_t *) "> ", 2, 10);
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
	if (cliByteRecved == '\b' && recvBufSize != 0) {
		recvBufSize--;
		recvBuf[recvBufSize] = 0;
		HAL_UART_Transmit(&huart2, (uint8_t *) "\b", 1, 10);
	} else {
		HAL_UART_Transmit(&huart2, &cliByteRecved, 1, 10);
		recvBuf[recvBufSize] = cliByteRecved;
		recvBufSize++;
	}

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
	FreeRTOS_CLIRegisterCommand(&commandClear);
	FreeRTOS_CLIRegisterCommand(&commandFreq);
	FreeRTOS_CLIRegisterCommand(&commandFreqDeviation);
	FreeRTOS_CLIRegisterCommand(&commandPower);
	FreeRTOS_CLIRegisterCommand(&commandDatarate);
	FreeRTOS_CLIRegisterCommand(&commandPreamble);
	FreeRTOS_CLIRegisterCommand(&commandCRC);
	FreeRTOS_CLIRegisterCommand(&commandSyncword);
	FreeRTOS_CLIRegisterCommand(&commandTransmit);
	FreeRTOS_CLIRegisterCommand(&commandTransmitContinuous);

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
