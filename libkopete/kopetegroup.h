/*
    kopetegroup.h - Kopete (Meta)Contact Group

    Copyright (c) 2002-2003 by Olivier Goffart       <ogoffart@tiscalinet.be>
    Copyright (c) 2003      by Martijn Klingens      <klingens@kde.org>

    Kopete    (c) 2002-2003 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#ifndef KOPETEGROUP_H
#define KOPETEGROUP_H

#include "kopetenotifydataobject.h"
#include "kopeteplugindataobject.h"
#include "kopetemessage.h"
#include "kopetemessagemanager.h"

#include <qptrlist.h>

class QDomElement;

class KopeteGroupPrivate;

namespace Kopete
{

class Group;
class MetaContact;

typedef QPtrList<Group> GroupList;

/**
 * @author Olivier Goffart
 */
class Group : public PluginDataObject, public NotifyDataObject
{
	Q_PROPERTY( QString displayName READ displayName WRITE setDisplayName )
	Q_PROPERTY( uint groupId READ groupId )
	Q_PROPERTY( bool expanded READ isExpanded WRITE setExpanded )

	Q_OBJECT

public:
	enum GroupType { Normal=0, Temporary, TopLevel };

	/**
	 * \brief Create an empty group
	 *
	 * Note that the constructor will not add the group automatically to the contact list.
	 * Use @ref Kopete::ContactList::addGroup() to add it
	 */
	Group();

	/**
	 * \brief Create a group of the specified type
	 *
	 * Overloaded constructor to create a group of the specified type.
	 */
	Group( const QString &displayName, GroupType type = Normal );
	
	/**
	 * \brief Create a group with the specified internal name
	 *
	 * Overloaded constructor to create a group with the specified internal
	 * name
	 */
	Group( const QString &displayName, const QString& internalName,
			 GroupType type = Normal );

	~Group();

	/**
	 * \brief Return the group's display name
	 *
	 * \return the display name of the group
	 */
	QString displayName() const;

	/**
	 * \brief Rename the group
	 */
	void setDisplayName( const QString &newName );

	/**
	 * \brief Get the group's internal name
	 * \return Return a QString containing the internal name
	 */
	QString internalName() const;
	
	/**
	 * \return the group type
	 */
	GroupType type() const;

	/**
	 * \brief Set the group type
	 */
	void setType( GroupType newType );

	/**
	 * \return the unique id for this group
	 */
	uint groupId() const;

	/**
	 * \return the members of this group
	 */
	QPtrList<MetaContact> members() const;
	
	/**
	 * \return the online members of this group
	 */
	QPtrList<MetaContact> onlineMembers() const;

	/**
	 * @internal
	 * Outputs the group data in XML
	 */
	const QDomElement toXML();

	/**
	 * @internal
	 * Loads the group data from XML
	 */
	bool fromXML( const QDomElement &data );

	/**
	 * \brief Set if the group is expanded.
	 *
	 * This is saved to the xml contactlist file
	 * FIXME: the group should not need to know this
	 */
	void setExpanded( bool expanded );

	/**
	 *
	 * \return true if the group is expanded.
	 * \return false otherwise
	 */
	bool isExpanded() const;

	/**
	 * \return a Kopete::Group pointer to the toplevel group
	 */
	static Group *topLevel();

	/**
	 * \return a Kopete::Group pointer to the temporary group
	 */
	static Group *temporary();
	
public slots:
	/**
	 * Send a message to all contacts in the group
	 */
	void sendMessage();

signals:
	/**
	 * \brief Emitted when the group has been renamed
	 */
	void renamed( Kopete::Group *group , const QString &oldName );

private:
	static Group *s_topLevel;
	static Group *s_temporary;

	KopeteGroupPrivate *d;
	
private slots:
	void sendMessage( Kopete::Message& );
};

}

#endif

// vim: set noet ts=4 sts=4 sw=4:

