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

#if !defined(AFX_SYSTEMSCOPEAGENT_H__9F5C8044_B2B8_4680_8125_131E6473CADD__INCLUDED_)
#define AFX_SYSTEMSCOPEAGENT_H__9F5C8044_B2B8_4680_8125_131E6473CADD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Root/Agent.h>
#include <Root/ActionHandler.h>
#include <Spec/RangeFilterSpace.h>
#include <Spec/AnchorSpace.h>
#include <Spec/Project.h>
#include <Spec/SpecProjector.h>
#include <SpecView/ContourView.h>
#include <SpecView/SpecViewer.h>
#include <SpecView/SpecBufferMdl.h>
#include <SpecView/SpinPointView.h>
#include <SpecView/CursorMdl.h>
#include <Gui/Menu.h>

namespace Spec
{
	using Root::Action;
	class CursorCtrl;
	class SpinPointSpace;
	class SliceView;

	class SystemScopeAgent2 : public Root::Agent
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
		static Action::CmdStr AutoCenter;
		static Action::CmdStr ShowFolded;
		static Action::CmdStr ShowOrthogonal;
		static Action::CmdStr PickOrtho;
		static Action::CmdStr MoveOrtho;
		static Action::CmdStr MoveOrthoAlias;
		static Action::CmdStr LabelOrtho;
		static Action::CmdStr ForceLabelOrtho;
		static Action::CmdStr DeleteOrthos;
		static Action::CmdStr ShowVCursor;
		static Action::CmdStr OrthoCalibrate;
		static Action::CmdStr StripCalibrate;
		static Action::CmdStr ShowDepth;
		static Action::CmdStr SetWidth;
		static Action::CmdStr ShowWithOff;
		static Action::CmdStr EditAtts;
		static Action::CmdStr EditAttsSys;
		static Action::CmdStr EditAttsLink;
		static Action::CmdStr EditAttsOrtho;
		static Action::CmdStr CursorSync;
		static Action::CmdStr NextSpec;
		static Action::CmdStr PrevSpec;
		static Action::CmdStr FitOrtho;
		static Action::CmdStr FitOrthoX;
		static Action::CmdStr AutoGain;
		static Action::CmdStr ShowGhosts;
		static Action::CmdStr DoPathSim;
		static Action::CmdStr ShowUnlabeled;
		static Action::CmdStr ShowInfered;
		static Action::CmdStr ShowLinks;
		static Action::CmdStr ProposeSpin;
		static Action::CmdStr ProposeOrtho;
		static Action::CmdStr PickLabel;
		static Action::CmdStr PickLabelOrtho;
		static Action::CmdStr ViewLabels;
		static Action::CmdStr SetDepth;
		static Action::CmdStr GhostLabels;
		static Action::CmdStr ViewLabels2;
		static Action::CmdStr PickBounds;	
		static Action::CmdStr SetBounds;	
		static Action::CmdStr AutoSlice;	
		static Action::CmdStr PickBoundsSym;	
		static Action::CmdStr DeleteLinks;	
		static Action::CmdStr ShowAllPeaks;	
		static Action::CmdStr HidePeak;	
		static Action::CmdStr ShowUnknown;	

		struct StripSocket
		{
			Root::Ref<SpecViewer> d_viewer;
			Root::Ref<CursorMdl> d_cur; 
			Root::Ref<SpecProjector> d_spec;
			Root::Ref<SpinPointView> d_tuples;
			Root::Ref<RangeFilterSpace> d_mdl;		
			Root::Ref<SpecBufferMdl> d_buf;
			CursorCtrl* d_curCtl;
		};
		struct SliceSocket
		{
			Root::Ref<SpecViewer> d_viewer;
			Root::Ref<CursorMdl> d_cur;
			Root::Ref<SpecProjector> d_spec;
			Root::Ref<SpecBufferMdl> d_buf;
			Root::Ref<SpecProjector> d_specOrtho;
			Root::Ref<SpecBufferMdl> d_bufOrtho;
			SliceView* d_slice;
			SliceView* d_sliceOrtho;
		};
		struct OrthoSocket
		{
			Root::Ref<SpecViewer> d_viewer;
			Root::Ref<CursorMdl> d_cur; 
			Root::Ref<SpecProjector> d_spec;
			Root::Ref<SpinPointView> d_tuples;
			Root::Ref<RangeFilterSpace> d_mdl;		
			Root::Ref<SpecBufferMdl> d_buf;
			SpinPoint d_tuple;
		};
		typedef Root::Vector<OrthoSocket> Orthos;

		void setAnchor( const SpinPoint& );
		const SpinPoint& getAnchor() const { return d_anchor; }
		void setSpec( Spectrum* );
		AnchorSpace* getModel() const { return d_anchs; }
		LinkFilterRotSpace* getSpace() const { return d_mdl; }
		Project* getProject() const { return d_pro; }
		Spectrum* getSpec() const { return d_spec; }
		Gui::Menu* getPopLabel() { return d_popLabel; }
		Gui::Menu* getPopLabelOrtho() { return d_popLabelOrtho; }
		Gui::Menu* getPopSpec() { return d_popSpec; }

