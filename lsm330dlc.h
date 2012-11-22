#ifndef LSM330DLC_H
#define LSM330DLC_H

struct mux_spi_single;
struct lsm330dlc ;

extern struct lsm330dlc *
lsm330dlc_open(
	struct mux_spi_single *spi_a,
	struct mux_spi_single *spi_g
);

extern int
lsm330dlc_dump_regs(struct lsm330dlc *p);

struct lsm330dlc_acc_rdg {
	unsigned char status;
	unsigned char src;
	unsigned char fss;
	short acc[3];
};

struct lsm330dlc_gyro_rdg {
	unsigned char status;
	unsigned char src;
	unsigned char fss;
	short rot[3];
};

extern int
lsm330dlc_read_acc(struct lsm330dlc *p, struct lsm330dlc_acc_rdg *rdg);

extern int
lsm330dlc_read_gyro(struct lsm330dlc *p, struct lsm330dlc_gyro_rdg *rdg);

extern int
lsm330dlc_read_temp(struct lsm330dlc *p, int *t);

#endif
