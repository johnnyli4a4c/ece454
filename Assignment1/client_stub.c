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

void serializeList(arg_type *item, char *buffer)
{
	int seeker = 0;  /* integer to keep record of the wrinting position in 'buffer' */

	while(item != 0) /* copy contents of the linked list in buffer as long there 
			      are items in the list. */
	{
		memcpy(&buffer[seeker], &item->arg_size, sizeof(item->arg_size));
		seeker += sizeof(item->arg_size); /* move seeker ahead by 4 bytes */

		/* copy arg_val contents to the buffer */
		memcpy(&buffer[seeker], item->arg_val, item->arg_size);
		seeker += item->arg_size; /* ... and move the seeker ahead by the amount of	
					         characters in the array. */

		item = item->next; /* move on to the next item (or node) in the list */
	}
}

int listSize(arg_type *item)
{
	int size = 0;

	while (item != 0) {
		size += item->arg_size;         /* add arrayLen bytes to 'size' */
		size += sizeof(item->arg_size); /* add 4 bytes to 'size' */
		item = item->next;              /* ... and to the batmobil! */
	}
	return size;
}

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
	int listLength;

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
	va_list list;
	va_start(list, nparams);

	arg_type arg1;
	arg1.arg_size = va_arg(list, int);
	arg1.arg_val = va_arg(list, void*);
	arg_type arg2;
	arg2.arg_size = va_arg(list, int);
	arg2.arg_val = va_arg(list, void*);
	arg2.next = 0;
	arg1.next = &arg2;

	va_end(list);

	listLength = listSize(&arg1);

	char* buffer;
	/* allocate memory for the list, and let 'buffer' point to it. */
  	buffer = (char *)malloc(listLength);
  
  	/* serializing list pointed by *ptr to char pointer named buffer */
	serializeList(&arg1, buffer);

	if (sendto(fd, buffer, listLength, 0, (struct sockaddr *)&remaddr, slen)==-1) {
		perror("sendto");
		exit(1);
	}
	/* now receive an acknowledgement from the server */
	recvlen = recvfrom(fd, buf, BUFLEN, 0, (struct sockaddr *)&remaddr, &slen);
            if (recvlen >= 0) {
                    buf[recvlen] = 0;	/* expect a printable string - terminate it */
                    printf("received message: \"%s\"\n", buf);
            }

	close(fd);
	return;
}

int main() {
	int a = -10, b = 20;
	make_remote_call("ecelinux2.uwaterloo.ca", 
							10071,
							"addtwo",
							2,
							sizeof(int), (void *)(&a),
							sizeof(int), (void *)(&b));
}
