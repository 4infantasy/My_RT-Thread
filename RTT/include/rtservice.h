
// 双向链表的相关操作，都在本文件中实现。

#include <rtdef.h>


// rt_list_t 类型的节点的初始化，就是将节点里面的next和prev这两个节点指针指向节点本身
rt_inline void rt_list_init ( rt_list_t *l )
{
	l->next = l->prev = l;
}



// 在双向链表头部插入一个节点
rt_inline void rt_list_insert_after ( rt_list_t *l, rt_list_t *n )
{
	l->next->prev = n;		// 把第二个节点的prev指向新节点
	n->next = l->next;		// 把新节点的next指向第二个节点
	
	l->next = n;			// 把头节点的next指向新节点
	n->prev = l;			// 把新节点的prev指向头节点
}



// 在双向链表尾部插入一个节点（在双向链表头部前面插入一个节点）
rt_inline void rt_list_insert_before ( rt_list_t *l, rt_list_t *n )
{
	l->prev->next = n;		// 把尾部节点的next指向新节点
	n->prev = l->prev;		// 把新节点的prev指向尾部节点
	
	l->prev = n;			// 把头节点的prev指向新节点
	n->next = l;			// 把新节点的next指向头节点
}



// 从双向链表中删除一个节点
rt_inline void rt_list_remove ( rt_list_t *n )
{
	n->prev->next = n->next;
	n->next->prev = n->prev;
	
	n->next = n->prev = n;
}


// 判断链表是否为空
rt_inline int rt_list_isempty(const rt_list_t *l)
{
    return l->next == l;
}

