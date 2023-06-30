/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 */
#include <sys/types.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <syscall.h>
#include <stdio.h>
// #include <assert.h>
// #include <memcpy.c>
//实现首次分配算法
struct chunk {
	char signature[4];  /* "OSEX" */
	struct chunk *next; /* ptr. to next chunk */
	int state;          /* 0 - free, 1 - used */
#define FREE   0
#define USED   1

	int size;           /* size of this chunk */
};

static struct chunk *chunk_head;

static void* mutex = NULL;
//mutex = sem_create(1);!!!!他不能在这里初始化要到函数去(初始化函数）原因未知 只一个全局互斥量为了保护内存分配
void *g_heap;
void *tlsf_create_with_pool(uint8_t *heap_base, size_t heap_size)
{
	chunk_head = (struct chunk *)heap_base;
	strncpy(chunk_head->signature, "OSEX", 4);
	chunk_head->next = NULL;
	chunk_head->state = FREE;
	chunk_head->size  = heap_size;
	mutex = sem_create(1);
	// printf("create!!!!!\n" );
	return NULL;
}

void *malloc(size_t size)
{//bugs:花了半小时debug发现warn是无返回值;第二bug是createsem；第三次是类型转换值转换指针类型 我没加*(然而并不是；经过一次次断点发现是strncpy函数报错；耗时2h)
	//(然而也不是，发现errcode=6是new的结构体无法初始化；是自己多想了想head一样给结构体分配位置空间)直接声明会分配到前面位置吗
	//发现结构体内部无法访问都是空但又存在//发现warn说new未初始化肯定我没想多 测试发现是new位置没找对!!!!!起初以为是类型问题其实是错用ptr的size代替size溢出了
	//第一阶段总结：malloc的语法bug就第一二用了1-2小时，第三个逻辑bug用了起码三个小时断点找位置猜错原因继续假设才成功 结果是小小的逻辑错误
	//1.5huge blockbug 又是06的code估计又是那个new位置问题（确实）但看群说是free确实有可能,查验并无，于是考虑到下面测试可能覆盖全部空间，也就是插入时不产生新节点(size+struct>ptr->size>size)
	//考虑到等号临界条件：大于等于size而小于等于size+struct，等于是因为不能分配size为0的会直接return
	//注释前面4测试才发现size是全堆空间，说明我free增大了chunkhead的大小(找headsize更新)实在找不出去测试代码检查接口函数调用哪里发生size变化结果是第二个free
	//2h最终bug解决!!!震惊的发现是ptr只更新state和next之前竟然没有发现问题
	if(size==0)
		return NULL;
	else
	{//返回地址应该是uint的指针类型；
		sem_wait(mutex);
		// printf("%s\n",chunk_head->size);
	//uint8_t* ret_ptr = (uint8_t*)chunk_head+sizeof(struct chunk);
		struct chunk* ptr = chunk_head;
		while(ptr!=NULL){//补充考虑到size还需要个chunk的头，又考虑到初始化new所有得初始化
			if(((ptr->size)>(size+sizeof(struct chunk)))&&(ptr->state==FREE)){//首次分配符合 插入节点
			//2.0:返回时想起忘了改state
				//下面注释代码为分类版
				struct chunk* new = (struct chunk*)((uint8_t*)ptr+sizeof(struct chunk)+size);
				new->state = FREE;
				new->size = ptr->size - size - sizeof(struct chunk);
				strncpy(new->signature, "OSEX", 4);
				if(ptr->next==NULL) new->next=NULL;
				else new->next = ptr->next;
				ptr->state = USED;
				ptr->next = new;
				ptr->size = size;
				// if(ptr->next==NULL){//在尾部插入
				// 	struct chunk* new;
				// 	// uint8_t* adr = ((uint8_t *)ptr+(size+sizeof(struct chunk)));
				// 	new = (struct chunk*)((uint8_t *)ptr+sizeof(struct chunk)+size);
				// 	new->state = FREE;
				// 	new->size = ptr->size - size - sizeof(struct chunk);
				// 	strncpy(new->signature, "OSEX", 4);
				// 	new->next = NULL;
				// 	ptr->next = new;}
				// else{//1 2 3.。。。的之间例如2 3间插入
				// 	// struct chunk* temp = ptr->next;
				// 	struct chunk* new = (struct chunk*)((uint8_t*)ptr+sizeof(struct chunk)+size);
				// 	new->state = FREE;
				// 	new->size = ptr->size - size - sizeof(struct chunk);
				// 	strncpy(new->signature, "OSEX", 4);
				// 	new->next = ptr->next;
				// 	ptr->next = new;}

				sem_signal(mutex);
				return (void*)((uint8_t*)ptr + sizeof(struct chunk));
			}
			else if((ptr->size>=size)&&(ptr->state==FREE)){
				ptr->state = USED;//不用加节点的操作
				sem_signal(mutex);
				return (void*)((uint8_t*)ptr+sizeof(struct chunk));
			}
			ptr = ptr->next;
		}
		sem_signal(mutex);
		return NULL;//到这里可能是break成功返回可能是while结束；还是将break换为return
	}
}//debug:是int+uint8不行但应该符合大的位数那个
void free(void *ptr)
{
  if(ptr==NULL) return ;
  else{
  	sem_wait(mutex);
	struct chunk* cur = (struct chunk*)(((uint8_t*)ptr)-sizeof(struct chunk));
	// assert(cur->signature=="OSEX");//本句查定义无用

	if(!strncmp(cur->signature,"OSEX",4)){//查阅string的strncmp定义
	  //检查前后节点是否是free，依次合并
	  struct chunk* ptr = chunk_head;
	  cur->state = FREE;
	  while(ptr!=NULL){//循环才能找上个节点
		if(ptr->next==cur){//找到了上一节点
		  if(ptr->state==FREE){
			struct chunk* temp = cur;
			cur = ptr;
			cur->size = ptr->size+sizeof(struct chunk)+temp->size;
			cur->next = temp->next;  //相当于删除了原来cur节点
		  }
		}
		ptr = ptr->next;
	  }
	  if(cur->next->state==FREE){//那么这next节点删除接上下下个保证链表；修正size是第一步
		cur->size = cur->size + sizeof(struct chunk)+cur->next->size;//这里假设sizeof能自动转int
		cur->next = cur->next->next;//1.5debug:如果这项为空
	  }
	}
	sem_signal(mutex);
  }
}

