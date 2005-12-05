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
//  Creation date: Tue Feb 25 2003
//         Author: Svilen Krustev - s_krustev@yahoo.com
//      Copyright: (C) 2001-2004 by Svilen Krustev
//    Description: wxWidget version
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------                
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================
#include <sstream>
#include <iostream>
#include <math.h>
#include <algorithm>
#include <GL/glut.h>
#include "tedat.h"
#include "tedcell.h"
#include "logicop.h"
#include "../tpd_common/tedop.h"
#include "../tpd_common/outbox.h"

GLubyte select_mark[30] = {0x00, 0x00, 0x00, 0x00, 0x3F, 0xF8, 0x3F, 0xF8, 0x30, 0x18,
                           0x30, 0x18, 0x30, 0x18, 0x30, 0x18, 0x30, 0x18, 0x30, 0x18, 
                           0x30, 0x18, 0x3F, 0xF8, 0x3F, 0xF8, 0x00, 0x00, 0x00, 0x00};

                           
//-----------------------------------------------------------------------------
// Initialize the GLU tessellator static member
//-----------------------------------------------------------------------------
GLUtriangulatorObj *laydata::tdtdata::tessellObj = gluNewTess();

/*===========================================================================
      Select and subsequent operations over the existing tdtdata
=============================================================================
It appears this is not so strightforward as it seems at first sight. There
are several important points to consider here.
   - On first place of course fitting this operatoins within the existing
     data base structure and esspecially make quadTree trasparent to them.
     The latter appears to be quite important, because this structure is
     not existing for the user, yet plays a major role in the database 
     integrity. The other thing of a significant importance is that the 
     selected structures have to be drawn differently - i.e. they need to be 
     easyly distinguishable on the screen from the other shapes.
   - Minimization of the database. As now the database is build strictly
     on a hierarchical and "conspirative" principles so that every part of
     it holds the absolute minimum of data. For example the layer structure 
     does not now shit about the cell it belongs to, it even doesn't know 
     what is its layer number. Similarly the shape doesn't know neither it's
     placeholder nor its layer or cell. Everything is on kind of "need to know
     basis". Lower levels of the database are unaware of the hier levels.
     Furthermore on certain hierarchical level every component knows only about 
     the neithboring component on the right (next one), but nothing about the 
     one on the left (previous one)
   - Select operation assumes however that an operation like move/delete/copy 
     etc. is about to follow, i.e. the selected shapes will be accessed shortly
     and this access has to be swift and reliable. Certainly we need some short 
     way to access the already selected structures. 
     There are generally two possible ways to do this 
     -> mark every selected figure 
     -> create a list that contains a pointers to the selected figures
     The irony here is that both possibilities suffer from the "conspirative"
     principle on which database is build. In other words it appears that it 
     is not enough to know what is the selected shape. Why?
     How on earth you are going to delete a shape if you know only its next of 
     kin? As a matter of fact it is possible ;), the only detail is that 
     "Segmentatoin fault" will follow inevitably.
     Appparently you need to know what is the structure in the upper level
     of the hierarchy (it's father if you want). 
   - The vast majority  of the shapes (may be all of them in the near feature) 
     are placed in a quadTree. The objects of this class however "by nature"
     are like moving sands. They might change dramatically potentially with 
     every little change in the data base. This effectively means that the
     pointer to the shape placeholder is not that reliable as one would wish to.
     It is OK as long as data base hasn't changed after the select operation.
   - By definition TELL should be able to operate with TOPED shapes (or list of 
     them). Select operation should be able to return to TELL a reliable list
     of TOPED shapes, that can be used in any moment. A "small detail" here is 
     the modify or info operation on a TOPED shape. We certainly need to know the 
     layer, shape is placed on, and maybe the cell. TELL doesn't know (and doesn't 
     want to know) about the TOPED database structure, quadTree's etc. shit.
   - Undo list - this is not started yet, but seems obvoius that in order to 
     execute undelete for example quadTree pointers will not be very helpfull, 
     because they migh be gone already. Instead a layer number will be 
     appreciated. All this in case the shape itself is not deleted, but just moved
     in the Attic.
   ----------------------------------------------------------------------------
   In short:
   - It doesn't seems a good idea to sacrifice the database (in terms of size 
     and speed of drawing) for select operations. Means I don't want to add 
     more data to every single shape object.
   - The TELL operations with TOPED shapes on second though seem not to be so
     convinient and required "by definition" (see the paragraph below)
   - Pointer to a quadTree as a parent object to the selected shape seems to 
     be inevitable despite the unstable nature of teh quadTree - otherwise every
     subsequent operation on the selected objects will require searching of the
     marked components one by one troughout entire cell or at least layer. This 
     might take more time than the select itself, which doesn't seems to be very 
     wise.
   - For undo purposes we will need separate list type for selected components 
     that will contain layer number instead of quadTree.
   -----------------------------------------------------------------------------
   Now about TELL operations over TOPED shapes.
   What we need in TELL without doubt is;
   -> select TOPED objects
   -> move/copy/delete etc. selected objects.
   -> modify selected object - that is the same as previous, but also - change 
      the contents and size of text data, change the cell in ref/aref data etc.
   Do we need to be able to keep more than one set of selected objects?
   This is quite tempting. A lot I would say. So what it will give to TELL?
   Ability to keep a list of TOPED objects in a variable, that can be used
   later... and to make a big mess !!! 
   - Object selection in TOPED is done ALWAYS withing a cell. Having a TELL 
     variable stored somewhere, how the parser will control where the selection
     is from and what is the active cell at the moment?
   - Even if you want to copy or move some group of shapes from one cell to
     another there is still a way: opencell<source>->select->group<new_cell>
     ->opencell<destination>->addref<new_cell>
   - What happens if the TOPED components listed in the TELL variable or part
     of them are deleted or moved subsequently? That will just confuse the 
     TELL user. (Who the fuck is messing with my list?)
   - I can not think of a case when two or more separate lists of TOPED shapes,
     will be needed in the same time. We don't have TELL operation that works
     with more than one list (we don't have even with one yet ;)).
   - At the end of the day if for some reason somebody needs to keep certain
     list he should better call the TELL code that selects this list. It seems
     that simple.
   The old question how eventually TELL can benefit from the TOPED data base.
   The connection TELL - TOPED looks at the moment like a one way road, but
   this doen't seem to harm the abilities of the language.
   -----------------------------------------------------------------------------
   Then here are the rules:
   - There is ONE select list in TOPED and in TELL. That will be internal TELL 
     variable(the first one). We should think about a convention for internal
     variables - $selected is the first idea. The variable will be updated 
     automatically as long as it will point to the list of the selected 
     components in the active cell (oops - here it is the way to maintain more 
     than one list - per cell basis!)
   - The only placeholder type is quadTree. This means that ref/arefs should be
     placed in a quadTree instead of a C++ standard container  
   - No tdtdata is deleted during the session. Delete operation will move
     the shape in a different placeholder (Attic), or alternatively will mark it
     as deleted. This is to make undo operations possible. It seems appropriate,
     every cell to have an attic where the rubbish will be stored.
   - Select list contains a pointer to the lowest quadTree placeholder that
     contains seleted shapes.
   - tdtdata contains a field that will indicate the status of the shape - 
     selected or not. The same field might be used to indicate the shape is 
     deleted or any other status. This adds a byte to the every single component.
     It seems however that it will pay-back when drawing a large amount of 
     selected data. As usual - speed first!
   - The select list is invalidated with every cell change. The new shape might
     be selected.
   -----------------------------------------------------------------------------
   Now when already the only placeholder is quadTree ...
   -----------------------------------------------------------------------------
   Having a possibility to keep the list of components in TELL still seems
   quite tempting. It still seems that this might speed-up a lot of TELL 
   operations, although it is not clear at the moment how exactly. In the same 
   time all written above about the possible mess is absolutely true. 
   It looks however that there is bright way out of the situation so that 
   "the wolf is happy and the sheep is alive". Here it is:
   - All of the above rules are still in place EXCEPT the first one
   - TOPED has still ONE active list of selected components. It is stored in 
     the active cell structure and can be obtained from TELL using the internal
     variable $seleted(or similar).
   - All select operatoins return a list of ttlayout that can be stored in
     a TELL variable of the same type. It is not possible to use this variables 
     directly as a parameter for any modification operations (copy/move/delete
     etc.)
   - All modification related operations work with the TOPED list of selected
     components
   - The TELL component list variables will be used mainly as a parameter of a
     dedicated select function, so that the stored lists can be reselected later.
   The bottom line is: There is always ONE active TOPED list of selected shapes.
   TELL has an oportunity to reselect a sertain list of shapes using a dedicated
   select function. All modifications are executed over the current TOPED list.
   TOPED list of selected components is invalidated after each cell change.
   Thus the mess with the multiply selected lists seems to be sorted. 
   
*/

/*===========================================================================
                        Toped DaTa (TDT) databse structure
=============================================================================

                           tdtcell
tdtlayer tdtlayer etc.
                      
quadTree                   quadTree
                           
shapes                     tdtcellref/tdtaref

*/

//-----------------------------------------------------------------------------
// class tdtdata
//-----------------------------------------------------------------------------
GLvoid laydata::tdtdata::polyVertex (GLvoid *point) {
   TP *pnt = (TP *) point;
   glVertex2i(pnt->x(), pnt->y());
}

bool laydata::tdtdata::point_inside(const TP pnt) {
   DBbox ovl = overlap();ovl.normalize();
   if (ovl.inside(pnt)) return true;
   else return false;
}

