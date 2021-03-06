/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: pictureviewer.cpp,v 1.80 2012/08/14 18:19:53 rhabarber1848 Exp $

	MP3Player by Dirch
	
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/pictureviewer.h>
#include <gui/imageinfo.h>

#include <global.h>
#include <neutrino.h>

#include <daemonc/remotecontrol.h>

#include <driver/encoding.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/screen_max.h>

#ifdef ENABLE_GUI_MOUNT
#include <gui/nfs.h>
#endif
#include <gui/infoviewer.h>

#include <gui/widget/buttons.h>
#include <gui/widget/icons.h>
#include <gui/widget/menue.h>
#include <gui/widget/messagebox.h>

// remove this
#include <gui/widget/hintbox.h>

#include <gui/widget/helpbox.h>
#include <gui/widget/stringinput.h>

#include <system/settings.h>
#include <system/helper.h>

#include <algorithm>
#include <sys/stat.h>
#include <sys/time.h>

#define PICTUREVIEWER_START_SCRIPT CONFIGDIR "/pictureviewer.start"
#define PICTUREVIEWER_END_SCRIPT CONFIGDIR "/pictureviewer.end"

//------------------------------------------------------------------------
bool comparePictureByDate (const CPicture& a, const CPicture& b)
{
	return a.Date < b.Date ;
}
//------------------------------------------------------------------------
extern bool comparetolower(const char a, const char b); /* filebrowser.cpp */
//------------------------------------------------------------------------
bool comparePictureByFilename (const CPicture& a, const CPicture& b)
{
	return std::lexicographical_compare(a.Filename.begin(), a.Filename.end(), b.Filename.begin(), b.Filename.end(), comparetolower);
}
//------------------------------------------------------------------------

CPictureViewerGui::CPictureViewerGui()
{
	frameBuffer = CFrameBuffer::getInstance();

	visible = false;
	selected = 0;
	m_sort = FILENAME;
	m_viewer = new CPictureViewer();
	if(strlen(g_settings.picviewer_picturedir)!=0)
		Path = g_settings.picviewer_picturedir;
	else
		Path = "/";

	picture_filter.addFilter("png");
	picture_filter.addFilter("bmp");
	picture_filter.addFilter("jpg");
	picture_filter.addFilter("jpeg");
	picture_filter.addFilter("gif");
	picture_filter.addFilter("crw");
}

//------------------------------------------------------------------------

CPictureViewerGui::~CPictureViewerGui()
{
	delete m_viewer;
}

//------------------------------------------------------------------------
int CPictureViewerGui::exec(CMenuTarget* parent, const std::string & /*actionKey*/)
{
	selected = 0;
	width = w_max (710, 30);
	height = h_max (570, 0);
	sheight      = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight();
	buttonHeight = std::max(25, sheight);
	frameBuffer->getIconSize(NEUTRINO_ICON_MP3, &ticonwidth, &ticonheight);
	theight      = std::max(ticonheight, g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight());
	fheight      = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	listmaxshow = (height-theight-2*buttonHeight)/(fheight);
	height = theight+2*buttonHeight+listmaxshow*fheight;	// recalc height

	x = getScreenStartX (width);
	y = getScreenStartY (height);

	m_viewer->SetScaling((CPictureViewer::ScalingMode)g_settings.picviewer_scaling);
	m_viewer->SetVisible(g_settings.screen_StartX, g_settings.screen_EndX, g_settings.screen_StartY, g_settings.screen_EndY);

	if(g_settings.video_Format == CControldClient::VIDEOFORMAT_16_9)
		m_viewer->SetAspectRatio(16.0/9);
	else if(g_settings.video_Format == CControldClient::VIDEOFORMAT_AUTO)
	{
		g_Controld->setVideoFormat(CControldClient::VIDEOFORMAT_4_3);
		m_viewer->SetAspectRatio(4.0/3);
	}
	else
		m_viewer->SetAspectRatio(4.0/3);

	if(parent)
	{
		parent->hide();
	}

	puts("[pictureviewer.cpp] executing " PICTUREVIEWER_START_SCRIPT ".");
	if (my_system(PICTUREVIEWER_START_SCRIPT) != 0)
		perror(PICTUREVIEWER_START_SCRIPT " failed");

	// tell neutrino we're in pic_mode
	CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , NeutrinoMessages::mode_pic );
	// remember last mode
	CZapitClient::responseGetLastChannel firstchannel;
	g_Zapit->getLastChannel(firstchannel.channelNumber, firstchannel.mode);
	if ((firstchannel.mode == 'r') ?
	    (CNeutrinoApp::getInstance()->zapto_radio_on_init_done) :
	    (CNeutrinoApp::getInstance()->zapto_tv_on_init_done))
		m_LastMode=(CNeutrinoApp::getInstance()->getLastMode() | NeutrinoMessages::norezap);
	else
		m_LastMode=(CNeutrinoApp::getInstance()->getLastMode());

	g_Sectionsd->setPauseScanning(true); 

	show();

	// free picviewer mem
	m_viewer->Cleanup();

	puts("[pictureviewer.cpp] executing " PICTUREVIEWER_END_SCRIPT ".");
	if (my_system(PICTUREVIEWER_END_SCRIPT) != 0)
		perror(PICTUREVIEWER_END_SCRIPT " failed");

	// Start Sectionsd
	g_Sectionsd->setPauseScanning(false);
	
	// Restore last mode
	CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , m_LastMode );
	g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );

	// always exit all	
	return menu_return::RETURN_EXIT_ALL;
}

//------------------------------------------------------------------------

int CPictureViewerGui::show()
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = -1;

	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, g_Locale->getText(LOCALE_PICTUREVIEWER_HEAD));
	m_state = MENU;

	int timeout;

	bool loop = true;
	bool update = true;
	
	while (loop)
	{
		if (update)
		{
			hide();
			update = false;
			paint();
		}
		
		if (m_state != SLIDESHOW)
			timeout = 50; // egal
		else
		{
			timeout = (m_time + atoi(g_settings.picviewer_slide_time) - (long)time(NULL))*10;
			if (timeout < 0)
				timeout = 1;
		}
		g_RCInput->getMsg(&msg, &data, timeout);
		neutrino_msg_t msg_repeatok = msg & ~CRCInput::RC_Repeat;

		if (msg == CRCInput::RC_home)
		{ //Exit after cancel key
			if (m_state != MENU)
			{
				endView();
				update = true;
			}
			else
				loop = false;
		}
		else if (msg == CRCInput::RC_timeout)
		{
			if (m_state == SLIDESHOW)
			{
				m_time = (long)time(NULL);
				unsigned int next = selected + 1;
				if (next >= playlist.size())
					next = 0;
				view(next);
			}
		}
		else if (msg == CRCInput::RC_left)
		{
			if (!playlist.empty())
			{
				if (m_state == MENU)
				{
					if (selected < listmaxshow)
						selected = playlist.size()-1;
					else
						selected -= listmaxshow;
					liststart = (selected / listmaxshow) * listmaxshow;
					paint();
				}
				else
				{
					view((selected == 0) ? (playlist.size() - 1) : (selected - 1));
				}
			}
		}
		else if (msg == CRCInput::RC_right)
		{
			if (!playlist.empty())
			{
				if (m_state == MENU)
				{
					selected += listmaxshow;
					if (selected >= playlist.size())
						if (((playlist.size() / listmaxshow) + 1) * listmaxshow == playlist.size() + listmaxshow)
							selected = 0;
						else
							selected = selected < (((playlist.size() / listmaxshow) + 1) * listmaxshow) ? (playlist.size() - 1) : 0;
					liststart = (selected / listmaxshow) * listmaxshow;
					paint();
				}
				else
				{
					unsigned int next = selected + 1;
					if (next >= playlist.size())
						next = 0;
					view(next);
				}
			}
		}
		else if (msg_repeatok == CRCInput::RC_up || msg_repeatok == CRCInput::RC_down)
		{
			if ((m_state == MENU) && (!playlist.empty()))
			{
				int prevselected = selected;
				int direction = (msg_repeatok == CRCInput::RC_up) ? -1 : 1;
				selected = (selected + playlist.size() + direction) % playlist.size();
				unsigned int oldliststart = liststart;
				liststart = (selected / listmaxshow) * listmaxshow;
				if (oldliststart != liststart)
				{
					update = true;
				}
				else
				{
					paintItem(prevselected - liststart);
					paintItem(selected - liststart);
				}
			}
		}
		else if (msg == CRCInput::RC_ok)
		{
			if (!playlist.empty())
				view(selected);
		}
		else if (msg == CRCInput::RC_red)
		{
			if (m_state == MENU)
			{
				if (!playlist.empty())
				{
					CViewList::iterator p = playlist.begin() + selected;
					playlist.erase(p);
					if (selected >= playlist.size())
						selected = playlist.empty() ? 0 : playlist.size() - 1;
					update = true;
				}
			}
			else if (m_state == SLIDESHOW)
			{
				m_state = VIEW;
			}
		}
		else if (msg == CRCInput::RC_green)
		{
			if (m_state == MENU)
			{
				CFileBrowser filebrowser((g_settings.filebrowser_denydirectoryleave) ? g_settings.picviewer_picturedir : "");

				filebrowser.Multi_Select    = true;
				filebrowser.Dirs_Selectable = true;
				filebrowser.Filter          = &picture_filter;

				hide();

				if (filebrowser.exec(Path.c_str()))
				{
					Path = filebrowser.getCurrentDir();
					
					CFileList::const_iterator files = filebrowser.getSelectedFiles().begin();
					for(; files != filebrowser.getSelectedFiles().end(); ++files)
					{
						if(files->getType() == CFile::FILE_PICTURE)
						{
							CPicture pic;
							pic.Filename = files->Name;
							std::string tmp   = files->Name.substr(files->Name.rfind('/')+1);
							tmp          = FILESYSTEM_ENCODING_TO_UTF8_STRING(tmp);
							pic.Name     = tmp.substr(0,tmp.rfind('.'));
							pic.Type     = tmp.substr(tmp.rfind('.')+1);
							struct stat statbuf;
							if(stat(pic.Filename.c_str(),&statbuf) != 0)
								fprintf(stderr, "stat '%s' error: %m\n", pic.Filename.c_str());
							pic.Date     = statbuf.st_mtime;
							playlist.push_back(pic);
						}
						else
							printf("Wrong Filetype %s:%d\n",files->Name.c_str(), files->getType());
					}
					if (m_sort == FILENAME)
						std::sort(playlist.begin(), playlist.end(), comparePictureByFilename);
					else if (m_sort == DATE)
						std::sort(playlist.begin(), playlist.end(), comparePictureByDate);
				}
				update = true;
				CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, g_Locale->getText(LOCALE_PICTUREVIEWER_HEAD));
			}
		}
		else if (msg == CRCInput::RC_yellow)
		{
			if (m_state == MENU && !playlist.empty())
			{
				playlist.clear();
				selected = 0;
				update = true;
			}
		}
		else if (msg == CRCInput::RC_blue)
		{
			if ((m_state == MENU || m_state == VIEW) && (!playlist.empty()))
			{
				m_time = (long)time(NULL);
				view(selected);
				m_state=SLIDESHOW;
			}
		}
		else if (msg == CRCInput::RC_help)
		{
			if (m_state == MENU)
			{
				showHelp();
				paint();
			}
		}
		else if (msg == CRCInput::RC_1)
		{ 
			if (m_state != MENU)
				m_viewer->Zoom(200/3);

		}
		else if (msg == CRCInput::RC_2)
		{ 
			if (m_state != MENU)
				m_viewer->Move(0,-50);
		}
		else if (msg == CRCInput::RC_3)
		{ 
			if (m_state != MENU)
				m_viewer->Zoom(150);
		}
		else if (msg == CRCInput::RC_4)
		{ 
			if (m_state != MENU)
				m_viewer->Move(-50,0);
		}
		else if (msg == CRCInput::RC_5)
		{
			if (!playlist.empty())
			{
				if (m_sort == FILENAME)
				{
					m_sort=DATE;
					std::sort(playlist.begin(),playlist.end(),comparePictureByDate);
				}
				else if (m_sort==DATE)
				{
					m_sort=FILENAME;
					std::sort(playlist.begin(),playlist.end(),comparePictureByFilename);
				}
				update = true;
			 }
		}
		else if (msg == CRCInput::RC_6)
		{ 
			if (m_state != MENU)
				m_viewer->Move(50,0);
		}
		else if (msg == CRCInput::RC_8)
		{ 
			if (m_state != MENU)
				m_viewer->Move(0,50);
		}
		else if (msg == CRCInput::RC_0)
		{
			if (!playlist.empty())
				view(selected, true);
		}
