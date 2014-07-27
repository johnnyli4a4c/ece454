#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ece454_fs.h"

void printBuf(char *buf, int size) {
    /* Should match the output from od -x */
    int i;
    for(i = 0; i < size; ) {
	if(i%16 == 0) {
	    printf("%08o ", i);
	}

	int j;
	for(j = 0; j < 16;) {
	    int k;
	    for(k = 0; k < 2; k++) {
		if(i+j+(1-k) < size) {
		    printf("%02x", (unsigned char)(buf[i+j+(1-k)]));
		}
	    }

	    printf(" ");
	    j += k;
	}

	printf("\n");
	i += j;
    }
}

int main(int argc, char *argv[]) {
    if(argc < 4) {
	fprintf(stderr, "usage: %s <srv-ip/name> <srv-port> <local dir name>\n", argv[0]);
	exit(1);
    }

    char *dirname = argv[3];
    printf("fsMount(): %d\n", fsMount(argv[1], atoi(argv[2]), dirname));

    int clients = 2;
    int ff[clients];
    char buf1[256];
    char buf2[256];
    char fname[256];

    sprintf(fname, "%s/testFile.txt", dirname);
    // fsRemove(fname);

    int i;
    for (i = 0; i < clients; i++) {    
        if((ff[i] = fsOpen(fname, 1)) < 0) {
    	perror("fsOpen(write)"); exit(1);
        }
        printf("client %d opens %d\n", i, ff[i]);
    }

    for (i = 0; i < 2000; i++) {
        sprintf(buf1, "client 1 writes %d\n", i);
        fsWrite(ff[0], buf1, strlen(buf1));
        if (i == 500) {
            // sprintf(buf2, "client 2 writes %d\n", i);
            // fsWrite(ff[1], buf2, strlen(buf2));
            // fsRemove(fname);
        }
    }

    for (i = 0; i < clients; i++) {
        if(fsClose(ff[i]) < 0) {
    	perror("fsClose"); exit(1);
        }
    }

    if(fsUnmount(dirname) < 0) {
	perror("fsUnmount"); exit(1);
    }

    return 0;
}