void laydata::tdtdata::select_inBox(DBbox& select_in, dataList* selist, bool pselect) {
   // if shape is already fully selected, nothing to do here
   if (sh_selected == _status) return;
   float clip;
   // get the clip area and if it is 0 - run away
   if (0 == (clip = select_in.cliparea(overlap()))) return;
   if (-1 == clip) {// entire shape is in
      if (sh_partsel == _status) {
         // if the shape has already been partially selected
         for (dataList::iterator SI = selist->begin(); SI != selist->end(); SI++)
         // find it in the select list and remove the list of selected points
            if (SI->first == this) {
               delete (SI->second); SI->second = NULL;
               break;
            }
      }      
      else
         // otherwise - simply add it to the list
         selist->push_back(selectDataPair(this,NULL));
      _status = sh_selected; 
   }   
   else if ((clip > 0) && pselect) { // shape partially is in the select box
      SGBitSet* pntlst = NULL;
      if (sh_partsel == _status) {
      // if the shape has already been partially selected
         dataList::iterator SI;
         for (SI = selist->begin(); SI != selist->end(); SI++)
            // get the pointlist
            if (SI->first == this) { pntlst = SI->second; break; }
         assert(pntlst);   
         // select some more points using shape specific procedures
         select_points(select_in, pntlst);
         // check that after the selection shape doesn't end up fully selected
         if (pntlst->isallset()) {
            _status = sh_selected;
            delete pntlst; SI->second = NULL;
            selist->push_back(selectDataPair(this, pntlst));
         }
      }      
      else {
         // otherwise create a new pointlist
         pntlst = new SGBitSet(numpoints());
         // select some more points using shape specific procedures
         select_points(select_in, pntlst);
         // and check 
         if (!pntlst->isallclear()) {
            _status = sh_partsel;
            selist->push_back(selectDataPair(this, pntlst));
         }
         else delete pntlst;
      }   
   }   
}   

bool  laydata::tdtdata::unselect(DBbox& select_in, selectDataPair& SI, bool pselect) {
   // check the integrity of the select list
   assert((sh_selected == _status) || (sh_partsel == _status));
   float clip;
   // get the clip area and if it is 0 - run away
   if (0 == (clip = select_in.cliparea(overlap()))) return false;
   // if select_in overlaps the entire shape
   if (-1 == clip) {
      if (SI.second) {
         //remove the list of selected points if it exists ...
         delete (SI.second); SI.second = NULL;
      }
      _status = sh_active;
      return true;// i.e. remove this from the list of selected shapes
   }   
   // if select_in intersects with the overlapping box
   else if ((clip > 0) && pselect) {
      SGBitSet* pntlst;
      // get the pointlist
      if (sh_partsel == _status) // if the shape is already partially selected
         pntlst = SI.second;
      else // otherwise (sh_selected) create a new pointlist
         SI.second = pntlst = new SGBitSet(numpoints());
      // finally - go and unselect some points   
      unselect_points(select_in, pntlst);
      if (pntlst->isallclear()) {//none selected
         _status = sh_active; delete pntlst;SI.second = NULL; return true;
      }
      else if (pntlst->isallset()) { //all points selected; 
         _status = sh_selected; delete pntlst; SI.second = NULL; return false;
      }   
      else { // some selected
         _status = sh_partsel; return false;
      }
   }
   // if pselect is false and the shape is not fully overlapped by 
   // the select_in - do nothing
   return false;
}

//-----------------------------------------------------------------------------
// class tdtbox
//-----------------------------------------------------------------------------
laydata::tdtbox::tdtbox(TEDfile* const tedfile) : tdtdata() {
   _p1 = new TP(tedfile->getTP()); if (!tedfile->status()) return;
   _p2 = new TP(tedfile->getTP()); if (!tedfile->status()) return;
   normalize();
}

void laydata::tdtbox::normalize() {
   int4b swap;
   if (_p1->x() > _p2->x()) { swap = _p1->x(); _p1->setX(_p2->x()); _p2->setX(swap);}
   if (_p1->y() > _p2->y()) { swap = _p1->y(); _p1->setY(_p2->y()); _p2->setY(swap);}
}   

void laydata::tdtbox::openGL_draw(ctmstack& transtack, 
                                          const layprop::DrawProperties& drawprop) const {
   if (!((_p1) && (_p2))) return;
   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   TP pt1 = (*_p1) * transtack.top();
   TP pt2 = (*_p2) * transtack.top();
   glRecti(pt1.x(),pt1.y(),pt2.x(),pt2.y());
   if (drawprop.getCurrentFill()) {
      glRecti(pt1.x(),pt1.y(),pt2.x(),pt2.y());
      glDisable(GL_POLYGON_STIPPLE);
   }
}

void laydata::tdtbox::tmp_draw(const layprop::DrawProperties&, ctmqueue& transtack,
                              SGBitSet* plst, bool under_construct) const {
   CTM trans = transtack.front();
   if (!(_p1)) return;
   else if (under_construct) {
      TP pt2 = (*_p1) * trans;
      glRecti(_p1->x(),_p1->y(),pt2.x(),pt2.y());
   }
   else if (_p2){
      TP pt1, pt2;
      if (sh_partsel == status()) {      
         CTM strans = transtack.back();
         assert(plst);
         pointlist nshape = movePointsSelected(plst, trans, strans);
         pt1 = nshape[0]; pt2 = nshape[2];
      }
      else {
         pt1 = (*_p1) * trans;
         pt2 = (*_p2) * trans;
      }   
      glRecti(pt1.x(),pt1.y(),pt2.x(),pt2.y());
   }
}

void  laydata::tdtbox::draw_select(CTM trans,const SGBitSet* pslist) const {
   if       (sh_selected == status()) draw_select_marks(overlap() * trans);
   else if (  sh_partsel == status()) {
      assert(pslist);
      if (pslist->check(0)) draw_select_mark(*_p1 * trans);
      if (pslist->check(1)) draw_select_mark(TP(_p2->x(), _p1->y()) * trans);
      if (pslist->check(2)) draw_select_mark(*_p2 * trans);
      if (pslist->check(3)) draw_select_mark(TP(_p1->x(), _p2->y()) * trans);
   }   
}

void  laydata::tdtbox::select_points(DBbox& select_in, SGBitSet* pntlst) {
   if (select_in.inside(*_p1))  pntlst->set(0);
   if (select_in.inside(TP(_p2->x(), _p1->y())))  pntlst->set(1);
   if (select_in.inside(*_p2))  pntlst->set(2);
   if (select_in.inside(TP(_p1->x(), _p2->y())))  pntlst->set(3);
   pntlst->check_neighbours_set(false);   
}

void  laydata::tdtbox::unselect_points(DBbox& select_in, SGBitSet* pntlst) {
   if (sh_selected == _status) pntlst->setall();
   if (select_in.inside(*_p1))  pntlst->reset(0);
   if (select_in.inside(TP(_p2->x(), _p1->y())))  pntlst->reset(1);
   if (select_in.inside(*_p2))  pntlst->reset(3);
   if (select_in.inside(TP(_p1->x(), _p2->y())))  pntlst->reset(3);
}

laydata::validator* laydata::tdtbox::move(const CTM& trans, const SGBitSet* plst) {
   if (plst) {
      pointlist nshape = movePointsSelected(plst, trans);
      *_p1 = nshape[0]; *_p2 = nshape[2];
      normalize();
   }
   else transfer(trans);
   return NULL;
}

void laydata::tdtbox::transfer(const CTM& trans) {
   *_p1 *= trans;
   *_p2 *= trans;
}

laydata::tdtdata* laydata::tdtbox::copy(const CTM& trans) {
   TP *cp1 = new TP(*_p1 * trans);
   TP *cp2 = new TP(*_p2 * trans);
   return new tdtbox(cp1, cp2);
}

void laydata::tdtbox::info(std::ostringstream& ost) const {
   ost << "box at (";
   _p1->info(ost);
   ost << " , ";
   _p2->info(ost);
   ost << ");";
}

void laydata::tdtbox::write(TEDfile* const tedfile) const {
   tedfile->putByte(tedf_BOX);
   tedfile->putTP(_p1);
   tedfile->putTP(_p2);
}

void laydata::tdtbox::GDSwrite(GDSin::GDSFile& gdsf, word lay) const
{
   GDSin::GDSrecord* wr = gdsf.SetNextRecord(gds_BOUNDARY);
   gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_LAYER);
   wr->add_int2b(lay);gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_DATATYPE);
   wr->add_int2b(0);gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_XY,5);
   wr->add_int4b(_p1->x());wr->add_int4b(_p1->y());
   wr->add_int4b(_p1->x());wr->add_int4b(_p2->y());
   wr->add_int4b(_p2->x());wr->add_int4b(_p2->y());
   wr->add_int4b(_p2->x());wr->add_int4b(_p1->y());
   wr->add_int4b(_p1->x());wr->add_int4b(_p1->y());
   gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_ENDEL);
   gdsf.flush(wr);
}

DBbox laydata::tdtbox::overlap() const {
   DBbox ovl = DBbox(*_p1);
   ovl.overlap(*_p2);
   return ovl;
}

void  laydata::tdtbox::addpoint(TP p) {
   if (!_p1) _p1 = new TP(p);
   else {
      if (_p2) delete _p2; // This line seems to be redundant
      _p2 = new TP(p);
   }
}

void  laydata::tdtbox::rmpoint(TP& lp) {
   if (NULL != _p2) {delete _p2; _p2 = NULL;}
   if (NULL != _p1) {delete _p1; _p1 = NULL;}
};

const pointlist laydata::tdtbox::shape2poly() {
  // convert box to polygon
   pointlist _plist;
   _plist.push_back(TP(*_p1));_plist.push_back(TP(_p2->x(), _p1->y()));
   _plist.push_back(TP(*_p2));_plist.push_back(TP(_p1->x(), _p2->y()));
   return _plist;
};

