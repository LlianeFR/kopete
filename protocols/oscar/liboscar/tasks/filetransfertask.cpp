/*
    Kopete Oscar Protocol
    File Transfer Task

    Copyright 2006 Chani Armitage <chanika@gmail.com>
    Copyright 2007 Matt Rogers <mattr@kde.org>

    Kopete ( c ) 2002-2004 by the Kopete developers <kopete-devel@kde.org>

    based on ssidata.h and ssidata.cpp ( c ) 2002 Tom Linsky <twl6@po.cwru.edu>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or ( at your option ) any later version.    *
    *                                                                       *
    *************************************************************************
*/

#include "filetransfertask.h"

#include <QtCore/QFileInfo>
#include <QtCore/QTextCodec>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtXml/QDomDocument>

#include <ksocketfactory.h>
#include <krandom.h>

#include <klocale.h>
#include <kdebug.h>

#include "buffer.h"
#include "oscarutils.h"
#include "oscarmessage.h"
#include "connection.h"
#include "oscarsettings.h"
#include "oftmetatransfer.h"

#include "kopetetransfermanager.h"

//receive
FileTransferTask::FileTransferTask( Task* parent, const QString& contact,
                                    const QString& self, QByteArray cookie,
                                    Buffer b  )
: Task( parent ), m_contactName( contact ), m_selfName( self ),
  m_timer( this )
{
	init( Receive );
	initOft();
	m_oftRendezvous.cookie = cookie;
	parseReq( b );
}

//send
FileTransferTask::FileTransferTask( Task* parent, const QString& contact,
                                    const QString& self, const QStringList& files,
                                    Kopete::Transfer *transfer )
:Task( parent ), m_contactName( contact ),
 m_selfName( self ), m_timer( this )
{
	init( Send );
	initOft();

	m_oftRendezvous.files = files;
	m_oftRendezvous.fileCount = files.size();
	for ( int i = 0; i < m_oftRendezvous.fileCount; ++i )
	{
		QFileInfo fileInfo( m_oftRendezvous.files.at(i) );
		m_oftRendezvous.totalSize += fileInfo.size();
	}

	if ( m_oftRendezvous.fileCount == 1 )
	{ //get filename without path
		m_oftRendezvous.fileName = QFileInfo( files.at(0) ).fileName();
	}

	//we get to make up an icbm cookie!
	Buffer b;
	Oscar::DWORD cookie = KRandom::random();
	b.addDWord( cookie );
	cookie = KRandom::random();
	b.addDWord( cookie );
	m_oftRendezvous.cookie = b.buffer();

	//hook up ui cancel
	connect( transfer, SIGNAL(result(KJob*)), this, SLOT(transferResult(KJob*)) );
#ifdef __GNUC__
#warning Remove connection below when transferResult correctly sends cancel from cancel button. (Kedge)
#endif
	connect( transfer, SIGNAL(finished(KJob*)), this, SLOT(doCancel()) );
	//hook up our ui signals
	connect( this, SIGNAL(transferCancelled()), transfer, SLOT(slotCancelled()) );
	connect( this, SIGNAL(transferError(int, const QString&)), transfer, SLOT(slotError(int, const QString&)) );
	connect( this, SIGNAL(transferProcessed(unsigned int)), transfer, SLOT(slotProcessed(unsigned int)) );
	connect( this, SIGNAL(transferFinished()), transfer, SLOT(slotComplete()) );
}

void FileTransferTask::init( Action act )
{
	m_action = act;
	m_tcpServer = 0;
	m_connection = 0;
	m_port = 0;
	m_proxy = false;
	m_proxyRequester = false;
	m_state = Default;
}

FileTransferTask::~FileTransferTask()
{
	if( m_tcpServer )
	{
		delete m_tcpServer;
		m_tcpServer = 0;
	}

	if( m_connection )
	{
		m_connection->close();
		delete m_connection;
		m_connection = 0;
	}
	kDebug(OSCAR_RAW_DEBUG) << "done";
}

