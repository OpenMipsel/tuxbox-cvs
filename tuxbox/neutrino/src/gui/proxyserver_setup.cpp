/*
	$Id: proxyserver_setup.cpp,v 1.6 2011/12/09 22:36:28 dbt Exp $

	proxyserver_setup menue - Neutrino-GUI

	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2010 T. Graf 'dbt'
	Homepage: http://www.dbox2-tuning.net/

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

#include "gui/proxyserver_setup.h"

#include <global.h>
#include <neutrino.h>

#include <gui/widget/stringinput.h>

#include <driver/screen_max.h>
#include <system/debug.h>

CProxySetup::CProxySetup(const neutrino_locale_t title, const char * const IconName )
{
	menue_title = title != NONEXISTANT_LOCALE ? title : LOCALE_FLASHUPDATE_PROXYSERVER_SEP;
	menue_icon = IconName != NEUTRINO_ICON_SETTINGS ? IconName : NEUTRINO_ICON_SETTINGS;

	width = w_max (500, 100);
	selected = -1;
}

CProxySetup::~CProxySetup()
{
}

int CProxySetup::exec(CMenuTarget* parent, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_DEBUG, "init proxy menu\n");
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

	res = showProxySetup();

	return res;
}

/* shows entries for proxy settings */
int CProxySetup::showProxySetup()
{
	//init
	CMenuWidget * mn = new CMenuWidget(menue_title, menue_icon, width);
	mn->setPreselected(selected);

	if (menue_title != NONEXISTANT_LOCALE)
	{
		CMenuSeparator * proxy_subhead = new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_FLASHUPDATE_PROXYSERVER_SEP);
		mn->addItem(proxy_subhead);
	}

	mn->addItem(GenericMenuSeparator);
	mn->addItem(GenericMenuBack);
	mn->addItem(GenericMenuSeparatorLine);

	CStringInputSMS softUpdate_proxy(LOCALE_FLASHUPDATE_PROXYSERVER, g_settings.softupdate_proxyserver, 23, LOCALE_FLASHUPDATE_PROXYSERVER_HINT1, LOCALE_FLASHUPDATE_PROXYSERVER_HINT2, "abcdefghijklmnopqrstuvwxyz0123456789-.: ");
	mn->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_PROXYSERVER, true, g_settings.softupdate_proxyserver, &softUpdate_proxy, NULL, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));

	CStringInputSMS softUpdate_proxyuser(LOCALE_FLASHUPDATE_PROXYUSERNAME, g_settings.softupdate_proxyusername, 23, LOCALE_FLASHUPDATE_PROXYUSERNAME_HINT1, LOCALE_FLASHUPDATE_PROXYUSERNAME_HINT2, "abcdefghijklmnopqrstuvwxyz0123456789!""§$%&/()=?-. ");
	mn->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_PROXYUSERNAME, true, g_settings.softupdate_proxyusername, &softUpdate_proxyuser, NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));

	CStringInputSMS softUpdate_proxypass(LOCALE_FLASHUPDATE_PROXYPASSWORD, g_settings.softupdate_proxypassword, 20, LOCALE_FLASHUPDATE_PROXYPASSWORD_HINT1, LOCALE_FLASHUPDATE_PROXYPASSWORD_HINT2, "abcdefghijklmnopqrstuvwxyz0123456789!""§$%&/()=?-. ");
	mn->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_PROXYPASSWORD, true, g_settings.softupdate_proxypassword, &softUpdate_proxypass, NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));

	int res = mn->exec(NULL, "");
	mn->hide();
	selected = mn->getSelected();
	delete mn;

	return res;
}

