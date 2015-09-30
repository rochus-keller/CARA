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

#if !defined(PolyScope3__INCLUDED_)
#define PolyScope3__INCLUDED_

#include <SpecView3/GenericScope.h>
#include <Root/UndoManager.h>
#include <Root/ActionHandler.h>
#include <Lexi/GlyphWidget.h>
#include <Lexi/FocusManager.h>
#include <Spec/PointSet.h>
#include <SpecView/OverviewCtrl.h>
#include <Gui2/AutoMenu.h>
#include <SpecView/CursorMdl.h>
#include <SpecView/ViewAreaMdl.h>
#include <SpecView/SpecViewer.h>
#include <SpecView/SpinPointView.h>
#include <SpecView/ContourView.h>
#include <SpecView/SpecBufferMdl.h>
#include <SpecView/IntensityView.h>
#include <SpecView/PeakPlaneView.h>
#include <Spec/SpecProjector.h>
#include <Spec/RangeFilterSpace.h>
#include <Spec/Project.h>
#include <Spec/PeakProjector.h>

namespace Spec
{
	class SpecViewer;
	class Project;
	class Spectrum;
	class CursorMdl;
	class ViewAreaMdl;
	class PeakListPeer;
	class SliceView;
	class SpecBufferMdl;
	class SpinPointSpace;
	class PeakPlaneView;
    class StripListGadget2;
    class CommandLine2;

    class PolyScope3 : public GenericScope
	{
        Q_OBJECT
	public:
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
        Gui2::AutoMenu* getPopSpec2D() const { return d_popSpec2D; }
        Gui2::AutoMenu* getPopSpec3D() const { return d_popSpec3D; }
        Gui2::AutoMenu* getPopLabel() { return d_popLabel; }
		LinkFilterRotSpace* getMdl2D() const { return d_mdl2D; }
		LinkFilterRotSpace* getMdl3D() const { return d_mdl3D; }
		Spin* getSel( bool hori ) const;
		void gotoTuple( SpinSystem*, Spin*, Spin*, bool twoD );
		void setCursor(PpmPoint = PpmPoint());

        bool askToClosePeaklist();

		void setPeakList( PeakList* );

		class SpecChanged : public Root::UserMessage
		{
		public:
			SpecChanged( bool threeD ):d_threeD( threeD ) {}
			bool d_threeD;
		};

		PolyScope3(Root::Agent *, Spectrum*, Project*);
    public slots:
        void handleSaveColors();
        void handleGoto3D();
        void handleUse3DSpec();
        void handleSetWidthFactor();
        void handleGotoResidue();
        void handleSyncVerti();
        void handleSyncHori();
        void handleGoto();
        void handleToMonoScope();
        void handleToMonoScope2D();
        void handleSetTolStrip();
        void handleExportStripPeaks();
        void handleSetTolVerti();
        void handleSetTolHori();
        void handleExportPeaklist();
        void handleSetTolerance();
        void handleShowList();
        void handleShowUnknown();
        void handleShowUnknown2();
        void handleAdjustIntensity();
        void handleSyncDepth();
        void handleViewPlLabels();
        void handleGotoPlPeak();
        void handleFitWindowY();
        void handleFitWindowX();
        void handleDeleteAliasPeak();
        void handleMovePlAlias();
        void handleSetPlColor();
        void handleEditPlPeakAtts();
        void handleDeletePlPeaks();
        void handleLabelPlPeak();
        void handleMovePlPeak();
        void handlePickPlPeak();
        void handleMapPeakList();
        void handleSavePeakList();
        void handleOpenPeakList();
        void handleNewPeakList();
        void handleGotoPoint();
        void handleSetLinkParams3D();
        void handleSetLinkParams();
        void handleUseLinkColors3D();
        void handleUseLinkColors();
        void handleComposeLayers();
        void handleAddLayer();
        void handleCntOption();
        void handleCntThreshold();
        void handleCntFactor();
        void handleOverlaySpec();
        void handleSetNegColor();
        void handleSetPosColor();
        void handleActiveOverlay();
        void handleOverlayCount();
        void handleEditAttsLink3D();
        void handleEditAttsSys3D();
        void handleRangeSync();
        void handleGotoPeak();
        void handleHidePeak2();
        void handleGhostLabels();
        void handleSetDepth();
        void handleShowUnlabeled2();
        void handleShowInfered2();
        void handleShowLinks2();
        void handlePickLabel3D();
        void handleAutoHold();
        void handleShowGhosts();
        void handleAutoGain3D();
        void handleAutoGain();
        void handleViewLabels3D();
        void handleDeleteLinks3D();
        void handleForceCross();
        void handleCreateLinks();
        void handleShowUnlabeled();
        void handleShowInfered();
        void handleSetCandidates();
        void handleLabelHori();
        void handleLabelVerti();
        void handleDeleteLinks();
        void handleShowLinks();
        void handleShowWithOff2();
        void handleBackwardPlane();
        void handleForwardPlane();
        void handlePrevSpec2D();
        void handleNextSpec2D();
        void handlePrevSpec3D();
        void handleNextSpec3D();
        void handleGotoSystem();
        void handleCursorSync();
        void handleEditAttsSpin3D();
        void handleEditAttsSysV();
        void handleEditAttsSysH();
        void handleEditAttsLink();
        void handleEditAttsSpinV();
        void handleEditAttsSpinH();
        void handleProposeSpin();
        void handleStripCalibrate();
        void handleAutoHide();
        void handleShow3dPlane();
        void handleShowWithOff();
        void handleContourParams2();
        void handleAutoContour2();
        void handleFitWindow3D();
        void handleSetWidth();
        void handleForceLabelSpin3D();
        void handleLabelSpin3D();
        void handleDeleteSpins3D();
        void handleMoveSpinAlias3D();
        void handleMoveSpin3D();
        void handlePickSpin3D();
        void handleSelectSpec3D();
        void handleProposePeak();
        void handleProposeVerti();
        void handleProposeHori();
        void handleAutoRuler();
        void handleRemoveAllRulers();
        void handleRemoveRulers();
        void handleAddRulerHori();
        void handleAddRulerVerti();
        void handleCreateReport();
        void handleHoldReference();
        void handleUnhidePeak();
        void handleUnlinkSucc();
        void handleUnlinkPred();
        void handleLinkSystems();
        void handleSelectSpec2D();
        void handleViewLabels();
        void handleSetSystemType();
        void handleUnassign();
        void handleAssign();
        void handleShowAlignment();
        void handleShowAllPeaks();
        void handleDeleteSpinY();
        void handleDeleteSpinX();
        void handleDeletePeak();
        void handleHidePeak();
        void handleLabelPeak();
        void handleMovePeakAlias();
        void handleMovePeak();
        void handlePickVerti();
        void handlePickHori();
        void handlePickSystem();
        void handleContourParams();
        void handleAutoContour();
        void handleShowIntensity();
        void handleShowContour();
        void handleAutoCenter();
        void handleSpecCalibrate();
        void handleShowFolded();
        void handleFitWindow();
        void handleBackward();
        void handleForward();
        void handleShowLowRes();
        void handleSetResolution();
    protected slots:
        void onListActivate();
	protected:
		Root::Ref<PointSet> createStripPeaks();
		Root::Ref<PointSet> createPlanePeaks();
		PolyScope3(Root::Agent *);
		virtual ~PolyScope3();
		void handle(Root::Message&);
        bool askToCloseWindow(); // Override
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
		void updatePlane( CursorMdl::Update* );
		void updateSlice( Dimension, CursorMdl::Update * msg );
		void updatePlane( ViewAreaMdl::Update* );
		void updateSlice( Dimension, ViewAreaMdl::Update * msg );
		void selectCurSystem(bool force = false);
		void updateContour2( Dimension d, bool );
		void update3dPlane();
        PolyScope3* getQt() { return this; } // helper für Migration
	private:	
		void buildMenus();
        void buildViews( );
        void buildCommands();
    protected:
		virtual void updateCaption();
		Root::Ref<Lexi::FocusManager> d_focus;
		Lexi::GlyphWidget* d_widget;
		Root::Ref<SpecViewer> d_ov;
		Root::Ref<OverviewCtrl> d_ovCtrl;
        StripListGadget2* d_list;
		bool d_use3D;				// Verwende 3D-Spektrum in Liste
		bool d_goto3D;				// DblClck und Goto zu 3D
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
        Gui2::AutoMenu* d_popPlane;
        Gui2::AutoMenu* d_popSpec2D;
        Gui2::AutoMenu* d_popStrip;
        Gui2::AutoMenu* d_popSpec3D;
        Gui2::AutoMenu* d_popLabel;
        CommandLine2* d_cl;
    };
}

#endif // !defined(PolyScope3__INCLUDED_)
