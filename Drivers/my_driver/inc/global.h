/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef GLOBAL_H
#define GLOBAL_H

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "main.h"

#define RS485_TX_PIN			GPIO_PIN_2//串口2的发送引脚
#define RS485_RX_PIN			GPIO_PIN_3//串口2的接收引脚
#define RS485_DIR_PIN		GPIO_PIN_12//485总线方向的控制引脚
#define RS485_PORT	GPIOA//引脚的端口
#define UART1_BAUT_RATE	57600//通讯口波特率57600
#define UART2_BAUT_RATE	115200//调试口波特率115200

#define LED_APP_PIN	GPIO_PIN_14//程序运行状态LED引脚
#define LED_APP_PORT	GPIOB//引脚的端口

#define CFG_SIZE	256
/* Exported defines ----------------------------------------------------------*/
/* APP 升级请求 */
#define APP_UPGREQ_IS_VALID			0x214D4478		/* APP升级请求标志：!MDx */
#define APP_JUMP_TO_APP				0x214A554D		/* 跳转到APP标志：!JUM */
/* APP起始地址是固定的 */
#define APPLICATION_ADDRESS_IS_FIXED

#if FLASH_IS_32K
/* APP 头信息 */
#define APPHEADER_ADDRESS				ADDR_FLASH_PAGE_11//配置区域地址
#define APP_UPGRADE_ADDRESS				ADDR_FLASH_PAGE_12//升级标志区域地址
#define APPHEADER_SIZE					(APPLICATION_ADDRESS - APPHEADER_ADDRESS)				/* Size of page : 1 Kbytes */
/* Define the address from where user application will be loaded. */
#define APPLICATION_ADDRESS     ADDR_FLASH_PAGE_13			/* Start user code address: ADDR_FLASH_PAGE_12 */
/* 计算APP区每页/扇区的大小 */
#define APP_FLASH_STEP          (ADDR_FLASH_PAGE_12 - ADDR_FLASH_PAGE_11)				/* Size of page : 1 Kbytes */
/* End of the Flash address */
#define USER_FLASH_END_ADDRESS     (ADDR_FLASH_PAGE_31 + APP_FLASH_STEP - 1)


#elif FLASH_IS_64K
/* APP 头信息 */
#define APPHEADER_ADDRESS				ADDR_FLASH_PAGE_11//配置区域地址
#define APP_UPGRADE_ADDRESS				ADDR_FLASH_PAGE_12//升级标志区域地址
#define APPHEADER_SIZE					(APPLICATION_ADDRESS - APPHEADER_ADDRESS)				/* Size of page : 1 Kbytes */
/* Define the address from where user application will be loaded. */
#define APPLICATION_ADDRESS     ADDR_FLASH_PAGE_13			/* Start user code address: ADDR_FLASH_PAGE_13 */
/* 计算APP区每页/扇区的大小 */
#define APP_FLASH_STEP          (ADDR_FLASH_PAGE_12 - ADDR_FLASH_PAGE_11)				/* Size of page : 1 Kbytes */
/* End of the Flash address */
#define USER_FLASH_END_ADDRESS     (ADDR_FLASH_PAGE_59 + APP_FLASH_STEP - 1)            /* Page 60~64 for database */


#elif FLASH_IS_1024K
/* APP 头信息 */
#define APPHEADER_ADDRESS       ADDR_FLASH_SECTOR_4
#define APPHEADER_SIZE					(APPLICATION_ADDRESS - APPHEADER_ADDRESS)           /* Size of page : 64 Kbytes */
/* Define the address from where user application will be loaded. */
#define APPLICATION_ADDRESS			ADDR_FLASH_SECTOR_5
/* 计算APP区每页/扇区的大小 */
#define APP_FLASH_STEP          (ADDR_FLASH_SECTOR_6 - ADDR_FLASH_SECTOR_5)				/* Size of page : 128 Kbytes */
/* End of the Flash address */
#define USER_FLASH_END_ADDRESS        (ADDR_FLASH_SECTOR_11 + APP_FLASH_STEP - 1)	/* End @ of user Flash area : sector start address + sector size -1 */

#endif

/* Exported macro ------------------------------------------------------------*/
#define IS_CAP_LETTER(c)    (((c) >= 'A') && ((c) <= 'F'))
#define IS_LC_LETTER(c)     (((c) >= 'a') && ((c) <= 'f'))
#define IS_09(c)            (((c) >= '0') && ((c) <= '9'))
#define ISVALIDHEX(c)       (IS_CAP_LETTER(c) || IS_LC_LETTER(c) || IS_09(c))
#define ISVALIDDEC(c)       IS_09(c)
#define CONVERTDEC(c)       (c - '0')

#define CONVERTHEX_ALPHA(c) (IS_CAP_LETTER(c) ? ((c) - 'A'+10) : ((c) - 'a'+10))
#define CONVERTHEX(c)       (IS_09(c) ? ((c) - '0') : CONVERTHEX_ALPHA(c))

/** 长整型大小端互换 **/
#define BigLittleSwap32(A)  ((((uint32_t)(A) & 0xff000000) >> 24) | \
    (((uint32_t)(A) & 0x00ff0000) >> 8) | \
    (((uint32_t)(A) & 0x0000ff00) << 8) | \
    (((uint32_t)(A) & 0x000000ff) << 24))
		
/* ABSoulute value */
#define ABS_RETURN(x,y)               ((x) < (y)) ? ((y)-(x)) : ((x)-(y))

/* Exported types ------------------------------------------------------------*/
typedef struct {
    uint16_t crc;
} CRC16_CTX;
/* Exported constants --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void Int2Str(uint8_t *p_str, uint32_t intnum);
uint32_t Str2Int(uint8_t *p_inputstr, uint32_t *p_intnum);
uint16_t CRC16_CCITT(const uint8_t *data, uint32_t len, uint16_t oldCRC16);

void CRC16_Init(CRC16_CTX *ctx);
void CRC16_Update(CRC16_CTX *ctx, const uint8_t *data, uint16_t len);
void CRC16_Final(CRC16_CTX *ctx, uint8_t *md);
#endif