void laydata::tdtbox::polycut(pointlist& cutter, shapeList** decure) {
   pointlist _plist = shape2poly();
   // and proceed in the same was as for the polygon
   logicop::pcollection cut_shapes;
   logicop::logic operation(_plist, cutter);
   laydata::tdtdata* newshape;
   if (operation.AND(cut_shapes)) {
      logicop::pcollection::const_iterator CI;
      // add the resulting cut_shapes to the_cut shapeList
      for (CI = cut_shapes.begin(); CI != cut_shapes.end(); CI++) 
         if (NULL != (newshape = createValidShape(**CI)))
            decure[1]->push_back(newshape);
      // if there is a cut - there will be (most likely) be cut remains as well
      operation.reset_visited();
      logicop::pcollection rest_shapes;
      if (operation.ANDNOT(rest_shapes))
         // add the resulting cut remainings to the_rest shapeList
         for (CI = rest_shapes.begin(); CI != rest_shapes.end(); CI++) 
            if (NULL != (newshape = createValidShape(**CI)))
               decure[2]->push_back(newshape);
      // and finally add this to the_delete shapelist
      decure[0]->push_back(this);
   }
}

pointlist& laydata::tdtbox::movePointsSelected(const SGBitSet* pset, 
                                    const CTM&  movedM, const CTM& stableM) const {
  // convert box to polygon
   pointlist* mlist = new pointlist();
   mlist->push_back(TP(*_p1));mlist->push_back(TP(_p2->x(), _p1->y()));
   mlist->push_back(TP(*_p2));mlist->push_back(TP(_p1->x(), _p2->y()));

   word size = mlist->size();
   PSegment seg1,seg0;
   // Things to remember in this algo...
   // Each of the points in the initial mlist is recalculated in the seg1.crossP
   // method. This actually means that on pass 0 (i == 0), no points are
   // recalculated because seg0 at that moment is empty. On pass 1 (i == 1),
   // point mlist[1] is recalculated etc. The catch comes on the last pass
   // (i == size) when constructing the seg1, we need mlist[0] and mlist[1], but
   // mlist[1] has been already recalculated and multiplying it with CTM 
   // matrix again has pretty funny effect.
   // That's why another condition is introduced -> if (i == size)
   for (unsigned i = 0; i <= size; i++) {
      if (i == size) 
         if (pset->check(i%size) && pset->check((i+1) % size))
            seg1 = PSegment((*mlist)[(i  ) % size] * movedM,
                            (*mlist)[(i+1) % size]         );
         else
            seg1 = PSegment((*mlist)[(i  ) % size] * stableM,
                            (*mlist)[(i+1) % size]          );
      else
         if (pset->check(i%size) && pset->check((i+1) % size))
            seg1 = PSegment((*mlist)[(i  ) % size] * movedM, 
                            (*mlist)[(i+1) % size] * movedM);
         else
            seg1 = PSegment((*mlist)[(i  ) % size] * stableM, 
                            (*mlist)[(i+1) % size] * stableM);
      if (!seg0.empty()) {
         seg1.crossP(seg0,(*mlist)[i%size]);
      }
      seg0 = seg1;
   }
   return (*mlist);
}

laydata::tdtbox::~tdtbox() {
   if (_p1) delete _p1; 
   if (_p2) delete _p2;
}

//-----------------------------------------------------------------------------
// class tdtpoly
//-----------------------------------------------------------------------------
laydata::tdtpoly::tdtpoly(TEDfile* const tedfile) : tdtdata(){
   word numpoints = tedfile->getWord(); if (!tedfile->status()) return;
   _plist.reserve(numpoints);
   for (word i = 0 ; i < numpoints; i++) {
      _plist.push_back(tedfile->getTP()); if (!tedfile->status()) return;
   }
}

void laydata::tdtpoly::openGL_draw(ctmstack& transtack, 
                                 const layprop::DrawProperties& drawprop) const {
   word i;
   pointlist ptlist;
   // translate the points using the current CTM
   ptlist.reserve(_plist.size());
   for (i = 0; i < _plist.size(); i++) 
      ptlist.push_back(_plist[i] * transtack.top());
   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   glBegin(GL_LINE_LOOP);
   for (i = 0; i < ptlist.size(); i++) 
      glVertex2i(ptlist[i].x(), ptlist[i].y());
   glEnd();
   if (drawprop.getCurrentFill()) {
      // Start tessellation
      gluTessBeginPolygon(tessellObj, NULL);
      GLdouble pv[3]; 
      pv[2] = 0;
      for (i = 0; i < ptlist.size(); i++) {
         pv[0] = ptlist[i].x(); pv[1] = ptlist[i].y();
         gluTessVertex(tessellObj,pv,&ptlist[i]);
      }
      gluTessEndPolygon(tessellObj);
      // End tesselation
      glDisable(GL_POLYGON_STIPPLE);
   }
}

void laydata::tdtpoly::tmp_draw(const layprop::DrawProperties&, ctmqueue& transtack,
                                 SGBitSet* plst, bool under_construct) const {
   CTM trans = transtack.front();
   _dbl_word numpnts;
//   trans = transtack.top();
   if ((numpnts = _plist.size()) == 0) return;
   word i;
   if (under_construct) {
      glBegin(GL_LINE_STRIP);
      for (i = 0; i < numpnts; i++) 
         glVertex2i(_plist[i].x(), _plist[i].y());
      TP newp = _plist[numpnts-1] * trans;
      glVertex2i(newp.x(), newp.y());
      if ((numpnts > 2) || ((2 == numpnts) && (newp != _plist[numpnts-1])))
         glVertex2i(_plist[0].x(), _plist[0].y());
      glEnd();
   }
   else {
      pointlist ptlist;
      if (sh_partsel == status()) {
         CTM strans = transtack.back();
         assert(plst);
         ptlist = movePointsSelected(plst, trans, strans);
      }
      else {
         ptlist.reserve(numpnts);
         for (i = 0; i < numpnts; i++) 
            ptlist.push_back(_plist[i] * trans);
      }
      glBegin(GL_LINE_LOOP);
      for (i = 0; i < numpnts; i++) 
         glVertex2i(ptlist[i].x(), ptlist[i].y());
      glEnd();
      ptlist.clear();
   }
}

void laydata::tdtpoly::rmpoint(TP& lp) {
   assert(_plist.size() > 0);
   _plist.pop_back();
   if (_plist.size() > 0) lp = _plist.back();
};

void  laydata::tdtpoly::draw_select(CTM trans, const SGBitSet* pslist) const {
   if (sh_selected == status())
      for (word i = 0; i < _plist.size(); i++)
         draw_select_mark(_plist[i] * trans);
   else if (sh_partsel == status()) {
      assert(pslist);
      for (word i = 0; i < _plist.size(); i++)
         if (pslist->check(i)) draw_select_mark(_plist[i] * trans);
   }
}

void  laydata::tdtpoly::select_points(DBbox& select_in, SGBitSet* pntlst) {
   for (word i = 0; i < _plist.size(); i++) 
      if (select_in.inside(_plist[i])) pntlst->set(i);
   pntlst->check_neighbours_set(false);   
}

void laydata::tdtpoly::unselect_points(DBbox& select_in, SGBitSet* pntlst) {
   if (sh_selected == _status)  // the whole shape use to be selected
      pntlst->setall();
   for (word i = 0; i < _plist.size(); i++) 
      if (select_in.inside(_plist[i])) pntlst->reset(i);
   pntlst->check_neighbours_set(false);   
}

laydata::validator* laydata::tdtpoly::move(const CTM& trans, const SGBitSet* plst) {
   if (plst) {
      pointlist& nshape = movePointsSelected(plst, trans);
      laydata::valid_poly* check = new laydata::valid_poly(nshape);
      if (laydata::shp_OK == check->status()) {
         // assign the modified pointlist ONLY if the resulting shape is perfect
         _plist.clear(); _plist = nshape;
      }
      // in all other cases keep the origical pointlist, depending on the check->status()
      // the shape will be replaced, or marked as failed to modify
      return check;
   }
   else transfer(trans);
   return NULL;
}

void laydata::tdtpoly::transfer(const CTM& trans) {
   for (unsigned i = 0; i < _plist.size(); i++) 
      _plist[i] *= trans;
}

laydata::tdtdata* laydata::tdtpoly::copy(const CTM& trans) {
   // copy the points of the polygon
   pointlist ptlist;
   ptlist.reserve(_plist.size());
   for (unsigned i = 0; i < _plist.size(); i++) 
      ptlist.push_back(_plist[i] * trans);
   return new tdtpoly(ptlist);
}

bool laydata::tdtpoly::point_inside(TP pnt) {
   TP p0, p1;
   byte cc = 0;
   unsigned size = _plist.size();
   for (unsigned i = 0; i < size ; i++) {
      p0 = _plist[i]; p1 = _plist[(i+1) % size];
      if (((p0.y() <= pnt.y()) && (p1.y() >  pnt.y())) 
        ||((p0.y() >  pnt.y()) && (p1.y() <= pnt.y())) ) {
         float tngns = (float) (pnt.y() - p0.y())/(p1.y() - p0.y());
         if (pnt.x() < p0.x() + tngns * (p1.x() - p0.x()))
            cc++;
      }
   }
   return (cc & 0x01) ? true : false;
}

