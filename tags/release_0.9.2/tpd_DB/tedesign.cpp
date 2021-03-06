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
//        Created: Sun Apr 18 10:59:37 BST 2004
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: The top class in the layout database
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================

#include "tpdph.h"
#include <sstream>
#include "../tpd_common/ttt.h"
#include "../tpd_common/outbox.h"
#include "../tpd_ifaces/cif_io.h"
#include "../tpd_ifaces/gds_io.h"
#include "tedesign.h"
#include "tedat.h"
#include "viewprop.h"

//! the stack of all previously edited (opened) cells
laydata::editcellstack      laydata::editobject::_editstack;
layprop::ViewProperties*    laydata::editobject::_viewprop = NULL;

// initializing the static variables
laydata::TDTHierTree* laydata::tdtlibrary::_hiertree = NULL;

//-----------------------------------------------------------------------------
// class tdtlibrary
//-----------------------------------------------------------------------------
laydata::tdtlibrary::tdtlibrary(std::string name, real DBU, real UU, int libID) :
   _name(name), _libID(libID), _DBU(DBU), _UU(UU) {}

//void laydata::tdtlibrary::unloadprep(laydata::tdtlibdir* libdir)
//{
//   laydata::cellList::const_iterator wc;
//   for (wc = _cells.begin(); wc != _cells.end(); wc++)
//     if (!wc->second->orphan())
//      {
////         laydata::tdtdefaultcell* boza = new laydata::tdtdefaultcell(wc->first, 0, false);
//         _hiertree->replaceChild(wc->second, libdir->adddefaultcell(wc->first), _hiertree);
//      }
//}

void laydata::tdtlibrary::relink(tdtlibdir* libdir)
{
   laydata::cellList::iterator wc;
   bool need_validation = false;
   for (wc = _cells.begin(); wc != _cells.end(); wc++)
   {
      assert(wc->second);
      need_validation |= wc->second->relink(libdir, _hiertree);
   }
   if (need_validation)
      do {} while(validate_cells());
}

laydata::tdtlibrary::~tdtlibrary()
{
   clearLib();
}

void laydata::tdtlibrary::clearHierTree()
{
   // get rid of the hierarchy tree
   const TDTHierTree* var1 = _hiertree;
   _hiertree = NULL;
   TDTHierTree* lastValid = NULL;
   while (var1)
   {
      const TDTHierTree* var2 = var1->GetLast();
      if (var1->itemRefdIn(_libID))
      {
         if (NULL != lastValid)
            lastValid->relink(var1);
         delete var1;
      }
      else
      {
         if (NULL != lastValid)
            lastValid = const_cast<TDTHierTree*>(var1);
         else
            _hiertree = lastValid = const_cast<TDTHierTree*>(var1);
      }
      var1 = var2;
   }
}

void laydata::tdtlibrary::clearEntireHierTree()
{
   // get rid of the hierarchy tree
   const TDTHierTree* var1 = _hiertree;
   while (var1)
   {
      const TDTHierTree* var2 = var1->GetLast();
      delete var1; var1 = var2;
   }
   _hiertree = NULL;
}

void laydata::tdtlibrary::read(TEDfile* const tedfile)
{
   std::string cellname;
   while (tedf_CELL == tedfile->getByte())
   {
      cellname = tedfile->getString();
      tell_log(console::MT_CELLNAME, cellname);
      registercellread(cellname, DEBUG_NEW tdtcell(tedfile, cellname, _libID));
   }
   recreate_hierarchy(tedfile->TEDLIB());
   tell_log(console::MT_INFO, "Done");
}

void laydata::tdtlibrary::registercellread(std::string cellname, tdtcell* cell) {
   if (_cells.end() != _cells.find(cellname)) 
   // There are several possiblirities here:
   // 1. Cell has been referenced before the definition takes place
   // 2. The same case 1, but the reason is circular reference. 
   // 3. Cell is defined more than once
   // Case 3 seems to be just theoretical and we should abort the reading 
   // and retun with error in this case.
   // Case 2 is really dangerous and once again theoretically we need to 
   // break the circularity. This might happen however once the whole file
   // is parced
   // Case 1 is quite OK, although, the write sequence should use the 
   // cell structure tree and start the writing from the leaf cells. At the 
   // moment writing is in kind of alphabetical order and case 1 is more
   // than possible. In the future it might me appropriate to issue a warning
   // for possible circular reference.
      if (NULL == _cells[cellname]) {
         // case 1 or case 2 -> can't be distiguised in this moment
         //_cells[cellname] = cell;
         // cell has been referenced already, so it's not an orphan
         cell->parentfound();
      }
      else {
         //@FIXME case 3 -> parsing should be stopped !
      }
   _cells[cellname] = cell;
}