		const SliceSocket& getSlice() const { return d_slice; }
		const StripSocket& getStrip() const { return d_strip; }
		const Orthos& getOrthos() const { return d_orthos; }
		void createOrthos(bool depthOnly = false );
		void killOrthos();

		SystemScopeAgent2(Root::Agent*,Project*,Spectrum* );
	protected:
		void updateOrthoLabel();
		void updateLabelPop();
		void updateOrthoContour();
		void updateSpecPop();
		void updateOrtho( int i, CursorMdl::Update* );
		void updateOrtho( int i, ViewAreaMdl::Update* );
		void buildPopup();
		void initParams();
		void createContour();
		void buildViews();
		void createSlice();
		void createStrip();
		void updateStrip( CursorMdl::Update *msg );
		void updateStrip( ViewAreaMdl::Update *msg );
		void updateSlice( CursorMdl::Update *msg );
		void updateSlice( ViewAreaMdl::Update *msg );
		void notifyCursor(bool ortho = false );
		virtual ~SystemScopeAgent2();
		void handle(Root::Message&);
		void stepSpec( bool next );
	private:
		void handleShowUnknown( Action& );
		void handleFitOrthoX( Action& );
		void handleEditAttsLink( Action& );
		void handleEditAttsSys( Action& );
		void handleHidePeak( Action& );
		void handleShowAllPeaks( Action& );
		void handleDeleteLinks( Action& );
		void handlePickBoundsSym( Action& );
		void handleAutoSlice( Action& );
		void handleSetBounds( Action& );
		void handlePickBounds( Action& );
		void handleViewLabels2( Action& );
		void handleGhostLabels( Action& );
		void handleSetDepth( Action& );
		void handleViewLabels( Action& );
		void handlePickLabelOrtho( Action& );
		void handlePickLabel( Action& );
		void handleProposeOrtho( Action& );
		void handleProposeSpin( Action& );
		void handleShowLinks( Action& );
		void handleShowInfered( Action& );
		void handleShowUnlabeled( Action& );
		void handleDoPathSim( Action& );
		void handleShowGhosts( Action& );
		void handleAutoGain( Action& );
		void handleFitOrtho( Action& );
		void handlePrevSpec( Action& );
		void handleNextSpec( Action& );
		void handleCursorSync( Action& );
		void handleEditAttsOrtho( Action& );
		void handleEditAtts( Action& );
		void handleShowWithOff( Action& );
		void handleSetWidth( Action& );
		void handleStripCalibrate( Action& );
		void handleOrthoCalibrate( Action& );
		void handleShowVCursor( Action& );
		void handleDeleteOrthos( Action& );
		void handleForceLabelOrtho( Action& );
		void handleLabelOrtho( Action& );
		void handleMoveOrthoAlias( Action& );
		void handleMoveOrtho( Action& );
		void handlePickOrtho( Action& );
		void handleShowFolded( Action& );
		void handleAutoCenter( Action& );
		void handleShowLowRes( Action& );
		void handleSetResolution( Action& );
		void handleSelectSpec( Action& );
		void handleForceLabelSpin( Action& );
		void handleLabelSpin( Action& );
		void handleMoveSpinAlias( Action& );
		void handleMoveSpin( Action& );
		void handlePickSpin( Action& );
		void handleDeleteSpins( Action& );
		void handleFitWindow( Action& );
		void handleAutoContour( Action& );
		void handleContourParams( Action& );
		FRIEND_ACTION_HANDLER( SystemScopeAgent2 );

		Root::Ref<AnchorSpace> d_anchs;
		Root::Ref<LinkFilterRotSpace> d_mdl;
		Root::Ref<ViewAreaMdl> d_area0;
		SpinPointSpace* d_src;
		Root::Ref<Project> d_pro;
		PpmPoint d_cursor;

		Project::SpecSet d_specList;
		SpecRef<Spectrum> d_spec;
		Root::Ref<Spectrum> d_orig;
		SpinPoint d_anchor;
		PPM d_anchorX, d_anchorZ;
		typedef std::map<QByteArray ,Root::Ref<Spectrum> > Sort;
		Sort d_sort;

		SliceSocket d_slice;
		StripSocket d_strip;
		Orthos d_orthos;
		Root::Index d_curOrtho;
		SpinPointView::Label d_ol;
		
		float d_gain;
		float d_contourFactor;
		ContourView::Option d_contourOption;
		Root::Byte d_resol;
		bool d_autoContour;
		bool d_lowResol;
		bool d_folding;
		bool d_autoCenter;
		bool d_showVCur;
		bool d_cursorSync;

		bool d_lock;
		Gui::Menu* d_popLabel;
		Gui::Menu* d_popLabelOrtho;
		Gui::Menu* d_popSpec;
		Gui::Menu* d_popStrip;
		Gui::Menu* d_popOrtho;
	};
}

#endif // !defined(AFX_SYSTEMSCOPEAGENT_H__9F5C8044_B2B8_4680_8125_131E6473CADD__INCLUDED_)
