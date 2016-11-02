
NAME = cl-heatmap
CC = gcc
CFLAGS += -std=gnu11 -Wall -g3
#CFLAGS += -fsanitize=address -fno-omit-frame-pointer -lasan -g3
CFLAGS += -lOpenCL -lproj -lpthread -lm -lpng -lgsl -ljson-c

CSRCS = $(addprefix src/, main.c colormaps.c coords.c utils.c)

OBJS = $(CSRCS:.c=.o)

all: $(NAME)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(NAME) $(OBJS)

.PHONY: clean
