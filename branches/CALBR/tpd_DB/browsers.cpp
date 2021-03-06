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
//        Created: Mon Aug 11 2003
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: GDSII/TDT hierarchy browser, layer browser, TELL fuction
//                 definition browser
//===========================================================================
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================
//      Comments :
//===========================================================================

#include "tpdph.h"
#include <wx/tooltip.h>
#include "browsers.h"
#include "viewprop.h"
#include "../tpd_common/tuidefs.h"
#include "../tpd_common/outbox.h"
#include "datacenter.h"
#include "../src/toped.h"
#include "../ui/activelay.xpm"
#include "../ui/lock.xpm"
#include "../ui/cell_normal.xpm"
#include "../ui/cell_expanded.xpm"
#include "../ui/nolay.xpm"

extern tui::TopedFrame*          Toped;
extern DataCenter*               DATC;
extern Calbr::CalbrFile*			DRCData;
extern const wxEventType         wxEVT_CMD_BROWSER;
extern const wxEventType         wxEVT_CONSOLE_PARSE;
extern const wxEventType         wxEVT_CANVAS_ZOOM;


browsers::browserTAB*     Browsers = NULL;

//==============================================================================
//
// CellBrowser
//
//==============================================================================
BEGIN_EVENT_TABLE(browsers::CellBrowser, wxTreeCtrl)
   EVT_TREE_ITEM_RIGHT_CLICK( tui::ID_TPD_CELLTREE, browsers::CellBrowser::onItemRightClick)
   EVT_RIGHT_UP(browsers::CellBrowser::onBlankRMouseUp)
   EVT_LEFT_DCLICK(browsers::CellBrowser::onLMouseDblClk)
   EVT_MENU(CELLTREEOPENCELL, browsers::CellBrowser::onWxOpenCell)
   EVT_TECUSTOM_COMMAND(wxEVT_CMD_BROWSER, wxID_ANY, browsers::CellBrowser::onCommand)
END_EVENT_TABLE()

browsers::CellBrowser::CellBrowser(wxWindow *parent, wxWindowID id,
                           const wxPoint& pos, const wxSize& size, long style) :
      wxTreeCtrl(parent, id, pos, size, style | wxTR_FULL_ROW_HIGHLIGHT )
{
   _hierarchy_view   = true;
   _listColor        = wxColor(128,128,128);
   _editColor        = *wxBLACK;
}

void browsers::CellBrowser::initialize()
{
   DeleteAllItems();
   AddRoot(wxT("hidden_wxroot"));
   _topStructure.Unset();
   _activeStructure.Unset();
}

void browsers::CellBrowser::showMenu(wxTreeItemId id, const wxPoint& pt)
{
   wxMenu menu;
   _rbCellID = id;
   if ( id.IsOk() && (id != _dbroot))
   {
      wxString RBcellname = GetItemText(id);
      menu.Append(CELLTREEOPENCELL, wxT("Open " + RBcellname));
      menu.Append(tui::TMCELL_REF_B , wxT("Add reference to " + RBcellname));
      menu.Append(tui::TMCELL_AREF_B, wxT("Add array of " + RBcellname));
      wxString ost;
      ost << wxT("export ") << RBcellname << wxT(" to GDS");
      menu.Append(tui::TMGDS_EXPORTC, ost);
      ost.Clear();
      ost << wxT("export ") << RBcellname << wxT(" to CIF");
      menu.Append(tui::TMCIF_EXPORTC, ost);
      menu.Append(tui::TMCELL_REPORTLAY, wxT("Report layers used in " + RBcellname));
   }
   else
   {
      menu.Append(tui::TMCELL_NEW, wxT("New cell")); // will be catched up in toped.cpp
      menu.Append(tui::TMGDS_EXPORTL, wxT("GDS export"));
      menu.Append(tui::TMCIF_EXPORTL, wxT("CIF export"));
   }
   PopupMenu(&menu, pt);
}

void browsers::CellBrowser::onWxOpenCell(wxCommandEvent& event)
{
   wxString cmd;
   cmd << wxT("opencell(\"") << GetItemText(_rbCellID) <<wxT("\");");
   parseCommand(cmd);
}

void browsers::CellBrowser::onItemRightClick(wxTreeEvent& event)
{
   showMenu(event.GetItem(), event.GetPoint());
}

void browsers::CellBrowser::onBlankRMouseUp(wxMouseEvent& event)
{
   wxPoint pt = event.GetPosition();
   showMenu(HitTest(pt), pt);
}

void  browsers::CellBrowser::onLMouseDblClk(wxMouseEvent& event)
{
   int flags;
   wxPoint pt = event.GetPosition();
   wxTreeItemId id = HitTest(pt, flags);
   if (id.IsOk() && (id != _dbroot) && (flags & wxTREE_HITTEST_ONITEMLABEL))
   {
      wxString cmd;
      cmd << wxT("opencell(\"") << GetItemText(id) <<wxT("\");");
      parseCommand(cmd);
   }
   else
      event.Skip();
}

bool browsers::CellBrowser::findItem(const wxString name, wxTreeItemId& item, const wxTreeItemId parent) 
{
   wxTreeItemIdValue cookie;
   wxTreeItemId child = GetFirstChild(parent,cookie);
   while (child.IsOk())
   {
      if (item.IsOk())
      {
         if (child == item) item = wxTreeItemId();
      }
      else if (name == GetItemText(child))
      {
         item = child; return true;
      }
      if (findItem(name, item, child)) return true;
      child = GetNextChild(parent,cookie);
   }
   return false;
}

void browsers::CellBrowser::copyItem(const wxTreeItemId item, const wxTreeItemId newparent) 
{
   wxTreeItemId newitem = AppendItem(newparent, GetItemText(item));
   SetItemImage(newitem, GetItemImage(item,wxTreeItemIcon_Normal), wxTreeItemIcon_Normal);
   SetItemImage(newitem, GetItemImage(item,wxTreeItemIcon_Expanded), wxTreeItemIcon_Expanded);
   SetItemImage(newparent,0,wxTreeItemIcon_Normal);
   SetItemImage(newparent,1,wxTreeItemIcon_Expanded);
   SetItemTextColour(newitem, GetItemTextColour(newparent));
   wxTreeItemIdValue cookie;
   wxTreeItemId child = GetFirstChild(item,cookie);
   while (child.IsOk())
   {
      copyItem(child, newitem);
      child = GetNextChild(item,cookie);
   }
}

void browsers::CellBrowser::highlightChildren(wxTreeItemId parent, wxColour clr) 
{
   wxTreeItemIdValue cookie;
   if (!parent.IsOk()) return;
   SetItemTextColour(parent,clr);
   wxTreeItemId child = GetFirstChild(parent,cookie);
   while (child.IsOk())
   {
      highlightChildren(child,clr);
      child = GetNextChild(parent,cookie);
   }
}

wxString browsers::CellBrowser::selectedCellName()
{
   wxTreeItemId selected = GetSelection();
   if (selected.IsOk())
      return GetItemText(selected);
   else
      return wxT("");
}

wxString browsers::CellBrowser::topCellName()
{
   if (_topStructure.IsOk())
      return GetItemText(_topStructure);
   else
      return wxT("");
}

wxString browsers::CellBrowser::activeCellName()
{
   if (_activeStructure.IsOk())
      return GetItemText(_activeStructure);
   else
      return wxT("");
}

wxString browsers::CellBrowser::rbCellName()
{
   if (_rbCellID.IsOk())
      return GetItemText(_rbCellID);
   else
      return wxT("");
}

void browsers::CellBrowser::statusHighlight(wxString top, wxString active, wxString selected)
{
   if (findItem(top, _topStructure, GetRootItem()))
      highlightChildren(_topStructure, _editColor);
   if (findItem(active, _activeStructure, GetRootItem()))
      SetItemBold(_activeStructure, true);

   wxTreeItemId      item;
   if (findItem(selected, item, GetRootItem()))
   {
      SelectItem(item);
      EnsureVisible(item);
   }
}

void browsers::CellBrowser::collectInfo( bool hier )
{
   initialize();
   _hierarchy_view = hier;
   if(_hierarchy_view)  updateHier();
   else                 updateFlat();
   highlightChildren(GetRootItem(), _listColor);
}

void browsers::CellBrowser::updateFlat()
{
   wxTreeItemId temp, nroot;
   laydata::LibCellLists *cll;
   laydata::LibCellLists::iterator curlib;
   bool rootexists = true;
   try
   {
      DATC->lockDB(false);
   }
   catch (EXPTNactive_DB) {rootexists = false;}
   if (rootexists)
   {
      _dbroot = GetRootItem();
      // get undefined cells first
      cll = DATC->getCells(UNDEFCELL_LIB);
      for (curlib = cll->begin(); curlib != cll->end(); curlib++)
      {
         laydata::cellList::const_iterator CL;
         for (CL = (*curlib)->begin(); CL != (*curlib)->end(); CL++)
         {
            wxString cellName = wxString( CL->first.c_str(),  wxConvUTF8);
            if (!findItem(cellName, temp, GetRootItem()))
               AppendItem(GetRootItem(), cellName);
         }
      }
      delete cll;
   }
   // get all libraries
   cll = DATC->getCells(ALL_LIB);
   for (curlib = cll->begin(); curlib != cll->end(); curlib++)
   {
      laydata::cellList::const_iterator CL;
      for (CL = (*curlib)->begin(); CL != (*curlib)->end(); CL++)
      {
         wxString cellName = wxString( CL->first.c_str(),  wxConvUTF8);
         if (!findItem(cellName, temp, GetRootItem()))
            AppendItem(GetRootItem(), cellName);
      }
   }
   delete cll;
   DATC->unlockDB();

   SortChildren(GetRootItem());
}

void browsers::CellBrowser::updateHier()
{
   laydata::tdtdesign* design;
   bool rootexists = true;
   try
   {
      design = DATC->lockDB(false);
   }
   catch (EXPTNactive_DB) {rootexists = false;}
   laydata::TDTHierTree *tdtH = NULL;
   wxTreeItemId nroot;
   // traverse the target design - if it exists
   if (rootexists)
   {
      // design name ...
      _dbroot = AppendItem(GetRootItem(),wxString(design->name().c_str(),  wxConvUTF8));
      SetItemImage(_dbroot,0,wxTreeItemIcon_Normal);
      SetItemImage(_dbroot,1,wxTreeItemIcon_Expanded);
      // ... and the cells
      tdtH = design->hiertree()->GetFirstRoot(TARGETDB_LIB);
      while (tdtH)
      {
         std::string str = tdtH->GetItem()->name();
         nroot = AppendItem(_dbroot, wxString(tdtH->GetItem()->name().c_str(), wxConvUTF8));
         SetItemImage(nroot,0,wxTreeItemIcon_Normal);
         SetItemImage(nroot,1,wxTreeItemIcon_Expanded);
         collectChildren(tdtH, ALL_LIB, nroot);
         tdtH = tdtH->GetNextRoot(TARGETDB_LIB);
      }
   }
   // traverse the libraries now
   for(int libID = 1; libID < DATC->TEDLIB()->getLastLibRefNo(); libID++)
   {
      // library name ...
      wxTreeItemId libroot = AppendItem(GetRootItem(),wxString(DATC->getLib(libID)->name().c_str(),  wxConvUTF8));
      SetItemImage(libroot,0,wxTreeItemIcon_Normal);
      SetItemImage(libroot,1,wxTreeItemIcon_Expanded);
      // ... and the cells
      tdtH = DATC->getLib(libID)->hiertree()->GetFirstRoot(libID);
      while (tdtH)
      {
         std::string str = tdtH->GetItem()->name();
         nroot = AppendItem(libroot, wxString(tdtH->GetItem()->name().c_str(), wxConvUTF8));
         SetItemImage(nroot,0,wxTreeItemIcon_Normal);
         SetItemImage(nroot,1,wxTreeItemIcon_Expanded);
         collectChildren(tdtH, libID, nroot);
         tdtH = tdtH->GetNextRoot(libID);
      }
   }
   DATC->unlockDB();
   // And now - deal with the undefined cells
   const laydata::cellList& cellList= DATC->TEDLIB()->getUndefinedCells();
   if (cellList.size() != 0)
   {
      // the type ...
      wxTreeItemId nrootUndef = AppendItem(GetRootItem(), wxString("Undefined Cells", wxConvUTF8));
      SetItemImage(nrootUndef,0,wxTreeItemIcon_Normal);
      SetItemImage(nrootUndef,1,wxTreeItemIcon_Expanded);
      // ... and the cells
      for(laydata::cellList::const_iterator it=cellList.begin(); it!= cellList.end(); it++)
         AppendItem(nrootUndef, wxString( (*it).first.c_str(),  wxConvUTF8));
   }
   SortChildren(GetRootItem());

}

void browsers::CellBrowser::collectChildren(const laydata::TDTHierTree *root,
                                           int libID, const wxTreeItemId& lroot)
{
   const laydata::TDTHierTree* child= root->GetChild(libID);
   wxTreeItemId nroot;
   wxTreeItemId temp;
   while (child)
   {
      SetItemImage(lroot,0,wxTreeItemIcon_Normal);
      SetItemImage(lroot,1,wxTreeItemIcon_Expanded);
      nroot = AppendItem(lroot, wxString(child->GetItem()->name().c_str(), wxConvUTF8));
      SortChildren(lroot);
      collectChildren(child, libID, nroot);
      child = child->GetBrother(libID);
   }
}

void browsers::CellBrowser::onCommand( wxCommandEvent& event )
{
   switch ( event.GetInt() )
   {
      case BT_CELL_OPEN :{
         wxTreeItemId topItem;
         VERIFY(findItem(event.GetString(), topItem, GetRootItem()));
         tdtCellSpot(topItem, topItem);
         break;
      }
      case BT_CELL_HIGHLIGHT: {
         wxTreeItemId actItem;
         VERIFY(findItem(event.GetString(), actItem, GetRootItem()));
         tdtCellSpot(_topStructure, actItem);
         break;
      }
      case BT_CELL_ADD :
         onTellAddCell(event.GetString(),
                       *(static_cast<wxString*>(event.GetClientData())),
                         static_cast<int>(event.GetExtraLong()));
         delete (static_cast<wxString*>(event.GetClientData()));
         break;
      case BT_CELL_REMOVE:
         onTellRemoveCell(event.GetString(),
                          *(static_cast<wxString*>(event.GetClientData())),
                            static_cast<int>(event.GetExtraLong()));
         delete (static_cast<wxString*>(event.GetClientData()));
         break;
      default: assert(false);
   }
}

void browsers::CellBrowser::tdtCellSpot( const wxTreeItemId& topItem, const wxTreeItemId& actItem )
{
   //
   highlightChildren(_topStructure, _listColor);
   if (_activeStructure.IsOk()) SetItemBold(_activeStructure, false);
   if (  _topStructure.IsOk() ) SetItemBold(   _topStructure, false);
   //
   _topStructure    = topItem;
   _activeStructure = actItem;
   highlightChildren(_topStructure, _editColor);
   SetItemBold(_activeStructure, true);
}

void browsers::CellBrowser::onTellAddCell(wxString cellname, wxString parentname, int action)
{
   wxTreeItemId item;
   switch (action)
   {
      case 0://new cell
         if (_hierarchy_view)
         {
            wxTreeItemId hnewparent;
            VERIFY(findItem(parentname, hnewparent, GetRootItem()));
            item = AppendItem(hnewparent, cellname);
            SetItemTextColour(item,GetItemTextColour(GetRootItem()));
            SortChildren(GetRootItem());
         }
         else
         {
            VERIFY(!findItem(cellname, item, GetRootItem()));
            AppendItem(GetRootItem(), cellname);
         }
         break;
      case 1://first reference of existing cell
         if (_hierarchy_view)
         {
            wxTreeItemId newparent;
            VERIFY(findItem(cellname, item, GetRootItem()));
            while (findItem(parentname, newparent, GetRootItem()))
            {
               copyItem(item,newparent);
               SortChildren(newparent);
            }
            DeleteChildren(item);
            Delete(item);
         }
         break;
      case 2:
      case 3:
         if (_hierarchy_view)
         {//
            wxTreeItemId newparent;
            VERIFY(findItem(cellname, item, GetRootItem()));
            while (findItem(parentname, newparent, GetRootItem()))
            {
               copyItem(item,newparent);
               SortChildren(newparent);
            }
         }
         break;
      default: assert(false);
   }
}

void browsers::CellBrowser::onTellRemoveCell(wxString cellname, wxString parentname, int action)
{
   wxTreeItemId newparent;
   switch (action)
   {
      case 0:// no longer child of this parent - remove it from all parent instances
      case 1:// Lib cells not more referenced in the DB
         if (_hierarchy_view)
         {
            while (findItem(parentname, newparent, GetRootItem()))
            {
               wxTreeItemId item;
               VERIFY(findItem(cellname, item, newparent));
               DeleteChildren(item);
               Delete(item);
            }
         }
         break;
      case 2://DB cell, which has no parents anymore
         if (_hierarchy_view)
         {
            wxTreeItemId item;
            findItem(cellname, item, GetRootItem());
            copyItem(item, _dbroot);
            item = wxTreeItemId();
            VERIFY(findItem(parentname, newparent, GetRootItem()));
            VERIFY(findItem(cellname, item, newparent));
            DeleteChildren(item);
            Delete(item);
         }
         break;
      case 3:// we are removing the cell, not it's reference
      {
         wxTreeItemId item;
         VERIFY(findItem(cellname, item, GetRootItem()));
         // copy all children
         // This part is "in case". The thing is that children should have been
         // removed already, by tdtcell::removePrep
         wxTreeItemIdValue cookie;
         wxTreeItemId child = GetFirstChild(item,cookie);
         while (child.IsOk())
         {
            copyItem(child, GetRootItem());
            child = GetNextChild(item,cookie);
         }
         // finally delete the item and it's children
         DeleteChildren(item);
         Delete(item);
         break;
      }
      default: assert(false);
   }
}

//==============================================================================
//
// GDSCellBrowser
//
//==============================================================================
BEGIN_EVENT_TABLE(browsers::GDSCellBrowser, wxTreeCtrl)
   EVT_TREE_ITEM_RIGHT_CLICK( tui::ID_GDS_CELLTREE, browsers::GDSCellBrowser::onItemRightClick)
   EVT_RIGHT_UP(browsers::GDSCellBrowser::onBlankRMouseUp)
   EVT_MENU(GDSTREEREPORTLAY, browsers::GDSCellBrowser::onReportlay)
END_EVENT_TABLE()

browsers::GDSCellBrowser::GDSCellBrowser(wxWindow *parent, wxWindowID id,
   const wxPoint& pos, const wxSize& size, long style) :
                                       CellBrowser(parent, id, pos, size, style )
{}

void browsers::GDSCellBrowser::onItemRightClick(wxTreeEvent& event)
{
   showMenu(event.GetItem(), event.GetPoint());
}

void browsers::GDSCellBrowser::onBlankRMouseUp(wxMouseEvent& event)
{
   wxPoint pt = event.GetPosition();
   showMenu(HitTest(pt), pt);
}

void browsers::GDSCellBrowser::onReportlay(wxCommandEvent& WXUNUSED(event))
{
   wxString cmd;
   cmd << wxT("report_gdslayers(\"") << GetItemText(_rbCellID) <<wxT("\");");
   parseCommand(cmd);
}

void browsers::GDSCellBrowser::showMenu(wxTreeItemId id, const wxPoint& pt)
{
   wxMenu menu;
   _rbCellID = id;
   if ( id.IsOk() && (id != GetRootItem()))   {
      wxString RBcellname = GetItemText(id);
      menu.Append(tui::TMGDS_TRANSLATE, wxT("Translate " + RBcellname));
      menu.Append(GDSTREEREPORTLAY, wxT("Report layers used in " + RBcellname));
   }
   else {
      menu.Append(tui::TMGDS_CLOSE, wxT("Close GDS")); // will be catched up in toped.cpp
   }
   PopupMenu(&menu, pt);
}

void browsers::GDSCellBrowser::collectInfo(bool hier)
{
   DeleteAllItems();

   GDSin::GdsFile* AGDSDB = DATC->lockGDS(false);
   if (NULL != AGDSDB)
   {
      AddRoot(wxString((AGDSDB->libname()).c_str(), wxConvUTF8));

      if (NULL != AGDSDB->hierTree())
      {
         GDSin::GDSHierTree* root = AGDSDB->hierTree()->GetFirstRoot(TARGETDB_LIB);
         wxTreeItemId nroot;
         while (root)
         {
            nroot = AppendItem(GetRootItem(), wxString(root->GetItem()->name(),wxConvUTF8));
            collectChildren(root, nroot, hier);
            root = root->GetNextRoot(TARGETDB_LIB);
         }
      }
   }
   DATC->unlockGDS();
   SortChildren(GetRootItem());
}

void browsers::GDSCellBrowser::collectChildren(const GDSin::GDSHierTree* root,
                                               const wxTreeItemId& lroot, bool _hierarchy_view)
{
   const GDSin::GDSHierTree* Child= root->GetChild(TARGETDB_LIB);
   wxTreeItemId nroot;
   wxTreeItemId temp;

   while (Child)
   {
      if (_hierarchy_view)
      {
         nroot = AppendItem(lroot, wxString(Child->GetItem()->name(), wxConvUTF8));
         collectChildren(Child, nroot, _hierarchy_view);
      }
      else
      {
         if (!findItem(wxString(Child->GetItem()->name(), wxConvUTF8), temp, GetRootItem()))
         {
            nroot = AppendItem(GetRootItem(), wxString(Child->GetItem()->name(), wxConvUTF8));
            collectChildren(Child, nroot, _hierarchy_view);
         }
      }
      SortChildren(lroot);
      Child = Child->GetBrother(TARGETDB_LIB);
   }
}

//==============================================================================
//
// CIFCellBrowser
//
//==============================================================================
BEGIN_EVENT_TABLE(browsers::CIFCellBrowser, wxTreeCtrl)
   EVT_TREE_ITEM_RIGHT_CLICK( tui::ID_CIF_CELLTREE, browsers::CIFCellBrowser::onItemRightClick)
   EVT_RIGHT_UP(browsers::CIFCellBrowser::onBlankRMouseUp)
   EVT_MENU(CIFTREEREPORTLAY, browsers::CIFCellBrowser::onReportlay)
END_EVENT_TABLE()

      browsers::CIFCellBrowser::CIFCellBrowser(wxWindow *parent, wxWindowID id,
   const wxPoint& pos, const wxSize& size, long style) :
      CellBrowser(parent, id, pos, size, style )
{ }

void browsers::CIFCellBrowser::onItemRightClick(wxTreeEvent& event)
{
   showMenu(event.GetItem(), event.GetPoint());
}

void browsers::CIFCellBrowser::onBlankRMouseUp(wxMouseEvent& event)
{
   wxPoint pt = event.GetPosition();
   showMenu(HitTest(pt), pt);
}

void browsers::CIFCellBrowser::onReportlay(wxCommandEvent& WXUNUSED(event))
{
   wxString cmd;
   cmd << wxT("report_ciflayers(\"") << GetItemText(_rbCellID) <<wxT("\");");
   parseCommand(cmd);
}

void browsers::CIFCellBrowser::showMenu(wxTreeItemId id, const wxPoint& pt)
{
   wxMenu menu;
   _rbCellID = id;
   if ( id.IsOk() && (id != GetRootItem()) )
   {
      wxString RBcellname = GetItemText(id);
      menu.Append(tui::TMCIF_TRANSLATE, wxT("Translate " + RBcellname));
      menu.Append(CIFTREEREPORTLAY, wxT("Report layers used in " + RBcellname));
   }
   else
   {
      menu.Append(tui::TMCIF_CLOSE, wxT("Close CIF")); // will be catched up in toped.cpp
   }
   PopupMenu(&menu, pt);
}

void browsers::CIFCellBrowser::collectInfo(bool hier)
{
   DeleteAllItems();

   CIFin::CifFile* ACIFDB = DATC->lockCIF(false);
   if (NULL != ACIFDB)
   {
      AddRoot(wxString((ACIFDB->Get_libname()).c_str(), wxConvUTF8));

      if (NULL != ACIFDB->hiertree())
      {
         CIFin::CIFHierTree* root = ACIFDB->hiertree()->GetFirstRoot(TARGETDB_LIB);
         wxTreeItemId nroot;
         while (root)
         {
            nroot = AppendItem(GetRootItem(), wxString(root->GetItem()->name().c_str(),wxConvUTF8));
            collectChildren(root, nroot, hier);
            root = root->GetNextRoot(TARGETDB_LIB);
         }
      }
   }
   DATC->unlockCIF();
   SortChildren(GetRootItem());
}

void browsers::CIFCellBrowser::collectChildren(const CIFin::CIFHierTree* root,
                                               const wxTreeItemId& lroot, bool _hierarchy_view)
{
   const CIFin::CIFHierTree* Child= root->GetChild(TARGETDB_LIB);
   wxTreeItemId nroot;
   wxTreeItemId temp;

   while (Child)
   {
      if (_hierarchy_view)
      {
         nroot = AppendItem(lroot, wxString(Child->GetItem()->name().c_str(), wxConvUTF8));
         collectChildren(Child, nroot, _hierarchy_view);
      }
      else
      {
         if (!findItem(wxString(Child->GetItem()->name().c_str(), wxConvUTF8), temp, GetRootItem()))
         {
            nroot = AppendItem(GetRootItem(), wxString(Child->GetItem()->name().c_str(), wxConvUTF8));
            collectChildren(Child, nroot, _hierarchy_view);
         }
      }
      SortChildren(lroot);
      Child = Child->GetBrother(TARGETDB_LIB);
   }
}


//==============================================================================
//
// TDTbrowser
//
//==============================================================================
BEGIN_EVENT_TABLE(browsers::TDTbrowser, wxPanel)
   EVT_MENU(tui::TMCELL_REPORTLAY, browsers::TDTbrowser::onReportUsedLayers)
   EVT_BUTTON(BT_CELLS_HIER, browsers::TDTbrowser::onHierView)
   EVT_BUTTON(BT_CELLS_FLAT, browsers::TDTbrowser::onFlatView)
END_EVENT_TABLE()

browsers::TDTbrowser::TDTbrowser(wxWindow *parent, wxWindowID id,
                              const wxPoint& pos, const wxSize& size, long style) :
      wxPanel(parent, id, pos, size)
{
   _hierarchy_view = true;
   wxBoxSizer *thesizer = DEBUG_NEW wxBoxSizer( wxVERTICAL );
   wxBoxSizer *sizer1   = DEBUG_NEW wxBoxSizer( wxHORIZONTAL );
   _hierButton = DEBUG_NEW wxButton( this, BT_CELLS_HIER, wxT("Hier") );
   _flatButton = DEBUG_NEW wxButton( this, BT_CELLS_FLAT, wxT("Flat") );
   //Set bold font for _hierButton
   wxFont font = _hierButton->GetFont();
   font.SetWeight(wxFONTWEIGHT_BOLD);
   _hierButton->SetFont(font);

   sizer1->Add(_hierButton, 1, wxEXPAND|wxBOTTOM, 3);
   sizer1->Add(_flatButton, 1, wxEXPAND|wxBOTTOM, 3);
   _cellBrowser = DEBUG_NEW CellBrowser(this, tui::ID_TPD_CELLTREE, pos, size, style | wxTR_HIDE_ROOT);
   thesizer->Add(_cellBrowser, 1, wxEXPAND | wxBOTTOM);
   thesizer->Add(sizer1, 0, wxEXPAND | wxALL);

   _imageList = DEBUG_NEW wxImageList(16, 16, TRUE);
   _imageList->Add( wxIcon( cell_normal   ) );
   _imageList->Add( wxIcon( cell_expanded ) );

   _cellBrowser->SetImageList(_imageList);
   SetSizerAndFit(thesizer);
   thesizer->SetSizeHints( this );
}

