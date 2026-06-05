CC      ?= gcc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -pedantic
LDFLAGS ?=
LDLIBS  ?=

TARGET = sample_sort_serial
SRC    = sample_sort_serial.c

.PHONY: all clean run check

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS) $(LDLIBS)

run: $(TARGET)
	./$(TARGET) --n 1000000 --nbuckets 8 --distribution uniform

check: $(TARGET)
	./$(TARGET) --n 100000 --nbuckets 8 --distribution uniform
	./$(TARGET) --n 100000 --nbuckets 8 --distribution skewed --oversample 4
	./$(TARGET) --n 100000 --nbuckets 8 --distribution few-unique
	./$(TARGET) --n 100000 --nbuckets 8 --distribution reverse
	./$(TARGET) --n 100000 --nbuckets 8 --distribution almost-sorted

clean:
	rm -f $(TARGET)
