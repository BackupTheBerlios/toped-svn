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
//                                                                          =
//   This file is a part of Toped project (C) 2001-2006 Toped developers    =
// ------------------------------------------------------------------------ =
//           $URL$
//        Created: Fri Nov 08 2002
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: TELL parser
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================
// type / operation |  +  |  -  |  *  |  /  |  u- |
//------------------+-----+-----+-----+-----+-----+
//   real           |  x  |  x  |  x  |  x  |  x  |
//   int            |  x  |  x  |  x  |  x  |  x  |
//   point          |  x  |  x  |     |     |     |
//   box            |  x  |  x  |     |     |     |
//   string         |     |     |     |     |     |
//   bool           |     |     |     |     |     |
//
//===========================================================================*/

/*Switch on code processing locations for more acurate error messages*/
%locations
%{
#include "tpdph.h"
#include <sstream>
#include "tellyzer.h"
#include "ted_prompt.h"
#include "../tpd_common/outbox.h"
/* Switch on verbose error reporting messages*/
#define YYERROR_VERBOSE 1
/*Current command block - defined in tellyzer.cpp*/
extern parsercmd::cmdBLOCK*       CMDBlock;
/*Global console object*/
extern console::ted_cmd*           Console;
namespace parsercmd
{
   extern int EOfile();
}

/*Current tell variable name*/
telldata::tell_var *tellvar = NULL;
telldata::tell_var *tell_lvalue = NULL;
/*Current variable is a list*/
bool indexed = false;
bool lindexed = false;
/*Current commands - parsed, but not yet in the command stack*/
parsercmd::cmdFOREACH *foreach_command = NULL;
parsercmd::cmdLISTADD *listadd_command = NULL;
/*Current tell struct */
telldata::tell_type *tellstruct = NULL;
/* used for argument type checking during function call parse */
telldata::argumentQ   *argmap  = NULL;
/*taking care when a function is called from the argument list of another call*/
std::stack<telldata::argumentQ*>  argmapstack;

/* Current function declaration structure */
parsercmd::FuncDeclaration *cfd;
/*number of errors in a function defintion*/
int funcdeferrors = 0;
/*number of poits of the current polygon*/
unsigned listlength = 0;
void tellerror(std::string s, TpdYYLtype loc);
void tellerror (std::string s);
void cleanonabort();

%}
/*=============================================================================
Well some remarks may save some time in the future...
 The parser is supposed to work as a command line parser as well as a normal
 file parser. The language has a data types, local/global  variables, blocks,
 functions, cycles etc. stuff. So what's the main idea - nothing is executed
 during the parsing. The parser actually just creates a list of cmdVIRTUAL
 objects by calling their constructors. So if nothing's executed there is no
 way to calculate any result during parsing - so nothing to assign to
 $$ variable. In the same time it's a good idea to control data types during
 the parsing. Assignment statement, function parameters - these are all
 good examples. That's why $$ variable is used (where appropriate) to hold
 the language types (enum telldata::type). Of course not all predicates have
 telldata::type. That might be a source of confusion - so some further comments
 concerning the %union statement below:
    real        - this holds the real numbers during parsing. tknREAL is the
                  only token of this type
    parsestr    - to hold variable/function names during parsing
    pttname     - that's the tell_type we spoke about above. Once again - this is
                  NOT a tell variable. Just the ID of the type.
    pfblock     - same as previous, however to avoid generally some casting
                  the {} blocks and function blocks have been separated.
                  funcblock is obviously the only predicate of this type
    pfarguments - Holds the function definition arguments. funcarguments is
                  the only predicate of this type
    pargumants  - Structure with function call arguments
 
 There are two telldata::tell_var* variables defined in the parser.
   - tellvar - stores a pointer to the last referenced (or defined) tell
               variable. In all cases cmdPUSH is added to the commands with
               a tellvar argument wich means that during the execution phase,
               a copy of tellvar will be pushed in the operand stack. Normally,
               that COPY is used by the subsequent commands and then destroyed.
               Not all the commands use the copy in the operand stack thought.
               An important exception is cmdASSIGN (see tell_lvalue below) and 
               list operations - cmdLISTADD.
               Another example is the loop variable (foreach). Generally this
               variable is hardly used directly.
   - tell_lvalue - is storing always the tellvar and is used as an argument of
               Assign. Assign obviously needs the original variable - not a copy
               of it which is normally used for most of the operaions. In other
               words lvalues are not handled trough the operand stack, instead
               this variable is used.


   Some more questions:
   - Why variables are cloned in the operand stack? If they weren't - we don't
     need tell_lvalue and most likely tellvar as well.
     The main reason are the memory leakeges. It is getting very messy with the
     loops, constants, intermediate results etc. Some of them should be deleted,
     some of them should be kept.
     Operations like recursive list union:
         int list a = {1,2,3};
         a[:+] = a;
     can't be executed properly if rvalue is not cloned
   - What happens with the items in the operand stack which are not used?
     ( for example function returns a value, but it is not assigned to anything)
     The parser pushes cmdSTACKRST in the instructions stack at the end of every
     tell statement. During execution this class clears the operand stack.
   - Does all variables exists during parsing? What about dynamic variables?
     By definition we don't want pointers in the script ( don't we ?) - means
     that dynamic variables don't exists. This is not quite correct though.
     Lists are an important exception here. See below.

Now something about the arrays. Actually there will be lists. Why? The main
 reason is that select functions may (and certainly will) return various
 objects - point, box, polygons, path, text etc together. Arrays by nature
 contain fixed number of homogenous objects and I can't think of a convinient
 language construct for selection that will return such a thing. That's why it
 seems better to introduce a list instead. The list members can be of any tell
 type and they don't need to be the same type.
 The question then becames - do we need a function that will return the type of
 a list member? Seems the answer is - NO!Of course I might be wrong - let's see
 Enough rubish! The constructs are:
 list <variable>         -> definition;
 variable[<index>]       -> indexing;
 word length(<variable>) -> standard function will return the number of members
 assignment will have the normal form and a normal type checks will apply.
 Besides polygons and paths will be represented as a list of points

Ooops! Second thought!
 The thing is that we have two different thigns here:
   One (array or list) is a langage construct
   Another (that thing that select will return) is a layout data base object(s).
 What does it mean...
 If the list are implemented as described above this effectively means that we'll
 loose a grip on the type checking or in words there is no way to check and
 control the types of the list components during the parsing. This is a major
 gap in the concept of a strict type checking in tell. From the other point of
 view the problem with the select still remains...
 Select however have to return a pointers to layout DB objects. POINTERS to the
 OBJECTS that ALREADY EXIST in the DB. We can NOT treat them as any other tell
 types and normal tell operations can not apply to them or at least they will
 need a special attention. Operation like delete will be unique for this
 type of objects. There is nothing like cell for example in the language but
 this is a major DB object...
 Well, it seems that IS the milestone where tell and toped split (or converge
 if you want. It's simple!
 box a = ((10,10,20,20)) -> is a tell construct defining and initializing the
                            tell variable a of type box
 addbox(a)               -> is creating a rectangular layout object in the
                             current cell.
 These two things have nothing in common from the moment when addbox(a) is
 executed. A lot of addbox fuctions can be executed with this variable.
 Select function will provide a way to get a reference to the existing layout
 objects. This references can be used aftewrards to move, copy, delete etc.
 these objects, but these operations will not affect by any means tell variable 'a'.
 Having this cleared-up the concept of lists is now crystal. All lists will be
 homogeneous and their definitions will look like.
  <tell_type> list <variable>
 thus the strict type-cheking is still in place. I still insist on the typename
 list (not array) because the size of these things will not be fixed. The side
 effect here is that polygons and wires now became a list of points. We will
 have list of words (for definition of the fills for example) - we can have
 list of any tell type and the type-check will still work perfectly!

 A new type obviously will be introduced to accomodate the pointers to the
 layout objects described above.This is still to be thought out but it seems it
 will look like
  layout <variable>
 There will not be a tell const available of this type. This will be the type
 returned from addbox, addpoly, addcell etc. functions.
 Select of course will return: 
  layout list select(box<variable>)
 Simple!

 Now about indexing operations. The lists in tell are nothing else but a
 dynamic arrays. Which means that the index can't be checked during the parsing
 but only during runtime. The second trouble is when an indexed component is
 an lvalue. It means that tellvar and tell_lvalue can't get a pointer to that
 list component.
 The problem is resolved with the introduction of the "indexed" variable in the
 parser. tellvar is getting a pointer to the entire list and "indexed" is set to
 true which allows the indexing to be postponed to the execution phase.

=============================================================================*/
%union {
   float                       real;
   bool                        ptypedef;
   int                         integer;
   char*                       parsestr;
    telldata::typeID           pttname;
   parsercmd::argumentLIST*    pfarguments;
    telldata::argumentQ*       plarguments;
    telldata::argumentID*      parguments;
   parsercmd::cmdBLOCK*        pblock;
   parsercmd::cmdFUNC*         pfblock;
   parsercmd::FuncDeclaration* pfdeclaration;
}

%start input
/*---------------------------------------------------------------------------*/
%token                 tknERROR
%token	               tknIF tknELSE tknWHILE tknREPEAT tknUNTIL tknFOREACH
%token                 tknSTRUCTdef tknVOIDdef tknREALdef tknBOOLdef tknINTdef
%token                 tknSTRINGdef tknLAYOUTdef tknLISTdef tknRETURN
%token                 tknTRUE tknFALSE tknLEQ tknGEQ tknEQ tknNEQ
%token                 tknAND tknOR tknNOT tknBWAND tknBWOR tknBWNOT
%token                 tknSW tknSE tknNE tknNW tknPREADD tknPRESUB
%token                 tknPOSTADD tknPOSTSUB tknCONST
%token <parsestr>      tknIDENTIFIER tknFIELD tknSTRING
%token <real>          tknREAL
%token <integer>       tknINT
/* parser types*/
%type <pttname>        primaryexpression unaryexpression
%type <pttname>        multiexpression addexpression expression
%type <pttname>        assignment fieldname funccall telllist
%type <pttname>        lvalue telltype telltypeID variable anonymousvar
%type <pttname>        variabledeclaration andexpression eqexpression relexpression
%type <pttname>        listindex listinsert listremove listslice
%type <pfarguments>    funcarguments
%type <parguments>     structure argument
%type <plarguments>    nearguments arguments
%type <pfblock>        funcblock
%type <pblock>         ifcommon
%type <ptypedef>       fielddeclaration typedefstruct
%type <pfdeclaration>  funcdeclaration
/*=============================================================================*/
%%
input:
     entrance                              {}
   | input  entrance                       {}
