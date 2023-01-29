
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/


#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void flush(CONSOLE* p_con);

/*======================================================================*
			   init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY* p_tty)
{
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	int v_mem_size = V_MEM_SIZE >> 1;	/* 显存总大小 (in WORD) */

	int con_v_mem_size                   = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr      = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit        = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;

	if (nr_tty == 0) {
		/* 第一个控制台沿用原来的光标位置 */
		p_tty->p_console->cursor = disp_pos / 2;
		disp_pos = 0;
	}
	else {
		out_char(p_tty->p_console, nr_tty + '0');
		out_char(p_tty->p_console, '#');
	}

	set_cursor(p_tty->p_console->cursor);
}


/*======================================================================*
			   is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE* p_con)
{
	return (p_con == &console_table[nr_current_console]);
}




/*=============================================================================
 *           add: 退出查找模式的方法
 ==============================================================================*/

/**
 *
 * @param p_con
 * @return
 */
PUBLIC void exit_search_mode(CONSOLE* p_con){
    u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
    //清除该模式下新键入的红色字体
    p_vmem-=2;
    for(int i=1;i<=p_con->cursor-p_con->start_of_searchMode;i++){
        *(p_vmem+1) = DEFAULT_CHAR_COLOR;
        *(p_vmem) = ' ';
        p_vmem-=2;

    }
    //把所有的字体变白 注意：不能变白space和tab

    for(int i=0;i<p_con->start_of_searchMode*2;i+=2){
        if(*(u8*)(V_MEM_BASE + i )!=' ')
            *(u8*)(V_MEM_BASE + i + 1) = DEFAULT_CHAR_COLOR;
    }
    // 指针回到最后的正常模式下的位置
    p_con->cursor = p_con->start_of_searchMode;
    flush(p_con);
}

/*===================================================================
 *       add: 搜索
 ==================================================================*/


PUBLIC void search(CONSOLE* p_con){
    //找出要找的字符串
    int begin,end; // 滑动窗口
    for(int i = 0; i < p_con->start_of_searchMode*2;i+=2){ // 遍历原始白色输入
        int index=i;
        int ret = 1;
        // 遍历匹配
        for(int j = p_con->start_of_searchMode*2;j<p_con->cursor*2;j+=2,index+=2){
            if(*(u8*)(V_MEM_BASE+index)!=*(u8*)(V_MEM_BASE+j)){
                ret = 0;
                break;

            }
            if(*(u8*)(V_MEM_BASE+j)==' '){ //当要查找的字符串含空白符时
                if(*(u8*)(V_MEM_BASE+j+1)==TAB_FLAG&&*(u8*)(V_MEM_BASE+index+1)!=TAB_FLAG){
                    ret = 0;
                    break;
                }else if(*(u8*)(V_MEM_BASE+j+1)!=TAB_FLAG&&*(u8*)(V_MEM_BASE+index+1)==TAB_FLAG){
                    ret = 0;
                    break;
                }
            }//tab和space的匹配问题

        }

        if(ret == 1){
            for(int j = i;j<index;j+=2){
                if(*(u8*)(V_MEM_BASE+j+1)!=TAB_FLAG&&*(u8*)(V_MEM_BASE+j+1)!=Enter_FILL){
                    *(u8*)(V_MEM_BASE + j + 1) = RED;
                }//如果此处为\n或\t，则不要变色，否则下次就看不出来是space还是tab了

            }
        }
    }
}





/*======================================================================*
			   out_char
 *======================================================================*/
