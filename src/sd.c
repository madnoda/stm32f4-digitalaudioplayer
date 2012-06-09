#include "integer.h"

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
#define BUFSIZE  32*1024

DWORD acc_size;
WORD acc_files, acc_dirs;
FILINFO Finfo;
DIR  dir;
FATFS Fatfs[_VOLUMES];
FIL fileR;
BYTE Buff[BUFSIZE] __attribute__ ((aligned (4))) ;
__IO uint8_t Command_index = 0;

UINT sWAV; // WAV ファイルの数

FATFS *fs;  /* Pointer to file system object */

#if _USE_LFN
char Lfname[512];
#endif

uint32_t get_fattime (void)
{
  uint32_t res;
#ifdef USE_REDBULL
  /* See rtc_support.h */
  RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);  
  RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);
  
  res =  (( (uint32_t)ts_year - 1980) << 25)
      | ((uint32_t)(ts_mon) << 21)
      | ((uint32_t)ts_mday << 16)
      | (uint32_t)(ts_hour << 11)
      | (uint32_t)(ts_min << 5)
      | (uint32_t)(ts_sec >> 1);
#else
  res =  (( (uint32_t)2011 - 1980) << 25)
      | ((uint32_t)(11+1) << 21)
      | ((uint32_t)13 << 16)
      | (uint32_t)(22 << 11)
      | (uint32_t)(23 << 5)
      | (uint32_t)(24 >> 1);
#endif
  return res;
}

static void put_rc (FRESULT rc)
{
#ifdef USE_PRINTF
  const char *str =
    "OK\0" "DISK_ERR\0" "INT_ERR\0" "NOT_READY\0" "NO_FILE\0" "NO_PATH\0"
    "INVALID_NAME\0" "DENIED\0" "EXIST\0" "INVALID_OBJECT\0" "WRITE_PROTECTED\0"
    "INVALID_DRIVE\0" "NOT_ENABLED\0" "NO_FILE_SYSTEM\0" "MKFS_ABORTED\0" "TIMEOUT\0"
    "LOCKED\0" "NOT_ENOUGH_CORE\0" "TOO_MANY_OPEN_FILES\0";
  FRESULT i;

  for (i = 0; i != rc && *str; i++) {
    while (*str++) ;
  }
  printf("rc=%u FR_%s\n", (UINT)rc, str);
#endif
}

static FRESULT scan_files (
  char* path /* Pointer to the path name working buffer */
)
{
  DIR dirs;
  FRESULT res;
  BYTE i;
  char *fn;
  if ((res = f_opendir(&dirs, path)) == FR_OK) {
    i = strlen(path);
    while (((res = f_readdir(&dirs, &Finfo)) == FR_OK) && Finfo.fname[0]){
      if (_FS_RPATH && Finfo.fname[0] == '.') continue;
#if _USE_LFN
      fn = *Finfo.lfname ? Finfo.lfname : Finfo.fname;
#else
      fn = Finfo.fname;
#endif
      if (Finfo.fattrib & AM_DIR) {
        acc_dirs++;
        *(path+i) = '/';
        strcpy(path+i+1, fn);
        res = scan_files(path);
        *(path+i) = '\0';
        if (res != FR_OK) break;
      } else {
        acc_files++;
        acc_size += Finfo.fsize;
      }
    }
  }
  return res;
}

