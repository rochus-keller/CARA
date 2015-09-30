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

#if !defined(AFX_HOMOSCOPEAGENT_H__968502C8_16EB_4753_964A_2A91CCC977D7__INCLUDED_)
#define AFX_HOMOSCOPEAGENT_H__968502C8_16EB_4753_964A_2A91CCC977D7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Root/Agent.h>
#include <Root/ActionHandler.h>
#include <Lexi/Layer.h>
#include <Root/Vector.h>
#include <Spec/SpecProjector.h>
#include <Spec/RangeFilterSpace.h>
#include <Spec/Project.h>
#include <Spec/PeakProjector.h>
#include <SpecView/CursorMdl.h>
#include <SpecView/ViewAreaMdl.h>
#include <SpecView/SpecViewer.h>
#include <SpecView/SpinPointView.h>
#include <SpecView/ContourView.h>
#include <SpecView/SpecBufferMdl.h>
#include <SpecView/IntensityView.h>
#include <SpecView/PeakPlaneView.h>
#include <Gui/Menu.h>

namespace Spec
{
	class CursorMdl;
	class ViewAreaMdl;
	class SpecViewer;
	class PeakListPeer;
	class SliceView;
	class SpecBufferMdl;
	class SpinPointSpace;
	class PeakPlaneView;
	using Root::Action;

