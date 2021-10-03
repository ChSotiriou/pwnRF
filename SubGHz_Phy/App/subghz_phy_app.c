/*!
 * \file      subghz_phy_app.c
 *
 * \brief     Ping-Pong implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */
/**
  ******************************************************************************
  *
  *          Portions COPYRIGHT 2020 STMicroelectronics
  *
  * @file    subghz_phy_app.c
  * @author  MCD Application Team
  * @brief   Application of the SubGHz_Phy Middleware
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "platform.h"
#include "stm32_timer.h"
#include "sys_app.h"
#include "subghz_phy_app.h"
#include "radio.h"
#include "cmsis_os.h"
#include "utilities_def.h"
#include "app_version.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* External variables ---------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MAX_TX_BUF 64
#define SYNCWORD_MAX_LEN 8
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* Radio events function pointer */
static RadioEvents_t RadioEvents;

/* USER CODE BEGIN PV */
static RadioModems_t radioModem = MODEM_FSK;


/* Tx Config */
static TxConfigGeneric_t txConfig;

static uint8_t TXsyncWord[SYNCWORD_MAX_LEN];
static uint32_t TXfreq = 433e6;
static uint8_t TXpower = 15;
static uint32_t TXtimeout;


static osTimerId_t subghzTimer;
static osTimerAttr_t subghzTimerAttr = {
		.name = "SUBGHZ Timer"
};

static char continuousMsg[MAX_TX_BUF];
static uint32_t continuousSize;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/*!
 * @brief Function to be executed on Radio Tx Done event
 * @param  none
 * @retval none
 */
static void OnTxDone(void);

/*!
 * @brief Function to be executed on Radio Rx Done event
 * @param  payload sent
 * @param  payload size
 * @param  rssi
 * @param  snr
 * @retval none
 */
static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);

/*!
 * @brief Function executed on Radio Tx Timeout event
 * @param  none
 * @retval none
 */
static void OnTxTimeout(void);

/*!
 * @brief Function executed on Radio Rx Timeout event
 * @param  none
 * @retval none
 */
static void OnRxTimeout(void);

/*!
 * @brief Function executed on Radio Rx Error event
 * @param  none
 * @retval none
 */
static void OnRxError(void);

/* USER CODE BEGIN PFP */
static void SubghzTimerCallback(void *argument);
static void SubghzRegisterTxConfig();
/* USER CODE END PFP */

/* Exported functions ---------------------------------------------------------*/
void SubghzApp_Init(void)
{
  /* USER CODE BEGIN SubghzApp_Init_1 */

  /* USER CODE END SubghzApp_Init_1 */

  /* Radio initialization */
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.RxDone = OnRxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError = OnRxError;

  Radio.Init(&RadioEvents);

  /* USER CODE BEGIN SubghzApp_Init_2 */

  Radio.SetChannel(TXfreq);

  txConfig.fsk.Bandwidth = 0;
  txConfig.fsk.BitRate = 600;
  txConfig.fsk.FrequencyDeviation = 5e3;
  txConfig.fsk.CrcLength = RADIO_FSK_CRC_OFF;
  txConfig.fsk.CrcPolynomial = 0x8005; /* IBM CRC | Reference Manual */
  txConfig.fsk.HeaderType = RADIO_FSK_PACKET_FIXED_LENGTH; /* No Header */
  txConfig.fsk.Whitening = RADIO_FSK_DC_FREE_OFF; /* No Whitening */
  txConfig.fsk.whiteSeed = 0xffff; /* Not Used */
  txConfig.fsk.ModulationShaping = RADIO_FSK_MOD_SHAPING_OFF; /* Extra Filtering */
  txConfig.fsk.PreambleLen = 5;
  txConfig.fsk.SyncWordLength = 0;
  txConfig.fsk.SyncWord = TXsyncWord;

  TXtimeout = 2 * MAX_TX_BUF * 1000 / txConfig.fsk.BitRate;

  SubghzRegisterTxConfig();

  Radio.SetMaxPayloadLength(radioModem, MAX_TX_BUF);

  /* Create Continuous Timer */
  subghzTimer = osTimerNew(SubghzTimerCallback, osTimerPeriodic, NULL, &subghzTimerAttr);
  /* USER CODE END SubghzApp_Init_2 */
}

/* USER CODE BEGIN EF */
/*
 * @brief: Sents an RF packet based on the current settings
 */
void SubghzApp_Sent(char *msg, uint8_t size) {
	Radio.Send((uint8_t *) msg, size);
}

/*
 * @brief: Continuously sents RF packets every <ms> milliseconds
 */
void SubghzApp_StartContinuous(char *msg, uint8_t size, uint32_t ms) {
	memcpy(continuousMsg, msg, size);
	continuousSize = size;

	osTimerStart(subghzTimer, ms);
}

/*
 * @brief: Stop sending continuous packets
 */
void SubghzApp_StopContinuous() {
	osTimerStop(subghzTimer);
}

/*
 * @brief: Get RF frequency
 */