void browsers::TDTbrowser::onFlatView( wxCommandEvent& event )
{
   _hierarchy_view = false;

   wxString cell_top_str = _cellBrowser->topCellName();
   wxString cell_act_str = _cellBrowser->activeCellName();
   wxString cell_sel_str = _cellBrowser->selectedCellName();
   _cellBrowser->collectInfo(_hierarchy_view);
   _cellBrowser->statusHighlight(cell_top_str, cell_act_str, cell_sel_str);

   //Set normal font for  _hierButton
   wxFont font = _hierButton->GetFont();
   font.SetWeight(wxFONTWEIGHT_NORMAL);
   _hierButton->SetFont(font);
   //Set bold font for _flatButton;
   font = _flatButton->GetFont();
   font.SetWeight(wxFONTWEIGHT_BOLD);
   _flatButton->SetFont(font);

   Show();
   (this->GetSizer())->Layout();

}

void browsers::TDTbrowser::onHierView( wxCommandEvent& event )
{
   _hierarchy_view = true;

   wxString cell_top_str = _cellBrowser->topCellName();
   wxString cell_act_str = _cellBrowser->activeCellName();
   wxString cell_sel_str = _cellBrowser->selectedCellName();
   _cellBrowser->collectInfo(_hierarchy_view);
   _cellBrowser->statusHighlight(cell_top_str, cell_act_str, cell_sel_str);

   //Set bold  font for  _hierButton
   wxFont font = _hierButton->GetFont();
   font.SetWeight(wxFONTWEIGHT_BOLD);
   _hierButton->SetFont(font);
   //Set normal  font for _flatButton;
   font = _flatButton->GetFont();
   font.SetWeight(wxFONTWEIGHT_NORMAL);
   _flatButton->SetFont(font);

   Show();
   (this->GetSizer())->Layout();
}

void browsers::TDTbrowser::onReportUsedLayers(wxCommandEvent& WXUNUSED(event))
{
   wxString cmd;
   cmd << wxT("report_layers(\"") << selectedCellName() << wxT("\" , true);");
   parseCommand(cmd);
}

wxString browsers::TDTbrowser::selectedCellName() const
{
   return _cellBrowser->selectedCellName();
}

browsers::TDTbrowser::~TDTbrowser()
{
   _imageList->RemoveAll();
   delete _imageList;
   _cellBrowser->DeleteAllItems();
   delete _cellBrowser;
}

//==============================================================================
//
// XdbBrowser (External Data base browser)
//
//==============================================================================
BEGIN_EVENT_TABLE(browsers::XdbBrowser, wxPanel)
   EVT_BUTTON(BT_CELLS_HIER2, browsers::XdbBrowser::onHierView)
   EVT_BUTTON(BT_CELLS_FLAT2, browsers::XdbBrowser::onFlatView)
END_EVENT_TABLE()
//==============================================================================
browsers::XdbBrowser::XdbBrowser(   wxWindow *parent,
                                    wxWindowID id,
                                    const wxPoint& pos ,
                                    const wxSize& size ,
                                    long style ):
      wxPanel(parent, id, pos, size, style)
{
   _hierarchy_view = true;
   switch (id)
   {
      case tui::ID_CIF_CELLTREE:
         _cellBrowser = DEBUG_NEW CIFCellBrowser(this, tui::ID_CIF_CELLTREE, pos, size, style);
         break;
      case tui::ID_GDS_CELLTREE:
         _cellBrowser = DEBUG_NEW GDSCellBrowser(this, tui::ID_GDS_CELLTREE, pos, size, style);
         break;
      default: assert(false);
   }

   wxBoxSizer *thesizer = DEBUG_NEW wxBoxSizer( wxVERTICAL );

   wxBoxSizer *sizer1 = DEBUG_NEW wxBoxSizer( wxHORIZONTAL );
   _hierButton = DEBUG_NEW wxButton( this, BT_CELLS_HIER2, wxT("Hier") );
   //Set bold font for _hierButton
   wxFont font = _hierButton->GetFont();
   font.SetWeight(wxFONTWEIGHT_BOLD);
   _hierButton->SetFont(font);

   _flatButton = DEBUG_NEW wxButton( this, BT_CELLS_FLAT2, wxT("Flat") );

   sizer1->Add(_hierButton, 1, wxEXPAND|wxBOTTOM, 3);
   sizer1->Add(_flatButton, 1, wxEXPAND|wxBOTTOM, 3);

   thesizer->Add(_cellBrowser, 1, wxEXPAND | wxBOTTOM);
   thesizer->Add(sizer1, 0, wxEXPAND | wxALL);

   SetSizerAndFit(thesizer);
   thesizer->SetSizeHints( this );
}

wxString browsers::XdbBrowser::selectedCellName() const
{
   return _cellBrowser->selectedCellName();
}

void browsers::XdbBrowser::deleteAllItems(void)
{
   _cellBrowser->DeleteAllItems();
}

void browsers::XdbBrowser::onFlatView(wxCommandEvent& event)
{
   if (!_hierarchy_view) return;
   _hierarchy_view = false;

   wxString cell_sel_str = _cellBrowser->selectedCellName();
   _cellBrowser->collectInfo(_hierarchy_view);
   _cellBrowser->statusHighlight(wxT(""), wxT(""), cell_sel_str);

   //Set normal font for  _hierButton 
   wxFont font = _flatButton->GetFont();
   _hierButton->SetFont(font);
   //Set bold font for _flatButton;
   font.SetWeight(wxFONTWEIGHT_BOLD);
   _flatButton->SetFont(font);
}

void browsers::XdbBrowser::onHierView(wxCommandEvent& event)
{
   if (_hierarchy_view) return;
   _hierarchy_view = true;

   wxString cell_sel_str = _cellBrowser->selectedCellName();
   _cellBrowser->collectInfo(_hierarchy_view);
   _cellBrowser->statusHighlight(wxT(""), wxT(""), cell_sel_str);

   //Set normal  font for _flatButton;
   wxFont font = _hierButton->GetFont();
   _flatButton->SetFont(font);
   //Set bold font for _hierButton
   font.SetWeight(wxFONTWEIGHT_BOLD);
   _hierButton->SetFont(font);
}

//==============================================================================
BEGIN_EVENT_TABLE(browsers::browserTAB, wxAuiNotebook)
   EVT_TECUSTOM_COMMAND(wxEVT_CMD_BROWSER, wxID_ANY, browsers::browserTAB::onCommand)
END_EVENT_TABLE()
//==============================================================================
browsers::browserTAB::browserTAB(wxWindow *parent, wxWindowID id,const 
   wxPoint& pos, const wxSize& size, long style) : 
                                 wxAuiNotebook(parent, id, pos, size, style) 
{
   _tdtStruct = DEBUG_NEW TDTbrowser(this, tui::ID_TPD_CELLTREE);
   AddPage(_tdtStruct, wxT("Cells"));
   _layers = DEBUG_NEW LayerBrowser(this,  tui::ID_TPD_LAYERS);
   AddPage(_layers, wxT("Layers"));

   _gdsStruct = NULL;
   _cifStruct = NULL;
	 _drcStruct = NULL;
   _tellParser = NULL;
   Browsers = this;
}

browsers::browserTAB::~browserTAB() 
{
//   It appears that wx is calling automatically the destructors of the
//   child windows, so no need to call them here
//   delete _tdtStruct; _tdtStruct = NULL;
//   delete _TDTlayers; _TDTlayers = NULL;
}

wxString browsers::browserTAB::tdtSelectedGdsName() const
{
   if (NULL != _gdsStruct)
      return _gdsStruct->selectedCellName();
   else return wxT("");
}

wxString browsers::browserTAB::tdtSelectedCifName() const
{
   if (NULL != _cifStruct)
      return _cifStruct->selectedCellName();
   else 
      return wxT("");
}

void browsers::browserTAB::onCommand(wxCommandEvent& event)
{
   int command = event.GetInt();
   switch (command) 
   {
      case BT_ADDTDT_LIB:onTellAddTdtLib();break;
      case BT_ADDGDS_TAB:onTellAddGdsTab();break;
      case BT_CLEARGDS_TAB:onTellClearGdsTab(); break;
      case BT_ADDCIF_TAB:onTellAddCifTab();break;
      case BT_CLEARCIF_TAB:onTellClearCifTab(); break;
		case BT_ADDDRC_TAB:onTellAddDRCTab();break;
      case BT_CLEARDRC_TAB:onTellClearDRCTab(); break;
      default: event.Skip();
   }
}

void browsers::browserTAB::onTellAddTdtLib()
{
   _tdtStruct->collectInfo();
}

