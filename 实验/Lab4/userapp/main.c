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

void main(void *pv)
{
    printf("task #%d: I'm the first user task(pv=0x%08x)!\r\n",
            task_getid(), pv);

    //TODO: Your code goes here
    printf("test");
    extern void test_allocator();
    test_allocator();//未找到这个函数怎么办 extern
    while(1)
        ;
    exit_graphic();

    task_exit(0);
}

//最后一个bug非sem函数搞错了，调了一两个小时，最后发现：生产者和消费者不能同时工作，原因其实是mutex信号量
//应该给每个缓冲区分配个互斥信号量要么就不要互斥只是同步，为什么因为racecondition避免操作同一数据，对于这二维数组数据单元不是整个，c可以同时做不同区操作；