#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simplified_rpc/ece454rpc_types.h"
#include "ece454_fs.h"

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
	char *folder = (char *)malloc(a->arg_size);
	DIR *ret_dir;

    memcpy(folder, a->arg_val, a->arg_size);
    char *fullPath = (char *) calloc(1, strlen(dirname)+strlen(folder));
    sprintf(fullPath, "%s%s", dirname, folder);
    ret_dir = opendir(fullPath);
	if (ret_dir == NULL) {
		r.return_val = NULL;
		r.return_size = 0;
	} else {
        r.return_val = (void *)malloc(sizeof(ret_dir));
        memcpy(r.return_val, &ret_dir, sizeof(ret_dir));
	    r.return_size = sizeof(ret_dir);
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
    r.return_val = (void *)(result);
    r.return_size = sizeof(int);
    return r;
}

return_type srvReadDir(const int nparams, arg_type* a)
{
    DIR *fd = (DIR *)malloc(a->arg_size);

    memcpy(&fd, a->arg_val, a->arg_size);
	struct dirent *d = readdir(fd);
    if (d == NULL) {
        r.return_val = NULL;
        r.return_size = 0;
    } else {
        r.return_val = (void *)calloc(1,sizeof(struct dirent));
        memcpy(r.return_val, d, sizeof(struct dirent));
        r.return_size = sizeof(struct dirent);
    }
    return r;
}

return_type srvOpen(const int nparams, arg_type* a)
{
    char *fname = (char*)malloc(a->arg_size);
    int flags; 
    int mode;

    memcpy(fname, a->arg_val, a->arg_size);
    flags = *(int *)(a->next->arg_val);
    mode = *(int *)(a->next->next->arg_val);

    int *fd = (int *)malloc(sizeof(int));
    char *fullPath = (char *) calloc(1, strlen(dirname)+strlen(fname));
    sprintf(fullPath, "%s%s", dirname, fname);
    *fd = open(fullPath, flags, mode);
    if (*fd == -1) {
        r.return_val = NULL;
        r.return_size = 0;
    } else {    
        r.return_val = (void *)(fd);
        r.return_size = sizeof(int);
    }

    return r;
}

return_type srvClose(const int nparams, arg_type* a)
{
    int fd = *(int *)(a->arg_val);
    int *result = (int *) malloc(sizeof(int));

    *result = close(fd);
    r.return_val = (void *)(result);
    r.return_size = sizeof(int);
    return r;
}

return_type srvRead(const int nparams, arg_type* a)
{
    int fd;
    void *buf;
    unsigned int count;
    int *numBytes = (int *)malloc(sizeof(int));

    fd = *(int *)(a->arg_val);
    count = *(int *)(a->next->arg_val);
    buf = (void *) malloc(count);

    *numBytes = read(fd, buf, (size_t)count);

    r.return_val = (void *)calloc(1, sizeof(int) + *numBytes);
    memcpy(r.return_val, numBytes, sizeof(int));
    memcpy(r.return_val+sizeof(int), buf, *numBytes);
    r.return_size = sizeof(int) + *numBytes;

    return r;
}

return_type srvRemove(const int nparams, arg_type* a)
{
    char *fname = (char*)malloc(a->arg_size);

    memcpy(fname, a->arg_val, a->arg_size);

    int *result = (int *)malloc(sizeof(int));
    char *fullPath = (char *) calloc(1, strlen(dirname)+strlen(fname));
    sprintf(fullPath, "%s%s", dirname, fname);
    *result = remove(fullPath);
    
    r.return_val = (void *)(result);
    r.return_size = sizeof(int);

    return r;
}

return_type srvWrite(const int nparams, arg_type* a)
{
    int fd;
    void *buf = (void *) malloc(a->next->arg_size);
    unsigned int count;
    int *numBytes = (int *)malloc(sizeof(int));

    fd = *(int *)(a->arg_val);
    memcpy(buf, a->next->arg_val, a->next->arg_size);
    count = *(int *)(a->next->next->arg_val);

    *numBytes = write(fd, buf, (size_t)count);

    r.return_val = (void *)(numBytes);
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
    register_procedure("srvRemove", 3, srvRemove);

    launch_server();
    return 0;
}
