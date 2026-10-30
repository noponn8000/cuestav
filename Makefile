CC      := gcc
CFLAGS  := -Wall -Wextra -O2 
LDFLAGS := -lm -lncursesw -lpanel -lmenu

TARGET  := cuestav
SRC     := main.c file_tree.c cuefile.c
OBJ     := $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean

