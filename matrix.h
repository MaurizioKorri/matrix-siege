#include <time.h>
#include <math.h>
#include "semaphore.h"

typedef enum{
    FALSE = 0, TRUE
} boolean;

typedef struct coordinate{
		int x;
		int y;
} coordinate;

typedef struct bandierina{
		coordinate posizione;
		int punteggio;
} bandierina;

typedef struct pedina{
		int mosse_residue;
		coordinate posizione_pedina;
} pedina;

typedef struct giocatore{
		int punti_giocatore;
		int mosse_rimanenti;
} giocatore;

typedef struct cella{
		int pid_giocatore;
		int num_semaforo;
		int pedina;
		int punteggio_bandierina;
		boolean mosse_finite;
} cella;

typedef struct timespec{
		time_t tv_sec;
		long tv_nsec;
} timespec ;
/*********************************************************DICHIARAZIONI********************************************************************************************************************/
/*UTILITY SCACCHIERA*/
int numero_random(int range);
int offset_matrix(int i, int j, int dim_riga);
void set_matrix(cella* c,int set_id, int altezza,int base);
void print_matrix(cella* c, int id_semafori_matrix, int altezza , int base);
void stampa_stato(cella *c, int id_semafori_matrix, int altezza, int base, pid_t *giocatori, giocatore *rif_G, int num_g);
void set_campo_mosse(cella *c, int altezza, int base);
void check_pedine_isolate(cella *c, pedina* dati_sulle_pedine, int num_p, bandierina *bandiere, int num_b, int base);
/*UTILITY BANDIERINE*/
void punteggi_random(bandierina punteggi[], int dim, int punteggio_tot);
void piazzamento_bandierina(bandierina punteggi[], int dim, int altezza, int base, cella* c);
bandierina* get_array_bandierine(cella *c, int altezza, int base, bandierina *bandiere);
int conta_bandierine(cella *c, int altezza, int base);
int bandierina_vicina_percorribile(pedina p, bandierina* bandiere, int num_b);
/*STRATEGIA DI GIOCO*/
int calcola_percorso(coordinate p, coordinate b);
boolean mi_muovo_o_no(coordinate band, pedina p, cella *c, int altezza, int base);
void spostamento(cella* c, int id_semafori_matrix, int id_p, pedina* rif_p, int dest_x, int dest_y,  int base, int altezza,long int so_nsec);
coordinate scelta_movimento(cella *c, int id_semafori_matrix, pedina p, int dest_x, int dest_y, int base);
/*************************************************************************************CODE*******************************************************************************************/
int numero_random(int range){
		srand(clock());
		return rand()%range;			/*RITORNA UN VALORE TRA 0 E (RANGE-1)*/
}

