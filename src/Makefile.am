bin_PROGRAMS = itpl2dirtree

itpl2dirtree_SOURCES = itpl2dirtree.h itpl2dirtree.c hashint.c alhash.h hash.c

LDADD = -L@EXPAT_LDADD@ -lexpat
AM_CPPFLAGS = -I$(top_builddir) @EXPAT_INCLUDES@
AM_LDFLAGS = -Xlinker -rpath -Xlinker @EXPAT_LDADD@
