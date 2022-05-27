#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>

#define WSIZE 8 //双字数对齐 
#define DSIZE 4 //单字数对齐
#define CHUNKSIZE (1<<8) //扩展堆时的扩展的默认大小  
//开小一点当然ok了　如果开太大会导致　gdb运行成功　正常跑程序会失败
#define MAX_HEAP  (1<<12) //模拟虚拟内存大小
#define MAX(x,y) ((x)>(y)?(x):(y))
//因为此内存分配器使用边界分离法 所以第一个define实际是对边界的一个计算 分别为大小位和存在位
#define PACK(size,alloc) ((size)|(alloc)) // 

#define GET(p)     (*(unsigned int *)(p))
#define PUT(p,val) (*(unsigned int *)(p) = (val))//得到地址 从而赋值

//这里的0x7实际上就是后三位的存在位 加上~以后不就是我们的size位了吗 
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
//位运算得到的答案其实就是最后一位 就是边界标记上的是否存在

//bp -> block pointer 这两个宏函数通过当前指针位置计算头尾标记起始位
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

//计算next块的头 和prev块的尾
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) //加上get
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static char *mem_heap;    //用一个全局变量来保存堆顶 以便后面的 放置策略可以进行
static char *mem_brk;     //heap 与 brk之间的代表已分配的虚拟内存 brk与max_addr代表未分配的
static char *mem_max_addr;//最大的模拟虚拟内存地址
void *heap_listp;        //序言块与结尾块之间靠序言块那一侧的指针
void *next_listp;         //下一次适配的全局变量

//所使用函数
void men_init();
void *mem_sbrk(int incr);
int mm_init();
static void *extend_heap(size_t word);
void mm_free(void *bp);
static void* coalesce(void *bp);
void *mm_malloc(size_t size);
void *find_fit(size_t size);
void *next_find_fit(size_t size);
void *place(void *bp,size_t size);

/*对我们的三个全局变量赋值　为了更贴近我们的实际　所以有了mem_brk 
因为虚拟内存并不是初始时全部分配的　而是一个区域的集合　也就是按片分配的　
我们可以打开/proc 中任意一个数字目录进入maps目录查看虚拟内存的映射 显然是片状分配的
*/
void men_init()
{
    mem_heap=(char*)malloc(MAX_HEAP);
    mem_brk =(char*)mem_heap;
    mem_max_addr=(char*)(mem_heap+MAX_HEAP); 
}

void *mem_sbrk(int incr)
{
    char *old_brk=mem_brk;
    
    if(incr < 0 || ((mem_brk + incr) > mem_max_addr))
    {
        errno = ENOMEM;
        perror("error:ran out of memory!\n");
        return (void*)-1;
    }
    mem_brk=mem_brk+incr;
    return (void*)old_brk;  //malloc默认返回void类型指针
}

//初始化堆　使其具有堆的头与尾　把这个作为一个范围　头：序言块　　位：结尾块
int mm_init()
{
    if((heap_listp = mem_sbrk(4*WSIZE))==(void*) - 1) //证明虚拟内存不够
    return -1;
    PUT(heap_listp,0);   //内存对齐块　处于堆刚开始时的位置
    PUT(heap_listp+WSIZE,PACK(DSIZE,1)); 
    PUT(heap_listp+2*WSIZE,PACK(DSIZE,1)); 
    //因为是堆的初始化　所以这两个边界标记构成了一个内存块　有效荷载为零
    PUT(heap_listp+3*WSIZE,PACK(0,1));
    //在整个堆里有且只有一个结尾块　很特殊　size位为0 存在位为1

    heap_listp+=(2*WSIZE); 
    next_listp=heap_listp;
    //在序言块之后　结尾块之前　创建一个带空闲块的堆 注意只有一个　意味这堆的初始往往只有一个

    if(extend_heap(CHUNKSIZE/WSIZE)==NULL)
    return -1;
}

static void *extend_heap(size_t word) //一个字四个字节
{
    char *bp;
    size_t size;

    size = (word%2)?((word+1)*WSIZE):(word*WSIZE);//字节对齐
    if(((long)(bp = mem_sbrk(size)))==-1)
    return NULL;   //当请求的内存大于虚拟内存总内存的时候　返回NULL

    PUT(HDRP(bp),PACK(size,0));
    PUT(FTRP(bp),PACK(size,0));  //代表这个大的空闲块大小为size 且为空闲块
    PUT(HDRP(NEXT_BLKP(bp)),PACK(0,1)); //更新结尾块
    return coalesce(bp); //从bp位置可以开始分配内存　把内存块的后面与新分配的内存进行合并
}

void mm_free(void *bp)
{
    size_t size=GET_SIZE(HDRP(bp));

    PUT(HDRP(bp),PACK(size,0));
    PUT(FTRP(bp),PACK(size,0));
    coalesce(bp); //很容易理解　在每次free过后进行内存合并
}

