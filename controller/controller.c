// 0.0.3 2015-10-14
// controller.c
// Guillaume Legrain <guillaume.legrain@edu.esiee.fr>
// Florian Martin <florian.martin@edu.esiee.fr>
//
// main module to control pendulum

#include<linux/init.h>
#include<linux/module.h>
MODULE_LICENSE("GPL");

#include <asm/io.h>
#include <linux/pci.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>
#include <rtai_sem.h>
#include "matrix.h"
#include "3712.h"
#include "3718.h"


// Scheduling parameters
#define STACK_SIZE  2000
//#define PERIOD      100000000   //  100 ms // MATLAB peeriod = 0.010
#define PERIOD      1000000000  //  1s // for debug
#define TICK_PERIOD 1000000     //  1 ms
#define NB_LOOP     10          // number of times the task are executed


// Controller state matrices
matrix *Adc, *Bdc, *Cdc;
matrix *x, *u;

static RT_TASK sens_task, act_task, pid_task;
int ctrlcode(u16 currentAngle, u16 currentPosition);
SEM sensDone;
RTIME now; // tasks start time

int raw2mRad(int raw_value) {
  return 3.845e-1 * raw_value - 782;
}

void senscode(int arg) {
  static int loop = NB_LOOP;
  RTIME t, t_old;

  while (loop--) {
    t = rt_get_time();
    printk("[sens_task] time: %llu ns\n", count2nano(t - now));
    /* sensor acquisition code */
    ADRangeSelect(0,8);
    int currentAngle = raw2mRad(readAD());
    printk("angle = %d mRad\n", currentAngle);

    ADRangeSelect(1,8);
    int currentPosition = readAD();
    printk("position = %d\n", currentPosition);

    printk("\n");

    // signal end of acquisition, ready for control
    rt_sem_signal(&sensDone);

    /* end of sensor acquisition code */
    t_old = t;
    rt_task_wait_period();
  }
}

void actcode(int arg) {
  static int loop = NB_LOOP;
  RTIME t, t_old;

  while (loop--) {
    rt_sem_wait(&sensDone); // wait for sensor acquisition
    t = rt_get_time();
    printk("[act_task] time: %llu ms\n", count2nano(t - now));
    /* controller code */
    ctrlcode(0,0);
    /* end of controller code */
    t_old = t;
  }
}
static void init_matrices(void) {
  // Init controller state matrices
  Adc = newMatrix(4,4);
  setElement(Adc,1,1, 504); setElement(Adc, 1,2, 138); setElement(Adc, 1,3, 0); setElement(Adc, 1,4, 1);
  setElement(Adc,1,1, 147); setElement(Adc, 1,2, 486); setElement(Adc, 1,3, 0); setElement(Adc, 1,4, 1);
  setElement(Adc,1,1, 504); setElement(Adc, 1,2, 138); setElement(Adc, 1,3, 0); setElement(Adc, 1,4, 1);
  setElement(Adc,1,1, 504); setElement(Adc, 1,2, 138); setElement(Adc, 1,3, 0); setElement(Adc, 1,4, 1);

  Bdc = newMatrix(4,2);
  setElement(Bdc,1,1, 0); setElement(Bdc, 1,2, 0);
  setElement(Bdc,2,1, 0); setElement(Bdc, 2,2, 0);
  setElement(Bdc,3,1, 0); setElement(Bdc, 3,2, 0);
  setElement(Bdc,4,1, 0); setElement(Bdc, 4,2, 0);

  Cdc = newMatrix(1,4);
  setElement(Cdc,1,1, 0); setElement(Cdc, 1,2, 0); setElement(Cdc, 1,3, 0); setElement(Cdc, 1,4, 0);

  Ddc = newMatrix(1,2);
  setElement(Ddc,1,1, 0); setElement(Ddc, 1,2, 0);

  // Init ...
  x = newMatrix(4,4);
  u = newMatrix(1,1);
}

static int test_init(void) {
  int ierr;

  rt_set_oneshot_mode();

  printk("PGM STARTING\n");

  init_matrices();

  // Create real-time tasks
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

  // init semaphores
  rt_typed_sem_init(&sensDone,  // semaphore pointer
		                0,          // initial value
                    BIN_SEM);   // semaphore type

  if (!ierr) {

    start_rt_timer(nano2count(TICK_PERIOD));

    now = rt_get_time();

    // Start tasks
    rt_task_make_periodic(&sens_task, now,  nano2count(PERIOD));
    rt_task_resume(&act_task);

  }
  //return ierr;
  return 0; // pour ne pas faire planter le kernel
}


int ctrlcode(u16 currentAngle, u16 currentPosition){

  matrix *A = newMatrix(2,2);
  setElement(A,1,1,1);
  setElement(A,1,2,1);
  setElement(A,2,1,1);
  setElement(A,2,2,1);
  printMatrix(A);

  // temp matrices for operations
  matrix *tmp4x4_1 = newMatrix(4,4);
  matrix *tmp4x4_2 = newMatrix(4,4);

  // update y with current sensor readings
  setElement(y, 1, 1, currentAngle);
  setElement(y, 1, 2, currentPosition);

  // update state
  // x = Adc * x + Bdc * y
  product(Adc, x, tmp4x4_1);
  product(Adc, x, tmp4x4_1);
  sum(tmp4x4_1, tmp4x4_2, x);

  // update comand
  // u = Cdc * x // TODO: change sign
  product(Cdc, x, u);

  // delete allocated temp matrices
  delete(tmp4x4_1);
  delete(tmp4x4_2);

  return 0;
}//ctrlcode

void test_exit(void) {

  stop_rt_timer();

  // delete tasks
  rt_task_delete(&sens_task);
  rt_task_delete(&act_task);
  //rt_task_delete(&pid_task);

  // delete semaphores
  rt_sem_delete(&sensDone);

  // delete allocated matrices
  delete(Adc);
  delete(Bdc);
  delete(Cdc);
  delete(Bdc);
}

module_init(test_init);
module_exit(test_exit);
