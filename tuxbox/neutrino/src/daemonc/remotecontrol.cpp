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

//
// $Id: remotecontrol.cpp,v 1.30 2001/11/15 11:42:41 McClean Exp $
//
// $Log: remotecontrol.cpp,v $
// Revision 1.30  2001/11/15 11:42:41  McClean
// gpl-headers added
//
// Revision 1.29  2001/11/05 16:04:25  field
// nvods/subchannels ver"c++"ed
//
// Revision 1.28  2001/11/03 15:43:17  field
// Perspektiven
//
// Revision 1.27  2001/10/25 12:26:09  field
// NVOD-Zeiten im Infoviewer stimmen
//
// Revision 1.26  2001/10/21 13:06:17  field
// nvod-zeiten funktionieren!
//
// Revision 1.25  2001/10/18 21:03:14  field
// EPG Previous/Next
//
// Revision 1.24  2001/10/16 21:22:44  field
// NVODs besser
//
// Revision 1.23  2001/10/16 19:21:30  field
// NVODs! Zeitanzeige geht noch nicht
//
// Revision 1.20  2001/10/16 17:00:13  faralla
// nvod nearly ready
//
// Revision 1.19  2001/10/15 17:27:19  field
// nvods (fast) implementiert (umschalten funkt noch nicht)
//
// Revision 1.17  2001/10/13 00:46:48  McClean
// nstreamzapd-support broken - repaired
//
// Revision 1.16  2001/10/10 17:17:13  field
// zappen auf onid_sid umgestellt
//
// Revision 1.15  2001/10/10 14:58:09  fnbrd
// Angepasst an neuen sectionsd
//
// Revision 1.14  2001/10/10 02:56:34  fnbrd
// nvod vorbereitet
//
// Revision 1.13  2001/10/09 20:10:08  fnbrd
// Ein paar fehlende Initialisierungen implementiert.
//
//

#include "remotecontrol.h"
#include "../global.h"


CRemoteControl::CRemoteControl()
{
	memset(&remotemsg, 0, sizeof(remotemsg) );
    memset(&audio_chans, 0, sizeof(audio_chans));
    memset(&audio_chans_int, 0, sizeof(audio_chans_int));
    ecm_pid=0;
    zapit_mode=false;

    pthread_cond_init( &send_cond, NULL );
    pthread_mutex_init( &send_mutex, NULL );

    if (pthread_create (&thrSender, NULL, RemoteControlThread, (void *) this) != 0 )
	{
		perror("CRemoteControl: Create RemoteControlThread failed\n");
	}
}

void CRemoteControl::setZapper(bool zapper)
{
	zapit_mode = zapper;
}


void CRemoteControl::send()
{
    pthread_cond_signal( &send_cond );
    usleep(10);
//    printf("CRemoteControl: after pthread_cond_signal (with %s)\n", remotemsg.param3);
}

void CRemoteControl::getNVODs( char *channel_name )
{
    char rip[]="127.0.0.1";
    int rep_cnt= 0;
    CSubServiceListSorted   nvod_list;

    unsigned int onidSid;
    sscanf( channel_name, "%x", &onidSid );

    do
    {
        pthread_mutex_unlock( &send_mutex );
        rep_cnt++;
        if (rep_cnt> 1 )
        {
            usleep(200000);
            printf("CRemoteControl - getNVODs - try #%d\n", rep_cnt);
        }

        int sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        SAI servaddr;
        memset(&servaddr,0,sizeof(servaddr));
        servaddr.sin_family=AF_INET;
        servaddr.sin_port=htons(sectionsd::portNumber);
        inet_pton(AF_INET, rip, &servaddr.sin_addr);

        if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
        {
            perror("CRemoteControl - getNVODs - couldn't connect to sectionsd!\n");
        }
        else
        {
            sectionsd::msgRequestHeader req;
            req.version = 2;
            req.command = sectionsd::timesNVODservice;
            req.dataLength = 4;
            write(sock_fd, &req, sizeof(req));
            write(sock_fd, &onidSid, req.dataLength);
            sectionsd::msgResponseHeader resp;
            memset(&resp, 0, sizeof(resp));
            if(read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader))<=0)
            {
                close(sock_fd);
                return;
            }

            if(resp.dataLength)
            {
                char* pData = new char[resp.dataLength] ;
                if(read(sock_fd, pData, resp.dataLength)>0)
                {
                    //printf("dataLength: %u\n", resp.dataLength);
                    char *p=pData;

                    while(p<pData+resp.dataLength)
                    {
                        unsigned onidsid2=*(unsigned *)p;
                        p+=4;
                        unsigned short tsid=*(unsigned short *)p;
                        p+=2;
                        time_t zeit=*(time_t *)p;
                        p+=4;
                        unsigned dauer = *(unsigned *)p;
                        p+=4;

                        if (dauer> 0)
                            nvod_list.insert( CSubService(onidsid2, tsid, zeit, dauer) );
                    }
                }
                delete[] pData;
            }
            close(sock_fd);
        }
        pthread_mutex_trylock( &send_mutex );

    } while ( ( nvod_list.size()== 0 ) && ( rep_cnt< 10) && ( strlen( audio_chans_int.name )!= 0 ) );

    subChannels_internal.clear( channel_name );

    for(CSubServiceListSorted::iterator nvod=nvod_list.begin(); nvod!=nvod_list.end(); ++nvod)
        subChannels_internal.list.insert(subChannels_internal.list.end(), * nvod );
}

