SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

default: $(BIN_DIR)/gameserver $(BIN_DIR)/player

$(OBJ_DIR)/gameserver.o: $(SRC_DIR)/gameserver_threads.c $(SRC_DIR)/header.h
	gcc -c $(SRC_DIR)/gameserver_threads.c -o $(OBJ_DIR)/gameserver.o

$(BIN_DIR)/gameserver: $(OBJ_DIR)/gameserver.o
	gcc $(OBJ_DIR)/gameserver.o -o $(BIN_DIR)/gameserver -lpthread

$(OBJ_DIR)/player.o: $(SRC_DIR)/player.c $(SRC_DIR)/header.h
	gcc -c $(SRC_DIR)/player.c -o $(OBJ_DIR)/player.o

$(BIN_DIR)/player: $(OBJ_DIR)/player.o
	gcc $(OBJ_DIR)/player.o -o $(BIN_DIR)/player

clean:
	-rm -f $(OBJ_DIR)/gameserver.o
	-rm -f $(OBJ_DIR)/player.o