#include "integer.h"
#include "math.h"

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

#ifdef MP3
# define SIZEOF_INT 4
# define SIZEOF_LONG 4
# define SIZEOF_LONG_LONG 8

#include "version.h"
#include "fixed.h"
#include "bit.h"
#include "timer.h"
#include "stream.h"
#include "frame.h"
#include "synth.h"
#include "decoder.h"
#define MAX_BUFF 2304 // <= 1152 * 2
#endif

DWORD acc_size;
WORD acc_files, acc_dirs;
FILINFO Finfo;
DIR  dir;
FATFS Fatfs[_VOLUMES];
FIL fileR;
__IO uint8_t Command_index = 0;

UINT sMUSIC; // Music ファイルの数

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
#endif

#ifdef USE_PRINTF
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
#ifndef MP3
  printf("_MAX_SS : %d\n",_MAX_SS);
#else
  printf("MAX_BUFF : %d\n",MAX_BUFF);
#endif
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
    sMUSIC = 0;
    for(;;) {
      res = f_readdir(& dir, &Finfo);
      if ((res != FR_OK) || !Finfo.fname[0]) break;
      if (Finfo.fattrib & AM_DIR) {
        s2++;
      } else {
        s1++; p1 += Finfo.fsize;
        if (isEx(&(Finfo.fname[0]),"WAV") || isEx(&(Finfo.fname[0]),"MP3")) {
          sMUSIC++;
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
      printf(" %s\n", Lfname);
#else
      cputs("\r\n");
#endif
#endif
    }
#ifdef USE_PRINTF
    printf("%4u File(s),%10lu bytes total\n%4u  dir(s)", s1, p1, s2);
#endif
    res = f_getfree("", (DWORD*)&p1, &fs);
    if (res == FR_OK) {
#ifdef USE_PRINTF
      printf(", %10lu bytes free\n", p1 * fs->csize * 512);
#endif
    } else {
      put_rc(res);
    }
#ifdef USE_PRINTF
    printf("WAVE File %u \n", sMUSIC);
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
    sMUSIC = 0;
    for(;;) {
      res = f_readdir(& dir, &Finfo);
      if ((res != FR_OK) || !Finfo.fname[0]) break;
      if (Finfo.fattrib & AM_DIR) {
        s2++;
      } else {
        s1++; p1 += Finfo.fsize;
        if (isEx(&(Finfo.fname[0]),"WAV") || isEx(&(Finfo.fname[0]),"MP3")) {
          if (n == sMUSIC) {
            sdio_play(&(Finfo.fname[0]));
          }
          sMUSIC++;
        }
      }
    }
  }
}

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
 #define SMALL_BUFF_SIZE 512
 uint8_t small_buffer[SMALL_BUFF_SIZE] ={0x00};
 int small_buffer_pointer = -1;
#ifndef MP3
 uint16_t buffer[_MAX_SS] ={0x00};
 #define RING_BUF_SIZE 4
 uint16_t ring_buffer[RING_BUF_SIZE][_MAX_SS];
 volatile uint8_t ring_buffer_p_in  = 0;
 volatile uint8_t ring_buffer_p_out = 0;
#else
 uint16_t buffer[MAX_BUFF] ={0x00};
 #define RING_BUF_SIZE 8
 uint16_t ring_buffer[RING_BUF_SIZE][MAX_BUFF];
 volatile uint8_t ring_buffer_p_in  = 0;
 volatile uint8_t ring_buffer_p_out = 0;
 int ring_buffer_sub_p = 0;
#endif
 static __IO uint32_t SpeechDataOffset = 0x00;
 UINT BytesRead;
 WAVE_FormatTypeDef WAVE_Format;
 extern FIL fileR;
 extern DIR dir;
 void WavePlayBack(uint32_t AudioFreq,uint32_t AudioBit);
 int i2s_clockset(uint32_t AudioFreq,uint32_t AudioBit);

#ifdef MP3
 int oldFrameSize = 0;
 void Mp3PlayBack(uint32_t AudioFreq);
 uint32_t  mp3SampleRate;
 uint8_t  mp3FlgFirst;
 uint16_t  mp3i;
#endif

__IO uint32_t WaveCounter;
__IO uint32_t WaveDataLength = 0;
static __IO uint32_t TimingDelay;
  int sw_status;
  int sw_count;

 static ErrorCode WavePlayer_WaveParsing(uint32_t *FileLen);
#ifdef MP3
 static ErrorCode Mp3Player_Mp3Parsing(uint32_t *FileLen);
#endif
uint32_t ReadUnit(uint8_t *buffer, uint8_t idx, uint8_t NbrOfBytes, Endianness BytesFormat);

