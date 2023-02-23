CC = gcc
DEPS = mystdio.h myutil.h mystdlib.h mysh_commands.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $<

mysh: mysh.o mystdio.o myutil.o mystdlib.o mysh_commands.o
	$(CC) -o mysh mysh.o mystdio.o myutil.o mystdlib.o mysh_commands.o
