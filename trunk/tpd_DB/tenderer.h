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
//   This file is a part of Toped project (C) 2001-2009 Toped developers    =
// ------------------------------------------------------------------------ =
//           $URL$
//        Created: Sun Jan 11 2009
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: OpenGL renderer
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================
/*!
The idea behind the Toped renderer:

*/
#ifndef TENDERER_H
#define TENDERER_H

#include <GL/glew.h>

#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include "drawprop.h"

typedef std::list<word> TeselVertices;

class TeselChunk {
   public:
                        TeselChunk(const TeselVertices&, GLenum, unsigned);
                        TeselChunk(const TeselChunk*, unsigned);
                        TeselChunk(const int*, unsigned, unsigned);
                       ~TeselChunk();
      GLenum            type() const      {return _type;}
      word              size() const      {return _size;}
      const unsigned*   index_seq() const {return _index_seq;}
   private:
      unsigned*         _index_seq;  // index sequence
      word              _size;       // size of the index sequence
      GLenum            _type;
};

typedef std::list<TeselChunk*> TeselChain;

class TeselTempData {
   public:
                        TeselTempData(unsigned);
                        TeselTempData(TeselChain* tc);
      void              setChainP(TeselChain* tc)  {_the_chain = tc;}
      void              newChunk(GLenum type)      {_ctype = type; _cindexes.clear();}
      void              newIndex(word vx)          {_cindexes.push_back(vx);}
      void              storeChunk();
      word              num_ftrs()                 { return _all_ftrs;}
      word              num_ftfs()                 { return _all_ftfs;}
      word              num_ftss()                 { return _all_ftss;}

   private:
      TeselChain*       _the_chain;
      GLenum            _ctype;
      TeselVertices     _cindexes;
      word              _all_ftrs;
      word              _all_ftfs;
      word              _all_ftss;
      unsigned          _offset;
};

class TeselPoly {
   public:
                        TeselPoly(const int4b* pdata, unsigned psize);
                       ~TeselPoly();
      TeselChain*       tdata()                    { return &_tdata;  }
      word              num_ftrs()                 { return _all_ftrs;}
      word              num_ftfs()                 { return _all_ftfs;}
      word              num_ftss()                 { return _all_ftss;}
      void              num_indexs(unsigned&, unsigned&, unsigned&);
      static GLUtriangulatorObj* tenderTesel; //! A pointer to the OpenGL object tesselator
#ifdef WIN32
      static GLvoid CALLBACK teselVertex(GLvoid *, GLvoid *);
      static GLvoid CALLBACK teselBegin(GLenum, GLvoid *);
      static GLvoid CALLBACK teselEnd(GLvoid *);
#else
      static GLvoid     teselVertex(GLvoid *, GLvoid *);
      static GLvoid     teselBegin(GLenum, GLvoid *);
      static GLvoid     teselEnd(GLvoid *);
#endif
   private:
      TeselChain        _tdata;
      word              _all_ftrs;
      word              _all_ftfs;
      word              _all_ftss;
};

/**
*   holds box representation - The same four points will be used for the
*   contour as well as for the fill
*/
class TenderObj {
   public:
                        TenderObj(int4b* pdata, unsigned psize) : _cdata(pdata), _csize(psize) {}
      virtual          ~TenderObj() {};
      int4b*            cdata()     {return _cdata;}
      unsigned          csize()     {return _csize;}
   protected:
      int4b*            _cdata;  // contour data
      unsigned          _csize;
};

class TenderRefBox : public TenderObj {
   public:
                        TenderRefBox(int4b* pdata) : TenderObj(pdata, 4) {}
      virtual          ~TenderRefBox() {delete [] _cdata;}
};
/**
*   holds polygon representations - the contour will be drawn using the
*   inherited _cdata holder. The chains of point indexes resulting from the
*   tesselation are stored in _tdata. They will be used together with the _cdata
*   during the polygon fill
*/
class TenderPoly : public TenderObj {
   public:
                        TenderPoly(int4b* pdata, unsigned psize):TenderObj(pdata, psize), _tdata(NULL) {}
      void              setTeselData(TeselPoly* tdata) {_tdata = tdata;}
      virtual          ~TenderPoly(){};
      virtual TeselChain* tdata()              {return _tdata->tdata();}
   private:
      TeselPoly*        _tdata;
};

/**
*   holds wire representation - the contour and the fill - exactly as in the
*   inherited class. The _ldata stores the central line which is effectively
*   the original point list from tdtwire
*/
class TenderWire : public TenderPoly {
   public:
                        TenderWire(int4b*, unsigned, const word, bool);
      virtual          ~TenderWire();
      void              Tesselate();
      int*              ldata()                 {return _ldata;}
      unsigned          lsize()                 {return _lsize;}
      bool              center_line_only()      {return _center_line_only;}
      virtual TeselChain* tdata()               {return _tdata;}
   private:
      void              precalc(const word);
      DBbox*            endPnts(const word, word, word, bool);
      DBbox*            mdlPnts(const word, word, word, const word);
      int*              _ldata;  // central line data
      unsigned          _lsize;
      bool              _center_line_only;
      TeselChain*       _tdata;
};

/**
 *   holds the representation of selected objects and segments
 */
class TenderLine {
   public:
                        TenderLine(TenderObj*, const SGBitSet*);
                        TenderLine(TenderPoly*, const SGBitSet*);
                        TenderLine(TenderWire*, const SGBitSet*);
                       ~TenderLine();
      int*              ldata()                 {return _ldata;}
      unsigned          lsize()                 {return _lsize;}
   private:
      int4b*            _ldata;
      unsigned          _lsize;
      bool              _partial;
};

typedef std::list<TenderObj*>  SliceObjects;
typedef std::list<TenderPoly*> SlicePolygons;
typedef std::list<TenderWire*> SliceWires;
typedef std::list<TenderLine*> SliceLines;

/**
 *  Reference boxes
 */
class TenderRB {
   public:
                        TenderRB(const CTM&, const DBbox&);
      void              draw();
   private:
      CTM               _tmatrix;
      DBbox             _obox;
};

/**
 *  Translation View of the selected shapes
 */
class TenderTVB {
   public:
                        TenderTVB();
      void              add(TenderObj*, const SGBitSet*);
      void              add(TenderPoly*, const SGBitSet*);
      void              add(TenderWire*, const SGBitSet*);
      void              draw_lloops();
      void              draw_lines();
      void              draw_lsegments();
   private:
      SliceLines        _ln_data;
      SliceLines        _ll_data;
      SliceLines        _ls_data;
      unsigned          _num_ln_points;
      unsigned          _num_ll_points;
      unsigned          _num_ls_points;
      unsigned          _num_ln; // lines
      unsigned          _num_ll; // line loop
      unsigned          _num_ls; // line segment
};

/**
*  translation view - effectively a layer slice of the visible cell data
*/
class TenderTV {
   public:
      enum {fqss, ftrs, ftfs, ftss} NcvxTypes;
      enum {cont, line, cnvx, ncvx} ObjtTypes;
                        TenderTV(CTM&, bool, unsigned, unsigned);
                       ~TenderTV();
      TenderObj*        box  (int4b*);
      TenderPoly*       poly (int4b*, unsigned, TeselPoly*);
      TenderWire*       wire (int4b*, unsigned, word, bool);
      const CTM*        tmatrix()            {return &_tmatrix;}

      void              collect(int*, unsigned int*);
      void              draw();

      unsigned          num_total_points();
      unsigned          num_total_indexs();
   private:
      void              collectIndexs(unsigned int*, TeselChain*, unsigned*, unsigned*, unsigned);

      CTM               _tmatrix;
      // collected data lists
      SliceObjects      _cont_data; //! Countour data
      SliceWires        _line_data; //! Line data
      SliceObjects      _cnvx_data; //! Convex polygon data (Only boxes are here at the moment. TODO - all convex polygons)
      SlicePolygons     _ncvx_data; //! Non convex data
      // vertex related data
      unsigned          _alvrtxs[4]; //! array with the total number of vertexes
      unsigned          _alobjvx[4]; //! array with the total number of objects that will be drawn with vertex related functions
      GLsizei*          _sizesvx[4]; //! arrays of sizes for vertex sets
      GLsizei*          _firstvx[4]; //! arrays of first vertexes
      // index related data
      unsigned          _alindxs[4]; //! array with the total number of indexes
      unsigned          _alobjix[4]; //! array with the total number of objects that will be drawn with index related functions
      GLsizei*          _sizesix[4]; //! arrays of sizes for indexes sets
      GLuint*           _firstix[4]; //! arrays of first indexes
      // offsets in the VBO
      unsigned          _point_array_offset;
      unsigned          _index_array_offset;
      //
      bool              _filled;
};

class TenderLay {
   public:
      typedef std::list<TenderTV*> TenderTVList;
                        TenderLay(): _cslice(NULL), _num_total_points(0u), _num_total_indexs(0u) {};
                       ~TenderLay();
      TenderObj*        box  (int4b* pdata)  {return _cslice->box(pdata);}
      TenderPoly*       poly (int4b* pdata, unsigned psize, TeselPoly* tpoly) {return _cslice->poly(pdata, psize, tpoly);}
      TenderWire*       wire (int4b* pdata, unsigned psize, word width, bool center_only) {return _cslice->wire(pdata, psize, width, center_only);}

      void              newSlice(CTM&, bool);
      void              ppSlice(); //! Post process the slice
      void              draw(bool);
      void              collect(bool, GLuint, GLuint);
      unsigned          total_points() {return _num_total_points;}
      unsigned          total_indexs() {return _num_total_indexs;}

   private:
      TenderTVList      _layData;
      TenderTV*         _cslice;    //!Working variable pointing to the current slice
      unsigned          _num_total_points;
      unsigned          _num_total_indexs;
      GLuint            _pbuffer;
      GLuint            _ibuffer;
};

//-----------------------------------------------------------------------------
//
//! contains all the data across cells on a given layer
typedef std::map<word, TenderLay*> DataLay;
typedef std::map<word, TenderTVB*> DataSel;
typedef std::list<TenderRB*> TenderRBL;

class Tenderer {
   public:
                        Tenderer( layprop::DrawProperties* drawprop, real UU );
                       ~Tenderer();
      void              Grid( const real, const std::string );
      void              setLayer(word);
      void              setSdataContainer(word);
      void              pushCell(const CTM&, const DBbox&, bool, bool);
      void              popCTM()                               {_drawprop->popCTM(); _ctrans = _drawprop->topCTM();}
      void              box  (int4b* pdata)                    {_clayer->box(pdata);}
      void              box  (int4b*, const SGBitSet*);
      void              poly (int4b* pdata, unsigned psize, TeselPoly* tpoly)    {_clayer->poly(pdata, psize, tpoly);}
      void              poly (int4b*, unsigned, TeselPoly* , const SGBitSet*);
      void              wire (int4b*, unsigned, word);
      void              wire (int4b*, unsigned, word, const SGBitSet*);
      void              collect();
      void              draw();

      // temporary!
      void              initCTMstack()                {        _drawprop->initCTMstack()        ;}
      void              clearCTMstack()               {        _drawprop->clearCTMstack()       ;}
      void              setCurrentColor(word layno)   {        _drawprop->setCurrentColor(layno);}
      bool              layerHidden(word layno) const {return  _drawprop->layerHidden(layno)    ;}
      const CTM&        ScrCTM() const                {return  _drawprop->ScrCTM()              ;}
      const CTM&        topCTM() const                {return  _drawprop->topCTM()              ;}
      const DBbox&      clipRegion() const            {return  _drawprop->clipRegion()          ;}
      void              pushref(const laydata::tdtcellref* ref)
                                                      {        _drawprop->pushref(ref)          ;}
      byte              popref(const laydata::tdtcellref* ref)
                                                      {return  _drawprop->popref(ref)           ;}
   private:
      layprop::DrawProperties*   _drawprop;
      real              _UU;
      DataLay           _data;      //!All data for drawing
      TenderLay*        _clayer;    //!Working variable pointing to the current slice
      DataSel           _sdata;     //!Selected data - to be redrawn on top of the _data
      TenderTVB*        _sslice;    //!Selected shapes in the active cell
      TenderRBL         _oboxes;    //!All reference overlapping boxes
      TenderRBL         _osboxes;   //!All selected reference overlapping boxes
      CTM               _atrans;    //!The translation of the active cell
      CTM               _ctrans;    //!Working variable storing the current translation
      unsigned          _num_ogl_buffers;
      //
      GLuint*           _ogl_buffers;
};

void checkOGLError(std::string);

class HiResTimer {
   public:
      HiResTimer();
      void           report(std::string);
   private:
      timeval        _start_time;
      timeval        _end_time;
#ifdef WIN32
      // System frequency of timer for Windows.
      LARGE_INTEGER  _freq;
      LARGE_INTEGER  _inittime;
#endif

};

#endif //TENDERER_H
