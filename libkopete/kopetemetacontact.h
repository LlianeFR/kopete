/*
    kopetemetacontact.h - Kopete Meta Contact

    Copyright (c) 2002 by Martijn Klingens       <klingens@kde.org>
    Copyright (c) 2002 by Duncan Mac-Vicar Prett <duncan@kde.org>

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

#ifndef __kopetemetacontact_h__
#define __kopetemetacontact_h__

#include <qobject.h>
#include <qptrlist.h>

#include "kopetecontact.h"
#include "kopeteplugindataobject.h"

class QDomNode;

class KURL;

class KopeteGroup;

struct KopeteMetaContactPrivate;

/**
 * @author Martijn Klingens <klingens@kde.org>
 */
class KopeteMetaContact : public KopetePluginDataObject
{
	Q_OBJECT

public:
	KopeteMetaContact();
	~KopeteMetaContact();

	/**
	 * Retrieve the list of contacts that are part of the meta contact
	 */
	QPtrList<KopeteContact> contacts() const;

	/**
	 * Add contact to the meta contact
	 */
	void addContact( KopeteContact *c );

	/**
	 * Find the KopeteContact to a given contact. If contact
	 * is not found, a null pointer is returned.
	 */
	KopeteContact *findContact( const QString &protocolId, const QString &accountId, const QString &contactId );
	/**
	 * The name of the icon associated with the contact's status
	 */
	virtual QString statusIcon() const;

	/**
	 * The status-string of the contact
	 */
	QString statusString() const;

	/**
	 * Returns whether this contact can be reached online for at least one
	 * protocol. Protocols are processed in loading order.
	 * FIXME: Make that user preference order!
	 * FIXME: Make that an enum, because status can be unknown for certain
	 *        protocols
	 */
	bool isOnline() const;

	/**
	 * Returns weather this contact can accept files
	 * @return True if the user is online with a file capable protocol, false otherwise
	 */
	bool canAcceptFiles() const;

	/**
	 * Contact's status
	 */
	enum OnlineStatus { Unknown, Offline, Online, Away };

	/**
	 * Contact's idle state
	 */
	enum IdleState { Unspecified, Idle, Active };

	/**
	 * Return more fine-grained status.
	 * Online means at least one sub-contact is online, away means at least
	 * one is away, but nobody is online and offline speaks for itself
	 */
	OnlineStatus status() const;

	/**
	 * Like isOnline, but returns true even if the contact is not online, but
	 * can be reached trough offline-messages.
	 * FIXME: Here too, use preference order, not append order!
	 * FIXME: Here too an enum.
	 */
	bool isReachable() const;

	/**
	 * Unspecified means that none of the contacts' protocols advertise idle time
	 * Idle means that at least one sub-contact is idle
	 * Active means that no subcontacts are idle and at least one are active
	 */
	IdleState idleState() const;

	/**
	 * Get/set the display name
	 */
	QString displayName() const;
	void setDisplayName( const QString &name );

	/**
	 * Get/set the tracking of contact names
	 * The MetaContact will adjust its displayName() everytime the contact inside changes its name
	 * This should only work for MCs with exactly ONE contact inside in order to not confuse users
	 * (think about 4 subcontacts and what happens if one changes nickname...)
	 */
	bool trackChildNameChanges() const;
	void setTrackChildNameChanges( bool track );

	/**
	 * The groups the contact is stored in
	 */
	KopeteGroupList groups() const;

	/**
	 * Return a XML representation of the metacontact
	 */
	QString toXML();

	/**
	 * Creates a metacontact from XML
	 * Return value of false indicated that
	 * creation failed and this contact should be
	 * discarded.
	 */
	bool fromXML( const QDomNode& cnode );

	/**
	 * Move a contact from one group to another.
	 */
	void moveToGroup( KopeteGroup *from, KopeteGroup *to );

	/**
	 * Remove a contact from one group
	 */
	void removeFromGroup( KopeteGroup *from );

	/**
	 * Add a contact to another group.
	 */
	void addToGroup( KopeteGroup *to );

	/**
	 * Temporary contacts will not be sarialized
	 */
	bool isTemporary() const;
	void setTemporary( bool b = true ,KopeteGroup *group = 0L );

	/**
	 * When true, the meta-contact needs to be serialized
	 * and the previous serialize can't be used anymore
	 */
	bool isDirty() const;
	/**
	 * Plugins should set the metacontact to dirty
	 * as a save-me request
	 */
	void setDirty( bool b = true );

	/**
	 * Return true if the contact is shown at toplevel
	 */
	bool isTopLevel();

	/**
	 * add or remove from top-level
	 */
	void setTopLevel( bool b = true );

	/**
	 * remove the contact from this metacontact
	 * set 'deleted' to true if the KopeteContact is already deleted
	 */
	void removeContact( KopeteContact *c , bool deleted = false );