void laydata::tdtlibrary::GDSwrite(GDSin::GdsFile& gdsf, tdtcell* top, bool recur)
{
   GDSin::GdsRecord* wr = gdsf.setNextRecord(gds_LIBNAME, _name.size());
   wr->add_ascii(_name.c_str()); gdsf.flush(wr);

   wr = gdsf.setNextRecord(gds_UNITS);
   wr->add_real8b(_UU); wr->add_real8b(_DBU);
   gdsf.flush(wr);
   //
   if (NULL == top)
   {
      laydata::TDTHierTree* root = _hiertree->GetFirstRoot(TARGETDB_LIB);
      while (root) {
         _cells[root->GetItem()->name()]->GDSwrite(gdsf, _cells, root, _UU, recur);
         root = root->GetNextRoot(TARGETDB_LIB);
      }
   }
   else
   {
      laydata::TDTHierTree* root_cell = _hiertree->GetMember(top);
      top->GDSwrite(gdsf, _cells, root_cell, _UU, recur);
   }
   wr = gdsf.setNextRecord(gds_ENDLIB);gdsf.flush(wr);
}

void laydata::tdtlibrary::CIFwrite(CIFin::CifExportFile& ciff, tdtcell* top, bool recur)
{
/*   GDSin::GdsRecord* wr = gdsf.setNextRecord(gds_LIBNAME, _name.size());
   wr->add_ascii(_name.c_str()); gdsf.flush(wr);

   wr = gdsf.setNextRecord(gds_UNITS);
   wr->add_real8b(_UU); wr->add_real8b(_DBU);
   gdsf.flush(wr);
   */

   TpdTime timelu(_lastUpdated);
   ciff.file() << "(       TDT source : " << name() << ");" << std::endl;
   ciff.file() << "(    Last Modified : " << timelu() << ");" << std::endl;

   if (NULL == top)
   {
      ciff.file() << "(         Top Cell :  - );" << std::endl;
      laydata::TDTHierTree* root = _hiertree->GetFirstRoot(TARGETDB_LIB);
      while (root) {
         _cells[root->GetItem()->name()]->CIFwrite(ciff, _cells, root, _UU, recur);
         root = root->GetNextRoot(TARGETDB_LIB);
      }
   }
   else
   {
      ciff.file() << "(         Top Cell : " << top->name() << ");" << std::endl;
      laydata::TDTHierTree* root_cell = _hiertree->GetMember(top);
      top->CIFwrite(ciff, _cells, root_cell, _UU, recur);
   }
//   wr = gdsf.setNextRecord(gds_ENDLIB);gdsf.flush(wr);
}

void laydata::tdtlibrary::PSwrite(PSFile& psf, const tdtcell* top, const layprop::DrawProperties& drawprop)
{
   laydata::TDTHierTree* root_cell = _hiertree->GetMember(top);
   if (psf.hier())
   {
      top->PSwrite(psf, drawprop, &_cells, root_cell);
      psf.pspage_header(top->overlap());
      psf.pspage_footer(top->name());
   }
   else
   {
      psf.pspage_header(top->overlap());
      top->PSwrite(psf, drawprop, &_cells, root_cell);
      psf.pspage_footer(top->name());
   }
}

laydata::tdtcell* laydata::tdtlibrary::checkcell(std::string name, bool undeflib)
{
   if ((!undeflib && (UNDEFCELL_LIB == _libID)) || (_cells.end() == _cells.find(name)))
      return NULL;
   else return static_cast<laydata::tdtcell*>(_cells[name]);
}

void laydata::tdtlibrary::recreate_hierarchy(const laydata::tdtlibdir* libdir)
{
   if (TARGETDB_LIB == _libID)
   {
      clearHierTree();
   }
   // here - run the hierarchy extraction on orphans only
   for (laydata::cellList::const_iterator wc = _cells.begin();
                                          wc != _cells.end(); wc++) {
      if (wc->second && wc->second->orphan())
         _hiertree = wc->second->hierout(_hiertree, NULL, &_cells, libdir);
   }
}

laydata::refnamepair laydata::tdtlibrary::secure_defaultcell(std::string name)
{
   assert(UNDEFCELL_LIB == _libID);
   if (_cells.end() == _cells.find(name))
   {
      _cells[name] = DEBUG_NEW tdtdefaultcell(name, UNDEFCELL_LIB, true);
   }
   return _cells.find(name);
}

bool laydata::tdtlibrary::validate_cells()
{
   bool invalidParents = false;
   laydata::cellList::const_iterator wc;
   for (wc = _cells.begin(); wc != _cells.end(); wc++)
   {
      if (NULL != wc->second)
         invalidParents |= static_cast<tdtcell*>(wc->second)->validate_cells(this);
   }
   return invalidParents;
}

void laydata::tdtlibrary::clearLib()
{
   laydata::cellList::const_iterator wc;
   for (wc = _cells.begin(); wc != _cells.end(); wc++)
      delete wc->second;
   _cells.clear();
}

void laydata::tdtlibrary::cleanUnreferenced()
{
   laydata::cellList::iterator wc = _cells.begin();
   while (wc != _cells.end())
   {
      const TDTHierTree* hcell = _hiertree->GetMember(wc->second);
      if ((NULL != hcell) && (NULL == hcell->Getparent()))
      {
         _hiertree->removeRootItem(wc->second, _hiertree);
         delete wc->second;
         laydata::cellList::iterator wcd = wc++;
         _cells.erase(wcd);
      }
      else wc++;
   }
}

