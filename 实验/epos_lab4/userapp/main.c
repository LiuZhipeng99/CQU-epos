/*
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 */
#include <inttypes.h>
#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <sys/mman.h>
#include <syscall.h>
#include <netinet/in.h>
#include <stdlib.h>
#include "graphics.h"

extern void *tlsf_create_with_pool(void* mem, size_t bytes);
extern void *g_heap;

/**
 * GCC insists on __main
 *    http://gcc.gnu.org/onlinedocs/gccint/Collect2.html
 */
void __main()
{
    size_t heap_size = 32*1024*1024;
    void  *heap_base = mmap(NULL, heap_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
	g_heap = tlsf_create_with_pool(heap_base, heap_size);
}

/**
 * 第一个运行在用户模式的线程所执行的函数
 */
#define N 6 //缓冲区数
#define NUM 400
static void* mutex = NULL;
//互斥信号量 有什么用==不能同时读写一个数组
static void* full = NULL;//已生产产品数
static void* empty = NULL; //空闲数
static int startposition= 0;//生 产者缓冲区x坐标
static int count = 0; //消费者缓冲区x坐标
int temp[N][NUM] = { 0 };
int array[N] [NUM];
long delay1=200000;
long delay2=400000L;

void task_conducter(void* pv)
{
	while (1){
		nanosleep((const struct timespec[]){{0,delay1}}, NULL);

		int i=0;
		srand(time(NULL));
		sem_wait(empty) ;//是否有空闲位置?
		sem_wait(mutex) ;
		for (i=0;i<g_graphic_dev.YResolution;i++){

		array [startposition][i] =  rand()% (g_graphic_dev.XResolution/N);
		line(startposition*g_graphic_dev.XResolution/N,i,array[startposition][i]+startposition*g_graphic_dev.XResolution/N,i,RGB(255,0,255));
		nanosleep((const struct timespec[]){{0,delay1}}, NULL);
		}		
		sem_signal(mutex) ;
		startposition=startposition+1;
		startposition = startposition%N;


		sem_signal(full) ;//有产品生产好
	}



	task_exit(0);
}
void task_consumer(void* pv){

	while(1){

		sem_wait(full) ;
		sem_wait(mutex) ;
		int i;
		for (i=0;i<g_graphic_dev.YResolution;i++){;
		//temp[count][i] = array[count][i];
		line(count*g_graphic_dev.XResolution/N,i,(count+1)*g_graphic_dev.XResolution/N,i,RGB(0,0,0));
		nanosleep((const struct timespec[]){{0,delay2}}, NULL);
		}
		sem_signal(mutex) ;//让出临界区使生产与消费并行
		//sort_m(array[count], g_graphic_dev.YResolution, count * g_graphic_dev.XResolution);// 冒泡排序

		//resetAllBK (array[count], g_graphic_dev.YResolution, count * g_graphic_dev.XResolution); //清空该缓冲区
		
		count++;
		count = count%N;//要0123
		sem_signal(empty);//消费完- -个产品



	}

	task_exit(0);
}

void task_controler(void* pv){
	while(1){
		int key = getchar();
		switch(key){
			case 0x4800:
				delay1-=2000000L;break;
			case 0x5000:
				delay1+=2000000L;break;
			case 0x4d00:
				delay2-=2000000L;break;
			case 0x4b00:
				delay2+=2000000L;break;
		}
	}
	task_exit(0);
}
void main(void *pv)
{
    printf("task #%d: I'm the first user task(pv=0x%08x)!\r\n",
            task_getid(), pv);

    //TODO: Your code goes here
    init_graphic(0x143);
    //int X = g_graphic_dev.XResolution;
    //int Y = g_graphic_dev.YResolution;
    mutex=sem_create(5);
    full = sem_create(0);
    empty = sem_create(N);
    void* stack1Top = malloc(1024*1024)+1024*1024;
    void* stack2Top = malloc(1024*1024)+1024*1024;
    void* stack3Top = malloc(1024*1024)+1024*1024;
    task_create(stack1Top,&task_conducter,NULL);
    task_create(stack2Top,&task_consumer,NULL);
    task_create(stack3Top,&task_controler,NULL);

    while(1)
        ;
    exit_graphic();
    printf("test");

    void* test=NULL;
    test = sem_create(3);
    //sem_wait(test);
sem_wait(test);
sem_wait(test);
sem_wait(test);
sem_wait(test);
sem_signal(test);
sem_signal(test);
    printf("test");

    task_exit(0);
}

//最后一个bug非sem函数搞错了，调了一两个小时，最后发现：生产者和消费者不能同时工作，原因其实是mutex信号量
//应该给每个缓冲区分配个互斥信号量要么就不要互斥只是同步，为什么因为racecondition避免操作同一数据，对于这二维数组数据单元不是整个，c可以同时做不同区操作；