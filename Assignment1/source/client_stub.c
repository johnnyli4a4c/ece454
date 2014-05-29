#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "ece454rpc_types.h"
#include "mybind.c"
#include <stdarg.h>

#define BUFLEN 2048

return_type make_remote_call(const char *servernameorip, 
							const int serverportnumber,
							const char *procedure_name,
							const int nparams,
							...) {
	struct sockaddr_in myaddr, remaddr;
	int fd, i, slen=sizeof(remaddr);
	char buf[BUFLEN];	/* message buffer */
	int recvlen;		/* # bytes in acknowledgement message */
	struct hostent *he;	/* used to get ip from hostname */
	int bufferSize, seeker, procedureNameSize;
	va_list list;
	char *buffer;

	/* create a socket */
	if ((fd=socket(AF_INET, SOCK_DGRAM, 0))==-1)
		perror("socket not created\n");

	/* bind it to all local addresses and pick any port number */
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(0);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return;
	}       

	/* now define remaddr, the address to whom we want to send messages */
	/* For convenience, the host address is expressed as a numeric IP address */
	/* that we will convert to a binary format via inet_aton */

	/* resolve hostname */
	if ( (he = gethostbyname(servernameorip) ) == NULL ) {
		exit(1); /* error */
	}

	memcpy(&remaddr.sin_addr, he->h_addr_list[0], he->h_length);
	remaddr.sin_family = AF_INET;
	remaddr.sin_port = htons(serverportnumber);

	/* now let's send the messages */
	va_start(list, nparams);
	bufferSize = 0;	
	for (i = 0; i < nparams; i++) {
		int arg_size = va_arg(list, int);
		bufferSize += sizeof(arg_size);
		bufferSize += arg_size;
		va_arg(list, void*);
	}
	va_end(list);

	procedureNameSize = sizeof(procedure_name);
	bufferSize += sizeof(procedureNameSize) + procedureNameSize + sizeof(nparams);
	/* allocate memory for the list, and let 'buffer' point to it. */
  	buffer = (char *)malloc(bufferSize);
  	seeker = 0;
  	memcpy(&buffer[seeker], &procedureNameSize, sizeof(procedureNameSize));
  	seeker += sizeof(procedureNameSize);
  	memcpy(&buffer[seeker], procedure_name, procedureNameSize);
  	seeker += procedureNameSize;
  	memcpy(&buffer[seeker], &nparams, sizeof(nparams));
  	seeker += sizeof(nparams);

	va_start(list, nparams);
	for (i = 0; i < nparams; i++) {
		int arg_size = va_arg(list, int);
		void* arg_val = va_arg(list, void*);

		memcpy(&buffer[seeker], &arg_size, sizeof(arg_size));
		seeker += sizeof(arg_size); /* move seeker ahead by 4 bytes */

		/* copy arg_val contents to the buffer */
		memcpy(&buffer[seeker], arg_val, arg_size);
		seeker += arg_size; /* ... and move the seeker ahead by the size of the argument. */
	}
	va_end(list);

	if (sendto(fd, buffer, bufferSize, 0, (struct sockaddr *)&remaddr, slen)==-1) {
		perror("sendto");
		exit(1);
	}
	/* now receive an acknowledgement from the server */
	return_type returnedVal;

	recvlen = recvfrom(fd, buf, BUFLEN, 0, (struct sockaddr *)&remaddr, &slen);
    if (recvlen >= 0) {
    	memcpy(&returnedVal.return_size, buf, sizeof(int));
    	returnedVal.return_val = (void *)malloc(returnedVal.return_size);
		memcpy(returnedVal.return_val, &buf[sizeof(int)], returnedVal.return_size);
    }

	close(fd);
	return returnedVal;
}
