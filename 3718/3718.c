// 0.0.4 2015-10-14
// 3718.c
// Guillaume Legrain <guillaume.legrain@edu.esiee.fr>
// Florian Martin <florian.martin@edu.esiee.fr>
//
// PC104 Analog-to-Digital  ADVANTECH PCM-3718 Driver

#include <linux/init.h>
#include <linux/module.h>
MODULE_LICENSE("GPL");

#include <asm/io.h>
#include <linux/pci.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>

// BASE register address for ADVANTECH PCM-3718HG PC1-4
// Should match hardware jumper settings
#define BASE 0x320

// Write registers
#define AD_RANGE_CTRL BASE+1
#define MUX_CTRL      BASE+2
#define CONTROL       BASE+9
#define PACER         BASE+10
#define COUNTER0      BASE+12
#define COUNTER1      BASE+13
#define COUNTER2      BASE+14
#define COUNTER_CTRL  BASE+15

// Read registers
#define AD_LOW_BYTE_AND_CH BASE+0
#define AD_HIGH_BYTE       BASE+1
#define AD_STATUS_REGISTER BASE+8

void setChannel(int in_channel);
void ADRangeSelect(int channel, int range);
u16 readAD(void);
double readAD_mVolt(void);
void configure_pacer(void);


/**
 * Driver initilisation
 */
static int init_3718(void) {
    printk("install 3718 driver\n");

    // CONTROL REGISTER
    // D7: INTE: Enable/Disable interrupts
    // D6-4 Enable/Disable interrupt INL0, INL1, INL3
    // D3: N/A
    // D2: DMAE: Enable DMA
    // D1-D0: Trigger source: Sfwr: 0X, Ext: 10, Pacer: 11
    outb(0xFF,CONTROL);

    configure_pacer();

    return(0);
}
EXPORT_SYMBOL(init_3718);

/**
 * Configure pacer
 * to generate A/D conversions trigger pulses.
 * Program counters 1 and 2 as pacers
 * 25kHz
 */
void configure_pacer(void) {
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
}

/**
 * Set channel to perform A/D conversion
 * @arg int channel (0-3)
 */
void setChannel(int in_channel) {
    // Disable channel scanning by setting the start and stop channel
    // to the same value
    // 4 MSB = scan stop; 4 LSB = start scan
    char value = (in_channel<<4) + in_channel;
    outb(value , MUX_CTRL);
    // Write anything to BASE to trigger A/D conversion
    // removed because A/D conversion is trigger by the pacer
    //outb(0x1,BASE);

    // wait for channel change
    //u8 result_low_channel= inb(AD_LOW_BYTE_AND_CH) & 0x7;
    //while ((int)result_low_channel != in_channel) {
    //printk("waiting for channel change\n");
    //result_low_channel = inb(AD_STATUS_REGISTER) & 0x7;
    //} // Not working, prink or delay hack works ?????
    // printk("Channel changed\n");
    int i = 0;
    for (i = 0; i < 100000; i++); // delay hack to wait for channel change

}
EXPORT_SYMBOL(setChannel);

/**
 * Set input range and channel
 * @arg int channel (0-3)
 * @arg int range see datasheet for values; 8 for +/- 10V
 */
void ADRangeSelect(int channel, int range) {
    setChannel(channel);
    // According to the datasheet, the MUX start/stop channel changes each time
    // when you change the input range.
    outb(range, AD_RANGE_CTRL);
    // reset channel to prevent channel change after wrting in AD_RANGE_CTRL
    // NOTE: disabled because start channel = stop channel in setChannel()
    // setChannel(channel);
}
EXPORT_SYMBOL(ADRangeSelect);

/**
 * Read ADC register value
 * on the channel selected by ADRangeSelect
 *
 * @return u16 voltage (0-4096)
 */
u16 readAD(void) {
    // STATUS REGISTER
    // D7: End Of Conversion:
    //   1 Busy,
    //   0 Ready for next conversion data from the previous
    //     conversion is available in the A/D register
    // D6: N/A
    // D5: MUX: 0 8 Diff channels; 1 16 Single Ended Channels
    // D4: INT Data valid
    // D3-D0: CN3-CN0 When EOC = 0,
    //
    // these status bits contain the channel number of the next
    // channel to be converted.
    u8 status_register = inb(AD_STATUS_REGISTER);
    int status_register_int = 0x01 & (status_register >> 4);
    u8 result_low = inb(AD_LOW_BYTE_AND_CH);
    u8 result_high = inb(AD_HIGH_BYTE);

    // return -1 if no A/D conversion hqs been completed since the
    // last time the INT bit was cleared.
    if (status_register_int != 1) {
      return -1;
    }

    u16 result =  0x0FFF & ((result_high<<4) + (result_low>>4));
    //printk("LOW = 0x%x\n", result_low);
    //printk("LOW_D7D4 = 0x%x\n", result_low>>4);
    //printk("HIGH = 0x%x\n", result_high);
    //printk("regToRead = 0x%x\n", result);
    //printk("valeur= %d\n", result);
    //printk("channelAD= %d\n", result_low & 0x7); // on recup les 3 LSB
    //printk("status register= 0x%x\n", status_register);
    //printk("status_register_int = 0x%x\n", status_register_int);
    return result;
}
EXPORT_SYMBOL(readAD);

/**
 * Read voltage in mVolts
 * on the channel selected by ADRangeSelect.
 * @note: assuming range is +/-10V
 *
 * @return double voltage (mVolts)
 */
double readAD_mVolt(void) {
    return ((readAD()-2048)*1000)/205;
}
EXPORT_SYMBOL(readAD_mVolt);

/**
 * Uninstall driver module
 */
void exit_3718(void) {
    printk("uninstall 3718 driver\n");
}
EXPORT_SYMBOL(exit_3718);


module_init(init_3718);
module_exit(exit_3718);
