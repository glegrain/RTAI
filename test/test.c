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

int raw2mRad(int raw_value) {
  return 3.845e-1 * raw_value - 782;
}

static int test_init(void) {
	
	// write -9 V to channel 1 using DAC
	setDA_mVolt(1,-3000);
	// write -7 V to channel 2 using DAC
	setDA_mVolt(2,-7000);


	//while(1){

	// Hack delay to wait for DAC response
	//int i = 0;
	//for (i = 0; i < 5000000; i++);

	printk("\n");
        //Configure ADC to use channel 1 +/-10V
	ADRangeSelect(0,8);
	printk("res0 en= %d mV\n", readAD_mVolt());
	printk("res0 = %d\n", readAD());
        printk("angle = %d (mRad)\n", raw2mRad(readAD()));
	
	printk("\n");
	////Confgure ADC to use channel 2 +/-10V
	ADRangeSelect(1,8);
	printk("res1 = %d mV\n", readAD_mVolt());
	printk("res1 = %d\n", readAD());
	//	}
	return(0);
}

void test_exit(void) {
		
}

module_init(test_init);
module_exit(test_exit);
