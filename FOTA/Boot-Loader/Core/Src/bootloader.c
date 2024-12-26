/****************************************************************************************************************/
/* Author  : Reem Mohy & Mohamed Gamal 													  						*/
/* Date    : 25 NOV. 2024  													  									*/
/* Version : V01														      									*/
/****************************************************************************************************************/

/*---------------------------------------------- Includes ------------------------------------------------------*/
#include "bootloader.h"
/*--------------------------------------------------------------------------------------------------------------*/

/*------------------------------------------ Global Variables --------------------------------------------------*/
static uint8_t BL_Buffer [BL_BUFFER_SIZE];

static uint8_t BL_Supported_CMD[BL_NUMBERS_OF_CMD] = {BL_GET_VER_CMD, BL_GET_HELP_CMD, BL_GET_CHIP_ID_CMD, \
		BL_FLASH_ERASE_CMD, BL_MEM_WRITE_CMD };
/*--------------------------------------------------------------------------------------------------------------*/


/*---------------------------------------- Functions Definition ------------------------------------------------*/

/*
 * Brief: a variadic function used for debugging only.
 * Used to print a message on the Realterm using UART.
 *
 */
void BL_PrintDebugMessage (char *Format, ...)
{
	char Mess[200] = {0};

	va_list args;					/* Holds the information needed by va_start, va_arg, va_end */
	va_start(args, Format);			/* Enables access to the variable arguments */
	vsprintf(Mess, Format, args);	/* Write formatted data from variable argument list to string */

	/* Trasmit the formatted data through the defined UART */
	HAL_UART_Transmit(BL_DEBUG_UART, (uint8_t *) Mess, sizeof(Mess), HAL_MAX_DELAY);

	va_end(args);	/* Performs cleanup for an object initialized by a call to va_start */
}


/*
 * Brief: used to receive the command (packet) from the host.
 * Then save it in a buffer and parse it.
 * Return : this function return the status of the bootloader (BL_ACK or BL_NACK).
 *
 */
BL_Status BL_FeatchHostCommand (void)
{
	BL_Status ReplyStatus = BL_NACK;
	HAL_StatusTypeDef HAL_Status = HAL_ERROR;
	uint8_t Host_DataLength = 0;	/* One byte that store the data length received from the host*/

	memset (BL_Buffer, 0, BL_BUFFER_SIZE);	/* First step is to clear the buffer from any previous data*/
	/* Receive the data length from the host through uart*/
	HAL_Status = HAL_UART_Receive(BL_HOST_COMM_UART, BL_Buffer, HOST_DATA_LEN_BYTE_SIZE, HAL_MAX_DELAY);



	if(HAL_Status != HAL_OK)	/* Error in UART receive*/
	{
		ReplyStatus = BL_NACK;
	}
	else
	{
		Host_DataLength = BL_Buffer[0];
		/* Receive the hole packet according to the length provided above*/
		HAL_Status = HAL_UART_Receive(BL_HOST_COMM_UART, &BL_Buffer[1], Host_DataLength, HAL_MAX_DELAY);
		if(HAL_Status != HAL_OK)
		{
			ReplyStatus = BL_NACK;
		}
		else
		{
			//BL_PrintDebugMessage("Inside Fetach %d \r\n" , BL_Buffer[1]);
			switch (BL_Buffer[1])
			{
			case BL_GET_VER_CMD:

				BL_Get_Version(BL_Buffer);
				ReplyStatus = BL_ACK;

				break;

			case BL_GET_HELP_CMD:

				BL_Get_Help(BL_Buffer);
				ReplyStatus = BL_ACK;
				break;

			case BL_GET_CHIP_ID_CMD:

				BL_Get_ChipID(BL_Buffer);
				ReplyStatus = BL_ACK;
				break;

			case BL_FLASH_ERASE_CMD:

				BL_Flash_Erase(BL_Buffer);
				ReplyStatus = BL_ACK;
				break;

			case BL_MEM_WRITE_CMD:

				BL_Memory_Write(BL_Buffer);
				ReplyStatus = BL_ACK;
				break;

			case BL_GO_TO_ADDR_CMD:

				ReplyStatus = BL_ACK;
				BL_Jump_To_Application(BL_Buffer);
				break;

			default:

				break;
			}
		}
	}
	return ReplyStatus;
}


