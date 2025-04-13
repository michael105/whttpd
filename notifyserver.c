
#undef MODULE
#define MODULE whttpd_notify

// maximum of pages browsed, before an update in the clients is triggered
// to reset fdpos. also maximum of concurrent readers.
# define MAXRELOAD 1024 

int fds[MAXRELOAD];
int fdpos = 0;
int nsockfd = 0; // socket of the notifyserver
int sigreload = 0;



void notify_sighandler(int sig){
	sigreload = 1;
	if ( sig == SIGUSR1 )
		return;
	if ( sig != SIGQUIT ){
		if ( parentpid ) kill( parentpid, SIGUSR1 );
	}
	verbose(0,"Quit");
	exit(0);
}



void triggerreload(){
	verbose(0,"Trigger reload");
	while ( fdpos --> 0 ){
		//write( fds[fdpos], "\030\n", 3); // Cancel 
		close(fds[fdpos]);
	}
	fdpos = 0;
}


static int openport(){

	notifypid = fork();

	if ( notifypid != 0 ){
		return(notifypid);
	}
	
	prctl(PR_SET_NAME, (ulong)_Q(MODULE), 0, 0, 0);

	sigreload=0;
	nsockfd = 0;

   int rfd = 0;
   socklen_t addrlen;
   struct sockaddr_in address;

	// install signal handlers
	struct sigaction sa;

	sigfillset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = notify_sighandler;

	if ( sigaction (SIGTERM, &sa, 0) || 
			sigaction(SIGINT, &sa,0) || 
			sigaction(SIGQUIT, &sa,0) || 
			sigaction(SIGUSR1, &sa,0) ){
		err(0,"Couldn't install signal handler");
	}

	int rep = 0;

#define RETRY(_r,_msg) {\
		if ( ++rep > 10 ){ \
			kill(serverpid,SIGTERM); \
			err( EFAULT, "watcher:" _msg ); \
		} \
		warning(ERRNO(_r), "watcher: " _msg "\nRetry ", FI(rep),"\n" ); \
		usleep(50000*rep*rep); \
}

#define RETRY2(_ret,_repeat,_msg) int _rep = 0; do {\
		if ( _rep++ ){ \
			if ( _rep > _repeat ){ \
				kill(serverpid,SIGTERM); \
				err( EFAULT, "watcher:" _msg ); \
			} \
		warning(ERRNO(_ret), "watcher: " _msg "\nRetry ", FI(_rep),"\n" ); \
		usleep(50000*_rep*_rep); \
		} } while


#define RETRY4(_try,_msg,_repeat,_condition) {\
	int _rep = 0,_ret; \
		while ( sigreload || ( ( _ret =  (_try) ) _condition ) ){ \
			if ( sigreload ){ \
				verbose(1,"SIGRELOAD"); \
				sigreload = _rep = 0; \
				/*if ( nsockfd ) close(nsockfd);*/ \
				/*exit(0);*/ \
			}  \
			if ( _rep >= _repeat ){ \
				kill(serverpid,SIGTERM); \
				err( ERRNO(_ret), "watcher:" _msg ); \
			} \
			warning(ERRNO(_ret), "watcher: " _msg "\nRetries: ", FI(_rep),"\nerrno: ",FI(ERRNO(_ret))  ); \
			usleep(50000*_rep*_rep); \
			_rep++; \
		} } 




   // Check for successful socket initialization
	//while ((nsockfd = socket(AF_INET, SOCK_STREAM, 0)) <= 0){
    //  RETRY(nsockfd," Error create socket");
   //}
   //RETRY2(nsockfd,10,"Create socket")
	//RETRY4((nsockfd = socket(AF_INET, SOCK_STREAM, 0)), "watcher: open socket", 10, <0 );

		rep = 0;
		int tmpfd = 0;
		while ( sigreload || ( (tmpfd = socket(AF_INET, SOCK_STREAM, 0) ) < 0 )){ 
			if ( sigreload ){ 
				verbose(1,"SIGRELOAD"); 
				rep = 0;
				sigreload=0;
				//if ( tmpfd>=0 ) close(tmpfd); 
				//if ( nsockfd>=0 ) close(nsockfd); 
				//exit(0); 
			}  
			if ( rep >= 10 ){ 
				kill(serverpid,SIGTERM); 
				kill(notifypid,SIGTERM); 
				err( ERRNO(tmpfd), "watcher:  cannot open socket"  ); 
			} 
			warning(ERRNO(tmpfd), "watcher:  open socket\nRetries: ", FI(rep),"\nerrno: ",FI(ERRNO(tmpfd))  ); 
			if ( nsockfd>0 ) close(nsockfd); 
			nsockfd = 0;
			usleep(50000*rep*rep); 
			rep++; 
		} 
	nsockfd = tmpfd;

	int r,ret;
	struct linger li;
	li.l_onoff = 1;
	li.l_linger = 0; // timeout, 0 seconds
	if ( (ret=setsockopt(nsockfd, SOL_SOCKET, SO_LINGER, &li, sizeof(li))<0 ) )
			warning(ERRNO(ret),"se so_linger: ", strerror( ERRNO(ret) ) );

	ulong l = 1;
	if ( (r =setsockopt(nsockfd, SOL_SOCKET, SO_REUSEADDR, &l, sizeof(l))) < 0){
		err(ERRNO(r),"Error setting socket options" );
	}

   // Set binding parameters
   int port = GET(P,int);

   address.sin_family = AF_INET;
   address.sin_addr.s_addr = INADDR_ANY;
   address.sin_port = htons(port);

	rep = 0;
   // Bind socket to address and port
 // while ( (r=bind(nsockfd, (struct sockaddr *) &address, sizeof(address))) ){
  //    RETRY(r,"watcher: Error opening port");
  // }
  	RETRY4( (r=bind(nsockfd, (struct sockaddr *) &address, sizeof(address))), 
			"bind port", 10, != 0 );

	
	rep = 0;
	while ( sigreload || (r=listen(nsockfd, 10)) < 0) {
		//if ( ERRNO(r) == EINTR ){
		if ( sigreload ){
			verbose(1, "sigreload" );
			sigreload = 0;
			triggerreload();
			//close(nsockfd);
			//exit(0);
	
		} else 
			RETRY(r,"listen");
	}
	
	verbose(0,"started, listening at port ", FI(port));
	
	while ( 1 ){
		rep=0; // can also be a client side abort/whatever these javascript
				 // implementations are doing. Trying harder.
		while ( sigreload ||  (rfd = accept(nsockfd, (struct sockaddr *) &address, &addrlen)) < 0 ){
			//if	( ERRNO(rfd) == EINTR ) {
			if ( sigreload ){
				sigreload = 0;
				triggerreload();
				verbose(1, "sigreload" );
				//close(nsockfd);
				//exit(0);
	
			} else
				RETRY(rfd,"Accept aborted");
		}
	
		//	nwrites(fds[a], "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n"
		//		"\r\n\r\n<html></html>\r\n\r\n" );
		nwrites(rfd,	"HTTP/1.0 200 Ok\r\n"
				"Access-Control-Allow-Origin: *\r\n"
				"Access-Control-Allow-Methods: GET\r\n"
				"Access-Control-Allow-Headers: *\r\n"
				"Abort\018\e" ); // trigger error

	
		verbose(1,"notifyserver, accepted: ", FI(fdpos) );
		if ( fdpos >= MAXRELOAD )
			triggerreload(); // reload all other clients, start new fdlist
	
		fds[fdpos] = rfd;
		fdpos++;
	
	}

}