;

entrance:
      statement                            {
      if (!yynerrs)  CMDBlock->execute();
      else 
      {
         CMDBlock = CMDBlock->cleaner();
         parsercmd::EOfile();
      }
   }
   | funcdefinition                        {}
   | funcdeclaration ';'                   {
      delete($1); cfd = NULL;
   }
   | tknERROR                              {
      tellerror("Unexpected symbol", @1);
      parsercmd::EOfile();
   }
   | error                                 {
      CMDBlock = CMDBlock->cleaner();
      parsercmd::EOfile();
      /*yynerrs = 0;*/
   }
;

funcdeclaration:
     telltypeID tknIDENTIFIER '('          {
      cfd = DEBUG_NEW parsercmd::FuncDeclaration($2, $1) ;
   }
     funcarguments ')'                     {
      CMDBlock->addUSERFUNCDECL(cfd,@$);
      $$ = cfd;
      delete [] $2;
   }
;

funcdefinition:
     funcdeclaration funcblock             {
      CMDBlock->addUSERFUNC($1, $2, @$);
      // addUSERFUNC will take care about cleaning the funcdeclaration ($1) and funcblock ($2) 
      cfd = NULL;
   }
;

funcblock :
     '{'                                   {
         CMDBlock = DEBUG_NEW parsercmd::cmdFUNC(cfd->argListCopy(),cfd->type(),false);
         CMDBlock->pushblk();
      }
     statements '}'                        {
         $$ = static_cast<parsercmd::cmdFUNC*>(CMDBlock);
         CMDBlock = CMDBlock->popblk();
      }
;

returnstatement :
     tknRETURN                             {
      if (!cfd) tellerror("return statement outside function body", @1);
      else {
         parsercmd::cmdRETURN* rcmd = DEBUG_NEW parsercmd::cmdRETURN(cfd->type());
         if (rcmd->checkRetype(NULL)) CMDBlock->pushcmd(rcmd);
         else {
            tellerror("return value expected", @1);
            delete rcmd;
         }
      }
      cfd->incReturns();
   }
   | tknRETURN  argument                   {
      if (!cfd) tellerror("return statement outside function body", @1);
      else {
         parsercmd::cmdRETURN* rcmd = DEBUG_NEW parsercmd::cmdRETURN(cfd->type());
         if (rcmd->checkRetype($2)) CMDBlock->pushcmd(rcmd);
         else {
            tellerror("return type different from function type", @2);
            delete rcmd;
         }
         delete $2;
      }
      cfd->incReturns();
   }
;

ifcommon:
     tknIF '(' expression ')'       {
         CMDBlock = DEBUG_NEW parsercmd::cmdBLOCK();
         CMDBlock->pushblk();
     }
     blockstatement {
         parsercmd::cmdBLOCK* if_block = CMDBlock;
         CMDBlock = CMDBlock->popblk();
         if (telldata::tn_bool != $3) {
            tellerror("bool type expected",@3);
            delete if_block;
            $$ = NULL;
         }
         else {
            $$ = if_block;
         }
      }
;

ifstatement:
     ifcommon                       {
        if (NULL != $1)
           CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdIFELSE($1, NULL));
   }
   | ifcommon tknELSE               {
         CMDBlock = DEBUG_NEW parsercmd::cmdBLOCK();
         CMDBlock->pushblk();
   }  
     blockstatement                   {
         parsercmd::cmdBLOCK* else_block = CMDBlock;
         CMDBlock = CMDBlock->popblk();
         if (NULL != $1)
            CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdIFELSE($1,else_block));
      }
;

