#include <unistd.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/shm.h>
#include <errno.h>
#include "constants.h"
#include "parameters.h"
#include "messages.h"

int main(int argc, char** argv){
		FILE* f;
		struct parameters infoGame;

		int i,j;  /*INDICI DI CICLO*/
		boolean in_game, azioni_pre_round; /*CONTROLLI PARTITA*/
		int n_bandierine, cont_round, somma_punti_totali_ottenuti, mosse_utilizzate_totali; /*CONTATORI DI VARIO USO*/

		double tempo_di_gioco;

		int shm_id, id_semafori_matrix, id_semafori_giocatori, id_semaforo_master;  /*SEGMENTO DI MEMORIA E SEMAFORI*/

		size_t dim_scacchiera;
		/*UTILITY PER CODE DI MESSAGGI*/
		int msgq_id_per_g;
		struct da_master_a_giocatore *messaggio_per_giocatore;
		struct da_giocatore_a_master messaggio_da_giocatore;
		/*UTILITY TIMER DEL ROUND*/
		time_t round_timer, check_timer;
		/*STRUTTURE DATI NECESSARIE*/
		cella *ptr;
		bandierina *score_bandierine;
		giocatore *dati_sui_giocatori;
		pid_t *pid_giocatori;
		/*************************************************************************************************************************************************************/
		/*INIZIALIZZAZIONE STRUTTURE DATI E IPC*/

		set_parameters(f,&infoGame);

		dim_scacchiera = sizeof(cella) * infoGame.SO_ALTEZZA * infoGame.SO_BASE;

		shm_id = shmget(KEY_MEMORY, dim_scacchiera, IPC_CREAT|IPC_EXCL|0600);
		if(shm_id == -1){
				perror("SHMGET IN MASTER");
				exit(EXIT_FAILURE);
		}
		ptr = (cella*)shmat(shm_id,NULL,0);
		if(ptr == (void*) -1){
				perror("SHMAT IN MASTER");
				exit(EXIT_FAILURE);
		}

		id_semafori_matrix = semget(KEY_CELLS_SET, infoGame.SO_ALTEZZA*infoGame.SO_BASE, IPC_CREAT|IPC_EXCL|0600);
		if(id_semafori_matrix == -1){
				perror("SEMGET FOR id_semafori_matrix IN MASTER");
				exit(EXIT_FAILURE);
		}

		set_matrix(ptr, id_semafori_matrix, infoGame.SO_ALTEZZA, infoGame.SO_BASE);

		id_semafori_giocatori = semget(KEY_PLAYERS_SET, infoGame.SO_NUM_G, IPC_CREAT|IPC_EXCL|0600);
		if(id_semafori_giocatori == -1){
				perror("SEMGET FOR SET_MATRIX IN MASTER");
				exit(EXIT_FAILURE);
		}
		for(i=0; i<infoGame.SO_NUM_G; i++){
				if(initSemInUse(id_semafori_giocatori,i) == -1){
						perror("INITSEMINUSE() FOR id_semafori_giocatori IN MASTER");
						exit(EXIT_FAILURE);
				}
		}

		id_semaforo_master = semget(KEY_MASTER_SET, 1, IPC_CREAT|IPC_EXCL|0600);
		if(id_semaforo_master == -1){
				perror("SEMGET FOR id_semaforo_master IN MASTER");
				exit(EXIT_FAILURE);
		}
		if(initSemInUse(id_semaforo_master,0) == -1){
				perror("INITSEMINUSE() FOR id_semaforo_master IN MASTER");
				exit(EXIT_FAILURE);
		}

		n_bandierine = 0;

		score_bandierine = (bandierina*)malloc(sizeof(bandierina)*n_bandierine);
		if(score_bandierine == NULL){
					perror("MALLOC IN SCORE_BANDIERINE IN MASTER");
					exit(EXIT_FAILURE);
		}

		dati_sui_giocatori = (giocatore*)malloc(sizeof(giocatore) * infoGame.SO_NUM_G);
		if(dati_sui_giocatori == NULL){
				perror("MALLOC IN dati_sui_giocatori IN MASTER");
				exit(EXIT_FAILURE);
		}
		for(i=0; i<infoGame.SO_NUM_G; i++){
				dati_sui_giocatori[i].punti_giocatore = 0;
				dati_sui_giocatori[i].mosse_rimanenti = infoGame.SO_N_MOVES * infoGame.SO_NUM_P;
		}

		pid_giocatori = (pid_t*)malloc(sizeof(pid_t)*infoGame.SO_NUM_G);
		if(pid_giocatori == NULL){
				perror("MALLOC IN PID_GIOCATORI IN MASTER");
				exit(EXIT_FAILURE);
		}

		msgq_id_per_g = msgget(getpid(), IPC_CREAT|IPC_EXCL|0600);
		if(msgq_id_per_g == -1){
				perror("MSGGET FOR MSGQ_ID_PER_G IN MASTER");
				exit(EXIT_FAILURE);
		}

		messaggio_per_giocatore = (struct da_master_a_giocatore*)malloc(sizeof(struct da_master_a_giocatore));
		if(messaggio_per_giocatore == (void*) -1){
				perror("MALLOC IN MESSAGGIO_PER_GIOCATORE IN MASTER");
				exit(EXIT_FAILURE);
		}
		/*FORK DEI PROCESSI GIOCATORE CHE LANCERANNO LA EXEC IN CUI PASSERANNO IL LORO ID*/
		for(i=0; i<infoGame.SO_NUM_G;i++){
				pid_giocatori[i] = fork();
				if(pid_giocatori[i] == -1){
						perror("FORK() IN MASTER");
						exit(EXIT_FAILURE);
				}
				else{
						if(pid_giocatori[i] == 0){
								char id_giocatore[2];
								sprintf(id_giocatore, "%d", i);
								execlp("./giocatore", "giocatore", id_giocatore, (char*)NULL);
						}
				}
		}
		/*QUI ATTENDO CHE I GIOCATORI PIAZZINO A TURNO LE LORO PEDINE */
		for(j=0; j < infoGame.SO_NUM_P; j++){
				for(i=0; i<infoGame.SO_NUM_G; i++){
						releaseSem(id_semafori_giocatori,i); /*sblocco il giocatore i-esimo*/
						reserveSem(id_semaforo_master,0);  /*attendo che il giocatore i-esimo mi dia il controllo*/

				}
		}

		tempo_di_gioco = 0;
		cont_round = 0;
		in_game = TRUE;
		azioni_pre_round = TRUE;
		while(in_game){
				if(azioni_pre_round){
						printf("PREPARAZIONE AL ROUND %d\n", ++cont_round);
						n_bandierine = infoGame.SO_FLAG_MIN+numero_random((infoGame.SO_FLAG_MAX - infoGame.SO_FLAG_MIN)+1);
						score_bandierine = (bandierina*)realloc(score_bandierine, sizeof(bandierina) * n_bandierine);
						punteggi_random(score_bandierine,n_bandierine,infoGame.SO_ROUND_SCORE);
						set_campo_mosse(ptr, infoGame.SO_ALTEZZA, infoGame.SO_BASE);
						piazzamento_bandierina(score_bandierine,n_bandierine,infoGame.SO_ALTEZZA,infoGame.SO_BASE,ptr);
						stampa_stato(ptr, id_semafori_matrix, infoGame.SO_ALTEZZA, infoGame.SO_BASE, pid_giocatori, dati_sui_giocatori, infoGame.SO_NUM_G);
						/*LASCIO ESEGUIRE AI GIOCATORI LE LORO AZIONI DI PRE-ROUND (CHE SONO DARE LE DIRETTIVE ALLE PEDINE)*/
						for(j=0; j < infoGame.SO_NUM_P; j++){
								for(i=0; i<infoGame.SO_NUM_G;i++){
										releaseSem(id_semafori_giocatori,i); /*sblocco il giocatore i-esimo*/
										reserveSem(id_semaforo_master,0); /*attendo che il giocatore i-esimo mi dia il controllo*/
								}
						}
						azioni_pre_round = FALSE; /*E` FINITO IL PRE-ROUND*/
						printf("4 SECONDI ALL'INIZIO DEL ROUND\n");
						sleep(4);
						time(&round_timer); /*TEMPO DI INIZIO DEL SINGOLO ROUND*/
				}

				for(j=0;j<infoGame.SO_NUM_P && in_game && !azioni_pre_round; j++){
						for(i=0; i<infoGame.SO_NUM_G && in_game && !azioni_pre_round; i++){
								time(&check_timer); /*TEMPO DI GIOCO ATTUALE*/
								if(!(difftime(check_timer,round_timer) > (float)infoGame.SO_MAX_TIME)){	/*IL TEMPO DI GIOCO DEL ROUND NON ECCEDE IL SO_MAX_TIME*/
										messaggio_per_giocatore->mtype=1;
										messaggio_per_giocatore->siamo_in_game=TRUE;
										/*COMUNICO AL GIOCATORE i-ESIMO CHE SIAMO IN GIOCO*/
										if(msgsnd(msgq_id_per_g,messaggio_per_giocatore,(sizeof(struct da_master_a_giocatore)-sizeof(long)),0)==-1){
												perror("MSGSND() IN MASTER");
												exit(EXIT_FAILURE);
										}
										releaseSem(id_semafori_giocatori,i); /*sblocco il giocatore i-esimo*/
										reserveSem(id_semaforo_master,0); /*attendo che il giocatore i-esimo mi dia il controllo*/
								}
								else{ /*IL TEMPO DI GIOCO DEL ROUND ECCEDE IL SO_MAX_TIME*/
										tempo_di_gioco = tempo_di_gioco + difftime(check_timer, round_timer);
										for(i=0; i<infoGame.SO_NUM_G; i++){
												messaggio_per_giocatore->mtype=1;
												messaggio_per_giocatore->siamo_in_game=FALSE;
												/*COMUNICO AD OGNI GIOCATORE LA FINE DEL GIOCO*/
												if(msgsnd(msgq_id_per_g, messaggio_per_giocatore, sizeof(struct da_master_a_giocatore) - sizeof(long), 0) == -1){
														perror("MSGSND() IN MASTER");
														exit(EXIT_FAILURE);
												}
												releaseSem(id_semafori_giocatori,i); /*sblocco il giocatore i-esimo*/
												reserveSem(id_semaforo_master,0); /*attendo che il giocatore i-esimo mi dia il controllo*/
												if(kill(pid_giocatori[i],SIGINT)==-1){
														perror("KILL() IN MASTER");
														exit(EXIT_FAILURE);
												}
										}
										in_game = FALSE;
								}
								if(in_game){
										releaseSem(id_semafori_giocatori,i); /*sblocco il giocatore i-esimo*/
										reserveSem(id_semaforo_master,0);    /*attendo che il giocatore i-esimo mi dia il controllo*/
										/*PRELEVO IL MESSAGGIO DAL GIOCATORE i-ESIMO PER AGGIORNARE LE INFORMAZIONI DI GIOCO*/
										if(msgrcv(msgq_id_per_g,&messaggio_da_giocatore,MAX_DIM,0,0)==-1){
												perror("MSGRCV IN MASTER");
												exit(EXIT_FAILURE);
										}
										dati_sui_giocatori[i] = messaggio_da_giocatore.player_ref;
										if((ptr+offset_matrix(messaggio_da_giocatore.posizione_pedina.x, messaggio_da_giocatore.posizione_pedina.y,infoGame.SO_BASE))->punteggio_bandierina > 0){
												(ptr+offset_matrix(messaggio_da_giocatore.posizione_pedina.x,messaggio_da_giocatore.posizione_pedina.y,infoGame.SO_BASE))->punteggio_bandierina = 0;
												n_bandierine--;/*ho tolto la bandierina e quindi decremento il numero di bandierine*/
										}
										/*CONTROLLO SE E`FINITO IL ROUND*/
										if(n_bandierine == 0){
												/*PER RICAVARE IL TEMPO TOTALE DEL SINGOLO ROUND*/
												time(&check_timer);
												tempo_di_gioco = tempo_di_gioco + difftime(check_timer, round_timer);
												azioni_pre_round = TRUE; /*PER IL NUOVO ROUND*/
												for(i=0; i<infoGame.SO_NUM_G; i++){
														releaseSem(id_semafori_giocatori,i); /*sblocco il giocatore i-esimo*/
														reserveSem(id_semaforo_master,0);    /*attendo che il giocatore i-esimo mi dia il controllo*/
												}
												printf("ROUND TERMINATO\n");
												stampa_stato(ptr, id_semafori_matrix, infoGame.SO_ALTEZZA, infoGame.SO_BASE, pid_giocatori, dati_sui_giocatori, infoGame.SO_NUM_G);
										}
								}
						}
				}
		}
		printf("[MASTER]: IL GIOCO E' FINITO CON PUNTEGGI E MOSSE IN QUESTO MODO:\n");
		stampa_stato(ptr, id_semafori_matrix, infoGame.SO_ALTEZZA, infoGame.SO_BASE, pid_giocatori, dati_sui_giocatori, infoGame.SO_NUM_G);
		/*METRICHE DI GIOCO*/
		printf("NUMERO DI ROUND GIOCATI: %d\n\n", cont_round);
		printf("RAPPORTO DI MOSSE UTILIZZATE E MOSSE TOTALI PER CIASCUN GIOCATORE:\n");
		for(i=0;i<infoGame.SO_NUM_G;i++){
				printf("GIOCATORE %d: %d/%d\n", i+1, infoGame.SO_N_MOVES*infoGame.SO_NUM_P-dati_sui_giocatori[i].mosse_rimanenti, infoGame.SO_N_MOVES*infoGame.SO_NUM_P);
		}
		somma_punti_totali_ottenuti = 0;
		mosse_utilizzate_totali = infoGame.SO_N_MOVES*infoGame.SO_NUM_P*infoGame.SO_NUM_G;
		for(i=0;i<infoGame.SO_NUM_G;i++){
			somma_punti_totali_ottenuti = somma_punti_totali_ottenuti + dati_sui_giocatori[i].punti_giocatore;
			mosse_utilizzate_totali = mosse_utilizzate_totali - dati_sui_giocatori[i].mosse_rimanenti;
		}
		printf("\nRAPPORTO DI PUNTI OTTENUTI E MOSSE UTILIZZATE: %d/%d\n", somma_punti_totali_ottenuti, mosse_utilizzate_totali);
		printf("\nRAPPORTO DI PUNTI TOTALI SOMMATI FRA TUTTI I GIOCATORI E TEMPO DI GIOCO TOTALE: %d/%f\n", somma_punti_totali_ottenuti ,tempo_di_gioco);
		/*DEALLOCAZIONE STRUTTURE DATI E IPC*/
		free(dati_sui_giocatori);
		free(pid_giocatori);
		free(score_bandierine);
		free(messaggio_per_giocatore);
		if(semctl(id_semafori_giocatori, 0 , IPC_RMID, NULL) == -1){
				perror("SEMCTL FOR id_semafori_giocatori IN MASTER");
		}
		if(semctl(id_semaforo_master, 0, IPC_RMID , NULL) == -1){
				perror("SEMCTL FOR id_semaforo_master IN MASTER");
		}
		if(semctl(id_semafori_matrix, 0 , IPC_RMID , NULL) == -1){
				perror("SEMCTL FOR id_semafori_matrix IN MASTER");
		}
		if(msgctl(msgq_id_per_g, IPC_RMID,0)==-1){
			perror("MSGCTL FOR MSGQ_ID_PER_G IN MASTER");
		}
		if(shmctl(shm_id, IPC_RMID, NULL) == -1){
				perror("SHMCTL FOR SHM_ID IN MASTER");
		}
		printf("\n[MASTER]: TUTTE LE STRUTTURE DATI DEALLOCATE\n");
		exit(EXIT_SUCCESS);
}
