

#ifdef SMALL
#define NSMALL(...) // __VA_ARGS__
#else
#define NSMALL(...) __VA_ARGS__
#endif

#ifndef PAGESIZE
#include "compat.h"
#endif


#define with_strftime

#include "macros/macrolib.h"
#include "macros/options.h"


#define OPTIONS \
	h,,"show usage", \
	NSMALL(H,,"show help",) \
	v,,"increase verbosity (max -vvv)", \
	q,,"quiet", \
\
	r,htmlroot,"path of the html server root (defaults to . or arg1)", \
	p,serverport,"port of the webserver (4000)", \
	g,gid,"Restrict served files to group owner gid", \
	u,uid,"Restrict served files to file owner uid", \
	C,,"add to http a 'Pragma: no-cache'", \
\
NSMALL(	\
	a,,"watch hidden files/dirs (. and ~)", \
	P,notifyport,"port used to trigger reloads (4001)", \
	R,recursion,"maximum recursion depth of watched directories (255)", \
	L,,"watcher follows symbolic links", \
	w,,"modify pages to push updates", \
	m,,"execute lowdown to convert markdown to html", \
	i,,"lowdown, with options --html-no-skiphtml and --html-no-escapehtml", \
	I,header,"html before the output of -i", \
	A,append,"html after the output of -i", \
	M,,"execute make in 'htmlroot', when files are changed", \
	e,cmd,"execute cmd in 'htmlroot', when files are changed", \
)


	//x,,"execute cgi scripts/binaries (danger)", 
#ifndef SMALL
#define INT_OPTS p|P|R|g|u
#else
#define INT_OPTS p|g|u
#endif

CHECK_OPTIONS;

DECLARE_SETTING;


#define TOOL whttpd
#define LICENSE CC-BY-SA-4.0
#define VERSION "0.1.devel-370a"


#include "tools.h"
#include "utilities.h"

USAGE("[htmlroot]"NSMALL(" [watchdir1] [watchdir2 ...]"));

#ifndef SMALL
HELP(    "Serve a directory at (default) port 4000.\n"
         "Watch for changes, opionally rebuild pages,\n"
		   "trigger reloads in the clients. ");
#endif



uint opts=0;
uint verbose=0;
pid_t serverpid, notifypid, parentpid;


#define MAXPATHREC 255 // default maximum of nested directory recursion
#define MAXWATCHES 1024

int watches[MAXWATCHES];
int watchpos = 0;
static int removewatches();
int nfd; // inotifyfd


#define _ifdef_MODULE
#define _MODULE_PREFIX()  _IFDEF_ELSE( MODULE, _Q(MODULE) ": ", __FILE__ ": ")


#define AC_N         "\033[0;37;40m"
#define AC_LCYAN     "\033[1;36m"
#define AC_BROWN     "\033[0;33m"
#define AC_LBLUE     "\033[1;34m"
#define AC_RED       "\033[0;31m"
#define AC_ORANGE    "\033[0;38;5;214m"

// verbose: either macro (0,1,2) or variable
#define __verbose(_v,...) { if ( verbose > _v ) eprintsl( _MODULE_PREFIX(),  __VA_ARGS__); }

#ifndef MICRO
#define verbose0(...) __verbose(0,AC_LCYAN,__VA_ARGS__,AC_N)
#define verbose1(...) __verbose(1,AC_LBLUE, __VA_ARGS__,AC_N)
#define verbose2(...) __verbose(2,AC_BROWN,__VA_ARGS__,AC_N)

#define err(_errno,...) { if (_errno) { eprintsl("\n"AC_RED, _MODULE_PREFIX(), __VA_ARGS__,"\nError: ",strerror(_errno),AC_N); exit(_errno); } }
#define warning(_errno,...) { if (_errno) { eprintsl("\n"AC_ORANGE, _MODULE_PREFIX(), __VA_ARGS__,"\nError: ",strerror(_errno),AC_N );  } }

