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
//                                                                          =
//   This file is a part of Toped project (C) 2001-2007 Toped developers    =
// ------------------------------------------------------------------------ =
//           $URL$
//        Created: Fri Jan 24 2003
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: Implementation of all TELL build-in functions
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================

#include "tpdph.h"
#include <math.h>
#include <sstream>
#include "tellibin.h"
#include "../tpd_parser/ted_prompt.h"
#include "../tpd_DB/tedat.h"
#include "../tpd_DB/datacenter.h"
#include "../tpd_DB/browsers.h"
#include "../tpd_DB/viewprop.h"
#include "../tpd_common/tuidefs.h"

extern DataCenter*               DATC;
extern console::toped_logfile    LogFile;
extern console::ted_cmd*         Console;

extern wxFrame*                  TopedMainW;
extern wxWindow*                 TopedCanvasW;


extern const wxEventType         wxEVT_SETINGSMENU;
extern const wxEventType         wxEVT_CANVAS_ZOOM;
extern const wxEventType         wxEVT_MOUSE_INPUT;
extern const wxEventType         wxEVT_CANVAS_CURSOR;


//=============================================================================
int tellstdfunc::stdECHO::argsOK(argumentQ* amap) {
   return (!(amap->size() == 1));
}

nameList* tellstdfunc::stdECHO::callingConv(const telldata::typeMAP*) {
   nameList* argtypes = DEBUG_NEW nameList();
   argtypes->push_back("void");
   argtypes->push_back("<...anything...>");
   return argtypes;
}

int tellstdfunc::stdECHO::execute() {
   real DBscale = DATC->DBscale();
   telldata::tell_var *p = OPstack.top();OPstack.pop();
   std::string news;
   p->echo(news, DBscale);
   tell_log(console::MT_INFO,news);
   delete p;
   return EXEC_NEXT;
}

//=============================================================================
tellstdfunc::stdTELLSTATUS::stdTELLSTATUS(telldata::typeID retype, bool eor) :
      cmdSTDFUNC(DEBUG_NEW parsercmd::argumentLIST,retype,eor)
{}

int tellstdfunc::stdTELLSTATUS::execute() {
   real DBscale = DATC->DBscale();
   telldata::tell_var *y;
   std::string news;
   while (OPstack.size()) {
      y = OPstack.top(); OPstack.pop();
      y->echo(news, DBscale);
      tell_log(console::MT_ERROR,news);
   }
   news = "Bottom of the operand stack reached";
   tell_log(console::MT_ERROR,news);
   return EXEC_NEXT;
}

//=============================================================================
tellstdfunc::stdDISTANCE::stdDISTANCE(telldata::typeID retype, bool eor) :
      cmdSTDFUNC(DEBUG_NEW parsercmd::argumentLIST,retype,eor)
{
   arguments->push_back(DEBUG_NEW argumentTYPE("", DEBUG_NEW telldata::ttlist(telldata::tn_pnt)));
}

int tellstdfunc::stdDISTANCE::execute()
{
   // get the data from the stack
   telldata::ttlist *pl = static_cast<telldata::ttlist*>(OPstack.top());OPstack.pop();

   telldata::ttpnt* p1 = NULL;
   telldata::ttpnt* p2 = NULL;
   for (unsigned i = 0; i < pl->size(); i++) {
      p1 = p2;
      p2 = static_cast<telldata::ttpnt*>((pl->mlist())[i]);
      if (NULL != p1)
      {
         TP ap1 = TP(p1->x(),p1->y(), DATC->DBscale());
         TP ap2 = TP(p2->x(),p2->y(), DATC->DBscale());
         DATC->addRuler(ap1,ap2);
/*         std::ostringstream info;
         info << "Distance {" << p1->x() << " , " << p1->y() <<"}  -  {"
                              << p2->x() << " , " << p2->y() <<"}  is ";
         tell_log(console::MT_WARNING,info.str());*/
      }
   }
   delete pl;
   RefreshGL();
   return EXEC_NEXT;
}

//=============================================================================
tellstdfunc::stdDISTANCE_D::stdDISTANCE_D(telldata::typeID retype, bool eor) :
      stdDISTANCE(DEBUG_NEW parsercmd::argumentLIST,retype,eor)
{
}

