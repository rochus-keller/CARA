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

#if !defined(FourDScope2__INCLUDED_)
#define FourDScope2__INCLUDED_

#include <SpecView3/GenericScope.h>
#include <Spec/PointSet.h>
#include <Spec/SpecProjector.h>
#include <Spec/RangeFilterSpace.h>
#include <Spec/Project.h>
#include <Spec/PeakProjector.h>
#include <SpecView3/ViewAreaMdl3.h>
#include <SpecView3/SpecViewer3.h>
#include <SpecView3/SpinPointView3.h>
#include <SpecView3/ContourView3.h>
#include <SpecView3/SpecBufferMdl3.h>
#include <SpecView3/IntensityView3.h>
#include <SpecView3/PeakPlaneView3.h>
#include <SpecView3/OverviewCtrl3.h>
#include <SpecView3/SpinPoint1DView3.h>
#include <Gui2/AutoMenu.h>
#include <Gui2/SplitGrid2.h>

namespace Spec
{
	class SpecViewer;
	class Project;
	class Spectrum;
    class PeakListPeer;
    class SpinPointSpace;
    class CommandLine2;
    class StripListGadget2;

    class FourDScope2 : public GenericScope
	{
        Q_OBJECT
	public:
        enum { DimCount = 4 };

        struct PlaneSocket
        {
            struct Overlay
            {
                Root::Ref<ContourView3> d_iso;
            };
            SpecViewer3* d_viewer;
            Root::Ref<IntensityView3> d_intens;
            Root::Ref<SpinPointView3> d_points;
            Root::Ref<LinkFilterRotSpace> d_mdl2D;
            Root::Ref<RangeFilterSpaceND> d_mdlND;
            Root::Ref<SpinPoint1DView3> d_hRuler;
            Root::Ref<ManualSpinSpace> d_hRulerMdl;
            Root::Ref<SpinPoint1DView3> d_vRuler;
            Root::Ref<PeakPlaneView3> d_peaks;
            Root::Ref<PeakSubSpaceND> d_peakMdl;
            Root::Ref<ManualSpinSpace> d_vRulerMdl;
            QVector<Overlay> d_ol;
            PlaneSocket():d_viewer(0) {}
        };
        struct SliceSocket
        {
            SpecViewer3* d_viewer;
            Root::Ref<SpecBufferMdl3> d_buf2D;
            Root::Ref<SpecBufferMdl3> d_bufND;
            SliceSocket():d_viewer(0){}
        };
        struct OrthoSocket
        {
            SpecViewer3* d_viewer;
            Root::Ref<ContourView3> d_iso;
            Root::Ref<SpinPointView3> d_points;
            Root::Ref<LinkFilterRotSpace> d_rot;
            Root::Ref<RangeFilterSpaceND> d_range;
            Root::Ref<PeakPlaneView3> d_peaks;
            Root::Ref<PeakSubSpaceND> d_peakMdl;
            OrthoSocket():d_viewer(0){}
        };

