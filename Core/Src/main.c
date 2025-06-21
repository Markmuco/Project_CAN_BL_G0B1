/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 ** This notice applies to any and all portions of this file
 * that are not between comment pairs USER CODE BEGIN and
 * USER CODE END. Other portions of this file, whether
 * inserted by the user or by software development tools
 * are owned by their respective copyright owners.
 *
 * COPYRIGHT(c) 2019 STMicroelectronics
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "crc.h"
#include "fdcan.h"
#include "iwdg.h"
#include "rtc.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "time.h"
#include "shell.h"
#include "flash.h"
#include "crc.h"
#include "uart_sci.h"

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
/*
 *  __  __                  _______           _                    _                _
 * |  \/  |                |__   __|         | |                  | |              (�)
 * | \  / | _   _   ___  ___  | |  ___   ___ | |__   _ __    ___  | |  ___    __ _  _   ___  ___
 * | |\/| || | | | / __|/ _ \ | | / _ \ / __|| '_ \ | '_ \  / _ \ | | / _ \  / _` || | / _ \/ __|
 * | |  | || |_| || (__| (_) || ||  __/| (__ | | | || | | || (_) || || (_) || (_| || ||  __/\__ \
 * |_|  |_| \__,_| \___|\___/ |_| \___| \___||_| |_||_| |_| \___/ |_| \___/  \__, ||_| \___||___/
 *                                                                           __/  |
 *                                                                           |___/
 **************************************************************************************************

 DESCRIPTION:

 REMARKS:

 arm-none-eabi-objcopy -O binary "${BuildArtifactFileBaseName}.elf" "${BuildArtifactFileBaseName}.bin" && arm-none-eabi-size "${BuildArtifactFileName}";${ProjDirPath}/writeCRC.exe "${BuildArtifactFileBaseName}.bin"

 - section MYSEC
 - .data:  *(.RamFunc)
 - Optimize more (-O2)

 NOTES
 CPU freq 48 Mhz
 Uart1 COM port MODBUS baudrate: 115k, 8 databits, 1 stopbit, no parity
 Uart3 DEBUG/ PROGRAM baudrate: 115k, 8 databits, 1 stopbit, no parity
 IWDT 40khz/4095/4 = 0.4096 sec
 Timer6 voor timer functie 48mHz /47/999 = 1msec

 TOOLS
 - CubeMX 5.3.0
 - AC6 System Workbench

 PRINCIPE
 0x08000000 Bootloader 18K
 0x08004000 Application 55kb
 0x08010000 Shadow flash 55kb

 VERSION OVERVIEW:
 0.01
 - eerste versie
 0.02
 - mucu timer on systick
 - flash erase in pages


 */
uint8_t toggle_tmr = NO_TIMER;
uint8_t autostart_tmr = NO_TIMER;

uint32_t *ram_sleep = (uint32_t*) RAM_SLEEP;

/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/*!< Info structure for bootloader */
volatile const app_info_t __attribute__((section (".mySection"))) c_app_info =
{ .key = APP_KEY,				// Key
		.version = VERSION,		// Version
		.crc32 = 0xFFFFFFFF,		// CCITT-CRC32
		.size = 0xFFFFFFFF,		// File size
		.build_date = __DATE__,	// Build date
		.build_time = __TIME__,	// Build time
		.dummy =
		{ 0xFF, 0xFF, 0xFF } // Dummy[3]
};

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

	//__disable_irq();
	app_info_t *p_app_info = (app_info_t*) FLASH_APP_START_ADDR;
//	app_info_t *p_shadow_info = (app_info_t*) FLASH_SHADOW_START_ADDR;
	uint32_t *ram_key = (uint32_t*) RAM_KEY;

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
  MX_CRC_Init();
  MX_IWDG_Init();
  MX_RTC_Init();
  MX_USART1_UART_Init();
  MX_FDCAN1_Init();
  /* USER CODE BEGIN 2 */

	__enable_irq();

	// Open the byteQ
	init_sci();

	// setup the function timer
	timer_open();

	// init the Shell
	shell_open(*ram_key);

//	if (RCC->CSR & RCC_CSR_IWDGRSTF)
//		tty_puts("WDT reboot\r\n");
//	RCC->CSR = RCC_CSR_RMVF;

	if (*ram_key == WAIT_KEY_1 || *ram_key == WAIT_KEY_3)
	{
		tty_puts("'MT' (@) 2025 STM32G0 CHG_BL, Stop\r\n");
	}
	else if (*ram_key == CRASH_KEY)
	{
		tty_puts("'MT' (@) 2025 STM32G0 CHG_BL, Crash\r\n");
		autostart_tmr = timer_get();
		timer_start(autostart_tmr, 2500, NULL);
	}
	else if (p_app_info->key == APP_KEY)
	{
		tty_puts("'MT' (@) 2025 STM32G0 CHG_BL, Start\r\n");
		autostart_tmr = timer_get();
		timer_start(autostart_tmr, 10, NULL);
	}

	else
		tty_puts("'MT' (@) 2025 STM32G0 CHG_BL\r\n");

	// APP need to clear this flag or else app was crashed
	*ram_key = CRASH_KEY;

	app_info_t app_info;

