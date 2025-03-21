/**
  ******************************************************************************
  * @file    IAP_Main/Src/ymodem.c 
  * @author  MCD Application Team
  * @version 2.1.1
  * @date    2021-3-18
  * @brief   This file provides all the software functions related to the ymodem 
  *          protocol.
  ******************************************************************************
  */
  
/* Includes ------------------------------------------------------------------*/
#include "flash_if.h"
#include "ymodem.h"
#include "usart.h"

#define DEBUG_PRINT 0 //串口接收过程不能开启打印
//extern uint32_t cfg;
//extern uint32_t cfg[CFG_SIZE];
uint16_t dataIndex = 0;
CRC16_CTX ctx = { 0 };
uint8_t sum_crc[2] = { 0 };
COM_StatusTypeDef result;
/* Private typedef -----------------------------------------------------------*/
/* 用于记录 Ymodem 传输过程中用到的标志位 */
struct YMtrans_t
{
	/*会话结束标志*/
	uint32_t session_done;
	/*会话开始标志*/
	uint32_t session_begin;
	/*文件接收结束标志*/
	uint32_t file_done;
	/*文件大小*/
	uint32_t filesize;
	/* 用于记录收到数据包的序号 */
	uint8_t packets_received;
	/* 用于记录数据的总包数，防止数据包计数重新开始时出现误判 */
	int32_t num;
	/* 用于记录本次Ymodem传输的模式 */
	int16_t mode;
	/* 用于记录收到 EOT 的次数 */
	uint8_t EOTNum;
	/* 用于记录是否为传输过程中的第一包有效数据，指除了文件名之后的第一包数据 */
	bool is1stData;
	/* 数据在 Flash 中写入的地址 */
	uint32_t flashdes;
	/* 最后一包数据中有效数据长度（剔除填充的0x1A） */
	int16_t finishDataLen;
} YMtrans;
	
/* Private define ------------------------------------------------------------*/
#define CRC16_F       /* activate the CRC16 integrity */
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern AppHeader_t AppHeader;

pFunction JumpToApplication;
uint32_t JumpAddress;
uint32_t FlashProtection = 0;
char aFileName[FILE_NAME_LENGTH];
char aFileSize[FILE_SIZE_LENGTH];

//uint32_t packetCrc =0;
//uint32_t binCrc =0;
//uint32_t newPacketSize =0;

CrcCheck appDataCrc;

/* @note ATTENTION - please keep this variable 32bit alligned */
uint8_t aPacketData[PACKET_1K_SIZE + PACKET_DATA_INDEX + PACKET_TRAILER_SIZE];

/* Private function prototypes -----------------------------------------------*/
static HAL_StatusTypeDef ReceivePacket(uint8_t *p_data, uint32_t *p_length, uint32_t timeout);
uint16_t UpdateCRC16(uint16_t crc_in, uint8_t byte);
uint16_t Cal_CRC16(const uint8_t* p_data, uint32_t size);
//uint8_t CalcChecksum(const uint8_t *p_data, uint32_t size);

static COM_StatusTypeDef DealYmodemData(uint32_t dataLen, uint32_t *p_size);
static bool DealYmodemZeroPacket(void);
static COM_StatusTypeDef DealYmodem1stPacket(uint32_t dataLen, uint32_t srcAddr);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Receive a packet from sender
  * @param  data
  * @param  length
  *     0: end of transmission
  *     2: abort by sender
  *    >0: packet length
  * @param  timeout
  * @retval HAL_OK: normally return
  *         HAL_BUSY: abort by user
  */
