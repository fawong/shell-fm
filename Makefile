PREFIX	:= /usr
MANDIR	:= $(PREFIX)/share/man
DOCDIR  := ${PREFIX}/share/doc

.PHONY			: shell-fm-macports manual all clean tags cscope install

all				: shell-fm-macports manual

shell-fm-macports		:
	$(MAKE) -C source

manual			:
	$(MAKE) -C manual

install			: all
	install -m 0755 -d $(DESTDIR)$(PREFIX)/bin/
	install -m 0755 -d $(DESTDIR)$(MANDIR)/man1/
	install -m 0755 source/shell-fm-macports $(DESTDIR)$(PREFIX)/bin/
	install -m 0644 manual/shell-fm-macports.1.gz $(DESTDIR)$(MANDIR)/man1/

install-extras		:
	install -m 0755 -d $(DESTDIR)$(DOCDIR)/shell-fm-macports/
	cp -r scripts $(DESTDIR)$(DOCDIR)/shell-fm-macports/



install-strip	: install
	strip $(DESTDIR)$(PREFIX)/bin/shell-fm-macports

clean			:
	$(MAKE) -C source clean
	$(MAKE) -C manual clean


tags		: cscope.files
	@rm -f tags
	xargs -n50 ctags -a < cscope.files

cscope		: cscope.files
	cscope -b

cscope.files	:
	find source -name '*.[ch]' > cscope.files

