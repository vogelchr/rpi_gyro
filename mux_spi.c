#include "mux_spi.h"
#include "sysfs_gpio.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <linux/types.h>
#include <linux/spi/spidev.h>


#define NUM_SPI 2
#define NUM_GPIO 1

static const int gpio_num[NUM_GPIO] = { 25 };


struct mux_spi {
	unsigned int gpiobits;
	struct sysfs_gpio *gpio[NUM_GPIO];

	int spi_fd[NUM_SPI];
	char spibpw[NUM_SPI];
	char spimode[NUM_SPI];
	char spilsb[NUM_SPI];
};

/* do a spi_ioc transfer (xfer, nxfer) on the spi-bus number
   spinum, with multiplexer bits set as gpiobits, and with
   the SPI settings specified in spibpw, spimode, spilsb.
   Returns result of ioctl() if everything works out fine,
   or -1 on error. */

int
mux_spi_ioc_msg(struct mux_spi *p,
		unsigned int gpiobits,
		int spinum,
		char spibpw,
		char spimode,
		char spilsb,
		struct spi_ioc_transfer *xfer,
		int nxfer)
{
	int i;
	int ret;

#if 0
	fprintf(stderr,"%s(0x%08x,%d,%d,%d,%d,%p,%d)\n",
		__FUNCTION__,gpiobits,spinum,spibpw,spimode,spilsb,xfer,nxfer);
	fprintf(stderr,"old: 0x%08x\n",p->gpiobits);
#endif

	/* check multiplexer bits, set gpios if necessary */
	for(i=0;i<NUM_GPIO;i++){
		/* check if bit i differs between old/new gpiobits */
		if(!((gpiobits ^ p->gpiobits) & (1<<i)))
			continue;
		if(sysfs_gpio_value(p->gpio[i],gpiobits & (1<<i)))
			return -1;

	}
	p->gpiobits = gpiobits;

	/* if necessary, update bits-per-word, lsb first and spi mode */
	if(spibpw != p->spibpw[spinum]){
		if(ioctl(p->spi_fd[spinum],SPI_IOC_WR_BITS_PER_WORD,&spibpw)){
			perror("ioctl(SPI_IOC_WR_BITS_PER_WORD)");
			return -1;
		}
	}

	if(spimode != p->spimode[spinum]){
		if(ioctl(p->spi_fd[spinum],SPI_IOC_WR_MODE,&spimode)){
			perror("ioctl(SPI_IOC_WR_MODE)");
			return -1;
		}
	}

	if(spilsb != p->spilsb[spinum]){
		if(ioctl(p->spi_fd[spinum],SPI_IOC_WR_LSB_FIRST,&spilsb)){
			perror("ioctl(SPI_IOC_WR_LSB_FIRST)");
			return -1;
		}
	}

	/* do the ioctl */
	ret=ioctl(p->spi_fd[spinum],SPI_IOC_MESSAGE(nxfer),xfer);
	if(ret == -1)
		perror("ioctl(SPI_IOC_MESSAGE(n))");
	return ret;
}
		
		

struct mux_spi *
mux_spi_open(){
	struct mux_spi *p;
	char buf[64];
	int i;

	p = calloc(1,sizeof(struct mux_spi));

	for(i=0;i<NUM_SPI;i++)
		p->spi_fd[i]=-1;

	for(i=0;i<NUM_GPIO;i++){
		p->gpio[i] = sysfs_gpio_open(gpio_num[i]);
		if(!p->gpio[i])
			goto err;
		/* initialize multiplexer bits to 0 */
		if(sysfs_gpio_direction(p->gpio[i],1) ||
		   sysfs_gpio_value(p->gpio[i],0)
		)
			goto err;
	}

	for(i=0;i<NUM_SPI;i++){
		sprintf(buf,"/dev/spidev0.%d",i);
		p->spi_fd[i] = open(buf,O_RDWR);
		if(p->spi_fd[i] == -1){
			perror(buf);
			goto err;
		}

		if(ioctl(p->spi_fd[i],SPI_IOC_RD_BITS_PER_WORD,&p->spibpw[i])){
			perror("ioctl(SPI_IOC_RD_BITS_PER_WORD)");
			goto err;
		}

		if(ioctl(p->spi_fd[i],SPI_IOC_RD_MODE,&p->spimode[i])){
			perror("ioctl(SPI_IOC_RD_MODE)");
			goto err;
		}

		if(ioctl(p->spi_fd[i],SPI_IOC_RD_LSB_FIRST,&p->spilsb[i])){
			perror("ioctl(SPI_IOC_RD_LSB_FIRST)");
			goto err;
		}
		
		fprintf(stderr,"Opened %s as fd %d (0x%02x, %d, %d).\n",
			buf,p->spi_fd[i],
			p->spimode[i],p->spibpw[i],p->spilsb[i]);
	}

	return p;

err:
	for(i=0;i<NUM_SPI;i++){
		if(p->spi_fd[i] != -1)
			close(p->spi_fd[i]);
	}
	for(i=0;i<NUM_GPIO;i++){
		if(p->gpio[i])
			sysfs_gpio_close(p->gpio[i]);
	}
	return NULL;
}

struct mux_spi_single {
	struct mux_spi *parent;
	unsigned int gpiobits;
	int spinum;
	char spibpw;
	char spimode;
	char spilsb;
};

struct mux_spi_single *
mux_spi_single_create(struct mux_spi *parent, unsigned int gpiobits,
	int spinum, char spibpw, char spimode, char spilsb)
{
	struct mux_spi_single *p;

	p=calloc(1,sizeof(*p));

	p->parent = parent;
	p->gpiobits = gpiobits;
	p->spinum = spinum;
	p->spibpw = spibpw;
	p->spimode = spimode;
	p->spilsb = spilsb;
#if 0
	fprintf(stderr,"%s: %p, 0x%08x, %d, %d, %d, %d -> %p\n",
		__FUNCTION__,parent,gpiobits,spinum,spibpw,spimode,spilsb,p);
#endif
	return p;
}

int
mux_spi_single_ioc_msg(struct mux_spi_single *p,
		struct spi_ioc_transfer *xfer,
		int nxfer)
{

//	fprintf(stderr,"%s: %p, %p, %d\n",__FUNCTION__,p,xfer,nxfer);

	return mux_spi_ioc_msg(p->parent,
		p->gpiobits,
		p->spinum,
		p->spibpw,
		p->spimode,
		p->spilsb,
		xfer,nxfer);
}
