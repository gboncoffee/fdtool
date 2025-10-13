CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pedantic -g -Og

TARGET = fdtool
OBJS = fdtool.o fds_parser.o closure.o min_cover.o keys.o normal_form.o

.PHONY: target clean

target: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	-rm *.o $(TARGET) $(TARGET)-debug
