server: server.c libstubs.a
	gcc server.c -Lsimplified_rpc -lstubs -o fsServer

client-api.a: client-api.c libstubs.a
	gcc -c client-api.c -Lsimplified_rpc -lstubs
	ar r libclient-api.a client-api.o 

libstubs.a: 
	cd simplified_rpc; make libstubs.a

clean:
	rm -rf *.a *.o server a.out core
