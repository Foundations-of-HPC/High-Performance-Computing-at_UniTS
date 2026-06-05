CC       ?= gcc
CFLAGS   ?= -std=c11 -O2 -Wall -Wextra -pedantic
LDFLAGS  ?=
LDLIBS   ?= -lm -lz

TARGET = mandelbrot_serial
SRC    = mandelbrot_serial.c

.PHONY: all clean smoke

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

smoke: $(TARGET)
	./$(TARGET) --ppu 64 --kmax 256 --output smoke.png
	file smoke.png

clean:
	rm -f $(TARGET) smoke.png