static HAL_StatusTypeDef ReceivePacket(uint8_t *p_data, uint32_t *p_length, uint32_t timeout)
{
  uint16_t crc;
  uint16_t packet_size = 0;
  HAL_StatusTypeDef status ;
	uint8_t char1;
//	uint16_t index =0;
  *p_length = 0;
	
	status = HAL_UART_Receive(&huart1, &char1, 1,DOWNLOAD_TIMEOUT);
  if (status == HAL_OK) {
    switch (char1) {
      case SOH:
        packet_size = PACKET_SIZE;
        break;
      case STX:
        packet_size = PACKET_1K_SIZE;
        break;
      case EOT:
        break;
      case CA:
				if (HAL_OK == HAL_UART_Receive(&huart1, &char1, 1,10) && (char1 == CA)) {
          packet_size = 2;
        } else {
          status = HAL_ERROR;
        }
        break;
      case ABORT1:
      case ABORT2:
        status = HAL_BUSY;
        break;
      default:
        status = HAL_ERROR;
        break;
    }
    
    //if (packet_size >= PACKET_SIZE && index >= packet_size)
		if (packet_size >= PACKET_SIZE) {	
			p_data[PACKET_START_INDEX] = char1;
			status = HAL_UART_Receive(&huart1, &p_data[PACKET_NUMBER_INDEX], packet_size+PACKET_OVERHEAD_SIZE,PACKET_TIMEOUT);
      /* Simple packet sanity check */
      if (status == HAL_OK ) {
        if (p_data[PACKET_NUMBER_INDEX] != ((p_data[PACKET_CNUMBER_INDEX]) ^ NEGATIVE_BYTE)) {
          packet_size = 0;
          status = HAL_ERROR;
        } else {
          /* Check packet CRC */
          crc = p_data[ packet_size + PACKET_DATA_INDEX ] << 8;
          crc += p_data[ packet_size + PACKET_DATA_INDEX + 1 ];
					appDataCrc.nowCrc = Cal_CRC16(&p_data[PACKET_DATA_INDEX], packet_size);
					appDataCrc.crcLen = packet_size;
					if (appDataCrc.nowCrc != crc ) {   
						packet_size = 0;
            status = HAL_ERROR;
          }
        }
      } else {
        packet_size = 0;
      }
    }
  }
  *p_length = packet_size;
  return status;
}

/**
  * @brief  Update CRC16 for input byte
  * @param  crc_in input value 
  * @param  input byte
  * @retval None
  */
uint16_t UpdateCRC16(uint16_t crc_in, uint8_t byte)
{
  uint32_t crc = crc_in;
  uint32_t in = byte | 0x100;

  do {
    crc <<= 1;
    in <<= 1;
    if(in & 0x100)
      ++crc;
    if(crc & 0x10000)
      crc ^= 0x1021;
  } while(!(in & 0x10000));

  return crc & 0xffffu;
}

/**
  * @brief  Cal CRC16 for YModem Packet
  * @param  data
  * @param  length
  * @retval None
  */
uint16_t Cal_CRC16(const uint8_t* p_data, uint32_t size)
{
	unsigned short crc = 0;
	int i;

	while (size--) {
		crc = crc ^ *p_data++ << 8;
		for (i = 0; i < 8; i++) {
				if (crc & 0x8000) {
					crc = crc << 1 ^ 0x1021;
				} else {
					crc = crc << 1;
				}
		}
	}
	return crc;
}

//----------------------------
static void ymodem_BREAK(void)
{
	Serial_PutByte(CA);
  Serial_PutByte(CA);
}

//-----------------------------
static void ymodem_ACK(void)
{
  Serial_PutByte(ACK);
}

//----------------------------------
static void ymodem_ACK_REQ(void)
{
  Serial_PutByte(ACK);
  Serial_PutByte(REQ);
}

//-----------------------------
static void ymodem_NAK(void)
{
  Serial_PutByte(NAK);
}

//-------------------------------
static void ymodem_REQ(void)
{
  Serial_PutByte(REQ);
}


/* Public functions ---------------------------------------------------------*/
/**
  * @brief  Receive a file using the ymodem protocol with CRC16.
  * @param  p_size The size of the file.
  * @retval COM_StatusTypeDef result of reception/programming
  */
COM_StatusTypeDef Ymodem_Receive ( uint32_t *p_size )
{
	uint32_t packet_length, errors = 0;
  COM_StatusTypeDef result = COM_OK;	
	
	/* 用于记录请求升级的json语句发出之后的等待时间，每秒自增1 */
	uint32_t updateAskNum = 0;
	
	/* Ymodem 传输开启前相关标志位初始化 */	
	YMtrans.session_begin = 0;
	YMtrans.session_done = 0;
	YMtrans.num = 0;
	YMtrans.mode = -1;
	YMtrans.is1stData = false;
	YMtrans.finishDataLen = 0;
	memset(aPacketData,0,sizeof(aPacketData));
  while ((YMtrans.session_done == 0) && (result == COM_OK)) {//(YMtrans.file_done == 0) && 
    YMtrans.packets_received = 0;
    YMtrans.file_done = 0;
		YMtrans.EOTNum = 0;
			
    while ((YMtrans.file_done == 0) && (result == COM_OK)) {
      switch (ReceivePacket(aPacketData, &packet_length, DOWNLOAD_TIMEOUT)) {
				case HAL_OK:
					updateAskNum = 0;
          errors = 0;
					/* 处理收到的 Ymodem 数据 */	
					result = DealYmodemData(packet_length, p_size);
					break;
        case HAL_BUSY: /* Abort actually */
					ymodem_BREAK();
          result = COM_ABORT;
          break;
				case HAL_ERROR:
					if (YMtrans.session_begin > 0) {
            errors ++;
          }
          if (errors > MAX_ERRORS) {
            /* Abort communication */	
						ymodem_BREAK();
						result = COM_ABORT;
          }
					break;
        default://timeout
					ymodem_REQ(); /* Ask for a packet */								
					if (++updateAskNum > MAX_UPDATE_ASK_NUM) {						
						result = COM_ERROR;
					}
          break;
      }
    }
  }
  return result;
}

