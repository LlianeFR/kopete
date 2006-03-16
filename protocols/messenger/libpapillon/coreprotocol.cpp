/*
   coreprotocol.cpp - Messenger core protocol for Papillon 

   Copyright (c) 2006 by Michaël Larouche <michael.larouche@kdemail.net>

   Based on code copyright (c) 2004 SuSE Linux AG <http://www.suse.com>
   Based on Iris, Copyright (C) 2003  Justin Karneges

   *************************************************************************
   *                                                                       *
   * This library is free software; you can redistribute it and/or         *
   * modify it under the terms of the GNU Lesser General Public            *
   * License as published by the Free Software Foundation; either          *
   * version 2 of the License, or (at your option) any later version.      *
   *                                                                       *
   *************************************************************************
*/
#include "coreprotocol.h"

// Qt includes
#include <QDebug>
#include <QDataStream>
#include <QTextStream>
#include <QLatin1String>

// Papillon includes
#include "transfer.h"

namespace Papillon
{

class CoreProtocol::Private
{
public:
	Private()
	 : inTransfer(0)
	{}
	
	// buffer containing unprocessed bytes we received
	QByteArray in;
	int error;
	// the transfer that is being received
	Transfer *inTransfer;
	// represents the protocol's overall state
	int state;
	// Represend the length of payload data to read.
	int payloadLength;
};

CoreProtocol::CoreProtocol() : QObject(), d(new Private)
{}

CoreProtocol::~CoreProtocol()
{
	delete d;
}

int CoreProtocol::state()
{
	return d->state;
}

void CoreProtocol::addIncomingData(const QByteArray &incomingBytes )
{
	// store locally
	int oldsize = d->in.size();
	d->in.resize( oldsize + incomingBytes.size() );
	memcpy( d->in.data() + oldsize, incomingBytes.data(), incomingBytes.size() );
	d->state = Available;

	// convert every event in the chunk to a Transfer, signalling it back to the clientstream
	int parsedBytes = 0;
	int transferCount = 0;
	// while there is data left in the input buffer, and we are able to parse something out of it
	while ( d->in.size() && ( parsedBytes = rawToTransfer( d->in ) ) )
	{
		transferCount++;
		int size =  d->in.size();
		if ( parsedBytes < size )
		{
			// copy the unparsed bytes into a new qbytearray and replace d->in with that
			QByteArray remainder( size - parsedBytes );
			memcpy( remainder.data(), d->in.data() + parsedBytes, remainder.size() );
			d->in = remainder;
		}
		else
		{
			d->in.truncate( 0 );
		}
	}

}

Transfer *CoreProtocol::incomingTransfer()
{
	if ( d->state == Available )
	{
		d->state = NoData;
		return d->inTransfer;
	}
	else
	{
		return 0;
	}
}

void CoreProtocol::outgoingTransfer(Transfer *outgoing)
{
	emit outgoingData( outgoing->toRawCommand() );
	// Clear the transfer.
	delete outgoing;
}

int CoreProtocol::rawToTransfer(const QByteArray &raw)
{
	uint bytesParsed = 0;

	if ( raw.size() < 4 )
	{
		d->state = NeedMore;
		return bytesParsed;
	}	
	
	QByteArray tempRaw = raw;
	QDataStream din( &tempRaw, IO_ReadOnly );
	
	// look at first four bytes and decide what to do with the chunk
	if ( okToProceed( din ) )
	{
		if(d->state != WaitForPayload && d->state != NeedMore)
		{
			QTextStream lineStream(tempRaw);
			
			QString parsedLine = lineStream.readLine();
			qDebug() << PAPILLON_FUNCINFO << parsedLine;
			
			QStringList commandList = parsedLine.split(" ");
			
			QString command;
			QStringList arguments;
			
			Transfer::TransferType transferType;
			bool dummy, isNumber;
			int trId = 0, payloadLength = 0;
			
			command = commandList.at(0);
			
			// Determine the transfer type.
			if(isPayloadCommand(command))
			{
				transferType |= Transfer::PayloadTransfer;
				// Remove the last parameter from the command list and set the payload length.
				// So it will not be in the arguments.
				payloadLength = commandList.takeLast().toUInt(&dummy);
			}
			
			// Check for a transaction ID.
			trId = commandList[1].toUInt(&isNumber);
			if(isNumber)
				transferType |= Transfer::TransactionTransfer;
			
			// Begin at the third command arguments if we have a transaction ID.
			int beginAt = isNumber ? 2 : 1;
			// Fill the arguments.
			for(int i = beginAt; i < commandList.size(); ++i)
			{
				arguments << commandList[i];
			}
			
			Transfer *receivedTransfer = new Transfer(transferType);
			receivedTransfer->setCommand(command);
			receivedTransfer->setArguments(arguments);
			
			if(isNumber)
				receivedTransfer->setTransactionId( QString::number(trId) );
			
			d->inTransfer = receivedTransfer;
			
			if(payloadLength > 0)
			{
				d->payloadLength = payloadLength;
				d->state = WaitForPayload;
			}
			else
			{
				d->state = Available;
				emit incomingData();
			}
			
			bytesParsed = parsedLine.size() + 2; // 2 is to add \r\n to the size which was trimmed.
		}
		else if(d->state == WaitForPayload)
		{
			if(raw.size() < d->payloadLength)
			{
				d->state = NeedMore;
				return bytesParsed;
			}
			
			QByteArray payloadData;
			payloadData.reserve(d->payloadLength);
			din.readRawBytes(payloadData.data(), d->payloadLength);
			
			d->inTransfer->setPayloadData(payloadData);
			
			d->state = Available;
			emit incomingData();
			
			bytesParsed = d->payloadLength;
		}
	}
	return bytesParsed;
}

void CoreProtocol::reset()
{
	d->in.resize( 0 );
}

bool CoreProtocol::okToProceed(const QDataStream &din)
{
	if( din.atEnd() )
	{
		d->state = NeedMore;
		return false;
	}
	else
		return true;
}

bool CoreProtocol::isPayloadCommand(const QString &command)
{
	if( command == QLatin1String("ADL") ||
	    command == QLatin1String("MSG") ||
	    command == QLatin1String("QRY") ||
	    command == QLatin1String("RML") ||
	    command == QLatin1String("UBX") ||
	    command == QLatin1String("UBN") ||
	    command == QLatin1String("UUN") ||
	    command == QLatin1String("UUX")
	  )
		return true;
	else
		return false;
}

}

#include "coreprotocol.moc"
//kate: indent-mode csands; tab-width 4;
