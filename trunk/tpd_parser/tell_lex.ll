/*===========================================================================
//                                                                          =
//   This program is free software; you can redistribute it and/or modify   =
//   it under the terms of the GNU General Public License as published by   =
//   the Free Software Foundation; either version 2 of the License, or      =
//   (at your option) any later version.                                    =
// ------------------------------------------------------------------------ =
//                  TTTTT    OOO    PPPP    EEEE    DDDD                    =
//                  T T T   O   O   P   P   E       D   D                   =
//                    T    O     O  PPPP    EEE     D    D                  =
//                    T     O   O   P       E       D   D                   =
//                    T      OOO    P       EEEEE   DDDD                    =
// ------------------------------------------------------------------------ =
//           $URL: http://perun/tpd_svn/trunk/toped_wx/tpd_parser/tell_lex.ll $
//  Creation date: Fri Nov 08 2002
//     Created by: Svilen Krustev - s_krustev@yahoo.com
//      Copyright: (C) 2001-2004 by Svilen Krustev
//    Description: wxWidget version
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------                
//      $Revision: 228 $
//          $Date: 2005-10-16 11:03:08 +0100 (Sun, 16 Oct 2005) $
//        $Author: skr $
//---------------------------------------------------------------------------
// A non-reentrant parser for tell
//===========================================================================*/