#ifndef NO_ADC
  int adcCount;
  int lowVoltage;
  unsigned int adc1w,adc2w;
#endif

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
      f_read (&fileR, buffer, 512, &BytesRead);
      {
#ifdef USE_PRINTF
         int i,j;
         unsigned char w;
         for(i = 0; i < 256 ; i += 16) {
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
        printf("Sample Rate : %0.1f kHz %d bits\n",(double)WAVE_Format.SampleRate/1000.0,WAVE_Format.BitsPerSample);
        printf("Byte Rate : %d\n",WAVE_Format.ByteRate);
        printf("Time : %0.1f sec\n",((double)WaveDataLength) / ((double)WAVE_Format.ByteRate));
#endif
        /* Play the wave */
        WavePlayBack(WAVE_Format.SampleRate,WAVE_Format.BitsPerSample);
      } else {
#ifdef MP3
        WaveFileStatus = Mp3Player_Mp3Parsing(&wavelen);
        /* Unvalid wave file */
        if (WaveFileStatus == Valid_WAVE_File) {
#ifdef USE_PRINTF
          /* the .MP3 file is valid */
          printf("the .MP3 file %s is valid\r\n",filename);
          /* Set WaveDataLenght to the Speech wave length */
#endif
          Mp3PlayBack(mp3SampleRate);
          return;
        }
#ifdef USE_PRINTF
        int i,j,k;
        k = 512;
        while (k < 512) {
          f_read (&fileR, buffer, 512, &BytesRead);
          unsigned char w;
          for(i = 0; i < 256; i += 16) {
            printf("%04x ",k + i);
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
          k += 512;
        }
#endif
#ifdef USE_PRINTF
        cputs("Unvalid wave file\r\n");
#endif
#endif
      }
    }    
  }
}

int SetSmallBuffer(unsigned long p,int i)
{
  int new_pointer = small_buffer_pointer;
  if (small_buffer_pointer == -1) {
    new_pointer = p;
  } else {
    if ((p + i - small_buffer_pointer) > SMALL_BUFF_SIZE) {
      new_pointer = p;
    }
  }
  if (small_buffer_pointer != new_pointer) {
    small_buffer_pointer = new_pointer;
    f_lseek(&fileR, small_buffer_pointer);
    f_read (&fileR, small_buffer, _MAX_SS, &BytesRead) ;
  }
}

int CmpSmallBuffer(unsigned long p,char *b)
{
  int i = 0;
  while(b[i]) {
    i++;
  }
  i--;
  SetSmallBuffer(p,i);
  i = 0;
  while(b[i]) {
    if (small_buffer[p + i - small_buffer_pointer] != b[i])
      return 0;
    i++;
  }
  return 1;
}

int CmpSmallChunkList(unsigned long p)
{
  if (CmpSmallBuffer(p,"str2")) return 1;
  if (CmpSmallBuffer(p,"bmrk")) return 1;
  if (CmpSmallBuffer(p,"dtbt")) return 1;
  if (CmpSmallBuffer(p,"acid")) return 1;
  if (CmpSmallBuffer(p,"strc")) return 1;
  if (CmpSmallBuffer(p,"LIST")) return 1;
  if (CmpSmallBuffer(p,"ICRD")) return 1;
  if (CmpSmallBuffer(p,"PAD ")) return 1;
  if (CmpSmallBuffer(p,"bext")) return 1;
  if (CmpSmallBuffer(p,"minf")) return 1;
  if (CmpSmallBuffer(p,"elm1")) return 1;
  if (CmpSmallBuffer(p,"JUNK")) return 1;
  return 0;
}

unsigned long ReadSmallBufferULONG(unsigned long p)
{
  unsigned long index = 0;
  unsigned long temp = 0;
  SetSmallBuffer(p,4);
  for (index = 0; index < 4; index++) {
    temp |= small_buffer[p + index - small_buffer_pointer] << (index * 8);
  }
  return temp;
}

unsigned long ReadSmallBufferUSHORT(unsigned long p)
{
  unsigned long index = 0;
  unsigned long temp = 0;
  SetSmallBuffer(p,2);
  for (index = 0; index < 2; index++) {
    temp |= small_buffer[p + index - small_buffer_pointer] << (index * 8);
  }
  return temp;
}

