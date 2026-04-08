CC = gcc
CFLAGS = -Wall -g -MMD -MP

SRCS = $(wildcard *.c)
OBJDIR = $(BIN_DIR)
OBJS = $(SRCS:%.c=$(OBJDIR)/%.o)
DEPS = $(OBJS:.o=.d)

# Nome do executável final
BIN_DIR = bin
TARGET = main

# Regra principal
all: $(TARGET)

# Como ligar os objetos para criar o executável
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Regra genérica para compilar .c em .o
$(OBJDIR)/%.o: %.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Limpeza
clean:
	rm -f $(OBJS) $(TARGET)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

-include $(DEPS)
