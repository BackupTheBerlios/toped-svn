noinst_HEADERS = ted_prompt.h  tellyzer.h tldat.h
#INCLUDES = 
#METASOURCES = AUTO
BUILT_SOURCES = tell_yacc.h
AM_YFLAGS = -p tell -d -y -v
AM_LFLAGS = -Ptell

lib_LTLIBRARIES = libtpd_parser.la
libtpd_parser_la_LDFLAGS = -no-undefined
libtpd_parser_la_SOURCES = ted_prompt.cpp tellyzer.cpp \
		tell_lex.ll tell_yacc.yy tldat.cpp
#########################################################################
# EXTRAS
#########################################################################
LEX_OUTPUT_ROOT = lex.tell

INCLUDES = -I$(top_srcdir)/tpd_DB -I$(top_srcdir)/tpd_common
libtpd_parser_la_LTADD = ../tpd_common/libtpd_common.la
METASOURCES = AUTO
