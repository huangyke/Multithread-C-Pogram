
#Thisnext linedefines arguments that are passed to all compilation steps

CCFLAGS = -g -Wall -lpthread 

main: main.c test_messages.o pmessages.o list.o
	gcc -o main main.c test_messages.o pmessages.o list.o $(CCFLAGS)
#$@ $^ $(CCFLAGS)

test_messages.o: test_messages.c pmessages.h pmessages.o list.o
	gcc $(CCFLAGS) -c test_messages.c
	
list.o: list.c list.h
	gcc $(CCFLAGS) -c list.c
pmessages.o: pmessages.c pmessages.h pmessages_private.h
	gcc $(CCFLAGS) -c pmessages.c

clean:
	-rm list.o pmessages.o

spotless: clean
	-rm test_messages

