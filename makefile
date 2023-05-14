CC := gcc
EXEC := parser
SRCS := parser.c
OBJS := $(SRCS:.c=.o)
HEADER := stb_image_write.h

make: $(EXEC)

$(EXEC): $(OBJS) Makefile
	$(CC) -o $@ $(OBJS)

$(OBJS): $(@:.o=.c) $(HEADER) Makefile
	$(CC) -o $@ $(@:.o=.c) -c

clean:
	rm -f $(EXEC) $(OBJS)
