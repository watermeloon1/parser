CC := gcc
EXEC := parser
SRCS := parser.c
OBJS := $(SRCS:.c=.o)

make: $(EXEC)

$(EXEC): $(OBJS) Makefile
	$(CC) -o $@ $(OBJS)

$(OBJS): $(@:.o=.c) Makefile
	$(CC) -o $@ $(@:.o=.c) -c

clean:
	rm -f $(EXEC) $(OBJS)