void laydata::tdtpoly::polycut(pointlist& cutter, shapeList** decure) {
   logicop::pcollection cut_shapes;
   logicop::logic operation(_plist, cutter);
   laydata::tdtdata* newshape;
   if (operation.AND(cut_shapes)) {
      logicop::pcollection::const_iterator CI;
      // add the resulting cut_shapes to the_cut shapeList
      for (CI = cut_shapes.begin(); CI != cut_shapes.end(); CI++)
         if (NULL != (newshape = createValidShape(**CI)))
            decure[1]->push_back(newshape);
      // if there is a cut - there should be cut remains as well
      operation.reset_visited();
      logicop::pcollection rest_shapes;
      if (operation.ANDNOT(rest_shapes))
         // add the resulting cut remainings to the_rest shapeList
         for (CI = rest_shapes.begin(); CI != rest_shapes.end(); CI++) 
            if (NULL != (newshape = createValidShape(**CI)))
               decure[2]->push_back(newshape);
      // and finally add this to the_delete shapelist
      decure[0]->push_back(this);
   }
}

void laydata::tdtpoly::info(std::ostringstream& ost) const {
   ost << "polygon at (";
   unsigned lstsize = _plist.size();
   unsigned lastpnt = lstsize-1;
   for (unsigned i = 0; i < lstsize; i++) {
      _plist[i].info(ost);
      if (i != lastpnt) ost << " , ";
   }
   ost << ");";
}

void laydata::tdtpoly::write(TEDfile* const tedfile) const {
   tedfile->putByte(tedf_POLY);
   tedfile->putWord(_plist.size());
   for (word i = 0; i < _plist.size(); i++)
      tedfile->putTP(&_plist[i]);
}

void laydata::tdtpoly::GDSwrite(GDSin::GDSFile& gdsf, word lay) const
{
   GDSin::GDSrecord* wr = gdsf.SetNextRecord(gds_BOUNDARY);
   gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_LAYER);
   wr->add_int2b(lay);gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_DATATYPE);
   wr->add_int2b(0);gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_XY,_plist.size()+1);
   for (word i = 0; i < _plist.size(); i++)
   {
      wr->add_int4b(_plist[i].x());wr->add_int4b(_plist[i].y());
   }
   wr->add_int4b(_plist[0].x());wr->add_int4b(_plist[0].y());
   gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_ENDEL);
   gdsf.flush(wr);
}

DBbox laydata::tdtpoly::overlap() const {
   DBbox ovl = DBbox(_plist[0]);
   for (word i = 1; i < _plist.size(); i++)
      ovl.overlap(_plist[i]);
   return ovl;
}

pointlist& laydata::tdtpoly::movePointsSelected(const SGBitSet* pset, 
                                    const CTM&  movedM, const CTM& stableM) const {
   pointlist* mlist = new pointlist(_plist);
   word size = mlist->size();
   PSegment seg1,seg0;
   // See the note about this algo in tdtbox::movePointsSelected above
   for (unsigned i = 0; i <= size; i++) {
      if (i == size)
         if (pset->check(i%size) && pset->check((i+1) % size))
            seg1 = PSegment((*mlist)[(i  ) % size] * movedM,
                            (*mlist)[(i+1) % size]         );
         else
            seg1 = PSegment((*mlist)[(i  ) % size] * stableM,
                            (*mlist)[(i+1) % size]          );
      else   
         if (pset->check(i%size) && pset->check((i+1) % size))
            seg1 = PSegment((*mlist)[(i  ) % size] * movedM, 
                            (*mlist)[(i+1) % size] * movedM);
         else
            seg1 = PSegment((*mlist)[(i  ) % size] * stableM, 
                            (*mlist)[(i+1) % size] * stableM);
      if (!seg0.empty()) {
         seg1.crossP(seg0,(*mlist)[i%size]);
      }
      seg0 = seg1;
   }
   return (*mlist);
}

//-----------------------------------------------------------------------------
// class tdtwire
//-----------------------------------------------------------------------------
laydata::tdtwire::tdtwire(TEDfile* const tedfile) : tdtdata() {
   word numpoints = tedfile->getWord();  if (!tedfile->status()) return;
   _width = tedfile->getWord();  if (!tedfile->status()) return;
   _plist.reserve(numpoints);
   for (word i = 0 ; i < numpoints; i++) {
      _plist.push_back(tedfile->getTP()); if (!tedfile->status()) return;
   }   
}

DBbox* laydata::tdtwire::endPnts(const TP& p1, const TP& p2, bool first) const {
   double     w = _width/2;
   double denom = p2.x() - p1.x();
   double   nom = p2.y() - p1.y();
   double signX = (nom   > 0) ? 1.0 : -1.0;
   double signY = (denom > 0) ? 1.0 : -1.0;
   TP pt = first ? p1 : p2;
   double xcorr, ycorr; // the corrections
   if ((0 == nom) && (0 == denom)) return NULL;
   else if (0 == denom)   {xcorr = signX*w; ycorr = 0;} // vertical
   else if (0 == nom  )   {xcorr = 0; ycorr = signY*w;} // horizontal
   else {
      double sl   = nom / denom;
      double sqsl = sqrt( sl*sl + 1);
      xcorr = rint(w * (sl / sqsl)); 
      ycorr = rint(w * ( 1 / sqsl));
   }
   return new DBbox((int4b) rint(pt.x() - xcorr), (int4b) rint(pt.y() + ycorr),
                     (int4b) rint(pt.x() + xcorr), (int4b) rint(pt.y() - ycorr));
}

DBbox* laydata::tdtwire::mdlPnts(const TP& p1, const TP& p2, const TP& p3) const {
   double    w = _width/2;
   double  x32 = p3.x() - p2.x();
   double  x21 = p2.x() - p1.x();
   double  y32 = p3.y() - p2.y();
   double  y21 = p2.y() - p1.y();
   double   L1 = sqrt(x21*x21 + y21*y21); //the length of the segment 1
   double   L2 = sqrt(x32*x32 + y32*y32); //the length of the segment 2
   // the corrections
   double denom = x32 * y21 - x21 * y32;
//SGREM !!! THINK about next two lines!!!    They are wrong !!!
   if ((0 == denom) || (0 == L1)) return endPnts(p2,p3,false);
   if (0 == L2) return NULL;
   double xcorr = w * ((x32 * L1 - x21 * L2) / denom);
   double ycorr = w * ((y21 * L2 - y32 * L1) / denom);
   return new DBbox((int4b) rint(p2.x() - xcorr), (int4b) rint(p2.y() + ycorr),
                     (int4b) rint(p2.x() + xcorr), (int4b) rint(p2.y() - ycorr));
}

void laydata::tdtwire::drawSegment(const layprop::DrawProperties& drawprop, const TP& p1,
      const TP& p2, const TP& p3,const TP& p4, bool begin, bool end) const {
   glBegin(GL_LINES);
   glVertex2i(p2.x(), p2.y()); glVertex2i(p3.x(), p3.y());
   glVertex2i(p4.x(), p4.y()); glVertex2i(p1.x(), p1.y());
   if (begin) { // is it the first or the only segment ?
      glVertex2i(p1.x(), p1.y()); glVertex2i(p2.x(), p2.y());
   }
   if (end) { // is it the last or the only segment
      glVertex2i(p3.x(), p3.y()); glVertex2i(p4.x(), p4.y());
   }
   glEnd();
   if (drawprop.getCurrentFill()) {
      glBegin(GL_POLYGON);
      glVertex2i(p1.x(), p1.y()); glVertex2i(p2.x(), p2.y());
      glVertex2i(p3.x(), p3.y()); glVertex2i(p4.x(), p4.y());
      glEnd();
      glDisable(GL_POLYGON_STIPPLE);
   }
}

void laydata::tdtwire::openGL_draw(ctmstack& transtack, 
                                 const layprop::DrawProperties& drawprop) const {
   unsigned i;
   pointlist ptlist;
   if (_plist.size() < 2) return;
   // translate the points using the current CTM
   ptlist.reserve(_plist.size());
   for (i = 0; i < _plist.size(); i++) 
      ptlist.push_back(_plist[i] * transtack.top());
   // now check whether to draw only the center line
   DBbox wsquare = DBbox(TP(0,0),TP(_width,_width)) * transtack.top(); 
   wsquare = wsquare * drawprop.ScrCTM();
   if (wsquare.area() > MIN_VISUAL_AREA) {
      DBbox* ln1 = endPnts(ptlist[0],ptlist[1], true);
      DBbox* ln2;
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      for (i = 1; i < ptlist.size() - 1; i++) {
         ln2 = mdlPnts(ptlist[i-1],ptlist[i],ptlist[i+1]);
         if ((NULL != ln1) && (NULL != ln2))
            drawSegment(drawprop, 
                        ln1->p1(), ln1->p2(), ln2->p2(), ln2->p1(), 1 == i,false);
         if (ln1) delete ln1; 
         ln1 = ln2;
      }
      ln2 = endPnts(ptlist[i-1],ptlist[i],false);
      if ((NULL != ln1) && (NULL != ln2))
         drawSegment(drawprop, 
                         ln1->p1(), ln1->p2(), ln2->p2(), ln2->p1(), 1 == i,true);
      if (ln1) delete ln1;  if (ln2) delete ln2;
   }   
   // draw the central line
   glBegin(GL_LINE_STRIP);
   for (i = 0; i < ptlist.size(); i++) glVertex2i(ptlist[i].x(), ptlist[i].y());
   glEnd();
}

