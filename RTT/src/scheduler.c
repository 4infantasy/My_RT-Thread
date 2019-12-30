
#include <rtthread.h>
#include <rthw.h>


// 调度器是操作系统的核心，其主要功能就是实现线程的切换，即从就绪列表里面找到优先级最高的线程，然后去执行该线程。
// 从代码上看，调度器无非也就是由几个全局变量和一些可以实现线程切换的函数组成


// 线程就绪列表，下标数字越小，逻辑优先级越高。
rt_list_t rt_thread_priority_table[RT_THREAD_PRIORITY_MAX];
// 线程就绪列表，实际上就是一个rt_list_t类型的数组（链表数组），数组的大小由决定最大线程优先级的宏RT_THREAD_PRIORITY_MAX来决定。
// RT_THREAD_PRIORITY_MAX在reconfig.h中默认定义为32。
// 数组的下标对应了线程的优先级，同一优先级的线程统一插入到就绪列表的同一链表中。


// 当前线程控制块指针，用于指向当前正在运行的线程的线程控制块。
struct rt_thread *rt_current_thread;



// 线程就绪优先级组
rt_uint32_t rt_thread_ready_priority_group;

// 当前运行线程的优先级
rt_uint8_t rt_current_priority;



/* 调度器初始化函数 */
void rt_system_scheduler_init (void)
{
	
#if 0
	// register防止被编译器优化
	register rt_base_t offset;
	
	/* 线程就绪列表初始化 */
	for ( offset = 0; offset < RT_THREAD_PRIORITY_MAX; offset++ ) {
	
		rt_list_init(&rt_thread_priority_table[offset]);
	
	}
	
	/* 初始化当前线程控制块指针 */
	rt_current_thread = RT_NULL;
	
#else
	
	register rt_base_t offset;
	
	// 线程优先级表初始化
	for ( offset = 0; offset < RT_THREAD_PRIORITY_MAX; offset++ ) {
	
		rt_list_init(&rt_thread_priority_table[offset]);
	
	}
	
	// 初始化当前优先级为空闲线程的优先级
	rt_current_priority = RT_THREAD_PRIORITY_MAX - 1;
	
	// 初始化当前线程控制块指针
	rt_current_thread = RT_NULL;
	
	// 初始化线程就绪优先级组为0，即还没有一个线程就绪。
	rt_thread_ready_priority_group = 0;
	
	
#endif
}



/* 启动调度器函数 */
void rt_system_scheduler_start (void)
{

#if 0
	register struct rt_thread *to_thread;
	
	
	/* 手动制定第一个运行的线程 */
	// rt_list_entry()是一个已知一个结构体里面的成员的地址，反推出该结构体的首地址的宏
	to_thread = rt_list_entry(	rt_thread_priority_table[0].next,
								struct rt_thread,
								tlist );
	// 调度器在启动的时候，会从就绪列表中取出优先级最高的线程的线程控制块，然后切换到该线程。但是目前还不支持优先级，
	// 所以就手动指定第一个运行的线程为就绪列表下标为0这条链表里面挂着的线程。
	
	
	
	// 将获取到的第一个要运行的线程控制块指针传到全局变量rt_current_thread中。
	rt_current_thread = to_thread;
	
	
	/* 切换到第一个线程，该函数在context_rvds.S中实现，在rthw.h中声明，用于实现第一次线程切换。
	   当一个汇编函数在C文件中调用的时候，如果有形参，则执行的时候会将形参传入到CPU寄存器r0，
	   如果有两个形参，第二个则传入到r1。 */			
	rt_hw_context_switch_to( (rt_uint32_t)&to_thread->sp );

#else

	register struct rt_thread *to_thread;
	register rt_ubase_t highest_ready_priority;
	
	// 获取就绪的最高优先级
	highest_ready_priority = __rt_ffs(rt_thread_ready_priority_group) - 1;
	
	// 获取将要运行线程的线程控制块
	to_thread = rt_list_entry(	rt_thread_priority_table[highest_ready_priority].next,
								struct rt_thread,
								tlist );

	// 更新全局指针rt_current_thread的值。
	rt_current_thread = to_thread;
								
	// 切换到新的线程
	rt_hw_context_switch_to( (rt_uint32_t)&to_thread->sp );

								
	// 永远不会返回。将会在线程之间不断切换。。
#endif
}


