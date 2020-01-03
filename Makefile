CC=mpicc
MPI=mpirun
NP=7
build: tema3

tema3: tema3.c
	$(CC) $^ -o $@ -lm

run: tema3
	$(MPI) -np $(NP) $< Colectie\ Poze\ Intrare/Colectie\ Poze/PGM/darth-vader.pgm darth-vader.pgm

clean:
	rm -f tema3