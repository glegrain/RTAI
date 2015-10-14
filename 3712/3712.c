// 0.0.4 2015-10-14
// 3712.c
// Guillaume Legrain <guillaume.legrain@edu.esiee.fr>
// Florian Martin <florian.martin@edu.esiee.fr>
//
// PC104 Digital-to-Analog  ADVANTECH PCM-3712 Driver

#include <linux/init.h>
#include <linux/module.h>
MODULE_LICENSE("GPL");

#include <asm/io.h>
#include <linux/pci.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>

// BASE register address for ADVANTECH PCM-3712 PC104
// Should match hardware jumper settings
#define BASE 0x300

#define DA_LOW_BYTE_CH1  BASE+0
#define DA_HIGH_BYTE_CH1 BASE+1
#define DA_LOW_BYTE_CH2  BASE+2
#define DA_HIGH_BYTE_CH2 BASE+3
#define SYNC_TRANS_CTRL  BASE+4
#define OUTPUT_CTRL      BASE+5

#define VREF 10

void setDA_mVolt(int, int);
void setDA_raw(int, int);

/**
 * Driver initilisation
 * Reset output to 0V and enable DAC
 */
static int init_3712(void) {
    printk("install 3712 driver\n");

    // reset output value for both channel to 0V
    setDA_mVolt(1,0);
    setDA_mVolt(2,0);

    //enable output
    outb(0xFF, OUTPUT_CTRL);

    return(0);
}
EXPORT_SYMBOL(init_3712);

/**
 * Set output voltage using raw register values
 * @arg int channel output channel (1-2)
 * @arg int value DAC register value (0x00-0xFF)
 */
void setDA_raw(int channel, int value){

    // First 8 LSB (D7-D0) from value
    char value_low = value & 0xFF;
    // Remaining 8 MSB from value
    char value_high = value >> 8;

    // Write desired value into registers based on selected channel
    if (channel == 1) {
        outb(value_low, DA_LOW_BYTE_CH1);
        outb(value_high, DA_HIGH_BYTE_CH1);
    } else if (channel == 2) {
        outb(value_low, DA_LOW_BYTE_CH2);
        outb(value_high, DA_HIGH_BYTE_CH2);
    } else {
        printk("ERROR: invalid channel");
        return;
    }
    //update DAC output value // Looks like this has no effect
    //outb(0xFF, SYNC_TRANS_CTRL);
}
EXPORT_SYMBOL(setDA_raw);

/**
 * Set output voltage in mVolts
 * @arg int channel output channel (1-2)
 * @arg int value_mVolt desired voltage (+/- 10000mV)
 */
void setDA_mVolt(int channel, int value_mVolt) {
    // Limit voltage to +/- 10V
    if (value_mVolt >= 10000) {
        setDA_raw(channel, 0xFFF);
        return;
    }
    if (value_mVolt <= -10000) {
        setDA_raw(channel, 0x000);
        return;
    }

    // convert mVolt value to register value
    int value = (2.048*value_mVolt / VREF) + 2048;
    setDA_raw(channel, value);
}
EXPORT_SYMBOL(setDA_mVolt);

/**
 * Uninstall driver module
 * Disable DAC output
 */
void exit_3712(void) {
    //disable output
    outb(0x00, OUTPUT_CTRL);
    printk("uninstall 3712 driver\n");

}
EXPORT_SYMBOL(exit_3712);

module_init(init_3712);
module_exit(exit_3712);
