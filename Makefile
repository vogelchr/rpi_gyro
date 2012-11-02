REMOTE_HOST = chris@rpi
REMOTE_DIR = rpi_gyro
CPPFLAGS=
CFLAGS=-Wall -Wextra -Os -ggdb

all : rpi_gyro

OBJS = rpi_gyro.o mux_spi.o sysfs_gpio.o lsm330dlc.o

rpi_gyro : $(OBJS)

ifneq ($(MAKECMDGOALS),clean)
include $(OBJS:.o=.d)
endif

%.d : %.c
	$(CC) $(CPPFLAGS) -o $@ -MM $<

.PHONY : clean remote
clean :
	rm -f *~ *.o *.d rpi_gyro

# first compile locally, only if successful make on target
remote : all
	rsync -av *.c *.h Makefile $(REMOTE_HOST):$(REMOTE_DIR)
	ssh $(REMOTE_HOST) cd $(REMOTE_DIR) \&\& make

