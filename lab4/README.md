# 读者写者

## 一.实现方式

### 1.读者优先
读者：

这里用到了maxReaderMutex（用于控制同时读的数目，它的初始value为最大同时读读者数）、r_Mutex（用于控制readers数，readers为当前的读者数）和rw_Mutex（用于控制读写资源）
```
void read_rf(char proc, int slices){


    P(&r_mutex);
    if (readers==0) //这个锁会导致readers畅通无阻，但是写者被阻挡
        P(&rw_mutex);
    readers++;
    V(&r_mutex);

    P(&maxReaderMutex);

    read_proc(proc, slices);

    V(&maxReaderMutex);

    P(&r_mutex);
    readers--;
    if (readers==0)
        V(&rw_mutex); // 没有读者，可以开始写了
    V(&r_mutex);

}
```

写者：
写者这里只用到了rw_mutex，这个只有当最后的读者读完了才会被释放，所以会造成读者饿死

```
void write_rf(char proc, int slices){

    P(&rw_mutex);

    // 写过程
    write_proc(proc, slices);

    V(&rw_mutex);
}
```


### 2.写者优先

读者:
    在读者优先的读者代码基础上，这里又添加了queue锁，这个锁用于控制是否有写者在排队，如果有，则读者无法拿到读权限。

```
void read_wf(char proc, int slices){

    
    P(&queue);
    P(&r_mutex);
    if (readers==0)
        P(&rw_mutex);
    readers++;
    V(&r_mutex);
    V(&queue);
    //读过程开始
    P(&maxReaderMutex);
    read_proc(proc, slices);
    V(&maxReaderMutex);
    P(&r_mutex);
    readers--;
    if (readers==0)
        V(&rw_mutex); // 没有读者，可以开始写了
    V(&r_mutex);
    
}
```

写者：
在读者优先的读者代码的基础上，这里又添加了对queue的控制，即如果有写者正在写，则读者就有queue的控制，但写者没有。同时，还添加了w_mutex用于记录是否有写者在写和在排队，只有当没有写者在写或者在排队时才会释放queue资源
```
void write_wf(char proc, int slices){

    P(&w_mutex);
    
    if (writers==0)
        P(&queue);
    writers++;
    V(&w_mutex);

    P(&rw_mutex);
  // 写过程
    write_proc(proc, slices);

    V(&rw_mutex);

    P(&w_mutex);
    writers--;
    if (writers==0)
        V(&queue);
    V(&w_mutex);
}
```

### 3.解决饿死——公平读写

读者：

```
void read_fair(char proc, int slices){

	P(&queue); //排队

    
	P(&r_mutex); //读写锁
	if (readers==0)
		P(&rw_mutex); // 有读者正在使用，写者不可抢占
	readers++;
	V(&r_mutex);

	V(&queue);
    //进行读操作
    P(&maxReaderMutex); //最大读者锁
	read_proc(proc, slices);
    V(&maxReaderMutex);


	P(&r_mutex);
	readers--;
	if (readers==0)
		V(&rw_mutex); // 没有读者时，可以开始写了
	V(&r_mutex);

   
}
```

写者：
```
void write_fair(char proc, int slices){

	P(&queue);
	P(&rw_mutex);

	V(&queue);
	// 写过程
	write_proc(proc, slices);

	V(&rw_mutex);
}
```

公平读写里读者写者都要公平地拿queue锁


### 4.schedule调度

调度时，会首先遍历读者和写者，如果读者和写者都遍历完了才会进行输出，所以从实现层面上讲reporterA并不是每个时间片都会调用
```
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
                }
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
```
睡眠的实现时通过进程中的sleeping属性进行的，当sleeping>0时，认为进程还在睡眠，不给予时间片。每个时钟中断都会将睡眠的进程的时间片
```
PUBLIC void clock_handler(int irq)
{
	ticks++;
	p_proc_ready->ticks--;

        for (PROCESS* p = proc_table; p < proc_table+NR_TASKS+NR_PROCS; p++){
	        if (p->sleeping > 0)
			p->sleeping--;
	    } // 减少沉睡时间

        if (k_reenter != 0) {
	        return;
	}

	if (p_proc_ready->ticks > 0) {
	        return;
	}

	schedule();

}
```

```
PUBLIC void sys_sleep(int milli_sec) 
{
	int ticks = milli_sec / 1000 * HZ * 10;  // 乘10是为了修正系统的时钟中断错误
	p_proc_ready->sleeping = ticks;
    schedule();
}
```



## 截图展示
最大读者为1的公平读写
![G7C0r.jpg](https://i.imgtg.com/2023/01/05/G7C0r.jpg)
最大读者为2的公平读写
![G7IBq.jpg](https://i.imgtg.com/2023/01/05/G7IBq.jpg)
最大读者为3的公平读写
![G73Kc.jpg](https://i.imgtg.com/2023/01/05/G73Kc.jpg)
最大读者为1时的读者优先
![G7Xfv.jpg](https://i.imgtg.com/2023/01/05/G7Xfv.jpg)
最大读者为2时的读者优先
![G78pY.jpg](https://i.imgtg.com/2023/01/05/G78pY.jpg)
最大读者为3时的读者优先
![GNMMM.jpg](https://i.imgtg.com/2023/01/05/GNMMM.jpg)
写者优先
![GNO1G.jpg](https://i.imgtg.com/2023/01/05/GNO1G.jpg)

