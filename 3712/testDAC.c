// 0.0.2 2015-09-14

#include<linux/init.h>
#include<linux/module.h>

#include <asm/io.h>
#include <linux/pci.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>
#include "3712.h"

static int test_init(void) {
	
	int ierr;
	setDA(1,10);
	setDA(2,-15);
	return(0);
}

void test_exit(void) {
		
}

module_init(test_init);
module_exit(test_exit);
