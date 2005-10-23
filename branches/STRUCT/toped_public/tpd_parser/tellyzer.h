//===========================================================================
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
//           $URL$
//  Creation date: Fri Nov 08 2002
//     Created by: Svilen Krustev - s_krustev@yahoo.com
//      Copyright: (C) 2001-2004 by Svilen Krustev
//    Description: wxWidget version
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------                
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================

#if !defined(TELLYZER_H_INCLUDED)
#define TELLYZER_H_INCLUDED

#include <stdio.h>
#include <string>
#include <map>
#include <deque>
#include <stack>
#include <fstream>
#include "tldat.h"

#define  EXEC_NEXT      0
#define  EXEC_RETURN    1

#define YYLTYPE parsercmd::yyltype

//-----------------------------------------------------------------------------
// Some forward declarations
//-----------------------------------------------------------------------------
int yylex(void);
int yyerror (char *s);

namespace parsercmd {
   class cmdVIRTUAL;
   class cmdSTDFUNC;
   class cmdBLOCK;

   struct yyltype {
      int          first_line;
      int          first_column;
      int          last_line;
      int          last_column;
      char*        filename;
   };
   // Used by lexer to include multiply files and for error tracing
   class lexer_files {
   public:
      lexer_files(void* fh, YYLTYPE* loc) : 
                                 lexfilehandler(fh), location(loc) {};
      ~lexer_files()                               {delete location;};
      void*    lexfilehandler;
      YYLTYPE* location;
   };

//-----------------------------------------------------------------------------
// Define the types of tell scructures
//-----------------------------------------------------------------------------
// tell_tn     - list of tell types - used for argument type checking
// variableMAP - a map (name - variable) structure used to accomodate the
//               tell variables
// functionMAP - a map (name - function block) structure conaining all defined
//               tell functions. There is only one defined variable of this
//               type funcMAP - static member of cmdBLOCK class.
// blockSTACK  - a stack strucure containing current blocks nesting. Only
//               one variable is defined of this class - blocks - static member
//               of cmdBLOCK.
// cmdQUEUE    - a queue structure containing the list of tell operators
//operandSTACK - a stack structure used for storing the operans and results.
//               A single static variable - OPstack - is defined in cmdVIRTUAL
//               class. Used during execution
// argumentTYPE- ???
// argumentLIST- ???
// argumentMAP - ???
//-----------------------------------------------------------------------------
   class argumentID;
   typedef  std::multimap<std::string, cmdSTDFUNC*>      functionMAP;
   typedef  std::deque<cmdBLOCK*>                        blockSTACK;
   typedef  std::deque<cmdVIRTUAL*>                      cmdQUEUE;
   typedef  std::deque<cmdSTDFUNC*>                      undoQUEUE;
   typedef  std::stack<telldata::tell_var*>              operandSTACK;
   typedef  std::deque<telldata::tell_var*>              UNDOPerandQUEUE;
   typedef  std::pair<std::string,telldata::tell_var*>   argumentTYPE;
   typedef  std::deque<argumentTYPE*>                    argumentLIST;
//   typedef  std::deque<telldata::typeID>                 argumentMAP;
   typedef  std::deque<argumentID>                       argumentQ;

   class argumentID {
   public:
                        argumentID(telldata::typeID ID = telldata::tn_NULL) :
                                                         _ID(ID), _child(NULL){};
                        argumentID(argumentQ* child);
                       ~argumentID() {if (NULL != _child) delete _child;}
      telldata::typeID  operator () () const        {return _ID;}
      void              addChild(argumentQ* child)  {_child = child;}
      argumentQ*        child() const               {return _child;};
   private:
      telldata::typeID  _ID;
      argumentQ*        _child;
   };