void * CRemoteControl::RemoteControlThread (void *arg)
{
	CRemoteControl* RemoteControl = (CRemoteControl*) arg;

    int sock_fd;
	SAI servaddr;
	char rip[]="127.0.0.1";
    bool redo, do_immediatly;

	while(1)
	{
//        printf("CRemoteControl: before pthread_cond_wait\n");

        pthread_mutex_trylock( &RemoteControl->send_mutex );
        pthread_cond_wait( &RemoteControl->send_cond, &RemoteControl->send_mutex );
      
//        printf("CRemoteControl: after pthread_cond_wait for %s\n", RemoteControl->remotemsg.param3);

        st_rmsg r_msg;

        do
        {
            pthread_mutex_trylock( &RemoteControl->send_mutex );
            memcpy( &r_msg, &RemoteControl->remotemsg, sizeof(r_msg) );
            pthread_mutex_unlock( &RemoteControl->send_mutex );

            memset(&servaddr,0,sizeof(servaddr));
            servaddr.sin_family=AF_INET;

            #ifdef HAS_SIN_LEN
            servaddr.sin_len = sizeof(servaddr); // needed ???
           	#endif
            inet_pton(AF_INET, rip, &servaddr.sin_addr);
            sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

            if ( RemoteControl->zapit_mode )
            {
                char *return_buf;
                int bytes_recvd = 0;
	
                servaddr.sin_port=htons(1505);
            	if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
            	{
              		perror("CRemoteControl::RemoteControlThread - Couldn't connect to serverd zapit!");
//            		exit(-1);
            	}
//                printf("sending %d\n", r_msg.cmd);
                write(sock_fd, &r_msg, sizeof(r_msg));
	
                return_buf = (char*) malloc(4);
                memset(return_buf,0,sizeof(return_buf));
                bytes_recvd = recv(sock_fd, return_buf, 3,0);
                if (bytes_recvd <= 0 )
                {
                    perror("CRemoteControl::RemoteControlThread - Nothing could be received from serverd zapit\n");
//                    exit(-1);
                }
//                printf("Received %d bytes\n", bytes_recvd);
//                printf("That was returned: %s\n", return_buf);
	
                char ZapStatus = return_buf[1];

                do_immediatly = false;

                if ( return_buf[0] == '-' )
                {
                    printf("zapit failed for function >%s<\n", &return_buf[2]);
                }
                else
                {
                    switch ( return_buf[2] )
                    {
                        case '0':   printf("Unknown error reported from zapper\n");
                                    break;
                        case '1':   {
                                        printf("Zapping by number returned successful\n");
                                        break;
                                    }
                        case '2':   printf("zapit should be killed now.\n");
                                    break;
                        case '3':   {
                                        // printf("Zapping by name returned successful\n");

                                        // ueberpruefen, ob wir die Audio-PIDs holen sollen...
                                        // printf("Checking for Audio-PIDs %s - %s - %d\n", RemoteControl->remotemsg.param3, r_msg.param3, RemoteControl->remotemsg.cmd);
                                        pthread_mutex_trylock( &RemoteControl->send_mutex );
                                        if ( ( RemoteControl->remotemsg.cmd== 3 ) &&
                                             ( strcmp(RemoteControl->remotemsg.param3, r_msg.param3 )== 0 ) )
                                        {
                                            // noch immer der gleiche Kanal, Abfrage 8 starten
                                            RemoteControl->remotemsg.cmd= 8;

                                            strcpy( RemoteControl->audio_chans_int.name, r_msg.param3 );
                                            do_immediatly = true;
                                            // printf("Audio-PIDs holen for %s\n", RemoteControl->apids.name);
                                        }
                                        else
                                            pthread_mutex_unlock( &RemoteControl->send_mutex );

                                        break;
                                    }
                                    break;
                        case '4':   printf("Shutdown Box returned successful\n");
                                    break;
                        case '5':   printf("get Channellist returned successful\n");
                                    printf("Should not be received in remotecontrol.cpp. Exiting\n");
                                    break;
                        case '6':   printf("Changed to radio-mode\n");
                                    break;
                        case '7':   printf("Changed to TV-mode\n");
                                    break;
                        case '8':
                        case 'd':
                        case 'e':   {
                                        struct  pids    apid_return_buf;
                                        memset(&apid_return_buf, 0, sizeof(apid_return_buf));

                                        if ( read(sock_fd, &apid_return_buf, sizeof(apid_return_buf)) > 0 )
                                        {
                                            // PIDs emfangen...

                                            pthread_mutex_trylock( &RemoteControl->send_mutex );
                                            if ( ( strlen( RemoteControl->audio_chans_int.name )!= 0 ) ||
                                                 ( ( strcmp(RemoteControl->remotemsg.param3, r_msg.param3 )== 0 ) && (return_buf[2] == 'd') ) ||
                                                 (return_buf[2] == 'e') )
                                            {
                                                // noch immer der gleiche Kanal

                                                if ( (return_buf[2] == 'd') && ( ZapStatus & 0x80 ) )
                                                {
                                                    RemoteControl->getNVODs( r_msg.param3 );
                                                    // send_mutex ist danach wieder locked

                                                    printf("NVOD-Basechannel - got %d nvods for >%s<!\n", RemoteControl->subChannels_internal.list.size(), RemoteControl->subChannels_internal.name.c_str());

                                                    if ( RemoteControl->subChannels_internal.list.size()> 0 )
                                                    {
                                                        // übertragen der ids an zapit initialisieren
                                                        RemoteControl->remotemsg.cmd= 'i';
                                                        RemoteControl->remotemsg.param= 1;
                                                        do_immediatly = true;
                                                    }
                                                }
                                                else
                                                {
                                                    if (return_buf[2] == 'd')
                                                        strcpy( RemoteControl->audio_chans_int.name, r_msg.param3 );
                                                    if (return_buf[2] == 'e')
                                                        strcpy( RemoteControl->audio_chans_int.name, RemoteControl->subChannels_internal.name.c_str() );

                                                    // Nur dann die Audio-Channels abholen, wenn nicht NVOD-Basechannel

                                                    RemoteControl->audio_chans_int.count_apids = apid_return_buf.count_apids;
                                                    printf("got apids for: %s - %d apids!\n", RemoteControl->audio_chans_int.name, RemoteControl->audio_chans_int.count_apids);
                                                    // printf("%d - %d - %d - %d - %d\n", apid_return_buf.apid[0], apid_return_buf.apid[1], apid_return_buf.apid[2], apid_return_buf.apid[3], apid_return_buf.apid[4] );
                                                    for(int count=0;count<apid_return_buf.count_apids;count++)
                                                    {
                                                        // printf("%s \n", apid_return_buf.apids[count].desc);
                                                        strcpy(RemoteControl->audio_chans_int.apids[count].name, apid_return_buf.apids[count].desc);
                                                        RemoteControl->audio_chans_int.apids[count].ctag= apid_return_buf.apids[count].component_tag;
                                                        RemoteControl->audio_chans_int.apids[count].is_ac3= apid_return_buf.apids[count].is_ac3;
                                                    }
                                                    RemoteControl->ecm_pid= apid_return_buf.ecmpid;
                                                }

                                                pthread_cond_signal( &g_InfoViewer->lang_cond );
                                            }
                                            if (!do_immediatly)
                                                pthread_mutex_unlock( &RemoteControl->send_mutex );
                                        }
                                        else
                                            printf("pid-description fetch failed!\n");
                                        break;
                                    }
                        case 'i':   {
                                        pthread_mutex_trylock( &RemoteControl->send_mutex );
                                        unsigned short nvodcount= RemoteControl->subChannels_internal.list.size();
                                        write(sock_fd, &nvodcount, 2);

                                        //printf("Sending NVODs to zapit\n");
                                        for(int count=0; count<nvodcount; count++)
                                        {
                                            write(sock_fd, &RemoteControl->subChannels_internal.list[count].onid_sid, 4);
                                            write(sock_fd, &RemoteControl->subChannels_internal.list[count].tsid, 2);
                                        }

                                        if (RemoteControl->remotemsg.param== 1)
                                        {
                                            // called from NVOD - immediately change to nvod #max...
                                            RemoteControl->remotemsg.cmd= 'e';
                                            RemoteControl->subChannels_internal.selected= nvodcount- 1;
                                            snprintf( (char*) &RemoteControl->remotemsg.param3, 10, "%x", RemoteControl->subChannels_internal.list[nvodcount- 1].onid_sid);

                                            do_immediatly = true;
                                        }

                                        // pthread_mutex_unlock( &RemoteControl->send_mutex );
                                        break;
                                    }
                        case '9':   printf("Changed apid\n");
                                    break;

                        default: printf("Unknown return-code >%s<, %d\n", return_buf, return_buf[2]);
                    }
                }
                if ( !do_immediatly )
                    usleep(100000);
                    //usleep(100);
            }
            else
            {
                servaddr.sin_port=htons(1500);
                if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))!=-1)
                {
                    write(sock_fd, &r_msg, sizeof(r_msg) );

                    usleep(1500000);
                };
            }

            close(sock_fd);

            pthread_mutex_trylock( &RemoteControl->send_mutex );
            redo= memcmp(&r_msg, &RemoteControl->remotemsg, sizeof(r_msg)) != 0;

        } while ( redo );
	}
	return NULL;
}

