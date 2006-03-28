/*
   client.h - Papillon Client to Windows Live Messenger.

   Copyright (c) 2006 by Michaël Larouche <michael.larouche@kdemail.net>

   *************************************************************************
   *                                                                       *
   * This library is free software; you can redistribute it and/or         *
   * modify it under the terms of the GNU Lesser General Public            *
   * License as published by the Free Software Foundation; either          *
   * version 2 of the License, or (at your option) any later version.      *
   *                                                                       *
   *************************************************************************
*/
#ifndef PAPILLONCLIENT_H
#define PAPILLONCLIENT_H

#include <QtCore/QObject>
#include <papillon_macros.h>

namespace Papillon
{

class Connector;
class Connection;
class SecureStream;
class Task;

/**
 * @brief Client to Windows Live Messenger.
 *
 * @author Michaël Larouche <michael.larouche@kdemail.net>
 */
class PAPILLON_EXPORT Client : public QObject
{
	Q_OBJECT
public:
	/**
	 * Create a new Client.
	 * @param connector Connector to use.
	 * @param parent QObject parent.
	 */
	Client(Connector *connector, QObject *parent = 0);
	/**
	 * d-tor.
	 */
	~Client();

	/**
	 * @brief Create a new SecureStream ready to be used.
	 * You must delete the SecureStream instance yourself.
	 *
	 * @return the new SecureStream instance.
	 */
	SecureStream *createSecureStream();

	/**
	 * @brief Create and a setup a new connection
	 * @return the new Connection instance.
	 */
	Connection *createConnection();

	/**
	 * @brief Set client Info
	 * @param passportId Client Passport ID
	 * @param password Client password.
	 * TODO: Use a ClientInfo data class.
	 * TODO: Use QSecureArray for password.
	 */
	void setClientInfo(const QString &passportId, const QString &password);

signals:
	void connected();
	void disconnected();

public slots:
	void connectToServer(const QString &server = QString(), quint16 port = 0);
	void login();

private slots:
	/**
	 * Notification Connection is connected.
	 * Do the login
	 */
	void notifyConnected();
	/**
	 * Result of Login process.
	 * @param task the LoginTask.
	 */
	void loginResult(Papillon::Task *task);

private:
	class Private;
	Private *d;
};

}

#endif