void laydata::tdtlibrary::collect_usedlays(WordList& laylist) const
{
   for (cellList::const_iterator CC = _cells.begin(); CC != _cells.end(); CC++)
   {
      CC->second->collect_usedlays(NULL, false,laylist);
   }
   laylist.sort();
   laylist.unique();
   if ( (0 < laylist.size()) && (0 == laylist.front()) )
      laylist.pop_front();
}

//-----------------------------------------------------------------------------
// class tdtlibdir
//-----------------------------------------------------------------------------
laydata::tdtlibdir::tdtlibdir()
{
   // create the default library of unknown cells
   tdtlibrary* undeflib = DEBUG_NEW tdtlibrary("__UNDEFINED__", 1e-9, 1e-3, UNDEFCELL_LIB);
   _libdirectory.insert( _libdirectory.end(), DEBUG_NEW LibItem("__UNDEFINED__", undeflib) );
   // toped data base
   _TEDDB = NULL;
}

laydata::tdtlibdir::~tdtlibdir()
{
   for (word i = 0; i < _libdirectory.size(); i++)
   {
      delete _libdirectory[i]->second;
      delete _libdirectory[i];
   }
   if (NULL != _TEDDB) delete _TEDDB;
}

int laydata::tdtlibdir::getLastLibRefNo()
{
   return _libdirectory.size();
}

void laydata::tdtlibdir::addlibrary(tdtlibrary* const lib, word libRef)
{
   assert(libRef == _libdirectory.size());
   _libdirectory.insert( _libdirectory.end(), DEBUG_NEW LibItem(lib->name(), lib) );
}

laydata::tdtlibrary* laydata::tdtlibdir::removelibrary(std::string libname)
{
   tdtlibrary* tberased = NULL;
   for (Catalog::iterator LDI = _libdirectory.begin(); LDI != _libdirectory.end(); LDI++)
   {
      if (libname == (*LDI)->first)
      {
         tberased = (*LDI)->second;
         _libdirectory.erase(LDI);
         break;
      }
   }
   return tberased;
}

laydata::tdtlibrary* laydata::tdtlibdir::getLib(int libID)
{
   assert(libID); // make sure that nobody asks for the default library
   assert(libID <= (int)_libdirectory.size());
   return _libdirectory[libID]->second;
}

std::string laydata::tdtlibdir::getLibName(int libID)
{
   assert(libID); // make sure that nobody asks for the default library
   assert(libID <= (int)_libdirectory.size());
   return _libdirectory[libID]->first;
}

void laydata::tdtlibdir::relink()
{
   // relink starting from the back of the library queue
   for (int i = _libdirectory.size() - 2; i > 0 ; i--)
   {
      _libdirectory[i]->second->relink(this);
   }
   // finally - relink the active database
   if (NULL !=_TEDDB) 
      _TEDDB->relink(this);
}

void laydata::tdtlibdir::reextract_hierarchy()
{
   // parse starting from the back of the library queue
   for (int i = _libdirectory.size() - 2; i > 0 ; i--)
   {
      _libdirectory[i]->second->recreate_hierarchy(this);
   }
   // finally - relink the active database
   if (NULL !=_TEDDB) 
      _TEDDB->recreate_hierarchy(this);
}

/*! Searches the library directory for a cell \a name. Returns true and updates \a striter if
the cell is found. Returns false otherwise. The method never searches in the UNDEFCELL_LIB and
TARGETDB_LIB. It starts searching from the library after \a libID .
*/
bool laydata::tdtlibdir::getLibCellRNP(std::string name, laydata::refnamepair& striter, const int libID) const
{
   // start searching form the first library after the current 
   word first2search = (TARGETDB_LIB == libID) ? 1 : libID + 1;
   for (word i = first2search; i < _libdirectory.size(); i++)
   {
      if (NULL != _libdirectory[i]->second->checkcell(name))
      {
         striter = _libdirectory[i]->second->getcellnamepair(name);
         return true;
      }
   }
   return false;
}

/*! Searches the library directory for a cell \a name. Returns the cell definition if
the cell is found. Returns NULL otherwise. The method never searches in the TARGETDB_LIB.
It starts searching from the library after \a libID . If the cell is not found in the
libraries it also checks the UNDEFCELL_LIB
 */
laydata::tdtdefaultcell* laydata::tdtlibdir::getLibCellDef(std::string name, const int libID) const
{
   // start searching form the first library after the current 
   word first2search = (TARGETDB_LIB == libID) ? 1 : libID + 1;
   for (word i = first2search; i < _libdirectory.size(); i++)
   {
      if (NULL != _libdirectory[i]->second->checkcell(name))
      {
         return _libdirectory[i]->second->getcellnamepair(name)->second;
      }
   }
   // not in the libraries - must be in the defaultlib
   if (NULL != _libdirectory[UNDEFCELL_LIB]->second->checkcell(name, true))
   {
      return _libdirectory[UNDEFCELL_LIB]->second->getcellnamepair(name)->second;
   }
   return NULL;
}

