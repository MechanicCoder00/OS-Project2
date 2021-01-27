CC	= gcc
TARGET1	= oss
TARGET2 = prime
OBJ1	= oss.o 
OBJ2	= prime.o

ALL:	$(TARGET1) $(TARGET2)

$(TARGET1): $(OBJ1)
	$(CC) -o $@ $(OBJ1)
	
$(TARGET2): $(OBJ2)
	$(CC) -o $@ $(OBJ2)

oss.o: oss.c
	$(CC) -c oss.c

prime.o: prime.c
	$(CC) -c prime.c

.PHONY: clean
clean:
	/bin/rm -f *.o *.log $(TARGET1) $(TARGET2)
