#ifndef MUX_SPI_H
#define MUX_SPI_H

struct mux_spi ;
struct spi_ioc_transfer;

/* simple interface to access a SPI bus that
   has uses a gpio-controlled multiplexer in a
   uniform manner. GPIO pins and number of SPI
   interfaces are hardcoded in the .c file! */

/* open the spi */
struct mux_spi *
mux_spi_open();

/* close everything */
void
mux_spi_close();

/* do a IOC_MSG transfer (see linux docs)
   with GPIO pins set to gpiobits (bitmask)
   and on spin bus #spinum.

   spimode, spibpw, spilsb are the SPI bus settings. */

int
mux_spi_ioc_msg(struct mux_spi *p,
		unsigned int gpiobits,
		int spinum,
		char spibpw,
		char spimode,
		char spilsb,
		struct spi_ioc_transfer *xfer,
		int nxfer);

struct mux_spi_single;

extern struct mux_spi_single *
mux_spi_single_create(struct mux_spi *parent, unsigned int gpiobits,
        int spinum, char spibpw, char spimode, char spilsb);

extern void
mux_spi_single_config(struct mux_spi_single *p,
	char spibpw, char spimode, char spilsb);

extern int
mux_spi_single_ioc_msg(struct mux_spi_single *p,
                struct spi_ioc_transfer *xfer,
                int nxfer);

#endif