laydata::refnamepair laydata::tdtlibdir::adddefaultcell( std::string name )
{
   laydata::tdtlibrary* undeflib = _libdirectory[UNDEFCELL_LIB]->second;
   return undeflib->secure_defaultcell(name);
}

bool laydata::tdtlibdir::collect_usedlays(std::string cellname, bool recursive, WordList& laylist) const
{
   tdtcell* topcell = NULL;
   if (NULL != _TEDDB) topcell = _TEDDB->checkcell(cellname);
   if (NULL != topcell)
   {
      topcell->collect_usedlays(this, recursive, laylist);
      return true;
   }
   else
   {
      laydata::refnamepair striter;
      if (getLibCellRNP(cellname, striter))
      {
         static_cast<laydata::tdtcell*>(striter->second)->collect_usedlays(this, recursive, laylist);
         return true;
      }
      else return false;
   }
}

void laydata::tdtlibdir::collect_usedlays( int libID, WordList& laylist) const
{
   assert(UNDEFCELL_LIB != libID);
   laydata::tdtlibrary* curlib = (TARGETDB_LIB == libID) ? _TEDDB : _libdirectory[libID]->second;
   if (NULL != curlib) curlib->collect_usedlays(laylist);
}

/*! Used to link the cell references with their definitions. This function is called to relink
databases (libraries) already loaded in memory. A function with completely the same functionality
and name is defined in the TEDfile. That one is used to link cell references during tdt parsing
phase
*/
laydata::refnamepair laydata::tdtlibdir::linkcellref(std::string cellname, int libID, TDTHierTree*& Htree)
{
   assert(UNDEFCELL_LIB != libID);
   laydata::tdtlibrary* curlib = (TARGETDB_LIB == libID) ? _TEDDB : _libdirectory[libID]->second;
   laydata::refnamepair striter = curlib->_cells.find(cellname);
   // link the cells instances with their definitions
   if (curlib->_cells.end() == striter)
   {
      // search the cell in the rest of the libraries because it's not in the current
      if (!getLibCellRNP(cellname, striter, libID))
      {
         // not found! make a default cell
         striter = adddefaultcell(cellname);
         Htree = DEBUG_NEW TDTHierTree(striter->second, NULL, Htree);
      }
   }
   // Mark that the cell definition is referenced, i.e. it is not the top 
   // of the tree (orphan flag in the tdtcell)
   assert(striter->second);
   striter->second->parentfound();
   return striter;
}

void  laydata::tdtlibdir::cleanUndefLib()
{
   _libdirectory[UNDEFCELL_LIB]->second->cleanUnreferenced();
}

//-----------------------------------------------------------------------------
// class tdtdesign
//-----------------------------------------------------------------------------
laydata::tdtdesign::tdtdesign(std::string name, time_t created, time_t lastUpdated,
           real DBU, real UU)  :    laydata::tdtdesign::tdtlibrary(name, DBU, UU, TARGETDB_LIB)
{
   _tmpdata       = NULL;
   modified       = false;
   _created       = created;
   _lastUpdated   = lastUpdated;
}

void laydata::tdtdesign::read(TEDfile* const tedfile)
{
   tdtlibrary::read(tedfile);
   _tmpdata = NULL;
   modified = false;
}

// !!! Do not forget that the indexing[] operations over std::map can alter the structure !!!
// use find for searching.
laydata::tdtcell* laydata::tdtdesign::addcell(std::string name) {
   if (_cells.end() != _cells.find(name)) return NULL; // cell already exists
   else {
      modified = true;
      tdtcell* ncl = DEBUG_NEW tdtcell(name);
      _cells[name] = ncl;
      _hiertree = DEBUG_NEW TDTHierTree(ncl, NULL, _hiertree);
       btreeAddMember(_hiertree->GetItem()->name().c_str(), _name.c_str(), 0);
      return ncl;
   }
}

bool laydata::tdtdesign::removecell(std::string& name, laydata::atticList* fsel, laydata::tdtlibdir* libdir)
{
   if (_cells.end() == _cells.find(name))
   {
      std::string news = "Cell \"";
      news += name; news += "\" doesn't exists";
      tell_log(console::MT_ERROR,news);
      return false; // cell doesn't exists
   }
   else
   {
      TDTHierTree* ccl = _hiertree->GetMember(_cells[name]);
      if (ccl->Getparent())
      {
         std::string news = "Cell \"";
         news += name; news += "\" is referenced and can't be removed";
         tell_log(console::MT_ERROR,news);
         return false;
      }
      else if ((NULL != _target.edit()) && (_target.edit()->name() == name))
      {
         tell_log(console::MT_ERROR,"Active cell can't be removed");
         return false;
      }
      else
      {
         modified = true;
         // get the cell by name
         tdtcell* remcl = static_cast<laydata::tdtcell*>(_cells[name]);
         // update the _hiertree
         remcl->removePrep(this);
         // remove the cell from the list of all design cells
         _cells.erase(_cells.find(name));
         //empty the contents of the removed cell and return it in atticList
         remcl->full_select();
         remcl->delete_selected(fsel, libdir); // validation is not required here
         // finally - delete the cell. Cell is already empty
         delete remcl;
         return true;
      }
   }
}