static ErrorCode WavePlayer_WaveParsing(uint32_t *FileLen)
{
  uint32_t temp = 0x00;
  small_buffer_pointer = -1;
  if (! CmpSmallBuffer(0,"RIFF")) {
    return(Unvalid_RIFF_ID);
  }
  WAVE_Format.RIFFchunksize = ReadSmallBufferULONG(4);
  if (! CmpSmallBuffer(8,"WAVE")) {
    return(Unvalid_WAVE_Format);
  }
  int p = 12;
  while (CmpSmallChunkList(p)) {
    p += 4;
    temp = ReadSmallBufferULONG(p) + 4;
    p += temp;
  }
  if (! CmpSmallBuffer(p,"fmt ")) {
    SetSmallBuffer(p,4);
    printf("Error Chunk_ID is \"%c%c%c%c\"\n",small_buffer[p - small_buffer_pointer],small_buffer[p - small_buffer_pointer + 1],small_buffer[p - small_buffer_pointer + 2],small_buffer[p - small_buffer_pointer + 3]);
    return(Unvalid_FactChunk_ID);
  } else {
    temp = ReadSmallBufferULONG(p + 4);
  }
  WAVE_Format.FormatTag = ReadSmallBufferUSHORT(p + 8);
  if (WAVE_Format.FormatTag != 0x01) {
      return(Unsupporetd_FormatTag);
  }
  WAVE_Format.NumChannels = ReadSmallBufferUSHORT(p + 10);
  WAVE_Format.SampleRate = ReadSmallBufferULONG(p + 12);
  WAVE_Format.ByteRate = ReadSmallBufferULONG(p + 16);
  WAVE_Format.BlockAlign = ReadSmallBufferUSHORT(p + 20);
  WAVE_Format.BitsPerSample = ReadSmallBufferUSHORT(p + 22);
  if ((WAVE_Format.BitsPerSample !=16) && (WAVE_Format.BitsPerSample !=24)) {
    return(Unsupporetd_Bits_Per_Sample);
  }
  p += (temp + 8);
  while (CmpSmallChunkList(p)) {
    p += 4;
    temp = ReadSmallBufferULONG(p) + 4;
    p += temp;
  }
  if (! CmpSmallBuffer(p,"data")) {
    SetSmallBuffer(p,4);
    printf("Unvalid_DataChunk_ID is \"%c%c%c%c\"\n",small_buffer[p - small_buffer_pointer],small_buffer[p - small_buffer_pointer + 1],small_buffer[p - small_buffer_pointer + 2],small_buffer[p - small_buffer_pointer + 3]);
    return(Unvalid_DataChunk_ID);
  }
  SpeechDataOffset = p;
  SpeechDataOffset += 4;
  WAVE_Format.DataSize = ReadSmallBufferULONG(SpeechDataOffset);
  SpeechDataOffset += 4;
  WaveCounter =  SpeechDataOffset;
  return(Valid_WAVE_File);
}

