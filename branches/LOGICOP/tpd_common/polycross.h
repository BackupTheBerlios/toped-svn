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
//        Created: Tue Mar 21 2006
//         Author: s_krustev@yahoo.com
//      Copyright: (C) 2001-2006 by Svilen Krustev
//    Description: Modified Bentley-Ottman algorithm
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================

#ifndef PLYCROSS_H_INCLUDED
#define PLYCROSS_H_INCLUDED

#include <vector>
#include "../tpd_common/ttt.h"
#include "../tpd_common/avl_def.h"

namespace polycross
{
   class YQ;
   class XQ;
   class BindCollection;
   class TEvent;
   typedef std::list<unsigned> ThreadList;
   typedef enum {_endE, _modifyE, _beginE, _crossE} EventTypes;
   
   int xyorder(const TP*, const TP*);
   int orientation(const TP*, const TP*, const TP*);
   float getLambda( const TP* p1, const TP* p2, const TP* p);
   bool coinsidingSegm(const TP*, const TP*, const TP*);
   //===========================================================================
   // VPoint
   //===========================================================================
   class VPoint
   {
      public:
         VPoint(const TP* cp) : _cp(cp),_next(NULL),_prev(NULL) {};
         VPoint(const TP* cp, VPoint* prev);
         virtual VPoint*  follower(bool& direction, bool modify = false);
         virtual bool     inside(const pointlist&, bool touching = false);
         virtual char     visited() const      {return 1;}
         virtual void     reset_visited()      {};
         virtual VPoint*  checkNreorder();
         const TP*         cp() const           {return _cp;};
         VPoint*           next() const         {return _next;};
         VPoint*           prev() const         {return _prev;};
         void              set_next(VPoint* nx) {_next = nx;};
         void              set_prev(VPoint* pr) {_prev = pr;};
         virtual          ~VPoint() {};
      protected:
         const TP*         _cp;
         VPoint*           _next;
         VPoint*           _prev;
   };

   //===========================================================================
   // CPoint
   //===========================================================================
   class CPoint : public VPoint
   {
      public:
         CPoint(const TP* cp) : VPoint(cp),_link(NULL),_visited(0) {};
         virtual VPoint*  follower(bool& direction, bool modify = false);
         bool              inside(const pointlist&, bool touching = false) {return true;}
         char              visited() const {return _visited;}
         void              linkto(CPoint* link) {_link = link;}
         CPoint*           link() const {return _link;}
         void              reset_visited() {_visited = 0;};
         void              linkage(VPoint*& prev);
         VPoint*           checkNreorder()       {assert(false);}
      protected:
         CPoint*           _link;
         char              _visited;
   };

   //===========================================================================
   // CPoint
   //===========================================================================
   class BPoint : public CPoint
   {
      public:
      //! Creates a new BPoint simply by calling the CPoint constructor
         BPoint(const TP* cp) : CPoint(cp) {};
      //! Returns always 1 - VPoint is considered always visited
         char              visited() const      {return -1;}
      //! Returns the following point for the currently generated polygon
         VPoint*           follower(bool& direction, bool modify = false);
   };

   //===========================================================================
   // SortLine
   //===========================================================================
   class SortLine
   {
      public:
         SortLine(const TP* p1, const TP* p2) {direction = xyorder(p1,p2);};
         bool operator    () (CPoint*, CPoint*);
      protected:
         int               direction;
   };

   //===========================================================================
   // polysegment
   //===========================================================================
   class polysegment
   {
      public:
         typedef std::vector<CPoint*> crossCList;
         polysegment(const TP*, const TP*, int, char);
         ~polysegment();
         void              freeMem();
         CPoint*           insertCrossPoint(const TP*);
         BPoint*           insertBindPoint(const TP* pnt);
         unsigned          normalize(const TP*, const TP*);
         void              dump_points(polycross::VPoint*&);
         unsigned          threadID() const           {return _threadID;}
         void              set_threadID(unsigned ID)  {_threadID = ID;}
         const TP*         lP() const                 {return _lP;}
         const TP*         rP() const                 {return _rP;}
         byte              polyNo() const             {return _polyNo;}
         int               edge() const               {return _edge;}
         TP*               checkIntersect(polysegment*, XQ&);
      protected:
         unsigned          _threadID;
         crossCList        crosspoints;
         byte              _polyNo;
         int               _edge;
         const TP*         _lP;
         const TP*         _rP;
   };