	/**
	 * Get or set a field for the KDE address book backend. Fields not
	 * registered during the call to KopetePlugin::addressBookFields()
	 * cannot be altered!
	 *
	 * @param app refers to the application id in the libkabc database.
	 * This should be a standardized format to make sense in the address
	 * book in the first place - if you could use "kopete" as application
	 * then probably you should use the plugin data API instead of the
	 * address book fields.
	 *
	 * FIXME: In the code the requirement that fields are registered first
	 *        is already lifted, but the API needs some review before we
	 *        can remove it here too.
	 *        Probably it requires once more some rewrites to get it working
	 *        properly :( - Martijn
	 */
	QString addressBookField( KopetePlugin *p, const QString &app, const QString &key ) const;
	void setAddressBookField( KopetePlugin *p, const QString &app, const QString &key, const QString &value );

public slots:
	/**
	 * Contact another user.
	 * Depending on the config settings, call sendMessage() or
	 * startChat()
	 */
	void execute();

	/**
	 * Send a single message, classic ICQ style.
	 * The actual sending is done by the KopeteContact, but the meta contact
	 * does the GUI side of things.
	 * This is a slot to allow being called easily from e.g. a GUI.
	 */
	void sendMessage();

	/**
	 * Like sendMessage, but this time a full-blown chat will be opened.
	 * Most protocols can't distinguish between the two and are either
	 * completely session based like MSN or completely message based like
	 * ICQ the only true difference is the GUI shown to the user.
	 */
	void startChat();

	/**
	 * This is the KopeteMetaContact level slot for sending files. It may be called through the
	 * "Send File" entry in the GUI, or over DCOP. If the function is called through the GUI,
	 * no parameters are sent and they assume default values. This slot calls the slotSendFile
	 * with identical params of the highest ranked contact capable of sending files (if any)
	 *
	 * @param sourceURL The actual KURL of the file you are sending
	 * @param altFileName (Optional) An alternate name for the file - what the reciever will see
	 * @param fileSize (Optional) Size of the file being sent. Used when sending a nondeterminate
	 *                file size (such as over a socket)
	 *
	 */
	void sendFile( const KURL &sourceURL, const QString &altFileName = QString::null,
		unsigned long fileSize = 0L );

signals:
	/**
	 * The contact's online status changed
	 * Do *NOT* emit this signal directly, unless you also update the
	 * cache m_onlineStatus value! In all other case, just call
	 * updateOnlineStatus() instead.
	 */
	void onlineStatusChanged( KopeteMetaContact *contact, KopeteMetaContact::OnlineStatus status );

	/**
	 * A contact's online status changed
	 * this signal differs from onlineStatusChanged because a contact can
	 * change his status without changing MetaContact status
	 */
	void contactStatusChanged( KopeteContact *contact, const KopeteOnlineStatus &status );

	/**
	 * The meta contact's display name changed
	 */
	void displayNameChanged( const QString &oldName, const QString &newName );

	/**
	 * The contact was moved
	 */
	void movedToGroup( KopeteMetaContact *contact, KopeteGroup *from, KopeteGroup *to );

	/**
	 * The contact was removed from group
	 */
	void removedFromGroup( KopeteMetaContact *contact, KopeteGroup *group );

	/**
	 * The contact was added to another group
	 */
	void addedToGroup( KopeteMetaContact *contact, KopeteGroup *to );

	/**
	 * This signal is emmited when a contact is added to this metacontact
	 */
	void contactAdded( KopeteContact *c );

	/**
	 * This signal is emmited when a contact is removed from this metacontact
	 */
	void contactRemoved( KopeteContact *c );

	/**
	 * This metaContact is going to be saved to the contactlist. Plugin should
	 * connect to this signal to update data with setPluginData()
	 */
	void aboutToSave(KopeteMetaContact*);

	/**
		* The metacontact's idle status changed.  KopeteMetaContactLVI should
		* connect to this signal
		*/
	void idleStateChanged( KopeteMetaContact *contact, KopeteMetaContact::IdleState newState );

	/**
	 * One of the subcontacts' idle status has changed.  As with online status,
	 * this can occur without the metacontact changing idle state
	 */
	void contactIdleStateChanged( KopeteContact *contact, KopeteContact::IdleState newState );

private slots:
	/**
	 * Update the contact's online status and emit onlineStatusChanged
	 * when appropriate
	 */
	void updateOnlineStatus();

	/**
	  * Update the contact's idle status and emit idleStateChanged when
	  * appropriate
	  */
	void updateIdleState();

	/**
	 * One of the child contact's online status changed
	 */
	void slotContactStatusChanged( KopeteContact *c, const KopeteOnlineStatus &status );

	/**
	 * One of the child contact's display names changed
	 */
	void slotContactNameChanged( const QString &oldName, const QString &newName );

	/**
	 * A child contact was deleted, remove it from the list, if it's still
	 * there
	 */
	void slotContactDestroyed( KopeteContact* );

	/**
	 * If a plugin is loaded, maybe dada about this plugins are already cached in the metacontact
	 */
	void slotPluginLoaded(KopetePlugin *p);

	/**
	 * One of the child contact's idle state changed
	 */
	void slotContactIdleStateChanged( KopeteContact *c,
		KopeteContact::IdleState s );

private:
	KopeteContact *preferredContact();

	KopeteMetaContactPrivate *d;
};

#endif

// vim: set noet ts=4 sts=4 sw=4:

