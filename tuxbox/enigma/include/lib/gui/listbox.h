#ifndef __listbox_h
#define __listbox_h

#include <sstream>

#include <lib/driver/rc.h>
#include <lib/gdi/grc.h>
#include <lib/gdi/fb.h>
#include <lib/gui/ewidget.h>
#include <lib/gui/eskin.h>
#include <lib/gui/ewindow.h>
#include <lib/gui/guiactions.h>
#include <lib/gui/statusbar.h>

int calcFontHeight( const gFont& font );

class eListBoxEntry;

class eListBoxBase: public eDecoWidget
{
	ePtrList<eListBoxEntry>::iterator top, bottom, current;
	int recalced;
	const eWidget* descr;
	eLabel* tmpDescr; // used for description Label in LCD
	gColor colorActiveB, colorActiveF;
	eRect crect, crect_selected;
	enum  { arNothing, arCurrentOld, arAll};
	int movemode, MaxEntries, flags, item_height, columns, in_atomic, atomic_redraw, atomic_old, atomic_new;
	bool atomic_selchanged;
	bool atomic_selected;
protected:
	eListBoxBase(eWidget* parent, const eWidget* descr=0, int takefocus=1, int item_height=0, const char *deco="eListBox" );
	ePtrList<eListBoxEntry> childs;
	eListBoxEntry* getCurrent()	{ return current != childs.end() ? *current : 0; }
	eListBoxEntry* getNext() { ePtrList<eListBoxEntry>::iterator c=current; ++c; return c != childs.end() ? *c : 0; }
	eListBoxEntry* goNext();
	eListBoxEntry* goPrev();
	int setProperty(const eString &prop, const eString &value);
private:
	eRect getEntryRect(int n);
	int eventHandler(const eWidgetEvent &event);
	void recalcMaxEntries();
	void recalcClientRect();
	int newFocus();
	void redrawWidget(gPainter *target, const eRect &area);
	void lostFocus();
	void gotFocus();
	void init();
	inline virtual void SendSelected( eListBoxEntry* entry )=0;
	inline virtual void SendSelChanged( eListBoxEntry* entry )=0;
public:
	~eListBoxBase();
	enum	{		dirPageDown, dirPageUp, dirDown, dirUp, dirFirst, dirLast	};
	enum	{		flagNoUpDownMovement=1,		flagNoPageMovement=2,		flagShowEntryHelp=4, flagShowPartial=8 };
	enum	{		OK = 0,		ERROR=1,		E_ALLREADY_SELECTED = 2,		E_COULDNT_FIND = 4,		E_INVALID_ENTRY = 8,	 E_NOT_VISIBLE = 16		};
	void setFlags(int);
	void removeFlags(int);
	void invalidateEntry(int n){	invalidate(getEntryRect(n));}
	void invalidateContent();
	void setColumns(int col);
	int getColumns() { return columns; }
	void setMoveMode(int move) { movemode=move; }
	void append(eListBoxEntry* e, bool holdCurrent=false, bool front=false);
	void remove(eListBoxEntry* e, bool holdCurrent=false);
	void clearList();
	int getCount() { return childs.size(); }
	int setCurrent(const eListBoxEntry *c, bool sendSelected=false);
	void sort();
	int moveSelection(int dir, bool sendSelected=false);
	void setActiveColor(gColor back, gColor front);
	void beginAtomic();
	void endAtomic();
	void FakeFocus( int i ) { have_focus=i; }
	void invalidateCurrent()
	{
		int n=0;
		for (ePtrList<eListBoxEntry>::iterator i(top); i != bottom; ++i, n++)
			if ( i == current )
				invalidate(getEntryRect(n));
	}
};

template <class T>
class eListBox: public eListBoxBase
{
	void SendSelected( eListBoxEntry* entry )
	{
		/*emit*/ selected((T*)entry);
	}
	void SendSelChanged( eListBoxEntry* entry )
	{
		/*emit*/ selchanged((T*)entry);
	}
public:
	Signal1<void, T*> selected;
	Signal1<void, T*> selchanged;
	eListBox(eWidget *parent, const eWidget* descr=0, int takefocus=1 )
		:eListBoxBase( parent, descr, takefocus, T::getEntryHeight() )
	{
	}
	T* getCurrent()	{ return (T*)eListBoxBase::getCurrent(); }
	T* getNext() { return (T*)eListBoxBase::getNext(); }
	T* goNext() { return (T*)eListBoxBase::goNext(); }
	T* goPrev() { return (T*)eListBoxBase::goPrev(); }

	template <class Z>
	int forEachEntry(Z ob)
	{
		for (ePtrList<eListBoxEntry>::iterator i(childs.begin()); i!=childs.end(); ++i)
			if ( ob((T&)(**i)) )
				return OK;

		return ERROR;
	}

