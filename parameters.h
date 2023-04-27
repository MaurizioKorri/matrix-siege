#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/*STRUTTURA DATI CHE RAPPRESENTA I PARAMETRI DA CONFIGURARE*/
struct parameters {
		int SO_NUM_G;
		int SO_NUM_P;
		int SO_MAX_TIME;
		int SO_BASE;
		int SO_ALTEZZA;
		int SO_FLAG_MIN;
		int SO_FLAG_MAX;
		int SO_ROUND_SCORE;
		int SO_N_MOVES;
		int SO_MIN_HOLD_NSEC;
};


void set_parameters(FILE* f, struct parameters* p); /*Questa setta i parametri della struttura dati leggendo i valori dal file config.txt*/

/*LA FUNZIONE LEGGE DA FILE I PARAMETRI DEL GIOCO*/
void set_parameters(FILE* f, struct parameters* p){
		int infoGame[10];
		char line[30];
		int i;
		f = fopen("config.txt" , "r");
		if(!f){
				perror("FOPEN() IN PARAMETERS.H");
		}
		i = 0;
		/*AD OGNI LINEA DEL FILE ESTRAGGO SOLTANTO IL NUMERO CHE SALVO NELL`ARRAY infoGame[]*/
		while(fgets(line, 30, f) != NULL){
				char num[10];
				int numCounter, n;
				numCounter = 0;
				n = 0;
				while(line[n] != '\0'){
						if(isdigit(line[n])){
								num[numCounter] = line[n];
								numCounter++;
						}
						n++;
				}
				num[numCounter] = '\0';
				infoGame[i] = atoi(num);
				i++;
		}
		/*OGNI ELEMENTO DELL`ARRAY infoGame CORRISPONDE AD UN VALORE DA ASSEGNARE AL CAMPO i-ESIMO DELLA STRUTTURA*/
		p->SO_NUM_G = infoGame[0];
		p->SO_NUM_P = infoGame[1];
		p->SO_MAX_TIME = infoGame[2];
		p->SO_BASE = infoGame[3];
		p->SO_ALTEZZA = infoGame[4];
		p->SO_FLAG_MIN = infoGame[5];
		p->SO_FLAG_MAX = infoGame[6];
		p->SO_ROUND_SCORE = infoGame[7];
		p->SO_N_MOVES = infoGame[8];
		p->SO_MIN_HOLD_NSEC = infoGame[9];
		fclose(f);
}
