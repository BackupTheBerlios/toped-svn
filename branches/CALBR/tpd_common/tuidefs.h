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
//        Created: Sun Apr 15 18:08:11 BST 2007
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: Toped User Interface enum definitions
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================

#ifndef TUIDEFS_H
#define TUIDEFS_H
namespace tui
{
   typedef enum
   {
      ID_WIN_TOPED  = 100 ,
      ID_WIN_BROWSERS     ,
      ID_WIN_GLSTATUS     ,
      ID_WIN_COMMAND      ,
      ID_WIN_LOG          ,
      ID_WIN_CANVAS       ,
      ID_TPD_CANVAS       ,
      ID_TPD_LAYERS       ,
      ID_TPD_CELLTREE     ,
      ID_GDS_CELLTREE     ,
      ID_CIF_CELLTREE     ,
		ID_DRC_CELLTREE     ,
      //Warning!!! Do not use IDs between ID_DUMMY_WIN and ID_DUMMY_WIN_END
      ID_DUMMY_WIN = 500  ,
      ID_DUMMY_WIN_END = 600
   } WX_WINDOW_IDS_TYPE;

   typedef enum  {
      TMFILE_NEW = 100    ,
      TMFILE_OPEN         ,
      TMFILE_INCLUDE      ,
      TMLIB_LOAD          ,
      TMLIB_UNLOAD        ,
      TMGDS_OPEN          ,
      TMGDS_IMPORT        ,
      TMGDS_EXPORTL       ,
      TMGDS_EXPORTC       ,
      TMGDS_TRANSLATE     ,
      TMGDS_CLOSE         ,
      // <- CIF stuff
      TMCIF_OPEN          ,
      TMCIF_TRANSLATE     ,
      TMCIF_EXPORTC       ,
      TMCIF_EXPORTL       ,
      TMCIF_CLOSE         ,
      TMFILE_SAVE         ,
      TMFILE_SAVEAS       ,
      TMPROP_SAVE         ,
      TMFILE_EXIT         ,

      TMEDIT_UNDO         ,
      TMEDIT_COPY         ,
      TMEDIT_MOVE         ,
      TMEDIT_DELETE       ,
      TMEDIT_ROTATE90     ,
      TMEDIT_FLIPX        ,
      TMEDIT_FLIPY        ,
      TMEDIT_POLYCUT      ,
      TMEDIT_MERGE        ,
      TMEDIT_RESIZE       ,

      TMVIEW_VIEWTOOLBAR  ,
      TMVIEW_VIEWSTATUSBAR,
      TMVIEW_ZOOMIN       ,
      TMVIEW_ZOOMOUT      ,
      TMVIEW_ZOOMALL      ,
      TMVIEW_PANLEFT      ,
      TMVIEW_PANRIGHT     ,
      TMVIEW_PANUP        ,
      TMVIEW_PANDOWN      ,
      TMVIEW_PANCENTER    ,

      TMCELL_NEW          ,
      TMCELL_OPEN         ,
      TMCELL_REMOVE       ,
      TMCELL_PUSH         ,
      TMCELL_POP          ,
      TMCELL_TOP          ,
      TMCELL_PREV         ,
      TMCELL_REF_M        ,
      TMCELL_REF_B        ,
      TMCELL_AREF_M       ,
      TMCELL_AREF_B       ,
      TMCELL_REPORTLAY    ,
      TMCELL_GROUP        ,
      TMCELL_UNGROUP      ,

      TMDRAW_BOX          ,
      TMDRAW_POLY         ,
      TMDRAW_WIRE         ,
      TMDRAW_TEXT         ,

      TMSEL_SELECT_IN     ,
      TMSEL_PSELECT_IN    ,
      TMSEL_SELECT_ALL    ,
      TMSEL_UNSELECT_IN   ,
      TMSEL_PUNSELECT_IN  ,
      TMSEL_UNSELECT_ALL  ,
      TMSEL_REPORT_SLCTD  ,

      TMSET_STEP          ,
      TMSET_AUTOPAN       ,
      TMSET_ZEROCROSS     ,
      TMSET_GRIDDEF       ,
      TMSET_GRID0         ,
      TMSET_GRID1         ,
      TMSET_GRID2         ,
      TMSET_CELLMARK      ,
      TMSET_CELLBOX       ,
      TMSET_TEXTMARK      ,
      TMSET_TEXTBOX       ,
      TMSET_MARKER0       ,
      TMSET_MARKER45      ,
      TMSET_MARKER90      ,
      TMSET_CURLONG       ,
      TMSET_MARKER        ,
      TMSET_HTOOLSIZE	  ,
      TMSET_HTOOLSIZE16	  ,
      TMSET_HTOOLSIZE24	  ,
      TMSET_HTOOLSIZE32	  ,
      TMSET_HTOOLSIZE48	  ,
      TMSET_VTOOLSIZE	  ,
      TMSET_VTOOLSIZE16	  ,
      TMSET_VTOOLSIZE24	  ,
      TMSET_VTOOLSIZE32	  ,
      TMSET_VTOOLSIZE48	  ,
      TMSET_UNDODEPTH     ,
      TMSET_DEFLAY        ,
      TMSET_DEFCOLOR      ,
      TMSET_DEFFILL       ,

      TMADD_RULER         ,
      TMCLEAR_RULERS      ,

		TMCADENCE_CONVERT	  ,
      TMGET_SNAPSHOT      ,

      TMHELP_ABOUTAPP     ,
      TBSTAT_ABORT        ,
      //Warning!!! Do not use IDs between TMDUMMY and TMDUMMY_END
      TMDUMMY        = 500   ,
      TDUMMY_TOOL    = 2000  ,
      TMDUMMY_LAYER  = 10000 ,
      TMDUMMY_END    = 11000
   } TOPED_MENUID;


