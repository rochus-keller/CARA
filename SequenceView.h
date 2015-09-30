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

#if !defined(AFX_SEQUENCEVIEW_H__B09334DA_F090_4F2B_9D64_1C21099F9B61__INCLUDED_)
#define AFX_SEQUENCEVIEW_H__B09334DA_F090_4F2B_9D64_1C21099F9B61__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Gui/ListView.h>
#include <Gui/Menu.h>
#include <Spec/Project.h>
#include <Root/ActionHandler.h>


class SequenceView : public Gui::ListView, public Root::Messenger
{
public:
	static Root::Action::CmdStr AddValue;
	static Root::Action::CmdStr RemoveValue;
	static Root::Action::CmdStr ChangeValue;
	static Root::Action::CmdStr ImportValues;
	static Root::Action::CmdStr EditAtts;
	static Root::Action::CmdStr ShowTable;
	static Root::Action::CmdStr ExportValues;
	static Root::Action::CmdStr SetNumber;
	static Root::Action::CmdStr SetChain;
	static Root::Action::CmdStr AddChain;
	static Root::Action::CmdStr RemoveChain;
	static Root::Action::CmdStr RemoveResidue;
	static Root::Action::CmdStr SetNumberFrom;
	static Root::Action::CmdStr ExportChain;

	void refill();
	SequenceView(QWidget*,Spec::Project*);
	virtual ~SequenceView();
	static Gui::Menu* createPopup();
protected:
	bool isValidChain( Root::SymbolString );
	GENERIC_MESSENGER(Gui::ListView)
	void handle( Root::Message& );
	void onCurrentChanged();
private:
	void handleExportChain( Root::Action& );
	void handleSetNumberFrom( Root::Action& );
	void handleRemoveResidue( Root::Action& );
	void handleRemoveChain( Root::Action& );
	void handleAddChain( Root::Action& );
	void handleSetChain( Root::Action& );
	void handleSetNumber( Root::Action& );
	void handleExportValues( Root::Action& );
	void handleShowTable( Root::Action& );
	void handleEditAtts( Root::Action& );
	void handleImportValues( Root::Action& );
	void handleChangeValue( Root::Action& );
	void handleRemoveValue( Root::Action& );
	void handleAddValue( Root::Action& );
	FRIEND_ACTION_HANDLER( SequenceView );
	Root::Ref<Spec::Project> d_pro;
};

#endif // !defined(AFX_SEQUENCEVIEW_H__B09334DA_F090_4F2B_9D64_1C21099F9B61__INCLUDED_)
