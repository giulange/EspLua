/******************************************************************************
 * PV` FileName: flash.c
 *******************************************************************************/

#include "user_config.h"
#include "c_types.h"
#include "bios.h"
#include "hw/esp8266.h"
#include "hw/spi_register.h"
#include "flash.h"
#include "user_interface.h"
#include "c_string.h"
#include "c_stdlib.h"
#include "c_stdio.h"

#ifdef USE_MAX_IRAM
#define Cache_Read_Enable_def() Cache_Read_Enable(0, 0, 0)
#else
#define Cache_Read_Enable_def() Cache_Read_Enable(0, 0, 1)
#endif

uint32_t flash_size;

#ifndef USE_PVV_SPIFLASH
//=============================================================================
// define
//-----------------------------------------------------------------------------
#ifdef USE_MAX_IRAM
#define Cache_Read_Enable_def() Cache_Read_Enable(0, 0, 0)
#else
#define Cache_Read_Enable_def() Cache_Read_Enable(0, 0, 1)
#endif
/******************************************************************************
 * FunctionName : spi_flash_read
 * Returns      : SpiFlashOpResult 0 - ok
 * options gcc: -mno-serialize-volatile !
 *******************************************************************************/
#define SPI_FBLK 32
SpiFlashOpResult ICACHE_RAM_ATTR __attribute__((optimize("O2"))) flash_safe_read(uint32 faddr, void *des, uint32 size)
{
#if DEBUGSOO > 2
	ets_printf("fread:%p<-%p[%u]\n", des, faddr, size);
#endif
	if(des == NULL) return SPI_FLASH_RESULT_ERR;
	if(size != 0) {
//		faddr <<= 8; faddr >>= 8; //	faddr &= (1 << 24) - 1; //	if((faddr >> 24) || ((faddr + size) >> 24)) return SPI_FLASH_RESULT_ERR;
        faddr &= (1<<24)-1;
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
			//__asm__ __volatile__("memw" : : : "memory");
			// volatile uint32 *srcdw = (volatile uint32 *)(SPI0_BASE+0x40);
			uint32 *srcdw = (uint32 *)(&SPI0_W0);
//			uint32 *srcdw = (uint32 *)(SPI0_BASE+0x40);
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
 * FunctionName : spi_flash_erase_sector
 * Returns      : SpiFlashOpResult
 *******************************************************************************/
SpiFlashOpResult ICACHE_RAM_ATTR flash_safe_erase_sector(uint16 sec)
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
SpiFlashOpResult ICACHE_RAM_ATTR flash_safe_write(uint32 des_addr, uint32 *src_addr, uint32 size)
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
SpiFlashOpResult ICACHE_RAM_ATTR flash_safe_erase_block(uint32 blk)
{
	Cache_Read_Disable();
	open_16m();
	SpiFlashOpResult ret = SPIEraseBlock(blk);
	close_16m();
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
	if (flash_safe_read(0, x1, 8*4) == SPI_FLASH_RESULT_OK) {
		for (size = FLASH_MIN_SIZE; size < FLASH_MAX_SIZE; size <<= 1) {
			if (flash_safe_read(size, x2, 8*4) != SPI_FLASH_RESULT_OK)	break;
			else if (!ets_memcmp(x1, x2, 8*4)) break;
		};
	};
	return size;
}

#endif
/******************************************************************************
 * Flash api
*******************************************************************************/
void flash_size_init(void)
{
	flash_size = spi_flash_real_size();
}

uint8_t byte_of_aligned_array(const uint8_t *aligned_array, uint32_t index)
{
    if ( (((uint32_t)aligned_array) % 4) != 0 )
    {
        NODE_DBG("aligned_array is not 4-byte aligned.\n");
        return 0;
    }
    uint32_t v = ((uint32_t *)aligned_array)[ index / 4 ];
    uint8_t *p = (uint8_t *) (&v);
    return p[ (index % 4) ];
}


/******************************************************************************
 * FunctionName : spi_flash_write
 * Returns      : SpiFlashOpResult
 *******************************************************************************/
SpiFlashOpResult ICACHE_RAM_ATTR flash_safe_write(uint32 des_addr, uint8 *src_addr, uint32 size)
{
#if DEBUGSOO > 2
	ets_printf("fwrite:%p[%u]->%p\n", des_addr, src_addr, size);
#endif
	SpiFlashOpResult ret = SPI_FLASH_RESULT_ERR;
	if(src_addr == NULL || size == 0) return ret;
	Cache_Read_Disable();
	open_16m();
	union {
		uint8 uc[32];
		uint32 ui[8];
	}tmp;
	uint8 *p = (uint8 *)src_addr;
	uint32 xlen = des_addr & 3;
	uint32 addr = des_addr & (~3);
	if(xlen) {
		if(SPIRead(addr, (uint32_t *)&tmp.ui[0], 4) != 0) {
			close_16m();
			Cache_Read_Enable_def();
			return ret;
		}
		while (size)  {
			size--;
			tmp.uc[xlen++] = *p++;
			if(xlen & 4) break;
		}
		if(SPIWrite(addr, (const uint32_t *)&tmp.ui[0], 4) !=0) {
			close_16m();
			Cache_Read_Enable_def();
			return ret;
		}
		addr += 4;
	}
	while(size) {
		if(size < 32) {
			xlen = size;
			if(xlen & 3) if(SPIRead(addr, (uint32_t *)&tmp.ui[xlen >> 2], 4) !=0) {
				close_16m();
				Cache_Read_Enable_def();
				return ret;
			}
		}
		else xlen = 32;
		ets_memcpy(tmp.uc, p, xlen);
		p += xlen;
		if(SPIWrite(addr, (const uint32_t *)&tmp.ui[0], (xlen + 3) & (~3)) !=0) {
			close_16m();
			Cache_Read_Enable_def();
			return ret;
		}
		size -= xlen;
		addr += 32;
	}
	ret = SPI_FLASH_RESULT_OK;
	close_16m();
	Cache_Read_Enable_def();
	return ret;
}

char * flash_strcpy(char * pd_, void * ps, uint32 len);

extern char print_mem_buf[256];

const char *flash_str2buf(const char * ps)
{
	return (const char *)flash_strcpy(print_mem_buf, (void *) ps, sizeof(print_mem_buf));
}

char * __attribute__((optimize("Os"))) ICACHE_RAM_ATTR flash_strcpy(char * pd_, void * ps, uint32 len)
{
	union {
		uint8 uc[4];
		uint32 ud;
	}tmp;
	char * pd = pd_;
	pd[len] = 0;
	uint32 *p = (uint32 *)((uint32)ps & (~3));
	char c;

	uint32 xlen = (uint32)ps & 3;
	if(xlen) {
		tmp.ud = *p++;
		while (len)  {
			len--;
			c = *pd++ = tmp.uc[xlen++];
			if(c == 0) return pd_;
			if(xlen & 4) break;
		}
	}
	xlen = len >> 2;
	while(xlen) {
		tmp.ud = *p++;
		c = *pd++ = tmp.uc[0];
		if(c == 0) return pd_;
		c = *pd++ = tmp.uc[1];
		if(c == 0) return pd_;
		c = *pd++ = tmp.uc[2];
		if(c == 0) return pd_;
		c = *pd++ = tmp.uc[3];
		if(c == 0) return pd_;
		xlen--;
	}
	if(len & 3) {
		tmp.ud = *p;
		c = pd[0] = tmp.uc[0];
		if(c == 0) return pd_;
		if(len & 2) {
			c = pd[1] = tmp.uc[1];
			if(c == 0) return pd_;
			if(len & 1) {
				c = pd[2] = tmp.uc[2];
				if(c == 0) return pd_;
			}
		}
	}
	return pd_;
}

