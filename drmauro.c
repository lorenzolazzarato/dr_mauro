#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "drmauro.h"

/* TO DO LIST:
*/

/* Questa funzione ritorna un codice enum colore in base all'intero passato */
enum colore assegna_colore(int codice_colore) {
	switch(codice_colore) {
		case 0:
			return ROSSO;
			break;
		case 1:
			return GIALLO;
			break;
		case 2:
			return BLU;
			break;
	}
}

/* Funzione di supporto a riempi_campo: controlla che un mostro possa essere generato
nella posizione r,c in base alle regole descritte nel documento di progetto.
Ritorno: 1 se è possibile posizionare il mostro nella cella r,c ; 0 altrimenti */
int controlla_mostri(struct cella campo[RIGHE][COLONNE], enum colore colore, int riga, int colonna) {
	int r, c;
	int contatore = 0;

	for(r = riga - 2; r <= riga + 2; r++) {
		if(r >= 0 && r < RIGHE && campo[r][colonna].tipo == MOSTRO && campo[r][colonna].colore == colore) {
			contatore++;
		} else if (r == riga) {
			contatore++;
		} else {
			contatore = 0;
		}
		if(contatore >= 3) {
			return 0;
		}
	}

	contatore = 0;
	for(c = colonna - 2; c <= colonna + 2; c++) {
		if(c >= 0 && c < COLONNE && campo[riga][c].tipo == MOSTRO && campo[riga][c].colore == colore) {
			contatore++;
		} else if (c == colonna) {
			contatore++;
		} else {
			contatore = 0;
		}
		if(contatore >= 3) {
			return 0;
		}
	}

	return 1;
}

/* Carica il campo dal file indicato da "percorso", se presente.
Assumiamo che il campo sia formattato correttamente */
void carica_campo(struct gioco *gioco, char *percorso) {
	int r, c;
	char cella;
	FILE *f = fopen(percorso, "r");
	if(f) {
		r = 0;
		c = 0;
		while(fscanf(f, "%c", &cella) && !feof(f)) {
			if(cella == '\n') {
				c = 0;
				r++;
			} else {
				switch(cella) {
					case 'R':
						(gioco -> campo[r][c]).tipo = MOSTRO;
						(gioco -> campo[r][c]).colore = ROSSO;
						break;
					case 'G':
						(gioco -> campo[r][c]).tipo = MOSTRO;
						(gioco -> campo[r][c]).colore = GIALLO;
						break;
					case 'B':
						(gioco -> campo[r][c]).tipo = MOSTRO;
						(gioco -> campo[r][c]).colore = BLU;
						break;
				}
				c++;
			}
		}
	} else {
		printf("Errore sul file\n");
	}
	fclose(f);
}

/* Carico il campo con mostri generati casualmente, secondo le regole descritte
dal documento di progetto */
void riempi_campo(struct gioco *gioco, int difficolta) {
	int n_virus = (difficolta + 1) * 4;
	int r, c;
	struct cella *corrente;
	enum colore colore;
	srand(time(NULL));

	while(n_virus != 0) {
		/* calcolo riga e colonna casualmente attraverso rand
		e prendo la cella corrispondente*/
		do {
			r = rand() % RIGHE;
			c = rand() % COLONNE;
		}while(r<5);
		corrente = &(gioco -> campo[r][c]);

		/* bisogna controllare che non ci siano 3 mostri dello stesso
		colore in fila */
		colore = assegna_colore(rand() % 3);
		if(corrente -> tipo == VUOTO && controlla_mostri(gioco -> campo, colore, r, c)) {
			corrente -> tipo = MOSTRO;
			corrente -> colore = colore;
			n_virus--;
		}
	}
}

/* Genero una pastiglia con un id univoco nella posizione di partenza */
void crea_pastiglia(struct gioco *gioco) {
	int c = COLONNE / 2;
	srand(time(NULL));
	gioco -> active_id++;
	(gioco -> campo[0][c-1]).tipo = PASTIGLIA;
	(gioco -> campo[0][c-1]).colore = assegna_colore(rand() % 3);
	(gioco -> campo[0][c-1]).id = gioco -> active_id;
	(gioco -> campo[0][c]).tipo = PASTIGLIA;
	(gioco -> campo[0][c]).colore = assegna_colore(rand() % 3);
	(gioco -> campo[0][c]).id = gioco -> active_id;
	gioco -> comando_id = 1;
}

