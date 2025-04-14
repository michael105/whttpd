

whttpd:
	gcc -Os -o whttpd -s -static watcher.c


up:
	updateversion.sh watcher.c
	gtam rev. `revision.sh`
	git push