#else
#warning Target: MICRO
#define verbose0(...) // __verbose(0,__VA_ARGS__)
#define verbose1(...) 
#define verbose2(...) 
#define warning(_errno,...) { if(_errno) eprintvl("Warn "__STRLINE__": ", _errno ); }
#define err(_errno,...) { if (_errno) { eprintvl("Err "__STRLINE__ ": ",_errno); exit(_errno); } }
#endif

#define verbose(_v,...) verbose##_v(__VA_ARGS__)

#define nwrites(_fd,_s) nwrite(_fd,_s,sizeof(_s))



#ifndef WATCHERONLY

#include "server.c"

#endif

#ifndef HTTPDONLY

#include "notifyserver.c"


#undef MODULE
#define MODULE whttpd_watcher


int abouttoquit = 0;
void watcher_sighandler(int sig){

	verbose(0,"Quit");
	switch ( sig ){
		case SIGUSR1:
			if ( serverpid ) kill( serverpid, SIGQUIT );
			break;

		case SIGUSR2:
			if ( notifypid ) kill( notifypid, SIGQUIT );
			break;

		default:
			if ( ! abouttoquit ){
				abouttoquit = 1;
				if ( serverpid ) kill( serverpid, SIGQUIT );
				if ( notifypid ) kill( notifypid, SIGQUIT );
			}
	}

	exit(0);
}


static int reopenport( pid_t pid ){
	//return(pid);
	kill(pid,SIGUSR1); // trigger reload
	//kill(pid,SIGTERM); // close all connections
	/*								 
	int ws;
	pid_t wpid;
	do {
		wpid = waitpid( -1, &ws, 0 ); // wait for pid to exit (reap zombies)
	} while ( !( ( (wpid == pid) && (WIFEXITED(ws) || WIFSIGNALED(ws) ) ) ) );

	int rep = 0;
	//usleep(50000);
	while ( kill(pid,SIGTERM) == 0 ){
		warning( ERRNO(0),"rekill zombie");
		if ( rep++>10 ){
			warning( ERRNO(0),"Kill: ",pid);
			kill(pid,SIGKILL);
		}
		usleep(200000);
	}


	pid = openport(); // restart */
	return(pid);
}

static int watchpath( const char* path, int nfd ){
		int ir = inotify_add_watch( nfd, path, IN_CREATE | IN_MODIFY | IN_MOVE | IN_DELETE );
		verbose(2,"Add watch to ",path," ir: " AC_LBLUE, FI(ir),AC_N " nfd: ", FI(nfd));
		if ( ir<=0 ){ 
				warning( ERRNO(ir),"Couldn't add an inotify watch to ", path );
				return(ir);
		}
		//watches[watchpos] = ir;
		//watchpos++;
		return(0);
}

static int removewatches(){
	int ow = watchpos;
	while ( watchpos > 0 ){
		watchpos--;
		verbose2("remove watch, watchpos: ",FI(watchpos),"  ir:",AC_RED,FI(watches[watchpos]),AC_N" nfd: ",FI(nfd));
		int r = inotify_rm_watch(nfd,watches[watchpos]);
		if ( r ) warning( ERRNO(r),"remove watch: ",FI(r));
	}
	return(ow);
}

	
#define MODMASK 0170000

// recursively add inotify watches
static int traverse_dir( const char* path, int maxdepth, int nfd ){ 
		watchpath( path, nfd );
		int ret = 0;

		if ( maxdepth == 0 )
				return(0);
		maxdepth--;

		DIR *dir = opendir( path );
		verbose(1,"opendir: ",path);
		if ( !dir )
				return(0);

		struct dirent *de;
		char pathname[PATH_MAX];
		char *p = stpcpy(pathname, path);
		*p++='/';

		while( ( de = readdir( dir ) ) ){
				verbose(2,"Path: ",(char*)de->d_name);
				if ( (de->d_name[0] == '.' ) && 
						( (de->d_name[1] == 0 ) || ( de->d_name[1] == '.' ) ) ) 
						continue; // skip . and ..
				if (de->d_name[0] == '.' )  // hidden files
						continue;

				verbose(1,"Watch: ",(char*)de->d_name);

				struct stat st;
				strcpy( p, de->d_name );
				if ( lstat( pathname, &st ) != 0 ){
						verbose(0,"Couldn't stat ",pathname);
						continue;
				}
				if ( S_ISDIR(st.st_mode) || ( OPT(L) && S_ISLNK(st.st_mode) ) ){ // is dir // option follow links
						ret |= traverse_dir( pathname, maxdepth, nfd );
				} else {
					ret |= watchpath( pathname, nfd );
				}

		}
		verbose(2,"Out");

		closedir(dir);
		//free(dir);
		return(ret);
}

