cp ../source/sampleclientapp.c ./
cp ../source/sampleserverapp.c ./

gcc -c sampleclientapp.c
gcc sampleclientapp.o -L. -lstubs -o sampleclientapp.out

gcc -c sampleserverapp.c
gcc sampleserverapp.o -L. -lstubs -o sampleserverapp.out