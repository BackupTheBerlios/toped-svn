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
#include "../tpd_ifaces/calbr_reader.h"
#include "../tpd_ifaces/gds_io.h"
#include "../tpd_ifaces/cif_io.h"

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
      BT_ADDTDT_LIB,
      BT_NEWTDTDB,
      BT_ADDGDS_TAB,
      BT_CLEARGDS_TAB,
      BT_ADDCIF_TAB,
      BT_CLEARCIF_TAB,
		BT_ADDDRC_TAB,
		BT_CLEARDRC_TAB,
      BT_CELLS_HIER,
      BT_CELLS_FLAT,
      BT_CELLS_HIER2,
      BT_CELLS_FLAT2,
		BT_DRC_SHOW_ALL,
		BT_DRC_HIDE_ALL,
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
      CIFTREEREPORTLAY        ,
      LAYERHIDESELECTED       ,
      LAYERSHOWSELECTED       ,
      LAYERLOCKSELECTED       ,
      LAYERUNLOCKSELECTED     ,
      LAYERCURRENTSELECTED
   };


   //===========================================================================
   class CellBrowser: public wxTreeCtrl {
      public:
                           CellBrowser(wxWindow* parent, wxWindowID id = -1, 
                              const wxPoint& pos = wxDefaultPosition, 
                              const wxSize& size = wxDefaultSize,
                              long style = wxTR_DEFAULT_STYLE);
         virtual          ~CellBrowser()  {};
         virtual  void    showMenu(wxTreeItemId id, const wxPoint& pt);
         bool              findItem(const wxString name, wxTreeItemId& item, const wxTreeItemId parent);
         void              copyItem(const wxTreeItemId, const wxTreeItemId);
         void              highlightChildren(wxTreeItemId, wxColour);
         wxString          selectedCellName();
         wxString          topCellName();
         wxString          activeCellName();
         void              statusHighlight(wxString, wxString, wxString);
         wxString          rbCellName();
         virtual void     collectInfo(bool);
      protected:
         wxTreeItemId      _rbCellID;
         virtual void     onItemRightClick(wxTreeEvent&);
         virtual void     onBlankRMouseUp(wxMouseEvent&);
         virtual void     onReportlay(wxCommandEvent& WXUNUSED(event)) {assert(false);}
      private:
         void              initialize();
         void              collectChildren(const laydata::TDTHierTree*, int, const wxTreeItemId&);
         void              updateFlat();
         void              updateHier();
         void              onCommand(wxCommandEvent&);
         void              onLMouseDblClk(wxMouseEvent&);
         void              onWxOpenCell(wxCommandEvent&);
         void              tdtCellSpot(const wxTreeItemId&, const wxTreeItemId&);
         void              onTellAddCell(wxString, wxString, int);
         void              onTellRemoveCell(wxString, wxString, int);
         wxTreeItemId      _topStructure;
         wxTreeItemId      _activeStructure;
         wxTreeItemId      _dbroot; // The actual Root always invisible (because of the libraries)
         bool              _hierarchy_view;
         wxColor           _listColor;
         wxColor           _editColor;
         DECLARE_EVENT_TABLE();
   };

   //===========================================================================
   class GDSCellBrowser:public CellBrowser {
      public:
                           GDSCellBrowser(wxWindow* parent, wxWindowID id = -1, 
                              const wxPoint& pos = wxDefaultPosition, 
                              const wxSize& size = wxDefaultSize,
                              long style = wxTR_DEFAULT_STYLE);
         void              showMenu(wxTreeItemId id, const wxPoint& pt);
         void              collectInfo(bool);
      private:
         void              collectChildren(const GDSin::GDSHierTree*, const wxTreeItemId&, bool);
         void              onItemRightClick(wxTreeEvent&);
         void              onBlankRMouseUp(wxMouseEvent&);
         void              onReportlay(wxCommandEvent& WXUNUSED(event));
         DECLARE_EVENT_TABLE();
   };

   //===========================================================================
   class CIFCellBrowser:public CellBrowser {
      public:
                           CIFCellBrowser(wxWindow* parent, wxWindowID id = -1,
                              const wxPoint& pos = wxDefaultPosition,
                              const wxSize& size = wxDefaultSize,
                              long style = wxTR_DEFAULT_STYLE);
         void              showMenu(wxTreeItemId id, const wxPoint& pt);
         void              collectInfo(bool);
      private:
         void              collectChildren(const CIFin::CIFHierTree*, const wxTreeItemId&, bool);
         void              onItemRightClick(wxTreeEvent&);
         void              onBlankRMouseUp(wxMouseEvent&);
         void              onReportlay(wxCommandEvent& WXUNUSED(event));
         DECLARE_EVENT_TABLE();
   };

   //===========================================================================
   class TDTbrowser : public wxPanel {
      public:
                           TDTbrowser(wxWindow* parent, wxWindowID id = -1,
                              const wxPoint& pos = wxDefaultPosition,
                              const wxSize& size = wxDefaultSize,
                              long style = wxTR_DEFAULT_STYLE);
                          ~TDTbrowser();
         void              initialize();
         wxString          selectedCellName() const;
         void              collectInfo()              {_cellBrowser->collectInfo(_hierarchy_view);}
         CellBrowser*      cellBrowser() const        {return _cellBrowser;}
      private:
         void              onReportUsedLayers(wxCommandEvent& WXUNUSED(event));
         void              onHierView(wxCommandEvent&);
         void              onFlatView(wxCommandEvent&);
         wxImageList*      _imageList;
         wxButton*         _hierButton;
         wxButton*         _flatButton;
         CellBrowser*      _cellBrowser;
         bool              _hierarchy_view;
         DECLARE_EVENT_TABLE();
   };

   //===========================================================================
   class XdbBrowser : public wxPanel {
      public:
                           XdbBrowser(wxWindow*, wxWindowID id = -1,
                              const wxPoint& pos = wxDefaultPosition,
                              const wxSize& size = wxDefaultSize,
                              long style = wxTR_DEFAULT_STYLE);
         wxString          selectedCellName() const;
         void              deleteAllItems(void);
         void              collectInfo()              {_cellBrowser->collectInfo(_hierarchy_view);}
      private:
         void              onHierView(wxCommandEvent&);
         void              onFlatView(wxCommandEvent&);
         wxButton*         _hierButton;
         wxButton*         _flatButton;
         CellBrowser*      _cellBrowser;
         bool              _hierarchy_view;
         DECLARE_EVENT_TABLE();
   };

   //===========================================================================
   class LayerInfo {
      public:
                              LayerInfo(const LayerInfo& lay);
                              LayerInfo(const std::string &name, const word layno);
                              ~LayerInfo()         {                 }
         std::string          name()               { return _name;  }
         word                 layno()              { return _layno; }
      private:
         std::string          _name;
         word                 _layno;
         std::string          _col;
         std::string          _fill;
   };

   //===========================================================================
   class LayerButton:public wxPanel {
      public:
                              LayerButton(wxWindow* parent, wxWindowID id,
                                          const wxPoint& pos = wxDefaultPosition,
                                          const wxSize& size = wxDefaultSize,
                                          long style = wxBU_AUTODRAW,
                                          const wxValidator& validator = wxDefaultValidator,
                                          const wxString& name = wxT("button"),
                                          LayerInfo *layer = NULL);
                              ~LayerButton();
         void                 onLeftClick(wxMouseEvent &event);
         void                 onMiddleClick(wxMouseEvent &event);
         void                 onPaint(wxPaintEvent&event);
         //Call when other button is selected
         void                 unselect();
         void                 select();
         void                 hideLayer(bool);
         void                 lockLayer(bool);
         void                 preparePicture();
         word                 getLayNo()          {return _layer->layno();}

      private:
         int                  _buttonWidth;
         int                  _buttonHeight;
         LayerInfo*           _layer;
         wxBitmap*            _picture;
         wxBrush*             _brush;
         wxPen*               _pen;
         bool                 _selected;
         bool                 _hidden;
         bool                 _locked;

      DECLARE_EVENT_TABLE();
   };

   //===========================================================================
   class LayerPanel:public wxScrolledWindow {
      public:
                              LayerPanel(wxWindow* parent, wxWindowID id = -1,
                                          const wxPoint& pos = wxDefaultPosition,
                                          const wxSize& size = wxDefaultSize,
                                          long style = wxHSCROLL |  wxVSCROLL,
                                          const wxString& name = wxT("LayerPanel"));

         virtual					~LayerPanel();
         wxString             getAllSelected();
			void						refresh(void);

      private:
         typedef std::map <word, LayerButton*> LayerButtonMap;

         void                 onSize(wxSizeEvent&);
         void                 onCommand(wxCommandEvent&);
			void						addButton(LayerInfo *layer);
         LayerButton*         checkDefined(word);
         LayerButtonMap       _buttonMap;
         int                  _buttonCount;
         LayerButton*         _selectedButton;
         DECLARE_EVENT_TABLE();
   };

   //===========================================================================
   class LayerBrowser : public wxPanel {
      public:
                              LayerBrowser(wxWindow* parent, wxWindowID id);
         virtual            ~LayerBrowser();
         LayerPanel*          getLayerPanel() {return _layerPanel;};
      private:
         void                 onShowAll(wxCommandEvent& WXUNUSED(event));
         void                 onHideAll(wxCommandEvent& WXUNUSED(event));
         void                 onLockAll(wxCommandEvent& WXUNUSED(event));
         void                 onUnlockAll(wxCommandEvent& WXUNUSED(event));
         wxString             getAllSelected();
         LayerPanel*          _layerPanel;
         wxBoxSizer*          _thesizer;
         DECLARE_EVENT_TABLE();
   };
 //===========================================================================
   class ErrorBrowser: public wxTreeCtrl 
	{
   public:
                           ErrorBrowser(wxWindow* parent, wxWindowID id = -1, 
                              const wxPoint& pos = wxDefaultPosition, 
                              const wxSize& size = wxDefaultSize,
                              long style = wxTR_DEFAULT_STYLE);
//	   void	               saveInfo(const Calbr::drcPolygon &poly);
//		void	               saveInfo(const Calbr::drcEdge &edge);
		void						onLMouseDblClk(wxMouseEvent&);
	private:
		/*Calbr::drcPolygon		_poly;
		bool						_polyError;
		Calbr::drcEdge 		_edge;
		bool						_edgeError;*/
		DECLARE_EVENT_TABLE();
	};

  //===========================================================================
   class DRCBrowser : public wxPanel {
	public:
										DRCBrowser(wxWindow* parent, wxWindowID id);
		virtual						~DRCBrowser();
		void							deleteAllItems(void);
		void							onShowAll(wxCommandEvent&);
		void							onHideAll(wxCommandEvent&);
	private:
		ErrorBrowser*				_errorBrowser;
		wxButton*					_showAllButton;
		wxButton*					_hideAllButton;
		DECLARE_EVENT_TABLE();
	};

   //===========================================================================
   class browserTAB : public wxAuiNotebook {
      public:
                              browserTAB(wxWindow *parent, wxWindowID id = -1,
                                          const wxPoint& pos = wxDefaultPosition,
                                          const wxSize& size = wxDefaultSize,
                                          long style = 0);
         virtual            ~browserTAB();
         wxString             tdtSelectedGdsName() const;
         wxString             tdtSelectedCifName() const;
         LayerBrowser*        tdtLayers() const             { return _layers;       }
         CellBrowser*         tdtCellBrowser() const        { return _tdtStruct->cellBrowser(); }
         wxString             tdtSelectedCellName() const   { return _tdtStruct->selectedCellName();}
         wxWindow*            tellParser() const            { return _tellParser;   }
         void                 setTellParser(wxWindow* tp)   { _tellParser = tp;      }
         private:
         void                 onCommand(wxCommandEvent&);
         void                 onTellAddTdtLib();
         void                 onTellAddGdsTab();
         void                 onTellClearGdsTab();
         void                 onTellAddCifTab();
         void                 onTellClearCifTab();
			void                 onTellAddDRCTab();
         void                 onTellClearDRCTab();
         XdbBrowser*         _gdsStruct;
         XdbBrowser*         _cifStruct;
         TDTbrowser*         _tdtStruct;
			DRCBrowser*			  _drcStruct;
         int                 _gdsPageIndex;
         int                 _cifPageIndex;
         LayerBrowser*       _layers;
         wxWindow*           _tellParser;
         DECLARE_EVENT_TABLE();
   };
 
   void layer_status(BROWSER_EVT_TYPE, const word, const bool);
   void layer_add(const std::string, const word);
   void layer_default(const word, const word);
   void addTDTtab(bool newthread = false);
   void addGDStab();
   void addCIFtab();
	void addDRCtab();
   void clearGDStab();
   void clearCIFtab();
	void clearDRCtab();
   void celltree_open(const std::string);
   void celltree_highlight(const std::string);
   void treeAddMember(const char*, const char*, int action = 0);
   void treeRemoveMember(const char*, const char*, int orphan);
   void parseCommand(const wxString);
}

#endif //BROWSERS_H_INCLUDED
