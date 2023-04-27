#include <unistd.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/shm.h>
#include <errno.h>
#include "constants.h"
#include "parameters.h"
#include "messages.h"

int main(int argc, char** argv) {
		FILE* f;
		struct parameters infoGame;

		int shm_id, id_semafori_pedine, id_semaforo_giocatore, id_semafori_matrix; /*SEGMENTO DI MEMORIA E SEMAFORI*/

		int id_pedina;
		boolean in_game, azioni_pre_round, mosse_di_preround_ricevute; /*CONTROLLI PARTITA*/
		/*STRUTTURE DATI NECESSARIE*/
		cella *ptr;
		pedina riferimento_P;
		coordinate dest;
		/*UTILITY PER CODE DI MESSAGGI*/
		int msgq_id_per_p;
		struct da_pedina_a_giocatore *messaggio_per_giocatore;
		struct da_giocatore_a_pedina messaggio_da_giocatore;
		/*****************************************************************************************************************************************/
		/*INIZIALIZZAZIONE STRUTTRE DATI E IPC*/

		set_parameters(f, &infoGame);

		id_semafori_pedine = semget(getppid()+32, 0, 0666);
		if(id_semafori_pedine == -1){
				perror("SEMGET FOR id_semafori_pedine IN PEDINA");
		}

		id_pedina = atoi(argv[1]); /*PRELEVO IL MIO ID DALL`ARGOMENTO PASSATO NELL`EXEC IN GIOCATORE*/

		reserveSem(id_semafori_pedine,id_pedina); /*attendo che il giocatore mi ridia il controllo*/

		id_semaforo_giocatore = semget(getppid(), 0, 0600);
		if(id_semaforo_giocatore == -1) {
				perror("SEMGET FOR id_semaforo_giocatore IN PEDINA");
				exit(EXIT_FAILURE);
		}

		shm_id = shmget(KEY_MEMORY, 0, 0600);
		if(shm_id == -1) {
				perror("SHMGET IN PEDINA");
				exit(EXIT_FAILURE);
		}
		ptr =(cella*) shmat(shm_id, NULL, 0);
		if(ptr == (void*) - 1){
				perror("SHMAT IN PEDINA");
				exit(EXIT_FAILURE);
		}

		id_semafori_matrix = semget(KEY_CELLS_SET, 0, 0600);
		if(id_semafori_matrix == -1){
				perror("SEMGET FOR id_semafori_matrix IN PEDINA");
				exit(EXIT_FAILURE);
		}

		msgq_id_per_p = msgget(getppid(), 0600);
		if(msgq_id_per_p == -1){
				perror("MSGGET FOR MSGQ_ID_PER_P IN PEDINA");
				exit(EXIT_FAILURE);
		}

		messaggio_per_giocatore = (struct da_pedina_a_giocatore*)malloc(sizeof(struct da_pedina_a_giocatore));
		if(messaggio_per_giocatore == NULL) {
				perror("MALLOC FOR MESSAGGIO_PER_GIOCATORE IN PEDINA");
				exit(EXIT_FAILURE);
		}
		/*RICEVO DAL GIOCATORE LA POSIZIONE DI PIAZZAMENTO*/
		if(msgrcv(msgq_id_per_p, &messaggio_da_giocatore, MAX_DIM, 0, 0) == -1){
				perror("MSGRCV IN PEDINA");
				exit(EXIT_FAILURE);
		}

		riferimento_P.mosse_residue = infoGame.SO_N_MOVES;

		riferimento_P.posizione_pedina.x = messaggio_da_giocatore.destinazione.x;
		riferimento_P.posizione_pedina.y = messaggio_da_giocatore.destinazione.y;

		/*PIAZZAMENTO*/
		reserveSem(id_semafori_matrix, (ptr+offset_matrix(messaggio_da_giocatore.destinazione.x, messaggio_da_giocatore.destinazione.y, infoGame.SO_BASE))->num_semaforo);
		(ptr+offset_matrix(messaggio_da_giocatore.destinazione.x, messaggio_da_giocatore.destinazione.y, infoGame.SO_BASE))->pedina = id_pedina+1;
		(ptr+offset_matrix(messaggio_da_giocatore.destinazione.x, messaggio_da_giocatore.destinazione.y, infoGame.SO_BASE))->pid_giocatore = getppid();


		releaseSem(id_semaforo_giocatore,0); 	 /*sblocco il  giocatore*/
		reserveSem(id_semafori_pedine,id_pedina); /*attendo che il giocatore mi ridia il controllo*/

		azioni_pre_round = TRUE;
		in_game = TRUE;


		while(in_game){
				if(azioni_pre_round){
						/*RICEVO DAL GIOCATORE LA MOSSA DI PRE-ROUND CHE CONTIENE IL MIO PROSSIMO SPOSTAMENTO*/
						if(msgrcv(msgq_id_per_p, &messaggio_da_giocatore, MAX_DIM, 0, 0) == -1){
								perror("MSGRCV IN PEDINA");
								exit(EXIT_FAILURE);
						}
						releaseSem(id_semaforo_giocatore,0); 	 /*sblocco il  giocatore*/
						reserveSem(id_semafori_pedine,id_pedina); /*attendo che il giocatore mi ridia il controllo*/

						mosse_di_preround_ricevute = TRUE;
						azioni_pre_round = FALSE;
				}
				/*SE HO GIA` ESEGUITO LA MOSSA DI PRE-ROUND*/
				if(!mosse_di_preround_ricevute){
						/*RICEVO DAL GIOCATORE LA MOSSA CHE CONTIENE IL MIO PROSSIMO SPOSTAMENTO*/
						if((msgrcv(msgq_id_per_p, &messaggio_da_giocatore, 512, 0, 0)) == -1){
								perror("MSGRCV IN PEDINA");
								exit(EXIT_FAILURE);
						}
				}
				if(mosse_di_preround_ricevute){
						mosse_di_preround_ricevute = FALSE;
				}
				dest = messaggio_da_giocatore.destinazione;

				if(dest.x==riferimento_P.posizione_pedina.x && dest.y==riferimento_P.posizione_pedina.y){
						messaggio_per_giocatore->mtype = 1;
						messaggio_per_giocatore->nuova_posizione = riferimento_P.posizione_pedina;

				}
				else{
						spostamento(ptr,id_semafori_matrix,id_pedina,&riferimento_P,dest.x,dest.y,infoGame.SO_BASE,infoGame.SO_ALTEZZA,infoGame.SO_MIN_HOLD_NSEC);
						/*SE LO SPOSTAMENTO E` ANDATO A BUON FINE RICAVO IL PUNTEGGIO*/
						if(dest.x==riferimento_P.posizione_pedina.x && dest.y==riferimento_P.posizione_pedina.y){
								messaggio_per_giocatore->mtype = 1;
								messaggio_per_giocatore->nuova_posizione = riferimento_P.posizione_pedina;


						}
						else{ /*NON MI SONO SPOSTATA*/
								messaggio_per_giocatore->mtype = 1;
								messaggio_per_giocatore->nuova_posizione = riferimento_P.posizione_pedina;

						}
				}
				/*COMUNICO AL GIOCATORE LA NUOVA POSIZIONE E IL PUNTEGGIO OTTENUTO*/
				if(msgsnd( msgq_id_per_p, messaggio_per_giocatore, (sizeof(struct da_pedina_a_giocatore) - sizeof(long)),0) == -1){
						perror("MSGSND IN PEDINA");
						exit(EXIT_FAILURE);
				}

				releaseSem(id_semaforo_giocatore,0);		 /*sblocco il  giocatore*/
				reserveSem(id_semafori_pedine,id_pedina); /*attendo che il giocatore mi ridia il controllo*/
				/*CONTROLLO SE E` FINITO IL ROUND*/
				if(conta_bandierine(ptr, infoGame.SO_ALTEZZA, infoGame.SO_BASE) == 0){
						azioni_pre_round = TRUE; /*PER IL NUOVO ROUND*/
						releaseSem(id_semaforo_giocatore,0); 	  /*sblocco il  giocatore*/
						reserveSem(id_semafori_pedine, id_pedina); /*attendo che il giocatore mi ridia il controllo*/
				}
		}
		/*DEALLOCAZIONE STRUTTURE DATI E IPC*/
		if(shmdt(ptr)==-1){
			perror("SHMDT FOR ptr IN PEDINA");
			exit(EXIT_FAILURE);
		}
		free(messaggio_per_giocatore);
		exit(EXIT_SUCCESS);
}