whilestatement:
     tknWHILE '('                   {
         CMDBlock = DEBUG_NEW parsercmd::cmdBLOCK();
         CMDBlock->pushblk();
      }
     expression ')'                 {
         if (telldata::tn_bool != $4) tellerror("bool type expected", @4);
         CMDBlock = DEBUG_NEW parsercmd::cmdBLOCK();
         CMDBlock->pushblk();
      }
     blockstatement {
         parsercmd::cmdBLOCK* body_block = CMDBlock;
         CMDBlock = CMDBlock->popblk();
         parsercmd::cmdBLOCK* cond_block = CMDBlock;
         CMDBlock = CMDBlock->popblk();
         CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdWHILE(cond_block,body_block));
      }
;

telllist:
     expression                     {
         if       (!($1 & telldata::tn_listmask)) {
            tellerror("list expected",@1);
            $$ = telldata::tn_NULL;
         }
         else
            $$ = $1;
      }
;

foreachstatement:
     tknFOREACH '('                 {
         CMDBlock = DEBUG_NEW parsercmd::cmdBLOCK();
         CMDBlock->pushblk();
      }
     lvalue ';' telllist ')'        {
         if  (($4 | telldata::tn_listmask) != $6)
            tellerror("unappropriate variable type",@4);
         foreach_command = DEBUG_NEW parsercmd::cmdFOREACH(tell_lvalue,tellvar);
         CMDBlock = DEBUG_NEW parsercmd::cmdBLOCK();
         CMDBlock->pushblk();
      }
     blockstatement                 {
         parsercmd::cmdBLOCK* body_block = CMDBlock;
         CMDBlock = CMDBlock->popblk();
         parsercmd::cmdBLOCK* header_block = CMDBlock;
         CMDBlock = CMDBlock->popblk();         
         foreach_command->addBlocks(header_block, body_block);
         CMDBlock->pushcmd(foreach_command);
      }
;

repeatstatement:
     tknREPEAT                      {
         CMDBlock = DEBUG_NEW parsercmd::cmdBLOCK();
         CMDBlock->pushblk();
      }
     blockstatement tknUNTIL '('   {
         CMDBlock = DEBUG_NEW parsercmd::cmdBLOCK();
         CMDBlock->pushblk();
      }
     expression ')'                 {
         parsercmd::cmdBLOCK* cond_block = CMDBlock;
         CMDBlock = CMDBlock->popblk();
         parsercmd::cmdBLOCK* body_block = CMDBlock;
         CMDBlock = CMDBlock->popblk();
         if (telldata::tn_bool != $7) {
            tellerror("bool type expected", @7);
            delete cond_block;
         }
         else CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdREPEAT(cond_block,body_block));
   }
;

blockstatement:
     statement                             { }
   | '{' '}'                               { }
   | '{' statements '}'                    { }
;

statements:
     statement                             { }
   | statements     statement              { }
;

statement:
     ';'                                   { }
   | variabledeclaration ';'               { }
   | assignment          ';'               {CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdSTACKRST());}
   | ifstatement                           {CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdSTACKRST());}
   | whilestatement                        {CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdSTACKRST());}
   | foreachstatement                      {CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdSTACKRST());}
   | repeatstatement                       {CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdSTACKRST());}
   | returnstatement     ';'               {/*keep the return value in the stack*/}
   | funccall            ';'               {CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdSTACKRST());}
   | listremove          ';'               {CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdSTACKRST());}
   | listslice           ';'               {CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdSTACKRST());}
   | recorddefinition    ';'               { }
;

funccall:
     tknIDENTIFIER '('                     {
        argmap = DEBUG_NEW telldata::argumentQ;
        argmapstack.push(argmap);
   }
      arguments ')'                        {
      parsercmd::cmdSTDFUNC *fc = CMDBlock->getFuncBody($1,$4);
      if (fc) {
         CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdFUNCCALL(fc,$1));
         $$ = fc->gettype();
      }
      else {
         tellerror("unknown function name or wrong parameter list",@1);
         $$ = telldata::tn_NULL;
      }
      telldata::argQClear(argmap);
      argmapstack.pop();
      delete argmap;
      if (argmapstack.size() > 0) argmap = argmapstack.top();
      else argmap = NULL;
      delete [] $1;
   }
;

assignment:
     lvalue '=' argument                   {
      /*because of the (possible) structure that has an unknown yet tn_usertypes type,
      here we are doing the type checking, using the type of the lvalue*/
      $$ = parsercmd::Assign(tell_lvalue, lindexed, $3, @2);
      delete $3;
   }
   | listinsert '=' argument               {
      $$ = parsercmd::Uninsert(tell_lvalue, $3, listadd_command, @2);
      delete $3;
   }
;

arguments:
                                           {$$ = NULL;}
    | nearguments                          {$$ = $1;}
;

