# Copy-on-write fork
在fork时先不copy物理地址中的内容，将child的地址映射到和parent相同的地方。此时需要在PTE中禁用写权限。当需要对内存进行写入时，触发page fault，再分配内存，完成copy，开启写权限，并继续之前的程序。  

在`usertrap`中，检查scause寄存器，若其值为13和15表示是page fault引发的，此时判断是否是COW引发，若是，则分配内存页并拷贝。发生page fault时，stval寄存器存放的是对应的virtual address。  

判断是否因为COW而触发page fault，可以采用PTE的标志位。PTE共用10个标志位(0~9)，其中8和9是保留的，可以用作判断COW。  

在`cowAlloc`中，如果`kalloc()`失败，则返回错误，之后结束程序。由于结束程序会调用系统`freeproc`的指令，其中会unmap pagetable并释放对应的内存，所以在`cowAlloc`中，只需返回错误信息，不用unmap pagetable，不然之后在结束程序时，unmap会报错。

`copyout`也会对内存写入，所以在其中需要加入COW机制，判断该页是否为COW，若是，需要分配一个新的页。  

加入了COW机制使得从va到pa的映射存在多对一的情况，当释放内存时需要判断该内存区域是否还有其他va指向它。设置一个全局变量`referCount[]`，记录每一个物理页有多少个引用。只用当对应页的`referCount`值减为0时才真正释放该物理空间。每次`kfree()`需要对`referCount`减一。每次fork的copy需要将`referCount`加一。每次`kalloc()`需要设置`referCount`等于1。