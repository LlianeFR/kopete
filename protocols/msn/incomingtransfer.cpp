/*
    incomingtransfer.cpp - msn p2p protocol

    Copyright (c) 2003-2005 by Olivier Goffart        <ogoffart@ kde.org>
    Copyright (c) 2005      by Gregg Edghill          <gregg.edghill@gmail.com>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "incomingtransfer.h"
using P2P::TransferContext;
using P2P::IncomingTransfer;
using P2P::Message;

// Kde includes
#include <kbufferedsocket.h>
#include <kdebug.h>
#include <klocale.h>
#include <kserversocket.h>
#include <kstandarddirs.h>
#include <ktempfile.h>
using namespace KNetwork;

// Qt includes
#include <qfile.h>
#include <qregexp.h>

// Kopete includes
#include <kopetetransfermanager.h>

IncomingTransfer::IncomingTransfer(const QString& from, P2P::Dispatcher *dispatcher, Q_UINT32 sessionId)
: TransferContext(dispatcher)
{
	m_direction = P2P::Incoming;
	m_listener  = 0l;
	m_recipient = from;
	m_sessionId = sessionId;
}

IncomingTransfer::~IncomingTransfer()
{
	kdDebug(14140) << k_funcinfo << endl;
	if(m_listener)
	{
		delete m_listener;
		m_listener = 0l;
	}

	if(m_socket)
	{
		delete m_socket;
		m_socket = 0l;
	}
}

void IncomingTransfer::acknowledged()
{
	kdDebug(14140) << k_funcinfo << endl;
	
	switch(m_state)
	{
		case Invitation:
				// NOTE UDI: base identifier acknowledge message, ignore.
				//      UDI: 200 OK message should follow.
				if(m_type == File){
					// FT: 200 OK acknowledged message.
					// Direct connection invitation should follow.
					m_state = Negotiation;
				}
			break;

		case Negotiation:
				// 200 OK acknowledge message.
			break;

		case DataTransfer:
			break;
			
		case Finished:
			// UDI: Bye acknowledge message.
			m_dispatcher->detach(this);
			break;
	}
}

void IncomingTransfer::processMessage(const Message& message)
{
	if(m_file && (message.header.flag == 0x20 || message.header.flag == 0x01000030))
	{
		// UserDisplayIcon data or File data is in this message.
		// Write the recieved data to the file.
		kdDebug(14140) << k_funcinfo << QString("Received, %1 bytes").arg(message.header.dataSize) << endl;
		
		m_file->writeBlock(message.body.data(), message.header.dataSize);
		if(m_transfer){
			m_transfer->slotProcessed(message.header.dataOffset + message.header.dataSize);
		}
		
		if((message.header.dataOffset + message.header.dataSize) == message.header.totalDataSize)
		{
			// Transfer is complete.
			if(m_type == UserDisplayIcon){
				m_tempFile->close();
				m_dispatcher->displayIconReceived(m_tempFile, m_object);
				m_tempFile = 0l;
				m_file = 0l;
			}
			else
			{
				m_file->close();
			}

			m_isComplete = true;
			// Send data acknowledge message.
			acknowledge(message);

			if(m_type == UserDisplayIcon)
			{
				m_state = Finished;
				// Send BYE message.
				sendMessage(BYE, "\r\n");
			}
		}
	}
	else if(message.header.dataSize == 4 && message.applicationIdentifier == 1)
	{
		// Data preparation message.
		m_tempFile = new KTempFile(locateLocal("tmp", "msnpicture--"), ".png");
		m_tempFile->setAutoDelete(true);
		m_file = m_tempFile->file();
		m_state = DataTransfer;
		// Send data preparation acknowledge message.
		acknowledge(message);
	}
	else
	{
		QString body =
			QCString(message.body.data(), message.header.dataSize);
		kdDebug(14140) << k_funcinfo << "received, " << body << endl;

		if(body.startsWith("INVITE"))
		{
			// Retrieve some MSNSLP headers used when
			// replying to this INVITE message.
			QRegExp regex(";branch=\\{([0-9A-F\\-]*)\\}\r\n");
			regex.search(body);
			m_branch = regex.cap(1);
			// NOTE Call-ID never changes.
			regex = QRegExp("Call-ID: \\{([0-9A-F\\-]*)\\}\r\n");
			regex.search(body);
			m_callId = regex.cap(1);
			regex = QRegExp("Bridges: ([^\r\n]*)\r\n");
			regex.search(body);
			QString bridges = regex.cap(1);
			// The NetID field is 0 if the Conn-Type is
			// Direct-Connect or Firewall, otherwise, it is
			// a randomly generated number.
			regex = QRegExp("NetID: (\\-?\\d+)\r\n");
			regex.search(body);
			QString netId = regex.cap(1);
			kdDebug(14140) << "net id, " << netId << endl;
			// Connection Types
			// - Direct-Connect
			// - Port-Restrict-NAT
			// - IP-Restrict-NAT
			// - Symmetric-NAT
			// - Firewall
			regex = QRegExp("Conn-Type: ([^\r\n]+)\r\n");
			regex.search(body);
			QString connType = regex.cap(1);

			bool wouldListen = false;
			if(netId.toUInt() == 0 && connType == "Direct-Connect"){
				wouldListen = true;

			}
			else if(connType == "IP-Restrict-NAT"){
				wouldListen = true;
			}

			wouldListen = false; // TODO Direct connection support
			QString content;
			
			if(wouldListen)
			{
				// Create a listening socket for direct file transfer.
				m_listener = new KServerSocket("", "");
				m_listener->setResolutionEnabled(true);
				// Create the callback that will try to accept incoming connections.
				QObject::connect(m_listener, SIGNAL(readyAccept()), SLOT(slotAccept()));
				QObject::connect(m_listener, SIGNAL(gotError(int)), this, SLOT(slotListenError(int)));
				// Listen for incoming connections.
				bool isListening = m_listener->listen(1);
				kdDebug(14140) << k_funcinfo << (isListening ? "listening" : "not listening") << endl;
				kdDebug(14140) << k_funcinfo
					<< "local endpoint, " << m_listener->localAddress().nodeName()
					<< endl;
				
				content = "Bridge: TCPv1\r\n"
					"Listening: true\r\n" +
					QString("Hashed-Nonce: {%1}\r\n").arg(P2P::Uid::createUid()) +
					QString("IPv4Internal-Addrs: %1\r\n").arg(m_listener->localAddress().nodeName())   +
					QString("IPv4Internal-Port: %1\r\n").arg(m_listener->localAddress().serviceName()) +
					"\r\n";
			}
			else
			{
				content =
					"Bridge: TCPv1\r\n"
					"Listening: false\r\n"
					"Hashed-Nonce: {00000000-0000-0000-0000-000000000000}\r\n"
					"\r\n";
			}

			acknowledge(message);

			if(m_transfer)
			{
				// NOTE The sending client can ask for a direct connections
				// if one was established before.
				QFile *destionation = new QFile(m_transfer->destinationURL().path());
				if(!destionation->open(IO_WriteOnly))
				{
					if(m_transfer){
						m_transfer->slotError(KIO::ERR_CANNOT_OPEN_FOR_WRITING, i18n("Cannot open file for writing"));
						m_transfer = 0l;
					}
					
					error();
					return;
				}
			
				m_file = destionation;
			}
			
			m_state = DataTransfer;
			// Send 200 OK message to the sending client.
			sendMessage(OK, content);
		}
		else if(body.startsWith("BYE"))
		{
			m_state = Finished;
			// Send the sending client an acknowledge message.
			acknowledge(message);

			if(m_file && m_transfer)
			{
				if(m_isComplete){
					// The transfer is complete.
					m_transfer->slotComplete();
				}
				else
				{
					// The transfer has been canceled remotely.
					if(m_transfer){
						// Inform the user of the file transfer cancelation.
						m_transfer->slotError(KIO::ERR_ABORTED, i18n("File transfer canceled."));
					}
					// Remove the partially received file.
					m_file->remove();
				}
			}

			// Dispose of this transfer context.
			m_dispatcher->detach(this);
		}
		else if(body.startsWith("MSNSLP/1.0 200 OK"))
		{
			if(m_type == UserDisplayIcon){
				m_state = Negotiation;
				// Acknowledge the 200 OK message.
				acknowledge(message);
			}
		}
	}
}

void IncomingTransfer::slotListenError(int errorCode)
{
	kdDebug(14140) << k_funcinfo << m_listener->errorString() << endl;
}

void IncomingTransfer::slotAccept()
{
	// Try to accept an incoming connection from the sending client.
	m_socket = static_cast<KBufferedSocket*>(m_listener->accept());
	if(!m_socket)
	{
		// NOTE If direct connection fails, the sending
		// client wil transfer the file data through the
		// existing session.
		kdDebug(14140) << k_funcinfo << "Direct connection failed." << endl;
		// Close the listening endpoint.
		m_listener->close();
		return;
	}

	kdDebug(14140) << k_funcinfo << "Direct connection established." << endl;

	// Set the socket to non blocking,
	// enable the ready read signal and disable
	// ready write signal.
	// NOTE readyWrite consumes too much cpu usage.
	m_socket->setBlocking(false);
	m_socket->enableRead(true);
	m_socket->enableWrite(false);

	// Create the callback that will try to read bytes from the accepted socket.
	QObject::connect(m_socket, SIGNAL(readyRead()),   this, SLOT(slotSocketRead()));
	// Create the callback that will try to handle the socket close event.
	QObject::connect(m_socket, SIGNAL(closed()),      this, SLOT(slotSocketClosed()));
	// Create the callback that will try to handle the socket error event.
	QObject::connect(m_socket, SIGNAL(gotError(int)), this, SLOT(slotSocketError(int)));
}

void IncomingTransfer::slotSocketRead()
{
	int available = m_socket->bytesAvailable();
	kdDebug(14140) << k_funcinfo << available << ", bytes available." << endl;
	if(available > 0)
	{
		QByteArray buffer(available);
		m_socket->readBlock(buffer.data(), buffer.size());
		kdDebug(14140) << k_funcinfo << QString(buffer) << endl;

		if(QString(buffer) == "foo"){
			kdDebug(14140) << "Connection Check." << endl;
		}
	}
}

void IncomingTransfer::slotSocketClosed()
{
	kdDebug(14140) << k_funcinfo << endl;
}

void IncomingTransfer::slotSocketError(int errorCode)
{
	kdDebug(14140) << k_funcinfo << endl;
}

#include "incomingtransfer.moc"
