#ifndef LSM330DLC_H
#define LSM330DLC_H

struct mux_spi;

extern int lsm330dlc_identify(
	struct mux_spi *spi,
	unsigned int gpiobits,
	int spinum);

extern int lsm330dlc_dump_regs(
	struct mux_spi *spi,
	unsigned int gpiobits,
	int spinum);


#endif
