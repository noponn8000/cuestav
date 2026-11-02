CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -g
LDFLAGS := -lm -lncursesw 

TARGET  := cuestav
SRC     := main.c file_tree.c cuefile.c stringview.c
OBJ     := $(SRC:%.c=build/%.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

build/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean

