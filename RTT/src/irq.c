
#include <rtthread.h>
#include <rthw.h>


// 中断计数器，用来记录中断嵌套次数。
volatile rt_uint8_t rt_interrupt_nest;


/*
	当BSP文件的中断服务函数进入时，会调用该函数
	不要再应用程序中调用
*/
void rt_interrupt_enter(void)
{
	
	rt_base_t level;
	
	// 关中断
	level = rt_hw_interrupt_disable();
	
	// 中断计数器++
	rt_interrupt_nest++;
	
	// 开中断
	rt_hw_interrupt_enable(level);

}


/*
	当BSP文件的中断服务函数离开时，会调用该函数
	不要再应用程序中调用
*/
void rt_interrupt_leave(void)
{

	rt_base_t level;
	
	// 关中断
	level = rt_hw_interrupt_disable();
	
	// 中断计数器--
	rt_interrupt_nest--;
	
	// 开中断
	rt_hw_interrupt_enable(level);

}

