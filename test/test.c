// 0.0.2 2015-09-14

#include<linux/init.h>
#include<linux/module.h>

#include <asm/io.h>
#include <linux/pci.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>
#include "3712.h"
#include "3718.h"

static int test_init(void) {
	
	// write 10 V to channel 1 using DAC
	setDA(1,10);
	// write 7 V to channel 7 using DAC
	setDA(2,7);

	//Confgure ADC to use channel 1 +/-10V
	ADRangeSelect(1,8);
	printk("res = %d\n", readAD());

	//Confgure ADC to use channel 2 +/-10V
	ADRangeSelect(2,8);
	printk("res = %d\n", readAD());

	return(0);
}

void test_exit(void) {
		
}

module_init(test_init);
module_exit(test_exit);
