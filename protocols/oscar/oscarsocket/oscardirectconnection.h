/*
    oscardirectconnection.h  -  Implementation of an oscar direct connection

    Copyright (c) 2002 by Tom Linsky <twl6@po.cwru.edu>

    Kopete    (c) 2002 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/
#ifndef OSCARDIRECTCONNECTION_H
#define OSCARDIRECTCONNECTION_H

#include "oscarconnection.h"

/**Direct oscar connection
  *@author Tom Linsky
  */

struct ODC2 { //direct connect header
	WORD channel;
	WORD headerLength;
  char *cookie;
  WORD type;
  DWORD length;
  char *sn;
  char *message;	
};

class OscarSocket;

class OscarDirectConnection : public OscarConnection  {
	Q_OBJECT
public: 
	OscarDirectConnection(OscarSocket *serverconn, const QString &connName, QObject *parent=0, const char *name=0);
	~OscarDirectConnection();
  /** Sets the socket to use socket, state() to connected, and emit connected() */
  virtual void setSocket( int socket );
  /** Sends the direct IM message to buddy */
  void sendIM(const QString &message, bool isAuto);
  /** Sends a typing notification to the server
			@param notifyType Type of notify to send
	 */
  void sendTypingNotify(TypingNotify notifyType);
	
protected slots: // Protected slots
  /** Called when there is data to be read from peer */
  virtual void slotRead(void);

private: // Private methods
  /** Gets an ODC2 header */
  ODC2 getODC2(void);
  /** Prepares and sends a block with the given message and typing notify flag attached */
  void sendODC2Block(const QString &message, WORD typingnotify);
  
private slots: // Private slots
  /** Called when we have established a connection */
  void slotConnected(void);
  /** Called when the connection is closed */
  void slotConnectionClosed(void);

private: //private members
	/** The main oscar connection */
	OscarSocket *mMainConn;
	/** the message cookie */
	char mCookie[8];

signals:
  /** Emitted when the connection is closed */
  void connectionClosed(OscarDirectConnection *conn);
};

#endif