#endif // HTTPDONLY

// convert to absolute paths
// depending on its input, returns a new (malloc) string.
// Doesn't free the old path.
static char *getpath(char* ppath){
	char *path = ppath;
	char *pp = path;

	if ( *ppath == '.' ){
		ppath++;
		if ( *ppath == '.' ){
			err(EINVAL,"No relative path starting with .. allowed");
		}

		path = malloc(PATH_MAX);
		int l = ml_getcwd(path,PATH_MAX);

		if ( *ppath  ){
			if ( *ppath != '/' )
				strcat(path,"/");
			strcat(path,ppath);
		}

		if ( !(path = realloc( path, strlen(path)+1 ) ) )
			err(ENOMEM,"realloc failed");

		verbose(1,"Path converted: ",pp," --> ",path);
	}

	return(path);
}


# define abort(...) { kill(pid,9);exit(__VA_ARGS__ + 0); }



MAIN{
	char **pargv = argv;

	verbose = 1;
	int r;

	NSMALL( SET(R,MAXPATHREC,int);
	SET(P,4001,int); )
	SET(p,4000,int);


	PARSEARGV( 'h': usage() NSMALL(, 'H': help(), 'v': verbose++, 'q': verbose=0) );

	verbose(0, "started, pid: ",FI(getpid()), " pm : ",FI(PATH_MAX) );

	// should be possible to submit more files / paths
	
	char *path;
	const char **watchpaths = calloc( argc-(argv-pargv) + 2, sizeof(POINTER) );

	if ( OPT(r) ){
		 path = GET(r);
	} else {
		if ( *argv ){
			path = *argv; 
			argv++;
		} else { 
			path = ".";
		}
		SET(r,path); // can use relative paths in the webserver.
	}

	// convert pathname
	path = getpath(path);
	watchpaths[0] = path;


#ifdef HTTPDONLY
	httpd_serve( opts, setting, getpid(), GET(r) );
#else

#ifndef WATCHERONLY
	parentpid = getpid();
	serverpid = fork();

	if ( !serverpid )
			httpd_serve( opts, setting, parentpid, GET(r) );

	// install signal handlers
	struct sigaction sa;

	sigfillset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = watcher_sighandler;

	if ( sigaction (SIGTERM, &sa, 0) ||
		  sigaction (SIGQUIT, &sa, 0) ||
		  sigaction (SIGUSR1, &sa, 0) ||
		  sigaction (SIGUSR2, &sa, 0) ||
		  sigaction (SIGINT, &sa, 0) ){
		warning( ERRNO(0),"Couldn't install signal handler");
		// continue anyways, this isn't essential
	}

#endif

	pid_t pid = openport();
	notifypid = pid;


	// initiate inotify
	nfd = inotify_init();
	//nfd = inotify_init1(IN_CLOEXEC);
	if ( nfd<0 ){
		ewritesl("Couldn't initiate inotify. No kernel support?" );
		abort(EFAULT);
	}

	verbose(1,"inotify initiated");

	//if ( (r=watchpath(path,nfd)) ){
	//	warning( ERRNO(ERRNO(r)),"Cannot add inotify watch to ",path);
	//	abort();
	//}
	if ( ( r = traverse_dir( path, GET(R,int), nfd ) ) ){
		warning( ERRNO(r),"Error adding watches to ",path,"\nContinue");
	}


	int countwp = 1;
	while ( *argv ){ // watch other directories
		char *p = getpath(*argv);
		if ( ( r = traverse_dir( p, GET(R,int), nfd ) ) ){
			warning( ERRNO(r),"Error adding watches to ",p,"\nContinue");
		} 
		// could be created later
		watchpaths[countwp] = p;
		countwp++;
		argv++;
	}


#define BUFLEN PAGESIZE
	char buf[BUFLEN];
	const struct inotify_event *e;

	while( 1 ){
		while ( (r=read(nfd,buf,BUFLEN)) <= 0 ){
			warning( ERRNO(r),"read from inotify." );
			sleep(1);
		}
		verbose(1,"files changed");

		const struct inotify_event *e;
		int reopen = 0, traverse = 0, make = 0, reread=0;
		do {
		for ( char *p = buf; p < buf+r;	p += sizeof(struct inotify_event) + e->len) {
			e = (const struct inotify_event *) p;
			verbose(2,"wd: ",FI(e->wd), " mask: ",FIHEX(e->mask), 
					" cookie: ",FI(e->cookie)," len: ",FL(e->len) );
			if ( e->len < 2 )
				continue;

			verbose(1," Path update: ",(char*)e->name );
			if ( !OPT(a) && ( e->name[0] == '.' || ( e->name[strlen(e->name)-1] == '~') ) ){
				verbose(1,"Ignore");
				continue;
			}

			if ( e->mask & ( IN_DELETE ) ){
				verbose(1,"Deleted");
				reopen = 1;
			} 
			if ( e->mask & ( IN_MODIFY ) ){
				verbose(1,"Modified");
				reopen = 1;
				make = 1;
			} 
			if ( e->mask & ( IN_IGNORED ) ){
				//r = inotify_rm_watch( nfd, e->wd );
				verbose(2," rmwatch: ",FI(r)," wd ", FI(e->wd));
				continue;
			} 
			if ( e->mask & ( IN_CREATE | IN_MOVE ) ){ // new file / path
				verbose(1,"New");
				reopen = 1;
				traverse = 1;
				make = 1;
			}
		}
			
   		  fd_set rfds;
           FD_ZERO(&rfds);
           FD_SET(nfd, &rfds);
       	  int rs = -255;
			  //struct timeval rtv;
			 if ( reread == 0 ) reread = 10;
      	struct timeval tv = { 0,50000 }; // wait this time for new events (max *10), before notifying
           if ( (rs=select(nfd+1, &rfds, NULL, NULL, &tv)) > 0 ){
           //rtv=ml_select(&rfds, 0, 1 );
				//if ( rtv.tv_usec ){
				  reread--;
				  if ( reread>0 ){
					  verbose2("REREAD");
					  //usleep(50000);
					  r=read(nfd,buf,BUFLEN);
				  }
			  } else
				  reread = 0;
			verbose2("rs: xxx ",FI(rs), " tv: ",FI(tv.tv_usec)," : ",FI(tv.tv_sec));


		} while ( reread > 0 && r > 0 );

	
		if ( traverse ){
			//int i = removewatches(nfd);
			//eprints("removed watches, waiting: ",FI(i));
			//close(nfd);
			//nfd = inotify_init();
			for ( const char **p = watchpaths; *p; p++ ){
				verbose2("traverse: ",*p);
				if ( ( r = traverse_dir( *p, GET(R,int), nfd ) ) ){
					warning( ERRNO(r),"Error adding inotify watches to ",*p,"\nContinue");
		}
			}
		}

		if ( make && ( OPT(M) || OPT(e) )){
			//char cmd[PATH_MAX];
			snprints( buf, PATH_MAX, "chdir ", path, "; ", (OPT(e)?GET(e):"make") );
			system( buf );
		}

		if ( reopen )
			pid = reopenport(pid);
	
	}

	abort();

#endif // HTTPDONLY
	exit(0);
}

