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
//        Created: Mon Aug 11 2003
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: GDSII/TDT hierarchy browser, layer browser, TELL fuction
//                 definition browser
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================
//      Comments :
//===========================================================================

#if !defined(BROWSERS_H_INCLUDED)
#define BROWSERS_H_INCLUDED
#include <wx/notebook.h>
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/treectrl.h>
#include <wx/imaglist.h>
#include <wx/aui/aui.h>
#include <string>
#include "tedesign.h"
#include "gds_io.h"

namespace browsers 
{
   const int buttonHeight = 30;
   
   typedef enum
   {
      BT_LAYER_DEFAULT,
      BT_LAYER_HIDE,
      BT_LAYER_LOCK,
      BT_LAYER_ADD,
      BT_LAYER_ACTION,
      BT_LAYER_DO,
      BT_LAYER_SELECTWILD,
      BT_LAYER_ACTIONWILD,
      BT_CELL_OPEN,
      BT_CELL_HIGHLIGHT,
      BT_CELL_REF,
      BT_CELL_AREF,
      BT_CELL_ADD,
      BT_CELL_REMOVE,
      BT_ADDTDT_TAB,
      BT_ADDGDS_TAB,
      BT_CLEARGDS_TAB,
      BT_CELLS_HIER,
      BT_CELLS_FLAT,
      BT_CELLS_HIER2,
      BT_CELLS_FLAT2,
      BT_LAYER_SELECT,
      BT_LAYER_SHOW_ALL,
      BT_LAYER_HIDE_ALL,
      BT_LAYER_LOCK_ALL,
      BT_LAYER_UNLOCK_ALL
   } BROWSER_EVT_TYPE;

   enum 
   {
      CELLTREEOPENCELL  = 1000,
      GDSTREEREPORTLAY        ,
      LAYERHIDESELECTED       ,
      LAYERSHOWSELECTED       ,
      LAYERLOCKSELECTED       ,
      LAYERUNLOCKSELECTED     ,
      LAYERCURRENTSELECTED
   };


   //===========================================================================
   class CellBrowser: public wxTreeCtrl
   {
   public:
      CellBrowser(wxWindow* parent, wxWindowID id = -1, 
                        const wxPoint& pos = wxDefaultPosition, 
                        const wxSize& size = wxDefaultSize,
                        long style = wxTR_DEFAULT_STYLE);
      virtual           ~CellBrowser();
      virtual  void     ShowMenu(wxTreeItemId id, const wxPoint& pt);
      bool              findItem(const wxString name, wxTreeItemId& item, const wxTreeItemId parent);
      void              copyItem(const wxTreeItemId, const wxTreeItemId);
      void              highlightChildren(wxTreeItemId, wxColour);
      wxTreeItemId      activeStructure(void) {return _activeStructure;};
      wxString          selectedCellname() const {if (RBcellID.IsOk())
         return GetItemText(RBcellID); else return wxT("");}
   protected:
      wxTreeItemId      RBcellID;
   private:
      wxTreeItemId      top_structure;
      wxTreeItemId      _activeStructure;

      void              OnItemRightClick(wxTreeEvent&);
      void              OnLMouseDblClk(wxMouseEvent&);
      void              OnWXOpenCell(wxCommandEvent&);
      void              OnBlankRMouseUp(wxMouseEvent&);

      DECLARE_EVENT_TABLE();
   };

   class GDSCellBrowser:public CellBrowser
   {
   public:
      GDSCellBrowser(wxWindow* parent, wxWindowID id = -1, 
                        const wxPoint& pos = wxDefaultPosition, 
                        const wxSize& size = wxDefaultSize,
                        long style = wxTR_DEFAULT_STYLE);
      virtual  void     ShowMenu(wxTreeItemId id, const wxPoint& pt);
   private:
      void              OnItemRightClick(wxTreeEvent&);
      void              OnBlankRMouseUp(wxMouseEvent&);
      void              OnGDSreportlay(wxCommandEvent& WXUNUSED(event));
      DECLARE_EVENT_TABLE();
   };


   //===========================================================================
   class GDSbrowser : public wxPanel 
   {
   public:
                        GDSbrowser(wxWindow *parent, wxWindowID id = -1, 
                        const wxPoint& pos = wxDefaultPosition, 
                        const wxSize& size = wxDefaultSize,
                        long style = wxTR_DEFAULT_STYLE);
      void              collectInfo();
      wxString          selectedCellname() const {if (RBcellID.IsOk())
         return hCellBrowser->GetItemText(RBcellID); else return wxT("");}
      void DeleteAllItems(void);
   protected:
      void              collectChildren(GDSin::GDSHierTree *root, 
                                                   wxTreeItemId& lroot);
   private:
      wxButton*         _hierButton;
      wxButton*         _flatButton;
      wxTreeItemId      RBcellID;
      GDSCellBrowser*   hCellBrowser;//Hierarchy cell browser
      GDSCellBrowser*   fCellBrowser;//Flat cell browser
      void              OnCommand(wxCommandEvent&);
      void              OnWXImportCell(wxCommandEvent&);
      void              OnHierView(wxCommandEvent&);
      void              OnFlatView(wxCommandEvent&);
      DECLARE_EVENT_TABLE();
   };


