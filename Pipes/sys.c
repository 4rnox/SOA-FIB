/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <p_stats.h>

#include <errno.h>
#define TAM_BUFFER 512
#define LECTURA 0
#define ESCRIPTURA 1

void * get_ebp();

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -EBADF; 
  if (permissions!=ESCRIPTURA) return -EACCES; 
  return 0;
}

void user_to_system(void)
{
  update_stats(&(current()->p_stats.user_ticks), &(current()->p_stats.elapsed_total_ticks));
}

void system_to_user(void)
{
  update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
}

int sys_ni_syscall()
{
  return -ENOSYS; 
}

int sys_getpid()
{
  return current()->PID;
}

int global_PID=1000;

int ret_from_fork()
{
  return 0;
}

int sys_fork(void)
{
  struct list_head *lhcurrent = NULL;
  union task_union *uchild;
  
  /* Any free task_struct? */
  if (list_empty(&freequeue)) return -ENOMEM;

  lhcurrent=list_first(&freequeue);
  
  list_del(lhcurrent);
  
  uchild=(union task_union*)list_head_to_task_struct(lhcurrent);
  
  /* Copy the parent's task struct to child's */
  copy_data(current(), uchild, sizeof(union task_union));
  
  /* new pages dir */
  allocate_DIR((struct task_struct*)uchild);
  
  /* Allocate pages for DATA+STACK */
  int new_ph_pag, pag, i;
  page_table_entry *process_PT = get_PT(&uchild->task);
  for (pag=0; pag<NUM_PAG_DATA; pag++)
  {
    new_ph_pag=alloc_frame();
    if (new_ph_pag!=-1) /* One page allocated */
    {
      set_ss_pag(process_PT, PAG_LOG_INIT_DATA+pag, new_ph_pag);
    }
    else /* No more free pages left. Deallocate everything */
    {
      /* Deallocate allocated pages. Up to pag. */
      for (i=0; i<pag; i++)
      {
        free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
        del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
      }
      /* Deallocate task_struct */
      list_add_tail(lhcurrent, &freequeue);
      
      /* Return error */
      return -EAGAIN; 
    }
  }

  /* Copy parent's SYSTEM && CODE to child. */
  page_table_entry *parent_PT = get_PT(current());
  for (pag=0; pag<NUM_PAG_KERNEL; pag++)
  {
    set_ss_pag(process_PT, pag, get_frame(parent_PT, pag));
  }
  for (pag=0; pag<NUM_PAG_CODE; pag++)
  {
    set_ss_pag(process_PT, PAG_LOG_INIT_CODE+pag, get_frame(parent_PT, PAG_LOG_INIT_CODE+pag));
  }
  /* Copy parent's DATA to child. We will use TOTAL_PAGES-1 as a temp logical page to map to */
  for (pag=NUM_PAG_KERNEL+NUM_PAG_CODE; pag<NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA; pag++)
  {
    /* Map one child page to parent's address space. */
    set_ss_pag(parent_PT, pag+NUM_PAG_DATA, get_frame(process_PT, pag));
    copy_data((void*)(pag<<12), (void*)((pag+NUM_PAG_DATA)<<12), PAGE_SIZE);
    del_ss_pag(parent_PT, pag+NUM_PAG_DATA);
  }
  for(int i = 1; i < 11; ++i){
    page_table_entry *process_PT = get_PT(current());
    int end_of_log = (NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA)*4096;
    int pag1 = end_of_log + (4096*i);
    set_ss_pag(parent_PT, pag1, get_frame(process_PT, pag1));
    copy_data((void*)(pag1<<12), (void*)((pag1+NUM_PAG_DATA)<<12), PAGE_SIZE);

  }
  for (int i = 0; i < 16; ++i){
    uchild->task.canals[i] = current()->canals[i];
    if (current()->canals[i].free == 0) {
      if (current()->canals[i].lec == 1) ++tfo[current()->canals[i].entrada].readers;
      else if (current()->canals[i].lec == 0) ++tfo[current()->canals[i].entrada].writers;
    }
  }
  /* Deny access to the child's memory space */
  set_cr3(get_DIR(current()));

  uchild->task.PID=++global_PID;
  uchild->task.state=ST_READY;
  uchild->task.sembit=0;

  int register_ebp;   /* frame pointer */
  /* Map Parent's ebp to child's stack */
  register_ebp = (int) get_ebp();
  register_ebp=(register_ebp - (int)current()) + (int)(uchild);

  uchild->task.register_esp=register_ebp + sizeof(DWord);

  DWord temp_ebp=*(DWord*)register_ebp;
  /* Prepare child stack for context switch */
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=(DWord)&ret_from_fork;
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=temp_ebp;

  /* Set stats to 0 */
  init_stats(&(uchild->task.p_stats));

  /* Queue child process into readyqueue */
  uchild->task.state=ST_READY;
  list_add_tail(&(uchild->task.list), &readyqueue);
  
  return uchild->task.PID;
}

int pipe(int *fd){
  int pag;
  if (current()-> pipecounter >= 10 ) return -1;
  ++(current()->pipecounter);
  int freeframe = alloc_frame();
  if (freeframe == -1) return -1;
  page_table_entry *process_PT = get_PT(current());
  int end_of_log = (NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA)*4096;
  for (int i = 0; i < 12; ++i){
    if (i == 12) return -1;
    else{
    int x = get_frame(process_PT, end_of_log + (4096*i));
    if (x == 0) set_ss_pag(process_PT, end_of_log + (4096*i), freeframe);
    i = 12;
    pag = end_of_log + (4096*i);
    }
  }

  int k;
  for (int i = 0; i < 21 && i != -1; ++i) if (tfo[i].free == 1) {int k = i; i = -1; }
  if (k != -1) return -1;
  tfo[k].free = 0;
  tfo[k].bytesleft = 4096;

  *fd = add_entrada_lec(k);
  if (*fd == -1) return -1;
  ++tfo[k].readers;

  *(((char*)fd)+4) = add_entrada_esc(k);
  if (*(((char*)fd)+4) == -1) return -1;
  ++tfo[k].writers;

  tfo[k].posread = pag;
  tfo[k].poswrite = pag;
  tfo[k].ini = pag;
  return 0;
}

int dellh(struct list_head *kabot, struct list_head *del){
  struct list_head *aux = list_first(kabot);
  list_for_each(kabot, &aux)if (list_head_to_task_struct(aux) == del) { list_del(kabot); return 0; }
  return -1;
}

int close(int fd){
  current()->canals[fd].free = 1;
  int b = current()->canals[fd].lec;
  current()->canals[fd].lec = -1;
  int a = current()->canals[fd].entrada;
  current()->canals[fd].entrada = -1;
  
  if (tfo[a].readers == 1 && tfo[a].writers == 0 && b == 1) {
    tfo[a].posread = 0;
    tfo[a].poswrite = 0;
    tfo[a].bytesleft = 0;
    tfo[a].readers = 0;
    tfo[a].writers = 0;
    tfo[a].free = 1;
    int f = get_frame(current()->dir_pages_baseAddr, tfo[a].poswrite << 12);
    del_ss_pag(current()->dir_pages_baseAddr, tfo[a].poswrite << 12);
    free_frame(f);

  }

  else if (tfo[a].readers == 0 && tfo[a].writers == 1 && b == 0) {
    tfo[a].posread = 0;
    tfo[a].poswrite = 0;
    tfo[a].bytesleft = 0;
    tfo[a].readers = 0;
    tfo[a].writers = 0;
    tfo[a].free = 1;
      int f = get_frame(current()->dir_pages_baseAddr, tfo[a].poswrite << 12);
  del_ss_pag(current()->dir_pages_baseAddr, tfo[a].poswrite << 12);
  free_frame(f);

  }
  else if (tfo[a].readers > 0 && tfo[a].writers > 1 && b == 1) {
    --tfo[a].readers;
    dellh(&tfo[a].lecsem.queue, &(current()->list));
  }
  else if (tfo[a].readers >= 0 && tfo[a].writers > 1 && b == 0) {
    --tfo[a].writers;
    dellh(&tfo[a].escsem.queue, &(current()->list));

  }
  else if (tfo[a].readers > 0 && tfo[a].writers == 1 && b == 0)  {
    while (!(list_empty(&(tfo[current()->canals[fd].entrada].lecsem.queue)))){
    list_del(list_first(tfo[current()->canals[fd].entrada].lecsem.queue));
    list_head_to_task_struct(list_first(tfo[current()->canals[fd].entrada].lecsem.queue))->sembit = 1;
    list_add_tail(list_first(tfo[current()->canals[fd].entrada].lecsem.queue), &readyqueue);
  }
}
  else if (tfo[a].readers == 1 && tfo[a].writers > 0 && b == 1){
    while (!(list_empty(&(tfo[current()->canals[fd].entrada].escsem.queue)))){
    list_del(list_first(tfo[current()->canals[fd].entrada].escsem.queue));
    list_head_to_task_struct(list_first(tfo[current()->canals[fd].entrada].escsem.queue))->sembit = 1;
    list_add_tail(list_first(tfo[current()->canals[fd].entrada].escsem.queue), &readyqueue);
  }
}


return 0;
}

