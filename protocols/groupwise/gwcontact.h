/*
    gwcontact.h - Kopete GroupWise Protocol

    Copyright (c) 2004      SUSE Linux AG	 	 http://www.suse.com
    
    Based on Testbed   
    Copyright (c) 2003      by Will Stephenson		 <will@stevello.free-online.co.uk>
    
	Blocking status taken from MSN
    Copyright (c) 2002      by Duncan Mac-Vicar Prett <duncan@kde.org>
    Copyright (c) 2002      by Ryan Cumming           <bodnar42@phalynx.dhs.org>
    Copyright (c) 2002-2003 by Martijn Klingens       <klingens@kde.org>
    Copyright (c) 2002-2004 by Olivier Goffart        <ogoffart@tiscalinet.be>
    
	Kopete    (c) 2002-2003 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU General Public                   *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#ifndef GW_CONTACT_H
#define GW_CONTACT_H

#include <qdict.h>
#include <qmap.h>

#include "kopetecontact.h"
#include "kopetemessage.h"

#include "gwerror.h"
#include "gwfield.h"
#include "gwmessagemanager.h"

class KAction;
class KActionCollection;
class KopeteAccount;
class GroupWiseAccount;
class GroupWiseMessageManager;
class GroupWiseProtocol;
class KopeteMetaContact;

using namespace GroupWise;

/**
@author Will Stephenson
*/
class GroupWiseContact : public KopeteContact
{
	Q_OBJECT
public:
	/** 
	 * Constructor
	 * @param account The GroupWiseAccount this belongs to.
	 * @param uniqueName The userId for this contact.  May be a DN, in which case it will be converted to dotted format for the contactId and stored.
	 * @param parent The KopeteMetaContact this contact is part of.
	 * @param objectId The contact's numeric object ID.
	 * @param parentId The ID of this contact's parent (folder).
	 * @param sequence This contact's sequence number (The position it appears in within its parent).
	 */
	GroupWiseContact( KopeteAccount* account, const QString &uniqueName, 
			KopeteMetaContact *parent, 
			const int objectId, const int parentId, const int sequence );

    ~GroupWiseContact();

	/** 
	 * Access this contact's KopeteAccount subclass
	 */
	GroupWiseAccount * account();

	/** 
	 * Access this contact's KopeteProtocol subclass
	 */
	GroupWiseProtocol * protocol();

	/**
	 * Get the contact's DN (used for communications with the server, not the contactId )
	 */
	QString dn() const;
	
	/**
	 * Update the contact's status and metadata from the supplied fields
	 */
	void updateDetails( const GroupWise::ContactDetails & details );
	
	virtual bool isReachable();
	/**
	 * Serialize the contact's data into a key-value map
	 * suitable for writing to a file
	 */
    virtual void serialize(QMap< QString, QString >& serializedData,
			QMap< QString, QString >& addressBookData);
	/**
	 * Return the actions for this contact
	 */
	virtual QPtrList<KAction> *customContextMenuActions();
	
	/**
	 * Returns a KopeteMessageManager associated with this contact
	 */
	virtual KopeteMessageManager *manager( bool canCreate = false );

	/**
	 * Access the contact's server properties
	 */
	QMap< QString, QString > serverProperties();
	
	// CONTACT LIST MANAGEMENT FUNCTIONS
	/**
	 *  These functions model the server side contact list structure enough to allow Kopete to manipulate it correctly
	 *  In GroupWise, a contactlist is composed of folders, containing contacts.  But the contacts don't record which 
	 *  folders they are in.  Instead, each contact entry represents an instance of that contact within the list.  
	 *  In Kopete's model, this looks like duplicate contacts (illegal), so instead we have unique contacts, 
	 *  each (by way of its metacontact) knowing membership of potentially >1 KopeteGroups.  Contacts contain a list of the 
	 *  server side list instances.  Contact list management operations affect this list, which is updated during every
	 *  operation.  Having this list allows us to update the server side contact list and keep changes synchronised across 
	 *  different clients.
	 *  The list is volatile - it is not stored in stable storage, but is purged on disconnect and recreated at login.
	 */
	/**
	 * Add an instance to this contact
	 */
	void addCLInstance( const ContactListInstance & );
	/**
	 * Remove an instance from this contact
	 */
	void removeCLInstance( const int objectId );
	/** 
	 * See if this contact contains an instance with this ID
	 */
	bool hasCLObjectId( const int objectId ) const;
	/**
	 * Get a list of all this contact's instances
	 */
	CLInstanceList instances() const;
	/**
	 * Remove all this contact's contact list instances, called on disconnect so that a clean list is formed on reconnect.
	 */
	void purgeCLInstances();

	/** 
	 * Updates this contact's group membership and display name on the server
	 */
	void syncGroups();
	/**
	 * Updates this contact's online status, including blocking status
	 */
	void setOnlineStatus(const KopeteOnlineStatus& status);
	/**
	 * Are this contact's chats being administratively logged?
	 */
	bool archiving();

public slots:
	/**
	 * Transmits an outgoing message to the server 
	 * Called when the chat window send button has been pressed
	 * (in response to the relevant KopeteMessageManager signal)
	 */
	void sendMessage( KopeteMessage &message );
	/**
	 * Delete this contact on the server
	 */
	virtual void slotDeleteContact();
	/**
	 * Receive notification that an instance of this contact on the server was deleted
	 * If all the instance of the contact are deleted, the contact will delete itself with deleteLater()
	 */
	void receiveContactDeleted( const ContactItem & );
	/**
	 * Called when the call to rename the contact on the server has completed
	 */
	void slotRenamedOnServer();
	
protected:
	// debug function to see what message managers we have on the server
	void dumpManagers();
protected slots:
	/**
	 * Show the contact's properties
	 */
	void slotUserInfo();
	/**
	 * Block or unblock the contact, toggle its current blocking state
	 */
	void slotBlock();
	/**
	 * Receive notification that this contact's privacy setting changed - update status
	 */
	void receivePrivacyChanged( const QString &, bool );
protected:
	KActionCollection* m_actionCollection;
	
	int m_objectId;
	int m_parentId;
	int m_sequence;
	QString m_dn;
	QString m_displayName;
	KAction* m_actionPrefs;
	KAction *m_actionBlock;
	// all the message managers that this contact is currently chatting via
	GroupWiseMessageManager::Dict m_msgManagers;
	// a list of all the instances that this contact appears in the server side contact list
	CLInstanceList m_instances;
	// Novell Messenger Properties, as received by the server.  
	// Unfortunately we don't the domain of the set of keys, so they are not easily mappable to KopeteContactProperties
	QMap< QString, QString > m_serverProperties;
	bool m_archiving;
	// HACK: flag used to differentiate between 'all contact list instances gone while we are moving on the server' 
	// and 'all contact list instances gone because we wanted to delete them all'
	bool m_deleting;
};

#endif
