#include <lib/gui/listbox.h>
#include <lib/gdi/font.h>

gFont eListBoxEntryText::font;
gFont eListBoxEntryTextStream::font;

eListBoxBase::eListBoxBase(eWidget* parent, const eWidget* descr, int takefocus, int item_height, const char *deco )
:   eDecoWidget(parent, takefocus, deco),
		top(childs.end()), bottom(childs.end()), current(childs.end()), recalced(0),
		descr(descr),
		tmpDescr(0),
		colorActiveB(eSkin::getActive()->queryScheme("global.selected.background")),
		colorActiveF(eSkin::getActive()->queryScheme("global.selected.foreground")),
		movemode(0),
		MaxEntries(0),
		flags(0),
		item_height(item_height),
		columns(1),
		in_atomic(0)
{
	childs.setAutoDelete(false);	// machen wir selber
	addActionMap(&i_cursorActions->map);
	addActionMap(&i_listActions->map);
}

eListBoxBase::~eListBoxBase()
{
	while (childs.begin() != childs.end())
		delete childs.front();
}

void eListBoxBase::setFlags(int _flags)	
{
	flags |= _flags;	
}

void eListBoxBase::removeFlags(int _flags)	
{
	flags &= ~_flags;	
}

void eListBoxBase::recalcMaxEntries()
{
		// MaxEntries is PER COLUMN
	if (deco_selected && have_focus)
		MaxEntries = crect_selected.height();
	else if (deco)
		MaxEntries = crect.height();
	else
		MaxEntries = height();
	MaxEntries /= item_height;
}

eRect eListBoxBase::getEntryRect(int pos)
{
	int lme=MaxEntries;
			// in case we show partial last lines (which only works in single-column),
			// we increase MaxEntries by one since we don't want the last line
			// one the next (invisible) column
	if ( (columns == 1) && (flags & flagShowPartial))
		lme++;
	if ( deco_selected && have_focus )
		return eRect( ePoint( deco_selected.borderLeft + ( ( pos / lme) * ( crect_selected.width() / columns ) ) , deco_selected.borderTop + ( pos % lme) * item_height ), eSize( crect_selected.width() / columns , item_height ) );
	else if (deco)
		return eRect( ePoint( deco.borderLeft + ( ( pos / lme ) * ( crect.width() / columns ) ) , deco.borderTop + ( pos % lme) * item_height ), eSize( crect.width() / columns , item_height ) );
	else if ( deco_selected )
		return eRect( ePoint( deco_selected.borderLeft + ( ( pos / lme) * ( crect_selected.width() / columns ) ) , deco_selected.borderTop + ( pos % lme) * item_height ), eSize( crect_selected.width() / columns , item_height ) );
	else
		return eRect( ePoint( ( ( pos / lme ) * ( size.width() / columns ) ) , ( pos % lme) * item_height ), eSize( size.width() / columns , item_height ) );
}

void eListBoxBase::setColumns(int col)
{
	if (col)
		columns=col;
}

int eListBoxBase::setProperty(const eString &prop, const eString &value)
{
	if (prop == "noPageMovement")
	{
		if (value == "off")
			flags |= ~flagNoPageMovement;
		else
			flags |= flagNoPageMovement;
	}
	else if (prop == "noUpDownMovement")
	{
		if (value == "off")
			flags |= ~flagNoUpDownMovement;
		else
			flags |= flagNoUpDownMovement;
	}
	else if (prop=="activeForegroundColor")
		colorActiveF=eSkin::getActive()->queryScheme(value);
	else if (prop=="activeBackgroundColor")
		colorActiveB=eSkin::getActive()->queryScheme(value);
	else if (prop=="showEntryHelp")
		setFlags( flagShowEntryHelp );
	else if (prop=="columns")
		setColumns( value?atoi(value.c_str()):1 );
	else
		return eDecoWidget::setProperty(prop, value);

	return 0;
}