/* Define the (exclusive) start condition when the parser includes a file */
%x incl
lex_string  \"[^"]*\"
%{ /**************************************************************************/
#include <stdio.h>
#include <string.h>
#include "tellyzer.h"
#include "tell_yacc.h"
#include "ted_prompt.h"

/* Definitions for handling multiply input buffers */
#define MAX_INCLUDE_DEPTH 10
parsercmd::lexer_files* include_stack[MAX_INCLUDE_DEPTH];
int include_stack_ptr = 0;
/*Global console object*/
extern console::ted_cmd*           Console;
/*****************************************************************************
Function declarations
*****************************************************************************/
#define YY_USER_ACTION  telllloc.last_column += yyleng;
namespace parsercmd {
   void     location_step(YYLTYPE *loc);
   void     location_lines(YYLTYPE *loc, int num);
   char*    charcopy(std::string source, bool quotes = false);
   unsigned getllint(char* source);
   int      includefile(const char* name, FILE* &handler);
   int      EOfile();
}
extern YYLTYPE telllloc;
%}
%%
%{ /*******************************************************************************
     This section contains local definitions and also actions executable at each
     invocation of yylex. */
/* Mark the current position as a start of the next token */
location_step(&telllloc);
%} /*******************************************************************************/
[ \t]+                     location_step(&telllloc);
\n+                        location_lines(&telllloc,yyleng);location_step(&telllloc);
"//".*\n                   location_lines(&telllloc,1);/* comment line */
`include                   BEGIN(incl);
<incl>{lex_string}       { /*first change the scanner state, otherwise there
                             is a risk to remain in <incl>*/
                           BEGIN(INITIAL); 
                           if (! parsercmd::includefile(parsercmd::charcopy(yytext, true), yyin)) 
                              yyterminate(); }
<incl><<EOF>>            { BEGIN(INITIAL); return tknERROR; }                              
<<EOF>>                  { if (!parsercmd::EOfile()) yyterminate();}
void                       return tknVOIDdef;
real                       return tknREALdef;
int                        return tknINTdef;
bool                       return tknBOOLdef;
point                      return tknPOINTdef;
box                        return tknBOXdef;
string                     return tknSTRINGdef;
layout                     return tknLAYOUTdef;
list                       return tknLISTdef;
return                     return tknRETURN;
true                       return tknTRUE;
false                      return tknFALSE;
if                         return tknIF;
else                       return tknELSE;
while                      return tknWHILE;
repeat                     return tknREPEAT;
struct                     return tknSTRUCTdef;
until                      return tknUNTIL;
{lex_string}             { telllval.parsestr = parsercmd::charcopy(yytext, true);
                           return tknSTRING;                               }
"."[Pp]"1"               { telllval.parsestr = parsercmd::charcopy("1");return tknFIELD;}
"."[Pp]"2"               { telllval.parsestr = parsercmd::charcopy("2");return tknFIELD;}
"."[Xx]                  { telllval.parsestr = parsercmd::charcopy("x");return tknFIELD;}
"."[Yy]                  { telllval.parsestr = parsercmd::charcopy("y");return tknFIELD;}
"<="                       return tknLEQ;
">="                       return tknGEQ;
"=="                       return tknEQ;
"!="                       return tknNEQ;
"&&"                       return tknAND;
"||"                       return tknOR;
"("                        return '(';
")"                        return ')';
"["                        return '[';
"]"                        return ']';
"{"                        return '{';
"}"                        return '}';
"<"                        return '<';
">"                        return '>';
","                        return ',';
"+"                        return '+';
"-"                        return '-';
"*"                        return '*';
"/"                        return '/';
"="                        return '=';
";"   	                  return ';';
0x[0-9A-Fa-f]+    |
[0-9]+                   { telllval.integer = parsercmd::getllint(yytext); return tknINT;}
[0-9]+"."[0-9]*   |
[0-9]*"."[0-9]+       	 { telllval.real = atof(yytext); return tknREAL;}
[a-zA-Z0-9_$]*           { telllval.parsestr = parsercmd::charcopy(yytext);return tknIDENTIFIER;}
.     	             	   return tknERROR;
%% 
/**************************************************************************/
/*Support functions for the flex parser*/
/**************************************************************************/
int yywrap() {
   return 1;/*line by line*/
}

void my_delete_yy_buffer( void* b ) {
   yy_delete_buffer((YY_BUFFER_STATE) b);
}

//=============================================================================
// define scanner location tracking functions -
// see the link below - idea is taken from there
// from http://www.lrde.epita.fr/~akim/compil/gnuprog2/Advanced-Use-of-Flex.html
//=============================================================================

void parsercmd::location_step(YYLTYPE *loc) {
   loc->first_column = loc->last_column;
   loc->first_line = loc->last_line;
}

void parsercmd::location_lines(YYLTYPE *loc, int num) {
   loc->last_column = 0;
   loc->last_line += num;
}

//=============================================================================
// Some C string management stuff - avoid malloc
//=============================================================================

char* parsercmd::charcopy(std::string source, bool quotes) {
   int length = source.length() - (quotes ? 2 : 0);
   char* newstr = new char[length+2];
   memcpy(newstr,&(source.c_str()[quotes ? 1 : 0]),length);
   newstr[length] = 0x00;
   return newstr;
}

unsigned parsercmd::getllint(char* source) {
   char* boza;
   unsigned result = strtoul(source, &boza, 0);
   return result;
}

//=============================================================================
// File include handling
//=============================================================================
int parsercmd::includefile(const char* name, FILE* &handler) {
   FILE* newfilehandle;
   int retvalue = 0;
   if ( include_stack_ptr >= MAX_INCLUDE_DEPTH )
      tell_log(console::MT_ERROR,"Too many nested includes");
   else {
      newfilehandle = fopen(name, "r");
      if (! newfilehandle)
         tell_log(console::MT_ERROR,"Included file can not be open");
      else {   
         handler = newfilehandle;
         /* create a new record with file handler and location objects*/
         include_stack[include_stack_ptr++] = new parsercmd::lexer_files(
                    static_cast<void*>(YY_CURRENT_BUFFER), new YYLTYPE(telllloc));
         /* switch the buffer to the new file */
         yy_switch_to_buffer(yy_create_buffer( newfilehandle, YY_BUF_SIZE ) );
         /* initialize the current error location object */
         telllloc.first_column = telllloc.last_column = 1;
         telllloc.first_line = telllloc.last_line = 1;
         telllloc.filename = name;
         retvalue = 1;
      }
   }
   return retvalue;   
}

int parsercmd::EOfile() {
   if ( include_stack_ptr > 0 ) {
      /* get the previous file record from the array */
      parsercmd::lexer_files* prev = include_stack[--include_stack_ptr];
      /* take care to free the memory from the current file name */
      if (telllloc.filename) delete (telllloc.filename);
      /* restore the error location object*/
      telllloc = *(prev->location);
      /* delete the current file buffer (I suppose file is also closed)*/
      yy_delete_buffer( YY_CURRENT_BUFFER );
      /* switch to the restored buffer */
      yy_switch_to_buffer(static_cast<YY_BUFFER_STATE>(prev->lexfilehandler));
      delete prev;
      return 1;
  }
  return 0;
}
