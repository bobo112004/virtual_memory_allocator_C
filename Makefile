#Popescu Bogdan-Stefan 312CAa 2023-2024

# compiler setup
CC=gcc
CFLAGS=-Wall -Wextra -std=c99

# define targets
TARGETS = sfl

build: $(TARGETS)

sfl: sfl.c
	$(CC) $(CFLAGS) sfl.c -o sfl

pack:
	zip -FSr 312CA_PopescuBogdanStefan_Tema1.zip README.md Makefile *.c *.h
run_sfl:
	./sfl
clean:
	rm -f $(TARGETS)

.PHONY: pack clean