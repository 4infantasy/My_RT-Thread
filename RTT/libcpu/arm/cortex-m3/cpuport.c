#include <rtthread.h>


struct exception_stack_frame
{
	// 自动加载到CPU寄存器
	rt_uint32_t r0;
	rt_uint32_t r1;
	rt_uint32_t r2;
	rt_uint32_t r3;
	rt_uint32_t r12;
	rt_uint32_t lr;
	rt_uint32_t pc;
	rt_uint32_t psr;
};


struct stack_frame
{
	// 异常发生时，需手动加载到CPU寄存器的内容
	rt_uint32_t r4;
	rt_uint32_t r5;
	rt_uint32_t r6;
	rt_uint32_t r7;
	rt_uint32_t r8;
	rt_uint32_t r9;
	rt_uint32_t r10;
	rt_uint32_t r11;
	
	struct exception_stack_frame exception_stack_frame;
};


// 用于存储上一个线程的栈的sp的指针
rt_uint32_t rt_interrupt_from_thread;

// 用于存储下一个将要运行的线程的栈的sp的指针
rt_uint32_t rt_interrupt_to_thread;

// PendSV中断服务函数执行标志
rt_uint32_t rt_thread_switch_interrupt_flag;


/*
	// 初始化线程栈，并返回线程指针
	thread->sp = (void *)rt_hw_stack_init ( thread->entry,
											thread->parameter,
											(void *)((char *)thread->stack_addr + thread->stack_size - 4) );
*/
// rt_hw_stack_init() 用来初始化线程栈，当线程第一次运行的时候，加载到CPU寄存器的参数就放在线程栈里面。
rt_uint8_t *rt_hw_stack_init ( 	void		*tentry,			// 线程入口
								void		*parameter,			// 线程形参
								rt_uint8_t	*stack_addr )		// 线程栈顶地址-4，在该函数调用的时候，传进来的是线程栈的栈顶地址-4.
{
	
	struct stack_frame 	*stack_frame;
	rt_uint8_t			*stk;
	rt_uint32_t			i;
	
	
	/*  获取栈顶指针rt_hw_stack_init 在调用的时候，传给stack_addr的是(栈顶指针 - 4)  */
	stk = stack_addr + sizeof(rt_uint32_t) ;
	/* 获取栈顶指针，将栈顶指针传给stk。rt_hw_stack_init()函数在rt_thread_init()函数中调用的时候，
	传给的形参stack_addr的值是栈顶指针减去4，所以现在加上sizeof(rt_uint32_t)刚好与减掉的4相互抵消，即传给stk的是栈顶指针。 */
	
	
	/* 让stk指针向下8字节对齐 */
	stk = (rt_uint8_t *)RT_ALIGN_DOWN((rt_uint32_t)stk, 8) ;
	/* 让stk这个指针向下8个字节对齐，确保stk是8字节对齐的地址。 */
	/* 在Cortex-M内核的处理器里面，要确保栈顶指针是8字节对齐的。如果栈顶指针是8字节对齐的，在进行向下8字节对齐的时候，指针不会移动，
	   如果不是8字节对齐的，在做向下8字节对齐的时候，就会空出几个字节，不会使用，比如当stk是33，明显不能整除8，进行向下8字节对齐就是32，
	那么就会空出一个字节不使用。 */
	
	
	/* 将stk指针继续向下移动sizeof(struct stack_frame)个偏移 */
	stk -= sizeof( struct stack_frame ) ;
	/* stk指针继续向下移动sizeof(struct stack_frame)个编译，即16个字节的大小。 */
	
	
	/* 将stk指针强制转化为stack_frame类型后，存到stack_frame */
	stack_frame = ( struct stack_frame * )stk ;
	/* 将stk指针强制转化为stack_frame类型后存到指针变量stack_frame中。 */
	
	
	/* 以stack_frame 为起始地址，将栈空间里面的sizeof(struct stack_frame)个内存初始化为 0xdeadbeef */
	for ( i=0; i < sizeof(struct stack_frame) / sizeof(rt_uint32_t); i++ ) {
	
		((rt_uint32_t *)stack_frame)[i] = 0xdeadbeef;
	}
	

	/* 初始化异常发生时自动保存的寄存器 */
	stack_frame->exception_stack_frame.r0  = (unsigned long)parameter;	// r0: argument, 参数地址存在r0
	stack_frame->exception_stack_frame.r1  = 0;							// r1
	stack_frame->exception_stack_frame.r2  = 0;							// r2
	stack_frame->exception_stack_frame.r3  = 0;							// r3
	stack_frame->exception_stack_frame.r12 = 0;							// r12
	stack_frame->exception_stack_frame.lr  = 0;							// lr: lr暂时初始化为0
	stack_frame->exception_stack_frame.pc  = (unsigned long)tentry;		// entry point, pc， 入口地址存在pc
	stack_frame->exception_stack_frame.psr = 0x01000000L;				// PSR
	
	/* 线程第一次运行的时候，加载到CPU寄存器的环境参数我们要预先初始化好。从栈顶开始，初始化的顺序固定，
	   首先是异常发生时自动保存的8个寄存器，即xPSR、R15、R14、R12、R3、R2、R1和R0。
	   其中xPSR寄存器的位24必须是1，R15(PC)指针必须存的是线程的入口地址，R0必须是线程形参，剩下的R14、R12、R3、R2和R1我们初始化为0。*/
	
	/* 剩下的是8个需要手动加载到CPU寄存器的参数，即R4~R11，默认初始化为0xdeadbeef */
	
	
	/* 返回线程栈指针，这个时候stk指向剩余栈的栈顶 */
	return stk;
}


