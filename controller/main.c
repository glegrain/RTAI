// 0.0.4 2015-10-20
// main.c
// Guillaume Legrain <guillaume.legrain@edu.esiee.fr>
// Florian Martin <florian.martin@edu.esiee.fr>
//
// main module to control pendulum

#include <linux/init.h>
#include <linux/module.h>
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
#define PERIOD      100000000   //  100 ms // MATLAB peeriod = 0.010
//#define PERIOD      50000000   //  50 ms
//#define PERIOD      1000000000  //  1s // for debug
#define TICK_PERIOD 1000000     //  1 ms

// Controller state matrices
matrix *Adc, *Bdc, *Cdc, *Ddc;
matrix *x, *y, *u;
matrix *tmp4x4_1, *tmp4x4_2;

float currentAngle, currentPosition;

static RT_TASK sens_task, act_task, pid_task;
int ctrlcode(float currentAngle_rad, float currentPosition_volt);
SEM sensDone;
RTIME now; // tasks start time

int raw2mRad(u16 raw_value) {
    return 3.845e-1 * raw_value - 782;
}

int raw2mVolt(u16 raw_value) {
    return (( (int) readAD() - 2048)*1000)/204.8;
}

void senscode(int arg) {
  RTIME t, t_old;

    while (1) {
        t = rt_get_time();
        printk("[sens_task] time: %llu ns\n", count2nano(t - now));
        /* sensor acquisition code */
        ADRangeSelect(0,8);
        currentAngle = raw2mRad(readAD()) / 1000.0;
        printk("angle = %d mRad\n", (int) currentAngle);

        ADRangeSelect(1,8);
        currentPosition = raw2mVolt(readAD()) / 1000.0;
        printk("position = %d\n", (int) currentPosition);

        printk("\n");

        // signal end of acquisition, ready for control
        rt_sem_signal(&sensDone);

        /* end of sensor acquisition code */
        t_old = t;
        rt_task_wait_period();
    }
}