	class PolyScopeAgent2 : public Root::Agent 
	{
	public:
		static Action::CmdStr SetResolution;
		static Action::CmdStr ShowLowRes;
		static Action::CmdStr Forward;
		static Action::CmdStr Backward;
		static Action::CmdStr FitWindow;
		static Action::CmdStr FitWindowX;
		static Action::CmdStr FitWindowY;
		static Action::CmdStr ShowFolded;
		static Action::CmdStr SpecCalibrate;
		static Action::CmdStr AutoCenter;
		static Action::CmdStr ShowContour;	
		static Action::CmdStr ShowIntensity;
		static Action::CmdStr AutoContour;
		static Action::CmdStr ContourParams;
		static Action::CmdStr AutoContour2;
		static Action::CmdStr ContourParams2;
		static Action::CmdStr PickSystem;
		static Action::CmdStr PickHori;
		static Action::CmdStr PickVerti;
		static Action::CmdStr MovePeak;
		static Action::CmdStr MovePeakAlias;
		static Action::CmdStr LabelPeak;
		static Action::CmdStr HidePeak;
		static Action::CmdStr UnhidePeak;
		static Action::CmdStr DeletePeak;
		static Action::CmdStr DeleteSpinX;
		static Action::CmdStr DeleteSpinY;
		static Action::CmdStr ShowAllPeaks;
		static Action::CmdStr ShowAlignment;
		static Action::CmdStr Assign;
		static Action::CmdStr Unassign;
		static Action::CmdStr SetSystemType;
		static Action::CmdStr ViewLabels;
		static Action::CmdStr SelectSpec2D;
		static Action::CmdStr LinkSystems;
		static Action::CmdStr UnlinkPred;
		static Action::CmdStr UnlinkSucc;
		static Action::CmdStr SelectSystems;
		static Action::CmdStr SelectSpins;
		static Action::CmdStr HoldReference;
		static Action::CmdStr AutoHold;
		static Action::CmdStr CreateReport;	
		static Action::CmdStr AddRulerVerti;	
		static Action::CmdStr AddRulerHori;	
		static Action::CmdStr RemoveRulers;	
		static Action::CmdStr RemoveAllRulers;	
		static Action::CmdStr AutoRuler;	
		static Action::CmdStr ProposeHori;	
		static Action::CmdStr ProposeVerti;	
		static Action::CmdStr ProposePeak;
		static Action::CmdStr SelectSpec3D;	
		static Action::CmdStr PickSpin3D;
		static Action::CmdStr MoveSpin3D;
		static Action::CmdStr MoveSpinAlias3D;
		static Action::CmdStr DeleteSpins3D;
		static Action::CmdStr LabelSpin3D;
		static Action::CmdStr ForceLabelSpin3D;
		static Action::CmdStr SetWidth;
		static Action::CmdStr FitWindow3D;
		static Action::CmdStr ShowWithOff;
		static Action::CmdStr Show3dPlane;
		static Action::CmdStr AutoHide;
		static Action::CmdStr StripCalibrate;
		static Action::CmdStr ProposeSpin;
		static Action::CmdStr EditAttsSpinH;
		static Action::CmdStr EditAttsSpinV;
		static Action::CmdStr EditAttsLink;
		static Action::CmdStr EditAttsSysH;
		static Action::CmdStr EditAttsSysV;
		static Action::CmdStr EditAttsSpin3D;
		static Action::CmdStr EditAttsSys3D;
		static Action::CmdStr EditAttsLink3D;
		static Action::CmdStr CursorSync;
		static Action::CmdStr GotoSystem;
		static Action::CmdStr NextSpec3D;
		static Action::CmdStr PrevSpec3D;
		static Action::CmdStr NextSpec2D;
		static Action::CmdStr PrevSpec2D;
		static Action::CmdStr ForwardPlane;
		static Action::CmdStr BackwardPlane;
		static Action::CmdStr ShowWithOff2;
		static Action::CmdStr DeleteLinks;
		static Action::CmdStr LabelVerti;
		static Action::CmdStr LabelHori;
		static Action::CmdStr SetCandidates;
		static Action::CmdStr ShowUnlabeled;
		static Action::CmdStr CreateLinks;
		static Action::CmdStr ForceCross;
		static Action::CmdStr DeleteLinks3D;
		static Action::CmdStr ViewLabels3D;
		static Action::CmdStr AutoGain;
		static Action::CmdStr AutoGain3D;
		static Action::CmdStr ShowGhosts;
		static Action::CmdStr PickLabel3D;
		static Action::CmdStr ShowLinks;
		static Action::CmdStr ShowInfered;
		static Action::CmdStr ShowLinks2;
		static Action::CmdStr ShowInfered2;
		static Action::CmdStr ShowUnlabeled2;
		static Action::CmdStr SetDepth;
		static Action::CmdStr GhostLabels;
		static Action::CmdStr HidePeak2;
		static Action::CmdStr GotoPeak;
		static Action::CmdStr RangeSync;
		static Action::CmdStr OverlayCount;
		static Action::CmdStr ActiveOverlay;
		static Action::CmdStr SetPosColor;
		static Action::CmdStr SetNegColor;
		static Action::CmdStr OverlaySpec;
		static Action::CmdStr CntFactor;
		static Action::CmdStr CntThreshold;
		static Action::CmdStr CntOption;
		static Action::CmdStr AddLayer;
		static Action::CmdStr ComposeLayers;
		static Action::CmdStr UseLinkColors;
		static Action::CmdStr UseLinkColors3D;
		static Action::CmdStr SetLinkParams;
		static Action::CmdStr SetLinkParams3D;
		static Action::CmdStr GotoPoint;
		static Action::CmdStr NewPeakList;
		static Action::CmdStr OpenPeakList;
		static Action::CmdStr SavePeakList;
		static Action::CmdStr MapPeakList;
		static Action::CmdStr PickPlPeak;
		static Action::CmdStr MovePlPeak;
		static Action::CmdStr MovePlAlias;
		static Action::CmdStr LabelPlPeak;
		static Action::CmdStr DeletePlPeaks;
		static Action::CmdStr EditPlPeakAtts;
		static Action::CmdStr SetPlColor;
		static Action::CmdStr DeleteAliasPeak;
		static Action::CmdStr GotoPlPeak;
		static Action::CmdStr ViewPlLabels;
		static Action::CmdStr SyncDepth;
		static Action::CmdStr AdjustIntensity;
		static Action::CmdStr ShowUnknown;
		static Action::CmdStr ShowUnknown2;

