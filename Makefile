SRCS 	  := src/main.c src/hashmap.c
CFLAGS_O  := -O2 -march=native -Iinclude
CFLAGS_D  := -Iinclude
all:
	@echo "Use make optimized for optimized build, or make default for default configuration"

optimized:
	gcc $(SRCS) $(CFLAGS_O) -o mcr

default:
	gcc $(SRCS) $(CFLAGS_D) -o mcr