uint32_t calculate_crc(uint8_t *Buffer, size_t Buffer_Length)
{
	uint32_t CRC_Value = 0xFFFFFFFF; /* Initial CRC value */
	uint8_t DataElem;
	uint8_t DataElemBitLen;

	for (size_t i = 0; i < Buffer_Length; i++)
	{
		DataElem = Buffer[i];
		CRC_Value ^= ((uint32_t)DataElem << 24);   /* Align 8-bit data to the MSB of 32-bit CRC */

		for (DataElemBitLen = 0; DataElemBitLen < 8; DataElemBitLen++)
		{
			if (CRC_Value & 0x80000000)
			{
				CRC_Value = (CRC_Value << 1) ^ 0x04C11DB7;    /* Polynomial XOR */
			}
			else
			{
				CRC_Value = (CRC_Value << 1);
			}
		}
	}

	return CRC_Value;
}

/*--------------------------------------------------------------------------------------------------------------*/

/*------------------------------------- Static Functions Definition --------------------------------------------*/

static BL_CRC_Verify_Status  BL_CRC_Verify(uint8_t * BL_ReplyData, uint32_t BL_ReplyDataLength, uint32_t Host_CRC)
{
	BL_PrintDebugMessage("Inside CRC\r\n");

	BL_CRC_Verify_Status CRC_Status = CRC_NOT_VRIFIED;
	uint32_t MCU_Calculated_CRC = 0;
	uint8_t  Data_Counter = 0;
	uint32_t Data_Buffer  = 0;

	/* Calculate the CRC */
	MCU_Calculated_CRC = calculate_crc(BL_ReplyData, BL_ReplyDataLength);


	BL_PrintDebugMessage("Host_CRC is 0x%x\r\n",Host_CRC);
	BL_PrintDebugMessage("Reem_CRC is 0x%x\r\n",MCU_Calculated_CRC);

	if(MCU_Calculated_CRC == Host_CRC)  /* Compare the Host CRC and Calculated CRC */
	{
		CRC_Status = CRC_VRIFIED;
	}
	else
	{
		CRC_Status = CRC_NOT_VRIFIED;
	}

	BL_PrintDebugMessage("CRC Status %d\r\n",CRC_Status);
	return CRC_Status;

}



static void BL_Send_ACK(uint8_t BL_Replay_Len)
{
	uint8_t Ack_Value[2] = {0};
	Ack_Value[0] = BL_SEND_ACK;
	Ack_Value[1] = BL_Replay_Len;
	/* Send ACK to the host via uart */
	BL_PrintDebugMessage("ACK value is 0x%x\r\n",Ack_Value[0]);
	HAL_UART_Transmit(BL_HOST_COMM_UART, Ack_Value, sizeof(Ack_Value), HAL_MAX_DELAY);
}


static void BL_Send_NACK(void)
{
	uint8_t Nack_Value = BL_SEND_NACK;
	HAL_UART_Transmit(BL_HOST_COMM_UART, &Nack_Value, 1, HAL_MAX_DELAY);
}