void laydata::tdtwire::tmp_draw(const layprop::DrawProperties& drawprop,
               ctmqueue& transtack, SGBitSet* plst, bool under_construct) const {
   CTM trans = transtack.front();
   unsigned i;
   pointlist ptlist;
   _dbl_word numpnts = _plist.size();
   // translate the points using the current CTM
   if (under_construct) {
      if (numpnts == 0) return;
      ptlist.reserve(numpnts+1);
      for (i = 0; i < numpnts; i++) 
         ptlist.push_back(_plist[i]);
      TP npnt = _plist[numpnts-1] * trans;
      if (npnt != _plist[numpnts-1]) 
         ptlist.push_back(npnt);
      else if (1 == numpnts) return;
   }
   else {   
      if (numpnts < 2) return;
      ptlist.reserve(numpnts);
      if (sh_partsel == status()) {
         CTM strans = transtack.back();
         assert(plst);
         ptlist = movePointsSelected(plst, trans, strans);
      }
      else
         for (i = 0; i < numpnts; i++) 
            ptlist.push_back(_plist[i] * trans);
   }      
   DBbox* ln1 = endPnts(ptlist[0],ptlist[1], true);
   DBbox* ln2;
   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   for (i = 1; i < ptlist.size() - 1; i++) {
      ln2 = mdlPnts(ptlist[i-1],ptlist[i],ptlist[i+1]);
      if ((NULL != ln1) && (NULL != ln2))
         drawSegment(drawprop, ln1->p1(), ln1->p2(), ln2->p2(), ln2->p1(), 1 == i,false);
      if (ln1) delete ln1; 
      ln1 = ln2;
   }
   ln2 = endPnts(ptlist[i-1],ptlist[i],false);
   if ((NULL != ln1) && (NULL != ln2))
      drawSegment(drawprop, ln1->p1(), ln1->p2(), ln2->p2(), ln2->p1(), 1 == i,true);
   if (ln1) delete ln1;  if (ln2) delete ln2;
   // draw the central line
   glBegin(GL_LINE_STRIP);
   for (i = 0; i < ptlist.size(); i++) glVertex2i(ptlist[i].x(), ptlist[i].y());
   glEnd();
   ptlist.clear();
}

void  laydata::tdtwire::rmpoint(TP& lp) {
   assert(_plist.size() > 0);
   _plist.pop_back();
   if (_plist.size() > 0) lp = _plist.back();
};


void  laydata::tdtwire::draw_select(CTM trans, const SGBitSet* pslist) const {
   if (sh_selected == status())
      for (word i = 0; i < _plist.size(); i++)
         draw_select_mark(_plist[i] * trans);
   else if (sh_partsel == status()) {
      assert(pslist); 
      for (word i = 0; i < _plist.size(); i++)
         if (pslist->check(i)) draw_select_mark(_plist[i] * trans);
   }      
}

bool laydata::tdtwire::point_inside(TP pnt) {
   TP p0, p1;
   for (unsigned i = 0; i < _plist.size() - 1 ; i++) {
      p0 = _plist[i]; p1 = _plist[i+1];
      float distance = get_distance(p0,p1,pnt);
      if ((distance > 0) && (distance < _width/2))
         return true;
   }
   return false;
}

float laydata::tdtwire::get_distance(TP p1, TP p2, TP p0) {
   if (p1.x() == p2.x())
      // if the segment is parallel to Y axis
      if ( ((p0.y() >= p1.y()) && (p0.y() <= p2.y())) 
         ||((p0.y() <= p1.y()) && (p0.y() >= p2.y())) )
         return fabsf(p0.x() - p1.x());
      else return -1;
   else if (p1.y() == p2.y())
      // if the segment is parallel to X axis
      if ( ((p0.x() >= p1.x()) && (p0.x() <= p2.x())) 
         ||((p0.x() <= p1.x()) && (p0.x() >= p2.x())) )
         return fabsf(p0.y() - p1.y());
      else return -1;
   else {
      // segment is not parallel to any axis
      float A = p2.y() - p1.y();
      float B = p1.x() - p2.x();
      float C = - (p1.y() * B) - (p1.x() * A);
      float dist = A*A + B*B;
      float Cn = A*p0.x() + B*p0.y() + C;
      float X = p0.x() - (A / dist) * Cn;
      float Y = p0.y() - (B / dist) * Cn;
      // now check that the new coordinate is on the p1-p2 line
      if ((((Y >= p1.y()) && (Y <= p2.y()))||((Y <= p1.y()) && (Y >= p2.y()))) &&
          (((X >= p1.x()) && (X <= p2.x()))||((X <= p1.x()) && (X >= p2.x())))   )
         return fabsf(Cn / sqrt(dist));
      else return -1;          
   }
}

void  laydata::tdtwire::select_points(DBbox& select_in, SGBitSet* pntlst) {
   for (word i = 0; i < _plist.size(); i++) 
      if (select_in.inside(_plist[i])) pntlst->set(i);
   pntlst->check_neighbours_set(true);   
 }

void laydata::tdtwire::unselect_points(DBbox& select_in, SGBitSet* pntlst) {
   if (sh_selected == _status) // the whole shape use to be selected
      pntlst->setall();      
   for (word i = 0; i < _plist.size(); i++) 
      if (select_in.inside(_plist[i])) pntlst->reset(i);
   pntlst->check_neighbours_set(true);   
}

laydata::validator* laydata::tdtwire::move(const CTM& trans, const SGBitSet* plst) {
   if (plst) {
      pointlist nshape = movePointsSelected(plst, trans);
      laydata::valid_wire* check = new laydata::valid_wire(nshape, _width);
      if (laydata::shp_OK == check->status()) {
         // assign the modified pointlist ONLY if the resulting shape is perfect
         _plist.clear(); _plist = nshape;
      }
      // in all other cases keep the origical pointlist, depending on the check->status()
      // the shape will be replaced, or marked as failed to modify
      return check;
   }
   else transfer(trans);
   return NULL;
}

void laydata::tdtwire::transfer(const CTM& trans) {
   for (unsigned i = 0; i < _plist.size(); i++) 
      _plist[i] *= trans;
}

laydata::tdtdata* laydata::tdtwire::copy(const CTM& trans) {
   // copy the points of the wire
   pointlist ptlist;
   ptlist.reserve(_plist.size());
   for (unsigned i = 0; i < _plist.size(); i++) 
      ptlist.push_back(_plist[i] * trans);
   return new tdtwire(ptlist,_width);
}

void laydata::tdtwire::info(std::ostringstream& ost) const {
   ost << "wire #" << _width << " wide at (";
   unsigned lstsize = _plist.size();
   unsigned lastpnt = lstsize-1;
   for (unsigned i = 0; i < lstsize; i++) {
      _plist[i].info(ost);
      if (i != lastpnt) ost << " , ";
   }
   ost << ");";
}

void laydata::tdtwire::write(TEDfile* const tedfile) const {
   tedfile->putByte(tedf_WIRE);
   tedfile->putWord(_plist.size());
   tedfile->putWord(_width);
   for (word i = 0; i < _plist.size(); i++)
      tedfile->putTP(&_plist[i]);
}

void laydata::tdtwire::GDSwrite(GDSin::GDSFile& gdsf, word lay) const
{
   GDSin::GDSrecord* wr = gdsf.SetNextRecord(gds_PATH);
   gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_LAYER);
   wr->add_int2b(lay);gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_DATATYPE);
   wr->add_int2b(0);gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_WIDTH);
   wr->add_int4b(_width);gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_XY,_plist.size()+1);
   for (word i = 0; i < _plist.size(); i++)
   {
      wr->add_int4b(_plist[i].x());wr->add_int4b(_plist[i].y());
   }
   wr->add_int4b(_plist[0].x());wr->add_int4b(_plist[0].y());
   gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_ENDEL);
   gdsf.flush(wr);
}

DBbox laydata::tdtwire::overlap() const {
   DBbox ovl = DBbox(_plist[0]);
   DBbox* ln1 = endPnts(_plist[0],_plist[1], true);
   DBbox* ln2;
   word i;
   for (i = 1; i < _plist.size() - 1; i++) {
      ln2 = mdlPnts(_plist[i-1],_plist[i],_plist[i+1]);
      if (ln1) {
         ovl.overlap(*ln1);
         delete ln1; 
      }
      ln1 = ln2;
   }
   ln2 = endPnts(_plist[i-1],_plist[i],false);
   if (ln2) {
      ovl.overlap(*ln2);
      delete ln2; 
   }
   return ovl;
}

pointlist& laydata::tdtwire::movePointsSelected(const SGBitSet* pset, 
                                    const CTM& movedM, const CTM& stableM) const {
   pointlist* mlist = new pointlist(_plist);
   word size = mlist->size();
   PSegment seg1,seg0;
   for (int i = 0; i < size; i++) {
      if ((size-1) == i) {
         if (pset->check(size-1))
            seg1 = seg1.ortho((*mlist)[size-1] * movedM);
         else
            seg1 = seg1.ortho((*mlist)[size-1] * stableM);
      }
      else {
         const CTM& transM = ((pset->check(i) && pset->check(i+1))) ? 
                                                               movedM : stableM;
         seg1 = PSegment((*mlist)[(i  )] * transM, (*mlist)[(i+1)] * transM);
         if (0 == i)
            if (pset->check(0))
               seg0 = seg1.ortho((*mlist)[i] * movedM);
            else
               seg0 = seg1.ortho((*mlist)[i] * stableM);
      }
      if (!seg0.empty()) seg1.crossP(seg0,(*mlist)[i]);
      seg0 = seg1;
   }
   return (*mlist);
}
//-----------------------------------------------------------------------------
// class tdtcellref
//-----------------------------------------------------------------------------
laydata::tdtcellref::tdtcellref(TEDfile* const tedfile) {
   // read the name of the referenced cell
   std::string cellrefname = tedfile->getString(); if (!tedfile->status()) return;
   // get the cell definition pointer and register the cellrefname as a child 
   // to the currently parsed cell
   _structure = tedfile->getcellinstance(cellrefname);
   // get the translation   
   _translation = tedfile->getCTM();//if (!tedfile->status()) return;
}