FRESULT initSD()
{
  BYTE res;
  long p2;
#ifdef USE_PRINTF
  static const BYTE ft[] = {0, 12, 16, 32};
#endif

#if _USE_LFN
  Finfo.lfname = Lfname;
  Finfo.lfsize = sizeof(Lfname);
#endif
  f_mount(0, &Fatfs[0]);
#ifdef USE_PRINTF
  printf("_MAX_SS : %d\n",_MAX_SS);
  printf("FatFs module test terminal for %s\n",MPU_SUBMODEL);
  cputs(_USE_LFN ? "LFN Enabled" : "LFN Disabled");
  printf(", Code page: %u\n", _CODE_PAGE);
#endif
  res = f_getfree("", (DWORD*)&p2, &fs);
  if (res) {
    put_rc(res);
  } else {
#ifdef USE_PRINTF
    printf("FAT type = FAT%u\nBytes/Cluster = %lu\nNumber of FATs = %u\n"
            "Root DIR entries = %u\nSectors/FAT = %lu\nNumber of clusters = %lu\n"
            "FAT start (lba) = %lu\nDIR start (lba,clustor) = %lu\nData start (lba) = %lu\n\n...",
            ft[fs->fs_type & 3], (DWORD)fs->csize * 512, (WORD)fs->n_fats,
            fs->n_rootdir, fs->fsize, (DWORD)fs->n_fatent - 2,
            fs->fatbase, fs->dirbase, fs->database
    );
#endif
    acc_size = acc_files = acc_dirs = 0;
#if _USE_LFN
    Finfo.lfname = Lfname;
    Finfo.lfsize = sizeof(Lfname);
#endif
    res = scan_files("");
    if (res) {
      put_rc(res);
    } else {
#ifdef USE_PRINTF

      printf("\r%u files, %lu bytes.\n%u folders.\n"
              "%lu KB total disk space.\n%lu KB available.\n",
              acc_files, acc_size, acc_dirs,
              (fs->n_fatent - 2) * (fs->csize / 2), p2 * (fs->csize / 2)
      );
#endif
    }
  }
  return res;
}

char upper(char c)
{
  if ((c >= 'a') && (c <= 'z'))
    return c - 'a' + 'A';
  return c;
}

int isEx(char *filename,char * ex)
{
  int p1,p2;
  p1 = 0;
  while ((filename[p1] != 0) && (filename[p1] != '.')) {
    p1++;
  }
  if (filename[p1] == 0)
    return 0;
  p1++;
  if (filename[p1] == 0)
    return 0;
  p2 = 0;
  while ((filename[p1] != 0) && (ex[p2] != 0)) {
    if (upper(filename[p1]) != upper(ex[p2]))
      return 0;
    p1++;
    p2++;
  }
  if ((filename[p1] == 0) && (ex[p2] == 0))
    return 1;
  return 0;
}