#if 0
	app_info_t app_info;
	app_info_t shadow_info;
	HAL_StatusTypeDef hal_err;

	if (p_shadow_info->key == APP_KEY)
	{
		// Verify integrity of shadow
		if (crc_verify_flash(&shadow_info, FLASH_SHADOW_START_ADDR, SHADOW_SIZE) == 0)
		{
			// get CRC of application
			crc_verify_flash(&app_info, FLASH_APP_START_ADDR, APP_SIZE);
			if ((shadow_info.version & 0xFF) == VERSION_KEY_SHIFTER)
			{
				if (app_info.crc32 != shadow_info.crc32)
				{
					tty_puts("Copy Shadow to App..");
					NVIC_DisableIRQ(USART3_4_LPUART1_IRQn); // IRQs from the main will stall copying
					HAL_Delay(10);

					if (copy_shadow(APP_SIZE))
					{
						autostart_tmr = NO_TIMER;
						tty_puts("Error\r\n");
					}
					else
					{
						// Delete the header
						tty_puts("Erasing shadow flash... ");
						HAL_Delay(10);
						if ((hal_err = stm32_flash_erase(FLASH_SHADOW_START_ADDR, SHADOW_SIZE)) == HAL_OK)
						{
							sci1_puts("OK\r\n");
							autostart_tmr = timer_get();
							timer_start(autostart_tmr, 250, NULL);
						}
						else
						sci1_puts("Error\r\n");

					}
				}
			}
			else
			tty_puts("No Shifter C9 code\r\n");
		}
		else
		tty_puts("Shadow CRC error\r\n");
	}
#endif
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

//	app_info_t app_info;

	while (1)
	{
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		wdt_clr();

		char c;

		shell_process();

		if (timer_elapsed(autostart_tmr))
		{
			timer_free(&autostart_tmr);

			// Verify integrity.
			if (crc_verify_flash(&app_info, FLASH_APP_START_ADDR, APP_SIZE) == 0)
			{
				tty_puts("Jump\r\n");
				HAL_Delay(15);
				JumptoApp();
			}
			else
				tty_puts("APP CRC error\r\n");

		}
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

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 12;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/**
 * Jump to application
 */
void JumptoApp(void)
{
	// disable global interrupt
	__disable_irq();

	// Disable all peripheral interrupts
	NVIC_DisableIRQ(NonMaskableInt_IRQn);
	NVIC_DisableIRQ(HardFault_IRQn);
	NVIC_DisableIRQ(SVC_IRQn);
	NVIC_DisableIRQ(PendSV_IRQn);
	NVIC_DisableIRQ(WWDG_IRQn);
//	NVIC_DisableIRQ(PVD_IRQn);
	NVIC_DisableIRQ(FLASH_IRQn);
//	NVIC_DisableIRQ(RCC_IRQn);
	NVIC_DisableIRQ(EXTI0_1_IRQn);
	NVIC_DisableIRQ(EXTI2_3_IRQn);
	NVIC_DisableIRQ(DMA1_Channel1_IRQn);
	NVIC_DisableIRQ(DMA1_Channel2_3_IRQn);
//	NVIC_DisableIRQ(ADC1_COMP_IRQn);
//	NVIC_DisableIRQ(TIM2_IRQn);
//	NVIC_DisableIRQ(TIM3_IRQn);
	NVIC_DisableIRQ(I2C1_IRQn);
//	NVIC_DisableIRQ(I2C2_IRQn);
	NVIC_DisableIRQ(SPI1_IRQn);
//	NVIC_DisableIRQ(SPI2_IRQn);
	NVIC_DisableIRQ(USART1_IRQn);
//	NVIC_DisableIRQ(USART2_IRQn);
//	NVIC_DisableIRQ(USART3_4_LPUART1_IRQn);



	// main app start address defined in linker file
	extern uint32_t _main_app_start_address;

	uint32_t MemoryAddr = (uint32_t) &_main_app_start_address;
	uint32_t *pMem = (uint32_t *) MemoryAddr;

	// First address is the stack pointer initial value
	__set_MSP(*pMem); // Set stack pointer

	// Now get main app entry point address
	pMem++;
	void (*pMainApp)(void) = (void (*)(void))(*pMem);

	// Jump to main application (0x0800 0004)
	pMainApp();
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
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
