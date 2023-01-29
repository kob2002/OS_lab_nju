
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"



int state[5]; //用于记录状态
int restTime[5];//5个人的休息时间
PRIVATE void init_tasks()
{
	init_screen(tty_table);
	clean(console_table);


	//初始赋予1次机会
	for (int i = 0; i < 7; ++i) {
        proc_table[i].ticks    = 1;
        proc_table[i].priority = 1;
	}

	// initialization
	k_reenter = 0;
	ticks = 0;
	readers = 0;
	writers = 0;
	writing = 0;


//0: fair 1: rf 2:wf
	strategy = 1; // 切换策略



    if(strategy==2){
        //如果写者优先，则把写者的优先级提高
        proc_table[4].ticks=2;
        proc_table[4].priority = 2;

        proc_table[5].ticks=2;
        proc_table[5].priority = 2;
    }

    //休息时间设置
    restTime[0]=0;
    restTime[1]=0;
    restTime[2]=0;
    restTime[3]=0;
    restTime[4]=0;

	p_proc_ready = proc_table;
}
/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	TASK*		p_task		= task_table;
	PROCESS*	p_proc		= proc_table;
	char*		p_task_stack	= task_stack + STACK_SIZE_TOTAL;
	u16		selector_ldt	= SELECTOR_LDT_FIRST;
	int i;
    u8              privilege;
    u8              rpl;
    int             eflags;
	for (i = 0; i < NR_TASKS + NR_PROCS; i++) {
        if (i < NR_TASKS) {     /* 任务 */
                        p_task    = task_table + i;
                        privilege = PRIVILEGE_TASK;
                        rpl       = RPL_TASK;
                        eflags    = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1 */
                }
                else {                  /* 用户进程 */
                        p_task    = user_proc_table + (i - NR_TASKS);
                        privilege = PRIVILEGE_USER;
                        rpl       = RPL_USER;
                        eflags    = 0x202; /* IF=1, bit 2 is always 1 */
                }
                
		strcpy(p_proc->p_name, p_task->name);	// name of the process
		p_proc->pid = i;			// pid
		p_proc->sleeping = 0; // 初始化结构体新增成员
		p_proc->blocked = 0;
		
		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | privilege << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | privilege << 5;
		p_proc->regs.cs	= (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ds	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.es	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.fs	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ss	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = eflags;

		p_proc->nr_tty = 0;

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

    for(int i=0;i<5;i++){
        state[i]=0;
    }
	init_tasks();
	init_clock();
    init_keyboard();
	restart();

	while(1){}
}

PRIVATE read(char proc, int slices){
    switch (proc) {
        case 'B':
            state[0]=1;
            break;
        case 'C':
            state[1]=1;
            break;
        case 'D':
            state[2]=1;
            break;
        case 'E':
            state[3]=1;
            break;
        case 'F':
            state[4]=1;
            break;
    }
	sleep_ms(slices * TIME_SLICE); // 读耗时slices个时间片


}

PRIVATE	write(char proc, int slices){
    switch (proc) {
        case 'B':
            state[0]=1;
            break;
        case 'C':
            state[1]=1;
            break;
        case 'D':
            state[2]=1;
            break;
        case 'E':
            state[3]=1;
            break;
        case 'F':
            state[4]=1;
            break;
    }
	sleep_ms(slices * TIME_SLICE); // 写耗时slices个时间片

}

//读写公平方案
void read_fair(char proc, int slices){

	P(&queue); //排队



	P(&r_mutex); //针对readers的锁

	if (readers==0)
		P(&rw_mutex); // 有读者正在使用，写者不可抢占
	readers++;
	V(&r_mutex);

	V(&queue);
    //进行读操作
    P(&maxReaderMutex); //最大读者锁

	read(proc, slices);

    V(&maxReaderMutex);

	P(&r_mutex);
	readers--;
	if (readers==0)
		V(&rw_mutex); // 没有读者时，可以开始写了
	V(&r_mutex);


}

void write_fair(char proc, int slices){

	P(&queue);
	P(&rw_mutex);

	V(&queue);
	// 写过程
	write(proc, slices);

	V(&rw_mutex);
}

// 读者优先
void read_rf(char proc, int slices){


    P(&r_mutex);
    if (readers==0) //这个锁会导致readers畅通无阻，但是写者被阻挡
        P(&rw_mutex);
    readers++;
    V(&r_mutex);

    P(&maxReaderMutex);

    read(proc, slices);



    V(&maxReaderMutex);

    P(&r_mutex);
    readers--;
    if (readers==0)
        V(&rw_mutex); // 没有读者，可以开始写了
    V(&r_mutex);

}

void write_rf(char proc, int slices){

    P(&rw_mutex);

    // 写过程
    write(proc, slices);

    V(&rw_mutex);
}

// 写者优先
void read_wf(char proc, int slices){


    P(&queue); //读者要排在写者之后

    P(&r_mutex);
    if (readers==0)
        P(&rw_mutex);
    readers++;
    V(&r_mutex);

    V(&queue);

    //读过程开始
    P(&maxReaderMutex); //最大同时读的读者数
    read(proc, slices);
    V(&maxReaderMutex);

    P(&r_mutex);
    readers--;
    if (readers==0)
        V(&rw_mutex); // 没有读者，可以开始写了
    V(&r_mutex);

}

void write_wf(char proc, int slices){

    P(&w_mutex); //针对writers的锁
    // 写过程
    if (writers==0)
        P(&queue); //如果有writer在排，就不会释放这个锁
    writers++;
    V(&w_mutex);

    P(&rw_mutex);//资源锁 写者要互斥所有人

    write(proc, slices);

    V(&rw_mutex);

    P(&w_mutex);
    writers--;
    if (writers==0)
        V(&queue);
    V(&w_mutex);
}



/*======================================================================*
                               ReporterA
 *======================================================================*/
void ReporterA()
{

    color=1;
    int time = 0;
    while (1) {
        if(time>20){
            continue;
        }
        color=1;
        char outputInfo[10];//要打印的信息
        if(time>=20){
            outputInfo[0]='2';
            outputInfo[1]=time-20+'0';
            outputInfo[2]='\0';
        }
        else if(time>=10){
            outputInfo[0]='1';
            outputInfo[1]=time-10+'0';
            outputInfo[2]='\0';
        }else{
            outputInfo[0]=time+'0';
            outputInfo[1]=' ';
            outputInfo[2]='\0';
        }
        color=1; //白色
        myPrint(outputInfo);
        time++;

        for(int i=0;i<5;i++){
            switch (state[i]) {
                case 0:
                    color=3;
                    myPrint(" X");
                    break;
                case 1:
                    color=4;
                    myPrint(" O");
                    break;
                case 2:
                    color=2;
                    myPrint(" Z");
                    break;
            }
        }
        color=1;
        myPrint("\n");
        sleep_ms(TIME_SLICE);
    }
}

/*======================================================================*
                               ReaderB
 *======================================================================*/
void ReaderB()
{

	while(1){
        state[0]=0;
        switch (strategy) {
            case 0:
                read_fair('B', 2);
                break;
            case 1:
                read_rf('B', 2);
                break;
            case 2:
                read_wf('B', 2);
                break;
        }
        state[0]=2;
		sleep_ms(restTime[0]*TIME_SLICE);
	}
}

/*======================================================================*
                               ReaderC
 *======================================================================*/
void ReaderC()
{

	while(1){
        state[1]=0;
        switch (strategy) {
            case 0:
                read_fair('C', 3);
                break;
            case 1:
                read_rf('C', 3);
                break;
            case 2:
                read_wf('C', 3);
                break;
        }
        state[1]=2;
		sleep_ms(restTime[1]*TIME_SLICE);
	}
}

/*======================================================================*
                               ReaderD
 *======================================================================*/
void ReaderD()
{

	while(1){
        state[2]=0;
        switch (strategy) {
            case 0:
                read_fair('D', 3);
                break;
            case 1:
                read_rf('D', 3);
                break;
            case 2:
                read_wf('D', 3);
                break;
        }
        state[2]=2;
		sleep_ms(restTime[2]*TIME_SLICE);
	}
}

/*======================================================================*
                               WriterE
 *======================================================================*/
void WriterE()
{

	while(1){
        state[3]=0;
        switch (strategy) {
            case 0:
                write_fair('E', 3);
                break;
            case 1:
                write_rf('E', 3);
                break;
            case 2:
                write_wf('E', 3);
                break;
        }
        state[3]=2;
		sleep_ms(restTime[3]*TIME_SLICE);
	}
}

/*======================================================================*
                               WriterF
 *======================================================================*/
void WriterF()
{

	while(1){
        state[4]=0;
        switch (strategy) {
            case 0:
                write_fair('F', 4);
                break;
            case 1:
                write_rf('F', 4);
                break;
            case 2:
                write_wf('F', 4);
                break;
        }

        state[4]=2;
		sleep_ms(restTime[4]*TIME_SLICE);
	}
}

