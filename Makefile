C99 = gcc
CFLAGS = -Wall -pedantic -Werror

SERVER = rakeserver
S_HEADER = $(SERVER).h
S_C = rakeserver.c

CLIENT = rake-c
C_C = rake-c.c

build_s:
	@echo "Compile rakeserver"
	$(C99) $(CFLAGS) -o $(SERVER) $(S_C)

build_c:
	@echo "Compile rake-c"
	$(C99) $(CFLAGS) -o $(CLIENT) $(C_C)

clean:
	rm -f $(CLIENT) $(SERVER)