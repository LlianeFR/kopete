/*
    Kopete Oscar Protocol
    aimlogintask.h - Handles logging into to the AIM service

    Copyright (c) 2004 Matt Rogers <mattr@kde.org>

    Kopete (c) 2002-2004 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/
#include "aimlogintask.h"

#include <stdlib.h>
#include <kdebug.h>
#include <klocale.h>
#include "connection.h"
#include "oscartypes.h"
#include "oscarutils.h"
#include "transfer.h"

#include "md5.h"

using namespace Oscar;

AimLoginTask::AimLoginTask( Task* parent )
	: Task ( parent )
{
}

AimLoginTask::~AimLoginTask()
{
}

void AimLoginTask::onGo()
{
	//send Snac 17,06
	sendAuthStringRequest();
	//when we have the authKey, login
	connect( this, SIGNAL( haveAuthKey() ), this, SLOT( sendLoginRequest() ) );
}

bool AimLoginTask::forMe( Transfer* transfer ) const
{
	SnacTransfer* st = dynamic_cast<SnacTransfer*>( transfer );

	if (!st)
		return false;

	if ( st && st->snacService() == 0x17 )
	{
		WORD subtype = st->snacSubtype();
		switch ( subtype )
		{
		case 0x0002:
		case 0x0003:
		case 0x0006:
		case 0x0007:
			return true;
			break;
		default:
			return false;
			break;
		}
	}
	return false;
}

const QByteArray& AimLoginTask::cookie() const
{
	return m_cookie;
}

const QString& AimLoginTask::bosHost() const
{
	return m_bosHost;
}

const QString& AimLoginTask::bosPort() const
{
	return m_bosPort;
}

bool AimLoginTask::take( Transfer* transfer )
{
	if ( forMe( transfer ) )
	{
		setTransfer( transfer );
		SnacTransfer* st = dynamic_cast<SnacTransfer*>( transfer );
		if (!st)
			return false;
		
		WORD subtype = st->snacSubtype();
		switch ( subtype )
		{
		case 0x0003:
			handleLoginResponse();
			return true;
			break;
		case 0x0007:
			processAuthStringReply();
			return true;
			break;
		default:
			return false;
			break;
		}
		
		return false;
	}
	return false;
}

void AimLoginTask::sendAuthStringRequest()
{
	kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo 
		<< "SEND CLI_AUTH_REQUEST, sending login request" << endl;

	FLAP f = { 0x02, client()->flapSequence(), 0 };
	SNAC s = { 0x0017, 0x0006, 0x0000, client()->snacSequence() };
		
	Buffer* outbuf = new Buffer;
	outbuf->addTLV(0x0001, client()->userId().length(), client()->userId().latin1() );
	outbuf->addDWord(0x004B0000); // empty TLV 0x004B
	outbuf->addDWord(0x005A0000); // empty TLV 0x005A
	
	Transfer* st = createTransfer( f, s, outbuf );
	send( st );
}

void AimLoginTask::processAuthStringReply()
{
	kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "Got the authorization key" << endl;
	Buffer *inbuf = transfer()->buffer();
	WORD keylen = inbuf->getWord();
	kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "Key length is " << keylen << endl;
	m_authKey.duplicate( inbuf->getBlock(keylen) );
	emit haveAuthKey();
}

void AimLoginTask::sendLoginRequest()
{
	kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo <<  "SEND (CLI_MD5_LOGIN) sending AIM login" << endl;

	FLAP f = { 0x02, client()->flapSequence(), 0 };
	SNAC s = { 0x0017, 0x0002, 0x0000, client()->snacSequence() };
	Buffer *outbuf = new Buffer;
	outbuf->addTLV(0x0001, client()->userId().length(), client()->userId().latin1());

	QByteArray digest( 17 ); //apparently MD5 digests are 16 bytes long
	encodePassword( digest );
	digest[16] = '\0';  //do this so that addTLV sees a NULL-terminator

	outbuf->addTLV(0x0025, 16, digest);
	outbuf->addTLV(0x0003, 0x32, AIM_CLIENTSTRING);
	outbuf->addTLV16(0x0016, AIM_CLIENTID);
	outbuf->addTLV16(0x0017, AIM_MAJOR);
	outbuf->addTLV16(0x0018, AIM_MINOR);
	outbuf->addTLV16(0x0019, AIM_POINT);
	outbuf->addTLV16(0x001a, AIM_BUILD);
	outbuf->addDWord(0x00140004); //TLV type 0x0014, length 0x0004
	outbuf->addDWord(AIM_OTHER); //TLV data for type 0x0014
	outbuf->addTLV(0x000f, 0x0002, AIM_LANG);
	outbuf->addTLV(0x000e, 0x0002, AIM_COUNTRY);

	//if set, old-style buddy lists will not work... you will need to use SSI
	outbuf->addTLV8(0x004a,0x01);

	Transfer *st = createTransfer( f, s, outbuf );
	send( st );
}

void AimLoginTask::handleLoginResponse()
{
	kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "RECV SNAC 0x17, 0x07 - AIM Login Response" << endl;

	SnacTransfer* st = dynamic_cast<SnacTransfer*> ( transfer() );

	if ( !st )
	{
		kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo 
			<< "Could not convert transfer object to type SnacTransfer!!"  << endl;
		setError( -1 , QString::null );
		return;
	}

	QValueList<TLV> tlvList = st->buffer()->getTLVList();

	TLV uin = findTLV( tlvList, 0x0001 );
	if ( uin )
	{
		kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "found TLV(1) [SN], SN=" << QString( uin.data ) << endl;
	}

	TLV err = findTLV( tlvList, 0x0008 );

	if ( err )
	{
		WORD errorNum = ( ( err.data[0] << 8 ) | err.data[1] );

		kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << k_funcinfo << "found TLV(8) [ERROR] error= " <<
			errorNum << endl;

		QString errorReason;
		bool needDisconnect = parseDisconnectCode(errorNum, errorReason);
		if ( needDisconnect )
		{
			setError( errorNum, errorReason );
			return; //if there's an error, we'll need to disconnect anyways
		}
	}

	TLV server = findTLV( tlvList, 0x0005 );
	if ( server )
	{
		QString ip = QString( server.data );
		kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "found TLV(5) [SERVER] " << ip << endl;
		int index = ip.find( ':' );
		m_bosHost = ip.left( index );
		ip.remove( 0 , index+1 ); //get rid of the colon and everything before it
		m_bosPort = ip.left(4); //we only need 4 bytes
		kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "We should reconnect to server '" << m_bosHost <<
			"' on port " << m_bosPort << endl;
	}

	TLV cookie = findTLV( tlvList, 0x0006 );
	if ( cookie )
	{
		kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << "found TLV(6) [COOKIE]" << endl;
		m_cookie.duplicate( cookie.data );
		setSuccess( 0, QString::null );
	}
	tlvList.clear();
}

bool AimLoginTask::parseDisconnectCode( int error, QString& reason )
{
	kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << endl;
	QString acctType = ( client()->isIcq() ? i18n("ICQ") : i18n("AIM"));
	QString acctDescription = ( client()->isIcq() ? "UIN" : "screen name");

	switch ( error )
	{
	case 0x0001:
	{
		if ( client()->isLoggedIn() ) // multiple logins (on same UIN)
		{
			reason = i18n( "You have logged in more than once with the same %1," \
				" account %2 is now disconnected.")
				.arg( acctDescription ).arg( client()->userId() );
		}
		else // error while logging in
		{
			reason = i18n( "Sign on failed because either your %1 or " \
				"password are invalid. Please check your settings for account %2.")
				.arg( acctDescription ).arg( client()->userId() );
			return true;
		}
		break;
	}

	case 0x0002: // Service temporarily unavailable
	case 0x0014: // Reservation map error
	{
		reason = i18n("The %1 service is temporarily unavailable. Please try again later.")
			.arg( acctType );
		break;
	}

	case 0x0004: // Incorrect nick or password, re-enter
	case 0x0005: // Mismatch nick or password, re-enter
	{
		
		reason = i18n("Could not sign on to %1 with account %2 as the " \
			"password was incorrect.").arg( acctType ).arg( client()->userId() );
		
		return true;
		break;
	}

	case 0x0007: // non-existant ICQ#
	case 0x0008: // non-existant ICQ#
	{
		reason = i18n("Could not sign on to %1 with nonexistent account %2.")
			.arg( acctType ).arg( client()->userId() );
		break;
	}

	case 0x0009: // Expired account
	{
		reason = i18n("Sign on to %1 failed because your account %2 expired.")
			.arg( acctType ).arg( client()->userId() );
		break;
	}

	case 0x0011: // Suspended account
	{
		reason = i18n("Sign on to %1 failed because your account %2 is " \
			"currently suspended.").arg( acctType ).arg( client()->userId() );
		break;
	}

	case 0x0015: // too many clients from same IP
	case 0x0016: // too many clients from same IP
	case 0x0017: // too many clients from same IP (reservation)
	{
		reason = i18n("Could not sign on to %1 as there are too many clients" \
			" from the same computer.").arg( acctType );
		break;
	}

	case 0x0018: // rate exceeded (turboing)
	{
		if ( client()->isLoggedIn() )
		{
			reason = i18n("Account %1 was blocked on the %2 server for" \
				" sending messages too quickly." \
				" Wait ten minutes and try again." \
				" If you continue to try, you will" \
				" need to wait even longer.")
				.arg( client()->userId() ).arg( acctType );
		}
		else
		{
			reason = i18n("Account %1 was blocked on the %2 server for" \
				" reconnecting too quickly." \
				" Wait ten minutes and try again." \
				" If you continue to try, you will" \
				" need to wait even longer.")
				.arg( client()->userId() ).arg( acctType) ;
		}
		break;
	}

	case 0x001C:
	{
		reason = i18n("The %1 server thinks the client you are using is " \
			"too old. Please report this as a bug at http://bugs.kde.org")
			.arg( acctType );
		break;
	}

	case 0x0022: // Account suspended because of your age (age < 13)
	{
		reason = i18n("Account %1 was disabled on the %2 server because " \
			"of your age (less than 13).")
		.arg( client()->userId() ).arg( acctType );
		break;
	}

	default:
	{
		if ( !client()->isLoggedIn() )
		{
			reason = i18n("Sign on to %1 with your account %2 failed.")
				.arg( acctType ).arg( client()->userId() );
		}
		break;
	}
	}

	return ( error > 0 );
}

void AimLoginTask::encodePassword( QByteArray& digest ) const
{
	kdDebug(OSCAR_RAW_DEBUG) << k_funcinfo << endl;
	md5_state_t state;
	md5_init( &state );
	md5_append( &state, ( const md5_byte_t* ) m_authKey.data(), m_authKey.size() );
	md5_append( &state, ( const md5_byte_t* ) client()->password().latin1(), client()->password().length() );
	md5_append( &state, ( const md5_byte_t* ) AIM_MD5_STRING, strlen( AIM_MD5_STRING ) );
	md5_finish( &state, ( md5_byte_t* ) digest.data() );
}

//kate: indent-mode csands; tab-width 4;
