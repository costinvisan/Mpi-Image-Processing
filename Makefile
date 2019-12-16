CC=mpicc
MPI=mpirun
NP=1
build: tema3

tema3: tema3.c
	$(CC) $^ -o $@

run: tema3
	$(MPI) -np $(NP) $<

clean:
	rm -f tema3