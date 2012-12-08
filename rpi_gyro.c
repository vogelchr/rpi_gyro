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
	struct lsm330dlc_acc_rdg acc[2];
	struct lsm330dlc_gyro_rdg gyro[2];
	struct lsm330dlc *chip[2];
	int i,j,ch;
	FILE *f;

	/* for udelay "calibration" */
	int delta_meas = 0;
	int got_data = 0;

	if(argc != 2){
		fprintf(stderr,"Usage: %s output_file\n",argv[0]);
		exit(1);
	}

	f = fopen(argv[1],"w");
	if(!f){
		perror(argv[1]);
		exit(1);
	}

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
	    got_data=0;
	    for(ch=0;ch<2;ch++){
		i=lsm330dlc_read_acc(chip[ch],&acc[ch]);
		if(i == -1)
			exit(1);

		i=lsm330dlc_read_gyro(chip[ch],&gyro[ch]);
		if(i == -1)
			exit(1);

		if(acc[ch].fss){
			fprintf(f,"ACC%d  %6d 0x%02x 0x%02x %2d %6d %6d %6d\n",ch,j,
				acc[ch].status, acc[ch].src, acc[ch].fss,
				acc[ch].acc[0], acc[ch].acc[1], acc[ch].acc[2]);
			got_data++;
		}

		if(gyro[ch].fss){
			fprintf(f,"GYRO%d  %6d 0x%02x 0x%02x %2d %6d %6d %6d\n",ch,j,
				gyro[ch].status, gyro[ch].src, gyro[ch].fss,
				gyro[ch].rot[0], gyro[ch].rot[1], gyro[ch].rot[2]);
			got_data++;
		}
	    }
	    j++;

		/* ----- this is a quick hack to optimize the proper udelay value ----- */
		if(got_data)
			delta_meas++;
		else
			delta_meas--;

		if(delta_meas < 2000)
			usleep(2000 - delta_meas);

		if (!( j & 63 )){
			printf("Fifos: %d %d %d %d, read/idle delta %d\n",
				acc[0].fss, acc[1].fss, gyro[0].fss, gyro[1].fss,delta_meas);
		}

	}

	exit(0);
}
