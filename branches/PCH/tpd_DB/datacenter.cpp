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
//        Created: Sat Jan 10 2004
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: TDT I/O and database access control
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================

#include "tpdph.h"
#include <sstream>
#include "datacenter.h"
#include "../tpd_common/outbox.h"
#include "tedat.h"
#include "viewprop.h"
#include "ps_out.h"

// Global variables
DataCenter*               DATC = NULL;

//-----------------------------------------------------------------------------
// class gds2ted
//-----------------------------------------------------------------------------
GDSin::gds2ted::gds2ted(GDSin::GDSFile* src_lib, laydata::tdtdesign* dst_lib) {
   _src_lib = src_lib;
   _dst_lib = dst_lib;
   coeff = dst_lib->UU() / src_lib->Get_LibUnits();
}   

void GDSin::gds2ted::structure(const char* gname, bool recursive, bool overwrite) {
   // source structure
   GDSin::GDSstructure *src_structure = _src_lib->GetStructure(gname);
   if (!src_structure) {
      std::string news = "GDS structure named \"";
      news += gname; news += "\" does not exists";
      tell_log(console::MT_ERROR,news);
      return;
   }
   // check that destination structure with this name exists
   laydata::tdtcell* dst_structure = _dst_lib->checkcell(gname);
   std::ostringstream ost; ost << "GDS import: ";
   if (NULL != dst_structure) {
      if (overwrite) {
         /*@TODO Erase the existing structure and convert*/
         ost << "Structure "<< gname << " should be overwritten, but cell erase is not implemened yet ...";
         tell_log(console::MT_WARNING,ost.str());
      }
   // Don't report this , except maybe in case of a verbose or similar option is introduced
   // On a large GDS file those messages are simply anoying
      else
      {
         ost << "Structure "<< gname << " already exists. Omitted";
         tell_log(console::MT_INFO,ost.str());
      }
   }
   else {
      // proceed with children first
      if (recursive) {
         GDSin::ChildStructure nextofkin = src_structure->children;
         typedef GDSin::ChildStructure::iterator CI;
         for (CI ci = nextofkin.begin(); ci != nextofkin.end(); ci++)
            structure((*ci)->Get_StrName(), recursive, overwrite);
      }
      ost << "Importing structure " << gname << "...";
      tell_log(console::MT_INFO,ost.str());
      // first create a DEBUG_NEW cell
      dst_structure = _dst_lib->addcell(gname);
      // now call the cell converter
      convert(src_structure, dst_structure);
   }
}

void GDSin::gds2ted::convert(GDSin::GDSstructure* src, laydata::tdtcell* dst) {
   GDSin::GDSdata *wd = src->Get_Fdata();
   while( wd ){
      switch( wd->GetGDSDatatype() ){
//         case      gds_BOX: box(static_cast<GDSin::GDSbox*>(wd), dst);  break;
         case      gds_BOX:
         case gds_BOUNDARY: polygon(static_cast<GDSin::GDSpolygon*>(wd), dst);  break;
         case     gds_PATH: path(static_cast<GDSin::GDSpath*>(wd), dst);  break;
         case     gds_SREF: ref(static_cast<GDSin::GDSref*>(wd), dst);  break;
         case     gds_AREF: aref(static_cast<GDSin::GDSaref*>(wd), dst);  break;
         case     gds_TEXT: text(static_cast<GDSin::GDStext*>(wd), dst);  break;
                   default: {/*Error - unexpected type*/}
      }
      wd = wd->GetLast();
   }
   dst->resort();
//   dst->secure_layprop();
}

void GDSin::gds2ted::polygon(GDSin::GDSpolygon* wd, laydata::tdtcell* dst) {
   laydata::tdtlayer* wl = static_cast<laydata::tdtlayer*>
                                          (dst->securelayer(wd->GetLayer()));
   pointlist &pl = wd->GetPlist();
   laydata::valid_poly check(pl);

   if (!check.valid()) {
      std::ostringstream ost; ost << "Layer " << wd->GetLayer();
      ost << ": Polygon check fails - " << check.failtype();
      tell_log(console::MT_ERROR, ost.str());
   }   
   else pl = check.get_validated() ;
   if (check.box()) {
      wl->addbox(DEBUG_NEW TP(pl[0]), DEBUG_NEW TP(pl[2]),false);
   }
   else wl->addpoly(pl,false);
}

