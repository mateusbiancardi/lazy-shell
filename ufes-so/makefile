CC = gcc
CFLAGS = -Wall -g

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

# Nome do executável final
TARGET = main

# Regra principal
all: $(TARGET)

# Como ligar os objetos para criar o executável
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Regra genérica para compilar .c em .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpeza
clean:
	rm -f $(OBJS) $(TARGET)