#ifdef ENABLE_GUI_MOUNT
		else if (msg == CRCInput::RC_setup)
		{
			if (m_state == MENU)
			{
				CNFSSmallMenu nfsMenu;
				nfsMenu.exec(this, "");
				update = true;
				CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, g_Locale->getText(LOCALE_PICTUREVIEWER_HEAD));
			}
		}
#endif
		else if (msg == NeutrinoMessages::CHANGEMODE)
		{
			if ((data & NeutrinoMessages::mode_mask) != NeutrinoMessages::mode_pic)
			{
				loop = false;
				m_LastMode = data;
			}
		}
		else if (msg == NeutrinoMessages::RECORD_START ||
				  msg == NeutrinoMessages::ZAPTO ||
				  msg == NeutrinoMessages::STANDBY_ON ||
				  msg == NeutrinoMessages::SHUTDOWN ||
				  msg == NeutrinoMessages::SLEEPTIMER)
		{
			// Exit for Record/Zapto Timers
			if (m_state != MENU)
				endView();
			loop = false;
			g_RCInput->postMsg(msg, data);
		}
		else
		{
			if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
			{
				loop = false;
			}
		}
	}
	hide();

	return (res);
}

//------------------------------------------------------------------------

void CPictureViewerGui::hide()
{
//	printf("hide(){\n");
	if(visible)
	{
		frameBuffer->paintBackgroundBoxRel(x-1, y-1, width+2, height+2);
		visible = false;
	}
//	printf("hide()}\n");
}

