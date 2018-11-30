jobExecutor : jobExecutor.o functions.o
	gcc jobExecutor.o functions.o -o jobExecutor

jobExecutor.o : jobExecutor.c header.h
	gcc -c jobExecutor.c

functions.o : functions.c header.h
	gcc -c functions.c


clean:
	rm jobExecutor jobExecutor.o functions.o
