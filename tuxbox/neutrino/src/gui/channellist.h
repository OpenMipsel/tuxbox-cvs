#ifndef __channellist__
#define __channellist__

/*
	$Id: channellist.h,v 1.80 2012/08/14 18:26:54 rhabarber1848 Exp $

	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <driver/framebuffer.h>
#include <driver/pig.h>
#include <gui/widget/menue.h>
#include <system/lastchannel.h>

#include <sectionsdclient/sectionsdclient.h>
#include <zapit/client/zapitclient.h>

#include <string>
#include <vector>

class CChannelList
{
	public:
		class CChannel
		{
			private:
				unsigned long long	last_unlocked_EPGid;
				std::string             name;
				t_satellite_position	satellitePosition;
				int         	        key;

			public:
				int         	number;
				t_channel_id    channel_id;
				CChannelEvent	currentEvent;
				CChannelEvent	nextEvent;
				inline const std::string & getName(void) const { return name; };

				// flag that tells if channel is staticly locked by bouquet-locking
				bool bAlwaysLocked;

				// constructor
				CChannel(const int key, const int number, const std::string& name, const t_satellite_position satellitePosition, const t_channel_id ids);

				friend class CChannelList;
		};

	private:
		CFrameBuffer		*frameBuffer;
		unsigned int		selected;
		unsigned int		tuned;
		CLastChannel		lastChList;
		unsigned int		liststart;
		unsigned int		listmaxshow;
		unsigned int		numwidth;
		int			fheight; // Fonthoehe Channellist-Inhalt
		int			theight; // Fonthoehe Channellist-Titel
		int			footerHeight;
		int			info_height;
		int			eventFont;
		int			ffheight;

		static CPIG *pig;

		std::string             name;
		std::vector<CChannel*>	chanlist;
		CZapProtection* 	zapProtection;

		int			full_width;
		int			width;
		int			height;
		int			x;
		int			y;
		int			pig_width;
		int			pig_height;
		int			infozone_width;
		int			infozone_height;

		CShortEPGData epgData;
		bool historyMode;
		bool usedInBouquet;
		bool displayNext;
		bool displayList;

		void paintDetails(unsigned int index);
		void clearItem2DetailsLine();
		void paintItem2DetailsLine(int pos);
		void paintItemDetailsBox();
		void paintItem(int pos);
		void updateSelection(unsigned int newpos);
		void paint();
		void paintHead();
		void paintButtonBar();
		void hide();
		void calcSize();
		void paint_pig(int x, int y, int w, int h);
		void paint_events(int index);
		CChannelEventList	evtlist;
		void readEvents(const t_channel_id channel_id);
		void showdescription(int index);
		std::vector<std::string> epgText;
		int emptyLineCount;
		void addTextToArray( const std::string & text );
		void processTextToArray(std::string text);

	public:
		enum
		{
			ADDITIONAL_OFF,
			ADDITIONAL_ON,
			ADDITIONAL_MTV
		};

		enum
		{
			FOOT_FREQ,
			FOOT_NEXT,
			FOOT_OFF
		};

		CChannelList(const char * const Name, bool historyMode = false, bool UsedInBouquet = false);
		~CChannelList();
		void addChannel(int key, int number, const std::string& name, const t_satellite_position satellitePosition, t_channel_id ids = 0); // UTF-8
		void addChannel(CChannel* chan);
		CChannel* getChannel( int number);
		CChannel* operator[]( uint index) {
			if (chanlist.size() > index) return chanlist[index];
			else return NULL;
		};
		int getKey(int);

		const char         * getName                   (void) const { return name.c_str(); };
		const std::string &  getActiveChannelName      (void) const; // UTF-8
		t_satellite_position getActiveSatellitePosition(void) const;
		int                  getActiveChannelNumber    (void) const;
		t_channel_id         getActiveChannel_ChannelID(void) const;

/*		CChannel *   getChannelFromChannelID(const t_channel_id channel_id); */

		void zapTo(int pos, bool forceStoreToLastChannels = false);
		void virtual_zap_mode(bool up);
		bool zapTo_ChannelID(const t_channel_id channel_id);
		bool adjustToChannelID(const t_channel_id channel_id);
		bool showInfo(int pos, int epgpos = 0);
		void updateEvents(void);
		int  numericZap(neutrino_msg_t key);
		int  show();
		int  exec();
		void quickZap(neutrino_msg_t key);
		int  hasChannel(int nChannelNr);
		void setSelected( int nChannelNr); // for adjusting bouquet's channel list after numzap or quickzap
		void clearTuned(void) { tuned = 0xfffffff; }

		int handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data);

		bool isEmpty() const;
		int getSize() const;
		int getSelectedChannelIndex() const;

};


#endif
