AM_CPPFLAGS = -I$(srcdir)/../lib -I$(top_srcdir)/include

noinst_LTLIBRARIES = mines.la

mines_la_SOURCES = board.c somain.c

mines_la_LIBFLAGS = $(top_builddir)/fx2/lib/libfx2.la

mines_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version -lfx2

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)
	$(LIBTOOL) --mode=install install mines.la $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/mines.cfg $(DESTDIR)$(PLUGINDIR)

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/mines.so
	-rm $(DESTDIR)$(PLUGINDIR)/mines.cfg
	-rmdir $(DESTDIR)$(PLUGINDIR)
