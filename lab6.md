# Uthread: switching between threads
在`thread_schedule()`中调用`thread_switch()`实现储存和恢复寄存器，完成thread的切换。  
查看汇编代码，发现`thread_a`, `thread_b`, `thread_c`三个函数在调用时分别加载了寄存器`ra, s0, s1, s2, s3, s4`以及需要一个栈`sp`。所以在进程切换时，这些寄存器是需要保存的。其中，`s1`存放的是循环计数器的值，`s2`是变量`a_n, b_n, c_n`的地址，`s3`是for循环中的条件100，`s4`是`printf`中对应的语句的地址。  
在`thread_create()`中要向thread结构体中存放返回地址ra和栈指针sp，这使得schedule可以切换到对应的进程。入参`func`是对应的函数地址，`ra = func`，schedule就可以返回到这个函数继续执行了。

# Using threads
在`put()`中涉及对共享数据的写操作，需要加锁，`insert()`返回的是新创建的entry的地址，如果不加锁，两个进程同时创建一个新的entry指向原来的entry并返回，会丢失一个新建的entry。  
由于各个bucket的链表是独立的，为了能够并行操作，为每个bucket配一把锁，这样涉及到不同的bucket时就可以并行执行了。  
`pthread_mutex_t lock`：定义锁  
`pthread_mutex_init(&lock, NULL)`：初始化锁，所有锁需要先初始化后才能使用  
`pthread_mutex_lock(&lock)`：加锁  
`pthread_mutex_unlock(&lock)`：释放  
`assert(expression)`：当expression不为真时，程序报错并退出。

# Barrier
当所有进程都到达`barrier()`之后`bstate.round`加1，`bstate.nthread`清零。这两部操作在每一个round都只能执行一次。  
`pthread_cond_wait(&cond, &mutex)`：类似于sleep，释放锁并等待，当被唤醒后，重新获取锁并返回。调用前需要获得锁。  
`pthread_cond_broadcast(&cond)`：类似于wakeup，唤醒在cond等待的所有进程。