		struct PlaneSocket
		{
			struct Overlay
			{
				Root::Ref<ContourView> d_view;
				Root::Ref<SpecBufferMdl> d_buf;
				Root::Ref<SpecProjector> d_spec;
				/* folgende bereits in ContourView
				float d_gain; 
				float d_contourFactor;
				ContourView::Option d_contourOption;
				bool d_autoContour;
				*/
			};
			Root::Ref<SpecViewer> d_viewer;
			Root::Ref<IntensityView> d_intens;
			Root::Ptr<CursorMdl> d_cur;
			Root::Ref<SpinPointView> d_tuples;
			Root::Ref<RangeFilterSpace> d_mdl3D;		
			Root::Ref<SpinPoint1DView> d_hRuler;
			ManualSpinSpace* d_hRulerMdl;
			Root::Ref<SpinPoint1DView> d_vRuler;
			Root::Ref<PeakPlaneView> d_peaks;
			Root::Ref<PeakSubSpace> d_pp;
			ManualSpinSpace* d_vRulerMdl;
			Root::Vector<Overlay> d_ol;
			PlaneSocket():d_hRulerMdl(0),d_vRulerMdl(0),d_pp(0) {}
		};
		struct SliceSocket
		{
			Root::Ref<SpecViewer> d_viewer;
			Root::Ref<CursorMdl> d_cur;
			Root::Ref<SpecProjector> d_spec2D;
			Root::Ref<SpecBufferMdl> d_buf2D;
			Root::Ref<SpecProjector> d_spec3D;
			Root::Ref<SpecBufferMdl> d_buf3D;
		};
		struct StripSocket
		{
			Root::Ref<ContourView> d_view;
			Root::Ref<SpecViewer> d_viewer;
			Root::Ref<CursorMdl> d_cur; 
			Root::Ref<SpecProjector> d_spec;
			Root::Ref<SpinPointView> d_tuples;
			Root::Ref<RangeFilterSpace> d_mdl;
			Root::Ref<SpecBufferMdl> d_buf;
			Root::Ref<PeakPlaneView> d_peaks;
			Root::Ref<PeakSubSpace> d_pp;
			StripSocket():d_pp(0){}
		};

		const PlaneSocket& getPlane() const { return d_plane; }
		const SliceSocket& getSlice( Dimension d ) const { return d_slices[ d ]; }
		const StripSocket& getStrip( Dimension d ) const { return d_strips[ d ]; }
		Spectrum* getSpec() const { return d_spec2D; }
		Spectrum* getSpec2D() const { return d_spec2D; }
		Spectrum* getSpec3D() const { return d_spec3D; }
		bool show3DPlane() const { return d_show3DPlane; }
		Project* getProject() const { return d_pro; }
		Gui::Menu* getPopSpec2D() const { return d_popSpec2D; }
		Gui::Menu* getPopSpec3D() const { return d_popSpec3D; }
		Gui::Menu* getPopLabel() { return d_popLabel; }
		LinkFilterRotSpace* getMdl2D() const { return d_mdl2D; }
		LinkFilterRotSpace* getMdl3D() const { return d_mdl3D; }
		Spin* getSel( bool hori ) const;
		void gotoTuple( SpinSystem*, Spin*, Spin*, bool twoD );
		void setCursor(PpmPoint = PpmPoint());

		bool askToClosePeaklist();

		void setPeakList( PeakList* );

		PolyScopeAgent2(Root::Agent* parent, Spectrum*, Project*, 
			bool homo = false, bool synchro = false );

