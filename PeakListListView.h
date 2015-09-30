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

#if !defined(AFX_PEAKLISTLISTVIEW_H__52587945_E9E0_4FD8_A2EC_0FD52511EF86__INCLUDED_)
#define AFX_PEAKLISTLISTVIEW_H__52587945_E9E0_4FD8_A2EC_0FD52511EF86__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Gui/ListView.h>
#include <Gui/Menu.h>
#include <Spec/Repository.h>
#include <Root/ActionHandler.h>

class AidaCentral;

namespace Spec
{
	class PeakList;
}

class PeakListListView : public Gui::ListView, public Root::Messenger
{
public:
	static Root::Action::CmdStr OpenMonoScope;
	static Root::Action::CmdStr Remove;
	static Root::Action::CmdStr EditAtts;
	static Root::Action::CmdStr ShowTable;

	PeakListListView(QWidget*, AidaCentral*, Spec::Repository*,Spec::Project*);
	virtual ~PeakListListView();
	static Gui::Menu* createPopup();
protected:
	GENERIC_MESSENGER(Gui::ListView)
	void handle( Root::Message& );
	void refill();
	Gui::ListViewItem* addItem( Spec::PeakList* );
	void onCurrentChanged();
private:
	void handleShowTable( Root::Action& );
	void handleEditAtts( Root::Action& );
	void handleRemove( Root::Action& );
	void handleOpenMonoScope( Root::Action& );
	FRIEND_ACTION_HANDLER( PeakListListView );
	Root::ExRef<Spec::Project> d_pro;
	Root::ExRef<Spec::Repository> d_rep;
	Root::Ptr<AidaCentral> d_central;
};

#endif // !defined(AFX_PEAKLISTLISTVIEW_H__52587945_E9E0_4FD8_A2EC_0FD52511EF86__INCLUDED_)