void FileTransferTask::onGo()
{
	if ( m_action == Receive )
	{
		//we have to send a signal because liboscar isn't supposed to know about OscarContact.

		Kopete::TransferManager *tm = 0;
		emit getTransferManager( &tm );
		connect( tm, SIGNAL(refused(const Kopete::FileTransferInfo&)), this, SLOT(doCancel(const Kopete::FileTransferInfo&)) );
		connect( tm, SIGNAL(accepted(Kopete::Transfer*, const QString&)), this, SLOT(doAccept(Kopete::Transfer*, const QString&)) );

		emit askIncoming( m_contactName, m_oftRendezvous.fileName, m_oftRendezvous.totalSize, m_desc, m_oftRendezvous.cookie.toHex() );
		return;
	}
	//else, send
	if ( m_contactName.isEmpty() )
	{
		setSuccess( 0 );
		return;
	}

	for ( int i = 0; i < m_oftRendezvous.fileCount; ++i )
	{
		if ( !validFile( m_oftRendezvous.files.at(i) ) )
		{
			setSuccess( 0 );
			return;
		}
	}

	if ( client()->settings()->fileProxy() )
	{ //proxy stage 1
		m_proxy = true;
		m_proxyRequester = true;
		doConnect();
	}
	else
		sendReq();
}

void FileTransferTask::parseReq( Buffer b )
{
	QByteArray proxy_ip;
	QByteArray client_ip;
	QByteArray verified_ip;
	QByteArray fileName;
	QTextCodec *c=0;
	m_altIp.clear(); //just to be sure

	while( b.bytesAvailable() )
	{
		TLV tlv = b.getTLV();
		Buffer b2( tlv.data );
		switch( tlv.type )
		{
		 case 0x2711: //file-specific stuff
			{
			if ( m_action == Send ) //then we don't care
				break;

			bool multipleFiles = (b2.getWord() == 0x02);
			kDebug(OSCAR_RAW_DEBUG) << "multiple file flag: " << multipleFiles;
			m_oftRendezvous.fileCount = b2.getWord();
			m_oftRendezvous.totalSize = b2.getDWord();
			fileName = b2.getBlock( b2.bytesAvailable() - 1 ); //null terminated
			kDebug(OSCAR_RAW_DEBUG) << "file: " << fileName;
			kDebug(OSCAR_RAW_DEBUG) << "total size: " << m_oftRendezvous.totalSize << " files: " << m_oftRendezvous.fileCount;
			break;
			}
		 case 0x2712:
			c = Oscar::codecForName( tlv.data );
			kDebug(OSCAR_RAW_DEBUG) << "filename encoding " << tlv.data;
			break;
		 case 2:
		 	proxy_ip = tlv.data;
			kDebug(OSCAR_RAW_DEBUG) << "proxy ip " << QHostAddress( b2.getDWord() ).toString();
			break;
		 case 3:
		 	client_ip = tlv.data;
			kDebug(OSCAR_RAW_DEBUG) << "client ip " << QHostAddress( b2.getDWord() ).toString();
			break;
		 case 4:
		 	verified_ip = tlv.data;
			kDebug(OSCAR_RAW_DEBUG) << "verified ip " << QHostAddress( b2.getDWord() ).toString();
			break;
		 case 5:
		 	m_port = b2.getWord();
			kDebug(OSCAR_RAW_DEBUG) << "port " << m_port;
			break;
		 case 0x0c:
			m_desc = parseDescription( tlv.data ); //FIXME: what codec?
			kDebug(OSCAR_RAW_DEBUG) << "user message: " << tlv.data;
			break;
		 case 0x0d:
			kDebug(OSCAR_RAW_DEBUG) << "default encoding " << tlv.data;
			break;
		 case 0x0e:
			kDebug(OSCAR_RAW_DEBUG) << "language " << tlv.data;
			break;
		 case 0x10:
		 	m_proxy = true;
			break;
		 default:
			kDebug(OSCAR_RAW_DEBUG) << "ignoring tlv type " << tlv.type;
		}
	}

	if ( !c )
	{
		c = QTextCodec::codecForName( "UTF-8" );
		kDebug(OSCAR_RAW_DEBUG) << "no codec, assuming utf8";
	}

	if ( c )
		m_oftRendezvous.fileName = c->toUnicode( fileName );
	else
	{
		kWarning(OSCAR_RAW_DEBUG) << "couldn't get any codec! ";
		m_oftRendezvous.fileName = fileName;
	}

	if( m_proxy )
	{
		kDebug(OSCAR_RAW_DEBUG) << "proxy requested";
		m_ip = proxy_ip;
	}
	else if ( client_ip.isEmpty() )
		m_ip = verified_ip;
	else if ( client_ip == "\0\0\0\0" )
	{ //ip is all 0's
		kDebug(OSCAR_RAW_DEBUG) << "proxy??";
		//wtf... I guess it wants *me* to request a proxy?
		m_proxy = true;
		m_proxyRequester = true;
	}
	else
	{
		m_ip = client_ip;
		if ( verified_ip != client_ip )
			m_altIp = verified_ip;
	}

}

