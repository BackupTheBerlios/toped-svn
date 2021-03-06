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
//        Created: Sun Mar 07 2004
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: Database structure in the memory - clipping
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================

#ifndef QUADTREE_H
#define QUADTREE_H

#include "tedstd.h"
#include "gds_io.h"
#include "ps_out.h"

namespace laydata {

//==============================================================================
   /*! quadTree class implements the main clipping algorithm of toped. Its main 
      purpose is to speed-up the drawing of the database. All objects of type
      tdtdata or its derivatives - means all layout data - are stored into the 
      objects of quadTree type. Each object of quadTree class is responsible for a 
      rectangular area defined by the _overlap field. This area is dynamic and 
      updated with every added, moved or deleted layout object.\n Each quadTree 
      object might be a parent of maximum four child objects of the same quadTree 
      class, so that each of the children is responsible for one out of the 
      possible four sub-rectangles. Every layout object is fitted during the 
      construction into the smallest possible quadTree object. The children are
      created dynamically when a new object is about to fit into one of the four 
      possible sub-rectangles of the _overlap area. Child quadTree objects are
      stored into an quads[4] array and each of them is responsible by convention
      for the NW(north-west), NE(north-east), SE(south-east) and SW(south-west)
      subrectangles of the _overlap box.\n
      The methods can be split on several groups:
         - add a single layout object - add(), fitintree()
         - add a group of layout objects - put(), sort(), fitsubtree()
         - object selection - select_inBox(), unselect_inBox(), select_fromList(), 
           select_all()
         - design modification - delete_marked()
         - tree maintanence - validate(), full_validate(), sort(), resort(),
           tmpstore()
            */
   class quadTree {
   public:
                           quadTree();
                           quadTree(TEDfile* const tedfile);
      virtual             ~quadTree();
      virtual void         openGL_draw(layprop::DrawProperties&, const dataList*, bool) const;
      virtual void         tmp_draw(const layprop::DrawProperties&, ctmqueue&) const;
      void                 add(tdtdata* shape);
      void                 put(tdtdata* shape);
      void                 write(TEDfile* const) const;
      void                 GDSwrite(GDSin::GDSFile&, word const, real UU) const;
      void                 PSwrite(PSFile&, const layprop::DrawProperties&) const;
      void                 select_inBox(DBbox&, dataList*, bool);
      void                 select_fromList(dataList*, dataList*);
      void                 select_all(dataList*, bool mark = true);
      void                 unselect_inBox(DBbox&, dataList*, bool);
      bool                 delete_marked(SH_STATUS stat=sh_selected, bool partselect=false);
      bool                 delete_this(laydata::tdtdata*);
      void                 cutpoly_selected(pointlist&, DBbox&, shapeList**);
      tdtdata*             merge_selected(tdtdata*& shapeRef);
/*      tdtdata*             getfirstover(const TP);
      tdtdata*             getnextover(const TP, laydata::tdtdata*, bool& check);*/
      bool                 getobjectover(const TP pnt, laydata::tdtdata*& prev);
      void                 validate();
      bool                 full_validate();
      void                 resort();
      bool                 empty() const;
      void                 freememory();
      /*! Return the overlapping box*/
      DBbox                overlap() const   {return _overlap;};
      /*! Mark the tree as invalid*/
      void                 invalidate()      {_invalid = true;};
      /*! Return the status of _invalid flag*/
      bool                 invalid() const   {return _invalid;};
   private:
      void                 sort(dataList);
      bool                 fitintree(tdtdata* shape);
      int                  fitsubtree(DBbox, DBbox*);
      void                 tmpstore(dataList &store);
      byte                 biggest(float* array) const;
      void                 update_overlap(DBbox hovl);
      /*! The overlapping box*/
      DBbox               _overlap;
      /*! A pointers to four child quadTree structures*/
      quadTree*           _quads[4];
      /*! Pointer to the first tdtdata stored in this quadTree*/
      tdtdata*            _first;
      /*! Flag indicates tha the containter needs to be resorted*/
      bool                _invalid; 
   };

//==============================================================================
/*! Represent the layer placeholder of the tedat database. The expected 
functionality is mostly implemented in the parent class. 
*/   
   class tdtlayer : public quadTree {
   public:
                           tdtlayer() : quadTree() {};
                           tdtlayer(TEDfile* const tedfile) : quadTree(tedfile) {};
                          ~tdtlayer() {freememory();};
      void                 tmp_draw(const layprop::DrawProperties&, ctmqueue& ) const;
      tdtdata*             addbox(TP* p1, TP* p2, bool sortnow = true);
      tdtdata*             addpoly(pointlist& pl, bool sortnow = true);
      tdtdata*             addwire(pointlist& pl,word w, bool sortnow = true);
      tdtdata*             addtext(std::string text, CTM trans, bool sortnow = true);
   };

}

#endif
