#include "ece454_fs.h"
#include <errno.h>
#include "simplified_rpc/ece454rpc_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *dirname = NULL;
return_type r;

return_type srvMount(const int nparams, arg_type* a)
{
    int *ret_int = (int *)malloc(sizeof(int));
    
    *ret_int = 0;
    r.return_val = (void *)(ret_int);
    r.return_size = sizeof(int);

    return r;
}

return_type srvOpenDir(const int nparams, arg_type* a)
{
    int *errNum = (int *)malloc(sizeof(int));
	char *folder = (char *)malloc(a->arg_size);
	DIR *ret_dir;

    memcpy(folder, a->arg_val, a->arg_size);
    char *fullPath = (char *) calloc(1, strlen(dirname)+strlen(folder)+1);
    sprintf(fullPath, "%s%s%c", dirname, folder, '\0');
    
    ret_dir = opendir(fullPath);

	if (ret_dir == NULL) {
        *errNum = errno;
        r.return_val = (void *)(errNum);
		r.return_size = sizeof(int);
	} else {
        r.return_val = (void *)malloc(sizeof(int)+sizeof(ret_dir));
        *errNum = 0;
        memcpy(r.return_val, errNum, sizeof(int));
        memcpy(r.return_val+sizeof(int), &ret_dir, sizeof(ret_dir));
	    r.return_size = sizeof(int)+sizeof(ret_dir);
	}

    free(folder);
    return r;
}

return_type srvCloseDir(const int nparams, arg_type* a)
{
    DIR *fd = (DIR *) malloc(a->arg_size);
    int *result = (int *) malloc(sizeof(int));

    memcpy(&fd, a->arg_val, a->arg_size);

    *result = closedir(fd);

    if (*result == -1) {
        *result = errno;
    }
    r.return_val = (void *)(result);
    r.return_size = sizeof(int);

    return r;
}

return_type srvReadDir(const int nparams, arg_type* a)
{
    const int initErrno = errno;
    int *errNum = (int *)malloc(sizeof(int));
    DIR *fd = (DIR *)malloc(a->arg_size);

    memcpy(&fd, a->arg_val, a->arg_size);

	struct dirent *d = readdir(fd);

    if (d == NULL) {
        if (errno == initErrno) *errNum = 0;
        r.return_val = (void *)(errNum);
        r.return_size = sizeof(int);
    } else {
        r.return_val = (void *)malloc(sizeof(int)+sizeof(struct dirent));
        *errNum = 0;
        memcpy(r.return_val, errNum, sizeof(int));
        memcpy(r.return_val+sizeof(int), d, sizeof(struct dirent));
        r.return_size = sizeof(int)+sizeof(struct dirent);
    }

    return r;
}

return_type srvOpen(const int nparams, arg_type* a)
{
    int *errNum = (int *)malloc(sizeof(int));
    int *fd = (int *)malloc(sizeof(int));
    char *fname = (char*)malloc(a->arg_size);
    int flags; 
    int mode;

    memcpy(fname, a->arg_val, a->arg_size);
    flags = *(int *)(a->next->arg_val);
    mode = *(int *)(a->next->next->arg_val);

    char *fullPath = (char *) calloc(1, strlen(dirname)+strlen(fname)+1);
    sprintf(fullPath, "%s%s%c", dirname, fname, '\0');

    *fd = open(fullPath, flags, mode);

    if (*fd == -1) {
        *errNum = errno;
        r.return_val = (void *)(errNum);
        r.return_size = sizeof(int);
    } else {
        r.return_val = (void *)malloc(sizeof(int)+sizeof(int));
        *errNum = 0;
        memcpy(r.return_val, errNum, sizeof(int));
        memcpy(r.return_val+sizeof(int), fd, sizeof(int));
        r.return_size = sizeof(int)+sizeof(int);
    }

    return r;
}

return_type srvClose(const int nparams, arg_type* a)
{
    int fd = *(int *)(a->arg_val);
    int *result = (int *) malloc(sizeof(int));

    *result = close(fd);

    if (*result == -1) {
        *result = errno;
    }
    r.return_val = (void *)(result);
    r.return_size = sizeof(int);

    return r;
}

return_type srvRead(const int nparams, arg_type* a)
{
    int *errNum = (int *)malloc(sizeof(int));
    int fd;
    void *buf;
    unsigned int count;
    int *numBytes = (int *)malloc(sizeof(int));

    fd = *(int *)(a->arg_val);
    count = *(int *)(a->next->arg_val);
    buf = (void *) malloc(count);

    *numBytes = read(fd, buf, (size_t)count);

    if (*numBytes == -1) {
        *errNum = errno;
        r.return_val = (void *)(errNum);
        r.return_size = sizeof(int);
    } else {
        r.return_val = (void *)calloc(1, sizeof(int) + sizeof(int) + *numBytes);
        *errNum = 0;
        memcpy(r.return_val, errNum, sizeof(int));
        memcpy(r.return_val+sizeof(int), numBytes, sizeof(int));
        memcpy(r.return_val+sizeof(int)*2, buf, *numBytes);
        r.return_size = sizeof(int) + sizeof(int) + *numBytes;
    }

    return r;
}

return_type srvWrite(const int nparams, arg_type* a)
{
    int *errNum = (int *)malloc(sizeof(int));
    int fd;
    void *buf = (void *) malloc(a->next->arg_size);
    unsigned int count;
    int *numBytes = (int *)malloc(sizeof(int));

    fd = *(int *)(a->arg_val);
    memcpy(buf, a->next->arg_val, a->next->arg_size);
    count = *(int *)(a->next->next->arg_val);

    *numBytes = write(fd, buf, (size_t)count);

    if (*numBytes == -1) {
        *errNum = errno;
        r.return_val = (void *)(errNum);
        r.return_size = sizeof(int);
    } else {
        r.return_val = (void *)malloc(sizeof(int)+sizeof(int));
        *errNum = 0;
        memcpy(r.return_val, errNum, sizeof(int));
        memcpy(r.return_val+sizeof(int), numBytes, sizeof(int));
        r.return_size = sizeof(int)+sizeof(int);
    }

    return r;
}

return_type srvRemove(const int nparams, arg_type* a)
{
    char *fname = (char*)malloc(a->arg_size);
    int *result = (int *)malloc(sizeof(int));

    memcpy(fname, a->arg_val, a->arg_size);
    
    char *fullPath = (char *) calloc(1, strlen(dirname)+strlen(fname)+1);
    sprintf(fullPath, "%s%s%c", dirname, fname, '\0');

    *result = remove(fullPath);

    if (*result == -1) {
        *result = errno;
    }
    r.return_val = (void *)(result);
    r.return_size = sizeof(int);

    return r;
}

int main(int argc, char *argv[]) {
	if (argc > 1) {
        dirname = argv[1];
        struct stat sbuf;
        if (stat(dirname, &sbuf) != 0) {
            perror("argument 1");
            exit(1);
        }        
    }
    else {
    	fprintf(stderr, "usage: %s <folder>\n", argv[0]);
		exit(1);
    }

    register_procedure("srvMount", 0, srvMount);
    register_procedure("srvOpenDir", 1, srvOpenDir);
    register_procedure("srvCloseDir", 1, srvCloseDir);
    register_procedure("srvReadDir", 1, srvReadDir);
    register_procedure("srvOpen", 3, srvOpen);
    register_procedure("srvClose", 1, srvClose);
    register_procedure("srvRead", 2, srvRead);
    register_procedure("srvWrite", 3, srvWrite);
    register_procedure("srvRemove", 1, srvRemove);

    launch_server();
    fflush(stdout);
    return 0;
}
