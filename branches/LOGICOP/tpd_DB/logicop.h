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
//        Created: Mon Oct 01 2004
//         Author: s_krustev@yahoo.com
//      Copyright: (C) 2001-2004 by Svilen Krustev
//    Description: polygon logic operations
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------                
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================
#ifndef LOGICOP_H_INCLUDED
#define LOGICOP_H_INCLUDED

#include <vector>
#include "../tpd_common/ttt.h"
#include "../tpd_common/avl_def.h"

namespace logicop {
   typedef enum {revent_pri = 0, cevent_pri = 1, levent_pri = 2} EventPriority;
   class SweepLine;
   class BindCollection;
   class CEvent;
   class logic;
   //===========================================================================
   // Bentley-Ottmann alghorithm related definitions      
   int xyorder(const TP*, const TP*);
   float isLeft(const TP*, const TP*, const TP*);
   //===========================================================================
   // VPoint
   //===========================================================================
   /*! This class is effectively a double linked polygon vertex representation. 
   It is deriving its data from segmentlist and plysegment classes. VPoint data
   structure is created by segmentlist::dump_points() method and is used as
   a source data by all consequitive methods to generate logic AND/OR etc.
   output polygons \n
   This class unlike STL containters does not have a top level wrapper. Instead
   we have to keep a pointer to one of the end VPoints in the list, that will 
   ensure us an access to the entire list using next/prev fields. The last thing 
   to note is that the classes that use this structure require circular closed
   lists, so normally there are no first and last objects in this type of lists*/
   class VPoint {
   public:
      //! Constructor called by the CPoint constructor to create a new vertex
                        VPoint(const TP* cp) : _cp(cp),_next(NULL),_prev(NULL) {};
      //! Used to create new VPoint, called in plysegment::dump_points()
                        VPoint(const TP* cp, VPoint* prev);
      //! Returns the following point for the currently generated polygon
      virtual VPoint*  follower(bool& direction, bool modify = false);
      //! Return true if #_cp is inside the polygon described by the input parameter
      virtual bool     inside(const pointlist&);
      //! Returns always 1 - VPoint is considered always visited
      virtual char     visited() const      {return 1;} 
      //! Does nothing here, because VPoints are considered always visited
      virtual void     reset_visited()      {};
      //! Returns a pointer to the actual point that this object represents
      const TP*         cp() const           {return _cp;};
      //! Returns a pointer to the next point in the sequence
      VPoint*           next() const         {return _next;};
      //! Returns a pointer to the previous point in the sequence
      VPoint*           prev() const         {return _prev;};
      //! Setup the pointer to the next VPoint in the sequence
      void              set_next(VPoint* nx) {_next = nx;};
      //! Setup the pointer to the previous VPoint in the sequence
      void              set_prev(VPoint* pr) {_prev = pr;};
      virtual          ~VPoint() {};
   protected:
      //! A pointer to the actual vertex
      const TP*         _cp;
      //! A pointer to the next VPoint in the sequence
      VPoint*           _next;
      //! A pointer to the previous VPoint in the sequence
      VPoint*           _prev;
   };
   
   //===========================================================================
   // CPoint
   //===========================================================================
   /*! The crossing points generated by the Bentley-Ottmann algorithm and 
   extracted from the segmentlist objects still require a special treatment 
   during the polygon generation for all logic operation. This class is 
   implementing this behaviour in order to utilize the polygon generation. It
   inherits VPoint class and the most important update here is the _link field.
   It simply points to the coresponding crossing point in the other VPoint list 
   structure. This link makes possible the switching between VPoint lists when 
   new polygons are generated by the logic operation methods of logic class.
   The class also keep a track of the visited CPoint object via the field
   _visited. This is required for the proper operation of the logic class
   methods.*/
   class CPoint : public VPoint {
   public:
      //! Creates a new CPoint simply by calling on of the VPoint constructors
                        CPoint(const TP* cp) : VPoint(cp),_link(NULL),_visited(0) {};
      //! Returns the following point for the currently generated polygon
      virtual VPoint*  follower(bool& direction, bool modify = false);
      //! Return always true - crossing points are always considered inside both polygons
      bool              inside(const pointlist&) {return true;}
      //! Returns the value of the _visited field
      char              visited() const {return _visited;} 
      //! Initialises the _link field pointing to the corresponding CPoint in the other polygon
      void              linkto(CPoint* link) {_link = link;}
      //! Initialises _visited field to 0
      void              reset_visited() {_visited = 0;};
      //! Links current object in the dumped VPoint list
      void              linkage(VPoint*& prev);
   protected:
      //! Points to the corresponding CPoint in the other VPoint list (raw polygon)
      CPoint*           _link;
      //! 0 if this CPoint has not been traversed, otherwise, the number of visits during the traversing
      char              _visited;
   };
   
   class BPoint : public CPoint {
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
   class SortLine {
   /*!A functor class used as a criteria to sort plysegment::crosspoints. The 
   sorting itself is performed in plysegment::normalize() - see its description.
   The need for this class is determined by the fact that a certain segment can
   contain more than one crossing points. These points has to be lined-up 
   according to the order of the segment endpoints, i.e. if segment start point
   is the righmost point, crossing points has to be ordered from right to left,
   and vice versa. The segment direction is determined during the construction
   and used afterwards as a criteria by the () operator
   */
   public:
      //! Functor constructor. Calls xyorder(p1,p2) to establish the direction of the segment                        
                        SortLine(const TP* p1, const TP* p2) 
                                                   {direction = xyorder(p1,p2);};
      //! The actual operator called by the vector sort algorithm
      bool operator    () (CPoint*, CPoint*);
   protected:
      //! The direction of the segment
      int               direction;
   };
                              
   //===========================================================================
   // plysegment
   //===========================================================================
   /*!This class together with the assotiated segmentlist class play a central
   role in the whole Bentley-Ottmann algorithm as well as in the consequitive
   logic operations. It holds the basic data about polygon segments excersised
   for crossing points, the crossing points themselfs as well as the links 
   between them.*/
   class plysegment {
   public:
      //! A definition of the cross point array  
      typedef std::vector<CPoint*> crossCList;
      //! The only constructor of the class
                        plysegment(const TP*, const TP*, int, char);
      //! Add a new cross point for this segment to the crosspoints array 
      CPoint*           insertcrosspoint(const TP*);
      //! Add a new bind point for this segment to the crosspoints array 
      BPoint*           insertbindpoint(const TP*);
      //! sort the crosspoints array
      unsigned          normalize(const TP*, const TP*);
      //! Dump lP and crosspoints in a VPoint list
      void              dump_points(VPoint*&);
      //! keeps track of the edge numbers
      int               edge;
      //! A pointer to the segment left point
      const TP*         lP;
      //! A pointer to the segment right point
      const TP*         rP;
      //! the array of the crossing points
      crossCList        crosspoints;
      //! A pointer to the plysegment currently above this in the sweep line
      plysegment*       above;
      //! A pointer to the plysegment currently below this in the sweep line
      plysegment*       below;
      //! A polygon ID to which this segment belongs - used in the AVL tree comparison functions
      char              polyNo;
   };
   
   //===========================================================================
   /*!A class - wrapper for plysegment class. It provides some convinient methods 
   that effectively traverse the whole segment collection applying some of the 
   plysegment methods. The segment collection itelf is implemented using vector 
   STL container. In this sence the name of the class can be little confusing
   having in mind that it uses vector container \n
   Another interpretation of this class can be - a polygon, but represented by
   its segments instead of its vertices. The fact is that this is not strictly
   true because plysegment might contain not exactly the polygon vertices.*/
   class segmentlist {
   public:
      //! The only constructor of this class
                        segmentlist(const pointlist&, byte);
      //! The class descructor
                       ~segmentlist();
      //! Return a ponter to the i-th segment of the collection
      plysegment*       operator [](unsigned int& i) const {return _segs[i];};
      //! Returns the number of segments in the collection
      unsigned          size() const {return _segs.size();};
      //! Collection traverser, calling plysegment::normalize() method
      unsigned          normalize(const pointlist&);
      //! Collection traverser, calling plysegment::dump_points() method
      VPoint*           dump_points();
      BPoint*           insertbindpoint(unsigned, const TP*);
   private:
      //! plysegment collection stored in a vector container
      std::vector<plysegment*> _segs;
   };
   
   //===========================================================================
   // Event
   //===========================================================================
   /*!A pure virtual class implementing the event fuctionality needed for 
   Bentley-Ottmann algorithm. This class is inherited by 3 other classes - LEvent,
   REvent and CEvent dealing with left, right and crossing events respectively.
   EventQueue class holds a structure of all Event objects in an AVL tree and 
   swipes (traverses) them from left to rights. \n
   Class does not define neither a default constructor nor destructor */
   class Event {
   public:
      //! Return the event vertex
      virtual const TP* evertex() const = 0;
      //! Return the oposite vertex (not the event one)
      virtual const TP* overtex() const = 0;
      //! Return the event priority
      virtual const EventPriority epriority() const = 0;
      //! Perform the operations as defined in the Bentley-Ottman algorithm
      virtual void      swipe4cross(SweepLine&, avl_table*) {assert(false);};
      //! Generate binding points of a hole polygon
      virtual void      swipe4bind(SweepLine&, BindCollection&) {assert(false);};
      //! Return the polygon number for this event
      virtual char      polyNo() const = 0;
      //! Return the edge number for this event
      virtual int       edgeNo() const = 0;
      //! Check the input crossing event and adds it to the event queue
      void               checkNupdate(avl_table*, plysegment*, plysegment*, 
                                      CEvent*, bool check=true);
      virtual           ~Event() {};
   };
   
   //===========================================================================
   // LEvent
   //===========================================================================
   /*! Implements the functionality required by the EventQueue when a segment
   has to be added to the SweepLine. Inherits the Event class. The only 
   important method in this class is swipe4cross() */
   class LEvent : public Event {
   public:
      //! The constructor of the left event - it just initialises the #_seg pointer
                         LEvent(plysegment* seg) : _seg(seg) {};
      //! Return the event vertex
      const TP*          evertex() const {return _seg->lP;}
      //! Return the oposite vertex (not the event one)
      const TP*          overtex() const {return _seg->rP;}
      //! Return the event priority
      const EventPriority epriority() const {return levent_pri;}
      //! Perform the operations as defined by Bentley-Ottman for left point
      void               swipe4cross(SweepLine&, avl_table*);
      //! Perform the operations for binding point generation
      void               swipe4bind(SweepLine&, BindCollection&);
      //! Return the polygon number for this event
      char               polyNo() const {return _seg->polyNo;};
      //! Return the edge number for this event
      int                edgeNo() const {return _seg->edge;};
   protected:   
      //! A pointer to the polygon segment assigned to this event
      plysegment*        _seg;
   };
   
   //===========================================================================
   // REvent
   //===========================================================================
   /*! Implements the functionality required by the EventQueue when a segment
   has to be removed from the SweepLine. Inherits the Event class. The only 
   important method in this class is swipe4cross() */
   class REvent : public Event {
   public:
      //! The constructor of the right event - it just initialises the #_seg pointer
                         REvent(plysegment* seg) : _seg(seg) {};
      //! Return the event vertex
      const TP*          evertex() const {return _seg->rP;};
      //! Return the oposite vertex (not the event one)
      const TP*          overtex() const {return _seg->lP;}
      //! Return the event priority
      const EventPriority epriority() const {return revent_pri;}
      //! Perform the operations as defined by Bentley-Ottman for right point
      void               swipe4cross(SweepLine&, avl_table*);
      //! Perform the operations for binding point generation
      void               swipe4bind(SweepLine&, BindCollection&);
      //! Return the polygon number for this event
      char               polyNo() const {return _seg->polyNo;};
      //! Return the edge number for this event
      int                edgeNo() const {return _seg->edge;};
   protected:   
      //! A pointer to the polygon segment assigned to this event
      plysegment*        _seg;
   };
   
   //===========================================================================
   // CEvent
   //===========================================================================
   /*! Implements the functionality required by the EventQueue when a cross
   point event is digitized in the event queue. Inherits the Event class and has 
   a slightly different structure compared with LEvent and REvent classes. */
   class CEvent : public Event {
   public:
      //! The cross event constructor
                         CEvent(plysegment*, plysegment*);
                         CEvent(plysegment*, plysegment*, const TP*, bool);
      //! Return the event vertex - in this case cross point
      const TP*          evertex() const {return _cp;};
      //! Return the oposite vertex (not the event one)
      const TP*          overtex() const {assert(false); return NULL;}
      //! Return the event priority
      const EventPriority epriority() const {return cevent_pri;}
      //! Perform the operations as defined by Bentley-Ottman for cross point
      void               swipe4cross(SweepLine&, avl_table*);
      //! Return -1 as polygon number for the cross event
      char               polyNo() const {return -1;};
      //! Return -1 as edge number for the cross event
      int                edgeNo() const {return -1;};
      //! Return a pointer to the crossing point #_cp
//      TP*                cp() const {return _cp;};
      bool               swaponly() const {return _swaponly;}
      const plysegment*  below() const {return _below;}
      const plysegment*  above() const {return _above;}
      protected:
      //! A pointer to the first crossing segment
      plysegment*       _above;
      //! A pointer to the second crossing segment
      plysegment*       _below;
      bool              _swaponly;
      //! A pointer to the crossing point produced by the #_above/#_below pair of polygon segments
      TP*               _cp;
   };

   //===========================================================================
   // EventQueue
   //===========================================================================
   /*! The class implements the event queue in the Bentley-Ottmann algorithm. 
   For the actual implementation of the event queue structure another AVL balanced 
   tree is used of avl_table type. May be the queue could have been implemented 
   using an STL container. The structure however is updated with cross point events
   during the traversing, what makes me a bit nervous. The other point here is that
   cross points should be checked for existence pretty often. avl_tree seem to handle
   all this quite naturally.*/
   class EventQueue {
   public:
      //! The constructor of the event queue
                        EventQueue(const segmentlist&, const segmentlist&);
      //! EventQueue traverser for crossing points
      void              swipe4cross(SweepLine&);
      //! EventQueue traverser for binding points
      void              swipe4bind(SweepLine&, BindCollection&);
      //! The destructor
                       ~EventQueue();
   private:
      //!The callback function used to sort the events in the _equeue
      static int        E_compare( const void*, const void*, void* );
      //!Callback used by avl_destroy to delete the actual event objects
      static void       destroy_event(void*, void*);
      //! An AVL tree structure containing the actual event queue
      avl_table*        _equeue;
   };

   //===========================================================================
   // SweepLine
   //===========================================================================
   /*! The class implements so-called sweep line used in the Bentley-Ottmann algorithm.
   It uses an external implementation of Adelson-Velskii & Landis balanced binary tree - 
   avl_table to store the segments in up-to-down order, as the line swipes from left to 
   right. */
   class SweepLine {
   public:
      //! Default and the only constructor
                        SweepLine(const pointlist&, const pointlist&);
      //! The destructor of the sweep line
                       ~SweepLine(); 
      //!Add a segment to the sweep line
      void              add(plysegment*);
      //!Remove a segemnt form the sweep line
      void              remove(plysegment*);
      //!Determines whether the input two segments intersect
      CEvent*           intersect(plysegment*, plysegment*);
      //!Perform the swap of the input segments
      void              swap(plysegment*&, plysegment*&);
      //! set the _current_param
      void              set_curE(const Event* cE) {_curE = cE;};
   private:
      //!The callback function used as a sort creteria by the AVL tree
      static int        compare_seg(const void*, const void*, void*);
      //!Determines the lexicographical order of two points comparing Y first. 
      static int        yxorder(const TP*, const TP*);

      const TP*         joiningSegments(plysegment*, plysegment*, float, float);
      CEvent*           oneLineSegments(plysegment*, plysegment*);
//      CEvent*           coinsideCheck(plysegment*, plysegment*, float, float);
      float             getLambda( const TP*, const TP*, const TP*);
      TP*               getMiddle(const TP*, const TP*);

      //! An AVL tree structure containing the segments currently in the sweep line
      avl_table*        _tree;
      //!Current x position of the sweep line
      static const Event*     _curE;
      const  pointlist  _plist0;
      const  pointlist  _plist1;
   };
         
   typedef std::list<pointlist*> pcollection; // point list collection
   //===========================================================================
   /*!This class is the top level wrapper or the interface to all logic 
   operations with shapes. As a general all logic operations are following
   the same procedural path:
   - find all crossing points between the input polygons
   - prepare the data for new shape generation
   - generate the new shape as a result of a certain logic operation
   
   So the class constructor is executing the first two steps, manufacturing the 
   "raw" product #_shape1 and #_shape2. The user has to call afterwards the actual
   logic operation method that is producing the final result. Most if not all 
   logic operations can produce a collection of polygons, that's why a new 
   container type pcollection is introduced. If more than one operation is 
   desired, the user has to call reset_visited(), to prepare the #_shape1/#_shape2
   structures for another traversing.\n It has to be noted that in the general 
   the input polygons and repectively _poly1 and _poly2 fields are not 
   interchangable. For example polyA ANDNOT polyB produces different result from
   polyB ANDNOT polyA. Of course for some operations (AND, OR) that restriction 
   does not apply. */
   class logic {
   public:
      //! The class constructor preparing all data fields
                        logic(const pointlist&, const pointlist&);
      //! Perform logic AND and returns the result in plycol
      bool              AND(pcollection&);
      //! Perform logic ANDNOT and returns the result in plycol
      bool              ANDNOT(pcollection&);
      //! Perform logic OR and returns the result in plycol
      bool              OR(pcollection&);
      //! Prepare #_shape1 and #_shape2 data fields for reuse
      void              reset_visited();
   private:
      //! Convert a polygon with hole to simple polygon
      pointlist*        hole2simple(const pointlist&, const pointlist&);
      //
      VPoint*           getFirstOutside(const pointlist&, VPoint*);
      //! The first input polygon
      const pointlist&  _poly1;
      //! The second input polygon
      const pointlist&  _poly2;
      //! The raw data, corresponding to _poly1, used by all logic methods
      VPoint*           _shape1;
      //! The raw data, corresponding to _poly2, used by all logic methods
      VPoint*           _shape2;
      //! Number of crossing points found from the constructor
      unsigned          _crossp;
   };
   
   class BindSegment {
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
   
   class BindCollection {
   public:
      void              update_BL(plysegment*, Event*);
      BindSegment*      get_highest();
   private:
      typedef std::list<BindSegment*> BindList;
      bool              is_shorter(unsigned segno, real dist);
      BindList          _blist;
   };
}
#endif
