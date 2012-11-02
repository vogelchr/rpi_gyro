#include "lsm330dlc.h"
#include "mux_spi.h"

#include <linux/spi/spidev.h>
#include <linux/types.h>

#include <string.h>
#include <stdio.h>

int lsm330dlc_rdwr_reg(
	struct mux_spi *spi,
	unsigned int gpiobits,
	int spinum,
	int reg,
	unsigned char *data)
{
	struct spi_ioc_transfer xfer[2];
	char rxdata;
	char txdata;
	int ret;

	txdata = *data;

	memset(&xfer,0,sizeof(xfer));
        txdata = *data;
	
        xfer[0].tx_buf = (unsigned long)&reg;
        xfer[0].len = 1;
	
        xfer[1].tx_buf = (unsigned long)&txdata;
        xfer[1].rx_buf = (unsigned long)&rxdata;
        xfer[1].len = 1;

	ret = mux_spi_ioc_msg(spi,gpiobits,spinum,8,0,0,xfer,2);
	*data = rxdata;
	return ret;
}

int
lsm330dlc_identify(
	struct mux_spi *spi,
	unsigned int gpiobits,
	int spinum)
{
	unsigned char c;
	int ret;

	ret = lsm330dlc_rdwr_reg(spi,gpiobits,spinum,0x20|0x80,&c);
	if(ret == -1)
		return -1;
	return c;
}

int
lsm330dlc_dump_regs(
	struct mux_spi *spi,
	unsigned int gpiobits,
	int spinum)
{
	int i,ret;
	unsigned char c;

	for(i=0;i<64;i++){
		if(!(i & 7))
			printf("$%02x:",i);

		ret=lsm330dlc_rdwr_reg(spi,gpiobits,spinum,0x80|i,&c);
		if(ret == -1){
			fprintf(stderr,"\nERROR!\n");
			return ret;
		}

		printf(" $%02x",(unsigned int)c);
		if((i&7)==3)
			printf("  ");
		if((i&7)==7)
			printf("\n");
	}
}
