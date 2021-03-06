/*
 * FileName: spi_flash.c
 * Description: Alternate SDK (main.a)
 * (c) pvvx 2015
 */

#include "user_config.h"
#include "bios.h"
#include "hw/esp8266.h"
#include "hw/spi_register.h"
#include "flash.h"
#include "user_interface.h"

#ifdef USE_OVERLAP_MODE
int	dual_flash_flag;
#else
	void *flash_read;
#endif
//=============================================================================
// define
//-----------------------------------------------------------------------------
#ifdef USE_MAX_IRAM
#define Cache_Read_Enable_def() Cache_Read_Enable(0, 0, 0)
#else
#define Cache_Read_Enable_def() Cache_Read_Enable(0, 0, 1)
#endif
/******************************************************************************
 * FunctionName : Cache_Read_Enable_New
 * Returns      : none
 *******************************************************************************/
/* 
void Cache_Read_Enable_New(void)
{
#ifdef USE_OVERLAP_MODE
	if(dual_flash_flag)
#ifdef USE_MAX_IRAM
		Cache_Read_Enable(1,0,0);
#else
		Cache_Read_Enable(1,0,1);
#endif
	else
#endif
	Cache_Read_Enable_def();
}
*/
/******************************************************************************
 * FunctionName : spi_flash_read
 * Description  : чтение массива байт из flash
 *  			  читает из flash по QSPI блоками по SPI_FBLK байт
 *  			  в ROM-BIOS SPI_FBLK = 32 байта, 64 - предел SPI буфера
 * Parameters   : flash Addr, pointer, кол-во
 * Returns      : SpiFlashOpResult 0 - ok
 * Опции gcc: -mno-serialize-volatile !
 *******************************************************************************/
#define SPI_FBLK 32
SpiFlashOpResult ICACHE_RAM_ATTR __attribute__((optimize("O2"))) spi_flash_read(uint32 faddr, void *des, uint32 size)
{
#if DEBUGSOO > 2
	ets_printf("fread:%p<-%p[%u]\n", des, faddr, size);
#endif
	if(des == NULL) return SPI_FLASH_RESULT_ERR;
#ifdef USE_OVERLAP_MODE
	if(flash_read != NULL) return flash_read(flashchip, faddr, des, size);
#endif
	if(size != 0) {
		faddr <<= 8; faddr >>= 8; //	faddr &= (1 << 24) - 1; //	if((faddr >> 24) || ((faddr + size) >> 24)) return SPI_FLASH_RESULT_ERR;
		Cache_Read_Disable();
		Wait_SPI_Idle(flashchip);
		uint32 blksize = (uint32)des & 3;
		if(blksize) {
			blksize = 4 - blksize;
#if DEBUGSOO > 4
			ets_printf("fr1:%p<-%p[%u]\n", des, faddr, blksize);
#endif
			if(size < blksize) blksize = size;
			SPI0_ADDR = faddr | (blksize << 24);
			SPI0_CMD = SPI_READ;
			size -= blksize;
			faddr += blksize;
			while(SPI0_CMD);
			register uint32 data_buf = SPI0_W0;
			do {
				*(uint8 *)des = data_buf;
				des = (uint8 *)des + 1;
				data_buf >>= 8;
			} while(--blksize);
		}
		while(size) {
			if(size < SPI_FBLK) blksize = size;
			else blksize = SPI_FBLK;
#if DEBUGSOO > 5
			ets_printf("fr2:%p<-%p[%u]\n", des, faddr, blksize);
#endif
			SPI0_ADDR = faddr | (blksize << 24);
			SPI0_CMD = SPI_READ;
			size -= blksize;
			faddr += blksize;
			while(SPI0_CMD);
			uint32 *srcdw = (uint32 *)(&SPI0_W0);
			while(blksize >> 2) {
				*((uint32 *)des) = *srcdw++;
				des = ((uint32 *)des) + 1;
				blksize -= 4;
			}
			if(blksize) {
#if DEBUGSOO > 4
				ets_printf("fr3:%p<-%p[%u]\n", des, faddr, blksize);
#endif
				uint32 data_buf = *srcdw;
				do {
					*(uint8 *)des = data_buf;
					des = (uint8 *)des + 1;
					data_buf >>= 8;
				} while(--blksize);
				break;
			}
		}
		Cache_Read_Enable_def();
	}
	return SPI_FLASH_RESULT_OK;
}
/******************************************************************************
 * FunctionName : spi_flash_get_id
 * Returns      : flash id
 *******************************************************************************/
