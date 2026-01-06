CC=cc
DESTDIR=/
PREFIX=/usr

duplicate: duplicate.c
	$(CC) $< -o $@

.PHONY:
install: duplicate
	mkdir -p $(DESTDIR)/$(PREFIX)
	cp $^ $(DESTDIR)/$(PREFIX)
