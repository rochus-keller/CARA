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

#if !defined(AFX_SPECTRUMTYPEVIEW_H__34ADD07F_9DC0_4648_9792_F706519C1AE2__INCLUDED_)
#define AFX_SPECTRUMTYPEVIEW_H__34ADD07F_9DC0_4648_9792_F706519C1AE2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Gui/ListView.h>
#include <Gui/Menu.h>
#include <Spec/Repository.h>
#include <Root/ActionHandler.h>


class SpectrumTypeView : public Gui::ListView, public Root::Messenger
{
public:
	static Root::Action::CmdStr AddType;
	static Root::Action::CmdStr AddLabel;
	static Root::Action::CmdStr RemoveLabel;
	static Root::Action::CmdStr EditOrder;
	static Root::Action::CmdStr RemoveType;
	static Root::Action::CmdStr RenameType;
	static Root::Action::CmdStr DuplicateType;
	static Root::Action::CmdStr ShowPath;
	static Root::Action::CmdStr EditProc;
	static Root::Action::CmdStr EditAtts;
	static Root::Action::CmdStr ShowTable;
	static Root::Action::CmdStr PeakWidth;
	static Root::Action::CmdStr ReverseProc;
	static Root::Action::CmdStr ImportTypes;

	void refill();
	SpectrumTypeView(QWidget*,Spec::Repository*);
	virtual ~SpectrumTypeView();
    static Gui::Menu *createPopup();
protected:
	GENERIC_MESSENGER(Gui::ListView)
	void handle( Root::Message& );
	void onCurrentChanged();
private:
	void handleImportTypes( Root::Action& );
	void handleReverseProc( Root::Action& );
	void handlePeakWidth( Root::Action& );
	void handleShowTable( Root::Action& );
	void handleEditAtts( Root::Action& );
	void handleEditProc( Root::Action& );
	void handleShowPath( Root::Action& );
	void handleDuplicateType( Root::Action& );
	void handleRenameType( Root::Action& );
	void handleRemoveType( Root::Action& );
	void handleEditOrder( Root::Action& );
	void handleRemoveLabel( Root::Action& );
	void handleAddLabel( Root::Action& );
	void handleAddType( Root::Action& );
	FRIEND_ACTION_HANDLER( SpectrumTypeView );
	Root::ExRef<Spec::Repository> d_rep;
};

#endif // !defined(AFX_SPECTRUMTYPEVIEW_H__34ADD07F_9DC0_4648_9792_F706519C1AE2__INCLUDED_)