/**
  * @brief  根据长度处理接收到的 Ymodem 数据
  * @param  
  * @retval 
  */
static COM_StatusTypeDef DealYmodemData(uint32_t dataLen, uint32_t *p_size)
{
	uint32_t ramsource, errors = 0;
	uint16_t sumcrc = 0;
	COM_StatusTypeDef ret = COM_OK;

	switch (dataLen) {
		case ABORT_LEN:
			/* Abort by sender */
			ymodem_ACK();
			ret = COM_ABORT;
			break;
		case EOT_LEN:	/* EOT */
			/* End of transmission */
			if (YMtrans.EOTNum == 0) {
				YMtrans.EOTNum++;
				*p_size = YMtrans.filesize;
				ymodem_NAK();
			} else {
//				YMtrans.file_done = 1;
				YMtrans.EOTNum = 0;
				ymodem_ACK_REQ();
			}
			break;
		default:
			/* Normal packet */
			if (aPacketData[PACKET_NUMBER_INDEX] != YMtrans.packets_received && YMtrans.num) {
				if (++errors > MAX_ERRORS) {
					/* Abort communication */
					ymodem_BREAK();
					ret = COM_ABORT;
				} else {
					ymodem_NAK();
				}
			} else {									
				if ((YMtrans.packets_received == 0) && (YMtrans.num == 0)) {
					/* 处理 Ymodem 传输中序列号为0的数据包 */
					if(false == DealYmodemZeroPacket())
						break;
				} else { /* Data packet */
					//UpdateCRC16
					if(YMtrans.num != 1){
						CRC16_Update(&ctx,&aPacketData[PACKET_DATA_INDEX],appDataCrc.crcLen);
					} else {
						CRC16_Update(&ctx,&aPacketData[PACKET_DATA_INDEX],YMtrans.finishDataLen);
						CRC16_Final(&ctx, sum_crc);
						sumcrc = sum_crc[0];
						sumcrc = (sumcrc<<8) + sum_crc[1];
						appDataCrc.binCrc = sumcrc;
					}
					ramsource = (uint32_t) & aPacketData[PACKET_DATA_INDEX];
					if(YMtrans.packets_received == 1) {
						/* 处理 Ymodem 传输中序列号为1的数据包 */
						ret = DealYmodem1stPacket(dataLen, ramsource);
					} else {
						
						if(YMtrans.num > 0) {
							/* 表示最后一包数据 */
							if(YMtrans.num == 1) {
								memset((uint32_t *)(ramsource + YMtrans.finishDataLen), 0xff, (YMtrans.mode - YMtrans.finishDataLen));
							}
							/* Write received data in Flash */
							if (FLASH_If_Write(YMtrans.flashdes, (uint32_t*) ramsource, dataLen/4) == FLASHIF_OK) {
								uint16_t calcCRC = CRC16_CCITT((uint8_t *)ramsource, dataLen, 0);
								YMtrans.flashdes += dataLen;
								ymodem_ACK();							
							} else { /* An error occurred while writing to Flash memory */
								/* End session */
								ymodem_BREAK();
								ret = COM_DATA;
							}
							YMtrans.num--;
						} else {
							ymodem_ACK();	
							YMtrans.file_done = 1;
							YMtrans.session_done = 1;
							return COM_OK;
						}
					}
				}
				YMtrans.packets_received ++;
				YMtrans.session_begin = 1;
			}
			break;
	}
	
	return ret;
}

/**
  * @brief  处理 Ymodem 传输中序号为0的数据
  * @param  
  * @retval 
  */