static void BL_Get_Version(uint8_t * Copy_Buffer)
{
	BL_PrintDebugMessage("Inside Get Version\r\n");
	/* Store the total length of the packet sent by the host (data length + 1) */
	uint16_t Host_PacketLength = 0;
	uint32_t Host_CRC = 0;		/* Store the 4 bytes of the Host CRC */

	uint8_t BL_Version[BL_VER_COMPONENTS_NUM] = {BL_VENDOR_ID, BL_SW_MAJOR_VERSION,\
			BL_SW_MINOR_VERSION, BL_SW_PATCH_VERSION};

	/* Extract the packet length and CRC sent by the host */
	Host_PacketLength = Copy_Buffer[0] + 1;
	/* Dereference the pointer that points to the first byte of CRC */
	Host_CRC = *((uint32_t*)((Copy_Buffer + Host_PacketLength) - CRC_BYTE_SIZE));

	/* CRC verification */
	if (BL_CRC_Verify(Copy_Buffer, (Host_PacketLength - CRC_BYTE_SIZE), Host_CRC) == CRC_VRIFIED)
	{
		BL_PrintDebugMessage("inside if crc passed\r\n");
		/* Send ACK + reply data length */
		BL_Send_ACK(sizeof(BL_Version));
		/* Send the Version back to the host */
		HAL_UART_Transmit(BL_HOST_COMM_UART, BL_Version, sizeof(BL_Version), HAL_MAX_DELAY);
	}
	else
	{
		/* Send NACK only */
		BL_Send_NACK();
	}
}

static void BL_Get_Help(uint8_t * Copy_Buffer)
{
	/* Store the total length of the packet sent by the host (data length + 1) */
	uint16_t Host_PacketLength = 0;
	uint32_t Host_CRC = 0;		/* Store the 4 bytes of the Host CRC */

	/* Extract the packet length and CRC sent by the host */
	Host_PacketLength = Copy_Buffer[0] + 1;
	/* Dereference the pointer that points to the first byte of CRC */
	Host_CRC = *((uint32_t*)((Copy_Buffer + Host_PacketLength) - CRC_BYTE_SIZE));

	/* CRC verification */
	if (BL_CRC_Verify(Copy_Buffer, (Host_PacketLength - CRC_BYTE_SIZE), Host_CRC) == CRC_VRIFIED)
	{
		/* Send ACK + reply data length */
		BL_Send_ACK(BL_NUMBERS_OF_CMD);
		/* Send the Version back to the host */
		HAL_UART_Transmit(BL_HOST_COMM_UART, BL_Supported_CMD, BL_NUMBERS_OF_CMD, HAL_MAX_DELAY);
	}
	else
	{
		/* Send NACK only */
		BL_Send_NACK();
	}
}


static void BL_Get_ChipID(uint8_t * Copy_Buffer)
{
	/* Store the total length of the packet sent by the host (data length + 1) */
	uint16_t Host_PacketLength = 0;
	uint32_t Host_CRC = 0;		/* Store the 4 bytes of the Host CRC */

	/* Extract the Chip ID from the register that defined in the data sheet */
	uint16_t Chip_CodeIDValue = (uint16_t)((*CHIP_ID_REGISTER_ADDRESS) & (0x00000FFF));

	/* Extract the packet length and CRC sent by the host */
	Host_PacketLength = Copy_Buffer[0] + 1;
	/* Dereference the pointer that points to the first byte of CRC */
	Host_CRC = *((uint32_t*)((Copy_Buffer + Host_PacketLength) - CRC_BYTE_SIZE));

	/* CRC verification */
	if (BL_CRC_Verify(Copy_Buffer, (Host_PacketLength - CRC_BYTE_SIZE), Host_CRC) == CRC_VRIFIED)
	{
		/* Send ACK + reply data length */
		BL_Send_ACK(CHIP_ID_BYTE_SIZE);
		/* Send the Version back to the host */
		HAL_UART_Transmit(BL_HOST_COMM_UART, (uint8_t *)(&Chip_CodeIDValue) , CHIP_ID_BYTE_SIZE, HAL_MAX_DELAY);
	}
	else
	{
		/* Send NACK only */
		BL_Send_NACK();
	}
}



