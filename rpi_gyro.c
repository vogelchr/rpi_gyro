#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mux_spi.h"
#include "lsm330dlc.h"

int
main(int argc,char **argv){
	struct mux_spi *spi;
	int i,j;

	if(!(spi = mux_spi_open()))
		exit(1);

	for(i=0;i<2;i++){
		for(j=0;j<2;j++){
			printf("--- now on SPI 0x%08x/%d ---\n",i,j);
			lsm330dlc_dump_regs(spi,i,j);

		}
	}

	exit(0);
}
