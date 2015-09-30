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

#if !defined(AFX_STRIPSCOPEAGENT_H__67A659AD_C734_4320_8E8A_F3E0FB430EB7__INCLUDED_)
#define AFX_STRIPSCOPEAGENT_H__67A659AD_C734_4320_8E8A_F3E0FB430EB7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Root/Agent.h>
#include <Root/ActionHandler.h>
#include <Gui/Menu.h>
#include <Spec/Spectrum.h>
#include <Spec/SpecProjector.h>
#include <Spec/Project.h>
#include <Spec/StripQuery2.h>
#include <Spec/AnchorSpace.h>
#include <Spec/SpinPointSpace.h>
#include <Spec/RangeFilterSpace.h>
#include <SpecView/ContourView.h>
#include <SpecView/SpecViewer.h>
#include <SpecView/SpecBufferMdl.h>
#include <SpecView/SpinPointView.h>
#include <SpecView/CursorMdl.h>

namespace Spec
{
	using Root::Action;
	class SliceView;

	class StripScopeAgent2 : public Root::Agent
	{
	public:
		static Action::CmdStr ContourParams;
		static Action::CmdStr AutoContour;
		static Action::CmdStr FitWindow;
		static Action::CmdStr DeleteSpins;
		static Action::CmdStr PickSpin;
		static Action::CmdStr MoveSpin;
		static Action::CmdStr MoveSpinAlias;
		static Action::CmdStr LabelSpin;
		static Action::CmdStr ForceLabelSpin;
		static Action::CmdStr ShowAlignment;
		static Action::CmdStr SelectSpec;
		static Action::CmdStr SetResolution;
		static Action::CmdStr ShowLowRes;
		static Action::CmdStr ForwardStrip;
		static Action::CmdStr BackwardStrip;
		static Action::CmdStr SelectStrips;
		static Action::CmdStr HoldStrip;
		static Action::CmdStr GotoStrip;
		static Action::CmdStr LinkReference;
		static Action::CmdStr UnlinkSucc;
		static Action::CmdStr UnlinkPred;
		static Action::CmdStr LinkStrip;
		static Action::CmdStr AssignStrip;
		static Action::CmdStr AutoCenter;
		static Action::CmdStr UnassignStrip;
		static Action::CmdStr StrictStripMatch;
		static Action::CmdStr UnlabeledStripMatch;
		static Action::CmdStr ShowFolded;
		static Action::CmdStr SetCandidates;
		static Action::CmdStr SetWidth;
		static Action::CmdStr SetTol;
		static Action::CmdStr CreateReport;
		static Action::CmdStr EditAtts;
		static Action::CmdStr EditAttsSys;
		static Action::CmdStr EditAttsLink;
		static Action::CmdStr ReplaceStrip;
		static Action::CmdStr CursorSync;
		static Action::CmdStr NextSpec;
		static Action::CmdStr PrevSpec;
		static Action::CmdStr AutoGain;
		static Action::CmdStr NextStrip;
		static Action::CmdStr PrevStrip;

		static Action::CmdStr ShowWithOff;
		static Action::CmdStr ShowGhosts;
		static Action::CmdStr DoPathSim;
		static Action::CmdStr ShowUnlabeled;
		static Action::CmdStr ShowInfered;
		static Action::CmdStr ShowLinks;
		static Action::CmdStr ProposeSpin;
		static Action::CmdStr PickLabel;
		static Action::CmdStr SyncStrip;
		static Action::CmdStr ViewLabels;
		static Action::CmdStr SetDepth;
		static Action::CmdStr AnchPathSim;
		static Action::CmdStr GhostLabels;
		static Action::CmdStr PickBounds;	
		static Action::CmdStr SetBounds;	
		static Action::CmdStr AutoSlice;	
		static Action::CmdStr PickBoundsSym;	
		static Action::CmdStr DeleteLinks;	
		static Action::CmdStr ShowAllPeaks;	
		static Action::CmdStr HidePeak;	
		static Action::CmdStr KeepSpec;	
		static Action::CmdStr Calibrate;	
		static Action::CmdStr ShowUnknown;

		class AdjustSelection : public Root::UserMessage
		{
			SpinSystem* d_sys;
			SpinPoint d_point;
		public:
			AdjustSelection( const SpinPoint& p ):d_point( p ) {}
			const SpinPoint& getPoint() const { return d_point; }
		};

