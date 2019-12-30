#include <rtthread.h>
#include <rthw.h>



// 对象容器数组的下标定义，决定容器的大小
enum rt_object_info_type
{
	RT_Object_Info_Thread = 0,		// 对象是线程
	
#ifdef RT_USING_SEMAPHORE

	RT_Object_Info_Semaphore,		// 对象是信号量
	
#endif

#ifdef RT_USING_MUTEX
	
	RT_Object_Info_Mutex,			// 对象是互斥量

#endif

#ifdef RT_USING_EVENT
	
	RT_Object_Info_Event,			// 对象是事件

#endif

#ifdef RT_USING_MAILBOX

	RT_Object_Info_MailBox,			// 对象是邮箱

#endif

#ifdef RT_USING_MESSAGEQUEUE

	RT_Object_Info_MessageQueue,	// 对象是消息队列
	
#endif

#ifdef RT_USING_MEMHEAP

	RT_Object_Info_MemHeap,			// 对象是内存堆

#endif

#ifdef RT_USING_MEMPOOL

	RT_Object_Info_MemPool,			// 对象是内存池

#endif

#ifdef RT_USING_DEVICE

	RT_Object_Info_Device,			// 对象是设备

#endif

#ifdef RT_USING_TIMER

	RT_Object_Info_Timer,			// 对象是定时器

#endif

#ifdef RT_USING_MODULE

	RT_Object_Info_Module,			// 对象是模块

#endif

	RT_Object_Info_Unknown,			// 对象未知

};


// 初始化对象列表节点头里面的next和prev两个节点指针分别指向自身。
#define _OBJ_CONTAINER_LIST_INIT(c)		\
{ &(rt_object_container[c].object_list), &(rt_object_container[c].object_list) }



static struct rt_object_information
rt_object_container[RT_Object_Info_Unknown] = {

	// 初始化对象容器 - 线程
	{
		RT_Object_Class_Thread,
		_OBJ_CONTAINER_LIST_INIT(RT_Object_Info_Thread),
		sizeof(struct rt_thread)
	},

#ifdef RT_USING_SEMAPHORE
	// 初始化对象容器 - 信号量
	{
		RT_Object_Class_Semaphore,
		_OBJ_CONTAINER_LIST_INIT(RT_Object_Info_Semaphore),
		sizeof(struct rt_semaphore)
	},
#endif

#ifdef RT_USING_MUTEX
	// 初始化对象容器 - 互斥量
	{
		RT_Object_Class_Mutex,
		_OBJ_CONTAINER_LIST_INIT(RT_Object_Info_Mutex),
		sizeof(struct rt_mutex)
	},
#endif

#ifdef RT_USING_EVENT
	// 初始化对象容器 - 事件
	{
		RT_Object_Class_Event,
		_OBJ_CONTAINER_LIST_INIT(RT_Object_Info_Event),
		sizeof(struct rt_event)
	},
#endif

#ifdef RT_USING_MAILBOX
	// 初始化对象容器 - 邮箱
	{
		RT_Object_Class_MailBox,
		_OBJ_CONTAINER_LIST_INIT(RT_Object_Info_MailBox),
		sizeof(struct rt_mailbox)
	},
#endif

#ifdef RT_USING_MESSAGEQUEUE
	// 初始化对象容器 - 消息队列
	{
		RT_Object_Class_MessageQueue,
		_OBJ_CONTAINER_LIST_INIT(RT_Object_Info_MessageQueue),
		sizeof(struct rt_messagequeue)
	},
#endif

#ifdef RT_USING_MEMHEAP
	// 初始化对象容器 - 内存堆
	{
		RT_Object_Class_MemHeap,
		_OBJ_CONTAINER_LIST_INIT(RT_Object_Info_MemHeap),
		sizeof(struct rt_memheap)
	},
#endif

#ifdef RT_USING_MEMPOOL
	// 初始化对象容器 - 内存池
	{
		RT_Object_Class_MemPool,
		_OBJ_CONTAINER_LIST_INIT(RT_Object_Info_MemPool),
		sizeof(struct rt_mempool)
	},
#endif

#ifdef RT_USING_DEVICE
	// 初始化对象容器 - 设备
	{
		RT_Object_Class_Device,
		_OBJ_CONTAINER_LIST_INIT(RT_Object_Info_Device),
		sizeof(struct rt_device)
	},
#endif

#ifdef RT_USING_TIMER
	// 初始化对象容器 - 定时器
	// 每个线程在创建的时候都会自带一个定时器，但是目前还没有在线程中加入定时器，所以这部分初始化先注释掉。
	{
		RT_Object_Class_Timer,
		_OBJ_CONTAINER_LIST_INIT(RT_Object_Info_Timer),
		sizeof(struct rt_timer)
	},
#endif

#ifdef RT_USING_MODULE
	// 初始化对象容器 - 模块
	{
		RT_Object_Class_Module,
		_OBJ_CONTAINER_LIST_INIT(RT_Object_Info_Module),
		sizeof(struct rt_module)
	},
#endif


};




// 从容器中获取指定类型的对象的信息，由函数rt_object_get_information实现。
struct rt_object_information *
rt_object_get_information(enum rt_object_class_type type)
{
	
	int index;
	
	for ( index = 0; index < RT_Object_Info_Unknown; index ++ ) {
	
		if ( rt_object_container[index].type == type ) {
		
			return &rt_object_container[index];
		}
	}

	return RT_NULL;
}


// 该函数将初始化对象并将对象添加到对象容器中
// 每个对象的控制块开头的成员都是对象信息相关的成员，比如一个线程控制块，它的开头前面4个成员都是与对象信息相关的，
// 在调用rt_object_init()函数的时候，只需将线程控制块强制类型转化为struct rt_object作为第一个形参即可。
void rt_object_init( struct rt_object *object, 			// 要初始化的对象
					 enum rt_object_class_type type, 	// 对象的类型
					 const char *name )					// 对象的名字
{

	register rt_base_t temp;
	struct rt_object_information *information;
	
	// 获取对象信息，即从容器里拿到对应对象列表头指针
	information = rt_object_get_information(type);
	
	// 设置对象类型为静态
	object->type = type | RT_Object_Class_Static;
	
	// 拷贝名字
	rt_strncpy(object->name, name, RT_NAME_MAX);
	
	// 关中断
	temp = rt_hw_interrupt_disable();
	
	// 将对象插入到容器的对应列表中，不同类型的对象所在的列表不一样。
	rt_list_insert_after( &(information->object_list), &(object->list) );
	
	// 使能中断
	rt_hw_interrupt_enable(temp);

}