FRESULT lsSD()
{
  BYTE res;
  long p1, p2;
  UINT s1, s2;
  res = f_getfree("", (DWORD*)&p2, &fs);// testcode
  res = f_opendir(& dir, "");
  if (res) {
    put_rc(res);
  } else {
    p1 = s1 = s2 = 0;
    sWAV = 0;
    for(;;) {
      res = f_readdir(& dir, &Finfo);
      if ((res != FR_OK) || !Finfo.fname[0]) break;
      if (Finfo.fattrib & AM_DIR) {
        s2++;
      } else {
        s1++; p1 += Finfo.fsize;
        if (isEx(&(Finfo.fname[0]),"WAV")) {
          sWAV++;
        }
      }
#ifdef USE_PRINTF
      printf("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s",
          (Finfo.fattrib & AM_DIR) ? 'D' : '-',
          (Finfo.fattrib & AM_RDO) ? 'R' : '-',
          (Finfo.fattrib & AM_HID) ? 'H' : '-',
          (Finfo.fattrib & AM_SYS) ? 'S' : '-',
          (Finfo.fattrib & AM_ARC) ? 'A' : '-',
          (Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
          (Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,
          Finfo.fsize, &(Finfo.fname[0]));
#if _USE_LFN
      for (p2 = strlen(Finfo.fname); p2 < 14; p2++)
        cputs(" ");
      printf("%s\n", Lfname);
#else
      cputs("\r\n");
#endif
#endif
    }
#ifdef USE_PRINTF
    printf("%4u File(s),%10lu bytes total\n%4u  dir(s)", s1, p1, s2);
#endif
    res = f_getfree("", (DWORD*)&p1, &fs);
#ifdef USE_PRINTF
    if (res == FR_OK)
      printf(", %10lu bytes free\n", p1 * fs->csize * 512);
    else
      put_rc(res);
    printf("WAVE File %u \n", sWAV);
#endif
  }
  return res;
}

void sdio_playNO(UINT n)
{
  BYTE res;
  long p1, p2;
  UINT s1, s2;
  res = f_getfree("", (DWORD*)&p2, &fs);// testcode
  res = f_opendir(& dir, "");
  if (res) {
    put_rc(res);
  } else {
    p1 = s1 = s2 = 0;
    sWAV = 0;
    for(;;) {
      res = f_readdir(& dir, &Finfo);
      if ((res != FR_OK) || !Finfo.fname[0]) break;
      if (Finfo.fattrib & AM_DIR) {
        s2++;
      } else {
        s1++; p1 += Finfo.fsize;
        if (isEx(&(Finfo.fname[0]),"WAV")) {
          if (n == sWAV) {
            sdio_play(&(Finfo.fname[0]));
          }
          sWAV++;
        }
      }
    }
  }
}

#define  CHUNK_ID                            0x52494646  /* correspond to the letters 'RIFF' */
#define  FILE_FORMAT                         0x57415645  /* correspond to the letters 'WAVE' */
#define  FORMAT_ID                           0x666D7420  /* correspond to the letters 'fmt ' */
#define  DATA_ID                             0x64617461  /* correspond to the letters 'data' */
#define  LIST_ID                             0x4C495354  /* correspond to the letters 'LIST' */
#define  FACT_ID                             0x66616374  /* correspond to the letters 'fact' */
#define  WAVE_FORMAT_PCM                     0x01
#define  FORMAT_CHNUK_SIZE                   0x10
#define  CHANNEL_MONO                        0x01
#define  CHANNEL_STEREO                      0x02
#define  SAMPLE_RATE_8000                    8000
#define  SAMPLE_RATE_11025                   11025
#define  SAMPLE_RATE_22050                   22050
#define  SAMPLE_RATE_44100                   44100
#define  BITS_PER_SAMPLE_8                   8
#define  BITS_PER_SAMPLE_16                  16

typedef enum
{
  LittleEndian,
  BigEndian
}Endianness;

typedef struct
{
  uint32_t  RIFFchunksize;
  uint16_t  FormatTag;
  uint16_t  NumChannels;
  uint32_t  SampleRate;
  uint32_t  ByteRate;
  uint16_t  BlockAlign;
  uint16_t  BitsPerSample;
  uint32_t  DataSize;
}
WAVE_FormatTypeDef;

typedef enum
{
  Valid_WAVE_File = 0,
  Unvalid_RIFF_ID,
  Unvalid_WAVE_Format,
  Unvalid_FormatChunk_ID,
  Unsupporetd_FormatTag,
  Unsupporetd_Number_Of_Channel,
  Unsupporetd_Sample_Rate,
  Unsupporetd_Bits_Per_Sample,
  Unvalid_DataChunk_ID,
  Unsupporetd_ExtraFormatBytes,
  Unvalid_FactChunk_ID
} ErrorCode;

 static uint32_t wavelen = 0;
 __IO ErrorCode WaveFileStatus = Unvalid_RIFF_ID;
 uint16_t buffer[_MAX_SS] ={0x00};
 #define RING_BUF_SIZE 4
 uint16_t ring_buffer[RING_BUF_SIZE][_MAX_SS] ={0x00};
 volatile uint8_t ring_buffer_p_in  = 0;
 volatile uint8_t ring_buffer_p_out = 0;
 volatile uint8_t ring_buffer_p_out_old = 0;
 static __IO uint32_t SpeechDataOffset = 0x00;
 UINT BytesRead;
 WAVE_FormatTypeDef WAVE_Format;
// uint8_t buffer_switch = 1;
 extern FIL fileR;
 extern DIR dir;

__IO uint8_t volume = 70, AudioPlayStart = 0;
__IO uint32_t WaveCounter;
__IO uint32_t WaveDataLength = 0;
static __IO uint32_t TimingDelay;

 static ErrorCode WavePlayer_WaveParsing(uint32_t *FileLen);
uint32_t ReadUnit(uint8_t *buffer, uint8_t idx, uint8_t NbrOfBytes, Endianness BytesFormat);

void Delay(__IO uint32_t nTime)
{
  TimingDelay = nTime;
  
  while(TimingDelay != 0);
}

void sdio_play(char * filename)
{ 

  char path[] = "0:/";
  ring_buffer_p_in  = 0;
  ring_buffer_p_out = 0;
  ring_buffer_p_out_old = ring_buffer_p_out;
  /* Get the read out protection status */
  if (f_opendir(&dir, path)!= FR_OK)
  {
    while(1)
    {
      Delay(10);
    }
  } else {
    if (f_open(&fileR, filename , FA_READ) != FR_OK) {
      Command_index = 1;
    } else {    
      f_read (&fileR, buffer, _MAX_SS, &BytesRead);
      {
#ifdef USE_PRINTF
         int i,j;
         unsigned char w;
         for(i = 0; i < 256; i += 16) {
           printf("%04x ",i);
           for(j = 0; j < 16; j += 2) {
             printf("%02x ",buffer[(i + j) / 2] % 256);
             printf("%02x ",buffer[(i + j) / 2] / 256);
           }
           for(j = 0; j < 16; j += 2) {
             w = buffer[(i + j) / 2] % 256;
             if ((w >= '0') && (w <= '9')) {
               printf("%c",w);
             } else if ((w >= 'A') && (w <= 'Z')) {
               printf("%c",w);
             } else if ((w >= 'a') && (w <= 'z')) {
               printf("%c",w);
             } else {
               printf(".");
             }
             w = buffer[(i + j) / 2] / 256;
             if ((w >= '0') && (w <= '9')) {
               printf("%c",w);
             } else if ((w >= 'A') && (w <= 'Z')) {
               printf("%c",w);
             } else if ((w >= 'a') && (w <= 'z')) {
               printf("%c",w);
             } else {
               printf(".");
             }
           }
           printf("\n");
         }
#endif
      }
      WaveFileStatus = WavePlayer_WaveParsing(&wavelen);
      if (WaveFileStatus == Valid_WAVE_File) {
#ifdef USE_PRINTF

        /* the .WAV file is valid */
        printf("the .WAV file %s is valid\r\n",filename);
        /* Set WaveDataLenght to the Speech wave length */
#endif
        WaveDataLength = WAVE_Format.DataSize;
#ifdef USE_PRINTF
        printf("Data Size : %d\n",WAVE_Format.DataSize);
        printf("Sample Rate : %0.1f Hz\n",(double)WAVE_Format.SampleRate/1000.0);
        printf("Byte Rate : %d\n",WAVE_Format.ByteRate);
        printf("Time : %0.1f sec\n",((double)WaveDataLength) / ((double)WAVE_Format.ByteRate));
#endif
      } else{
        /* Unvalid wave file */
#ifdef USE_PRINTF
        cputs("Unvalid wave file\r\n");
#endif
        while(1)
        {
          Delay(10);
        }
      }
      /* Play the wave */
      WavePlayBack(WAVE_Format.SampleRate);
    }    
  }
}

static ErrorCode WavePlayer_WaveParsing(uint32_t *FileLen)
{
  uint32_t temp = 0x00;
  uint32_t extraformatbytes = 0;
  
  /* Read chunkID, must be 'RIFF' */
  temp = ReadUnit((uint8_t*)buffer, 0, 4, BigEndian);
  if (temp != CHUNK_ID) {
    return(Unvalid_RIFF_ID);
  }
  
  /* Read the file length */
  WAVE_Format.RIFFchunksize = ReadUnit((uint8_t*)buffer, 4, 4, LittleEndian);
  
  /* Read the file format, must be 'WAVE' */
  temp = ReadUnit((uint8_t*)buffer, 8, 4, BigEndian);
  if (temp != FILE_FORMAT) {
    return(Unvalid_WAVE_Format);
  }
  
  /* Read the format chunk, must be'fmt ' */
  temp = ReadUnit((uint8_t*)buffer, 12, 4, BigEndian);
  if (temp != FORMAT_ID) {
    return(Unvalid_FormatChunk_ID);
  }
  /* Read the length of the 'fmt' data, must be 0x10 -------------------------*/
  temp = ReadUnit((uint8_t*)buffer, 16, 4, LittleEndian);
  if (temp != 0x10) {
    extraformatbytes = 1;
  }
  /* Read the audio format, must be 0x01 (PCM) */
  WAVE_Format.FormatTag = ReadUnit((uint8_t*)buffer, 20, 2, LittleEndian);
  if (WAVE_Format.FormatTag != WAVE_FORMAT_PCM) {
    return(Unsupporetd_FormatTag);
  }
  
  /* Read the number of channels, must be 0x01 (Mono) or 0x02 (Stereo) */
  WAVE_Format.NumChannels = ReadUnit((uint8_t*)buffer, 22, 2, LittleEndian);
  
  /* Read the Sample Rate */
  WAVE_Format.SampleRate = ReadUnit((uint8_t*)buffer, 24, 4, LittleEndian);

  /* Read the Byte Rate */
  WAVE_Format.ByteRate = ReadUnit((uint8_t*)buffer, 28, 4, LittleEndian);
  
  /* Read the block alignment */
  WAVE_Format.BlockAlign = ReadUnit((uint8_t*)buffer, 32, 2, LittleEndian);
  
  /* Read the number of bits per sample */
  WAVE_Format.BitsPerSample = ReadUnit((uint8_t*)buffer, 34, 2, LittleEndian);
  if (WAVE_Format.BitsPerSample != BITS_PER_SAMPLE_16) {
    return(Unsupporetd_Bits_Per_Sample);
  }
  SpeechDataOffset = 36;
  /* If there is Extra format bytes, these bytes will be defined in "Fact Chunk" */
  if (extraformatbytes == 1) {
    /* Read th Extra format bytes, must be 0x00 */
    temp = ReadUnit((uint8_t*)buffer, 36, 2, LittleEndian);
    if (temp != 0x00)
    {
      return(Unsupporetd_ExtraFormatBytes);
    }
    /* Read the Fact chunk, must be 'fact' */
    temp = ReadUnit((uint8_t*)buffer, 38, 4, BigEndian);
    if (temp != FACT_ID) {
      return(Unvalid_FactChunk_ID);
    }
    /* Read Fact chunk data Size */

    temp = ReadUnit((uint8_t*)buffer, 42, 4, LittleEndian);
    
    SpeechDataOffset += 10 + temp;
  }
  /* Read the Data chunk, must be 'data' */
  temp = ReadUnit((uint8_t*)buffer, SpeechDataOffset, 4, BigEndian);
  SpeechDataOffset += 4;
  while (temp == LIST_ID) {
    temp = ReadUnit((uint8_t*)buffer, SpeechDataOffset, 4, LittleEndian);
#ifdef USE_PRINTF
    printf("LIST_ID %d\n",temp);
#endif
    SpeechDataOffset += 4;
    SpeechDataOffset += temp;
#ifdef USE_PRINTF
    printf("NEXT %x\n",SpeechDataOffset);
#endif
    temp = ReadUnit((uint8_t*)buffer, SpeechDataOffset, 4, BigEndian);
    SpeechDataOffset += 4;
  }
  if (temp != DATA_ID) {
    return(Unvalid_DataChunk_ID);
  }
  
  /* Read the number of sample data */
  WAVE_Format.DataSize = ReadUnit((uint8_t*)buffer, SpeechDataOffset, 4, LittleEndian);
  SpeechDataOffset += 4;
  WaveCounter =  SpeechDataOffset;
  return(Valid_WAVE_File);
}

uint32_t ReadUnit(uint8_t *buffer, uint8_t idx, uint8_t NbrOfBytes, Endianness BytesFormat)
{
  uint32_t index = 0;
  uint32_t temp = 0;
  
  for (index = 0; index < NbrOfBytes; index++)
  {
    temp |= buffer[idx + index] << (index * 8);
  }
  
  if (BytesFormat == BigEndian)
  {
    temp = __REV(temp);
  }
  return temp;
}

void play(uint32_t Addr, uint32_t Size);

void WavePlayBack(uint32_t AudioFreq)
{ 
  /* Start playing */
  AudioPlayStart = 1;
  Command_index = 0;
  int sw_status = 1;
  int sw_count = 0;
//  RepeatState =0;

  /* Initialize wave player (Codec, DMA, I2C) */
//  WavePlayerInit(AudioFreq);

// ADC Init
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

  int adcCount = 0;
  int lowVoltage = 0;
  uint16_t adc1w,adc2w;
  /* Get Data from SD CARD */
  f_lseek(&fileR, WaveCounter);
  ring_buffer_p_in  = 0;
  ring_buffer_p_out = 0;
  ring_buffer_p_out_old = ring_buffer_p_out;
  while (ring_buffer_p_in < RING_BUF_SIZE) {
    f_read (&fileR, ring_buffer[ring_buffer_p_in], _MAX_SS, &BytesRead); 
    ring_buffer_p_in++;
  }
  ring_buffer_p_in  = 0;
  /* Start playing wave */
  play((uint32_t)ring_buffer[0], _MAX_SS);
//  buffer_switch = 1;
  ring_buffer_p_in  = 0;
  ring_buffer_p_out = 0;
  ring_buffer_p_out_old = ring_buffer_p_out;
//  PauseResumeStatus = 1;
//  Count = 0;
  
  while(WaveDataLength != 0)
  { 
    /* Test on the command: Playing */
    if (Command_index == 0)
    { 
      /* wait for DMA transfert complete */
      while(ring_buffer_p_in == ring_buffer_p_out) { }
      if (ring_buffer_p_out_old != ring_buffer_p_out) {
        play((uint32_t)ring_buffer[ring_buffer_p_out], _MAX_SS);
        ring_buffer_p_out_old = ring_buffer_p_out;
      }
      f_read (&fileR, ring_buffer[ring_buffer_p_in], _MAX_SS, &BytesRead);
      ring_buffer_p_in++;
      if (ring_buffer_p_in >= RING_BUF_SIZE) {
        ring_buffer_p_in = 0;
      }
      if (adcCount == 0) {
        adcCount = 100;
        while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
        while(ADC_GetFlagStatus(ADC2, ADC_FLAG_EOC) == RESET);
        adc1w = ADC_GetConversionValue(ADC1);
        adc2w = ADC_GetConversionValue(ADC2);
#ifdef USE_PRINTF
        printf("ADC %d %d\n",adc1w,adc2w);
#endif
        if (adc2w >= 2050) {
          lowVoltage++;
          if (lowVoltage >= 5) {
            Command_index = 3;
          }
        } else {
          lowVoltage = 0;
        }
        ADC_SoftwareStartConv(ADC1);
        ADC_SoftwareStartConv(ADC2);
      }
      adcCount--;
      if (sw_status) {
        if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7) && GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_8)) {
          sw_count ++;
          if (sw_count >= 5) {
            sw_status = 0;
            sw_count = 0;
          }
        } else {
          sw_count = 0;
        }
      } else {
        if ((! GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7)) || (! GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_8))) {
          sw_count ++;
          if (sw_count >= 5) {
            if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_6)) {
              if (! GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_8)) {
                Command_index = 1;
              } else {
                Command_index = 2;
              }
            }
            sw_count = 0;
          }
        } else {
          sw_count = 0;
        }
      } 
    } else {
//      WavePlayerStop();
      WaveDataLength = 0;
//      RepeatState =0;
      break;
    }
  }
  while(ring_buffer_p_in != ring_buffer_p_out_old) {
   if (ring_buffer_p_out_old != ring_buffer_p_out) {
     play((uint32_t)ring_buffer[ring_buffer_p_out], _MAX_SS);
     ring_buffer_p_out_old = ring_buffer_p_out;
    }
  }
