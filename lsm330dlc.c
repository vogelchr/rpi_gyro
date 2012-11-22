#include "lsm330dlc.h"
#include "mux_spi.h"

#include <linux/spi/spidev.h>
#include <linux/types.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int
lsm330dlc_read_regs(
	struct mux_spi_single *spi,
	int reg,
	unsigned char *data,
	int len)
{
	struct spi_ioc_transfer xfer[2];
	int ret;

	reg |= 0x80; /* read! */

	memset(&xfer,0,sizeof(xfer));
	
        xfer[0].tx_buf = (unsigned long)&reg;
        xfer[0].len = 1;
	
        xfer[1].rx_buf = (unsigned long)data;
        xfer[1].len = len;

	ret = mux_spi_single_ioc_msg(spi,xfer,2);
	if(ret != len+1)
		ret=-1;
	return ret;
}


static int
lsm330dlc_rdwr_reg(
	struct mux_spi_single *spi,
	int reg,
	unsigned char *data)
{
	struct spi_ioc_transfer xfer[2];
	unsigned char rxdata,txdata;
	int ret;

	txdata = *data;

	memset(&xfer,0,sizeof(xfer));
        txdata = *data;
	
        xfer[0].tx_buf = (unsigned long)&reg;
        xfer[0].len = 1;
	
        xfer[1].tx_buf = (unsigned long)&txdata;
        xfer[1].rx_buf = (unsigned long)&rxdata;
        xfer[1].len = 1;

	ret = mux_spi_single_ioc_msg(spi,xfer,2);
	*data = rxdata;
	return ret;
}

int
lsm330dlc_identify( struct mux_spi_single *spi)
{
	unsigned char c;
	int ret;

	ret = lsm330dlc_rdwr_reg(spi,0x20|0x80,&c);
	if(ret == -1)
		return -1;
	return c;
}

struct lsm330dlc {
	struct mux_spi_single *spi_a;
	struct mux_spi_single *spi_g;
};

struct lsm330dlc_initdata {
	char device; /* A=0, G=1, END=2 */
	char regnum;
	unsigned char value;
};

static const struct lsm330dlc_initdata lsm330dlc_initdata[]={
	/* A/G Reg Value */
	{ 0, 0x20, 0x27 }, /* CTRL_REG1_A (20h) */
	{ 0, 0x23, 0x80 }, /* CTRL_REG4_A (23h) */
	{ 0, 0x24, 0x40 }, /* CTRL_REG5_A (24h) */
	{ 0, 0x2e, 0x80 }, /* FIFO_CTRL_REG_A (2eh) */
	{ 1, 0x20, 0x0f }, /* CTRL_REG1_G (20h) */
	{ 1, 0x21, 0x20 }, /* CTRL_REG2_G (21h) */
	{ 1, 0x23, 0x80 }, /* CTRL_REG4_G (23h) */
	{ 1, 0x24, 0x40 }, /* CTRL_REG5_G (24h) */
	{ 1, 0x2e, 0x40 }, /* FIFO_CTRL_REG_G (2eh) */
	{ 2, 0x00, 0x00 }  /* === END ============= */
};


extern struct lsm330dlc *
lsm330dlc_open(struct mux_spi_single *spi_a, struct mux_spi_single *spi_g)
{
	struct lsm330dlc *p;
	const struct lsm330dlc_initdata *id;
	struct mux_spi_single *spi;
	unsigned char val;
	int ret;

	p = calloc(1,sizeof(*p));

	p->spi_a = spi_a;
	p->spi_g = spi_g;

	mux_spi_single_config(p->spi_a,8,SPI_CPHA|SPI_CPOL,0);
	mux_spi_single_config(p->spi_g,8,SPI_CPHA|SPI_CPOL,0);


	id = lsm330dlc_initdata;
	while(id->device != 2){
		val = id->value;
		spi = p->spi_a;
		if(id->device)
			spi = p->spi_g;

		ret = lsm330dlc_rdwr_reg(spi,id->regnum,&val);
		if(ret == -1){
			fprintf(stderr,"%s: cannot initialize gyro!\n",__FUNCTION__);
			free(p);
			return NULL;
		}
		id++;
	}

	return p;
}

