#include "ece454_fs.h"
#include <errno.h>
#include "simplified_rpc/ece454rpc_types.h"
#include <stdlib.h>
#include <string.h>

mount_point *firstAlias;
mount_point *lastAlias;
fd_server *firstFD;
fd_server *lastFD;
return_type ans;
struct fsDirent dent;

mount_point *findMountPoint(const char *folderName) 
{
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
            return current;
        }
        current = current->next;
    } while (current != NULL);

    return NULL;
}

fd_server *lookupFD(const int fd) 
{
    fd_server *current = firstFD;
    do { 
        if (current->fd == fd) {
            return current;
        }
        current = current->next;
    } while (current != NULL);

    return NULL;
}

void removeFD(const int fd) 
{
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

int fsMount(const char *srvIpOrDomName, const unsigned int srvPort, const char *localFolderName) 
{
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

int fsUnmount(const char *localFolderName) 
{
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

FSDIR* fsOpenDir(const char *folderName) 
{
    mount_point *mp = findMountPoint(folderName);
    if (mp == NULL) {
        errno = ENOENT;
        return NULL;
    }

    char *relativePath = strchr(folderName,'/');
    if (relativePath == NULL) {
        relativePath = "/";
    }

    ans = make_remote_call(mp->srvIpOrDomName, mp->srvPort, "srvOpenDir", 1, 
        strlen(relativePath)+1, (void *)(relativePath));

    if (ans.return_size > 0) {
        int errNum = *(int *)(ans.return_val);
        if (errNum == 0) {
            FSDIR* return_val = (FSDIR *) malloc(sizeof(FSDIR));
            memcpy(&(return_val->dirPtr), ans.return_val+sizeof(int), ans.return_size);
            return_val->srvIpOrDomName = mp->srvIpOrDomName;
            return_val->srvPort = mp->srvPort;
            free(ans.return_val);
            return return_val;
        } else {
            errno = errNum;
        }
    }

    return NULL;
}

int fsCloseDir(FSDIR *folder) 
{
    ans = make_remote_call(folder->srvIpOrDomName, folder->srvPort, "srvCloseDir", 1, 
        sizeof(DIR *), (void *)(&(folder->dirPtr)));

    if (ans.return_size > 0) {
        int result = *(int*)(ans.return_val);
        if (result == 0) {
            free(folder);
            return result;
        } else {
            errno = result;
        }
    }

    return -1;
}

struct fsDirent *fsReadDir(FSDIR *folder) 
{
    if (folder->dirPtr == NULL) {
        return NULL;
    }

    ans = make_remote_call(folder->srvIpOrDomName, folder->srvPort, "srvReadDir", 1, 
        sizeof(DIR *), (void *)(&(folder->dirPtr)));

    struct dirent* temp = (struct dirent *) malloc(sizeof(struct dirent));

    if (ans.return_size > 0) {
        int errNum = *(int *)(ans.return_val);
        if ((errNum == 0) && (ans.return_size > sizeof(int))) {
            memcpy(temp, ans.return_val+sizeof(int), sizeof(struct dirent));
            if (temp->d_type == DT_DIR) {
                dent.entType = 1;
            }
            else if (temp->d_type == DT_REG) {
                dent.entType = 0;
            }
            else {
                dent.entType = -1;
            }

            memcpy(&(dent.entName), &(temp->d_name), 256);
            free(ans.return_val);
            return &dent;
        } else {
            errno = errNum;
        }
    }

    return NULL;
}

int fsOpen(const char *fname, int mode) 
{
    mount_point *mp = findMountPoint(fname);
    if (mp == NULL) {
        errno = ENOENT;
        return -1;
    }

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

    ans = make_remote_call(mp->srvIpOrDomName, mp->srvPort, "srvOpen", 3, 
        strlen(relativePath)+1, (void *)(relativePath),
        sizeof(int), (void *)(&flags),
        sizeof(int), (void *)(&perm));

    if (ans.return_size > 0) {
        int errNum = *(int *)(ans.return_val);
        if (errNum == 0) {
            int *fd = (int *) malloc(sizeof(int));
            memcpy(fd, ans.return_val+sizeof(int), sizeof(int));
            fd_server *temp;
            temp = (fd_server *) malloc(sizeof(fd_server));
            temp->fd = *fd;
            temp->srvIpOrDomName = mp->srvIpOrDomName;
            temp->srvPort = mp->srvPort;
            if (firstFD == NULL) {
                firstFD = temp;
                lastFD = firstFD;
            } else {
                lastFD->next = temp;
                lastFD = temp;
            }
            return *fd;
        } else {
            errno = errNum;
        }
    }

    return -1;
}

int fsClose(int fd) 
{
    fd_server *server = lookupFD(fd);
    if (server == NULL) {
        errno = EBADF;
        return -1;
    }

    ans = make_remote_call(server->srvIpOrDomName, server->srvPort, "srvClose", 1, 
        sizeof(int), (void *)(&fd));

    if (ans.return_size > 0) {
        int result = *(int*)(ans.return_val);
        if (result == 0) {
            removeFD(fd);
            return result;
        } else {
            errno = result;
        }
    }

    return -1;
}

int fsRead(int fd, void *buf, const unsigned int count) 
{
    fd_server *server = lookupFD(fd);
    if (server == NULL) {
        errno = EBADF;
        return -1;
    }

    ans = make_remote_call(server->srvIpOrDomName, server->srvPort, "srvRead", 2, 
        sizeof(int), (void *)(&fd),
        sizeof(int), (void *)(&count));

    if (ans.return_size > 0) {
        int errNum = *(int*)(ans.return_val);
        if (errNum == 0) {
            memset(buf, 0, count);
            int *numBytes = (int *)malloc(sizeof(int));
            memcpy(numBytes, ans.return_val+sizeof(int), sizeof(int));
            if (*numBytes > 0) {
                memcpy(buf, ans.return_val+sizeof(int)*2, *numBytes);
            }
            return *numBytes;
        } else {
            errno = errNum;
        }
    }

    return -1;
}

int fsWrite(int fd, const void *buf, const unsigned int count) 
{
    fd_server *server = lookupFD(fd);
    if (server == NULL) {
        errno = EBADF;
        return -1;
    }

    ans = make_remote_call(server->srvIpOrDomName, server->srvPort, "srvWrite", 3, 
        sizeof(int), (void *)(&fd),
        count, (void *)buf,
        sizeof(int), (void *)(&count));

    if (ans.return_size > 0) {
        int errNum = *(int*)(ans.return_val);
        if (errNum == 0) {
            int *numBytes = (int *)malloc(sizeof(int));
            memcpy(numBytes, ans.return_val+sizeof(int), sizeof(int));
            return *numBytes;
        } else {
            errno = errNum;
        }
    }

    return -1;
}

int fsRemove(const char *name) 
{
    mount_point *mp = findMountPoint(name);
    if (mp == NULL) {
        errno = ENOENT;
        return -1;
    }

    char *relativePath = strchr(name,'/');
    if (relativePath == NULL) {
        relativePath = "/";
    }

    ans = make_remote_call(mp->srvIpOrDomName, mp->srvPort, "srvRemove", 1, 
        strlen(relativePath)+1, (void *)(relativePath));

    if (ans.return_size > 0) {
        int result = *(int*)(ans.return_val);
        if (result == 0) {
            return result;
        } else {
            errno = result;
        }
    }

    return -1;
}
