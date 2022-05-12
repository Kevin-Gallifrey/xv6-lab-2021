# Backtrace
每次的函数调用会生成一个frame压入栈中。stack frame的结构如下：  
| Stack |
|:---:|
| Return Address |
| To Prev. Frame |
| Saved Registers |
| Local Variables |
|...|
| Return Address |
| To Prev. Frame |
| Saved Registers |
| Local Variables |
|...|  

Risc-V中stack的生长方向是从上至下的。寄存器fp中存放的是当前frame的地址，指向frame的顶部。在frame中return address和指向上一个frame的指针是固定在前两个的。  
fp(-8)可以得到return address；fp(-16)可以得到前一个frame的地址。  
在C中，使用`uint64`来接收得到的地址，并使用`(uint64*)`转换成指针，就可以用`*a`的方式访问其中的内容了。  
利用stack page的上下边界来判断回溯是否应该停止。

# Alarm
`sigalarm`仅负责将传入的参数(计数值和handler的地址)存入proc结构体中。  
由于每经过一个tick，系统会触发时钟中断，执行一次trap。在`usertrap`中`if(which_dev == 2)`表示这是一个时钟中断，在此处对时钟tick进行计数，并比较是否已经到了计数值，并调用handler函数。  
`usertrap`是在kernel中，要调用handler就是要回到user空间。trap的返回地址就是`trapframe`中所保存的`epc`。此时要调用handler，就是要返回到handler的地址，所以此时将保存的handler的地址赋值到`trapframe->epc`就实现调用handler函数。  
为了在调用完handler之后能够继续执行主程序，需要先对寄存器、栈等中的数据进行保护。在proc中创建一个新的区域，保存`trapframe`中的所有内容。handler最后执行`sigreturn`，在其中将保存的`trapframe`再还原，就可以让主程序继续运行。  
为了避免在handler执行时又调用handler。在proc中加入一个标志位，判断是否正在调用handler，并在`usertrap`根据标志位判断是否应该调用handler。  

handler的调用是时钟中断触发的，所以可以实现周期性的调用。  
函数的调用实际就是pc的跳转，pc的值决定了程序之后运行的位置。  
为了能够恢复主程序的运行需要保护现场。  
在proc结构体中存放信息实现数据的共享。