   /*** cmdVIRTUAL **************************************************************
   > virtual class inherited by all tell classes
   >>> Constructor --------------------------------------------------------------
   > -
   >>> Data fields --------------------------------------------------------------
   > OPstack				- Operand stack used to store operans and the results of
   >                      every executed operation
   >>> Methods ------------------------------------------------------------------
   > execute() 			- Execute the operator (or command)
   ******************************************************************************/
   class cmdVIRTUAL {
   public:
//      cmdVIRTUAL(yyltype loc): _loc(loc) {}
      virtual int  execute() = 0;
              real getOpValue(operandSTACK& OPs = OPstack);
              word getWordValue(operandSTACK& OPs = OPstack);
              byte getByteValue(operandSTACK& OPs = OPstack);
       std::string getStringValue(operandSTACK& OPs = OPstack);
              bool getBoolValue(operandSTACK& OPs = OPstack);
              real getOpValue(UNDOPerandQUEUE&, bool);
              word getWordValue(UNDOPerandQUEUE&, bool);
              byte getByteValue(UNDOPerandQUEUE&, bool);
       std::string getStringValue(UNDOPerandQUEUE&, bool);
              bool getBoolValue(UNDOPerandQUEUE&, bool);
      virtual ~cmdVIRTUAL() {};        
   protected:
      static operandSTACK      OPstack;      // Operand stack
//      yyltype                  _loc;
   };

   /*****************************************************************************
    The definition of the following classes is trivial
   ******************************************************************************/
   class cmdPLUS:public cmdVIRTUAL {
      int execute();
   };

   class cmdMINUS:public cmdVIRTUAL {
      int execute();
   };

   class cmdSHIFTPNT:public cmdVIRTUAL {
   public:
      cmdSHIFTPNT(int sign = 1):_sign(sign) {};
      int execute();
   private:
      int _sign;
   };

   class cmdSHIFTPNT2:public cmdVIRTUAL {
   public:
      cmdSHIFTPNT2(int sign = 1):_sign(sign) {};
      int execute();
   private:
      int _sign;
   };

   class cmdSHIFTBOX:public cmdVIRTUAL {
   public:
      cmdSHIFTBOX(int sign = 1):_sign(sign) {};
      int execute();
   private:
      int _sign;
   };

   class cmdMULTIPLY:public cmdVIRTUAL {
      int execute();
   };

   class cmdDIVISION:public cmdVIRTUAL {
      int execute();
   };

   class cmdUMINUS:public cmdVIRTUAL {
   public:
      cmdUMINUS(telldata::typeID type):_type(type) {};
      int execute();
   private:
      telldata::typeID  _type;
   };

   class cmdLT:public cmdVIRTUAL {
   public:
      int execute();
   };

   class cmdLET:public cmdVIRTUAL {
   public:
      int execute();
   };

   class cmdGT:public cmdVIRTUAL {
   public:
      int execute();
   };

   class cmdGET:public cmdVIRTUAL {
   public:
      int execute();
   };

   class cmdEQ:public cmdVIRTUAL {
   public:
      int execute();
   };

   class cmdNE:public cmdVIRTUAL {
   public:
      int execute();
   };

   class cmdAND:public cmdVIRTUAL {
   public:
      int execute();
   };

   class cmdOR:public cmdVIRTUAL {
   public:
      int execute();
   };

   class cmdSTACKRST:public cmdVIRTUAL {
   public:
      int execute();
   };

   class cmdASSIGN:public cmdVIRTUAL {
   public:
      cmdASSIGN(telldata::tell_var* var): _var(var) {};
      int execute();
//      ~cmdASSIGN() {delete _var;}
   protected:
      telldata::tell_var* _var;
   };

   class cmdPOINTFIELD:public cmdVIRTUAL {
   public:
      cmdPOINTFIELD(char* f, yyltype loc);
      int execute();
   private:
      char     _field;
   };

   class cmdWINDOWFIELD:public cmdVIRTUAL {
   public:
      cmdWINDOWFIELD(char* f, yyltype loc);
      int execute();
   private:
      char     _field;
   };

   class cmdPUSH:public cmdVIRTUAL {
   public:
      cmdPUSH(telldata::tell_var *v, bool constant=false):
                        _var(v),  _constant(constant) {};
      int execute();
      ~cmdPUSH() {if (_constant) delete _var;};
   private:
      telldata::tell_var* _var;
      bool                 _constant;
   };

   class cmdPOINT:public cmdVIRTUAL {
   public:
      int execute();
   };

   class cmdWINDOW:public cmdVIRTUAL {
   public:
      int execute();
   };