void GDSin::gds2ted::path(GDSin::GDSpath* wd, laydata::tdtcell* dst) {
   laydata::tdtlayer* wl = static_cast<laydata::tdtlayer*>
                                          (dst->securelayer(wd->GetLayer()));
   pointlist &pl = wd->GetPlist();
   laydata::valid_wire check(pl, wd->Get_width());

   if (!check.valid()) {
      std::ostringstream ost; ost << "Layer " << wd->GetLayer();
      ost << ": Wire check fails - " << check.failtype();
      tell_log(console::MT_ERROR, ost.str());
      return;
   }   
   else pl = check.get_validated() ;
   /* @TODO !!! GDS path type here!!!! */
   wl->addwire(pl, wd->Get_width(),false);
}

void GDSin::gds2ted::ref(GDSin::GDSref* wd, laydata::tdtcell* dst) {
   if (NULL != _dst_lib->checkcell(wd->GetStrname())) {
      laydata::refnamepair striter = _dst_lib->getcellnamepair(wd->GetStrname());
      // Absolute magnification, absolute angle should be reflected somehow!!!
      dst->addcellref(_dst_lib,
                   striter, 
                   CTM(wd->GetMagn_point(), 
                   wd->GetMagnification(),
                   wd->GetAngle(),
                   (0 != wd->GetReflection())),
                   false
      );
   }
   else {
      std::string news = "Referenced structure \"";
      news += wd->GetStrname(); news += "\" not found. Reference ignored";
      tell_log(console::MT_ERROR,news);
   }
   // How about structures defined, but not parsed yet????
}

void GDSin::gds2ted::aref(GDSin::GDSaref* wd, laydata::tdtcell* dst) {
   if (NULL != _dst_lib->checkcell(wd->GetStrname())) {
      laydata::refnamepair striter = _dst_lib->getcellnamepair(wd->GetStrname());
      // Absolute magnification, absolute angle should be reflected somehow!!!

      laydata::ArrayProperties arrprops(wd->Get_Xstep(),wd->Get_Ystep(),
                                 static_cast<word>(wd->Get_colnum()),
                                 static_cast<word>(wd->Get_rownum()));
      dst->addcellaref(_dst_lib,
         striter, 
         CTM( wd->GetMagn_point(), 
              wd->GetMagnification(), 
              wd->GetAngle(),
              (0 != wd->GetReflection()) ),
         arrprops,
         false
      );
   }
   else {
      std::string news = "Referenced structure \"";
      news += wd->GetStrname(); news += "\" not found. Reference ignored";
      tell_log(console::MT_ERROR,news);
   }
   // How about structures defined, but not parsed yet????
}

void GDSin::gds2ted::text(GDSin::GDStext* wd, laydata::tdtcell* dst) {
   laydata::tdtlayer* wl = static_cast<laydata::tdtlayer*>
                                       (dst->securelayer(wd->GetLayer()));
   // @FIXME absolute magnification, absolute angle should be reflected somehow!!!
   wl->addtext(wd->GetText(), 
               CTM(wd->GetMagn_point(), 
                   wd->GetMagnification() / (_dst_lib->UU() *  OPENGL_FONT_UNIT),
                   wd->GetAngle(),
                   (0 != wd->GetReflection())));
}

//-----------------------------------------------------------------------------
// class DataCenter
//-----------------------------------------------------------------------------
DataCenter::DataCenter() {
   _GDSDB = NULL; _TEDDB = NULL;
   _tedfilename = "unnamed";
   _curlay = 1;
}
   
DataCenter::~DataCenter() {
   if (NULL != _GDSDB) delete _GDSDB;
   if (NULL != _TEDDB) delete _TEDDB;
}
bool DataCenter::TDTcheckread(const std::string filename,
    const TpdTime& timeCreated, const TpdTime& timeSaved, bool& start_ignoring)
{
   bool retval = false;
   start_ignoring = false;
   laydata::TEDfile tempin(filename.c_str());
   if (!tempin.status()) return retval;

   std::string news = "Project created: ";
   TpdTime timec(tempin.created()); news += timec();
   tell_log(console::MT_INFO,news);
   news = "Last updated: ";
   TpdTime timeu(tempin.lastUpdated()); news += timeu();
   tell_log(console::MT_INFO,news);
   // File created time stamp must match exactly, otherwise it means
   // that we're reading not exactly the same file that is requested
   if (timeCreated != timec)
   {
      news = "time stamp \"Project created \" doesn't match";
      tell_log(console::MT_ERROR,news);
   }
   if (timeu.stdCTime() < timeSaved.stdCTime())
   {
      news = "time stamp \"Last updated \" is too old.";
      tell_log(console::MT_ERROR,news);
   }
   else if (timeu.stdCTime() > timeSaved.stdCTime())
   {
      news = "time stamp \"Last updated \" is is newer than requested.";
      news +="Some of the following commands will be ignored";
      tell_log(console::MT_WARNING,news);
      //Start ignoring
      start_ignoring = true;
      retval = true;
   }
   tempin.closeF();
   return retval;
} 