int tellstdfunc::stdDISTANCE_D::execute()
{
   // generally there is no point having rulers without having
   // active database and cell, besides rulers appears funny on the 
   // screen because of the default settings of the layout canvas
   // That's the reason to have the lock/unlock here, i.e. to get an exception
   // if there is no active database & cell. This is done for the interacive
   // version only
   DATC->lockDB();
   DATC->unlockDB();
   // stop the thread and wait for input from the GUI
   if (!tellstdfunc::waitGUInput(console::op_line, &OPstack)) return EXEC_ABORT;
   // get the data from the stack
   telldata::ttwnd *w = static_cast<telldata::ttwnd*>(OPstack.top());OPstack.pop();
   telldata::ttlist* plst = DEBUG_NEW telldata::ttlist(telldata::tn_pnt);
   plst->add(DEBUG_NEW telldata::ttpnt(w->p1().x(), w->p1().y()));
   plst->add(DEBUG_NEW telldata::ttpnt(w->p2().x(), w->p2().y()));
   OPstack.push(plst);
   delete w;
   return stdDISTANCE::execute();

}

//=============================================================================
tellstdfunc::stdCLEARRULERS::stdCLEARRULERS(telldata::typeID retype, bool eor) :
      cmdSTDFUNC(DEBUG_NEW parsercmd::argumentLIST,retype,eor)
{}

int tellstdfunc::stdCLEARRULERS::execute()
{
   DATC->clearRulers();
   RefreshGL();
   return EXEC_NEXT;
}

//=============================================================================
tellstdfunc::stdLONGCURSOR::stdLONGCURSOR(telldata::typeID retype, bool eor) :
      cmdSTDFUNC(DEBUG_NEW parsercmd::argumentLIST,retype,eor)
{
   arguments->push_back(DEBUG_NEW argumentTYPE("", DEBUG_NEW telldata::ttbool()));
}

int tellstdfunc::stdLONGCURSOR::execute()
{
   bool        longcur  = getBoolValue();
   
   wxCommandEvent eventGRIDUPD(wxEVT_SETINGSMENU);
   eventGRIDUPD.SetInt((longcur ? tui::STS_LONG_CURSOR : tui::STS_SHORT_CURSOR));
   wxPostEvent(TopedMainW, eventGRIDUPD);

   wxCommandEvent eventCNVS(wxEVT_CANVAS_CURSOR);
   eventCNVS.SetInt((longcur ? 1 : 0));
   wxPostEvent(TopedCanvasW, eventCNVS);

   LogFile << LogFile.getFN() << "(" << LogFile._2bool(longcur) << ");"; LogFile.flush();
   RefreshGL();
   return EXEC_NEXT;
}

//=============================================================================
tellstdfunc::stdUNDO::stdUNDO(telldata::typeID retype, bool eor) :
      cmdSTDFUNC(DEBUG_NEW parsercmd::argumentLIST,retype,eor)
{}

int tellstdfunc::stdUNDO::execute() {
   if (UNDOcmdQ.size() > 0) {
      UNDOcmdQ.front()->undo(); UNDOcmdQ.pop_front();
      LogFile << LogFile.getFN() << "();"; LogFile.flush();
   }
   else {
      std::string news = "UNDO buffer is empty";
      tell_log(console::MT_ERROR,news);
   }   
   return EXEC_NEXT;
}

//=============================================================================
tellstdfunc::stdREDRAW::stdREDRAW(telldata::typeID retype, bool eor) :
      cmdSTDFUNC(DEBUG_NEW parsercmd::argumentLIST,retype,eor)
{}

int tellstdfunc::stdREDRAW::execute()
{
   RefreshGL();
   return EXEC_NEXT;
}

//=============================================================================
tellstdfunc::stdZOOMWIN::stdZOOMWIN(telldata::typeID retype, bool eor) :
      cmdSTDFUNC(DEBUG_NEW parsercmd::argumentLIST,retype,eor)
{
   arguments->push_back(DEBUG_NEW argumentTYPE("", DEBUG_NEW telldata::ttpnt()));
   arguments->push_back(DEBUG_NEW argumentTYPE("", DEBUG_NEW telldata::ttpnt()));
}

int tellstdfunc::stdZOOMWIN::execute() {
   telldata::ttpnt *p1 = static_cast<telldata::ttpnt*>(OPstack.top());OPstack.pop();
   telldata::ttpnt *p2 = static_cast<telldata::ttpnt*>(OPstack.top());OPstack.pop();
   real DBscale = DATC->DBscale();
   DBbox* box = DEBUG_NEW DBbox(TP(p1->x(), p1->y(), DBscale), 
                          TP(p2->x(), p2->y(), DBscale));
   wxCommandEvent eventZOOM(wxEVT_CANVAS_ZOOM);
   eventZOOM.SetInt(tui::ZOOM_WINDOW);
   eventZOOM.SetClientData(static_cast<void*>(box));
   wxPostEvent(TopedCanvasW, eventZOOM);
   return EXEC_NEXT;
}

