
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "console.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "proto.h"



/**
 * FCFS算法
 */
PUBLIC void p_process(SEMAPHORE* s){
	disable_int();
    s->value--;
    if (s->value < 0){
        p_proc_ready->blocked = 1;
        s->p_list[s->tail] = p_proc_ready;//尾部加一个进程（入队）
        s->tail = (s->tail + 1) % NR_PROCS;
        schedule();
    }
	enable_int();
}

PUBLIC void v_process(SEMAPHORE* s){
	disable_int();
    s->value++;
    if (s->value <= 0){
        s->p_list[s->head]->blocked = 0; // 唤醒最先进入队列的进程(出队)
        s->head = (s->head + 1) % NR_PROCS;
    }
	enable_int();
}

/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule()
{
	PROCESS* p;
	int	temp = 0;

	while (!temp) {
            for (p = proc_table+1; p < proc_table+NR_TASKS+NR_PROCS; p++) {

                if (p->sleeping > 0 || p->blocked) continue; //如果该进程被阻塞或者正在睡眠 则不分配时间片

                if (p->ticks > temp) { //寻找ticks最大的进程（ticks==priority）
                    temp = p->ticks;
                    p_proc_ready = p;
                }//找到该轮还没有执行中的 最大的、可执行的 那个来执行
            }


		// 如果都是0，那么需要重设ticks
		if (!temp) {
			for (p = proc_table+1; p < proc_table+NR_TASKS+NR_PROCS; p++) {
				if (p->ticks > 0) continue; //阻塞的进程
				p->ticks = p->priority;
			}
		}
	}
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}

PUBLIC void sys_sleep(int milli_sec) 
{
	int ticks = milli_sec / 1000 * HZ;
	p_proc_ready->sleeping = ticks;
    schedule();
}

PUBLIC void sys_print(char*s)
{
    switch (color) {
        case 1:
            disp_color_str(s,BRIGHT | MAKE_COLOR(BLACK, WHITE));
            break;
        case 4:
            disp_color_str(s,BRIGHT | MAKE_COLOR(BLACK, GREEN));
            break;
        case 2:
            disp_color_str(s,BRIGHT | MAKE_COLOR(BLACK, BLUE));
            break;
        case 3:
            disp_color_str(s,BRIGHT | MAKE_COLOR(BLACK, RED));
            break;
        default:
            disp_color_str(s,BRIGHT | MAKE_COLOR(BLACK, RED));
            break;
    }

}

