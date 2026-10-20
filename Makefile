all: diskinfo disklist diskget diskput 

diskinfo: diskinfo.c filesystem.c
	gcc -Wall -Wextra -std=c11 diskinfo.c filesystem.c -o diskinfo

disklist: disklist.c filesystem.c
	gcc -Wall -Wextra -std=c11 disklist.c filesystem.c -o disklist

diskget: diskget.c filesystem.c
	gcc -Wall -Wextra -std=c11 diskget.c filesystem.c -o diskget

diskput: diskput.c filesystem.c
	gcc -Wall -Wextra -std=c11 diskput.c filesystem.c -o diskput

clean:
	rm -f *.o diskinfo disklist diskget diskput