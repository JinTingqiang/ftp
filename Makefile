CC=gcc
STD=-std=gnu99
FLAG=-Wall -Werror
OBJ=main.o ftp_client.o tools.o
BIN=ftp

all:$(OBJ)
	$(CC) $(OBJ) -o $(BIN) && ./$(BIN) 127.0.0.1

%.o:%.c
	$(CC) $(STD) $(FLAG) -c $< -o $@

clean:
	rm -rf $(OBJ) $(BIN)
	rm -rf *.h.gch *~
