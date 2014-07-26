OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c))

fs_client: fs_client.o fs_dummy.o

client: libstubs.a client-api.o
	gcc client-api.o -L./simplified_rpc -lstubs -o client

server: libstubs.a server.o
	gcc server.o -L./simplified_rpc -lstubs -o server

$(OBJECTS): %.o: %.c ece454_fs.h
	gcc -c $< -o $@

libstubs.a: 
	cd simplified_rpc; make libstubs.a

clean:
	rm -rf a.out *.o core *.a fs_client server client
