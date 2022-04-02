// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13

struct {
  struct spinlock biglock;    // lock the whole buffer
  struct spinlock lock[NBUCKET];
  struct buf buf[NBUF];
  struct buf hashtable[NBUCKET];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head;
} bcache;

void
binit(void)
{
  struct buf *b;
  initlock(&bcache.biglock, "bcache_biglock");
  for (int i = 0; i < NBUCKET; i++)
  {
    initlock(&bcache.lock[i], "bcache");
    bcache.hashtable[i].prev = &bcache.hashtable[i];
    bcache.hashtable[i].next = &bcache.hashtable[i];
  }

  // Create linked list of buffers
  /******
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  ******/
  for (int i = 0; i < NBUF; i++)
  {
    b = &bcache.buf[i];
    int num = i % NBUCKET;
    initsleeplock(&b->lock, "buffer");
    b->time = 0;
    b->next = bcache.hashtable[num].next;
    b->prev = &bcache.hashtable[num];
    bcache.hashtable[num].next->prev = b;
    bcache.hashtable[num].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int num = blockno % NBUCKET;
  acquire(&bcache.lock[num]);

  // Is the block already cached?
  for(b = bcache.hashtable[num].next; b != &bcache.hashtable[num]; b = b->next)
  {
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[num]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer in the current bucket.
  int least = ticks;
  struct buf *bLRU = 0;
  for(b = bcache.hashtable[num].next; b != &bcache.hashtable[num]; b = b->next){
    if(b->refcnt == 0 && least >= b->time) {
      least = b->time;
      bLRU = b;
    }
  }
  if(bLRU)
  {
    b = bLRU;
    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;
    release(&bcache.lock[num]);
    acquiresleep(&b->lock);
    return b;
  }
  
  // No free buffer in the current bucket
  // Search for the whole buffer
  release(&bcache.lock[num]);
  acquire(&bcache.biglock);

  acquire(&bcache.lock[num]);
  // Is the block already cached?
  // Search again
  for(b = bcache.hashtable[num].next; b != &bcache.hashtable[num]; b = b->next)
  {
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[num]);
      release(&bcache.biglock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer in the whole buffer.
  int lastbucket = -1, currentbucket;
  least = ticks;
  for (int i = 0; i < NBUCKET; i++)
  {
    currentbucket = (num + i) % NBUCKET;
    if(currentbucket != num)
      acquire(&bcache.lock[currentbucket]);
    for(b = bcache.hashtable[currentbucket].next; b != &bcache.hashtable[currentbucket]; b = b->next)
    {
      if(b->refcnt == 0 && least >= b->time) {
        least = b->time;
        bLRU = b;
      }
    }
    if(bLRU)
    {
      if(lastbucket > -1 && lastbucket != num)
        release(&bcache.lock[lastbucket]);
      lastbucket = currentbucket;
    }
    else
    {
      if(currentbucket != num)
        release(&bcache.lock[currentbucket]);
    }
  }
  
  if(bLRU)
  {
    b = bLRU;
    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;
    // move from the lastbucket
    b->next->prev = b->prev;
    b->prev->next = b->next;
    release(&bcache.lock[lastbucket]);
    // move to the target bucket
    b->prev = &bcache.hashtable[num];
    b->next = bcache.hashtable[num].next;
    bcache.hashtable[num].next->prev = b;
    bcache.hashtable[num].next = b;
    release(&bcache.lock[num]);
    release(&bcache.biglock);
    acquiresleep(&b->lock);
    return b;
  }

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  int num = b->blockno % NBUCKET;

  releasesleep(&b->lock);

  acquire(&bcache.lock[num]);
  b->refcnt--;
  /******
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  ******/
  if(b->refcnt == 0)
    b->time = ticks;
  
  release(&bcache.lock[num]);
}

void
bpin(struct buf *b) {
  int num = b->blockno % NBUCKET;
  acquire(&bcache.lock[num]);
  b->refcnt++;
  release(&bcache.lock[num]);
}

void
bunpin(struct buf *b) {
  int num = b->blockno % NBUCKET;
  acquire(&bcache.lock[num]);
  b->refcnt--;
  release(&bcache.lock[num]);
}