bool DataCenter::TDTread(std::string filename)
{
   laydata::TEDfile tempin(filename.c_str());
   if (!tempin.status()) return false;

   try
   {
      tempin.read();
   }
   catch (EXPTNreadTDT)
   {
      tempin.closeF();
      tempin.cleanup();
      return false;
   }
   tempin.closeF();
   delete _TEDDB;//Erase existing data
   _tedfilename = filename;
   _neversaved = false;
   _TEDDB = tempin.design();
   _TEDDB->assign_properties(_properties);
   // Update Canvas scale
   _properties.setUU(_TEDDB->UU());
   return true;
}

bool DataCenter::TDTcheckwrite(const TpdTime& timeCreated, const TpdTime& timeSaved, bool& stop_ignoring)
{
   std::string news;
   stop_ignoring = false;
   // File created time stamp must match exactly, otherwise it means
   // that we're saving not exactly the same file that is requested
   if (timeCreated.stdCTime() != _TEDDB->created())
   {
      news = "time stamp \"Project created \" doesn't match. File save aborted";
      tell_log(console::MT_ERROR,news);
      return false;
   }
   if (_TEDDB->lastUpdated() < timeSaved.stdCTime())
   {
      news = "Database in memory is older than the file. File save operation ignored.";
      tell_log(console::MT_WARNING,news);
      _neversaved = false;
      return false;
   }
   else if (_TEDDB->lastUpdated() > timeSaved.stdCTime())
      // database in memory is newer than the file - save has to go ahead
      // ignore on recovery has to stop
      stop_ignoring = true;
   else
   {
      // database in memory is exactly the same as the file. The save 
      // is going to be spared, ignore on recovery though has to stop
      stop_ignoring = true;
      return false;
   }
   return true;
}

bool DataCenter::TDTwrite(const char* filename)
{
   if (filename)  _tedfilename = filename;
   laydata::TEDfile tempin(_TEDDB, _tedfilename);
   _neversaved = false;
   return true;
}

void DataCenter::GDSexport(std::string& filename)
{
   std::string nfn;
   //Get actual time
   GDSin::GDSFile gdsex(filename, time(NULL));
   _TEDDB->GDSwrite(gdsex, NULL, true);
   gdsex.updateLastRecord();gdsex.closeFile();
}

void DataCenter::GDSexport(laydata::tdtcell* cell, bool recur, std::string& filename)
{
   std::string nfn;
   //Get actual time
   GDSin::GDSFile gdsex(filename, time(NULL));
   _TEDDB->GDSwrite(gdsex, cell, recur);
   gdsex.updateLastRecord();gdsex.closeFile();
}

void DataCenter::GDSparse(std::string filename) 
{
   if (lockGDS(false))
   {
      
      std::string news = "Removing existing GDS data from memory...";
      tell_log(console::MT_WARNING,news);
      GDSclose();
      unlockGDS();
   }
   // parse the GDS file - don't forget to lock the GDS mutex here!
   while (wxMUTEX_NO_ERROR != GDSLock.TryLock());
   _GDSDB = DEBUG_NEW GDSin::GDSFile(filename.c_str());
   if (_GDSDB->status())
   {
      // generate the hierarchy tree of cells
      _GDSDB->HierOut();
   }
   unlockGDS();
}

void DataCenter::importGDScell(const nameList& top_names, bool recur, bool over) {
   if (NULL == lockGDS())
   {
      std::string news = "No GDS data in memory. Parse GDS file first";
      tell_log(console::MT_ERROR,news);
   }
   {
      // Lock the DB here manually. Otherwise the cell browser is going mad
      GDSin::gds2ted converter(_GDSDB, _TEDDB);
      for (nameList::const_iterator CN = top_names.begin(); CN != top_names.end(); CN++)
         converter.structure(CN->c_str(), recur, over);
      _TEDDB->modified = true;
      unlockGDS();
   }
}

