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

#if !defined(AFX_OBJECTDEFVIEW_H__59CD0712_2F16_41C0_BACD_B5F82202D2F1__INCLUDED_)
#define AFX_OBJECTDEFVIEW_H__59CD0712_2F16_41C0_BACD_B5F82202D2F1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Gui/ListView.h>
#include <Gui/Menu.h>
#include <Spec/Repository.h>
#include <Root/ActionHandler.h>

class ObjectDefView : public Gui::ListView, public Root::Messenger
{
public:
	static Root::Action::CmdStr AddAtt;
	static Root::Action::CmdStr RemoveAtt;
	static Root::Action::CmdStr EditAtt;

	ObjectDefView(QWidget*,Spec::ObjectDef*);
	virtual ~ObjectDefView();
    static Gui::Menu *createPopup();
protected:
	GENERIC_MESSENGER(Gui::ListView)
	void handle( Root::Message& );
	void refill();
private:
	void handleEditAtt( Root::Action& );
	void handleRemoveAtt( Root::Action& );
	void handleAddAtt( Root::Action& );
	FRIEND_ACTION_HANDLER( ObjectDefView );
	Root::ExRef<Spec::ObjectDef> d_obj;
};

#endif // !defined(AFX_OBJECTDEFVIEW_H__59CD0712_2F16_41C0_BACD_B5F82202D2F1__INCLUDED_)
