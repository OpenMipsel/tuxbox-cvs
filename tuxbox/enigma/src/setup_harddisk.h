#ifndef __lib_apps_enigma_setup_harddisk_h
#define __lib_apps_enigma_setup_harddisk_h

#include <lib/gui/listbox.h>
#include <lib/gui/ebutton.h>

class eHarddiskSetup: public eListBoxWindow<eListBoxEntryText>
{
	int nr;
	void selectedHarddisk(eListBoxEntryText *sel);
public:
	eHarddiskSetup();
	int getNr() const { return nr; }
};

class eHarddiskMenu: public eWindow
{
	eButton *close, *format, *bcheck;
	eLabel *status, *model, *capacity, *bus;
	int dev;
	int numpart;
	void s_format();
	void check();
	void readStatus();
public:
	eHarddiskMenu(int dev);
};

class ePartitionCheck: public eWindow
{
	eLabel *lState;
	eButton *bCancel;
	int dev;
	char *outbuf;
	int fd[2];
	eSocketNotifier *in, *out;
	void msgAvail(int what);
	void strWritten(int what);
	void onCancel();
	int eventHandler( const eWidgetEvent &e );
public:
	ePartitionCheck( int dev );
};

#endif