        FourDScope2(Root::Agent * supervisor, Spectrum*, Project*);
    public slots:
        void handleSaveColors();
        void handleGoto4D();
        void handleUse4DSpec();
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
        void handleShowPathSim();
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
        void handleSetLinkParams4D();
        void handleSetLinkParams();
        void handleUseLinkColors4D();
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
        void handleEditAttsLink4D();
        void handleEditAttsSys4D();
        void handleRangeSync();
        void handleGotoPeak();
        void handleHidePeak4D();
        void handleGhostLabels();
        void handleShowUnlabeled2();
        void handleShowInfered2( );
        void handleShowLinks2( );
        void handlePickLabel4D();
        void handleAutoHold();
        void handleShowGhosts();
        void handleAutoGain4D();
        void handleAutoGain();
        void handleViewLabels4D();
        void handleDeleteLinks4D();
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
        void handlePrevSpec2D();
        void handleNextSpec2D();
        void handlePrevSpec4D();
        void handleNextSpec4D();
        void handleGotoSystem();
        void handleCursorSync();
        void handleEditAttsSpinX4D();
        void handleEditAttsSpinY4D();
        void handleEditAttsSysV();
        void handleEditAttsSysH();
        void handleEditAttsLink();
        void handleEditAttsSpinV();
        void handleEditAttsSpinH();
        void handleProposeSpin();
        void handleStripCalibrate();
        void handleAutoHide();
        void handleShow4dPlane();
        void handleShowWithOff();
        void handleContourParams2();
        void handleAutoContour2();
        void handleFitWindow4D();
        void handleSetWidth();
        void handleLabelSpin4D();
        void handleDeleteSpins4D();
        void handleMoveSpinAlias4D();
        void handleMoveSpin4D();
        void handlePickSpin4D();
        void handleSelectSpec4D();
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
        void handleDeleteSpinY4D();
        void handleDeleteSpinX4D();
        void handleDeletePeak();
        void handleHidePeak();
        void handleLabelPeak();
        void handleMovePeakAlias();
        void handleMovePeak();
        void handlePickVerti();
        void handlePickHori();
        void handlePickHori4D();
        void handlePickVerti4D();
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
        void handleSave();
        void handleClose();
        void handleUndo();
        void handleRedo();
        void handleHelpAbout();
        void handleExecuteLine();
        void onListActivate();
    protected:
        bool savePeakList();
        int selectLayer();
        void setActiveOverlay( int );
        void initOverlay( int );
        void centerToCursor(bool threeD = false);
        void pickSpin( Dimension, Spin* other, SpinSystem* owner = 0 );
        void extendSystem( Dimension source, Dimension target );
        void setSpec2D( Spectrum* );
        void stepSpec4D( bool next );
        void stepSpec2D( bool next );
        void setSpec4D( Spectrum* );
        void updateRef();
        void initParams();
        void createPopup();
        void createPlane();
        void createOrtho();
        void updatePlaneLabel();
        void initViews();
        void createSlice(Dimension view, Dimension spec);
        void notifyCursor(bool plane = true);
        void updateContour( int i = 0, bool redraw = true );
        void showIntens(bool on );
        void show4dPlaneSlice(Dimension,bool show);
        void updateGlobalCursor( PointMdl::Updated *msg );
        void updateGlobalCursor( CubeMdl::Updated *msg );
        void updateSpecPop2D();
        void updateSpecPop4D();
        void handle(Root::Message&);
        void planeAreaUpdated( ViewAreaMdl3::UpdRange* );
        void selectCurSystem(bool force = false);
        void updateContour4D( bool ac );
        void setShow4dPlane( bool );
        void extendSystem4D( Dimension, Dimension, bool plane = false );
        void specChanged( bool fourD );
    protected:
		Root::Ref<PointSet> createStripPeaks();
		Root::Ref<PointSet> createPlanePeaks();
		virtual ~FourDScope2();
		bool askToCloseWindow() const; // Override
        Spectrum* getSpec() const { return (d_spec2D.isNull())?d_spec4D:d_spec2D; }
        Spin* getSel( bool hori ) const;
        void gotoTuple( SpinSystem*, Spin*, Spin*, bool twoD );
        void setCursor(PpmPoint = PpmPoint());

        bool askToClosePeaklist();

        void setPeakList( PeakList* );
    protected slots:
        void restoreLayout();
    private:
		void buildMenus();
        void buildViews();
	protected:
		virtual void updateCaption();
        Root::Ref<OverviewCtrl3> d_ovCtrl;
        StripListGadget2* d_list;
        SpecRef<Spectrum> d_spec2D;				// 2D Hauptspektrum
        SpecRef<Spectrum> d_spec4D;				// 4D Zusatzspektrum (kann leer sein)
        Root::Ref<Spectrum> d_orig;

        Root::Ref<PointMdl> d_pointMdl;
        Root::Ref<CubeMdl> d_cubeMdl;

        PlaneSocket d_plane;						// Die Plane-Aufsicht
        QVector<SliceSocket> d_slices;
        OrthoSocket d_ortho;                        // Die orthogonale Plane

        Root::Ref<Project> d_pro;
        SpinPointSpace* d_src2D;
        SpinPointSpace* d_src4D;
        PeakListPeer* d_pl;
        SpinPoint d_ref;
        SpinPoint d_cur;
        typedef std::map<QByteArray ,Root::Ref<Spectrum> >  Sort;
        Sort d_sort2D;
        Sort d_sort4D;
        int d_aol;	// Aktiver Overlay, 0..n-1

        ContourView3::Params d_contourParamsTmp;
        Root::Byte d_resol;
        bool d_lowResol;
        bool d_folding;
        bool d_autoCenter;
        bool d_autoRuler;
        bool d_show4DPlane;				// Zeige spec4D in Plane
        bool d_autoHide;
        bool d_cursorSync;
        bool d_autoHold;
        bool d_rangeSync;
        bool d_syncDepth;
        bool d_lock;
        bool d_showList;
        bool d_use4D;				// Verwende 4D-Spektrum in Liste
        bool d_goto4D;				// DblClck und Goto zu 4D

        Root::Deque< std::pair<PpmCube,PpmPoint> > d_backward;
        Root::Deque< std::pair<PpmCube,PpmPoint> > d_forward;
        Gui2::AutoMenu* d_popPlane;
        Gui2::AutoMenu* d_popSpec2D;
        Gui2::AutoMenu* d_popOrtho;
        Gui2::AutoMenu* d_popSpec4D;
        Gui2::SplitGrid2* d_planeGrid;
        Gui2::SplitGrid2* d_orthoGrid;
        CommandLine2* d_cl;
	};

}

#endif // !defined(AFX_FourDScope2_H__BEF0EC37_D637_41A1_A04E_F72AC9EAEA32__INCLUDED_)