/*RITORNA L`OFFSET CHE SOMMATO AL PUNTATORE cella* IDENTIFICA L`ELEMENTO IN POSIZIONE [i][j]*/
int offset_matrix(int i, int j , int dim_riga){
		return (i * dim_riga) + j;/*i-esima riga di dimensione dim_riga + j-esimo elemento della riga*/
}
/*INIZIALIZZA I CAMPI DELLE CELLE DELLA MATRICE*/
void set_matrix(cella* c, int set_id,  int altezza, int base){
		int i, j;
		for(i=0; i<altezza; i++){
				for(j=0; j<base; j++){
						(c+offset_matrix(i,j,base))->num_semaforo = offset_matrix(i,j,base);
						if(initSemAvailable(set_id ,(c+offset_matrix(i,j,base))->num_semaforo) == -1){
								perror("INITSEMAVAILABLE() IN MATRIX.H");
								exit(EXIT_FAILURE);
						}
						(c+offset_matrix(i,j,base))->pedina = 0;
						(c+offset_matrix(i,j,base))->punteggio_bandierina = 0;
						(c+offset_matrix(i,j,base))->pid_giocatore = 0;
						(c+offset_matrix(i,j,base))->mosse_finite = FALSE;

				}
		}
}
/*STAMPA LA MATRICE CON LE PEDINE DISTINGUIBILI PER ID, USATA PER VISUALIZZARE LA PARTITA*/
void print_matrix(cella *c ,int id_semafori_matrix, int altezza, int base){
		int i, j, g;
		for(j=0; j<base; j++){/*indici della colonna*/
				if(j==0) printf("\t %d  ", j);

				if(j<10 && j!=0){
						if(j==9) printf("%d ",j);
						else printf("%d  ",j);
				}

				if(j>=10 && j!= base-1) printf("%d ", j);

				if(j==base-1) printf("%d\n", j);
		}
		for(i=0; i<altezza; i++){
				printf("%d\t",i);
				for(j=0; j<base; j++){
						if(semctl(id_semafori_matrix,(c+offset_matrix(i,j,base))->num_semaforo,GETVAL) == 0){
								if((c+offset_matrix(i,j,base))->pedina < 10){
										printf("|%d ", (c+offset_matrix(i,j,base))->pedina);
								}
								else{
										printf("|%d", (c+offset_matrix(i,j,base))->pedina);
								}
						}
						else{
								if((c+offset_matrix(i,j,base))->punteggio_bandierina > 0){
										printf("[%d]",(c+offset_matrix(i,j,base))->punteggio_bandierina);
								}
								else{
										printf("[ ]");
								}
						}
				}
				printf("\n");
		}
		printf("\n\n");
}
/*STAMPA LA MATRICE CON LE PEDINE DISTINGUIBILI PER GIOCATORE,IL PUNTEGGIO E LE MOSSE RIMANENTI*/
void stampa_stato(cella *c, int id_semafori_matrix, int altezza, int base, pid_t *giocatori, giocatore *rif_G, int num_g){
		int i, j, g;
		for(j=0; j<base; j++){/*indici della colonna*/
				if(j==0) printf("\t %d  ", j);

				if(j<10 && j!=0){
						if(j==9) printf("%d ",j);
						else printf("%d  ",j);
				}

				if(j>=10 && j!= base-1) printf("%d ", j);

				if(j==base-1) printf("%d\n", j);
		}
		for(i=0; i<altezza; i++){
				printf("%d\t",i);
				for(j=0; j<base; j++){
						if(semctl(id_semafori_matrix,(c+offset_matrix(i,j,base))->num_semaforo,GETVAL) == 0){
								for(g=0; g<num_g; g++){
										if(giocatori[g]==(c+offset_matrix(i,j,base))->pid_giocatore){
												printf("[%c]", (char)('A'+g));
										}
								}
						}
						else{
								if((c+offset_matrix(i,j,base))->punteggio_bandierina > 0){
										printf("[%d]",(c+offset_matrix(i,j,base))->punteggio_bandierina);
								}
								else{
										printf("[ ]");
								}
						}
				}
				printf("\n");
		}
		/*STAMPA PUNTEGGIO E MOSSE DEI GIOCATORI*/
		for(i=0; i<num_g;i++){
				printf("GIOCATORE %d: MOSSE RESIDUE: %d, PUNTEGGIO: %d\n", i+1, rif_G[i].mosse_rimanenti,rif_G[i].punti_giocatore);
		}
		printf("\n\n");

}
/*RESETTA IL FLAG MOSSE_FINITE DI TUTTE LE CELLE OCCUPATE DALLE PEDINE A FALSE */
void set_campo_mosse(cella *c, int altezza, int base){
		int i,j;
		for(i = 0; i < altezza;i++){
				for(j = 0; j < base; j++){
						if((c+offset_matrix(i,j,base))->pedina > 0){
								(c+offset_matrix(i,j,base))->mosse_finite = FALSE;
						}
				}
		}
}
/*SETTA IL FLAG MOSSE_FINITE DI TUTTE LE CELLE CHE SONO OCCUPATE DA PEDINE CHE NON HANNO NESSUNA BANDIERINA RAGGIUNGIBILE*/
void check_pedine_isolate(cella *c, pedina* dati_sulle_pedine, int num_p, bandierina *bandiere, int num_b, int base){
		int i;
		for(i=0;i<num_p;i++){
				if(bandierina_vicina_percorribile(dati_sulle_pedine[i],bandiere,num_b) == -1){
						(c+offset_matrix(dati_sulle_pedine[i].posizione_pedina.x,dati_sulle_pedine[i].posizione_pedina.y,base))->mosse_finite = TRUE;
				}
		}
}
/*ASSEGNA PUNTEGGI RANDOM ALLE BANDIERINE*/
																						/*		5							10	*/