static uint8_t Perform_Flash_Erase(uint8_t PageNumebr, uint8_t NumberOfPages)
{
	uint8_t Page_Validity_Status = INVALID_PAGE_NUMBER;
	FLASH_EraseInitTypeDef pEraseInit;
	uint8_t Remaining_Pages = 0;
	HAL_StatusTypeDef HAL_Status = HAL_ERROR;
	uint32_t PageError = 0;

	//	BL_PrintDebugMessage("Inside Perform \r\n");

	if(NumberOfPages > BL_FLASH_MAX_PAGE_NUMBER)
	{
		/* Number Of page is out of range */
		Page_Validity_Status = INVALID_PAGE_NUMBER;
	}

	else
	{
		if((PageNumebr <= (BL_FLASH_MAX_PAGE_NUMBER - 1)) || (PageNumebr == BL_FLASH_MASS_ERASE))
		{
			/* Check if user needs Mass erase */
			if(PageNumebr == BL_FLASH_MASS_ERASE)
			{
				pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES; 	/* Flash Mass erase activation (starting from the app) */
				pEraseInit.PageAddress = (0x8008000);        /* Initial FLASH page to erase when Mass erase is disabled */
				pEraseInit.NbPages = BL_FLASH_MAX_PAGE_NUMBER; 	/* Number of page to be erased. */
			}
			else
			{
				/* User needs page erase */
				Remaining_Pages = BL_FLASH_MAX_PAGE_NUMBER - PageNumebr;
				if(NumberOfPages > Remaining_Pages)
				{
					NumberOfPages = Remaining_Pages;
				}
				else { /* Nothing */ }

				pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES; 	/* Page erase only */
				pEraseInit.PageAddress = ((PageNumebr * 0x800) + (0x8008000));        /* Initial FLASH page to erase when Mass erase is disabled */
				pEraseInit.NbPages = NumberOfPages; 	/* Number of page to be erased. */
			}

			pEraseInit.Banks = FLASH_BANK_1; /* Bank 1  */

			/* Unlock the FLASH control register access */
			HAL_Status = HAL_FLASH_Unlock();

			/* Perform a mass erase or erase the specified FLASH memory pages */
			HAL_Status = HAL_FLASHEx_Erase(&pEraseInit, &PageError);

			if(PageError == HAL_SUCCESSFUL_ERASE)
			{
				Page_Validity_Status = SUCCESSFUL_ERASE;
			}
			else
			{
				Page_Validity_Status = UNSUCCESSFUL_ERASE;
			}
			/* Locks the FLASH control register access */
			HAL_Status = HAL_FLASH_Lock();
		}
		else
		{
			Page_Validity_Status = UNSUCCESSFUL_ERASE;
		}
	}
	return Page_Validity_Status;
}


static void BL_Flash_Erase(uint8_t * Copy_Buffer)
{
	/* Store the total length of the packet sent by the host (data length + 1) */
	uint16_t Host_PacketLength = 0;
	uint32_t Host_CRC = 0;		/* Store the 4 bytes of the Host CRC */
	uint8_t Erase_Status = UNSUCCESSFUL_ERASE;

	/* Extract the packet length and CRC sent by the host */
	Host_PacketLength = Copy_Buffer[0] + 1;
	/* Dereference the pointer that points to the first byte of CRC */
	Host_CRC = *((uint32_t*)((Copy_Buffer + Host_PacketLength) - CRC_BYTE_SIZE));

	//	BL_PrintDebugMessage("Inside BL_ERASE \r\n");
	/* CRC verification */
	if (BL_CRC_Verify(Copy_Buffer, (Host_PacketLength - CRC_BYTE_SIZE), Host_CRC) == CRC_VRIFIED)
	{
		/* Send ACK + reply data length (status of the erase) */
		BL_Send_ACK(1);

		/* Perform Mass erase or page erase of the user flash (takes the starting page no. and the number of pages needed to be erased) */
		Erase_Status = Perform_Flash_Erase(Copy_Buffer[2], Copy_Buffer[3]);

		/* Report the status of erase process */
		HAL_UART_Transmit(BL_HOST_COMM_UART, (uint8_t *)(&Erase_Status) , 1, HAL_MAX_DELAY);
	}
	else
	{
		/* Send NACK only */
		BL_Send_NACK();
	}
}


