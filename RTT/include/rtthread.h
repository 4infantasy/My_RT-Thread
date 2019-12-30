#ifndef __RT_THREAD_H__
#define __RT_THREAD_H__

#include <rtdef.h>
#include <rtconfig.h>
#include <rtservice.h>


// 将0地址强制类型类型转换为type，即(type *)0，然后通过指针访问结构体成员的方式获取到编译的大小，即( &((type *)0)->member )，
// 最后算出f_struct_ptr = ptr - ( &((type *)0)->member )
#define rt_container_of(ptr, type, member) \
		( (type *) ((char *)(ptr) - (unsigned long)( &((type *)0)->member ) ) )

		
/* 已知一个结构体里面的成员的地址，反推出该结构体的首地址 */
#define rt_list_entry(node, type, member) \
		rt_container_of(node, type, member)



extern rt_list_t rt_thread_priority_table[RT_THREAD_PRIORITY_MAX];

extern struct rt_thread *rt_current_thread;

extern struct rt_thread idle;
extern struct rt_thread rt_flag1_thread;
extern struct rt_thread rt_flag2_thread;


// 线程就绪优先级组
extern rt_uint32_t rt_thread_ready_priority_group;
		
		
		
rt_err_t rt_thread_init ( 	struct rt_thread *thread,
							const char *name,
							void (*entry)(void *parameter),
							void		*parameter,
							void		*stack_start,
							rt_uint32_t	stack_size,
							rt_uint8_t	priority);


rt_uint8_t *rt_hw_stack_init ( 	void		*tentry,
								void		*parameter,
								rt_uint8_t	*stack_addr );
								

void rt_system_scheduler_init (void);
void rt_system_scheduler_start (void);
void rt_schedule (void);

								
void rt_object_init( struct rt_object *object, 			// 要初始化的对象
					 enum rt_object_class_type type, 	// 对象的类型
					 const char *name )	;				// 对象的名字
								
								
char *rt_strncpy( char *dst, const char *src, rt_ubase_t n );
			

void rt_tick_increase(void);
					 

void rt_interrupt_enter(void);
void rt_interrupt_leave(void);
					 
					 
// 空闲线程初始化
void rt_thread_idle_init(void);
					 
// 阻塞延时
void rt_thread_delay(rt_tick_t tick);
					 
					 
// 调度器插入线程
void rt_schedule_insert_thread(struct rt_thread *thread);


// 调度器删除线程
void rt_schedule_remove_thread(struct rt_thread *thread);


// 线程启动函数
rt_err_t rt_thread_startup(rt_thread_t thread);


/*
	该函数用于从一个32位的数中寻找第一个被置1的位（从低位开始）
	返回该位的索引（即位号）
*/
int __rt_ffs(int value);





#endif


								
