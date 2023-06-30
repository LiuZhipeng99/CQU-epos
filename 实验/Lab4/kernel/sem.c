/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 */
#include <stddef.h>
#include<string.h>
#include "kernel.h"
typedef struct SEM 
{
	int semid;
	int value;
	// struct PCB *L;
	struct wait_queue *wq;
	//采用id模式需要（参照task）我使用指针模式struct SEM* next;//不能直接sema定义next需要临时的struct命名
}semaphore;
struct PCB
{
	//看sleep on实现了struct PCB* head;//TODO:考虑到后阻塞的线程会先ready。准备定个头来按序ready
	struct tcb* l;
	struct PCB* next;//这才是阻塞线程链表
	
};

//创建结构体也要申请空间 参见taskcreate，虽然声明了个指针S但它没有指向的目标
//TODO:我该如何通过id找到sem来free，参见task是create时调用add形成链表；
//写到这发现无法debug只好先写用户层再一步一步debug

//返回值是指针ctx怎么接受==；而传入是指针怎么读取得指针的指针再取址
void* sys_sem_create(int value)
{
	semaphore *new;
	//static int ID =0;
	new = (semaphore*)kmalloc(sizeof(semaphore));
	//检查大小
    memset(new, 0, sizeof(semaphore));

	new->value = value;
	// new->L = NULL;//(struct PCB*)kmalloc(sizeof(struct PCB));
	//new->semid = ID++;
	new->wq = NULL;
	if(new!=NULL) return ( semaphore*) new; //TODO:s不知道怎么判断成功
	else return NULL;
}

int sys_sem_destroy(void *sem)
{

	kfree(sem);
    return 0;
}

int sys_sem_wait(void *sem)
{	
	uint32_t flags;
	save_flags_cli(flags);
	((semaphore*)sem)->value--;
	if ((( semaphore*)sem)->value < 0) {
	sleep_on(&((( semaphore*)sem)->wq));
	}

	// semaphore* S;
	// S->value--;
	// if(S->value<0){
	// 	S->L = (struct PCB*)kmalloc(sizeof(struct PCB));
	// 	//S->L->l =S->L->head = g_task_running;
	// 	S->L->l = g_task_running;
	// 	S->L = S->L->next;
	// 	g_task_running->state=TASK_STATE_WAITING;
	// 	schedule();
	// }

	restore_flags(flags);
    return -1;
}

int sys_sem_signal(void *sem)
{

	uint32_t flags;
	save_flags_cli(flags);
	(( semaphore* )sem)->value++;//V操作
	if((( semaphore*)sem)->value<=0) {
	wake_up(&((( semaphore*)sem)->wq), 1);//等 待队列中唤醒-个线程
	}
	// semaphore* S;
	// S->value++;
	// if(S->value<=0){
	// 	struct tcb* task;
	// 	task->state = TASK_STATE_READY;
	// }
	restore_flags(flags);
    return 0;
}

