all: test wsserver

test: test-path test-watcher

test-path: path.o test-path.o
	$(CC) $(CFLAGS) -std=gnu11 path.o test-path.o -o test-path

test-watcher: path.o watcher.o test-watcher.o
	$(CC) $(CFLAGS) -std=gnu11 path.o watcher.o test-watcher.o -o test-watcher

test-img: img.o test-img.o read-file.o
	$(CC) $(CFLAGS) -std=gnu11 img.o test-img.o read-file.o `pkg-config --libs MagickWand` -o test-img

path.o: path.c
	$(CC) $(CFLAGS) -std=gnu11 -c path.c -o path.o

watcher.o: watcher.c
	$(CC) $(CFLAGS) -std=gnu11 -c watcher.c -o watcher.o

img.o: img.c
	$(CC) $(CFLAGS) -std=gnu11 `pkg-config --cflags MagickWand` -c img.c -o img.o

read-file.o: read-file.c
	$(CC) $(CFLAGS) -std=gnu11 -c read-file.c -o read-file.o

test-path.o: test-path.c
	$(CC) $(CFLAGS) -std=gnu11 -c test-path.c -o test-path.o

test-watcher.o: test-watcher.c
	$(CC) $(CFLAGS) -std=gnu11 -c test-watcher.c -o test-watcher.o

test-img.o: test-img.c
	$(CC) $(CFLAGS) -std=gnu11 -c test-img.c -o test-img.o

wsserver:
	./build.sh

clean: clean-test clean-wsserver

clean-test: clean-test-path clean-test-watcher

clean-test-path:
	rm -f test-path test-path.o path.o

clean-test-watcher:
	rm -f test-watcher test-watcher.o watcher.o

clean-test-img:
	rm -f test-img test-img.o img.o read-file.o

clean-wsserver:
	rm -f *.o
	rm -f wsserver moc_*