   class cmdLIST:public cmdVIRTUAL {
   public:
      cmdLIST(telldata::typeID ttn, unsigned length): _ttype(ttn), _length(length) {};
      int execute();
   private:
      telldata::typeID  _ttype;
      unsigned       _length;
   };

   class cmdRETURN:public cmdVIRTUAL {
   public:
      int execute()  {return EXEC_RETURN;};
   };

   class cmdFUNCCALL: public cmdVIRTUAL {
   public:
      cmdFUNCCALL(cmdSTDFUNC* bd, std::string fn):funcbody(bd), funcname(fn) {};
      int execute();
   protected:
      cmdSTDFUNC*       funcbody;
      std::string       funcname;
   };

   /*** cmdBLOCK ****************************************************************
   > The class defines the tell block of operators structure. It is the base
   > class for
   >>> Constructor --------------------------------------------------------------
   > -
   >>> Data fields --------------------------------------------------------------
   > cmdQ      - Contains the list of tell commands
   > VARlocal  - Conatains the map of the local variables
   > blocks    - static - structure of the nested blocks. Used during the parsing
   > funcMAP   - static - map of defined tell functions. No nested functions
                 allowed, so all of them are defined in the main block
   >>> Methods ------------------------------------------------------------------
   > addFUNC   -
   > addID     - add a new variable to the variableMAP structure. The new
                 variable is initialized. Called when an identifier is matched.
                 If VARlocal exists, the variable is added there, otherwise
                 VARglobal map is used
   > getID     - extracts tell_var* from variableMAP by name. Local variableMAP
                 has a priority(if exists). Returns NULL if such a name is not
                 found in both (VARlocal, VARglobal) maps.
   > getfuncID - extracts the cmdBLOCK* from the funcMAP by name. Returns NULL
                 if function name doesn't exist
   > pushcmd   - add the parsed command to the cmdQ queue
   > pushblock - push current block in the blocks stack structure
   > popblock  - returns (and removes) the top of the blocks structure
   > run       - override of the QThread::run(). The execution of the block 
                 called from the parser needs to start in a separate thread
                 because of the interactive functions.
   > cleaner   - clean the instruction que downto the previous ';'
   ******************************************************************************/
   class cmdBLOCK:public virtual cmdVIRTUAL {
   public:
                                 cmdBLOCK();
                                 cmdBLOCK(telldata::typeID lltID) :
                                                            _next_lcl_typeID(lltID){};
      int                        execute();
      void                       cleaner();
      virtual void               addFUNC(std::string, cmdSTDFUNC*);
      void                       addID(char*&, telldata::tell_var*);
      void                       addlocaltype(char*&, telldata::tell_type*);
      telldata::tell_type*       requesttypeID(char*&);
      const telldata::tell_type* getTypeByName(char*&) const;
//      telldata::typeID*          checkfield(telldata::typeID, char*&, yyltype) const;
      telldata::tell_var*        getID(char*&, bool local=false);
      telldata::tell_var*        newTellvar(telldata::typeID, yyltype);
      cmdSTDFUNC*  const         funcDefined(char*&,argumentLIST*) const;
      cmdSTDFUNC*  const         getFuncBody(char*&, argumentQ*) const;
      void                       pushcmd(cmdVIRTUAL* cmd) {cmdQ.push_back(cmd);};
      void                       pushblk()                {_blocks.push_front(this);};
      cmdBLOCK*                  popblk();
      functionMAP const          funcMAP() const {return _funcMAP;};
      virtual                   ~cmdBLOCK();
   protected:
      const telldata::tell_type* getTypeByID(const telldata::typeID ID) const;
      telldata::variableMAP      VARlocal;  // list of local variables
      telldata::typeMAP          TYPElocal; // list of local types
      cmdQUEUE                   cmdQ;      // list of commands
      static blockSTACK         _blocks;
      static functionMAP        _funcMAP;
      telldata::typeID          _next_lcl_typeID;
   };

