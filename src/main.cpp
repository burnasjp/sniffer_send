/**
 ******************************************************************************
 * File Name          : main.c
 * Description        : Main program body
 ******************************************************************************
 *
 * COPYRIGHT(c) 2016 STMicroelectronics
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cyccnt.h"

#include <init.hpp>
#include <llusart.hpp>

#include <string.h>
#include <stdio.h>
#include <stdint.h>

/* Private variables ---------------------------------------------------------*/
#define IN_MOSI_PORT GPIOA
#define IN_MOSI_PIN GPIO_PIN_9

#define IN_CLK_PORT GPIOA
#define IN_CLK_PIN GPIO_PIN_10

#define IN_D_PORT GPIOA
#define IN_D_PIN GPIO_PIN_11

#define OUT_RCLK_PORT GPIOA
#define OUT_RCLK_PIN GPIO_PIN_6

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

void MX_GPIO_Init();
void MX_DMA_Init();
void MX_NVIC_Init();

/* Private function prototypes -----------------------------------------------*/

void Loop();

extern "C" int main(void)
{

	/* MCU Configuration----------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

	/* System interrupt init*/
	NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

	/* SysTick_IRQn interrupt configuration */
	NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));

	/** NOJTAG: JTAG-DP Disabled and SW-DP Enabled
	 */
	LL_GPIO_AF_Remap_SWJ_NOJTAG();

	/* Configure the system clock */
	SystemClock_Config();
	CycCnt::Init();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	llusart2.Init();

	CycCnt::TimingDelay_ms(800);

	char mes[] = "\rboot_0203\r\r";
	llusart2.Send((uint8_t *)mes, strlen(mes));

	CycCnt::TimingDelay_ms(200);
	// HAL_Delay
	//  HAL_DelayはHAL_GetTick()に依存し、HAL_GetTickはSysTickに依存している。
	//  割り込み禁止にすると無限ループに陥るため、使用を推奨できない。

	/* Infinite loop */
	Loop();
}

void HAL_SYSTICK_Callback()
{
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @param  None
 * @retval None
 */

extern "C"
{
	void _Error_Handler(const char *file, int line)
	{
		/* USER CODE BEGIN Error_Handler_Debug */
		/* User can add his own implementation to report the HAL error return state */
		while (1)
		{
		}
		/* USER CODE END Error_Handler_Debug */
	}
}

#ifdef USE_FULL_ASSERT

/**
 * @brief Reports the name of the source file and the source line number
 * where the assert_param error has occurred.
 * @param file: pointer to the source file name
 * @param line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}

#endif

// IN_MOSIから32bit受信する。
// Clock　Dutyの極端な比率に対応するため、CLKとMOSIのポートは同じである必要がある

inline uint32_t rors(uint32_t rd, uint32_t rs)
{
	asm("ror %[x], %[x], %[y];"
		: [x] "+r"(rd)
		: [y] "r"(rs));
	return rd;
}

inline uint32_t rors_1(uint32_t rd)
{
	asm("ror %[x], %[x], #1;"
		: [x] "+r"(rd));
	//最適化で以下のコードに最適化される
	// asm( "mov.w %[x], %[x], ror #1;" :[x]"+r"(rd));
	return rd;
}

uint32_t polling()
{
	uint32_t reg = 0;
	uint32_t c = 0;
	volatile uint32_t *in_port = &IN_CLK_PORT->IDR;

	// 4バイト受信完了までに必要な時間は 220us
	// CLKがLOWになってから5.33us後に最初のトリガが来る
	//最初のCLK LOWまでは割り込み有効
	for (int i = 0; i < 4; i++)
	{
		//８ビット受信するまで割り込みを禁止する
		__disable_irq();
		// CLKトリガ間隔は6us 166.7KHz
		for (int j = 0; j < 8; j++)
		{
			while (*in_port & IN_CLK_PIN)
				; // CLKがLOWになるまで待つ

			// CLKがHIGHになっている状態　0.66us
			while (!((reg = *in_port) & IN_CLK_PIN))
				;							  // CLKがHIGHになるまで待つ
			c |= (reg & IN_MOSI_PIN) ? 1 : 0; // MOSIの状態をCに保存する
			c = rors_1(c);
		}
		//次のCLKがHIGHになるまでの時間 3.62us + 5.33us = 8.95us
		//この間に溜まっている割り込みを処理する。この間隔で処理できない内容を割り込みで行わないように注意
		__enable_irq();
		CycCnt::TimingDelay(10);
	}
	return c;
}

struct AA
{
	AA()
	{
		c[0] = 0;
		c[1] = 0;
		c[2] = 0;
		c[3] = 0;
	}
	uint32_t c[4];
	bool operator==(AA &other)
	{
		return memcmp(this, &other, sizeof(AA)) == 0;
	}
	bool operator!=(AA &other)
	{
		return memcmp(this, &other, sizeof(AA)) != 0;
	}
};

void Loop()
{
	AA c0;
	AA d0;
	AA last0;

	// uint32_t c[4] = {0};
	// uint32_t d[4] = {0};
	// uint32_t last[4] = {0};
	volatile uint32_t *in__d_port = &IN_D_PORT->IDR;
	for (;;)
	{
		while ((*in__d_port & IN_D_PIN) == 0)
			; // DがHighになるまで待つ
		while ((*in__d_port & IN_D_PIN) != 0)
			; // DがLOWになるまで待つ
		c0.c[0] = polling();
		c0.c[1] = polling();
		c0.c[2] = polling();
		c0.c[3] = polling();

		//スタートの点滅を無視する
		c0.c[0] &= 0xffff7fffL;

		if (c0 == d0)
		{
			if (c0 != last0)
			{
				last0 = c0;

				char b[128];
				sprintf(b, "S-%08lx-%08lx-%08lx-%08lx-E\n\r", c0.c[0], c0.c[1], c0.c[2], c0.c[3]);
				llusart2.Send((uint8_t *)b, strlen(b));
			}
		}
		d0 = c0;

		//次の32bitが送信されるまで3.7msの猶予がある

		// PC13 flicker
		LL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
	}
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
