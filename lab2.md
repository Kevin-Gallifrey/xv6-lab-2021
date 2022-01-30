# System call tracing
## system call 调用过程
从 user space 进入 kernel space 需要通过特定的`entry`，利用`ecall`函数将需要调用的system call所对应的编号传入kernel space，并接收对应的返回值，见`user/usys.pl`。  
在kernel space中，使用`syscall`函数接收传入的参数，调用对应的系统函数并返回，见`kernel/syscall.c`。  
## syscall 函数实现
在`syscall`中，首先通过`myproc()`获得了当前进程信息`proc`的指针。`proc`中存放进程的相关信息。系统初始时为每个进程都分配了一个`proc`。  
`trace`通过给`proc`中新定义的`mask`字段赋值，在`syscall`每次执行时都会检查`num`是否与`mask`相等，然后打印`trace`信息。  
在进程结束时，调用`freeproc`清理进程信息，此时需要将`mask`清零，以免之后的进程再次打印trace信息。

# Sysinfo
利用`argaddr`读取传入的地址参数。  
利用`copyout`将kernel中存在`s`的数据传到user中地址参数所指向的位置。  
`kernel/kalloc.c`中`freelist`是空页的链表，所有空页都能顺着链表找到。