/***************************************************************************
                          msnprotocol.h  -  description
                             -------------------
    begin                : Wed Jan 2 2002
    copyright            : (C) 2002 by duncan
    email                : duncan@tarro
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef IRCPROTOCOL_H
#define IRCPROTOCOL_H



#include "kopeteprotocol.h"

#include <qpixmap.h>



class AddContactPage;
class IRCServerManager;
class KIRC;
class StatusBarIcon;

class KPopupMenu;
class KSimpleConfig;

class QStringList;
class QWidget;



/**
  *@author duncan
  */

class IRCProtocol : public KopeteProtocol
{
	Q_OBJECT

public:
	IRCProtocol( QObject *parent, const char *name, const QStringList &args );
	~IRCProtocol();
	/* Plugin reimplementation */
	void init();
	bool unload();

	/** KopeteProtocol reimplementation */
	virtual KopeteContact* createContact( KopeteMetaContact *parent, const QString &serializedData );
	virtual QString protocolIcon() const;
	virtual AddContactPage *createAddContactWidget(QWidget *parent);
	virtual void Connect();
	virtual void Disconnect();
	virtual bool isConnected() const;
	virtual void setAway(void);
	virtual void setAvailable(void);
	virtual bool isAway(void) const;

	//following implementation is incorrect
	KopeteContact* myself() const { return 0L; }
	/** Internal */
	StatusBarIcon *statusBarIcon;
	/** The IRC Engine */
	QPixmap protocolSmallIcon;
	QPixmap onlineIcon;
	QPixmap offlineIcon;
	QPixmap awayIcon;
	QPixmap joinIcon;
	QPixmap privmsgIcon;

	void addContact(const QString &, const QString &, const QString &, bool, bool);

	IRCServerManager *serverManager() { return m_serverManager; }
	KSimpleConfig *config() { return m_config; };
	KIRC *engine() { return m_engine; };

private:
	void initIcons();
	KPopupMenu *popup;
	IRCServerManager *m_serverManager;
	KSimpleConfig *m_config;
	KIRC *m_engine;
	bool m_isConnected;

public slots: // Public slots
  /** No descriptions */
private slots:
	void slotIconRightClicked(const QPoint&);
	void slotNewConsole();
};

#endif
/*
 * Local variables:
 * c-indentation-style: k&r
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
// vim: set noet ts=4 sts=4 sw=4:

