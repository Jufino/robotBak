#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
int semCreate(key_t key,int poc);
int semInit(int sem_id, int sem_num, int val);
int semWait(int sem_id, int sem_num);
int semPost(int sem_id, int sem_num);
void semRem(int sem_id);