bool FileTransferTask::validFile( const QString& file )
{
	QFileInfo fileInfo( file );
	if ( m_action == Send )
	{
		if ( ! fileInfo.exists() )
		{
			emit transferError( KIO::ERR_DOES_NOT_EXIST, fileInfo.fileName() );
			return 0;
		}
		if ( fileInfo.size() == 0 )
		{
			emit transferError( KIO::ERR_COULD_NOT_READ, i18n("file is empty: ") + fileInfo.fileName() );
			return 0;
		}
		if ( ! fileInfo.isReadable() )
		{
			emit transferError( KIO::ERR_CANNOT_OPEN_FOR_READING, fileInfo.fileName() );
			return 0;
		}
	}
	else //receive
	{ //note: opening for writing clobbers the file
		if ( fileInfo.exists() )
		{
			if ( ! fileInfo.isWritable() )
			{ //it's there and readonly
				emit transferError( KIO::ERR_CANNOT_OPEN_FOR_WRITING, fileInfo.fileName() );
				return 0;
			}
		}
		else if ( ! QFileInfo( fileInfo.path() ).isWritable() )
		{ //not allowed to create it
			emit transferError( KIO::ERR_CANNOT_OPEN_FOR_WRITING, fileInfo.fileName() );
			return 0;
		}
	}
	return true;
}

bool FileTransferTask::validDir( const QString& dir )
{
	QFileInfo fileInfo( dir );
	if ( m_action == Receive )
	{
		if ( fileInfo.exists() && fileInfo.isDir() )
		{
			if ( !fileInfo.isWritable() )
			{ //it's there and readonly
				emit transferError( KIO::ERR_CANNOT_OPEN_FOR_WRITING, dir );
				return false;
			}
		}
		else
		{ //not allowed o create it
			emit transferError( KIO::ERR_CANNOT_OPEN_FOR_WRITING, dir );
			return false;
		}
	}
	return true;
}

bool FileTransferTask::take( Transfer* transfer )
{
	Q_UNUSED(transfer);
	return false;
}

bool FileTransferTask::take( int type, QByteArray cookie, Buffer b )
{
	kDebug(14151) << "comparing to " << m_oftRendezvous.cookie.toHex();
	if ( cookie != m_oftRendezvous.cookie )
		return false;

	//ooh, ooh, something happened!
	switch( type )
	{
	 case 0: //direct transfer ain't good enough
		kDebug(OSCAR_RAW_DEBUG) << "redirect or proxy request";
		if ( m_state != Listening )
		{
			kDebug(OSCAR_RAW_DEBUG) << "other client is insane.";
			break;
		}

		m_tcpServer->close();
		delete m_tcpServer;
		m_tcpServer = 0;
		parseReq( b );
		doConnect();
		break;
	 case 1:
		kDebug(OSCAR_RAW_DEBUG) << "other user cancelled filetransfer :(";
		emit transferCancelled(); //FIXME: what if it's not hooked up?
		emit cancelOft();
		m_timer.stop();
		setSuccess( true );
		break;
	 case 2:
		kDebug(OSCAR_RAW_DEBUG) << "other user acceptetd filetransfer :)";
		break;
	 default:
		kWarning(OSCAR_RAW_DEBUG) << "bad request type: " << type;
	}
	return true;
}

