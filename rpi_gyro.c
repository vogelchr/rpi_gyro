#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#include "mux_spi.h"
#include "lsm330dlc.h"

int
main(int argc,char **argv){
	struct mux_spi *spi;
	struct mux_spi_single *singlespi[4];
	struct lsm330dlc_acc_rdg acc;
	struct lsm330dlc_gyro_rdg gyro;
	struct lsm330dlc *chip[2];
	int i,j,temp;
	FILE *f;

	if(argc != 2){
		fprintf(stderr,"Usage: %s output_file\n",argv[0]);
		exit(1);
	}

	f = fopen(argv[1],"w");
	if(!f){
		perror(argv[1]);
		exit(1);
	}

	/* for udelay "calibration" */
	int usleep_value = 2000;
	int num_meas = 0;
	int num_skip = 0;

	if(!(spi = mux_spi_open()))
		exit(1);

	for(i=0;i<2;i++){
		for(j=0;j<2;j++){
			singlespi[j+2*i] = mux_spi_single_create(spi,i,j,8,0,0);
		}
		chip[i] = lsm330dlc_open(singlespi[2*i+0],singlespi[2*i+1]);
		lsm330dlc_dump_regs(chip[i]);
	}

	j=0;
	while(1){
		if(!(j % 1000)){
			i=lsm330dlc_read_temp(chip[0],&temp);
			if(i == -1)
				exit(1);
		}

		i=lsm330dlc_read_acc(chip[0],&acc);
		if(i == -1)
			exit(1);

		i=lsm330dlc_read_gyro(chip[0],&gyro);
		if(i == -1)
			exit(1);

		if(acc.fss)
			fprintf(f,"ACC  %6d 0x%02x 0x%02x %2d %6d %6d %6d\n",j,
				acc.status, acc.src, acc.fss,
				acc.acc[0], acc.acc[1], acc.acc[2]);

		if(gyro.fss)
			fprintf(f,"GYRO  %6d 0x%02x 0x%02x %2d %6d %6d %6d\n",j,
				gyro.status, gyro.src, gyro.fss,
				gyro.rot[0], gyro.rot[1], gyro.rot[2]);
		j++;

		/* ----- this is a quick hack to optimize the proper udelay value ----- */
		if(acc.fss || gyro.fss){
			putchar('+');
			num_meas++;
		} else {
			putchar('.');
			num_skip++;
		}

		if(num_meas > 20){
			if((num_meas > num_skip) && (usleep_value > 2000))
				usleep_value -= 200;
			if((num_meas < num_skip))
				usleep_value += 200;
			printf("meas = %d, skip = %d : usleep -> %d\n",num_meas, num_skip, usleep_value);
			num_meas = num_skip = 0;
		}

		usleep(usleep_value);
	}

	exit(0);
}