static uint8_t Host_Address_Verify(uint32_t Base_Addr)
{
	uint8_t Addr_Status = INVALID_ADDRESS;

	if((Base_Addr >= FLASH_WR_BASE_ADD) && (Base_Addr <= FLASH_WR_END_ADD))
	{
		Addr_Status = VALID_ADDRESS;
	}
	else
	{
		Addr_Status = INVALID_ADDRESS;
	}
	return Addr_Status;
}


static uint8_t Perform_Flash_Write(uint8_t *Host_Payload, uint32_t Payload_Base_Addr, uint16_t Payload_Len)
{
	HAL_StatusTypeDef HAL_Status = HAL_ERROR;
	uint8_t Flash_Write_Payload_Status = FLASH_WRITE_PAYLOAD_FAILED;
	uint16_t PayloadCounter = 0;

	//	BL_PrintDebugMessage("Inside BL_Perform_WRITE 0x%x\r\n", Payload_Base_Addr);

	/* Unlock the FLASH control register access */
	HAL_Status = HAL_FLASH_Unlock();

	if(HAL_Status != HAL_OK)
	{
		Flash_Write_Payload_Status = FLASH_WRITE_PAYLOAD_FAILED;
	}
	else
	{
		for(PayloadCounter = 0; PayloadCounter < Payload_Len; PayloadCounter += 2)
		{
			/* Program a byte at a specified address */
			HAL_Status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, Payload_Base_Addr + PayloadCounter, *((uint16_t *)(Host_Payload + PayloadCounter)));
			if(HAL_Status != HAL_OK)
			{
				Flash_Write_Payload_Status = FLASH_WRITE_PAYLOAD_FAILED;
				break;
			}
			else
			{
				Flash_Write_Payload_Status = FLASH_WRITE_PAYLOAD_PASSED;
			}

		}
	}

	if((Flash_Write_Payload_Status == FLASH_WRITE_PAYLOAD_PASSED) && (HAL_Status == HAL_OK))
	{
		/* Locks the FLASH control register access */
		HAL_Status = HAL_FLASH_Lock();

		if(HAL_Status != HAL_OK)
		{
			Flash_Write_Payload_Status = FLASH_WRITE_PAYLOAD_FAILED;
		}
		else
		{
			Flash_Write_Payload_Status = FLASH_WRITE_PAYLOAD_PASSED;
		}
	}
	else
	{
		Flash_Write_Payload_Status = FLASH_WRITE_PAYLOAD_FAILED;
	}

	return Flash_Write_Payload_Status;
}


static void BL_Memory_Write(uint8_t *Copy_Buffer)
{
	/* Store the total length of the packet sent by the host (data length + 1) */
	uint16_t Host_PacketLength = 0;
	uint32_t Host_CRC = 0;		/* Store the 4 bytes of the Host CRC */

	uint32_t HOST_BaseWriteAddr = 0;
	uint8_t  Payload_Len  = 0;

	uint8_t  Addr_Status = INVALID_ADDRESS;
	uint8_t  Flash_Write_Payload_Status = FLASH_WRITE_PAYLOAD_FAILED;

	/* Extract the packet length and CRC sent by the host */
	Host_PacketLength = Copy_Buffer[0] + 1;
	/* Dereference the pointer that points to the first byte of CRC */
	Host_CRC = *((uint32_t*)((Copy_Buffer + Host_PacketLength) - CRC_BYTE_SIZE));



	/* CRC verification */
	if (BL_CRC_Verify(Copy_Buffer, (Host_PacketLength - CRC_BYTE_SIZE), Host_CRC) == CRC_VRIFIED)
	{
		/* Send ACK + reply data length (status of the erase) */
		BL_Send_ACK(1);
		/* Extract the start address from the Host packet */
		HOST_BaseWriteAddr = *((uint32_t *)(&Copy_Buffer[2]));

		//BL_PrintDebugMessage("Inside BL_WRITE 0x%x\r\n", HOST_BaseWriteAddr);
		//BL_PrintDebugMessage("Inside BL_WRITE %d\r\n", HOST_BaseWriteAddr);

		/* Extract the payload length from the Host packet */
		Payload_Len = Copy_Buffer[6];
		/* Verify the Extracted address to be valid address */
		Addr_Status = Host_Address_Verify(HOST_BaseWriteAddr);

		if(Addr_Status == VALID_ADDRESS)
		{
			/* Write the payload to the Flash memory */
			Flash_Write_Payload_Status = Perform_Flash_Write((uint8_t *)&Copy_Buffer[7], HOST_BaseWriteAddr, Payload_Len);

			/* Report payload write passed */
			HAL_UART_Transmit(BL_HOST_COMM_UART, (uint8_t *)(&Flash_Write_Payload_Status) , 1, HAL_MAX_DELAY);
		}
		else
		{
			/* Report address verification failed */
			Addr_Status = INVALID_ADDRESS;
			HAL_UART_Transmit(BL_HOST_COMM_UART, (uint8_t *)(&Addr_Status) , 1, HAL_MAX_DELAY);
		}
	}
	else
	{
		/* Send NACK only */
		BL_Send_NACK();
	}
}