laydata::tdtdata* laydata::tdtdesign::addbox(word la, TP* p1, TP* p2)
{
   DBbox old_overlap = _target.edit()->overlap();
   tdtlayer *actlay = static_cast<tdtlayer*>(targetlayer(la));
   modified = true;
   (*p1) *= _target.rARTM();
   (*p2) *= _target.rARTM();
   laydata::tdtdata* newshape = actlay->addbox(p1,p2);
   if (_target.edit()->overlapChanged(old_overlap, this))
      do {} while(validate_cells());
   return newshape;
}

laydata::tdtdata* laydata::tdtdesign::addpoly(word la, const pointlist* pl) {
   laydata::valid_poly check(*pl);
   if (!check.valid()) {
      std::ostringstream ost;
      ost << "Polygon check fails - " << check.failtype();
      tell_log(console::MT_ERROR, ost.str());
      return NULL;
   }
   laydata::tdtdata* newshape;
   DBbox old_overlap = _target.edit()->overlap();
   tdtlayer *actlay = static_cast<tdtlayer*>(targetlayer(la));
   modified = true;
   pointlist vpl = check.get_validated();
   if (check.box()) {
      TP* p1= DEBUG_NEW TP(vpl[0] *_target.rARTM());
      TP* p2= DEBUG_NEW TP(vpl[2] *_target.rARTM());
      newshape = actlay->addbox(p1,p2);
   }
   for(pointlist::iterator PL = vpl.begin(); PL != vpl.end(); PL++)
      (*PL) *= _target.rARTM();
   newshape = actlay->addpoly(vpl);
   if (_target.edit()->overlapChanged(old_overlap, this))
      do {} while(validate_cells());
   return newshape;
}

laydata::tdtdata* laydata::tdtdesign::addwire(word la, const pointlist* pl, word w) {
   laydata::valid_wire check(*pl,w);
   if (!check.valid()) {
      std::ostringstream ost;
      ost << "Wire check fails - " << check.failtype();
      tell_log(console::MT_ERROR, ost.str());
      return NULL;
   }
   DBbox old_overlap = _target.edit()->overlap();
   tdtlayer *actlay = static_cast<tdtlayer*>(targetlayer(la));
   modified = true;
   pointlist vpl = check.get_validated();
   for(pointlist::iterator PL = vpl.begin(); PL != vpl.end(); PL++)
      (*PL) *= _target.rARTM();
   laydata::tdtdata* newshape = actlay->addwire(vpl,w);
   if (_target.edit()->overlapChanged(old_overlap, this))
      do {} while(validate_cells());
   return newshape;
}

laydata::tdtdata* laydata::tdtdesign::addtext(word la, std::string& text, CTM& ori) {
   DBbox old_overlap = _target.edit()->overlap();
   tdtlayer *actlay = static_cast<tdtlayer*>(targetlayer(la));
   modified = true;
   ori *= _target.rARTM();
   laydata::tdtdata* newshape = actlay->addtext(text,ori);
   if (_target.edit()->overlapChanged(old_overlap, this))
      do {} while(validate_cells());
   return newshape;
}

laydata::tdtdata* laydata::tdtdesign::addcellref(laydata::refnamepair striter, CTM& ori)
{
   modified = true;
   ori *= _target.rARTM();
   DBbox old_overlap = _target.edit()->overlap();
   tdtdata* ncrf = _target.edit()->addcellref(this, striter, ori);
   if (NULL == ncrf)
   {
     tell_log(console::MT_ERROR, "Circular reference is forbidden");
   }
   else
   {
      if (_target.edit()->overlapChanged(old_overlap, this))
         do {} while(validate_cells());
   }
   return ncrf;
}

laydata::tdtdata* laydata::tdtdesign::addcellaref(std::string& name, CTM& ori,
                             ArrayProperties& arrprops) {
   if (checkcell(name)) {
      laydata::refnamepair striter = getcellnamepair(name);
      modified = true;
      ori *= _target.rARTM();
      DBbox old_overlap = _target.edit()->overlap();
      tdtdata* ncrf = _target.edit()->addcellaref(this, striter, ori, arrprops);
      if (NULL == ncrf) {
        tell_log(console::MT_ERROR, "Circular reference is forbidden");
      }
      else
      {
         if (_target.edit()->overlapChanged(old_overlap, this))
            do {} while(validate_cells());
      }
      return ncrf;
   }
   else {
      std::string news = "Cell \"";
      news += name; news += "\" is not defined";
      tell_log(console::MT_ERROR,news);
      return NULL;
   }
}

