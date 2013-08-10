#include "uart_support.h"

static USART_TypeDef* UART;
static USART_Buffer_t* pUSART_Buf;
USART_Buffer_t USART1_Buf;
USART_Buffer_t USART2_Buf;

void conio_init(uint32_t port, uint32_t baudrate)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	/* Turn on USART*/
	switch (port)
	{
		case 1 :
			UART = (USART_TypeDef *) USART1_BASE;

			/* Enable GPIO clock */
			RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
			/* Enable UART clock */
			RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	
			/* Connect PXx to USARTx_Tx*/
			GPIO_PinAFConfig(GPIOA,  GPIO_PinSource9, GPIO_AF_USART1);
			/* Connect PXx to USARTx_Rx*/
			GPIO_PinAFConfig(GPIOA,  GPIO_PinSource10, GPIO_AF_USART1);

			/* Configure USART Tx as alternate function  */
			GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
			GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;

			GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
			GPIO_Init(GPIOA, &GPIO_InitStructure);

			/* Configure USART Rx as alternate function  */
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
			GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
			GPIO_Init(GPIOA, &GPIO_InitStructure);

		
			/* Configure one bit for preemption priority */
			NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

			/* Enable the USART1 Interrupt */
			NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
			NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
			NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
			NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
			NVIC_Init(&NVIC_InitStructure);

			USART_StructInit(&USART_InitStructure);
			USART_InitStructure.USART_BaudRate = baudrate;
			USART_Init(UART, &USART_InitStructure);

			/* Init Ring Buffer */
			pUSART_Buf = &USART1_Buf;
			USART1_Buf.RX_Tail = 0;
			USART1_Buf.RX_Head = 0;
			USART1_Buf.TX_Tail = 0;
			USART1_Buf.TX_Head = 0;

			/* Enable USART1 Receive interrupts */
			USART_ITConfig(UART, USART_IT_RXNE, ENABLE);
			USART_Cmd(UART, ENABLE);
	
		break;
 
		case 2 :
 			UART = (USART_TypeDef *) USART2_BASE;

			/* Enable GPIO clock */
			RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
			/* Enable UART clock */
			RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	
			/* Connect PXx to USARTx_Tx*/
			GPIO_PinAFConfig(GPIOA,  GPIO_PinSource2, GPIO_AF_USART2);
			/* Connect PXx to USARTx_Rx*/
			GPIO_PinAFConfig(GPIOA,  GPIO_PinSource3, GPIO_AF_USART2);

			/* Configure USART Tx as alternate function  */
			GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
			GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;

			GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
			GPIO_Init(GPIOA, &GPIO_InitStructure);

			/* Configure USART Rx as alternate function  */
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
			GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
			GPIO_Init(GPIOA, &GPIO_InitStructure);

		
			/* Configure one bit for preemption priority */
			NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

			/* Enable the USART1 Interrupt */
			NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
			NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
			NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
			NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
			NVIC_Init(&NVIC_InitStructure);

			USART_StructInit(&USART_InitStructure);
			USART_InitStructure.USART_BaudRate = baudrate;
			USART_Init(UART, &USART_InitStructure);

			/* Init Ring Buffer */
			pUSART_Buf = &USART2_Buf;
			USART2_Buf.RX_Tail = 0;
			USART2_Buf.RX_Head = 0;
			USART2_Buf.TX_Tail = 0;
			USART2_Buf.TX_Head = 0;

			/* Enable USART Receive interrupts */
			USART_ITConfig(UART, USART_IT_RXNE, ENABLE);
			USART_Cmd(UART, ENABLE);
			
		break;
	
		case 3 :

		break;

	}

}

bool USART_TXBuffer_FreeSpace(USART_Buffer_t* USART_buf)
{
	/* Make copies to make sure that volatile access is specified. */
	uint16_t tempHead = (USART_buf->TX_Head + 1) & (UART_BUFSIZE-1);
	uint16_t tempTail = USART_buf->TX_Tail;

	/* There are data left in the buffer unless Head and Tail are equal. */
	return (tempHead != tempTail);
}

bool USART_TXBuffer_PutByte(USART_Buffer_t* USART_buf, uint8_t data)
{

	uint16_t tempTX_Head;
	bool TXBuffer_FreeSpace;

	TXBuffer_FreeSpace = USART_TXBuffer_FreeSpace(USART_buf);


	if(TXBuffer_FreeSpace)
	{
	  	tempTX_Head = USART_buf->TX_Head;
		
		__disable_irq();
	  	USART_buf->TX[tempTX_Head]= data;
		/* Advance buffer head. */
		USART_buf->TX_Head = (tempTX_Head + 1) & (UART_BUFSIZE-1);
		__enable_irq();

		/* Enable TXE interrupt. */
		USART_ITConfig(UART, USART_IT_TXE, ENABLE);
	}
	return TXBuffer_FreeSpace;
}

bool USART_RXBufferData_Available(USART_Buffer_t* USART_buf)
{
	/* Make copies to make sure that volatile access is specified. */
	uint16_t tempHead = USART_buf->RX_Head;
	uint16_t tempTail = USART_buf->RX_Tail;

	/* There are data left in the buffer unless Head and Tail are equal. */
	return (tempHead != tempTail);
}

uint8_t USART_RXBuffer_GetByte(USART_Buffer_t* USART_buf)
{
	uint8_t ans;

	__disable_irq();
	ans = (USART_buf->RX[USART_buf->RX_Tail]);

	/* Advance buffer tail. */
	USART_buf->RX_Tail = (USART_buf->RX_Tail + 1) & (UART_BUFSIZE-1);
	
	__enable_irq();

	return ans;
}

inline void putch(uint8_t data)
{
	/* Interrupt Version */
	while(!USART_TXBuffer_FreeSpace(pUSART_Buf));
	USART_TXBuffer_PutByte(pUSART_Buf,data);

}

uint8_t getch(void)
{
	if (USART_RXBufferData_Available(pUSART_Buf))  return USART_RXBuffer_GetByte(pUSART_Buf);
	else										   return false;
}

uint8_t keypressed(void)
{
  return (USART_RXBufferData_Available(pUSART_Buf));
  /*return (UART->SR & USART_FLAG_RXNE);*/
}

void cputs(char *s)
{
  while (*s)
    putch(*s++);
}

void cgets(char *s, int bufsize)
{
  char *p;
  int c;

  memset(s, 0, bufsize);

  p = s;

  for (p = s; p < s + bufsize-1;)
  {
    /* 20090521Nemui */
	do{		
		c = getch();
	}while(c == false);
	/* 20090521Nemui */
    switch (c)
    {
      case '\r' :
      case '\n' :
        putch('\r');
        putch('\n');
        *p = '\n';
        return;

      case '\b' :
        if (p > s)
        {
          *p-- = 0;
          putch('\b');
          putch(' ');
          putch('\b');
        }
        break;

      default :
        putch(c);
        *p++ = c;
        break;
    }
  }

  return;
}