PUBLIC void out_char(CONSOLE* p_con, char ch)
{
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2); //指针
    //注意 p_vmem的每个字符是以两字节为单位的，而p_con->cursor是一个为单位

	switch(ch) {
	case '\n': //这里只移动了光标，并没有填充略过的格
		if (p_con->cursor < p_con->original_addr +
		    p_con->v_mem_limit - SCREEN_WIDTH) {
            unsigned int target = p_con->original_addr + SCREEN_WIDTH *((p_con->cursor - p_con->original_addr) /SCREEN_WIDTH + 1);
            p_vmem++;//把他移到当前位置的颜色处
            int i=p_con->cursor;
			while(i< target){
                *(p_vmem)=Enter_FILL ;//给换行填充
                p_vmem+=2;//移到下一个位置的颜色处
                i++;
            }
            p_con->cursor=target; //光标
		}
		break;
	case '\b': //退格键
		if (p_con->cursor > p_con->original_addr) {
            if(mode==0){
                if(*(p_vmem-1)==Enter_FILL){
                    //说明要退一个\n
                    p_vmem--;
                    int count=0;
                    while (*(p_vmem)==Enter_FILL&&count<SCREEN_WIDTH){
                        p_con->cursor--;
                        p_vmem-=2;
                        count++;
                    }
                }else if(*(p_vmem-1)==TAB_FLAG){
                    //说明要退一个\tab
                    p_con->cursor-=4;

                }else{
                    p_con->cursor--;
                    *(p_vmem-2) = ' ';
                    *(p_vmem-1) = DEFAULT_CHAR_COLOR;
                }
            }else if(mode==1){
                if (p_con->cursor > p_con->start_of_searchMode){
                    if(*(p_vmem-1)==Enter_FILL){
                        //说明要退一个\n
                        p_vmem--;
                        int count=0;
                        while (*(p_vmem)==Enter_FILL&&count<SCREEN_WIDTH){
                            p_con->cursor--;
                            p_vmem-=2;
                            count++;
                        }
                    }else if(*(p_vmem-1)==TAB_FLAG){
                        //说明要退一个\tab
                        p_con->cursor-=4;
                    }else{
                        p_con->cursor--;
                        *(p_vmem-2) = ' ';
                        *(p_vmem-1) = DEFAULT_CHAR_COLOR;
                    }
                }
            }


		}
		break;
//--------------------------- add tab (cursor:光标)  CONSOLE 的定义在console.h
//每个字符用两个字节存，第一个字节存内容，第二个用来存颜色，所以可以根据颜色辨别是否是Tab
    case '\t':
        if(p_con->cursor < p_con->original_addr+p_con->v_mem_limit-TAB_WIDTH){
            for(int i=0;i<TAB_WIDTH;i++){
                p_con->cursor++;
                *(p_vmem++) = ' ';
                *(p_vmem++) = TAB_FLAG; //用于区别空格
            }
        }
            break;
//----------------------------
	default:
		if (p_con->cursor <
		    p_con->original_addr + p_con->v_mem_limit - 1) {
			*p_vmem++ = ch;
            if(mode==0){
                *p_vmem++ = DEFAULT_CHAR_COLOR;
            }else{
                *p_vmem++ = RED; //add new color

            }

			p_con->cursor++;
		}
		break;
	}

	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {
		scroll_screen(p_con, SCR_DN);
	}

	flush(p_con);
}

/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE* p_con)
{
        set_cursor(p_con->cursor);
        set_video_start_addr(p_con->current_start_addr);
}

/*======================================================================*
			    set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}



/*======================================================================*
			   select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console)	/* 0 ~ (NR_CONSOLES - 1) */
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) {
		return;
	}

	nr_current_console = nr_console;

	set_cursor(console_table[nr_console].cursor);
	set_video_start_addr(console_table[nr_console].current_start_addr);
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE* p_con, int direction)
{
	if (direction == SCR_UP) {
		if (p_con->current_start_addr > p_con->original_addr) {
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	}
	else if (direction == SCR_DN) {
		if (p_con->current_start_addr + SCREEN_SIZE <
		    p_con->original_addr + p_con->v_mem_limit) {
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	}
	else{
	}

	set_video_start_addr(p_con->current_start_addr);
	set_cursor(p_con->cursor);
}

