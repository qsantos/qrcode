CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -pedantic -ansi -std=c99 -O3
LDFLAGS = -O3
TARGETS = qrcode

$(TARGETS): main.o pbm.o qrcode.o modules.o
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o

destroy: clean
	rm -f $(TARGETS)

rebuild: destroy $(TARGETS)