bool FileTransferTask::takeAutoResponse( int type, QByteArray cookie, Buffer* b )
{
	if ( cookie != m_oftRendezvous.cookie )
		return false;

	switch( type )
	{
	case 3: //channel specific data
		if ( b->getWord() == 0x0002 )
		{
			Oscar::WORD data = b->getWord();
			if ( data == 0x0001 )
				kDebug(OSCAR_RAW_DEBUG) << "other user cancelled filetransfer :(";
			else if ( data == 0x0006 )
				kDebug(OSCAR_RAW_DEBUG) << "other client terminated filetransfer :(";

			if ( data == 0x0001 || data == 0x0006 )
			{
				emit transferCancelled();
				emit cancelOft();
				m_timer.stop();
				setSuccess( true );
				break;
			}
		}
	case 1: //channel not supported
	case 2: //busted payload
	default:
		kWarning(OSCAR_RAW_DEBUG) << "unknown response for type: " << type;
	}
	return true;
}

void FileTransferTask::readyAccept()
{
	kDebug(OSCAR_RAW_DEBUG) << "******************";
	m_connection = m_tcpServer->nextPendingConnection();

	// Reparent because we want to delete TcpServer;
	if ( m_connection )
		m_connection->setParent( 0 );

	m_tcpServer->close(); //free up the port so others can listen
	delete m_tcpServer;
	m_tcpServer = 0;
	
	if ( !m_connection )
	{ //either it wasn't buffered, or it did something weird
		kDebug(OSCAR_RAW_DEBUG) << "connection failed somehow.";
		emit transferError( KIO::ERR_COULD_NOT_ACCEPT, QString() );
		doCancel();
		return;
	}
	//ok, so we have a direct connection. for filetransfers. cool.
	//now we go on to the OFT phase.
	doOft();
}

void FileTransferTask::doOft()
{
	kDebug(OSCAR_RAW_DEBUG) << "******************";
	QObject::disconnect( m_connection, 0, 0, 0 ); //disconnect signals
	m_state = OFT;
	OftMetaTransfer *oft;

	if ( m_action == Receive )
		oft = new OftMetaTransfer( m_oftRendezvous.cookie, m_oftRendezvous.dir, m_connection );
	else
		oft = new OftMetaTransfer( m_oftRendezvous.cookie, m_oftRendezvous.files, m_connection );

	m_connection = 0; //it's not ours any more
	//might be a good idea to hook up some signals&slots.
	connect( oft, SIGNAL(fileProcessed(unsigned int, unsigned int)),
	         this, SIGNAL(transferProcessed(unsigned int)) );
	connect( oft, SIGNAL(transferError(int, const QString&)),
	         this, SLOT(errorOft(int, const QString&)) );
	connect( oft, SIGNAL(transferCompleted()), this, SLOT(doneOft()) );
	connect( this, SIGNAL(cancelOft()), oft, SLOT(doCancel()) );
	//now we can finally send the first OFT packet.
	if ( m_action == Send )
		oft->start();
}

void FileTransferTask::errorOft( int errorCode, const QString &error )
{
	emit transferError( KIO::ERR_USER_CANCELED, error );
	doCancel();
}

void FileTransferTask::doneOft()
{
	emit transferFinished();
	setSuccess( true );
}

void FileTransferTask::transferResult( KJob* job )
{
	if( job->error() == KIO::ERR_USER_CANCELED )
		doCancel();
}

void FileTransferTask::socketError( QAbstractSocket::SocketError e )
{ //FIXME: handle this properly for all cases
	QString desc;
	
	desc = m_connection->errorString();
	kWarning(OSCAR_RAW_DEBUG) << "socket error: " << e << " : " << desc;
	if ( m_state == Connecting )
	{ //connection failed, try another way
		if ( m_proxy )
		{ //fuck, we failed at a proxy! just give up
			emit transferError( KIO::ERR_COULD_NOT_CONNECT, desc );
			doCancel();
		}
		else
		{
			m_timer.stop();
			connectFailed();
		}
	}
}

