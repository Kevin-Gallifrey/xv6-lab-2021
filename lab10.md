# mmap
mmap将文件映射到内存中一段连续的地址空间。这样对文件的操作可以转换为对内存的操作。最后将结果写回文件即可。  
在调用系统函数`read`或是`write`操作文件时，没有offset参数用于读写文件的特定位置。其真正的offset隐含在`struct file`中，在每次读写完毕后会自动增加。所以当想要再次访问offset之前的位置时，就会变得困难（需要另外的函数去修改`file->offset`）。而使用mmap就不会出现这样的问题。直接根据地址，可以访问文件的任意位置。  
需要一个结构VMA(virtual memory area)来记录mmap映射，包括file指针，地址，长度，偏移量，权限等。VMA放入proc中。  
mmap函数本身需要找到一个空的VMA并记录当前mmap，还需要将`file->ref`加1（使得当文件关闭时，file指针不会失效）。同时，采用lazy allocation，仅增加`p->sz`，不做实际的内存分配。  
在trap中捕捉page fault。首先判断va是否是mmap的地址，若是，为其分配内存，读入文件内容，并mappages；若不是，返回错误。  
使用`readi`读取文件内容，根据`va`，`vma->addr`和`vma->offset`确定读取文件的位置。一开始`vma->addr`对应的是文件起始位置，`va - vma->addr`就是相应的文件偏移量。当调用munmap时，会相应的改变`vma->addr`。`vma->addr`始终记录的是mmap的起始地址，当前面的页面munmap之后，`vma->addr`需要相应地增加。此时，`va - vma->addr`不再是文件的偏移量。用`vma->offset`记录`vma->addr`与文件起始位置的偏差，那么可以用`va - vma->addr + vma->offset`来表示`va`相对应的文件offset。  
munmap需要写回文件，修改VMA参数。当所有mmap的页面都被munmap之后，需要关闭文件，并清空VMA。munmap时会碰到没有map的页面（懒分配）。可以先判断va是否有效，若没有map，可以直接跳过，直接修改VMA参数即可。  
在`exit`中，检查所有VMA，并unmap所有mmap的区域。实现方法与munmap类似。  
在`fork`中，复制VMA，并增加相应的`file->ref`。  
在`exit`和`fork`中会分别遇到`uvmunmap`和`uvmcopy`。它们在遇到没有映射的地址时会panic。mmap中存在因懒分配而没有map的页面。此时，对于`uvmunmap`和`uvmcopy`忽略这些页面即可。