 /*
    Copyright (c) 2008 by Igor Janssen  <alaves17@gmail.com>

    Kopete    (c) 2008 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
 */

#include "dlgxmppconsole.h"

#include <QDialogButtonBox>
#include <QPushButton>

#include "jabberclient.h"

dlgXMPPConsole::dlgXMPPConsole(JabberClient *client, QWidget *parent):
QDialog(parent)
{
	mClient = client;
	setAttribute(Qt::WA_DeleteOnClose);
	QWidget *widget = new QWidget(this);
	ui.setupUi(widget);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->addWidget(widget);
	setWindowTitle(i18n("XML Console"));
	// Buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
	QPushButton *user1Button = new QPushButton;
	buttonBox->addButton(user1Button, QDialogButtonBox::ActionRole);
	QPushButton *user2Button = new QPushButton;
	buttonBox->addButton(user2Button, QDialogButtonBox::ActionRole);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	mainLayout->addWidget(buttonBox);
	user1Button->setText(i18n("Clear"));
	user2Button->setText(i18n("Send"));
    connect(user1Button, &QAbstractButton::clicked, this, &dlgXMPPConsole::slotClear);
    connect(user2Button, &QAbstractButton::clicked, this, &dlgXMPPConsole::slotSend);
}

dlgXMPPConsole::~dlgXMPPConsole()
{
}

void dlgXMPPConsole::slotIncomingXML(const QString &msg)
{
	ui.brLog->setTextColor(Qt::red);
	ui.brLog->append(msg);
}

void dlgXMPPConsole::slotOutgoingXML(const QString &msg)
{
	ui.brLog->setTextColor(Qt::blue);
	ui.brLog->append(msg);
}

void dlgXMPPConsole::slotSend()
{
	mClient->send(ui.mTextEdit->toPlainText());
}

void dlgXMPPConsole::slotClear()
{
	ui.brLog->clear();
}

