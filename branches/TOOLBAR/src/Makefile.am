# these are the headers for the project
noinst_HEADERS = layoutcanvas.h toped.h tui.h resourcecenter.h tpdph.h

#BUILT_SOURCES = tell_yacc.h
#AM_YFLAGS = -p tell -d -y -v
#AM_LFLAGS = -Ptell

#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = toped

# the application source, library search path, and link libraries
toped_SOURCES = layoutcanvas.cpp  resourcecenter.cpp tui.cpp toped.cpp main.cpp

#########################################################################
# EXTRAS
#########################################################################
#LEX_OUTPUT_ROOT = lex.tell
INCLUDES = -I$(top_srcdir)/tpd_parser -I$(top_srcdir)/tpd_DB \
	-I$(top_srcdir)/tpd_common -I$(top_srcdir)/tpd_bidfunc
toped_LDADD = ../tpd_parser/libtpd_parser.la ../tpd_common/libtpd_common.la \
              ../tpd_DB/libtpd_DB.la ../tpd_bidfunc/libtpd_bidfunc.la
#toped_LDFLAGS = -static

