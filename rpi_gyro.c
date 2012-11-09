#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mux_spi.h"
#include "lsm330dlc.h"

int
main(int argc,char **argv){
	struct mux_spi *spi;
	struct mux_spi_single *singlespi[4];
	struct lsm330dlc_acc_rdg acc;
	struct lsm330dlc *gyro[2];
	int i,j,temp;

	if(!(spi = mux_spi_open()))
		exit(1);

	for(i=0;i<2;i++){
		for(j=0;j<2;j++){
			singlespi[j+2*i] = mux_spi_single_create(spi,i,j,8,0,0);
		}
		gyro[i] = lsm330dlc_open(singlespi[2*i+0],singlespi[2*i+1]);
		lsm330dlc_dump_regs(gyro[i]);
	}

	j=0;
	while(1){
		i=lsm330dlc_read_temp(gyro[0],&temp);
		if(i == -1)
			exit(1);

		i=lsm330dlc_read_acc(gyro[0],&acc);
		if(i == -1)
			exit(1);

		if(acc.status & 0x08) /* new data available? */
			printf("%d 0x%02x 0x%02x %d %d %d %d\n",j++,
				acc.status, acc.src,
				acc.acc[0], acc.acc[1], acc.acc[2],temp);

		usleep(10000);
	}

	exit(0);
}
