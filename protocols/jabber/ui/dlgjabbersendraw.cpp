
/***************************************************************************
                      dlgjabbersendraw.cpp  -  Raw XML dialog
                             -------------------
    begin                : Sun Aug 25 2002
    copyright            : (C) 2002 by Till Gerken,
                           Kopete Development team
    email                : till@tantalo.net
		***************************************************************************

		***************************************************************************
		*                                                                         *
		*   This program is free software; you can redistribute it and/or modify  *
		*   it under the terms of the GNU General Public License as published by  *
		*   the Free Software Foundation; either version 2 of the License, or     *
		*   (at your option) any later version.                                   *
		*                                                                         *
		***************************************************************************/

#include <qcombobox.h>
#include <qpushbutton.h>
#include <qtextedit.h>
#include <kdebug.h>
#include "dlgjabbersendraw.h"


dlgJabberSendRaw::dlgJabberSendRaw (Jabber::Client * engine, QWidget * parent, const char *name)
	: DlgSendRaw (parent, name), mEngine(engine)
{
	// Connect the GUI elements to things that do stuff
	connect (btnSend, SIGNAL (clicked ()), this, SLOT (slotSend ()));
	connect (btnClose, SIGNAL (clicked ()), this, SLOT (slotCancel ()));
	connect (btnClear, SIGNAL (clicked ()), this, SLOT (slotClear ()));
	connect (inputWidget, SIGNAL (activated (int)), this, SLOT (slotCreateMessage (int)));

	show();
}

dlgJabberSendRaw::~dlgJabberSendRaw ()
{
	// Nothing yet
}

void dlgJabberSendRaw::slotCancel ()
{
	close(true);
}

void dlgJabberSendRaw::slotClear ()
{
	inputWidget->setCurrentItem(0);
	tePacket->clear();
}

void dlgJabberSendRaw::slotCreateMessage(int index)
{
	switch (index) {
		case 2:
			tePacket->setText("<presence>\n<show>\?\?\?</show>\n<status>\?\?\?</status>\n</presence>");
			break;
		case 3:
			tePacket->setText("<iq type='get' to='USER@DOMAIN'>\n<query xmlns='jabber:iq:last'/></iq>");
			break;
		case 4:
			tePacket->setText(QString("<message to='USER@DOMAIN' from='%1@%2/%3'>\n<body>Body text</body>\n</message>")
						.arg(mEngine->user())
						.arg(mEngine->host())
						.arg(mEngine->resource()));
			break;
		default:
			tePacket->clear();
			break;
	}
}

void dlgJabberSendRaw::slotSend()
{
	kdDebug (14130) << "[dlgJabberSendRaw] Sending RAW message" << endl;

	// Tell our engine to send
	mEngine->send (tePacket->text ());

	// set temlapte combobox to "User Defined" and clear content
	inputWidget->setCurrentItem(0);
	tePacket->clear();
}

#include "dlgjabbersendraw.moc"

/*
 * Local variables:
 * mode: c++
 * c-indentation-style: k&r
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
// vim: set noet ts=4 sts=4 sw=4:
