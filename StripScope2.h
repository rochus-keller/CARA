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

#if !defined(AFX_STRIPSCOPE_H__3ADC5F82_3DD2_4CA1_9696_34C8E5B8BF68__INCLUDED_)
#define AFX_STRIPSCOPE_H__3ADC5F82_3DD2_4CA1_9696_34C8E5B8BF68__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Lexi/MainWindow.h>
#include <Root/ActionHandler.h>
#include <Lexi/GlyphWidget.h>
#include <Lexi/FocusManager.h>
#include <SpecView/ResidueParamView.h>
#include <StripScopeAgent2.h>
#include <StripListGadget.h>

namespace Spec
{
	class SpecViewer;
	class ResidueParamView;
	class StripListGadget;
	class Project;
	class Spectrum;
	using Root::Action;

	class StripScope2 : public Lexi::MainWindow
	{
	public:
		static Action::CmdStr ShowRulers;
		static Action::CmdStr SelectRuler;
		static Action::CmdStr SetLaneCount;
		static Action::CmdStr ShowSucc;
		static Action::CmdStr ShowPred;
		static Action::CmdStr ShowAllSucc;
		static Action::CmdStr ShowAllPred;
		static Action::CmdStr ShowAllBest;
		static Action::CmdStr ShowFrag;
		static Action::CmdStr ShowSystem;
		static Action::CmdStr RunStripper;
		static Action::CmdStr StrictStripMatch;
		static Action::CmdStr RunMapper;
		static Action::CmdStr MapGeometric;
		static Action::CmdStr MapExclusive;
		static Action::CmdStr MapExpand;
		static Action::CmdStr MinFragLen;
		static Action::CmdStr MapMatchCount;
		static Action::CmdStr SetStripWidth;
		static Action::CmdStr SetStripTol;
		static Action::CmdStr ExportAnchorPeaks;
		static Action::CmdStr ExportStripPeaks;
		static Action::CmdStr ShowSpinFits;
		static Action::CmdStr EditAtts;
		static Action::CmdStr Goto;
		static Action::CmdStr GotoResidue;
		static Action::CmdStr SetWidthFactor;
		static Action::CmdStr UnlabeledStripMatch;

		StripScope2(Root::Agent*,Project*,Spectrum*);
	protected:
		virtual ~StripScope2();
		void handleAction(Root::Action&);
		void handle(Root::Message&);
	private:
		void handleUnlabeledStripMatch( Action& );
		void handleShowSystem( Action& );
		void handleSelectSpec( Action& );
		void handlePrevSpec( Action& );
		void handleNextSpec( Action& );
		void handleSetDepth( Action& );
		void handleSetWidthFactor( Action& );
		void handleGotoResidue( Action& );
		void handleGoto( Action& );
		void handleShowSpinFits( Action& );
		void handleShowAllBest( Action& );
		void handleExportStripPeaks( Action& );
		void handleExportAnchorPeaks( Action& );
		void handleStrictStripMatch( Action& );
		void handleRunStripper( Action& );
		void handleSetStripTol( Action& );
		void handleSetStripWidth( Action& );
		void handleShowFrag( Action& );
		void handleShowAllPred( Action& );
		void handleShowAllSucc( Action& );
		void handleShowPred( Action& );
		void handleShowSucc( Action& );
		void handleSelectRuler( Action& );
		void handleShowRulers( Action& );
		void handleSetLaneCount( Action& );
		FRIEND_ACTION_HANDLER( StripScope2 );
		void buildViews();
		void buildMenus();
		void updateCaption();
		Root::Ref<Lexi::FocusManager> d_focus;
		Lexi::GlyphWidget* d_widget;
		Root::Ptr<StripScopeAgent2> d_agent;
		Root::Ref<Lexi::Glyph> d_stripBox;
		Root::Ref<ResidueParamView> d_resiView;
		Root::Ref<StripListGadget> d_list;
	};
}

#endif // !defined(AFX_STRIPSCOPE_H__3ADC5F82_3DD2_4CA1_9696_34C8E5B8BF68__INCLUDED_)
