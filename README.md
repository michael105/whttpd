
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

I wouldn't recommend using this in the wild. 
There might be flaws, I don't know.
If you do so against this advise, I'm eager to hear about
the suspected vulnerabilities.


-----

##### Install:

Either build yourself, or download the binary for 64bit linux supplied in
/bin. The binary is statically linked with musl.

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


























