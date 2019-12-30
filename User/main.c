
#include <rtthread.h>
#include <rthw.h>
#include "ARMCM3.h"

//	==================	全局变量  ==================

rt_uint32_t flag1 = 0;
rt_uint32_t flag2 = 0;


//	==================	线程控制块 & STACK & 线程声明  ==================

// 线程控制块
struct rt_thread rt_flag1_thread;
struct rt_thread rt_flag2_thread;


// 设置4字节对齐
ALIGN(RT_ALIGN_SIZE)


// 定义线程栈
rt_uint8_t rt_flag1_thread_stack[512];
rt_uint8_t rt_flag2_thread_stack[512];


// 线程声明
void flag1_thread_entry ( void *p_arg );
void flag2_thread_entry ( void *p_arg );



// 软件延时
void delay( rt_uint32_t count )
{
	for( ; count != 0; count-- );
}

// 线程1
void flag1_thread_entry( void *p_arg )
{
	for( ;; ) {
	
#if 0
		flag1 = 1;
		delay(100);
		flag1 = 0;
		delay(100);
		
		// 线程切换，这里是手动切换
		// 因为目前还不支持优先级，每个线程执行完毕之后都主动调用系统调度函数rt_schedule()来实现线程的切换。
		rt_schedule();
#else
		// 延时函数由原来的软件延时替代为阻塞延时，延时时间均为2个SysTick中断周期，即20ms
		flag1 = 1;
		rt_thread_delay(2);
		flag1 = 0;
		rt_thread_delay(2);
#endif
	}
}

// 线程2
void flag2_thread_entry( void *p_arg )
{
	for( ;; ) {
	
#if 0
		flag2 = 1;
		delay(100);
		flag2 = 0;
		delay(100);
		
		// 线程切换，这里是手动切换
		rt_schedule();
#else
		flag2 = 1;
		rt_thread_delay(2);
		flag2 = 0;
		rt_thread_delay(2);
#endif
	}
}


/*
	remaining_tick的递减，在RT-Thread中，由SysTick中断提供，操作系统里面的最小的时间单位就是SysTick的中断周期，称之为一个tick。
*/
void SysTick_Handler(void)
{
	
	// 进入中断
	rt_interrupt_enter();
	
	// 时基更新
	rt_tick_increase();
	
	// 离开中断
	rt_interrupt_leave();
}



int main(void) 
{
	
	// 关中断
	rt_hw_interrupt_disable();
	
	// 初始化SysTick，调用固件库函数SysTick_Config来实现，配置中断周期为10ms
	// SysTick中断频率设置
	SysTick_Config( SystemCoreClock / RT_TICK_PER_SECOND );
	
	// 调度器初始化
	rt_system_scheduler_init();

	// 初始化空闲线程
	rt_thread_idle_init();
	
	
	// 初始化线程
	rt_thread_init(	&rt_flag1_thread,					// 线程控制块
					"thread1",							// 线程对象名字
					flag1_thread_entry,					// 线程入口地址
					RT_NULL,							// 线程形参
					&rt_flag1_thread_stack[0],			// 线程栈起始地址，
					sizeof(rt_flag1_thread_stack),		// 线程栈大小，单位为字节
					2 );								// 优先级
	
	// 将线程插入到就绪列表
	//rt_list_insert_before( &(rt_thread_priority_table[0]), &(rt_flag1_thread.tlist) );
	rt_thread_startup( &rt_flag1_thread );
	
	
	// 初始化线程
	rt_thread_init(	&rt_flag2_thread,					// 线程控制块
					"thread2",							// 线程对象名字
					flag2_thread_entry,					// 线程入口地址
					RT_NULL,							// 线程形参
					&rt_flag2_thread_stack[0],			// 线程栈起始地址，
					sizeof(rt_flag2_thread_stack),		// 线程栈大小，单位为字节
					3 );								// 优先级
	
	// 线程创建好之后，我们需要把线程添加到就绪列表里面，表示线程已经就绪，系统随时可以调度，就绪列表在scheduler.c中定义。
	//rt_list_insert_before( &(rt_thread_priority_table[1]), &(rt_flag2_thread.tlist) );
	rt_thread_startup( &rt_flag2_thread );
	
	// 启动系统调度器
	rt_system_scheduler_start();


}

