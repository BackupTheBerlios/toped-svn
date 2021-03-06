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
//        Created: Mon Mar 02 2009
//     Originator: Sergey Gaitukevich - gaitukevich.s@toped.org.uk
//    Description: Reader of Mentor Graphics Calibre drc errors files
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================
//      Comments :
//===========================================================================

#if !defined(CALBR_READER_H_INCLUDED)
#define CALBR_READER_H_INCLUDED

#include <fstream>
#include <string>
#include <vector>
#include <wx/wx.h>

#include "../tpd_DB/datacenter.h"

namespace Calbr
{

struct coord
{
	long x, y;
};

struct edge
{
	real x1, y1, x2, y2;
};

typedef std::vector <Calbr::coord> CoordsVector;
typedef std::vector <Calbr::edge> EdgesVector;

class drcEdge
{
public:
	drcEdge(long ordinal) { _ordinal = ordinal;};
	void				addCoord(long x1, long y1, long x2, long y2);
	edge*				coords() {return &_coords;};
	long				ordinal() {return _ordinal;};
	void				showError(laydata::tdtdesign* atdb, word la);
	static long		_precision;
private:
	edge				_coords;
	long				_ordinal;
};

class drcPolygon
{
public:
	drcPolygon(long ordinal) { _ordinal = ordinal;};
	void				addCoord(long x, long y);
	pointlist*		coords() {return &_coords;};
	long				ordinal() {return _ordinal;};
	void				showError(laydata::tdtdesign* atdb, word la);
	static long		_precision;
private:
	pointlist		_coords;
	long				_ordinal;

};

class drcRuleCheck
{
public:
	drcRuleCheck(const std::string &name);

	std::string ruleCheckName()	const {return _ruleCheckName;}
	std::string timeStamp()			const {return _timeStamp;}
	long			curResCount()		const {return  _curResCount;}
	long			origResCount()		const {return _origResCount;}
	std::vector <Calbr::drcPolygon>* polygons() {return &_polygons;};
	std::vector <Calbr::drcEdge>* edges() {return &_edges;};
	void			setTimeStamp(const std::string &timeStamp);
	void			setCurResCount(int curResCount);
	void			setOrigResCount(int origResCount);
	void			addDescrString(const std::string & str);
	void			addPolygon(const Calbr::drcPolygon &poly);
	void			addEdge(const Calbr::drcEdge &theEdge);
private:
	long			_curResCount; //current result count
	long			_origResCount;//original result count
	std::string _ruleCheckName;
	std::string _timeStamp;
	std::string _header;
	std::vector <std::string> _descrStrings;
	std::vector <Calbr::drcPolygon> _polygons;
	std::vector <Calbr::drcEdge> _edges;
};

typedef std::vector <Calbr::drcRuleCheck*> RuleChecksVector;

class CalbrFile
{
public:
	CalbrFile(const std::string &fileName);
	~CalbrFile();

	void					ShowResults();
	void					ShowError(const std::string & error, long  number);
	RuleChecksVector* Results() {return &_RuleChecks;};
	bool					isOk(void)	{return _ok;}
private:
	FILE*          _calbrFile;
	std::string    _fileName;
	std::string    _cellName;
	long				_precision;

	std::ifstream	_inFile;
	bool				parse();
	RuleChecksVector _RuleChecks;
	bool				_ok;
	laydata::tdtdesign* _ATDB;

};

wxString convert(int number, long precision);
}
#endif //CALBR_READER_H_INCLUDED