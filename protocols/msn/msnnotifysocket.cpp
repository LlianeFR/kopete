/***************************************************************************
                          imservicesocket.cpp  -  description
                             -------------------
    begin                : Mon Nov 12 2001
    copyright            : (C) 2001 by Olaf Lueg
    email                : olueg@olsd.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "msnnotifysocket.h"
#include "msndispatchsocket.h"
#include "msnprotocol.h"
#include "msncontact.h"
#include "msnpreferences.h"
#include "msnidentity.h"

#include <qregexp.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kdeversion.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmdcodec.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <ktempfile.h>
#include <krun.h>

// Ugly hack to remain compatible with KDE 3.0 - We can't #ifdef out
// preprocessor directives :(
#ifndef KDE_IS_VERSION
#define KDE_IS_VERSION(x,y,z) 0
#endif

MSNNotifySocket::MSNNotifySocket( MSNIdentity *identity, const QString &msnId )
: MSNAuthSocket( msnId, identity )
{
	m_identity = identity;
	QObject::connect( this, SIGNAL( blockRead( const QString & ) ),
		this, SLOT( slotReadMessage( const QString & ) ) );

	m_dispatchSocket = 0L;
	m_newstatus = MSNProtocol::NLN;

	m_keepaliveTimer = new QTimer( this, "m_keepaliveTimer" );
	QObject::connect( m_keepaliveTimer, SIGNAL( timeout() ), SLOT( slotSendKeepAlive() ) );

	QObject::connect( this, SIGNAL( commandSent() ), SLOT( slotResetKeepAlive() ) );
}

MSNNotifySocket::~MSNNotifySocket()
{
	kdDebug(14140) << "MSNNotifySocket::~MSNNotifySocket" << endl;
}

void MSNNotifySocket::connect( const QString &pwd )
{
	m_password = pwd;
	dispatchOK=false;
	m_isHotmailAccount=false;

	m_dispatchSocket = new MSNDispatchSocket( msnId() ,this);
	QObject::connect( m_dispatchSocket,
		SIGNAL( receivedNotificationServer( const QString &, uint ) ),
		this,
		SLOT( slotReceivedServer( const QString &, uint ) ) );

/*	QObject::connect( m_dispatchSocket,
		SIGNAL( connectionFailed( ) ),
		this,
		SLOT( slotDispatchFailed( ) ) );*/
	QObject::connect( m_dispatchSocket,
		SIGNAL( socketClosed( int ) ),
		this,
		SLOT( slotDispatchClosed( ) ) );


	m_dispatchSocket->connect();
}

void MSNNotifySocket::slotReceivedServer( const QString &server, uint port )
{
	dispatchOK=true;
	MSNAuthSocket::connect( server, port );
}

void MSNNotifySocket::disconnect()
{
	if( onlineStatus() == Connected )
		sendCommand( "OUT", QString::null, false );

	m_keepaliveTimer->stop();

	MSNAuthSocket::disconnect();
}

void MSNNotifySocket::handleError( uint code, uint id )
{
	// See http://www.hypothetic.org/docs/msn/basics.php for a
	// description of all possible error codes.
	// TODO: Add support for all of these!
	switch( code )
	{
	case 201:
	case 205:
	{
		QString msg = i18n( "Invalid user! \n"
			"This MSN user does not exist. Please check the MSN ID." );
		KMessageBox::error( 0, msg, i18n( "MSN Plugin - Kopete" ) );
		break;
	}
	case 209:
	{
		/*QString msg = i18n( "You are trying to change the display name of an user who has not "
			"confirmed his or her e-mail address.\n"
			"The contact was not renamed on the server." );
		KMessageBox::error( 0, msg, i18n( "MSN Plugin - Kopete" ) );*/
		break;
	}
	case 215:
	{
		QString msg = i18n( "This MSN user already exists in this group!\n"
			"If this is not the case, please send us a detailed bug report "
			"at kopete-devel@kde.org containing the raw output on the "
			"console (in gzipped format, as it is probably a lot of output!)" );
		KMessageBox::error( 0, msg, i18n( "MSN Plugin - Kopete" ) );
		break;
	}
	case 223:
	{
		QString msg = i18n( "The maximum number of group is reached.\n"
			"You can't have more than 30 groups" );
		KMessageBox::error( 0, msg, i18n( "MSN Plugin - Kopete" ) );
		break;
	}
	case 913:
	{
		QString msg = i18n( "You cannot send messages when you are offline or when you appear offline." );
		KMessageBox::error( 0, msg, i18n( "MSN Plugin - Kopete" ) );
		break;
	}
	default:
		MSNAuthSocket::handleError( code, id );
		break;
	}
}

void MSNNotifySocket::parseCommand( const QString &cmd, uint id,
	const QString &data )
{
	//kdDebug(14140) << "MSNNotifySocket::parseCommand: Command: " << cmd << endl;

	if( cmd == "USR" )
	{
		if( data.section( ' ', 1, 1 ) == "S" )
		{
			kdDebug(14140) << "MSNNotifySocket::parseCommand: Sending response Authentication" << endl;

			KMD5 context( data.section( ' ', 2, 2 ) + m_password );
			sendCommand( "USR", "MD5 S " + context.hexDigest() );
		}
		else
		{
			// Successful auth, sync contact list
//			kdDebug(14140) << "Sending serial number" << endl;

			//sendCommand( "SYN", QString::number( _serial ) );
			sendCommand( "SYN", "0" );

			// this is our current user and friendly name
			// do some nice things with it  :-)
			QString publicName = unescape( data.section( ' ', 2, 2 ) );
			emit publicNameChanged( publicName );
		}
	}
	else if( cmd == "NLN" )
	{
		// handle, publicName, status

		MSNContact *c = static_cast<MSNContact*>( m_identity->contacts()[ data.section( ' ', 1, 1 ) ] );
		if( c )
		{
			c->setMsnStatus( MSNProtocol::convertStatus( data.section( ' ', 0, 0 ) ) );
			QString publicName = unescape( data.section( ' ', 2, 2 ) );
			if( publicName != c->displayName())
				changePublicName( publicName, c->contactId() );
		}
	}
	else if( cmd == "LST" )
	{
		// handle, publicName, group, list
		emit contactList( data.section( ' ', 4, 4 ),
			unescape( data.section( ' ', 5, 5 ) ), data.section( ' ', 6, 6 ),
			data.section( ' ', 0, 0 ) );
	}
	else if( cmd == "MSG" )
	{
		readBlock( data.section( ' ', 2, 2 ).toUInt() );
	}
	else if( cmd == "FLN" )
	{
		MSNContact *c = static_cast<MSNContact*>( m_identity->contacts()[ data.section( ' ', 0, 0 ) ] );
		if( c )
			c->setMsnStatus( MSNProtocol::FLN );
	}
	else if( cmd == "ILN" )
	{
		// handle, publicName, Status
		MSNContact *c = static_cast<MSNContact*>( m_identity->contacts()[ data.section( ' ', 1, 1 ) ] );
		if( c )
		{
			c->setMsnStatus( MSNProtocol::convertStatus(data.section( ' ', 0, 0 )));
			QString publicName=unescape( data.section( ' ', 2, 2 ) );
			if (publicName!=c->displayName())
				changePublicName(publicName,c->contactId());
		}
	}
	else if( cmd == "XFR" )
	{
		// Address, AuthInfo
		emit startChat( data.section( ' ', 1, 1 ), data.section( ' ', 3, 3 ) );
	}
	else if( cmd == "RNG" )
	{
		// SessionID, Address, AuthInfo, handle, publicName
		emit invitedToChat( QString::number( id ), data.section( ' ', 0, 0 ), data.section( ' ', 2, 2 ),
			data.section( ' ', 3, 3 ), unescape( data.section( ' ', 4, 4 ) ) );
	}
	else if( cmd == "ADD" )
	{
		QString msnId = data.section( ' ', 2, 2 );
		uint group;
		if( data.section( ' ', 0, 0 ) == "FL" )
			group = data.section( ' ', 4, 4 ).toUInt();
		else
			group = 0;

		// handle, publicName, List, serial , group
		emit contactAdded( msnId, unescape( data.section( ' ', 2, 2 ) ),
			data.section( ' ', 0, 0 ), data.section( ' ', 1, 1 ).toUInt(), group );
	}
	else if( cmd == "REM" ) // someone is removed from a list
	{
		uint group;
		if( data.section( ' ', 0, 0 ) == "FL" )
			group = data.section( ' ', 3, 3 ).toUInt();
		else
			group = 0;

		// handle, list, serial, group
		emit contactRemoved( data.section( ' ', 2, 2 ), data.section( ' ', 0, 0 ),
			data.section( ' ', 1, 1 ).toUInt(), group );
	}
	else if( cmd == "OUT" )
	{
		disconnect();
		if( data.section( ' ', 0, 0 ) == "OTH" )
		{
			KMessageBox::information( 0, i18n( "You have connected from another client" ) , i18n ("MSN Plugin") );
		}
	}
	else if( cmd == "CHG" )
	{
		QString status = data.section( ' ', 0, 0 );
		setOnlineStatus( Connected );
		emit statusChanged( status );
	}
	else if( cmd == "REA" )
	{
		QString handle=data.section( ' ', 1, 1 );
		if( handle == msnId() )
			emit publicNameChanged( unescape( data.section( ' ', 2, 2 ) ) );
		else
		{
			MSNContact *c = static_cast<MSNContact*>( m_identity->contacts()[ handle ] );
			if( c )
				c->setDisplayName( unescape( data.section( ' ', 2, 2 ) ) );
		}
	}
	else if( cmd == "LSG" )
	{
		emit groupListed( unescape( data.section( ' ', 4, 4 ) ), data.section( ' ', 3, 3 ).toUInt() );
	}
	else if( cmd == "ADG" )
	{
		// groupName, group , serial
		emit groupAdded( unescape( data.section( ' ', 1, 1 ) ),
			data.section( ' ', 2, 2 ).toUInt(),
			data.section( ' ', 0, 0 ).toUInt() );
	}
	else if( cmd == "REG" )
	{
		// groupName, group , serial
		emit groupRenamed( unescape( data.section( ' ', 2, 2 ) ),
			data.section( ' ', 1, 1 ).toUInt(),
			data.section( ' ', 0, 0 ).toUInt() );
	}
	else if( cmd == "RMG" )
	{
		// group , serial
		emit groupRemoved( data.section( ' ', 1, 1 ).toUInt() , data.section( ' ', 0, 0 ).toUInt());
	}
	else if( cmd  == "CHL" )
	{
		kdDebug(14140) << "Sending final Authentication" << endl;
		KMD5 context( data.section( ' ', 0, 0 ) + "Q1P7W2E4J9R8U3S5" );
		sendCommand( "QRY", "msmsgs@msnmsgr.com", true,
			context.hexDigest());
	}
	else if( cmd == "SYN" )
	{
/*		// this is the current serial on the server, if its different with the own we can get the user list
		//isConnected = true;
		uint serial = data.section( ' ', 0, 0 ).toUInt();
		if( serial != _serial)
		{
			emit newSerial(serial);  // remove all contacts, msn sends a new contact list
			_serial = serial;
		}*/
		//emit connected(true);
		// set the status
		setStatus(m_newstatus);
	}
	else if( cmd == "BPR" )
	{
		MSNContact *c = static_cast<MSNContact*>( m_identity->contacts()[ data.section( ' ', 0, 0 ) ] );
		if( c )
		{
			c->setInfo(data.section( ' ', 1, 1 ),unescape(data.section( ' ', 2, 2 )));
		}
//		emit recievedInfo(data.section( ' ', 0, 0 ), data.section( ' ', 1, 1 ) , unescape(data.section( ' ', 2, 2 )));
	}
	else if( cmd == "QRY" )
	{
		//do nothing
	}
	else
	{
		// Let the base class handle the rest
		MSNAuthSocket::parseCommand( cmd, id, data );
	}
}