		struct StripSocket
		{
			Root::Ref<SpecViewer> d_viewer;
			Root::Ref<CursorMdl> d_cur; // ev. fr alle Strips gemeinsam?
			Root::Ref<SpecProjector> d_spec;
			Root::Ref<SpinPointView> d_tuples;
			Root::Ref<RangeFilterSpace> d_mdl;		
			Root::Ref<SpecBufferMdl> d_buf;
			SpinPoint d_tuple;
		};
		struct SliceSocket
		{
			Root::Ref<SpecViewer> d_viewer;
			Root::Ref<CursorMdl> d_cur;
			Root::Ref<SpecProjector> d_spec;
			Root::Ref<SpecBufferMdl> d_buf;
			Root::Ref<SpecProjector> d_specHold;
			Root::Ref<SpecBufferMdl> d_bufHold;
			SliceView* d_slice;
			SliceView* d_sliceHold;
		};
		typedef std::map<QByteArray ,Root::Ref<Spectrum> > Sort;
		const Sort& getSort() const { return d_sort; }
		void updateSpecType();
		void gotoTuple(SpinSystem * sys, Spin * spin );
		const SliceSocket& getSlice() const { return d_slice; }
		const StripSocket& getStrip( Dimension d ) const { return d_strips[ d ]; }
		const Project::SpecSet& getSpecList() const { return d_specList; }
		Project* getProject() const { return d_pro; }
		Gui::Menu* getPopLabel() { return d_popLabel; }
		Gui::Menu* getPopSpec() { return d_popSpec; }
		Spectrum* getSpec() const { return d_spec; }
		void stepSpec( bool next );
		void initViews( int count, bool slice = true );
		void setSpec( Spectrum* );
		void fillPage(bool autoFit = true);
		StripQuery2* getQuery() const { return d_query; }
		void resetFirst() { d_first = 0; }
		int getLaneCount() const { return d_strips.size(); }
		SpinSpace* getMdl2D() const { return d_mdl2D; }
		SpinSpace* getMdl3D() const { return d_mdl3D; }
		bool findTuple(SpinSystem *sys, Spin *spin, SpinPoint& tuple );
		StripScopeAgent2(Root::Agent*,Project*,Spectrum*);
	protected:
		bool findSystem( Index, SpinPoint& );
		void updateSpecPop();
		void buildPopup();
		void createContour( int i );
		void notifyCursor();
		void updateStrip( int lane, CursorMdl::Update *msg );
		void updateStrip( int lane, ViewAreaMdl::Update *msg );
		void updateSlice( CursorMdl::Update *msg );
		void updateSlice( ViewAreaMdl::Update *msg );
		void initParams();
		void createStrip( int lane );
		void createSlice();
		void fillStrip( int i, const PpmPoint& a, 
			const SpinPoint& strip, bool hold );
		virtual ~StripScopeAgent2();
		void handle(Root::Message&);
	private:
		void handleShowUnknown( Action& );
		void handleCalibrate( Action& );
		void handleEditAttsLink( Action& );
		void handleEditAttsSys( Action& );
		void handleGotoStrip( Action& );
		void handleKeepSpec( Action& );
		void handleHidePeak( Action& );
		void handleShowAllPeaks( Action& );
		void handleDeleteLinks( Action& );
		void handlePickBoundsSym( Action& );
		void handleAutoSlice( Action& );
		void handleSetBounds( Action& );
		void handlePickBounds( Action& );
		void handleGhostLabels( Action& );
		void handleAnchPathSim( Action& );
		void handleViewLabels( Action& );
		void handleSyncStrip( Action& );
		void handlePickLabel( Action& );
		void handleShowWithOff( Action& );
		void handleProposeSpin( Action& );
		void handleShowLinks( Action& );
		void handleShowInfered( Action& );
		void handleShowUnlabeled( Action& );
		void handleDoPathSim( Action& );
		void handleShowGhosts( Action& );
		void handlePrevStrip( Action& );
		void handleNextStrip( Action& );
		void handleAutoGain( Action& );
		void handleCursorSync( Action& );
		void handleReplaceStrip( Action& );
		void handleEditAtts( Action& );
		void handleCreateReport( Action& );
		void handleSetTol( Action& );
		void handleSetWidth( Action& );
		void handleSetCandidates( Action& );
		void handleShowFolded( Action& );
		void handleStrictStripMatch( Action& );
		void handleUnlabeledStripMatch( Action& );
		void handleUnassignStrip( Action& );
		void handleAutoCenter( Action& );
		void handleAssignStrip( Action& );
		void handleLinkStrip( Action& );
		void handleUnlinkPred( Action& );
		void handleUnlinkSucc( Action& );
		void handleLinkReference( Action& );
		void handleHoldStrip( Action& );
		void handleSelectStrips( Action& );
		void handleBackwardStrip( Action& );
		void handleForwardStrip( Action& );
		void handleShowLowRes( Action& );
		void handleSetResolution( Action& );
		void handleShowAlignment( Action& );
		void handleForceLabelSpin( Action& );
		void handleLabelSpin( Action& );
		void handleMoveSpinAlias( Action& );
		void handleMoveSpin( Action& );
		void handlePickSpin( Action& );
		void handleDeleteSpins( Action& );
		void handleFitWindow( Action& );
		void handleAutoContour( Action& );
		void handleContourParams( Action& );

		FRIEND_ACTION_HANDLER( StripScopeAgent2 );
		PpmPoint d_cursor;

		Root::Ref<Project> d_pro;
		Root::Ref<StripQuery2> d_query;
		Root::Ref<AnchorSpace> d_mdl2D;
		Root::Ref<LinkFilterRotSpace> d_mdl3D;
		SpinPointSpace* d_src3D;
		Root::Ref<LinkFilterRotSpace> d_mdl3DHold;
		SpinPointSpace* d_src3DHold;
		Project::SpecSet d_specList;
		Sort d_sort;

		Root::Vector<StripSocket> d_strips;	
		SliceSocket d_slice;

		float d_gain;
		float d_contourFactor;
		ContourView::Option d_contourOption;
		Root::Byte d_resol;
		bool d_autoContour;
		bool d_lowResol;
		bool d_folding;
		bool d_autoCenter;

		bool d_lock;
		bool d_hold;
		bool d_keep;
		bool d_cursorSync;
		int d_curLane;
		int d_lane;
		Root::Index d_first;	
		SpecRef<Spectrum> d_spec;
		SpecRef<Spectrum> d_holdSpec;
		Root::Ref<Spectrum> d_orig;
		Gui::Menu* d_popLabel;
		Gui::Menu* d_popSpec;
		Gui::Menu* d_popStrip;
	};
}

#endif // !defined(AFX_STRIPSCOPEAGENT_H__67A659AD_C734_4320_8E8A_F3E0FB430EB7__INCLUDED_)
