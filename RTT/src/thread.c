
#include <rtthread.h>
#include <rthw.h>


rt_err_t rt_thread_init ( 	struct rt_thread *thread,			// 线程控制块指针
							const char *name,					// 线程对象名字
							void (*entry)(void *parameter),		// entry是线程函数名，表示线程的入口
							void		*parameter,				// parameter是线程形参，用于传递线程参数
							void		*stack_start,			// stack_start用于指向线程栈的起始地址
							rt_uint32_t	stack_size,				// stack_size表示线程栈的大小，单位为字节
							rt_uint8_t	priority )				// 优先级
{
	
	// 线程对象初始化
	// 线程结构体开头部分的4个成员就是rt_object_t成员
	rt_object_init( (rt_object_t)thread, RT_Object_Class_Thread, name );
	
	
	rt_list_init( &(thread->tlist) );			// 初始化线程链表节点，可以把线程控制块挂到各种链表中（相当于把线程插入到各种链表中）
	
	thread->entry = (void *)entry;				// 将线程入口保存到线程控制块的entry成员中。
	thread->parameter = parameter;				// 将线程入口形参保存到线程控制块的parameter成员中。
	
	thread->stack_addr = stack_start;			// 将线程栈起始地址保存到线程控制块的stack_addr成员中。
	thread->stack_size = stack_size;			// 将线程栈大小保存到线程控制块的stack_size成员中。
	
	
	// 初始化线程栈，并返回线程指针
	thread->sp = (void *)rt_hw_stack_init ( thread->entry,
											thread->parameter,
											(void *)((char *)thread->stack_addr + thread->stack_size - 4) );
	// 这个时候返回的是剩余栈的栈顶指针
	
	
	// 加入了优先级支持
	thread->init_priority		= priority;
	thread->current_priority	= priority;
	thread->number_mask			= 0;
	
	// 错误码和状态
	thread->error	=	RT_EOK;
	thread->stat	=	RT_THREAD_INIT;
	
	return RT_EOK;
	
}



/*
阻塞延时的阻塞是指线程调用该延时函数后，线程会被剥离CPU使用权，然后进入阻塞状态，直到延时结束，线程重新获取CPU使用权，才能继续运行。

如果其他线程也在延时，那么CPU就将运行空闲线程。
*/

// 阻塞延时
void rt_thread_delay(rt_tick_t tick)
{
#if 0
	struct rt_thread *thread;
	
	// 获取当前线程的线程控制块
	thread = rt_current_thread;
	
	// 设置延时时间
	thread->remaining_tick = tick;
	
	// 进行系统调度
	rt_schedule();
#else
	
	register rt_base_t temp;
	struct rt_thread *thread;
	
	// 关中断
	temp = rt_hw_interrupt_disable();
	
	// 设置线程延时时间
	thread = rt_current_thread;
	thread->remaining_tick = tick;
	
	// 改变线程状态
	// 将线程的状态改为挂起，接下来将进入延时，暂时放弃CPU的使用权
	thread->stat = RT_THREAD_SUSPEND;
	// 根据优先级将线程就绪优先级中对应的位清零。
	rt_thread_ready_priority_group &= ~thread->number_mask;
	
	
	// 开中断
	rt_hw_interrupt_enable(temp);
	
	// 进行系统调度
	rt_schedule();
	
#endif
}




// rt_thread_self() 用于返回全局指针rt_current_thread，该指针指向当前正在运行的线程的线程控制块，
// 在系统没有启动之前，rt_current_thread的值为RT_NULL。
rt_thread_t rt_thread_self(void)
{
	return rt_current_thread;
}



// 线程恢复函数，该函数用于恢复一个线程，然后将其放到就绪列表
rt_err_t rt_thread_resume(rt_thread_t thread)
{

	register rt_base_t temp;
	
	
	// 被恢复的线程必须在挂起态，否则返回错误码
	if ( (thread->stat & RT_THREAD_STAT_MASK) != RT_THREAD_SUSPEND ) {
	
		return -RT_ERROR;
	}
	
	// 关中断
	temp = rt_hw_interrupt_disable();
	
	// 从挂起队列移除
	rt_list_remove( &(thread->tlist) );
	
	// 开中断
	rt_hw_interrupt_enable(temp);
	
	// 插入就绪列表
	rt_schedule_insert_thread(thread);

	
	return RT_EOK;
}





// 之前创建好线程后，调用rt_list_insert_before()将线程插入到线程优先级表，现在用线程启动函数来实现这个功能。
/*
	启动一个线程，并将其放到系统的就绪列表中
*/
// 线程启动函数
rt_err_t rt_thread_startup(rt_thread_t thread)
{
	
	// 设置当前优先级为初始优先级
	thread->current_priority	= thread->init_priority;				// 设置当前优先级为初始优先级
	thread->number_mask			= 1L << thread->current_priority;		// 根据优先级计算线程就绪优先级组的掩码值

	// 改变线程的状态为挂起状态
	thread->stat = RT_THREAD_SUSPEND;			// 设置线程的状态为挂起态，等下会恢复
	
	// 然后恢复线程
	rt_thread_resume( thread );					// 恢复线程，即将线程插入到就绪列表，由单独的rt_thread_resume()函数来实现
	
	
	if ( rt_thread_self() != RT_NULL ) {
	
		// 系统调度
		rt_schedule();
	}
	
	
	return RT_EOK;
}

