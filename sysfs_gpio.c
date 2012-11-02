#include "mux_spi.h"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static const char *GPIO_DIR_FMT = "/sys/class/gpio/gpio%d";
static const char *GPIO_EXPORT = "/sys/class/gpio/export";

struct sysfs_gpio {
	int ngpio;
	int dir_fd;
	int value_fd;
	int direction_fd;
};

static int export_gpio(int ngpio){
	struct stat ss;
	char gpio_dir[32],nstr[16];
	int nlen,fd;

	sprintf(gpio_dir,GPIO_DIR_FMT,ngpio);
	nlen = sprintf(nstr,"%d",ngpio);

	if(stat(gpio_dir,&ss) == 0)
		return 0;

	if(errno != ENOENT){
		perror(gpio_dir);
		return -1;
	}

	fd = open(GPIO_EXPORT,O_WRONLY);
	if(fd == -1){
		perror(GPIO_EXPORT);
		return -1;
	}

	if(write(fd,nstr,nlen) != nlen){
		close(fd);
		perror(GPIO_EXPORT);
		return -1;
	}

	close(fd);

	if(stat(gpio_dir,&ss) == 0)
		return 0;
	return -1;
}

void
sysfs_gpio_close(struct sysfs_gpio *p){
	if(p->dir_fd != -1)
		close(p->dir_fd);
	if(p->value_fd != -1)
		close(p->value_fd);
	if(p->direction_fd != -1)
		close(p->direction_fd);
	free(p);
}       

struct sysfs_gpio *
sysfs_gpio_open(int ngpio){
	struct sysfs_gpio *p;
	char dirname[32];

	if(export_gpio(ngpio))
		return NULL;

	p = calloc(1,sizeof(struct sysfs_gpio));
	p->ngpio = ngpio;
	p->dir_fd = p->value_fd = p->direction_fd = -1;

	sprintf(dirname,GPIO_DIR_FMT,ngpio);
	p->dir_fd = open(dirname,O_RDONLY);
	if(-1 == p->dir_fd){
		perror(dirname);
		free(p);
		return NULL;
	}

	return p;
}

extern int
sysfs_gpio_value(struct sysfs_gpio *p, int value){
	char strval = '0' + !!value; /* '0' or '1' */

	if(p->value_fd == -1){
		p->value_fd = openat(p->dir_fd,"value",O_RDWR);
		fprintf(stderr,"Opened gpio%d value as fd %d.\n",
			p->ngpio,p->value_fd);
	}
	if(p->value_fd == -1){
		perror("value");
		fprintf(stderr,"(for gpio %d)\n",p->ngpio);
		return -1;
	}
	if(write(p->value_fd,&strval,1) != 1){
		perror("write()");
		return -1;
	}
	return 0;
}

extern int
sysfs_gpio_get_value(struct sysfs_gpio *p, int *value){
	char strval;

	if(p->value_fd == -1){
		p->value_fd = openat(p->dir_fd,"value",O_RDWR);
		fprintf(stderr,"Opened gpio%d value as fd %d.\n",
			p->ngpio,p->value_fd);
	}
	if(p->value_fd == -1){
		perror("value");
		fprintf(stderr,"(for gpio %d)\n",p->ngpio);
		return -1;
	}
	if(read(p->value_fd,&strval,1) != 1){
		perror("read()");
		return -1;
	}

	if(strval == '0')
		*value = 0;
	if(strval == '1')
		*value = 1;

	return 0;
}


extern int
sysfs_gpio_direction(struct sysfs_gpio *p, int inout){
	static const char *words[2]={ "in", "out" };
	static const int   len[2]={ 2, 3 };

	inout = !!inout; /* only 0 or 1 */

	if(p->direction_fd == -1){
		p->direction_fd = openat(p->dir_fd,"direction",O_RDWR);
		fprintf(stderr,"Opened gpio%d direction as fd %d.\n",
			p->ngpio,p->direction_fd);
	}
	if(p->direction_fd == -1){
		perror("direction");
		fprintf(stderr,"(for gpio %d)\n",p->ngpio);
		return -1;
	}

	if(write(p->direction_fd,words[inout],len[inout]) != len[inout]){
		perror("direction");
		return -1;
	}

	return 0;
}