static bool DealYmodemZeroPacket(void)
{
	uint16_t nameIndex = 0;
	uint16_t sizeIndex = 0;
	/* File name packet */
	if (aPacketData[PACKET_DATA_INDEX] != 0) {					
		/* 收到 Ymodem 传输的第一包数据，包含文件名称等信息，
		 * 将标志位设置为 1，表示下一包是真实的有效数据
		 */
		YMtrans.is1stData = true;
		for(nameIndex = 0; aPacketData[PACKET_DATA_INDEX + nameIndex] != '\0' && nameIndex < FILE_NAME_LENGTH;) {
			aFileName[nameIndex] = aPacketData[PACKET_DATA_INDEX + nameIndex];
			nameIndex += 1;
		}
		aFileName[nameIndex] = '\0';
		nameIndex += 1;
		for(sizeIndex = 0; aPacketData[PACKET_DATA_INDEX + nameIndex + sizeIndex] != ' ';) {
			aFileSize[sizeIndex] = aPacketData[PACKET_DATA_INDEX + nameIndex + sizeIndex];
			sizeIndex += 1;
		}
		aFileSize[sizeIndex] = '\0';
		if (strlen(aFileSize) > 0) {
			Str2Int((uint8_t *)aFileSize,&YMtrans.filesize);
		}
		#ifdef APPLICATION_ADDRESS_IS_FIXED
		AppHeader.entryPointAddr = APPLICATION_ADDRESS;
		#endif
		AppHeader.dataLen = YMtrans.filesize;
		ymodem_ACK_REQ();
	} else {
		/* File header packet is empty, end session */
		YMtrans.file_done = 1;
		YMtrans.session_done = 1;
		ymodem_ACK();
		return false;
	}
	
	return true;
}

/**
  * @brief  处理 Ymodem 传输中序号为1的数据
  * @param  
  * @retval 
  */
static COM_StatusTypeDef DealYmodem1stPacket(uint32_t dataLen, uint32_t srcAddr)
{
	COM_StatusTypeDef ret = COM_OK;
	
//	if((YMtrans.num == 0) && (YMtrans.is1stData == true))
	
	if(YMtrans.is1stData == true) {
		YMtrans.is1stData = false;	/* 置位 */
		
		#ifdef APPLICATION_ADDRESS_IS_FIXED
		AppHeader.entryPointAddr = APPLICATION_ADDRESS;
		#endif
		
		/*!< Vector Table base must be a multiple of 0x200. */
//		if( (AppHeader.magicName != APP_UPGREQ_IS_VALID) 
//		 || (AppHeader.entryPointAddr < APPLICATION_ADDRESS)
//		 || (AppHeader.entryPointAddr % 512 != 0) )
		if((AppHeader.entryPointAddr < APPLICATION_ADDRESS)||(AppHeader.entryPointAddr % 512 != 0) ) {
			HAL_Delay(100);
			/* Initiates a system reset request to reset the MCU */
			HAL_NVIC_SystemReset();
		}
		
		/* 文件长度由第一个数据包的 APPHeader 中的数据为准 */
		//YMtrans.filesize = AppHeader.dataLen + LEN_OF_APPHEADER;
		
		/* 用于记录数据的总包数，防止数据包计数重新开始时出现误判 */
		YMtrans.mode = (dataLen == PACKET_1K_SIZE) ? PACKET_1K_SIZE : PACKET_SIZE;
		YMtrans.num = (YMtrans.filesize % YMtrans.mode != 0) ? (YMtrans.filesize / YMtrans.mode + 1) : (YMtrans.filesize / YMtrans.mode);

		/* 用于记录最后一包有效数据的长度，避免将填充的 0x1A 数据写入Flash */
		YMtrans.finishDataLen = (YMtrans.filesize % YMtrans.mode != 0) ? (YMtrans.filesize % YMtrans.mode) : YMtrans.mode;
		
		/* Initialize flashdestination variable */
		YMtrans.flashdes = AppHeader.entryPointAddr;
		
		/* Test the size of the image to be sent */
		/* Image size is greater than Flash size */
		if (YMtrans.filesize > (USER_FLASH_END_ADDRESS - AppHeader.entryPointAddr + 1)) {
			/* End session */
			ymodem_BREAK();
			ret = COM_LIMIT;
		}
		if (FLASH_If_Write(YMtrans.flashdes, (uint32_t*)(srcAddr ), dataLen/4 ) == FLASHIF_OK) {											
			uint16_t calcCRC = CRC16_CCITT((uint8_t *)srcAddr, dataLen, 0);
			YMtrans.flashdes += dataLen;
			ymodem_ACK();			
		} else {
		 /* An error occurred while writing to Flash memory */
			ymodem_BREAK();
			ret = COM_DATA;
			/* End session */
		}
	} else {
		/* Write received data in Flash */
		if (FLASH_If_Write(YMtrans.flashdes, (uint32_t*) srcAddr, dataLen/4) == FLASHIF_OK) {
			YMtrans.flashdes += dataLen;
			ymodem_ACK();
		} else {
		 /* An error occurred while writing to Flash memory */
			ymodem_BREAK();
			ret = COM_DATA;
			/* End session */
		}
	}
	YMtrans.num--;
	
	return ret;
}