void MSNNotifySocket::slotOpenInbox()
{
	if( m_isHotmailAccount )
	{
		KTempFile tmpFile( locateLocal( "tmp", "kopetehotmail-" ), ".html" );
		*tmpFile.textStream() << m_hotmailRequest;

		// In KDE 3.1 and older this will leave a stale temp file lying
		// around. There's no easy way for us to detect the browser exiting
		// though, so we can't do anything about it. For KDE 3.2 and newer
		// we use the improved KRun that auto-deletes the temp file when done.

#if KDE_IS_VERSION(3,1,90)
		KRun::runURL( tmpFile.name(), "text/html", true );
#else
		KRun::runURL( tmpFile.name(), "text/html" );
#endif
	}
}

void MSNNotifySocket::slotReadMessage( const QString &msg )
{
	if(msg.contains("text/x-msmsgsinitialemailnotification"))
	{
		 //this sends the server if we are going online, contains the unread message count
		QString m = msg.right(msg.length() - msg.find("Inbox-Unread:") );
		m = m.left(msg.find("\r\n"));
		mailCount = m.right(m.length() -m.find(" ")-1).toUInt();

		if(MSNPreferences::mailNotifications())
		{
			int answer = KMessageBox::No;

			if(mailCount != 0)
				answer = KMessageBox::questionYesNo( 0l, i18n( "<qt>You have %1 unread messages in your inbox.<br>Would you like to open your inbox now?</qt>" ).arg(mailCount), i18n( "MSN Plugin" ) );
 
			if(answer==KMessageBox::Yes)
				slotOpenInbox();
		}
	}
	else if(msg.contains("text/x-msmsgsactivemailnotification"))
	{
		 //this sends the server if mails are deleted
		 QString m = msg.right(msg.length() - msg.find("Message-Delta:") );
		 m = m.left(msg.find("\r\n"));
		 mailCount = mailCount - m.right(m.length() -m.find(" ")-1).toUInt();
	}
	else if(msg.contains("text/x-msmsgsemailnotification"))
	{
		 //this sends the server if a new mail has arrived
		QRegExp rx("From-Addr: ([A-Za-z0-9@._\\-]*)");
		rx.search(msg);
		QString m=rx.cap(1);

		mailCount++;

		if(MSNPreferences::mailNotifications())
		{
			int answer=KMessageBox::questionYesNo( 0l, i18n( "<qt>You have one new e-mail from %1.<br>Would you like to open your inbox now?</qt>" ).arg(m), i18n( "MSN Plugin" ) );

			if(answer==KMessageBox::Yes)
			{
				slotOpenInbox();
			}
		}
	}
	else if(msg.contains("text/x-msmsgsprofile"))
	{
		if( msnId().contains("@hotmail.com") || msnId().contains("@msn.com"))
		{
			//Hotmail profile
			if(msg.contains("MSPAuth:"))
			{
				QRegExp rx("MSPAuth: ([A-Za-z0-9$!*]*)");
				rx.search(msg);
				m_MSPAuth=rx.cap(1);
			}
			if(msg.contains("sid:"))
			{
				QRegExp rx("sid: ([0-9]*)");
				rx.search(msg);
				m_sid=rx.cap(1);
			}
			if(msg.contains("kv:"))
			{
				QRegExp rx("kv: ([0-9]*)");
				rx.search(msg);
				m_kv=rx.cap(1);
			}

			//write the tmp file
			QString UserID=MSNPreferences::msnId();

			QString md5this(m_MSPAuth+"1"+m_password);
			KMD5 md5(md5this);

			KTempFile tmpFile(locateLocal("tmp", "kopetehotmail"), ".html");

			m_hotmailRequest =
				"<html>\n"
				"<head>\n"
				"<noscript>\n"
				"<meta http-equiv=Refresh content=\"0; url=http://www.hotmail.com\">\n"
				"</noscript>\n"
				"</head>\n"
				"<body onload=\"document.pform.submit(); \">\n"
				"<form name=\"pform\" action=\"https://loginnet.passport.com/ppsecure/md5auth.srf?lc=1033\" method=\"POST\">\n"
				"<input type=\"hidden\" name=\"mode\" value=\"ttl\">\n"
				"<input type=\"hidden\" name=\"login\" value=\"" + UserID.left( UserID.find('@') ) + "\">\n"
				"<input type=\"hidden\" name=\"username\" value=\"" + UserID + "\">\n"
				"<input type=\"hidden\" name=\"sid\" value=\"" + m_sid + "\">\n"
				"<input type=\"hidden\" name=\"kv\" value=\"" + m_kv + "\">\n"
				"<input type=\"hidden\" name=\"id\" value=\"2\">\n"
				"<input type=\"hidden\" name=\"sl\" value=\"1\">\n"
				"<input type=\"hidden\" name=\"rru\" value=\"/cgi-bin/HoTMaiL\">\n"
				"<input type=\"hidden\" name=\"auth\" value=\"" + m_MSPAuth + "\">\n"
				"<input type=\"hidden\" name=\"creds\" value=\"" + QString::fromLatin1( md5.hexDigest() ) + "\">\n"
				"<input type=\"hidden\" name=\"svc\" value=\"mail\">\n"
				"<input type=\"hidden\" name=\"js\" value=\"yes\">\n"
				"</form></body>\n"
				"</html>\n";

			m_isHotmailAccount = true;
			emit hotmailSeted(true);
		}
	}
}