//=============================================================================
tellstdfunc::stdZOOMWINb::stdZOOMWINb(telldata::typeID retype, bool eor) :
      stdZOOMWIN(DEBUG_NEW parsercmd::argumentLIST,retype, eor)
{
   arguments->push_back(DEBUG_NEW argumentTYPE("", DEBUG_NEW telldata::ttwnd()));
}

int tellstdfunc::stdZOOMWINb::execute() {
   telldata::ttwnd *w = static_cast<telldata::ttwnd*>(OPstack.top());OPstack.pop();
   real DBscale = DATC->DBscale();
   DBbox* box = DEBUG_NEW DBbox(TP(w->p1().x(), w->p1().y(), DBscale), 
                          TP(w->p2().x(), w->p2().y(), DBscale));
   wxCommandEvent eventZOOM(wxEVT_CANVAS_ZOOM);
   eventZOOM.SetInt(tui::ZOOM_WINDOW);
   eventZOOM.SetClientData(static_cast<void*>(box));
   wxPostEvent(TopedCanvasW, eventZOOM);
   return EXEC_NEXT;
}

//=============================================================================
tellstdfunc::stdZOOMALL::stdZOOMALL(telldata::typeID retype, bool eor) :
      cmdSTDFUNC(DEBUG_NEW parsercmd::argumentLIST,retype, eor)
{}

int tellstdfunc::stdZOOMALL::execute() {
   laydata::tdtdesign* ATDB = DATC->lockDB();
      DBbox* ovl  = DEBUG_NEW DBbox(ATDB->activeoverlap());
   DATC->unlockDB();
   wxCommandEvent eventZOOM(wxEVT_CANVAS_ZOOM);
   eventZOOM.SetInt(tui::ZOOM_WINDOW);
   eventZOOM.SetClientData(static_cast<void*>(ovl));
   wxPostEvent(TopedCanvasW, eventZOOM);
   return EXEC_NEXT;
}

//=============================================================================
tellstdfunc::getPOINT::getPOINT(telldata::typeID retype, bool eor) :
      cmdSTDFUNC(DEBUG_NEW parsercmd::argumentLIST,retype,eor)
{}

int tellstdfunc::getPOINT::execute() {
   // Here - try a hollow lock/unlock the database just to check that it exists
   // The use of this function should be deprecated
   DATC->lockDB();
   DATC->unlockDB();
   // flag the prompt that we expect a single point & handle a pointer to
   // the operand stack
   Console->waitGUInput(&OPstack, console::op_point, CTM());
   // force the thread in wait condition until the ted_prompt has our data
   Console->threadWaits4->Wait();
   // ... and continue when the thread is woken up
   if (Console->mouseIN_OK())  return EXEC_NEXT;
   else return EXEC_RETURN;
}

//=============================================================================
tellstdfunc::getPOINTLIST::getPOINTLIST(telldata::typeID retype, bool eor) :
      cmdSTDFUNC(DEBUG_NEW parsercmd::argumentLIST,retype,eor)
{}

int tellstdfunc::getPOINTLIST::execute() {
   // Here - try a hollow lock/unlock the database just to check that it exists
   // The use of this function should be deprecated
   DATC->lockDB();
   DATC->unlockDB();
   // flag the prompt that we expect a list of points & handle a pointer to
   // the operand stack
   Console->waitGUInput(&OPstack, console::op_dpoly, CTM());
   // 
   wxCommandEvent eventMOUSEIN(wxEVT_MOUSE_INPUT);
   eventMOUSEIN.SetInt(-1);
   eventMOUSEIN.SetExtraLong(1);
   wxPostEvent(TopedCanvasW, eventMOUSEIN);
   // force the thread in wait condition until the ted_prompt has our data
   Console->threadWaits4->Wait();
   // ... and continue when the thread is woken up
   eventMOUSEIN.SetExtraLong(0);
   wxPostEvent(TopedCanvasW, eventMOUSEIN);
   if (Console->mouseIN_OK())  return EXEC_NEXT;
   else return EXEC_RETURN;
}

