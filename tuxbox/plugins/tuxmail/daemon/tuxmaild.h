/******************************************************************************
 *                       <<< TuxMailD - POP3 Daemon >>>
 *                (c) Thomas "LazyT" Loewe 2003 (LazyT@gmx.net)
 *-----------------------------------------------------------------------------
 * $Log: tuxmaild.h,v $
 * Revision 1.14  2005/05/10 12:55:16  lazyt
 * - LCD-Fix for DM500
 * - Autostart for DM7020 (use -DOE, put Init-Script to /etc/init.d/tuxmail)
 * - try again after 10s if first DNS-Lookup failed
 * - don't try to read Mails on empty Accounts
 *
 * Revision 1.13  2005/05/09 19:33:15  robspr1
 * support for mail reading
 *
 * Revision 1.12  2005/03/28 14:14:15  lazyt
 * support for userdefined audio notify (put your 12/24/48KHz pcm wavefile to /var/tuxbox/config/tuxmail/tuxmail.wav)
 *
 * Revision 1.11  2005/03/24 13:12:11  lazyt
 * cosmetics, support for syslog-server (start with -syslog)
 *
 * Revision 1.10  2005/03/22 13:31:48  lazyt
 * support for english osd (OSD=G/E)
 *
 * Revision 1.9  2005/03/22 09:35:21  lazyt
 * lcd support for daemon (LCD=Y/N, GUI should support /tmp/lcd.locked)
 *
 * Revision 1.8  2005/03/14 17:45:27  lazyt
 * simple base64 & quotedprintable decoding
 *
 * Revision 1.7  2005/02/26 10:23:49  lazyt
 * workaround for corrupt mail-db
 * add ADMIN=Y/N to conf (N to disable mail deletion via plugin)
 * show versioninfo via "?" button
 * limit display to last 100 mails (increase MAXMAIL if you need more)
 *
 * Revision 1.6  2004/08/20 14:57:37  lazyt
 * add http-auth support for password protected webinterface
 *
 * Revision 1.5  2004/04/03 17:33:08  lazyt
 * remove curl stuff
 * fix audio
 * add new options PORT=n, SAVEDB=Y/N
 *
 * Revision 1.4  2003/05/16 15:07:23  lazyt
 * skip unused accounts via "plus/minus", add mailaddress to spamlist via "blue"
 *
 * Revision 1.3  2003/05/10 08:24:35  lazyt
 * add simple spamfilter, show account details in message/popup
 *
 * Revision 1.2  2003/04/29 10:36:43  lazyt
 * enable/disable audio via .conf
 *
 * Revision 1.1  2003/04/21 09:24:52  lazyt
 * add tuxmail, todo: sync (filelocking?) between daemon and plugin
 ******************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/soundcard.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <syslog.h>

#include "audio.h"

#define DSP "/dev/sound/dsp"
#define LCD "/dev/dbox/lcd0"

#define RIFF	0x46464952
#define WAVE	0x45564157
#define FMT	0x20746D66
#define DATA	0x61746164
#define PCM	1

#define CFGPATH "/var/tuxbox/config/tuxmail/"
#define CFGFILE "tuxmail.conf"
#define SPMFILE "spamlist"
#define SNDFILE "tuxmail.wav"
#define SCKFILE "/tmp/tuxmaild.socket"
#define LOGFILE "/tmp/tuxmaild.log"
#define PIDFILE "/tmp/tuxmaild.pid"
#define LCKFILE "/tmp/lcd.locked"
#define MAILFILE "/tmp/popmail"

#define bool char
#define true 1
#define false 0

// maximum number of chars in a line
#define cnRAND  	78
// maximum charcters in a word
#define cnMaxWordLen	20

FILE *fd_mail;
int  nStartSpalte, nCharInLine, nCharInWord, nRead, nWrite, nStrich ; 
int  nIn, nSo, nTr; 
char  cLast; 
bool  fPre; 							//! pre-formated HTML code
bool  fHtml; 							//! HTML code
int  nCRLF = 0; 
int  nLine = 1; 
int  nRef  = 1;
int   nHyp  = 0 ;
char  sSond[355],sRef[355], sWord[85];
static enum  t_state { cNorm, cInTag, cSond, cInComment, cTrans } state ;

#define szsize 64

char *szTab[szsize] = {
  /*192 */ "Agrave"  ,   /*193 */ "Aacute"  ,
  /*194 */ "Acirc"   ,   /*195 */ "Atilde"  ,
  /*196 */ "Auml"    ,   /*197 */ "Aring"   ,
  /*198 */ "Aelig"   ,   /*199 */ "Ccedil"  ,
  /*200 */ "Egrave"  ,   /*201 */ "Eacute"  ,
  /*202 */ "Ecirc"   ,   /*203 */ "Euml"    ,
  /*204 */ "Igrave"  ,   /*205 */ "Iacute"  ,
  /*206 */ "Icirc"   ,   /*207 */ "Iuml"    ,
  /*208 */ "ETH"     ,   /*209 */ "Ntilde"  ,
  /*210 */ "Ograve"  ,   /*211 */ "Oacute"  ,
  /*212 */ "Ocirc"   ,   /*213 */ "Otilde"  ,
  /*214 */ "Ouml"    ,   /*215 */ "XXXXXX"  ,
  /*216 */ "Oslash"  ,   /*217 */ "Ugrave"  ,
  /*218 */ "Uacute"  ,   /*219 */ "Ucirc"   ,
  /*220 */ "Uuml"    ,   /*221 */ "Yacute"  ,
  /*222 */ "THORN"   ,   /*223 */ "szlig"   ,
  /*224 */ "agrave"  ,   /*225 */ "aacute"  ,
  /*226 */ "acirc"   ,   /*227 */ "atilde"  ,
  /*228 */ "auml"    ,   /*229 */ "aring"   ,
  /*230 */ "aelig"   ,   /*231 */ "ccedil"  ,
  /*232 */ "egrave"  ,   /*233 */ "eacute"  ,
  /*234 */ "ecirc"   ,   /*235 */ "euml"    ,
  /*236 */ "igrave"  ,   /*237 */ "iacute"  ,
  /*238 */ "icirc"   ,   /*239 */ "iuml"    ,
  /*240 */ "eth"     ,   /*241 */ "ntilde"  ,
  /*242 */ "ograve"  ,   /*243 */ "oacute"  ,
  /*244 */ "ocirc"   ,   /*245 */ "otilde"  ,
  /*246 */ "ouml"    ,   /*247 */ "XXXXXX"  ,
  /*248 */ "oslash"  ,   /*249 */ "ugrave"  ,
  /*250 */ "uacute"  ,   /*251 */ "ucirc"   ,
  /*252 */ "uuml"    ,   /*253 */ "yacute"  ,
  /*254 */ "thorn"   ,   /*255 */ "yuml"
};

