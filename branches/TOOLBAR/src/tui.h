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
//          $URL$
//       Created: Thu Jun 17 2004
//    Originator: Svilen Krustev - skr@toped.org.uk
//   Description: Tell user interface classes
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================
//      Comments :
//===========================================================================

#if !defined(TUI_H_INCLUDED)
#define TUI_H_INCLUDED

#include <wx/wx.h>
#include <wx/spinbutt.h>
#include "../tpd_common/ttt.h"
#include "resourcecenter.h"
#include "../tpd_DB/viewprop.h"

namespace tui {

   enum {
      COLOR_COMBO = 100 ,
      FILL_COMBO        ,
      LINE_COMBO        ,
      DRAW_SELECTED     ,
      ID_NEWITEM        ,
      ID_ITEMLIST       ,
      ID_REDVAL         ,
      ID_GREENVAL       ,
      ID_BLUEVAL        ,
      ID_ALPHAVAL       ,
      ID_BTNEDIT        ,
      ID_BTNAPPLY       ,
      ID_BTNCLEAR       ,
      ID_BTNFILL        ,
      ID_RADIOBSIZE     ,
      ID_CBDEFCOLOR     ,
      ID_CBDEFPATTERN   ,
      ID_CBDEFLINE
   };

   class getSize : public wxDialog {
   public:
      getSize(wxFrame *parent, wxWindowID id, const wxString &title, wxPoint pos, real step, byte precision );
      wxString value() const {return _wxText->GetValue();};
   private:   
      wxTextCtrl* _wxText;
   };

   class getStep : public wxDialog {
   public:
      getStep(wxFrame *parent, wxWindowID id, const wxString &title, wxPoint pos, 
                                                                     real init );
      wxString value() const {return _wxText->GetValue();};
   private:   
      wxTextCtrl* _wxText;
   };

   class getGrid : public wxDialog {
   public:
      getGrid(wxFrame *parent, wxWindowID id, const wxString &title, wxPoint pos, 
                                                   real ig0, real ig1, real ig2);
      wxString grid0() const {return _wxGrid0->GetValue();};
      wxString grid1() const {return _wxGrid1->GetValue();};
      wxString grid2() const {return _wxGrid2->GetValue();};
   private:   
      wxTextCtrl* _wxGrid0;
      wxTextCtrl* _wxGrid1;
      wxTextCtrl* _wxGrid2;
   };

   class getCellOpen : public wxDialog {
   public:
      getCellOpen(wxFrame *parent, wxWindowID id, const wxString &title, wxPoint pos, wxString init);
      wxString get_selectedcell() const {return _nameList->GetStringSelection();};
      wxListBox*  _nameList;
   };

   class getCellRef : public wxDialog {
   public:
      getCellRef(wxFrame *parent, wxWindowID id, const wxString &title, wxPoint pos, wxString init);
      wxString get_selectedcell() const {return _nameList->GetStringSelection();};
//      wxString get_angle() const {return _rotation->GetValue();};
//      bool  get_flip() const {return _flip->GetValue();};
//      wxTextCtrl* _rotation;
//      wxCheckBox* _flip;
      wxListBox*  _nameList;
   };

   class getCellARef : public wxDialog {
   public:
      getCellARef(wxFrame *parent, wxWindowID id, const wxString &title, wxPoint pos, wxString init);
      wxString get_selectedcell() const {return _nameList->GetStringSelection();};
//      wxString get_angle() const {return _rotation->GetValue();};
      wxString get_col() const {return _numY->GetValue();};
      wxString get_row() const {return _numX->GetValue();};
      wxString get_stepX() const {return _stepX->GetValue();};
      wxString get_stepY() const {return _stepY->GetValue();};

//      bool  get_flip() const {return _flip->GetValue();};
//      wxTextCtrl* _rotation;
      wxTextCtrl* _numX;
      wxTextCtrl* _numY;
      wxTextCtrl* _stepX;
      wxTextCtrl* _stepY;
//      wxCheckBox* _flip;
      wxListBox*  _nameList;
   };

