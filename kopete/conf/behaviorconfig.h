/*
    behaviorconfig.h  -  Kopete Look Feel Config

    Kopete    (c) 2002-2003 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef __BEHAVIOR_H
#define __BEHAVIOR_H

#include "configmodule.h"

class QFrame;
class QTabWidget;

class BehaviorConfig_General;
class BehaviorConfig_Chat;
class KopeteAwayConfigUI;

class BehaviorConfig : public ConfigModule
{
	Q_OBJECT

	public:
		BehaviorConfig(QWidget * parent);

		virtual void save();
		virtual void reopen();

	private slots:
		void slotConfigSound();
		void slotShowTrayChanged(bool);

	private:
		QTabWidget* mBehaviorTabCtl;
		BehaviorConfig_General *mPrfsGeneral;
		BehaviorConfig_Chat *mPrfsChat;
		KopeteAwayConfigUI *mAwayConfigUI;
};
#endif
// vim: set noet ts=4 sts=4 sw=4:
