SRCS := main.c hashmap.c

all:
    @echo "Use make optimized for optimized build, or make default for default configuration"

optimized:
    gcc $(SRCS) -march=native -O2 -o mcr

default:
    gcc $(SRCS) -o mcr