#ifdef MP3
static ErrorCode Mp3Player_Mp3Parsing(uint32_t *FileLen)
{
  uint8_t* buf;
  buf = (uint8_t*)buffer;
  SpeechDataOffset = 0;
  if ((buf[0] == 'I') && (buf[1] == 'D') && (buf[2] == '3')) {
    SpeechDataOffset = 10;
    SpeechDataOffset += (buf[6] << 21);
    SpeechDataOffset += (buf[7] << 14);
    SpeechDataOffset += (buf[8] << 7);
    SpeechDataOffset += (buf[9]);
#ifdef USE_PRINTF
    printf("ID3 ver2.x HEADER\n");
    printf("offset :%ld\n",SpeechDataOffset);
#endif
  }
  f_lseek(&fileR, SpeechDataOffset);
  f_read (&fileR, buf, 512, &BytesRead); 
  if (BytesRead <= 0) {
#ifdef USE_PRINTF
    printf("ファイルがロードできません。\n");
#endif
    return(Unvalid_DataChunk_ID);
  }
  if ((buf[0] != 0xff) && ((buf[1] & 0xf0) != 0xf0))
    return(Unvalid_DataChunk_ID);
#ifdef USE_PRINTF
  if (buf[1] & 0x08) {
    printf("MPEG-1 ");
  } else {
    printf("MPEG-2 ");
  }
  switch (buf[1] & 0x06) {
    case 2:
      printf("Layer3 ");
      break;
    case 4:
      printf("Layer2 ");
      break;
    case 6:
      printf("Layer1 ");
      break;
  }
  if (buf[1] & 0x01) {
    printf("ERRbit\n");
  } else {
    printf("NoERRbit\n");
  }
  if (buf[1] & 0x08) {
    switch (buf[2] & 0xf0) {
      case 0x00:
        printf("FreeFormat ");
        break;
      case 0x10:
        printf("32kbps ");
        break;
      case 0x20:
        printf("40kbps ");
        break;
      case 0x30:
        printf("48kbps ");
        break;
      case 0x40:
        printf("56kbps ");
        break;
      case 0x50:
        printf("64kbps ");
        break;
      case 0x60:
        printf("80kbps ");
        break;
      case 0x70:
        printf("96kbps ");
        break;
      case 0x80:
        printf("112kbps ");
        break;
      case 0x90:
        printf("128kbps ");
        break;
      case 0xa0:
        printf("160kbps ");
        break;
      case 0xb0:
        printf("192kbps ");
        break;
      case 0xc0:
        printf("224kbps ");
        break;
      case 0xd0:
        printf("256kbps ");
        break;
      case 0xe0:
        printf("320kbps ");
        break;
      default:
        printf("%02x\n",buf[2] & 0xf0);
        break;
    }
  } else {
    switch (buf[2] & 0xf0) {
      case 0x00:
        printf("FreeFormat ");
        break;
      case 0x10:
        printf("8kbps ");
        break;
      case 0x20:
        printf("16kbps ");
        break;
      case 0x30:
        printf("24kbps ");
        break;
      case 0x40:
        printf("32kbps ");
        break;
      case 0x50:
        printf("40kbps ");
        break;
      case 0x60:
        printf("48kbps ");
        break;
      case 0x70:
        printf("56kbps ");
        break;
      case 0x80:
        printf("64kbps ");
        break;
      case 0x90:
        printf("80kbps ");
        break;
      case 0xa0:
        printf("96kbps ");
        break;
      case 0xb0:
//        printf("64kbps ??");
        printf("112kbps ??");
        break;
      case 0xc0:
        printf("128kbps ");
        break;
      case 0xd0:
        printf("144kbps ");
        break;
      case 0xe0:
        printf("160kbps ");
        break;
      default:
        printf("%02x\n",buf[2] & 0xf0);
        break;
    }
  }
  if (buf[1] & 0x08) {
    switch (buf[2] & 0x0c) {
      case 0x00:
        printf("44.1kHz ");
        mp3SampleRate = 44100;
        break;
      case 0x04:
        printf("48kHz ");
        mp3SampleRate = 48000;
        break;
      case 0x08:
        printf("32kHz ");
        mp3SampleRate = 32000;
        break;
    }
  } else {
    switch (buf[2] & 0x0c) {
      case 0x00:
        printf("22.05kHz ");
        mp3SampleRate = 22050;
        break;
      case 0x04:
        printf("24kHz ");
        mp3SampleRate = 24000;
        break;
      case 0x08:
        printf("16kHz ");
        mp3SampleRate = 16000;
        break;
    }
  }
  printf("\n");
#else
  if (buf[1] & 0x08) {
    switch (buf[2] & 0x0c) {
      case 0x00:
        mp3SampleRate = 44100;
        break;
      case 0x04:
        mp3SampleRate = 48000;
        break;
      case 0x08:
        mp3SampleRate = 32000;
        break;
    }
  } else {
    switch (buf[2] & 0x0c) {
      case 0x00:
        mp3SampleRate = 22050;
        break;
      case 0x04:
        mp3SampleRate = 24000;
        break;
      case 0x08:
        mp3SampleRate = 16000;
        break;
    }
  }
#endif
  return(Valid_WAVE_File);
}
#endif

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

void setDAC(uint32_t AudioFreq,uint32_t AudioBit)
{
  uint16_t work = 0;
  int i;
  work |= 0x1000;		// MODE2
  if (AudioBit == 24)
    work |= 0x0080;		// 24bit
  if (AudioFreq == 44100)
    work |= 0x0001;		// 44.1kHz
  else if (AudioFreq == 48000)
    work |= 0x0002;		// 48kHz
  else
    work |= 0x0003;		// ect
  for (i = 0;i < 16;i++) {
    GPIO_ResetBits(GPIOC,GPIO_Pin_4); // MC <= 0
    if (work & 0x8000) {
      GPIO_SetBits(GPIOC,GPIO_Pin_5); // MD <= 1
    } else {
      GPIO_ResetBits(GPIOC,GPIO_Pin_5); // MD <= 0

    }
    GPIO_SetBits(GPIOC,GPIO_Pin_4); // MC <= 1
    work = work << 1;
  }
  GPIO_ResetBits(GPIOC,GPIO_Pin_4); // MC <= 0
  GPIO_ResetBits(GPIOC,GPIO_Pin_6); // ML <= 0
  GPIO_SetBits(GPIOC,GPIO_Pin_6);   // ML <= 1
}

