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
#define CONTROL      BASE+9
#define PACER        BASE+10
#define COUNTER0     BASE+12
#define COUNTER1     BASE+13
#define COUNTER2     BASE+14
#define COUNTER_CTRL BASE+15

#define AD_LOW_BYTE_AND_CH BASE+0  //A utiliser dans ADRead
#define AD_HIGH_BYTE BASE+1        //A utiliser dans ADRead
#define AD_STATUS_REGISTER BASE+8

void setChannel(int in_channel);
void ADRangeSelect(int channel, int range);
u16 readAD(void);
double readAD_mVolt(void);


static int init_3718(void) { 

	printk("install 3718 driver\n");
	
        // CONTROL REGISTER
        // D7: INTE: Enable/Disable interrupts
        // D6-4 Enable/Disable interrupt INL0, INL1, INL3
        // D3: N/A
        // D2: DMAE: Enable DMA
        // D1-D0: Trigger source: Sfwr: 0X, Ext: 10, Pacer: 11  	
	outb(0xFF,CONTROL);
	
	// Program counters 1 and 2 as pacers to generate A/D conversions trigger pulses
	// 25kHz

  	// PACER ENABLE REGISTER
        // TC0: Enable/Disable pacer
        outb(0x00, PACER);
	
	// COUNTER CONTROL
	// Set Counter 1 to mode 3
	outb(0x76 , COUNTER_CTRL); 

	// COUNTER 1
	// Write low Byte of C1
	outb(40, COUNTER1);
	// Write high Byte of C1	
	outb(0, COUNTER1);
	
	// COUNTER CONTROL
	// Set Counter 2 to mode 3
	outb(0xB6 , COUNTER_CTRL);

	// COUNTER 2
	// Write low byte of C2
	outb(10, COUNTER2);
	// Write high byte of C2
	outb(0, COUNTER2);
	
	return(0);	

}//init_3718

void setChannel(int in_channel) {
	//on ne scan pas, les 4 msb = stop scan, les 4 lsb = start scan
	char value = (in_channel<<4) + in_channel;
	//printk("channel selectesfd: 0x%x (%d)\n", value);
	outb(value , MUX_CTRL);
	outb(0x1,BASE);

}//SetChanel

void ADRangeSelect(int channel, int range) {
	setChannel(channel);
	// on passe au channel suivant lorsqu'on écrit dans AD_RANGE_CTRL
	outb(range, AD_RANGE_CTRL);
	//on reset channel
	//setChannel(channel);

}//ADRangeSelect

u16 readAD(void) {

	// STATUS REGISTER
	// D7: End Of Conversion: 1 Busy, 0 Ready for next conversionm data from the previous conversion is available in the A/D register
	// D6: N/A
	// D5: MUX: 0 8 Diff channels; 1 16 Single Ended Channels
	// D4: INT Data valid //TODO
 	// D3-D0: CN3-CN0 When EOC = 0, these status bits contain the channel number of the next channel to be converted.
	u8 status_register = inb(AD_STATUS_REGISTER);
	int status_register_int = 0x00 | (status_register >> 4); // TODO: truncate	
	u8 result_low = inb(AD_LOW_BYTE_AND_CH);
	u8 result_high = inb(AD_HIGH_BYTE);
			
	u16 result =  0x0FFF & ((result_high<<4) + (result_low>>4));
	/*printk("LOW = 0x%x\n", result_low);
	printk("LOW_D7D4 = 0x%x\n", result_low>>4);
	printk("HIGH = 0x%x\n", result_high);
	printk("regToRead = 0x%x\n", result);
	printk("valeur= %d\n", result); 
	printk("channelAD= %d\n", result_low & 0x7); // on recup les 3 LSB	
        printk("status register= 0x%x\n", status_register);
	printk("status_register_int = %d\n", status_register_int);*/
	return result;
}//ReadAD

double readAD_mVolt(void) {

	return ((readAD()-2048)*1000)/205;

}
void exit_3718(void) {
  printk("uninstall 3718 driver\n");
}


EXPORT_SYMBOL(init_3718);
EXPORT_SYMBOL(exit_3718);
EXPORT_SYMBOL(setChannel);
EXPORT_SYMBOL(ADRangeSelect);
EXPORT_SYMBOL(readAD);
EXPORT_SYMBOL(readAD_mVolt);


module_init(init_3718);
module_exit(exit_3718);
