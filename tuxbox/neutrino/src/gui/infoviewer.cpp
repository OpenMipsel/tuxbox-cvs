/*
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

#include "infoviewer.h"
#include "../global.h"

#define COL_INFOBAR_BUTTONS				COL_INFOBAR_SHADOW+ 1
#define COL_INFOBAR_BUTTONS_GRAY		COL_INFOBAR_SHADOW+ 1

#define ICON_LARGE 30
#define ICON_SMALL 20
#define ICON_OFFSET (2*ICON_LARGE+ ICON_SMALL+ 5)
#define BOTTOM_BAR_OFFSET 0
#define SHADOW_OFFSET 6


CInfoViewer::CInfoViewer()
{
        BoxStartX= BoxStartY= BoxEndX= BoxEndY=0;
        is_visible		= false;
        showButtonBar	= false;
        gotTime 		= g_Sectionsd->getIsTimeSet();

        strcpy( running, "");
        strcpy( next, "");
        strcpy( runningStart, "");
        strcpy( nextStart, "");
        strcpy( runningDuration, "");
        strcpy( runningRest, "");
        strcpy( nextDuration, "");

        runningPercent = 0;
        CurrentChannel = "";

        pthread_cond_init( &epg_cond, NULL );
        pthread_mutexattr_t   mta;

	    if (pthread_mutexattr_init(&mta) != 0 )
    		perror("CInfoViewer: pthread_mutexattr_init failed\n");
    	if (pthread_mutexattr_settype( &mta, PTHREAD_MUTEX_ERRORCHECK ) != 0 )
			perror("CInfoViewer: pthread_mutexattr_settype failed\n");
		if (pthread_mutex_init( &epg_mutex, &mta ) != 0)
			perror("CInfoViewer: pthread_mutex_init failed\n");


        if (pthread_create (&thrViewer, NULL, InfoViewerThread, (void *) this) != 0 )
        {
                perror("CInfoViewer::CInfoViewer create thrViewer failed\n");
        }

        pthread_cond_init( &cond_PIDs_available, NULL );

        if (pthread_create (&thrLangViewer, NULL, LangViewerThread, (void *) this) != 0 )
        {
                perror("CInfoViewer::CInfoViewer create thrLangViewer failed\n");
        }

}

void CInfoViewer::start()
{
        InfoHeightY = g_Fonts->infobar_number->getHeight()*9/8 +
                      2*g_Fonts->infobar_info->getHeight() +
                      25;
        InfoHeightY_Info = g_Fonts->infobar_small->getHeight()+ 5;

        ChanWidth = g_Fonts->infobar_number->getRenderWidth("0000") + 10;
        ChanHeight = g_Fonts->infobar_number->getHeight()*9/8;

        aspectRatio = g_Controld->getAspectRatio();

}

const std::string CInfoViewer::getActiveChannelID()
{
        string  s_id;
        char anid[10];
        snprintf( anid, 10, "%x", Current_onid_tsid );
        s_id= anid;

        return s_id;
}

void CInfoViewer::showTitle( int ChanNum, string Channel, unsigned int onid_tsid, bool CalledFromNumZap )
{
        pthread_mutex_lock( &epg_mutex );

        CurrentChannel = Channel;
        Current_onid_tsid = onid_tsid;

        showButtonBar = !CalledFromNumZap;

        EPG_NotFound_Text = (char*) g_Locale->getText(CalledFromNumZap?"infoviewer.epgnotload":"infoviewer.epgwait").c_str();

        BoxStartX = g_settings.screen_StartX+ 20;
        BoxEndX   = g_settings.screen_EndX- 20;
        BoxEndY   = g_settings.screen_EndY- 20;

        int BoxEndInfoY = showButtonBar?(BoxEndY- InfoHeightY_Info):(BoxEndY);
		BoxStartY = BoxEndInfoY- InfoHeightY;

        KillShowEPG = false;
        pthread_mutex_unlock( &epg_mutex );

		// kill linke seite
        g_FrameBuffer->paintBackgroundBox(BoxStartX, BoxStartY+ ChanHeight, BoxStartX + (ChanWidth/3), BoxStartY+ ChanHeight+ InfoHeightY_Info+ 10);
        // kill progressbar
        g_FrameBuffer->paintBackgroundBox(BoxEndX- 120, BoxStartY, BoxEndX, BoxStartY+ ChanHeight);

        //number box
        g_FrameBuffer->paintBoxRel(BoxStartX+10, BoxStartY+10, ChanWidth, ChanHeight, COL_INFOBAR_SHADOW);
        g_FrameBuffer->paintBoxRel(BoxStartX,    BoxStartY,    ChanWidth, ChanHeight, COL_INFOBAR);

        //channel number
        char strChanNum[10];
        sprintf( (char*) strChanNum, "%d", ChanNum);
        int ChanNumXPos = BoxStartX + ((ChanWidth - g_Fonts->infobar_number->getRenderWidth(strChanNum))>>1);
        g_Fonts->infobar_number->RenderString(ChanNumXPos, BoxStartY+ChanHeight, ChanWidth, strChanNum, COL_INFOBAR);

        //infobox
        int ChanNameX = BoxStartX + ChanWidth + 10;
        int ChanNameY = BoxStartY + (ChanHeight>>1)   + 5; //oberkante schatten?

       	g_FrameBuffer->paintBox(ChanNameX, ChanNameY, BoxEndX, BoxEndInfoY, COL_INFOBAR);

        int height=g_Fonts->infobar_channame->getHeight()+5;

        //time? todo - thread suxx...
        char timestr[50];
        struct timeb tm;
        ftime(&tm);
        strftime((char*) &timestr, 20, "%H:%M", localtime(&tm.time) );
        int timewidth = g_Fonts->infobar_channame->getRenderWidth(timestr);

      	g_Fonts->infobar_channame->RenderString(BoxEndX-timewidth-10, ChanNameY+height, timewidth+ 5, timestr, COL_INFOBAR);

		// ... with channel name
        g_Fonts->infobar_channame->RenderString(ChanNameX+ 10, ChanNameY+height, BoxEndX- (ChanNameX+ 20)- timewidth- 15, Channel.c_str(), COL_INFOBAR);

        ChanInfoX = BoxStartX + (ChanWidth / 3);
        int ChanInfoY = BoxStartY + ChanHeight+10;

        g_FrameBuffer->paintBox(ChanInfoX, ChanInfoY, ChanNameX, BoxEndInfoY, COL_INFOBAR);

        if ( showButtonBar )
        {
        		if ( BOTTOM_BAR_OFFSET> 0 )
	        		g_FrameBuffer->paintBackgroundBox(ChanInfoX, BoxEndInfoY, BoxEndX, BoxEndInfoY+ BOTTOM_BAR_OFFSET);
        		g_FrameBuffer->paintBox(ChanInfoX, BoxEndInfoY+ BOTTOM_BAR_OFFSET, BoxEndX, BoxEndY, COL_INFOBAR_BUTTONS);

                ButtonWidth = (BoxEndX- ChanInfoX- ICON_OFFSET)>> 2;

                //g_FrameBuffer->paintHLine(ChanInfoX, BoxEndX,  BoxEndY-InfoHeightY_Info, COL_INFOBAR_SHADOW);
                //g_FrameBuffer->paintHLine(ChanInfoX, BoxEndX,  BoxEndY-InfoHeightY_Info+1, COL_INFOBAR_SHADOW); 2Lines wegen scanline?

                // blau
                g_FrameBuffer->paintIcon("blau.raw", BoxEndX- ICON_OFFSET- ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
                g_Fonts->infobar_small->RenderString(BoxEndX- ICON_OFFSET- ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 30, g_Locale->getText("infoviewer.streaminfo").c_str(), COL_INFOBAR_BUTTONS);

				// gelb
				//g_FrameBuffer->paintIcon("gray.raw", BoxEndX- ICON_OFFSET- 2* ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
				//g_Fonts->infobar_small->RenderString(BoxEndX- 2* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 26, g_Locale->getText("infoviewer.subservice").c_str(), COL_INFOBAR_BUTTONS_GRAY);

				// gr�n
				//g_FrameBuffer->paintIcon("gray.raw", BoxEndX- ICON_OFFSET- 3* ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
				//g_Fonts->infobar_small->RenderString(BoxEndX- 3* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 26, g_Locale->getText("infoviewer.languages").c_str(), COL_INFOBAR_BUTTONS_GRAY);

				// rot
				//g_FrameBuffer->paintIcon("gray.raw", BoxEndX- ICON_OFFSET- 4* ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
				//g_Fonts->infobar_small->RenderString(BoxEndX- 4* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 26, g_Locale->getText("infoviewer.eventlist").c_str(), COL_INFOBAR_BUTTONS_GRAY);

                g_FrameBuffer->paintIcon("dd_gray.raw", BoxEndX- ICON_LARGE- ICON_SMALL, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
                g_FrameBuffer->paintIcon("vtxt_gray.raw", BoxEndX- ICON_SMALL, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );

                showButtonNVOD(true);

                g_RemoteControl->CopyPIDs();
                showButtonAudio();
                show16_9();
        }

		// Schatten
        g_FrameBuffer->paintBox(BoxEndX, ChanNameY+ SHADOW_OFFSET, BoxEndX+ SHADOW_OFFSET, BoxEndY, COL_INFOBAR_SHADOW);
        g_FrameBuffer->paintBox(ChanInfoX+ SHADOW_OFFSET, BoxEndY, BoxEndX+ SHADOW_OFFSET, BoxEndY+ SHADOW_OFFSET, COL_INFOBAR_SHADOW);


        pthread_mutex_lock( &epg_mutex );
        is_visible = true;
        pthread_mutex_unlock( &epg_mutex );

        pthread_cond_signal( &epg_cond );

        usleep(50);

        uint msg; uint data;

        if ( !CalledFromNumZap )
        {
       		bool hideIt = true;
			long long timeoutEnd = g_RCInput->calcTimeoutEnd( g_settings.timing_infobar >> 1 );

			int res = messages_return::none;

			while ( ! ( res & ( messages_return::cancel_info | messages_return::cancel_all ) ) )
			{
				g_RCInput->getMsgAbsoluteTimeout( &msg, &data, timeoutEnd );

				if ( msg == CRCInput::RC_help )
				{
					g_RCInput->pushbackMsg( messages::SHOW_EPG, 0 );
					res = messages_return::cancel_info;
				}
				else if ( ( msg == CRCInput::RC_timeout ) ||
				          ( msg == CRCInput::RC_ok ) )
				{
					res = messages_return::cancel_info;
				}
				else if ( ( msg == g_settings.key_quickzap_up ) ||
               	 	 	  ( msg == g_settings.key_quickzap_down ) )
				{
					hideIt = false;
					g_RCInput->pushbackMsg(  msg, data );
					res = messages_return::cancel_info;
				}
				else if ( msg == messages::EVT_TIMESET )
				{
					// Handle anyway!
					neutrino->handleMsg( msg, data );
        			g_RCInput->pushbackMsg( messages::SHOW_INFOBAR, 0 );
					res = messages_return::cancel_all;
				}
				else
				{
            		res = neutrino->handleMsg( msg, data );

            		if ( res == messages_return::unhandled )
            		{
            			// raus hier und im Hauptfenster behandeln...
            			g_RCInput->pushbackMsg(  msg, data );
						res = messages_return::cancel_info;
					}
				}
			}

            if ( hideIt )
				killTitle();
        }
}

void CInfoViewer::show16_9()
{
	if ( ( is_visible ) && ( showButtonBar ) )
		g_FrameBuffer->paintIcon( ( aspectRatio == 3 )?"16_9.raw":"16_9_gray.raw", BoxEndX- 2* ICON_LARGE- ICON_SMALL, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
}

int CInfoViewer::handleMsg(uint msg, uint data)
{

    if ( msg == messages::EVT_MODECHANGED )
	{
        aspectRatio = data;
		show16_9();
        return messages_return::handled;
	}
	else if ( msg == messages::EVT_TIMESET )
	{
		gotTime = true;
		return messages_return::unhandled;
	}
	else
		return messages_return::unhandled;
}


void CInfoViewer::showButtonNVOD(bool CalledFromShowData = false)
{
        CSubChannel_Infos subChannels= g_RemoteControl->getSubChannels();

        if ( subChannels.has_subChannels_for( getActiveChannelID() ) )
        {
                // gelbe Taste f�r NVODs / Subservices
                g_FrameBuffer->paintIcon("gelb.raw", BoxEndX- ICON_OFFSET- 2* ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
                if ( subChannels.are_subchannels )
                        // SubServices
                        g_Fonts->infobar_small->RenderString(BoxEndX- ICON_OFFSET- 2* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 30, g_Locale->getText("infoviewer.subservice").c_str(), COL_INFOBAR_BUTTONS);
                else
                        // NVOD
                        g_Fonts->infobar_small->RenderString(BoxEndX- ICON_OFFSET- 2* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 30, g_Locale->getText("infoviewer.selecttime").c_str(), COL_INFOBAR_BUTTONS);

                if (!CalledFromShowData)
                        showData();
        };
}

void CInfoViewer::showData()
{
        int is_nvod= false;

        if (showButtonBar)
        {
                CSubChannel_Infos subChannels= g_RemoteControl->getSubChannels();

                if ( SubServiceList.size()> 0 )
                {
                        string activeID= getActiveChannelID();
                        if ( !subChannels.has_subChannels_for( activeID ) )
                        {
                                //printf("subservices %d\n", SubServiceList.size());
                                subChannels= CSubChannel_Infos( activeID.c_str(), true );
                                for(unsigned int count=0;count<SubServiceList.size();count++)
                                {
                                        subChannels.list.insert( subChannels.list.end(),
                                                                 CSubService(SubServiceList[count]->originalNetworkId<<16 | SubServiceList[count]->serviceId,
                                                                             SubServiceList[count]->transportStreamId,
                                                                             SubServiceList[count]->name) );
                                }
                                g_RemoteControl->CopySubChannelsToZapit( subChannels );
                                showButtonNVOD(true);
                        }
                }

                if ( !subChannels.are_subchannels )
                {
                        if ( subChannels.has_subChannels_for( getActiveChannelID() ) )
                        {
                                // NVOD- Zeiten aus dem aktuell selektierten holen!
                                is_nvod= true;

                                unsigned sel= subChannels.selected;
                                unsigned dauer= subChannels.list[sel].dauer/ 60;
                                //sprintf((char*) &runningDuration, "%d min", dauer);

                                struct      tm *pStartZeit = localtime(&subChannels.list[sel].startzeit);
                                sprintf((char*) &runningStart, "%02d:%02d", pStartZeit->tm_hour, pStartZeit->tm_min);
                                runningPercent=(unsigned)((float)(time(NULL)-subChannels.list[sel].startzeit)/(float)subChannels.list[sel].dauer*100.);

        						unsigned seit = ( time(NULL) - subChannels.list[sel].startzeit ) / 60;
                                unsigned rest = ( (subChannels.list[sel].startzeit + subChannels.list[sel].dauer) - time(NULL) ) / 60;

								sprintf((char*) &runningRest, "%d / %d min", seit, rest);
                                if (runningPercent>100)
                                        runningPercent=0;

                                Flag|= sectionsd::epg_has_current;
                                //printf("%s %s %d\n", runningDuration, runningStart, runningPercent);
                        }
                }
        }

        int height = g_Fonts->infobar_channame->getHeight()/3;
        int ChanInfoY = BoxStartY + ChanHeight+ 15; //+10

        //percent
        if ( showButtonBar )
        {
                int posy = BoxStartY+12;
                int height2= 20;//int( g_Fonts->infobar_channame->getHeight()/1.7);

                //      g_FrameBuffer->paintBox(BoxEndX-130, BoxStartY, BoxEndX, ChanNameY+2, COL_INFOBAR+1); //bounding box (off)
                if ( Flag & sectionsd::epg_has_current)
                {
                        g_FrameBuffer->paintBoxRel(BoxEndX-114, posy,   2+100+2, height2, COL_INFOBAR_SHADOW); //border
                        g_FrameBuffer->paintBoxRel(BoxEndX-112, posy+2, runningPercent+2, height2-4, COL_INFOBAR+7);//fill(active)
                        g_FrameBuffer->paintBoxRel(BoxEndX-112+runningPercent, posy+2, 100-runningPercent, height2-4, COL_INFOBAR+3);//fill passive
                }
                if ( Flag & sectionsd::epg_has_anything )
                {
                        g_FrameBuffer->paintIcon("rot.raw", BoxEndX- ICON_OFFSET- 4* ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
                        g_Fonts->infobar_small->RenderString(BoxEndX- ICON_OFFSET- 4* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 30, g_Locale->getText("infoviewer.eventlist").c_str(), COL_INFOBAR_BUTTONS);
                }

        }

        height = g_Fonts->infobar_info->getHeight();
        int xStart= BoxStartX + ChanWidth;// + 20;

//        if ( is_nvod )
                g_FrameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height , COL_INFOBAR);

        if ( Flag & sectionsd::epg_not_broadcast )
        {
                // kein EPG verf�gbar
                ChanInfoY += height;
                g_FrameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height, COL_INFOBAR);
                g_Fonts->infobar_info->RenderString(BoxStartX + ChanWidth + 20,  ChanInfoY+height, BoxEndX- (BoxStartX + ChanWidth + 20), g_Locale->getText("infoviewer.noepg").c_str(), COL_INFOBAR);
        }
        else
        {
                // irgendein EPG gefunden
                int duration1Width   = g_Fonts->infobar_info->getRenderWidth(runningRest);
        		int duration1TextPos = BoxEndX- duration1Width- 10;

                int duration2Width   = g_Fonts->infobar_info->getRenderWidth(nextDuration);
                int duration2TextPos = BoxEndX- duration2Width- 10;

                if ( ( Flag & sectionsd::epg_has_next ) && ( !( Flag & sectionsd::epg_has_current )) )
                {
                        // sp�tere Events da, aber kein aktuelles...
                        //g_FrameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height, COL_INFOBAR);
                        g_Fonts->infobar_info->RenderString(xStart,  ChanInfoY+height, BoxEndX- xStart, g_Locale->getText("infoviewer.nocurrent").c_str(), COL_INFOBAR);

                        ChanInfoY += height;

                        //info next
                        g_FrameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height , COL_INFOBAR);

                        g_Fonts->infobar_info->RenderString(ChanInfoX+10,                ChanInfoY+height, 100, nextStart, COL_INFOBAR);
                        g_Fonts->infobar_info->RenderString(xStart,  ChanInfoY+height, duration2TextPos- xStart- 5, next, COL_INFOBAR);
                        g_Fonts->infobar_info->RenderString(duration2TextPos,            ChanInfoY+height, duration2Width, nextDuration, COL_INFOBAR);
                }
                else
                {
                		//g_FrameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height, COL_INFOBAR);
                        g_Fonts->infobar_info->RenderString(ChanInfoX+10,                ChanInfoY+height, 100, runningStart, COL_INFOBAR);
                        g_Fonts->infobar_info->RenderString(xStart,  ChanInfoY+height, duration1TextPos- xStart- 5, running, COL_INFOBAR);
                        g_Fonts->infobar_info->RenderString(duration1TextPos,            ChanInfoY+height, duration1Width, runningRest, COL_INFOBAR);

                        ChanInfoY += height;

                        //info next
                        g_FrameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height , COL_INFOBAR);

                        if ( ( !is_nvod ) && ( Flag & sectionsd::epg_has_next ) )
                        {
                                g_Fonts->infobar_info->RenderString(ChanInfoX+10,                ChanInfoY+height, 100, nextStart, COL_INFOBAR);
                                g_Fonts->infobar_info->RenderString(xStart,  ChanInfoY+height, duration2TextPos- xStart- 5, next, COL_INFOBAR);
                                g_Fonts->infobar_info->RenderString(duration2TextPos,            ChanInfoY+height, duration2Width, nextDuration, COL_INFOBAR);
                        }
                }
        }
        if (!is_visible)
        {
                killTitle();
        }
}

void CInfoViewer::showButtonAudio()
{
        string  to_compare= getActiveChannelID();

        if ( strcmp(g_RemoteControl->audio_chans.name, to_compare.c_str() )== 0 )
        {
                if ( ( g_RemoteControl->ecmpid == invalid_ecmpid_found ) ||
                     ( ( g_RemoteControl->audio_chans.count_apids == 0 ) && ( g_RemoteControl->vpid == 0 ) ) )
                {
                        int height = g_Fonts->infobar_info->getHeight();
                        int ChanInfoY = BoxStartY + ChanHeight+ 15+ 2* height;
                        int xStart= BoxStartX + ChanWidth + 20;

                        //int ChanNameX = BoxStartX + ChanWidth + 10;
                        int ChanNameY = BoxStartY + ChanHeight + 10;


                        string  disp_text;
                        if ( ( g_RemoteControl->ecmpid == invalid_ecmpid_found ) )
						{
                                disp_text= g_Locale->getText("infoviewer.cantdecode");
								#ifdef USEACTIONLOG
									g_ActionLog->println("cannot decode");
								#endif
						}
                        else
						{
                                disp_text= g_Locale->getText("infoviewer.notavailable");
								#ifdef USEACTIONLOG
									g_ActionLog->println("not available");
								#endif
						}

                        g_FrameBuffer->paintBox(ChanInfoX, ChanNameY, BoxEndX, ChanInfoY, COL_INFOBAR);
                        g_Fonts->infobar_info->RenderString(xStart, ChanInfoY, BoxEndX- xStart, disp_text.c_str(), COL_INFOBAR);
                        KillShowEPG = true;
                };


                // gr�n, wenn mehrere APIDs
                if ( g_RemoteControl->audio_chans.count_apids> 1 )
                {
                        g_FrameBuffer->paintIcon("gruen.raw", BoxEndX- ICON_OFFSET- 3* ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
                        g_Fonts->infobar_small->RenderString(BoxEndX- ICON_OFFSET- 3* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 30, g_Locale->getText("infoviewer.languages").c_str(), COL_INFOBAR_BUTTONS);
                };

                for (int count= 0; count< g_RemoteControl->audio_chans.count_apids; count++)
                	if ( g_RemoteControl->audio_chans.apids[count].is_ac3 )
                	{
                		if (g_RemoteControl->audio_chans.selected== count )
                		{
	                		g_FrameBuffer->paintIcon("dd.raw", BoxEndX- ICON_LARGE- ICON_SMALL, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
	                		break;
	                	}
	                	else
	                		g_FrameBuffer->paintIcon("dd_avail.raw", BoxEndX- ICON_LARGE- ICON_SMALL, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
                	}

                if ( g_RemoteControl->vtxtpid != 0 )
                	g_FrameBuffer->paintIcon("vtxt.raw", BoxEndX- ICON_SMALL, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
        };
}

void CInfoViewer::showWarte()
{

        int height = g_Fonts->infobar_info->getHeight();
        int ChanInfoY = BoxStartY + ChanHeight+ 15+ 2* height;
        int xStart= BoxStartX + ChanWidth + 20;

        pthread_mutex_lock( &epg_mutex );
        if ( ( !KillShowEPG ) && ( is_visible ) )
                g_Fonts->infobar_info->RenderString(xStart, ChanInfoY, BoxEndX- xStart, EPG_NotFound_Text, COL_INFOBAR);
        pthread_mutex_unlock( &epg_mutex );
}

void CInfoViewer::killTitle()
{
        pthread_mutex_lock( &epg_mutex );
        if (is_visible )
        {
                is_visible = false;
                g_FrameBuffer->paintBackgroundBox(BoxStartX, BoxStartY, BoxEndX+ SHADOW_OFFSET, BoxEndY+ SHADOW_OFFSET );
        }
        pthread_mutex_unlock( &epg_mutex );
}


void * CInfoViewer::LangViewerThread (void *arg)
{
        CInfoViewer* InfoViewer = (CInfoViewer*) arg;
        while(1)
        {
                pthread_mutex_lock( &InfoViewer->epg_mutex );
                pthread_cond_wait( &InfoViewer->cond_PIDs_available, &InfoViewer->epg_mutex );

                if ( ( InfoViewer->is_visible ) && ( InfoViewer->showButtonBar ) )
                {
                        g_RemoteControl->CopyPIDs();
                        InfoViewer->showButtonAudio();

                        InfoViewer->showButtonNVOD();
                }

                pthread_mutex_unlock( &InfoViewer->epg_mutex );
        }
}

void * CInfoViewer::InfoViewerThread (void *arg)
{
	int repCount;
	string query = "";
	string old_query = "";
	unsigned int    query_onid_tsid;
	bool gotEPG, requeryEPG;
	struct timespec abs_wait;
	struct timeval now;
	char old_flags;

	CInfoViewer* InfoViewer = (CInfoViewer*) arg;
	while(1)
	{
		pthread_mutex_lock( &InfoViewer->epg_mutex );
		pthread_cond_wait( &InfoViewer->epg_cond, &InfoViewer->epg_mutex );

		if ( ( InfoViewer->is_visible ) )
		{
			gotEPG = true;
			repCount = 10;
            query = "";
			do
			{
				if ( !gotEPG )
				{
					if ( ( repCount > 0 ) &&
					     !( InfoViewer->Flag & ( sectionsd::epg_has_later | sectionsd::epg_has_current ) ) )
						InfoViewer->showWarte();

					gettimeofday(&now, NULL);
					TIMEVAL_TO_TIMESPEC(&now, &abs_wait);
					abs_wait.tv_sec += 1;

					pthread_mutex_lock( &InfoViewer->epg_mutex );
					pthread_cond_timedwait( &InfoViewer->epg_cond, &InfoViewer->epg_mutex, &abs_wait );

					repCount--;
				}

				old_flags = InfoViewer->Flag;
				old_query = query;

				pthread_mutex_lock( &InfoViewer->epg_mutex );
				query = InfoViewer->CurrentChannel;
				query_onid_tsid = InfoViewer->Current_onid_tsid;
				pthread_mutex_unlock( &InfoViewer->epg_mutex );

				gotEPG = ( ( InfoViewer->getEPGData(query, query_onid_tsid) ) &&
				           ( InfoViewer->Flag & sectionsd::epg_has_next ) &&
						   ( ( InfoViewer->Flag & sectionsd::epg_has_current ) || ( InfoViewer->Flag & sectionsd::epg_has_no_current ) ) );

				pthread_mutex_lock( &InfoViewer->epg_mutex );

				if ( ( InfoViewer->Flag & ( sectionsd::epg_has_later | sectionsd::epg_has_current ) ) && (!gotEPG) )
				{
					if (!InfoViewer->showButtonBar)
					{
						gotEPG= true;
					}
					else
					if ( ( (query!=old_query) ||
						   ( (InfoViewer->Flag & sectionsd::epg_has_current) != (old_flags & sectionsd::epg_has_current) ) ||
						   ( (InfoViewer->Flag & sectionsd::epg_has_later) != (old_flags & sectionsd::epg_has_later) ) ) &&
						 ( !InfoViewer->KillShowEPG ) && ( InfoViewer->is_visible ) )
					{
						InfoViewer->showData();
					}
				}
				else
				{
					gotEPG= gotEPG || ( InfoViewer->Flag & sectionsd::epg_not_broadcast );
				}

				requeryEPG = ( ( (!gotEPG) || (query!=InfoViewer->CurrentChannel) ) &&
				               ( InfoViewer->is_visible ) );

				if (query!=InfoViewer->CurrentChannel)
					repCount = 10;

				if ( InfoViewer->KillShowEPG )
					repCount = 0;


				if ( ( !requeryEPG) && ( InfoViewer->is_visible ) && ( !InfoViewer->KillShowEPG) )
					InfoViewer->showData();

				pthread_mutex_unlock( &InfoViewer->epg_mutex );

			} while ( ( requeryEPG ) && (repCount > 0) );
		}

	}
    return NULL;
}


bool CInfoViewer::getEPGData( string channelName, unsigned int onid_tsid )
{
        int sock_fd;
        SAI servaddr;
        char rip[]="127.0.0.1";
        bool retval = false;
        unsigned short SubServiceCount;

        sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        memset(&servaddr,0,sizeof(servaddr));
        servaddr.sin_family=AF_INET;
        servaddr.sin_port=htons(sectionsd::portNumber);
        inet_pton(AF_INET, rip, &servaddr.sin_addr);

        strcpy( running, "");
        strcpy( next, "");
        strcpy( runningStart, "");
        strcpy( nextStart, "");
        strcpy( runningDuration, "");
        strcpy( runningRest, "");
        strcpy( nextDuration, "");
        runningPercent = 0;
        Flag= 0;

        for(unsigned int count=0;count<SubServiceList.size();count++)
        {
                delete SubServiceList[count];
        }
        SubServiceList.clear();

        if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
        {
                perror("Couldn't connect to server!");
                return false;
        }

        if ( ( onid_tsid != 0 ) )
        {
                // query mit onid_tsid...

                sectionsd::msgRequestHeader req;
                req.version = 2;
                req.command = sectionsd::currentNextInformationID;
                req.dataLength = 4;
                write(sock_fd,&req,sizeof(req));

                write(sock_fd, &onid_tsid, sizeof(onid_tsid));
                //            char    num_evts = 2;
                //            write(sock_fd, &num_evts, 1);
                printf("[infoviewer]: query epg for >%x< (%s)\n", onid_tsid, channelName.c_str());

                sectionsd::msgResponseHeader resp;
                memset(&resp, 0, sizeof(resp));

                read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader));

                int nBufSize = resp.dataLength;
                if(nBufSize>0)
                {

                        char* pData = new char[nBufSize+1] ;
                        //read(sock_fd, pData, nBufSize);
                        recv(sock_fd, pData, nBufSize, MSG_WAITALL);

                        unsigned long long          tmp_id;
                        sectionsd::sectionsdTime*   epg_times;
                        char* dp = pData;

                        // current
                        tmp_id = *((unsigned long long *)dp);
                        dp+= sizeof(tmp_id);
                        epg_times = (sectionsd::sectionsdTime*) dp;
                        dp+= sizeof(sectionsd::sectionsdTime);

                        unsigned dauer = epg_times->dauer / 60;
                        unsigned rest = ( (epg_times->startzeit + epg_times->dauer) - time(NULL) ) / 60;
                        //sprintf((char*) &runningDuration, "%d min", dauer);

                        unsigned seit = ( time(NULL) - epg_times->startzeit ) / 60;
						sprintf((char*) &runningRest, "%d / %d min", seit, rest);
                        //sprintf((char*) &runningRest, "%d min", rest);

                        struct      tm *pStartZeit = localtime(&epg_times->startzeit);
                        sprintf((char*) &runningStart, "%02d:%02d", pStartZeit->tm_hour, pStartZeit->tm_min);
                        runningPercent=(unsigned)((float)(time(NULL)-epg_times->startzeit)/(float)epg_times->dauer*100.);
                        strncpy(running, dp, sizeof(running));
                        dp+=strlen(dp)+1;

                        // next
                        tmp_id = *((unsigned long long *)dp);
                        dp+= sizeof(tmp_id);
                        epg_times = (sectionsd::sectionsdTime*) dp;
                        dp+= sizeof(sectionsd::sectionsdTime);

                        dauer = epg_times->dauer/ 60;
                        sprintf((char*) &nextDuration, "%d min", dauer);
                        pStartZeit = localtime(&epg_times->startzeit);
                        sprintf((char*) &nextStart, "%02d:%02d", pStartZeit->tm_hour, pStartZeit->tm_min);
                        strncpy(next, dp, sizeof(next));
                        //printf("[infoviewer]: next= %s\n", next);
                        dp+=strlen(dp)+1;

                        Flag = (unsigned char)* dp;
                        dp+= sizeof(unsigned char);
                        SubServiceCount= *((unsigned short *)dp);
                        //printf("got %d SubServiceCount\n", SubServiceCount);
                        dp+= sizeof(unsigned short);
                        for (int count= 0; count< SubServiceCount; count++)
                        {
                                SubService* aSubService = new SubService();
                                aSubService->name = dp;
                                //printf("SubServiceName %s\n", aSubService->name.c_str());
                                dp+= strlen(dp)+1;
                                aSubService->transportStreamId= *((unsigned short *)dp);
                                dp+= sizeof(unsigned short);
                                aSubService->originalNetworkId= *((unsigned short *)dp);
                                dp+= sizeof(unsigned short);
                                aSubService->serviceId= *((unsigned short *)dp);
                                dp+= sizeof(unsigned short);

                                SubServiceList.insert(SubServiceList.end(), aSubService);
                        }


                        delete[] pData;
                        retval = true;

                }
        }
        close(sock_fd);
        return retval;
}
