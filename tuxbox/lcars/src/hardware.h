/***************************************************************************
    copyright            : (C) 2001 by TheDOC
    email                : thedoc@chatville.de
	homepage			 : www.chatville.de
	modified by			 : -
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*
$Log: hardware.h,v $
Revision 1.4  2002/03/03 22:57:59  TheDOC
lcars 0.20

Revision 1.4  2001/12/18 02:03:29  tux
VCR-Switch-Eventkram implementiert

Revision 1.3  2001/12/17 16:54:47  tux
Settings halb komplett

Revision 1.2  2001/12/16 22:36:05  tux
IP Eingaben erweitert

Revision 1.3  2001/12/11 13:38:44  TheDOC
new cdk-path-variables, about 10 new features and stuff

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#ifndef HARDWARE_H
#define HARDWARE_H

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <dbox/avs_core.h>
#include <ost/audio.h>
#include <dbox/fp.h>

#include "settings.h"

#define OUTPUT_FBAS 0
#define OUTPUT_RGB 1

#define VCR_STATUS_OFF 0
#define VCR_STATUS_ON 1
#define VCR_STATUS_16_9 2

class hardware
{
	int fblk;
	bool muted;
	int avs;
	settings *setting;
	bool vcr_on;
	bool old_DD_state;
	int old_fblk;
public:	
	hardware(settings *s);
	void hardware::setOutputMode(int i);
	void setfblk(int i);
	int getfblk();
	bool switch_vcr();
	bool vcrIsOn() { return vcr_on; }
	int getAvsStatus();
	int getVCRStatus();
	int getARatio();

	void switch_mute();
	bool isMuted() { return muted; }
	int vol_plus(int value);
	int vol_minus(int value);
	void fnc(int i);
	void shutdown();
	void reboot();
	void useDD(bool use);
};

#endif
