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
//   This file is a part of Toped project (C) 2001-2006 Toped developers    =
// ------------------------------------------------------------------------ =
//           $URL$
//        Created: Sun Mar 17 2002
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: Toped prompt
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------                
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================
#include <string>
#include "ted_prompt.h"
#include <wx/regex.h>
#include "tell_yacc.h"

//-----------------------------------------------------------------------------
// Some external definitions
//-----------------------------------------------------------------------------
//??? I need help in this place
#ifdef WIN32
typedef unsigned int yy_size_t;
struct yy_buffer_state
	{
	FILE *yy_input_file;
	char *yy_ch_buf;		/* input buffer */
	char *yy_buf_pos;		/* current position in input buffer */
	yy_size_t yy_buf_size;
	int yy_n_chars;
	int yy_is_our_buffer;
	int yy_is_interactive;
	int yy_at_bol;
	int yy_fill_buffer;
	int yy_buffer_status;
	};
extern yy_buffer_state* tell_scan_string(const char *str);

#else
extern void* tell_scan_string(const char *str);
#endif

extern void my_delete_yy_buffer( void* b );
extern int tellparse(); // Calls the bison generated parser
extern YYLTYPE telllloc; // parser current location - global variable, defined in bison
wxMutex              Mutex;


extern const wxEventType    wxEVT_LOG_ERRMESSAGE;
console::ted_cmd*           Console = NULL;

//==============================================================================
bool console::patternFound(const wxString templ,  wxString str) {
   patternNormalize(str);
   wxRegEx src_tmpl(templ);
   assert(src_tmpl.IsValid());
   return src_tmpl.Matches(str);
}

void console::patternNormalize(wxString& str) {
   wxRegEx regex;
   // replace tabs with spaces
   assert(regex.Compile(wxT("\\t")));
   regex.ReplaceAll(&str,wxT(" "));
   // remove continious spaces
   assert(regex.Compile(wxT("[[:space:]]{2,}")));
   regex.ReplaceAll(&str,wxT(""));
   //remove leading spaces
   assert(regex.Compile(wxT("^[[:space:]]")));
   regex.ReplaceAll(&str,wxT(""));
   // remove trailing spaces
   assert(regex.Compile(wxT("[[:space:]]$")));
   regex.ReplaceAll(&str,wxT(""));
   //remove spaces before brackets and separators
   assert(regex.Compile(wxT("([[:space:]])([\\{\\}\\,\\-\\+])")));
   regex.ReplaceAll(&str,wxT("\\2"));
   // remove spaces after brackets and separators
   assert(regex.Compile(wxT("([\\{\\}\\,\\-\\+])([[:space:]])")));
   regex.ReplaceAll(&str,wxT("\\1"));

}

//==============================================================================
console::miniParser::miniParser(telldata::operandSTACK *cs,telldata::typeID et) {
   client_stack = cs;  _wait4type = et;
}

bool console::miniParser::getGUInput(wxString expression) {
   exp = expression;
   patternNormalize(exp);
   switch (_wait4type) {
      case         telldata::tn_pnt : return getPoint();
      case         telldata::tn_box : return getBox();
      case TLISTOF(telldata::tn_pnt): return getList();
               default: return false;// unexpected type
   }
}

bool console::miniParser::getPoint() {
   wxRegEx src_tmpl(point_tmpl);
   assert(src_tmpl.IsValid());
   // search the entire pattern
   if (!src_tmpl.Matches(exp)) return false;
   // get the coordinates
   assert(src_tmpl.Compile(real_tmpl));
   src_tmpl.Matches(exp);
   // first one...
   wxString p1s = src_tmpl.GetMatch(exp);
   // The expression is greedy and if you try to get simply the second match,
   // then, you might get the fractional part of the first coord as second 
   // coordinate, so remove and match again - not quie elegant, but works
   src_tmpl.ReplaceFirst(&exp,wxT(""));
   src_tmpl.Matches(exp);
   wxString p2s = src_tmpl.GetMatch(exp);
   double p1,p2;
   p1s.ToDouble(&p1);p2s.ToDouble(&p2);
   // convert the coordinates to ttpoint ...
   telldata::ttpnt* pp = new telldata::ttpnt(p1,p2);
   // and push it into the operand stack
   client_stack->push(pp);
   return true;
}

bool console::miniParser::getBox() {
   wxRegEx src_tmpl(box_tmpl);
   assert(src_tmpl.IsValid());
   // search the entire pattern
   if (!src_tmpl.Matches(exp)) return false;
   // remove the outside brackets
   assert(src_tmpl.Compile(wxT("^\\{{2}")));
   src_tmpl.ReplaceAll(&exp,wxT("{"));
   assert(src_tmpl.Compile(wxT("\\}{2}$")));
   src_tmpl.ReplaceAll(&exp,wxT("}"));
   // now we are going to extract the points
   assert(src_tmpl.Compile(point_tmpl));
   telldata::ttpnt pp[2];
   for (int i = 0; i < 2; i++) {
      if (!src_tmpl.Matches(exp)) return false;
      wxString ps = src_tmpl.GetMatch(exp);
      src_tmpl.ReplaceFirst(&exp,wxT(""));
      
      wxRegEx crd_tmpl(real_tmpl);
      assert(crd_tmpl.IsValid());
      crd_tmpl.Matches(ps);
      wxString p1s = crd_tmpl.GetMatch(ps);
      crd_tmpl.ReplaceFirst(&ps,wxT(""));
      crd_tmpl.Matches(ps);
      wxString p2s = crd_tmpl.GetMatch(ps);
      double p1,p2;
      p1s.ToDouble(&p1);p2s.ToDouble(&p2);
      // convert the coordinates to ttpoint ...
      pp[i] = telldata::ttpnt(p1,p2);
   }
   client_stack->push(new telldata::ttwnd(pp[0],pp[1]));
   return true;
}

bool console::miniParser::getList() {
   wxRegEx src_tmpl(pointlist_tmpl);
   assert(src_tmpl.IsValid());
   // search the entire pattern
   if (!src_tmpl.Matches(exp)) return false;
   // remove the outside brackets
   assert(src_tmpl.Compile(wxT("^\\{")));
   src_tmpl.ReplaceAll(&exp,wxT(""));    
   assert(src_tmpl.Compile(wxT("\\}$")));
   src_tmpl.ReplaceAll(&exp,wxT(""));    
   // now we are going to extract the points
   assert(src_tmpl.Compile(point_tmpl));
   telldata::ttlist *pl = new telldata::ttlist(telldata::tn_pnt);
   telldata::ttpnt* pp = NULL;
   while (src_tmpl.Matches(exp)) {
      wxString ps = src_tmpl.GetMatch(exp);
      src_tmpl.ReplaceFirst(&exp,wxT(""));
      wxRegEx crd_tmpl(real_tmpl);
      assert(crd_tmpl.IsValid());
      crd_tmpl.Matches(ps);
      wxString p1s = crd_tmpl.GetMatch(ps);
      crd_tmpl.ReplaceFirst(&ps,wxT(""));
      crd_tmpl.Matches(ps);
      wxString p2s = crd_tmpl.GetMatch(ps);
      double p1,p2;
      p1s.ToDouble(&p1);p2s.ToDouble(&p2);
      pp = new telldata::ttpnt(p1,p2);
      // add it to the point list
      pl->add(pp);
   }
   // and push it into the operand stack
   client_stack->push(pl);
   return true;
}

//==============================================================================
void* console::parse_thread::Entry() {
//   wxLogMessage(_T("Mouse is %s (%ld, %ld)"), where.c_str(), x, y);
//   wxLogMessage(_T("Mutex try to lock..."));
   while (wxMUTEX_NO_ERROR != Mutex.TryLock());
//   wxLogMessage(_T("Mutex locked!"));
   
   telllloc.first_column = telllloc.first_line = 1;
   telllloc.last_column  = telllloc.last_line  = 1;
   telllloc.filename = NULL;
   void* b = tell_scan_string( command.mb_str() );
   tellparse();
   my_delete_yy_buffer( b );
   
   Mutex.Unlock();
//   wxLogMessage(_T("Mutex unlocked"));
   return NULL;
};

//==============================================================================
// The ted_cmd event table
BEGIN_EVENT_TABLE( console::ted_cmd, wxTextCtrl )
   EVT_TEXT_ENTER(wxID_ANY, ted_cmd::getCommand)
   EVT_KEY_UP(ted_cmd::OnKeyUP)
END_EVENT_TABLE()

//==============================================================================
console::ted_cmd::ted_cmd(wxWindow *parent) :
      wxTextCtrl( parent, -1, wxT(""), wxDefaultPosition, wxDefaultSize,
                  wxTE_PROCESS_ENTER | wxNO_BORDER), puc(NULL), _numpoints(0) {
   
   threadWaits4 = new wxCondition(Mutex);
   assert(threadWaits4->IsOk());
   _mouseIN_OK = true;
   Console = this;
   _history_position = _cmd_history.begin();
};

void console::ted_cmd::getCommand(wxCommandEvent& WXUNUSED(event)) {
   if (puc)  getGUInput(); // run the local GUInput parser
   else {
      wxString command = GetValue();
      tell_log(MT_COMMAND, command);
      _cmd_history.push_back(std::string(command.mb_str()));
      _history_position = _cmd_history.end();
      Clear();
      parse_thread *pthrd = new parse_thread(command);
      pthrd->Create();
      pthrd->Run();
   }   
}

void console::ted_cmd::getCommandA() {
   if (puc)  getGUInput(); // run the local GUInput parser
   else {
      wxString command = GetValue();
      tell_log(MT_COMMAND, command);
      _cmd_history.push_back(std::string(command.mb_str()));
      _history_position = _cmd_history.end();
      Clear();
      if (!_thread)
      { // executing the parser without thread
         // essentially the same code as in parse_thread::Entry, but
         // without the mutexes
         telllloc.first_column = telllloc.first_line = 1;
         telllloc.last_column  = telllloc.last_line  = 1;
         telllloc.filename = NULL;
         void* b = tell_scan_string( command.mb_str() );
         tellparse();
         my_delete_yy_buffer( b );
      }
      else
      {
         // executing the parser in a separate thread
         //wxTHREAD_JOINABLE, wxTHREAD_DETACHED
         parse_thread *pthrd = new parse_thread(command);
         wxThreadError result = pthrd->Create();
         if (wxTHREAD_NO_ERROR == result)
            pthrd->Run();
         else
         {
            tell_log( MT_ERROR, "Can't execute the command in a separate thread");
            delete(pthrd);
         }
//         if (_wait) pthrd->Wait();
      }
   }
}

void console::ted_cmd::OnKeyUP(wxKeyEvent& event) {

   if ((WXK_UP != event.GetKeyCode()) &&  (WXK_DOWN != event.GetKeyCode())) {
      event.Skip();return;
   }
   if (WXK_DOWN != event.GetKeyCode())
      if (_cmd_history.begin() == _history_position)
         _history_position = _cmd_history.end();
      else _history_position--;
   else 
      if (_cmd_history.end() == _history_position)
         _history_position = _cmd_history.begin();
      else _history_position++;
      if (_cmd_history.end() == _history_position) SetValue(wxT(""));
   else 
   {
      SetValue(wxString(_history_position->c_str(), wxConvUTF8));
   }
}

void console::ted_cmd::parseCommand(wxString cmd, bool thread) {
   _thread = thread;
   if (NULL != puc) return; // don't accept commands during shape input sessions
   SetValue(cmd);
   getCommandA();
}
   

void console::ted_cmd::waitGUInput(telldata::operandSTACK *clst,telldata::typeID ttype) {
   puc = new miniParser(clst, ttype);_numpoints = 0;
   _mouseIN_OK = true;
   _guinput.Clear();
   tell_log(MT_GUIPROMPT);
   Connect(-1, wxEVT_COMMAND_ENTER,
           (wxObjectEventFunction) (wxEventFunction)
           (wxCommandEventFunction)&ted_cmd::OnGUInput);
}

