/**
  ******************************************************************************
  * @file    IAP_Main/Inc/ymodem.h 
  * @author  MCD Application Team
  * @version 1.0.0
  * @date    8-April-2015
  * @brief   This file provides all the software function headers of the ymodem.c 
  *          file.
  ******************************************************************************
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __YMODEM_H_
#define __YMODEM_H_

/* Includes ------------------------------------------------------------------*/
#include "global.h"

/* Exported types ------------------------------------------------------------*/
typedef  void (*pFunction)(void);

/**
  * @brief  Comm status structures definition
  */
typedef enum
{
  COM_OK       = 0x00,//成功
  COM_ERROR    = 0x01,//失败
  COM_ABORT    = 0x02,//用户中止
  COM_TIMEOUT  = 0x03,//超时
  COM_DATA     = 0x04,//数据区
  COM_LIMIT    = 0x05,//缓存失败
	COM_UPERR    = 0x06//上位机异常
} COM_StatusTypeDef;
/**
  * @}
  */
typedef enum
{
    YM_OK           = 0,
    YM_TIMEOUT      =-1,
    YM_U_ABOUT      =-2,
    YM_S_ABOUT      =-3,
    YM_CRC_ERR      =-4,
    YM_MAX_ERR      =-5,
    YM_SAVE_ERR     =-6,
    YM_LINK_ERR     =-7,
    YM_ERR          =-8,
    YM_OVER_SIZE    =-9,
    YM_ERR_SIZE     =-10,
    YM_NACK         =-11,
    YM_END          =-12
}YM_RES_CODE;



typedef struct 
{
	uint32_t magicName;//镜像名称
	uint32_t dataCRC;//数据的CRC
	uint32_t dataLen;//数据长度
	uint32_t dataLoadAddr;//数据地址
	uint32_t entryPointAddr;//镜像载入地址
} AppHeader_t;

typedef struct 
{
	uint16_t lastCrc;//上一段数据的CRC
	uint16_t nowCrc;//现阶段CRC
	uint16_t binCrc;//文件总CRC
	uint16_t crcLen;//crc长度
} CrcCheck;

/* Exported constants --------------------------------------------------------*/
#define LEN_OF_APPHEADER					( sizeof(AppHeader_t) )//app头总大小
#define NUM_OF_APPHEADER_MEMBER		( sizeof(AppHeader_t) / sizeof(int) )//app头的成员个数
	
/* Packet structure defines */
#define PACKET_HEADER_SIZE      ((uint32_t)3)//数据包头的长度
#define PACKET_START_INDEX      ((uint32_t)1)//数据包的起始标志下标
#define PACKET_NUMBER_INDEX     ((uint32_t)2)//数据包的长度标志下标
#define PACKET_CNUMBER_INDEX    ((uint32_t)3)//数据包的长度标志下标
#define PACKET_DATA_INDEX       ((uint32_t)4)//数据区的下标
#define PACKET_TRAILER_SIZE     ((uint32_t)2)//数据包的包尾下标
#define PACKET_OVERHEAD_SIZE    (PACKET_HEADER_SIZE + PACKET_TRAILER_SIZE - 1)
//#define PACKET_OVERHEAD_SIZE    (PACKET_HEADER_SIZE + PACKET_TRAILER_SIZE)
#define PACKET_SIZE             ((uint32_t)128)
#define PACKET_1K_SIZE          ((uint32_t)1024)

#define ABORT_LEN	2
#define EOT_LEN	0

/* /-------- Packet in IAP memory ------------------------------------------\
 * | 0      |  1    |  2     |  3   |  4      | ... | n+4     | n+5  | n+6  | 
 * |------------------------------------------------------------------------|
 * | unused | start | number | !num | data[0] | ... | data[n] | crc0 | crc1 |
 * \------------------------------------------------------------------------/
 * the first byte is left unused for memory alignment reasons                 */

#define FILE_NAME_LENGTH        ((uint32_t)64)//文件长度的限制
#define FILE_SIZE_LENGTH        ((uint32_t)16)//文件大小转换为字符串长度后的长度限制，如123转为“123”

#define SOH                     ((uint8_t)0x01)  /* start of 128-byte data packet */
#define STX                     ((uint8_t)0x02)  /* start of 1024-byte data packet */
#define EOT                     ((uint8_t)0x04)  /* end of transmission */
#define ACK                     ((uint8_t)0x06)  /* acknowledge */
#define NAK                     ((uint8_t)0x15)  /* negative acknowledge */
#define CA                      ((uint32_t)0x18) /* two of these in succession aborts transfer */
#define REQ                     (0x43)  /* 'C' == 0x43, request 16-bit CRC */
#define NEGATIVE_BYTE           ((uint8_t)0xFF)

#define ABORT1                  ((uint8_t)0x41)  /* 'A' == 0x41, abort by user */
#define ABORT2                  ((uint8_t)0x61)  /* 'a' == 0x61, abort by user */

#define NAK_TIMEOUT             ((uint32_t)1000)//0x100000
#define DOWNLOAD_TIMEOUT        ((uint32_t)1000) /* One second retry delay */
#define MAX_ERRORS              ((uint32_t)45)//((uint32_t)0xff)
#define PACKET_TIMEOUT          ((uint32_t)1000) /* One second retry delay */
#define UPDATE_ASK_TIMEOUT			((uint32_t)100*1000)	/* 连续请求的超时时间，10秒 */
#define MAX_UPDATE_ASK_NUM      (UPDATE_ASK_TIMEOUT/DOWNLOAD_TIMEOUT - 1)

/* Exported functions ------------------------------------------------------- */
COM_StatusTypeDef Ymodem_Receive(uint32_t *p_size);
void SerialDownload(void);

#endif  /* __YMODEM_H_ */

/*******************(C)COPYRIGHT STMicroelectronics ********END OF FILE********/
