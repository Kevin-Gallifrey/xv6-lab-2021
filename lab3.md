# Speed up system calls
在`kernel/proc.c`中，`allocproc()`初始化分配进程，需要为每个进程分配一个pagetable，参考`trapframe`，在kernel中分配`usyscall`空间。若分配失败，则释放该进程并返回；若分配成功，则对`usyscall`进行初始化，对其pid赋值。  
注：在proc结构体中需要加入`struct usyscall`指针。  

之后，设置user pagetable，使用`proc_pagetable()`完成。  
在`proc_pagetable()`中，可以仿照`trampoline`和`trapframe`，实现`usyscall`的映射。利用`mappages()`完成从虚拟地址到物理地址的映射。若失败，则需将之前的映射表删除并返回。在`mappages()`中可以设置权限，`PTE_R`是可读，`PTE_U`表示用户可以访问。  

在进程结束后需要释放内存。`freeproc()`用于释放proc结构体。同样参考`trapframe`，释放`usyscall`的空间并将指针指向0。对于proc中的`pagetable`同样需要释放，使用`proc_freepagetable()`完成。  
由于在初始化时，创建了`trampoline`，`trapframe`和`usyscall`的地址映射，此时需要将它们全部删除，最后释放user pagetable空间。

# Print a page table
递归调用`vmprint()`实现对整个pagetable tree的搜索，`pte & PTE_V`判断PTE是否合法，`PTE2PA(pte)`得到pte所对应的pa。

# Detecting which pages have been accessed
通过`argaddr()`和`argint()`获得传入的参数。  
通过`walk()`获得虚拟地址va对应的PTE。RISC-V中规定PTE的第十位是标志位，其中就有PTE_A。A位在程序运行过程中会被机器自动设置，此处只需要对A位进行读取，并且读取完成后清零。  
最后使用`copyout()`将kernel中得到的结果返回给user。