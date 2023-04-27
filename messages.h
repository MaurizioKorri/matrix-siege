#include "matrix.h"
/*DIMENSIONE MASSIMA DA USARE NELLA MSGRCV PERCHE IL MESSAGGIO PIU GRANDE E' da_giocatore_a_master DA 24 BYTE - sizeof(long)*/
#define MAX_DIM 16
/*DICHIARIZIONI DELLE STRUTTURE NECESSARIE PER I MESSAGGI DA INVIARE/RICEVERE*/
struct da_master_a_giocatore{
		long mtype;
		boolean siamo_in_game;
};

struct da_giocatore_a_master{
		long mtype;
		giocatore player_ref;
		coordinate posizione_pedina;
};

struct da_giocatore_a_pedina{
		long mtype;
		coordinate destinazione;
};

struct da_pedina_a_giocatore{
		long mtype;
		coordinate nuova_posizione;

};