		class SpecChanged : public Root::UserMessage
		{
		public:
			SpecChanged( bool threeD ):d_threeD( threeD ) {}
			bool d_threeD;
		};
	protected:
		void updateStripLabelPop();
		bool savePeakList();
		int selectLayer();
		void setActiveOverlay( int );
		void initOverlay( int );
		Spin* getSelectedSpin();
		void syncStripWidth();
		void centerToCursor(bool threeD = false);
		void pickSpin( Dimension, Spin* other, SpinSystem* owner = 0 );
		void extendSystem( Dimension source, Dimension target );
		void setSpec2D( Spectrum* );
		void stepSpec3D( bool next );
		void stepSpec2D( bool next );
		void setSpec3D( Spectrum* );
		void updateRef();
		virtual ~PolyScopeAgent2();
		void initParams();
		void buildPopup();
		void buildPopup2();
		void createPlane();
		void createStrip(Dimension);
		void updatePlaneLabel();
		void initViews( bool homo );
		void createSlice(Dimension view, Dimension spec);
		void notifyCursor(bool = true);
		void updateContour( int i = 0, bool redraw = true );
		void showIntens(bool on );
		void sync2dPlaneSliceToCur();
		void sync3dZSliceToCur();
		void syncStripsToCur();
		void sync3dXySliceToCur(Dimension,bool show);
		void updateStrip( Dimension dim, CursorMdl::Update *msg );
		void updateStrip( Dimension dim, ViewAreaMdl::Update *msg );
		void updateSpecPop2D();
		void updateSpecPop3D();
		void handle(Root::Message&);
		void updatePlane( CursorMdl::Update* );
		void updateSlice( Dimension, CursorMdl::Update * msg );
		void updatePlane( ViewAreaMdl::Update* );
		void updateSlice( Dimension, ViewAreaMdl::Update * msg );
		void selectCurSystem(bool force = false);
		void updateContour2( Dimension d, bool );
		void update3dPlane();
	private:
		void handleShowUnknown( Action& );
		void handleShowUnknown2( Action& );
		void handleAdjustIntensity( Action& );
		void handleSyncDepth( Action& );
		void handleViewPlLabels( Action& );
		void handleGotoPlPeak( Action& );
		void handleFitWindowY( Action& );
		void handleFitWindowX( Action& );
		void handleDeleteAliasPeak( Action& );
		void handleMovePlAlias( Action& );
		void handleSetPlColor( Action& );
		void handleEditPlPeakAtts( Action& );
		void handleDeletePlPeaks( Action& );
		void handleLabelPlPeak( Action& );
		void handleMovePlPeak( Action& );
		void handlePickPlPeak( Action& );
		void handleMapPeakList( Action& );
		void handleSavePeakList( Action& );
		void handleOpenPeakList( Action& );
		void handleNewPeakList( Action& );
		void handleGotoPoint( Action& );
		void handleSetLinkParams3D( Action& );
		void handleSetLinkParams( Action& );
		void handleUseLinkColors3D( Action& );
		void handleUseLinkColors( Action& );
		void handleComposeLayers( Action& );
		void handleAddLayer( Action& );
		void handleCntOption( Action& );
		void handleCntThreshold( Action& );
		void handleCntFactor( Action& );
		void handleOverlaySpec( Action& );
		void handleSetNegColor( Action&);
		void handleSetPosColor( Action& );
		void handleActiveOverlay( Action& );
		void handleOverlayCount( Action& );
		void handleEditAttsLink3D( Action& );
		void handleEditAttsSys3D( Action& );
		void handleRangeSync( Action& );
		void handleGotoPeak( Action& );
		void handleHidePeak2( Action& );
		void handleGhostLabels( Action& );
		void handleSetDepth( Action& );
		void handleShowUnlabeled2( Action& );
		void handleShowInfered2(Action & );
		void handleShowLinks2(Action & );
		void handlePickLabel3D( Action& );
		void handleAutoHold( Action& );
		void handleShowGhosts( Action& );
		void handleAutoGain3D( Action& );
		void handleAutoGain( Action& );
		void handleViewLabels3D( Action& );
		void handleDeleteLinks3D( Action& );
		void handleForceCross( Action& );
		void handleCreateLinks( Action& );
		void handleShowUnlabeled( Action& );
		void handleShowInfered( Action& );
		void handleSetCandidates( Action& );
		void handleLabelHori( Action& );
		void handleLabelVerti( Action& );
		void handleDeleteLinks( Action& );
		void handleShowLinks( Action& );
		void handleShowWithOff2( Action& );
		void handleBackwardPlane( Action& );
		void handleForwardPlane( Action& );
		void handlePrevSpec2D( Action& );
		void handleNextSpec2D( Action& );
		void handlePrevSpec3D( Action& );
		void handleNextSpec3D( Action& );
		void handleGotoSystem( Action& );
		void handleCursorSync( Action& );
		void handleEditAttsSpin3D( Action& );
		void handleEditAttsSysV( Action& );
		void handleEditAttsSysH( Action& );
		void handleEditAttsLink( Action& );
		void handleEditAttsSpinV( Action& );
		void handleEditAttsSpinH( Action& );
		void handleProposeSpin( Action& );
		void handleStripCalibrate( Action& );
		void handleAutoHide( Action& );
		void handleShow3dPlane( Action& );
		void handleShowWithOff( Action& );
		void handleContourParams2( Action& );
		void handleAutoContour2( Action& );
		void handleFitWindow3D( Action& );
		void handleSetWidth( Action& );
		void handleForceLabelSpin3D( Action& );
		void handleLabelSpin3D( Action& );
		void handleDeleteSpins3D( Action& );
		void handleMoveSpinAlias3D( Action& );
		void handleMoveSpin3D( Action& );
		void handlePickSpin3D( Action& );
		void handleSelectSpec3D( Action& );
		void handleProposePeak( Action& );
		void handleProposeVerti( Action& );
		void handleProposeHori( Action& );
		void handleAutoRuler( Action& );
		void handleRemoveAllRulers( Action& );
		void handleRemoveRulers( Action& );
		void handleAddRulerHori( Action& );
		void handleAddRulerVerti( Action& );
		void handleCreateReport( Action& );
		void handleHoldReference( Action& );
		void handleUnhidePeak( Action& );
		void handleUnlinkSucc( Action& );
		void handleUnlinkPred( Action& );
		void handleLinkSystems( Action& );
		void handleSelectSpec2D( Action& );
		void handleViewLabels( Action& );
		void handleSetSystemType( Action& );
		void handleUnassign( Action& );
		void handleAssign( Action& );
		void handleShowAlignment( Action& );
		void handleShowAllPeaks( Action& );
		void handleDeleteSpinY( Action& );
		void handleDeleteSpinX( Action& );
		void handleDeletePeak( Action& );
		void handleHidePeak( Action& );
		void handleLabelPeak( Action& );
		void handleMovePeakAlias( Action& );
		void handleMovePeak( Action& );
		void handlePickVerti( Action& );
		void handlePickHori( Action& );
		void handlePickSystem( Action& );
		void handleContourParams( Action& );
		void handleAutoContour( Action& );
		void handleShowIntensity( Action& );
		void handleShowContour( Action& );
		void handleAutoCenter( Action& );
		void handleSpecCalibrate( Action& );
		void handleShowFolded( Action& );
		void handleFitWindow( Action& );
		void handleBackward( Action& );
		void handleForward( Action& );
		void handleShowLowRes( Action& );
		void handleSetResolution( Action& );
		FRIEND_ACTION_HANDLER( PolyScopeAgent2 );

