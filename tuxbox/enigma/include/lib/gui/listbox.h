#ifndef __listbox_h
#define __listbox_h

#include <core/driver/rc.h>
#include <core/gdi/grc.h>
#include <core/gdi/fb.h>
#include <core/gui/ewidget.h>
#include <core/gui/eskin.h>
#include <core/gui/ewindow.h>
#include <core/gui/guiactions.h>

#include <sstream>

template <class T>
class eListBox: public eWidget
{
	typedef typename ePtrList<T>::iterator ePtrList_T_iterator;
	void redrawWidget(gPainter *target, const eRect &area);
	ePtrList<T> childs;
	ePtrList_T_iterator top, bottom, current;

	int entries, font_size, item_height, flags;
	gColor col_active;
	gFont entryFnt;

	void gotFocus();
	void lostFocus();
	eRect getEntryRect(int n);
	void invalidateEntry(int n);
protected:
	int eventHandler(const eWidgetEvent &event);
public:
	void append(T* e);
	void remove(T* e);
	const gFont& getEntryFnt()	{		return entryFnt;	}
	eListBox(eWidget *parent, int FontSize=20);
	~eListBox();

	Signal1<void, T*> selected;	
	Signal1<void, T*> selchanged;

	void init();
	void clearList();
	void setCurrent(T *c);
	T* getCurrent()	{ return current != childs.end() ? *current : 0; }
	void sort();
	T* goNext();
	T* goPrev();
	int setProperty(const eString &prop, const eString &value);

	int have_focus;
	void setActiveColor(gColor active);
	enum
	{
		dirPageDown, dirPageUp, dirDown, dirUp
	};
	int moveSelection(int dir);
	
	enum
	{
		flagNoUpDownMovement=1,
		flagNoPageMovement=2
	};
	
	void setFlags(int flags);
};

class eListBoxEntry: public Object
{
	friend class eListBox<eListBoxEntry>;
protected:
	eListBox<eListBoxEntry>* listbox;
public:
	eListBoxEntry(eListBox<eListBoxEntry>* parent)
		:listbox(parent)
	{	
		if (listbox)
			listbox->append(this);
	}
	~eListBoxEntry()
	{
		if (listbox)
			listbox->remove(this);
	}
};

class eListBoxEntryText: public eListBoxEntry
{
	friend class eListBox<eListBoxEntryText>;
protected:
	eString text;
	void *key;
public:
	eListBoxEntryText(eListBox<eListBoxEntryText>* lb, const char* txt=0, void *key=0)
		:eListBoxEntry((eListBox<eListBoxEntry>*)lb), text(txt), key(key)
	{
	}

	eListBoxEntryText(eListBox<eListBoxEntryText>* lb, const eString& txt, void* key=0)
		:eListBoxEntry((eListBox<eListBoxEntry>*)lb), text(txt), key(key)
	{
	}

	bool operator < ( const eListBoxEntryText& e) const
	{
		if (key == e.key)
			return text < e.text;	
		else
			return key < e.key;
	}
	
	void *getKey() { return key; }

protected:
	void redraw(gPainter *rc, const eRect& rect, const gColor& coActive, const gColor& coNormal, bool highlited) const
	{
			rc->setForegroundColor(highlited?coActive:coNormal);
			rc->setFont(listbox->getEntryFnt());

			if ((coNormal != -1 && !highlited) || (highlited && coActive != -1))
					rc->fill(rect);

			rc->renderText(rect, text);
			
			eWidget* p = listbox->getParent();			
			if (highlited && p && p->LCDElement)
				p->LCDElement->setText(text);
	}
};

class eListBoxEntryTextStream: public eListBoxEntry
{
	friend class eListBox<eListBoxEntryTextStream>;
protected:
	std::stringstream text;
public:
	eListBoxEntryTextStream(eListBox<eListBoxEntryTextStream>* lb)
		:eListBoxEntry((eListBox<eListBoxEntry>*)lb)
	{		
	
	}

	bool operator < ( const eListBoxEntryTextStream& e) const
	{
		return text.str() < e.text.str();	
	}

protected:
	void redraw(gPainter *rc, const eRect& rect, const gColor& coActive, const gColor& coNormal, bool highlited) const
	{
			rc->setForegroundColor(highlited?coActive:coNormal);
			rc->setFont(listbox->getEntryFnt());

			if ((coNormal != -1 && !highlited) || (highlited && coActive != -1))
					rc->fill(rect);

			rc->renderText(rect, text.str());

			eWidget* p = listbox->getParent();			
			if (highlited && p && p->LCDElement)
				p->LCDElement->setText(text.str());
	}
};

class eListBoxEntryMenu: public eListBoxEntryText
{
	friend class eListBox<eListBoxEntryMenu>;
public:
	Signal0<void> selected;

	eListBoxEntryMenu(eListBox<eListBoxEntryMenu>* lb, const char* txt)
		:eListBoxEntryText((eListBox<eListBoxEntryText>*)lb, txt)
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

////////////////////////////////////// inline Methoden eListBox //////////////////////////////////////////
template <class T>
inline void eListBox<T>::append(T* entry)
{
	childs.push_back(entry);
	
	init();
}

template <class T>
inline void eListBox<T>::remove(T* entry)
{
	childs.take(entry);

	init();
}

template <class T>
inline void eListBox<T>::clearList()
{
	while (!childs.empty())
		delete childs.first();
}

template <class T>
inline void eListBox<T>::sort()
{
	childs.sort();
	init();
}

template <class T>
inline eRect eListBox<T>::getEntryRect(int pos)
{
	return eRect(ePoint(0, pos*item_height), eSize(size.width(), item_height));
}

template <class T>
inline void eListBox<T>::invalidateEntry(int n)
{
	invalidate(getEntryRect(n));
}

template <class T>
inline int eListBox<T>::setProperty(const eString &prop, const eString &value)
{
	if (prop=="col_active")
		col_active=eSkin::getActive()->queryScheme(value);
	else
		return eWidget::setProperty(prop, value);
	return 0;
}

template <class T>
inline void eListBox<T>::setActiveColor(gColor active)
{
	col_active=active;

	if (current != childs.end())
	{
		ePtrList_T_iterator it(top);

		for (int i = 0; i < entries; i++, it++)
		{
			if (it == current)
			{
				invalidateEntry(i);
				break;
			} else if (it == childs.end())
				break;
		}
	}
}

template <class T>
inline T* eListBox<T>::goNext()
{
	moveSelection(dirDown);
	return current!=childs.end() ? *current : 0;
}

template <class T>
inline T* eListBox<T>::goPrev()
{
	moveSelection(dirUp);
	return current!=childs.end() ? *current : 0;
}

template <class T>
inline eListBox<T>::eListBox(eWidget *parent, int ih)
	 :eWidget(parent, 1),
		top(childs.end()), bottom(childs.end()), current(childs.end()),
		font_size(ih),
		item_height(ih+2),
		flags(0),
		col_active(eSkin::getActive()->queryScheme("focusedColor")),
		entryFnt(gFont("NimbusSansL-Regular Sans L Regular", font_size)),
		have_focus(0)
{
	childs.setAutoDelete(false);	// machen wir selber

	addActionMap(&i_cursorActions->map);
	addActionMap(&i_listActions->map);
}

template <class T>
inline eListBox<T>::~eListBox()
{
	while (childs.begin() != childs.end())
	{
		T* l=childs.front();
		delete l;
	}
}

template <class T>
inline void eListBox<T>::redrawWidget(gPainter *target, const eRect &where)
{
	int i=0;
	for (ePtrList_T_iterator entry(top); (entry != bottom) && (entry != childs.end()); ++entry)
	{
		eRect rect = getEntryRect(i);

		if ( where.contains(rect) )
			entry->redraw(target, rect, col_active, getBackgroundColor(), have_focus && (entry == current));
		
		i++;
	}

	target->flush();
}

template <class T>
inline void eListBox<T>::gotFocus()
{
	have_focus++;

	if (childs.empty())
		return;

	ePtrList_T_iterator entry(top);
	
	for (int i=0; (i<entries) && (entry != childs.end()); i++, ++entry)
		if (*entry == *current)
			invalidateEntry(i);
}

template <class T>
inline void eListBox<T>::lostFocus()
{	
	have_focus--;

	if (childs.empty())
		return;

	ePtrList_T_iterator entry(top);

	if (isVisible())
	{
		for (int i=0; (i<entries) && (entry != childs.end()); i++, ++entry)
			if (*entry == *current)
				invalidateEntry(i);
	}

	if (parent && parent->LCDElement)
		parent->LCDElement->setText("");
}

template <class T>
inline void eListBox<T>::init()
{
	entries = size.height() / item_height;

	current = top = bottom = childs.begin();

	for (int i=0; i < entries; i++, bottom++)
	{
		if (bottom == childs.end() )
			break;	
	}
}

template <class T>
inline int eListBox<T>::moveSelection(int dir)
{
	if (childs.empty())
		return 0;
		
	T *oldptr = *current,
		*oldtop = *top;
	switch (dir)
	{
		case dirPageDown:
			if (bottom == childs.end())
			{
				current = bottom;		// --bottom always valid because !childs.empty()
				--current;
			} else
				for (int i = 0; i < entries; i++)
				{
					if (bottom == childs.end())
						break;
					bottom++;
					top++;
					current++;
				}
		break;

		case dirPageUp:
			if (top == childs.begin())
				current = top;
			else
				for (int i = 0; i < entries; i++)
				{	
					if (top == childs.begin())
						break;
					bottom--;
					top--;
					current--;
				}
		break;
		
		case dirUp:
			if ( current == childs.begin() )				// wrap around?
			{
				top = current = --childs.end();					// select last
				bottom = childs.end();
				for (int i = 1; i < entries; i++, top--)
					if (top == childs.begin())
						break;
			}
			else
				if (current-- == top) // new top must set
				{
					for (int i = 0;i < entries; i++, top--, bottom--)
						if (top == childs.begin())
							break;
				}
		break;

		case dirDown:
			if ( current == --childs.end() )				// wrap around?
			{
				top = current = bottom = childs.begin(); 	// goto first;
				for (int i = 0; i < entries; i++, bottom++)
					if ( bottom == childs.end() )
						break;
			}
			else
				if (++current == bottom)
				{
					for (int i=0; i<entries; i++, top++, bottom++)
						if ( bottom == childs.end() )
							break;
				}
		break;
		default:
			return 0;
	}

	if (isVisible())
	{
		if (*current != oldptr)  // current has changed
			/*emit*/ selchanged(*current);

		if (oldtop != *top)
		{
			invalidate();
		}
		else if ( *current != oldptr)
		{
			int i=0;
			int old=-1, cur=-1;
			
			for (ePtrList_T_iterator entry(top); i<entries; i++, ++entry)
				if ( entry == childs.end())
					break;
				else if ( *entry == oldptr)
					old=i;
				else if ( *entry == *current )
					cur=i;
			
			if (old != -1)
				invalidateEntry(old);

			if ( (cur != -1) )
				invalidateEntry(cur);
		}
	}
	return 1;
}

template <class T>
inline void eListBox<T>::setFlags(int _flags)
{
	flags=_flags;
}

template <class T>
inline int eListBox<T>::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
		case eWidgetEvent::evtAction:
			if ((event.action == &i_listActions->pageup) && !(flags & flagNoPageMovement))
				moveSelection(dirPageUp);
			else if ((event.action == &i_listActions->pagedown) && !(flags & flagNoPageMovement))
				moveSelection(dirPageDown);
			else if ((event.action == &i_cursorActions->up) && !(flags & flagNoUpDownMovement))
				moveSelection(dirUp);
			else if ((event.action == &i_cursorActions->down) && !(flags & flagNoUpDownMovement))
				moveSelection(dirDown);
			else if (event.action == &i_cursorActions->ok)
			{
				if ( current == childs.end() )
					/*emit*/ selected(0);
				else
					/*emit*/ selected(*current);
			}
//			else if (event.action == &i_cursorActions->cancel)
//			{
//				/*emit*/ selected(0);
//			}
			else
				break;
		return 1;
	case eWidgetEvent::changedSize:
		init();
		break;
	default:
		break;
	}
	return eWidget::eventHandler(event);
}

