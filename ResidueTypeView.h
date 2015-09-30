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

#if !defined(AFX_RESIDUETYPEVIEW_H__2968585E_F40F_4DCB_820A_5AF7E134D82A__INCLUDED_)
#define AFX_RESIDUETYPEVIEW_H__2968585E_F40F_4DCB_820A_5AF7E134D82A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Gui/ListView.h>
#include <Gui/Menu.h>
#include <Spec/Repository.h>
#include <Root/ActionHandler.h>

class ResidueTypeView : public Gui::ListView, public Root::Messenger
{
public:
	static Root::Action::CmdStr OpenMolecule;
	static Root::Action::CmdStr NewType;
	static Root::Action::CmdStr SetDev;
	static Root::Action::CmdStr SetMean;
	static Root::Action::CmdStr RemoveType;
	static Root::Action::CmdStr RenameType;
	static Root::Action::CmdStr SetSysType;
	static Root::Action::CmdStr EditAtts;
	static Root::Action::CmdStr ShowTable;
	static Root::Action::CmdStr Reload;
	static Root::Action::CmdStr SetGeneric;
	static Root::Action::CmdStr SetTerminals;
	static Root::Action::CmdStr ImportValues;
	static Root::Action::CmdStr ExportValues;
	static Root::Action::CmdStr ImportTypes;
	static Root::Action::CmdStr AddIsotope;
	static Root::Action::CmdStr RemoveIsotope;

	void refill();
	ResidueTypeView(QWidget*,Spec::Repository*);
	virtual ~ResidueTypeView();
	static Gui::Menu* createPopup();
protected:
	GENERIC_MESSENGER(Gui::ListView)
	void handle( Root::Message& );
	void onCurrentChanged();
	void addItem( Spec::ResidueType* );
	Gui::ListViewItem* findItem( Spec::ResidueType* ) const;
private:
	void handleRemoveIsotope( Root::Action& );
	void handleAddIsotope( Root::Action& );
	void handleImportTypes( Root::Action& );
	void handleExportValues( Root::Action& );
	void handleImportValues( Root::Action& );
	void handleSetTerminals( Root::Action& );
	void handleSetGeneric( Root::Action& );
	void handleReload( Root::Action& );
	void handleShowTable( Root::Action& );
	void handleEditAtts( Root::Action& );
	void handleSetSysType( Root::Action& );
	void handleRenameType( Root::Action& );
	void handleRemoveType( Root::Action& );
	void handleSetMean( Root::Action& );
	void handleSetDev( Root::Action& );
	void handleNewType( Root::Action& );
	void handleOpenMolecule( Root::Action& );
	FRIEND_ACTION_HANDLER( ResidueTypeView );
	void buildPopup();
	Root::Ref<Spec::Repository> d_rep;
};

#endif // !defined(AFX_RESIDUETYPEVIEW_H__2968585E_F40F_4DCB_820A_5AF7E134D82A__INCLUDED_)
