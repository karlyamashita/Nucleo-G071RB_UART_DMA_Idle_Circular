/*
 * PollingRoutine.c
 *
 *  Created on: Oct 24, 2023
 *      Author: karl.yamashita
 *
 *
 *      Template for projects.
 *
 *      The object of this PollingRoutine.c/h files is to not have to write code in main.c which already has a lot of generated code.
 *      It is cumbersome having to scroll through all the generated code for your own code and having to find a USER CODE section so your code is not erased when CubeMX re-generates code.
 *      
 *      Direction: Call PollingInit before the main while loop. Call PollingRoutine from within the main while loop
 * 
 *      Example;
        // USER CODE BEGIN WHILE
        PollingInit();
        while (1)
        {
            PollingRoutine();
            // USER CODE END WHILE

            // USER CODE BEGIN 3
        }
        // USER CODE END 3
 */


#include "main.h"


const char fwversion[] = "v1.0.1";


extern UART_HandleTypeDef huart2;


// UART2 for Command
#define UART2_DMA_RX_QUEUE_SIZE 10 // queue size
#define UART2_DMA_TX_QUEUE_SIZE 4

UART_DMA_Data uart2_dmaDataRxQueue[UART2_DMA_RX_QUEUE_SIZE] = {0};
UART_DMA_Data uart2_dmaDataTxQueue[UART2_DMA_TX_QUEUE_SIZE] = {0};
UART_DMA_Struct_t uart2_msg =
{
	.huart = &huart2,
	.rx.queueSize = UART2_DMA_RX_QUEUE_SIZE,
	.rx.msgQueue = uart2_dmaDataRxQueue,
	.tx.queueSize = UART2_DMA_TX_QUEUE_SIZE,
	.tx.msgQueue = uart2_dmaDataTxQueue,
	.ringBuffer.dmaPtr.SkipOverFlow = true
};


/*
 * Description: Initialize other routines before main while loop.
 *
 */
void PollingInit(void)
{
	// Enable UART2 interrupt
	UART_DMA_EnableRxInterruptIdle(&uart2_msg);

	// start blinking SOH LED
	TimerCallbackRegisterOnly(&timerCallback, GPIO_LED_Green_Toggle);
	TimerCallbackTimerStart(&timerCallback, GPIO_LED_Green_Toggle, 500, true);

	// let the user know we're up and running
	MCU_Prompt();
}

/*
 * Description: Called from main while loop.
 */
void PollingRoutine(void)
{
	TimerCallbackCheck(&timerCallback);

	UART_DMA_ParseCircularBuffer(&uart2_msg);

	UART_Parse(&uart2_msg);
}

/*
 * Description: Parse Command messages.
 */
void UART_Parse(UART_DMA_Struct_t *msg)
{
	int status = 0;
	char *ptr;
	char retStr[128] = "OK";
	char msg_copy[128] = {0};

	if(UART_DMA_RxMsgRdy(msg))
	{
		Nop();

		memcpy(&msg_copy, msg->rx.msgToParse->data, strlen((char*)msg->rx.msgToParse->data) - 2); // remove CR/LF

		ptr = (char*)msg->rx.msgToParse->data;

		// remove spaces and lower case
		RemoveSpaces(ptr);
		ToLower(ptr);

		if(strncmp((char*)ptr, "set", strlen("set")) == 0)
		{
			ptr += strlen("set");
			status = ParseSet(ptr);
		}
		else if(strncmp((char*)ptr, "get", strlen("get")) == 0)
		{
			ptr += strlen("get");
			status = ParseGet(ptr, retStr);
		}
		else
		{
			status = COMMAND_UNKNOWN;
		}

		// check return status
		if(status == NO_ACK)
		{
			return;
		}
		else if(status != 0) // other return status other than NO_ACK or NO_ERROR
		{
			PrintError(msg, msg_copy, status);
		}
		else // NO_ERROR
		{
			PrintReply(msg, msg_copy, retStr);
		}

		memset(&msg->rx.msgToParse->data, 0, UART_DMA_QUEUE_DATA_SIZE); // clear current buffer index
	}
}

/*
 * Description: Parse Get commands
 */
int ParseGet(char *msg, char *retStr)
{
	int status = NO_ERROR;

	if(strncmp(msg, "fwversion", strlen("fwversion"))== 0)
	{
		sprintf(retStr, "%s", fwversion);
	}
	else
	{
		status = COMMAND_UNKNOWN;
	}

	return status;
}

/*
 * Description: Parse Set commands
 */
int ParseSet(char *msg)
{
	int status = NO_ERROR;

	if(strncmp(msg, "ledtoggle:", strlen("ledtoggle:")) == 0)
	{
		msg += strlen("ledtoggle:");
		if(atoi(msg))
		{
			TimerCallbackTimerStart(&timerCallback, GPIO_LED_Green_Toggle, 500, TIMER_REPEAT);
		}
		else
		{
			TimerCallbackDisable(&timerCallback, GPIO_LED_Green_Toggle);
		}
	}
	else
	{
		status = COMMAND_UNKNOWN;
	}

	return status;
}

void MCU_Prompt(void)
{
	char str[64] = "STM32 Ready";
	strcat(str, " ");
	strcat(str, fwversion);

	UART_DMA_NotifyUser(&uart2_msg, str, strlen(str), true);
}

/*
 * Description: Returns original message + error message.
 * Input: DMA message data structure, the original string message, the error status
 * Output: none
 * Return: none
 */
void PrintError(UART_DMA_Struct_t *msg, char *msg_copy, uint32_t error)
{
	char str[64] = {0};

	GetErrorString(error, str);

	strcat(msg_copy, "=");
	strcat(msg_copy, str);

	UART_DMA_NotifyUser(&uart2_msg, msg_copy, strlen(msg_copy), true);
}

/*
 * Description: Returns the original message + string arguments
 * Input: DMA message data structure, the original string message, the string to add to the original message
 * Output: none
 * Return: none
 */
void PrintReply(UART_DMA_Struct_t *msg, char *msg_copy, char *msg2)
{
	strcat(msg_copy, "=");
	strcat(msg_copy, msg2);
	UART_DMA_NotifyUser(&uart2_msg, msg_copy, strlen(msg_copy), true);
}


void GPIO_LED_Green_Toggle(void)
{
	HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
}