argument :
     expression                            {$$ = DEBUG_NEW telldata::argumentID($1);}
   | assignment                            {$$ = DEBUG_NEW telldata::argumentID($1);}
   | structure                             {$$ = $1;}
;

nearguments :
      argument                             {argmap->push_back($1); $$ = argmap;}
   | nearguments ',' argument              {argmap->push_back($3); $$ = argmap;}
;

funcarguments:
                                           {}
   | funcneargument                        {}
;

funcneargument:
     funcargument                          {}
   | funcneargument ',' funcargument       {}
;

funcargument:
     telltypeID tknIDENTIFIER              {
      tellvar = CMDBlock->newTellvar($1, @1);
      cfd->pushArg(DEBUG_NEW parsercmd::argumentTYPE($2,tellvar));
      delete [] $2;
   }
;

lvalue:
     variable                               {$$ = $1;tell_lvalue = tellvar; lindexed = indexed;}
   | variabledeclaration                    {$$ = $1;tell_lvalue = tellvar; lindexed = indexed;}
;

variable:
     tknIDENTIFIER                         {
      tellvar = CMDBlock->getID($1);
      if (tellvar) $$ = tellvar->get_type();
      else
      {
         tellerror("variable not defined in this scope", @1);
         $$ = telldata::tn_NULL;
      }
      delete [] $1;
      indexed = false;
   }
   | fieldname                             {$$ = $1; indexed = false;}
   | listindex                             {$$ = $1; indexed = true;}
;

variabledeclaration:
     telltypeID   tknIDENTIFIER            {
      telldata::tell_var* v = CMDBlock->getID($2, true);
      if (!v) {/* if this variableID doesn't exist already in the local scope*/
         /* add it to the local variable map */
         tellvar = CMDBlock->newTellvar($1, @1);
         CMDBlock->addID($2,tellvar); 
      }
      else
         tellerror("variable already defined in this scope", @2);
      $$ = $1; delete [] $2;indexed = false;
   }
   | tknCONST telltypeID tknIDENTIFIER      {
      telldata::tell_var* v = CMDBlock->getID($3, true);
      if (!v) {/* if this variableID doesn't exist already in the local scope*/
         /* add it to the local variable map */
         tellvar = CMDBlock->newTellvar($2, @2);
         CMDBlock->addconstID($3,tellvar,false);
      }
      else
         tellerror("variable already defined in this scope", @2);
      $$ = $2; delete [] $3;indexed = false;
   }
   | variabledeclaration ',' tknIDENTIFIER  {
      telldata::tell_var* v = CMDBlock->getID($3, true);
      if (!v) {/* if this variableID doesn't exist already in the local scope*/
         /* add it to the local variable map */
         tellvar = CMDBlock->newTellvar($1, @1);
         CMDBlock->addID($3,tellvar);
      }
      else tellerror("variable already defined in this scope", @3);
      $$ = $1; delete [] $3;indexed = false;
   }
;

fielddeclaration:
     telltypeID  tknIDENTIFIER              {
      const telldata::tell_type* ftype =
            CMDBlock->getTypeByID($1 & ~telldata::tn_listmask);
      if (!tellstruct->addfield($2, $1, ftype)) {
         tellerror("field with this name already defined in this strucutre", @2);
         $$ = false; // indicates that definition fails
      }
      else $$ = true;
      delete [] $2;
   }
;

telltypeID:
     telltype                              {$$ = $1;}
   | telltype tknLISTdef                   {$$ = $1 | telldata::tn_listmask;}
/*   | telltype '[' ']'                      {$$ = $1 | telldata::tn_listmask;}*/
;

telltype:
     tknVOIDdef                            {$$ = telldata::tn_void;}
   | tknREALdef                            {$$ = telldata::tn_real;}
   | tknINTdef                             {$$ = telldata::tn_int;}
   | tknBOOLdef                            {$$ = telldata::tn_bool;}
   | tknSTRINGdef                          {$$ = telldata::tn_string;}
   | tknLAYOUTdef                          {$$ = telldata::tn_layout;}
   | tknIDENTIFIER                         {
        const telldata::tell_type* ttype = CMDBlock->getTypeByName($1);
        if (NULL == ttype)  {
           tellerror("Bad type specifier", @1);
           $$ = telldata::tn_NULL;
           delete [] $1;
           cleanonabort();
           YYABORT;
        }
        else $$ = ttype->ID();
        delete [] $1;
      }
;

recorddefinition:
     tknSTRUCTdef tknIDENTIFIER            {
        tellstruct = CMDBlock->requesttypeID($2);
        if (NULL == tellstruct) {
           tellerror("type with this name already defined", @1);
           delete [] $2;
           cleanonabort();
           YYABORT;
        }
     }
     '{' typedefstruct '}'                {
        if ($5) CMDBlock->addlocaltype($2,tellstruct);
        else delete tellstruct;
        tellstruct = NULL;
        delete [] $2;
     }
