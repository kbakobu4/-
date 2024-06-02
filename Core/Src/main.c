/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "gps.h"
#include "BMP085.h"
#include "HMC5883L.h"
#include "ds18b20.h"
#include "mpu6050.h"
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
      float t, p, a;
      int16_t x;
      int16_t y;
      int16_t z;
      uint16_t raw_temper;
      uint16_t bmp180_error;
      uint16_t hmc5883l_error;
      int16_t flag_uart_busy;
      double temper[4];
      char sign;
      uint8_t Data[8];
      char paket[500];
      char control_data[100];
      MPU6050_t MPU6050;
      int16_t pitch_test;
      int16_t roll_test;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart == &huart1){
	GPS_UART_CallBack(GPS.flag_uart_busy);
	}
}



void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{

	if(htim->Instance == TIM3)
	{
		 MPU6050_Read_All(&hi2c1, &MPU6050);
		if(GPS.flag_uart_busy == 0){
			if(HMC5883L_testConnection()){
				hmc5883l_error = 0;
				HMC5883L_getHeading (&x,&y,&z);
			}
			else{
				hmc5883l_error = 1;
				x = 10000;
				y = 10000;
				z = 10000;
			}
			pitch_test = (int)((float) MPU6050.KalmanAngleY * 100);
			roll_test =	(int)((float) MPU6050.KalmanAngleX * 100);
			HAL_UART_Transmit(&huart3, (uint8_t*)paket, sprintf(paket, "X = %d, Y = %d, Z = %d, Pitch = %d, Roll = %d\r\n", x, y, z, pitch_test, roll_test), 1000);
			HMC5883L_initialize();
		}
	}

}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */


  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_TIM4_Init();
  MX_TIM3_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  I2Cdev_init(&hi2c1);
  GPS_Init();
  HAL_TIM_Base_Start(&htim4);
  HAL_TIM_Base_Start_IT(&htim3);
  htim3.Init.Period = 20000;
  HMC5883L_initialize();
  while (MPU6050_Init(&hi2c1) == 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {


	  for(int j = 1; j <= 4; j++){
		  if (init_ds18b20(j)){
			  HAL_NVIC_DisableIRQ(TIM3_IRQn);
			  HAL_NVIC_DisableIRQ(USART1_IRQn);
			  measure_cmd_ds18b20();
			  HAL_NVIC_EnableIRQ(USART1_IRQn);
			  HAL_NVIC_EnableIRQ(TIM3_IRQn);
			  HAL_Delay(800);
			  HAL_NVIC_DisableIRQ(USART1_IRQn);
			  HAL_NVIC_DisableIRQ(TIM3_IRQn);
			  read_stratcpad_ds18b20(Data);
			  raw_temper = ((uint16_t)Data[1]<<8)|Data[0];
			  if(get_sign_ds18b20(raw_temper)) sign ='-';
			  else sign='+';
			  temper[j-1] = ds18b20_Convert(raw_temper);
		  }
		  else temper[j-1] = 10000;
		  HAL_NVIC_EnableIRQ(USART1_IRQn);
		  HAL_NVIC_EnableIRQ(TIM3_IRQn);
		}

		  BMP085_initialize();
		  if(BMP085_testConnection()){
			  bmp180_error = 0;
			  BMP085_setControl(BMP085_MODE_TEMPERATURE);
			  HAL_Delay(BMP085_getMeasureDelayMilliseconds(BMP085_MODE_TEMPERATURE));
			  t = BMP085_getTemperatureC();
			  BMP085_setControl(BMP085_MODE_PRESSURE_3);
			  HAL_Delay(BMP085_getMeasureDelayMilliseconds(BMP085_MODE_PRESSURE_3));
			  p = BMP085_getPressure();
			  a = BMP085_getAltitude(p, 101325);
		  }
		  else{
			  bmp180_error = 1;
			  t = 10000;
			  p = 10000;
			  a = 10000;
		  }
		  GPS.flag_uart_busy = 1;
		  HAL_UART_Transmit(&huart3, (uint8_t*)paket, sprintf(paket, "Press = %.2f, Temp = %.2f, Alt = %.2f, T1 = %.2f, T2 = %.2f, T3 = %.2f, T4 = %.2f \r\n", p, t, a, temper[0], temper[1], temper[2], temper[3]), 1000);

		  GPS.flag_uart_busy = 0;




    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