void eListBoxBase::recalcClientRect()
{
	if (deco)
	{
		crect.setLeft( deco.borderLeft );
		crect.setTop( deco.borderTop );
		crect.setRight( width() - deco.borderRight );
		crect.setBottom( height()  - deco.borderBottom );
	}
	if (deco_selected)
	{
		crect_selected.setLeft( deco_selected.borderLeft );
		crect_selected.setTop( deco_selected.borderTop );
		crect_selected.setRight( width() - deco_selected.borderRight );
		crect_selected.setBottom( height() - deco_selected.borderBottom );
	}
}

int eListBoxBase::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
		case eWidgetEvent::changedSize:
			recalcClientRect();
			recalcMaxEntries();
			init();
		break;
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
					/*emit*/ SendSelected(0);
				else
					/*emit*/ SendSelected(*current);
			}
			else if (event.action == &i_cursorActions->cancel)
				/*emit*/ SendSelected(0);
			else
				break;
		return 1;
		default:
		break;
	}
	return eWidget::eventHandler(event);
}

void eListBoxBase::invalidateContent()
{
	if ( have_focus && deco_selected )
		invalidate( crect_selected );
	else if ( deco )
		invalidate( crect );
	else
		invalidate();
}

int eListBoxBase::newFocus()
{
	if (deco && deco_selected)
	{
		recalcMaxEntries();

		if (isVisible())
			invalidate();

		return 1;
	}
	return 0;
}

int eListBoxBase::setCurrent(const eListBoxEntry *c, bool sendSelected )
{
/*	if (childs.empty() || ((current != childs.end()) && (*current == c)))  // no entries or current is equal the entry to search
		return E_ALLREADY_SELECTED;	// do nothing*/

	if ( childs.empty() )
		return E_COULDNT_FIND;

	ePtrList<eListBoxEntry>::iterator item(childs.begin()), it(childs.begin());

	for ( ; item != childs.end(); item++)
		if ( *item == c )
			break;

	if ( item == childs.end() ) // entry not in listbox... do nothing
		return E_COULDNT_FIND;

	int newCurPos=-1;
	int oldCurPos=-1;
	ePtrList<eListBoxEntry>::iterator oldCur(current);

	int i = 0;

	for (it=top; it != bottom; ++it, ++i)  // check if entry to set between bottom and top
	{
		if (it == item)
		{
			newCurPos=i;
			current = it;
		}
		if ( it == oldCur)
			oldCurPos=i;
	}

	if (newCurPos != -1)	// found on current screen, so redraw only old and new
	{
		if (isVisible())
		{
			if (in_atomic)
			{
				if (atomic_redraw == arNothing)
				{
					atomic_redraw = arCurrentOld;
					atomic_old = oldCurPos;
				}
				if (atomic_redraw == arCurrentOld)
					atomic_new = newCurPos;
			} else
			{
				invalidateEntry(newCurPos);
				if (oldCurPos != -1)
					invalidateEntry(oldCurPos);
			}
		}
	}	else // the we start to search from begin
	{
		bottom = childs.begin();

		while (newCurPos == -1 && MaxEntries )  // MaxEntries is already checked above...
		{
			if ( bottom != childs.end() )
				top = bottom;		// n�chster Durchlauf

			for (	i = 0; (i < (MaxEntries*columns) ) && (bottom != childs.end()); ++bottom, ++i)
			{
				if (bottom == item)
				{
					current = bottom;  // we have found
					newCurPos++;
				}
      }
		}
		if (isVisible())
		{
			if (!in_atomic)
				invalidateContent();   // Draw all
			else
				atomic_redraw=arAll;
		}
  }

	if (!in_atomic)
	{
		/*emit*/ SendSelChanged(*current);
		if ( sendSelected )
			/*emit*/ SendSelected(*current);
	}
	else
	{
		atomic_selchanged=1;
		if ( sendSelected )
			atomic_selected=1;
	}

	return OK;
}

void eListBoxBase::append(eListBoxEntry* entry, bool holdCurrent, bool front)
{
	eListBoxEntry* cur = 0;
	if (holdCurrent)
		cur = current;

	if ( front )
		childs.push_front(entry);
	else
		childs.push_back(entry);
	init();

	if (cur)
		setCurrent(cur);
}