extern int
lsm330dlc_dump_regs(struct lsm330dlc *p)
{
	struct mux_spi_single *spi[2] = { p->spi_a, p->spi_g };
	static const char *what[2] = { "accelerometer", "gyro" };

	int i,ret,n;
	unsigned char c;

	for(n=0;n<2;n++){
		printf("%s: dumping %s registers.\n",__FUNCTION__,what[n]);

		for(i=0;i<64;i++){
			if(!(i & 7))
				printf("$%02x:",i);

			ret=lsm330dlc_rdwr_reg(spi[n],0x80|i,&c);
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
	return 0;
}

static short
signed_short_from_buf(unsigned char *buf)
{
	unsigned short us = buf[0]+(buf[1]<<8);
	return *((short*)&us);
}

int
lsm330dlc_read_acc(struct lsm330dlc *p, struct lsm330dlc_acc_rdg *rdg)
{
	unsigned char buf[9];
	int ret;

	/* FIFO_SRC_REG_A
	   WTM | OVRN | EMPTY | FSS4 | FSS3 | FSS2 | FSS1 | FSS0 */
	ret = lsm330dlc_read_regs(p->spi_a,0x2f,buf,1);
	if(ret == -1)
		return -1;

	rdg->fss = buf[0] & 0x1f;

	if (buf[0] & 0x20){ /* empty */
		return 0;
	}

	/* register starts at 0x27, bit 0x40 causes the address to
	   increment on consecutive SPI bytes */
	ret = lsm330dlc_read_regs(p->spi_a,0x27|0x40,buf,sizeof(buf));
	if(ret == -1)
		return -1;

	rdg->status = buf[0];  /* 27h STATUS_REG_A */

	/* 28h, 29h: OUT_X_L_A, OUT_X_H_A  = buf[1], buf[2] */
	/* 2ah, 2bh: OUT_Y_L_A, OUT_Y_H_A  = buf[3], buf[4] */
	/* 2ch, 2dh: OUT_Z_L_A, OUT_X_H_A  = buf[5], buf[6] */
	rdg->acc[0] = signed_short_from_buf(buf+1);
	rdg->acc[1] = signed_short_from_buf(buf+3);
	rdg->acc[2] = signed_short_from_buf(buf+5);

	/* 2f: FIFO_SRC_REG_A, buf[8] */
	rdg->src = buf[8];
	return 0;
}

int
lsm330dlc_read_gyro(struct lsm330dlc *p, struct lsm330dlc_gyro_rdg *rdg){
	unsigned char buf[9];
	int ret;

	/* FIFO_SRC_REG_G
	   WTM | OVRN | EMPTY | FSS4 | FSS3 | FSS2 | FSS1 | FSS0 */
	ret = lsm330dlc_read_regs(p->spi_g,0x2f,buf,1);
	if(ret == -1)
		return -1;

	rdg->fss = buf[0] & 0x1f;

	if (buf[0] & 0x20) /* empty */
		return 0;

	/* register starts at 0x27, bit 0x40 causes the address to
	   increment on consecutive SPI bytes */
	ret = lsm330dlc_read_regs(p->spi_g,0x27|0x40,buf,sizeof(buf));
	if(ret == -1)
		return -1;

	rdg->status = buf[0];  /* 27h STATUS_REG_A */

	/* 28h, 29h: OUT_X_L_A, OUT_X_H_A  = buf[1], buf[2] */
	/* 2ah, 2bh: OUT_Y_L_A, OUT_Y_H_A  = buf[3], buf[4] */
	/* 2ch, 2dh: OUT_Z_L_A, OUT_X_H_A  = buf[5], buf[6] */
	rdg->rot[0] = signed_short_from_buf(buf+1);
	rdg->rot[1] = signed_short_from_buf(buf+3);
	rdg->rot[2] = signed_short_from_buf(buf+5);

	/* 2f: FIFO_SRC_REG_A, buf[8] */
	rdg->src = buf[8];
	return 0;
}



int
lsm330dlc_read_temp(struct lsm330dlc *p, int *t){
	unsigned char c;
	int ret;

	ret = lsm330dlc_rdwr_reg(p->spi_a,0x26|0x80,&c);
	*t = c;
	if(ret != 2)
		ret=-1;
	return ret;
}