void punteggi_random(bandierina *punteggi , int dim, int punteggio_tot){
		int sum, i;
		srand(time(0));
		sum = 0;
		for(i = 0; i < dim; i++){
				if(!(i == dim-1)){/*se non sono l'ultimo elemento dell'array*/
						punteggi[i].punteggio  = (0+rand()%(punteggio_tot-sum))/(dim-i) + 1; /*casuale tra 1 e 10*/
						/*(nr random tra 0 e 9 diviso 5-i) + 1*/
						/*sum e' la somma di tutti i punteggi fino all'i attuale*/
						/*punteggio tot-sum e' quello che rimane di punteggio da assegnare*/
						/*(punteggio_tot - sum) e' la quantita' che rimane per arrivare a 10, e non vogliamo superarla*/
						/*/ (dim - i) se la tua somma ha raggiunto ad esempio 6  (servono ancora 4 punti) e tu hai ancora 3 caselle, ti
						assicura che il prossimo numero random non e' piu di 1, per lasciare un po di punti alle prossime 3 caselle*/
						/*+1 per evitare che una bandierina valga 0 */
						sum = sum + punteggi[i].punteggio;
				}
				else{	/*l'ultimo elemento e' quello che rimane*/
							/*non userai tutti i 10 (punteggio_tot) prima di arrivare al 5o elemento*/
						punteggi[i].punteggio = (punteggio_tot - sum);
						sum = sum + punteggi[i].punteggio;
				}
		}
}
/*PIAZZA RANDOMICAMENTE IN CELLE LIBERE DA PEDINE LE BANDIERINE*/
void piazzamento_bandierina(bandierina *punteggi, int dim, int altezza, int base, cella* c){
 		int i;
		for(i = 0; i < dim; i++){
				boolean cella_con_bandierina;
				int x, y;
				cella_con_bandierina = FALSE;
				while(!cella_con_bandierina){
						x = (rand()%(altezza)); /*voglio valori tra 0 e (altezza -1)*/
						y = (rand()%(base));
						if((c + offset_matrix(x,y,base))->pedina == 0){
								if((c + offset_matrix(x,y,base))->punteggio_bandierina == 0){
										(c + offset_matrix(x,y,base))->punteggio_bandierina = punteggi[i].punteggio;
										punteggi[i].posizione.x = x;
										punteggi[i].posizione.y = y;
										cella_con_bandierina = TRUE;
								}
								else{
										cella_con_bandierina = FALSE;
								}
						}
						else{
								cella_con_bandierina = FALSE;
						}
				}
		}
}
/*RICAVO IL VETTORE DI BANDIERINE NELLA MATRICE*/
bandierina* get_array_bandierine(cella *c, int altezza, int base, bandierina *bandiere){
		int dim_arr, i, j;
		dim_arr = 0;
		for(i=0; i<altezza; i++){
				for(j=0; j<base; j++){
						if((c+offset_matrix(i,j,base))->punteggio_bandierina>0){
								bandiere = (bandierina*)realloc(bandiere,sizeof(bandierina) * (dim_arr+1));
								bandiere[dim_arr].posizione.x = i;
								bandiere[dim_arr].posizione.y = j;
								bandiere[dim_arr].punteggio = (c+offset_matrix(i,j,base))->punteggio_bandierina;
								dim_arr++;
						}
				}
		}
		return bandiere;
}
/*RICAVO LA LUNGHEZZA DEL VETTORE DELLE BANDIERINE*/
int conta_bandierine(cella *c, int altezza, int base){
		int i, j, counter;
		counter = 0;
		for(i=0; i<altezza; i++){
				for(j=0; j<base; j++){
						if((c+offset_matrix(i,j,base))->punteggio_bandierina > 0){
								counter++;
						}
				}
		}
		return counter;
}
/*LA FUNZIONE RITORNA L`INDICE DELLA BANDIERINA PIU VICINA PERCORRIBILE DALLA PEDINA P
OPPURE -1 SE LA PEDINA NON PUO` RAGGIUNGERE NESSUNA BANDIERINA*/
int bandierina_vicina_percorribile(pedina p, bandierina* bandiere,int num_b){
		int index = -1;
		int percorso = 99999999;
		int b;
		for(b = 0; b < num_b; b++){
				if(p.mosse_residue >= calcola_percorso(p.posizione_pedina, bandiere[b].posizione)){
						if(calcola_percorso(p.posizione_pedina, bandiere[b].posizione) <= percorso){
								if(calcola_percorso(p.posizione_pedina, bandiere[b].posizione) == percorso){/*CASO PARTICOLARE: PRENDIAMO LA BAND CON PUNTEGGIO PIU ALTO*/
										if(bandiere[b].punteggio >= bandiere[index].punteggio){
												index = b;
												percorso = calcola_percorso(p.posizione_pedina, bandiere[b].posizione);
										}
										else{
												index = index;
												percorso = percorso;
										}
								}
								else{
										index = b;
										percorso = calcola_percorso(p.posizione_pedina, bandiere[b].posizione);
								}
						}
						else{
								index = index;
								percorso = percorso;
						}
				}
		}
		return index;
}
/*RITORNA IL NUMERO DI PASSI NECESSARIE CHE UNA PEDINA p DEVE FARE PER RAGGIUNGERE UNA BANDIERNA b*/
int calcola_percorso(coordinate p, coordinate b){
		int percorso;
		percorso = 0;
		if(p.x==b.x && p.y==b.y){
				return 0;
		}
		else{/*sono diverse entrambe*/
				if(p.x>b.x){
						percorso = percorso + (p.x-b.x);
				}
				else{
						percorso = percorso + (b.x-p.x);
				}
				if(p.y>b.y){
						percorso = percorso + (p.y-b.y);
				}
				else{
						percorso = percorso + (b.y-p.y);
				}
				return percorso;
		}
}
/*FUNZIONE CHE RENDE POSSIBILE LO SPOSTAMENTO DI UNA PEDINA ATTUANDO LA NANOSLEEP E AGGIORNADO LA SUA STRUCT*/
void spostamento(cella* c, int id_semafori_matrix, int id_p,pedina* rif_p, int dest_x, int dest_y,  int base,int altezza, long  so_nsec){
		if((semctl(id_semafori_matrix, (c+offset_matrix(dest_x, dest_y, base))->num_semaforo,GETVAL))==1){
				timespec t_spec;
				(c+offset_matrix(rif_p->posizione_pedina.x, rif_p->posizione_pedina.y, base))->pedina = 0;
				(c+offset_matrix(rif_p->posizione_pedina.x, rif_p->posizione_pedina.y, base))->pid_giocatore = 0;
				releaseSem(id_semafori_matrix,(c+offset_matrix(rif_p->posizione_pedina.x,rif_p->posizione_pedina.y,base))->num_semaforo);

				reserveSem(id_semafori_matrix,(c+offset_matrix(dest_x,dest_y,base))->num_semaforo);
				(c+offset_matrix(dest_x,dest_y,base))->pedina = id_p+1;
				(c+offset_matrix(dest_x,dest_y,base))->pid_giocatore = getppid();
				t_spec.tv_sec = 0;
				t_spec.tv_nsec =so_nsec;
				if(nanosleep(&t_spec, NULL)==-1){
						perror("NANOSLEEP() IN MATRIX.H");
						exit(EXIT_FAILURE);
				}
				rif_p->mosse_residue = rif_p->mosse_residue - 1;
				rif_p->posizione_pedina.x = dest_x;
				rif_p->posizione_pedina.y = dest_y;
				print_matrix(c, id_semafori_matrix,altezza,base);
		}
		else{
			rif_p->mosse_residue = rif_p->mosse_residue;
			rif_p->posizione_pedina.x = rif_p->posizione_pedina.x;
			rif_p->posizione_pedina.y = rif_p->posizione_pedina.y;
		}
}
/*FUNZIONE CHE RESTITUISCE TRUE SE LA PEDINA p E' LA PEDINA PIU VICINA DI TUTTE ALLA BANDIERNA b, ALTRIMENTI
	RESTITUISCE FALSE*/