void eListBoxBase::remove(eListBoxEntry* entry, bool holdCurrent)
{
	eListBoxEntry* cur = 0;

	if (holdCurrent && current != entry)
		cur = current;

	childs.take(entry);
	init();

	if (cur)
		setCurrent(cur);
}

void eListBoxBase::clearList()
{
	while (!childs.empty())
		delete childs.first();
	current = top = bottom = childs.end();
	if (!in_atomic)
	{
		/*emit*/ SendSelChanged(0);
		invalidateContent();
	} else
	{
		atomic_selected=0;
		atomic_selchanged=0;
		atomic_redraw=arAll;
		atomic_new=0;
		atomic_old=0;
	}
}

eListBoxEntry *eListBoxBase::goNext()
{
	moveSelection(dirDown);
	return current!=childs.end() ? *current : 0;
}

eListBoxEntry* eListBoxBase::goPrev()
{
	moveSelection(dirUp);
	return current!=childs.end() ? *current : 0;
}

void eListBoxBase::redrawWidget(gPainter *target, const eRect &where)
{
	eRect rc;

	if (deco_selected && have_focus)
	{
		deco_selected.drawDecoration(target, ePoint(width(), height()));
		rc = crect_selected;
	}
	else if (deco)
	{
		deco.drawDecoration(target, ePoint(width(), height()));
		rc = crect;
	}
	else
		rc = where;

	int i=0;
	for (ePtrList<eListBoxEntry>::iterator entry(top); ((flags & flagShowPartial) || (entry != bottom)) && (entry != childs.end()); ++entry)
	{
		eRect rect = getEntryRect(i);

		eString s;

		if ( rc.intersects(rect) )
		{
			target->clip(rect & rc);
			if ( entry == current )
			{
				if ( LCDTmp ) // LCDTmp is only valid, when we have the focus
					LCDTmp->setText( entry->redraw(target, rect, colorActiveB, colorActiveF, getBackgroundColor(), getForegroundColor(), 1 ) );
				else if ( parent->LCDElement && have_focus )
					parent->LCDElement->setText( entry->redraw(target, rect, colorActiveB, colorActiveF, getBackgroundColor(), getForegroundColor(), 1 ) );
				else
					entry->redraw(target, rect, colorActiveB, colorActiveF, getBackgroundColor(), getForegroundColor(), ( have_focus ? 1 : ( MaxEntries > 1 ? 2 : 0 ) )	);
			}
			else
				entry->redraw(target, rect, colorActiveB, colorActiveF, getBackgroundColor(), getForegroundColor(), 0 /*( have_focus ? 0 : ( MaxEntries > 1 ? 2 : 0 ) )*/	);
			target->clippop();
		}
				// special case for "showPartial": as bottom is set to the
				// last, half visible entry we want to redraw this, too.
		if (flags & flagShowPartial)
			if (entry == bottom)
				break;

		i++;
	}
}

void eListBoxBase::gotFocus()
{
	if (parent && parent->LCDElement)  // detect if LCD Avail
		if (descr)
		{
			parent->LCDElement->setText("");
			LCDTmp = new eLabel(parent->LCDElement);
			LCDTmp->hide();
			eSize s = parent->LCDElement->getSize();
			LCDTmp->move(ePoint(0,s.height()/2));
			LCDTmp->resize(eSize(s.width(), s.height()/2));
			LCDTmp->show();
			tmpDescr = new eLabel(parent->LCDElement);
			tmpDescr->hide();
			tmpDescr->move(ePoint(0,0));
			tmpDescr->resize(eSize(s.width(), s.height()/2));
			tmpDescr->setText( descr->getText() );
			tmpDescr->show();
		}

	have_focus++;

	if (!childs.empty())
	{
		if ( newFocus() )   // recalced ?
		{
			ePtrList<eListBoxEntry>::iterator it = current;
			init();
			setCurrent(it);
		}
		else if ( isVisible() )
		{
			int i=0;
			for (ePtrList<eListBoxEntry>::iterator entry(top); entry != bottom; ++i, ++entry)
				if (entry == current)
					invalidateEntry(i);
		}
	}
	if (flags & flagShowEntryHelp)
	{
		setHelpText( current != childs.end() ? current->getHelpText():eString(_("no description available")));
	}
}

