// 0.0.2 2015-09-14

#include<linux/init.h>
#include<linux/module.h>

#include <asm/io.h>
#include <linux/pci.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>

/* define pour gestion PCI CARTE ADVANTECH 3718HG PC104 */
#define BASE 0x320
#define AD_RANGE_CTRL BASE+1       //A utiliser dans ADRangeselect  
#define MUX_CTRL BASE+2            //A utiliser dans setChannel 

#define AD_LOW_BYTE_AND_CH BASE+0  //A utiliser dans ADRead
#define AD_HIGH_BYTE BASE+1        //A utiliser dans ADRead
#define AD_STATUS_REGISTER BASE+8

void setChannel(int in_channel);
void ADRangeSelect(int channel, int range);
u16 readAD(void);

static int init_3718(void) { 

	printk("install 3718 driver\n");
	return(0);	

}//init_3718

void setChannel(int in_channel) {
	//on ne scan pas, les 4 msb = stop scan, les 4 lsb = start scan
	outb((in_channel<<4) + in_channel , MUX_CTRL);
	outb(0x1,BASE);

}//SetChanel

void ADRangeSelect(int channel, int range) {
	// on passe au channel suivant lorsqu'on écrit dans AD_RANGE_CTRL
	outb(range, AD_RANGE_CTRL);
	//on reset channel
	setChannel(channel);

}//ADRangeSelect

u16 readAD(void) {
	printk("\n\n READ \n");
	char result_low = inb(AD_LOW_BYTE_AND_CH);
	char result_high = inb(AD_HIGH_BYTE);
	char result_status = inb(AD_STATUS_REGISTER);		
	u16 result = (result_high<<8) + (result_low>>3);
	//printk("valeur= %d\n", (result_high<<8) + (result_low>>3));
	//printk("channelAD= %d\n", result_low & 0x7); // on recup les 3 LSB	
        //printk("status register= 0x%x\n", result_status);
	return result;
}//ReadAD

void exit_3718(void) {
  printk("uninstall 3718 driver\n");
}


EXPORT_SYMBOL(init_3718);
EXPORT_SYMBOL(exit_3718);
EXPORT_SYMBOL(setChannel);
EXPORT_SYMBOL(ADRangeSelect);
EXPORT_SYMBOL(readAD);


module_init(init_3718);
module_exit(exit_3718);
