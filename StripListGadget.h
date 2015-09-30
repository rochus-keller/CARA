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

#if !defined(AFX_STRIPLISTGADGET_H__368D2CE6_6563_4D13_997A_1CEDBC5A786D__INCLUDED_)
#define AFX_STRIPLISTGADGET_H__368D2CE6_6563_4D13_997A_1CEDBC5A786D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Lexi/Interactor.h>
#include <Lexi/LexiListView.h>
#include <Root/ActionHandler.h>
#include <Gui/Menu.h>

struct _StripListView;

namespace Spec
{
	class Project;
	class SpinSystem;
	class Spin;
	class Spectrum;

	class StripListGadget : public Lexi::Interactor
	{
	public:
		static Root::Action::CmdStr ShowLinks;
		static Root::Action::CmdStr CreateLink;
		static Root::Action::CmdStr EatSystem;	// Fresse ein anderes System
		static Root::Action::CmdStr CreateSystem;
		static Root::Action::CmdStr ForceLabel;
		static Root::Action::CmdStr CreateSpin;
		static Root::Action::CmdStr LabelSpin;
		static Root::Action::CmdStr MoveSpin;
		static Root::Action::CmdStr MoveSpinAlias;
		static Root::Action::CmdStr SetSysType;
		static Root::Action::CmdStr Delete;	// System oder Spin
		static Root::Action::CmdStr LinkThis;
		static Root::Action::CmdStr ShowAlignment;
		static Root::Action::CmdStr UnlinkSucc;
		static Root::Action::CmdStr UnlinkPred;
		static Root::Action::CmdStr Assign;	// Spin oder System. Bei Spin Sys angeben
		static Root::Action::CmdStr Unassign;	// Spin oder System
		static Root::Action::CmdStr RunStripper;
		static Root::Action::CmdStr StrictStripMatch;
		static Root::Action::CmdStr UnlabeledStripMatch;
		static Root::Action::CmdStr SetSpinTol;
		static Root::Action::CmdStr SetCandidates;
		static Root::Action::CmdStr EditAtts;	// System oder Spin
		static Root::Action::CmdStr FindSpin;
		static Root::Action::CmdStr GotoOther;
		static Root::Action::CmdStr AcceptLabel;
		static Root::Action::CmdStr ShowTable;
		static Root::Action::CmdStr OpenAll;
		static Root::Action::CmdStr CloseAll;
		static Root::Action::CmdStr LinkParams;

		void showStrip( SpinSystem* );
		SpinSystem* getCandPred() const; // selektierter Link als potentieller Predecessor des Strips
		SpinSystem* getCandSucc() const; // selektierter Link als potentieller Successor des Strips
		SpinSystem* getSelectedStrip();
		Spin* getSelectedSpin() const;
		Spin* getSelectedLink() const;
		StripListGadget( Lexi::Viewport*, Project*, 
			Root::Agent* main = 0, Lexi::Glyph* popup = 0, bool links = false,
			bool border = false );
		bool gotoSpin( Spin* );
		Spectrum* getSpec() const;
		void setSpec( Spectrum* );
		Lexi::ListView* getListView() const;
		static Gui::Menu* createPopup();

		//* Callback Messages
		class ActivateMsg : public Root::UserMessage
		{
		public:
			ActivateMsg() {}
		};
	protected:
		virtual ~StripListGadget();
		void handle( Root::Message& );
		void pick( Lexi::Twips x, Lexi::Twips y, const Lexi::Allocation&, Trace&);
	private:
		void handleLinkParams( Root::Action& );
		void handleCloseAll( Root::Action& );
		void handleOpenAll( Root::Action& );
		void handleShowTable( Root::Action& );
		void handleAcceptLabel( Root::Action& );
		void handleGotoOther( Root::Action& );
		void handleFindSpin( Root::Action& );
		void handleShowLinks( Root::Action& );
		void handleCreateLink( Root::Action& );
		void handleEatSystem( Root::Action& );
		void handleCreateSystem( Root::Action& );
		void handleForceLabel( Root::Action& );
		void handleCreateSpin( Root::Action& );
		void handleLabelSpin( Root::Action& );
		void handleMoveSpin( Root::Action& );
		void handleMoveSpinAlias( Root::Action& );
		void handleSetSysType( Root::Action& );
		void handleDelete( Root::Action& );
		void handleLinkThis( Root::Action& );
		void handleShowAlignment( Root::Action& );
		void handleUnlinkSucc( Root::Action& );
		void handleUnlinkPred( Root::Action& );
		void handleAssign( Root::Action& );
		void handleUnassign( Root::Action& );
		void handleRunStripper( Root::Action& );
		void handleUnlabeledStripMatch( Root::Action& );
		void handleStrictStripMatch( Root::Action& );
		void handleSetSpinTol( Root::Action& );
		void handleSetCandidates( Root::Action& );
		void handleEditAtts( Root::Action& );
		FRIEND_ACTION_HANDLER( StripListGadget );
		_StripListView* d_this;
	};
}

#endif // !defined(AFX_STRIPLISTGADGET_H__368D2CE6_6563_4D13_997A_1CEDBC5A786D__INCLUDED_)
