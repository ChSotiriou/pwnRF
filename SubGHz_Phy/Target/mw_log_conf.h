/**
  ******************************************************************************
  * @file    mw_log_conf.h
  * @author  MCD Application Team
  * @brief   Configure (enable/disable) traces for CM0
  *******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MW_LOG_CONF_H__
#define __MW_LOG_CONF_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "sys_app.h"   /* temporary w.a waiting Ticket 92161 resolution */

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
//#define MW_LOG_ENABLED

/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* External variables --------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Exported macro ------------------------------------------------------------*/
#ifdef MW_LOG_ENABLED
/* USER CODE BEGIN Mw_Logs_En*/
#define MW_LOG(TS,VL, ...)
/* USER CODE END Mw_Logs_En */
#else  /* MW_LOG_ENABLED */
/* USER CODE BEGIN Mw_Logs_Dis*/
#define MW_LOG(TS,VL, ...)
/* USER CODE END Mw_Logs_Dis */
#endif /* MW_LOG_ENABLED */
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

#ifdef __cplusplus
}
#endif

#endif /*__MW_LOG_CONF_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