static void* coalesce(void *bp)  //合并
{
    //得到当前块前后块的存在位信息
    size_t prev_alloc=GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc=GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size=GET_SIZE(HDRP(bp));  //要初始化合并以后的空闲块
    //对四种合并情况进行讨论
    if(prev_alloc && next_alloc)  //不进行合并
    {
        return bp;
    }   
    else if(!prev_alloc && next_alloc) //prev可合并　next不可合并
    {
        size+=GET_SIZE(HDRP(PREV_BLKP(bp)));//这个位置头尾(第二个宏函数)都可以　都是相同的
        PUT(FTRP(bp),PACK(size,0)); //先把本身的尾部标记设置
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0)); //再把prev的头部标记设置
        bp=PREV_BLKP(bp); //感觉有点问题
    }else if (prev_alloc && !next_alloc) //prev不可合并 next可合并
    {
        size+=GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp),PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0));
    }else  //均可合并
    {
        size=size+GET_SIZE(HDRP(PREV_BLKP(bp)))+GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0)); 
        //相当与把三个内存块和成了一个只需要把前面的头　与后面的尾进行设置就好
    }
    return bp;
}

void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extandsizes;
    char *bp;

    if(size==0)
    return NULL;
    else if (size < 2*DSIZE) 
    asize = 2*DSIZE;
    else 
    asize = DSIZE*((size+DSIZE+DSIZE-1)/DSIZE);//进行字节对齐　加上两个标记位　一个为WSIZE

    if((bp=find_fit(asize))!=NULL) //命中　也就是说目前堆中已载入的空闲链表中有一个空闲块大于等于size
    {
        place(bp,asize);
        next_listp=NEXT_BLKP(HDRP(bp)); //更新下一次适配标记位　同下
        return bp;
    }
    extandsizes = MAX(asize,CHUNKSIZE);  //否则就是不命中的情况
    if((bp=extend_heap(extandsizes/DSIZE))==NULL)
    return NULL;   //说明虚拟内存已经消耗殆尽了
    place(bp,asize);//asize个字节 进行内存替换　
    next_listp=NEXT_BLKP(HDRP(bp));
    return bp;
}

void *find_fit(size_t size)
{
    void *bp;  //放置策略为首次适配 因为有序言块　所以从heap_listp开始寻找 而不是mem_heap
    //printf("%d\n",GET_SIZE(HDRP(bp)));
    for(bp = heap_listp;GET_SIZE(HDRP(bp))!=0;bp=NEXT_BLKP(bp)) //终止条件原因是终止块为零
    if(!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp))>=size)
    return bp; //找到匹配
    return NULL;
}

void *next_find_fit(size_t size) //放置策略为 下一次放置
{
    void *bp;  //放置策略为首次适配 因为有序言块　所以从heap_listp开始寻找 而不是mem_heap
    //printf("%d\n",GET_SIZE(HDRP(bp)));
    for(bp = next_listp;GET_SIZE(HDRP(bp))!=0;bp=NEXT_BLKP(bp)) //终止条件原因是终止块为零
    if(!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp))>=size)
    return bp; //找到匹配
    else
    {
        if(bp=find_fit(size))
        {
            next_listp=NEXT_BLKP(HDRP(bp)); //更新下一次适配位
            return bp;
        }
        return NULL;//从头开始还是没有找到的话进行从新分配块
    }
}

void *place(void *bp,size_t size)
{
    size_t asize=GET_SIZE(HDRP(bp));
    if((asize-size)>=2*DSIZE)//大于最小块 进行分离操作 这也是产生外部内存碎片的原因
    {
        PUT(HDRP(bp),PACK(size,1));
        PUT(FTRP(bp),PACK(size,1));
        *(FTRP(bp)-1)='\0';     //分配一个字符串末尾
        bp=NEXT_BLKP(bp);//头的大小位被设为size 所以bp可定位到
        PUT(HDRP(bp),PACK(asize-size,0));
        PUT(FTRP(bp),PACK(asize-size,0));
    }else
    {
        PUT(HDRP(bp),PACK(asize,1));
        PUT(FTRP(bp),PACK(asize,1));
        *(FTRP(bp)-1)='\0';
    }
}

int main()
{
    men_init(); //初始化虚拟空间
    mm_init();  //初始化堆空间
    //具体的思路是当堆空间不够的时候进行重新分配　当虚拟空间不够的时候进行报错
    char *tmp=(char*)mm_malloc(sizeof(char)*256);
    strcpy(tmp,"hello world!");
    strcat(tmp,"\0");
    printf("::%s\n",tmp);
    size_t bbb = GET_ALLOC(HDRP(tmp));
    mm_free(tmp);
    size_t aaa = GET_ALLOC(HDRP(tmp));
    printf("%s\nend\n",tmp);
    /*可以执行的原因是因为mm_free只是把标记改变而已 
    本质上内存还是在刚开始我们malloc出的那个虚拟空间上 只不过这部分内存现在可以被mm_malloc分配 
    也就是说值可以被改变　其实已经成了一个　假的“空悬指针”　*/
    free(mem_heap); //防止内存泄露　
    //写完这个程序　再看这个系统的free　真是别有一番感觉啊
    return 0;
}
