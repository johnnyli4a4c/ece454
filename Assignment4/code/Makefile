server: server.c libstubs.a
	gcc server.c -Lsimplified_rpc -lstubs -o fsServer

client-api.a: client-api.c libstubs.a
	gcc -c client-api.c
	ar r libclient-api.a client-api.o simplified_rpc/server_stub.o simplified_rpc/client_stub.o simplified_rpc/helper.o simplified_rpc/mybind.o

libstubs.a: 
	cd simplified_rpc; make libstubs.a

clean:
	rm -rf *.a *.o fsServer a.out core