/* Dato un id, trova la pastiglia all'interno del campo da gioco e ne ritorna la
posizione e l'orientamento attraverso dei puntatori */
int trova_pastiglia(struct gioco* gioco, int *r, int *c, int *d, int id) {
	int riga, colonna, trovato;
	riga = 0;
	colonna = 0;
	trovato = 0;
	while(riga < RIGHE && !trovato) {
		colonna = 0;
		while(colonna < COLONNE && !trovato) {
			if(gioco -> campo[riga][colonna].id == id) {
				trovato = 1;
				*r = riga;
				*c = colonna;
				/* controllo l'orientamento della pastiglia: 0 orizzontale, 1 verticale */
				if(colonna < COLONNE-1 && gioco -> campo[riga][colonna+1].tipo == PASTIGLIA && gioco -> campo[riga][colonna+1].id == id) {
					*d = 0;
				} else {
					*d = 1;
				}
			}
			colonna++;
		}
		riga++;
	}

	return trovato;
}

/* Conto quanti elementi di tipo MOSTRO sono presenti in campo */
int conta_mostri(struct gioco* gioco) {
	int r, c;
	int mostri = 0;
	for(r = 0; r < RIGHE; r++) {
		for(c = 0; c < COLONNE; c++) {
			if(gioco -> campo[r][c].tipo == MOSTRO) { mostri++; }
		}
	}
	return mostri;
}

/* Quando una pastiglia viene appoggiata e non può più muoversi, bisogna
controllare se ci sono 4+ celle contigue dello stesso colore che possono essere
eliminate.
Ritorno: 1 se ho eliminato qualcosa, 0 altrimenti. */
int elimina(struct gioco* gioco) {
	int riga, colonna, rc;
	int indice = 0;
	int consecutivi = 0;
	int registrando = 0;
	int coordinate[COLONNE*RIGHE][2];

	/* Segniamo in una matrice di supporto la posizione di tutte le celle idonee
	ad essere eliminate, controllando sia per riga che per colonna.
	Questo ci consente di eliminare tutte le celle in una volta sola e di considerare
	anche le combo ad L e a T. */
	for(riga = 0; riga < COLONNE*RIGHE; riga++) {
		for(colonna = 0; colonna < 2; colonna++) {
			coordinate[riga][colonna] = -1;
		}
	}
	for(riga = 0; riga < RIGHE; riga++) {
			consecutivi = 0;
			registrando = 0;
			for(colonna = 1; colonna < COLONNE; colonna++) {
					if(gioco -> campo[riga][colonna].tipo != VUOTO && gioco -> campo[riga][colonna-1].tipo != VUOTO && gioco -> campo[riga][colonna].colore == gioco -> campo[riga][colonna-1].colore) {
							consecutivi++;
					} else {
							consecutivi = 0;
							registrando = 0;
					}
					if(consecutivi >= 3) {
							if(registrando) {
									coordinate[indice][0] = riga;
									coordinate[indice][1] = colonna;
									indice++;
							} else {
									for(rc = (colonna - consecutivi); rc <= colonna; rc++) {
											coordinate[indice][0] = riga;
											coordinate[indice][1] = rc;
											indice++;
									}
									registrando = 1;
							}
					}
			}
	}

	consecutivi = 0;
	registrando = 0;
	for(colonna = 0; colonna < COLONNE; colonna++) {
			consecutivi = 0;
			registrando = 0;
			for(riga = 1; riga < RIGHE; riga++) {
					if(gioco -> campo[riga][colonna].tipo != VUOTO && gioco -> campo[riga-1][colonna].tipo != VUOTO && gioco -> campo[riga][colonna].colore == gioco -> campo[riga-1][colonna].colore) {
							consecutivi++;
					} else {
							consecutivi = 0;
							registrando = 0;
					}
					if(consecutivi >= 3) {
							if(registrando) {
									coordinate[indice][0] = riga;
									coordinate[indice][1] = colonna;
									indice++;
							} else {
									for(rc = (riga - consecutivi); rc <= riga; rc++) {
											coordinate[indice][0] = rc;
											coordinate[indice][1] = colonna;
											indice++;
									}
									registrando = 1;
							}
					}

			}
	}

	for(rc = 0; rc < indice; rc++) {
		gioco -> campo[coordinate[rc][0]][coordinate[rc][1]].tipo = VUOTO;
		gioco -> campo[coordinate[rc][0]][coordinate[rc][1]].id = 0;
	}

	return (indice > 0) ? 1 : 0;
}

/* Dopo aver eliminato qualcosa, devo controllare se ci sono delle pastiglie da far
cadere verso il basso. Questa funzione sposta di 1 posizione verso il basso tutte
le pastiglie che possono cadere.
Ritorno: 1 se è caduto qualcosa, 0 altrimenti */
int gravita(struct gioco* gioco) {
	int r, c, id;
	int risultato = 0;
	int dir = 0;

	for(r = RIGHE - 2; r >= 0; r--) {
		for(c = 0; c < COLONNE; c++) {
			if(gioco -> campo[r][c].tipo == PASTIGLIA) {
				id = gioco -> campo[r][c].id;

				/* controllo se la pastiglia è singola oppure se è parte di una
				pastiglia più grande */
				if((r-1) >= 0 && gioco -> campo[r-1][c].tipo == PASTIGLIA && gioco -> campo[r-1][c].id == id) {
					dir = 2;
				} else if((c-1) >= 0 && gioco -> campo[r][c-1].tipo == PASTIGLIA && gioco -> campo[r][c-1].id == id) {
					dir = -1;
				} else if((c+1) < COLONNE && gioco -> campo[r][c+1].tipo == PASTIGLIA && gioco -> campo[r][c+1].id == id) {
					dir = 1;
				} else {
					dir = 0;
				}
				/* caduta della pastiglia */
				switch(dir) {
					case -1:
						if(gioco -> campo[r+1][c].tipo == VUOTO && gioco -> campo[r+1][c-1].tipo == VUOTO) {
							gioco -> campo[r+1][c] = gioco -> campo[r][c];
							gioco -> campo[r+1][c-1] = gioco -> campo[r][c-1];
							gioco -> campo[r][c].tipo = VUOTO;
							gioco -> campo[r][c].id = 0;
							gioco -> campo[r][c-1].tipo = VUOTO;
							gioco -> campo[r][c-1].id = 0;
							risultato = 1;
						}
						break;

					case 0:
						if(gioco -> campo[r+1][c].tipo == VUOTO) {
							gioco -> campo[r+1][c] = gioco -> campo[r][c];
							gioco -> campo[r][c].tipo = VUOTO;
							gioco -> campo[r][c].id = 0;
							risultato = 1;
						}
						break;

					case 1:
						if(gioco -> campo[r+1][c].tipo == VUOTO && gioco -> campo[r+1][c+1].tipo == VUOTO) {
							gioco -> campo[r+1][c] = gioco -> campo[r][c];
							gioco -> campo[r+1][c+1] = gioco -> campo[r][c+1];
							gioco -> campo[r][c].tipo = VUOTO;
							gioco -> campo[r][c].id = 0;
							gioco -> campo[r][c+1].tipo = VUOTO;
							gioco -> campo[r][c+1].id = 0;
							risultato = 1;
						}
						break;

					case 2:
						if(gioco -> campo[r+1][c].tipo == VUOTO) {
							gioco -> campo[r+1][c] = gioco -> campo[r][c];
							gioco -> campo[r][c] = gioco -> campo[r-1][c];
							gioco -> campo[r-1][c].tipo = VUOTO;
							gioco -> campo[r-1][c].id = 0;
							risultato = 1;
						}
						break;

					default: break;
				}
			}
		}
	}

	return risultato;
}

/* Abbiamo messo lo spostamento della pastiglia di una cella verso il basso
dentro una funzione, visto che viene richiamata anche in altri casi; per esempio
quando non può più muoversi verso la direzione scelta */
void sposta_giu(struct gioco* gioco, int r, int c, int direzione) {
	/* effettuo lo spostamento in base all'orientamento della pastiglia.
	Quando ho finito aggiorno comando_id per passare alla fase di eventuale eliminazione
	delle pastiglie */
	if(r < RIGHE - 1) {
		if(direzione == 0) {
			if(gioco -> campo[r+1][c].tipo == VUOTO && gioco -> campo[r+1][c+1].tipo == VUOTO) {
				gioco -> campo[r+1][c] = gioco -> campo[r][c];
				gioco -> campo[r+1][c+1] = gioco -> campo[r][c+1];
				gioco -> campo[r][c].tipo = VUOTO;
				gioco -> campo[r][c].id = 0;
				gioco -> campo[r][c+1].tipo = VUOTO;
				gioco -> campo[r][c+1].id = 0;
			} else {
				gioco -> comando_id = 2;
			}
		} else {
			if(r < RIGHE - 2 && gioco -> campo[r+2][c].tipo == VUOTO) {
				gioco -> campo[r+2][c] = gioco -> campo[r+1][c];
				gioco -> campo[r+1][c] = gioco -> campo[r][c];
				gioco -> campo[r][c].tipo = VUOTO;
				gioco -> campo[r][c].id = 0;
			} else {
				gioco -> comando_id = 2;
			}
		}
	} else {
		gioco -> comando_id = 2;
	}
}

/* Data la posizione di una pastiglia all'interno del campo, effettua uno spostamento
in base al comando ricevuto in input */
void muovi_pastiglia(struct gioco* gioco, enum mossa comando, int r, int c, int direzione) {
	switch(comando) {
		case SINISTRA:
			if(c > 0 && gioco -> campo[r][c-1].tipo == VUOTO) {
				if(direzione == 0) {
					gioco -> campo[r][c-1] = gioco -> campo[r][c];
					gioco -> campo[r][c] = gioco -> campo[r][c+1];
					gioco -> campo[r][c+1].tipo = VUOTO;
					gioco -> campo[r][c+1].id = 0;
				} else if(r < RIGHE-1 && gioco -> campo[r+1][c-1].tipo == VUOTO){
					gioco -> campo[r][c-1] = gioco -> campo[r][c];
					gioco -> campo[r+1][c-1] = gioco -> campo[r+1][c];
					gioco -> campo[r][c].tipo = VUOTO;
					gioco -> campo[r][c].id = 0;
					gioco -> campo[r+1][c].tipo = VUOTO;
					gioco -> campo[r+1][c].id = 0;
				}
			} else {
				sposta_giu(gioco, r, c, direzione);
			}
			break;
		case DESTRA:
			if(c < COLONNE -1) {
				if(direzione == 0 && c < COLONNE - 2 && gioco -> campo[r][c+2].tipo == VUOTO) {
					gioco -> campo[r][c+2] = gioco -> campo[r][c+1];
					gioco -> campo[r][c+1] = gioco -> campo[r][c];
					gioco -> campo[r][c].tipo = VUOTO;
					gioco -> campo[r][c].id = 0;
				} else if(r < RIGHE - 1 && gioco -> campo[r+1][c+1].tipo == VUOTO && gioco -> campo[r][c+1].tipo == VUOTO){
					gioco -> campo[r][c+1] = gioco -> campo[r][c];
					gioco -> campo[r+1][c+1] = gioco -> campo[r+1][c];
					gioco -> campo[r][c].tipo = VUOTO;
					gioco -> campo[r][c].id = 0;
					gioco -> campo[r+1][c].tipo = VUOTO;
					gioco -> campo[r+1][c].id = 0;
				} else {
					sposta_giu(gioco, r, c, direzione);
				}
			}
			break;
		case NONE:
			sposta_giu(gioco, r, c, direzione);
			break;
		case ROT_SX:
			if(direzione == 0 && r > 0 && gioco -> campo[r-1][c].tipo == VUOTO) {
				gioco -> campo[r-1][c] = gioco -> campo[r][c+1];
				gioco -> campo [r][c+1].tipo = VUOTO;
				gioco -> campo[r][c+1].id = 0;
			} else if(direzione == 1 && c < COLONNE - 1 && gioco -> campo[r+1][c+1].tipo == VUOTO) {
				gioco -> campo[r+1][c+1] = gioco -> campo[r+1][c];
				gioco -> campo[r+1][c] = gioco -> campo[r][c];
				gioco -> campo[r][c].tipo = VUOTO;
				gioco -> campo[r][c].id = 0;
			} else {
				sposta_giu(gioco, r, c, direzione);
			}
			break;
		case ROT_DX:
			if(direzione == 0 && r > 0 && gioco -> campo[r-1][c].tipo == VUOTO) {
					gioco -> campo[r-1][c] = gioco -> campo[r][c];
					gioco -> campo[r][c] = gioco -> campo[r][c+1];
					gioco -> campo[r][c+1].tipo = VUOTO;
					gioco -> campo[r][c+1].id = 0;
			} else if(direzione == 1 && c < COLONNE - 1 && gioco -> campo[r+1][c+1].tipo == VUOTO){
					gioco -> campo[r+1][c+1] = gioco -> campo[r][c];
					gioco -> campo[r][c].tipo = VUOTO;
					gioco -> campo[r][c].id = 0;
			} else {
				sposta_giu(gioco, r, c, direzione);
			}
			break;

		case GIU:
			gioco -> comando_id = 3;
			break;
	}


}