#define ttsize  9

char ttable[ttsize*3] = {
	'F','C', 252,
  	'D','F', 223,
	'E','4', 228,
	'F','6', 246,
	'D','6', 214,
	'3','D', '=',
	'2','0',  20,
	'0','D',  13,
	13 , 10,  0
};

void writeFOut( char *s);
int SaveMail(int account, char* uid);

// pop3 commands

enum {INIT, USER, PASS, STAT, UIDL, TOP, DELE, QUIT, RETR};

// account database

struct
{
	char name[32];
	char host[64];
	char user[64];
	char pass[64];
	int  mail_all;
	int  mail_new;

}account_db[10];

// spam database

struct
{
	char address[64];

}spamfilter[100];

// waveheader

struct WAVEHEADER
{
	unsigned long	ChunkID1;
	unsigned long	ChunkSize1;
	unsigned long	ChunkType;

	unsigned long	ChunkID2;
	unsigned long	ChunkSize2;
	unsigned short	Format;
	unsigned short	Channels;
	unsigned long	SampleRate;
	unsigned long	BytesPerSecond;
	unsigned short	BlockAlign;
	unsigned short	BitsPerSample;

	unsigned long	ChunkID3;
	unsigned long	ChunkSize3;
};

// some data

char versioninfo[12];
FILE *fd_pid;
int slog = 0;
int pid;
int webport;
char webuser[32], webpass[32];
char plainstring[64], encodedstring[64];
char decodedstring[512];
int startdelay, intervall, skin;
char pop3log, logmode, audio, lcd, osd, admin, savedb, mailrd;
int video;
char online = 1;
char mailread = 0;
char inPOPCmd = 0;
int accounts;
int sock;
int messages, deleted_messages;
int stringindex;
int use_spamfilter, spam_entries, spam_detected;
char uid[128];
char header[1024];
int headersize;
char timeinfo[22];
time_t tt;

// lcd stuff

