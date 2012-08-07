#include <math.h>

/* HLD: N/A  => PB6 */
/* RWD: N/A  => PB7 */
/* FF:  PB2  => PB8 */

#ifdef USE_USART
#include "uart_support.h"
#endif
#ifdef USE_PRINTF
#ifndef USE_USART
#error "USE_USART should be defined !!"
#endif
#include <stdio.h>
#endif

#include "main.h"
#include "ff.h"
#include "sd.h"

__IO uint8_t RepeatState = 0;

void initI2S(void);
void startI2S(void);
void setIRQandDMA(void);
void initADC();
extern __IO uint8_t Command_index;


int main(void)
{
  UINT n = 0;
  GPIO_InitTypeDef  GPIO_InitStructure;

/* FF用のPB8の設定 */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
/* RWD用のPB7の設定 */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
/* HOLD用のPB6の設定 */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

#ifdef USE_USART
  conio_init(UART_DEFAULT_NUM,UART_BAUDLATE);
#ifdef USE_PRINTF
  printf("\n");
  printf("Welcome to %s test program !!\n",MPU_SUBMODEL);
  printf("Version %s!!\n",APP_VERSION);
  printf("Build Date : %s\n",__DATE__);
#else
  cputs("\r\n");
  cputs("Welcome to");
  cputs(MPU_SUBMODEL);
  cputs("test program !!\r\n");
  cputs("Version");
  cputs(APP_VERSION);
  cputs("!!\r\n");
  cputs("Build Date : ");
  cputs(__DATE__);
  cputs("\r\n");
#endif
#endif

/* ADCの初期化 */
  initADC();

/* I2Sポートの初期化*/
  initI2S();
/* I2Sポートをスタート */
  startI2S();

/* IRQの設定とDMAのイネーブル */
  setIRQandDMA();

  if ((!initSD()) && (lsSD() == FR_OK)) {
    while (Command_index != 3) {
      sdio_playNO(n);
      if ((Command_index == 0) || (Command_index == 1)) {
        n++;
        if (n >= sMUSIC) {
          n = 0;
        }
      } else if (Command_index == 2){
        if (n == 0) {
          if (sMUSIC > 0) {
            n = sMUSIC - 1;
          } else {
            n = 0;
          }
        } else {
          n--;
        }
      }
    }
  }
#ifndef NO_ADC
#ifdef USE_PRINTF
  printf("Low Voltage Stop\n");
#endif
  PWR_EnterSTANDBYMode();
#endif

  while(1) {
  }
}

void initI2S(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* WS  PA4 */
  /* SCK PB3*/
  /* SD  PB5*/
  /* MCK PC7*/
  /* Enable I2S GPIO clocks */
  RCC_AHB1PeriphClockCmd((RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOA), ENABLE);
  /* CODEC_I2S pins configuration: WS, SCK and SD pins -----------------------------*/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_5; 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  /* Connect pins to I2S peripheral  */
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource4, GPIO_AF_SPI3);  
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource3, GPIO_AF_SPI3);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 ;
  GPIO_Init(GPIOA, &GPIO_InitStructure); 
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_SPI3);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7; 
  GPIO_InitStructure.GPIO_Mode = GPIO_AF_SPI3;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOC, &GPIO_InitStructure);   
  /* Connect pins to I2S peripheral  */
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_SPI3); 
}

void startI2S(void)
{
  I2S_InitTypeDef I2S_InitStructure;
  /* Enable the CODEC_I2S peripheral clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
  /* CODEC_I2S peripheral configuration */
  SPI_I2S_DeInit(SPI3);
  I2S_InitStructure.I2S_AudioFreq = 48000;
  I2S_InitStructure.I2S_Standard = I2S_Standard_Phillips;
  I2S_InitStructure.I2S_DataFormat = I2S_DataFormat_16b;
  I2S_InitStructure.I2S_CPOL = I2S_CPOL_Low;
  I2S_InitStructure.I2S_Mode = I2S_Mode_MasterTx;
  I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Enable;
  I2S_Init(SPI3, &I2S_InitStructure);
}

#define AUDIO_MAL_DMA_IT_TC_EN
#define AUDIO_MAL_MODE_NORMAL
DMA_InitTypeDef DMA_InitStructure; 

void setIRQandDMA(void)  
{ 
  NVIC_InitTypeDef NVIC_InitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE); 
  DMA_Cmd(DMA1_Stream7, DISABLE);
  DMA_DeInit(DMA1_Stream7);
  DMA_InitStructure.DMA_Channel = DMA_Channel_0;  
  DMA_InitStructure.DMA_PeripheralBaseAddr = 0x40003C0C;
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)0;
  DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
  DMA_InitStructure.DMA_BufferSize = (uint32_t)0xFFFE;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord; 
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;  
  DMA_Init(DMA1_Stream7, &DMA_InitStructure);  
  DMA_ITConfig(DMA1_Stream7, DMA_IT_TC, ENABLE);
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream7_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Tx, ENABLE);  
}

// ADC Init
void initADC()
{
#ifndef NO_ADC
  GPIO_InitTypeDef      GPIO_InitStructure;
  ADC_InitTypeDef       ADC_InitStructure;
  ADC_CommonInitTypeDef ADC_CommonInitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div6;
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
  ADC_CommonInit(&ADC_CommonInitStructure);
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
//  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfConversion = 1;
  ADC_Init(ADC1, &ADC_InitStructure);
  ADC_Cmd(ADC1, ENABLE);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_7, 1, ADC_SampleTime_3Cycles);

  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
//  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfConversion = 1;
  ADC_Init(ADC2, &ADC_InitStructure);
  ADC_Cmd(ADC2, ENABLE);
  ADC_RegularChannelConfig(ADC2, ADC_Channel_6, 1, ADC_SampleTime_3Cycles);
  ADC_SoftwareStartConv(ADC1);
  ADC_SoftwareStartConv(ADC2);
#endif
}