void MSNNotifySocket::addGroup(QString groupName)
{
	// escape spaces
	sendCommand( "ADG", escape( groupName ) + " 0" );
}

void MSNNotifySocket::renameGroup( QString groupName, uint group )
{
	// escape spaces
	sendCommand( "REG", QString::number( group ) + " " +
		escape( groupName ) + " 0" );
}

void MSNNotifySocket::removeGroup( uint group )
{
	sendCommand( "RMG", QString::number( group ) );
}

void MSNNotifySocket::addContact( const QString &handle, QString publicName, uint group, int list )
{
	QString args;
	switch( list )
	{
	case MSNProtocol::FL:
		args = "FL " + handle + " " + escape( publicName ) + " " + QString::number( group );
		break;
	case MSNProtocol::AL:
		args = "AL " + handle + " " + escape( publicName );
		break;
	case MSNProtocol::BL:
		args = "BL " + handle + " " + escape( publicName );
		break;
	default:
		kdDebug(14140) << "MSNNotifySocket::addContact: WARNING! Unknown list " <<
			list << "!" << endl;
		return;
	}
	sendCommand( "ADD", args );
}

void MSNNotifySocket::removeContact( const QString &handle, uint group,	int list )
{
	QString args;
	switch( list )
	{
	case MSNProtocol::FL:
		args = "FL " + handle + " " + QString::number( group );
		break;
	case MSNProtocol::AL:
		args = "AL " + handle;
		break;
	case MSNProtocol::BL:
		args = "BL " + handle;
		break;
	default:
		kdDebug(14140) << "MSNNotifySocket::removeContact: " <<
			"WARNING! Unknown list " << list << "!" << endl;
		return;
	}
	sendCommand( "REM", args );
}

