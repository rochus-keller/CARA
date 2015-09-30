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

#if !defined(AFX_SPINLINKVIEW_H__EC157F4D_52D0_41E6_BF62_DB65A0254591__INCLUDED_)
#define AFX_SPINLINKVIEW_H__EC157F4D_52D0_41E6_BF62_DB65A0254591__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Gui/ListView.h>
#include <Gui/Menu.h>
#include <Spec/Project.h>
#include <Root/ActionHandler.h>

class AidaCentral;
using Root::Action;

class SpinLinkListView : public Gui::ListView, public Root::Messenger
{
public:
	static Action::CmdStr ShowTable;
	static Action::CmdStr EditAtts;
	static Action::CmdStr Delete;
	static Action::CmdStr Create;

	SpinLinkListView(QWidget*,AidaCentral*,Spec::Project*);
	virtual ~SpinLinkListView();
	static Gui::Menu* createPopup();
protected:
	void handleMoveSpin( Action& );
	void refill();
	GENERIC_MESSENGER(Gui::ListView)
	void handle( Root::Message& );
	void onCurrentChanged();
private:
	void handleCreate( Root::Action& );
	void handleDelete( Root::Action& );
	void handleShowTable( Root::Action& );
	void handleEditAtts( Root::Action& );
	FRIEND_ACTION_HANDLER( SpinLinkListView );
	Root::Ptr<AidaCentral> d_parent;
	Root::ExRef<Spec::Project> d_pro;
};

#endif // !defined(AFX_SPINLINKVIEW_H__EC157F4D_52D0_41E6_BF62_DB65A0254591__INCLUDED_)