void play(uint32_t Addr, uint32_t Size);
uint32_t old_bit = 16;

void WavePlayBack(uint32_t AudioFreq,uint32_t AudioBit)
{
#ifndef W96K
  if (AudioFreq == 96000) return;
#endif
  if (! i2s_clockset(AudioFreq,AudioBit)) return;
  setDAC(AudioFreq,AudioBit);

  /* Start playing */
  Command_index = 0;
  sw_status = 1;
  sw_count = 0;
#ifndef NO_ADC
  adcCount = 0;
  lowVoltage = 0;
#endif
  /* Get Data from SD CARD */
  f_lseek(&fileR, WaveCounter);
  ring_buffer_p_in  = 0;
  ring_buffer_p_out = 0;
#ifdef MP3
  ring_buffer_sub_p = 0;
#endif
  while (ring_buffer_p_in < RING_BUF_SIZE) {
#ifndef MP3
    if (old_bit == 16) {
      f_read (&fileR, ring_buffer[ring_buffer_p_in], _MAX_SS * 2, &BytesRead);
    } else {
      uint8_t* buf;
      buf = (uint8_t*)buffer;
      f_read (&fileR, buffer, _MAX_SS * 3 / 2, &BytesRead); 
      uint8_t* b;
      b = (uint8_t*)(ring_buffer[ring_buffer_p_in]);
      int j = 0;
      for (int i = 0;i < _MAX_SS * 2;i += 4) {
        b[i+3] = buf[j++]; // 7-0
        b[i+0] = buf[j++]; // 15-8

        b[i+1] = buf[j++]; // 23-16
        b[i+2] = 0;
      }
    }
#else
    if (old_bit == 16) {
      f_read (&fileR, ring_buffer[ring_buffer_p_in], MAX_BUFF * 2, &BytesRead);
    } else {
      uint8_t* buf;
      buf = (uint8_t*)buffer;
      f_read (&fileR, buffer, MAX_BUFF * 3 / 2, &BytesRead); 
      uint8_t* b;
      b = (uint8_t*)(ring_buffer[ring_buffer_p_in]);
      int j = 0;
      for (int i = 0;i < MAX_BUFF * 2;i += 4) {
        b[i+3] = buf[j++]; // 7-0
        b[i+0] = buf[j++]; // 15-8
        b[i+1] = buf[j++]; // 23-16
        b[i+2] = 0;
      }
    }
#endif 
    ring_buffer_p_in++;
  }
  ring_buffer_p_in  = 0;
  /* Start playing wave */
#ifndef MP3
  play((uint32_t)ring_buffer[0], _MAX_SS * 2);
#else
  play((uint32_t)ring_buffer[0], MAX_BUFF * 2);
#endif
  ring_buffer_p_in  = 0;
  ring_buffer_p_out = 0;
/* 2013.08.07 曲の終わりのジャっと言う音の対策 未だ完全ではない */
#ifndef MP3
  if (WaveDataLength < 2 * _MAX_SS * RING_BUF_SIZE) {
    WaveDataLength = 0;
  } else {
    WaveDataLength -= 2 * _MAX_SS * RING_BUF_SIZE;
  }
#else
  if(WaveDataLength < 2 * MAX_BUFF * RING_BUF_SIZE) {
    WaveDataLength = 0;
  } else {
    WaveDataLength -= 2 * MAX_BUFF * RING_BUF_SIZE;
  }
#endif   
  while(WaveDataLength != 0)
  { 
    /* Test on the command: Playing */
    if (Command_index == 0)
    { 
      /* wait for DMA transfert complete */
      while (ring_buffer_p_in == ring_buffer_p_out);
#ifndef MP3
      if (old_bit == 16) {
        f_read (&fileR, ring_buffer[ring_buffer_p_in], _MAX_SS * 2, &BytesRead);
      } else {
        uint8_t* buf;
        buf = (uint8_t*)buffer;
        f_read (&fileR, buffer, _MAX_SS * 3 / 2, &BytesRead); 
        uint8_t* b;
        b = (uint8_t*)(ring_buffer[ring_buffer_p_in]);
        int j = 0;
        for (int i = 0;i < _MAX_SS * 2;i += 4) {
          b[i+3] = buf[j++]; // 7-0
          b[i+0] = buf[j++]; // 15-8
          b[i+1] = buf[j++]; // 23-16
          b[i+2] = 0;
        }
      }
#else
      if (old_bit == 16) {
        f_read (&fileR, ring_buffer[ring_buffer_p_in], MAX_BUFF * 2, &BytesRead);
      } else {
        uint8_t* buf;
        buf = (uint8_t*)buffer;
        f_read (&fileR, buffer, MAX_BUFF * 3 / 2, &BytesRead); 
        uint8_t* b;
        b = (uint8_t*)(ring_buffer[ring_buffer_p_in]);
        int j = 0;
        for (int i = 0;i < MAX_BUFF * 2;i += 4) {
          b[i+3] = buf[j++]; // 7-0
          b[i+0] = buf[j++]; // 15-8
          b[i+1] = buf[j++]; // 23-16
          b[i+2] = 0;
        }
      }
#endif
      ring_buffer_p_in++;
      if (ring_buffer_p_in >= RING_BUF_SIZE) {
        ring_buffer_p_in = 0;
      }
#ifndef NO_ADC
      if (adcCount == 0) {
        if (AudioFreq <= 48000) {
          if (AudioBit == 16) {
            adcCount = 100;
          } else {
            adcCount = 150;
          }
        } else {
          if (AudioBit == 16) {
            adcCount = 200;
          } else {
            adcCount = 300;
          }
        }
        while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
        while(ADC_GetFlagStatus(ADC2, ADC_FLAG_EOC) == RESET);
        adc1w = ADC_GetConversionValue(ADC1) & 0x0fff;
        adc2w = ADC_GetConversionValue(ADC2) & 0x0fff;
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
#endif
      if (sw_status) {
        if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13) &&
            GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14)) {
          sw_count ++;
          if (sw_count >= 5) {
            sw_status = 0;
            sw_count = 0;
          }
        } else {
          sw_count = 0;
        }
      } else {
        if ((! GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13)) || (! GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14))) {
          sw_count ++;
          if (sw_count >= 5) {
            if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12)) {
              if (! GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14)) {
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
      WaveDataLength = 0;
      break;
    }
  }
  while(ring_buffer_p_in != ring_buffer_p_out) {};
}

