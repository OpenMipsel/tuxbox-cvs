/*
$Id: dsmcc.h,v 1.3 2004/02/15 01:02:58 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)


*/


#ifndef __DSMCC_H
#define __DSMCC_H 


void  decode_DSMCC_section (u_char *b, int len);


#endif



