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
//        Created: Wed Dec 26 2001
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: TELL data types
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================

#ifndef CTM_H
#define CTM_H

#include <string>
#include <vector>
#include <algorithm>
#include "../tpd_common/ttt.h"
#include "../tpd_DB/tedat.h"

#define NUMBER_TYPE(op) ((op > telldata::tn_void) && (op < telldata::tn_bool) && !(op & telldata::tn_listmask))
#define TLISTOF(op) (op | telldata::tn_listmask)
#define TLISALIST(op) (op & telldata::tn_listmask)
#define TLCOMPOSIT_TYPE(op) (op > telldata::tn_composite)
#define TLUNKNOWN_TYPE(op) (op == telldata::tn_composite)

//=============================================================================
// TELL types
//=============================================================================
namespace telldata {
   typedef unsigned int typeID;
   const typeID tn_NULL       = 0 ;
   const typeID tn_void       = 1 ;
   const typeID tn_int        = 2 ;
   const typeID tn_real       = 3 ;
   const typeID tn_bool       = 4 ;
   const typeID tn_string     = 5 ;
   const typeID tn_layout     = 6 ;
   const typeID tn_composite  = 10;
   const typeID tn_pnt        = 11;
   const typeID tn_box        = 12;
   const typeID tn_bnd        = 13;
   const typeID tn_usertypes  = 16;
   const typeID tn_listmask = typeID(1) << (8 * sizeof(typeID) - 1);

   class tell_var;
   class tell_type;
   class ttint;
   class argumentID;

   typedef std::pair<std::string, typeID>    structRECID;
   typedef std::pair<std::string, tell_var*> structRECNAME;
   typedef std::deque<structRECID>           recfieldsID;
   typedef std::deque<structRECNAME>         recfieldsNAME;
   typedef std::map<std::string, tell_type*> typeMAP;
   typedef std::map<std::string, tell_var* > variableMAP;
   typedef std::vector<tell_var*>            memlist;
   typedef std::deque<argumentID*>           argumentQ;
   typedef std::stack<telldata::tell_var*>   operandSTACK;
   typedef std::deque<telldata::tell_var*>   UNDOPerandQUEUE;
   

   //==============================================================================
   /*Every block (parsercmd::cmdBLOCK) defined maintains a table (map) to the
     locally defined user types in a form <typename - tell_type>. Every user type
     (telldata::tell_type), maintains a table (map) to the user defined types
     (telldata::tell_type) used in this type in a form <ID - tell_type. The latter is
     updated by addfield method. Thus the tell_type can execute its own copy
     constructor
   */
   class tell_type {
   public:
                           tell_type(typeID ID) : _ID(ID) {assert(TLCOMPOSIT_TYPE(ID));}
      bool                 addfield(std::string, typeID, const tell_type* utype);
      tell_var*            initfield(const typeID) const;
      const tell_type*     findtype(const typeID) const;
      const recfieldsID&   fields() const    {return _fields;}
      const typeID         ID() const        {return _ID;}
      typedef std::map<const typeID, const tell_type*> typeIDMAP;
   protected:
      typeID               _ID;
      recfieldsID          _fields;
      typeIDMAP            _tIDMAP;
   };

   //==============================================================================
   class point_type : public tell_type {
   public:
                           point_type();
   };

   //==============================================================================
   class box_type : public tell_type {
   public:
                           box_type(point_type*);
   };

   //==============================================================================
   class bnd_type : public tell_type {
   public:
                           bnd_type(point_type*);
   };

   //==============================================================================
   class tell_var {
   public:
                           tell_var(typeID ID) : _ID(ID) {}
      virtual tell_var*    selfcopy() const = 0;
      virtual void         echo(std::string&, real) = 0;
      virtual const typeID get_type() const {return _ID;}
      virtual void         assign(tell_var*) = 0;
      virtual tell_var*    field_var(char*& fname) {return NULL;}
      virtual             ~tell_var() {};
   protected:
      typeID              _ID;
   };

   //==============================================================================
   class ttreal:public tell_var {
   public:
                           ttreal(real  num=0.0) : tell_var(tn_real), _value(num) {}
                           ttreal(const ttreal& cobj) :
                                          tell_var(tn_real), _value(cobj.value()) {}
      const ttreal&        operator =(const ttreal&);
      const ttreal&        operator =(const ttint&);
      void                 echo(std::string&, real);
      void                 assign(tell_var*);
      real                 value() const        {return _value;};
      void                 uminus()             {_value  = -_value;   };
      tell_var*            selfcopy() const     {return new ttreal(_value);};
      friend class ttpnt;
   private:
      real  _value;
   };

   //==============================================================================
   class ttint:public tell_var {
   public:
                           ttint(int4b  num = 0) : tell_var(tn_int), _value(num) {}
                           ttint(const ttint& cobj) :
                                          tell_var(tn_int), _value(cobj.value()) {}
      const ttint&         operator =(const ttint&);
      const ttint&         operator =(const ttreal&);
      void                 echo(std::string&, real);
      void                 assign(tell_var*);
      int4b                value() const        {return _value;};
      void                 uminus()             {_value  = -_value;   };
      tell_var*            selfcopy() const     {return new ttint(_value);};
   protected:
      int4b               _value;
   };

   //==============================================================================
   class ttbool:public tell_var {
   public:
                           ttbool(bool value = false) :
                                                tell_var(tn_bool), _value(value) {}
                           ttbool(const ttbool& cobj) :
                                         tell_var(tn_bool), _value(cobj.value()) {};
      const ttbool&        operator = (const ttbool&);
      void                 echo(std::string&, real);
      void                 assign(tell_var*);
      bool                 value() const        {return _value;};
      tell_var*            selfcopy() const     {return new ttbool(_value);};
      void                 AND(bool op)         {_value = _value && op;};
      void                 OR(bool op)          {_value = _value || op; };
   private:
      bool  _value;
   };

   //==============================================================================
   class ttstring: public tell_var {
   public:
                           ttstring() : tell_var(tn_string) {}
                           ttstring(char* value) :
                                              tell_var(tn_layout), _value(value) {}
                           ttstring(const std::string& value):
                                              tell_var(tn_string), _value(value) {};
                           ttstring(const ttstring& cobj) :
                                        tell_var(tn_string), _value(cobj.value()){};
      const ttstring&      operator = (const ttstring&);
      void                 echo(std::string&, real);// {wstr += _value;};
      void                 assign(tell_var*);
      tell_var*            selfcopy() const    {return new ttstring(_value);}
      const std::string    value() const       {return _value;};
   private:
      std::string         _value;
   };

   //==============================================================================
   class ttlayout: public tell_var {
   public:
                           ttlayout(): tell_var(tn_layout), _data(NULL),
                                                      _layer(65535), _selp(NULL) {};
                           ttlayout(laydata::tdtdata* pdat, word lay, SGBitSet* selp = NULL):
                             tell_var(tn_layout), _data(pdat), _layer(lay), _selp(selp) {};
                           ttlayout(const ttlayout& cobj);
      const ttlayout&      operator = (const ttlayout&);
      void                 echo(std::string&, real);
      void                 assign(tell_var*);
      tell_var*            selfcopy() const {return new ttlayout(*this);};
      laydata::tdtdata*    data() const     {return _data;};
      word                 layer() const    {return _layer;};
      SGBitSet*            selp() const     {return _selp;};
                          ~ttlayout()       {if (_selp) delete _selp;};
   private:
      laydata::tdtdata*   _data;
      word                _layer;
      SGBitSet*           _selp; // selected points;
   };

   //==============================================================================
   class ttlist:public tell_var {
   public:
                           ttlist(typeID ltype): tell_var(ltype) {};
                           ttlist(const ttlist& cobj);
      const ttlist&        operator = (const ttlist&);
      void                 echo(std::string&, real);
      void                 assign(tell_var*);
      tell_var*            selfcopy() const  {return new ttlist(*this);};
      const typeID         get_type() const  {return _ID | tn_listmask;};
      memlist              mlist() const     {return _mlist;};
      void                 add(tell_var* p) {_mlist.push_back(p);};
      void                 reserve(unsigned num) {_mlist.reserve(num);};
      void                 reverse()         {std::reverse(_mlist.begin(), _mlist.end());};
      unsigned             size() const      {return _mlist.size();};
                          ~ttlist();
   private:
      memlist             _mlist;    // the list itself
   };

   //==============================================================================
   class user_struct : public tell_var {
   public:
                           user_struct(const typeID ID) : tell_var(ID) {};
                           user_struct(const tell_type*);
                           user_struct(const tell_type*, operandSTACK&);
                           user_struct(const user_struct&);
                          ~user_struct();
      tell_var*            selfcopy() const  {return new user_struct(*this);}
      void                 echo(std::string&, real);
      void                 assign(tell_var*);
      tell_var*            field_var(char*& fname);
   protected:
      recfieldsNAME        _fieldList;
   };

   //==============================================================================
   // Don't destruct _x and _y here. They are just pointing to the structures in
   // the parent _fieldList and obviously should be destroyed there
   class ttpnt : public user_struct {
   public:
                           ttpnt (real x=0, real y=0);
                           ttpnt(const ttpnt&);
                           ttpnt(operandSTACK& OPStack);
                           tell_var*            selfcopy() const    {return new ttpnt(*this);}
      void                 echo(std::string&, real);
      void                 assign(tell_var*);
      const real           x() const           {return _x->value();}
      const real           y() const           {return _y->value();}
      void                 scale(real sf)      {_x->_value *= sf;_y->_value *= sf;};
      void                 set_x(const real x) {_x->_value = x; }
      void                 set_y(const real y) {_y->_value = y; }
      const ttpnt&         operator = (const ttpnt&);
   private:
      ttreal*              _x;
      ttreal*              _y;
   };

   //==============================================================================
   // Don't destruct _p1 and _p1 here. They are just pointing to the structures in
   // the parent _fieldList and obviously should be destroyed there
   class ttwnd : public user_struct {
   public:
                           ttwnd( real bl_x=0.0, real bl_y=0.0,
                                  real tr_x=0.0, real tr_y=0.0);
                           ttwnd( ttpnt tl, ttpnt br);
                           ttwnd(operandSTACK& OPStack);
                           ttwnd(const ttwnd& cobj);
      tell_var*            selfcopy() const    {return new ttwnd(*this);};
      void                 echo(std::string&, real);
      void                 assign(tell_var*);
      const ttpnt&         p1() const          {return *_p1;};
      const ttpnt&         p2() const          {return *_p2;};
      void                 scale(real sf)      {_p1->scale(sf); _p1->scale(sf);};
      const ttwnd&         operator = (const ttwnd&);
      void                 normalize(bool&, bool&);
      void                 denormalize(bool, bool);
   private:
      ttpnt*               _p1;
      ttpnt*               _p2;
   };

   //==============================================================================
   // Don't destruct _p, _rot etc. here. They are just pointing to the structures in
   // the parent _fieldList and obviously should be destroyed there
   class ttbnd : public user_struct {
   public:
                           ttbnd( real p_x=0.0, real p_y=0.0,
                                  real rot=0.0, bool flx=false, real scale = 1.0);
                           ttbnd( ttpnt p, ttreal rot, ttbool flx, ttreal sc);
                           ttbnd(operandSTACK& OPStack);
                           ttbnd(const ttbnd& cobj);
      tell_var*            selfcopy() const    {return new ttbnd(*this);};
      void                 echo(std::string&, real);
      void                 assign(tell_var*);
      const ttpnt&         p() const           {return *_p;}
      const ttreal&        rot() const         {return *_rot;}
      const ttbool&        flx() const         {return *_flx;}
      const ttreal&        sc() const          {return *_sc;}
      const ttbnd&         operator = (const ttbnd&);
   private:
      ttpnt*               _p;
      ttreal*              _rot;
      ttbool*              _flx;
      ttreal*              _sc;
   };

   //==============================================================================
   class argumentID {
   public:
                           argumentID(telldata::typeID ID = telldata::tn_NULL) :
                                                          _ID(ID), _command(NULL){};
                           argumentID(argumentQ* child, void* cmd) :
                                                         _ID(telldata::tn_composite),
                                                    _child(*child), _command(cmd) {};
                           argumentID(const argumentID&);
                           ~argumentID();//               {_child.clear();}
      void                 toList(bool);
      void                 adjustID(const argumentID&);
      void                 userStructCheck(const telldata::tell_type&, bool);
      void                 userStructListCheck(const telldata::tell_type&, bool);
      telldata::typeID     operator () () const        {return _ID;}
      const argumentQ&     child() const               {return _child;}
   private:
      telldata::typeID     _ID;
      argumentQ            _child;
      void*                _command;
   };

   void argQClear(argumentQ*);
   std::string echoType( const telldata::typeID, const telldata::typeMAP*);
}

#endif
