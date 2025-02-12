/*
 * HAL_Callbacks.c
 *
 *  Created on: Nov 27, 2024
 *      Author: karl.yamashita
 */

#include "main.h"


/*
 * Description: Copy the DMA buffer data to circular buffer. The circular buffer will be parsed in polling routine.
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	int i = 0;
	uint32_t size = 0;

	if(huart == uart2_msg.huart)
	{
		size = UART_DMA_GetSize(&uart2_msg, Size);

		for(i = 0; i < size; i++)
		{
			uart2_msg.rx.circularBuffer[uart2_msg.ringBuffer.circularPtr.index_IN] = uart2_msg.rx.dma_data[uart2_msg.ringBuffer.dmaPtr.index_IN];
			RingBuff_Ptr_Input(&uart2_msg.ringBuffer.circularPtr, UART_DMA_CIRCULAR_SIZE);
			RingBuff_Ptr_Input(&uart2_msg.ringBuffer.dmaPtr, UART_DMA_BUFFER_SIZE);
		}
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart == uart2_msg.huart)
	{
		uart2_msg.tx.txPending = false;
		UART_DMA_SendMessage(&uart2_msg);
	}
}


