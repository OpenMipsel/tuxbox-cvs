#ifndef DISABLE_HDD
#ifndef DISABLE_FILE

#ifndef __lib_apps_enigma_setup_timeshift_h
#define __lib_apps_enigma_setup_timeshift_h

#include <lib/gui/ewindow.h>
#include <lib/gui/statusbar.h>

class eButton;
class eCheckbox;
class eNumber;

class eZapTimeshiftSetup: public eWindow
{
	
	eNumber *delay;
	eNumber *minutes;
	eCheckbox* active;
	eCheckbox* pause;
	eButton *store;
	eStatusBar* sbar;
private:
	void storePressed();

public:
	eZapTimeshiftSetup();
	~eZapTimeshiftSetup();
};

#endif

#endif
#endif