void eListBoxBase::lostFocus()
{
	if ( descr )
	{
		delete LCDTmp;
		LCDTmp=0;
		delete tmpDescr;
		tmpDescr=0;
	}
	have_focus--;

	if (!childs.empty())
		if ( newFocus() ) //recalced ?
		{
			ePtrList<eListBoxEntry>::iterator it = current;
			init();
			setCurrent(it);
		}
		else if ( isVisible() )
		{
			int i = 0;
			for (ePtrList<eListBoxEntry>::iterator entry(top); entry != bottom; i++, ++entry)
				if (entry == current)
					invalidateEntry(i);
		}

	if (parent && parent->LCDElement)
		parent->LCDElement->setText("");
}

void eListBoxBase::init()
{
	current = top = bottom = childs.begin();
	for (int i = 0; i < (MaxEntries*columns); ++i, ++bottom)
		if (bottom == childs.end() )
			break;
	if (!in_atomic)
	{
		invalidateContent();
	}
	else
	{
		atomic_redraw=arAll;
	}
}

int eListBoxBase::moveSelection(int dir, bool sendSelected)
{
	int direction=0, forceredraw=0;

	if (childs.empty())
		return 0;

	ePtrList<eListBoxEntry>::iterator oldptr=current, oldtop=top;

	switch (dir)
	{
		case dirPageDown:
			direction=+1;
			for (int i = 0; i < MaxEntries; i++)
			{
				if (++current == bottom) // unten (rechts) angekommen? page down
				{
					if (bottom == childs.end()) // einzige ausnahme: unten (rechts) angekommen
					{
						--current;
						break;
					}
					for (int i = 0; i < MaxEntries * columns; ++i)
					{
						if (bottom != childs.end())
						{
							++bottom;
							++top;
						}
					}
				}
			}
		break;

		case dirPageUp:
			direction=-1;
			for (int i = 0; i < MaxEntries; ++i)
			{
				if (current == childs.begin())
					break;

				if (current-- == top/* && current != childs.begin()*/ )	// oben (links) angekommen? page up
				{
					for (int i = 0; i < MaxEntries * columns; ++i)
					{
						if (--top == childs.begin()) 		// einzige ausnahme: oben (links) angekommen
							break;
					}

					// und einmal bottom neuberechnen :)
					bottom=top;
					for (int i = 0; i < MaxEntries*columns; ++i)
						if (bottom != childs.end())
							++bottom;
				}
			}
			break;

		case dirUp:
			if ( current == childs.begin() )				// wrap around?
			{
				direction=+1;
				current = childs.end();					// select last
				--current;
				top = bottom = childs.end();
				for (int i = 0; i < MaxEntries*columns; i++, top--)
					if (top == childs.begin())
						break;
			} else
			{
				direction=-1;
				if (current-- == top) // new top must set
				{
					for (int i = 0; i < MaxEntries*columns; i++, top--)
						if (top == childs.begin())
							break;
					bottom=top;
					for (int i = 0; i < MaxEntries*columns; ++i, ++bottom)
						if (bottom == childs.end())
							break;
				}
			}
		break;

		case dirDown:
			if ( current == --ePtrList<eListBoxEntry>::iterator(childs.end()) )				// wrap around?
			{
				direction=-1;
				top = current = bottom = childs.begin(); 	// goto first
				for (int i = 0; i < MaxEntries * columns; ++i, ++bottom)
					if ( bottom == childs.end() )
						break;
			}
			else
			{
				direction=+1;
				if (++current == bottom)   // ++current ??
				{
					for (int i = 0; i<MaxEntries * columns; ++i)
					{
						if (bottom != childs.end() )
							++bottom;
						if (top != childs.end() )
							++top;
					}
				}
			}
			break;
		case dirFirst:
			direction=-1;
			top = current = bottom = childs.begin(); 	// goto first;
			for (int i = 0; i < MaxEntries * columns; i++, bottom++)
				if ( bottom == childs.end() )
					break;
			break;
		case dirLast:
			direction=1;
			top=bottom=current=childs.end();
			if (current == childs.begin())
				break;	// empty.

			for (int i = 0; i < MaxEntries * columns; ++i)
				if (top != childs.begin())
					--top;
			--current;
			break;
		default:
			return 0;
	}

	if (current != oldptr)  // current has changed
	{
		if (movemode)
		{
				// feel free to rewrite using stl::copy[_backward], but i didn't succeed.
			std::list<eListBoxEntry*>::iterator o=oldptr,
																				c=current,
																				curi=current,
																				oldi=oldptr;
			int count=0;

			eListBoxEntry* old=*o;

			if (direction > 0)
			{
				++o;
				++c;
				while (o != c)
				{
					*oldi++=*o++;
					count++;
				}
			} else
			{
				while (o != curi)
				{
					*oldi--=*--o;
					count++;
				}
			}

			if (count > 1)
				forceredraw=1;

			*curi=old;
		}
	}

	if (!in_atomic)
	{
		/*emit*/ SendSelChanged(*current);
		if ( sendSelected )
			/*emit*/ SendSelected(*current);
	}
	else
	{
		atomic_selchanged=1;
		if ( sendSelected )
			atomic_selected=1;
	}

	if (flags & flagShowEntryHelp)
	{
		setHelpText( current != childs.end() ? current->getHelpText():eString(_("no description available")));
	}

	if (isVisible())
	{
		if ((oldtop != top) || forceredraw)
		{
			if (in_atomic)
				atomic_redraw=arAll;
			else
				invalidateContent();
		} else if ( current != oldptr)
		{
			int i=0;
			int old=-1, cur=-1;

			for (ePtrList<eListBoxEntry>::iterator entry(top); entry != bottom; i++, ++entry)
				if ( entry == oldptr)
					old=i;
				else if ( entry == current )
					cur=i;

			if (in_atomic)
			{
				if (atomic_redraw == arNothing)
				{
					atomic_old=old;
					atomic_redraw = arCurrentOld;
				}
				if (atomic_redraw == arCurrentOld)
					atomic_new=cur;
			} else
			{
				if (old != -1)
					invalidateEntry(old);
				if (cur != -1)
					invalidateEntry(cur);
			}
		}
	}
	return 1;
}

void eListBoxBase::setActiveColor(gColor back, gColor front)
{
	colorActiveB=back;
	colorActiveF=front;

	if (current != childs.end())
	{
		int i = 0;
		for (ePtrList<eListBoxEntry>::iterator it(top); it != bottom; i++, it++)
		{
			if (it == current)
			{
				invalidateEntry(i);
				break;
			}
		}
	}
}

void eListBoxBase::beginAtomic()
{
	if (!in_atomic++)
	{
		atomic_redraw=arNothing;
		atomic_selchanged=0;
		atomic_selected=0;
		atomic_new=-1;
	}
}

void eListBoxBase::endAtomic()
{
	if (!--in_atomic)
	{
		if (atomic_redraw == arAll)
			invalidateContent();
		else if (atomic_redraw == arCurrentOld)
		{
			if (atomic_new != -1)
				invalidateEntry(atomic_new);
			if (atomic_old != -1)
				invalidateEntry(atomic_old);
		}
		if (atomic_selchanged)
			if (childs.empty())
				/*emit*/ SendSelChanged(0);
			else
				/*emit*/ SendSelChanged(*current);

		if (atomic_selected)
			if (childs.empty())
				/*emit*/ SendSelected(0);
			else
				/*emit*/ SendSelected(*current);
	}
}

void eListBoxBase::sort()
{
	eListBoxEntry* cur = current;
	childs.sort();

	init();

	if (cur)
		setCurrent(cur);
}

