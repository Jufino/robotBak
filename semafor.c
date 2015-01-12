#include "semafor.h"
typedef union{
    int val; /* Value for SETVAL */
    struct semid_ds *buf; /* Buffer for IPC_STAT, IPC_SET */
    unsigned short *array; /* Array for GETALL, SETALL */
    struct seminfo *__buf; /* Buffer for IPC_INFO
                                           (Linux-specific) */
}semun;

int semInit(int sem_id,int sem_num,int val){
	semun un;
	un.val = val;
	semctl(sem_id, sem_num, SETVAL, un);
}
int semCreate(key_t key,int poc){
    int sem_id=0;
    if ((sem_id = semget(key, poc, 0666 | IPC_CREAT)) < 0) {
        printf("Chyba\n");
        exit(-2);
    }
    return sem_id;
}
int semWait(int sem_id, int sem_num) {
    struct sembuf my;
    my.sem_num = sem_num;
    my.sem_op = -1;
    my.sem_flg = 0;

    return semop(sem_id, &my, 1);
}

/*
 *POST zvysuje hodnotu semafora
 */
int semPost(int sem_id, int sem_num) {
    struct sembuf my;
    my.sem_num = sem_num;
    my.sem_op = 1;
    my.sem_flg = 0;

    return semop(sem_id, &my, 1);
}
void semRem(int sem_id){
	semctl(sem_id, 0, IPC_RMID, NULL);
}
