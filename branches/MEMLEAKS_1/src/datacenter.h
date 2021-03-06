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
// ------------------------------------------------------------------------ =
//           $URL$
//  Creation date: Sat Jan 10 2004
//     Created by: Svilen Krustev - s_krustev@yahoo.com
//      Copyright: (C) 2001-2004 by Svilen Krustev
//    Description: wxWidget version
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------                
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================

#ifndef DATA_HANDLER_INCLUDED
#define DATA_HANDLER_INCLUDED
#include "../tpd_DB/tedesign.h"
#include "gds_io.h"

namespace GDSin {
   class gds2ted {
   public:
      gds2ted(GDSin::GDSFile* src_lib, laydata::tdtdesign* dst_lib);
      void structure(const char* gname, bool recursive, bool overwrite);
   protected:
      void                 convert(GDSin::GDSstructure*, laydata::tdtcell*);
      void                 polygon(GDSin::GDSpolygon*, laydata::tdtcell*);
      void                 path(GDSin::GDSpath*, laydata::tdtcell*);
      void                 ref(GDSin::GDSref*, laydata::tdtcell*);
      void                 aref(GDSin::GDSaref*, laydata::tdtcell*);
      void                 text(GDSin::GDStext*, laydata::tdtcell*);
      GDSin::GDSFile*      _src_lib;
      laydata::tdtdesign*  _dst_lib;
      real                 coeff; // DBU difference
   };
}

class DataCenter {
public:
                              DataCenter();
                             ~DataCenter(); 
   void                       GDSparse(std::string filename, std::list<std::string>&);
   void                       GDSexport(std::string&);
   void                       GDSexport(laydata::tdtcell*, bool, std::string&);
   void                       importGDScell(const char* name, bool recur, bool over);
   void                       reportGDSlay(const char* name);
   GDSin::GDSstructure*       GDSstructures();
   void                       GDSclose();
   bool                       TDTread(std::string filename);
   void                       TDTwrite(const char* filename = NULL);
   void                       newDesign(std::string name);
   laydata::tdtdesign*        lockDB(bool checkACTcell = true);
   void                       unlockDB();
   unsigned int               numselected() const;
   void                       mouseStart(int input_type);
   void                       mousePointCancel(TP&);
   void                       mousePoint(TP p);
   void                       mouseStop();
   void                       openGL_draw(const layprop::DrawProperties&);
   void                       tmp_draw(const layprop::DrawProperties&, TP, TP);
   const laydata::cellList&   cells();
   
   std::string                tedfilename() const  {return _tedfilename;};
   bool                       neversaved()  const  {return _neversaved;}; 
   bool                       modified() const     {return (NULL == _TEDDB) ? false : _TEDDB->modified;}; 
   TIME_TPD                   tedtimestamp() const {return _tedtimestamp;};
protected:
   laydata::tdtdesign*        _TEDDB;      // toped data base
   GDSin::GDSFile*            _GDSDB;      // GDS parsed data
   std::string                _tedfilename;
   bool                       _neversaved;
   TIME_TPD                   _tedtimestamp;
};

#endif