   typedef enum  {
      ZOOM_WINDOW  = 0    ,
      ZOOM_WINDOWM        ,
      ZOOM_IN             ,
      ZOOM_OUT            ,
      ZOOM_LEFT           ,
      ZOOM_RIGHT          ,
      ZOOM_UP             ,
      ZOOM_DOWN           ,
      ZOOM_EMPTY          ,
      ZOOM_REFRESH
   } ZOOM_TYPE;

   typedef enum  {
      CM_CONTINUE  = 0    ,
      CM_ABORT            ,
      CM_RULER            , 
      CM_CHLAY            ,
      CM_CANCEL_LAST      ,
      CM_CLOSE            ,
      CM_AGAIN            ,
      CM_ROTATE           ,
      CM_FLIP
   } CONTEXT_MENU_TYPE;

   typedef enum {
      CNVS_POS_X          ,
      CNVS_POS_Y          ,
      CNVS_DEL_X          ,
      CNVS_DEL_Y          ,
      CNVS_SELECTED
   } CVSSTATUS_TYPE;

   typedef enum {
      STS_GRID0_ON        ,
      STS_GRID0_OFF       ,
      STS_GRID1_ON        ,
      STS_GRID1_OFF       ,
      STS_GRID2_ON        ,
      STS_GRID2_OFF       ,
      STS_CELLMARK_ON     ,
      STS_CELLMARK_OFF    ,
      STS_CELLBOX_ON      ,
      STS_CELLBOX_OFF     ,
      STS_TEXTMARK_ON     ,
      STS_TEXTMARK_OFF    ,
      STS_TEXTBOX_ON      ,
      STS_TEXTBOX_OFF     ,
      STS_AUTOPAN_ON      ,
      STS_AUTOPAN_OFF     ,
      STS_ZEROCROSS_ON    ,
      STS_ZEROCROSS_OFF   ,
      STS_ANGLE_0         ,
      STS_ANGLE_45        ,
      STS_ANGLE_90        ,
      STS_LONG_CURSOR     ,
      STS_SHORT_CURSOR
   } SETTINGSMENU_TYPE;

}

#endif //TUIDEFS_H
