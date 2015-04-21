#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>

#include "mux_spi.h"
#include "lsm330dlc.h"

int main(int argc, char **argv)
{
	struct mux_spi *spi;
	struct mux_spi_single *singlespi[4];
	struct lsm330dlc_acc_rdg acc[2];
	struct lsm330dlc_gyro_rdg gyro[2];
	struct lsm330dlc *chip[2];
	time_t last_sec;

	/* for printing the time */
	struct timespec tsp;
	char tbuf[64];
	struct tm *stm;

	int i, j, ch;
	FILE *f;

	/* for udelay "calibration" */
	int sleep_value = 0;
	int max_fifo = 0;

	unsigned int gyro_cnt, acc_cnt;

	gyro_cnt=0;
	acc_cnt=0;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s output_file\n", argv[0]);
		exit(1);
	}

	f = fopen(argv[1], "w");
	if (!f) {
		perror(argv[1]);
		exit(1);
	}

	if (!(spi = mux_spi_open()))
		exit(1);

	for (i = 0; i < 2; i++) {
		for (j = 0; j < 2; j++) {
			singlespi[j + 2 * i] =
			    mux_spi_single_create(spi, i, j, 8, 0, 0);
		}
		chip[i] =
		    lsm330dlc_open(singlespi[2 * i + 0],
				   singlespi[2 * i + 1]);
		lsm330dlc_dump_regs(chip[i]);
	}

	if (clock_gettime(CLOCK_REALTIME, &tsp) == -1) {
		perror("clock_gettime(CLOCK_REALTIME)");
		exit(1);
	}
	last_sec = tsp.tv_sec;

	while (1) {

		if (clock_gettime(CLOCK_REALTIME, &tsp) == -1) {
			perror("clock_gettime(CLOCK_REALTIME)");
			exit(1);
		}

		stm = localtime(& tsp.tv_sec);
		j = strftime(tbuf, sizeof(tbuf)-1, "%H:%M:%S", stm);
		assert(j>0);

		snprintf(tbuf+j, sizeof(tbuf)-1-j, ".%09ld", tsp.tv_nsec);
		tbuf[sizeof(tbuf)-1]='\0';

		max_fifo = 0;
		for (ch = 0; ch < 2; ch++) {
			i = lsm330dlc_read_acc(chip[ch], &acc[ch]);
			if (i == -1)
				exit(1);

			i = lsm330dlc_read_gyro(chip[ch], &gyro[ch]);
			if (i == -1)
				exit(1);

			acc_cnt += acc[ch].fss;
			gyro_cnt += gyro[ch].fss;

			if (gyro[ch].fss >= 31)
				fprintf(stderr,"Gyro %d: fifo overflow with %d cnts!\n",ch, gyro[ch].fss);
			if (acc[ch].fss >= 31)
				fprintf(stderr,"Acc %d: fifo overflow with %d cnts!\n",ch, acc[ch].fss);

			if (acc[ch].fss > max_fifo)
				max_fifo = acc[ch].fss;
			if (gyro[ch].fss > max_fifo)
				max_fifo = gyro[ch].fss;

accel_again:
			if (acc[ch].fss) {
				fprintf(f,
					"ACC%d %s 0x%02x 0x%02x %2d %6d %6d %6d\n",
					ch, tbuf, acc[ch].status, acc[ch].src,
					acc[ch].fss, acc[ch].acc[0],
					acc[ch].acc[1], acc[ch].acc[2]);

				if (acc[ch].fss > 1) {
					i = lsm330dlc_read_acc(chip[ch], &acc[ch]);
					if (i == -1)
						exit(1);
					goto accel_again;
				}
			}

gyro_again:
			if (gyro[ch].fss) {
				fprintf(f,
					"GYR%d %s 0x%02x 0x%02x %2d %6d %6d %6d\n",
					ch, tbuf, gyro[ch].status,
					gyro[ch].src, gyro[ch].fss,
					gyro[ch].rot[0], gyro[ch].rot[1],
					gyro[ch].rot[2]);

				if (gyro[ch].fss > 1) {
					i = lsm330dlc_read_gyro(chip[ch], &gyro[ch]);
					if (i == -1)
						exit(1);
					goto gyro_again;
				}
			}
		}

		fprintf(f,"STAT %s %d %d %d %d\n",tbuf, gyro_cnt, acc_cnt, max_fifo, sleep_value);

		if (last_sec != tsp.tv_sec) {
			fprintf(stderr,"Gyro: %9u Acc: %9u MxF: %d Sleep: %d us\n",
				gyro_cnt, acc_cnt, max_fifo, sleep_value);
			gyro_cnt = 0;
			acc_cnt = 0;
			fflush(f);
		}
		last_sec = tsp.tv_sec;


		/* ----- this is a quick hack to optimize the proper udelay value ----- */
		if (max_fifo > 24 && sleep_value > 100)
			sleep_value -= 100;
		if (max_fifo < 8)
			sleep_value += 100;
		if (sleep_value > 0)
			usleep(sleep_value);
	}

	exit(0);
}