	template <class Z>
	int forEachVisibleEntry(Z ob)
	{
		if (!isVisible())
			return E_NOT_VISIBLE;

		for (ePtrList<eListBoxEntry>::iterator i(top); i!=bottom; ++i)
			if ( ob((T&)(**i)) )
				return OK;

		return ERROR;
	}
};

class eListBoxEntry: public Object
{
	friend class eListBox<eListBoxEntry>;
protected:
	eListBox<eListBoxEntry>* listbox;
	eString helptext;
public:
	eListBoxEntry(eListBox<eListBoxEntry>* parent, eString hlptxt=0)
		:listbox(parent), helptext(hlptxt?hlptxt:_("no description avail"))
	{
		if (listbox)
			listbox->append(this);
	}
	virtual ~eListBoxEntry()
	{
		if (listbox)
			listbox->remove(this);
	}
	virtual bool operator < ( const eListBoxEntry& e)const
	{
		return false;
	}
	virtual const eString& redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int state )=0;
	void drawEntryRect(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int state);
	void drawEntryBorder(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF);
	const eString &getHelpText() const { return helptext; }
};

class eListBoxEntryText: public eListBoxEntry
{
	friend class eListBox<eListBoxEntryText>;
protected:
	eString text;
	void *key;
	int align;
	eTextPara *para;
	int yOffs;
	static gFont font;
	int keytype;
public:
	enum { value, ptr };
	static int getEntryHeight();

	eListBoxEntryText(eListBox<eListBoxEntryText>* lb, const char* txt=0, void *key=0, int align=0, const eString &hlptxt="", int keytype = value )
		:eListBoxEntry( (eListBox<eListBoxEntry>*)lb, hlptxt ), text(txt),
		 key(key), align(align), para(0), keytype(keytype)
	{
	}

	eListBoxEntryText(eListBox<eListBoxEntryText>* lb, const eString& txt, void* key=0, int align=0, const eString &hlptxt="", int keytype = value )
		:eListBoxEntry( (eListBox<eListBoxEntry>*)lb, hlptxt ), text(txt),
		 key(key), align(align), para(0), keytype(keytype)
	{
	}

	~eListBoxEntryText();

	bool operator < ( const eListBoxEntry& e) const
	{
		if (key == ((eListBoxEntryText&)e).key || keytype == ptr)
			return text < ((eListBoxEntryText&)e).text;
		else
			return key < ((eListBoxEntryText&)e).key;
	}

	void *& getKey() { return key; }
	const void* getKey() const { return key; }
	const eString& getText() { return text; }
protected:
	const eString& redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int state );
};

class eListBoxEntryTextStream: public eListBoxEntry
{
	friend class eListBox<eListBoxEntryTextStream>;
protected:
	std::stringstream text;
	static gFont font;
public:
	static int getEntryHeight();

	eListBoxEntryTextStream(eListBox<eListBoxEntryTextStream>* lb)
		:eListBoxEntry((eListBox<eListBoxEntry>*)lb)
	{
	}

	bool operator < ( const eListBoxEntryTextStream& e) const
	{
		return text.str() < e.text.str();
	}

protected:
	const eString &redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int state );
};

class eListBoxEntryMenu: public eListBoxEntryText
{
	friend class eListBox<eListBoxEntryMenu>;
public:
	Signal0<void> selected;

	eListBoxEntryMenu(eListBox<eListBoxEntryMenu>* lb, const char* txt, const eString &hlptxt="", int align=0 )
		:eListBoxEntryText((eListBox<eListBoxEntryText>*)lb, txt, 0, align, hlptxt)
	{
		if (listbox)
			CONNECT(listbox->selected, eListBoxEntryMenu::LBSelected);
	}
	void LBSelected(eListBoxEntry* t)
	{
		if (t == this)
			/* emit */ selected();
	}
};

template <class T>
class eListBoxWindow: public eWindow
{
protected:
	int Entrys;
	int width;
	eListBox<T> list;
	eStatusBar *statusbar;
public:
	eListBoxWindow(eString Title="", int Entrys=0, int width=400, bool sbar=0);
};

template <class T>
inline eListBoxWindow<T>::eListBoxWindow(eString Title, int Entrys, int width, bool sbar)
	: eWindow(0), Entrys(Entrys), width(width), list(this), statusbar(sbar?new eStatusBar(this):0)
{
	list.setFlags( eListBoxBase::flagShowEntryHelp );
	setText(Title);
	cresize( eSize(width, (sbar?40:10)+Entrys*T::getEntryHeight() ) );
	list.move(ePoint(10, 5));
	list.resize(eSize(getClientSize().width()-20, getClientSize().height()-(sbar?35:5) ));
	if (sbar)
	{
		statusbar->setFlags(eStatusBar::flagVCenter);
		statusbar->move( ePoint(0, getClientSize().height()-30) );
		statusbar->resize( eSize( getClientSize().width(), 30) );
		statusbar->loadDeco();
		statusbar->show();
	}
}

#endif