void FileTransferTask::proxyRead()
{
	if ( m_state != ProxySetup )
	{
		kWarning(OSCAR_RAW_DEBUG) << "reading non-proxy data!";
	}

	kDebug(OSCAR_RAW_DEBUG) ;
	QByteArray raw = m_connection->readAll(); //is this safe?
	kDebug(OSCAR_RAW_DEBUG) ;
	Buffer b( raw );
	Oscar::WORD length = b.getWord();
	if ( b.bytesAvailable() != length )
		kWarning(OSCAR_RAW_DEBUG) << "length is " << length << " but we have " << b.bytesAvailable() << "bytes!";
	b.skipBytes( 2 ); //protocol version
	Oscar::WORD command = b.getWord();
	b.skipBytes( 6 ); //4 unknown, 2 flags

	switch( command )
	{
		case 1: //error
		{
			Oscar::WORD err = b.getWord();
			QString errMsg;
			switch( err )
			{
				case 0x0d: //we did something wrong (eg. wrong username)
				case 0x0e: //we did something wronger (eg. no cookie)
					errMsg = i18n("Bad Request");
					break;
				case 0x10: //we did something else wrong
					errMsg = i18n("Request Timed Out");
					break;
				case 0x1a: //other side was too slow
					errMsg = i18n("Acceptance Period Timed Out");
					break;
				default:
					errMsg = i18n("Unknown Error: ") + QString::number( err );
			}

			emit transferError( KIO::ERR_COULD_NOT_LOGIN, errMsg );
			doCancel();
			break;
		}
		case 3: //ack
			m_port = b.getWord();
			m_ip = b.getBlock( 4 );

			kDebug(OSCAR_RAW_DEBUG) << "got port " << m_port << " ip "
			                        << QHostAddress( Buffer( m_ip ).getDWord() ).toString();;
			//now we send a proxy request to the other side
			sendReq();
			break;
		case 5: //ready
			doneConnect();
	}
}

void FileTransferTask::initOft()
{
	//set up the default values for the oft
	m_oftRendezvous.cookie = 0;
	m_oftRendezvous.fileCount = 0;
	m_oftRendezvous.totalSize = 0;
}

void FileTransferTask::doCancel()
{
	kDebug(OSCAR_RAW_DEBUG) ;
	//tell the other side
	if ( m_state != OFT )
	{
		Oscar::Message msg = makeFTMsg();
		msg.setRequestType( 1 );
		emit sendMessage( msg );
	}
	//stop our timer in case we were doing stuff
	m_timer.stop();
	emit cancelOft();
	setSuccess( true );
}


void FileTransferTask::doCancel( const Kopete::FileTransferInfo &info )
{
	kDebug(OSCAR_RAW_DEBUG) ;
	//check that it's really for us
	if ( info.internalId() == QString( m_oftRendezvous.cookie.toHex() ) )
		doCancel();
	else
		kDebug(OSCAR_RAW_DEBUG) << "ID mismatch";
}

void FileTransferTask::doAccept( Kopete::Transfer *t, const QString & localName )
{
	kDebug(OSCAR_RAW_DEBUG) ;
	//check that it's really for us
	if ( t->info().internalId() != QString( m_oftRendezvous.cookie.toHex() ) )
	{
		kDebug(OSCAR_RAW_DEBUG) << "ID mismatch";
		return;
	}

	//TODO: we should unhook the old transfermanager signals now
	//hook up the ones for the transfer
	connect( t, SIGNAL(result(KJob*)), this, SLOT(transferResult(KJob*)) );
#ifdef __GNUC__
#warning Remove connection below when transferResult correctly sends cancel from cancel button. (Kedge)
#endif
	connect( t, SIGNAL(finished(KJob*)), this, SLOT(doCancel()) );
	connect( this, SIGNAL(transferCancelled()), t, SLOT(slotCancelled()) );
	connect( this, SIGNAL(transferError(int, const QString&)), t, SLOT(slotError(int, const QString&)) );
	connect( this, SIGNAL(transferProcessed(unsigned int)), t, SLOT(slotProcessed(unsigned int)) );
	connect( this, SIGNAL(transferFinished()), t, SLOT(slotComplete()) );
	//and save the chosen filename

	/*FIXME: Should we prompt for a file name if we receive one file or only for a directory like the official
	         icq and aim clients (if we have multiple files we don't know their names at this stage) */
#ifdef __GNUC__
#warning Hack, this will be removed as soon as I add a function into Kopete::TransferManager that will prompt for a directory (Kedge)
#endif
	QFileInfo fileInfo( localName );
	m_oftRendezvous.dir = fileInfo.absolutePath() + '/';

	if( validDir( m_oftRendezvous.dir ) )
		doConnect();
	else
		doCancel();
}