bool laydata::tdtcellref::ref_visible(ctmstack& transtack, const layprop::DrawProperties& drawprop) const {
   // Get the areal. What is important here - overlap() will return a zero-area box using the
   // reference point, so this should be catched from the MIN_VISUAL_AREA check
   // SGREM !! So, if a referenced structure doesn't exists, we'll draw nothing!!
   DBbox areal = overlap() * transtack.top();
   areal.normalize();
   // check that the cell (or part of it) is in the visual window
   DBbox clip = drawprop.clipRegion();
   if (clip.cliparea(areal) == 0) return false;
   // check that the cell area is bigger that the MIN_VISUAL_AREA
   DBbox minareal = areal * drawprop.ScrCTM();
   if (minareal.area() < MIN_VISUAL_AREA) return false;
   // If we get here - means that the cell (or part of it) is visible
   CTM newtrans = _translation * transtack.top();
   // draw the cell mark ...
   drawprop.draw_reference_marks(TP(0,0) * newtrans, layprop::cell_mark);
   // ... and the overlapping box
   draw_overlapping_box(areal, 0xf18f);
   // push the new translation in the stack
   transtack.push(newtrans);
   return true;
}

void laydata::tdtcellref::openGL_draw(ctmstack& transtack, const layprop::DrawProperties& drawprop) const {
   // first check that the entire cell has to be drawn
   if (ref_visible(transtack,drawprop)) {
      // draw the structure itself. Pop/push ref stuff is when
      // edit in place is active
      byte crchain = const_cast<layprop::DrawProperties&>(drawprop).popref(this);
      structure()->openGL_draw(transtack, drawprop, crchain == 2);
      // push is done in the ref_visible(), befire returning true
      transtack.pop();
      if (crchain) const_cast<layprop::DrawProperties&>(drawprop).pushref(this);
   }
}

void laydata::tdtcellref::tmp_draw(const layprop::DrawProperties& drawprop,
                 ctmqueue& transtack, SGBitSet*, bool under_construct) const {
   if (under_construct) return;
   if (structure()) {
      transtack.push_front(_translation * transtack.front());
      structure()->tmp_draw(drawprop, transtack);
   }   
}

void  laydata::tdtcellref::draw_select(CTM trans, const SGBitSet*) const {
   if (sh_selected == status()) draw_select_marks(overlap() * trans);
}

void laydata::tdtcellref::info(std::ostringstream& ost) const {
   ost << "cell \"" << _structure->first << "\" - reference @ (";
   ost << _translation.tx() << " , " << _translation.ty() << ")";
}

void laydata::tdtcellref::write(TEDfile* const tedfile) const {
   tedfile->putByte(tedf_CELLREF);
   tedfile->putString(_structure->first);
   tedfile->putCTM(_translation);
}

void laydata::tdtcellref::GDSwrite(GDSin::GDSFile& gdsf, word lay) const
{
   GDSin::GDSrecord* wr = gdsf.SetNextRecord(gds_SREF);
   gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_SNAME, _structure->first.size());
   wr->add_ascii(_structure->first.c_str());gdsf.flush(wr);
   TP trans;
   real rotation, scale;
   bool flipX;
   _translation.toGDS(trans,rotation,scale,flipX);
   wr = gdsf.SetNextRecord(gds_STRANS);
   if (flipX) wr->add_int2b(0x0000);
   else       wr->add_int2b(0x8000);
   gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_MAG);
   wr->add_real8b(scale);gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_ANGLE);
   wr->add_real8b(rotation);gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_XY,1);
   wr->add_int4b(trans.x());wr->add_int4b(trans.y());
   gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_ENDEL);
   gdsf.flush(wr);
}

void laydata::tdtcellref::ungroup(laydata::tdtdesign* ATDB, tdtcell* dst, atticList* nshp) {
   tdtdata *data_copy;
   shapeList* ssl;
   // select all the shapes of the referenced tdtcell
   structure()->select_all(true);
   for (selectList::const_iterator CL = structure()->shapesel()->begin();
                                   CL != structure()->shapesel()->end(); CL++) {
      // secure the target layer
      quadTree* wl = dst->securelayer(CL->first);
      // secure the select layer (for undo)
      if (nshp->end() != nshp->find(CL->first))  
         ssl = (*nshp)[CL->first];
      else {
         ssl = new shapeList();
         (*nshp)[CL->first] = ssl;
      }   
      // for every single shape on the layer
      for (dataList::const_iterator DI = CL->second->begin();
                                    DI != CL->second->end(); DI++) {
         // create a new copy of the data
         data_copy = DI->first->copy(_translation);
         // add it to the corresponding layer of the dst cell
         wl->put(data_copy);
         // ... and to the list of the new shapes (for undo)
         ssl->push_back(data_copy);
         //update the hierarchy tree if this is a cell
         if (0 == CL->first) dst->addchild(ATDB, 
                            static_cast<tdtcellref*>(data_copy)->structure());
         // add it to the selection list of the dst cell
         dst->select_this(data_copy,CL->first);
      }
      wl->invalidate();
   }
   structure()->unselect_all();
}

DBbox laydata::tdtcellref::overlap() const {
   if (structure())
      return structure()->overlap() * _translation;
   else return DBbox(TP(static_cast<int4b>(rint(_translation.tx())),
                         static_cast<int4b>(rint(_translation.ty()))));
}      
   
//-----------------------------------------------------------------------------
// class tdtcellaref
//-----------------------------------------------------------------------------
laydata::tdtcellaref::tdtcellaref(TEDfile* const tedfile) : tdtcellref(tedfile) {
   // get the matrix properties
   _stepX = tedfile->get4b();  if (!tedfile->status()) return;
   _stepY = tedfile->get4b();  if (!tedfile->status()) return;
   _rows  = tedfile->getWord();if (!tedfile->status()) return;
   _cols  = tedfile->getWord();if (!tedfile->status()) return;
}

bool laydata::tdtcellaref::aref_visible(ctmstack& transtack, const layprop::DrawProperties& drawprop, int* stst) const {
   // make sure that the referenced structure exists
   if (NULL == structure()) return false;
   // Get the areal of entire matrix, but NOT TRANSLATED !
   DBbox array_overlap = clear_overlap();
   // Calculate the CTM for the array
   CTM newtrans = _translation * transtack.top();
   // ... get the current visual (clipping) window, and make a REVERSE TRANSLATION
   DBbox clip = drawprop.clipRegion() * newtrans.Reversed();
   clip.normalize();
   // initialize the visual box from the overlap area of the array ...
   DBbox visual_box(array_overlap);
   // ... and check the visibility of entire array. if mutual_position
   // is 1, visual_box will be modified and will contain the visual region
   // of the array
   int mutual_position = clip.clipbox(visual_box);
   // if array is entirely outside the visual window - bail out
   if (0 == mutual_position) return false;

   // If we get here - means that the array (or part of it) is visible
   // draw the cell mark ...
   drawprop.draw_reference_marks(TP(0,0) * newtrans, layprop::array_mark);
   // ... and the overlapping box
   draw_overlapping_box(array_overlap * newtrans, 0xf18f);
   // now check that a single structure is big enough to be visible
   DBbox structure_overlap = structure()->overlap();
   DBbox minareal = structure_overlap * transtack.top() * drawprop.ScrCTM();
   if (minareal.area() < MIN_VISUAL_AREA) return false;
   // We are going to draw cells, so push the new translation matrix in the stack
   transtack.push(newtrans);
   // now calculate the start/stop values of the visible references in the matrix
   if (-1 == mutual_position) {
      // entire matrix is visible
      stst[0] = 0; stst[1] = _cols;
      stst[2] = 0; stst[3] = _rows;
   }
   else {
      real cstepX = (array_overlap.p2().x() - array_overlap.p1().x()) / _cols;
      real cstepY = (array_overlap.p2().y() - array_overlap.p1().y()) / _rows;
      // matrix is partially visible
      stst[0] = array_overlap.p1().x() < clip.p1().x() ?
          (int) rint((clip.p1().x() - array_overlap.p1().x()) / cstepX) : 0;
      stst[2] = array_overlap.p1().y() < clip.p1().y() ?
          (int) rint((clip.p1().y() - array_overlap.p1().y()) / cstepY) : 0;
      stst[1] = stst[0] + (int) rint((visual_box.p2().x() - visual_box.p1().x()) / cstepX);
      stst[3] = stst[2] + (int) rint((visual_box.p2().y() - visual_box.p1().y()) / cstepY);
   }
   return true;
}

void laydata::tdtcellaref::openGL_draw(ctmstack& transtack,
                                    const layprop::DrawProperties& drawprop) const {
   int stst[4];
   // now check that the entire cell array has to be drawn
   if (aref_visible(transtack, drawprop, stst)) {
      for (int i = stst[0]; i < stst[1]; i++) // start/stop rows
         for(int j = stst[2]; j < stst[3]; j++) { // start/stop columns
            // for each of the visual array figures...
            // ... get the translation matrix ...
            CTM refCTM(TP(_stepX * i , _stepY * j ), 1, 0, false);
            refCTM *= transtack.top();
            // ...draw the structure itself, not forgeting to push/pop the refCTM
            transtack.push(refCTM);
            structure()->openGL_draw(transtack, drawprop);
            transtack.pop();
         }
      // push is done in the aref_visible(), before returning true
      transtack.pop();
   }
}

void laydata::tdtcellaref::info(std::ostringstream& ost) const {
   ost << "cell \"" << _structure->first << "\" - array reference @ (";
   ost << _translation.tx() << " , " << _translation.ty() << ") ->";
   ost << " [" << _cols << " x " << _stepX << " , " ;
   ost <<         _rows << " x " << _stepY << "]";
}