   class getTextdlg : public wxDialog {
   public:
      getTextdlg(wxFrame *parent, wxWindowID id, const wxString &title, wxPoint pos);
      wxString    get_text()  const {return _text->GetValue();};
      wxString    get_size()  const {return _size->GetValue();};
      wxString    get_angle() const {return _rotation->GetValue();};
      bool        get_flip() const  {return _flip->GetValue();};
   protected:   
      wxTextCtrl* _text;
      wxTextCtrl* _size;
      wxTextCtrl* _rotation;
      wxCheckBox* _flip;
   };

   class sgSpinButton : public wxSpinButton {
   public:
      sgSpinButton(wxWindow *parent, wxTextCtrl* textW, const float step, 
      const float min, const float max, const float value, const int prec) ;
   private:
      wxTextCtrl* _wxText;
      float _step;
      int   _prec;
      void  OnSpin(wxSpinEvent&);
      DECLARE_EVENT_TABLE();
   };

   class getGDSimport : public wxDialog {
   public:
                     getGDSimport(wxFrame *parent, wxWindowID id, const wxString &title,
                                                                  wxPoint pos, wxString init);
      wxString       get_selectedcell() const {return _nameList->GetStringSelection();};
      bool           get_overwrite()    const {return _overwrite->GetValue();};
      bool           get_recursive()    const {return _recursive->GetValue();};
   private:
      wxCheckBox*    _overwrite;
      wxCheckBox*    _recursive;
      wxListBox*     _nameList;
   };

   class getGDSexport : public wxDialog {
   public:
                     getGDSexport(wxFrame *parent, wxWindowID id, const wxString &title,
                                                                  wxPoint pos, wxString init);
      wxString       get_selectedcell() const {return _nameList->GetStringSelection();};
      bool           get_recursive()    const {return _recursive->GetValue();};
   private:
      wxCheckBox*    _recursive;
      wxListBox*     _nameList;
   };

   class layset_sample : public wxWindow {
   public:
                     layset_sample(wxWindow*, wxWindowID, wxPoint, wxSize, word);
      void           setColor(word);
      void           setColor(const layprop::tellRGB&);
      void           setFill(word);
      void           setFill(const byte*);
      void           setLine(word);
      void           setLine(const layprop::LineSettings*);
      void           setSelected(bool selected) {_selected = selected;}
      void           OnPaint(wxPaintEvent&);
   protected:
      void           drawOutline(wxPaintDC&, wxCoord, wxCoord);
      wxColour             _color;
      wxBrush              _brush;
      wxPen                _pen;
      std::vector<byte>    _dashes;
      byte                 _linew;
      bool                 _selected;
      DECLARE_EVENT_TABLE();
   };

   class defineLayer : public wxDialog {
   public:
                     defineLayer(wxFrame*, wxWindowID, const wxString&, wxPoint, word);
      virtual       ~defineLayer();
      void           OnColorChanged(wxCommandEvent&);
      void           OnFillChanged(wxCommandEvent&);
      void           OnLineChanged(wxCommandEvent&);
      void           OnSelectedChanged(wxCommandEvent&);
      void           OnDefaultColor(wxCommandEvent&);
      void           OnDefaultPattern(wxCommandEvent&);
      void           OnDefaultLine(wxCommandEvent&);
      wxString       layno()       {return _layno;}
      wxString       layname()     {return _layname;}
      wxString       color();//       {return _colors->GetValue();}
      wxString       fill();//        {return _fills->GetValue();}
      wxString       line();//        {return _lines->GetValue();}
   private:
      wxString       _layno;
      wxString       _layname;
      wxString       _fillname;
      wxComboBox*    _colors;
      wxComboBox*    _fills;
      wxComboBox*    _lines;
      layset_sample* _sample;
      wxCheckBox*    _selected;
      DECLARE_EVENT_TABLE();
   };
   