void *calloc(size_t num, size_t size)
{//定义上猜测这元素地址连续的因为通过[]寻址
	if(size==0||num==0)
		return NULL;
	else{
		uint8_t* addr = (uint8_t*)malloc(num*size);
		for(size_t i=0;i<num*size;i++){
			*(addr+i) = 0;
		}
		return (void*)addr;

	}
}

void *realloc(void *oldptr, size_t size)
{
	if(oldptr==NULL) return malloc(size);
	if(size==0){
		free(oldptr);
		return NULL;}
	struct chunk* cur = (struct chunk*)(((uint8_t*)oldptr)-sizeof(struct chunk));
	if(!strncmp(cur->signature,"OSEX",4)){
		if(cur->size<size){//还得去找块size大小空间,然后复制原地址并free
			uint8_t* new = (uint8_t*)malloc(size);
			for(size_t i=0;i<cur->size;i++)
				*(new+i) = *((uint8_t*)oldptr+i);
			free(oldptr);
			// memcpy(new,oldptr,cur->size);
			return (void*)new;
		}
		else{//原空间够了，只需要释放后面的PS这里注意到只需要新空间末地址，但参数是默认有struct的
			free(oldptr+size+sizeof(struct chunk));
			return oldptr;
		}
	}
	return NULL;
}

/*************D O  N O T  T O U C H  A N Y T H I N G  B E L O W*************/
static void tsk_malloc(void *pv)
{
  int i, c = (int)pv;
  char **a = malloc(c*sizeof(char *));
  for(i = 0; i < c; i++) {
	  a[i]=malloc(i+1);
	  a[i][i]=17;
  }
  for(i = 0; i < c; i++) {
	  free(a[i]);
  }
  free(a);

  task_exit(0);
}

