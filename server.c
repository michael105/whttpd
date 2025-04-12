
#undef MODULE
// verbose prefix and processname
#define MODULE whttpd_server

#include "mimetypes.h"


int sockfd = 0;

// bbuf unused.
#define BBUFSIZE BUFSIZE

char *bbuffer;
char *bpos;
char *bend;
int bfd;
int berr;


void setbfd(int fd){
	bfd = fd;
}

void setbbuf(char *buf){
	bbuffer = bpos = buf;
}

int bflush(){
	if ( !bbuffer ) return ( -EINVAL );

	if ( !bfd ) bfd=1; // stdout

	int ret = nwrite(bfd,bbuffer,(bpos-bbuffer));
	if ( ret>0 )
		bpos = bbuffer;
	return(ret);
}


// write to buf, flush, if buffer full
// doesn't append a 0
// return negative errno on errors.
//   else the number of bytes written.
int bwrite( const char* str ){
	//int ret = -(bpos-bbuf);
	const char *p = str;
	while ( *str ){
		if ( bpos>= bend ){
			int r;
			if ( (r=bflush()) < 0 ){
				berr = r;
				return(r);
			}
		}
		*bpos++ = *p++;
	}
	return(p-str);
}


// write strings
// a null pointer aborts writing of the arguments,
// is also used as sentinel.
int _bprints( const char* str, ... ){
#define bprints(...) (__VA_ARGS__,0)
		int ret = 0;
		const char *msg;
		va_list args;
		va_start(args,str);

		while( (msg=va_arg(args,char*) ) ){
			int i = bwrite(msg);
			if ( i<0 )
				return(i);
			ret += i;
		}

		va_end(args);
		return(ret);
}



#define SCRIPTMAXSIZE 512