;

typedefstruct:
     fielddeclaration                     { $$ = $1;      }
   | typedefstruct ';' fielddeclaration   { $$ = $1 && $3;}
;

fieldname:
     variable tknFIELD              {
      if (NULL == tellvar)
      {
         tellerror("Unexisting variable", @1);
         cleanonabort();
         YYABORT;
      }
      else if (indexed)
      {
         tellerror("Can't dereference a field of a listed structures(Bug #14112). Please use intermediate variables.", @1);
         $$ = telldata::tn_NULL;
      }
      else
      {
         tellvar = tellvar->field_var($2);
         if (tellvar) $$ = tellvar->get_type();
         else
         {
            tellerror("Bad field identifier", @2);
            $$ = telldata::tn_NULL;
         }
      }
      delete [] $2;
   }
;

listindex:
     variable '[' expression ']'    {
      if (parsercmd::ListIndexCheck($1, @1, $3, @3))
      {
         $$ = ($1 & (~telldata::tn_listmask));
      }
      else {
         $$ = telldata::tn_NULL;
      }
    }
;

listinsert:
     variable '[' tknPREADD expression ']'    {
      if (parsercmd::ListIndexCheck($1, @1, $4, @4))
      {
         listadd_command = DEBUG_NEW parsercmd::cmdLISTADD(tellvar,true, true);
         $$ = $1; tell_lvalue = tellvar; lindexed = true;
      }
      else {
         $$ = telldata::tn_NULL; listadd_command = NULL;
      }
    }
   | variable '[' tknPREADD ']'               {
      if ($1 & telldata::tn_listmask) {
         listadd_command = DEBUG_NEW parsercmd::cmdLISTADD(tellvar,true, false);
         $$ = $1; tell_lvalue = tellvar; lindexed = true;
      }
      else {
         tellerror("list expected",@1);
         $$ = telldata::tn_NULL; listadd_command = NULL;
      }
    }
   | variable '[' expression tknPOSTADD ']'   {
      if (parsercmd::ListIndexCheck($1, @1, $3, @3))
      {
         listadd_command = DEBUG_NEW parsercmd::cmdLISTADD(tellvar,false, true);
         $$ = $1; tell_lvalue = tellvar; lindexed = true;
      }
      else {
         $$ = telldata::tn_NULL; listadd_command = NULL;
      }
    }
   | variable '[' tknPOSTADD ']'              {
      if ($1 & telldata::tn_listmask) {
         listadd_command = DEBUG_NEW parsercmd::cmdLISTADD(tellvar,false, false);
         $$ = $1; tell_lvalue = tellvar; lindexed = true;
      }
      else {
         tellerror("list expected",@1);
         $$ = telldata::tn_NULL; listadd_command = NULL;
      }
    }
;

listremove:
     variable '[' tknPRESUB expression ']'    {
      if (parsercmd::ListIndexCheck($1, @1, $4, @4))
         CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdLISTSUB(tellvar,true, true));
      $$ = ($1 & (~telldata::tn_listmask));
    }
   | variable '[' tknPRESUB ']'               {
      if ($1 & telldata::tn_listmask)
         CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdLISTSUB(tellvar,true, false));
      else
         tellerror("list expected",@1);
      $$ = ($1 & (~telldata::tn_listmask));
    }
   | variable '[' expression tknPOSTSUB ']'   {
      if (parsercmd::ListIndexCheck($1, @1, $3, @3))
         CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdLISTSUB(tellvar,false, true));
      $$ = ($1 & (~telldata::tn_listmask));
    }
   | variable '[' tknPOSTSUB ']'              {
      if ($1 & telldata::tn_listmask)
         CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdLISTSUB(tellvar,false, false));
      else
         tellerror("list expected",@1);
      $$ = ($1 & (~telldata::tn_listmask));
    }
;

listslice:
     variable '[' expression tknPRESUB expression ']'    {
      if (parsercmd::ListSliceCheck($1, @1, $5, @5, $3, @3))
      {
         CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdLISTSLICE(tellvar, true, true));
         $$ = $1;
      }
      else
         $$ = telldata::tn_NULL;
    }
   | variable '[' expression tknPRESUB ']'    {
      if (parsercmd::ListSliceCheck($1, @1, $3, @3))
      {
         CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdLISTSLICE(tellvar, false, false));
         $$ = $1;
      }
      else
         $$ = telldata::tn_NULL;
    }
   | variable '[' expression tknPOSTSUB expression ']'   {
      if (parsercmd::ListSliceCheck($1, @1, $3, @3, $5, @5))
      {
         CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdLISTSLICE(tellvar, false, true));
         $$ = $1;
      }
      else
         $$ = telldata::tn_NULL;
    }
   | variable '[' tknPOSTSUB  expression ']'   {
      if (parsercmd::ListSliceCheck($1, @1, $4, @4))
      {
         CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdLISTSLICE(tellvar, true, false));
         $$ = $1;
      }
      else
         $$ = telldata::tn_NULL;
    }