/* 
	v1. 系统调度就是在就绪列表中寻找优先级最高的就绪线程，然后去执行该线程。但是目前还不支持优先级，仅实现两个线程轮流切换
	v2. 判断每个线程的线程控制块中延时成员remaining_tick的值是否为0，如果为0就要将对应的程序就绪，如果不为0就继续延时。
*/
// 系统调度函数
void rt_schedule(void)
{

#if 0
	
	struct rt_thread *to_thread;
	struct rt_thread *from_thread;
	
	
#if 0
	// 两个线程轮流切换
	if ( rt_current_thread == rt_list_entry(	rt_thread_priority_table[0].next ,
												struct rt_thread ,
												tlist) ) {
		// 如果当前线程为1，则把下一个要运行的线程改为线程2	
													
		from_thread = rt_current_thread;
		
		to_thread = rt_list_entry(	rt_thread_priority_table[1].next ,
									struct rt_thread ,
									tlist ) ;
									
		rt_current_thread = to_thread;
									
	} else {
		// 如果当前线程为2，则把下一个要运行的线程改为线程1
		
		from_thread = rt_current_thread;
		
		to_thread = rt_list_entry(	rt_thread_priority_table[0].next, 
									struct rt_thread, 
									tlist );
									
		rt_current_thread = to_thread ;
								
	}
#else

	/*
		如果当前线程是空线程，那么就去尝试执行线程1或者线程2，
		看看他们的延时时间是否结束，如果线程的延时时间均没有到期，
		那就返回继续执行空闲线程。
	*/

	if ( rt_current_thread == &idle ) {
	
		if ( rt_flag1_thread.remaining_tick == 0 ) {
		
			from_thread 		= rt_current_thread;
			to_thread			= &rt_flag1_thread;
			rt_current_thread 	= to_thread;
		
		} else if ( rt_flag2_thread.remaining_tick == 0 ) {
		
			from_thread			= rt_current_thread;
			to_thread			= &rt_flag2_thread;
			rt_current_thread	= to_thread;
		
		} else {
		
			return ;	// 线程延时均没有到期则返回，继续执行空闲线程
		}
	
	} else {	// 当前线程不是空闲线程则会执行到这里
	
		/*
			如果当前线程是线程1或者线程2的话，检查下另外一个线程，
			如果另外的线程不在延时中，就切换到该线程。
			否则，判断下当前线程是否应该进入延时状态，如果是的话，就切换到空闲线程，
			否则就不进行如何切换。
		*/
		
		if ( rt_current_thread == &rt_flag1_thread ) {
		
			if ( rt_flag2_thread.remaining_tick == 0 ) {
			
				from_thread			= rt_current_thread;
				to_thread			= &rt_flag2_thread;
				rt_current_thread	= to_thread;
				
			} else if ( rt_current_thread->remaining_tick != 0 ) {
			
				from_thread			= rt_current_thread;
				to_thread			= &idle;
				rt_current_thread	= to_thread;
			
			} else {
			
				return ;	// 返回，不进行切换，因为两个线程都处于延时中
			}
		
		} else if ( rt_current_thread == &rt_flag2_thread ) {
		
			if ( rt_flag1_thread.remaining_tick == 0 ) {
			
				from_thread			= rt_current_thread;
				to_thread			= &rt_flag1_thread;
				rt_current_thread	= to_thread;
				
			} else if ( rt_current_thread->remaining_tick != 0 ) {
			
				from_thread			= rt_current_thread;
				to_thread			= &idle;
				rt_current_thread	= to_thread;
			
			} else {
			
				return ;	// 返回，不进行切换，因为两个线程都处于延时中
			}
		
		}
	}

#endif
	
	
#else

	rt_base_t level;
	register rt_ubase_t highest_ready_priority;
	struct rt_thread *to_thread;
	struct rt_thread *from_thread;

	// 关中断
	level = rt_hw_interrupt_disable();
	
	// 获取就绪的最高优先级
	highest_ready_priority = __rt_ffs(rt_thread_ready_priority_group) - 1;
	
	// 获取就绪的最高优先级对应的线程控制块
	to_thread = rt_list_entry(	rt_thread_priority_table[highest_ready_priority].next,
								struct rt_thread,
								tlist );

	// 如果目标线程不是当前线程，则要进行线程切换
	if ( to_thread != rt_current_thread ) {
	
		rt_current_priority = (rt_uint8_t)highest_ready_priority;
		from_thread			= rt_current_thread;
		rt_current_thread	= to_thread;
	
		rt_hw_context_switch( (rt_uint32_t)&from_thread->sp, 
							  (rt_uint32_t)&to_thread->sp );
		
		// 开中断
		rt_hw_interrupt_enable(level);
		
	} else {
	
		// 开中断
		rt_hw_interrupt_enable(level);
	}
								
#endif
	
	
	// 产生上下文切换
	rt_hw_context_switch( (rt_uint32_t)&from_thread->sp, (rt_uint32_t)&to_thread->sp );
	
}



// 调度器插入线程
void rt_schedule_insert_thread(struct rt_thread *thread)
{

	register rt_base_t temp;
	
	// 关中断
	temp = rt_hw_interrupt_disable();
	
	
	// 改变线程状态
	thread->stat = RT_THREAD_READY;
	
	// 将线程插入就绪列表
	rt_list_insert_before(	&(rt_thread_priority_table[thread->current_priority]), 
							&(thread->tlist) );

	// 设置线程就绪优先级组中对应的位
	rt_thread_ready_priority_group |= thread->number_mask;
	
	
	// 开中断
	rt_hw_interrupt_enable(temp);
	
}


// 调度器删除线程
void rt_schedule_remove_thread(struct rt_thread *thread)
{

	register rt_base_t temp;
	
	// 关中断
	temp = rt_hw_interrupt_disable();
	
	
	// 将线程从就绪列表删除
	rt_list_remove( &(thread->tlist) );
	
	// 将线程就绪优先级组队员的位清除
	if ( rt_list_isempty(&(rt_thread_priority_table[thread->current_priority])) ) {
	
		rt_thread_ready_priority_group &= ~thread->number_mask;
	
	}

	
	// 开中断
	rt_hw_interrupt_enable(temp);
}