static void Perform_Jump_To_APPlication(uint32_t Jump_Addr)
{
	/* Turn OFF the Boot-loader LED */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);

	/* Extract the value of the main stack pointer of the needed application */
	uint32_t APP_MSP_Value = * ((volatile uint32_t *) Jump_Addr);	/* First 4 bytes from the base address */

	/* Extract the reset handler function of the needed application */
	uint32_t APP_Reset_Hndler_Addr = * ((volatile uint32_t *)(Jump_Addr + 4));	/* Second 4 bytes from the base address */

	void (*Ptr_ResetHandlerFunc) (void) = (void *)APP_Reset_Hndler_Addr; 		/* Pointer to function */

	/* Assembly instruction that set the main stack pointer to the new location */
	__set_MSP(APP_MSP_Value);

	/* Deinitialize the modules (like RCC) */
	HAL_UART_DeInit(&huart2);
	HAL_UART_DeInit(&huart3);
	HAL_RCC_DeInit();

	/* Call the Reset Handler Function (That contains the needed main application) */
	Ptr_ResetHandlerFunc();		/* Jump to our application */
}


static void BL_Jump_To_Application(uint8_t * Copy_Buffer)
{
	/* Store the total length of the packet sent by the host (data length + 1) */
	uint16_t Host_PacketLength = 0;
	uint32_t Host_CRC = 0;		/* Store the 4 bytes of the Host CRC */

	uint32_t HOST_JumpAddr = 0;
	uint8_t  Addr_Status = INVALID_ADDRESS;

	/* Extract the packet length and CRC sent by the host */
	Host_PacketLength = Copy_Buffer[0] + 1;
	/* Dereference the pointer that points to the first byte of CRC */
	Host_CRC = *((uint32_t*)((Copy_Buffer + Host_PacketLength) - CRC_BYTE_SIZE));

	/* CRC verification */
	if (BL_CRC_Verify(Copy_Buffer, (Host_PacketLength - CRC_BYTE_SIZE), Host_CRC) == CRC_VRIFIED)
	{
		/* Send ACK + reply data length */
		BL_Send_ACK(1);
		/* Extract the required address to jump from the Host packet */
		HOST_JumpAddr = *((uint32_t *)(&Copy_Buffer[2]));

		Addr_Status = Host_Address_Verify(HOST_JumpAddr);

		if(Addr_Status == VALID_ADDRESS)
		{
			/* Report jump passed */
			HAL_UART_Transmit(BL_HOST_COMM_UART, (uint8_t *)(&Addr_Status) , 1, HAL_MAX_DELAY);

			Perform_Jump_To_APPlication(HOST_JumpAddr);
		}
		else
		{
			/* Report jump verification failed */
			Addr_Status = INVALID_ADDRESS;
			HAL_UART_Transmit(BL_HOST_COMM_UART, (uint8_t *)(&Addr_Status) , 1, HAL_MAX_DELAY);
		}
	}
	else
	{
		/* Send NACK only */
		BL_Send_NACK();
	}
}
/*--------------------------------------------------------------------------------------------------------------*/