;

structure:
     '{'                                  {
        argmap = DEBUG_NEW telldata::argumentQ;
        argmapstack.push(argmap);
   }
      arguments '}'                       {
        /*Important note!. Here we will get a list of components that could be
          a tell list or some kind of tell struct or even tell list of tell struct.
          There is no way at this moment to determine the type of the input structure
          for (seems) obvious reasons. So - the type check and the eventual pushcmd
          are postponed untill we get the recepient - i.e. the lvalue or the
          function call. $$ is assigned to argumentID, that caries the whole argument
          queue listed in structure*/
        parsercmd::cmdSTRUCT* struct_command = DEBUG_NEW parsercmd::cmdSTRUCT();
        CMDBlock->pushcmd(struct_command);
        $$ = DEBUG_NEW telldata::argumentID(argmap, struct_command);
        //argQClear(argmap);
        argmapstack.pop();
        delete argmap;
        if (argmapstack.size() > 0) argmap = argmapstack.top();
        else argmap = NULL;
   }
;

anonymousvar:
     telltypeID   structure               {
      telldata::argumentID* op2 = $2;
      // the structure is without a type at this moment, so here we do the type checking
      if (parsercmd::StructTypeCheck($1, op2, @2)) {
         tellvar = CMDBlock->newTellvar($1, @1);
         parsercmd::Assign(tellvar, false, $2, @2);
         delete $2;
         $$ = $1;
      }
      else {
         tellerror("Type mismatch", @2);
         $$ = telldata::tn_NULL;
         delete $2;
         cleanonabort();
         YYABORT;
      }
   }
;

/*==EXPRESSION===============================================================*/
/*orexpression*/
expression : 
     andexpression                         {$$ = $1;}
   | expression tknOR   andexpression      {$$ = parsercmd::BoolEx($1,$3,"||",@1,@2);}
   | expression tknBWOR andexpression      {$$ = parsercmd::BoolEx($1,$3,"|",@1,@2);}
;

andexpression :
     eqexpression                          {$$ = $1;}
   | andexpression tknAND   eqexpression   {$$ = parsercmd::BoolEx($1,$3,"&&",@1,@2);}
   | andexpression tknBWAND eqexpression   {$$ = parsercmd::BoolEx($1,$3, "&",@1,@2);}
;

eqexpression :
     relexpression                         {$$ = $1;}
   | eqexpression tknEQ relexpression      {$$ = parsercmd::BoolEx($1,$3,"==",@1,@2);}
   | eqexpression tknNEQ relexpression     {$$ = parsercmd::BoolEx($1,$3,"!=",@1,@2);}
;

relexpression :
     addexpression                         {$$ = $1;}
   | relexpression '<' addexpression       {$$ = parsercmd::BoolEx($1,$3,"<",@1,@2);}
   | relexpression '>' addexpression       {$$ = parsercmd::BoolEx($1,$3,">",@1,@2);}
   | relexpression tknLEQ addexpression    {$$ = parsercmd::BoolEx($1,$3,"<=",@1,@2);}
   | relexpression tknGEQ addexpression    {$$ = parsercmd::BoolEx($1,$3,">=",@1,@2);}
;

addexpression: 
     multiexpression                       {$$ = $1;}
   | addexpression '+' multiexpression     {$$ = parsercmd::Plus($1,$3,@1,@3);}
   | addexpression '-' multiexpression     {$$ = parsercmd::Minus($1,$3,@1,@3);}
   | addexpression tknNE multiexpression   {$$ = parsercmd::PointMv($1,$3,@1,@3,+1,+1);}
   | addexpression tknNW multiexpression   {$$ = parsercmd::PointMv($1,$3,@1,@3,-1,+1);}
   | addexpression tknSE multiexpression   {$$ = parsercmd::PointMv($1,$3,@1,@3,+1,-1);}
   | addexpression tknSW multiexpression   {$$ = parsercmd::PointMv($1,$3,@1,@3,-1,-1);}
;

multiexpression : 
     unaryexpression                       {$$ = $1;}
   | multiexpression '*' unaryexpression   {$$ = parsercmd::Multiply($1,$3,@1,@3);}
   | multiexpression '/' unaryexpression   {$$ = parsercmd::Divide($1,$3,@1,@3);}
;

unaryexpression : 
     primaryexpression	                   {$$ = $1;}
   | '-' primaryexpression                 {$$ = parsercmd::UMinus($2,@2);}
   | tknNOT   primaryexpression            {$$ = parsercmd::BoolEx($2, "!",@2);}
   | tknBWNOT primaryexpression            {$$ = parsercmd::BoolEx($2, "~",@2);}
