/*
 * flash.c
 *
 *  Created on: 24 jan. 2017
 *      Author: Mark Ursum
 */

#include "main.h"
#include "flash.h"
#include "string.h"
#include "ymodem.h"
#include "stdbool.h"
#include "iwdg.h"

static FLASH_EraseInitTypeDef EraseInitStruct;

static uint32_t GetPage(uint32_t Addr);
static uint32_t GetBank(uint32_t Addr);

#if 1
HAL_StatusTypeDef stm32_flash_erase(uint32_t start, uint32_t size)
{
	uint32_t FirstPage = 0, NbOfPages = 0, BankNumber = 0, PageError = 0;

	/* Unlock the Flash to enable the flash control register access *************/
	HAL_FLASH_Unlock();

	/* Clear OPTVERR bit set on virgin samples */
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);

	/* Erase the user Flash area
	 (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

	/* Get the 1st page to erase */
	FirstPage = GetPage(start);

	/* Get the number of pages to erase from 1st page */
	NbOfPages = size / FLASH_PAGE_SIZE;

	/* Get the bank */
	BankNumber = GetBank(start);

	/* Fill EraseInit structure*/
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.Banks = BankNumber;
	EraseInitStruct.Page = FirstPage;
	EraseInitStruct.NbPages = NbOfPages;

	tty_puts(" BK1");


	/* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
	 you have to make sure that these data are rewritten before they are accessed during code
	 execution. If this cannot be done safely, it is recommended to flush the caches by setting the
	 DCRST and ICRST bits in the FLASH_CR register. */
	if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK)
	{
		/*
		 Error occurred while page erase.
		 User can add here some code to deal with this error.
		 PageError will contain the faulty page and then to know the code error on this page,
		 user can call function 'HAL_FLASH_GetError()'
		 */
			return (HAL_ERROR);
	}

//	if (EraseInitStruct.NbPages > 64)
//	{
//		EraseInitStruct.Banks = FLASH_BANK_2;
//		EraseInitStruct.Page = 0;
//		EraseInitStruct.NbPages = NbOfPages - 64;
//
//		tty_puts(" BK2");
//
//		if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK)
//		{
//			/*
//			 Error occurred while page erase.
//			 User can add here some code to deal with this error.
//			 PageError will contain the faulty page and then to know the code error on this page,
//			 user can call function 'HAL_FLASH_GetError()'
//			*/
//				return (HAL_ERROR);
//		}
//
//	}
	HAL_FLASH_Lock();

	return (HAL_OK);
}

/**
  * @brief  Gets the bank of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The bank of a given address
  */
static uint32_t GetBank(uint32_t Addr)
{
  return FLASH_BANK_1;
}
/**
 * @brief  Gets the page of a given address
 * @param  Addr: Address of the FLASH Memory
 * @retval The page of a given address
 */
static uint32_t GetPage(uint32_t Addr)
{
	uint32_t page = 0;

	if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
	{
		/* Bank 1 */
		page = (Addr - FLASH_BASE) / FLASH_PAGE_SIZE;
	}
	else
	{
		/* Bank 2 */
		page = (Addr - (FLASH_BASE + FLASH_BANK_SIZE)) / FLASH_PAGE_SIZE;
	}

	return page;
}

#else
/*!
 * \brief Erase the application memory.
 *
 * \param -
 *
 * \return true successful
 */
