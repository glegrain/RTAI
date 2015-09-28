// 0.0.2 2015-09-14

#include<linux/init.h>
#include<linux/module.h>

#include <asm/io.h>
#include <linux/pci.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>
#include "3718.h"

static int test_init(void) {
	
	int ierr;
        ADRangeSelect(1,8);
	double res = readAD();
	printk("res = %d\n", res);
	res = readAD();
	printk("res = %d\n", res);
	res = readAD();
	printk("res = %d\n", res);
	return(0);
}

void test_exit(void) {
		
}

module_init(test_init);
module_exit(test_exit);
