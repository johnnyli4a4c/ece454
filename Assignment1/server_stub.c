#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "ece454rpc_types.h"
#include "mybind.c"

#define BUFSIZE 2048



int ret_int;
return_type r;

return_type add(const int nparams, arg_type* a)
{
	if(nparams != 2) {
		/* Error! */
		r.return_val = NULL;
		r.return_size = 0;
		return r;
	}

	if(a->arg_size != sizeof(int) ||
		a->next->arg_size != sizeof(int)) {
		/* Error! */
		r.return_val = NULL;
		r.return_size = 0;
		return r;
	}

	int i = *(int *)(a->arg_val);
	int j = *(int *)(a->next->arg_val);

	ret_int = i+j;
	r.return_val = (void *)(&ret_int);
	r.return_size = sizeof(int);

	return r;
}



struct func_type {
    const char		*procedure_name;
    int 			nparams;
    fp_type 		fnpointer;
};

struct func_type func_db[50];
int func_db_count = 0; 

bool register_procedure(const char *procedure_name, const int nparams, fp_type fnpointer)
{
	func_db[func_db_count].procedure_name = procedure_name;
	func_db[func_db_count].nparams = nparams;
	func_db[func_db_count].fnpointer = fnpointer;
	func_db_count++;
}

int main() {
	register_procedure("addtwo", 2, add);

	// int a = -10, b = 20;
	// arg_type arg1;
	// arg1.arg_val = (void *)(&a);
	// arg1.arg_size = sizeof(a);
	// arg_type arg2;
	// arg2.arg_val = (void*) (&b);
	// arg2.arg_size = sizeof(b);
	// arg1.next = &arg2;

	// fp_type f = (fp_type)func_db[0].fnpointer;
	// return_type result = f(2, &arg1);
	// printf("%d\n", *(int *)(result.return_val));

	launch_server();
	
	/* should never get here, because
	launch_server(); runs forever. */

	return 0;
}

struct func_call {
		const char *procedure_name;
		int nparams;
		arg_type arg;
	};

arg_type *start = 0;
arg_type *end = 0;

/* function to add 'item' to a linked list */
void addToList(int arrayLen, void **buffer)
{
	arg_type *ptr;                 /* creating a pointer */
	ptr = malloc(sizeof(ptr)); /* allocate space for array and arrayLen */

	if (start == 0) {           /* if this is the first element added to the list */
		start = ptr;              /* structure pointed by ptr is first in the list */
		ptr->next = 0;            /* next is 0, this is also the last item in the list */
	}
	else {                      /* if there are allready items in the list */
		end->next = ptr;          /* last item in the list points to this. */
	}
	end = ptr;                  /* last item in the list is ptr */
	ptr->next = 0;              /* this is the last item */

	ptr->arg_size = arrayLen;   /* put the values in the strucure */
	// strcpy(ptr->array, buffer); /* same here */
	ptr->arg_val = (void *)malloc(arrayLen);
	memcpy(ptr->arg_val, buffer, arrayLen);
	// ptr->arg_val = buffer;
}

void launch_server()
{
	struct sockaddr_in myaddr;	/* our address */
	struct sockaddr_in remaddr;	/* remote address */
	socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
	int recvlen;			/* # bytes received */
	int fd;				/* our socket */
	int msgcnt = 0;			/* count # of messages we received */
	char buf[BUFSIZE];	/* receive buffer */


	/* create a UDP socket */
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		return;
	}

	/* bind the socket to any valid IP address and a specific port */
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (mybind(fd, &myaddr) < 0) {
		perror("bind failed");
		return;
	}

	/* get hostname */
	char hostname[HOST_NAME_MAX];
	gethostname(hostname, sizeof(hostname));

	struct func_call fn;

	arg_type arg1;

	/* now loop, receiving data and printing what we received */
	for (;;) {
		printf("%s %d\n", hostname, ntohs(myaddr.sin_port));
		recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
		if (recvlen > 0) {
			// buf[recvlen] = 0;
			// printf("received message: \"%s\" (%d bytes)\n", buf, recvlen);
			// memcpy(&arg1, buf, recvlen);
			// printf("%d\n", arg1.arg_size);
			// printf("%d\n", ((arg_type*)arg1.next)->arg_size);

			int done = 0;
			int arg_size;

			void** buffer;

			while (done < recvlen) {
				memcpy(&arg_size, buf, sizeof(arg_size));      /* read first byte from file to arrayLen
				                                         first byte saved in the file was the length of 
				                                         'apple' in bytes. */
				done += sizeof(arg_size);
				buffer = (void *)malloc(arg_size); /* allocate space for array and null character ('\0') */
				memcpy(buffer, &buf[done], arg_size);  /* read arrayLen amount of bytes from file */
				              /* add ending character to character array */
				addToList(arg_size, buffer);          /* adding arrayLen and array(located in buffer) to linked list */
				done += arg_size;                 /* done is length of array plus one byte for arrayLen */
				free(buffer);
			}

			// printf("%d\n", start->arg_size);
			// printf("%d\n", *(int*)start->arg_val);
			// printf("%d\n", (start->next)->arg_size);
			// printf("%d\n", *(int*)(start->next)->arg_val);

			fp_type f = (fp_type)func_db[0].fnpointer;
			return_type result = f(2, start);
			printf("%d\n", *(int *)(result.return_val));
		}
		else
			printf("uh oh - something went wrong!\n");
		// sprintf(buf, "ack %d", msgcnt++);
		// printf("sending response \"%s\"\n", buf);
		if (sendto(fd, "success!", strlen("success!"), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
			perror("sendto");
	}
	/* never exits */
}