//__attribute__((section(".ramfunc")))
HAL_StatusTypeDef stm32_flash_erase(uint32_t start, uint32_t size)
{

	uint32_t FirstPage = 0, NbOfPages = 0;
	uint32_t PageError = 0;

	/* Get the 1st page to erase */
	FirstPage = (start - FLASH_BASE) / FLASH_PAGE_SIZE; //GetPage(start - FLASH_BASE_ADDRESS);

	/* Get the number of pages to erase from 1st page */
	NbOfPages = size / FLASH_PAGE_SIZE;

	tty_printf("page %d nr %d\r\n", FirstPage, NbOfPages);

	/* Fill EraseInit structure*/
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.Page = FirstPage;
	EraseInitStruct.NbPages = NbOfPages;
	EraseInitStruct.Banks = FLASH_BANK_1;

	HAL_FLASH_Unlock();

	/* Clear error programming flags */
	__HAL_FLASH_CLEAR_FLAG(FLASH_SR_ERRORS);

	/* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
	 you have to make sure that these data are rewritten before they are accessed during code
	 execution. If this cannot be done safely, it is recommended to flush the caches by setting the
	 DCRST and ICRST bits in the FLASH_CR register. */
	if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK)
	{
		/*
		 Error occurred while page erase.
		 User can add here some code to deal with this error.
		 PageError will contain the faulty page and then to know the code error on this page,
		 user can call function 'HAL_FLASH_GetError()'
		 */
		return (HAL_ERROR);
	}

	HAL_FLASH_Lock();

	return (HAL_OK);
}
#endif
#define DATA_64                 ((uint64_t)0x0123456789ABCDEF)
#define DATA_32                 ((uint32_t)0x12345678)

/*!
 * \brief Write data to the application memory.
 *
 * \param address   Start address of write.
 * \param p_data   Pointer to data to write.
 * \param size   Size of data to write.
 *
 * \return false if unsuccessful, else true
 *
 * \note The flash memory must be erased before it can be written.
 */
bool stm32_flash_write(uint32_t address, uint8_t *p_data, uint32_t size)
{

	HAL_FLASH_Unlock();

	uint64_t temp;

	// Iterate through the number of data bytes
	for (uint32_t var = 0; var < size; var += 8)
	{
		wdt_clr();

		memcpy(&temp, p_data, sizeof(temp));

// write block of 2*4 bytes
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, (uint64_t) temp) != HAL_OK) /*!< Fast program a 32 row double-word (64-bit) at a specified address */
		{
			return (0); // fout
		}

		address += 8;
		p_data = p_data + 8;
	}

	HAL_FLASH_Lock();

	return (1);
}

/*!
 * \brief Wrapper function for YMODEM.
 *
 * \param p_args   Pointer to arguments structure.
 *
 * \return false if unsuccessful, else true
 *
 * \note The flash memory must be erased before it can be written.
 */
bool stm32_ymodem_flash_write(void *p_args)
{
	ym_data_t ym_data;

	// Cast p_args back to a ym_data_t struct
	ym_data = *((ym_data_t*) p_args);

	// Perform flash write
	return (stm32_flash_write(ym_data.addr, ym_data.p_buf, ym_data.len));
}

#if 0
/*!
 * \brief Copy shadow flash to application address
 *
 * \param to_add	destination
 * \param from_add	source
 *
 * \return false if successful
 *
 * \note The flash memory is erased before it can be written.
 */
bool copy_shadow(uint32_t size)

//static uint8_t do_WriteStruct2Flash(flashvar_t to_save, uint32_t start_addr, uint32_t end_addr)
{

	uint32_t ReadAddress = 0, ReadEndAddress = 0, WriteAddress = 0;
	uint64_t temp;
	/* Erase the user Flash area
	 (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

	if (stm32_flash_erase(FLASH_APP_START_ADDR, APP_SIZE) != HAL_OK)
	{
		return (1);
	}

	FLASH_WaitForLastOperation(0xFFFF);

	/* Program the user Flash area word by word
	 (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

	HAL_FLASH_Unlock();

	ReadAddress = FLASH_SHADOW_START_ADDR;
	ReadEndAddress = FLASH_SHADOW_START_ADDR + size;
	WriteAddress = FLASH_APP_START_ADDR;

	for (uint32_t addr = ReadAddress; addr < ReadEndAddress; addr += 8)
	{
		wdt_clr();

		memcpy(&temp, (*(__IO uint32_t *) (addr)), sizeof(temp));

		// write block of 2*4 bytes
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, WriteAddress, (uint64_t) temp) == HAL_OK) /*!< Fast program a 32 row double-word (64-bit) at a specified address */
		{
			WriteAddress += 8;
		}
		else
		{
			return (1); // fout
		}

	}

	/* Lock the Flash to disable the flash control register access (recommended
	 to protect the FLASH memory against possible unwanted operation) *********/
	HAL_FLASH_Lock();

	return (0);
}

#endif