   class color_sample : public wxWindow {
   public:
                     color_sample(wxWindow*, wxWindowID, wxPoint, wxSize, std::string);
      void           setColor(const layprop::tellRGB&);
      void           OnPaint(wxPaintEvent&);
   protected:
      wxColour             _color;
      DECLARE_EVENT_TABLE();
   };
   
   class defineColor : public wxDialog
   {
   public:
      typedef  std::map<std::string, layprop::tellRGB*>  colorMAP;
                     defineColor(wxFrame *parent, wxWindowID id, const wxString &title,
                                                                  wxPoint pos);
      virtual          ~defineColor();
      void              OnDefineColor(wxCommandEvent&);
      void              OnColorSelected(wxCommandEvent&);
      void              OnApply(wxCommandEvent&);
      void              OnColorPropChanged(wxCommandEvent&);
      void              OnColorNameAdded(wxCommandEvent&);
      const colorMAP&   allColors() const {return _allColors;}
   private:
      void                    nameNormalize(wxString&);
      const layprop::tellRGB* getColor(std::string color_name) const;
      colorMAP                _allColors;
      wxListBox*              _colorList;
      wxTextCtrl*             _dwcolname;
      color_sample*           _colorsample;
      wxString                _colname;
      wxString                _red;
      wxString                _green;
      wxString                _blue;
      wxString                _alpha;
      wxTextCtrl*             _c_red;
      wxTextCtrl*             _c_green;
      wxTextCtrl*             _c_blue;
      wxTextCtrl*             _c_alpha;
      
      DECLARE_EVENT_TABLE();
   };

   class pattern_canvas : public wxWindow {
   public:
                     pattern_canvas(wxWindow*, wxWindowID, wxPoint, wxSize, const byte*);
      void           OnPaint(wxPaintEvent&);
      void           Clear();
      void           Fill();
      void           setBrushSize(byte bsize) {_brushsize = bsize;}
      byte*          pattern() {return _pattern;}
   protected:
      void           OnMouseLeftUp(wxMouseEvent& event);
      void           OnMouseRightUp(wxMouseEvent& event);
   private:
      byte           _pattern[128];
      byte           _brushsize;
      DECLARE_EVENT_TABLE();
   };

   class drawFillDef : public wxDialog {
   public:
                     drawFillDef(wxWindow *parent, wxWindowID id, const wxString &title,
                                                                  wxPoint pos, const byte*);
      byte*          pattern() {return _sampleDraw->pattern();}
      virtual       ~drawFillDef();
   protected:
      void              OnClear(wxCommandEvent&);
      void              OnFill(wxCommandEvent&);
      void              OnBrushSize(wxCommandEvent&);
      pattern_canvas*   _sampleDraw;
      wxRadioBox*       _radioBrushSize;
      DECLARE_EVENT_TABLE();
   };

   class fill_sample : public wxWindow {
   public:
                     fill_sample(wxWindow*, wxWindowID, wxPoint, wxSize, std::string);
      void           setFill(const byte*);
      void           OnPaint(wxPaintEvent&);
   protected:
      wxBrush        _brush;
      DECLARE_EVENT_TABLE();
   };
   
   class defineFill : public wxDialog
   {
   public:
      typedef  std::map<std::string, byte*         >  fillMAP;
                     defineFill(wxFrame *parent, wxWindowID id, const wxString &title,
                                                                  wxPoint pos);
      virtual       ~defineFill();
      void           OnDefineFill(wxCommandEvent&);
      void           OnFillSelected(wxCommandEvent&);
      void           OnFillNameAdded(wxCommandEvent&);
      fillMAP&       allPatterns() {return _allFills;}
   private:
      void           nameNormalize(wxString&);
      void           fillcopy(const byte*, byte*);
      const byte*    getFill(const std::string) const;
      fillMAP        _allFills;
      wxListBox*     _fillList;
      wxTextCtrl*    _dwfilname;
      fill_sample*   _fillsample;
      wxString       _filname;
      byte           _current_pattern[128];

      DECLARE_EVENT_TABLE();
   };
}
#endif

