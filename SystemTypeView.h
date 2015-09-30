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

#if !defined(AFX_SYSTEMTYPEVIEW_H__E2F7F16D_F980_49A8_B18D_CBE34EECF11F__INCLUDED_)
#define AFX_SYSTEMTYPEVIEW_H__E2F7F16D_F980_49A8_B18D_CBE34EECF11F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Gui/ListView.h>
#include <Gui/Menu.h>
#include <Spec/Repository.h>
#include <Root/ActionHandler.h>

class SystemTypeView : public Gui::ListView, public Root::Messenger
{
public:
	static Root::Action::CmdStr AddType;
	static Root::Action::CmdStr RenameType;
	static Root::Action::CmdStr RemoveType;
	static Root::Action::CmdStr EditAtts;
	static Root::Action::CmdStr ShowTable;
	static Root::Action::CmdStr SetModel;
	static Root::Action::CmdStr SetGeneric;
	static Root::Action::CmdStr SetTerminals;
	static Root::Action::CmdStr SetClass;
	static Root::Action::CmdStr SetCands;

	SystemTypeView(QWidget*,Spec::Repository*);
	virtual ~SystemTypeView();
	static Gui::Menu* createPopup();
protected:
	GENERIC_MESSENGER(Gui::ListView)
	void handle( Root::Message& );
	void refill();
	void onCurrentChanged();
private:
	void handleSetCands( Root::Action& );
	void handleSetClass( Root::Action& );
	void handleSetTerminals( Root::Action& );
	void handleSetGeneric( Root::Action& );
	void handleSetModel( Root::Action& );
	void handleShowTable( Root::Action& );
	void handleEditAtts( Root::Action& );
	void handleRenameType( Root::Action& );
	void handleRemoveType( Root::Action& );
	void handleNewType( Root::Action& );
	FRIEND_ACTION_HANDLER( SystemTypeView );
	void buildPopup();
	Root::ExRef<Spec::Repository> d_rep;
};

#endif // !defined(AFX_SYSTEMTYPEVIEW_H__E2F7F16D_F980_49A8_B18D_CBE34EECF11F__INCLUDED_)
