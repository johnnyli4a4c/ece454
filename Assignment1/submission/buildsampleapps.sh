cp ../source/sampleclientapp.c ./
cp ../source/sampleserverapp.c ./

gcc -c sampleclientapp.c -g
gcc sampleclientapp.o -L. -lstubs -o sampleclientapp.out -g

gcc -c sampleserverapp.c -g
gcc sampleserverapp.o -L. -lstubs -o sampleserverapp.out -g