void laydata::tdtcellaref::write(TEDfile* const tedfile) const {
   tedfile->putByte(tedf_CELLAREF);
   tedfile->putString(_structure->first);
   tedfile->putCTM(_translation);
   tedfile->put4b(_stepX);
   tedfile->put4b(_stepY);
   tedfile->putWord(_rows);
   tedfile->putWord(_cols);
}

void laydata::tdtcellaref::GDSwrite(GDSin::GDSFile& gdsf, word lay) const
{
   GDSin::GDSrecord* wr = gdsf.SetNextRecord(gds_AREF);
   gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_SNAME, _structure->first.size());
   wr->add_ascii(_structure->first.c_str());gdsf.flush(wr);
   TP trans;
   real rotation, scale;
   bool flipX;
   _translation.toGDS(trans,rotation,scale,flipX);
   wr = gdsf.SetNextRecord(gds_STRANS);
   if (flipX) wr->add_int2b(0x0000);
   else       wr->add_int2b(0x8000);
   gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_MAG);
   wr->add_real8b(scale);gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_ANGLE);
   wr->add_real8b(rotation);gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_COLROW);
   wr->add_int2b(_cols);wr->add_int2b(_rows);
   gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_XY,3);
   wr->add_int4b(trans.x());wr->add_int4b(trans.y());
   wr->add_int4b(trans.x() + _cols * _stepX);wr->add_int4b(trans.y());
   wr->add_int4b(trans.x());wr->add_int4b(trans.y() + _rows * _stepY);
   gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_ENDEL);
   gdsf.flush(wr);
}

void  laydata::tdtcellaref::ungroup(laydata::tdtdesign* ATDB, tdtcell* dst, laydata::atticList* nshp) {
   for (word i = 0; i < _cols; i++)
      for(word j = 0; j < _rows; j++) {
         // for each of the array figures
         CTM refCTM;
         refCTM.Translate(_stepX * i , _stepY * j );
         refCTM *= _translation;
         laydata::tdtcellref* cellref = new tdtcellref(_structure, refCTM);
         cellref->ungroup(ATDB, dst, nshp);
         delete cellref;
      }
}

DBbox laydata::tdtcellaref::overlap() const {
   if (structure()) {
     DBbox ovl(clear_overlap() * _translation);
     ovl.normalize();
     return ovl;
   }
   else return DBbox(TP(static_cast<int4b>(rint(_translation.tx())),
                         static_cast<int4b>(rint(_translation.ty()))));
}

DBbox laydata::tdtcellaref::clear_overlap() const {
   DBbox bx = structure()->overlap();
   DBbox ovl = bx;
   CTM refCTM(1.0,0.0,0.0,1.0,_stepX * (_cols-1), _stepY * (_rows - 1));
   ovl.overlap(bx * refCTM);
   return ovl;
}

//-----------------------------------------------------------------------------
// class tdttext
//-----------------------------------------------------------------------------
laydata::tdttext::tdttext(std::string text, CTM trans) : tdtdata() {
   _text = text; _translation = trans;
   _width = 0;
   for (unsigned i = 0; i < _text.length(); i++) 
      _width += glutStrokeWidth(GLUT_STROKE_ROMAN, _text[i]);
   
}
   
laydata::tdttext::tdttext(TEDfile* const tedfile) : tdtdata() {
   _width = 0;
   _text = tedfile->getString(); if (!tedfile->status()) return;
   _translation = tedfile->getCTM();  if (!tedfile->status()) return;
   for (unsigned i = 0; i < _text.length(); i++) 
      _width += glutStrokeWidth(GLUT_STROKE_ROMAN, _text[i]);
}

void laydata::tdttext::openGL_draw(ctmstack& transtack, 
                                 const layprop::DrawProperties& drawprop) const {
   //  Things to remember...
   // Font has to be translated using its own matrix in which
   // tx/ty are forced to zero. Below they are not used (and not zeroed)
   // because of the conversion to the openGL matrix. 
   // The text binding point is multiplied ALONE with the current
   // translation matrix, but NEVER with the font matrix.
   // All this as far as I remember is described in the PS manual
   // OpenGL seems to have more primitive font handling - no offense
   // IMHO.
   // The other "discovery" for the GLUT font rendering...
   // They are talking in the doc's that stroke fonts can vary from
   // 119.05 units down to 33.33 units. It is not quite clear however
   // how big (in pixels say) is one unit. After a lot of experiments
   // it appears that if you draw a character with font scale = 1, then
   // you will get a font with height 119.05 units. In order to translate 
   // the font to DBU's I need to multiply it by DBU and divide it to 119.05
   // This is done in the tellibin - int tellstdfunc::stdADDTEXT::execute()
   // Things to consider ...
   // Independently of the orientation (and flip) font matrix 
   // can be trimmed always so that texts to appear either left-to-right
   // or bottom-to top (Remember Catena?). In order to compensate the text 
   // placement, the binding point (justification) can be compensated
   // And the last, but not the least...
   // GDSII text justification
   //====================================================================
   // font translation matrix
   CTM ftmtrx =  _translation * transtack.top();
   int4b height = static_cast<int4b>(rint(OPENGL_FONT_UNIT));
   DBbox wsquare = DBbox(TP(),TP(height, height)) * ftmtrx; 
   wsquare = wsquare * drawprop.ScrCTM();
   TP bindt;
   if (wsquare.area() > MIN_VISUAL_AREA) {
      bindt = TP(static_cast<int4b>(rint(_translation.tx())), 
                 static_cast<int4b>(rint(_translation.ty())) ) * transtack.top();
      glPushMatrix();
      double ori_mtrx[] = { ftmtrx.a(), ftmtrx.b(),0,0,
                            ftmtrx.c(), ftmtrx.d(),0,0,
                                     0,          0,0,0,
                             bindt.x(),  bindt.y(),0,1};
      glMultMatrixd(ori_mtrx);                             
      for (unsigned i = 0; i < _text.length(); i++) {
         glutStrokeCharacter(GLUT_STROKE_ROMAN, _text[i]);
      }   
      glPopMatrix();
      float cclr[4];
      glGetFloatv(GL_CURRENT_COLOR, cclr);
      draw_overlapping_box(overlap() * transtack.top(), 0x3030);
      drawprop.draw_reference_marks(bindt, layprop::text_mark);
      glColor4f(cclr[0], cclr[1], cclr[2], cclr[3]);
   }
}

void laydata::tdttext::tmp_draw(const layprop::DrawProperties& drawprop,
               ctmqueue& transtack, SGBitSet*, bool under_construct) const {
   if (under_construct) return;
   //====================================================================
   // font translation matrix
   CTM trans = transtack.front();
   CTM ftmtrx =  _translation * trans;
   int4b height = static_cast<int4b>(rint(OPENGL_FONT_UNIT));
   DBbox wsquare = DBbox(TP(),TP(height, height)) * ftmtrx; 
   wsquare = wsquare * drawprop.ScrCTM();
   TP bindt;
   if (wsquare.area() > MIN_VISUAL_AREA) {
      bindt = TP(static_cast<int4b>(rint(_translation.tx())), 
                 static_cast<int4b>(rint(_translation.ty())) ) * trans;
      glPushMatrix();
      double ori_mtrx[] = { ftmtrx.a(), ftmtrx.b(),0,0,
                            ftmtrx.c(), ftmtrx.d(),0,0,
                                     0,          0,0,0,
                             bindt.x(),  bindt.y(),0,1};
      glMultMatrixd(ori_mtrx);                             
      for (unsigned i = 0; i < _text.length(); i++) {
         glutStrokeCharacter(GLUT_STROKE_ROMAN, _text[i]);
      }   
      glPopMatrix();
   }
}

void  laydata::tdttext::draw_select(CTM trans, const SGBitSet*) const {
   if (sh_selected == status()) draw_select_marks(overlap() * trans);
}

void laydata::tdttext::info(std::ostringstream&) const {
}

void laydata::tdttext::write(TEDfile* const tedfile) const {
   tedfile->putByte(tedf_TEXT);
   tedfile->putString(_text);
   tedfile->putCTM(_translation);
}

void laydata::tdttext::GDSwrite(GDSin::GDSFile& gdsf, word lay) const
{
   GDSin::GDSrecord* wr = gdsf.SetNextRecord(gds_TEXT);
   gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_LAYER);
   wr->add_int2b(lay);gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_TEXTTYPE);
   wr->add_int2b(0);gdsf.flush(wr);
   TP trans;
   real rotation, scale;
   bool flipX;
   _translation.toGDS(trans,rotation,scale,flipX);
   wr = gdsf.SetNextRecord(gds_STRANS);
   if (flipX) wr->add_int2b(0x0000);
   else       wr->add_int2b(0x8000);
   gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_MAG);
   wr->add_real8b(scale);gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_ANGLE);
   wr->add_real8b(rotation);gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_XY,1);
   wr->add_int4b(trans.x());wr->add_int4b(trans.y());
   gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_STRING, _text.size());
   wr->add_ascii(_text.c_str());gdsf.flush(wr);
   wr = gdsf.SetNextRecord(gds_ENDEL);
   gdsf.flush(wr);
}

DBbox laydata::tdttext::overlap() const {
   return DBbox(TP(),TP(_width,static_cast<int4b>(rint(OPENGL_FONT_UNIT)))) * 
                                                                   _translation;
}


//-----------------------------------------------------------------------------
// class valid_poly
//-----------------------------------------------------------------------------
laydata::valid_poly::valid_poly(const pointlist& plist) : validator(plist) { 
   // first check whether adjacent points coincide and remove them
   // this will remove the last point if it coincides with the first one
   identical();
   if (_status > 0x10) return;
   // now check the angles of the polygon & whether the polygon is a box
   angles();
   if (_status > 0x10) return;
   selfcrossing();
   if (_status > 0x10) return;
   //reorder the chain (if needed) to get the points in anticlockwise order
   normalize();
}

laydata::tdtdata* laydata::valid_poly::replacement() {
   laydata::tdtdata* newshape;
   if (box()) {
      TP* p1= new TP(_plist[0]); TP* p2= new TP(_plist[2]);
      newshape = new laydata::tdtbox(p1, p2);
   }
   else newshape = new laydata::tdtpoly(_plist);
   return newshape;
}   

void laydata::valid_poly::identical() {
   pointlist::iterator cp1 = _plist.end(); cp1--;
   pointlist::iterator cp2 = _plist.begin();
   while (cp2 != _plist.end()) {
      if (*cp1 == *cp2) {
         cp2 = _plist.erase(cp2);
         _status |= laydata::shp_ident;
      }
      else { cp1 = cp2; cp2++;};
   }
   if (0 == _plist.size()) _status |= laydata::shp_null; // 0 unequal points
}

/*! Checks the polygon angles. Filters out intermediate points. Flags
    a box if found. Flags an error if acute angle is digitized
*/
void laydata::valid_poly::angles() {
   word cp1 = _plist.size() - 1;
   word cp2 = 0;
   word cp3 = 1;
   int ang;
   pointlist::iterator cp = _plist.begin();
   while (cp != _plist.end()) {
      ang = abs(xangle(_plist[cp2], _plist[cp3]) - xangle(_plist[cp2], _plist[cp1]));
      if ((0 == ang) || (180 == ang)) {
         cp = _plist.erase(cp);
         _status |= laydata::shp_line;
         cp3 = cp3 % _plist.size(); // !!
      }   
      else if (ang < 90 || ang > 270) {
         _status |= laydata::shp_acute;
         return;
      }   
      else {
         cp1 = cp2; cp2 = cp3; cp3 = ((cp3 + 1) % _plist.size());cp++;
      }
   }
   // check the resulting shape has more than 4 vertexes
   if (_plist.size() < 4) {_status = 0x80; return;}
   // finally check whether it's a box - if any segment of 4 vertex polygon
   // (already checked for acute angles) has an angle towards x axis
   // that is multiply on 90 - then it should be a box
   if ((4 == _plist.size()) && 
                        (0 == remainder(xangle(_plist[cp2], _plist[cp3]),90.0)))
      _status |= laydata::shp_box;
}

void laydata::valid_poly::normalize() {
   real area = 0;
   word size = _plist.size();
   word i,j;
   for (i = 0, j = 1; i < size; i++, j = (j+1) % size) 
      area += _plist[i].x()*_plist[j].y() - _plist[j].x()*_plist[i].y();
   if (area < 0)  {
	   std::reverse(_plist.begin(),_plist.end());
      _status |= laydata::shp_clock;
   }
}

/*! Implements  algorithm to check that the polygon is not
self crossing. Alters the laydata::shp_cross bit of _status if the polygon
is selfcrossing
*/ 
void laydata::valid_poly::selfcrossing() {
   // after all checks above, polygon has to have at least 6 points
   // to be self-crossing
   if (_plist.size() < 6) return;
   tedop::segmentlist segs(_plist, false);
   tedop::EventQueue Eq(segs); // initialize the event queue
   tedop::SweepLine  SL(_plist); // initialize the sweep line
   if (!Eq.check_valid(SL)) 
      _status |= laydata::shp_cross;
}

char* laydata::valid_poly::failtype() {
   if      (_status & shp_null)  return "Zero area";
   else if (_status & shp_acute) return "Acute angle";
   else if (_status & shp_cross) return "Self-crossing";
   else if (_status & shp_4pnts) return "Unsuficcient points";
   else return "OK";
}   

//-----------------------------------------------------------------------------
// class valid_wire
//-----------------------------------------------------------------------------
laydata::valid_wire::valid_wire(const pointlist& plist, word width) : 
                                     validator(plist), _width(width) { 
   // first check whether adjacent points coincide and remove them
   identical();
   if (_status > 0x10) return;
   // now check the angles of the wire
   angles();
   if (_status > 0x10) return;
   selfcrossing();
}

void laydata::valid_wire::identical() {
   pointlist::iterator cp1 = _plist.begin();
   pointlist::iterator cp2 = cp1;cp2++;
   while (cp2 != _plist.end()) {
      if (*cp1 == *cp2) {
         cp2 = _plist.erase(cp2);
         _status |= laydata::shp_ident;
      }
      else {cp1 = cp2;cp2++;}
   }
   if (0 == _plist.size()) _status |= laydata::shp_null; // 0 unequal points
}

/*! Checks the wire angles. Filters out intermediate points. 
Flags an error if acute angle is digitized
*/
void laydata::valid_wire::angles() {
   word cp3 = 2;
   int ang;
   pointlist::iterator cp = _plist.begin();cp++;
   while (cp3 < _plist.size()) {
      ang = abs(xangle(_plist[cp3-1], _plist[cp3]) - xangle(_plist[cp3-1], _plist[cp3-2]));
      if ((0 == ang) || (180 == ang)) {
         cp = _plist.erase(cp);
         _status |= laydata::shp_line;
      }   
      else if (ang < 90 || ang > 270) {
         _status |= laydata::shp_acute;
         return;
      }   
      else {cp3++;cp++;}
   }
   // check the resulting shape has more than 4 vertexes
   if (_plist.size() < 2) {_status = 0x80; return;}
}

/*! Implements  algorithm to check that the wire is not simple crossing. 
Alters the laydata::shp_cross bit of _status if the wire is selfcrossing
*/ 
void laydata::valid_wire::selfcrossing() {
   // after all previous checks , wre has to have at least 5 points
   // to be self-crossing
   if (_plist.size() < 5) return;
   tedop::segmentlist segs(_plist, true);
   tedop::EventQueue Eq(segs); // initialize the event queue
   tedop::SweepLine  SL(_plist); // initialize the sweep line
   if (!Eq.check_valid(SL)) 
      _status |= laydata::shp_cross;
}

laydata::tdtdata* laydata::valid_wire::replacement() {
   return new laydata::tdtwire(_plist, _width);
}   

char* laydata::valid_wire::failtype() {
//   if (_status & shp_null)  return "Zero area";
   if      (_status & shp_acute) return "Acute angle";
   else if (_status & shp_cross) return "Self-crossing";
   else if (_status & shp_4pnts) return "Unsuficcient points";
   else return "OK";
}   

//-----------------------------------------------------------------------------
/*! Returns the angle between the line and the X axis
*/
int laydata::xangle(TP& p1, TP& p2) {
   const long double Pi = 3.1415926535897932384626433832795;
   if (p1.x() == p2.x()) { //vertcal line
      assert(p1.y() != p2.y()); // make sure both points do not coinside
      if   (p2.y() > p1.y()) return  90;
      else                   return -90;
   }
   else if (p1.y() == p2.y()) { // horizontal line
      if (p2.x() > p1.x()) return 0;
      else                 return 180;
   }   
   else
      return (int)rint(180*atan2(p2.y() - p1.y(), p2.x() - p1.x())/Pi);
}

//-----------------------------------------------------------------------------
// other...
//-----------------------------------------------------------------------------
laydata::tdtdata* laydata::createValidShape(const pointlist& pl) {
   laydata::valid_poly check(pl);
   if (!check.valid()) {
      std::ostringstream ost;
      ost << "Resulting shape is invalid - " << check.failtype();
      tell_log(console::MT_ERROR, ost.str().c_str());
      return NULL;
   }
   laydata::tdtdata* newshape;
   pointlist npl = check.get_validated();
   if (check.box())
      newshape = new laydata::tdtbox(new TP(npl[2]),new TP(npl[0]));
   else
      newshape = new laydata::tdtpoly(npl);
   npl.clear();
   return newshape;
}

laydata::tdtdata* laydata::polymerge(const pointlist& _plist0, const pointlist& _plist1) {
   if(_plist0.empty() || _plist1.empty()) return NULL;
   logicop::pcollection merge_shape;
   logicop::logic operation(_plist0, _plist1);
   if (operation.OR(merge_shape)) {
      assert(1 == merge_shape.size());
 //      logicop::pcollection::const_iterator CI;
 //      // add the resulting cut_shapes to the_cut shapeList
 //      for (CI = cut_shapes.begin(); CI != cut_shapes.end(); CI++) {
 //         decure[1]->push_back(new laydata::tdtpoly(**CI));
 //      }
      return new laydata::tdtpoly(**merge_shape.begin());
   }
   else return NULL;
}

void laydata::draw_select_mark(const TP& pnt) {
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glRasterPos2i(pnt.x(), pnt.y());
   glBitmap(15,15,7,7,0,0, select_mark);
}

void laydata::draw_select_marks(DBbox box) {
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glRasterPos2i(box.p1().x(), box.p1().y());
   glBitmap(15,15,7,7,0,0, select_mark);
   glRasterPos2i(box.p1().x(), box.p2().y());
   glBitmap(15,15,7,7,0,0, select_mark);
   glRasterPos2i(box.p2().x(), box.p1().y());
   glBitmap(15,15,7,7,0,0, select_mark);
   glRasterPos2i(box.p2().x(), box.p2().y());
   glBitmap(15,15,7,7,0,0, select_mark);
}

void laydata::draw_overlapping_box(const DBbox& areal, const GLushort stipple) {
   glColor4f(1.0, 1.0, 1.0, 0.5);
   glLineStipple(1,stipple);
   glEnable(GL_LINE_STIPPLE);
   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   glRecti(areal.p1().x(), areal.p1().y(), areal.p2().x(), areal.p2().y());
   glDisable(GL_LINE_STIPPLE);
}