void actcode(int arg) {
  RTIME t, t_old;

    while (1) {
        rt_sem_wait(&sensDone); // wait for sensor acquisition
        t = rt_get_time();
        printk("[act_task] time: %llu ms\n", count2nano(t - now));
        /* controller code */
        ctrlcode(currentAngle, currentPosition);
        //ctrlcode(0.1, 0.0);
        /* end of controller code */
        t_old = t;
    }
}
static void init_matrices(void) {
    // Init controller state matrices
    // Adc = newMatrix(4,4);
    // setElement(Adc,1,1,   619); setElement(Adc, 1,2,    96); setElement(Adc, 1,3,   -1); setElement(Adc, 1,4,   8);
    // setElement(Adc,1,1,    97); setElement(Adc, 1,2,   703); setElement(Adc, 1,3,   10); 0, 0x0setElement(Adc, 1,4,   1);
    // setElement(Adc,1,1,  1812); setElement(Adc, 1,2, -1800); setElement(Adc, 1,3, 1130); setElement(Adc, 1,4, 235);
    // setElement(Adc,1,1, -3884); setElement(Adc, 1,2,   872); setElement(Adc, 1,3, -155); setElement(Adc, 1,4, 722);

    // Bdc = newMatrix(4,2);
    // setElement(Bdc,1,1,   376); setElement(Bdc, 1,2,  -98);
    // setElement(Bdc,2,1,   -94); setElement(Bdc, 2,2,  296);
    // setElement(Bdc,3,1, -1014); setElement(Bdc, 3,2, 1895);
    // setElement(Bdc,4,1,  3053); setElement(Bdc, 4,2, -986);

    // Cdc = newMatrix(1,4);
    // setElement(Cdc,1,1, -80310); setElement(Cdc, 1,2, -9624); setElement(Cdc, 1,3, -14122); setElement(Cdc, 1,4, -23626);

    Adc = newMatrix(4,4);
    setElement(Adc,1,1,  0.61963); setElement(Adc, 1,2,  0.09677); setElement(Adc, 1,3, -0.00077); setElement(Adc, 1,4, 0.00861);
    setElement(Adc,2,1,  0.09708); setElement(Adc, 2,2,  0.70384); setElement(Adc, 2,3,  0.01065); setElement(Adc, 2,4, 0.00118);
    setElement(Adc,3,1,  1.81243); setElement(Adc, 3,2, -1.79966); setElement(Adc, 3,3,  1.13056); setElement(Adc, 3,4, 0.23508);
    setElement(Adc,4,1, -3.88366); setElement(Adc, 4,2,  0.87240); setElement(Adc, 4,3, -0.15461); setElement(Adc, 4,4, 0.72219);

    // printk("Adc init:\n");
    // printMatrix(Adc);
    // printk("dim Adc: %d x %d\n", Adc->rows, Adc->cols);
    // float tmp;
    // getElement(Adc, 1,1, &tmp);
    // printk("Adc 1_1: %d\n", (int) (tmp*1000));

    Bdc = newMatrix(4,2);
    setElement(Bdc,1,1,  0.37621); setElement(Bdc, 1,2, -0.09734);
    setElement(Bdc,2,1, -0.09308); setElement(Bdc, 2,2,  0.29664);
    setElement(Bdc,3,1, -1.01334); setElement(Bdc, 3,2,  1.89542);
    setElement(Bdc,4,1,  3.05338); setElement(Bdc, 4,2, -0.98579);

    Cdc = newMatrix(1,4);
    setElement(Cdc,1,1, -80.30915); setElement(Cdc, 1,2, -9.62374); setElement(Cdc, 1,3, -14.12152); setElement(Cdc, 1,4, -23.62599);

    Ddc = newMatrix(1,2);
    setElement(Ddc,1,1, 0.0); setElement(Ddc, 1,2, 0.0);

    // Init ...
    x = newMatrix(4,1);
    y = newMatrix(2,1);
    u = newMatrix(1,1);

    // temp matrices for operations
    tmp4x4_1 = newMatrix(4,1);
    tmp4x4_2 = newMatrix(4,1);
}

static int test_init(void) {
    int ierr;

    rt_set_oneshot_mode();

    printk("PGM STARTING\n");
    float testVar = 1.234;
    printk("testVar = %d, 0x%x\n", (int) testVar, testVar);

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


int ctrlcode(float currentAngle_rad, float currentPosition_volt){
    // update y with current sensor readings
    setElement(y, 1, 1, currentAngle_rad);
    setElement(y, 2, 1, currentPosition_volt);

    // update state
    // eq: x = Adc * x + Bdc * y
    product(Adc, x, tmp4x4_1);
    product(Bdc, y, tmp4x4_2);
    sum(tmp4x4_1, tmp4x4_2, x);

    // update comand
    // eq: u = Cdc * x // TODO: change sign
    product(Cdc, x, u);

    //printk("Adc: \n");
    //printMatrix(Adc);
    //printk("Bdc: \n");
    //printMatrix(Bdc);
    //printk("Cdc: \n");
    //printMatrix(Cdc);
    // printk("y: \n");
    // printMatrix(y);
    // printk("x: \n");
    // printMatrix(x);
    // printk("u: \n");
    // printMatrix(u);

    // convert command matrix u to scalar in mVolt
    float command_mVolt = - u->data[0] * 1000;
    //getElement(u, 1, 1, &command);
    printk("Command u = %d, 0x%x\n", (int) command, command);

    // send command
    setDA_mVolt(1, command);

    printk("\n");

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
    deleteMatrix(Adc);
    deleteMatrix(Bdc);
    deleteMatrix(Cdc);
    deleteMatrix(Ddc);
    deleteMatrix(x);
    deleteMatrix(y);
    deleteMatrix(u);
    deleteMatrix(tmp4x4_1);
    deleteMatrix(tmp4x4_2);
}

module_init(test_init);
module_exit(test_exit);
