meteod:	meteod.o httpd.o
	$(CC) $(LDFLAGS) meteod.o httpd.o $(shell pkg-config --libs libmicrohttpd) -o meteod

meteod.o: meteod.c
	$(CC) $(CFLAGS) $(shell pkg-config --cflags libmicrohttpd) -c meteod.c

httpd.o: httpd.c
	$(CC) $(CFLAGS) $(shell pkg-config --cflags libmicrohttpd) -c httpd.c 

clean:
	rm -f *.o meteod

install:
