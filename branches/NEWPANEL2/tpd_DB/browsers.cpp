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

#include "browsers.h"
#include "viewprop.h"
#include "../tpd_common/tuidefs.h"
#include "../tpd_common/outbox.h"
#include "../tpd_DB/viewprop.h"
#include "../tpd_parser/ted_prompt.h"
#include "datacenter.h"
#include "../ui/activelay.xpm"
#include "../ui/lock.xpm"
#include "../ui/cell_normal.xpm"
#include "../ui/cell_expanded.xpm"
#include "../ui/nolay.xpm"


extern console::ted_cmd*         Console;
extern DataCenter*               DATC;
//extern layprop::ViewProperties*  Properties;
extern const wxEventType         wxEVT_CMD_BROWSER;
extern const wxEventType         wxEVT_CONSOLE_PARSE;

browsers::browserTAB*     Browsers = NULL;



BEGIN_EVENT_TABLE(browsers::GDSCellBrowser, CellBrowser)
   EVT_RIGHT_UP(browsers::GDSCellBrowser::OnBlankRMouseUp)
   EVT_MENU(GDSTREEREPORTLAY, browsers::GDSCellBrowser::OnGDSreportlay)
   EVT_LEFT_DCLICK(browsers::GDSCellBrowser::OnLMouseDblClk)
END_EVENT_TABLE()

browsers::GDSCellBrowser::GDSCellBrowser(wxWindow *parent, wxWindowID id, 
   const wxPoint& pos, const wxSize& size, long style) : 
                                       CellBrowser(parent, id, pos, size, style )
{

}

void browsers::GDSCellBrowser::OnItemRightClick(wxTreeEvent& event)
{
   ShowMenu(event.GetItem(), event.GetPoint());
}

void browsers::GDSCellBrowser::OnBlankRMouseUp(wxMouseEvent& event)
{
   wxPoint pt = event.GetPosition();
   ShowMenu(HitTest(pt), pt);
}

void browsers::GDSCellBrowser::OnLMouseDblClk(wxMouseEvent& event)
{
   //Empty
   //Use for overwriting CellBrowser::OnLMouseDblClk
}

void browsers::GDSCellBrowser::OnGDSreportlay(wxCommandEvent& WXUNUSED(event)) 
{
   wxString cmd;
   cmd << wxT("report_gdslayers(\"") << GetItemText(RBcellID) <<wxT("\");");
   parseCommand(cmd);
}

