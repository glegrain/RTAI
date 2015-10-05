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
	
	// write -9 V to channel 1 using DAC
	setDA(1,-9);
	// write -7 V to channel 2 using DAC
	setDA(2,-7);

        // Hack delay to wait for DAC response
	int i = 0;
	for (i = 0; i < 10000000; i++);

        //Confgure ADC to use channel 1 +/-10V
	ADRangeSelect(1,8);
	printk("res1 = %d mV\n", readAD_mVolt());

	////Confgure ADC to use channel 2 +/-10V
	ADRangeSelect(2,8);
	printk("res2 = %d\n mV", readAD_mVolt());

	return(0);
}

void test_exit(void) {
		
}

module_init(test_init);
module_exit(test_exit);