void MSNNotifySocket::setStatus( int status )
{
	kdDebug(14140) << "MSNNotifySocket::setStatus : " <<statusToString(status) <<endl;
	if(onlineStatus() == Disconnected)
		m_newstatus=status;
	else
		sendCommand( "CHG", statusToString( status ) );

}

void MSNNotifySocket::changePublicName( const QString &publicName, const QString &handle )
{
	if( handle.isNull() )
		sendCommand( "REA", msnId() + " " + escape ( publicName ) );
	else
		sendCommand( "REA", handle + " " + escape ( publicName ) );
}

void MSNNotifySocket::createChatSession()
{
	sendCommand( "XFR", "SB" );
}

QString MSNNotifySocket::statusToString( int status ) const
{
	switch( status )
	{
	case MSNProtocol::NLN:
		return "NLN";
	case MSNProtocol::BSY:
		return "BSY";
	case MSNProtocol::BRB:
		return "BRB";
	case MSNProtocol::AWY:
		return "AWY";
	case MSNProtocol::PHN:
		return "PHN";
	case MSNProtocol::LUN:
		return "LUN";
	case MSNProtocol::FLN:
		return "FLN";
	case MSNProtocol::HDN:
		return "HDN";
	case MSNProtocol::IDL:
		return "IDL";
	default:
		kdDebug(14140) << "MSNNotifySocket::statusToString: " <<
			"WARNING! Unknown status " << status << "!" << endl;
		return QString::null;
	}
}

void MSNNotifySocket::slotDispatchClosed()
{
	delete m_dispatchSocket;
	m_dispatchSocket = 0L;
	if(!dispatchOK)
	{
		KMessageBox::error( 0, i18n( "Connection failed\nTry again later." ) , i18n ("MSN Plugin") );
		//because "this socket" isn't already connected, doing this manualy
		emit onlineStatusChanged( Disconnected );
		emit socketClosed(-1);
	}
}

void MSNNotifySocket::slotSendKeepAlive()
{
	// Send a dummy command to fake activity. This makes sure MSN doesn't
	// disconnect you when the notify socket is idle.
	sendCommand( "LSG", QString::null );
}

void MSNNotifySocket::slotResetKeepAlive()
{
	// Fire the timer every 60 seconds. QTimer will reset a running timer
	// on a subsequent call if there has been activity again.
	m_keepaliveTimer->start( 60000 );
}

#include "msnnotifysocket.moc"

// vim: set noet ts=4 sts=4 sw=4:

