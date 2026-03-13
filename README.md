### whttpd

#### "SSA" micro webserver


```
This README is written by me, and corrected for grammar by gemini.
For several reasons I'm clearly marking any ai usage. 
The code is written by me, but I did a short ai security review.
misc147
```


For local website development, instantly usable without configuration.

Forking, using sendfile ('new' Linux syscall).

Uses inotify and "SSA" (Server-Side Aborts) :innocent: to watch the webroot and instantly reload all HTML pages in the browser when files within the htmlroot directory hierarchy are modified. 
Also reloads 404 pages and directory indexes with new content.

Can optionally automatically rebuild pages with make or any other tool if sources are changed in the htmlroot or another watched location.

Optionally builds HTML pages from Markdown files with lowdown on the fly.

To trigger reloads in the browser, a small (10 lines) JavaScript snippet is appended to each page, which shouldn't interfere with other JavaScript frameworks. 
(Basic background fetch, waiting asynchronously for errors, with the SSA - Server-Side Aborts - :triumph: triggered by file modifications on the server side.)

-----

I wouldn't recommend using this in the wild. 
There might be flaws, I don't know.
If you do so against this advice, I'm eager to hear about the suspected vulnerabilities.
I'd recommend: Use an isolated container. Watch used resources (memory, cpu).
	Switch to an unprivileged user, and use a port above 2000. 
	E.g., whttpd parses paths only very briefly. But there's
	no use in trying to implement security for an HTTP server which is
	intended for local development usage.

	Eventually, I'm going to replace the call to sed / sh for website directories.
	But it's the same, and it should be safe since at this place no
	input is involved besides the directory and file names.
	If you name a file `\\'\\"\rm \\-rf\\*` or something pathological
	like that, I don't know what's going to happen. There might be a breakout
	possible, and there might be other possibilities.
	It's written for local development, not for security.
	This is also the reason for not preforking or threading.

	It would be possible to prefork when the socket's flags are changed.
	The whole binary size is around 26kB with all flags enabled; 
	using 20 or 50 preforked servers might even be performant.

	When compiling without inotify and the JavaScript 'SSA' server part,
	the binary's size drops below 8kB.

	Yet I need to get rid of the huge stack which the kernel on my system 
	bloats to a legendary 170kB.
	Compared to one page (4kB) for the globals (512 bytes would be enough, haha)
	and some bytes on the stack —
	I did set buffer sizes to 4kB or something like that — well.
	'Stackless' is what I'm now aiming for.
 	I also believe it's a mistake to keep all the environment variables
	for a potentially vulnerable process, which should consequently be regarded as
	infected and malicious.
	Since this is a version for local website development, I will
	not change that. 

	I even regard this server as stable for the intended job; 
	I'm using it myself.

-----

##### Installation:

Either build it yourself, or download the binary for 64-bit Linux supplied in
/bin. The binary is statically linked with all dependencies.

Configuration is done via command-line arguments.

To browse Markdown files as HTML, lowdown is needed.[https://github.com/kristapsdz/lowdown](https://github.com/kristapsdz/lowdown) /[https://github.com/michael105/static-bin](https://github.com/michael105/static-bin)


##### Usage:


`whttpd`

Serves the current directory at port 4000.
(http://localhost:4000)


`whttpd -w [htmlroot]`

Starts the webserver (default port 4000) and the 'SSA' server (port 4001).  
Modifies all sent HTML pages, appends a small JavaScript snippet, and triggers reloads  
in the clients as soon as files within htmlroot are modified, added, or
deleted.

<br>


```
whttpd [-hHvqCaLwmiM] [-r htmlroot] [-p serverport] [-g gid] [-u uid] [-P notifyport] [-R recursion] [-I header][-A append] [-e cmd] [htmlroot] [watchdir1] [watchdir2 ...]

 -h                show usage
 -H                show help
 -v                increase verbosity (max -vvv)
 -q                quiet
 -r htmlroot       path of the html server root (defaults to . or arg1)
 -p serverport     port of the webserver (4000)
 -l listenaddr     Address to listen (127.0.0.1)
 -g gid            Restrict served files to group owner gid
 -u uid            Restrict served files to file owner uid
 -C                add to http 'Pragma: no-cache'
 -a                watch hidden files/dirs (. and ~)
 -P notifyport     port used to trigger reloads (4001)
 -R recursion      maximum recursion depth of watched directories (255)
 -L                watcher follows symbolic links
 -w                modify pages to push updates
 -m                execute lowdown to convert markdown to html
 -i                lowdown, with options --html-no-skiphtml and --html-no-escapehtml
 -I header         html before the output of -i
 -A append         html after the output of -i
 -M                execute make in 'htmlroot' when files are changed
 -e cmd            execute cmd in 'htmlroot' when files are changed
```


-----


CC-BY-SA 4.0, 2025, 2026 misc147 (fee227), codeberg.org/misc1

In my interpretation and intention, the CC-BY-SA license
allows reusing this work in whole or in part,
even commercially, without infecting other code.

(As long as my work isn't the substantial part of the derived work, this
might be debatable, but silly as well.)