uint32_t SubghzApp_GetFreq() {
	return TXfreq;
}

/*
 * @brief: Set RF frequency
 */
void SubghzApp_SetFreq(uint32_t freq) {
	TXfreq = freq;

	Radio.SetChannel(TXfreq);
}

/*
 * @brief: Get RF power
 */
uint32_t SubghzApp_GetPower() {
	return TXpower;
}

/*
 * @brief: Set RF power
 */
void SubghzApp_SetPower(uint32_t power) {
	TXpower = power;

	SubghzRegisterTxConfig();
}


/*
 * @brief: Get RF packet CRC status
 */
uint8_t SubghzApp_GetCRC() {
	return txConfig.fsk.CrcLength != RADIO_FSK_CRC_OFF;
}

/*
 * @brief: Enable RF packet CRC
 */
void SubghzApp_SetCRC(uint8_t crcEn) {
	txConfig.fsk.CrcLength = crcEn ? RADIO_FSK_CRC_2_BYTES : RADIO_FSK_CRC_OFF;

	SubghzRegisterTxConfig();
}

/*
 * @brief: Get RF datarate
 */
uint32_t SubghzApp_GetDatarate() {
	return txConfig.fsk.BitRate;
}

/*
 * @brief: Set RF datarate
 */
void SubghzApp_SetDatarate(uint32_t datarate) {
	txConfig.fsk.BitRate = datarate;
	TXtimeout = 2 * MAX_TX_BUF * 1000 / datarate;

	SubghzRegisterTxConfig();
}

/*
 * @brief: Get RF FSK frequency deviation
 */
uint32_t SubghzApp_GetFreqDeviation() {
	return txConfig.fsk.FrequencyDeviation;
}

/*
 * @brief: Set RF FSK frequency deviation
 */
void SubghzApp_SetFreqDeviation(uint32_t fdev) {
	txConfig.fsk.FrequencyDeviation = fdev;

	SubghzRegisterTxConfig();
}

/*
 * @brief: Get RF packet preamble length
 */
uint32_t SubghzApp_GetPreambleLength() {
	return txConfig.fsk.PreambleLen;
}

/*
 * @brief: Set RF packet preamble length
 */
void SubghzApp_SetPreambleLength(uint32_t preamble) {
	txConfig.fsk.PreambleLen = preamble;

	SubghzRegisterTxConfig();
}

/*
 * @brief: Get RF packet syncword
 */
uint32_t SubghzApp_GetSyncword(char *word) {
	memcpy(word, txConfig.fsk.SyncWord, txConfig.fsk.SyncWordLength);
	return txConfig.fsk.SyncWordLength;

}

/*
 * @brief: Set RF packet syncword
 */
void SubghzApp_SetSyncword(uint32_t len, const char *word) {
	txConfig.fsk.SyncWordLength = len;
	memcpy(txConfig.fsk.SyncWord, word, len);

	SubghzRegisterTxConfig();
}

/*
 * @brief: Check if whitening is enabled
 */
uint8_t SubghzApp_GetWhiteningStatus() {
	return txConfig.fsk.Whitening == RADIO_FSK_DC_FREEWHITENING ? true : false;
}

/*
 * @brief: Set whitening enable
 */
void SubghzApp_SetWhitening(uint8_t active, uint16_t seed) {
	/* Uses x^9 + x^5 + 1 polynomial */
	txConfig.fsk.Whitening = active ? RADIO_FSK_DC_FREEWHITENING : RADIO_FSK_DC_FREE_OFF;
	txConfig.fsk.whiteSeed = seed;

	SubghzRegisterTxConfig();
}

/*
 * @brief: Register an updated TX Configuration to the peripheral
 */
static void SubghzRegisterTxConfig() {
	/* ( GenericModems_t modem, TxConfigGeneric_t* config, int8_t power, uint32_t timeout ); */
	Radio.RadioSetTxGenericConfig(radioModem, &txConfig, TXpower, TXtimeout);
}

/*
 * @brief: Triggers when the continuous trigger timer triggers
 */
static void SubghzTimerCallback(void *argument) {
	SubghzApp_Sent(continuousMsg, continuousSize);
}
/* USER CODE END EF */

/* Private functions ---------------------------------------------------------*/
static void OnTxDone(void)
{
  /* USER CODE BEGIN OnTxDone_1 */

  /* USER CODE END OnTxDone_1 */
}

static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
  /* USER CODE BEGIN OnRxDone_1 */

  /* USER CODE END OnRxDone_1 */
}

static void OnTxTimeout(void)
{
  /* USER CODE BEGIN OnTxTimeout_1 */

  /* USER CODE END OnTxTimeout_1 */
}

static void OnRxTimeout(void)
{
  /* USER CODE BEGIN OnRxTimeout_1 */

  /* USER CODE END OnRxTimeout_1 */
}

static void OnRxError(void)
{
  /* USER CODE BEGIN OnRxError_1 */

  /* USER CODE END OnRxError_1 */
}

/* USER CODE BEGIN PrFD */

/* USER CODE END PrFD */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
