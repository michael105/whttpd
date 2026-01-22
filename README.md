
### whttpd

#### ''SSA'' micro webserver


For local website development, instantly usable without configuration.

Forking, using sendfile. ('new' linux syscall).


Uses inotify and ''SSA'' (Server Side Aborts) to watch the webroot and instantly reload all html pages in the browser,
when files within the htmlroot directory hierarchy are modified. 
Reload also 404 sites and directory indexes with new content.

Can optionally automatically rebuild pages with make or any other tool, 
if sources are changed in the htmlroot or another watched location.

Optionally build html pages from markdown files with lowdown on the fly.

To trigger reloads a small (10 lines) javascript is appended to each page,
which shouldn't interfere with other javascript frameworks. (basic fetch, wait asyncron for errors)

-----

I wouldn't recommend using this in the wild. 
There might be flaws, I don't know.
If you do so against this advise, I'm eager to hear about
the suspected vulnerabilities.
I'd recommend: Use an isolated container. Watch used ressources (memory,cpu).
	switch to an unprivileged user, and use a port above 2000. 
	E.g., whttpd parses paths only very briefly. But there's
	no use in trying to implement security for a http server, which is
	intended for local development usage..

	Eventually I'm going to replace the call to sed / sh for website directories.
	But it's the same. And should be save, since at this place no
	input is involved besides the directory and file names.
	IF you name a file `\\'\\"\rm \\-rf\\*` or something pathological
	like that, I don't know, what's going to happen. there might be a breakout
	possible. And there might be other possibilities.
	It's written for local development, not for security.
	It's also the reason for not preforking or threading.

	It would be possible to prefork, when the socket's flags are changed.
	The whole binary size comes with all flags enabled with 26kB, 
	using 20 or 50 preforked servers might even become performant.

	When compiling without inotify and javascript 'SSA' server part,
	the binary's size gets down below 8kB.

	Yet I need to get rid of the huge stack, which the kernel at my system 
	bloats to legendary 170kB.
	Compared to one page (4kB) for the globals (512Bytes would be enough, huua),
	and some bytes at the stack -
	I did set buffersizes to 4kB or something like that - well.
	'Stackless' is, what I'm now aiming for.
s 	I also do believe, it's an error in advance, keeping all the env variables
	for a potentially endangoured process. Which should be regarded as
	infected and malicious, in consequence.
	Since this is a version for local website development, I will
	not change that. 

	I even regard this server for the intended job as stable, 
	I'm using it myself.

-----

##### Install:

Either build yourself, or download the binary for 64bit linux supplied in
/bin. The binary is statically linked with all requirements.

Configuration is done via commandline arguments.

To browse markdown files as html, lowdown is needed.  
[https://github.com/kristapsdz/lowdown](https://github.com/kristapsdz/lowdown) / [https://github.com/michael105/static-bin](https://github.com/michael105/static-bin)


##### Usage:


   `whttpd`

serve the current directory at port 4000.
(http://localhost:4000)


`whttpd -w [htmlroot]`

Start webserver (default port 4000) and the 'ssa' server (port 4001).  
Modify all sent html pages, append a small javascript, and trigger reloads  
in the clients, as soon files within htmlroot are modified, added or
deleted.

<br>


```
whttpd [-hHvqCaLwmiM] [-r htmlroot] [-p serverport] [-g gid] [-u uid] [-P notifyport] [-R recursion] [-I header] [-A append] [-e cmd] [htmlroot] [watchdir1] [watchdir2 ...]

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
 -M                execute make in 'htmlroot', when files are changed
 -e cmd            execute cmd in 'htmlroot', when files are changed
 ```


-----


CC-BY-SA 4.0, 2025,26 misc147 (fee227), codeberg.org/misc1



In my interpretation and intention, the cc-by-sa license
allows to reuse this work in its whole or parts,
also commercially, without infecting.


(As long, my work isn't the substantial part of the derived work, this
might be discussable, but silly as well.)


























