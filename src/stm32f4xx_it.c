#include "main.h"
#ifdef USE_USART
#include "uart_support.h"
#endif

__IO uint8_t PauseResumeStatus = 2, Count = 0;
uint16_t capture = 0;
extern __IO uint16_t CCR_Val;
extern __IO uint8_t RepeatState, AudioPlayStart;
extern uint8_t Buffer[];

void NMI_Handler(void)
{
}

void HardFault_Handler(void)
{
  while (1)
  {
  }
}

void MemManage_Handler(void)
{
  while (1)
  {
  }
}

void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

__IO uint32_t TimingDelay;

void SysTick_Handler(void)
{
}

void USART2_IRQHandler(void)
{
#ifdef USE_USART
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		/* Advance buffer head. */
		uint16_t tempRX_Head = ((&USART2_Buf)->RX_Head + 1) & (UART_BUFSIZE-1);

		/* Check for overflow. */
		uint16_t tempRX_Tail = (&USART2_Buf)->RX_Tail;
		uint8_t data =  USART_ReceiveData(USART2);

		if (tempRX_Head == tempRX_Tail) {
			/* Disable the USART2 Receive interrupt */
			USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
		}else{
			(&USART2_Buf)->RX[(&USART2_Buf)->RX_Head] = data;
			(&USART2_Buf)->RX_Head = tempRX_Head;
		}
	
	}

	if(USART_GetITStatus(USART2, USART_IT_TXE) != RESET)
	{   
		/* Check if all data is transmitted. */
		uint16_t tempTX_Tail = (&USART2_Buf)->TX_Tail;
		if ((&USART2_Buf)->TX_Head == tempTX_Tail){
			/* Overflow MAX size Situation */
			/* Disable the USART2 Transmit interrupt */
			USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
		}else{
			/* Start transmitting. */
			uint8_t data = (&USART2_Buf)->TX[(&USART2_Buf)->TX_Tail];
			USART2->DR = data;

			/* Advance buffer tail. */
			(&USART2_Buf)->TX_Tail = ((&USART2_Buf)->TX_Tail + 1) & (UART_BUFSIZE-1);
		}

	}
#endif
}

void USART1_IRQHandler(void)
{
#ifdef USE_USART
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		/* Advance buffer head. */
		uint16_t tempRX_Head = ((&USART1_Buf)->RX_Head + 1) & (UART_BUFSIZE-1);

		/* Check for overflow. */
		uint16_t tempRX_Tail = (&USART1_Buf)->RX_Tail;
		uint8_t data =  USART_ReceiveData(USART1);

		if (tempRX_Head == tempRX_Tail) {
			/* Disable the USART1 Receive interrupt */
			USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
		}else{
			(&USART1_Buf)->RX[(&USART1_Buf)->RX_Head] = data;
			(&USART1_Buf)->RX_Head = tempRX_Head;
		}
	
	}

	if(USART_GetITStatus(USART1, USART_IT_TXE) != RESET)
	{   
		/* Check if all data is transmitted. */
		uint16_t tempTX_Tail = (&USART1_Buf)->TX_Tail;
		if ((&USART1_Buf)->TX_Head == tempTX_Tail){
			/* Overflow MAX size Situation */
			/* Disable the USART1 Transmit interrupt */
			USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
		}else{
			/* Start transmitting. */
			uint8_t data = (&USART1_Buf)->TX[(&USART1_Buf)->TX_Tail];
			USART1->DR = data;

			/* Advance buffer tail. */
			(&USART1_Buf)->TX_Tail = ((&USART1_Buf)->TX_Tail + 1) & (UART_BUFSIZE-1);
		}

	}
#endif
}

void EXTI1_IRQHandler(void)
{
}

void TIM4_IRQHandler(void)
{
}

void DMA2_Stream0_IRQHandler(void)
{
  /* Test on DMA Stream Transfer Complete interrupt */
  if(DMA_GetITStatus(DMA2_Stream0, DMA_IT_TCIF0))
  {
    /* Clear DMA Stream Transfer Complete interrupt pending bit */
    DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF0);  
    
  }
}

void DMA1_Stream0_IRQHandler(void)
{
  /* Test on DMA Stream Transfer Complete interrupt */
  if(DMA_GetITStatus(DMA1_Stream4, DMA_IT_TCIF0))
  {
    /* Clear DMA Stream Transfer Complete interrupt pending bit */
    DMA_ClearITPendingBit(DMA1_Stream4, DMA_IT_TCIF0);  
    
  }
}

