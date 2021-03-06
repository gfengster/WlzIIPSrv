#ifndef WLZ_EXP_TYPE_PARSER_H
#define WLZ_EXP_TYPE_PARSER_H
#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _WlzExpTypeParser_h[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         WlzExpTypeParser.h
* \author       Bill Hill
* \date         October 2011
* \version      $Id$
* \par
* Address:
*               MRC Human Genetics Unit,
*               MRC Institute of Genetics and Molecular Medicine,
*               University of Edinburgh,
*               Western General Hospital,
*               Edinburgh, EH4 2XU, UK.
* \par
* Copyright (C), [2012],
* The University Court of the University of Edinburgh,
* Old College, Edinburgh, UK.
* 
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be
* useful but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
* PURPOSE.  See the GNU General Public License for more
* details.
*
* You should have received a copy of the GNU General Public
* License along with this program; if not, write to the Free
* Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
* Boston, MA  02110-1301, USA.
* \brief	A union and type definition for flex and bison in
* 		parsing Woolz expressions for the Woolz IIP server.
* \ingroup	WlzIIPServer
*/

#ifdef __cplusplus
extern "C"
{
#endif

#include "WlzExpression.h"

typedef union _WlzExpTypeParser
{
  int 		i;
  unsigned int 	u;
  double	d;
  WlzExpCmpType	cmp;
  WlzObject	*obj;
  WlzExp 	*exp;
  WlzExpIdxList *idx_lst;
  WlzExpIdxRange *idx_rng;
}WlzExpTypeParser;

#define YYSTYPE WlzExpTypeParser

#ifdef __cplusplus
}
#endif

#endif /* WLZ_EXP_TYPE_PARSER_H */
