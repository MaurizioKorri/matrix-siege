compiling_pedina:pedina.c
	gcc -std=c89 -pedantic -lm pedina.c -o pedina
compiling_giocatore:giocatore.c
	gcc -std=c89 -pedantic -lm giocatore.c -o giocatore
compiling_master:master.c
	gcc -std=c89 -pedantic -lm master.c -o master
compiling:compiling_pedina compiling_giocatore compiling_master
	make compiling_pedina compiling_giocatore compiling_master
