/*
    kopeteidentitymanager.h - Kopete Identity Manager

    Copyright (c) 2007      by Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>

    Kopete    (c) 2002-2007 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#ifndef __kopeteidentitymanager_h__
#define __kopeteidentitymanager_h__

#include <QObject>
#include "kopete_export.h"
#include "kopeteonlinestatus.h"


namespace Kopete 
{

class Identity;

/**
 * IdentityManager manages all defined identities in Kopete. You can
 * query them and globally set them all online or offline from here.
 *
 * IdentityManager is a singleton, you may uses it with @ref IdentityManager::self()
 *
 * @author Gustavo Pichorim Boiko <gustavo.boiko\@kdemail.net>
 */
class KOPETE_EXPORT IdentityManager : public QObject
{
	Q_OBJECT

public:
	/**
	 * \brief Retrieve the instance of IdentityManager.
	 *
	 * The identity manager is a singleton class of which only a single
	 * instance will exist. If no manager exists yet this function will
	 * create one for you.
	 *
	 * \return the instance of the IdentityManager
	 */
	static IdentityManager* self();

	~IdentityManager();

	/**
	 * \brief Retrieve the list of identities
	 * \return a list of all the identities
	 */
	const QList<Identity *> & identities() const;

	/**
	 * \brief Return the identity asked
	 * \param identityId is the ID for the identity
	 * \return the Identity object found or NULL if no identity was found
	 */
	Identity* findIdentity( const QString &identityId );

	/**
	 * \brief Delete the identity and clean the config data
	 *
	 * This will mostly be called when no account is assigned to an identity
	 */
	void removeIdentity( Identity *identity );

	/**
	 * @brief Register the identity.
	 *
	 * This adds the identity in the manager's identity list.
	 * It will check no identities already exist with the same ID, if any, the identity is deleted. and not added
	 *
	 * @return @p identity, or 0L if the identity was deleted because id collision
	 */
	Identity *registerIdentity( Identity *identity );

public slots:
	
	/**
	 * @brief Set all identities a status in the specified category
	 *
	 * @param category is one of the Kopete::OnlineStatusManager::Categories
	 * @param awayMessage is the new away message
	 * @param flags is a bitmask of SetOnlineStatusFlag
	 */
	void setOnlineStatus( /*Kopete::OnlineStatusManager::Categories*/ uint category,
						  const QString& awayMessage = QString::null, uint flags=0);

	/**
	 * \internal
	 * Save the identity data to KConfig
	 */
	void save();

	/**
	 * \internal
	 * Load the identity data from KConfig
	 */
	void load();

signals:
	void identityOnlineStatusChanged(Kopete::Identity *identity,
		const Kopete::OnlineStatus &oldStatus, const Kopete::OnlineStatus &newStatus);

private:
	/**
	 * Private constructor, because we're a singleton
	 */
	IdentityManager();

private slots:
	void slotIdentityOnlineStatusChanged(Kopete::Identity *i,
		const Kopete::OnlineStatus &oldStatus, const Kopete::OnlineStatus &newStatus);

	/**
	 * \internal
	 * Unregister the identity.
	 */
	void unregisterIdentity( const Kopete::Identity *identity );

private:
	static IdentityManager *s_self;
	class Private;
	Private *d;
};

} //END namespace Kopete


#endif


