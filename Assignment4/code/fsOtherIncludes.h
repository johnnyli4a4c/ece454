/* 
 * Mahesh V. Tripunitara
 * University of Waterloo
 * You specify what goes in this file. I just have a "dummy"
 * specification of the FSDIR type.
 */

#ifndef _ECE_FS_OTHER_INCLUDES_
#define _ECE_FS_OTHER_INCLUDES_
#include <sys/types.h>
#include <dirent.h>

typedef struct dirStruct {
	DIR *dirPtr;
	const char *srvIpOrDomName;
	unsigned int srvPort;
} FSDIR;

typedef struct mount {
	const char *alias;
	const char *srvIpOrDomName;
	unsigned int srvPort;
	struct mount *next;
} mount_point;

typedef struct fdServer {
	int fd;
	const char *srvIpOrDomName;
	unsigned int srvPort;
	struct fdServer *next;
} fd_server;

#endif