void browsers::GDSCellBrowser::ShowMenu(wxTreeItemId id, const wxPoint& pt)
{
   wxMenu menu;
   RBcellID = id;
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


//==============================================================================
BEGIN_EVENT_TABLE(browsers::GDSbrowser, wxPanel)
   EVT_BUTTON(BT_CELLS_HIER2, browsers::GDSbrowser::OnHierView)
   EVT_BUTTON(BT_CELLS_FLAT2, browsers::GDSbrowser::OnFlatView)
END_EVENT_TABLE()
//==============================================================================
browsers::GDSbrowser::GDSbrowser(wxWindow *parent, wxWindowID id, 
                        const wxPoint& pos , 
                        const wxSize& size ,
                        long style ):wxPanel(parent, id, pos, size, style)
{
   wxBoxSizer *thesizer = new wxBoxSizer( wxVERTICAL );
      
   wxBoxSizer *sizer1 = new wxBoxSizer( wxHORIZONTAL );
   _hierButton = new wxButton( this, BT_CELLS_HIER2, wxT("Hier") );
   //Set bold font for _hierButton
   wxFont font = _hierButton->GetFont();
   font.SetWeight(wxFONTWEIGHT_BOLD);
   _hierButton->SetFont(font);

   _flatButton = new wxButton( this, BT_CELLS_FLAT2, wxT("Flat") );

   sizer1->Add(_hierButton, 1, wxEXPAND|wxBOTTOM, 3);
   sizer1->Add(_flatButton, 1, wxEXPAND|wxBOTTOM, 3);
   
   fCellBrowser = new GDSCellBrowser(this, tui::ID_TPD_CELLTREE_F2,pos, size, style);
   
   hCellBrowser = new GDSCellBrowser(this, tui::ID_TPD_CELLTREE_H2, pos, size, style);
   
   thesizer->Add(hCellBrowser, 1, wxEXPAND | wxBOTTOM);
   thesizer->Add(fCellBrowser, 1, wxEXPAND | wxBOTTOM);
   fCellBrowser->Hide();
   thesizer->Add(sizer1, 0, wxEXPAND | wxALL);

   SetSizerAndFit(thesizer);
   thesizer->SetSizeHints( this );
}


void browsers::GDSbrowser::collectInfo() 
{
   GDSin::GDSFile* AGDSDB = DATC->lockGDS(false);
   if (NULL == AGDSDB) return;
   hCellBrowser->AddRoot(wxString((AGDSDB->Get_libname()).c_str(), wxConvUTF8));
   fCellBrowser->AddRoot(wxString((AGDSDB->Get_libname()).c_str(), wxConvUTF8));
  
   if (NULL == AGDSDB->hiertree()) return; // new, empty design
   GDSin::GDSHierTree* root = AGDSDB->hiertree()->GetFirstRoot();
   wxTreeItemId nroot;
   while (root){
      nroot = fCellBrowser->AppendItem(fCellBrowser->GetRootItem(), wxString(root->GetItem()->Get_StrName(),wxConvUTF8));
    
      nroot = hCellBrowser->AppendItem(hCellBrowser->GetRootItem(), wxString(root->GetItem()->Get_StrName(),wxConvUTF8));
//      SetItemTextColour(nroot,*wxLIGHT_GREY);
      collectChildren(root, nroot);
      root = root->GetNextRoot();
   }
   DATC->unlockGDS();
   hCellBrowser->SortChildren(hCellBrowser->GetRootItem());
   fCellBrowser->SortChildren(fCellBrowser->GetRootItem());
//   Toped->Resize();
}
      
void browsers::GDSbrowser::DeleteAllItems(void)
{
   hCellBrowser->DeleteAllItems();
   fCellBrowser->DeleteAllItems();
}

void browsers::GDSbrowser::collectChildren(GDSin::GDSHierTree *root, wxTreeItemId& lroot) 
{
   GDSin::GDSHierTree* Child= root->GetChild();
   wxTreeItemId nroot;
   wxTreeItemId temp;

   while (Child) {
      if (!fCellBrowser->findItem(wxString(Child->GetItem()->Get_StrName(), wxConvUTF8), temp, fCellBrowser-> GetRootItem()))
      {
         nroot = fCellBrowser->AppendItem(fCellBrowser->GetRootItem(), wxString(Child->GetItem()->Get_StrName(), wxConvUTF8));
      }
      nroot = hCellBrowser->AppendItem(lroot, wxString(Child->GetItem()->Get_StrName(), wxConvUTF8));
//      SetItemTextColour(nroot,*wxLIGHT_GREY);
      hCellBrowser->SortChildren(lroot);
      collectChildren(Child, nroot);
      Child = Child->GetBrother();
   }
}

void browsers::GDSbrowser::OnFlatView(wxCommandEvent& event)
{
   hCellBrowser->Hide();
   fCellBrowser->Show();
   (this->GetSizer())->Layout();
   if (hCellBrowser->IsExpanded(hCellBrowser->GetRootItem()))
   {
      fCellBrowser->Expand(fCellBrowser->GetRootItem());
   }
   //Set normal font for  _hierButton 
   //Set bold font for _flatButton;
   wxFont font = _flatButton->GetFont();
   _hierButton->SetFont(font);
   font.SetWeight(wxFONTWEIGHT_BOLD);
   _flatButton->SetFont(font);
}

void browsers::GDSbrowser::OnHierView(wxCommandEvent& event)
{
   fCellBrowser->Hide();

   hCellBrowser->Show();
   (this->GetSizer())->Layout();
   //Set normal  font for _flatButton;
   wxFont font = _hierButton->GetFont();
   _flatButton->SetFont(font);
   font.SetWeight(wxFONTWEIGHT_BOLD);
   _hierButton->SetFont(font);
}



//==============================================================================
BEGIN_EVENT_TABLE(browsers::CellBrowser, wxTreeCtrl)
   EVT_TREE_ITEM_RIGHT_CLICK( tui::ID_TPD_CELLTREE_H, browsers::CellBrowser::OnItemRightClick)
   EVT_TREE_ITEM_RIGHT_CLICK( tui::ID_TPD_CELLTREE_F, browsers::CellBrowser::OnItemRightClick)
   EVT_RIGHT_UP(browsers::CellBrowser::OnBlankRMouseUp)
   EVT_LEFT_DCLICK(browsers::CellBrowser::OnLMouseDblClk)
   EVT_MENU(CELLTREEOPENCELL, browsers::CellBrowser::OnWXOpenCell)
END_EVENT_TABLE()

browsers::CellBrowser::CellBrowser(wxWindow *parent, wxWindowID id, 
   const wxPoint& pos, const wxSize& size, long style) : 
   wxTreeCtrl(parent, id, pos, size, style | wxTR_FULL_ROW_HIGHLIGHT)
{

}

void browsers::CellBrowser::ShowMenu(wxTreeItemId id, const wxPoint& pt) 
{
    wxMenu menu;
    RBcellID = id;
    if ( id.IsOk() && (id != GetRootItem()))   
    {
      wxString RBcellname = GetItemText(id);
      menu.Append(CELLTREEOPENCELL, wxT("Open " + RBcellname));
      menu.Append(tui::TMCELL_REF_B , wxT("Add reference to " + RBcellname));
      menu.Append(tui::TMCELL_AREF_B, wxT("Add array of " + RBcellname));
      wxString ost;
      ost << wxT("export ") << RBcellname << wxT(" to GDS");
      menu.Append(tui::TMGDS_EXPORTC, ost);
      menu.Append(tui::TMCELL_REPORTLAY, wxT("Report layers used in " + RBcellname));
    }
    else 
    {
      menu.Append(tui::TMCELL_NEW, wxT("New cell")); // will be catched up in toped.cpp
      menu.Append(tui::TMGDS_EXPORTL, wxT("GDS export"));
    }
    PopupMenu(&menu, pt);
}

void browsers::CellBrowser::OnWXOpenCell(wxCommandEvent& event)
{
   _activeStructure = top_structure = RBcellID;
   wxString cmd; 
   cmd << wxT("opencell(\"") << GetItemText(RBcellID) <<wxT("\");");
   parseCommand(cmd);
}

void browsers::CellBrowser::OnItemRightClick(wxTreeEvent& event) 
{
   ShowMenu(event.GetItem(), event.GetPoint());
}

void browsers::CellBrowser::OnBlankRMouseUp(wxMouseEvent& event) 
{
   wxPoint pt = event.GetPosition();
   ShowMenu(HitTest(pt), pt);
}

void  browsers::CellBrowser::OnLMouseDblClk(wxMouseEvent& event)
{
   int flags;
   wxPoint pt = event.GetPosition();
   wxTreeItemId id = HitTest(pt, flags);
   if (id.IsOk() && (id != GetRootItem()) && (flags & wxTREE_HITTEST_ONITEMLABEL))
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
   SetItemTextColour(parent,clr);
   wxTreeItemId child = GetFirstChild(parent,cookie);
   while (child.IsOk()) 
   {
      highlightChildren(child,clr);
      child = GetNextChild(parent,cookie);
   }   
}

browsers::CellBrowser::~CellBrowser()
{
}

//==============================================================================
BEGIN_EVENT_TABLE(browsers::TDTbrowser, wxPanel)
   EVT_MENU(tui::TMCELL_REPORTLAY, browsers::TDTbrowser::OnReportUsedLayers)
   EVT_TECUSTOM_COMMAND(wxEVT_CMD_BROWSER, wxID_ANY, browsers::TDTbrowser::OnCommand)
   EVT_BUTTON(BT_CELLS_HIER, browsers::TDTbrowser::OnHierView)
   EVT_BUTTON(BT_CELLS_FLAT, browsers::TDTbrowser::OnFlatView)
END_EVENT_TABLE()
//==============================================================================
browsers::TDTbrowser::TDTbrowser(wxWindow *parent, wxWindowID id, 
   const wxPoint& pos, const wxSize& size, long style) : 
                                       wxPanel(parent, id, pos, size)
                                          //,
                                       //style | wxTR_FULL_ROW_HIGHLIGHT) 
{
   wxBoxSizer *thesizer = new wxBoxSizer( wxVERTICAL );
      
   wxBoxSizer *sizer1 = new wxBoxSizer( wxHORIZONTAL );
   
   _hierButton = new wxButton( this, BT_CELLS_HIER, wxT("Hier") );
   //Set bold font for _hierButton
   wxFont font = _hierButton->GetFont();
   font.SetWeight(wxFONTWEIGHT_BOLD);
   _hierButton->SetFont(font);

   _flatButton = new wxButton( this, BT_CELLS_FLAT, wxT("Flat") );
   sizer1->Add(_hierButton, 1, wxEXPAND|wxBOTTOM, 3);
   sizer1->Add(_flatButton, 1, wxEXPAND|wxBOTTOM, 3);
   fCellBrowser = new CellBrowser(this, tui::ID_TPD_CELLTREE_F,pos, size, style);
   
   hCellBrowser = new CellBrowser(this, tui::ID_TPD_CELLTREE_H, pos, size, style);
   
   thesizer->Add(hCellBrowser, 1, wxEXPAND | wxBOTTOM);
   thesizer->Add(fCellBrowser, 1, wxEXPAND | wxBOTTOM);
   fCellBrowser->Hide();
   thesizer->Add(sizer1, 0, wxEXPAND | wxALL);

   _imageList = new wxImageList(16, 16, TRUE);
#ifdef __WXMSW__
/*TODO : Under windows - resource loading*/
//    m_imageListNormal->Add( wxIcon(_T("icon1"), wxBITMAP_TYPE_ICO_RESOURCE) );
//
#else
    _imageList->Add( wxIcon( cell_normal   ) );
    _imageList->Add( wxIcon( cell_expanded ) );
#endif
   hCellBrowser->SetImageList(_imageList);
//   _llfont_bold.SetWeight(wxBOLD);
//   _llfont_normal.SetWeight(wxNORMAL);
   SetSizerAndFit(thesizer);
   thesizer->SetSizeHints( this );

}

void browsers::TDTbrowser::initialize() 
{
   fCellBrowser->DeleteAllItems();
   hCellBrowser->DeleteAllItems();
/*   RBcellID.Unset(); */top_structure.Unset(); active_structure.Unset();
}

void browsers::TDTbrowser::collectInfo(const wxString libname, laydata::TDTHierTree* tdtH) 
{
   hCellBrowser->AddRoot(libname);
   fCellBrowser->AddRoot(libname);
   if (!tdtH) return; // new, empty design 
   laydata::TDTHierTree* root = tdtH->GetFirstRoot();
   wxTreeItemId nroot;
   while (root){
      //Flat
      nroot = fCellBrowser->AppendItem(fCellBrowser-> GetRootItem(), 
         wxString(root->GetItem()->name().c_str(), wxConvUTF8));
      fCellBrowser->SetItemTextColour(nroot,*wxLIGHT_GREY);
      //Hier
      nroot = hCellBrowser->AppendItem(hCellBrowser-> GetRootItem(), 
         wxString(root->GetItem()->name().c_str(), wxConvUTF8));
      hCellBrowser->SetItemTextColour(nroot,*wxLIGHT_GREY);

      collectChildren(root, nroot);
      root = root->GetNextRoot();
   }
   hCellBrowser->SortChildren(hCellBrowser->GetRootItem());
   fCellBrowser->SortChildren(fCellBrowser->GetRootItem());
}
      
void browsers::TDTbrowser::collectChildren(laydata::TDTHierTree *root, wxTreeItemId& lroot) 
{
   laydata::TDTHierTree* Child= root->GetChild();
   wxTreeItemId nroot;
   wxTreeItemId temp;
   while (Child) 
   {
      //Flat
      if (!fCellBrowser->findItem(wxString(Child->GetItem()->name().c_str(), wxConvUTF8), temp, fCellBrowser-> GetRootItem()))
      {
         fCellBrowser->SetItemImage(lroot,0,wxTreeItemIcon_Normal);
         fCellBrowser->SetItemImage(lroot,1,wxTreeItemIcon_Expanded);
         nroot = fCellBrowser->AppendItem(fCellBrowser-> GetRootItem(), 
            wxString(Child->GetItem()->name().c_str(), wxConvUTF8));
         fCellBrowser->SetItemTextColour(nroot,*wxLIGHT_GREY);
      }
      //Hier
      hCellBrowser->SetItemImage(lroot,0,wxTreeItemIcon_Normal);
      hCellBrowser->SetItemImage(lroot,1,wxTreeItemIcon_Expanded);
      nroot = hCellBrowser->AppendItem(lroot, wxString(Child->GetItem()->name().c_str(), wxConvUTF8));
      hCellBrowser->SetItemTextColour(nroot,*wxLIGHT_GREY);
      hCellBrowser->SortChildren(lroot);
      collectChildren(Child, nroot);
      Child = Child->GetBrother();
   }
}

void browsers::TDTbrowser::OnCommand(wxCommandEvent& event) 
{
   switch (event.GetInt()) 
   {
      case BT_CELL_OPEN:OnTELLopencell(event.GetString());break;
      case BT_CELL_HIGHLIGHT:OnTELLhighlightcell(event.GetString());break;
      case BT_CELL_ADD :OnTELLaddcell(event.GetString(), 
          *(static_cast<wxString*>(event.GetClientData())), static_cast<int>(event.GetExtraLong()));
          delete (static_cast<wxString*>(event.GetClientData())); break;
      case BT_CELL_REMOVE:OnTELLremovecell(event.GetString(), 
          *(static_cast<wxString*>(event.GetClientData())), static_cast<bool>(event.GetExtraLong()));
          delete (static_cast<wxString*>(event.GetClientData())); break;

   }   
}


void browsers::TDTbrowser::OnFlatView(wxCommandEvent& event)
{
   hCellBrowser->Hide();
   fCellBrowser->Show();
   (this->GetSizer())->Layout();
   if (hCellBrowser->IsExpanded(hCellBrowser->GetRootItem()))
   {
      fCellBrowser->Expand(fCellBrowser->GetRootItem());
   }
   
   //Set normal font for  _hierButton 
   //Set bold font for _flatButton;
   wxFont font = _flatButton->GetFont();
   _hierButton->SetFont(font);
   font.SetWeight(wxFONTWEIGHT_BOLD);
   _flatButton->SetFont(font);
}

void browsers::TDTbrowser::OnHierView(wxCommandEvent& event)
{
   fCellBrowser->Hide();

   hCellBrowser->Show();
   (this->GetSizer())->Layout();
   /*if (fCellBrowser->IsExpanded(fCellBrowser->GetRootItem()))
   {
      hCellBrowser->Expand(hCellBrowser->activeStructure());
   }*/

   //Set bold  font for  _hierButton 
   //Set normal  font for _flatButton;
   wxFont font = _hierButton->GetFont();
   _flatButton->SetFont(font);
   font.SetWeight(wxFONTWEIGHT_BOLD);
   _hierButton->SetFont(font);
}

/*void browsers::TDTbrowser::OnWXOpenCell(wxCommandEvent& WXUNUSED(event)) {
   active_structure = top_structure = RBcellID;
   wxString ost; 
   ost << wxT("opencell(\"") << cellBrowser->GetItemText(RBcellID) <<wxT("\");");
   Console->parseCommand(ost);
}*/

void browsers::TDTbrowser::OnTELLopencell(wxString open_cell) 
{
   wxTreeItemId item1, item2;
   //Flat
   assert(fCellBrowser->findItem(open_cell, item1, fCellBrowser->GetRootItem()));
   fCellBrowser->highlightChildren(fCellBrowser->GetRootItem(), *wxLIGHT_GREY);
   top_structure = active_structure = item1;
   //fCellBrowser->highlightChildren(top_structure, *wxBLACK);
   fCellBrowser->SetItemTextColour(item1,*wxBLUE);
   
   //Hier
   assert(hCellBrowser->findItem(open_cell, item2, hCellBrowser->GetRootItem()));
   hCellBrowser->highlightChildren(hCellBrowser->GetRootItem(), *wxLIGHT_GREY);
//   if (top_structure.IsOk())
//      SetItemFont(active_structure,_llfont_normal);
   top_structure = active_structure = item2;
   hCellBrowser->highlightChildren(top_structure, *wxBLACK);
   hCellBrowser->SetItemTextColour(active_structure,*wxBLUE);
//   SetItemFont(active_structure,_llfont_bold);
}

void browsers::TDTbrowser::OnTELLhighlightcell(wxString open_cell) 
{
   //Only for hierarchy mode
   wxTreeItemId item;
   assert(hCellBrowser->findItem(open_cell, item, hCellBrowser->GetRootItem()));
   hCellBrowser->SetItemTextColour(active_structure,*wxBLACK);
//   SetItemFont(active_structure,_llfont_normal);
   active_structure = item;
   hCellBrowser->SetItemTextColour(active_structure,*wxBLUE);
//   SetItemFont(active_structure,_llfont_bold);
   hCellBrowser->EnsureVisible(active_structure);
}

void browsers::TDTbrowser::OnTELLaddcell(wxString cellname, wxString parentname, int action) 
{
   wxTreeItemId item, newparent;

   switch (action) 
   {
      case 0: 
      {//new cell
         //Flat
         wxTreeItemId item = fCellBrowser->AppendItem(fCellBrowser->GetRootItem(), 
            cellname);
         fCellBrowser->SetItemTextColour(item,
            fCellBrowser->GetItemTextColour(fCellBrowser->GetRootItem()));
         fCellBrowser->SortChildren(fCellBrowser->GetRootItem());
         //Hier
         item = hCellBrowser->AppendItem(hCellBrowser->GetRootItem(), 
            cellname);
         hCellBrowser->SetItemTextColour(item,
            hCellBrowser->GetItemTextColour(hCellBrowser->GetRootItem()));
         hCellBrowser->SortChildren(hCellBrowser->GetRootItem());
         break;
      }   
      case 1:
      {//first reference of existing cell
         assert(hCellBrowser->findItem(cellname, item, hCellBrowser->GetRootItem()));
         while (hCellBrowser->findItem(parentname, newparent, hCellBrowser->GetRootItem())) 
         {
            hCellBrowser->copyItem(item,newparent);
            hCellBrowser->SortChildren(newparent);
         }
         hCellBrowser->DeleteChildren(item);
         hCellBrowser->Delete(item);
         break;
      }   
      case 2: 
      {//
         assert(hCellBrowser->findItem(cellname, item, hCellBrowser->GetRootItem()));
         while (hCellBrowser->findItem(parentname, newparent, hCellBrowser->GetRootItem()))
         {
            hCellBrowser->copyItem(item,newparent);
            hCellBrowser->SortChildren(newparent);
         }
         break;
      }
      default: assert(false);
   }
}

void browsers::TDTbrowser::OnTELLremovecell(wxString cellname, wxString parentname, bool orphan)
{
   wxTreeItemId newparent;
   if (orphan)
   {
      wxTreeItemId item;
      hCellBrowser->findItem(cellname, item, hCellBrowser->GetRootItem());
      hCellBrowser->copyItem(item, hCellBrowser->GetRootItem());
      item = wxTreeItemId();
      assert(hCellBrowser->findItem(parentname, newparent, hCellBrowser->GetRootItem()));
      assert(hCellBrowser->findItem(cellname, item, newparent));
      hCellBrowser->DeleteChildren(item);
      hCellBrowser->Delete(item);
   }
   else if (wxT("") == parentname)
   {// no parent => we are removing the cell, not it's reference
      wxTreeItemId item;
      
      //Flat
      assert(fCellBrowser->findItem(cellname, item, fCellBrowser->GetRootItem()));
      fCellBrowser->Delete(item);

      //Hier
      wxTreeItemId item2;
      assert(hCellBrowser->findItem(cellname, item2, hCellBrowser->GetRootItem()));
      // copy all children
      // This part is "in case". The thing is that children should have been
      // removed already, by tdtcell::removePrep
      wxTreeItemIdValue cookie;
      wxTreeItemId child = hCellBrowser->GetFirstChild(item2,cookie);
      while (child.IsOk())
      {
         hCellBrowser->copyItem(child, hCellBrowser->GetRootItem());
         child = hCellBrowser->GetNextChild(item2,cookie);
      }
      // finally delete the item and it's children
      hCellBrowser->DeleteChildren(item2);
      hCellBrowser->Delete(item2);
   }
   else 
      while (hCellBrowser->findItem(parentname, newparent, hCellBrowser->GetRootItem()))
      {
         wxTreeItemId item;
         assert(hCellBrowser->findItem(cellname, item, newparent));
         hCellBrowser->DeleteChildren(item);
         hCellBrowser->Delete(item);
      }
}

void browsers::TDTbrowser::OnReportUsedLayers(wxCommandEvent& WXUNUSED(event))
{
   wxString cmd;
   cmd << wxT("report_layers(\"") << selectedCellname() << wxT("\" , true);");
   parseCommand(cmd);
}

wxString browsers::TDTbrowser::selectedCellname() const
{
   return hCellBrowser->selectedCellname();
}

browsers::TDTbrowser::~TDTbrowser()
{
   _imageList->RemoveAll();
   delete _imageList;
   hCellBrowser->DeleteAllItems();
   fCellBrowser->DeleteAllItems();
   delete hCellBrowser;
   delete fCellBrowser;
   
}

//==============================================================================
BEGIN_EVENT_TABLE(browsers::browserTAB, wxNotebook)
   EVT_TECUSTOM_COMMAND(wxEVT_CMD_BROWSER, wxID_ANY, browsers::browserTAB::OnCommand)
END_EVENT_TABLE()
//==============================================================================
browsers::browserTAB::browserTAB(wxWindow *parent, wxWindowID id,const 
   wxPoint& pos, const wxSize& size, long style) : 
                                 wxNotebook(parent, id, pos, size, style) 
{
   _TDTstruct = new TDTbrowser(this, tui::ID_TPD_CELLTREE);
   AddPage(_TDTstruct, wxT("Cells"));
   _layers = new LayerBrowser(this,  tui::ID_TPD_LAYERS);
   AddPage(_layers, wxT("Layers"));

   _GDSstruct = NULL;
   _tellParser = NULL;
   Browsers = this;
}

browsers::browserTAB::~browserTAB() 
{
//   It appears that wx is calling automatically the destructors of the
//   child windows, so no need to call them here
//   delete _TDTstruct; _TDTstruct = NULL;
//   delete _TDTlayers; _TDTlayers = NULL;
}

wxString browsers::browserTAB::TDTSelectedGDSName() const 
{
   if (NULL != _GDSstruct)
      return _GDSstruct->selectedCellname();
   else return wxT("");
}

void browsers::browserTAB::OnCommand(wxCommandEvent& event) 
{
   int command = event.GetInt();
   switch (command) 
   {
      case BT_ADDTDT_TAB:OnTELLaddTDTtab(event.GetString(), 
                            static_cast<laydata::TDTHierTree*>(event.GetClientData()));break;
      case BT_ADDGDS_TAB:OnTELLaddGDStab();break;
      case BT_CLEARGDS_TAB:OnTELLclearGDStab(); break;
   }
}

void browsers::browserTAB::OnTELLaddTDTtab(const wxString libname, laydata::TDTHierTree* tdtH) 
{
   _TDTstruct->initialize();
   _TDTstruct->collectInfo(libname, tdtH);
}

void browsers::browserTAB::OnTELLaddGDStab() 
{
   if (!_GDSstruct) {
      _GDSstruct = new GDSbrowser(this, tui::ID_GDS_CELLTREE);
      AddPage(_GDSstruct, wxT("GDS"));
   }
   else _GDSstruct->DeleteAllItems();
   _GDSstruct->collectInfo();
}

void browsers::browserTAB::OnTELLclearGDStab() 
{
   if (_GDSstruct) {
      _GDSstruct->DeleteAllItems();
      DeletePage(2);
      _GDSstruct = NULL;
   }
}

//==============================================================================
void browsers::layer_status(BROWSER_EVT_TYPE btype, const word layno, const bool status) 
{
   int* bt1 = new int(btype);
   wxCommandEvent eventLAYER_STATUS(wxEVT_CMD_BROWSER);
   eventLAYER_STATUS.SetExtraLong(status);
   eventLAYER_STATUS.SetInt(*bt1);
   word *laynotemp = new word(layno);
   eventLAYER_STATUS.SetClientData(static_cast<void*> (laynotemp));
   wxPostEvent(Browsers->TDTlayers(), eventLAYER_STATUS);


}

void browsers::layer_add(const std::string name, const word layno) 
{
   wxCommandEvent eventLAYER_ADD(wxEVT_CMD_BROWSER);
   LayerInfo *layer = new LayerInfo(name, layno);
   int* bt = new int(BT_LAYER_ADD);
   eventLAYER_ADD.SetClientData(static_cast<void*> (layer));
   eventLAYER_ADD.SetInt(*bt);
   
   wxPostEvent(Browsers->layers(), eventLAYER_ADD);

}

void browsers::layer_default(const word newlay, const word oldlay) 
{
   wxCommandEvent eventLAYER_DEF(wxEVT_CMD_BROWSER);
   int*bt = new int(BT_LAYER_DEFAULT);
   eventLAYER_DEF.SetExtraLong(newlay);
   word *laynotemp = new word(oldlay);
   eventLAYER_DEF.SetClientData(static_cast<void*> (laynotemp));
   eventLAYER_DEF.SetInt(*bt);
   
   wxPostEvent(Browsers->layers(), eventLAYER_DEF);
}

void browsers::addTDTtab(std::string libname, laydata::TDTHierTree* tdtH)
{
   wxCommandEvent eventADDTAB(wxEVT_CMD_BROWSER);
   eventADDTAB.SetInt(BT_ADDTDT_TAB);
   eventADDTAB.SetClientData(static_cast<void*> ( tdtH));
   eventADDTAB.SetString(wxString(libname.c_str(), wxConvUTF8));
   wxPostEvent(Browsers, eventADDTAB);
}

void browsers::addGDStab() 
{
   wxCommandEvent eventADDTAB(wxEVT_CMD_BROWSER);
   eventADDTAB.SetInt(BT_ADDGDS_TAB);
   wxPostEvent(Browsers, eventADDTAB);
}

void browsers::clearGDStab() 
{
   wxCommandEvent eventADDTAB(wxEVT_CMD_BROWSER);
   eventADDTAB.SetInt(BT_CLEARGDS_TAB);
   wxPostEvent(Browsers, eventADDTAB);
}

void browsers::celltree_open(const std::string cname) 
{
   wxCommandEvent eventCELLTREE(wxEVT_CMD_BROWSER);
   eventCELLTREE.SetInt(BT_CELL_OPEN);
   eventCELLTREE.SetString(wxString(cname.c_str(), wxConvUTF8));
   wxPostEvent(Browsers->TDTstruct(), eventCELLTREE);
}

void browsers::celltree_highlight(const std::string cname) 
{
   wxCommandEvent eventCELLTREE(wxEVT_CMD_BROWSER);
   eventCELLTREE.SetInt(BT_CELL_HIGHLIGHT);
   eventCELLTREE.SetString(wxString(cname.c_str(), wxConvUTF8));
   wxPostEvent(Browsers->TDTstruct(), eventCELLTREE);
}

void browsers::treeAddMember(const char* cell, const char* parent, int action) 
{
   wxCommandEvent eventCELLTREE(wxEVT_CMD_BROWSER);
   eventCELLTREE.SetInt(BT_CELL_ADD);
   eventCELLTREE.SetString(wxString(cell, wxConvUTF8));
   eventCELLTREE.SetExtraLong(action);
   wxString* prnt = new wxString(parent, wxConvUTF8);
   eventCELLTREE.SetClientData(static_cast<void*> (prnt));
   wxPostEvent(Browsers->TDTstruct(), eventCELLTREE);
}

void browsers::treeRemoveMember(const char* cell, const char* parent, bool orphan) 
{
   wxCommandEvent eventCELLTREE(wxEVT_CMD_BROWSER);
   eventCELLTREE.SetInt(BT_CELL_REMOVE);
   eventCELLTREE.SetString(wxString(cell, wxConvUTF8));
   eventCELLTREE.SetExtraLong(orphan);
   wxString* prnt = new wxString(parent, wxConvUTF8);
   eventCELLTREE.SetClientData(static_cast<void*> (prnt));
   wxPostEvent(Browsers->TDTstruct(), eventCELLTREE);
}

void browsers::parseCommand(const wxString cmd)
{
   assert(Browsers->tellParser());
   wxCommandEvent eventPARSE(wxEVT_CONSOLE_PARSE);
   eventPARSE.SetString(cmd);
   wxPostEvent(Browsers->tellParser(), eventPARSE);
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
   EVT_LEFT_DOWN(LayerButton::OnLeftClick)
   EVT_MIDDLE_DOWN(LayerButton::OnMiddleClick)
   EVT_PAINT(LayerButton::OnPaint)
END_EVENT_TABLE()

browsers::LayerButton::LayerButton(wxWindow* parent, wxWindowID id,  const wxPoint& pos , 
                                   const wxSize& size, long style , const wxValidator& validator ,
                                   const wxString& name, LayerInfo *layer)
{
   
   _layer   = layer;
   _selected= false;
   _hidden  = false;
   //_locked  = false;  

   std::string caption;
   
   _picture = new wxBitmap(size.GetWidth(), size.GetHeight(), -1);

   const byte *ifill= DATC->getFill(layer->layno());
   const layprop::tellRGB col = DATC->getColor(layer->layno());
   wxColour color(col.red(), col.green(), col.blue());

   if(ifill!=NULL)
   {     
      wxBitmap *stipplebrush = new wxBitmap((char  *)ifill, 32, 32, 1);
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
      stipplebrush = new wxBitmap(image, 1);
#endif
      _brush = new wxBrush(   *stipplebrush);
   }
   else
   {
     //???Warning!!!
      //if (NULL != col)
     //    _brush = new wxBrush(color, wxTRANSPARENT);
     // else
         _brush = new wxBrush(*wxLIGHT_GREY, wxTRANSPARENT);
   }

   _pen = new wxPen();    
            
   //if (col!=NULL)
   //{
      _pen->SetColour(color);
      _brush->SetColour(color);
   //}
            
   //***Draw main picture***   
   preparePicture(*_picture);
   
   Create(parent, id,  pos, size, style, name);
}

void browsers::LayerButton::preparePicture(wxBitmap &pict)
{
   const int clearence = 2;
   wxMemoryDC DC;
   wxFont font(10,wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
   DC.SetFont(font);
   DC.SelectObject(pict);

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
   DC.DrawText(layno, 0, int((buttonHeight - hno)/2));
   curw += wno + clearence;
   DC.DrawRectangle(curw, 1, 32, buttonHeight-1);
   curw += 32 + clearence;
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
   
   wxString caption(_layer->name().c_str(),wxConvUTF8);
   int hna,wna;
   DC.GetTextExtent(caption, &wna, &hna);
   DC.DrawRectangle(curw, clearence, buttonWidth - curw - clearence - 16, buttonHeight - 2*clearence);
   curw += clearence;
   DC.DrawText(caption, curw, int(buttonHeight/2 - hna/2));

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


void browsers::LayerButton::OnPaint(wxPaintEvent&event)
{
   wxPaintDC dc(this);
   dc.DrawBitmap(*_picture, 0, 0, false);
   if (_selected)
   {
      dc.DrawIcon(wxIcon(activelay),buttonWidth-16,15);
   }
   else if (DATC->layerLocked(_layer->layno()))
   {
      dc.DrawIcon(wxIcon(lock),buttonWidth-16,15);
   }

   if (DATC->layerHidden(_layer->layno()))
   {
      dc.DrawIcon(wxIcon(nolay_xpm),buttonWidth-16,0);
   }
}

void browsers::LayerButton::OnLeftClick(wxMouseEvent &event)
{

   if (event.ShiftDown())
   //Lock layer
   {
      //_hidden = !_hidden;
      hideLayer(!_hidden);
      wxString cmd;
      cmd << wxT("hidelayer(") <<_layer->layno() << wxT(", ");
      if (_hidden) cmd << wxT("true") << wxT(");");
      else cmd << wxT("false") << wxT(");");
      Console->parseCommand(cmd);
   }
   else
   //Select layer
   {
      wxString cmd;
      cmd << wxT("usinglayer(") << _layer->layno()<< wxT(");");
      Console->parseCommand(cmd);

      if (!_selected)
      {
         select();
   
         //Next block uses for unselect previous button
         int bt = BT_LAYER_SELECT;
         wxCommandEvent eventLAYER_SELECT(wxEVT_CMD_BROWSER);
   
         eventLAYER_SELECT.SetExtraLong(_layer->layno());
   
         eventLAYER_SELECT.SetInt(bt);
         wxPostEvent(Browsers->layers(), eventLAYER_SELECT);
      }
   }
}

void browsers::LayerButton::OnMiddleClick(wxMouseEvent &event)
{
   //_locked = !_locked;
   wxString cmd;
   cmd << wxT("locklayer(") <<_layer->layno() << wxT(", ");
   if (DATC->layerLocked(_layer->layno())) cmd << wxT("false") << wxT(");");
   else cmd << wxT("true") << wxT(");");
   Console->parseCommand(cmd);

}

void browsers::LayerButton::hideLayer(bool hide)
{
   _hidden = hide;
   preparePicture(*_picture);
//   SetBitmapLabel(*_picture);
}

void browsers::LayerButton::lockLayer(bool lock)
{
   //_locked = lock;
   preparePicture(*_picture);
 //  SetBitmapLabel(*_picture);
}

void browsers::LayerButton::select(void)
{
   _selected = true;
   preparePicture(*_picture);
 //  SetBitmapLabel(*_picture);
}

void browsers::LayerButton::unselect(void)
{
   _selected = false;
   preparePicture(*_picture);
  // SetBitmapLabel(*_picture);
}




//====================================================================
BEGIN_EVENT_TABLE(browsers::LayerBrowser, wxPanel)
   EVT_BUTTON(BT_LAYER_SHOW_ALL, browsers::LayerBrowser::OnShowAll)
   EVT_BUTTON(BT_LAYER_HIDE_ALL, browsers::LayerBrowser::OnHideAll)
   EVT_BUTTON(BT_LAYER_LOCK_ALL, browsers::LayerBrowser::OnLockAll)
   EVT_BUTTON(BT_LAYER_UNLOCK_ALL, browsers::LayerBrowser::OnUnlockAll)
   EVT_TECUSTOM_COMMAND(wxEVT_CMD_BROWSER, wxID_ANY, browsers::LayerBrowser::OnCommand)
END_EVENT_TABLE()
//====================================================================


browsers::LayerBrowser::LayerBrowser(wxWindow* parent, wxWindowID id) 
   :wxScrolledWindow(parent, id, wxDefaultPosition, wxDefaultSize),
   _selectedButton(NULL),
   _layerPanel(NULL),
   _thesizer(NULL)
{
   _buttonCount = 0;
   _thesizer = new wxBoxSizer(wxVERTICAL);

   wxBoxSizer *sizer1 = new wxBoxSizer(wxHORIZONTAL);
   sizer1->Add(new wxButton(this, BT_LAYER_SHOW_ALL, wxT("Show All")), 1, wxTOP, 3);
   sizer1->Add(new wxButton(this, BT_LAYER_HIDE_ALL, wxT("Hide All")), 1, wxTOP, 3);
   _thesizer->Add(sizer1, 0, wxTOP);

   wxBoxSizer *sizer2 = new wxBoxSizer(wxHORIZONTAL);
   sizer2->Add(new wxButton(this, BT_LAYER_LOCK_ALL, wxT("Lock All")), 1, wxTOP, 3);
   sizer2->Add(new wxButton(this, BT_LAYER_UNLOCK_ALL, wxT("Unlock All")), 1, wxTOP, 3);
   _thesizer->Add(sizer2, 0, wxTOP);

   _layerPanel = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
   _thesizer->Add(_layerPanel, 3, wxEXPAND);//|wxALL);

	wxString help;
	help<<_T("LBM		-select\n");
	help<<_T("shift+LBM	-hide\n");
	help<<_T("MBM		- lock\n");
	int w, h;
	_helpPanel = new wxTextCtrl(this,  wxID_ANY, help, wxDefaultPosition, wxSize(100, 60),wxTE_MULTILINE|wxTE_READONLY		);
	_thesizer->Add(_helpPanel, 0, wxALIGN_BOTTOM|wxEXPAND	);
   SetSizerAndFit(_thesizer);
}

browsers::LayerBrowser::~LayerBrowser() 
{
}

void browsers::LayerBrowser::OnCommand(wxCommandEvent& event)
{
   int command = event.GetInt();
   
   switch (command) 
   {

      case BT_LAYER_DEFAULT:
         {
            word *oldlay = static_cast<word*>(event.GetClientData());
            word layno = event.GetExtraLong();
            _buttonMap[*oldlay]->unselect();
            _buttonMap[layno]->select();
            //_layerlist->defaultLayer((word)event.GetExtraLong(), (word)event.GetInt());
            break;
         }
      case    BT_LAYER_HIDE:
         {
            word *layno = static_cast<word*>(event.GetClientData());
            bool status = event.GetExtraLong();
            //_buttonMap[layno]->hideLayer(event.IsChecked());
            _buttonMap[*layno]->hideLayer(status);
            break;
         }
         //_layerlist->hideLayer((word)event.GetExtraLong(),event.IsChecked());break;
      case    BT_LAYER_LOCK:
         {
            //_layerlist->lockLayer((word)event.GetExtraLong(),event.IsChecked());
            word *layno = static_cast<word*>(event.GetClientData());
            bool status = event.GetExtraLong();
            _buttonMap[*layno]->lockLayer(status);
            break;
         }
      case     BT_LAYER_SELECT:
         {
            word layno = event.GetExtraLong();
            _selectedButton->unselect();
            _selectedButton = _buttonMap[layno];
         
            break;
         }

      case     BT_LAYER_ADD:
         {
            LayerInfo* layer = static_cast<LayerInfo*>(event.GetClientData());

            LayerButton *layerButton;

            layerButtonMap::iterator it;
            //Remove selection from current button
            if (_selectedButton!=NULL) _selectedButton->unselect();

            it = _buttonMap.find(layer->layno());
            if (it!= _buttonMap.end())
            {
               //Button already exists, replace it
               LayerButton *tempButton = it->second;
               
               //layerButton = new LayerButton(this, tui::TMDUMMY_LAYER+_buttonCount, wxPoint (0, _buttonCount*30), wxSize(200, 30),
               //wxBU_AUTODRAW, wxDefaultValidator, _T("TTT"), layer);
               int x, y;
               int szx, szy;
               int ID;
               tempButton->GetPosition(&x, &y);
               tempButton->GetSize(&szx, &szy);
               ID = tempButton->GetId();
               layerButton = new LayerButton(_layerPanel, ID, wxPoint (x, y), wxSize(szx, szy),
               wxBU_AUTODRAW|wxNO_BORDER, wxDefaultValidator, _T("button"), layer);
               _buttonMap[layer->layno()] = layerButton;
               delete tempButton;

            }
            else
            {
               //Button doesn't exist, create new button
//               int szx, szy;
//               _layerPanel->GetSize(&szx, &szy);
               layerButton = new LayerButton(_layerPanel, tui::TMDUMMY_LAYER+_buttonCount, 
                                             wxPoint (0, _buttonCount*buttonHeight), wxSize(buttonWidth, buttonHeight),
               wxBU_AUTODRAW, wxDefaultValidator, _T("button"), layer);
               _buttonMap[layer->layno()] = layerButton;

               _buttonCount++; 
               _layerPanel->SetScrollbars(0, buttonHeight, 0, _buttonCount);
            }
				Resort();
            
            //Restore selection
            if ((it = _buttonMap.find(DATC->curlay()))!= _buttonMap.end())
            {
               _selectedButton = it->second;
               _selectedButton->select();
            }
            //_selectedButton = (_buttonMap.begin())->second;
            //_selectedButton->select();
            break;
         }
   }
}

void browsers::LayerBrowser::Resort(void)
{
	layerButtonMap::const_iterator it;
	int index = 0;
	for(it=_buttonMap.begin();it !=_buttonMap.end(); ++it, ++index)
	{
			LayerButton *tempButton = it->second;
         int szx, szy;
			GetSize(&szx, &szy);
			tempButton->SetSize(0, index*buttonHeight, szx, buttonHeight);
	}
}

void browsers::LayerBrowser::OnShowAll(wxCommandEvent& WXUNUSED(event))
{
   wxString cmd;
   cmd << wxT("hidelayer(") << getAllSelected() << wxT(", false);");
   parseCommand(cmd);
}

void browsers::LayerBrowser::OnHideAll(wxCommandEvent& WXUNUSED(event))
{
   wxString cmd;
   cmd << wxT("hidelayer(") << getAllSelected() << wxT(", true);");
   parseCommand(cmd);
}

void browsers::LayerBrowser::OnLockAll(wxCommandEvent& WXUNUSED(event))
{
   wxString cmd;
   cmd << wxT("locklayer(") << getAllSelected() << wxT(", true);");
   parseCommand(cmd);
}


void browsers::LayerBrowser::OnUnlockAll(wxCommandEvent& WXUNUSED(event))
{
   wxString cmd;
   cmd << wxT("locklayer(") << getAllSelected() << wxT(", false);");
   parseCommand(cmd);
}

wxString browsers::LayerBrowser::getAllSelected()
{
      //bool multi_selection = _layerlist->GetSelectedItemCount() > 1;
   
   wxString layers = wxT("{");
   for(layerButtonMap::iterator it = _buttonMap.begin(); it != _buttonMap.end(); it++)
   {
      word layNo = (it->second)->getLayNo();
      layers << wxT(" ") <<  layNo << wxT(",");
   }
   layers.RemoveLast();
   layers << wxT("}");
   return layers;
}

