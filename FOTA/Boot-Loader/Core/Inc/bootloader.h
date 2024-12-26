/****************************************************************************************************************/
/* Author  : Reem Mohy & Mohamed Gamal 													  						*/
/* Date    : 25 NOV. 2024  													  									*/
/* Version : V01														      									*/
/****************************************************************************************************************/

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

/*---------------------------------------------- Includes ------------------------------------------------------*/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "usart.h"
#include "crc.h"
/*--------------------------------------------------------------------------------------------------------------*/

/*----------------------------------------------- Macros -------------------------------------------------------*/
#define BL_DEBUG_UART							&huart3
#define BL_HOST_COMM_UART   					&huart2

#define CRC_ENGINE_OBJ               			&hcrc

#define BL_BUFFER_SIZE							200
#define HOST_DATA_LEN_BYTE_SIZE					1
#define CRC_BYTE_SIZE							4

#define BL_SEND_NACK                			0xAB
#define BL_SEND_ACK                				0xCD

/* Host Command Codes */
#define BL_GET_VER_CMD              			0x10
#define BL_GET_HELP_CMD             			0x11
#define BL_GET_CHIP_ID_CMD              		0x12
/* Get Read Protection Status */
#define BL_GET_RDP_STATUS_CMD       			0x13

#define BL_GO_TO_ADDR_CMD         			  	0x14
#define BL_FLASH_ERASE_CMD         	 			0x15
#define BL_MEM_WRITE_CMD            			0x16
/* Enable/Disable Write Protection */
#define BL_ED_W_PROTECT_CMD         			0x17
#define BL_MEM_READ_CMD             			0x18
/* Get Sector Read/Write Protection Status */
#define BL_READ_SECTOR_STATUS_CMD   			0x19
#define BL_OTP_READ_CMD             			0x20
/* Change Read Out Protection Level */
#define BL_CHANGE_ROP_Level_CMD     			0x21

/* Bootloader Version*/
#define BL_VER_COMPONENTS_NUM					4
#define BL_VENDOR_ID                			25
#define BL_SW_MAJOR_VERSION         			1
#define BL_SW_MINOR_VERSION         			1
#define BL_SW_PATCH_VERSION        			 	0

/* Bootloader Help Menu */
#define BL_NUMBERS_OF_CMD						5

/* Bootloader Chip ID */
#define CHIP_ID_BYTE_SIZE						2
#define CHIP_ID_REGISTER_ADDRESS				((volatile uint32_t *)0xE0042000)

/* Bootloader Jump to Specific Address*/
#define APP1_FLASH_PG_BASE_ADDR					(0x08008000)

/* Bootloader Flash Erase */
#define BL_FLASH_MAX_PAGE_NUMBER  				220
#define BL_FLASH_MASS_ERASE         			0xFF

#define INVALID_PAGE_NUMBER        				0x00
#define VALID_PAGE_NUMBER         			 	0x01
#define UNSUCCESSFUL_ERASE           			0x02
#define SUCCESSFUL_ERASE             			0x03

#define HAL_SUCCESSFUL_ERASE         			0xFFFFFFFFU

/* Bootloader Flash Write */
#define FLASH_WRITE_PAYLOAD_FAILED   			0x00
#define FLASH_WRITE_PAYLOAD_PASSED   			0x01

#define FLASH_LOCK_WRITE_FAILED      			0x00
#define FLASH_LOCK_WRITE_PASSED      			0x01

#define INVALID_ADDRESS           				0x00
#define VALID_ADDRESS             				0x01

/* Flash Memory Address Boundaries */
#define FLASH_WR_BASE_ADD         				(0x8008000)
#define FLASH_WR_END_ADD          				(0x0807F800)
/*--------------------------------------------------------------------------------------------------------------*/

/*---------------------------------------- User Defined DataType -----------------------------------------------*/
typedef enum
{
	BL_NACK = 0 ,
	BL_ACK
}BL_Status;

typedef enum
{
	CRC_NOT_VRIFIED = 0 ,
	CRC_VRIFIED
}BL_CRC_Verify_Status;

typedef void (*pMainApp)(void);
/*--------------------------------------------------------------------------------------------------------------*/

/*---------------------------------------- Functions Prototypes ------------------------------------------------*/
void BL_PrintDebugMessage (char *Format, ...);

BL_Status BL_FeatchHostCommand (void);
/*--------------------------------------------------------------------------------------------------------------*/

/*------------------------------------- Static Functions Prototypes --------------------------------------------*/
static void BL_Get_Version(uint8_t * Copy_Buffer);

static BL_CRC_Verify_Status  BL_CRC_Verify(uint8_t * BL_ReplyData, uint32_t BL_ReplyDataLength, uint32_t Host_CRC);

static void BL_Get_Help(uint8_t * Copy_Buffer);

static void BL_Get_ChipID(uint8_t * Copy_Buffer);

static void BL_Flash_Erase(uint8_t * Copy_Buffer);

static uint8_t Perform_Flash_Erase(uint8_t PageNumebr, uint8_t NumberOfPages);

static void BL_Memory_Write(uint8_t *Copy_Buffer);

static uint8_t Host_Address_Verify(uint32_t Base_Addr);

static uint8_t Perform_Flash_Write(uint8_t *Host_Payload, uint32_t Payload_Base_Addr, uint16_t Payload_Len);

static void BL_Jump_To_Application(uint8_t * Copy_Buffer);

static void Perform_Jump_To_APPlication(uint32_t Jump_Addr);

/*--------------------------------------------------------------------------------------------------------------*/

#endif   /* BOOTLOADER_H */
