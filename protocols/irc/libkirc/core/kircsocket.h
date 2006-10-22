/*
    kircsocket.h - IRC socket.

    Copyright     2006      by Tommi Rantala <tommi.rantala@cs.helsinki.fi>
    Copyright (c) 2003-2006 by Michel Hermier <michel.hermier@wanadoo.fr>

    Kopete    (c) 2002-2006 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef KIRCSOCKET_H
#define KIRCSOCKET_H

#include "kircconst.h"
#include "kircentity.h"
#include "kircevent.h"
#include "kircmessage.h"

#include <QTcpSocket>

class KUrl;

class QTextCodec;

namespace KIrc
{

class CommandHandler;
class EntityManager;
class Event;

/**
 * @author Michel Hermier <michel.hermier@wanadoo.fr>
 */
class Socket
	: public QObject
{
	Q_OBJECT

	Q_PROPERTY(ConnectionState connectionState READ connectionState)
	Q_ENUMS(ConnectionState)

public:
	enum ConnectionState
	{
		Idle,
		HostLookup,
		HostFound,
//		Bound,
		Connecting,
		Authentifying,
		Open,
		Closing
	};

	Socket(QObject *parent = 0);
	~Socket();

public: // READ properties accessors.
	ConnectionState connectionState() const;

public:
	QTcpSocket *socket();

	QTextCodec *defaultCodec() const;

	CommandHandler *commandHandler() const;
	EntityManager *entityManager() const;
	Entity::Ptr owner() const;

	/**
	 * The connection url.
	 */
	const KUrl &url() const;

public slots:
	void setDefaultCodec(QTextCodec *codec);

	void setCommandHandler(CommandHandler *newCommandHandler);
	void setEntityManager(EntityManager *newEntityManager);
	void setOwner(const Entity::Ptr &newOwner);

	void connectToServer(const KUrl &url);
	void close();

	void writeMessage(const QByteArray &message);
	void writeMessage(const QString &message, QTextCodec *codec = 0);
	void writeMessage(const Message &message);

	void showInfoDialog();

#if 0
public: //moc error
	/**
	 * Post an event for the given socket.
	 *
	 * @param messageType the type of event message.
	 * @param message the event message content.
	 */
	void postEvent(Event::MessageType messageType, const QString &message);

	/**
	 * Post an error event for the given socket.
	 *
	 * @param errStr the string describing the error.
	 *
	 * @note The error event is only informational and won't change the status.
	 */
	inline void postErrorEvent(const QString &errStr)
	{ postEvent(Event::ErrorMessage, errStr); }
#endif

signals:
//	void eventOccured(const Event *);

	void connectionStateChanged(Socket::ConnectionState newstate);

	void receivedMessage(const Message &message);

protected:
	void setConnectionState(Socket::ConnectionState newstate);
	virtual void authentify();

private slots:
	void onReadyRead();

	void socketStateChanged(QAbstractSocket::SocketState);
	void socketGotError(QAbstractSocket::SocketError);

private:
	QByteArray encode(const QString &str, bool *success, QTextCodec *codec = 0) const;

	Q_DISABLE_COPY(Socket)

	class Private;
	Private * const d;
};

}

#endif

