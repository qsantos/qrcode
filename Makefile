CC := gcc
CFLAGS += -Wall -Wextra -Wpedantic -std=c99 -O3
LDFLAGS += -O3

qrcode: qrcode.o pbm.o encoder.o decoder.o rs.o bch.o blocks.o modules.o data.o

clean:
	rm -f *.o

destroy: clean
	rm -f qrcode
