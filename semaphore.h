#include <stdio.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <stdlib.h>

union semun{
    int val;
    struct semid_ds *buf;
	  unsigned short *array;

    #if defined(__LINUX__)
        struct seminfo * __buf
    #endif
};


/*FUNZIONI CHE IMPLEMENTANO UN PROTOCOLLO DI SEMAFORI BINARI(1=FREE, 0=RISERVATO)*/

int initSemAvailable(int semid, int semnum); 
int initSemInUse(int semid, int semnum); 
int reserveSem(int semid, int semnum); 
int releaseSem(int semid, int semnum); 
void checkSemLimits(int semid, int semnum); 

/*INIZIALIZZA UN SEMAFORO AVAILABLE*/
int initSemAvailable(int semid, int semnum){
    union semun arg;
    arg.val = 1;

	  return semctl(semid, semnum, SETVAL, arg);
}
/*INIZIALIZZA UN SEMAFORO IN USE*/
int initSemInUse(int semid, int semnum){
    union semun arg;
		arg.val = 0;

	  return semctl(semid, semnum, SETVAL, arg);
}
/*TENTA DI RISERVARE UN SEMAFORO (DECREMENTANDONE IL VALORE DI 1)
LA FUNZIONE SI BLOCCA SE IL VALORE ATTUALE DEL SEMAFORO CHE VUOLE DECREMENTARE E` 0
E SI SBLOCCA QUANDO IL VALORE DI QUEL SEMAFORO SARA` INCREMENTATO DA UNA RELEASE()*/
int reserveSem(int semid, int semnum){
    struct sembuf soper;

	  soper.sem_num = semnum;
	  soper.sem_op = -1;
	  soper.sem_flg = 0;

	  checkSemLimits(semid,semnum);
	  return semop(semid, &soper, 1);
}
/*RILASCIA IL SEMAFORO IN USO(INCREMENTANDONE IL VALORE DI 1)*/
int releaseSem(int semid, int semnum){
    struct sembuf soper;

	  soper.sem_num = semnum;
	  soper.sem_op = 1;
	  soper.sem_flg = 0;

	  checkSemLimits(semid,semnum);
	  return semop(semid, &soper, 1);
}
/*CONTROLLA CHE IL SEMAFORO SEMNUM DEL SET SEMID ABBIA UN VALORE CHE SIA 0 OPPURE 1
ALTRIMENTI STAMPERA` UN ERRORE*/
void checkSemLimits(int semid , int semnum){
    int value;
    value = semctl(semid, semnum, GETVAL);
	  if(value != 0 && value != 1){
		    printf("Valore del semaforo errato (sem %d del set %d = %d)\n", semnum, semid, value);
		    exit(EXIT_FAILURE);
	  }
}