void Write_Flash_APP_UPGREQ_IS_VALID(){
		uint32_t writeRequestUpdateFlag = APP_UPGREQ_IS_VALID;
		HAL_FLASH_Unlock();
		FLASH_If_Erase(APP_UPGRADE_ADDRESS, APP_UPGRADE_ADDRESS+APP_FLASH_STEP);
		FLASH_If_Write(APP_UPGRADE_ADDRESS, &writeRequestUpdateFlag, sizeof(writeRequestUpdateFlag));
		HAL_FLASH_Lock();
}
/**
  * @brief  Download a file via serial port
  * @param  None
  * @retval None
  */
void SerialDownload(void)
{
	uint32_t size = 0;
	CRC16_Init(&ctx);

	do {
		result = Ymodem_Receive( &size );//进入ymodem传输模式
	} while(result == COM_UPERR);

	if (result == COM_OK) {	
		AppHeader.dataCRC = appDataCrc.binCrc;//
		uint16_t calcCRC = CRC16_CCITT((uint8_t *)AppHeader.entryPointAddr, size, 0);
		printf("binCrc:%04x,calcCRC:%04x,size:%d\r\n",AppHeader.dataCRC,calcCRC,size);
		/* 对整个APPLICATION文件的有效性进行判断，包括：
		* 1、文件长度；2、CCITT CRC16 的校验
		*/
		if ((AppHeader.dataLen == size) && (AppHeader.dataCRC == calcCRC)) {
			/* 擦除扇区：APP Header */
			printf("Waiting for APP Header Erase ...\n\r");
			HAL_FLASH_Unlock();
			FLASH_If_Erase(APPHEADER_ADDRESS, APPHEADER_ADDRESS + APP_FLASH_STEP); // 清除flash
			HAL_FLASH_Lock();
			/* 写入跳转APP标志 */
			uint32_t writeRequestUpdateFlag = APP_JUMP_TO_APP;
			HAL_FLASH_Unlock();
			FLASH_If_Erase(APP_UPGRADE_ADDRESS, APP_UPGRADE_ADDRESS+APP_FLASH_STEP);
			FLASH_If_Write(APP_UPGRADE_ADDRESS, &writeRequestUpdateFlag, sizeof(writeRequestUpdateFlag));
			HAL_FLASH_Lock();
			HAL_NVIC_SystemReset();
		} else {
			/* Initiates a system reset request to reset the MCU */
			printf("size err or crc err!\n\r");
			Write_Flash_APP_UPGREQ_IS_VALID();
			HAL_NVIC_SystemReset();
		}
	} else if (result == COM_LIMIT) {
		printf("\n\n\rThe image size is higher than the allowed space memory!\n\r");
		Write_Flash_APP_UPGREQ_IS_VALID();
		HAL_NVIC_SystemReset();
	} else if (result == COM_DATA) {
		printf("\n\n\rVerification failed!\n\r");
		Write_Flash_APP_UPGREQ_IS_VALID();
		HAL_NVIC_SystemReset();
	} else if (result == COM_ABORT) {
		printf("\r\n\nAborted by user.\n\r");
		Write_Flash_APP_UPGREQ_IS_VALID();
		HAL_NVIC_SystemReset();
	} else {
		printf("\n\rFailed to receive the file!\n\r");
		Write_Flash_APP_UPGREQ_IS_VALID();
		HAL_NVIC_SystemReset();
	}
}

/**
  * @}
  */

/*******************(C)COPYRIGHT 2015 STMicroelectronics *****END OF FILE****/