void console::ted_cmd::getGUInput(bool from_keyboard) {
   wxString command;
   if (from_keyboard) { // input is from keyboard
      command = GetValue();
      tell_log(MT_GUIINPUT, command);
      tell_log(MT_EOL);
      Clear();
   }   
   else   command = _guinput;
   //parse the data from the prompt
   if (puc->getGUInput(command)) {
      //if the proper pattern was found
      Disconnect(-1, wxEVT_COMMAND_ENTER);
      delete puc; puc = NULL;
      _mouseIN_OK = true;
      // wake-up the thread expecting this data
      threadWaits4->Signal();
   }
   else {
      tell_log(MT_ERROR, "Bad input data, Try again...");
      tell_log(MT_GUIPROMPT);
   }
   _guinput.Clear();
   _numpoints = 0;
}

void console::ted_cmd::OnGUInput(wxCommandEvent& evt) {
   switch (evt.GetInt()) {
      case -2: cancelLastPoint();break;
      case -1:   // abort current  mouse input
         Disconnect(-1, wxEVT_COMMAND_ENTER);
         delete puc; puc = NULL;
         _mouseIN_OK = false;
         tell_log(MT_WARNING, "input aborted");
         tell_log(MT_EOL);
         // wake-up the thread expecting this data
         threadWaits4->Signal();
         break;
      case  0:  {// left mouse button
         telldata::ttpnt* p = static_cast<telldata::ttpnt*>(evt.GetClientData());
         mouseLB(*p);
         break;
         }
      case  2: mouseRB(); break;
      default: assert(false);
   }
}

void console::ted_cmd::mouseLB(const telldata::ttpnt& p) {
   wxString ost1, ost2;
   // prepare the point string for the input log window
   ost1 << wxT("{ ")<< p.x() << wxT(" , ") << p.y() << wxT(" }");
   // take care about the entry brackets ...
   if (_numpoints == 0) 
      switch (puc->wait4type()) {
         case TLISTOF(telldata::tn_pnt):
         case         telldata::tn_box : ost2 << wxT("{ ") << ost1; break;
         default                       : ost2 << ost1;
      }
   // ... and separators between the points
   else ost2 << wxT(" , ") << ost1;
   // print the current point in the log window
   tell_log(MT_GUIINPUT, ost2);
   // and update the current input string
   _guinput << ost2;
   // actualize the number of points entered
   _numpoints++;
   // If there is nothing else to wait ... call end of mouse input
   if ( (_numpoints == 1) && (telldata::tn_pnt == puc->wait4type())
     || (_numpoints == 2) && (telldata::tn_box == puc->wait4type())) mouseRB();
}

void console::ted_cmd::mouseRB() {
   // End of input is not accepted if ... 
   if ( (_numpoints == 0) 
      ||((_numpoints == 1) && (telldata::tn_pnt != puc->wait4type()))) return;
   // put the proper closing bracket
   wxString close;
   switch (puc->wait4type()) {
      case TLISTOF(telldata::tn_pnt):
      case         telldata::tn_box : close = wxT(" }"); break;
      default         : close = wxT("")  ;
   }
   // print it
   tell_log(MT_GUIINPUT, close);
   tell_log(MT_EOL);
   // and update the current input string
   _guinput << close;
   // and go parse it
   getGUInput(false); // from GUI
   _guinput.Clear();
}

void console::ted_cmd::cancelLastPoint() {
   tell_log(MT_WARNING, "last point canceled");
   int pos = _guinput.Find('{',true);
   _guinput = _guinput.Left(pos-2);
   if (_numpoints > 0) _numpoints--;
   tell_log(MT_GUIPROMPT);
   tell_log(MT_GUIINPUT, _guinput);
}

console::ted_cmd::~ted_cmd() {
   delete threadWaits4; _cmd_history.clear();
}