void browsers::browserTAB::onTellAddGdsTab()
{
   if (!_gdsStruct)
   {
      _gdsStruct = DEBUG_NEW XdbBrowser(this, tui::ID_GDS_CELLTREE);
      AddPage(_gdsStruct, wxT("GDS"));
   }
   // don't bother to clean-up existing DB. It's done in the function called
   _gdsStruct->collectInfo();
}

void browsers::browserTAB::onTellClearGdsTab() 
{
   if (_gdsStruct)
   {
      int _gdsPageIndex = GetPageIndex(_gdsStruct);
      assert(wxNOT_FOUND != _gdsPageIndex);
      _gdsStruct->deleteAllItems();
      DeletePage(_gdsPageIndex);
      _gdsStruct = NULL;
   }
}

void browsers::browserTAB::onTellAddCifTab()
{
   if (NULL == _cifStruct)
   {
      _cifStruct = DEBUG_NEW XdbBrowser(this, tui::ID_CIF_CELLTREE);
      AddPage(_cifStruct, wxT("CIF"));
   }
   // don't bother to clean-up existing DB. It's done in the function called
   _cifStruct->collectInfo();
}

void browsers::browserTAB::onTellClearCifTab()
{
   if (_cifStruct)
   {
      int _cifPageIndex = GetPageIndex(_cifStruct);
      assert(wxNOT_FOUND != _gdsPageIndex);
      _cifStruct->deleteAllItems();
      DeletePage(_cifPageIndex);
      _cifStruct = NULL;
   }
}

void browsers::browserTAB::onTellAddDRCTab()
{
	if (NULL == _drcStruct)
   {
      _drcStruct = DEBUG_NEW DRCBrowser(this, tui::ID_DRC_CELLTREE);
      AddPage(_drcStruct, wxT("DRC results"));
   }
}

void browsers::browserTAB::onTellClearDRCTab()
{
	if (_drcStruct)
   {
     int _drcPageIndex = GetPageIndex(_drcStruct);
      assert(wxNOT_FOUND != _drcPageIndex);
      _drcStruct->deleteAllItems();
      DeletePage(_drcPageIndex);
      _drcStruct = NULL;
   }
}

//==============================================================================
void browsers::layer_status(BROWSER_EVT_TYPE btype, const word layno, const bool status) 
{
   assert(Browsers);
   int* bt1 = DEBUG_NEW int(btype);
   wxCommandEvent eventLAYER_STATUS(wxEVT_CMD_BROWSER);
   eventLAYER_STATUS.SetExtraLong(status);
   eventLAYER_STATUS.SetInt(*bt1);
   word *laynotemp = DEBUG_NEW word(layno);
   eventLAYER_STATUS.SetClientData(static_cast<void*> (laynotemp));
   wxPostEvent(Browsers->tdtLayers()->getLayerPanel(), eventLAYER_STATUS);
   delete bt1;
}

void browsers::layer_add(const std::string name, const word layno) 
{
   assert(Browsers);
   wxCommandEvent eventLAYER_ADD(wxEVT_CMD_BROWSER);
   LayerInfo *layer = DEBUG_NEW LayerInfo(name, layno);
   int* bt = DEBUG_NEW int(BT_LAYER_ADD);
   eventLAYER_ADD.SetClientData(static_cast<void*> (layer));
   eventLAYER_ADD.SetInt(*bt);

   wxPostEvent(Browsers->tdtLayers()->getLayerPanel(), eventLAYER_ADD);
   delete bt;
}

void browsers::layer_default(const word newlay, const word oldlay) 
{
   assert(Browsers);
   wxCommandEvent eventLAYER_DEF(wxEVT_CMD_BROWSER);
   int*bt = DEBUG_NEW int(BT_LAYER_DEFAULT);
   eventLAYER_DEF.SetExtraLong(newlay);
   word *laynotemp = DEBUG_NEW word(oldlay);
   eventLAYER_DEF.SetClientData(static_cast<void*> (laynotemp));
   eventLAYER_DEF.SetInt(*bt);
   
   wxPostEvent(Browsers->tdtLayers()->getLayerPanel(), eventLAYER_DEF);
   delete bt;
}

void browsers::addTDTtab(bool newthread)
{
   assert(Browsers);
   wxCommandEvent eventADDTAB(wxEVT_CMD_BROWSER);
   eventADDTAB.SetInt(BT_ADDTDT_LIB);
//   eventADDTAB.SetClientData(static_cast<void*> ( tdtLib));
//   eventADDTAB.SetExtraLong(traverse_all ? 1 : 0);
   // Note about threads here!
   // Traversing the entire hierarchy tree can not be done in a
   // separate thread. The main reason - when executing a script
   // that contains for example:
   //    new("a")); addcell("b");
   // it's quite possible that cell hierarchy will be traversed
   // after the execution of the second function. The latter will
   // send treeAddMember itself - in result the browser window
   // will get cell b twice. Bottom line: don't use PostEvent here!
   if (newthread)
      wxPostEvent( Browsers, eventADDTAB );
   else
      Browsers->GetEventHandler()->ProcessEvent( eventADDTAB );
   // the alternative is to call the function directly
//   Browsers->OnTELLaddTDTlib(tdtLib, traverse_all);
}

void browsers::addGDStab() 
{
   assert(Browsers);
   wxCommandEvent eventADDTAB(wxEVT_CMD_BROWSER);
   eventADDTAB.SetInt(BT_ADDGDS_TAB);
   wxPostEvent(Browsers, eventADDTAB);
}

void browsers::addCIFtab()
{
   assert(Browsers);
   wxCommandEvent eventADDTAB(wxEVT_CMD_BROWSER);
   eventADDTAB.SetInt(BT_ADDCIF_TAB);
   wxPostEvent(Browsers, eventADDTAB);
}

void browsers::addDRCtab()
{
   assert(Browsers);
   wxCommandEvent eventADDTAB(wxEVT_CMD_BROWSER);
   eventADDTAB.SetInt(BT_ADDDRC_TAB);
   wxPostEvent(Browsers, eventADDTAB);
}

void browsers::clearGDStab() 
{
   assert(Browsers);
   wxCommandEvent eventADDTAB(wxEVT_CMD_BROWSER);
   eventADDTAB.SetInt(BT_CLEARGDS_TAB);
   wxPostEvent(Browsers, eventADDTAB);
}

void browsers::clearCIFtab()
{
   assert(Browsers);
   wxCommandEvent eventADDTAB(wxEVT_CMD_BROWSER);
   eventADDTAB.SetInt(BT_CLEARCIF_TAB);
   wxPostEvent(Browsers, eventADDTAB);
}

void browsers::clearDRCtab()
{
   assert(Browsers);
   wxCommandEvent eventADDTAB(wxEVT_CMD_BROWSER);
   eventADDTAB.SetInt(BT_CLEARDRC_TAB);
   wxPostEvent(Browsers, eventADDTAB);
}
void browsers::celltree_open(const std::string cname) 
{
   assert(Browsers);
   wxCommandEvent eventCELLTREE(wxEVT_CMD_BROWSER);
   eventCELLTREE.SetInt(BT_CELL_OPEN);
   eventCELLTREE.SetString(wxString(cname.c_str(), wxConvUTF8));
   wxPostEvent(Browsers->tdtCellBrowser(), eventCELLTREE);
}

void browsers::celltree_highlight(const std::string cname) 
{
   assert(Browsers);
   wxCommandEvent eventCELLTREE(wxEVT_CMD_BROWSER);
   eventCELLTREE.SetInt(BT_CELL_HIGHLIGHT);
   eventCELLTREE.SetString(wxString(cname.c_str(), wxConvUTF8));
   wxPostEvent(Browsers->tdtCellBrowser(), eventCELLTREE);
}

void browsers::treeAddMember(const char* cell, const char* parent, int action) 
{
   assert(Browsers);
   wxCommandEvent eventCELLTREE(wxEVT_CMD_BROWSER);
   eventCELLTREE.SetInt(BT_CELL_ADD);
   eventCELLTREE.SetString(wxString(cell, wxConvUTF8));
   eventCELLTREE.SetExtraLong(action);
   wxString* prnt = DEBUG_NEW wxString(parent, wxConvUTF8);
   eventCELLTREE.SetClientData(static_cast<void*> (prnt));
   wxPostEvent(Browsers->tdtCellBrowser(), eventCELLTREE);
}

void browsers::treeRemoveMember(const char* cell, const char* parent, int action) 
{
   assert(Browsers);
   wxCommandEvent eventCELLTREE(wxEVT_CMD_BROWSER);
   eventCELLTREE.SetInt(BT_CELL_REMOVE);
   eventCELLTREE.SetString(wxString(cell, wxConvUTF8));
   eventCELLTREE.SetExtraLong(action);
   wxString* prnt = DEBUG_NEW wxString(parent, wxConvUTF8);
   eventCELLTREE.SetClientData(static_cast<void*> (prnt));
   wxPostEvent(Browsers->tdtCellBrowser(), eventCELLTREE);
}

void browsers::parseCommand(const wxString cmd)
{
   // Note! The use of wxPostEvent here is not enforced by a thread safety
   // reasons. This function as well as its callers and the event destination -
   // all of them are running in the main thread and parser can be called directly.
   // This simply avoids unnesessary links between the modules using the "magic"
   // of the wx event system
   assert(Browsers && Browsers->tellParser());
   wxCommandEvent eventPARSE(wxEVT_CONSOLE_PARSE);
   eventPARSE.SetString(cmd);
   wxPostEvent(Browsers->tellParser(), eventPARSE);
}

