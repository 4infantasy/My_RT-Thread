
#include <rtthread.h>
#include <rthw.h>


static rt_tick_t rt_tick = 0;  // 系统时基计数器，用来记录产生了多少次SysTick中断


void rt_tick_increase(void)
{
	
	rt_ubase_t i ;
	struct rt_thread *thread;
	
	rt_tick ++ ;	// 系统时基计数器加一操作
	
#if 0
	// 扫描就绪列表中所有线程的remaining_tick，如果不为0，则减1
	for ( i=0; i<RT_THREAD_PRIORITY_MAX; i++ ) {
	
		thread = rt_list_entry(	rt_thread_priority_table[i].next, 
								struct rt_thread, 
								tlist );
	
								
		if ( thread->remaining_tick > 0 ) {
			thread->remaining_tick--; 
		}
	}
#else
	
	// 扫描就绪列表中所有线程的remaining_tick，如果不为0，则减1
	for ( i=0; i<RT_THREAD_PRIORITY_MAX; i++ ) {
	
		thread = rt_list_entry(	rt_thread_priority_table[i].next, 
								struct rt_thread, 
								tlist );
	
								
		if ( thread->remaining_tick > 0 ) {
			thread->remaining_tick--; 
			
			// 如果线程的延时时间remaining_tick递减为0，则表示延时时间结束，需要将线程插入到线程优先级表，
			// 即简单根据优先级将线程就绪优先级组中对应的位置1即可，因为在阻塞延时函数中，是通过清除线程就绪优先级组中对应的位来让线程挂起的，
			// 并没有将线程从线程优先级表里面移除，所以注释掉rt_schedule_insert_thread(thread);
			if ( thread->remaining_tick == 0 ) {
			
				//rt_schedule_insert_thread(thread);
				rt_thread_ready_priority_group |= thread->number_mask;
			}
		}
	}
	
#endif
	
	// 系统调度
	rt_schedule();
}