void eListBoxEntry::drawEntryBorder(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF )
{
	rc->setForegroundColor(coActiveB);
	rc->line( ePoint(rect.left(), rect.bottom()-1), ePoint(rect.right()-1, rect.bottom()-1) );
	rc->line( ePoint(rect.left(), rect.top()), ePoint(rect.right()-1, rect.top()) );
	rc->line( ePoint(rect.left(), rect.top()), ePoint(rect.left(), rect.bottom()-1) );
	rc->line( ePoint(rect.right()-1, rect.top()), ePoint(rect.right()-1, rect.bottom()-1) );
	rc->line( ePoint(rect.left()+1, rect.bottom()-2), ePoint(rect.right()-2, rect.bottom()-2) );
	rc->line( ePoint(rect.left()+1, rect.top()+1), ePoint(rect.right()-2, rect.top()+1) );
	rc->line( ePoint(rect.left()+1, rect.top()+2), ePoint(rect.left()+1, rect.bottom()-3) );
	rc->line( ePoint(rect.right()-2, rect.top()+2), ePoint(rect.right()-2, rect.bottom()-3) );
	rc->line( ePoint(rect.left()+2, rect.bottom()-3), ePoint(rect.right()-3, rect.bottom()-3) );
	rc->line( ePoint(rect.left()+2, rect.top()+2), ePoint(rect.right()-3, rect.top()+2) );
	rc->line( ePoint(rect.left()+2, rect.top()+3), ePoint(rect.left()+2, rect.bottom()-4) );
	rc->line( ePoint(rect.right()-3, rect.top()+3), ePoint(rect.right()-3, rect.bottom()-4) );
}

void eListBoxEntry::drawEntryRect(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int state )
{
	if ( (coNormalB != -1 && !state) || (state && coActiveB != -1) )
	{
		rc->setForegroundColor(state?coActiveB:coNormalB);
		rc->fill(rect);
		rc->setBackgroundColor(state?coActiveB:coNormalB);
	}
	else
	{
		eWidget *w=listbox->getNonTransparentBackground();
		rc->setForegroundColor(w->getBackgroundColor());
		rc->fill(rect);
		rc->setBackgroundColor(w->getBackgroundColor());
	}
	rc->setForegroundColor(state?coActiveF:coNormalF);
}

eListBoxEntryText::~eListBoxEntryText()
{
	if (para)
	{
		para->destroy();
		para = 0;
	}
}

int eListBoxEntryText::getEntryHeight()
{
	if ( !font.pointSize)
		font = eSkin::getActive()->queryFont("eListBox.EntryText.normal");

	return calcFontHeight( font ) + 4;
}

int eListBoxEntryTextStream::getEntryHeight()
{
	if ( !font.pointSize)
		font = eSkin::getActive()->queryFont("eListBox.EntryText.normal");

	return calcFontHeight( font ) + 4;
}

int calcFontHeight( const gFont& font)
{
	eTextPara *test;
	test = new eTextPara( eRect(0,0,100,50) );
	test->setFont( font );
	test->renderString("Mjdyl");
	int i =  test->getBoundBox().height();
	test->destroy();
	return i;
}

const eString& eListBoxEntryText::redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int state)
{
	bool b;

	if ( (b = (state == 2)) )
		state = 0;

	drawEntryRect( rc, rect, coActiveB, coActiveF, coNormalB, coNormalF, state );

	if (!para)
	{
		para = new eTextPara( eRect( 0, 0, rect.width(), rect.height() ) );
		para->setFont( font );
		para->renderString(text);
		para->realign(align);
//		yOffs = ((rect.height() - para->getBoundBox().height()) / 2 + 0) - para->getBoundBox().top() ;
		yOffs=0;
	}
	rc->renderPara(*para, ePoint( rect.left(), rect.top()+yOffs ) );

	if (b)
		drawEntryBorder( rc, rect, coActiveB, coActiveF, coNormalB, coNormalF );

	return text;
}

const eString &eListBoxEntryTextStream::redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int state)
{
	rc->setFont( font );

	drawEntryRect( rc, rect, coActiveB, coActiveF, coNormalB, coNormalF, state );

	rc->setForegroundColor(state?coActiveF:coNormalF);
	rc->renderText(rect, text.str());

	static eString ret = text.str();
	return ret;
}