uint32_t AudioTotalSize = 0xFFFF; /* This variable holds the total size of the audio file */

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

void DMA1_Stream7_IRQHandler(void)
{
  /* Transfer complete interrupt */
  if (DMA_GetFlagStatus(DMA1_Stream7, DMA_FLAG_TCIF7) != RESET)
  {         
    DMA_Cmd(DMA1_Stream7, DISABLE);   
    DMA_ClearFlag(DMA1_Stream7, DMA_FLAG_TCIF7); 
    ring_buffer_p_out++;
    if (ring_buffer_p_out >= RING_BUF_SIZE) {
      ring_buffer_p_out = 0;
    }
    if (ring_buffer_p_in != ring_buffer_p_out) {
#ifndef MP3
      play((uint32_t)ring_buffer[ring_buffer_p_out], _MAX_SS * 2);
#else
      play((uint32_t)ring_buffer[ring_buffer_p_out], MAX_BUFF * 2);
#endif
    }
    if (old_bit == 16) {
#ifndef MP3
      if (WaveDataLength) WaveDataLength -= (_MAX_SS * 2);
      if (WaveDataLength < (_MAX_SS * 2)) WaveDataLength = 0;
#else
      if (WaveDataLength) WaveDataLength -= (MAX_BUFF * 2);
      if (WaveDataLength < (MAX_BUFF * 2)) WaveDataLength = 0;
#endif
    } else {
#ifndef MP3
      if (WaveDataLength) WaveDataLength -= (_MAX_SS * 3 / 2);
      if (WaveDataLength < (_MAX_SS * 3 / 2)) WaveDataLength = 0;
#else
      if (WaveDataLength) WaveDataLength -= (MAX_BUFF * 3 / 2);
      if (WaveDataLength < (MAX_BUFF * 3 / 2)) WaveDataLength = 0;
#endif
    }
  }
}

#ifdef MP3
static inline
signed int scale(mad_fixed_t sample)
{
  sample += 0x01000;
  if (sample >= 0x10000000)
    sample = 0x10000000 - 1;
  else if (sample < -0x10000000)
    sample = -0x10000000;
  return sample >> 13;
}

int f_read_512(uint8_t * buf,int size)
{
  UINT r,len;
  r = 0;
  while (size > 0) {
    if (size <= 512) {
      f_read (&fileR, buf, size, &len);
      r += len;
      buf += size; 
      size -= size;
    } else {
      f_read (&fileR, buf, 512, &len); 
      r += len;
      buf += 512; 
      size -= 512;
    }
  }
  return r;
}

