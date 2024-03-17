CC = gcc
CFLAGS = -Wall
DEPS = commandes.h jsh.h redirections.h jobs.h
EXEC = jsh
OBJS = jsh.c commandes.c redirections.c jobs.c

all : $(EXEC)

%.o : %.c $(DEPS)
	   $(CC) $(CFLAGS) -c $< -lreadline

jsh : $(OBJS)
		$(CC) $(CFLAGS) -o $@ $^ -lreadline

run : 
	./jsh

test : 
	valgrind ./jsh

clean:
	rm -rf $(EXEC) *.o 