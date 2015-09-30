/*
 * Copyright 2000-2015 Rochus Keller <mailto:rkeller@nmr.ch>
 *
 * This file is part of CARA (Computer Aided Resonance Assignment,
 * see <http://cara.nmr.ch/>).
 *
 * CARA is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (GPL) as
 * published by the Free Software Foundation, either version 2 of
 * the License, or (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#if !defined(AFX_MESSAGEVIEW_H__7608F5A7_B62E_43D2_8A7E_251609FC3BA9__INCLUDED_)
#define AFX_MESSAGEVIEW_H__7608F5A7_B62E_43D2_8A7E_251609FC3BA9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <QSplitter>
#include <Gui/Menu.h>
#include <Root/ActionHandler.h>

class QTreeView;
class QTextEdit;
class QModelIndex;

class MessageView : public QSplitter, public Root::Messenger
{
	Q_OBJECT
public:
	static Root::Action::CmdStr Clear;
	static Root::Action::CmdStr SaveTo;

	MessageView(QWidget*);
	virtual ~MessageView();
protected slots:
	void handleSelected(const QModelIndex &);
protected:
	GENERIC_MESSENGER(QSplitter)
	void handle( Root::Message& );
private:
	void handleSaveTo( Root::Action& );
	void handleClear( Root::Action& );
	FRIEND_ACTION_HANDLER( MessageView );
	QTreeView* d_list;
	QTextEdit* d_msg;
};

#endif // !defined(AFX_MESSAGEVIEW_H__7608F5A7_B62E_43D2_8A7E_251609FC3BA9__INCLUDED_)
