/*
    kyahoo.h - Qt based libyahoo2 wrapper II

    Copyright (c) 2002-2003 by Duncan Mac-Vicar Prett <duncan@kde.org>

    Copyright (c) 2002 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef KYAHOO_H
#define KYAHOO_H

// QT Includes
#include <qobject.h>
#include <kextendedsocket.h>
#include <qstring.h>
#include <qmap.h>
#include <qpixmap.h>

#include "libyahoo2/yahoo2.h"
#include "libyahoo2/yahoo2_callbacks.h"

// KDE Includes

class YahooSession;
class KExtendedSocket;
class QSocketNotifier;

class YahooConnectionManager
{
public:
	YahooConnectionManager();
	~YahooConnectionManager();
	
	/**
	 * Registers a connection with the connection manager so that it
	 * can be kept track of. When a connection is closed, it is unregistered
	 * from the connection manager and a new connection of that type is expected
	 * to be created.
	 */
	void addConnection( KExtendedSocket* socket );
	
	/**
	 * Get the connection by its file descriptor
	 */
	KExtendedSocket* connectionForFD( int fd );
	
	/**
	 * Remove a connection from the manager
	 * @overload
	 */
	void remove( KExtendedSocket* socket );
	
	/**
	 * Reset the connection manager.
	 */
	void reset();
	
private:
	
	QValueList<KExtendedSocket*> m_connectionList;
};
	
	

/* Yahoo Protocol Connection Manager */
class YahooSessionManager : public QObject
{
	Q_OBJECT

public:
	static YahooSessionManager *manager();
	YahooSessionManager();
	~YahooSessionManager();


	/* Creates a new session */
	YahooSession* createSession(const QString username, const QString password);
	bool cleanSessions();
	YahooSession* session(int id);
	int sessionCount() const;

	/* Sets the host and port for connection to the pager and f.t. servers */
	void setPager(QString host, int port);
	void setFileTransfer(QString host, int port);

	int _hostConnectReceiver(char *host, int port);

private:
	QMap< int, YahooSession*> m_sessionsMap;
	static YahooSessionManager *managerStatic_;
};

// Yahoo Protocol Connection
class YahooSession : public QObject
{
	friend class YahooSessionManager;
	Q_OBJECT

public:
	~YahooSession();
	int sessionId() const;

	int setLogLevel(enum yahoo_log_level level);

	/* YahooSession public API */

	void login(int initial);
	void logOff();

	void setIdentityStatus( const QString &identity, int active);
	void getList();

public slots:
	void keepalive();
	void refresh();

public:
	void sendIm( const QString &from, const QString &who, const QString &msg);
	void sendTyping( const QString &from, const QString &who, int typ);
	void setAway( enum yahoo_status state, const QString &msg, int away);
	void addBuddy( const QString &who, const QString &group);
	void removeBuddy( const QString &who, const QString &group);
	void rejectBuddy( const QString &who, const QString &msg);
	void ignoreBuddy( const QString &who, int unignore);
	void changeBuddyGroup( const QString &who, const QString &old_group, const QString &new_group);
	void conferenceInvite( const QString & from, const QStringList &who, const QString &room, const QString &msg);
	void conferenceAddinvite( const QString & from, const QString &who, const QString &room, const QStringList & members, const QString &msg);
	void conferenceDecline( const QString & from, const QStringList &who, const QString &room, const QString &msg);
	void conferenceMessage( const QString & from, const QStringList &who, const QString &room, const QString &msg);
	void conferenceLogon( const QString & from, const QStringList &who, const QString &room);
	void conferenceLogoff( const QString & from, const QStringList &who, const QString &room);
	int sendFile( const QString &who, const QString &msg, const QString &name, long size);
	int getUrlHandle( const QString &url, const QString &filename, unsigned long *filesize);
	enum yahoo_status currentStatus();
	const YList *getLegacyBuddyList();
	QStringList getBuddylist();
	QStringList getIgnorelist();
	QStringList getIdentities();
	QString getCookie( const QString &which);
	QString getProfile_url( void );
	
	//webcam handlers
	void requestWebcam( const QString& from );
	void closeWebcam( const QString& from );

	/* Private Receivers for libyahoo callbacks, we capture them  and emit signals
	   called only by libyahoo callbacks, don't use them */

