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

#if !defined(FourDScope3__INCLUDED_)
#define FourDScope3__INCLUDED_

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
#include <SpecView3/PlaneGrid.h>
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
    class SpinPointList;

    class FourDScope3 : public GenericScope
	{
        Q_OBJECT
	public:
        enum { DimCount = 4 };

        FourDScope3(Root::Agent * supervisor, Spectrum*, Project*);
    public slots:
        void handleSaveColors();
        void handleGoto4D();
        void handleUse4DSpec();
        void handleGotoResidue();
        void handleSyncVerti();
        void handleSyncHori();
        void handleGoto();
        void handleOrthoToMonoScope();
        void handlePlaneToMonoScope();
        void handleExportOrthoPeaks();
        void handleSetMatcherTol();
        void handleExportPlanePeaks();
        void handleSetTolerance();
        void handleShowSysList();
        void handleShowPlanePointList();
        void handleShowOrthoPointList();
        void handleShowPathSim();
        void handlePlaneShowUnknown();
        void handleOrthoShowUnknown();
        void handleGotoPlPeak();
        void handleDeleteAliasPeak();
        void handleMovePlAlias();
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
        void handleOrthoUseLinkColors();
        void handlePlaneUseLinkColors();
        void handleSetupPlaneOverlay();
        void handleSetupOrthoOverlay();
        void handleOrthoLinkAttr();
        void handleOrthoSysAttr();
        void handleGotoSpin();
        void handleHidePeak4D();
        void handleOrthoShowUnlabeled();
        void handleOrthoShowInfered( );
        void handleOrthoShowLinks( );
        void handlePickLabel4D();
        void handleAutoHold();
        void handleOrthoShowGhosts();
        void handleDeleteLinks4D();
        void handleDoPlanePathSim();
        void handleDoOrthoPathSim();
        void handleCreateLinks();
        void handlePlaneShowUnlabeled();
        void handlePlaneShowInfered();
        void handleSetCandidates();
        void handleLabelHori();
        void handleLabelVerti();
        void handleDeleteLinks();
        void handlePlaneShowLinks();
        void handlePlaneShowOffs();
        void handlePrevSpec2D();
        void handleNextSpec2D();
        void handlePrevSpec4D();
        void handleNextSpec4D();
        void handleGotoSystem();
        void handleOrthoSpinXAttr();
        void handleOrthoSpinYAttr();
        void handlePlaneSysYAttr();
        void handlePlaneSysXAttr();
        void handlePlaneLinkAttr();
        void handlePlaneSpinYAttr();
        void handlePlaneSpinXAttr();
        void handleProposeSpin4D();
        void handleStripCalibrate();
        void handleAutoHide();
        void handleShowOrthoPlane();
        void handleOrthoShowOffs();
        void handleSetPeakWidth();
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
        void handlePrintPlane();
        void handlePrintOrtho();
        void handleHoldReference();
        void handleUnhidePeak();
        void handleUnlinkSucc();
        void handleUnlinkPred();
        void handleLinkSystems();
        void handleSelectSpec2D();
        void handleSetSystemType();
        void handleUnassign();
        void handleAssign();
        void handleShowAlignment();
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
        void handleAutoCenter();
        void handleSpecCalibrate();
        void handleBackward();
        void handleForward();
        void handleSetPeakColors();
        void handle4dPlaneColors();
        void handleHoldOrthoCur();
		void handleFilterBySystem();
        void onListActivate();
    protected:
        bool savePeakList();
        void pickSpin( Dimension, Spin* other, SpinSystem* owner = 0 );
        void extendSystem( Dimension source, Dimension target );
        void setSpec2D( Spectrum* );
        void stepSpec4D( bool next );
        void stepSpec2D( bool next );
        void setSpec4D( Spectrum* );
        void updateRef();
        void createPopup();
        void createOrtho();
        void updatePlaneLabel();
        void notifyCursor(bool plane = true);
        void update4dPlaneContour();
        void show4dPlaneSlice(bool show);
        void updateSpecPop2D();
        void updateSpecPop4D();
        void handle(Root::Message&);
        void planeAreaUpdated( ViewAreaMdl3::UpdRange* );
        void selectCurSystem(bool force = false);
        void setShow4dPlane( bool );
        void extendSystem4D( Dimension, Dimension, bool plane = false );
        void specChanged( bool fourD );
        void createReport( PlaneGrid* );
    protected:
		Root::Ref<PointSet> createStripPeaks();
		Root::Ref<PointSet> createPlanePeaks();
		virtual ~FourDScope3();
		bool askToCloseWindow() const; // Override
        Spectrum* getSpec() const { return (d_spec2D.isNull())?d_spec4D:d_spec2D; }
        Spectrum* getSpec2() const { return ( d_show4DPlane && !d_spec4D.isNull() )?d_spec4D:d_spec2D; }
        Spin* getSel( bool hori ) const;
        void gotoTuple( SpinSystem*, Spin*, Spin*, bool twoD );
        void setCursor(PpmPoint = PpmPoint());
        bool askToClosePeaklist();
        void setPeakList( PeakList* );
    protected slots:
        void onGoto(Spec::SpinSpace::Element);
        void onShowPlanePoint(Spec::SpinSpace::Element);
        void onShowOrthoPoint(Spec::SpinSpace::Element);
        void onOrthoContourUpd( int );
    private:
		void buildMenus();
        void buildViews();
        void buildCommands();
	protected:
		virtual void updateCaption();
        StripListGadget2* d_sysList;
        SpinPointList* d_planePointList;
        SpinPointList* d_orthoPointList;
        SpecRef<Spectrum> d_spec2D;				// 2D Hauptspektrum
        SpecRef<Spectrum> d_spec4D;				// 4D Zusatzspektrum (kann leer sein)
        Root::Ref<Spectrum> d_orig;

        Root::Ref<PointMdl> d_pointMdl;
        Root::Ref<CubeMdl> d_cubeMdl;

        PlaneGrid* d_plane;						// Die Plane-Aufsicht
        PlaneGrid* d_ortho;                        // Die orthogonale Plane
        QColor d_4dPlanePos, d_4dPlaneNeg;

        Root::Ref<Project> d_pro;
        Root::Ref<SpinPointSpace> d_src2D;
        Root::Ref<SpecRotatedSpace> d_rot2D;
        Root::Ref<RangeFilterSpaceND> d_range2D;
        Root::Ref<SpinPointSpace> d_src4D;
        Root::Ref<SpecRotatedSpace> d_rot4D;
        Root::Ref<RangeFilterSpaceND> d_range4D;
        Root::Ref<PeakSubSpaceND> d_peaks2D;
        Root::Ref<PeakSubSpaceND> d_peaks4D;
        PeakListPeer* d_pl;
        SpinPoint d_ref;
        SpinPoint d_cur;
        typedef std::map<QByteArray ,Root::Ref<Spectrum> >  Sort;
        Sort d_sort2D;
        Sort d_sort4D;

        bool d_autoRuler;
        bool d_show4DPlane;				// Zeige spec4D in Plane
        bool d_autoHide;
        bool d_autoHold;
        bool d_lock;
        bool d_showList;
        bool d_use4D;				// Verwende 4D-Spektrum in Liste
        bool d_goto4D;				// DblClck und Goto zu 4D
		bool d_filterBySystem;

        Root::Deque< std::pair<PpmCube,PpmPoint> > d_backward;
        Root::Deque< std::pair<PpmCube,PpmPoint> > d_forward;
        Gui2::AutoMenu* d_popPlane;
        Gui2::AutoMenu* d_popSpec2D;
        Gui2::AutoMenu* d_popOrtho;
        Gui2::AutoMenu* d_popSpec4D;
        CommandLine2* d_cl;
	};

}

#endif // !defined(AFX_FourDScope3_H__BEF0EC37_D637_41A1_A04E_F72AC9EAEA32__INCLUDED_)