   //===========================================================================
   // segmentlist
   //===========================================================================
   class segmentlist
   {
      public:
         typedef std::vector<polysegment*> Segments;
         segmentlist(const pointlist&, byte);
         ~segmentlist();
         polysegment*      operator [](unsigned i) const {return _segs[i];};
         unsigned          size() const {return _segs.size();};
         unsigned          normalize(const pointlist&);
         VPoint*           dump_points();
         BPoint*           insertbindpoint(unsigned segno, const TP* point);
         const pointlist*  originalPL() const {return _originalPL;}
      private:
         Segments          _segs;
         const pointlist*  _originalPL;
   };

   //===========================================================================
   // Event Vertex - could be more than one event
   //===========================================================================
   class EventVertex
   {
      public:
         EventVertex(const TP* evertex) : _evertex(evertex) {};
         const TP*         operator () () {return _evertex;};
         void              addEvent(TEvent*, EventTypes);
         void              sweep(YQ&, XQ&);
         void              sweep2bind(YQ&, BindCollection&);
         void              CheckBEM(XQ&, TEvent&, TEvent&);
      private:
         typedef std::list<TEvent*> Events;
         typedef std::map<int, Events> AllEvents;
         AllEvents         _events;
         const TP*         _evertex;
         ThreadList        _threadsSweeped;
   };

   //===========================================================================
   // Thread event - pure virtual
   //===========================================================================
   class TEvent
   {
      public:
         friend void EventVertex::CheckBEM(XQ&, TEvent&, TEvent&);
         TEvent(byte shapeID) : _shapeID(shapeID) {};
         virtual void      sweep(XQ&, YQ&, ThreadList&) = 0;
         virtual void      sweep2bind(YQ&, BindCollection&) = 0;
         virtual const TP* avertex() = 0;
         virtual const TP* bvertex() = 0;
         const TP*         evertex() {return _evertex;}
         byte              shapeID() {return _shapeID;}
         polysegment*      aseg() {return _aseg;}
         polysegment*      bseg() {return _bseg;}
         virtual          ~TEvent() {};
      protected:
         TP*               checkIntersect(polysegment*, polysegment*, XQ&, const TP* iff=NULL);
         byte              _shapeID;
         const TP*         _evertex;
         void              insertCrossPoint(TP*, polysegment*, polysegment*,
                                            XQ&, bool dontswap = false);
         polysegment*      _aseg;
         polysegment*      _bseg;
      private:
         TP*               joiningSegments(polysegment*, polysegment*, float, float);
         TP*               oneLineSegments(polysegment*, polysegment*, YQ*);
         TP*               getCross(polysegment*, polysegment*);
         TP*               getMiddle(const TP*, const TP*);
   };

   //===========================================================================
   // Thread begin event
   //===========================================================================
   class TbEvent : public TEvent
   {
      public:
         TbEvent(polysegment*, polysegment*, byte);
         void              sweep(XQ&, YQ&, ThreadList&);
         void              sweep2bind(YQ&, BindCollection&);
         const TP*         avertex() {return _aseg->rP();}
         const TP*         bvertex() {return _bseg->rP();};
   };

   //===========================================================================
   // Thread end event
   //===========================================================================
   class TeEvent : public TEvent
   {
      public:
         TeEvent(polysegment*, polysegment*, byte);
         void              sweep(XQ&, YQ&, ThreadList&);
         void              sweep2bind(YQ&, BindCollection&);
         const TP*         avertex() {return _aseg->lP();}
         const TP*         bvertex() {return _bseg->lP();}
   };

   //===========================================================================
   // Thread modify event
   //===========================================================================
   class TmEvent : public TEvent
   {
      public:
         TmEvent(polysegment*, polysegment*, byte);
         void              sweep(XQ&, YQ&, ThreadList&);
         void              sweep2bind(YQ&, BindCollection&);
         const TP*         avertex() {return _aseg->lP();}
         const TP*         bvertex() {return _bseg->rP();}
   };

   //===========================================================================
   // Thread cross event
   //===========================================================================
   class TcEvent : public TEvent
   {
      public:
         TcEvent(TP* ev, polysegment* aseg, polysegment* bseg): TEvent(0),
            _threadAbove(aseg->threadID()),_threadBelow(bseg->threadID())
               {_aseg = aseg; _bseg = bseg;  _evertex = ev;}
         void              sweep(XQ&, YQ&, ThreadList&);
         void              sweep2bind(YQ&, BindCollection&) {assert(false);}
         const TP*         avertex() {assert(false);}
         const TP*         bvertex() {assert(false);}
         bool              operator == (const TcEvent&) const;
      private:
         unsigned          _threadAbove;
         unsigned          _threadBelow;
   };
   
   //===========================================================================
   // Segment Thread
   //===========================================================================
   class SegmentThread
   {
      public:
         SegmentThread(polysegment* cseg, SegmentThread* tb, SegmentThread* ta) :
            _cseg(cseg) , _threadBelow(tb), _threadAbove(ta) {};
         void              set_threadBelow(SegmentThread* td) {_threadBelow = td;}
         void              set_threadAbove(SegmentThread* td) {_threadAbove = td;}
         polysegment*      cseg()         {return _cseg;}
         polysegment*      set_cseg(polysegment* cs);
         virtual SegmentThread*    threadBelow()  {return _threadBelow;}
         virtual SegmentThread*    threadAbove()  {return _threadAbove;}
         virtual ~SegmentThread(){};
      protected:
         polysegment*      _cseg; // current segment
         SegmentThread*    _threadBelow;
         SegmentThread*    _threadAbove;
   };


   //===========================================================================
   // Y queue
   //===========================================================================
   class YQ
   {
      public:
         typedef std::map<int,SegmentThread*> Threads;
         YQ(DBbox&, const segmentlist*, const segmentlist*);
         ~YQ();
         SegmentThread*    beginThread(polysegment*);
         SegmentThread*    endThread(unsigned);
         SegmentThread*    modifyThread(unsigned, polysegment*);
         SegmentThread*    swapThreads(unsigned, unsigned);
         SegmentThread*    getThread(unsigned);
         void              report();
         const pointlist*  opl1() const {return _osl1->originalPL();}
         const pointlist*  opl2() const {return _osl2->originalPL();}
      private:
         class TopSentinel : public SegmentThread
         {
            public:
               TopSentinel(polysegment* cseg) : SegmentThread(cseg,NULL,NULL) {};
               SegmentThread*    threadAbove()  {assert(false);}
               ~TopSentinel() { _cseg->freeMem();delete _cseg;}
         };
         class BottomSentinel : public SegmentThread
         {
            public:
               BottomSentinel(polysegment* cseg) : SegmentThread(cseg,NULL,NULL) {};
               SegmentThread*    threadBelow()  {assert(false);}
               ~BottomSentinel() { _cseg->freeMem(); delete _cseg;}
         };

         BottomSentinel*   _bottomSentinel;
         TopSentinel*      _topSentinel;
         int               sCompare(const polysegment*, const polysegment*);
         Threads           _cthreads;
         int               _lastThreadID;
         const segmentlist* _osl1;
         const segmentlist* _osl2;
   };

   //===========================================================================
   // X queue
   //===========================================================================
   class XQ {
      public:
         XQ(const segmentlist &, const segmentlist&);
         ~XQ();
         void              sweep();
         void              sweep2bind(BindCollection&);
         void              addCrossEvent(TP*, polysegment*, polysegment*);
         YQ*               sweepline() {return _sweepline;}
      protected:
         void              createEvents(const segmentlist&, byte);
         static int        E_compare( const void*, const void*, void* );
         avl_table*        _xqueue;
         YQ*               _sweepline;
         DBbox             _overlap;
         TP                _bottom_left;
         TP                _top_right;
   };

   //===========================================================================
   // Bind segment
   //===========================================================================
   class BindSegment
   {
      public:
         BindSegment(unsigned p0s, unsigned p1s, const TP* p0p, const TP* p1p,
                     real dist) : _poly0seg(p0s), _poly1seg(p1s), _poly0pnt(p0p),
         _poly1pnt(p1p), _distance(dist) {};
         unsigned          poly0seg() { return _poly0seg;};
         unsigned          poly1seg() { return _poly1seg;};
         const TP*         poly0pnt() const {return _poly0pnt;}
         const TP*         poly1pnt() const {return _poly1pnt;}
         real              distance() { return _distance;};
      private:
         unsigned          _poly0seg;
         unsigned          _poly1seg;
         const TP*         _poly0pnt;
         const TP*         _poly1pnt;
         real              _distance;
   };
   
   //===========================================================================
   // BindCollection
   //===========================================================================
class BindCollection
   {
      public:
         void              update_BL(polysegment*, unsigned, const TP*);
         BindSegment*      get_highest();
      private:
         typedef std::list<BindSegment*> BindList;
         bool              is_shorter(unsigned segno, real dist);
         BindList          _blist;
   };

}

#endif