int read(int fd, char *buffer, int nbytes) {

  if (current()->canals[fd].lec != 1) return -1;
  if (nbytes < 0 || nbytes > 4096 || (nbytes < 4096 && nbytes > 0 && nbytes > (4096-tfo[current()->canals[fd].entrada].bytesleft))) return -EINVAL;
  if (!access_ok(VERIFY_READ, buffer, nbytes)) return -EFAULT;
  if (tfo[current()->canals[fd].entrada].bytesleft == 4096) {
    list_add_tail(&(current()->list), &(tfo[current()->canals[fd].entrada].lecsem.queue));
    sched_next_rr();
  }
  if (current()->sembit == 1) { current()->sembit == 0; return -EPIPE; }

  int source = tfo[current()->canals[fd].entrada].posread;
  int aux = nbytes;
  int count = 0;
  int ini = tfo[current()->canals[fd].entrada].ini;
  while (aux > 0) {
    if (tfo[current()->canals[fd].entrada].posread == ini+4096 && tfo[current()->canals[fd].entrada].bytesleft < 4096) tfo[current()->canals[fd].entrada].posread == ini;
    else return count;
    copy_to_user(source, buffer, 1);
    --aux;
    ++tfo[current()->canals[fd].entrada].posread;
    ++count;
    ++tfo[current()->canals[fd].entrada].bytesleft;
  }
  //|##############_____________________________|
  // |            |
  //read          write

  while (!(list_empty(&(tfo[current()->canals[fd].entrada].escsem.queue)))){
    list_del(list_first(tfo[current()->canals[fd].entrada].escsem.queue));
    list_add_tail(list_first(tfo[current()->canals[fd].entrada].escsem.queue), &readyqueue);
  }
  return nbytes;
}



int sys_write(int fd, char *buffer, int nbytes) {
if (fd < 3){

  char localbuffer [TAM_BUFFER];
  int bytes_left;
  int ret;

  if ((ret = check_fd(fd, ESCRIPTURA)))
    return ret;
  if (nbytes < 0)
    return -EINVAL;
  if (!access_ok(VERIFY_READ, buffer, nbytes))
    return -EFAULT;
  
  bytes_left = nbytes;
  while (bytes_left > TAM_BUFFER) {
    copy_from_user(buffer, localbuffer, TAM_BUFFER);
    ret = sys_write_console(localbuffer, TAM_BUFFER);
    bytes_left-=ret;
    buffer+=ret;
  }
  if (bytes_left > 0) {
    copy_from_user(buffer, localbuffer,bytes_left);
    ret = sys_write_console(localbuffer, bytes_left);
    bytes_left-=ret;
  }
  return (nbytes-bytes_left);
}
else {


  if (current()->canals[fd].lec != 0) return -1;
  if (nbytes < 0) return -EINVAL;
  if (!access_ok(VERIFY_READ, buffer, nbytes)) return -EFAULT;
  if (tfo[current()->canals[fd].entrada].bytesleft == 0) {
    list_add_tail(&(current()->list), &(tfo[current()->canals[fd].entrada].escsem.queue));
    sched_next_rr();
  }
  if (current()->sembit == 1) { current()->sembit == 0; return -EPIPE; }
  int aux = nbytes;
  int count = 0;
  int ini = tfo[current()->canals[fd].entrada].ini;
  while (aux > 0) {
    if (tfo[current()->canals[fd].entrada].poswrite == ini+4096 && tfo[current()->canals[fd].entrada].bytesleft > 0) tfo[current()->canals[fd].entrada].poswrite == ini;
    else return count;
    int dest = tfo[current()->canals[fd].entrada].poswrite;
    copy_from_user(buffer, dest, 1);
    --aux;
    ++buffer;
    ++tfo[current()->canals[fd].entrada].poswrite;
    ++count;
    --tfo[current()->canals[fd].entrada].bytesleft;
  }

  while (!(list_empty(&(tfo[current()->canals[fd].entrada].lecsem.queue)))){
    list_del(list_first(tfo[current()->canals[fd].entrada].lecsem.queue));
    list_add_tail(list_first(tfo[current()->canals[fd].entrada].lecsem.queue), &readyqueue);
  }
  return nbytes;
}
}


extern int zeos_ticks;

int sys_gettime()
{
  return zeos_ticks;
}

void sys_exit()
{  
  int i;

  for(int j = 3; j < 16; ++j) close(j);
  page_table_entry *process_PT = get_PT(current());
  sysexitedit();
  // Deallocate all the propietary physical pages
  for (i=0; i<NUM_PAG_DATA; i++)
  {
    free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
    del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
  }
  
  /* Free task_struct */
  list_add_tail(&(current()->list), &freequeue);
  
  current()->PID=-1;
  
  /* Restarts execution of the next process */
  sched_next_rr();
  return;
}

/* System call to force a task switch */
int sys_yield()
{
  force_task_switch();
  return 0;
}

extern int remaining_quantum;

int sys_get_stats(int pid, struct stats *st)
{
  int i;
  
  if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats))) return -EFAULT; 
  
  if (pid<0) return -EINVAL;
  for (i=0; i<NR_TASKS; i++)
  {
    if (task[i].task.PID==pid)
    {
      task[i].task.p_stats.remaining_ticks=remaining_quantum;
      copy_to_user(&(task[i].task.p_stats), st, sizeof(struct stats));
      return 0;
    }
  }
  return -ESRCH; /*ESRCH */
}