unsigned int CRemoteControl::GetECMPID()
{
    pthread_mutex_lock( &send_mutex );
    int ep = ecm_pid;
    pthread_mutex_unlock( &send_mutex );
    return ep;
}

void CRemoteControl::CopyAPIDs()
{
    pthread_mutex_lock( &send_mutex );
    memcpy(&audio_chans, &audio_chans_int, sizeof(audio_chans));
    pthread_mutex_unlock( &send_mutex );
}

const CSubChannel_Infos CRemoteControl::getSubChannels()
{
    pthread_mutex_lock( &send_mutex );
    CSubChannel_Infos subChannels(subChannels_internal);
    pthread_mutex_unlock( &send_mutex );
    return  subChannels;
}

void CRemoteControl::CopySubChannelsToZapit( const CSubChannel_Infos& subChannels )
{
    pthread_mutex_lock( &send_mutex );
    subChannels_internal= subChannels;

    remotemsg.version=1;
    remotemsg.cmd='i';
    remotemsg.param= 0;
    //printf("sending subservices\n");
    pthread_mutex_unlock( &send_mutex );
    send();
}


void CRemoteControl::queryAPIDs()
{
    pthread_mutex_lock( &send_mutex );

    remotemsg.version=1;
    remotemsg.cmd=8;

    pthread_mutex_unlock( &send_mutex );
	send();
}

