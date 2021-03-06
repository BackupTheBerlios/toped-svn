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
//        Created: Wed Dec 26 2001
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: Canvas control
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================

#ifndef LAYOUTCANVAS_H
#define LAYOUTCANVAS_H

#include <string>
#include <wx/glcanvas.h>
#include <wx/cursor.h>
#include <wx/image.h>
#include "../tpd_common/ttt.h"
#include "../tpd_common/tuidefs.h"

namespace tui {
   //=============================================================================
   class StatusLine
   {
      public:
                    StatusLine():_Xcoord(TP()),_Ycoord(TP()) {};
      void          update(const int4b, const CTM&);
      void          draw();
      void          update_coords(const TP&);
      private:
         TP             _sb_BL;
         TP             _sb_TR;
         double         _scaledpix;
         TP             _cX;
         TP             _cY;
         TP             _dX;
         TP             _dY;
         TP             _wcX;
         TP             _wcY;
         DBbox          _Xcoord;
         DBbox          _Ycoord;
         TP             _cp;
   };
   //=============================================================================
   class LayoutCanvas : public wxGLCanvas  {
   public: 
                     LayoutCanvas(wxWindow *parent, int* attribList);
      virtual       ~LayoutCanvas();
      wxImage        snapshot(void);
   protected:
      void           OnpaintGL(wxPaintEvent& event);
      void           OnresizeGL(wxSizeEvent& event);
      void           OnEraseBackground(wxEraseEvent&) {};
      void           OnMouseMotion(wxMouseEvent&);
      void           OnMouseRightUp(wxMouseEvent&);
      void           OnMouseRightDown(wxMouseEvent&);
      void           OnMouseLeftUp(wxMouseEvent&);
      void           OnMouseMiddleUp(wxMouseEvent&);
      void           OnMouseLeftDown(wxMouseEvent&);
      void           OnMouseLeftDClick(wxMouseEvent&);
      void           OnChar(wxKeyEvent&);
      void           OnZoom(wxCommandEvent&);
      void           OnMouseIN(wxCommandEvent&);
      void           OnCMcontinue(wxCommandEvent&);
      void           OnCMabort(wxCommandEvent&);
      void           OnCMchangeLayer(wxCommandEvent&);
      void           OnCMrulerState(wxCommandEvent&);
      void           OnCMcancel(wxCommandEvent&);
      void           OnCMclose(wxCommandEvent&);
      void           OnRepeatLastCmd(wxCommandEvent&);
      void           OnCMFlip(wxCommandEvent&);
      void           OnCMRotate(wxCommandEvent&);
      void           OnCursorType(wxCommandEvent&);
      void           initializeGL();
   private:
      void           CursorControl(bool, bool);
//      void           drawInterim(const TP&);
      CTM            _LayCTM;      // Layout translation matrix
      TP             ScrMARK;      // Current marker position in DB units
      TP             ScrMARKold;   // Old marker position  in DB units
      TP             n_ScrMARK;    // Normalized marker position in DB units
      TP             n_ScrMARKold; // Normalized Old marker position  in DB units
      TP             lp_BL;        // bottom left corner of the current visual window
      TP             lp_TR;        // top right corner of the current visual window
      double         WH_ratio;     // width/height ratio of the screen as reported by the openGL
      word           ap_trigger;   // autopan trigger limit
      TP             presspoint;   // store the location where a mouse button has been pressed
      TP             releasepoint; // store the location where a mouse button has been released
      void           update_viewport();
      bool           tmp_wnd;
      bool           invalid_window; // Indicates canvas needs repainting due to a change of the zoom
      bool           mouse_input;    // Indicates that a mouse input is expected
      bool           rubber_band;    // Indicates that moving or changing objects must be drawn
      bool           restricted_move;// when mouse controlled input
      wxCursor*      crossCur;
      //
      void           wnd_paint();
      void           rubber_paint();
      void           UpdateCoordWin(int coord, CVSSTATUS_TYPE postype, int dcoord, CVSSTATUS_TYPE dpostype);
      void           EventMouseClick(int button);
      bool           reperX;         // Draw a cursor line across the window parallel to the X axis
      bool           reperY;         // Draw a cursor line across the window parallel to the Y axis
      bool           long_cursor;    //
//      StatusLine     _status_line;
      DECLARE_EVENT_TABLE();
   };

   wxCursor* MakeCursor(const char * pXpm[36],  int HotX, int HotY );
}   
#endif