static int sprint_html_script(char* buf){
	return
		snprints( buf, SCRIPTMAXSIZE, 
R"js( 
<script>
let isUnloading = false;
window.addEventListener('beforeunload', () => {
  isUnloading = true;
});
async function checkForUpdate() {
	try {
		const response = await fetch('http://localhost:)js",
			FI(GET(P,int)),
R"js(', { timeout: 5000 }); // timeout is without effect
	} catch (error) {
		if ( !isUnloading ){
   	 	location.reload();
		}
	} finally {
		setTimeout(checkForUpdate, 500); // fallback
	}
}
checkForUpdate();
</script>

)js" );

}


void sighandler(int sig){
	verbose(0,"Quit: ",FI(sig));
	if ( sockfd ) close(sockfd);
	if ( sig != SIGQUIT && parentpid ) 
		kill(parentpid,SIGUSR2);
	exit(0);
}


# define http_header(_buf,_bufsize,_status,_phrase,...) \
	__http_header(_buf, _bufsize,_status,_phrase,__VA_OPT__(__VA_ARGS__,) 0,0,0,0 )
		#define __http_header(_a,_b,_c,_d,_e,_f,_g,...) \
			_http_header(_a,_b,_c,_d,_e,_f,_g)

static int _http_header( char *buf, uint bufsize, int status, const char *phrase, \
					  int mimetype, ulong len, time_t mod ){

	char *pos = buf;

	pos += snprints( pos,bufsize, "HTTP/1.0 ", FI(status), " ",phrase, "\r\n"
			"Server: minihttpd 0.1\r\n" 
			"Connection: close\r\n"
			"Public: GET\r\n"
			"Content-Encoding: none\r\n"
			"Access-Control-Allow-Origin: *\r\n"
			"Access-Control-Allow-Methods: GET\r\n"
			"Access-Control-Allow-Headers: *\r\n",
			(OPT(C)?"Pragma: no-cache\r\n":"")

			);

#ifdef with_strftime
	time_t now = time(0);
	struct tm tim;
   pos += strftime(pos, bufsize - (pos-buf), "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", localtime_r(&now,&tim) );

	// more or less useless. prefer etag
	if (mod)
   	pos += strftime(pos, bufsize - (pos-buf), "Last-Modified: %a, %d %b %Y %H:%M:%S GMT\r\n", localtime_r(&mod,&tim) );
#endif

	if ( mod )
		pos += snprints( pos, bufsize - ( pos-buf ), "ETag: ",FIxAP(mod), "\r\n" );
 
	/*
	if ( mod ) // deviating, totally. but works. and should work.  send the modification
				  // time of files as seconds since 1970, in hexadecimal. Will be sent back.
				  // local usage only. But according to the spec, the date shouldn't 
				  // be more than one year ago, so - it's already screwed by sending
				  // the file modification time. Should use Etag here instead.
			pos += snprints( pos, bufsize - ( pos-buf ), "Last-Modified: ",FIHEX(mod), "\r\n" );
			*/

	if ( mimetype )
		pos += snprints( pos, bufsize - (pos-buf), "Content-Type: ",MIMESTR(mimetype),"\r\n" );
	
	if ( len )
		pos += snprints( pos, bufsize - (pos-buf), "Content-Length: ",FL(len),"\r\n" );
	

	pos += snprints( pos, bufsize - (pos-buf) , "\r\n" );

	//nwrite( fd, buf, pos-buf );
	return ( pos-buf );
}


int htmlhead( char *buf, int maxlen, int status, const char* title ){
	return 
		snprints( buf, maxlen, 
R"(<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
	<head>
		<meta http-equiv="Content-type" "content="text/html;charset=UTF-8">
			<title> )",FI(status)," ",title,"</title>\n\
	</head>\n<body>\n" );
}



static void send_error(int fd, int e, char *msg){
	verbose(1,FI(e),": ",msg);

	char buf[BUFSIZE];
	char *pos = buf;

	pos += http_header(pos,BUFSIZE,e,msg, MIMETYPE(html) );
	pos += htmlhead(pos,BUFSIZE-(pos-buf),e,msg);

	pos += snprints( pos, BUFSIZE-(pos-buf), "Status: ",FI(e),"\n",(char*)msg, "\n</body>\n</html>\n" );

	NSMALL( if ( (pos-buf)+SCRIPTMAXSIZE > BUFSIZE ) // shouldn't happen
		err( EFAULT, "Buffer Overflow" );

		pos += sprint_html_script(pos);
	)

	nwrite(fd, buf, (pos-buf) );

	close(fd);
	exit(0);
}


static int cb_writebuffer( int fd, char *buf, char *pos ){
	int ret = nwrite( fd, buf, pos-buf );
	// no error checking
	return(ret);
}

static void send_dir( int fd, char *path, struct stat* st ){
	char buf[BUFSIZE];
	char *pos = buf;

	verbose(1,"List directory: ",path);
	//dbg(path);
	if ( chdir( path ) < 0 )
		send_error(fd,500,"chdir");

	pos += http_header(pos,BUFSIZE,200,"Ok", MIMETYPE(html) ,0,st->st_mtime);

	//dbg(buf);
	pos += htmlhead(pos,BUFSIZE-(pos-buf),200,path);

	//dbg(buf);

	if ( OPT(w) ){
		if ( BUFSIZE-(pos-buf) < SCRIPTMAXSIZE )
			send_error(fd, 500, "Buffer" );
		pos += sprint_html_script(pos);
	}

	// path should be cleaned here
	pos += snprints( pos, BUFSIZE-(pos-buf), "<h3>Directory: ",path,"</h1>\n\n" );

	//dbg(buf);

	if ( strlen(path)> 1 && strncmp(path,GET(r), strlen(path)-2 ) ) 
		pos += snprints( pos, BUFSIZE-(pos-buf),"<a href=\"..\">Up .. &uarr;</a><br/>\n<br/>\n" ); 
	else
		pos += snprints( pos, BUFSIZE-(pos-buf),"(Root)<br/>\n<br/>\n" ); 

	//dbg( fd, (pos-buf) );

	nwrite(fd,buf,pos-buf);


	snprints(buf, BUFSIZE, 
			"ls | "
			"sed -E 's;(.*);<a href=\\\""/*relpath*/"\\1\\\">\\1</a><br/>;'"
			"; echo '</body></html>'");

	//	printvl( );

	char *args[] = { "sh", "-c", buf,  0 };

	close(1);
	dup(fd);

	
	execve( "/bin/sh", args, 0 );

	send_error(fd,500,"execve");

	exit(0);
}


#ifndef MICRO
static int convert_file(int fd, char *buf, uint bufsize, char* converter, char *path ){
	verbose( 0, "convert: ", converter, " ", path );

	pid_t pid = vfork();

	if ( pid==0 ){
		char *p = stpcpy(buf,converter);
		*p++ = ' ';
		strcpy(p,path);
		verbose( 0, "converting: ", buf );

		close(1);
		dup(fd);
		char* const arg[4] = { "sh", "-c", buf, 0 };
		int ret = execve( "/bin/sh", arg, (char* const*)environ );
		ewrites("Error. Couldn't exec sh in system()\n");
		exit(ERRNO(ret));
	}
	int ws;

	pid_t wpid;
	do {
		wpid = waitpid( pid, &ws, 0 ); // wait for any child (reap zombies)
	} while ( !( (wpid == pid) && (WIFEXITED(ws) || WIFSIGNALED(ws) )));

	return(WEXITSTATUS(ws));
}
#endif


static void send_file( int fd, char *path, struct stat* st ){
	char buf[BUFSIZE];
	char *pos = buf;
	NSMALL( char script[SCRIPTMAXSIZE]; )
	uint scriptsize = 0;

	verbose(0,"Accepted: ",path);

	int ffd = open( path, O_RDONLY );
	verbose(2,"ffd: ",FI(ffd));

	if ( ffd<0 )
		send_error( fd, 423, strerror(ERRNO(ffd) ) );

	ulong size = st->st_size;

	int mimetype = getmimetype(path);
	verbose(2,"mimetype: ",FI(mimetype));
	char *converter = 0;

#ifndef SMALL
	if ( mimetype ){
	if ( OPT(w) && ( mimetype == MIMETYPE(html) ) ){ //
		scriptsize = sprint_html_script(script);
		size += scriptsize;
	} else if ( OPT(m|i) && ( mimetype == MIMETYPE(markdown) ) ){
		scriptsize = sprint_html_script(script);
		converter = "lowdown -s";
		if ( OPT(i) )
			converter = "lowdown --html-no-skiphtml --html-no-escapehtml";
		size = 0; // is unknown.
		mimetype = MIMETYPE( html );
	} 
	}
#endif

	pos += http_header(pos,BUFSIZE,200,"Ok",mimetype,size,st->st_mtime);

	nwrite(fd,buf,pos-buf);

	if ( !converter ){
		verbose(2,"sendfile: ",FI(fd),",",FI(ffd),",",FL(st->st_size));
		sendfile( fd, ffd, 0, st->st_size );
	} 
#ifndef SMALL
	else { // execute lowdown
		int htmfd;
		//long sz;
		struct stat hst;
		if ( OPT(I) ){
			verbose(1,"prepend: ",GET(I));
			if ( (htmfd=open( GET(I), O_RDONLY)) < 0 ){
				warning( ERRNO(htmfd), "Cannot open -I: ",GET(I));
			} else {
				stat( GET(I), &hst );
				verbose(1,"size: ",FL(hst.st_size));
				sendfile( fd, htmfd, 0, hst.st_size );
				close(htmfd);
			}
		}
		convert_file(fd,buf,BUFSIZE,converter,path);
		if ( OPT(A) ){
			verbose(1,"append: ",GET(A));
			if ( (htmfd=open( GET(A), O_RDONLY)) < 0 ){
				warning( ERRNO(htmfd), "Cannot open -I: ",GET(A));
			} else {
				stat( GET(A), &hst );
				verbose(1,"size: ",FL(hst.st_size));
				sendfile( fd, htmfd, 0, hst.st_size );
				close(htmfd);
			}
		}
	}

	
	if ( scriptsize )
		nwrite( fd, script, scriptsize );
#endif

	close(fd);
	close(ffd);

	exit(0);
}


static void __attribute__((noreturn))http_handler( int fd ){
	verbose(2,"http handler");
	char buf[BUFSIZE]; // fixed size. no uploads.
	//char *buf = (char*)mmap( 0, BUFSIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0 );

	int r = recv(fd, buf, BUFSIZE, 0 );
 	verbose(2,"r: ",FI(r),"\nbuf:\n",buf );

	char *p;
	
	// fuzzy parsing
	char* method = strtok_r( buf, " \t\n\r", &p );
	char* resource = strtok_r( 0, " \t\n\r", &p );
	char* prot = strtok_r( 0, " \t\n\r", &p );

	verbose(2,"\nmethod: ",method,"  resource: ", resource, "  prot: ", prot);

	// ignore the rest. 

	if ( strncasecmp( "get", method, 3 ) )
		send_error(fd,405,"Unsupported" );


	// Somehow unecessary checks. This should be used locally only. Anyways.
	if ( prot-resource > PATH_MAX - strlen ( GET(r) ) )
		send_error(fd,400,"Bad Request");

	if ( resource[0] != '/' )
		send_error(fd,400,"Bad Path: 01");

#ifndef MICRO
	if ( strstr( resource, ".." ) ) // bad manner in each case.
		send_error(fd,400,"Bad Path: 02");
#endif


	char path[PATH_MAX];
	char *pend = stpcpy( path, GET(r) );
	memcpy( pend, resource, prot-resource );

	verbose(1,"Path:\n", path );


	struct stat st;
	//strcpy( p, de->d_name );
	if ( stat( path, &st ) != 0 ){
		verbose(0,"Couldn't stat ",path);
		send_error(fd,404,"Not found");
	}

	if ( OPT(g)  && ( st.st_gid != GET(g,int) ) )
		send_error(fd,403,"Not allowed. (-g)");
	if ( OPT(u)  && ( st.st_uid != GET(u,int) ) )
		send_error(fd,403,"Not allowed. (-u)");



	if ( S_ISREG(st.st_mode) ){
		/*if ( OPT(x) && ( st.st_mode & ( S_IXGRP|S_IXOTH|S_IXUSR ) ) ){ // executable
			close(1);
			dup(fd);
			execlp( path );
			err(EFAULT,"Couldn't execute: ",path);
		} */
		send_file( fd, path, &st );
	}

	if ( S_ISDIR( st.st_mode ) ){
		send_dir( fd, path, &st );
	}

	warning(ENXIO, "serve: Unknown file type: ",path);

	send_error( fd, 500, "Unknown file type" );

	exit(0);
}



void __attribute__((noreturn))httpd_serve( pid_t parent ){
	// Create a socket
	int rfd;
	socklen_t addrlen;
	struct sockaddr_in address;


	// install signal handlers
	struct sigaction sa;

	sigfillset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = sighandler;

	if ( sigaction (SIGTERM, &sa, 0) ||
		  sigaction (SIGQUIT, &sa, 0) ||
		  sigaction (SIGINT, &sa, 0) ){
		warning(0,"Couldn't install signal handler");
		// try to continue anyways.
	}

	prctl(PR_SET_NAME, (ulong)_Q(MODULE), 0, 0, 0);

#define errk(_errno,_msg) { kill( parent, SIGTERM ); err( _errno,_msg ); }

	// Check for successful socket initialization
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) <= 0){
		errk(ERRNO(sockfd),"Socket creation failed");
	}

	// Set binding parameters
	int port = GET(serverport,int);

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	int r;
	ulong l = 1;
	if ( (r =setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &l, sizeof(l))) < 0){
		errk(ERRNO(r),"Error setting socket options" );
	}


	if ( (r=bind(sockfd, (struct sockaddr *) &address, sizeof(address))) != 0){
		errk(ERRNO(r),"Error binding socket" );
	}

	verbose(0,"started, listening at port ",FI(port));

	int retr = 0;
	if ( (r=listen(sockfd, 10)) < 0) {
		warning(ERRNO(r),"server: listen");
		if ( ++retr > 10 )
			errk(EFAULT,"server: abort");
		usleep(100);
		errk(ERRNO(r),"Error listen" );
	}
	retr=0;

	// Begin listen loop
	while (1) {
		// Check for incoming client connections

		verbose(2,"accept");
		while ((rfd = accept(sockfd, (struct sockaddr *) &address, &addrlen)) < 0){
			warning(ERRNO(rfd),"server: accept");
			usleep( retr*retr*20000 );
			retr = (retr+1)&0x7;
		}
		retr=0;

		verbose(1,"Connection accepted");

		// Spawns a child process which handles the request
		int pid = fork();
		if (pid < 0){
			warning(ERRNO(pid),"Error: fork");
		}

		// If child process, close the request socket, and initiate the
		// handler
		if (pid == 0){
			verbose(1,"forked");
			close(sockfd);
			http_handler(rfd);
			exit(0);
		}
		// If parent process, close the response socket
		else {
			close(rfd);
		}
	} // while(1)

	__builtin_unreachable();
}