//------------------------------------------------------------------------

void CPictureViewerGui::paintItem(int pos)
{
//	printf("paintItem{\n");
	int ypos = y+ theight + 0 + pos*fheight;
	int c_rad_small = 0;

	uint8_t    color;
	fb_pixel_t bgcolor;

	if ((liststart+pos < playlist.size()) && (pos & 1) )
	{
		color   = COL_MENUCONTENTDARK;
		bgcolor = COL_MENUCONTENTDARK_PLUS_0;
		
	}
	else
	{
		color	= COL_MENUCONTENT;
		bgcolor = COL_MENUCONTENT_PLUS_0;
	}

	if (liststart+pos == selected)
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
		c_rad_small = RADIUS_SMALL;
	}

	frameBuffer->paintBoxRel(x,ypos, width-15, fheight, bgcolor, c_rad_small);
	if(liststart+pos<playlist.size())
	{
		std::string tmp = playlist[liststart+pos].Name;
		tmp += " (";
		tmp += playlist[liststart+pos].Type;
		tmp += ')';
		char timestring[18];
		struct tm tm;
		gmtime_r(&playlist[liststart+pos].Date, &tm);
		strftime(timestring, sizeof(timestring), "%d-%m-%Y %H:%M", &tm);
		int w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(timestring);
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+10,ypos+fheight, width-30 - w, tmp, color, fheight, true); // UTF-8
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+width-20-w,ypos+fheight, w, timestring, color, fheight);
		if (liststart+pos == selected)
		{
			CLCD::getInstance()->showMenuText(0, tmp.c_str(), -1, true); // UTF-8
			CLCD::getInstance()->showMenuText(1, timestring);
		}
	}
//	printf("paintItem}\n");
}

//------------------------------------------------------------------------

