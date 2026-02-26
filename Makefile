DESTDIR=
PREFIX=/usr

all: duplicate

duplicate: duplicate.c
	$(CC) $(CFLAGS) $^ -o $@ 

.PHONY: install uninstall clean
install: duplicate
	cp duplicate $(DESTDIR)$(PREFIX)/bin/duplicate
uninstall:
	rm $(DESTDIR)$(PREFIX)/bin/duplicate
clean:
	rm -f duplicate
