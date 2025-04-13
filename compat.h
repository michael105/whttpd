

typedef unsigned int uint;
typedef unsigned int uint32_t;
typedef unsigned long ulong;
typedef unsigned char uchar;
typedef unsigned long POINTER;
#define PAGESIZE 4096

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/prctl.h>
#include <arpa/inet.h>
#include <sys/inotify.h>
#include <dirent.h>
#include <time.h>


extern char** environ;


#define _tolower(a) ((a)|0x20)
int tolower(int c){ return( _tolower(c) ); }


int nwrite(int fd, char *buf, int len){
   char *b = buf;
   char *e = buf+len;
   int ret;

   do {
		ret = write( fd, buf, len );

      if ( ret <= 0 ){
         if ( ret == -EAGAIN || ret == -ENOMEM || ret == -EINTR )
            continue;
         return( b-buf ? b-buf : ret ); // rw bytes (if), or error code / -1
      }

      b+=ret;

   } while ( b < e );

   return( len );
}


int ml_getcwd( char *buf, uint size ){
	getcwd(buf,size);
	return ( strlen(buf) );
}


