#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "fcntl.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_mmap(void)
{
  uint64 addr;
  int length, prot, flags, fd, offset;
  if(argaddr(0, &addr) < 0 || argint(1, &length) < 0 || argint(2, &prot) < 0
  || argint(3, &flags) < 0 || argint(4, &fd) < 0 || argint(5, &offset) < 0)
    return -1;
  
  // Assume addr and offset are always zero
  if(addr != 0 || offset != 0)
    return -1;

  struct proc *p = myproc();
  struct file *f;
  f = p->ofile[fd];

  // Do not allow read/write mapping of a file opened read-only.
  if(f->writable == 0 && flags == MAP_SHARED && (prot & PROT_WRITE))
    return -1;

  int i = 0;
  for(i = 0; i < 16; i++)
  {
    if(p->vma[i].f == 0)
      goto found;
  }
  return -1;

found:
  addr = p->sz;
  p->sz = addr + length;
  filedup(f);
  p->vma[i].f = f;
  p->vma[i].length = length;
  p->vma[i].prot = prot;
  p->vma[i].flag = flags;
  p->vma[i].addr = addr;
  p->vma[i].offset = offset;
  return addr;
}

uint64
sys_munmap(void)
{
  uint64 addr;
  int length;
  if(argaddr(0, &addr) < 0 || argint(1, &length) < 0)
    return -1;

  struct proc *p = myproc();
  struct VMA *vma;
  int npages;
  int i = 0;
  for(i = 0; i < 16; i++)
  {
    vma = &(p->vma[i]);
    if(addr >= vma->addr && addr < vma->addr + vma->length)
      goto found;
  }
  return -1;

found:
  npages = PGROUNDUP(length) / PGSIZE;
  for(int j = 0; j < npages; j++)
  {
    if(walkaddr(p->pagetable, addr) != 0)
    {
      if(vma->flag == MAP_SHARED)
        filewrite(vma->f, addr, PGSIZE);

      uvmunmap(p->pagetable, PGROUNDDOWN(addr), 1, 1);
    }

    if(addr == vma->addr)
    {
      addr += PGSIZE;
      vma->addr = addr;
      vma->length -= PGSIZE;
      vma->offset += PGSIZE;
    }
    else
    {
      addr += PGSIZE;
      vma->length -= PGSIZE;
    }
  }
  
  if(vma->length <= 0)
  {
    fileclose(vma->f);
    vma->f = 0;
    vma->addr = 0;
    vma->length = 0;
    vma->flag = 0;
    vma->prot = 0;
    vma->offset = 0;
  }
  return 0;
}
