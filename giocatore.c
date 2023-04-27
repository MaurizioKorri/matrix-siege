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

		int i,j; /*INDICI DI CICLO*/
		int n_bandierine; /*NUMERO ATTUALE DI BANDIERE NEL ROUND*/
		int id_semafori_giocatori,id_semaforo_master, shm_id, id_semafori_matrix, id_semafori_pedine, id_semaforo_giocatore; /*SEGMENTO DI MEMORIA E SEMAFORI*/
		int dest_x, dest_y; /*UTILITY STRATEGIA DI GIOCO*/
		int id_giocatore;
		boolean cella_vuota, in_game,azioni_pre_round, mosse_di_preround_inviate; /*CONTROLLI PARTITA*/
		/*STRUTTURE DATI NECESSARIE*/
		cella *ptr;
		giocatore riferimento_G;
		pid_t *pid_pedine;
		pedina *dati_sulle_pedine;
		bandierina *dati_sulle_bandiere;
		coordinate mossa;
		/*UTILITY PER CODE DI MESSAGGI*/
		int msgq_id_per_p, msgq_id_per_g;
		struct da_giocatore_a_pedina *messaggio_per_pedina;
		struct da_giocatore_a_master *messaggio_per_master;
		struct da_pedina_a_giocatore messaggio_dalla_pedina;
		struct da_master_a_giocatore messaggio_da_master;
		/*******************************************************************************************************************************************************/

		/*INIZIALIZZAZIONE STRUTTURE DATI E IPC*/

		set_parameters(f,&infoGame);

		id_semafori_giocatori = semget(KEY_PLAYERS_SET, 0, 0600);
		if(id_semafori_giocatori == -1){
				perror("SEMGET FOR id_semafori_giocatori IN GIOCATORE");
				exit(EXIT_FAILURE);
		}

		id_giocatore = atoi(argv[1]); /*PRELEVO IL MIO ID NELL`ARGOMENTO PASSATO NELL`EXEC IN MASTER*/
		reserveSem(id_semafori_giocatori,id_giocatore); /*attendo che il master mi ridia il controllo*/

		shm_id = shmget(KEY_MEMORY, 0, 0600);
		if(shm_id == -1){
				perror("SHMGET FOR SHM_ID IN GIOCATORE");
				exit(EXIT_FAILURE);
		}
		ptr = (cella*)shmat(shm_id,NULL,0);
		if(ptr == (void*)-1){
				perror("SHMAT IN GIOCATORE");
				exit(EXIT_FAILURE);
		}

		id_semafori_matrix = semget(KEY_CELLS_SET, 0, 0600);
		if(id_semafori_matrix == -1){
				perror("SEMGET FOR id_semafori_matrix IN GIOCATORE");
				exit(EXIT_FAILURE);
		}

		id_semaforo_master = semget(KEY_MASTER_SET, 0, 0600);
		if(id_semaforo_master == -1){
				perror("SEMGET FOR id_semaforo_master IN GIOCATORE");
				exit(EXIT_FAILURE);
		}

		id_semafori_pedine = semget(getpid()+32 , infoGame.SO_NUM_P, IPC_CREAT | IPC_EXCL | 0600);/**/
		if(id_semafori_pedine == -1){
				perror("SEMGET FOR id_semafori_pedine IN GIOCATORE");
				exit(EXIT_FAILURE);
		}
		for(i=0; i < infoGame.SO_NUM_P; i++){
				if(initSemInUse(id_semafori_pedine,i) == -1){
						perror("INITSEMINUSE() FOR id_semafori_pedine IN GIOCATORE");
						exit(EXIT_FAILURE);
				}
		}

		id_semaforo_giocatore = semget(getpid(), 1, IPC_CREAT | IPC_EXCL |0600);
		if(id_semaforo_giocatore == -1){
				perror("SEMGET FOR id_semaforo_giocatore IN GIOCATORE");
				exit(EXIT_FAILURE);
		}
		if(initSemInUse(id_semaforo_giocatore, 0) == -1){
				perror("INITSEMINUSE() FOR id_semaforo_giocatore IN GIOCATORE");
				exit(EXIT_FAILURE);
		}

		pid_pedine = (pid_t*)malloc(sizeof(pid_t)*infoGame.SO_NUM_P);
		if(pid_pedine == NULL){
				perror("MALLOC FOR PID_PEDINE IN GIOCATORE");
				exit(EXIT_FAILURE);
		}

		msgq_id_per_p = msgget(getpid( ), IPC_CREAT | IPC_EXCL | 0600);
		if(msgq_id_per_p == -1){
				perror("MSGGET FOR MSGQ_ID_PER_P IN GIOCATORE");
				exit(EXIT_FAILURE);
		}

		messaggio_per_pedina = (struct da_giocatore_a_pedina*)malloc(sizeof(struct da_giocatore_a_pedina));
		if(messaggio_per_pedina==NULL){
				perror("MALLOC FOR PID_PEDINE IN GIOCATORE");
				exit(EXIT_FAILURE);
		}

		msgq_id_per_g = msgget(getppid(), 0600);
		if(msgq_id_per_g == -1){
				perror("MSGGET FOR MSGQ_ID_PER_G IN GIOCATORE");
				exit(EXIT_FAILURE);
		}

		messaggio_per_master = (struct da_giocatore_a_master*)malloc(sizeof(struct da_giocatore_a_master));
		if(messaggio_per_master == NULL){
				perror("MALLOC FOR MESSAGGIO_PER_MASTER IN GIOCATORE");
				exit(EXIT_FAILURE);
		}

		dati_sulle_pedine = (pedina*)malloc(sizeof(pedina)*infoGame.SO_NUM_P);
		if(dati_sulle_pedine == NULL){
				perror("MALLOC FOR dati_sulle_pedine IN GIOCATORE");
				exit(EXIT_FAILURE);
		}

		riferimento_G.punti_giocatore = 0;
		riferimento_G.mosse_rimanenti = infoGame.SO_N_MOVES * infoGame.SO_NUM_P;

		dati_sulle_bandiere = (bandierina*)malloc(sizeof(bandierina)*1);
		if(dati_sulle_bandiere == NULL){
				perror("MALLOC FOR dati_sulle_bandiere IN GIOCATORE");;
				exit(EXIT_FAILURE);
		}
		/*FORK DEI PROCESSI PEDINA CHE LANCERANNO LA EXEC IN CUI PASSERANNO IL LORO ID*/
		for(i = 0; i < infoGame.SO_NUM_P; i++){
				pid_pedine[i] = fork();
				if(pid_pedine[i] == -1){
						perror("FORK IN GIOCATORE");
						exit(EXIT_FAILURE);
				}
				else{
						if(pid_pedine[i] == 0){
								char id_pedina[5];
								sprintf(id_pedina, "%d", i);
								execlp("./pedina", "pedina", id_pedina, (char*)NULL);
						}
				}
		}
		/*SCELGO RANDOMICAMENTE DOVE PIAZZARE LA PEDINA i-ESIMA*/
		for(i=0; i<infoGame.SO_NUM_P; i++){
				dati_sulle_pedine[i].mosse_residue = infoGame.SO_N_MOVES;
				cella_vuota = FALSE;
				while(!cella_vuota){
						dati_sulle_pedine[i].posizione_pedina.x = numero_random(infoGame.SO_ALTEZZA);
						dati_sulle_pedine[i].posizione_pedina.y = numero_random(infoGame.SO_BASE);
						if(semctl(id_semafori_matrix, (ptr+offset_matrix(dati_sulle_pedine[i].posizione_pedina.x,dati_sulle_pedine[i].posizione_pedina.y,infoGame.SO_BASE))->num_semaforo, GETVAL) == 1){
								messaggio_per_pedina->mtype = 1;
								messaggio_per_pedina->destinazione = dati_sulle_pedine[i].posizione_pedina;
								/*COMUNICO ALLA PEDINA i-ESIMA LA POSIZIONE IN CUI PIAZZARSI*/
								if(msgsnd(msgq_id_per_p, messaggio_per_pedina, sizeof(struct da_giocatore_a_pedina) - sizeof(long),0) == -1){
										perror("MSGSND IN GIOCATORE");
										exit(EXIT_FAILURE);
								}
								cella_vuota = TRUE;
						}
						else{
								cella_vuota = FALSE;
						}
				}
				releaseSem(id_semafori_pedine,i); 				/*sblocco la pedina i-esima*/
				reserveSem(id_semaforo_giocatore,0);				/*attendo che la pedina i-esima mi ridia il controllo*/
				releaseSem(id_semaforo_master,0); 				/*sblocco il master*/
				reserveSem(id_semafori_giocatori,id_giocatore);  /*attendo che il master mi ridia il controllo*/
		}
		in_game = TRUE;
		azioni_pre_round = TRUE;
		while(in_game){
				if(azioni_pre_round){
						dati_sulle_bandiere = get_array_bandierine(ptr, infoGame.SO_ALTEZZA, infoGame.SO_BASE, dati_sulle_bandiere);
						n_bandierine = conta_bandierine(ptr, infoGame.SO_ALTEZZA, infoGame.SO_BASE);
						/*CONTROLLO LE PEDINE CHE NON HANNO ABBASTANZA MOSSE PER RAGGIUNGERE LA LORO BANDIERINA
						PIU VICINA E SETTO IL FLAG MOSSE_FINITE DELLA CELLA A TRUE*/
						check_pedine_isolate(ptr, dati_sulle_pedine, infoGame.SO_NUM_P, dati_sulle_bandiere, n_bandierine, infoGame.SO_BASE);
						for(i = 0; i<infoGame.SO_NUM_P;i++){
								if(bandierina_vicina_percorribile(dati_sulle_pedine[i],dati_sulle_bandiere, n_bandierine) > -1){
										dest_x = dati_sulle_bandiere[bandierina_vicina_percorribile(dati_sulle_pedine[i],dati_sulle_bandiere,n_bandierine)].posizione.x;
										dest_y = dati_sulle_bandiere[bandierina_vicina_percorribile(dati_sulle_pedine[i],dati_sulle_bandiere,n_bandierine)].posizione.y;
										/*VALUTO IN CHE DIREZIONE ""POTREI"" SPOSTARE LA PEDINA i-ESIMA*/
										mossa = scelta_movimento(ptr, id_semafori_matrix, dati_sulle_pedine[i], dest_x, dest_y, infoGame.SO_BASE);
										/*VALUTO SE LA PEDINA i-ESIMA NON E` LA PIU` VICINA ALLA SUA "BANDIERINA PIU` VICINA PERCORRIBILE" */
										if(!mi_muovo_o_no(dati_sulle_bandiere[bandierina_vicina_percorribile(dati_sulle_pedine[i],dati_sulle_bandiere,n_bandierine)].posizione,
										dati_sulle_pedine[i],
										ptr, infoGame.SO_ALTEZZA, infoGame.SO_BASE))
										{/*NON muovo la pedina i-esima*/
											mossa.x = dati_sulle_pedine[i].posizione_pedina.x;
											mossa.y = dati_sulle_pedine[i].posizione_pedina.y;
										}
										messaggio_per_pedina->mtype = 1;
										messaggio_per_pedina->destinazione = mossa;

								}
								else{/*la pedina i-esima e` isolata e non devo muoverla*/
										messaggio_per_pedina->mtype = 1;
										messaggio_per_pedina->destinazione = dati_sulle_pedine[i].posizione_pedina;

								}
								/*COMUNICO ALLA PEDINA i-ESIMA LA MOSSA DI PRE-ROUND CHE CONTIENE IL PROSSIMO SPOSTAMENTO SE SI SPOSTA,ALTRIMENTI CONTIENE LA SUA ATTUALE POSIZIONE*/
								if(msgsnd(msgq_id_per_p, messaggio_per_pedina, sizeof(struct da_giocatore_a_pedina) - sizeof(long), 0) == -1){
										perror("MSGSND IN GIOCATORE");
										exit(EXIT_FAILURE);
								}
								releaseSem(id_semafori_pedine,i); 				/*sblocco la pedina i-esima*/
								reserveSem(id_semaforo_giocatore,0); 			/*attendo che la pedina i-esima mi ridia il controllo*/
								releaseSem(id_semaforo_master,0); 				/*sblocco il master*/
								reserveSem(id_semafori_giocatori,id_giocatore);  /*attendo che il master mi ridia il controllo*/
						}
						azioni_pre_round = FALSE;
						mosse_di_preround_inviate = TRUE; /*INDICA CHE HO MANDATO LE MOSSE DI PRE-ROUND A TUTTE LE MIE PEDINE*/
				}

				for(i=0; i<infoGame.SO_NUM_P&&in_game&&!azioni_pre_round; i++){
						/*RICEVO DAL MASTER IL MESSAGGIO PER CAPIRE SE SONO IN GIOCO O NO*/
						if(msgrcv(msgq_id_per_g, &messaggio_da_master, MAX_DIM,0,0) == -1){
								perror("MSGRCV IN GIOCATORE");
								exit(EXIT_FAILURE);
						}
						in_game = messaggio_da_master.siamo_in_game;
						if(!in_game){
								for(j=0; j<infoGame.SO_NUM_P; j++){
										printf("[PEDINA %d DI GIOCATORE %d]: MOSSE RIMANENTI: %d\n",j+1,id_giocatore+1, dati_sulle_pedine[j].mosse_residue);
										if(j == 9){
												printf("\n");
										}
										if(kill(pid_pedine[j],SIGINT) == -1){
												perror("KILL() IN GIOCATORE");
												exit(EXIT_FAILURE);
										}
								}
								/*DEALLOCAZIONE STRUTTURE DATI E IPC*/
								free(dati_sulle_bandiere);
								free(pid_pedine);
								free(dati_sulle_pedine);
								free(messaggio_per_pedina);
								free(messaggio_per_master);

								if(msgctl(msgq_id_per_p, IPC_RMID, 0) == -1){
										perror("MSGCTL() FOR MSGQ_ID_PER_P IN GIOCATORE");
										exit(EXIT_FAILURE);
								}
								if(semctl(id_semaforo_giocatore, 0, IPC_RMID, NULL) == -1){
										perror("SEMCTL  FOR id_semaforo_giocatore IN GIOCATORE");
										exit(EXIT_FAILURE);
								}
								if(semctl(id_semafori_pedine,0, IPC_RMID, NULL) == -1){
										perror("SEMCTL FOR id_semafori_pedine IN GIOCATORE");
										exit(EXIT_FAILURE);
								}
								if(shmdt(ptr)==-1){
									perror("SHMDT FOR ptr IN GIOCATORE");
									exit(EXIT_FAILURE);
								}
						}
						releaseSem(id_semaforo_master,0); 				/*sblocco il master*/
						reserveSem(id_semafori_giocatori,id_giocatore);  /*attendo che il master mi ridia il controllo*/
						/*SE LA PEDINA i-ESIMA HA MOSSE ATTUO LA STRATEGIA,ALTRIMENTI LE COMUNICO DI NON SPOSTARSI*/
						if(dati_sulle_pedine[i].mosse_residue > 0){
								if(!mosse_di_preround_inviate){
										dati_sulle_bandiere = get_array_bandierine(ptr, infoGame.SO_ALTEZZA, infoGame.SO_BASE, dati_sulle_bandiere);
										n_bandierine = conta_bandierine(ptr, infoGame.SO_ALTEZZA, infoGame.SO_BASE);
										/*CONTROLLO LE PEDINE CHE NON HANNO ABBASTANZA MOSSE PER RAGGIUNGERE LA LORO BANDIERINA
										PIU VICINA E SETTO IL FLAG MOSSE_FINITE DELLA CELLA A TRUE*/
										check_pedine_isolate(ptr, dati_sulle_pedine, infoGame.SO_NUM_P, dati_sulle_bandiere, n_bandierine, infoGame.SO_BASE);
										if(bandierina_vicina_percorribile(dati_sulle_pedine[i], dati_sulle_bandiere, n_bandierine) > -1){
												dest_x = dati_sulle_bandiere[bandierina_vicina_percorribile(dati_sulle_pedine[i],dati_sulle_bandiere,n_bandierine)].posizione.x;
												dest_y = dati_sulle_bandiere[bandierina_vicina_percorribile(dati_sulle_pedine[i],dati_sulle_bandiere,n_bandierine)].posizione.y;
												/*VALUTO IN CHE DIREZIONE ""POTREI"" SPOSTARE LA PEDINA i-ESIMA*/
												mossa = scelta_movimento(ptr, id_semafori_matrix, dati_sulle_pedine[i], dest_x, dest_y, infoGame.SO_BASE);
												/*VALUTO SE LA PEDINA i-ESIMA NON E` LA PIU` VICINA ALLA SUA "BANDIERINA PIU` VICINA PERCORRIBILE"*/
												if(!mi_muovo_o_no(dati_sulle_bandiere[bandierina_vicina_percorribile(dati_sulle_pedine[i],dati_sulle_bandiere,n_bandierine)].posizione,
												dati_sulle_pedine[i],
												ptr, infoGame.SO_ALTEZZA, infoGame.SO_BASE))
												{/*non muovo la pedina i-esima*/
													mossa.x = dati_sulle_pedine[i].posizione_pedina.x;
													mossa.y = dati_sulle_pedine[i].posizione_pedina.y;
												}
												messaggio_per_pedina->mtype = 1;
												messaggio_per_pedina->destinazione = mossa;
										}
										else{/*la pedina i-esima e` isolata e non devo muoverla*/
												messaggio_per_pedina->mtype = 1;
												messaggio_per_pedina->destinazione = dati_sulle_pedine[i].posizione_pedina;
										}
										/*COMUNICO ALLA PEDINA i-ESIMA LA MOSSA CHE CONTIENE IL PROSSIMO SPOSTAMENTO SE SI SPOSTA, ALTRIMENTI CONTIENE LA SUA ATTUALE POSIZIONE*/
										if(msgsnd(msgq_id_per_p,messaggio_per_pedina,(sizeof(struct da_giocatore_a_pedina) - sizeof(long)), 0) == -1){
												perror("MSGSND IN GIOCATORE");
												exit(EXIT_FAILURE);
										}
								}
						}
						else{ /*LA PEDINA i-ESIMA NON HA MOSSE*/
							if(!mosse_di_preround_inviate){
								messaggio_per_pedina->mtype = 1;
								messaggio_per_pedina->destinazione = dati_sulle_pedine[i].posizione_pedina;
								if(msgsnd(msgq_id_per_p,messaggio_per_pedina,(sizeof(struct da_giocatore_a_pedina) - sizeof(long)), 0) == -1){
										perror("MSGSND IN GIOCATORE");
										exit(EXIT_FAILURE);
								}
							}
						}
						releaseSem(id_semafori_pedine,i);	/*sblocco la pedina i-esima*/
						reserveSem(id_semaforo_giocatore,0); /*attendo che la pedina i-esima mi ridia il controllo*/
						/*RICEVO DALLA PEDINA i-ESIMA LA NUOVA POSIZIONE E IL PUNTEGGIO OTTENUTO*/
						if(msgrcv(msgq_id_per_p, &messaggio_dalla_pedina, MAX_DIM,0,0) == -1){
								perror("MSGRCV IN GIOCATORE");
								exit(EXIT_FAILURE);
						}
						if(dati_sulle_pedine[i].posizione_pedina.x != messaggio_dalla_pedina.nuova_posizione.x || dati_sulle_pedine[i].posizione_pedina.y != messaggio_dalla_pedina.nuova_posizione.y){
								dati_sulle_pedine[i].mosse_residue = dati_sulle_pedine[i].mosse_residue - 1;
								dati_sulle_pedine[i].posizione_pedina = messaggio_dalla_pedina.nuova_posizione;
								riferimento_G.mosse_rimanenti = riferimento_G.mosse_rimanenti -1;
								if((ptr+offset_matrix(dati_sulle_pedine[i].posizione_pedina.x,dati_sulle_pedine[i].posizione_pedina.y,infoGame.SO_BASE))->punteggio_bandierina > 0){
										riferimento_G.punti_giocatore = riferimento_G.punti_giocatore
										 +
										(ptr+offset_matrix(dati_sulle_pedine[i].posizione_pedina.x,dati_sulle_pedine[i].posizione_pedina.y,infoGame.SO_BASE))->punteggio_bandierina;
								}

						}

						messaggio_per_master->mtype = 1;
						messaggio_per_master->player_ref = riferimento_G;
						messaggio_per_master->posizione_pedina = dati_sulle_pedine[i].posizione_pedina;
						/*COMUNICO AL MASTER LA MIA STRUTTURA AGGIORNATA E LE COORDINATE DELLA BANDIERINA DA RIMUOVERE*/
						if(msgsnd(msgq_id_per_g, messaggio_per_master, sizeof(struct da_giocatore_a_master) - sizeof(long), 0) == -1){
								perror("MSGSND IN GIOCATORE");
								exit(EXIT_FAILURE);
						}
						/*SE TUTTE LE PEDINE HANNO ESEGUITO LE DIRETTIVE DI PRE-ROUND*/
						/*forse mettere if(mossedipreround){mossedipreround=false}*/

						if(i == infoGame.SO_NUM_P-1){
								mosse_di_preround_inviate = FALSE;
						}

						releaseSem(id_semaforo_master,0); 				/*sblocco il master*/
						reserveSem(id_semafori_giocatori,id_giocatore);  /*attendo che il master mi ridia il controllo*/
						/*CONTROLLO SE E` FINITO IL ROUND*/
						if(conta_bandierine(ptr, infoGame.SO_ALTEZZA, infoGame.SO_BASE) == 0){
								for(i=0; i<infoGame.SO_NUM_P; i++){
										releaseSem(id_semafori_pedine,i); 	/*sblocco la pedina i-esima*/
										reserveSem(id_semaforo_giocatore,0); /*attendo che la pedina i-esima mi ridia il controllo*/
								}
								releaseSem(id_semaforo_master,0); 			   /*sblocco il master*/
								reserveSem(id_semafori_giocatori,id_giocatore); /*attendo che il master mi ridia il controllo*/

								azioni_pre_round = TRUE; /*PER IL NUOVO ROUND*/
						}
				}
		}
		exit(EXIT_SUCCESS);
}