void DataCenter::GDSclose() {
   lockGDS();
      delete _GDSDB;
      _GDSDB = NULL;
   unlockGDS();
}

void DataCenter::PSexport(laydata::tdtcell* cell, std::string& filename)
{
   //Get actual time
   PSFile psex(filename);
   _properties.drawprop().PSwrite(psex);
   _TEDDB->PSwrite(psex, cell, _properties.drawprop());
//   gdsex.closeFile();
}

void DataCenter::newDesign(std::string name, time_t created) 
{
   if (_TEDDB) 
   {
      // Checks before closing(save?) available only when the command is launched 
      // via GUI(void TopedFrame::OnNewDesign(). If the command is typed directly 
      // on the command line, or parsed from file - no checks are executed.
      // In other words if we are already here we will destroy the current design
      // without much talking.
      // UNDO buffers will be reset as well in tellstdfunc::stdNEWDESIGN::execute()
      // but there is still a chance to restore everything - using the log file.
      delete _TEDDB;
   }   
   _TEDDB = DEBUG_NEW laydata::tdtdesign(name, created, 0);
   _TEDDB->assign_properties(_properties);
   _tedfilename = name + ".tdt";
   _neversaved = true;
   _properties.setUU(_TEDDB->UU());
}

laydata::tdtdesign*  DataCenter::lockDB(bool checkACTcell) 
{
   if (_TEDDB) 
   {
      if (checkACTcell) _TEDDB->check_active();
      while (wxMUTEX_NO_ERROR != DBLock.TryLock());
      return _TEDDB;
   }
   else throw EXPTNactive_DB();
}

void DataCenter::unlockDB() 
{
   DBLock.Unlock();
}

GDSin::GDSFile* DataCenter::lockGDS(bool throwexception) 
{
   // Carefull HERE! When GDS is locked form the main thread
   // (GDS browser), then there is no catch pending -i.e.
   // throwing an exception will make the things worse
   // When it is locked from the parser command - then exception
   // is fine 
   if (_GDSDB) 
   {
      while (wxMUTEX_NO_ERROR != GDSLock.TryLock());
      return _GDSDB;
   }
   else {
      if (throwexception) throw EXPTNactive_GDS();
      else return NULL;
   }
}

void DataCenter::unlockGDS() 
{
   GDSLock.Unlock();
}

unsigned int DataCenter::numselected() const {
   if (_TEDDB) return _TEDDB->numselected();
   else return 0;
}

void DataCenter::mouseStart(int input_type, std::string name, const CTM trans,
                            int4b stepX, int4b stepY, word cols, word rows)
{
   if (console::op_line == input_type) return;
   if (_TEDDB)
   {
      _TEDDB->check_active();
      _TEDDB->mouseStart(input_type, name, trans, stepX, stepY, cols, rows);
   }
   else throw EXPTNactive_DB();
}

void DataCenter::mousePoint(TP p)
{
   if (console::op_line == currentop())
      _properties.mousePoint(p);
   else if ((_TEDDB) && (console::op_cbind != currentop())
                     && (console::op_abind != currentop())
                     && (console::op_tbind != currentop()) )
      _TEDDB->mousePoint(p);
}

void DataCenter::mousePointCancel(TP& lp)
{
   if (console::op_line == currentop()) return;
   if (_TEDDB)
      _TEDDB->mousePointCancel(lp);
}

void DataCenter::mouseStop()
{
   if (console::op_line == currentop())
      _properties.mouseStop();
   if (_TEDDB) _TEDDB->mouseStop();
   else throw EXPTNactive_DB();
}

void DataCenter::mouseFlip()
{
   if (_TEDDB) _TEDDB->mouseFlip();
}

void DataCenter::mouseRotate()
{
   if (_TEDDB) _TEDDB->mouseRotate();
}

void DataCenter::openGL_draw(const CTM& layCTM) {
// Maybe we need another try/catch in the layoutcanvas ?   
   if (_TEDDB) {
//      _TEDDB->check_active();
      while (wxMUTEX_NO_ERROR != DBLock.TryLock());
      while (wxMUTEX_NO_ERROR != PROPLock.TryLock());
      _properties.drawGrid();
      _TEDDB->openGL_draw(_properties.drawprop());
      _properties.drawRulers(layCTM);
      DBLock.Unlock();
      PROPLock.Unlock();
   }
// 
//   else throw EXPTNactive_DB();      
}