/*
UNDO/REDO operation - some preliminary thoughts
   Where to fit it. Two major possibilities here:
      - generating string commands and using the parser
      - Maintaining UNDO command stack and UNDO operand stack
   1. At this moment seems that the first way has two major drawback - it will 
   mess around with the threads. Undo will be a command that will have to 
   reinvoke the parser to parse another command. The other thing that seems 
   to be potentially hazardous is the maintanance of the operand stack - it 
   seems impossible  to execute properly undo/redo if it is included somewhere
   in a function, script, file, preproc.definition or god knows where else in 
   the language.
   2. Separate UNDO command/operand stack. This solution seems much more robust
   (and of course more complicated to implement). At least it is free of the 
   troubles described above (though have others maybe...)
   The table below is trying to clarify operations over normal and UNDO operand
   stacks and when they are executed.
   ============================================================================
                    |    filled during        |         emptied
   ============================================================================
    CMD_STACK       |     parsing             |        execution
   ----------------------------------------------------------------------------
    UNDO CMD_STACK  | execution of UNDOable   | undo execution  OR reaching
                    |      command            |  the limit of UNDO CMD_STACK
   ----------------------------------------------------------------------------
    OP_STACK        |     execution           |        execution
   ----------------------------------------------------------------------------
    UNDO OP_STACK   | execution of UNDOable   |  undo execution  OR reaching
                    |      command            |  the limit of UNDO CMD_STACK
   ----------------------------------------------------------------------------
   What is needed for implementation...
    Additional method (UNDO) in the cmdSTDFUNC. Thus undo will be executed 
    in a similar way as normal commands, but one command at a time - when 
    undo() command is invoked - and calling undo() method instead of execute()
    This has a nasty drawback - all internal commands has to be implemented 
    separately and initialy this will swallow a lot of time. It seems to be
    flexible though and will require minimum amount of additional toped functions.
       

=========================================================================================
|   TELL command   |   tellibin class  |  Docu  |   LOG  |  UNDO  |        Note         |
=========================================================================================
|  "echo"          |  stdECHO          |        |    -   |   -    |                     |
|  "status"        |  stdTELLSTATUS    |        |    -   |   -    |                     |
|  "newdesign"     |  stdNEWDESIGN     |   OK   |    x   |   x*   | if not initial      |
|  "newcell"       |  stdNEWCELL       |   OK   |    x   |   x*   | if not initial      |
|  "gdsread"       |  GDSread          |   OK   |    x   |   -    |                     |
|  "importcell"    |  GDSimport       |   OK   |    x   |   -*   | issue a warning     |
|  "tdtread"       |  TDTread          |   OK   |    x   |   -*   | issue a warning     |
|  "tdtsave"       |  TDTsave          |   OK   |    x   |   -    |                     |
|  "tdtsaveas"     |  TDTsaveas        |   OK   |    x   |   -    |                     |
|  "opencell"      |  stdOPENCELL      |   OK   |    x   |   x*   | if not initial      |
|  "usinglayer"    |  stdUSINGLAYER    |   OK   |    x   |   x    |                     |
|  "addbox"        |  stdADDBOX        |   OK   |    x   |   x    |                     |
|  "addbox"        |  stdADDBOX_D      |   OK   |    x   |   x    |                     |
|  "addbox"        |  stdADDBOXr       |   OK   |    x   |   x    |                     |
|  "addbox"        |  stdADDBOXr_D     |   OK   |    x   |   x    |                     |
|  "addbox"        |  stdADDBOXp       |   OK   |    x   |   x    |                     |
|  "addbox"        |  stdADDBOXp_D     |   OK   |    x   |   x    |                     |
|  "addbox"        |  stdDRAWBOX       |   OK   |    x*  |   x    |   as add<blah-blah> |
|  "addbox"        |  stdDRAWBOX_D     |   OK   |    x*  |   x    |   as add<blah-blah> |
|  "addpoly"       |  stdADDPOLY       |   OK   |    x   |   x    |                     |
|  "addpoly"       |  stdADDPOLY_D     |   OK   |    x   |   x    |                     |
|  "addpoly"       |  stdDRAWPOLY      |   OK   |    x*  |   x    |   as add<blah-blah> |
|  "addpoly"       |  stdDRAWPOLY_D    |   OK   |    x*  |   x    |   as add<blah-blah> |
|  "addwire"       |  stdADDWIRE       |   OK   |    x   |   x    |                     |
|  "addwire"       |  stdADDWIRE_D     |   OK   |    x   |   x    |                     |
|  "addwire"       |  stdDRAWWIRE      |   OK   |    x*  |   x    |   as add<blah-blah> |
|  "addwire"       |  stdDRAWWIRE_D    |   OK   |    x*  |   x    |   as add<blah-blah> |
|  "addtext"       |  stdADDTEXT       |   OK   |    x   |   x    |                     |
|  "cellref"       |  stdCELLREF       |   OK   |    x   |   x    |                     |
|  "cellaref"      |  stdCELLAREF      |   OK   |    x   |   x    |                     |
|  "select"        |  stdSELECT        |   OK   |    x   |   x    |                     |
|  "select"        |  stdSELECT_I      |   OK   |    x   |   x    |                     |
|  "select"        |  stdSELECT_TL     |   OK   |    -*  |   x    |  see note 3         |
|  "pselect"       |  stdPNTSELECT     |   OK   |    x   |   x    |                     |
|  "pselect"       |  stdPNTSELECT_I   |   OK   |    x   |   x    |                     |
|  "unselect"      |  stdUNSELECT      |   OK   |    x   |   x    |                     |
|  "unselect"      |  stdUNSELECT_I    |   OK   |    x   |   x    |                     |
|  "unselect"      |  stdUNSELECT_TL   |   OK   |    -*  |   x    |  see note 3         |
|  "punselect"     |  stdPNTUNSELECT   |   OK   |    x   |   x    |                     |
|  "punselect"     |  stdPNTUNSELECT_I |   OK   |    x   |   x    |                     |
|  "select_all"    |  stdSELECTALL     |   OK   |    x   |   x    |                     |
|  "unselect_all"  |  stdUNSELECTALL   |   OK   |    x   |   x    |                     |
-----------------------------------------------------------------------------------------
|      operation on the toped data     |        |        |        |                     |
-----------------------------------------------------------------------------------------
|  "move"          |  stdMOVESEL       |   OK   |    x   |   x    |                     |
|  "move"          |  stdMOVESEL_D     |   OK   |    x   |   x    |                     |
|  "copy"          |  stdCOPYSEL       |   OK   |    x   |   x    |                     |
|  "copy"          |  stdCOPYSEL_D     |   OK   |    x   |   x    |                     |
|  "delete"        |  stdDELETESEL     |   OK   |    x   |   x    |                     |
|  "group"         |  stdGROUP         |   OK   |    x   |   x    |                     |
|  "ungroup"       |  stdUNGROUP       |   OK   |    x   |   x    |                     |
-----------------------------------------------------------------------------------------
|        toped specific functons       |        |        |        |                     |
-----------------------------------------------------------------------------------------
|  "getCW"         |  stdGETCW         |   OK   |    -   |   -    |                     |
|  "zoom"          |  stdZOOMWIN       |   OK   |    -   |   -    |  see note 1         |
|  "zoom"          |  stdZOOMWINb      |   OK   |    -   |   -    |  see note 1         |
|  "zoomall"       |  stdZOOMALL       |   OK   |    -   |   -    |  see note 1         |
|  "layprop"       |  stdLAYPROP       |   OK   |    x   |   x*   |  if redefined only  |
|  "hidelayer"     |  stdHIDELAYER     |   OK   |    x   |   x    |                     |
|  "locklayer"     |  stdLOCKLAYER     |   OK   |    x   |   x    |                     |
|  "filllayer"     |  stdFILLLAYER     |   OK   |    x   |   x    |                     |
|  "definecolor"   |  stdCOLORDEF      |   OK   |    x   |   x*   |  if redefined only  |
|  "definefill"    |  stdFILLDEF       |   OK   |    x   |   x*   |  if redefined only  |
|  "definegrid"    |  stdGRIDDEF       |   OK   |    x   |   x*   |  if redefined only  |
|  "step"          |  stdSTEP          |   OK   |    x   |   x*   |  if redefined only  |
|  "grid"          |  stdGRID          |   OK   |    x   |   x    |                     |
|  "getpoint"      |  getPOINT         |   OK   |    -   |   -    |  see note 2         |
|  "getpointlist"  |  getPOINTLIST     |   OK   |    -   |   -    |                     |
=========================================================================================
   Note 1: For zooming a separate stack of available views can be maintained.
   Note 2: For getpoint operation, a local undo should be maintained. Not here,
           but in the tedprompt maybe.
   Note 3: It is a real trouble how to log select/unselect from list! Have no
           idea really!!! Something really to think hard...
*/
