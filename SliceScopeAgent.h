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

#if !defined(AFX_SLICERSCOPEAGENT_H__2EFEBE1D_EEEA_497A_AB70_74D96825060A__INCLUDED_)
#define AFX_SLICERSCOPEAGENT_H__2EFEBE1D_EEEA_497A_AB70_74D96825060A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Root/Agent.h>
#include <Root/ActionHandler.h>
#include <Spec/SpecProjector.h>
#include <SpecView/SpecViewer.h>
#include <SpecView/SpecBufferMdl.h>
#include <SpecView/CursorMdl.h>
#include <SpecView/PeakPlaneView.h>
#include <Spec/PeakList.h>
#include <Spec/Project.h>
#include <Root/Vector.h>
#include <Gui/Menu.h>

namespace Spec
{
	using Root::Action;

	class SliceScopeAgent : public Root::Agent 
	{
	public:
		static Action::CmdStr AddSpec;
		static Action::CmdStr RemoveSpec;
		static Action::CmdStr FitToWindow;
		static Action::CmdStr CursorSyncX;
		static Action::CmdStr CursorSyncY;
		static Action::CmdStr ShowSpins;
		static Action::CmdStr PickBounds;	
		static Action::CmdStr SetSliceMinMax;	
		static Action::CmdStr SliceAutoScale;	
		static Action::CmdStr PickBoundsSym;	
		static Action::CmdStr CreateReport;	
		static Action::CmdStr AutoCenter;
		static Action::CmdStr DeleteSpins;
		static Action::CmdStr PickSpin;
		static Action::CmdStr MoveSpin;
		static Action::CmdStr MoveSpinAlias;
		static Action::CmdStr LabelSpin;
		static Action::CmdStr ForceLabelSpin;
		static Action::CmdStr ShowFolded;
		static Action::CmdStr EditAtts;
		static Action::CmdStr Calibrate;

		struct SliceSocket
		{
			Root::Ref<SpecViewer> d_viewer;
			Root::Ref<CursorMdl> d_cur;
			Root::Ref<Spectrum> d_spec;
			Root::Ref<SpecBufferMdl> d_buf;
			Root::Ref<PeakSliceView> d_peaks;
		};
		typedef std::set< SpecRef<Spectrum> > SpecSet;
		typedef Root::Vector<SliceSocket> Slices;

		void removeSpec( int lane );
		void setSpec( int i, Spectrum*, bool fit = false );
		const Slices& getSlices() const { return d_slices; }
		const SpecSet& getSpecs() const { return d_specs; }
		Project* getProject() const { return d_pro; }
		Gui::Menu* getPopLabel() { return d_popLabel; }
		Gui::Menu* getPopSpec() { return d_popSpec; }
		void addSpec( Spectrum* );
		int getLane() const { return d_lane; }

		SliceScopeAgent(Root::Agent* parent, Spectrum* s, Project*, PeakList*);
	protected:
		void updateSpecPop();
		virtual ~SliceScopeAgent();
		void initParams();
		void buildPopup();
		void createSlice(int);
		void notifyCursor();
		void handle(Root::Message&);
		void updateSlice( int, CursorMdl::Update * msg );
		void updateSlice( int, ViewAreaMdl::Update * msg );
	private:
		void handleCalibrate( Action& );
		void handleEditAtts( Action& );
		void handleShowFolded( Action& );
		void handleForceLabelSpin( Action& );
		void handleLabelSpin( Action& );
		void handleMoveSpinAlias( Action& );
		void handleMoveSpin( Action& );
		void handlePickSpin( Action& );
		void handleDeleteSpins( Action& );
		void handleAutoCenter( Action& );
		void handleCreateReport( Action& );
		void handlePickBoundsSym( Action& );
		void handleSliceAutoScale( Action& );
		void handleSetSliceMinMax( Action& );
		void handlePickBounds( Action& );
		void handleShowSpins( Action& );
		void handleCursorSyncX( Action& );
		void handleCursorSyncY( Action& );
		void handleFitToWindow( Action& );
		FRIEND_ACTION_HANDLER( SliceScopeAgent );
		SpecSet d_specs;
		Root::Ref<Project> d_pro;
		Root::Ref<PeakList> d_pl;

		PPM d_cursor;
		int d_lane;

		Slices d_slices;			// Alle drei Slices
		bool d_folding;
		bool d_lock;
		bool d_cursorSyncX;
		bool d_cursorSyncY;
		bool d_showPeaks;
		Gui::Menu* d_popLabel;
		Gui::Menu* d_popSlice;
		Gui::Menu* d_popSpec;
	};
}

#endif // !defined(AFX_SLICERSCOPEAGENT_H__2EFEBE1D_EEEA_497A_AB70_74D96825060A__INCLUDED_)
