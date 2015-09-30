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

#if !defined(AFX_SPINSYSTEMVIEW_H__678E5895_8BAC_469E_979C_BEF70123FF3B__INCLUDED_)
#define AFX_SPINSYSTEMVIEW_H__678E5895_8BAC_469E_979C_BEF70123FF3B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Gui/ListView.h>
#include <Gui/Menu.h>
#include <Spec/Project.h>
#include <Root/ActionHandler.h>

class AidaCentral;
using Root::Action;

class SpinSystemView : public Gui::ListView, public Root::Messenger
{
public:
	static Action::CmdStr SelectSys;
	static Action::CmdStr SelectAll;
	static Action::CmdStr SelectCur;
	static Action::CmdStr ForceLabel;
	static Action::CmdStr CreateSpin;
	static Action::CmdStr DeleteSpin;
	static Action::CmdStr LabelSpin;
	static Action::CmdStr MoveSpin;
	static Action::CmdStr MoveSpinAlias;
	static Action::CmdStr AssignSpin;
	static Action::CmdStr UnassignSpin;
	static Action::CmdStr AssignSystem;
	static Action::CmdStr UnassignSystem;
	static Action::CmdStr SetSysType;
	static Action::CmdStr SetCandidates;
	static Action::CmdStr RemoveAlias;
	static Action::CmdStr EditSpinAtts;
	static Action::CmdStr EditSysAtts;
	static Root::Action::CmdStr ShowTable;
	static Action::CmdStr SetLoc;
	static Action::CmdStr ClearLoc;

	SpinSystemView(QWidget*,AidaCentral*,Spec::Project*);
	virtual ~SpinSystemView();
	static Gui::Menu* createPopup();
protected:
	void handleMoveSpin( Action& );
	void refill();
	GENERIC_MESSENGER(Gui::ListView)
	void handle( Root::Message& );
	void onCurrentChanged();
private:
	void handleClearLoc( Root::Action& );
	void handleSetLoc( Root::Action& );
	void handleShowTable( Root::Action& );
	void handleEditSysAtts( Root::Action& );
	void handleEditSpinAtts( Root::Action& );
	void handleRemoveAlias( Action& );
	void handleMoveSpinAlias( Action& );
	void handleSetCandidates( Action& );
	void handleSetSysType( Action& );
	void handleUnassignSystem( Action& );
	void handleAssignSystem( Action& );
	void handleUnassignSpin( Action& );
	void handleAssignSpin( Action& );
	void handleLabelSpin( Action& );
	void handleDeleteSpin( Action& );
	void handleCreateSpin( Action& );
	void handleSelectCur( Root::Action& );
	void handleSelectAll( Root::Action& );
	void handleSelectSys( Root::Action& );
	void handleForceLabel( Root::Action& );
	FRIEND_ACTION_HANDLER( SpinSystemView );
	Gui::Menu* d_popLabel;
	Root::Index d_sys;	// Filter (<=0 wenn inaktiv)
	void* d_last;
	Spec::AtomType d_t;
	Root::Ref<Spec::Project> d_pro;
	Root::Ptr<AidaCentral> d_parent;
};

#endif // !defined(AFX_SPINSYSTEMVIEW_H__678E5895_8BAC_469E_979C_BEF70123FF3B__INCLUDED_)
