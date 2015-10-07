// 0.0.0 2015-08-07


#include<linux/init.h>
#include<linux/module.h>
MODULE_LICENSE("GPL");

#include <asm/io.h>
#include <linux/pci.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>
#include "3712.h"
#include "3718.h"


// Scheduling parameters
#define STACK_SIZE  2000
#define PERIOD      100000000   //  100 ms // MATLAB peeriod = 0.010
#define TICK_PERIOD 1000000     //  1 ms

static RT_TASK sens_task, act_task, pid_task;

int raw2mRad(int raw_value) {
  return 3.845e-1 * raw_value - 782;
}

void senscode(int arg) {
  printk("[sens_task]\n");
}

void actcode(int arg) {
  printk("[act_task]\n");
}

static int test_init(void) {
  int ierr;
  RTIME now;

  rt_set_oneshot_mode();

  printk("\033[32mPGM STARTING\033[0m\n");

  ierr = rt_task_init_cpuid(&sens_task,  // task
                            senscode,    // rt_thread
                            0,           // data
                            STACK_SIZE,  // stack_size
                            3,           // priority
                            0,           // uses_fpu
                            0,           // signal
                            0);          // cpuid

  ierr = rt_task_init_cpuid(&act_task,   // task
                            actcode,     // rt_thread
                            0,           // data
                            STACK_SIZE,  // stack_size
                            4,           // priority
                            0,           // uses_fpu
                            0,           // signal
                            0);          // cpuid


  if (!ierr) {

    start_rt_timer(nano2count(TICK_PERIOD));

    now = rt_get_time();

    // Start tasks
    rt_task_make_periodic(&sens_task, now,  nano2count(PERIOD));
    //rt_task_resume(&act_task);

  }
  //return ierr;
  return 0; // pour ne pas faire planter le kernel	
}

void test_exit(void) {
  
  stop_rt_timer();

  rt_task_delete(&sens_task);
  rt_task_delete(&act_task);
  //rt_task_delete(&pid_task);
}

module_init(test_init);
module_exit(test_exit);