void FileTransferTask::doConnect()
{
	kDebug(OSCAR_RAW_DEBUG) ;

	QString host;
	if ( m_proxyRequester )
		host = "ars.oscar.aol.com";
	else
	{
		if ( m_ip.length() != 4 || ! m_port )
		{
			emit transferError( KIO::ERR_COULD_NOT_CONNECT, i18n("missing IP or port") );
			doCancel();
			return;
		}

		//ksockets demand a qstring
		Buffer ipBuffer( m_ip );
		host = QHostAddress( ipBuffer.getDWord() ).toString();
		kDebug(OSCAR_RAW_DEBUG) << "ip: " << host;
	}

	//proxies *always* use port 5190; the "port" value is some retarded check
	m_connection = new QTcpSocket();
	connect( m_connection, SIGNAL(readyRead() ), this, SLOT(proxyRead()) );
	connect( m_connection, SIGNAL(error(QAbstractSocket::SocketError)),
	         this, SLOT(socketError(QAbstractSocket::SocketError)) );
	connect( m_connection, SIGNAL(connected()), this, SLOT(socketConnected()));

	m_state = Connecting;
	//socket doesn't seem to have its own timeout, so here's mine
	m_timer.disconnect();
	connect( &m_timer, SIGNAL( timeout() ), this, SLOT( timeout() ) );
	m_timer.start( client()->settings()->timeout()  * 1000 );
	//try it
	KSocketFactory::connectToHost( m_connection, QString(), host, m_proxy ? 5190 : m_port );
}

void FileTransferTask::socketConnected()
{
	kDebug(OSCAR_RAW_DEBUG) ;
	m_timer.stop();
	if( m_proxy )
	//we need to send an init recv first
		proxyInit();
	else
		doneConnect();
}

void FileTransferTask::doneConnect()
{
	m_state = Default;
	if ( ! m_proxyRequester )
	{ //yay! send an accept message
		Oscar::Message msg = makeFTMsg();
		msg.setRequestType( 2 );
		emit sendMessage( msg );
	}
	//next the receiver should get a prompt from the sender.
	doOft();
}

void FileTransferTask::proxyInit()
{
	m_state = ProxySetup;
	//init "send" is sent by whoever requested the proxy.
	//init "recv" is sent by the other side
	//because the second person has to include the port check

	Buffer data;
	data.addBUIN( m_selfName.toLatin1() );

	if ( !m_proxyRequester ) //if 'recv'
		data.addWord( m_port );

	data.addString( m_oftRendezvous.cookie );
	data.addTLV( 0x0001, oscar_caps[CAP_SENDFILE] ); //cap tlv

	Buffer header;
	header.addWord( 10 + data.length() ); //length
	header.addWord( 0x044a ); //packet version
	header.addWord( m_proxyRequester ? 2 : 4 ); //command: 2='send' 4='recv'
	header.addDWord( 0 ); //unknown
	header.addWord( 0 ); //flags

	header.addString( data );
	//send it to the proxy server
	int written = m_connection->write( header.buffer() );

	if( written == -1 ) //FIXME: handle this properly
		kDebug(OSCAR_RAW_DEBUG) << "failed to write :(";

}

void FileTransferTask::timeout()
{
	kDebug(OSCAR_RAW_DEBUG);
	m_timer.stop();
	if ( m_state == Connecting )
	{ //kbufferedsocket took too damn long
		if ( m_proxy )
		{ //fuck, we failed at a proxy! just give up
			emit transferError( KIO::ERR_COULD_NOT_CONNECT, i18n("Timeout") );
			doCancel();
		}
		else
			connectFailed();
		return;
	}

	//nothing's happened for ages - assume we're dead.
	//so tell the user, send off a cancel, and die
	emit transferError( KIO::ERR_ABORTED, i18n("Timeout") );
	doCancel();
}

