


; IMPORT 声明标号来自外部文件，跟C语言中的EXTERN关键字类似，这三个全局变量在cpuport.c中定义。
; 全局变量

	IMPORT	rt_thread_switch_interrupt_flag	
	IMPORT	rt_interrupt_from_thread		
	IMPORT	rt_interrupt_to_thread			
		
		
; 常量，这些都是内核里面的寄存器，等下触发PendSV异常会用到。

;有关内核外设寄存器定义可参考官方文档：STM32F10xx Cortex-M3 programming manual - 4 Core peripherals
;系统控制块外设SCB地址范围：0xE000ED00 - 0xE000ED3F

; EQU 给数字常量去一个符号名，相当于C语言中的define

SCB_VTOR			EQU		0xE000ED08	;	向量表偏移寄存器
NVIC_INT_CTRL		EQU		0xE000ED04	;	中断控制状态寄存器
NVIC_SYSPRI2		EQU		0xE000ED20	;	系统优先级寄存器(2)
NVIC_PENDSV_PRI		EQU		0x00FF0000	;	PendSV优先级值 (lowest)
NVIC_PENDSVSET		EQU		0x10000000	;	触发PendSV exception的值


; AREA 汇编一个新的代码段或数据段
; .text 表示段名字，如果段名不是以字母开头，而是以其他符号靠头，则需要在段名两边加上“|”
; CODE 表示为代码，READONLY 表示只读，ALIGN=2 表示当前文件指令要2^2字节对齐。
; THUMB 表示THUMB指令代码
; REQUIRE8和PRESERVE8 均表示当前文件栈需按照8字节对齐

; 代码产生指令

	AREA |.text|, CODE, READONLY, ALIGN=2
	THUMB
	REQUIRE8
	PRESERVE8
		

; EXPORT 声明一个标号具有全局属性，可被外部的文件使用
; ALIGN 编译器对指令或者数据的存放地址进行对齐
; END 到达文件的末尾，文件结束
; LDR 从存储器中加载字到一个寄存器中
; STR 把一个寄存器按字存储到存储器中
; ORR 按位或



; /*
;  * 函数原型： void rt_hw_context_switch_to(rt_uint32_t to);
;  * r0 -> to
;  * 该函数用于开启第一次线程切换
; */

; PROC 定义子程序，与ENDP成对使用，表示子程序结束，表示rt_hw_context_switch_to()函数开始
rt_hw_context_switch_to		PROC
	
	; 使用EXPORT关键字导出rt_hw_context_switch_to，让其具有全局属性，可以在C文件调用（但也要现在rthw.h中声明）
	EXPORT rt_hw_context_switch_to
		
		
	; 设置rt_interrupt_to_thread 的值
	; 将rt_interrupt_to_thread 的地址加载到r1
	LDR		r1, =rt_interrupt_to_thread
	
	; 将r0的值存储到rt_interrupt_to_thread，r0存的是下一个将要运行的线程的sp的地址，
	; 由rt_hw_context_switch_to( (rt_uint32_t)&to_thread->sp )调用的时候传到r0。
	STR		r0, [r1]
	
	
	; 设置rt_interrupt_from_thread 的值为0，表示启动第一次线程切换
	; 将rt_interrupt_from_thread 的地址加载到r1
	LDR		r1, =rt_interrupt_from_thread
	
	; 配置r0等于0
	MOV		r0, #0x0
	
	; 将r0的值存储到rt_interrupt_from_thread
	STR		r0, [r1]
	
	
	; 设置中断标志位rt_thread_switch_interrupt_flag 的值为1，当执行了PendSVC Handler时，rt_thread_switch_interrupt_flag的值会被清0。
	; 将rt_thread_switch_interrupt_flag 的地址加载到r1
	LDR		r1, =rt_thread_switch_interrupt_flag
	
	; 配置r0等于1
	MOV		r0, #1
	
	; 将r0的值存储到rt_thread_switch_interrupt_flag
	STR		r0, [r1]


	; 设置PendSV异常的优先级为最低
	LDR		r0, =NVIC_SYSPRI2		; 系统优先级寄存器(2)
	LDR		r1, =NVIC_PENDSV_PRI	; PendSV优先级值 (lowest)
	LDR.W	r2, [r0, #0x00]			; 读
	ORR		r1, r1, r2				; 改
	STR		r1, [r0]				; 写
	
	
	; 触发PendSV异常（产生上下文切换）。如果前面关了，还要等中断打开才能去执行PendSV中断服务函数。
	LDR		r0, =NVIC_INT_CTRL		; 中断控制状态寄存器
	LDR		r1, =NVIC_PENDSVSET		; 触发PendSV exception的值
	STR		r1, [r0]				; 将触发PendSV exception的值，存储到中断控制状态寄存器，产生PendSV异常中断。
	
	
	; 开中断
	CPSIE	F
	CPSIE	I
	
	
	; 永远不会到达这里，rt_hw_context_switch_to()程序结束，与PROC成对使用。
	ENDP
		
		
		
; void PendSV_Handler(void);
; r0 --> switch from thread stack
; r1 --> switch to thread stack
; psr, pc, lr, r12, r3, r2, r1, r0 are pushed into [from] stack

; PendSV_Handler()函数是真正实现线程上下文切换的地方
PendSV_Handler		PROC
	
	EXPORT PendSV_Handler
		
	; 失能中断，为了保护上下文切换不被中断
	MRS		r2, PRIMASK
	CPSID	I
	
	
	; 获取中断标志位rt_thread_switch_interrupt_flag，看看是否为0，如果为0则退出PendSV_Handler，如果不为0则继续往下执行。
	; 加载rt_thread_switch_interrupt_flag的地址到r0
	LDR		r0, =rt_thread_switch_interrupt_flag
	
	; 加载rt_thread_switch_interrupt_flag的值到r1
	LDR		r1, [r0]
	
	; 判断r1是否为0，为0则跳转到pendsv_exit，退出PendSV_Handler函数
	CBZ		r1, pendsv_exit
	
	
	; r1不为0则清0，中断标志位rt_thread_switch_interrupt_flag清0
	MOV		r1, #0x00
	
	; 将r1的值存储到rt_thread_switch_interrupt_flag，即清0
	STR		r1, [r0]
	
	
	; 判断rt_interrupt_from_thread的值是否为0，如果为0则表示第一次线程切换，不用做上文保存的工作，直接跳转的switch_to_thread执行下文切换即可。
	; 如果不为0，则需要先保存上文，然后再切换到下文。
	; 加载rt_interrupt_from_thread的地址到r0
	LDR		r0, =rt_interrupt_from_thread
	
	; 加载rt_interrupt_from_thread的值到r1
	LDR		r1, [r0]
	
	; 判断r1是否为0，为0则跳转到switch_to_thread
	; 第一次线程切换时rt_interrupt_from_thread肯定为0，则跳转到switch_to_thread
	CBZ		r1, switch_to_thread
		
		
; ================== 上文保存 ==================

	; rt_interrupt_from_thread的值不为0则表示不是第一次线程切换，需要先保存上文。
	; 当进入PendSVC Handler时，上一个线程运行的环境即：
	; xPSR, PC(线程入口地址), R14, R12, R3, R2, R1, R0(线程的形参)
	; 这些CPU寄存器的值会自动保存到线程的栈中，剩下的r4~r11需要手动保存
	
	; 获取线程栈指针到r1
	MRS		r1, psp
	
	; 将CPU寄存器r4~r11的值存储到r1指向的地址（每操作一次地址将递减一次）
	STMFD	r1!, {r4 - r11}
	
	; 加载r0指向值到r0， 即r0=rt_interrupt_from_thread
	LDR		r0, [r0]
	
	; 将r1的值存储到r0，即更新线程栈sp
	STR		r1, [r0]
		
		
; ================== 下文切换  ==================	
		
	; 下文切换实际上就是把接下来要运行的线程栈里面的内容加载到CPU寄存器，更改PC指针和PSP指针，从而实现程序的跳转
		
switch_to_thread

	; 加载rt_interrupt_to_thread的地址到r1
	; rt_interrupt_to_thread是一个全局变量，里面存的是线程栈指针SP的指针
	LDR		r1, =rt_interrupt_to_thread
	
	; 加载rt_interrupt_to_thread的值到r1，即sp指针的指针
	LDR		r1, [r1]
	
	; 加载rt_interrupt_to_thread的值r1，即sp
	LDR		r1, [r1]
	
	; 将线程栈指针r1（操作之前先递减）指向的内容加载到CPU寄存器r4~r11
	LDMFD	r1!, {r4 - r11}
	
	; 将线程栈指针更新到PSP
	MSR		psp, r1
	
	
pendsv_exit

	; 恢复中断
	MSR		PRIMASK, r2
	
	; 确保异常返回使用的栈指针是PSP，即LR寄存器的位2要为1
	ORR		lr, lr, #0x04
	
	; 异常返回，这个时候栈中的剩下内容将会自动加载到CPU寄存器：
	; xPSR, PC(线程入口地址), R14, R12, R3, R2, R1, R0(线程的形参)
	; 同时PSP的值也将更新，即指向线程栈的栈顶
	BX		lr
	
	
	; PendSV_Handler子程序结束，上下文切换完成，恢复中断。
	ENDP
	
		
		
		
; 
; void rt_hw_context_switch(rt_uint32_t from, rt_uint32_t to);
; r0 --> from
; r1 --> to
rt_hw_context_switch	PROC
	
	EXPORT rt_hw_context_switch
		
	; 设置中断标志位rt_thread_switch_interrupt_flag为1
	; 加载rt_thread_switch_interrupt_flag的地址到r2
	LDR		r2, =rt_thread_switch_interrupt_flag
	
	; 加载rt_thread_switch_interrupt_flag的值到r3
	LDR		r3, [r2]
	
	; r3与1比较，相等则执行BEQ指令，否则不执行
	CMP		r3, #1
	BEQ		_reswitch
	
	; 设置r3的值为1
	MOV		r3, #1
	
	; 将r3的值存储到rt_thread_switch_interrupt_flag，即置1
	STR		r3, [r2]
	
	
	; 设置rt_interrupt_from_thread的值
	; 加载rt_interrupt_from_thread的地址到r2
	LDR		r2, =rt_interrupt_from_thread
	
	; 存储r0的值到rt_interrupt_from_thread，即上一个线程栈指针sp的指针。
	; r0存储的是函数调用rt_hw_context_switch( (rt_uint32_t)&from_thread->sp, (rt_uint32_t)&to_thread->sp )时的第一个形参，即上一个线程栈指针sp的指针。
	STR		r0, [r2]
	
	
_reswitch

	; 设置rt_interrupt_to_thread的值
	; 加载rt_interrupt_to_thread的地址到r2
	LDR		r2, =rt_interrupt_to_thread
	
	; 存储r1的值到rt_interrupt_to_thread，即下一个线程栈指针sp的指针
	; r1存储的是函数调用rt_hw_context_switch( (rt_uint32_t)&from_thread->sp, (rt_uint32_t)&to_thread->sp )时的第二个形参，即下一个线程栈指针sp的指针。
	STR		r1, [r2]
	
	
	; 触发PendSV异常，实现上下文切换
	LDR		r0, =NVIC_INT_CTRL
	LDR		r1, =NVIC_PENDSVSET
	STR		r1, [r0]
	
	; 子程序返回
	BX		LR
	
	
	; 子程序结束
	ENDP




;
; rt_base_t rt_hw_interrupt_disable();
;
rt_hw_interrupt_disable		PROC
	
	EXPORT	rt_hw_interrupt_disable
		
	; 通过MRS指令将特殊寄存器PRIMASK寄存器的值存储到通用寄存器r。
	; 当在C中调用汇编的子程序返回时，会将r0作为函数的返回值。
	; 所以在C中调用rt_hw_interrupt_disable()的时候，需要事先声明一个变量用来存储rt_hw_interrupt_disable()的返回值，即r0寄存器的值，也就是PRIMASK的值。
	MRS		r0, PRIMASK
	
	; 关闭终端，即使用CPS指令将PRIMASK寄存器的值置1
	CPSID	I
	
	; 子程序返回。
	BX		LR
	
	; 汇编子程序结束。
	ENDP




;
; void rt_hw_interrupt_enable(rt_base_t level);
;
rt_hw_interrupt_enable	PROC
	
	EXPORT	rt_hw_interrupt_enable
		
	; 通过MSR指令将通用寄存器r0的值存储到特殊寄存器PRIMASK。
	MSR		PRIMASK, r0
	
	; 子程序返回。
	BX		LR
	
	; 汇编子程序结束。
	ENDP




		
; ================== 汇编结束 ==================
		
	; 当前文件指令代码要求4字节对齐，不然会有警告
	ALIGN	4
		
	; 汇编文件结束，每个汇编文件都需要一个END
	END
		
		