void CRemoteControl::setAPID(int APID)
{
    pthread_mutex_lock( &send_mutex );

    remotemsg.version=1;
    remotemsg.cmd=9;
    snprintf( (char*) &remotemsg.param, 2, "%.1d", APID);
    audio_chans_int.selected = APID;
    // printf("changing APID to %d\n", audio_chans_int.selected);

    pthread_mutex_unlock( &send_mutex );
	send();
}

void CRemoteControl::setSubChannel(unsigned numSub)
{
    pthread_mutex_lock( &send_mutex );
    if (subChannels_internal.selected== numSub )
    {
        pthread_mutex_unlock( &send_mutex );
        return;
    }
    memset(&audio_chans_int, 0, sizeof(audio_chans_int));

    remotemsg.version=1;
    remotemsg.cmd='e';
    snprintf( (char*) &remotemsg.param3, 10, "%x", subChannels_internal.list[numSub].onid_sid);
    subChannels_internal.selected = numSub;

    pthread_mutex_unlock( &send_mutex );
    send();
}


void CRemoteControl::zapTo_onid_sid( unsigned int onid_sid )
{
    pthread_mutex_lock( &send_mutex );
    remotemsg.version=1;
    remotemsg.cmd= 'd';
    snprintf( (char*) &remotemsg.param3, 10, "%x", onid_sid);

    memset(&audio_chans_int, 0, sizeof(audio_chans_int));
    subChannels_internal.clear();

    pthread_mutex_unlock( &send_mutex );

	send();
}

void CRemoteControl::zapTo(string chnlname )
{
    pthread_mutex_lock( &send_mutex );
    remotemsg.version=1;
    remotemsg.cmd=3;
    remotemsg.param=0x0100;
    strcpy( remotemsg.param3, chnlname.c_str() );

    memset(&audio_chans_int, 0, sizeof(audio_chans_int));
    subChannels_internal.clear();

    pthread_mutex_unlock( &send_mutex );

	send();
}

void CRemoteControl::radioMode()
{
    pthread_mutex_lock( &send_mutex );

	remotemsg.version=1;
	remotemsg.cmd=6;
	
    pthread_mutex_unlock( &send_mutex );

	send();
}

void CRemoteControl::tvMode()
{
    pthread_mutex_lock( &send_mutex );

	remotemsg.version=1;
	remotemsg.cmd=7;

    pthread_mutex_unlock( &send_mutex );	

	send();
}


void  CRemoteControl::shutdown()
{
    pthread_mutex_lock( &send_mutex );

    remotemsg.version=1;
    remotemsg.cmd=4;

    pthread_mutex_unlock( &send_mutex );

    send();
}