		SpecRef<Spectrum> d_spec2D;				// 2D Hauptspektrum
		SpecRef<Spectrum> d_spec3D;				// 3D Zusatzspektrum (kann leer sein)
		Root::Ref<Spectrum> d_orig;

		PpmPoint d_cursor;

		PlaneSocket d_plane;						// Die Plane-Aufsicht
		Root::Vector<SliceSocket> d_slices;			// Alle drei Slices
		Root::Vector<StripSocket> d_strips;			// Die beiden Strips

		Root::Ref<Project> d_pro;
		Root::Ref<LinkFilterRotSpace> d_mdl2D;
		SpinPointSpace* d_src2D;
		Root::Ref<LinkFilterRotSpace> d_mdl3D;
		SpinPointSpace* d_src3D;
		PeakListPeer* d_pl;
		SpinPoint d_ref;
		SpinPoint d_cur;
		typedef std::map<QByteArray ,Root::Ref<Spectrum> >  Sort;
		Sort d_sort2D;
		Sort d_sort3D;
		int d_aol;	// Aktiver Overlay, 0..n-1

		float d_contourFactor;
		ContourView::Option d_contourOption;
		float d_gain;
		Root::Byte d_resol;
		bool d_lowResol;
		bool d_folding;
		bool d_autoCenter;
		bool d_autoRuler;
		bool d_show3DPlane;				// Zeige spec3D in Plane
		bool d_autoHide;
		bool d_cursorSync;
		bool d_autoContour;
		bool d_autoHold;
		bool d_rangeSync;
		bool d_synchro;
		bool d_syncDepth;

		Root::Deque< std::pair<PpmCube,PpmPoint> > d_backward;
		Root::Deque< std::pair<PpmCube,PpmPoint> > d_forward;
		bool d_lock;
		Gui::Menu* d_popPlane;
		Gui::Menu* d_popSpec2D;
		Gui::Menu* d_popStrip;
		Gui::Menu* d_popSpec3D;
		Gui::Menu* d_popLabel;
	};
}

#endif // !defined(AFX_HOMOSCOPEAGENT_H__968502C8_16EB_4753_964A_2A91CCC977D7__INCLUDED_)
