#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _TIL_cc[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         TIL.cc
* \author       Ruven Pillay, Bill Hill
* \date         June 2008
* \version      $Id$
* \par
* Address:
*               MRC Human Genetics Unit,
*               MRC Institute of Genetics and Molecular Medicine,
*               University of Edinburgh,
*               Western General Hospital,
*               Edinburgh, EH4 2XU, UK.
* \par
* Copyright (C) 2006 Ruven Pillay.
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
* \brief	IIP TIL Command Handler Class Member Function.
* \ingroup	WlzIIPServer
*/

#include "Log.h"
#include "Task.h"

using namespace std;



void TIL::run( Session* session, std::string argument ){

  int resolution, start_tile, end_tile;
  this->session = session;
  LOG_INFO("TIL handler reached");
  checkImage();
  openIfWoolz();
  LOG_COND_INFO(command_timer.start());
  /* Parse the argument list of the form 'resolution,range'
   */
  int delimitter = argument.find( "," );
  string tmp = argument.substr( 0, delimitter );
  resolution = atoi( tmp.c_str() );
  argument = argument.substr( delimitter + 1, argument.length() );

  delimitter = argument.find( "-" );
  tmp = argument.substr( 0, delimitter );
  start_tile = atoi( tmp.c_str() );
  argument = argument.substr( delimitter + 1, argument.length() );
	  
  if( argument.length() ){
    tmp = argument.substr( 0, argument.length() );
    end_tile = atoi( tmp.c_str() );
  }
  else end_tile = start_tile;

  // Make sure our range is logical
  if( end_tile < start_tile ) end_tile = start_tile;

  /* But we don't necessarily want all of these tiles. The spec requires
     us to return the tiles within the square formed by the first and
     last tiles. So, create a list of the tiles we need.
   */

  // Calculate the number of tiles at the requested resolution
  unsigned int im_width = (*session->image)->getImageWidth();
  unsigned int im_height = (*session->image)->getImageHeight();
  int num_res = (*session->image)->getNumResolutions();

  int requested_res = resolution;

  // Calculate the image width and height for this resolution
  for( int i=0; i < (num_res - requested_res - 1); i++){
    im_width = (int) im_width / 2;
    im_height = (int) im_height / 2;
  }

  unsigned int tile_width = (*session->image)->getTileWidth();
  unsigned int tile_height = (*session->image)->getTileHeight();
  unsigned int rem_x = im_width % tile_width;
  unsigned int rem_y = im_height % tile_height;
  int ntlx = (im_width / tile_width) + (rem_x == 0 ? 0 : 1);
  int ntly = (im_height / tile_height) + (rem_y == 0 ? 0 : 1);

  int startx = start_tile % ntlx;
  int starty = (int) start_tile / ntlx;
  int endx = end_tile % ntlx;
  int endy = (int) end_tile / ntlx;

  /* Our end tile can be 'behind' the end tile, so swap them over
     to make sure our rectangle is properly formed
   */
  if( endx < startx ){
    int tmp = startx;
    startx = endx;
    endx = tmp;
  }
  LOG_INFO("TIL :: resolution requested: " << resolution);
  LOG_INFO("TIL :: total tiles horizontally: " << ntlx <<
            ", vertically: " << ntly);
  LOG_INFO("TIL :: start tile: " << start_tile << ", end tile: " << end_tile);
  LOG_INFO("TIL :: Rectangle: " << startx << "," << starty << " - " <<
            endx << "," << endy);

  /* Only send our MIME type once
   */
  if( (endx >= startx) && (endy >= starty) ){
    session->out->printf( "Content-type: application/vnd.netfpx\r\n\r\n" );
  }


  for( int i = startx; i <= endx; i++ ){
    for( int j = starty; j <= endy; j++ ){

      int n = i + (j*ntlx);

      // Get our tile using our tile manager
      TileManager tilemanager( session->tileCache, *session->image, session->jpeg, session->png);
      RawTile rawtile = tilemanager.getTile( resolution, n, session->view->xangle,
					     session->view->yangle, JPEG );

      int len = rawtile.dataLength;


      LOG_INFO("TIL :: Sending tile " << n << " at: " << i << "," << j);
      LOG_INFO("TIL :: Number of channels per sample is " <<
                rawtile.channels);
      LOG_INFO("TIL :: Raw data bits per channel is " << rawtile.bpc);
      LOG_INFO("TIL :: Raw data length is " << len);

      /* The IIP compression type. Set a default no compression type

         The compression type is a 32 bit unsigned int:
	 0x0: none (8 bit)
	 0x1: single colour compression
	 0x2: JPEG compression
	 0x3: none 16 bit  -- not a part of the IIP specification version 1.05
	 0xFFFFFFFF: invalid tile
      */
      unsigned char compType[4] = { 0x00,0x00,0x00,0x00 };


      /* Do JPEG compression if we have an 8 bit image and set the IIP compression type
       */
      if( rawtile.bpc == 8 ) compType[0] = 0x02;
      else if( rawtile.bpc == 16 ) compType[0] = 0x03;

      LOG_INFO("TIL :: Compressed tile size is " << len);


      /* Send the tile prefix, indicating which resolution,
	 tile number and data length
       */
      char buf[1024];
      snprintf( buf, 1024, "Tile,%d,%d,0/%d:", resolution, n, len + 8 );
      session->out->printf( (const char*) buf );

      /* Send out the IIP compression type
       */
      if( session->out->putStr( (const char*) compType, 4 ) != 4 ){
	LOG_ERROR("TIL :: Error writing compression type");
      }

      /* Send the compression subtype defined within the FlashPix specification

         The 4 bytes represent the following:
	 i)   Interleave Type
	   - 0x00 for 8x8 block interleaving or 0x01 for separate scans
	 ii)  Chroma Subsampling
	   - 0x11 or 0x22 etc.
	 iii) Internal Colour Conversion
	   - 0x00 for none or 0x01
	 iv)  JPEG table selector
	   - index of previously downloaded table or 0x00 for table
  	     included in datastream
      */

      unsigned char compSubType[4] = { 0x00,0x11,0x00,0x00 };
      if( session->out->putStr( (const char*) compSubType, 4 ) != 4 ){
	LOG_ERROR("TIL :: Error writing compression sub-type ");
      }

      /* Send the actual tile data
       */
      if( session->out->putStr( (const char*) rawtile.data, len ) != len ){
        LOG_ERROR("TIL :: Error writing jpeg tile");
      }

      /* And finally send the CRLF terminator for each tile
       */
      //session->out->printf( "\r\n" ); //causes incorrect packet length, that results in crash, Z Husz 16/04/2010
      if( session->out->flush()  == -1 ) {
	LOG_ERROR("TIL :: Error flushing jpeg tile");
      }
    } // End of for( starty, endy )
  } // End of for( startx, endx )


  if( session->out->flush()  == -1 ) {
    LOG_ERROR("TIL :: Error flushing jpeg tile");
  }


  /* Inform our response object that we have sent something to the client
   */
  session->response->setImageSent();


  // Total TIL response time
  LOG_INFO("TIL :: Total command time " << command_timer.getTime() << "us");
}
