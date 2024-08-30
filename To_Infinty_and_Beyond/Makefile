CC=/usr/bin/gcc
COMPILER_FLAGS=-Wall -g
LINKER_FLAGS=`sdl2-config --libs --cflags` -lSDL2_image -lSDL2_ttf -lm

bin/To_Infinity_and_Beyond: build/main.o build/sdl.o build/move_ship.o build/gravity.o build/gameconsole.o
	$(CC) $(COMPILER_FLAGS) build/main.o build/sdl.o build/move_ship.o build/gravity.o build/gameconsole.o $(LINKER_FLAGS) -o bin/To_Infinity_and_Beyond

build/main.o: src/main.c include/common.h include/structs.h
	$(CC) -c $(COMPILER_FLAGS) src/main.c $(LINKER_FLAGS) -o build/main.o

build/sdl.o: src/sdl.c include/common.h
	$(CC) -c $(COMPILER_FLAGS) src/sdl.c $(LINKER_FLAGS) -o build/sdl.o

build/move_ship.o: src/move_ship.c include/common.h
	$(CC) -c $(COMPILER_FLAGS) src/move_ship.c $(LINKER_FLAGS) -o build/move_ship.o

build/gravity.o: src/gravity.c include/common.h include/structs.h
	$(CC) -c $(COMPILER_FLAGS) src/gravity.c $(LINKER_FLAGS) -o build/gravity.o

build/gameconsole.o: src/gameconsole.c include/common.h include/structs.h
	$(CC) -c $(COMPILER_FLAGS) src/gameconsole.c $(LINKER_FLAGS) -o build/gameconsole.o

clean:
	$(RM) bin/* build/*