/*
    actiontest.cpp -

    Copyright (c) 2007-2008 by Bertrand Demay     <bertranddemay@gmail.com>

    Kopete    (c) 2002-2008 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include "actiontest.h"
#include "action.h"
#include <QString>
#include <QList>

ActionTest::ActionTest()
{
}

ActionTest::~ActionTest()
{
}

void ActionTest::testAction() 
{
	Action action;
	QVERIFY(action.name().isNull());
	QVERIFY(action.listArgument()->isEmpty());
}


void ActionTest::testAddArgument()
{
	Action action;
	action.setName(QString("action_name"));
	QVERIFY(action.listArgument()->isEmpty());
	action.addArgument(QString("argument_name"),QString("argument_direction"),QString("argument_relatedStateVariable"));
	QVERIFY(action.listArgument()->isEmpty() == false);
	QVERIFY(action.listArgument()->size() == 1);
	Argument argument = action.listArgument()->at(0);
	QCOMPARE(argument.name(),QString("argument_name"));
	QCOMPARE(argument.direction(),QString("argument_direction"));
	QCOMPARE(argument.relatedStateVariable(),QString("argument_relatedStateVariable"));
}

void ActionTest::testSetName()
{
	Action action;
	action.setName(QString("name_action"));
	QCOMPARE(action.name(),QString("name_action"));
}

void ActionTest::testOperatorEquals ()
{
	Action action, action2, action3, action4, action5;

	action.setName(QString("name_test"));
	action.addArgument(QString("argument_name"),QString("argument_direction"),QString("argument_relatedStateVariable"));

	action2.setName(QString("my_test"));

	action3.setName(QString("name_test"));
	action3.addArgument(QString("argument_name"),QString("argument_direction"),QString("argument_relatedStateVariable"));
	action3.addArgument(QString("argument_name_2"),QString("argument_direction_2"),QString("argument_relatedStateVariable_2"));

	action4.setName(QString("name_test"));
	action4.addArgument(QString("argument_name"),QString("argument_direction"),QString("argument_relatedStateVariable"));
	action4.addArgument(QString("argument_name_2"),QString("argument_direction_2"),QString("argument_relatedStateVariable_3"));

	action5.setName(QString("name_test"));
	action5.addArgument(QString("argument_name"),QString("argument_direction"),QString("argument_relatedStateVariable"));
	action5.addArgument(QString("argument_name_2"),QString("argument_direction_2"),QString("argument_relatedStateVariable_2"));
	
	QVERIFY((action == action2) == false);
	QVERIFY((action == action3) == false);
	QVERIFY((action == action4) == false);
	QVERIFY((action == action5) == false);

	QVERIFY((action2 == action) == false);
	QVERIFY((action2 == action3) == false);
	QVERIFY((action2 == action4) == false);
	QVERIFY((action2 == action5) == false);

	QVERIFY((action3 == action) == false);
	QVERIFY((action3 == action2) == false);
	QVERIFY((action3 == action4) == false);
	QVERIFY(action3 == action5);

	QVERIFY((action4 == action) == false);
	QVERIFY((action4 == action2) == false);
	QVERIFY((action4 == action3) == false);
	QVERIFY((action4 == action5) == false);

	QVERIFY((action5 == action) == false);
	QVERIFY((action5 == action2) == false);
	QVERIFY(action5 == action3);
	QVERIFY((action5 == action4) == false);	
}



QTEST_KDEMAIN_CORE(ActionTest)