static
enum mad_flow input(void *data,
		    struct mad_stream *stream)
{
  uint8_t* buf;
  buf = (uint8_t*)buffer;
  int i,n;
  static int work = 0;
  static uint8_t oldB2 = 0;
  if (oldFrameSize == 0) {
    if (f_read_512(buf, 10) < 10) {
      return MAD_FLOW_STOP;
    }
  } else {
    for (i = 0;i < 10;i++) {
      buf[i] = buf[oldFrameSize + i];
    }
  }
  if ((buf[2] & 0xfc) != oldB2) {
    oldB2 = buf[2] & 0xfc;
    if (buf[1] & 0x08) {
      switch (buf[2] & 0xf0) {
        case 0x10:
          work = 32000;
          break;
        case 0x20:
          work = 40000;
          break;
        case 0x30:
          work = 48000;
          break;
        case 0x40:
          work = 56000;
          break;
        case 0x50:
          work = 64000;
          break;
        case 0x60:
          work = 80000;
          break;
        case 0x70:
          work = 96000;
          break;
        case 0x80:
          work = 112000;
          break;
        case 0x90:
          work = 128000;
          break;
        case 0xa0:
          work = 160000;
          break;
        case 0xb0:
          work = 192000;
          break;
        case 0xc0:
          work = 224000;
          break;
        case 0xd0:
          work = 256000;
          break;
        case 0xe0:
          work = 320000;
          break;
      }
    } else {
      switch (buf[2] & 0xf0) {
        case 0x10:
          work = 8000;
          break;
        case 0x20:
          work = 16000;
          break;
        case 0x30:
          work = 24000;
          break;
        case 0x40:
          work = 32000;
          break;
        case 0x50:
          work = 40000;
          break;
        case 0x60:
          work = 48000;
          break;
        case 0x70:
          work = 56000;
          break;
        case 0x80:
          work = 64000;
          break;
        case 0x90:
          work = 80000;
          break;
        case 0xa0:
          work = 96000;
          break;
        case 0xb0:
//          work = 64000; // ??
          work = 112000; // ??
          break;
        case 0xc0:
          work = 128000;
          break;
        case 0xd0:
          work = 144000;
          break;
        case 0xe0:
          work = 160000;
          break;
      }
    }
    if ((buf[1] & 0x06) == 6) {
      switch (buf[2] & 0x0c) {
        case 0x00:
          work = 12 * work / 44100;
          break;
        case 0x04:
          work = 12 * work / 48000;
          break;
        case 0x08:
          work = 12 * work / 32000;
          break;
        default :
          return MAD_FLOW_STOP;
      }
    } else {
      switch (buf[2] & 0x0c) {
        case 0x00:
          work = 144 * work / 44100;
          break;
        case 0x04:
          work = 144 * work / 48000;
          break;
        case 0x08:
          work = 144 * work / 32000;
          break;
        default :
          return MAD_FLOW_STOP;
      }
    }
  }
  if (buf[2] & 0x02) {
    n = work + 1;
  } else {
    n = work;
  }
  if (f_read_512(buf + 10, n) < n) {
    return MAD_FLOW_STOP;
  }
  oldFrameSize = n;
  if (n) {
// なぜか、8バイト余計にするだけで、ちゃんとWAVファイルになる。
    mad_stream_buffer(stream, buf, n+8);
  }
#ifndef NO_ADC
  if (adcCount == 0) {
    adcCount = 100;
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    while(ADC_GetFlagStatus(ADC2, ADC_FLAG_EOC) == RESET);
    adc1w = ADC_GetConversionValue(ADC1) & 0x0fff;
    adc2w = ADC_GetConversionValue(ADC2) & 0x0fff;
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
  if (Command_index == 3)
    return MAD_FLOW_STOP;
#endif
  if (sw_status) {
    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13) &&
        GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14)) {
      sw_count ++;
      if (sw_count >= 5) {
        sw_status = 0;
        sw_count = 0;
      }
    } else {
      sw_count = 0;
    }
  } else {
    if ((! GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13)) || (! GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14))) {
      sw_count ++;
      if (sw_count >= 5) {
        if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12)) {
          if (! GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14)) {
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
  if (Command_index > 0)
    return MAD_FLOW_STOP;
  return MAD_FLOW_CONTINUE;
}

static
enum mad_flow output(void *data,
		     struct mad_header const *header,
		     struct mad_pcm *pcm)
{
  unsigned int nsamples;
  mad_fixed_t const *left_ch, *right_ch;

