#ifndef SYSFS_GPIO
#define SYSFS_GPIO

/* simple frontend to control a gpio via sysfs */

struct sysfs_gpio ;

/* export the gpio to sysfs, open directory */
extern struct sysfs_gpio *
sysfs_gpio_open(int ngpio);

/* close everything */
extern void
sysfs_gpio_close(struct sysfs_gpio *p);

/* set direction  1=output */
extern int
sysfs_gpio_direction(struct sysfs_gpio *p,int inout);

/* set value */
extern int
sysfs_gpio_value(struct sysfs_gpio *p, int val);

/* get value */
extern int
sysfs_gpio_get_value(struct sysfs_gpio *p, int *val);

#endif