#define MESSAGE(foo) printf("%s, line %d: %s", __FILE__, __LINE__, foo)
void test_allocator()
{
  char *p, *q, *t;
  // int ptr1 = chunk_head->size;

  MESSAGE("[1] Test malloc/free for unusual situations\r\n");
  MESSAGE("  [1.1]  Allocate small block ... ");
  p = malloc(17);
  if (p == NULL) {
	printf("FAILED\r\n");
	return;
  }
  p[0] = p[16] = 17;
  printf("PASSED\r\n");

  MESSAGE("  [1.2]  Allocate big block ... ");
  q = malloc(4711);
  if (q == NULL) {
	printf("FAILED\r\n");
	return;
  }
  q[4710] = 47;
  printf("PASSED\r\n");

  MESSAGE("  [1.3]  Free big block ... ");
  free(q);
  printf("PASSED\r\n");

  MESSAGE("  [1.4]  Free small block ... ");
  free(p);
  printf("PASSED\r\n");

  // int ptr2 = chunk_head->size;
  // if(ptr1!=ptr2) printf("yess!!\n");//只有size改变了
  MESSAGE("  [1.5]  Allocate huge block ... ");
  

  q = malloc(32*1024*1024-sizeof(struct chunk));
  if (q == NULL) {
	printf("FAILED\r\n");
	return;
  }
  q[32*1024*1024-sizeof(struct chunk)-1]=17;
  free(q);
  printf("PASSED\r\n");

  MESSAGE("  [1.6]  Allocate zero bytes ... ");
  if ((p = malloc(0)) != NULL) {
	printf("FAILED\r\n");
	return;
  }
  printf("PASSED\r\n");

  MESSAGE("  [1.7]  Free NULL ... ");
  free(p);
  printf("PASSED\r\n");

  MESSAGE("  [1.8]  Free non-allocated-via-malloc block ... ");
  int arr[5] = {0x55aa4711,0x5a5a1147,0xa5a51471,0xaa551741,0x5aa54171};
  free(&arr[4]);
  if(arr[0] == 0x55aa4711 &&
	 arr[1] == 0x5a5a1147 &&
	 arr[2] == 0xa5a51471 &&
	 arr[3] == 0xaa551741 &&
	 arr[4] == 0x5aa54171) {
	  printf("PASSED\r\n");
  } else {
	  printf("FAILED\r\n");
	  return;
  }

  MESSAGE("  [1.9]  Various allocation pattern ... ");
  int i;
  size_t pagesize = sysconf(_SC_PAGESIZE);
  for(i = 0; i < 7411; i++){
	p = malloc(pagesize);
	p[pagesize-1]=17;
	q = malloc(pagesize * 2 + 1);
	q[pagesize*2]=17;
	t = malloc(1);
	t[0]=17;
	free(p);
	free(q);
	free(t);
  }

  char **a = malloc(2741*sizeof(char *));
  for(i = 0; i < 2741; i++) {
	  a[i]=malloc(i+1);
	  a[i][i]=17;
  }
  for(i = 0; i < 2741; i++) {
	  free(a[i]);
  }
  free(a);

  if(chunk_head->next != NULL || chunk_head->size != 32*1024*1024) {
	printf("FAILED\r\n");
	return;
  }
  printf("PASSED\r\n");

  MESSAGE("  [1.10] Allocate using calloc ... ");
  int *x = calloc(17, 4);
  for(i = 0; i < 17; i++)
	  if(x[i] != 0) {
		  printf("FAILED\r\n");
		  return;
	  } else
		  x[i] = i;
  free(x);
  printf("PASSED\r\n");

  MESSAGE("[2] Test realloc() for unusual situations\r\n");

  MESSAGE("  [2.1]  Allocate 17 bytes by realloc(NULL, 17) ... ");
  p = realloc(NULL, 17);
  if (p == NULL) {
	printf("FAILED\r\n");
	return;
  }
  p[0] = p[16] = 17;
  printf("PASSED\r\n");
  MESSAGE("  [2.2]  Increase size by realloc(., 4711) ... ");
  p = realloc(p, 4711);
  if (p == NULL) {
	printf("FAILED\r\n");
	return;
  }
  if ( p[0] != 17 || p[16] != 17 ) {
	printf("FAILED\r\n");
	return;
  }
  p[4710] = 47;
  printf("PASSED\r\n");

  MESSAGE("  [2.3]  Decrease size by realloc(., 17) ... ");
  p = realloc(p, 17);
  if (p == NULL) {
	printf("FAILED\r\n");
	return;
  }
  if ( p[0] != 17 || p[16] != 17 ) {
	printf("FAILED\r\n");
	return;
  }
  printf("PASSED\r\n");

  MESSAGE("  [2.4]  Free block by realloc(., 0) ... ");
  p = realloc(p, 0);
  if (p != NULL) {
	printf("FAILED\r\n");
	return;
  } else
	printf("PASSED\r\n");

  MESSAGE("  [2.5]  Free block by realloc(NULL, 0) ... ");
  p = realloc(realloc(NULL, 0), 0);
  if (p != NULL) {
	printf("FAILED\r\n");
	return;
  } else
	printf("PASSED\r\n");

  MESSAGE("[3] Test malloc/free for thread-safe ... ");

  int t1, t2;
  char *s1 = malloc(1024*1024),
	   *s2 = malloc(1024*1024);
  t1=task_create(s1+1024*1024, tsk_malloc, (void *)5000);
  t2=task_create(s2+1024*1024, tsk_malloc, (void *)5000);
  task_wait(t1, NULL);
  task_wait(t2, NULL);
  free(s1);
  free(s2);

  if(chunk_head->next != NULL || chunk_head->size != 32*1024*1024) {
	printf("FAILED\r\n");
	return;
  }
  printf("PASSED\r\n");
}
/*************D O  N O T  T O U C H  A N Y T H I N G  A B O V E*************/