   //===========================================================================
   class TDTbrowser : public wxPanel 
   {
   public:
                        TDTbrowser(wxWindow* parent, wxWindowID id = -1, 
                        const wxPoint& pos = wxDefaultPosition, 
                        const wxSize& size = wxDefaultSize,
                        long style = wxTR_DEFAULT_STYLE);
      virtual           ~TDTbrowser();
      void              collectInfo(const wxString, laydata::TDTHierTree*);
      void              initialize();
      wxString          selectedCellname() const;
   protected:
      void              collectChildren(laydata::TDTHierTree *root, 
                                                 wxTreeItemId& lroot);
   private:
      wxTreeItemId      top_structure;
      wxTreeItemId      active_structure;
      wxImageList*      _imageList;
      CellBrowser*      hCellBrowser;//Hierarchy cell browser
      CellBrowser*      fCellBrowser;//Flat cell browser
      wxString          libName;
      wxButton*         _hierButton;
      wxButton*         _flatButton;
      void              OnCommand(wxCommandEvent&);
      void              OnWXCellARef(wxCommandEvent&);
      void              OnReportUsedLayers(wxCommandEvent&);
      void              OnTELLopencell(wxString);
      void              OnTELLhighlightcell(wxString);
      void              OnTELLaddcell(wxString, wxString, int);
      void              OnTELLremovecell(wxString, wxString, bool);
      void              OnHierView(wxCommandEvent&);
      void              OnFlatView(wxCommandEvent&);
      DECLARE_EVENT_TABLE();
   };

   class LayerInfo
   {
   public:
      LayerInfo(const LayerInfo& lay);
      LayerInfo(const std::string &name, const word layno);
      ~LayerInfo()         { };
      std::string name()   {return _name;};
      word        layno()  {return _layno;};
   private:
      std::string _name;
      word        _layno;
      std::string _col;
      std::string _fill;
   };

   class LayerButton:public wxPanel
   {
   public:
      LayerButton(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, 
                  const wxSize& size = wxDefaultSize, long style = wxBU_AUTODRAW,
                  const wxValidator& validator = wxDefaultValidator, const wxString& name = wxT("button"),
                  LayerInfo *layer = NULL);
      ~LayerButton();
      void OnLeftClick(wxMouseEvent &event);
      void OnMiddleClick(wxMouseEvent &event);
      void OnPaint(wxPaintEvent&event);
      //Call when other button is selected
      void unselect(void);
      void select(void);
      void hideLayer(bool hide);
      void lockLayer(bool lock);
      word getLayNo(void) {return _layer->layno();}
      void preparePicture();
      
   private:
      int         _buttonWidth;
      int         _buttonHeight;
      LayerInfo   *_layer;
      wxBitmap    *_picture;
      wxBrush     *_brush;
      wxPen       *_pen;
      bool        _selected;
      bool        _hidden;
      bool        _locked;
      
   DECLARE_EVENT_TABLE();
   };
   
   typedef std::map <word, LayerButton*> layerButtonMap;
   class LayerPanel:public wxScrolledWindow
   {
   public:
                           LayerPanel(wxWindow* parent, wxWindowID id = -1,
                              const wxPoint& pos = wxDefaultPosition,
                              const wxSize& size = wxDefaultSize,
                              long style = wxHSCROLL |  wxVSCROLL, const wxString& name = wxT("LayerPanel"));

      virtual               ~LayerPanel();
      wxString             getAllSelected();
   private:
      void                  OnSize(wxSizeEvent&);
      void                 OnCommand(wxCommandEvent&);

      //wxBitmap&            prepareBitmap(void);//-

      layerButtonMap          _buttonMap;
      int                     _buttonCount;
      LayerButton*            _selectedButton;
      DECLARE_EVENT_TABLE();

   };

   class LayerBrowser : public wxScrolledWindow 
   {
   public:
                           LayerBrowser(wxWindow* parent, wxWindowID id);
      virtual             ~LayerBrowser();
      //const layerButtonMap& getButtonMap(void) {return _buttonMap;};
      LayerPanel*      getLayerPanel() {return _layerPanel;};
      
   private:
      void                 OnShowAll(wxCommandEvent&);
      void                 OnHideAll(wxCommandEvent&);
      void                 OnLockAll(wxCommandEvent&);
      void                 OnUnlockAll(wxCommandEvent&);
      wxString             getAllSelected();
      LayerPanel*               _layerPanel;
      wxBoxSizer*             _thesizer;
      
      DECLARE_EVENT_TABLE();
   };

   //===========================================================================
   class browserTAB : public wxAuiNotebook 
   {
   public:
                        browserTAB(wxWindow *parent, wxWindowID id = -1,
        const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, 
                                                                  long style = 0);
      virtual          ~browserTAB();// {};
      LayerBrowser*    TDTlayers() const   {return _layers;};
      TDTbrowser*       TDTstruct() const    {return _TDTstruct;};
      wxString          TDTSelectedCellName() const {return _TDTstruct->selectedCellname();};
      wxString          TDTSelectedGDSName() const;// {return _GDSstruct->selectedCellname();};
      void              set_tellParser(wxWindow* tp) {_tellParser = tp;}
      wxWindow*         tellParser() const {return _tellParser;}
   private:
      void              OnCommand(wxCommandEvent&);
      void              OnTELLaddTDTtab(const wxString, laydata::TDTHierTree*);
      void              OnTELLaddGDStab();
      void              OnTELLclearGDStab();
      GDSbrowser*       _GDSstruct;
      TDTbrowser*       _TDTstruct;
      LayerBrowser*     _layers;
      wxWindow*         _tellParser;
      DECLARE_EVENT_TABLE();
   };
 
   void layer_status(BROWSER_EVT_TYPE, const word, const bool);
   void layer_add(const std::string, const word);
   void layer_default(const word, const word);
   void addTDTtab(std::string libname, laydata::TDTHierTree* tdtH);
   void addGDStab();
   void clearGDStab();
   void celltree_open(const std::string);
   void celltree_highlight(const std::string);
   void treeAddMember(const char*, const char*, int action = 0);
   void treeRemoveMember(const char*, const char*, bool orphan);
   void parseCommand(const wxString);
}

#endif //BROWSERS_H_INCLUDED
