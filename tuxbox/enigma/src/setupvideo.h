#ifndef __setupvideo_h
#define __setupvideo_h

#include <lib/gui/ewindow.h>
#include <lib/gui/listbox.h>
#include <lib/gui/statusbar.h>

class eNumber;
class eButton;
class eCheckbox;

class eZapVideoSetup: public eWindow
{
	eButton *ok;
	eStatusBar *status;
	eCheckbox *c_disableWSS, *ac3default;
	eListBox<eListBoxEntryText> *colorformat, *pin8;

	unsigned int v_colorformat, v_pin8, v_disableWSS;
	eStatusBar *statusbar;
private:
	void ac3defaultChanged( int i );
	void CFormatChanged( eListBoxEntryText * );
	void VPin8Changed( eListBoxEntryText *);
	void VDisableWSSChanged(int);
	void okPressed();
	int eventHandler( const eWidgetEvent &e );

public:
	eZapVideoSetup();
	~eZapVideoSetup();
};

#endif
