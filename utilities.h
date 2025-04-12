
// conversions
char* muitodec( char *buf /* size >=16 */, uint i ){
	//assert(base>4);

	char *p = buf+15; 
	*p = 0;

	do {
		*(--p) = '0' + i % 10; // surprisingly, the compiler emits a single div for mod and div..
		if ( *p > '9' )
			*p += 39;
	} while ( (i = i/10) );
	
	return(p);
}

const char* multodec( char *buf /* size >=24 */, ulong i ){
	//assert(base>4);

	char *p = buf+23; 
	*p = 0;

	do {
		*(--p) = '0' + i % 10; //
		if ( *p > '9' )
			*p += 39;
	} while ( (i = i/10) );
	
	return(p);
}

#define ROL(bits,a) asm("rol $"#bits",%0" : "+r"(a) :: "cc" )

const char* muitohex( char* buf, uint i, uint mindigits ){
	char *p = buf;
	for ( int a = 8; a--; ){
		ROL( 4, i );
		int b = (i&0xf);
		mindigits |= (-b); // gets positiv, when b != 0
		if ( b > 9 ) b+= 7;
		if ( mindigits>a )
			*p++ = b + '0';
	}
	*p = 0;

	return(buf);
}



#define FI(_x) muitodec( (char[16]){},(_x))
#define FL(_x) multodec( (char[24]){},(_x))
#define FIHEX(_x) muitohex( (char[10]){},(_x),0)

#ifdef va_start
#undef va_start
#undef va_end
#undef va_arg
#undef va_copy
#undef va_list
#endif
#define va_start(v,l)   __builtin_va_start(v,l)
#define va_end(v)       __builtin_va_end(v)
#define va_arg(v,l)     __builtin_va_arg(v,l)
#define va_copy(d,s)    __builtin_va_copy(d,s)
typedef __builtin_va_list va_list;


#ifndef prints
#define prints(...) dprints(STDOUT_FILENO,__VA_ARGS__,0)
#define eprints(...) dprints(STDERR_FILENO,__VA_ARGS__,0)

#define printsl(...) prints(__VA_ARGS__,"\n")
#define eprintsl(...) prints(__VA_ARGS__,"\n")

#define ewrite(_s,_len) write(STDERR_FILENO,_s,_len)
#define ewrites(_s) ewrite(_s,sizeof(_s))
#define ewritesl(_s) ewrites(_s "\n")

#define writes(_s) write(STDOUT_FILENO,_s,sizeof(_s))
#define writesl(_s) writes(_s "\n")

#define printl() writes("\n")

#ifndef ERRNO
#define ERRNO(_e) (_e<0?errno:0)
#endif

#ifdef dprints
#undef dprints
#endif

// free bsd 
char *stplcpy(char *dest, const char *src, uint size){
	char *d = dest;
	char *e = (dest+size);
	while ( *src && (d<e) ){
		*d=*src;
		d++;
		src++;
	}
	*d= 0;

	return(d);
}

static inline const char* chk_pchar( const char* pchar){
	return( pchar );
}

int __dprints( int fd, ... ){
	#define dprints(_fd,...) __dprints( _fd, FOREACH_K(chk_pchar,__VA_ARGS__) )
		int ret = 0;
		const char *msg;
		va_list args;
		va_start(args,fd);

		while( (msg=va_arg(args,char*) ) ){
			int i = write(fd,msg,strlen(msg));
			if ( i<0 )
				return(i);
			ret += i;
		}

		va_end(args);
		return(ret);
}

uint _snprints( char *buf, uint len, ... ){
	#define snprints( _buf, _len, ... ) _snprints( _buf, _len, FOREACH_K(chk_pchar,__VA_ARGS__), 0 )
		char *pos = buf;
		const char *msg;
		va_list args;
		va_start(args,len);

		while( (pos < (buf+len)) && (msg=va_arg(args,char*) ) ){
			pos = stplcpy( pos, msg, len-(pos-buf) );
		}
		va_end(args);
		*pos = 0;

		return(pos-buf);
}

#endif



