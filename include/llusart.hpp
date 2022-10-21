/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    usart.h
 * @brief   This file contains all the function prototypes for
 *          the usart.c file
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#pragma once

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <fifo_buf.hpp>

#ifdef USART1_ENABLE
void MX_USART1_UART_Init(void);
#endif

#ifdef USART2_ENABLE
void MX_USART2_UART_Init(void);
#endif

#ifdef USART3_ENABLE
void MX_USART3_UART_Init(void);
#endif

template <uint8_t port, uint8_t tx_buf_capatity, uint8_t rx_buf_capatity>
class LLusart
{
private:
	bool is_tx_dma_avtive;
	bool is_rx_dma_avtive;

	fifo_buf<tx_buf_capatity> tx_buf;
	fifo_buf<rx_buf_capatity> rx_buf;
	USART_TypeDef *usart;
	uint32_t dmaChannel_Tx;
	uint32_t dmaChannel_Rx;

public:
	LLusart() : is_tx_dma_avtive(false), is_rx_dma_avtive(false)
	{
		switch (port)
		{
#ifdef USART1_ENABLE
		case 1:
			usart = USART1;
			dmaChannel_Tx = LL_DMA_CHANNEL_4;
			dmaChannel_Rx = LL_DMA_CHANNEL_5;
			break;
#endif
#ifdef USART2_ENABLE
		case 2:
			usart = USART2;
			dmaChannel_Tx = LL_DMA_CHANNEL_7;
			dmaChannel_Rx = LL_DMA_CHANNEL_6;
			break;
#endif
#ifdef USART3_ENABLE
		case 3:
			usart = USART3;
			dmaChannel_Tx = LL_DMA_CHANNEL_2;
			dmaChannel_Rx = LL_DMA_CHANNEL_3;
			break;
#endif
		}
	}

	void Init()
	{
		switch (port)
		{
#ifdef USART1_ENABLE
		case 1:
			MX_USART1_UART_Init();
			break;
#endif
#ifdef USART2_ENABLE
		case 2:
			MX_USART2_UART_Init();
			break;
#endif
#ifdef USART3_ENABLE
		case 3:
			MX_USART3_UART_Init();
			break;
#endif
		}
	}

	void Send_Start()
	{
		if (!tx_buf.empty())
		{
			uint16_t d = tx_buf.front();
			tx_buf.pop_front();
			LL_USART_TransmitData8(usart, d);
			LL_USART_EnableIT_TXE(usart);
		}
	}

#if USART_TX_DMA_ENABLE
	void Transmit_DMA(void *p, size_t size)
	{
		while( LL_DMA_IsActiveFlag_TC7(DMA1) == 0 );
		LL_DMA_DisableChannel(DMA1, dmaChannel_Tx); 
		LL_DMA_SetMemoryAddress(DMA1, dmaChannel_Tx, (uint32_t)p);
		LL_DMA_SetPeriphAddress(DMA1, dmaChannel_Tx, (uint32_t)&usart->DR);
		LL_DMA_SetDataLength(DMA1, dmaChannel_Tx, size);
		LL_DMA_EnableIT_TC(DMA1, dmaChannel_Tx);
		LL_DMA_EnableIT_TE(DMA1, dmaChannel_Tx);
		LL_USART_EnableDMAReq_TX(usart);				
		LL_USART_ClearFlag_TC(usart);					
		LL_DMA_EnableChannel(DMA1, dmaChannel_Tx); 
	}
#endif

	void Send(uint8_t *buf, size_t size)
	{
		tx_buf.push_back(buf, size);
		Send_Start();
	}

	//受信バッファにあるデータのバイト数を返す
	void getReceiveDataSize()
	{
		return rx_buf.getLength();
	}

	size_t getReceiveData(void *buf, size_t size)
	{
		auto length = max(rx_buf.getLength(), size);
		return length;
	}

	void IRQ_Handler()
	{
		if (LL_USART_IsActiveFlag_TXE(usart))
		{
			if (tx_buf.getLength())
			{
				uint16_t d = tx_buf.front();
				tx_buf.pop_front();
				LL_USART_TransmitData8(usart, d);
			}
			else
			{
				LL_USART_DisableIT_TXE(usart);
			}
		}

		if (LL_USART_IsActiveFlag_RXNE(usart) && LL_USART_IsEnabledIT_RXNE(usart))
		{
			uint8_t d = LL_USART_ReceiveData8(usart);
			LL_USART_ClearFlag_RXNE(usart);
			rx_buf.push_back(d);
		}
	}
};

#ifdef USART1_ENABLE
typedef LLusart<1, 240, 240> LLusart1_t;
extern LLusart1_t llusart1;
#endif

#ifdef USART2_ENABLE
typedef LLusart<2, 240, 240> LLusart2_t;
extern LLusart2_t llusart2;
#endif

#ifdef USART3_ENABLE
typedef LLusart<3, 240, 240> LLusart3_t;
extern LLusart3_t llusart3;
#endif
