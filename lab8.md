# Memory allocator
为每个CPU分配一个freelist，各个CPU在自己的freelist中寻找空页并分配，这样各个CPU就可以并行运行。  
当自己的freelist中没有空页时，去其他CPU的freelist中寻找。  
每个freelist配一把锁，这样可以减少锁的竞争。

# Buffer cache
由于所有程序都共享buffer cache，所以不能像之前一样为每个CPU分配一块buffer cache。此时，需要将锁精细化，原来对整个buffer的大锁分成若干个小锁。  
将buffer划分为13个bucket，每个bucket配一把锁，依据blockno，将其哈希映射到不同的bucket中。每个bucket分配一个头buf，其他buf以链表的形式加入其中。  
`brelse, bpin, bunpin`只需根据blockno，锁上对应bucket的锁，然后操作buffer就可。在`brelse`中为buf记录下当前时间ticks，作为LRU的判断依据。  
`bget`要分情况考虑。首先，是缓存命中的情况，这只需锁上对应的bucket就可。当没有命中时，需要更新cache，此时就涉及到了需要持有其他bucket的锁，这里会遇到死锁或是竞争的情况。  
考虑当前的bucket锁是否需要释放。如果不释放，那么去下一个bucket中寻找LRU时必定需要对它的bucket上锁，这时正好有一个程序在操作该bucket，且它在寻找LRU时正好查找到了当前我们正锁住的bucket，这就是一个死锁。如果释放，那么另一个程序bget相同的buffer时就会发现它不在缓存中，并更新缓存，这样最终会有两相同的buffer在cache中。  
需要一个全局锁锁住整个buffer cache来避免寻找LRU buffer时的竞争。这保证了只有一个程序可以查看整个buffer并获得LRU buffer。在获得这个大锁之前需要释放当前的bucket锁，这是避免死锁。因为在遍历buffer寻找LRU时，需要获得对应的bucket锁才能查看其内部buffer状态。所以需要先获得大锁，再获得小锁。  
在获得大锁之后，首先要重新获得当前的bucket锁，避免其他程序更新其中缓存造成的错误。在之前释放当前bucket锁到重新获得期间，存在其他程序能够获得该锁的可能，并可能更新了缓存内容。所以，在寻找LRU之前还需再判断一遍buffer cache是否存在，若不存在，再去寻找LRU。在找LRU buffer的时候，需要锁住对应的bucket，同时要锁住当前的LRU所在的bucket。不然，其他程序可能会更改我们找到的LRU buffer，使之不再可用。当搜索完一个bucket且其中没有我们需要的LRU时，需要及时释放该bucket的锁。  
在整个寻找过程中需要获得的锁有：全局的大锁，当前bucket的锁，LRU buffer所对应的bucket的锁，正在查找的bucket的锁。LRU更新时需要释放锁，bucket搜索完且没有LRU也需要释放锁。  
最后，将LRU从其bucket中拿出，并接入当前的bucket中，更新缓存内容，返回。  
为了，能够获得更好的并行性能，我们希望每次尽可能在其对应的bucket中完成，尽可能不涉及其他的bucket。所以，在一开始缓存没命中后，先在对应的bucket中寻找LRU buffer。这个buffer可能不是全局的LRU，但也可以替换。当找不到可替换的buffer后再去全局搜索。这样降低了需要全局锁的频率，能够更好地体现并行性。  

*make grade中先bcachetest，再usertests会报错，但在bcachetest之前进行usertests就不会，不知道为什么。要先make clean，不然也会有出错。