void CPictureViewerGui::paintHead()
{
//	printf("paintHead{\n");
	int theight_mid = theight / 2;
	int ypos = y + theight_mid - (ticonheight / 2);

	frameBuffer->paintBoxRel(x, y, width, theight, COL_MENUHEAD_PLUS_0, RADIUS_MID, CORNER_TOP);
	frameBuffer->paintIcon(NEUTRINO_ICON_MP3, x + 7, ypos);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x + 7 + ticonwidth + 10, y + theight + 2, width - 7 - ticonwidth - 10, g_Locale->getText(LOCALE_PICTUREVIEWER_HEAD), COL_MENUHEAD, 0, true); // UTF-8

	int iconw, iconh;
	frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_HELP, &iconw, &iconh);
	int xpos = x + width - iconw - 6;
	ypos = y + theight_mid - (iconh / 2);
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_HELP, xpos, ypos );
#ifdef ENABLE_GUI_MOUNT
	frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_DBOX, &iconw, &iconh);
	xpos -= iconw + 6;
	ypos = y + theight_mid - (iconh / 2);
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_DBOX, xpos, ypos );
#endif
//	printf("paintHead}\n");
}

//------------------------------------------------------------------------
const struct button_label PictureViewerButtons[4] =
{
	{ NEUTRINO_ICON_BUTTON_RED   , LOCALE_AUDIOPLAYER_DELETE        },
	{ NEUTRINO_ICON_BUTTON_GREEN , LOCALE_AUDIOPLAYER_ADD           },
	{ NEUTRINO_ICON_BUTTON_YELLOW, LOCALE_AUDIOPLAYER_DELETEALL     },
	{ NEUTRINO_ICON_BUTTON_BLUE  , LOCALE_PICTUREVIEWER_SLIDESHOW }
};
struct button_label PictureViewerButtons2[2] =
{
	{ NEUTRINO_ICON_BUTTON_5     , LOCALE_GENERIC_EMPTY             },
	{ NEUTRINO_ICON_BUTTON_OKAY  , LOCALE_PICTUREVIEWER_SHOW        }
};

void CPictureViewerGui::paintFoot()
{
//	printf("paintFoot{\n");
	int ButtonWidth = (width - 20) / 4;
	int ButtonWidth2 = (width - 20) / 2;
	
	frameBuffer->paintBoxRel(x, y + height - 2 * buttonHeight, width, 2 * buttonHeight, COL_INFOBAR_SHADOW_PLUS_1, RADIUS_MID, CORNER_BOTTOM);

	if (!playlist.empty())
	{
		if ( m_sort == FILENAME)
			PictureViewerButtons2[0].locale = LOCALE_PICTUREVIEWER_SORTORDER_FILENAME;
		else if( m_sort == DATE )
			PictureViewerButtons2[0].locale = LOCALE_PICTUREVIEWER_SORTORDER_DATE;

		::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, x + 10, y + height - 2 * buttonHeight + 2, ButtonWidth, 4, PictureViewerButtons);
		::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, x + 10, y + height - buttonHeight - 2, ButtonWidth2, 2, PictureViewerButtons2);
	}
	else
		::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, x + ButtonWidth + 10, y + height - 2 * buttonHeight + 2, ButtonWidth, 1, &(PictureViewerButtons[1]));
//	printf("paintFoot}\n");
}
//------------------------------------------------------------------------
void CPictureViewerGui::paintInfo()
{
}
//------------------------------------------------------------------------

void CPictureViewerGui::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;

	paintHead();

	int ypos = y+ theight;
	int sb = fheight* listmaxshow;
	frameBuffer->paintBoxRel(x, ypos, width, sb, COL_MENUCONTENT_PLUS_0); //mainframe
	frameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb,  COL_MENUCONTENT_PLUS_1); //scrollbar

	for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(count);
	}

	int sbc= ((playlist.size()- 1)/ listmaxshow)+ 1;
	int sbs= (selected/listmaxshow);
	frameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ sbs*(sb-4)/sbc, 11, (sb-4)/sbc, COL_MENUCONTENT_PLUS_3, RADIUS_SMALL);

	paintFoot();
	paintInfo();
	
	visible = true;
}

