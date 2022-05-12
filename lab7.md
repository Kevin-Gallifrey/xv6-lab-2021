# Networking
收发分别有两个循环数组ring和mbuf，其中ring中放的是descriptor，mbuf中放的是mbuf指针，指向对应的packet buffer，二者的index相对应。  
tx中需要写入descriptor并将对应的mbuf指向传入的packet buffer。  
rx中读取mbuf中的数据，并分配一个空的buffer供下次接收数据，并初始化descriptor。由于接收到的数据可能很多，不止一个packet，所以使用while循环。如果队列满了，对于之后的来的包，NIC会丢包。循环可以快速读取收到的包，减少丢包现象。  
在tx中需要上锁，避免两个进程同时发送时产生冲突。  
在rx中不用上锁。rx由NIC中断触发，当上一个中断处理完之后，才能产生下一个中断，rx不会出现冲突的状态。rx中调用net_rx，对于arp报文会发送应答，调用tx，所以rx中不能使用和tx中相同的锁，否则会是deadlock。