template <class T>
inline void eListBox<T>::setCurrent(T *c)
{
	if (childs.empty())
		return;

	for (current = childs.begin(); current != childs.end() ; current++)
		if ( *current == c )
			break;

	if ( current == childs.end() )
		current = childs.begin();

	if (current != childs.end() )
	{
		ePtrList_T_iterator it(top);

		int i = 0;
		for (; i<entries; ++i, ++it)
			if (it == current)
				break;
			else if (it == childs.end())
				break;
		if ((i == entries) || (it == childs.end()))
		{
			top=bottom=current;
			for (int i=0; i<entries; i++, bottom++)
				if (bottom == childs.end() )
					break;
		}
	}
}

template <class T>
class eListBoxWindow: public eWindow
{
protected:
	int Entrys;
	int width;
public:
	eListBox<T> list;
	eListBoxWindow(eString Title="", int Entrys=0, int FontSize=0, int width=400);
};

template <class T>
inline eListBoxWindow<T>::eListBoxWindow(eString Title, int Entrys, int FontSize, int width)
	: eWindow(0), Entrys(Entrys), width(width), list(this, FontSize)
{
	setText(Title);
	cresize(eSize(width, 10+Entrys*(FontSize+4)));
	
	list.move(ePoint(10, 5));
	eSize size = getClientSize();
	size.setWidth(size.width()-20);
	size.setHeight(size.height()-10);
	list.resize(size);
}

#endif