void FileTransferTask::connectFailed()
{
	m_connection->close();
	delete m_connection;
	m_connection = 0;
	bool proxy = client()->settings()->fileProxy();
	if (! proxy )
	{
		if ( ! m_altIp.isEmpty() )
		{ //there's another ip to try
			m_ip = m_altIp;
			m_altIp.clear();
			doConnect();
			return;
		}
		if ( m_action == Receive )
		{ //try redirect
			sendReq();
			return;
		}
	}

	//proxy stage 2 or 3
	m_proxy = true;
	m_proxyRequester = true;
	doConnect();
}

bool FileTransferTask::listen()
{
	kDebug(OSCAR_RAW_DEBUG) ;
	m_state = Default;
	//listen for connections
	m_tcpServer = new QTcpServer( this );
	m_tcpServer->setProxy( KSocketFactory::proxyForListening( QString() ) );
	
	connect( m_tcpServer, SIGNAL(newConnection()), this, SLOT(readyAccept()) );
	
	bool success = false;
	int first = client()->settings()->firstPort();
	int last = client()->settings()->lastPort();
	//I don't trust the settings to be sane
	if ( last < first )
		last = first;

	for ( int i = first; i <= last; i++ )
	{ //try ports in the range (default 5190-5199)
		if( success = ( m_tcpServer->listen( QHostAddress::Any, i ) ) )
		{
			m_port = i;
			break;
		}
	}
	if ( !success )
	{ //uhoh... what do we do? FIXME: maybe tell the user too many filetransfers
		kDebug(OSCAR_RAW_DEBUG) << "listening failed. abandoning";
		emit transferError( KIO::ERR_COULD_NOT_LISTEN, QString::number( last ) );
		setSuccess(false);
		return false;
	}

	kDebug(OSCAR_RAW_DEBUG) << "listening for connections on port " << m_port;
	m_state = Listening;
	return true;
}

void FileTransferTask::sendReq()
{
	//if we're not using a proxy we need a working serversocket
	if (!( m_proxy || listen() ))
		return;

	Buffer b;
	b.addString( m_oftRendezvous.cookie );

	//set up a message for sendmessagetask
	Oscar::Message msg = makeFTMsg();

	//now set the rendezvous info
	msg.setRequestType( 0 );
	msg.setPort( m_port );
	msg.setFileName( m_oftRendezvous.fileName );
	msg.setFileCount( m_oftRendezvous.fileCount );
	msg.setFilesSize( m_oftRendezvous.totalSize );

	if ( m_proxy )
		msg.setProxy( m_ip );

	if ( m_action == Receive )
		msg.setRequestNumber( 2 );
	else if ( m_proxy && (! client()->settings()->fileProxy() ) )
		msg.setRequestNumber( 3 );

	//we're done, send it off!
	emit sendMessage( msg );
}

Oscar::Message FileTransferTask::makeFTMsg()
{
	Oscar::Message msg;
	msg.setMessageType( Oscar::MessageType::File );
	msg.setChannel( 2 ); //rendezvous
	msg.setIcbmCookie( m_oftRendezvous.cookie );
	msg.setReceiver( m_contactName );
	return msg;
}

QString FileTransferTask::parseDescription( const QByteArray &description ) const
{
	QString xmlDesc = QString::fromUtf8( description );
	xmlDesc.replace( QLatin1String( "&gt;" ), QLatin1String( ">" ) );
	xmlDesc.replace( QLatin1String( "&lt;" ), QLatin1String( "<" ) );
	xmlDesc.replace( QLatin1String( "&quot;" ), QLatin1String( "\"" ) );
	xmlDesc.replace( QLatin1String( "&nbsp;" ), QLatin1String( " " ) );
	xmlDesc.replace( QLatin1String( "&amp;" ), QLatin1String( "&" ) );
	
	QDomDocument xmlDocument;
	if ( !xmlDocument.setContent( xmlDesc ) )
	{
		kDebug(OSCAR_RAW_DEBUG) << "Cannot parse description!";
		return QString::fromUtf8( description );
	}
	
	QDomNodeList descList = xmlDocument.elementsByTagName( "DESC" );
	if ( descList.count() == 1 )
		return descList.at( 0 ).toElement().text();
	else
		return QString::fromUtf8( description );
}

#include "filetransfertask.moc"
//kate: space-indent off; tab-width 4; indent-mode csands;

