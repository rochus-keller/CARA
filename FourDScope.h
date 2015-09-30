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

#if !defined(AFX_FourDScope_H__BEF0EC37_D637_41A1_A04E_F72AC9EAEA32__INCLUDED_)
#define AFX_FourDScope_H__BEF0EC37_D637_41A1_A04E_F72AC9EAEA32__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Lexi/MainWindow.h>
#include <Root/UndoManager.h>
#include <Root/ActionHandler.h>
#include <Lexi/GlyphWidget.h>
#include <Lexi/FocusManager.h>
#include <Spec/PointSet.h>
#include <StripListGadget.h>
#include <FourDScopeAgent.h>
#include <SpecView/OverviewCtrl.h>

namespace Spec
{
	class SpecViewer;
	using Root::Action;
	class Project;
	class Spectrum;

	class FourDScope : public Lexi::MainWindow
	{
	public:
		static Action::CmdStr ShowList;
		static Action::CmdStr ShowAlignment;
		static Action::CmdStr UnlinkSucc;
		static Action::CmdStr UnlinkPred;
		static Action::CmdStr AssignStrip;
		static Action::CmdStr UnassignStrip;
		static Action::CmdStr SetCandidates;
		static Action::CmdStr SetTolerance;
		static Action::CmdStr ExportPeaklist;
		static Action::CmdStr ExportStripPeaks;
		static Action::CmdStr SetTolHori;
		static Action::CmdStr SetTolVerti;
		static Action::CmdStr SetTolStrip;
		static Action::CmdStr ToMonoScope;
		static Action::CmdStr ToMonoScope2D;
		static Action::CmdStr SyncHori;
		static Action::CmdStr SyncVerti;
		static Action::CmdStr Goto;
		static Action::CmdStr GotoResidue;
		static Action::CmdStr SetWidthFactor;
		static Action::CmdStr Use4DSpec;
		static Action::CmdStr Goto4D;
		static Action::CmdStr SaveColors;

		FourDScope(Root::Agent *, Spectrum*, Project*);
	protected:
		Root::Ref<PointSet> createStripPeaks();
		Root::Ref<PointSet> createPlanePeaks();
		FourDScope(Root::Agent *);
		virtual ~FourDScope();
		void handle(Root::Message&);
		bool askToCloseWindow() const; // Override
	private:
		void handleSaveColors( Action& );
		void handleGoto4D( Action& );
		void handleUse4DSpec( Action& );
		void handleSetWidthFactor( Action& );
		void handleGotoResidue( Action& );
		void handleSyncVerti( Action& );
		void handleSyncHori( Action& );
		void handleGoto( Action& );
		void handleToMonoScope( Action& );
		void handleToMonoScope2D( Action& );
		void handleSetTolStrip( Action& );
		void handleExportStripPeaks( Action& );
		void handleSetTolVerti( Action& );
		void handleSetTolHori( Action& );
		void handleExportPeaklist( Action& );
		void handleSetTolerance( Action& );
		void handleShowList( Action& );
		FRIEND_ACTION_HANDLER( FourDScope );
		void buildMenus();
		void buildViews( bool reuse = false);
	protected:
		virtual void updateCaption();
		Root::Ref<Lexi::FocusManager> d_focus;
		Lexi::GlyphWidget* d_widget;
		Root::Ptr<FourDScopeAgent> d_agent;
		Root::Ref<SpecViewer> d_ov;
		Root::Ref<OverviewCtrl> d_ovCtrl;
		Root::Ref<StripListGadget> d_list;
		bool d_showList;
		bool d_use4D;				// Verwende 4D-Spektrum in Liste
		bool d_goto4D;				// DblClck und Goto zu 4D
	};

}

#endif // !defined(AFX_FourDScope_H__BEF0EC37_D637_41A1_A04E_F72AC9EAEA32__INCLUDED_)