browsers::LayerInfo::LayerInfo(const LayerInfo& lay)
{
   _name = lay._name;
   _layno = lay._layno;
   _col = lay._col;
   _fill = lay._fill;
}

browsers::LayerInfo::LayerInfo(const std::string &name, const word layno)
{
   _name    = name;
   _layno   = layno;
   _col      = DATC->getColorName(layno);
   _fill      = DATC->getFillName(layno);
};

BEGIN_EVENT_TABLE(browsers::LayerButton, wxPanel)
   //EVT_COMMAND_RANGE(12000,  12100, wxEVT_COMMAND_BUTTON_CLICKED, LayerButton::OnClick)
   EVT_LEFT_DOWN(LayerButton::onLeftClick)
   EVT_MIDDLE_DOWN(LayerButton::onMiddleClick)
   EVT_PAINT(LayerButton::onPaint)
END_EVENT_TABLE()

browsers::LayerButton::LayerButton(wxWindow* parent, wxWindowID id,  const wxPoint& pos , 
                                   const wxSize& size, long style , const wxValidator& validator ,
                                   const wxString& name, LayerInfo *layer)
{
   
   _layer   = DEBUG_NEW LayerInfo(*layer);
   _selected= false;
   _hidden  = false;
   
   //_locked  = false;  
   
   _picture = DEBUG_NEW wxBitmap(size.GetWidth()-16, size.GetHeight(), -1);

   const byte *ifill= DATC->getFill(layer->layno());
   const layprop::tellRGB col = DATC->getColor(layer->layno());
   wxColour color(col.red(), col.green(), col.blue());

   if(ifill!=NULL)
   {     
      wxBitmap *stipplebrush = DEBUG_NEW wxBitmap((char  *)ifill, 32, 32, 1);
      wxImage image;
      image = stipplebrush->ConvertToImage();
#ifdef WIN32
 //Change white color for current one
      int w = image.GetWidth();
      int h = image.GetHeight();
      for (int i=0; i<w; i++)
         for (int j=0; j<h; j++)
         {
            if((image.GetRed(i,j)==0)&& (image.GetGreen(i,j)==0) && (image.GetBlue(i,j)==0))
            {
               image.SetRGB(i, j, col.red(), col.green(), col.blue());
            }
            else
            {
               image.SetRGB(i, j, 0, 0, 0);
            }
         }
      delete stipplebrush;

      //Recreate bitmap with new color
      stipplebrush = DEBUG_NEW wxBitmap(image, 1);
#endif
      _brush = DEBUG_NEW wxBrush(   *stipplebrush);
      delete stipplebrush;
   }
   else
   {
     //???Warning!!!
      //if (NULL != col)
     //    _brush = DEBUG_NEW wxBrush(color, wxTRANSPARENT);
     // else
         _brush = DEBUG_NEW wxBrush(*wxLIGHT_GREY, wxTRANSPARENT);
   }

   _pen = DEBUG_NEW wxPen();
            
   //if (col!=NULL)
   //{
      _pen->SetColour(color);
      _brush->SetColour(color);
   //}
         
   Create(parent, id,  pos, size, style, name);
   GetClientSize(&_buttonWidth, &_buttonHeight);
   //***Draw main picture***   
   preparePicture();
   
   wxString caption(_layer->name().c_str(),wxConvUTF8);
   SetToolTip(caption);
   
}

