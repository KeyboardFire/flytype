.PHONY: all clean

bin/%.o: src/%.c
	gcc -Wall -Wpedantic -Wextra -lncurses -c $< -o $@

bin/flytype: $(patsubst src/%.c, bin/%.o, $(wildcard src/*.c))
	gcc -Wall -Wpedantic -Wextra -lncurses $^ -o $@

all: bin/flytype

clean:
	-rm -f bin/*