void DataCenter::tmp_draw(const CTM& layCTM, TP base, TP newp) {
   if (console::op_line == currentop()) {
      // ruller
      while (wxMUTEX_NO_ERROR != PROPLock.TryLock());
      _properties.tmp_draw(layCTM, base, newp);
      PROPLock.Unlock();
   }
   else if (_TEDDB) {
//      _TEDDB->check_active();
      while (wxMUTEX_NO_ERROR != DBLock.TryLock());
      _TEDDB->tmp_draw(_properties.drawprop(), base, newp);
      DBLock.Unlock();
   }
// 
//   else throw EXPTNactive_DB();      
}

const laydata::cellList& DataCenter::cells() {
   if (_TEDDB) return _TEDDB->cells();
   else throw EXPTNactive_DB();
};


bool DataCenter::addlayer(std::string name, word layno, std::string col,
                                       std::string fill, std::string sline)

{
   bool status;
   while (wxMUTEX_NO_ERROR != PROPLock.TryLock());
   status = _properties.addlayer(name, layno, col, fill, sline);
   PROPLock.Unlock();
   return status;
}

bool DataCenter::addlayer(std::string name, word layno)
{
   bool status;
   while (wxMUTEX_NO_ERROR != PROPLock.TryLock());
   status = _properties.addlayer(name, layno);
   PROPLock.Unlock();
   return status;
}

void DataCenter::addline(std::string name, std::string col, word pattern,
                                      byte patscale, byte width)
{
   while (wxMUTEX_NO_ERROR != PROPLock.TryLock());
   _properties.addline(name, col, pattern, patscale, width);
   PROPLock.Unlock();
}

void DataCenter::addcolor(std::string name, byte R, byte G, byte B, byte A)
{
   while (wxMUTEX_NO_ERROR != PROPLock.TryLock());
   _properties.addcolor(name, R, G, B, A);
   PROPLock.Unlock();
}

void DataCenter::addfill(std::string name, byte *ptrn)
{
   while (wxMUTEX_NO_ERROR != PROPLock.TryLock());
   _properties.addfill(name, ptrn);
   PROPLock.Unlock();
}

void DataCenter::hideLayer(word layno, bool hide)
{
   while (wxMUTEX_NO_ERROR != PROPLock.TryLock());
   _properties.hideLayer(layno, hide);
   PROPLock.Unlock();
}

void DataCenter::lockLayer(word layno, bool lock)
{
   while (wxMUTEX_NO_ERROR != PROPLock.TryLock());
   _properties.lockLayer(layno, lock);
   PROPLock.Unlock();
}

void DataCenter::setcellmarks_hidden(bool hide)
{
   while (wxMUTEX_NO_ERROR != PROPLock.TryLock());
   _properties.setcellmarks_hidden(hide);
   PROPLock.Unlock();
}

void DataCenter::settextmarks_hidden(bool hide)
{
   while (wxMUTEX_NO_ERROR != PROPLock.TryLock());
   _properties.settextmarks_hidden(hide);
   PROPLock.Unlock();
}

void DataCenter::setcellbox_hidden(bool hide)
{
   while (wxMUTEX_NO_ERROR != PROPLock.TryLock());
   _properties.setcellbox_hidden(hide);
   PROPLock.Unlock();
}

void DataCenter::settextbox_hidden(bool hide)
{
   while (wxMUTEX_NO_ERROR != PROPLock.TryLock());
   _properties.settextbox_hidden(hide);
   PROPLock.Unlock();
}

void DataCenter::setGrid(byte No, real step, std::string colname)
{
   while (wxMUTEX_NO_ERROR != PROPLock.TryLock());
   _properties.setGrid(No, step, colname);
   PROPLock.Unlock();
}

bool DataCenter::viewGrid(byte No, bool status)
{
   while (wxMUTEX_NO_ERROR != PROPLock.TryLock());
   status = _properties.viewGrid(No, status);
   PROPLock.Unlock();
   return status;
}

void DataCenter::addRuler(TP& p1, TP& p2)
{
   while (wxMUTEX_NO_ERROR != PROPLock.TryLock());
   _properties.addRuler(p1, p2);
   PROPLock.Unlock();
}

void DataCenter::clearRulers()
{
   while (wxMUTEX_NO_ERROR != PROPLock.TryLock());
   _properties.clearRulers();
   PROPLock.Unlock();
}

void initDBLib()
{
   DATC = DEBUG_NEW DataCenter();
}