unsigned char lcd_buffer[] =
{
	0xE0, 0xF8, 0xFC, 0xFE, 0xFE, 0xFF, 0x7F, 0x7F, 0x7F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0xFF, 0xFF, 0xFF, 0xC7, 0xBB, 0x3B, 0xFB, 0xFB, 0x3B, 0xBB, 0xC7, 0xFF, 0x07, 0xFB, 0xFB, 0x07, 0xFB, 0xFB, 0xFB, 0x07, 0xFF, 0x87, 0x7B, 0xFB, 0xC7, 0xFB, 0x7B, 0x7B, 0x87, 0xFF, 0x07, 0xFB, 0xFB, 0x3B, 0xFB, 0xFB, 0xFB, 0x3B, 0xFB, 0xFB, 0x07, 0xFF, 0x07, 0xFB, 0xFB, 0xBB, 0xFB, 0xFB, 0xFB, 0x07, 0xFF, 0x27, 0xDB, 0xDB, 0x27, 0xFF, 0x07, 0xFB, 0xFB, 0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x7F, 0x7F, 0x7F, 0xFF, 0xFE, 0xFE, 0xFC, 0xF8, 0xE0,
	0xFF, 0x7F, 0x7F, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x70, 0x6F, 0x6F, 0x70, 0x7F, 0x7F, 0x7F, 0x70, 0x6F, 0x6F, 0x6C, 0x6F, 0x6F, 0x6F, 0x70, 0x7F, 0x70, 0x6F, 0x6F, 0x71, 0x6F, 0x6F, 0x6F, 0x70, 0x7F, 0x70, 0x6F, 0x6F, 0x70, 0x6F, 0x6F, 0x6F, 0x70, 0x6F, 0x6F, 0x70, 0x7F, 0x70, 0x6F, 0x6F, 0x71, 0x6F, 0x6F, 0x6F, 0x70, 0x7F, 0x70, 0x6F, 0x6F, 0x70, 0x7F, 0x70, 0x6F, 0x6F, 0x6C, 0x6D, 0x6D, 0x6D, 0x73, 0x7F, 0x7F, 0x7F, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7F, 0x7F, 0xFF,
	0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
	0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x01, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x01, 0xFF, 0x00, 0x00, 0xFF, 0x01, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x01, 0xFF, 0x00, 0x00, 0xFF, 0x01, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x01, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
	0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0xC0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x30, 0x20, 0x20, 0x20, 0x21, 0x20, 0x20, 0x30, 0x1F, 0x00, 0x00, 0x1F, 0x30, 0x20, 0x20, 0x20, 0x21, 0x20, 0x20, 0x30, 0x1F, 0x00, 0x00, 0x1F, 0x30, 0x20, 0x20, 0x20, 0x21, 0x20, 0x20, 0x30, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
	0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00, 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x40, 0x80, 0x80, 0x40, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00, 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x04, 0x04, 0x04, 0xC4, 0x04, 0x04, 0x04, 0xC4, 0x04, 0x04, 0x04, 0xF8, 0x00, 0xF8, 0x0C, 0x04, 0x04, 0x44, 0x04, 0x04, 0x0C, 0xF8, 0x00, 0xB8, 0x44, 0x44, 0x44, 0xB8, 0x00, 0xF8, 0x04, 0x04, 0x04, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
	0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x60, 0x50, 0x48, 0x44, 0x42, 0x41, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x41, 0x42, 0x44, 0x48, 0x50, 0x60, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x40, 0x40, 0x40, 0x3F, 0x40, 0x40, 0x40, 0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00, 0x3F, 0x40, 0x40, 0x40, 0x3C, 0x40, 0x40, 0x40, 0x3F, 0x00, 0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00, 0x3F, 0x40, 0x40, 0x40, 0x43, 0x42, 0x42, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
	0x07, 0x18, 0x20, 0x40, 0x40, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x40, 0x40, 0x20, 0x18, 0x07,
};

char lcd_digits[] =
{
	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,1,1,1,1,1,1,0,

	0,0,0,1,1,1,1,0,0,0,
	0,0,1,1,0,0,1,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,1,0,0,1,1,0,0,
	0,0,0,1,1,1,1,0,0,0,

	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	0,1,1,1,1,1,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,1,1,
	1,0,0,0,0,1,1,1,1,0,
	1,0,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,1,1,1,1,1,1,0,

	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	0,1,1,1,1,1,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,1,1,1,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,1,1,1,1,1,1,0,

	0,1,1,1,0,1,1,1,1,0,
	1,1,0,1,1,1,0,0,1,1,
	1,0,0,0,1,0,0,0,0,1,
	1,0,0,0,1,0,0,0,0,1,
	1,0,0,0,1,0,0,0,0,1,
	1,0,0,0,1,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	0,1,1,1,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,1,0,0,1,1,
	0,0,0,0,0,1,1,1,1,0,

	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,1,1,
	1,0,0,0,0,1,1,1,1,0,
	1,0,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	0,1,1,1,1,1,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,1,1,1,1,1,1,0,

	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,1,1,
	1,0,0,0,0,1,1,1,1,0,
	1,0,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,1,1,1,1,1,1,0,

	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	0,1,1,1,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,1,0,0,1,1,
	0,0,0,0,0,1,1,1,1,0,

	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,0,0,0,0,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,1,1,1,1,1,1,0,

	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	0,1,1,1,1,0,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,1,1,1,1,1,1,0,
};
