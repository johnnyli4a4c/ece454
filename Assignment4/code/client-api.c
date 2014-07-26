/* 
 * Mahesh V. Tripunitara
 * University of Waterloo
 * A dummy implementation of the functions for the remote file
 * system. This file just implements those functions as thin
 * wrappers around invocations to the local filesystem. In this
 * way, this dummy implementation helps clarify the semantics
 * of those functions. Look up the man pages for the calls
 * such as opendir() and read() that are made here.
 */
#include "ece454_fs.h"
#include "simplified_rpc/ece454rpc_types.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>

mount_point *firstAlias;
mount_point *lastAlias;
fd_server *firstFD;
fd_server *lastFD;
return_type ans;
struct fsDirent dent;

mount_point findMountPoint(const char *folderName) {
    int index;
    char *alias;
    
    if (strchr(folderName,'/') == NULL) {
        alias = (char *) malloc(strlen(folderName)+1);
        strncpy(alias, folderName, strlen(folderName));
        alias[strlen(folderName)] = '/';
        alias[strlen(folderName)+1] = '\0';
    } else {
        index = strchr(folderName,'/') - folderName + 1;
        alias = (char *) malloc(index+1);
        strncpy(alias, folderName, index);
        alias[index] = '\0';    
    }

    mount_point *current = firstAlias;

    do { 
        if (strcmp(current->alias,alias) == 0) {
            return *current;
        }
        current = current->next;
    } while (current != NULL);
}

fd_server lookupFD(const int fd) {
    fd_server *current = firstFD;
    do { 
        if (current->fd == fd) {
            return *current;
        }
        current = current->next;
    } while (current != NULL);   
}

void removeFD(const int fd) {
    if (firstFD == NULL) {
        return;
    }

    fd_server *current = firstFD;
    
    if (current->fd ==fd) {
        if (lastFD == firstFD) {
            lastFD = current->next;
        }
        firstFD = current->next;
        free(current);
        return;
    }
    while (current->next != NULL) { 
        if (current->next->fd == fd) {
            fd_server *temp = current->next;
            current->next = current->next->next;
            free(temp);
            return;
        }
        current = current->next;
    }
}

int fsMount(const char *srvIpOrDomName, const unsigned int srvPort, const char *localFolderName) {
    char *alias = (char *) malloc(sizeof(localFolderName)+2);

    fsUnmount(localFolderName);
    errno = 0;

    ans = make_remote_call(srvIpOrDomName, srvPort, "srvMount", 0);

    strncpy(alias,localFolderName,sizeof(localFolderName));
    if (localFolderName[strlen(localFolderName)-1] != '/') {
        alias[strlen(localFolderName)] = '/';
        alias[strlen(localFolderName)+1] = '\0';
    }

    if (ans.return_size > 0) {
        mount_point *temp;
        temp = (mount_point *) malloc(sizeof(mount_point));
        temp->alias = alias;
        temp->srvIpOrDomName = srvIpOrDomName;
        temp->srvPort = srvPort;
        if (firstAlias == NULL) {
            firstAlias = temp;
            lastAlias = firstAlias;
        } else {
            lastAlias->next = temp;
            lastAlias = temp;
        }
        int return_val = *(int *)(ans.return_val);
        return return_val;
    }
    return -1;
}

int fsUnmount(const char *localFolderName) {
    if (firstAlias == NULL) {
        errno = ENOENT;
        return -1;
    }

    char *alias = (char *) malloc(sizeof(localFolderName)+2);
    strncpy(alias,localFolderName,sizeof(localFolderName));
    if (localFolderName[strlen(localFolderName)-1] != '/') {
        alias[strlen(localFolderName)] = '/';
        alias[strlen(localFolderName)+1] = '\0';
    }

    mount_point *current = firstAlias;
    
    if (strcmp(current->alias,alias) == 0) {
        if (lastAlias == firstAlias) {
            lastAlias = current->next;
        }
        firstAlias = current->next;
        free(current);
        return 0;
    }
    while (current->next != NULL) { 
        if (strcmp(current->next->alias,alias) == 0) {
            mount_point *temp = current->next;
            current->next = current->next->next;
            free(temp);
            return 0;
        }
        current = current->next;
    }

    errno = ENOENT;
    return -1;
}

FSDIR* fsOpenDir(const char *folderName) {
    mount_point mp = findMountPoint(folderName);

    char *relativePath = strchr(folderName,'/');
    if (relativePath == NULL) {
        relativePath = "/";
    }

    ans = make_remote_call(mp.srvIpOrDomName, mp.srvPort, "srvOpenDir", 1, 
        strlen(relativePath)+1, (void *)(relativePath));

    if (ans.return_size > 0) {
        FSDIR* return_val = (FSDIR *) malloc(sizeof(FSDIR));
        memcpy(&(return_val->dirPtr), ans.return_val, ans.return_size);
        return_val->srvIpOrDomName = mp.srvIpOrDomName;
        return_val->srvPort = mp.srvPort;
        free(ans.return_val);
        return return_val;
    }
    return NULL;
}

int fsCloseDir(FSDIR *folder) {
    ans = make_remote_call(folder->srvIpOrDomName, folder->srvPort, "srvCloseDir", 1, 
        sizeof(FSDIR *), (void *)(folder));

    if (ans.return_size > 0) {
        int result = *(int*)(ans.return_val);
        if (result == 0) {
            free(folder);
        }
        return result;
    }

    return -1;
}

struct fsDirent *fsReadDir(FSDIR *folder) {
    const int initErrno = errno;

    if (folder->dirPtr == NULL) {
        return NULL;
    }

    ans = make_remote_call(folder->srvIpOrDomName, folder->srvPort, "srvReadDir", 1, 
        sizeof(FSDIR *), (void *)(&(folder->dirPtr)));

    if (ans.return_size > 0) {
        if (((struct dirent*)ans.return_val)->d_type == DT_DIR) {
            dent.entType = 1;
        }
        else if (((struct dirent*)ans.return_val)->d_type == DT_REG) {
            dent.entType = 0;
        }
        else {
            dent.entType = -1;
        }

        memcpy(&(dent.entName), &(((struct dirent*)ans.return_val)->d_name), 256);
        free(ans.return_val);
        return &dent;
    } else {
    	if(errno == initErrno) errno = 0;
    	return NULL;
    }
}

int fsOpen(const char *fname, int mode) {
    mount_point mp = findMountPoint(fname);
    int perm = S_IRWXU;
    int flags = -1;

    char *relativePath = strchr(fname,'/');
    if (relativePath == NULL) {
        relativePath = "/";
    }

    if(mode == 0) {
        flags = O_RDONLY;
    }
    else if(mode == 1) {
        flags = O_WRONLY | O_CREAT;
    }

    ans = make_remote_call(mp.srvIpOrDomName, mp.srvPort, "srvOpen", 3, 
        strlen(relativePath)+1, (void *)(relativePath),
        sizeof(int), (void *)(&flags),
        sizeof(int), (void *)(&perm));

    if (ans.return_size > 0) {
        int fd = *(int *)(ans.return_val);
        fd_server *temp;
        temp = (fd_server *) malloc(sizeof(fd_server));
        temp->fd = fd;
        temp->srvIpOrDomName = mp.srvIpOrDomName;
        temp->srvPort = mp.srvPort;
        if (firstFD == NULL) {
            firstFD = temp;
            lastFD = firstFD;
        } else {
            lastFD->next = temp;
            lastFD = temp;
        }
        return fd;
    }

    return -1;
}

int fsClose(int fd) {
    fd_server server = lookupFD(fd);

    ans = make_remote_call(server.srvIpOrDomName, server.srvPort, "srvClose", 1, 
        sizeof(int), (void *)(&fd));

    if (ans.return_size > 0) {
        int result = *(int*)(ans.return_val);
        if (result == 0) {
            removeFD(fd);
        }
        return result;
    }

    return -1;
}

int fsRead(int fd, void *buf, const unsigned int count) {
    fd_server server = lookupFD(fd);

    ans = make_remote_call(server.srvIpOrDomName, server.srvPort, "srvRead", 2, 
        sizeof(int), (void *)(&fd),
        sizeof(int), (void *)(&count));

    if (ans.return_size > 0) {
        int *numBytes = (int *)malloc(sizeof(int));
        memcpy(numBytes, ans.return_val, sizeof(int));
        memcpy(buf, ans.return_val+sizeof(int), ans.return_size-sizeof(int));
        return *numBytes;
    }

    return -1;
}

int fsWrite(int fd, const void *buf, const unsigned int count) {
    fd_server server = lookupFD(fd);

    ans = make_remote_call(server.srvIpOrDomName, server.srvPort, "srvWrite", 3, 
        sizeof(int), (void *)(&fd),
        count, (void *)buf,
        sizeof(int), (void *)(&count));

    int numBytes = *(int *)(ans.return_val);
    return numBytes;
}

int fsRemove(const char *name) {
    mount_point mp = findMountPoint(name);

    char *relativePath = strchr(name,'/');
    if (relativePath == NULL) {
        relativePath = "/";
    }

    ans = make_remote_call(mp.srvIpOrDomName, mp.srvPort, "srvRemove", 1, 
        strlen(relativePath)+1, (void *)(relativePath));

    int result = *(int *)(ans.return_val);
    return result;
}

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
    FSDIR *fd = fsOpenDir(dirname);
    if(fd == NULL) {
    perror("fsOpenDir"); exit(1);
    }

    struct fsDirent *fdent = NULL;
    for(fdent = fsReadDir(fd); fdent != NULL; fdent = fsReadDir(fd)) {
    printf("\t %s, %d\n", fdent->entName, (int)(fdent->entType));
    }

    if(errno != 0) {
    perror("fsReadDir");
    }

    printf("fsCloseDir(): %d\n", fsCloseDir(fd));
    for(fdent = fsReadDir(fd); fdent != NULL; fdent = fsReadDir(fd)) {
    printf("\t %s, %d\n", fdent->entName, (int)(fdent->entType));
    }

    int ff = open("/dev/urandom", 0);
    if(ff < 0) {
    perror("open(/dev/urandom)"); exit(1);
    }
    else printf("open(): %d\n", ff);

    char fname[256];
    sprintf(fname, "%s/", dirname);
    if(read(ff, (void *)(fname+strlen(dirname)+1), 10) < 0) {
    perror("read(/dev/urandom)"); exit(1);
    }

    int i;
    for(i = 0; i < 10; i++) {
    //printf("%d\n", ((unsigned char)(fname[i]))%26);
    fname[i+strlen(dirname)+1] = ((unsigned char)(fname[i+strlen(dirname)+1]))%26 + 'a';
    }
    fname[10+strlen(dirname)+1] = (char)0;
    printf("Filename to write: %s\n", (char *)fname);

    char buf[256];
    if(read(ff, (void *)buf, 256) < 0) {
    perror("read(2)"); exit(1);
    }

    printBuf(buf, 256);

    printf("close(): %d\n", close(ff));

    ff = fsOpen(fname, 1);
    if(ff < 0) {
    perror("fsOpen(write)"); exit(1);
    }

    if(fsWrite(ff, buf, 256) < 256) {
    fprintf(stderr, "fsWrite() wrote fewer than 256\n");
    }

    if(fsClose(ff) < 0) {
    perror("fsClose"); exit(1);
    }

    char readbuf[256];
    if((ff = fsOpen(fname, 0)) < 0) {
    perror("fsOpen(read)"); exit(1);
    }

    int readcount = -1;

    if((readcount = fsRead(ff, readbuf, 256)) < 256) {
    fprintf(stderr, "fsRead() read fewer than 256\n");
    }

    if(memcmp(readbuf, buf, readcount)) {
    fprintf(stderr, "return buf from fsRead() differs from data written!\n");
    }
    else {
    printf("fsread(): return buf identical to data written upto %d bytes.\n", readcount);
    }

    if(fsClose(ff) < 0) {
    perror("fsClose"); exit(1);
    }

    printf("fsRemove(%s): %d\n", fname, fsRemove(fname));

    if(fsUnmount(dirname) < 0) {
    perror("fsUnmount"); exit(1);
    }

    return 0;

}