  nsamples  = pcm->length;
  left_ch   = pcm->samples[0];
  right_ch  = pcm->samples[1];
  unsigned int i;
  while ((! mp3FlgFirst) && (ring_buffer_p_in == ring_buffer_p_out));
  i = 0;
  while (i < 2 * nsamples) {
    ring_buffer[ring_buffer_p_in][ring_buffer_sub_p + i++] = scale(*left_ch++);
    ring_buffer[ring_buffer_p_in][ring_buffer_sub_p + i++] = scale(*right_ch++);
  }
  ring_buffer_sub_p += (2 * nsamples);
  if (ring_buffer_sub_p >= MAX_BUFF) {
    ring_buffer_sub_p = 0;
    ring_buffer_p_in++;
    if (ring_buffer_p_in >= RING_BUF_SIZE) {
      ring_buffer_p_in  = 0;
      if (mp3FlgFirst) {
        play((uint32_t)ring_buffer[0], MAX_BUFF * 2);
        mp3FlgFirst = 0;
      }
    }
  }
  return MAD_FLOW_CONTINUE;
}

static
enum mad_flow error(void *data,
		    struct mad_stream *stream,
		    struct mad_frame *frame)
{
  return MAD_FLOW_CONTINUE;
}

void Mp3PlayBack(uint32_t AudioFreq)
{
  if (! i2s_clockset(AudioFreq,16)) return;
  setDAC(AudioFreq,16);
  /* Start playing */
  Command_index = 0;
  sw_status = 1;
  sw_count = 0;
#ifndef NO_ADC
  adcCount = 0;
  lowVoltage = 0;
#endif
  /* Get Data from SD CARD */
  f_lseek(&fileR, SpeechDataOffset);
  oldFrameSize = 0;
  ring_buffer_p_in  = 0;
  ring_buffer_p_out = 0;
  ring_buffer_sub_p = 0;
  mp3i = 0;
  mp3FlgFirst = 1;

  struct mad_decoder decoder;
  mad_decoder_init(&decoder, 0,
		   input, 0 /* header */, 0 /* filter */, output,
		   error, 0 /* message */);
  mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);
  mad_decoder_finish(&decoder);
  while(ring_buffer_p_in != ring_buffer_p_out) {};
}
#endif

void startI2S(uint32_t AudioFreq,uint32_t AudioBit);

int i2s_clockset(uint32_t AudioFreq,uint32_t AudioBit)
{
/*
  32kHz   16bit PLLI2S_N = 213 PLLI2S_R = 2
  44.1kHz 16bit PLLI2S_N = 271 PLLI2S_R = 6
  48kHz   16bit PLLI2S_N = 258 PLLI2S_R = 3
  96kHz   24bit PLLI2S_N = 344 PLLI2S_R = 2
*/
  static uint32_t old_freq = 44100;
  if ((old_freq == AudioFreq) && (old_bit == AudioBit)) return 1;
  if (!(((AudioBit == 16) && ((AudioFreq == 16000) || (AudioFreq == 22050) || (AudioFreq == 24000) || (AudioFreq == 32000) || (AudioFreq == 44100) || (AudioFreq == 48000))) || ((AudioBit == 24) && ((AudioFreq == 96000))))) return 0;

  old_freq = AudioFreq;
  old_bit = AudioBit;
  uint32_t PLLI2S_N = 271;
  uint32_t PLLI2S_R = 6;
  if (AudioFreq == 48000) {
    PLLI2S_N = 258;
    PLLI2S_R = 3;
  } else if (AudioFreq == 24000) {
    PLLI2S_N = 258;
    PLLI2S_R = 6;
  } else if (AudioFreq == 22050) {
    PLLI2S_N = 271;
    PLLI2S_R = 12;
  } else if (AudioFreq == 32000) {
    PLLI2S_N = 213;
    PLLI2S_R = 2;
  } else if (AudioFreq == 16000) {
    PLLI2S_N = 213;
    PLLI2S_R = 4;
  } else if (AudioFreq == 96000) {
    PLLI2S_N = 344;
    PLLI2S_R = 2;
  }
  /* Disable PLLI2S */
  RCC->CR &= ~((uint32_t)RCC_CR_PLLI2SON);
  /* PLLI2S clock used as I2S clock source */
  RCC->CFGR &= ~RCC_CFGR_I2SSRC;
  /* Configure PLLI2S */
  RCC->PLLI2SCFGR = (PLLI2S_N << 6) | (PLLI2S_R << 28);
  /* Enable PLLI2S */
  RCC->CR |= ((uint32_t)RCC_CR_PLLI2SON);
  /* Wait till PLLI2S is ready */
  while((RCC->CR & RCC_CR_PLLI2SRDY) == 0)
  {
  }
  /* I2Sポートをスタート */
  startI2S(AudioFreq,AudioBit);
  /* IRQの設定とDMAのイネーブル */
  setIRQandDMA();
  return 1;
}