//  RepeatState = 0;
  AudioPlayStart = 0;
//  WavePlayerStop();
}

uint32_t AudioTotalSize = 0xFFFF; /* This variable holds the total size of the audio file */
//uint16_t *CurrentPos ;             /* This variable holds the current position of audio pointer */

extern DMA_InitTypeDef DMA_InitStructure; 

void play(uint32_t Addr, uint32_t Size)
{         
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)Addr;
  DMA_InitStructure.DMA_BufferSize = (uint32_t)Size/2;
  DMA_Init(DMA1_Stream7, &DMA_InitStructure);
  DMA_Cmd(DMA1_Stream7, ENABLE);   
  if ((SPI3->I2SCFGR & 0x0400) == 0)
  {
    I2S_Cmd(SPI3, ENABLE);
  }
}

#define DMA_MAX_SZE                    0xFFFF
#define DMA_MAX(x)           (((x) <= DMA_MAX_SZE)? (x):DMA_MAX_SZE)

void DMA1_Stream7_IRQHandler(void)
{
  /* Transfer complete interrupt */
  if (DMA_GetFlagStatus(DMA1_Stream7, DMA_FLAG_TCIF7) != RESET)
  {         
    DMA_Cmd(DMA1_Stream7, DISABLE);   
    DMA_ClearFlag(DMA1_Stream7, DMA_FLAG_TCIF7); 
    if (WaveDataLength) WaveDataLength -= _MAX_SS;
    if (WaveDataLength < _MAX_SS) WaveDataLength = 0;
    ring_buffer_p_out++;
    if (ring_buffer_p_out >= RING_BUF_SIZE) {
      ring_buffer_p_out = 0;
    }
  }
}

