// 0.0.2 2015-09-14

#include<linux/init.h>
#include<linux/module.h>

#include <asm/io.h>
#include <linux/pci.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>

/* define pour gestion PCI CARTE ADVANTECH PCM-3712 PC104 */
#define BASE 0x300
#define DA_LOW_BYTE_CH1 BASE+0
#define DA_HIGH_BYTE_CH1 BASE+1
#define DA_LOW_BYTE_CH2 BASE+2
#define DA_HIGH_BYTE_CH2 BASE+3
#define SYNC_TRANS_CTRL BASE+4
#define OUTPUT_CTRL BASE+5

#define VREF 10

void setDA(int, int);
void setDA_raw(int, int);

static int init_3712(void) { 

	printk("install 3712 driver\n");
	// reset output value for both channel 0V
	setDA(1,0);
	setDA(2,0);

	//enable output
	outb(0xFF, OUTPUT_CTRL);
	return(0);	

}//init_3712

void setDA_raw(int channel, int value){

	// On récupère de D0 à D7
	char value_low = value & 0xFF;
	// On récupère de D8 à D11
	char value_high = value >> 8;
	
	// On écrit dans les registres en fonction du channel	
	if (channel == 1) {
		outb(value_low, DA_LOW_BYTE_CH1);
		outb(value_high, DA_HIGH_BYTE_CH1);
	} else if (channel == 2) {
		outb(value_low, DA_LOW_BYTE_CH2);
		outb(value_high, DA_HIGH_BYTE_CH2);
	} else {
		printk("ERROR: Channel non valide");
		return;
	}
	//update DA output value
	//outb(0xFF, SYNC_TRANS_CTRL);
}//setDA

void setDA(int channel, int value_volts) {
	// on borne le domaine de valeurs a +/-10V
	if (value_volts >= 10) {
		setDA_raw(channel, 0xFFF);
		return;
	}
	if (value_volts <= 10) {
		setDA_raw(channel, 0x000);
		return;
	}
	
	int value = (2048*value_volts / VREF) + 2048;
	setDA_raw(channel, value);	
}

void exit_3712(void) {
	
	//disable output
	outb(0x00, OUTPUT_CTRL);
	printk("uninstall 3712 driver\n");

}

EXPORT_SYMBOL(setDA);
EXPORT_SYMBOL(setDA_raw);
EXPORT_SYMBOL(init_3712);
EXPORT_SYMBOL(exit_3712);


module_init(init_3712);
module_exit(exit_3712);