uint32 ICACHE_RAM_ATTR spi_flash_get_id(void)
{
	Cache_Read_Disable();
	Wait_SPI_Idle(flashchip);
	SPI0_W0 = 0;     // 0x60000240 = 0
	SPI0_CMD = SPI_RDID; // 0x60000200 = 0x10000000
	while (SPI0_CMD);
	uint32_t id = SPI0_W0 & 0xffffff;
	Cache_Read_Enable_def();
	return id;
}
/******************************************************************************
 * FunctionName : spi_flash_read_status
 * Returns      : SpiFlashOpResult
 *******************************************************************************/
SpiFlashOpResult ICACHE_RAM_ATTR spi_flash_read_status(uint32_t * sta)
{
	Cache_Read_Disable();
	uint32 ret = SPI_read_status(flashchip, sta);
	Cache_Read_Enable_def();
	return ret;
}
/******************************************************************************
 * FunctionName : spi_flash_write_status
 * Returns      : SpiFlashOpResult
 *******************************************************************************/
SpiFlashOpResult ICACHE_RAM_ATTR spi_flash_write_status(uint32_t sta)
{
	Cache_Read_Disable();
	SpiFlashOpResult ret = SPI_write_status(flashchip, sta);
	Cache_Read_Enable_def();
	return ret;
}
/******************************************************************************
 * FunctionName : spi_flash_erase_sector
 * Returns      : SpiFlashOpResult
 *******************************************************************************/
SpiFlashOpResult ICACHE_RAM_ATTR spi_flash_erase_sector(uint16 sec)
{
	Cache_Read_Disable();
	open_16m();
	SpiFlashOpResult ret = SPIEraseSector(sec);
	close_16m();
	Cache_Read_Enable_def();
	return ret;
}
/******************************************************************************
 * FunctionName : spi_flash_write
 * Returns      : SpiFlashOpResult
 *******************************************************************************/
SpiFlashOpResult ICACHE_RAM_ATTR spi_flash_write(uint32 des_addr, uint32 *src_addr, uint32 size)
{
	if(src_addr == NULL) return SPI_FLASH_RESULT_ERR;
	if(size & 3) size &= ~3;
	Cache_Read_Disable();
	open_16m();
	SpiFlashOpResult ret = SPIWrite(des_addr, (uint32_t *) src_addr, size);
	close_16m();
	Cache_Read_Enable_def();
	return ret;
}
/******************************************************************************
 * FunctionName : spi_flash_erase_block
 * Returns      : SpiFlashOpResult
 *******************************************************************************/
SpiFlashOpResult ICACHE_RAM_ATTR spi_flash_erase_block(uint32 blk)
{
	Cache_Read_Disable();
//	ets_intr_lock();
	open_16m();
	SpiFlashOpResult ret = SPIEraseBlock(blk);
	close_16m();
//	ets_intr_unlock();
	Cache_Read_Enable_def();
	return ret;
}
/******************************************************************************
 * FunctionName : spi_flash_real_size
 * Returns      : real flash size (512k, 1M, 2M, 4M, 8M, 16M)
 *******************************************************************************/
uint32 spi_flash_real_size(void) {
	uint32 size = FLASH_MIN_SIZE;
	uint32 x1[8], x2[8];
	if (spi_flash_read(0, x1, 8*4) == SPI_FLASH_RESULT_OK) {
		for (size = FLASH_MIN_SIZE; size < FLASH_MAX_SIZE; size <<= 1) {
			if (spi_flash_read(size, x2, 8*4) != SPI_FLASH_RESULT_OK)	break;
			else if (!ets_memcmp(x1, x2, 8*4)) break;
		};
	};
	return size;
}