	void _loginResponseReceiver(int succ, const char *url);
	void _gotIgnoreReceiver(YList *igns);
	void _gotBuddiesReceiver(YList *buds);
	void _gotidentitiesReceiver(char *who, int stat, char *msg, int away);
	void _gotIdentitiesReceiver(YList *ids);
	void _statusChangedReceiver(char *who, int stat, char *msg, int away);
	void _gotImReceiver(char *who, char *msg, long tm, int stat, int utf8);
	void _gotConfInviteReceiver(char *who, char *room, char *msg, YList *members);
	void _confUserDeclineReceiver(char *who, char *room, char *msg);
	void _confUserJoinReceiver(char *who, char *room);
	void _confUserLeaveReceiver(char *who, char *room);
	void _confMessageReceiver(char *who, char *room, char *msg, int utf8);
	void _gotFileReceiver(char *who, char *url, long expires, char *msg, char *fname, unsigned long fesize);
	void _contactAddedReceiver(char *myid, char *who, char *msg);
	void _rejectedReceiver(char *who, char *msg);
	void _typingNotifyReceiver(char *who, int stat);
	void _gameNotifyReceiver(char *who, int stat);
	void _mailNotifyReceiver(char *from, char *subj, int cnt);
	void _systemMessageReceiver(char *msg);
	void _errorReceiver(char *err, int fatal);
	int _logReceiver(char *fmt, ...);
	void _addHandlerReceiver(int fd, yahoo_input_condition cond, void *data);
	void _removeHandlerReceiver(int fd);
	int _hostAsyncConnectReceiver(char *host, int port,  yahoo_connect_callback callback, void *callback_data);
	
	//webcam callbacks
	void _gotWebcamImage( const char* who, const unsigned char* image, unsigned int image_size,
	                      unsigned int real_size, unsigned int timestamp );
	void _webcamDisconnected( const char* who, int reason );

signals:
	/** emitted when server says login OK */
	void loginResponse( int succ, const QString &url);

	/** emitted when servers send us our contact list */
	void gotBuddy(const QString &userid, const QString &alias, const QString &group);

	/** emitted when we've finished getting the buddy list */
	void buddyListFetched( int numBuddies );

	/** emitted when server notifies us our ignore list */
	void gotIgnore( const QStringList &igns);

	/** emitted when server notify us our identities */
	void gotIdentities( const QStringList &ids);

	/** emitted when a contact changes status */
	void statusChanged( const QString &who, int stat, const QString &msg, int away);

	/** emitted when someone send us a message */
	void gotIm( const QString &who, const QString &msg, long tm, int stat);

	/** emitted when someone invites us into a conference room */
	void gotConfInvite( const QString &who, const QString &room, const QString &msg, const QStringList &members);

	/** emitted when someone declines joining a conference room */
	void confUserDecline( const QString &who, const QString &room, const QString &msg);

	/** emitted when someone joins a conference */
	void confUserJoin( const QString &who, const QString &room);

	/** emitted when someone leaves a conference */
	void confUserLeave( const QString &who, const QString &room);

	/** emitted when someone send us a Conference message */
	void confMessage( const QString &who, const QString &room, const QString &msg);

	/** emitted when someone wants to send us a file */
	void gotFile( const QString &who, const QString &url, long expires, const QString &msg, const QString &fname, unsigned long fesize);

	/** emitted when a contact is added */
	void contactAdded( const QString &myid, const QString &who, const QString &msg);

	/** emitted when someone rejects our auth request */
	void rejected( const QString &who, const QString &msg);

	/** emitted when someone is typing a message */
	void typingNotify( const QString &who, int stat);

	/** emitted when someone invites us to join a game */
	void gameNotify( const QString &who, int stat);

	/** Notify that we have mail */
	void mailNotify( const QString &from, const QString &subject, int cnt);

	/** emitted when Yahoo servers send us a admin message */
	void systemMessage( const QString &msg);

	/** emitted when error */
	void error( const QString &err, int fatal);
	//void hostConnect(char *host, int port);
	
	/** emitted when we have a webcam image available */
	void webcamImageReceived( const QPixmap&, const QString& from );
	
	/** emitted when the webcam has been closed from the other side */
	void webcamClosed( const QString& from, int reason );

private slots:

	void slotLoginResponseReceiver( int succ, char *url);
	void slotReadReady();
	void slotWriteReady();

private:
	/* Private constructor */
	YahooSession(int id, const QString username, const QString password);

	void addHandler(int fd, yahoo_input_condition cond);
	void removeHandler(int fd);

	YahooConnectionManager m_connManager;
	void *m_data;

	QString m_Username, m_Password, m_Server; // User data

	int m_Port;
	int m_Status;
	int m_connId;
	int m_fd;

	QString m_BuddyListServer; // Buddy List server
	int m_BuddyListPort;
	
	unsigned int m_lastWebcamTimestamp;
};

#endif

// vim: set noet ts=4 sts=4 sw=4:
// kate: indent-mode csands; tab-width 4; auto-insert-doxygen on;

