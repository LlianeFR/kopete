/*
    gwcontact.h - Kopete GroupWise Protocol

    Copyright (c) 2004      SUSE Linux AG	 	 http://www.suse.com
    
    Based on Testbed   
    Copyright (c) 2003      by Will Stephenson		 <will@stevello.free-online.co.uk>
    
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

#include <qmap.h>
#include "kopetecontact.h"
#include "kopetemessage.h"

#include "gwfield.h"

class KAction;
class KActionCollection;
class KopeteAccount;
class KopeteMessageManager;
class KopeteMetaContact;

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
	 * @param uniqueName The unique identifier for this contact, in GroupWise terms, the DN.
	 * @param parent The KopeteMetaContact this contact is part of.
	 * @param displayName The display name given to this contact by the protocol.
	 * @param objectId The contact's numeric object ID.
	 * @param parentId The ID of this contact's parent (folder).
	 * @param sequence This contact's sequence number (The position it appears in within its parent).
	 */
	GroupWiseContact( KopeteAccount* account, const QString &uniqueName, 
			KopeteMetaContact *parent, 
			const QString &displayName, const int objectId, const int parentId, const int sequence );

    ~GroupWiseContact();

	/** 
	 * Access this contact's KopeteProtocol subclass
	 */
	GroupWiseProtocol * protocol();

	/**
	 * Generates a contact by parsing the supplied field list,
	 *  and adding it to the supplied KopeteAccount and KopeteMetaContact.
	 */
	static GroupWiseContact* contactFromFields( KopeteAccount* account, KopeteMetaContact *parent, 
		const Field::MultiField & contact );
		
	/**
	 * Updates the contact's metadata from the supplied field list
	 */
    void updateDetailsFromFields( const Field::MultiField & details );
	
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

public slots:
	/**
	 * Transmits an outgoing message to the server 
	 * Called when the chat window send button has been pressed
	 * (in response to the relevant KopeteMessageManager signal)
	 */
	void sendMessage( KopeteMessage &message );
	/**
	 * Called when an incoming message arrived
	 * This displays it in the chatwindow
	 */
	void receivedMessage( const QString &message );

protected:
	/**
	 * Update the contact's status and metadata from the supplied fields
	 */
	void updateDetailsFromFields( const Field::FieldList & fields );
	
protected slots:
	/**
	 * Show the settings dialog
	 */
	void showContactSettings();
	/**
	 * Notify the contact that its current KopeteMessageManager was
	 * destroyed - probably by the chatwindow being closed
	 */
	void slotMessageManagerDestroyed();
	
protected:
	KopeteMessageManager* m_msgManager;
	KActionCollection* m_actionCollection;
	
	int m_objectId;
	int m_parentId;
	int m_sequence;
	QString m_dn;
	
	KAction* m_actionPrefs;
};

#endif