void CPictureViewerGui::view(unsigned int index, bool unscaled)
{
	selected=index;
	
	CLCD::getInstance()->showMenuText(0, playlist[index].Name.c_str(), -1, true); // UTF-8
	char timestring[19];
	struct tm tm;
	gmtime_r(&playlist[index].Date, &tm);
	strftime(timestring, sizeof(timestring), "%d-%m-%Y %H:%M", &tm);
	CLCD::getInstance()->showMenuText(1, timestring);
	
	if(m_state == MENU)
	{
		frameBuffer->setMode(720, 576, 16);
#ifdef HAVE_DBOX_HARDWARE
		frameBuffer->setTransparency(0);
#endif
	}
	if(unscaled)
		m_viewer->DecodeImage(playlist[index].Filename, true, unscaled);
	m_viewer->ShowImage(playlist[index].Filename, unscaled);

	//Decode next
	unsigned int next=selected+1;
	if(next > playlist.size()-1)
		next=0;
	if(m_state==MENU)
		m_state=VIEW;
	if(m_state==VIEW)
		m_viewer->DecodeImage(playlist[next].Filename,true);
	else
		m_viewer->DecodeImage(playlist[next].Filename,false);
}

void CPictureViewerGui::endView()
{
	if(m_state != MENU)
	{
		frameBuffer->setMode(720, 576, 8 * sizeof(fb_pixel_t));
#ifdef HAVE_DBOX_HARDWARE
		frameBuffer->setBlendLevel(g_settings.gtx_alpha1, g_settings.gtx_alpha2);
#endif
		frameBuffer->ClearFrameBuffer();
		m_state=MENU;
	}
}

/************************************************************************/
std::string CPictureViewerGui::getPictureViewerVersion(void)
{	
	static CImageInfo imageinfo;
	return imageinfo.getModulVersion("","$Revision: 1.80 $");
}

void CPictureViewerGui::showHelp()
{
	std::string version = "Version: " + getPictureViewerVersion();
	Helpbox helpbox;
	helpbox.addLine(g_Locale->getText(LOCALE_PICTUREVIEWER_HELP1));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_OKAY, g_Locale->getText(LOCALE_PICTUREVIEWER_HELP2));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_5, g_Locale->getText(LOCALE_PICTUREVIEWER_HELP3));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_0, g_Locale->getText(LOCALE_PICTUREVIEWER_HELP4));
	helpbox.addLine("");
	helpbox.addLine(g_Locale->getText(LOCALE_PICTUREVIEWER_HELP5));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_LEFT, g_Locale->getText(LOCALE_PICTUREVIEWER_HELP6));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_RIGHT, g_Locale->getText(LOCALE_PICTUREVIEWER_HELP7));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_5, g_Locale->getText(LOCALE_PICTUREVIEWER_HELP8));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_HOME, g_Locale->getText(LOCALE_PICTUREVIEWER_HELP9));
	helpbox.addLine("");
	helpbox.addLine(g_Locale->getText(LOCALE_PICTUREVIEWER_HELP10));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_OKAY, g_Locale->getText(LOCALE_PICTUREVIEWER_HELP11));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_LEFT, g_Locale->getText(LOCALE_PICTUREVIEWER_HELP12));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_RIGHT, g_Locale->getText(LOCALE_PICTUREVIEWER_HELP13));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_1, g_Locale->getText(LOCALE_PICTUREVIEWER_HELP14));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_3, g_Locale->getText(LOCALE_PICTUREVIEWER_HELP15));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_2, g_Locale->getText(LOCALE_PICTUREVIEWER_HELP16));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_4, g_Locale->getText(LOCALE_PICTUREVIEWER_HELP17));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_6, g_Locale->getText(LOCALE_PICTUREVIEWER_HELP18));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_8, g_Locale->getText(LOCALE_PICTUREVIEWER_HELP19));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_5, g_Locale->getText(LOCALE_PICTUREVIEWER_HELP20));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_0, g_Locale->getText(LOCALE_PICTUREVIEWER_HELP21));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_HOME, g_Locale->getText(LOCALE_PICTUREVIEWER_HELP22));
	helpbox.addLine("");
	helpbox.addLine(version);
	hide();
	helpbox.show(LOCALE_MESSAGEBOX_INFO);
}