boolean mi_muovo_o_no(coordinate band, pedina p, cella *c, int altezza, int base){
		int k, r;
		/*int max_x, max_y, i, j;*/
		coordinate temp;
		/*i = band.x-calcola_percorso(p.posizione_pedina, band);
		if(i<0){
				i = 0;
		}
		max_x = band.x+calcola_percorso(p.posizione_pedina, band);
		if(max_x>altezza){
				max_x = altezza;
		}

		j = band.y-calcola_percorso(p.posizione_pedina, band);
		if(j<0){
				j = 0;
		}
		max_y = band.y+calcola_percorso(p.posizione_pedina, band);
		if(max_y>base){
			max_y = base;
		}*/
		/*
		for(k = i;k<max_x;k++){
				for(r = j;r<max_y;r++){
		*/
		for(k = 0;k<altezza;k++){
				for(r = 0;r<base;r++){
						if((c+offset_matrix(k,r,base))->pedina>0){
								if((c+offset_matrix(k,r,base))->mosse_finite == FALSE){
										temp.x = k;
										temp.y = r;
										if(calcola_percorso(temp, band)<calcola_percorso(p.posizione_pedina, band)){
												return FALSE;
										}
								}
						}
				}
		}
		return TRUE;
}
/*FUNZIONE CHE RESTITUISCE LE COORDINATE DELLA PROSSIMA MOSSA CHE DEVE COMPIERE LA PEDINA p PER RAGGIUNGERE LA DESTINAZIONE (CHE SAREBBE LA BANDIERINA)*/
coordinate scelta_movimento(cella *c, int id_semafori_matrix, pedina p, int dest_x, int dest_y, int base){
		coordinate ritorno;
		ritorno.x = p.posizione_pedina.x;
		ritorno.y = p.posizione_pedina.y;
		/*DO LA PREFERENZA A SPOSTARMI PER RIGHE (X)*/
		if(ritorno.x < dest_x){
				if(semctl(id_semafori_matrix, (c+offset_matrix(ritorno.x+1,ritorno.y,base))->num_semaforo, GETVAL) == 1){
						ritorno.x = ritorno.x+1;
						ritorno.y = ritorno.y;
				}
				else{
						if(ritorno.y < dest_y){
								if(semctl(id_semafori_matrix,(c+offset_matrix(ritorno.x,ritorno.y+1,base))->num_semaforo, GETVAL) == 1){
										ritorno.x = ritorno.x;
										ritorno.y = ritorno.y+1;

								}
								else{
										ritorno.x = ritorno.x;
										ritorno.y = ritorno.y;
								}
						}
						else if(ritorno.y == dest_y){
								ritorno.x = ritorno.x;
								ritorno.y = ritorno.y;
						}

						else{
								if(semctl(id_semafori_matrix,(c+offset_matrix(ritorno.x,ritorno.y-1,base))->num_semaforo,GETVAL)==1){
										ritorno.x = ritorno.x;
										ritorno.y = ritorno.y-1;

								}
								else{
										ritorno.x = ritorno.x;
										ritorno.y = ritorno.y;
								}

						}
				}
		}

		else if(ritorno.x == dest_x){
				if(ritorno.y < dest_y){
						if(semctl(id_semafori_matrix,(c+offset_matrix(ritorno.x,ritorno.y+1,base))->num_semaforo,GETVAL) == 1){
								ritorno.x = ritorno.x;
								ritorno.y = ritorno.y+1;

						}
						else{
								ritorno.x = ritorno.x;
								ritorno.y = ritorno.y;
						}
				}
				else if(ritorno.y > dest_y){
						if(semctl(id_semafori_matrix,(c+offset_matrix(ritorno.x,ritorno.y-1,base))->num_semaforo,GETVAL) == 1){
								ritorno.x = ritorno.x;
								ritorno.y = ritorno.y-1;

						}
						else{
								ritorno.x = ritorno.x;
								ritorno.y = ritorno.y;
						}
				}
		}
		else{
				if(semctl(id_semafori_matrix,(c+offset_matrix(ritorno.x-1,ritorno.y,base))->num_semaforo,GETVAL) == 1){
						ritorno.x = ritorno.x-1;
						ritorno.y = ritorno.y;

				}
				else{
						if(ritorno.y < dest_y){
								if(semctl(id_semafori_matrix,(c+offset_matrix(ritorno.x,ritorno.y+1,base))->num_semaforo,GETVAL) == 1){
										ritorno.x = ritorno.x;
										ritorno.y = ritorno.y+1;

								}
								else{
										ritorno.x = ritorno.x;
										ritorno.y = ritorno.y;
								}
						}
						else if(ritorno.y == dest_y){
								ritorno.x = ritorno.x;
								ritorno.y = ritorno.y;
						}
						else{
								if(semctl(id_semafori_matrix,(c+offset_matrix(ritorno.x,ritorno.y-1,base))->num_semaforo, GETVAL) == 1){
										ritorno.x = ritorno.x;
										ritorno.y = ritorno.y-1;
								}
								else{
										ritorno.x = ritorno.x;
										ritorno.y = ritorno.y;
								}
						}
				}
		}
		return ritorno;
}
