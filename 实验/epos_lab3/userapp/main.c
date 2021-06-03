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
#define SPACING 4
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
void task_bubulle1Sort(void* pv){
	int* array = (int*)pv;
	//int x = g_graphic_dev.XResolution/2;
	int y = (g_graphic_dev.YResolution-100)/SPACING;
	for(int i=0;i<y;i++)
		line(0,i*SPACING,array[i],i*SPACING,RGB(255,0,0));
	for (int i = 0; i < y; i++)
 	{
    	for (int j = 0; j < y - i; j++)
      		if (array[j] > array[j + 1])      //两两比较，将小的数据放前面
      	{	int temp = array[j+1];
      		line(0,(j+1)*SPACING,array[j+1],(j+1)*SPACING,RGB(0,0,0));
       		array[j+1]=array[j];//交换arr数组arr[j+1]和arr[j]的值
      		line(0,(j+1)*SPACING,array[j+1],(j+1)*SPACING,RGB(255,0,0));
      		line(0,j*SPACING,array[j],j*SPACING,RGB(0,0,0));
			array[j]=temp;
      		line(0,j*SPACING,array[j],j*SPACING,RGB(255,0,0));
      		nanosleep((const struct timespec[]){{0,2000000L}}, NULL);
      	}
 	}
	task_exit(0);
}
void task_bubulle2Sort(void* pv){
	int* array = (int*)pv;
	int x = g_graphic_dev.XResolution/2;
	int y = (g_graphic_dev.YResolution-100)/SPACING;
	for(int i=0;i<y;i++)
		line(x,i*SPACING,x+array[i],i*SPACING,RGB(0,255,0));
	for (int i = 0; i < y; i++)
 	{
    	for (int j = 0; j < y - i; j++)
      		if (array[j] > array[j + 1])      //两两比较，将小的数据放前面
      	{	int temp = array[j+1];
      		line(x,(j+1)*SPACING,x+array[j+1],(j+1)*SPACING,RGB(0,0,0));
       		array[j+1]=array[j];//交换arr数组arr[j+1]和arr[j]的值
      		line(x,(j+1)*SPACING,x+array[j+1],(j+1)*SPACING,RGB(0,255,0));
      		line(x,j*SPACING,x+array[j],j*SPACING,RGB(0,0,0));
			array[j]=temp;
      		line(x,j*SPACING,x+array[j],j*SPACING,RGB(0,255,0));
      		nanosleep((const struct timespec[]){{0,2000000L}}, NULL);
      	}
 	}
	task_exit(0);
}
struct TID
{int tid1;
	int tid2;
	
};
void task_control(void* pv){//除了等待中断还有画优先级的功能(因为这里刷新优先级)
	struct TID* tids = (struct TID*)pv;
	int tidA = tids->tid1; int tidB = tids->tid2;
	int x = g_graphic_dev.XResolution/2;
    int y = g_graphic_dev.YResolution-100;
	line(0,y+50,2*x,y+50,RGB(255,255,255));//白线 刷新线和背景的作用 ps:y轴不变只关注x
	line(0,y+50,(40-getpriority(tidA))*x/40,y+50,RGB(255,0,0));
	line(x,y+50,(40-getpriority(tidB))*x/40+x,y+50,RGB(0,255,0));//这是b第二个优先级画线
	while(1){
		int key = getchar();
		if(key == 0x4800){
			if(getpriority(tidA)<=0) continue;
			setpriority(tidA,getpriority(tidA)-1);//优先级加一
			line(0,y+50,x,y+50,RGB(255,255,255));//白线 刷新线和背景的作用 ps:y轴不变只关注x
			line(0,y+50,(40-getpriority(tidA))*x/40,y+50,RGB(255,0,0));
		}
		if(key == 0x5000){
			if(getpriority(tidA)>=40) continue;
			setpriority(tidA,getpriority(tidA)+1);
			line(0,y+50,x,y+50,RGB(255,255,255));//白线 刷新线和背景的作用 ps:y轴不变只关注x
			line(0,y+50,(40-getpriority(tidA))*x/40,y+50,RGB(255,0,0));
		}
		if(key == 0x4d00){
			if(getpriority(tidB)<=0) continue;
			setpriority(tidB,getpriority(tidB)-1);
			line(x,y+50,2*x,y+50,RGB(255,255,255));//白线 刷新线和背景的作用 ps:y轴不变只关注x
			line(x,y+50,(40-getpriority(tidB))*x/40+x,y+50,RGB(0,255,0));
		}
		if(key == 0x4b00){
			if(getpriority(tidB)>=40) continue;
			setpriority(tidB,getpriority(tidB)+1);
			line(x,y+50,2*x,y+50,RGB(255,255,255));//白线 刷新线和背景的作用 ps:y轴不变只关注x
			line(x,y+50,(40-getpriority(tidB))*x/40+x,y+50,RGB(0,255,0));
		}
	}
	task_exit(0);
}
/**
 * 第一个运行在用户模式的线程所执行的函数
 */
void main(void *pv)
{	//test new syscall新的系统调用居然很花时间 注释后就很快了
    // printf("task #%d: I'm the first user task(pv=0x%08x)!\r\nits nice is %dand decrease2*1:%d\n",
    //         task_getid(), pv,getpriority(0),setpriority(0,1));

    //TODO: Your code goes here
    init_graphic(0x143);//143mode is 800*600的分辨率 144 is 1024*768
    //这会导致空白无法画线g_graphic_dev.YResolution = g_graphic_dev.YResolution-100;//这样能留空白？思考为什么乘(4/5)不行。。因为这个等于0
    int X = g_graphic_dev.XResolution/2;
    int Y = (g_graphic_dev.YResolution-100)/SPACING;
    srand(7777);//播种并生成随机数组
    int* array1=(int*)malloc(Y*sizeof(int));
    int* array2=(int*)malloc(Y*sizeof(int));
    for(int i=0;i<Y;i++){
    	array1[i]=rand()%X;
    	array2[i]=rand()%X;
    }//生成了五个范围0-159的大小600的数组
 	struct TID* tid = (struct TID*)malloc(sizeof(struct TID));
    void* stack1 = malloc(1024*1024)+1024*1024;//1M的栈空间的栈顶
    void* stack2 = malloc(1024*1024)+1024*1024;
    void* stack3 = malloc(1024*1024)+1024*1024;
    int tidA = task_create(stack1,&task_bubulle1Sort,(void*)array1);
    int tidB = task_create(stack2,&task_bubulle2Sort,(void*)array2);
    tid->tid1 = tidA;tid->tid2 = tidB;
  	int tidC = task_create(stack3,&task_control,(void*) tid);
  	setpriority(tidA,15);setpriority(tidB,15);
  	setpriority(tidC,0);//设置初始优先级为多少0~40 要求tidc最高，另外两个随便
    while(1)
        ;
    exit_graphic(); 
    task_exit(0);
}