void step(struct gioco *gioco, enum mossa comando) {
	/* COMANDI
	comando_id 0 -> nuova pastiglia
	comando_id 1 -> pastiglia in movimento
	comando_id 2 -> pastiglia appoggiata, eventuale eliminazione celle
	comando_id 3 -> fai cadere pastiglie sospese
	*/
	int r, c;
	int direzione = 0;
	int punteggio_parziale = 0;
	int n_mostri;

	if(gioco -> comando_id == 0) {
		/* nuova pastiglia */
		crea_pastiglia(gioco);
		gioco -> moltiplicatore = 1;
	} else if(gioco -> comando_id == 1){
		/* movimento pastiglia attiva */
		trova_pastiglia(gioco, &r, &c, &direzione, gioco -> active_id);
		muovi_pastiglia(gioco, comando, r, c, direzione);
	} else if (gioco -> comando_id == 2){
		/* Se elimina ritorna 1 (ha eliminato qualcosa), conto quanti mostri sono stati
		eliminati, facendo la differenza tra prima e dopo la chiamata alla funzione,
		e calcolo il punteggio della mossa.
		Se non ho eliminato nulla posso buttare una nuova pastiglia. */
		n_mostri = conta_mostri(gioco);
		if(elimina(gioco)) {
			n_mostri = n_mostri - conta_mostri(gioco);
			while(n_mostri != 0) {
				punteggio_parziale += (int) 100*pow(2, n_mostri);
				n_mostri--;
			}
			/* Aumento il punteggio, il moltiplicatore e passo al controllo della gravità */
			gioco -> punti += (punteggio_parziale * gioco -> moltiplicatore);
			gioco -> moltiplicatore *= 2;
			gioco -> comando_id = 3;
		} else {
			gioco -> comando_id = 0;
		}
	} else {
		/* Continuo a chiamare gravita finchè tutte le pastiglie si sono
		abbassate, poi richiamo elimina */
		if(!gravita(gioco)) {
			gioco -> comando_id = 2;
		}
	}
}

/* Controlla lo stato della partita */
enum stato vittoria(struct gioco *gioco) {
	/* se le celle dove deve essere piazzata una nuova pastiglia sono occupate,
	il gioco è finito */
	if(((gioco -> campo[0][(COLONNE/2)-1]).tipo != VUOTO || (gioco -> campo[0][COLONNE/2]).tipo != VUOTO) && gioco -> comando_id != 1) {
		return SCONFITTA;
	} else if(conta_mostri(gioco) == 0) {
		return VITTORIA;
	}
	return IN_CORSO;
}