void browsers::LayerButton::preparePicture()
{
   const int clearence = 2;
   int sizeX, sizeY;
   GetParent()->GetClientSize(&sizeX, &sizeY);
   _buttonWidth = sizeX;
   SetSize(_buttonWidth, _buttonHeight);
   
   wxMemoryDC DC;
   wxFont font(10,wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
   DC.SetFont(font);
   //pict.SetWidth(_buttonWidth);
   if (_picture)
   {
      delete _picture;
      _picture = DEBUG_NEW wxBitmap(_buttonWidth-16, _buttonHeight, -1);
   }
   DC.SelectObject(*_picture);

   DC.SetBrush(*_brush);
   DC.SetPen(*_pen);
   DC.SetBackground(*wxBLACK);
   DC.SetTextForeground(*wxWHITE);

   DC.Clear();
   int curw = clearence;
   
   char temp[100];
   sprintf(temp, "%3i", _layer->layno());
   wxString layno(temp, wxConvUTF8);
   int hno,wno;
   DC.GetTextExtent(layno, &wno, &hno);
   DC.DrawText(layno, 0, int((_buttonHeight - hno)/2));
   curw += wno + clearence;
   
   wxBrush tempBrush = DC.GetBrush();
   if (_selected)
   {
      DC.SetBrush(*wxWHITE_BRUSH);
      DC.SetTextForeground(*wxBLACK);
   }
   else
   {
      DC.SetBrush(*wxBLACK_BRUSH);
      DC.SetTextForeground(*wxWHITE);
   }
   const wxString dummy= _T("WWWWWWWWWW");
   wxString caption(_layer->name().c_str(),wxConvUTF8);
   if (caption.Len()>10)
   {
      caption[7]=wxT('.');
      caption[8]=wxT('.');
      caption[9]=wxT('.');
   }
   int hna,wna;
   DC.GetTextExtent(dummy, &wna, &hna);
   DC.DrawRectangle(curw, clearence, wna, _buttonHeight - 2*clearence);
   curw += clearence;
   DC.DrawText(caption, curw, int(_buttonHeight/2 - hna/2));
   curw += wna;
   
   DC.SetBrush(tempBrush);
   DC.DrawRectangle(curw, clearence, _buttonWidth-curw-16, _buttonHeight-2*clearence);

   DC.SelectObject(wxNullBitmap);
   Refresh();
}


browsers::LayerButton::~LayerButton()
{
   delete _picture;

   delete _brush;
   delete _pen;
   delete _layer;
}


void browsers::LayerButton::onPaint(wxPaintEvent&event)
{
   wxPaintDC dc(this);
   dc.DrawBitmap(*_picture, 0, 0, false);
   if (_selected)
   {
      dc.DrawIcon(wxIcon(activelay),_buttonWidth-16,15);
   }
   else if (DATC->layerLocked(_layer->layno()))
   {
      dc.DrawIcon(wxIcon(lock),_buttonWidth-16,15);
   }

   if (DATC->layerHidden(_layer->layno()))
   {
      dc.DrawIcon(wxIcon(nolay_xpm),_buttonWidth-16,0);
   }
}

void browsers::LayerButton::onLeftClick(wxMouseEvent &event)
{
	//???Warning! Here istemporary solution for right vusulalisation of layer buttons
	//after resizing
	LayerPanel *parent =  static_cast<browsers::LayerPanel*> (GetParent());
	if(_buttonWidth!=parent->GetClientSize().GetWidth())
	{
		parent->refresh();
	}
	//???

   if (event.ShiftDown())
   //Lock layer
   {
      //_hidden = !_hidden;
      hideLayer(!_hidden);
      wxString cmd;
      cmd << wxT("hidelayer(") <<_layer->layno() << wxT(", ");
      if (_hidden) cmd << wxT("true") << wxT(");");
      else cmd << wxT("false") << wxT(");");
      parseCommand(cmd);
   }
   else
   //Select layer
   {
      wxString cmd;
      cmd << wxT("usinglayer(") << _layer->layno()<< wxT(");");
      parseCommand(cmd);

      if (!_selected)
      {
         select();

         //Next block uses for unselect previous button
         int bt = BT_LAYER_SELECT;
         wxCommandEvent eventLAYER_SELECT(wxEVT_CMD_BROWSER);

         eventLAYER_SELECT.SetExtraLong(_layer->layno());

         eventLAYER_SELECT.SetInt(bt);
         assert(Browsers && Browsers->tdtLayers());
         wxPostEvent(Browsers->tdtLayers()->getLayerPanel(), eventLAYER_SELECT);
      }
   }
}

void browsers::LayerButton::onMiddleClick(wxMouseEvent &event)
{
	//???Warning! Here istemporary solution for right vusulalisation of layer buttons
	//after resizing
	LayerPanel *parent =  static_cast<browsers::LayerPanel*> (GetParent());
	if(_buttonWidth!=parent->GetClientSize().GetWidth())
	{
		parent->refresh();
	}
	//???

   //_locked = !_locked;
   wxString cmd;
   cmd << wxT("locklayer(") <<_layer->layno() << wxT(", ");
   if (DATC->layerLocked(_layer->layno())) cmd << wxT("false") << wxT(");");
   else cmd << wxT("true") << wxT(");");
   parseCommand(cmd);

}


void browsers::LayerButton::hideLayer(bool hide)
{
   _hidden = hide;
   preparePicture();
}

void browsers::LayerButton::lockLayer(bool lock)
{
   //_locked = lock;
   preparePicture();
}

void browsers::LayerButton::select(void)
{
   _selected = true;
   preparePicture();
}

void browsers::LayerButton::unselect(void)
{
   _selected = false;
   preparePicture();
}

//====================================================================
BEGIN_EVENT_TABLE(browsers::LayerPanel, wxScrolledWindow)
   EVT_TECUSTOM_COMMAND(wxEVT_CMD_BROWSER, wxID_ANY, browsers::LayerPanel::onCommand)
   EVT_SIZE(browsers::LayerPanel::OnSize)
END_EVENT_TABLE()
//====================================================================
browsers::LayerPanel::LayerPanel(wxWindow* parent, wxWindowID id, 
                              const wxPoint& pos,
                              const wxSize& size,
                              long style , const wxString& name)
                              :wxScrolledWindow(parent, id, pos, size, style, name),
                               _selectedButton(NULL)
{
      _buttonCount = 0;
}

browsers::LayerPanel::~LayerPanel() 
{
}

browsers::LayerButton* browsers::LayerPanel::checkDefined(word key)
{
   if (_buttonMap.end() == _buttonMap.find(key)) return NULL;
   return _buttonMap[key];
}

void browsers::LayerPanel::onCommand(wxCommandEvent& event)
{
   int command = event.GetInt();
   LayerButton* wbutton;
   switch (command) 
   {

      case BT_LAYER_DEFAULT:
         {
            word *oldlay = static_cast<word*>(event.GetClientData());
            word layno = event.GetExtraLong();
            if ((wbutton = checkDefined(*oldlay))) wbutton->unselect();
            if ((wbutton = checkDefined( layno ))) wbutton->select();
            //_layerlist->defaultLayer((word)event.GetExtraLong(), (word)event.GetInt());
            delete (oldlay);
            break;
         }
      case    BT_LAYER_HIDE:
         {
            word *layno = static_cast<word*>(event.GetClientData());
            bool status = (1 == event.GetExtraLong());
            if ((wbutton = checkDefined(*layno))) wbutton->hideLayer(status);
            delete (layno);
            break;
         }
         //_layerlist->hideLayer((word)event.GetExtraLong(),event.IsChecked());break;
      case    BT_LAYER_LOCK:
         {
            //_layerlist->lockLayer((word)event.GetExtraLong(),event.IsChecked());
            word *layno = static_cast<word*>(event.GetClientData());
            bool status = (1 == event.GetExtraLong());
            if ((wbutton = checkDefined(*layno))) wbutton->lockLayer(status);
            delete (layno);
            break;
         }
      case     BT_LAYER_SELECT:
         {
            word layno = event.GetExtraLong();
            if (NULL != _selectedButton) _selectedButton->unselect();
            if ((wbutton = checkDefined( layno))) _selectedButton = _buttonMap[layno];
            break;
         }

      case     BT_LAYER_ADD:
         {
            LayerInfo* layer = static_cast<LayerInfo*>(event.GetClientData());
				addButton(layer);
				delete (static_cast<LayerInfo*>(layer));
				break;
         }
      default: assert(false);
   }
}

void	browsers::LayerPanel::addButton(LayerInfo *layer)
{
	   LayerButton* wbutton;
      int szx, szy;

      //Remove selection from current button
      if (NULL != _selectedButton) _selectedButton->unselect();
      if ((wbutton = checkDefined( layer->layno() )))
	   {
			//Button already exists, replace it
         //layerButton = DEBUG_NEW LayerButton(this, tui::TMDUMMY_LAYER+_buttonCount, wxPoint (0, _buttonCount*30), wxSize(200, 30),
         //wxBU_AUTODRAW, wxDefaultValidator, _T("TTT"), layer);
         int x, y;
         int ID;
         wbutton->GetPosition(&x, &y);
         wbutton->GetSize(&szx, &szy);
         ID = wbutton->GetId();
         LayerButton* layerButton = DEBUG_NEW LayerButton(this, ID, wxPoint (x, y), wxSize(szx, szy),
            wxBU_AUTODRAW|wxNO_BORDER, wxDefaultValidator, _T("button"), layer);
         _buttonMap[layer->layno()] = layerButton;
         delete wbutton;
      }
      else
      {
			//Button doesn't exist, create new button
         GetClientSize(&szx, &szy);
         LayerButton* layerButton = DEBUG_NEW LayerButton(this, tui::TMDUMMY_LAYER+_buttonCount,
															wxPoint (0, _buttonCount*buttonHeight), wxSize(szx, buttonHeight),
															wxBU_AUTODRAW, wxDefaultValidator, _T("button"), layer);
         _buttonMap[layer->layno()] = layerButton;
         _buttonCount++;
         this->SetScrollbars(0, buttonHeight, 0, _buttonCount);
			//Reorder buttons
			int number = 0;
			for(LayerButtonMap::iterator it=_buttonMap.begin() ;it!=_buttonMap.end(); ++it, ++number)
			{
				LayerButton* tempButton = (*it).second;
				wxPoint point = wxPoint(0, number*buttonHeight);
				tempButton->Move(point);
			}
      }
      //Restore selection
      if ((wbutton = checkDefined( DATC->curlay())))
      {
			_selectedButton = wbutton;
         _selectedButton->select();
      }
}

void browsers::LayerPanel::onSize(wxSizeEvent& evt)
{
   for(LayerButtonMap::const_iterator it = _buttonMap.begin(); it!=_buttonMap.end();++it)
   {
      LayerButton *button = it->second;
      button->preparePicture();
   }
   Refresh();
}

void	browsers::LayerPanel::refresh(void)
{
   for(LayerButtonMap::const_iterator it = _buttonMap.begin(); it!=_buttonMap.end();++it)
   {
      LayerButton *button = it->second;
      button->preparePicture();
   }
   Refresh();
}

wxString browsers::LayerPanel::getAllSelected()
{
      //bool multi_selection = _layerlist->GetSelectedItemCount() > 1;
	if (_buttonMap.empty()) return wxEmptyString;
   wxString layers = wxT("{");
   for(LayerButtonMap::iterator it = _buttonMap.begin(); it != _buttonMap.end(); it++)
   {
      word layNo = (it->second)->getLayNo();
      layers << wxT(" ") <<  layNo << wxT(",");
   }
   layers.RemoveLast();
   layers << wxT("}");
   return layers;
}

//====================================================================
BEGIN_EVENT_TABLE(browsers::LayerBrowser, wxPanel)
   EVT_BUTTON(BT_LAYER_SHOW_ALL, browsers::LayerBrowser::onShowAll)
   EVT_BUTTON(BT_LAYER_HIDE_ALL, browsers::LayerBrowser::onHideAll)
   EVT_BUTTON(BT_LAYER_LOCK_ALL, browsers::LayerBrowser::onLockAll)
   EVT_BUTTON(BT_LAYER_UNLOCK_ALL, browsers::LayerBrowser::onUnlockAll)
   EVT_SIZE(browsers::LayerBrowser::OnSize)
END_EVENT_TABLE()
//====================================================================


browsers::LayerBrowser::LayerBrowser(wxWindow* parent, wxWindowID id) 
   :wxPanel(parent, id, wxDefaultPosition, wxDefaultSize),
   _layerPanel(NULL),
   _thesizer(NULL)
{
   _thesizer = DEBUG_NEW wxBoxSizer(wxVERTICAL);

   wxBoxSizer *sizer1 = DEBUG_NEW wxBoxSizer(wxHORIZONTAL);
   sizer1->Add(DEBUG_NEW wxButton(this, BT_LAYER_SHOW_ALL, wxT("Show All")), 1, wxTOP, 3);
   sizer1->Add(DEBUG_NEW wxButton(this, BT_LAYER_HIDE_ALL, wxT("Hide All")), 1, wxTOP, 3);
   _thesizer->Add(sizer1, 0, wxTOP);

   wxBoxSizer *sizer2 = DEBUG_NEW wxBoxSizer(wxHORIZONTAL);
   sizer2->Add(DEBUG_NEW wxButton(this, BT_LAYER_LOCK_ALL, wxT("Lock All")), 1, wxTOP, 3);
   sizer2->Add(DEBUG_NEW wxButton(this, BT_LAYER_UNLOCK_ALL, wxT("Unlock All")), 1, wxTOP, 3);
   _thesizer->Add(sizer2, 0, wxTOP);

   _layerPanel = DEBUG_NEW LayerPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
   _thesizer->Add(_layerPanel, 3, wxEXPAND|wxALL);
   SetSizerAndFit(_thesizer);
   _thesizer->SetSizeHints( this );
}

browsers::LayerBrowser::~LayerBrowser() 
{
}


void browsers::LayerBrowser::onShowAll(wxCommandEvent& WXUNUSED(event))
{
   wxString cmd;
	wxString layers=getAllSelected(); 
	if (layers != wxEmptyString)
	{
		cmd << wxT("hidelayer(") << getAllSelected() << wxT(", false);");
		parseCommand(cmd);
	}
}

void browsers::LayerBrowser::onHideAll(wxCommandEvent& WXUNUSED(event))
{
	wxString cmd;
	wxString layers=getAllSelected(); 
	if (layers != wxEmptyString)
	{
		cmd << wxT("hidelayer(") << getAllSelected() << wxT(", true);");
		parseCommand(cmd);
	}
}

void browsers::LayerBrowser::onLockAll(wxCommandEvent& WXUNUSED(event))
{
	wxString cmd;
	wxString layers=getAllSelected(); 
	if (layers != wxEmptyString)
	{
		cmd << wxT("locklayer(") << getAllSelected() << wxT(", true);");
		parseCommand(cmd);
	}

}


void browsers::LayerBrowser::onUnlockAll(wxCommandEvent& WXUNUSED(event))
{
	wxString cmd;
	wxString layers=getAllSelected(); 
	if (layers != wxEmptyString)
	{
		cmd << wxT("locklayer(") << getAllSelected() << wxT(", false);");
		parseCommand(cmd);
	}

}

wxString browsers::LayerBrowser::getAllSelected()
{
   return _layerPanel->getAllSelected();
}

//====================================================================
BEGIN_EVENT_TABLE(browsers::ErrorBrowser, wxTreeCtrl)
   EVT_LEFT_DCLICK(browsers::ErrorBrowser::onLMouseDblClk)
END_EVENT_TABLE()
//====================================================================
browsers::ErrorBrowser::ErrorBrowser(wxWindow* parent, wxWindowID id, 
                              const wxPoint& pos, 
                              const wxSize& size,
										long style):
							wxTreeCtrl(parent, id, pos, size, style |wxTR_HIDE_ROOT| wxTR_FULL_ROW_HIGHLIGHT )
{
}

/*void	browsers::ErrorBrowser::saveInfo(const Calbr::drcPolygon &poly)
{
	_poly = poly;
	_polyError = true;
}

void	browsers::ErrorBrowser::saveInfo(const Calbr::drcEdge &edge)
{
	_edge = edge;
	_edgeError = true;
}*/

void	browsers::ErrorBrowser::onLMouseDblClk(wxMouseEvent& event)
{
	int flags;
   wxPoint pt = event.GetPosition();
   wxTreeItemId id = HitTest(pt, flags);
   if (id.IsOk() &&(flags & wxTREE_HITTEST_ONITEMLABEL))
   {
		if (ItemHasChildren(id))
		{
			if(IsExpanded(id)) Expand(id); else Collapse(id);
		}
		else
		{
			//laydata::tdtdesign* _ATDB = DATC->lockDB();
				wxString numstr = GetItemText(id);
				long number;
				numstr.ToLong(&number);

				wxTreeItemId parent = GetItemParent(id);
				std::string error(GetItemText(parent).mb_str(wxConvUTF8));
			
				DRCData->ShowError(error, number);
				word drcLayer = DATC->getLayerNo("drcResults");
				assert(drcLayer);
//				DBbox* box;

			/*	if(_polyError)
				{
					_poly.showError(_ATDB, drcLayer);
					//define boundary of polygon
					int4b maxx, maxy, minx, miny;
					maxx = _poly.coords()->begin()->x();
					minx = _poly.coords()->begin()->x();
					maxy = _poly.coords()->begin()->y();
					miny = _poly.coords()->begin()->y();
					for(pointlist::const_iterator it = _poly.coords()->begin(); it!= _poly.coords()->end(); ++it)
					{
						
						maxx = std::max((*it).x(), maxx);
						maxy = std::max((*it).y(), maxy);
						minx = std::min((*it).x(), minx);
						miny = std::min((*it).y(), miny);
					}
					box = DEBUG_NEW DBbox(TP(minx, miny), TP(maxx, maxy));
				}
			
				if(_edgeError)
				{
					_edge.showError(_ATDB, drcLayer);
					//define boundary of edge

					box = DEBUG_NEW DBbox(TP(_edge.coords()->x1, _edge.coords()->y1), 
						TP(_edge.coords()->x2, _edge.coords()->y2));
					
				}
				DATC->unlockDB();

				wxCommandEvent eventZOOM(wxEVT_CANVAS_ZOOM);
				eventZOOM.SetInt(tui::ZOOM_WINDOW);
				eventZOOM.SetClientData(static_cast<void*>(box));
				
				wxPostEvent(Toped->view(), eventZOOM);

				delete box;*/
				/*telldata::ttpnt *p1;// = static_cast<telldata::ttpnt*>(OPstack.top());OPstack.pop();
				telldata::ttpnt *p2;// = static_cast<telldata::ttpnt*>(OPstack.top());OPstack.pop();
				real DBscale = DATC->DBscale();
				DBbox* box = DEBUG_NEW DBbox(TP(p1->x(), p1->y(), DBscale), 
                          TP(p2->x(), p2->y(), DBscale));
				wxCommandEvent eventZOOM(wxEVT_CANVAS_ZOOM);
				eventZOOM.SetInt(tui::ZOOM_WINDOW);
				eventZOOM.SetClientData(static_cast<void*>(box));
				
				wxPostEvent(Toped->view(), eventZOOM);*/
			
		}
   }
   else
      event.Skip();
}

//====================================================================
BEGIN_EVENT_TABLE(browsers::DRCBrowser, wxPanel)
   EVT_BUTTON(BT_DRC_SHOW_ALL, browsers::DRCBrowser::onShowAll)
   EVT_BUTTON(BT_DRC_HIDE_ALL, browsers::DRCBrowser::onHideAll)
END_EVENT_TABLE()
//====================================================================
//====================================================================
browsers::DRCBrowser::DRCBrowser(wxWindow* parent, wxWindowID id)
	:wxPanel(parent, id, wxDefaultPosition, wxDefaultSize)
{
	wxBoxSizer *thesizer = DEBUG_NEW wxBoxSizer( wxVERTICAL );
   wxBoxSizer *sizer1   = DEBUG_NEW wxBoxSizer( wxHORIZONTAL );
   _showAllButton = DEBUG_NEW wxButton( this, BT_DRC_SHOW_ALL, wxT("Show All") );
   _hideAllButton = DEBUG_NEW wxButton( this, BT_DRC_HIDE_ALL, wxT("Hide All") );
   //Set bold font for _hierButton

   sizer1->Add(_showAllButton, 1, wxEXPAND|wxBOTTOM, 3);
   sizer1->Add(_hideAllButton, 1, wxEXPAND|wxBOTTOM, 3);
	_errorBrowser = DEBUG_NEW ErrorBrowser(this);  
   thesizer->Add(sizer1, 0, wxEXPAND | wxALL);
   thesizer->Add(_errorBrowser, 1, wxEXPAND | wxBOTTOM);


		SetSizerAndFit(thesizer);
		Calbr::RuleChecksVector* errors = DRCData->Results();
		_errorBrowser->AddRoot(wxT("hidden_wxroot"));
		for(Calbr::RuleChecksVector::const_iterator it = errors->begin();it < errors->end(); ++it)
		{
			std::string name = (*it)->ruleCheckName();
			wxTreeItemId  id = _errorBrowser->AppendItem(_errorBrowser->GetRootItem(), wxString(name.c_str(), wxConvUTF8));
			std::vector <Calbr::drcPolygon>::iterator it2;
			std::vector <Calbr::drcPolygon> *polys = (*it)->polygons();

			//Save polygons
			long sz = polys->size();
			for(long i = 1; i <= sz; i++)
			{
				wxString str;
				str.Printf(wxT("%d"), i);
				_errorBrowser->AppendItem(id, str);
//				_errorBrowser->saveInfo(polys->at(i-1));

			}

			//Save Edges
			std::vector <Calbr::drcEdge> *edges = (*it)->edges();
			sz = edges->size();
			for(long i = 1; i <= sz; i++)
			{
				wxString str;
				str.Printf(wxT("%d"), i);
				_errorBrowser->AppendItem(id, str);
//				_errorBrowser->saveInfo(edges->at(i-1));

			}

		}
		
}

browsers::DRCBrowser::~DRCBrowser()
{
}

void browsers::DRCBrowser::deleteAllItems(void)
{
	_errorBrowser->DeleteAllItems();
}

void	browsers::DRCBrowser::onShowAll(wxCommandEvent& evt)
{
	DRCData->ShowResults();
	//Refresh
	wxCommandEvent eventZOOM(wxEVT_CANVAS_ZOOM);
   eventZOOM.SetInt(tui::ZOOM_REFRESH);
   wxPostEvent( Toped->view(), eventZOOM);
}

void	browsers::DRCBrowser::onHideAll(wxCommandEvent& evt)
{
	laydata::tdtdesign* design = DATC->lockDB();
	WordList lockedLayers = DATC->getLockedLayers();
	//Lock all layers
	WordList allLayers = DATC->getAllLayers();
	for(WordList::const_iterator it = allLayers.begin(); it!= allLayers.end(); ++it)
	{
		DATC->lockLayer((*it), true);
	}

	//unlock only drcResults layer
	word drcLayerNo = DATC->getLayerNo("drcResults");
	DATC->lockLayer(drcLayerNo, false);
   design->select_all();

	//delete selected
	laydata::atticList* sh_delist = DEBUG_NEW laydata::atticList();

	design->delete_selected(sh_delist, DATC->TEDLIB());
	tellstdfunc::clean_atticlist(sh_delist); delete sh_delist;


	//UnLock all layers
	for(WordList::const_iterator it = allLayers.begin(); it!= allLayers.end(); ++it)
	{
		DATC->lockLayer((*it), false);
	}

	//Lock only stored layers

	for(WordList::const_iterator it = lockedLayers.begin(); it!= lockedLayers.end(); ++it)
	{
		DATC->lockLayer((*it), true);
	}

	//Refresh
	wxCommandEvent eventZOOM(wxEVT_CANVAS_ZOOM);
   eventZOOM.SetInt(tui::ZOOM_REFRESH);
   wxPostEvent( Toped->view(), eventZOOM);

}