void laydata::tdtdesign::addlist(atticList* nlst)
{
   if (_target.edit()->addlist(this, nlst))
   {
      // needs validation
      do {} while(validate_cells());
   }
}

laydata::tdtcell* laydata::tdtdesign::opencell(std::string name)
{
   if (_cells.end() != _cells.find(name))
   {
      laydata::tdtdefaultcell* tcell = _cells[name];
      if (tcell && (UNDEFCELL_LIB != tcell->libID()))
      {
         _target.setcell(static_cast<tdtcell*>(_cells[name]));
         return _target.edit();
      }
   }
   return NULL; // Active cell has not changed if name is not found
}

bool laydata::tdtdesign::editpush(const TP& pnt) {
   if (_target.checkedit()) {//
      ctmstack transtack;
      transtack.push(CTM());
      laydata::cellrefstack* crstack = DEBUG_NEW laydata::cellrefstack();
      tdtcell* oldtvcell = _target.view();
      // Find the new active reference
      laydata::tdtcellref *new_activeref =
                      oldtvcell->getcellover(pnt,transtack,crstack,_target.viewprop());
      if (new_activeref) {
         // Set the new active reference and the chain to it
         _target.push(new_activeref,oldtvcell,crstack,transtack.top());
         return true;
      }
      else
         delete crstack;
   }
   return false;
}

bool laydata::tdtdesign::editprev(const bool undo) {
   return _target.previous(undo);
}

bool laydata::tdtdesign::editpop() {
   return _target.pop();
}

bool laydata::tdtdesign::edittop() {
   return _target.top();
}

void laydata::tdtdesign::openGL_draw(layprop::DrawProperties& drawprop) {
   if (_target.checkedit())
   {
      ctmstack transtack;
      drawprop.initCTMstack();
      _target.view()->openGL_draw(drawprop, _target.iscell());
      drawprop.clearCTMstack();
   }
}


void laydata::tdtdesign::write(TEDfile* const tedfile) {
   tedfile->putByte(tedf_DESIGN);
   tedfile->putString(_name);
   tedfile->putReal(_DBU);
   tedfile->putReal(_UU);
   //
   laydata::TDTHierTree* root = _hiertree->GetFirstRoot(TARGETDB_LIB);
   while (root) {
      _cells[root->GetItem()->name()]->write(tedfile, _cells, root);
      root = root->GetNextRoot(TARGETDB_LIB);
   }
   tedfile->putByte(tedf_DESIGNEND);
   modified = false;
}

void laydata::tdtdesign::tmp_draw(const layprop::DrawProperties& drawprop,
                                          TP base, TP newp) {
   ctmqueue tmp_stack;
   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   if (_tmpdata)
   {
      glColor4f((GLfloat)1.0, (GLfloat)1.0, (GLfloat)1.0, (GLfloat)0.7);
      tmp_stack.push_front(CTM(newp - base,1,0,false));
      _tmpdata->tmp_draw(drawprop, tmp_stack,NULL,true);
   }
   else if ((drawprop.currentop() != console::op_none) && _target.checkedit())
   {
      if ((console::op_copy == drawprop.currentop()) || (console::op_move == drawprop.currentop()))
      {
         base *= _target.rARTM();
         newp *= _target.rARTM();
         tmp_stack.push_front(CTM(_target.ARTM()));
         tmp_stack.push_front(CTM(newp - base,1,0,false)*_target.ARTM());
      }
      else if ((console::op_flipX == drawprop.currentop()) || (console::op_flipY == drawprop.currentop()))
      {
         CTM newpos = _target.ARTM();
         tmp_stack.push_front(newpos);
         if (console::op_flipX == drawprop.currentop())
            newpos.FlipX(newp.y());
         else
            newpos.FlipY(newp.x());
         tmp_stack.push_front(newpos);
      }
      else if (console::op_rotate == drawprop.currentop())
      {
         CTM newpos = _target.ARTM();
         tmp_stack.push_front(_target.ARTM());
         newpos.Translate(-newp.x(),-newp.y());
         newpos *= _tmpctm;
         newpos.Translate(newp.x(),newp.y());
         tmp_stack.push_front(newpos);
      }
      _target.edit()->tmp_draw(drawprop, tmp_stack, true);
      tmp_stack.clear();
   }
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void laydata::tdtdesign::mousePoint(TP p)
{
   if (_tmpdata) _tmpdata->addpoint(p);
}

void laydata::tdtdesign::mousePointCancel(TP& lp)
{
   if (_tmpdata) _tmpdata->rmpoint(lp);
}

void laydata::tdtdesign::mouseStop()
{
   if (_tmpdata) delete _tmpdata;
   _tmpdata = NULL;
}

void laydata::tdtdesign::mouseFlip()
{
   if (_tmpdata) _tmpdata->objFlip();
}

void laydata::tdtdesign::mouseRotate()
{
   if (_tmpdata) _tmpdata->objRotate();
}

void laydata::tdtdesign::select_inBox(TP* p1, TP* p2, bool pntsel)
{
   if (_target.checkedit())
   {
      DBbox select_in((*p1)*_target.rARTM(), (*p2)*_target.rARTM());
      select_in.normalize();
      _target.edit()->select_inBox(select_in, _target.viewprop(), pntsel);
   }
}

laydata::atticList* laydata::tdtdesign::change_select(TP* p1, bool select) {
   if (_target.checkedit()) {
      TP selp = (*p1) * _target.rARTM();
      return _target.edit()->changeselect(selp, select ? sh_selected:sh_active, _target.viewprop());
   }
   else return NULL;
}

void laydata::tdtdesign::unselect_inBox(TP* p1, TP* p2, bool pntsel) {
   if (_target.checkedit()) {
      DBbox unselect_in((*p1)*_target.rARTM(), (*p2)*_target.rARTM());
      unselect_in.normalize();
      _target.edit()->unselect_inBox(unselect_in, pntsel, _target.viewprop());
   }
}

void laydata::tdtdesign::copy_selected( TP p1, TP p2) {
   CTM trans;
   p1 *= _target.rARTM();
   p2 *= _target.rARTM();
   int4b dX = p2.x() - p1.x();
   int4b dY = p2.y() - p1.y();
   trans.Translate(dX,dY);
   if (_target.edit()->copy_selected(this, trans)) {
      // needs validation
      do {} while(validate_cells());
   }
}

void laydata::tdtdesign::move_selected( TP p1, TP p2, selectList** fadead)
{
   CTM trans;
   p1 *= _target.rARTM();
   p2 *= _target.rARTM();
   int4b dX = p2.x() - p1.x();
   int4b dY = p2.y() - p1.y();
   trans.Translate(dX,dY);
   if (_target.edit()->move_selected(this, trans, fadead))
      // needs validation
      do {} while(validate_cells());
}

bool laydata::tdtdesign::cutpoly(pointlist& pl, atticList** dasao)
{
   for (pointlist::iterator CP = pl.begin(); CP != pl.end(); CP ++)
      (*CP) *= _target.rARTM();
   return _target.edit()->cutpoly_selected(pl,dasao);
}

void laydata::tdtdesign::rotate_selected( TP p, real angle, selectList** fadead)
{
   // Things to remember...
   // To deal with edit in place, you have to :
   // - get the current translation matrix of the active cell
   // - apply the transformation - in this case rotation around a point
   //   is done in 3 steps - translate to 0, rotate, translate back
   // - multiply with the reversed CTM to get back in the current
   //   coordinates
   // If just rotation is applied (second step) operation can not deal
   // with the cases when the referenced cell is flipped
   CTM trans = _target.ARTM();
   trans.Translate(-p.x(),-p.y());
   trans.Rotate(angle);
   trans.Translate(p.x(),p.y());
   trans *= _target.rARTM();
   if (_target.edit()->rotate_selected(this, trans, fadead))
//   if (_target.edit()->transfer_selected(this, trans))
      // needs validation
      do {} while(validate_cells());
}

void laydata::tdtdesign::flip_selected( TP p, bool Xaxis) {
   // Here - same principle as for rotate_selected
   // othrwise flip toggles between X and Y depending on the angle
   // of rotation of the active cell reference
   CTM trans = _target.ARTM();
   if (Xaxis)  trans.FlipX(p.y());
   else        trans.FlipY(p.x());
   trans *= _target.rARTM();
   if (_target.edit()->transfer_selected(this, trans)) {
      // needs validation
      do {} while(validate_cells());
   }
}

void laydata::tdtdesign::delete_selected(laydata::atticList* fsel, 
                                         laydata::tdtlibdir* libdir) 
{
   //laydata::tdtdesign* ATDB 
   if (_target.edit()->delete_selected(fsel, libdir)) 
   {
      // needs validation
      do {} while(validate_cells());
   }
}

void laydata::tdtdesign::destroy_this(tdtdata* ds, word la, laydata::tdtlibdir* libdir) 
{
   if (_target.edit()->destroy_this(libdir, ds,la))
   {
      // needs validation
      do {} while(validate_cells());
   }
}

bool laydata::tdtdesign::group_selected(std::string name, laydata::tdtlibdir* libdir) {
   // first check that the cell with this name does not exist already
   if (_cells.end() != _cells.find(name)) {
      tell_log(console::MT_ERROR, "Cell with this name already exists. Group impossible");
      return false;
   }
   //unlink the fully selected shapes from the quadTree of the current cell
   atticList* TBgroup = _target.edit()->groupPrep(libdir);
   if (TBgroup->empty()) {
      tell_log(console::MT_WARNING, "Nothing to group");
      delete TBgroup; return false;
   }
   //Create a new cell
   tdtcell* newcell = addcell(name);
   assert(newcell);
   //Get the selected shapes from the current cell and add them to the new cell
   for(atticList::const_iterator CL = TBgroup->begin();
                                                   CL != TBgroup->end(); CL++) {
      shapeList* lslct = CL->second;
      quadTree* wl = newcell->securelayer(CL->first);
      // There is no point here to ensure that the layer definition exists.
      // We are just transfering shapes from one structure to another.
      // ATDB->securelaydef( CL->first );
      securelaydef( CL->first );
      for(shapeList::const_iterator CI = lslct->begin();
                                                     CI != lslct->end(); CI++) {
         wl->put(*CI);
         if (0 == CL->first) newcell->addchild(this,
                                    static_cast<tdtcellref*>(*CI)->structure());
      }
      lslct->clear();
      delete (lslct);
   }
   TBgroup->clear();delete TBgroup;
   newcell->resort();
   //reference the new cell into the current one.
   tdtdata* ref = _target.edit()->addcellref(this,
                                    getcellnamepair(name),CTM(TP(0,0),1,0,false));
   //select the new cell
   ref->set_status(sh_selected);
   _target.edit()->select_this(ref,0);
   //Just for the records. No shapes are moved to the Attic during this operation
   //Undo is possible simply by ungrouping the new cell
   return true;
}

laydata::shapeList* laydata::tdtdesign::ungroup_prep(laydata::tdtlibdir* libdir) 
{
   //unlink the selected ref/aref's from the quadTree of the current cell
   return _target.edit()->ungroupPrep(libdir);
}

laydata::atticList* laydata::tdtdesign::ungroup_this(laydata::shapeList* cells4u) {
   laydata::atticList* shapeUngr = DEBUG_NEW laydata::atticList();
   for (shapeList::const_iterator CC = cells4u->begin();
                                                     CC != cells4u->end(); CC++)
      static_cast<tdtcellref*>(*CC)->ungroup(this, _target.edit(), shapeUngr);
   _target.edit()->validate_layers();
   return shapeUngr;
}

bool laydata::tdtdesign::checkValidRef(std::string newref)
{
   if ( _cells.end() == _cells.find(newref) )
   {
      std::string news = "Cell \"";
      news += newref; news += "\" is not defined";
      tell_log(console::MT_ERROR,news);
      return false;
   }
   laydata::tdtdefaultcell* child = _cells[newref];
   if (_hiertree->checkAncestors(_target.edit(), child, _hiertree))
   {
      tell_log(console::MT_ERROR, "Circular reference is forbidden.");
      return false;
   }
   return true;
}

laydata::atticList* laydata::tdtdesign::changeref(shapeList* cells4u, std::string newref)
{
   assert(checkcell(newref));
   assert((!cells4u->empty()));
   laydata::shapeList* cellsUngr = DEBUG_NEW laydata::shapeList();
   laydata::refnamepair striter = getcellnamepair(newref);
   DBbox old_overlap = _target.edit()->overlap();

   for (shapeList::const_iterator CC = cells4u->begin(); CC != cells4u->end(); CC++)
   {
      CTM ori = static_cast<tdtcellref*>(*CC)->translation();
      ArrayProperties arrayprops = static_cast<tdtcellref*>(*CC)->arrayprops();
      tdtdata* ncrf;
      if (arrayprops.valid())
         ncrf = _target.edit()->addcellaref(this, striter, ori, arrayprops);
      else
         ncrf = _target.edit()->addcellref(this, striter, ori);
      assert(NULL != ncrf);
      ncrf->set_status(sh_selected);
      _target.edit()->select_this(ncrf,0);
      cellsUngr->push_back(ncrf);
   }
   laydata::atticList* shapeUngr = DEBUG_NEW laydata::atticList();
   (*shapeUngr)[0] = cellsUngr;
   if (_target.edit()->overlapChanged(old_overlap, this))
      do {} while(validate_cells());
   return shapeUngr;
}

unsigned int laydata::tdtdesign::numselected() const
{
   if (_target.checkedit()) return _target.edit()->numselected();
   else return 0;
}

DBbox laydata::tdtdesign::activeoverlap() {
   DBbox ovl = _target.overlap();
   if (ovl == DEFAULT_OVL_BOX) ovl = DEFAULT_ZOOM_BOX;
   return ovl;
//   if (_target.checkedit())
//      return _target.edit()->overlap() * _ARTM;
//   else return DEFAULT_OVL_BOX;
}

void laydata::tdtdesign::check_active() {
   if (NULL == _target.edit()) throw EXPTNactive_cell();
};


laydata::quadTree* laydata::tdtdesign::targetlayer(word layno)
{
   securelaydef( layno );
   return _target.edit()->securelayer(layno);
}

void laydata::tdtdesign::transferLayer(word dst)
{
   _target.securelaydef( dst );
   _target.edit()->transferLayer(dst);
}

void laydata::tdtdesign::transferLayer(laydata::selectList* slst, word dst)
{
   _target.securelaydef( dst );
   _target.edit()->transferLayer(slst, dst);
}

laydata::tdtdesign::~tdtdesign() {
   //clean-up the _target stack
   for( editcellstack::iterator CECS = _target._editstack.begin();
                                CECS != _target._editstack.end(); CECS++)
      delete (*CECS);
   _target._editstack.clear();
}
//
//