   class cmdSTDFUNC:public virtual cmdVIRTUAL {
   public:
                              cmdSTDFUNC(argumentLIST* vm, telldata::typeID tt):
                                                   arguments(vm), returntype(tt) {};
      virtual int             execute() = 0;
      // Next method - not virtual to save some hassle writing a plenty of empty methods
      virtual void            undo() {}; 
      virtual void            undo_cleanup();
      virtual std::string     callingConv() = 0;
//      virtual void         add2LogFile() = 0;
      virtual int             argsOK(argumentQ* amap);
      telldata::typeID        gettype() const {return returntype;};
      virtual                ~cmdSTDFUNC();
   protected:
      argumentLIST*           arguments;
      telldata::typeID        returntype;
      static UNDOPerandQUEUE  UNDOPstack;   // undo operand stack
      static undoQUEUE        UNDOcmdQ;     // undo command stack
   };

   class cmdFUNC:public cmdSTDFUNC, public cmdBLOCK {
   public:
      cmdFUNC(argumentLIST* vm, telldata::typeID tt);
      std::string             callingConv();
//      void                 add2LogFile() {};
      int                     execute();
   };

   class cmdIFELSE: public cmdVIRTUAL {
   public:
      cmdIFELSE(cmdBLOCK* tb, cmdBLOCK* fb):trueblock(tb),falseblock(fb) {};
      int                     execute();
      ~cmdIFELSE() {delete trueblock; delete falseblock;}
   private:
      cmdBLOCK*               trueblock;
      cmdBLOCK*               falseblock;
   };

   class cmdWHILE: public cmdVIRTUAL {
   public:
      cmdWHILE(cmdBLOCK* cnd, cmdBLOCK* bd):condblock(cnd),body(bd) {};
      int                  execute();
      ~cmdWHILE() {delete condblock; delete body;}
   private:
      cmdBLOCK *condblock;
      cmdBLOCK *body;
   };

   class cmdREPEAT: public cmdVIRTUAL {
   public:
      cmdREPEAT(cmdBLOCK* cnd, cmdBLOCK* bd):condblock(cnd),body(bd) {};
      int                  execute();
      ~cmdREPEAT() {delete condblock; delete body;}
   private:
      cmdBLOCK *condblock;
      cmdBLOCK *body;
   };

   class cmdMAIN:public cmdBLOCK {
   public:
      cmdMAIN();
      int   execute();
      void  addFUNC(std::string fname , cmdSTDFUNC* cQ);
      ~cmdMAIN();
   };

//   telldata::typeID newBox(telldata::type, telldata::type, telldata::type,
//                            telldata::type, yyltype, yyltype, yyltype, yyltype);
//   telldata::typeID newPoint(telldata::type, telldata::type, yyltype, yyltype);
   telldata::typeID newDataStructure(telldata::typeID, telldata::typeID, yyltype, yyltype);
   telldata::typeID UMinus(telldata::typeID, yyltype);
   telldata::typeID   Plus(telldata::typeID, telldata::typeID, yyltype, yyltype);
   telldata::typeID  Minus(telldata::typeID, telldata::typeID, yyltype, yyltype);
   telldata::typeID  Multiply(telldata::typeID, telldata::typeID, yyltype, yyltype);
   telldata::typeID  Divide(telldata::typeID, telldata::typeID, yyltype, yyltype);
   telldata::typeID  Assign(telldata::tell_var*, argumentID*, yyltype);
   telldata::typeID BoolEx(telldata::typeID, telldata::typeID, std::string, yyltype, yyltype);

}   

namespace console{
   class toped_logfile {
      public:
         toped_logfile() {};
         ~toped_logfile();
         void              init();
         void              setFN(std::string fn) {_funcname = fn;};
         std::string       getFN() const {return _funcname;};
         std::string       _2bool(bool b) {return b ? "true" : "false";};
         toped_logfile&    operator<< (const byte _i);
         toped_logfile&    operator<< (const word _i);
         toped_logfile&    operator<< (const int4b _i);
         toped_logfile&    operator<< (const real _r);
         toped_logfile&    operator<< (const std::string& _s);
         toped_logfile&    operator<< (const telldata::ttpnt& _p);
         toped_logfile&    operator<< (const telldata::ttwnd& _w);
         toped_logfile&    operator<< (const telldata::ttlist& _tl);
         toped_logfile&    flush();
      private:
         std::fstream     _file;
         std::string      _funcname;
   };
}
#endif