;

primaryexpression : 
     tknREAL                               {$$ = telldata::tn_real;
      CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdPUSH(DEBUG_NEW telldata::ttreal($1), false, true));}
   | tknINT                                {$$ = telldata::tn_int;
      CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdPUSH(DEBUG_NEW telldata::ttint($1), false, true));}
   | tknTRUE                               {$$ = telldata::tn_bool;
      CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdPUSH(DEBUG_NEW telldata::ttbool(true), false, true));}
   | tknFALSE                              {$$ = telldata::tn_bool;
      CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdPUSH(DEBUG_NEW telldata::ttbool(false), false, true));}
   | tknSTRING                             {$$ = telldata::tn_string;
      CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdPUSH(DEBUG_NEW telldata::ttstring($1), false, true));
                                                                delete [] $1;}
   | variable                              {$$ = $1;
      CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdPUSH(tellvar, indexed));}
   | anonymousvar                          {$$ = $1;
      CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdPUSH(tellvar, false, true));}
   | listremove                            {$$ = $1; indexed = false;}
   | listslice                             {$$ = $1; indexed = false;}
   | '(' expression ')'                    {$$ = $2;}
   | funccall                              {$$ = $1;}
   | tknERROR                              {tellerror("Unexpected symbol", @1);}
;

%%
/*-------------------------------------------------------------------------*/
int tellerror (char *s)
{  /* Called by yyparse on error */
   std::ostringstream ost;
   ost << "line " << telllloc.first_line << ": col " << telllloc.first_column
       << ": " << s;
   tell_log(console::MT_ERROR,ost.str());
   return 0;
}

void tellerror (std::string s, YYLTYPE loc) 
{
   if (cfd) cfd->incErrors();
   else     yynerrs++;
   std::ostringstream ost;
   ost << "line " << loc.first_line << ": col " << loc.first_column << ": ";
   if (loc.filename) {
      std::string fn = loc.filename;
      ost << "in file \"" << fn << "\" : ";
   }
   ost << s;
   tell_log(console::MT_ERROR,ost.str());
}

void tellerror (std::string s)
{
   if (cfd) cfd->incErrors();
   else     yynerrs++;
   std::ostringstream ost;
   ost << "line " << telllloc.first_line << ": col " << telllloc.first_column << ": " << s;
   tell_log(console::MT_ERROR,ost.str());
}

void cleanonabort()
{
   CMDBlock = CMDBlock->cleaner();
   parsercmd::EOfile();
   if (tellvar)         {delete tellvar;        tellvar           = NULL;}
   if (tell_lvalue)     {delete tell_lvalue;    tell_lvalue       = NULL;}
   if (foreach_command) {delete foreach_command;foreach_command   = NULL;}
   if (listadd_command) {delete listadd_command;listadd_command   = NULL;}
   if (tellstruct)      {delete tellstruct;     tellstruct        = NULL;}
   if (argmap)          {delete argmap;         argmap            = NULL;}
   if (cfd)             {delete cfd;            cfd               = NULL;}
}

/*
%type <pblock>         block
    pblock      - that is actually the list structure type where all commands
                  are stored for future execution. block is the only predicate
                  of this type
block:
     '{'                                   {
         CMDBlock = DEBUG_NEW parsercmd::cmdBLOCK();
         CMDBlock->pushblk();
      }
     statements '}'                        {
         $$ = CMDBlock;
         CMDBlock = CMDBlock->popblk();
      }
;
*/
/*
     tknIF '(' expression ')'       {
         CMDBlock = DEBUG_NEW parsercmd::cmdBLOCK();
         CMDBlock->pushblk();
     }
     blockstatement {
         parsercmd::cmdBLOCK* if_block = CMDBlock;
         CMDBlock = CMDBlock->popblk();
         if (telldata::tn_bool != $3) tellerror("bool type expected",@3);
         else CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdIFELSE(if_block, NULL));
      }

   | tknIF '(' expression ')'       {
         CMDBlock = DEBUG_NEW parsercmd::cmdBLOCK();
         CMDBlock->pushblk();
   }
   blockstatement tknELSE           {
         CMDBlock = DEBUG_NEW parsercmd::cmdBLOCK();
         CMDBlock->pushblk();
   }
   blockstatement                   {
         parsercmd::cmdBLOCK* else_block = CMDBlock;
         CMDBlock = CMDBlock->popblk();
         parsercmd::cmdBLOCK* if_block = CMDBlock;
         CMDBlock = CMDBlock->popblk();
         if (telldata::tn_bool != $3) tellerror("bool type expected",@3);
         else CMDBlock->pushcmd(DEBUG_NEW parsercmd::cmdIFELSE(if_block,else_block));
      }
;
*/
