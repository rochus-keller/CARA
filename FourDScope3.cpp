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

#include "FourDScope3.h"
// Qt
#include <QTextStream>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QMenuBar>
#include <QColor>
#include <QColorDialog>
#include <QtDebug>
#include <QDockWidget>


//* Root
#include <Root/ActionHandler.h>
#include <Root/Command.h>
#include <Root/Application.h>
#include <Root/UpstreamFilter.h>
#include <Root/Any.h>
#include <Root/MakroCommand.h>
#include <Root/MenuAction.h>
using namespace Root;

//* Spec
#include <Spec/Factory.h>
#include <Spec/SpecProjector.h>
#include <Spec/SpectrumPeer.h>
#include <Spec/SpectrumType.h>
#include <Spec/Repository.h>
#include <Spec/SpecRotator.h>
#include <Spec/SpinPointSpace.h>
#include <Spec/PeakListPeer.h>

// SpecView
#include <SpecView/DynValueEditor.h>
#include <SpecView/PeakColorDlg.h>

// SpecView3
#include <SpecView3/ViewStack.h>
#include <SpecView3/CursorView3.h>
#include <SpecView3/SelectZoomCtrl3.h>
#include <SpecView3/FoldingView3.h>
#include <SpecView3/ZoomCtrl3.h>
#include <SpecView3/ScrollCtrl3.h>
#include <SpecView3/CursorCtrl3.h>
#include <SpecView3/PointSelectCtrl3.h>
#include <SpecView3/PeakSelectCtrl3.h>
#include <SpecView3/LabelView.h>
#include <SpecView3/SliceView3.h>
#include <SpecView3/SelectRulerCtrl3.h>
#include <SpecView3/ContourParamDlg.h>
#include <SpecView3/OverlayManager.h>

#include <Gui2/SplitGrid2.h>

// Locals
#include <SingleAlignmentView.h>
#include <SpecView/RotateDlg.h>
#include <Dlg.h>
#include <SpecView/GotoDlg.h>
#include <ReportViewer.h>
#include <SpecView/CandidateDlg.h>
#include <SpecView/SpecBatchList.h>
#include "ProposeSpinDlg.h"
#include <SpecView/PathSimDlg.h>
#include <Dlg.h>
#include <SpecView3/CommandLine2.h>
#include "StripListGadget2.h"
#include <SpecView3/SpinPointList.h>

using namespace Spec;

GCC_IGNORE(-Wparentheses);

//////////////////////////////////////////////////////////////////////
// Entkopplung; nutzbar ohne include

void createFourDScope3(Root::Agent* a, Spec::Spectrum* s, Spec::Project* p)
{ 
	new FourDScope3( a, s, p );
}

//////////////////////////////////////////////////////////////////////

static QColor g_clrLabel = Qt::yellow;
static QColor g_clrSlice4D = Qt::cyan;
static QColor g_clrPeak = Qt::yellow;
static QColor g_clrPoint = Qt::white;


struct Locker
{
    Locker( bool& l ):lock(l) { l = true; }
    ~Locker() { lock = false; }
    bool& lock;
};


static QColor g_frameClr = Qt::lightGray;


FourDScope3::FourDScope3(Root::Agent * supervisor, Spectrum* spec, Project* pro ):
    GenericScope( supervisor ), d_use4D( true ), d_goto4D( false ), d_lock( false ), d_autoRuler( false ),
      d_popSpec2D( 0 ), d_pl(0), d_popSpec4D( 0 ), d_popOrtho(0), d_popPlane(0),
	d_4dPlanePos( g_clrSlice4D ), d_4dPlaneNeg( Qt::darkBlue ),d_filterBySystem(true)
{
    d_show4DPlane = false;
    d_autoHide = true;
    d_autoHold = false;
    // TODO: diese Werte sollen ab Konfigurations-Record gelesen werden

    Q_ASSERT( spec && ( spec->getDimCount() == 2 || spec->getDimCount() == DimCount ) );
    Q_ASSERT( pro );
    d_pro = pro;
    d_pro->addObserver( this );

    d_orig = spec;

    d_pointMdl = new PointMdl();
    d_pointMdl->setNotifyUpdating(true);
    d_pointMdl->addObserver( this );

    d_cubeMdl = new CubeMdl();
    d_cubeMdl->addObserver( this );

    d_src2D = new SpinPointSpace( pro->getSpins(),
                                 pro->getRepository()->getTypes(), false,
                                 true, true, false );
    d_rot2D = new SpecRotatedSpace( d_src2D );
    d_src4D = new SpinPointSpace( pro->getSpins(), pro->getRepository()->getTypes(), false,
                                 true, true, false );
    d_rot4D = new SpecRotatedSpace( d_src4D );
    d_range4D = new RangeFilterSpaceND( d_rot4D, d_pointMdl, DimVector( DimX, DimY ), true );
    d_range2D = new RangeFilterSpaceND( d_rot4D, d_pointMdl, DimVector(DimZ, DimW), true );

    d_peaks2D = new PeakSubSpaceND( new PeakSpaceDummy(2), DimX, DimY, d_pointMdl, DimVector( DimZ, DimW ) );
    d_peaks4D = new PeakSubSpaceND( new PeakSpaceDummy(2), DimW, DimZ, d_pointMdl, DimVector( DimX, DimY ) );

    d_cl = new CommandLine2( this );
    installEventFilter( d_cl );
    d_plane = new PlaneGrid(this, DimX, DimY, d_pointMdl, d_cubeMdl );
    d_plane->setObjectName( "Plane" );
    d_plane->getPlaneViewer()->installEventFilter( d_cl );
    d_plane->getOvViewer()->installEventFilter( d_cl );
    d_plane->getSliceViewerX()->installEventFilter( d_cl );
    d_plane->getSliceViewerY()->installEventFilter( d_cl );
    d_plane->setPeaks( d_peaks2D );
    d_plane->installOverlay( 0, true, true );
    d_plane->setSliceColor(g_clrSlice4D, 0);
    d_ortho = new PlaneGrid( this, DimW, DimZ, d_pointMdl, d_cubeMdl );
    d_ortho->setObjectName( "Ortho" );
    d_ortho->getPlaneViewer()->installEventFilter( d_cl );
    d_ortho->getOvViewer()->installEventFilter( d_cl );
    d_ortho->getSliceViewerX()->installEventFilter( d_cl );
    d_ortho->getSliceViewerY()->installEventFilter( d_cl );
    d_ortho->setPeaks( d_peaks4D );
	d_ortho->installOverlay( 0, false, true, true );
	d_ortho->setSliceColor(g_clrSlice4D, 0);
	connect( d_ortho, SIGNAL(contourParamsUpdated(int)), this, SLOT(onOrthoContourUpd(int)) );
    updatePlaneLabel();

    createPopup();
    d_popPlane->connectPopup( d_plane->getPlaneViewer() );
    d_popOrtho->connectPopup( d_ortho->getPlaneViewer() );
    buildMenus();
    buildCommands();
    showMaximized();
	buildViews();
    if( spec->getDimCount() == 2 )
    {
        setSpec2D( spec );
        d_pointMdl->setPos( DimX, spec->getScale( DimX ).getRange().first, DimY, spec->getScale( DimY ).getRange().first );
        d_plane->fitWindow();
    }else
    {
        d_show4DPlane = true;
        d_plane->setOvSource(0);
        setSpec4D( spec );
        d_pointMdl->setPos( DimZ, spec->getScale( DimZ ).getRange().first, DimW, spec->getScale( DimW ).getRange().first );
        d_plane->fitWindow(0);
    }
    updateSpecPop2D();
    updateSpecPop4D();
    updateCaption();
    setCursor();
}

FourDScope3::~FourDScope3()
{
    d_pro->removeObserver( this );

    if( d_popSpec2D )
        delete d_popSpec2D;
    if( d_popSpec4D )
        delete d_popSpec4D;
    if( d_popPlane )
        delete d_popPlane;
    if( d_popOrtho )
        delete d_popOrtho;
}

void FourDScope3::createPopup()
{
    d_popSpec2D = new Gui2::AutoMenu( tr("Select Spectrum") );
    d_popSpec4D = new Gui2::AutoMenu( tr("Select Spectrum"));

    d_popPlane = new Gui2::AutoMenu( tr("Plane") );

    Gui2::AutoMenu* menuPeaks = new Gui2::AutoMenu( "Peaks", d_popPlane );
    menuPeaks->addCommand( "&Pick Peak", this, SLOT( handlePickPlPeak() ) );
    menuPeaks->addCommand( "&Move Peak", this, SLOT( handleMovePlPeak() ) );
    menuPeaks->addCommand( "&Move Peak Alias", this, SLOT( handleMovePlAlias() ) );
    menuPeaks->addCommand( "&Label Peak...", this, SLOT( handleLabelPlPeak() ) );
    menuPeaks->addCommand( "&Delete Peaks", this, SLOT( handleDeletePlPeaks() ) );
    menuPeaks->addCommand( "&Edit Attributes...", this, SLOT( handleEditPlPeakAtts() ) );
    menuPeaks->addSeparator();
    menuPeaks->addCommand( "&Open Peaklist...", this, SLOT( handleOpenPeakList() ) );

    d_popPlane->addCommand( "Hold Reference", this, SLOT( handleHoldReference() ) );
    d_popPlane->addCommand( "Add Vertical Ruler", d_plane, SLOT( handleAddRulerHori()) );
    d_popPlane->addCommand( "Add Horizontal Ruler", d_plane, SLOT( handleAddRulerVerti() ) );
    d_popPlane->addCommand( "Remove All Rulers", d_plane, SLOT( handleRemoveAllRulers() ) );
    d_popPlane->addMenu( d_popSpec2D );
    d_popPlane->addMenu( menuPeaks );
    d_popPlane->addSeparator();
    d_popPlane->addCommand( "&Pick New System", this, SLOT( handlePickSystem()) );
    d_popPlane->addCommand( "Propose System...", this, SLOT( handleProposePeak()) );
    d_popPlane->addCommand( "Extend Horizontally...", this, SLOT( handlePickHori()) );
    d_popPlane->addCommand( "Propose Horizontally...", this, SLOT( handleProposeHori()) );
    d_popPlane->addCommand( "Extend Vertically...", this, SLOT( handlePickVerti()) );
    d_popPlane->addCommand( "Propose Vertically...", this, SLOT( handleProposeVerti()) );
    d_popPlane->addCommand( "&Move Spins", this, SLOT( handleMovePeak()));
    d_popPlane->addCommand( "Move Spin &Aliases", this, SLOT( handleMovePeakAlias()) );
    d_popPlane->addCommand( "&Label Spins...", this, SLOT( handleLabelPeak()) );
    d_popPlane->addCommand( "Hide/Show Link", this, SLOT( handleHidePeak()) );
    d_popPlane->addCommand( "Set System &Type...", this, SLOT( handleSetSystemType()) );
    d_popPlane->addCommand( "Set Link Params...", this, SLOT( handleSetLinkParams()) );

    Gui2::AutoMenu* menuAtts = new Gui2::AutoMenu( "&Edit Attributes", d_popPlane );
    menuAtts->addCommand( "Horizontal Spin...", this, SLOT( handlePlaneSpinXAttr() ) );
    menuAtts->addCommand( "Vertical Spin...", this, SLOT( handlePlaneSpinYAttr() ) );
    menuAtts->addCommand( "Horizontal System...", this, SLOT( handlePlaneSysXAttr() ) );
    menuAtts->addCommand( "Vertical System...", this, SLOT( handlePlaneSysYAttr() ) );
    menuAtts->addCommand( "Spin Link...", this, SLOT( handlePlaneLinkAttr() ) );
    d_popPlane->addMenu( menuAtts );

    d_popPlane->addSeparator();
    d_popPlane->addCommand( "Un-Alias Peaks", this, SLOT( handleDeleteAliasPeak()) );
    d_popPlane->addCommand( "Delete Peaks", this, SLOT( handleDeletePeak()) );
    d_popPlane->addCommand( "Delete Spin Links", this, SLOT( handleDeleteLinks()) );
    d_popPlane->addCommand( "Delete Horizontal Spin", this, SLOT( handleDeleteSpinX()) );
    d_popPlane->addCommand( "Delete Vertical Spin", this, SLOT( handleDeleteSpinY()) );
    d_popPlane->addSeparator();

    // ->addCommand( "Forward", Forward, false );
    d_popPlane->addCommand( "Set Peak Width...", this, SLOT( handleSetPeakWidth() ) );
    d_popPlane->addCommand( "Show Alignment...", this, SLOT( handleShowAlignment() ) );
    d_popPlane->addCommand( "Show Path Simulation...", this, SLOT( handleShowPathSim() ) );
    d_popPlane->addCommand( "Backward", this, SLOT( handleBackward()) );
    d_popPlane->addCommand( "Show 4D D1/D2 Plane", this, SLOT( handleShowOrthoPlane()) );
    d_popPlane->addCommand( "Fit Window", d_plane, SLOT( handleFitWindow()));

    d_popOrtho = new Gui2::AutoMenu("Select Spectrum");

    d_popOrtho->addMenu( d_popSpec4D );
    d_popOrtho->addMenu( menuPeaks );
    d_popOrtho->addSeparator();
    d_popOrtho->addCommand( "&Pick Spins", this, SLOT( handlePickSpin4D() ) );
    d_popOrtho->addCommand( "Pick Labels...", this, SLOT( handlePickLabel4D() ) );
    d_popOrtho->addCommand( "Extend Horizontally...", this, SLOT( handlePickHori4D() ) );
    d_popOrtho->addCommand( "Extend Vertically...", this, SLOT( handlePickVerti4D() ) );
    d_popOrtho->addCommand( "&Propose Spins...", this, SLOT( handleProposeSpin4D() ) );
    d_popOrtho->addCommand( "&Move Spins", this, SLOT( handleMoveSpin4D() ) );
    d_popOrtho->addCommand( "&Move Spin Aliasses", this, SLOT( handleMoveSpinAlias4D() ) );
    d_popOrtho->addCommand( "Label Spins...", this, SLOT( handleLabelSpin4D() ) );
    d_popOrtho->addCommand( "Set Link Params...", this, SLOT( handleSetLinkParams4D() ) );
    d_popOrtho->addSeparator();
    d_popOrtho->addCommand( "&Delete Spins", this, SLOT( handleDeleteSpins4D() ) ); // TODO: Delete X und Y
    d_popOrtho->addCommand( "Delete Horizontal Spin", this, SLOT( handleDeleteSpinX4D() ) );
    d_popOrtho->addCommand( "Delete Vertical Spin", this, SLOT( handleDeleteSpinY4D() ) );
    d_popOrtho->addCommand( "Delete Links", this, SLOT( handleDeleteLinks4D() ) );

    menuAtts = new Gui2::AutoMenu( "&Edit Attributes", d_popOrtho );
    menuAtts->addCommand( "Horizontal Spin...", this, SLOT( handleOrthoSpinXAttr() ) );
    menuAtts->addCommand( "Vertical Spin...", this, SLOT( handleOrthoSpinYAttr() ) );
    menuAtts->addCommand( "System...", this, SLOT( handleOrthoSysAttr() ) );
    menuAtts->addCommand( "Spin Link...", this, SLOT( handleOrthoLinkAttr() ) );
    d_popOrtho->addMenu( menuAtts );
    d_popOrtho->addSeparator();
	d_popOrtho->addCommand( tr("Hold Slices"), this,SLOT(handleHoldOrthoCur() ), tr("CTRL+0") );
	d_popOrtho->addCommand( "Set Peak Width...", this, SLOT( handleSetPeakWidth() ) );
    d_popOrtho->addCommand( "Show 4D D1/D2 Plane", this, SLOT( handleShowOrthoPlane() ) );
    d_popOrtho->addCommand( "Show Path Simulation...", this, SLOT( handleShowPathSim() ) );
    d_popOrtho->addCommand( "Fit Window", d_ortho, SLOT( handleFitWindow() ) );
}

void FourDScope3::buildCommands()
{
    d_cl->addCommand( this, SLOT( handleAutoCenter() ), "CC", "Center To Peak", true );
    d_cl->addCommand( this, SLOT( handleUndo() ), "ZZ", "Undo", true );
    d_cl->addCommand( this, SLOT( handleRedo() ), "YY", "Redo", true );

    d_cl->addCommand( this, SLOT( handlePickPlPeak() ), "PP", "Pick Peak", true );
    d_cl->addCommand( this, SLOT( handleMovePlPeak() ), "MP", "Move Peak", true );
    d_cl->addCommand( this, SLOT( handleMovePlAlias() ), "MA", "Move Peak Alias", true );
    d_cl->addCommand( this, SLOT( handleDeletePlPeaks() ), "DP", "Delete Peaks", true );
    d_cl->addCommand( this, SLOT( handleLabelPlPeak() ), "LP", "Label Peak", true );

    d_cl->addCommand( this, SLOT( handleDeleteAliasPeak() ), "UA", "Un-Alias Spins", true );
    d_cl->addCommand( this, SLOT( handleGotoPoint() ), "GT", "Goto", true );

    d_cl->addCommand( d_plane, SLOT( handleAdjustIntensity() ), "CW", "Adjust Intensity", true );
    d_cl->addCommand( d_ortho, SLOT( handleAdjustIntensity() ), "ZCW", "Adjust Intensity", true );
    d_cl->addCommand( d_plane, SLOT( handleRemoveRulers() ), "RS", "Remove Selected Rulers", true );
    d_cl->addCommand( d_plane, SLOT( handleRemoveAllRulers() ), "RA", "Remove All Rulers", true );
    d_cl->addCommand( d_plane, SLOT( handleAddRulerVerti() ), "RH", "Add Horizontal Ruler", true );
    d_cl->addCommand( d_plane, SLOT( handleAddRulerHori() ), "RV", "Add Vertical Ruler", true );
    d_cl->addCommand( d_ortho, SLOT( handleRemoveRulers() ), "ZRS", "Remove Selected Rulers", true );
    d_cl->addCommand( d_ortho, SLOT( handleRemoveAllRulers() ), "ZRA", "Remove All Rulers", true );
    d_cl->addCommand( d_ortho, SLOT( handleAddRulerVerti() ), "ZRH", "Add Horizontal Ruler", true );
    d_cl->addCommand( d_ortho, SLOT( handleAddRulerHori() ), "ZRV", "Add Vertical Ruler", true );
    d_cl->addCommand( this, SLOT( handleHoldReference() ), "HR", "Hold Reference", true );
    d_cl->addCommand( d_plane, SLOT( handleShowFolded() ), "SF", "Show Folded", true );
    d_cl->addCommand( d_ortho, SLOT( handleShowFolded() ), "ZSF", "Show Folded", true );
    d_cl->addCommand( this, SLOT( handleShowOrthoPlane() ), "4", "Show 4D D1/D2 Plane", true ); // vormals 4D
    d_cl->addCommand( this, SLOT( handleBackward() ), "BB", "Backward", true );
    d_cl->addCommand( this, SLOT( handleShowSysList() ), "SL", "Show System List", true );
    d_cl->addCommand( d_plane, SLOT( handleGlobalCursorSync() ), "GC", "Sync To Global Cursor", true );
    d_cl->addCommand( d_plane, SLOT( handleFitWindowX() ), "WX", "Fit X To Window", true );
    d_cl->addCommand( d_plane, SLOT( handleFitWindowY() ), "WY", "Fit Y To Window", true );
    d_cl->addCommand( d_plane, SLOT( handleFitWindow() ), "WW", "Plane Fit To Window", true );
    d_cl->addCommand( d_ortho, SLOT( handleGlobalCursorSync() ), "ZGC", "Sync To Global Cursor", true );
    d_cl->addCommand( d_ortho, SLOT( handleFitWindowX() ), "ZWX", "Fit X To Window", true );
    d_cl->addCommand( d_ortho, SLOT( handleFitWindowY() ), "ZWY", "Fit Y To Window", true );
    d_cl->addCommand( d_ortho, SLOT( handleFitWindow() ), "ZWW", "Ortho Fit To Window", true );
    d_cl->addCommand( d_plane, SLOT( handleShowContour() ), "SC", "Show Contour", true );
    d_cl->addCommand( d_plane, SLOT( handleShowIntensity() ), "SI", "Show Intensity", true );
    d_cl->addCommand( d_ortho, SLOT( handleShowContour() ), "ZSC", "Show Contour", true );
    d_cl->addCommand( d_ortho, SLOT( handleShowIntensity() ), "ZSI", "Show Intensity", true );
    d_cl->addCommand( d_plane, SLOT( handleAutoContour() ), "AC", "Auto Contour Level", true );
    d_cl->addCommand( d_ortho, SLOT( handleAutoContour() ), "ZAC", "Auto Contour Level", true );
	d_cl->addCommand( this, SLOT(handleHoldOrthoCur()), "0", "Ortho Hold Slice", true );

    CommandLine2::Command* cmd = d_cl->addCommand( d_plane, SLOT( handleContourParams() ), "CP", "Contour Params", false );
    cmd->registerParameter( Any::CStr, false ); // Leer, * oder Nr

    cmd = d_cl->addCommand( d_ortho, SLOT( handleContourParams() ), "ZCP", "Contour Params", false );
    cmd->registerParameter( Any::CStr, false ); // Leer, * oder Nr

    cmd = d_cl->addCommand( this, SLOT( handleSetTolerance() ), "ST", "Set Tolerance", false );
    cmd->registerParameter( Any::CStr, true, "Atom Type" );
    cmd->registerParameter( Any::Float, true, "Tolerance (ppm)" );

    cmd = d_cl->addCommand( this, SLOT( handleLinkSystems() ), "LK", "Link Systems", false );
    cmd->registerParameter( Any::ULong, false, "Predecessor" ); // 1..n Pred, Succ oder Nil
    cmd->registerParameter( Any::ULong, false, "Successor" ); // 1..n Succ oder Nil

    cmd = d_cl->addCommand( d_plane, SLOT( handleViewLabels() ), "LL", "Spin Label Format", false );
    cmd->registerParameter( Any::Long, false ); // Leer oder ID

    cmd = d_cl->addCommand( d_plane, SLOT( handleViewPlLabels() ), "LF", "Peak Label Format", false );
    cmd->registerParameter( Any::Long, false ); // Leer oder ID

    cmd = d_cl->addCommand( d_ortho, SLOT( handleViewLabels() ), "ZLL", "Spin Label Format", false );
    cmd->registerParameter( Any::Long, false ); // Leer oder ID

    cmd = d_cl->addCommand( d_ortho, SLOT( handleViewPlLabels() ), "ZLF", "Peak Label Format", false );
    cmd->registerParameter( Any::Long, false ); // Leer oder ID

    d_cl->addCommand( this, SLOT( handleSetLinkParams() ), "PA", "Set Plane Link Params", true );
    d_cl->addCommand( this, SLOT( handleSetLinkParams4D() ), "ZPA", "Set Ortho Link Params", true );
    d_cl->addCommand( this, SLOT( handleSetSystemType() ), "SY", "Set System Type", true );
    d_cl->addCommand( this, SLOT( handleSyncHori() ), "SH", "Sync. Hori. Spin", true );
    d_cl->addCommand( this, SLOT( handleSyncVerti() ), "SV", "Sync. Verti. Spin", true );

    d_cl->addCommand( this, SLOT( handlePickSystem() ), "PY", "Plane Pick System", true );
    d_cl->addCommand( this, SLOT( handleProposePeak() ), "PR", "Plane Propose Spins", true );
    d_cl->addCommand( this, SLOT( handleMovePeak() ), "MS", "Plane Move Spins", true );
    d_cl->addCommand( this, SLOT( handleMovePeakAlias() ), "AY", "Plane Move System Alias", true );
    d_cl->addCommand( this, SLOT( handlePickHori() ), "EH", "Plane Extend Horizontally", true );
    d_cl->addCommand( this, SLOT( handleProposeHori() ), "PH", "Propose Horizontally", true );
    d_cl->addCommand( this, SLOT( handlePickVerti() ), "EV", "Plane Extend Vertically", true );
    d_cl->addCommand( this, SLOT( handleProposeVerti() ), "PV", "Propose Vertically", true );
    d_cl->addCommand( this, SLOT( handleLabelPeak() ), "LS", "Label Spins", true );
    d_cl->addCommand( this, SLOT( handleDeletePeak() ), "DS", "Delete Spins", true );
    d_cl->addCommand( this, SLOT( handleDeleteLinks() ), "DL", "Delete Spin Links", true );

    d_cl->addCommand( this, SLOT( handlePickSpin4D() ), "PO", "Ortho Pick Peak", true );
    d_cl->addCommand( this, SLOT( handleProposeSpin4D() ), "RI", "Ortho Propose Spins", true );
    d_cl->addCommand( this, SLOT( handleMoveSpin4D() ), "MO", "Ortho Move Peak", true );
    d_cl->addCommand( this, SLOT( handleMoveSpinAlias4D() ), "OA", "Ortho Move Peak Alias", true );
    d_cl->addCommand( this, SLOT( handlePickHori4D() ), "ZEH", "Ortho Extend Horizontally", true );
    d_cl->addCommand( this, SLOT( handlePickVerti4D() ), "ZEV", "Ortho Extend Vertically", true );

    d_cl->addCommand( this, SLOT( handlePlaneShowOffs() ), "RP", "Resolve Projected Spins", true );

    cmd = d_cl->addCommand( this, SLOT( handleGotoSystem() ), "GY", "Goto System", false );
    cmd->registerParameter( Any::CStr, false ); // Leer oder ID

    cmd = d_cl->addCommand( this, SLOT( handleGotoSpin() ), "GS", "Goto Spins", false );
    cmd->registerParameter( Any::CStr, false ); // Leer oder ID

    cmd = d_cl->addCommand( this, SLOT( handleGotoPlPeak() ), "GP", "Goto Peak", false );
    cmd->registerParameter( Any::Long, false ); // Leer oder ID

    d_cl->addCommand( this, SLOT( handleSetupPlaneOverlay() ), "CL", "Compose Overlay Layers", true );
    d_cl->addCommand( this, SLOT( handleSetupOrthoOverlay() ), "ZCL", "Compose Overlay Layers", true );

    cmd = d_cl->addCommand( this, SLOT( handleGotoResidue() ), "GR", "Goto Residue", false );
    cmd->registerParameter( Any::Long, false ); // Leer oder ID

    d_cl->addCommand( this, SLOT( handleNextSpec2D() ), "NS", "Next Spectrum", true );
    d_cl->addCommand( this, SLOT( handlePrevSpec2D() ), "PS", "Prev. Spectrum", true );
    d_cl->addCommand( this, SLOT( handleNextSpec4D() ), "ZNS", "Ortho Next Spectrum", true );
    d_cl->addCommand( this, SLOT( handlePrevSpec4D() ), "ZPS", "Ortho Prev. Spectrum", true );

    cmd = d_cl->addCommand( this, SLOT( handleLabelHori() ), "LH", "Label Horizontal Spin", false );
    cmd->registerParameter( Any::CStr, false ); // Label oder leer

    cmd = d_cl->addCommand( this, SLOT( handleLabelVerti() ), "LV", "Label Vertical Spin", false );
    cmd->registerParameter( Any::CStr, false ); // Label oder leer

    cmd = d_cl->addCommand( this, SLOT( handlePickLabel4D() ), "PL", "Ortho Pick Labels", true );

    cmd = d_cl->addCommand( this, SLOT( handleLabelSpin4D() ),"LZ", "Label Ortho Spins", true );

    cmd = d_cl->addCommand( this, SLOT( handleExecuteLine() ), "LUA", "Lua", false );
    cmd->registerParameter( Any::Memo, true );

    cmd = d_cl->addCommand( d_plane, SLOT( handleAutoGain() ), "AG", "Set Auto Contour Gain", false );
    cmd->registerParameter( Any::Float, true );
    cmd->registerParameter( Any::CStr, false ); // Label oder leer

    cmd = d_cl->addCommand( d_ortho, SLOT( handleAutoGain() ), "ZAG", "Set Ortho Auto Contour Gain", false );
    cmd->registerParameter( Any::Float, true );
    cmd->registerParameter( Any::CStr, false ); // Label oder leer

    cmd = d_cl->addCommand( this, SLOT( handleSetPeakWidth() ), "SW", "Set Peak Width", true );

    d_cl->addCommand( this, SLOT( handleShowPathSim() ), "SP", "Show Path Simulation", true );
}

void FourDScope3::updateSpecPop2D()
{
    if( d_popSpec2D == 0 )
        return;

    d_popSpec2D->clear();
    ColorMap a, b;
    if( !d_spec2D.isNull() )
        d_spec2D->getColors(a);
    else if( !d_spec4D.isNull() )
        d_spec4D->getColors(a);
    Project::SpecSet l;
    Spectrum* spec = 0;
    const Project::SpectrumMap& sm = d_pro->getSpectra();
    Project::SpectrumMap::const_iterator p;
    if( d_orig->getDimCount() == 2 )
        l.insert( d_orig );
    for( p = sm.begin(); p != sm.end(); ++p )
    {
        spec = (*p).second;
        spec->getColors( b );
        if( spec->getDimCount() == 2 &&
            a[ DimX ] == b[ DimX ] && a[ DimY ] == b[ DimY ] &&
            spec->getId() != d_orig->getId() )
            l.insert( spec );
    }
    if( d_spec2D == d_spec4D )
    {
        d_popSpec2D->addCommand( d_spec4D->getName(), this,
                                 SLOT( handleSelectSpec2D() ) )->
                setProperty("0", QVariant::fromValue(d_spec4D.deref() ) );
    }
    Sort::const_iterator pp1;
    Project::SpecSet::const_iterator p1;
    for( p1 = l.begin(); p1 != l.end(); ++p1 )
        d_sort2D[ (*p1)->getName() ] = (*p1);
    for( pp1 = d_sort2D.begin(); pp1 != d_sort2D.end(); ++pp1 )
    {
        d_popSpec2D->addCommand( (*pp1).first.data(),
                                 this, SLOT( handleSelectSpec2D() ) )->
                setProperty("0", QVariant::fromValue((*pp1).second.deref()) );
    }
}

void FourDScope3::updateSpecPop4D()
{
    d_popSpec4D->clear();
    ColorMap a, b;
    if( !d_spec2D.isNull() )
        d_spec2D->getColors(a);
    else if( !d_spec4D.isNull() )
        d_spec4D->getColors(a);
    Project::SpecSet l;
    Spectrum* spec = 0;
    if( a.size() == DimCount )
    {
        a[ DimZ ] = AtomType(); // Joker
        a[ DimW ] = AtomType(); // Joker
    }else
    {
        a.push_back( AtomType() );
        a.push_back( AtomType() );
    }

    const Project::SpectrumMap& sm = d_pro->getSpectra();
    Project::SpectrumMap::const_iterator p;
    Rotation rot;
    if( d_orig->getDimCount() == DimCount )
        l.insert( d_orig );
    for( p = sm.begin(); p != sm.end(); ++p )
    {
        spec = (*p).second;
        if( spec->getDimCount() == DimCount && spec->getId() != d_orig->getId() )
        {
            spec->getColors( b );
            if( a[ DimX ] == b[ DimX ] && a[ DimY ] == b[ DimY ] )
                l.insert( spec );
            else
            {
                if( SpectrumType::autoRotate( a, spec, rot, false ) ) // Keine Auflsungsoptimierung
                {
                    l.insert( new SpecRotator( spec, rot ) );
                }
            }
        }
    }
    Project::SpecSet::const_iterator p1;
    Sort::const_iterator pp1;
    for( p1 = l.begin(); p1 != l.end(); ++p1 )
        d_sort4D[ (*p1)->getName() ] = (*p1);
    for( pp1 = d_sort4D.begin(); pp1 != d_sort4D.end(); ++pp1 )
    {
        d_popSpec4D->addCommand( (*pp1).first.data(),
                                 this, SLOT( handleSelectSpec4D() ) )->
                setProperty("0", QVariant::fromValue((*pp1).second.deref()) );
    }
}

void FourDScope3::update4dPlaneContour()
{
    ContourView3::Params p = d_ortho->getContourParams();
    if( d_spec4D )
        p.d_threshold = d_spec4D->getThreshold();
    p.d_pos = d_4dPlanePos;
    p.d_neg = d_4dPlaneNeg;
    d_plane->setContourParams( p, 0 );
}

bool FourDScope3::askToClosePeaklist()
{
    if( d_pl == 0 ||
        d_pl && ( d_pl->getId() != 0 || !d_pl->getPeakList()->isDirty() ) )
        return true;
    switch( QMessageBox::warning( this, "About to close peaklist",
            "Do you want to save the peaklist in the repository before closing?",
                                          "&Save", "&Don't Save", "&Cancel",
                                          0,		// Enter == button 0
                                          2 ) )		// Escape == button 2
    {
    case 0:
        return savePeakList(); // Do it with action.
    case 1:
        return true;	// Do it without action
    default:
        return false;	// Don't do it.
    }
}

bool FourDScope3::savePeakList()
{
    bool ok;
    QString res = QInputDialog::getText( "Name Peaklist",
        "Please enter a unique short name:", QLineEdit::Normal,
        d_pl->getName().data(), &ok, this );
    if( !ok )
        return false;
    if( res.isEmpty() || d_pro->findPeakList( res.toLatin1() ) != 0 )
    {
        QMessageBox::critical( this, "Save Peaklist",
                "This peaklist name is already used!", "&Cancel" );
        return false;
    }
    d_pl->getPeakList()->setName( res.toLatin1() );
    d_pro->addPeakList( d_pl->getPeakList() );
    d_pl->getPeakList()->clearDirty();
    return true;
}

void FourDScope3::setCursor( PpmPoint p)
{
    if( p.size() == 0 )
    {
        p.assign( getSpec()->getDimCount(), 0 );
        for( int i = 0; i < p.size(); i++ )
            p[ i ] = getSpec()->getScale( i ).getIdxN();
    }
    for( Dimension d = 0; d < p.size(); d++ )
        d_pointMdl->setPos( d, p[ d ] );

    d_plane->ensureCursorVisible();
    show4dPlaneSlice( true );
    selectCurSystem();
    notifyCursor( true );
}

void FourDScope3::planeAreaUpdated(ViewAreaMdl3::UpdRange *msg)
{
    // In Plane wurde Ausschnitt ge�dert
    PpmCube cube;
    cube.assign( 2, PpmRange() );
    cube[ DimX ] = d_plane->getPlaneViewer()->getArea()->getRange( DimX );
    cube[ DimY ] = d_plane->getPlaneViewer()->getArea()->getRange( DimY );
    d_backward.push_back( std::make_pair( cube, d_pointMdl->getPpmPoint(DimCount) ) );
}

void FourDScope3::updatePlaneLabel()
{
    Spectrum* spec = d_plane->getSpectrum();
    if( !d_spec4D.isNull() && d_show4DPlane )
        spec = d_spec4D;
    QString str;
    if( spec == 0 )
        str.sprintf( "<empty>" );
//    else if( d_plane->getOverlayCount() > 0 )
//        str.sprintf( " %d %s  %s / %s", d_aol, spec->getName(), spec->getDimName( DimX ), spec->getDimName( DimY ) );
    else
        str.sprintf( " %s  %s / %s", spec->getName(), spec->getDimName( DimX ), spec->getDimName( DimY ) );

    d_plane->setLabelTop( str, (d_show4DPlane)?g_clrSlice4D:g_clrPoint );
}

void FourDScope3::setSpec4D(Spectrum * spec)
{
    Q_ASSERT( spec == 0 || spec->getDimCount() == DimCount );

    if( d_spec4D == spec )
        return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    Spectrum* old = d_spec4D;
    d_spec4D = spec;
    if( !d_spec4D.isNull() )
    {
        d_ortho->setSpectrum( new SpecProjector( d_spec4D, d_pointMdl, DimW, DimZ ) );

        if( old == 0 || old->getColor( DimZ ) != spec->getColor( DimZ ) ||
            !old->getScale( DimZ ).getRange().intersects( spec->getScale( DimZ ).getRange() ) ||
            old->getColor( DimW ) != spec->getColor( DimW ) ||
            !old->getScale( DimW ).getRange().intersects( spec->getScale( DimW ).getRange()) )
        {
            // Nur FitWindow, wenn andere Farbe oder Bereich nicht berlappend
            d_ortho->fitWindow();
        }

        d_src4D->setSpec( d_spec4D );
        d_rot4D->setSpec( d_spec4D );
        d_peaks4D->setRefSpec( d_spec4D );
        d_ortho->setPoints( new RotatedSpace( d_range4D, Rotation( DimW, DimZ, DimX, DimY ) ) );

        d_ortho->setLabelTop( tr("%1 %2/%3").arg( d_spec4D->getName() ).
                              arg(d_spec4D->getDimName( DimW )).arg(d_spec4D->getDimName( DimZ )), g_clrSlice4D );
    }else // if d_spec4D.isNull()
    {
        d_ortho->setSpectrum( 0 );
        d_ortho->setLabelTop( QString() );
        d_ortho->setPoints(0);
        d_src4D->setSpecType( 0 );
    }
    setShow4dPlane( d_show4DPlane );
    selectCurSystem( true );
    QApplication::restoreOverrideCursor();
    specChanged( true );
}

void FourDScope3::stepSpec2D(bool next)
{
    if( d_sort2D.size() < 2 )
        return;
    Sort::const_iterator p = d_sort2D.end();
    if( d_spec2D )
        p = d_sort2D.find( d_spec2D->getName() );
    if( p == d_sort2D.end() )
    {
        if( next )
            p = d_sort2D.begin();
        else
            --p;
    }else
    {
        if( next )
        {
            ++p;
            if( p == d_sort2D.end() )
                p = d_sort2D.begin();
        }else if( p == d_sort2D.begin() )
        {
            p = d_sort2D.end();
            --p;
        }else
            --p;
    }
    Q_ASSERT( p != d_sort2D.end() );
    setSpec2D( (*p).second );
}

void FourDScope3::stepSpec4D(bool next)
{
    if( d_sort4D.size() < 2 )
        return;
    Sort::const_iterator p = d_sort4D.end();
    if( d_spec4D )
        p = d_sort4D.find( d_spec4D->getName() );
    if( p == d_sort4D.end() )
    {
        if( next )
            p = d_sort4D.begin();
        else
            --p;
    }else
    {
        if( next )
        {
            ++p;
            if( p == d_sort4D.end() )
                p = d_sort4D.begin();
        }else if( p == d_sort4D.begin() )
        {
            p = d_sort4D.end();
            --p;
        }else
            --p;
    }
    Q_ASSERT( p != d_sort4D.end() );
    setSpec4D( (*p).second );
}

void FourDScope3::setSpec2D(Spectrum * spec )
{
    Q_ASSERT( spec == 0 || spec->getDimCount() == 2 );

    if( d_spec2D == spec )
        return;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    d_spec2D = spec;
    d_plane->setOvSource(-1);

    d_src2D->setSpec( d_spec2D );
    d_rot2D->setSpec( d_spec2D );
    d_plane->setSpectrum( d_spec2D );

    setShow4dPlane( false );

    QApplication::restoreOverrideCursor();
}

void FourDScope3::setPeakList(PeakList * pl)
{
    if( pl )
    {
        d_pl = new PeakListPeer( pl );
        d_peaks2D->setPeakSpace( d_pl );
        d_peaks4D->setPeakSpace( d_pl );
        QString str;
        str.sprintf( "%dD %s", d_pl->getDimCount(), pl->getName().data() );
        d_plane->setLabelBottomRight( str, g_clrPeak );
    }else
    {
        d_pl = 0;
        d_peaks2D->setPeakSpace( new PeakSpaceDummy( 2 ) );
        d_peaks4D->setPeakSpace( new PeakSpaceDummy( 2 ) );
        d_plane->setLabelBottomRight( QString() );
    }
}

void FourDScope3::onGoto(SpinSpace::Element e)
{
    PeakPos p;
    if( sender() == d_planePointList )
    {
        if( e.d_point[DimZ] == 0 && e.d_point[DimW] == 0 )
            onShowPlanePoint(e);
        else
        {
            p[DimX] = e.d_point[DimX]->getShift();
            p[DimY] = e.d_point[DimY]->getShift();
            p[DimW] = e.d_point[DimW]->getShift();
            p[DimZ] = e.d_point[DimZ]->getShift();
            d_pointMdl->setPos( p );
			if( !e.isGhost() && !e.isHidden() )
				d_plane->getPointView()->select( e.d_point );
			else
				d_plane->getPointView()->clearSelection();
			selectCurSystem();
		}
    }else
    {
        if( e.d_point[DimX] && e.d_point[DimY] && e.d_point[DimZ] && e.d_point[DimW] )
        {
            p[DimX] = e.d_point[DimZ]->getShift();
            p[DimY] = e.d_point[DimW]->getShift();
            p[DimW] = e.d_point[DimX]->getShift();
            p[DimZ] = e.d_point[DimY]->getShift();
            d_pointMdl->setPos( p );
        }
    }
}

void FourDScope3::onShowPlanePoint(SpinSpace::Element e)
{
    if( e.d_point[DimX] && e.d_point[DimY] )
	{
        d_pointMdl->setPos( DimX, e.d_point[DimX]->getShift(), DimY, e.d_point[DimY]->getShift() );
		if( !e.isGhost() && !e.isHidden() )
			d_plane->getPointView()->select( e.d_point );
		else
			d_plane->getPointView()->clearSelection();
		selectCurSystem();
	}
}

void FourDScope3::onShowOrthoPoint(SpinSpace::Element e)
{
    if( e.d_point[DimX] && e.d_point[DimY] )
	{
        d_pointMdl->setPos( DimW, e.d_point[DimX]->getShift(), DimZ, e.d_point[DimY]->getShift() );
		if( !e.isGhost() && !e.isHidden() )
			d_ortho->getPointView()->select( e.d_point );
		else
			d_ortho->getPointView()->clearSelection();
	}
}

void FourDScope3::onOrthoContourUpd(int overlay)
{
    if( overlay == -1 )
        update4dPlaneContour();
}

void FourDScope3::extendSystem(Dimension source, Dimension target )
{
    // Ausser spec2D identisch mit HomoScope
    Spin* ref = 0;
    if( d_ref[ source ] != 0 )
        ref = d_ref[ source ];
    else if( d_plane->getPointView()->getSel().size() == 1 )
        ref = ( *d_plane->getPointView()->getSel().begin() )[ source ];
    else
    {
        // Der User kann Extend auch ausfhren, wenn kein Peak selektiert wurde.
        // In diesem Fall schlagen wir Peaks in der Region der Cursordimension vor.
        ProposeSpinDlg dlg( this, d_pro, getSpec2()->getColor( source ), d_pointMdl->getPos(source),
                            getSpec2(),	"Select Reference Spin" );
        dlg.setAnchor( source, ref );
        if( !dlg.exec() || dlg.getSpin() == 0 )
            return;
        ref = dlg.getSpin();
    }
    pickSpin( target, ref, ref->getSystem() );
}

void FourDScope3::pickSpin(Dimension d, Spin *other, SpinSystem *owner)
{
    SpinLabel l = d_spec2D->getKeyLabel( d );

    // 26.6.05: immer anzeigen if( !d_src2D->showNulls() )
    {
        SpinLabelSet ly = d_spec2D->getType()->getLabels( d_spec2D->mapToType( d ) );
            // Ich lasse das vorerst so, da nicht sicher ist, ob Inference Keys enth�t.

        NmrExperiment* e = d_pro->getRepository()->getTypes()->
			inferExperiment2( d_spec2D->getType(), owner, d_spec2D );
        if( e )
            e->getColumn( d_spec2D->mapToType( d ), ly, owner );
        if( owner )
            ly = owner->getAcceptables( ly );

        if( !Dlg::getLabel( this, l, ly ) )
            return;
        if( owner && !owner->isAcceptable( l ) )
        {
            Root::ReportToUser::alert( this, "Pick Label", "Label is not acceptable" );
            return;
        }
    }

    Root::Ref<PickSystemSpinLabelCmd> cmd = new PickSystemSpinLabelCmd( d_pro->getSpins(),
        owner, d_spec2D->getColor( d ), d_pointMdl->getPos(d), l, 0 );
    if( cmd->handle( getAgent() ) )
    {
        if( l.isNull() )
            d_src2D->showNulls( true );
        d_plane->getPointView()->selectPoint( d_pointMdl->getPos(DimX), d_pointMdl->getPos(DimY) );
    }
}

void FourDScope3::gotoTuple(SpinSystem * sys, Spin * spin, Spin * link, bool twoD )
{
    SpinPoint tuple;
    Dimension dim;
    SpinSpace::Result tuples;
    if( !twoD && d_spec4D )
    {
        if( link )
            d_rot4D->find( tuples, spin, link );
        else if( spin )
            d_rot4D->find( tuples, spin );
        else
            d_rot4D->find( tuples, sys );
        dim = DimCount;
    }else
    {
        if( link )
            d_rot2D->find( tuples, spin, link );
        else if( spin )
            d_rot2D->find( tuples, spin );
        else
            d_rot2D->find( tuples, sys );
        dim = 2;
    }
    if( tuples.empty() )
    {
        setStatusMessage( "Goto peak: element not found in inferred base" );
        return;
    }
    if( tuples.hasOne() )
    {
        tuple = tuples.first().d_point;
    }else
    {
        tuple = Dlg::selectTuple( this, tuples,
            dim, "Select Spin Tuple" );
        if( tuple.isZero() )
            return;
    }
    for( Dimension d = 0; d < dim; d++ )
        d_pointMdl->setPos( d, tuple[ d ]->getShift(
            ( dim == DimCount )?d_spec4D:d_spec2D ) );
    d_plane->getPointView()->select( tuple );
    if( dim == DimCount )
        show4dPlaneSlice( true );
    selectCurSystem();
    d_plane->ensureCursorVisible();
    if( dim == DimCount )
        d_ortho->ensureCursorVisible();
    notifyCursor( dim == 2 );
}

Spin* FourDScope3::getSel(bool hori) const
{
    // Kopie von HomoScope
    if( d_plane->getPointView()->getSel().size() != 1 )
        return 0;

    SpinPoint tuple = *d_plane->getPointView()->getSel().begin();
    if( hori )
        return tuple[ DimX ];
    else
        return tuple[ DimY ];
}

void FourDScope3::notifyCursor(bool plane)
{
    QString str;
    QTextStream ts( &str, QIODevice::WriteOnly );

    ts.setf( QTextStream::fixed );
    ts.precision( 3 ); // drei Stellen

    ts <<  "Cursor:  ";

    Spectrum* spec = getSpec();
    if( !plane && d_spec4D )
        spec = d_spec4D;

    for( Dimension d = 0; spec && d < spec->getDimCount(); d++ )
    {
        ts << Spec::getDimSymbolLetter( d, true ) << ": ";
        ts << spec->getDimName( d ) << "=";	// wegen Fred
        ts << d_pointMdl->getPos(d);
        if( d_plane->getFolding() )
            ts << " (" << spec->getScale( d ).getRangeOffset( d_pointMdl->getPos(d) ) << ")  ";
        else
            ts << "  ";
    }

    try
    {
        Amplitude val = 0;
        if( plane || d_spec4D.isNull() )
        {
            PpmPoint p;
            for( Dimension d = 0; d < getSpec()->getDimCount(); d++ )
                p.push_back( d_pointMdl->getPos(d) );
            val = getSpec()->getAt( p, d_plane->getFolding(), d_plane->getFolding() );
        }else
            val = d_spec4D->getAt( d_pointMdl->getPpmPoint(DimCount), d_ortho->getFolding(), d_ortho->getFolding() );
        ts.setf( QTextStream::showpos );
        ts.precision( 0 );
        ts << "Level=" << val;
    }catch( ... )
    {
        ts << " Out of Spectrum";
    }
    QByteArray  tmp;
    if( plane )
    {
        d_plane->getPointView()->formatSelection( tmp, SpinPointView3::PairAll, 3 );
        str += QLatin1String(",  ") + QString::fromLatin1( tmp );
    }else if( d_ortho->getPlaneViewer()->hasFocus() )
    {
        d_ortho->getPointView()->formatSelection( tmp, SpinPointView3::PairAll, 3 );
        str += QLatin1String(",  ") + QString::fromLatin1( tmp );
    }
    setStatusMessage( str );
}

void FourDScope3::show4dPlaneSlice( bool show )
{
    if( ( !d_autoHide || show || d_show4DPlane ) )
    {
        if( !d_spec4D.isNull() && d_plane->getSpectrum(0) == 0 )
            d_plane->setSpectrum( new SpecProjector( d_spec4D, d_pointMdl, DimX, DimY ), 0, d_show4DPlane );
    }else
        d_plane->setSpectrum(0, 0, d_show4DPlane);
}

void FourDScope3::updateRef()
{
    if( !d_ref.isZero() )
    {
        QByteArray buf = d_plane->getPointView()->formatLabel( d_ref );
        QString str;
        str.sprintf( "Reference: %s", buf.data() );
        d_plane->setLabelBottomLeft( str, g_clrLabel );
    }else
        d_plane->setLabelBottomLeft( QString() );
}

void FourDScope3::selectCurSystem( bool force )
{
    if( d_spec4D && ( d_plane->getPointView()->getSel().size() == 1 || !d_ref.isZero() ) )
    {
        SpinPoint tuple;
        if( !d_ref.isZero() )
            tuple = d_ref;
        else
            tuple = *d_plane->getPointView()->getSel().begin();
        if( !force && d_cur == tuple )
            return; // Bereits korrekt
        d_cur = tuple;

		char buf[32];
		SpinPointView::formatLabel( buf, sizeof(buf), d_cur,
									SpinPointView::PairIdLabelSysOrResi, DimUndefined );
		if( d_filterBySystem )
		{
			d_ortho->setLabelBottomLeft( tr("%1 (filtered)").arg(buf), g_clrSlice4D );
			d_range4D->setSys( d_cur[ DimX ]->getSystem() );
		}else
		{
			d_ortho->setLabelBottomLeft( buf, g_clrSlice4D );
			d_range4D->setSys( 0 );
		}
	}else
    {
        d_cur.zero();
        d_ortho->setLabelBottomLeft( QString() );
        d_range4D->setSys( 0 );
    }
}

void FourDScope3::onListActivate()
{
    gotoTuple( d_sysList->getSelectedStrip(), d_sysList->getSelectedSpin(),
        d_sysList->getSelectedLink(), !d_goto4D );
    d_plane->getPlaneViewer()->setFocus();
}

void FourDScope3::handleForward()
{
    ENABLED_IF( d_forward.size() > 0 );

    QApplication::setOverrideCursor(Qt::WaitCursor);
    d_backward.push_back( d_forward.back() );
    const PpmCube& cube = d_forward.back().first;
    for( Dimension d = 0; d < DimCount; d++ )
    {
        d_pointMdl->setPos( d, d_backward.back().second[d] );
        d_cubeMdl->setRange( d, cube[ d ] );
    }
    d_forward.pop_back();
    QApplication::restoreOverrideCursor();
}

void FourDScope3::handleSetPeakColors()
{
    ENABLED_IF(true);

    PeakColorDlg dlg(this);
    dlg.select( d_pro->getRepository()->getColors() );
}

void FourDScope3::handle4dPlaneColors()
{
    ENABLED_IF(true);

    if( ContourColorDlg::setColors( this, d_4dPlanePos, d_4dPlaneNeg ) )
		update4dPlaneContour();
}

void FourDScope3::handleHoldOrthoCur()
{
	ENABLED_IF(!d_spec4D.isNull());

	PeakPos cur = d_ortho->getCur2();
	if( cur[DimX] == d_pointMdl->getPos( DimW ) && cur[DimY] == d_pointMdl->getPos( DimZ ) )
	{
		d_ortho->setSpectrum( 0, 0 );
		d_ortho->setCur2(0,0);
		d_ortho->hideCur2();
	}else
	{
		d_ortho->setCur2( d_pointMdl->getPos( DimW ), d_pointMdl->getPos( DimZ ) );
		d_ortho->setSpectrum( new SpecProjector( d_spec4D, DimW, DimZ, d_pointMdl->getPpmPoint(DimCount) ), 0,
							  false, true, true );
	}
}

void FourDScope3::handleFilterBySystem()
{
	CHECKED_IF( true, d_filterBySystem );
	d_filterBySystem = !d_filterBySystem;
	selectCurSystem(true);
}

void FourDScope3::handleBackward()
{
    ENABLED_IF( d_backward.size() > 1 );

    QApplication::setOverrideCursor(Qt::WaitCursor);
    d_forward.push_back( d_backward.back() );
    d_backward.pop_back();
    const PpmCube& cube = d_backward.back().first;
    for( Dimension d = 0; d < DimCount; d++ )
    {
        d_pointMdl->setPos( d, d_forward.back().second[d] );
        d_cubeMdl->setRange( d, cube[ d ] );
    }
    d_backward.pop_back();
    QApplication::restoreOverrideCursor();
}

void FourDScope3::handleSpecCalibrate()
{
    ENABLED_IF( d_plane->getPointView()->getSel().size() == 1 );

    SpinPoint tuple = *d_plane->getPointView()->getSel().begin();

    Spectrum* spec = d_spec2D;
    if( d_spec4D && d_show4DPlane )
        spec = d_spec4D;

    PpmPoint p( 0, 0 );
    for( Dimension d = 0; d < 2; d++ )
        p[ d ] = tuple[ d ]->getShift( spec ) - d_pointMdl->getPos(d);

    QApplication::setOverrideCursor(Qt::WaitCursor);
    Root::Ref<SpecCalibrateCmd> cmd = new SpecCalibrateCmd( spec, p );
    cmd->handle( getAgent() );
    QApplication::restoreOverrideCursor();
}

void FourDScope3::handleAutoCenter()
{
    CHECKED_IF( true, d_pointMdl->getNotifyUpdating() );

    d_pointMdl->setNotifyUpdating(d_pointMdl->getNotifyUpdating());
}

void FourDScope3::handlePickSystem()
{
    ENABLED_IF( true );

    PpmPoint xyPoint( d_pointMdl->getPos(DimX), d_pointMdl->getPos(DimY) );
    bool ok = true;
    if( !d_spec2D.isNull() && !d_show4DPlane )
    {
        // 2D
        Dlg::LP lp;
        lp.d_x = d_spec2D->getKeyLabel( DimX );
        lp.d_y = d_spec2D->getKeyLabel( DimY );

        if( !Dlg::getLabelsSysType( this, lp, d_pro->getRepository(), d_spec2D->getType(),
                                    d_spec2D->mapToType( DimX ), d_spec2D->mapToType( DimY ) ) )
            return;
        Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Pick System" );
        Root::Ref<CreateSystemPairCmd> c1 =
                new CreateSystemPairCmd( d_pro->getSpins(),	xyPoint, d_spec2D, lp.d_sys );
        c1->execute();
        cmd->add( c1 );
        if( !c1->getSpin( DimX )->getLabel().equals( lp.d_x ) )
            cmd->add( new LabelSpinCmd( d_pro->getSpins(), c1->getSpin( DimX ), lp.d_x ) );
        if( !c1->getSpin( DimY )->getLabel().equals( lp.d_y ) )
            cmd->add( new LabelSpinCmd( d_pro->getSpins(), c1->getSpin( DimY ), lp.d_y ) );
        ok = cmd->handle( getAgent() );
        if( lp.d_x.isNull() || lp.d_y.isNull() )
            d_src2D->showNulls( true );
    }else // if( d_spec2D.isNull() || d_show4DPlane )
    {
        // 4D
        SpinLabelPoints pairs;
        pairs.append(SpinLabelPoint());
		NmrExperiment* e = d_pro->getRepository()->getTypes()->inferExperiment3( d_spec4D->getType(), true );
        if( e )
        {
            pairs = e->getQuadruples( PathTable::Path(), d_spec4D->mapToType( DimX ), d_spec4D->mapToType( DimY ),
                                      d_spec4D->mapToType( DimZ ), d_spec4D->mapToType( DimW ) );
        }
        SpinLabelPoint pair;
        if( !Dlg::getLabelPoint( this, pairs, pair, DimCount, "Pick System" ) )
            return;
		for( int i = 0; i < pair.maxSize(); i++ )
			pair[i].setAssigned(); // werde Wildcard los
        Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Pick System" );
        Root::Ref<CreateSystemPairCmd> c1 = new CreateSystemPairCmd( d_pro->getSpins(),	xyPoint, d_spec4D );
        c1->execute();
        cmd->add( c1 );
        if( !c1->getSpin( DimX )->getLabel().equals( pair[DimX] ) )
            cmd->add( new LabelSpinCmd( d_pro->getSpins(), c1->getSpin( DimX ), pair[DimX] ) );
        if( !c1->getSpin( DimY )->getLabel().equals( pair[DimY] ) )
            cmd->add( new LabelSpinCmd( d_pro->getSpins(), c1->getSpin( DimY ), pair[DimY] ) );
        cmd->add( new PickSystemSpinLabelCmd( d_pro->getSpins(), c1->getSystem(),
                                              d_spec4D->getColor( DimZ ), d_pointMdl->getPos(DimZ), pair[DimZ], 0 ) );
        cmd->add( new PickSystemSpinLabelCmd( d_pro->getSpins(), c1->getSystem(),
                                              d_spec4D->getColor( DimW ), d_pointMdl->getPos( DimW ), pair[DimW], 0 ) );
        ok = cmd->handle( getAgent() );
    }
    if( ok )
    {
        d_plane->getPointView()->selectPoint( d_pointMdl->getPos( DimX ), d_pointMdl->getPos( DimY ) );
        if( d_autoHold && !d_plane->getPointView()->getSel().empty() )
            d_ref = *d_plane->getPointView()->getSel().begin();
        updateRef();
        selectCurSystem();
        d_ortho->getPointView()->selectPoint( d_pointMdl->getPos( DimW ), d_pointMdl->getPos( DimZ ) );
    }
}

void FourDScope3::handlePickHori()
{
    ENABLED_IF( !d_show4DPlane && !d_spec2D.isNull() || d_show4DPlane && !d_spec4D.isNull() );
    if( !d_show4DPlane )
        extendSystem( DimY, DimX );
    else
        extendSystem4D( DimY, DimX, true );
}

void FourDScope3::extendSystem4D( Dimension main, Dimension ortho, bool plane )
{
    Q_ASSERT( d_spec4D );
    Spin* ref = 0;
    const Dimension dim = ( ortho== DimY || ortho == DimZ )?DimY:DimX;
    if( d_ref[ ortho ] != 0 )
        ref = d_ref[ ortho ];
    else if( !plane && d_ortho->getPointView()->getSel().size() == 1 )
        ref = ( *d_ortho->getPointView()->getSel().begin() )[ dim ];
    else if( plane && d_plane->getPointView()->getSel().size() == 1 )
        ref = ( *d_plane->getPointView()->getSel().begin() )[ dim ];
    else
    {
        // Der User kann Extend auch ausfhren, wenn kein Peak selektiert wurde.
        // In diesem Fall schlagen wir Peaks in der Region der Cursordimension vor.
        ProposeSpinDlg dlg( this, d_pro, d_spec4D->getColor( ortho ), d_pointMdl->getPos( ortho ),
                            d_spec4D,	"Select Reference Spin" );
        dlg.setAnchor( ortho, ref );
        if( !dlg.exec() || dlg.getSpin() == 0 )
            return;
        ref = dlg.getSpin();
    }
    SpinLabelSet ly = d_spec4D->getType()->getLabels( d_spec4D->mapToType( main ) );
    NmrExperiment* e = d_pro->getRepository()->getTypes()->
			inferExperiment2( d_spec4D->getType(), ref->getSystem(), d_spec4D );
    if( e )
        e->getColumn( d_spec4D->mapToType( main ), ly, ref->getSystem() );
    if( ref->getSystem() )
        ly = ref->getSystem()->getAcceptables( ly );

    SpinLabel l = d_spec4D->getKeyLabel( main );
    if( !Dlg::getLabel( this, l, ly ) )
            return;
    if( ref->getSystem() && !ref->getSystem()->isAcceptable( l ) )
    {
        Root::ReportToUser::alert( this, "Pick Spin", "Label is not acceptable" );
        return;
    }

    Root::Ref<PickSystemSpinLabelCmd> cmd = new PickSystemSpinLabelCmd( d_pro->getSpins(),
        ref->getSystem(), d_spec4D->getColor( main ), d_pointMdl->getPos( main ), l, 0 );
    if( cmd->handle( getAgent() ) )
    {
        if( l.isNull() )
            d_src4D->showNulls( true );
        d_ortho->getPointView()->selectPoint( d_pointMdl->getPos( DimW ), d_pointMdl->getPos( DimZ ) );
    }
}

void FourDScope3::specChanged(bool fourD)
{
    if( d_use4D )
        d_sysList->setSpec( d_spec4D );
    else
        d_sysList->setSpec( d_spec2D );
}

void FourDScope3::handlePickHori4D()
{
    ENABLED_IF( d_spec4D );
    extendSystem4D( DimW, DimZ, false );
}

void FourDScope3::handlePickVerti4D()
{
    ENABLED_IF( d_spec4D );
    extendSystem4D( DimZ, DimW, false );
}

void FourDScope3::handlePickVerti()
{
    ENABLED_IF( !d_show4DPlane && !d_spec2D.isNull() || d_show4DPlane && !d_spec4D.isNull() );
    if( !d_show4DPlane )
        extendSystem( DimX, DimY );
    else
        extendSystem4D( DimX, DimY, true );
}

void FourDScope3::handleMovePeak()
{
    // Verschiebt nur den 2D-Tuple
    ENABLED_IF( d_plane->getPointView()->getSel().size() == 1 );

    SpinPoint tuple = *d_plane->getPointView()->getSel().begin();
    Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Move Spin System" );
    for( Dimension d = 0; d < 2; d++ )
    {
        cmd->add( new MoveSpinCmd( d_pro->getSpins(), tuple[ d ],
            d_pointMdl->getPos( d ), 0 ) ); // Move generisches Spektrum
    }
    cmd->handle( getAgent() );
    selectCurSystem( true );
}

void FourDScope3::handleMovePeakAlias()
{
    // Verschiebt nur den 2D-Tuple
    ENABLED_IF( d_plane->getPointView()->getSel().size() == 1 );

    SpinPoint tuple = *d_plane->getPointView()->getSel().begin();
    Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Move Spin System" );
    Spectrum* spec = getSpec2();
    for( Dimension d = 0; d < 2; d++ )
    {
        if( d_pointMdl->getPos( d ) != tuple[ d ]->getShift( spec ) )
            cmd->add( new MoveSpinCmd( d_pro->getSpins(), tuple[ d ], d_pointMdl->getPos( d ), spec ) );
    }
    cmd->handle( getAgent() );
    selectCurSystem( true );
}

void FourDScope3::handleLabelPeak()
{
    ENABLED_IF( d_plane->getPointView()->getSel().size() == 1 );

    SpinPoint tuple = *d_plane->getPointView()->getSel().begin();

    Spectrum* spec = getSpec2();
    SpinLabel x = tuple[ DimX ]->getLabel();
    SpinLabel y = tuple[ DimY ]->getLabel();
    SpinLabelSet lx = spec->getType()->getLabels( spec->mapToType( DimX ) );
    SpinLabelSet ly = spec->getType()->getLabels( spec->mapToType( DimY ) );
        // lx/y soll vorerst alle statischen Labels auch sehen. Wenn man das wegl�st,
        // verschwinden zur Zeit noch CB aus der Liste von HNCACB von Freds Template.
        // Zu grosse Auswirkungen auf bestehende Projekte.
    NmrExperiment* e;
    e = d_pro->getRepository()->getTypes()->
		inferExperiment2( spec->getType(), tuple[ DimX ]->getSystem(), spec );
    if( e )
        e->getColumn( spec->mapToType( DimX ), lx );
    if( tuple[ DimX ]->getSystem() )
        lx = tuple[ DimX ]->getSystem()->getAcceptables( lx );
    e = d_pro->getRepository()->getTypes()->
		inferExperiment2( spec->getType(), tuple[ DimY ]->getSystem(), spec );
    if( e )
        e->getColumn( spec->mapToType( DimY ), ly );
    if( tuple[ DimY ]->getSystem() )
        ly = tuple[ DimY ]->getSystem()->getAcceptables( ly );
    if( !Dlg::getLabels( this, tuple[ DimX ]->getId(), tuple[ DimY ]->getId(), x, y, lx, ly ) )
        return;

    Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Label Peak" );
    if( !tuple[ DimX ]->getLabel().equals( x ) )
        cmd->add( new LabelSpinCmd( d_pro->getSpins(), tuple[ DimX ], x ) );
    if( !tuple[ DimY ]->getLabel().equals( y ) )
        cmd->add( new LabelSpinCmd( d_pro->getSpins(), tuple[ DimY ], y ) );
    if( !cmd->empty() )
        cmd->handle( getAgent() );
    if( x.isNull() || y.isNull() )
        d_src2D->showNulls( true );
}

void FourDScope3::handleHidePeak()
{
    if( d_plane->getPointView()->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane->getPointView()->getSel().begin();
    SpinLink* link = tuple[ DimX ]->findLink( tuple[ DimY ] );
    ENABLED_IF( link );
    Root::Ref<HideSpinLinkCmd> cmd = new HideSpinLinkCmd( d_pro->getSpins(), link, getSpec2() );
    cmd->handle( getAgent() );
    // TODO: Plural
}

void FourDScope3::handleDeletePeak()
{
    ENABLED_IF( !d_plane->getPointView()->getSel().empty() );

    SpinPointView::Selection sel = d_plane->getPointView()->getSel();
    SpinPointView::Selection::const_iterator p;
    SpinBase* base = d_pro->getSpins();
    Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Delete Spins" );
    std::set<Spin*> test;
    for( p = sel.begin(); p != sel.end(); ++p )
    {
        if( test.count( (*p)[ DimX ] ) == 0 )
        {
            cmd->add( new DeleteSpinCmd( base, (*p)[ DimX ] ) );
            test.insert( (*p)[ DimX ] );
        }
        if( test.count( (*p)[ DimY ] ) == 0 )
        {
            cmd->add( new DeleteSpinCmd( base, (*p)[ DimY ] ) );
            test.insert( (*p)[ DimY ] );
        }
    }
    cmd->handle( getAgent() );
    selectCurSystem();
}

void FourDScope3::handleDeleteSpinX()
{
    ENABLED_IF( d_plane->getPointView()->getSel().size() == 1 );
    SpinPoint tuple = *d_plane->getPointView()->getSel().begin();

    Root::Ref<DeleteSpinCmd> cmd = new DeleteSpinCmd( d_pro->getSpins(), tuple[ DimX ] );
    cmd->handle( getAgent() );
}

void FourDScope3::handleDeleteSpinY()
{
    ENABLED_IF( d_ortho->getPointView()->getSel().size() == 1 );
    SpinPoint tuple = *d_ortho->getPointView()->getSel().begin();

    Root::Ref<DeleteSpinCmd> cmd = new DeleteSpinCmd( d_pro->getSpins(), tuple[ DimY ] );
    cmd->handle( getAgent() );
}

void FourDScope3::handleDeleteSpinX4D()
{
    ENABLED_IF( d_ortho->getPointView()->getSel().size() == 1 );
    SpinPoint tuple = *d_ortho->getPointView()->getSel().begin();

    Root::Ref<DeleteSpinCmd> cmd = new DeleteSpinCmd( d_pro->getSpins(), tuple[ DimX ] );
    cmd->handle( getAgent() );
}

void FourDScope3::handleDeleteSpinY4D()
{
    ENABLED_IF( d_plane->getPointView()->getSel().size() == 1 );
    SpinPoint tuple = *d_plane->getPointView()->getSel().begin();

    Root::Ref<DeleteSpinCmd> cmd = new DeleteSpinCmd( d_pro->getSpins(), tuple[ DimY ] );
    cmd->handle( getAgent() );
}

void FourDScope3::handleShowAlignment()
{
    if( d_plane->getPointView()->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane->getPointView()->getSel().begin();
    ENABLED_IF( tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() );

    SpinSystem* sys = tuple[ DimX ]->getSystem();

    SpinSystemString fra;
    d_pro->getSpins()->fillString( sys, fra );

    FragmentAssignment* f = new FragmentAssignment( d_pro->getSpins(),
        d_pro->getMatcher(), fra );
    SingleAlignmentView* v = new SingleAlignmentView( getAgent(), f );
    v->show();
}

void FourDScope3::handleAssign()
{
    if( d_plane->getPointView()->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane->getPointView()->getSel().begin();
    ENABLED_IF( tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() );

    SpinSystem* sys = tuple[ DimX ]->getSystem();
    bool ok;
    QString str;
    str.sprintf( "Assignment for spin system %d:", sys->getId() );
    int r = QInputDialog::getInteger( "Assign Strips",
        str, (sys->getAssig())?sys->getAssig()->getId():0,
        -999999, 999999, 1, &ok, this );
    if( !ok )
        return;
    Residue* res = d_pro->getSequence()->getResidue( r );
    if( res == 0 )
    {
        Root::ReportToUser::alert( this, "Assign", "Unknown residue!" );
        return;
    }

    Root::Ref<AssignSystemCmd> cmd =
        new AssignSystemCmd( d_pro->getSpins(), sys, res );
    cmd->handle( getAgent() );
}

void FourDScope3::handleUnassign()
{
    if( d_plane->getPointView()->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane->getPointView()->getSel().begin();
    ENABLED_IF( tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() );

    SpinSystem* sys = tuple[ DimX ]->getSystem();

    Root::Ref<UnassignSystemCmd> cmd =
        new UnassignSystemCmd( d_pro->getSpins(), sys );
    cmd->handle( getAgent() );
}

void FourDScope3::handleSetSystemType()
{
    if( d_plane->getPointView()->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane->getPointView()->getSel().begin();
    ENABLED_IF( tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() );

    SpinSystem* sys = tuple[ DimX ]->getSystem();

    Repository::SystemTypeMap::const_iterator p;
    const Repository::SystemTypeMap& sm = d_pro->getRepository()->getSystemTypes();

    int cur = 0;
    QStringList l;
    l.append( "" );
    for( p = sm.begin(); p != sm.end(); ++p )
        l.append( (*p).second->getName().data() ); // NOTE: Name sollte Unique sein.
    l.sort();
    QString tn;
    if( sys->getSysType() )
        tn = sys->getSysType()->getName().data();
    for( int i = 0; i < l.count(); i++ )
        if( tn == l[ i ] )
        {
            cur = i;
            break;
        }
    bool ok;
    QString res = QInputDialog::getItem( "Set System Type", "Select a spin system type:",
        l, cur, false, &ok, this );
    if( !ok )
        return;

    SystemType* sst = 0;
    for( p = sm.begin(); p != sm.end(); ++p )
    {
        if( res == (*p).second->getName().data() )
        {
            sst = (*p).second;
            break;
        }
    }

    Root::Ref<ClassifySystemCmd> cmd =
        new ClassifySystemCmd( d_pro->getSpins(), sys, sst );
    cmd->handle( getAgent() );
}

void FourDScope3::handleSelectSpec2D()
{
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    CHECKED_IF( true, d_spec2D && d_spec2D == a->property( "0" ).value<Spec::Spectrum*>() );

    setSpec2D( a->property( "0" ).value<Spec::Spectrum*>() );
}

void FourDScope3::handleLinkSystems()
{
    ENABLED_IF( true );

    Gui2::UiFunction* a = Gui2::UiFunction::me();
    int pred, succ;
    if( !a->property( "0" ).isNull() && !a->property( "1" ).isNull() )
    {
        pred = a->property( "0" ).toInt();
        succ = a->property( "1" ).toInt();
    }else if( !Dlg::getPredSucc( this, pred, succ ) )
        return;

    SpinSystem* p = d_pro->getSpins()->getSystem( pred );
    SpinSystem* s = d_pro->getSpins()->getSystem( succ );
    if( p == 0 || s == 0 )
    {
        Root::ReportToUser::alert( this, "Link Systems", "Unknown Systems" );
        return;
    }

    try
    {
        Root::Ref<LinkSystemCmd> cmd =
            new LinkSystemCmd( d_pro->getSpins(), p, s );
        cmd->handle( getAgent() );
    }catch( Root::Exception& e )
    {
        Root::ReportToUser::alert( this, "Link to Reference", e.what() );
    }
}

void FourDScope3::handleUnlinkPred()
{
    if( d_plane->getPointView()->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane->getPointView()->getSel().begin();
    ENABLED_IF( tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() &&
        tuple[ DimX ]->getSystem()->getPred() != 0 );

    SpinSystem* other = tuple[ DimX ]->getSystem()->getPred();
    Root::Ref<UnlinkSystemCmd> cmd =
        new UnlinkSystemCmd( d_pro->getSpins(), other, tuple[ DimX ]->getSystem() );
    cmd->handle( getAgent() );
}

void FourDScope3::handleUnlinkSucc()
{
    if( d_plane->getPointView()->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane->getPointView()->getSel().begin();
    ENABLED_IF( tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() &&
        tuple[ DimX ]->getSystem()->getSucc() != 0 );

    SpinSystem* other = tuple[ DimX ]->getSystem()->getSucc();
    Root::Ref<UnlinkSystemCmd> cmd =
        new UnlinkSystemCmd( d_pro->getSpins(), tuple[ DimX ]->getSystem(), other );
    cmd->handle( getAgent() );
}

void FourDScope3::handleUnhidePeak()
{
    /*
    if( d_plane.d_tuples->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
    SpinLink* link = tuple[ DimX ]->findLink( tuple[ DimY ] );
    ENABLED_IF( link && !link->isVisible( d_spec2D ) );
    d_pro->getSpins()->setVisible( link, true, d_spec2D ); // TODO Undo
    // TODO: Plural
    */
}

void FourDScope3::handleHoldReference()
{
    CHECKED_IF( !d_ref.isZero() || d_plane->getPointView()->getSel().size() == 1, !d_ref.isZero() );

    if( !d_ref.isZero() && d_plane->getPointView()->getSel().empty() )
        d_ref.zero(); // Hebe Zustand auf
    else if( !d_plane->getPointView()->getSel().empty() )
    {
        d_ref = *d_plane->getPointView()->getSel().begin();
        if( d_autoRuler )
        {
            d_plane->removeAllRulers();
            d_plane->addRuler( d_ref[ DimY ], Qt::Horizontal );
            d_plane->addRuler( d_ref[ DimX ], Qt::Vertical );
        }
    }else
        return;

    updateRef();
    selectCurSystem();
}

void FourDScope3::handlePrintPlane()
{
    ENABLED_IF( true );

    createReport( d_plane );
}

void FourDScope3::handlePrintOrtho()
{
    ENABLED_IF( d_ortho->getSpectrum() );

    createReport( d_ortho );
}

void FourDScope3::createReport( PlaneGrid* pg )
{
    ContourView3::Params p0 = pg->getContourParams();
    Dlg::ContourParams p;
    p.d_factor = p0.d_factor;
    p.d_threshold =	pg->getSpectrum()->getThreshold();
    p.d_option = (Spec::ContourView::Option)p0.d_option; // RISK
    Q_ASSERT( getAgent()->getParent() );
    Q_ASSERT( getAgent()->getParent()->getParent() );
    ReportViewer* rv = ReportViewer::getViewer( getAgent()->getParent()->getParent(), p,
        p0.d_gain, p0.d_auto, pg->getFolding(), d_pro->getRepository() );
    ReportViewer::Spector vec;
    vec.push_back( new SpecProjector( pg->getSpectrum(), DimX, DimY ) );
    for( int i = pg->getMinOverlay(); i <= pg->getMaxOverlay(); i++ )
    {
        Spectrum* spec = pg->getSpectrum( i );
        if( spec )
            vec.push_back( new SpecProjector( spec, DimX, DimY ) );
    }
    Root::Ref<ViewAreaMdl> vam = new ViewAreaMdl(true,true,true,true);
    vam->setRange( pg->getPlaneViewer()->getArea()->getRange(DimX),
                   pg->getPlaneViewer()->getArea()->getRange(DimY) );
    vam->allocate( Lexi::Allocation( 0, 0, 10000, 10000 ) );
    rv->showPlane( vam, vec, pg->getPeaks(), pg->getPointView()->getModel() ); // RISK: gengt ev. pg->getPoints?
}

void FourDScope3::handleAutoRuler()
{
    CHECKED_IF( true, d_autoRuler );

    d_autoRuler = !d_autoRuler;
}

void FourDScope3::handleProposeHori()
{
    if( d_ref.isZero() && d_plane->getPointView()->getSel().size() != 1 )
        return;
    SpinPoint tuple = d_ref;
    if( d_ref[ DimX ] == 0 )
        tuple = *d_plane->getPointView()->getSel().begin();

    Spin* ref = tuple[ DimY ];
    ENABLED_IF( !d_show4DPlane && ref->getSystem() && getSpec2()->hasNoesy() );

    ProposeSpinDlg dlg( this, d_pro, getSpec2()->getColor( DimX ), d_pointMdl->getPos( DimX ),
                        getSpec2(), "Select Horizontal Partner" );
    dlg.setAnchor( DimY, ref );
    if( !dlg.exec() || dlg.getSpin() == 0 )
        return;

    // TODO: wenn target kein SpinSystem hat, dann ins ref einfgen.
    // TODO: wenn target und ref System haben, einen SysLink vorschlagen (Seite?)

    if( ref->findLink( dlg.getSpin() ) == 0 )	// Ref == target zul�sig wegen Diagonaler
    {
        Root::Ref<LinkSpinCmd> cmd = new LinkSpinCmd( d_pro->getSpins(), ref, dlg.getSpin() );
        cmd->handle( getAgent() );
    }else
        Root::ReportToUser::alert( this, "Propose Horizontal Extension",
            "The selected spins are already linked!" );

}

void FourDScope3::handleProposeVerti()
{
    if( d_ref.isZero() && d_plane->getPointView()->getSel().size() != 1 )
        return;
    SpinPoint tuple = d_ref;
    if( d_ref[ DimX ] == 0 )
        tuple = *d_plane->getPointView()->getSel().begin();

    Spin* ref = tuple[ DimX ];
    ENABLED_IF( !d_show4DPlane && ref->getSystem() && getSpec2()->hasNoesy()  );

    ProposeSpinDlg dlg( this, d_pro, getSpec2()->getColor( DimY ), d_pointMdl->getPos( DimY ),
        getSpec2(), "Select Vertical Partner" );
    dlg.setAnchor( DimX, ref );
    if( !dlg.exec() || dlg.getSpin() == 0 )
        return;

    // TODO: wenn target kein SpinSystem hat, dann ins ref einfgen.
    // TODO: wenn target und ref System haben, einen SysLink vorschlagen (Seite?)

    if( ref->findLink( dlg.getSpin() ) == 0 ) // Ref == target zul�sig wegen Diagonaler
    {
        Root::Ref<LinkSpinCmd> cmd = new LinkSpinCmd( d_pro->getSpins(), ref, dlg.getSpin() );
        cmd->handle( getAgent() );
    }else
        Root::ReportToUser::alert( this, "Propose Vertical Extension",
            "The selected spins are already linked!" );
}

void FourDScope3::handleProposePeak()
{
    ENABLED_IF( d_show4DPlane && d_spec4D && d_spec4D->hasNoesy() ||
        !d_show4DPlane && d_spec2D && d_spec2D->hasNoesy() );

    Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Propose Peak" );
    Dlg::SpinTuple res;
    if( d_show4DPlane || d_spec2D.isNull() )
    {
        Dlg::SpinRanking l1 = d_pro->getMatcher()->findMatchingSpins( d_pro->getSpins(),
            d_spec4D->getColor( DimX ), d_pointMdl->getPos( DimX ), d_spec4D );
        Dlg::SpinRanking l2 = d_pro->getMatcher()->findMatchingSpins( d_pro->getSpins(),
            d_spec4D->getColor( DimY ), d_pointMdl->getPos( DimY ), d_spec4D );
        Dlg::SpinRanking l3 = d_pro->getMatcher()->findMatchingSpins( d_pro->getSpins(),
            d_spec4D->getColor( DimZ ), d_pointMdl->getPos( DimZ ), d_spec4D );
        Dlg::SpinRanking l4 = d_pro->getMatcher()->findMatchingSpins( d_pro->getSpins(),
            d_spec4D->getColor( DimW ), d_pointMdl->getPos( DimW ), d_spec4D );
        res = Dlg::selectSpinQuadruple( this, l1, l2, l3, l4, "Select Matching Spins" );
        if( res.empty() )
            return;

        if( d_spec4D->isNoesy( DimX, DimY ) && res[DimX]->findLink( res[DimY] ) == 0 )
            cmd->add( new LinkSpinCmd( d_pro->getSpins(), res[DimX], res[DimY] ) );

        if( d_spec4D->isNoesy( DimY, DimZ ) && res[DimY]->findLink( res[DimZ] ) == 0 )
            cmd->add( new LinkSpinCmd( d_pro->getSpins(), res[DimY], res[DimZ] ) );

        if( d_spec4D->isNoesy( DimX, DimZ ) && res[DimX]->findLink( res[DimZ] ) == 0 )
            cmd->add( new LinkSpinCmd( d_pro->getSpins(), res[DimX], res[DimZ] ) );

        if( d_spec4D->isNoesy( DimX, DimW ) && res[DimX]->findLink( res[DimW] ) == 0 )
            cmd->add( new LinkSpinCmd( d_pro->getSpins(), res[DimX], res[DimW] ) );

        if( d_spec4D->isNoesy( DimY, DimW ) && res[DimY]->findLink( res[DimW] ) == 0 )
            cmd->add( new LinkSpinCmd( d_pro->getSpins(), res[DimY], res[DimW] ) );

        if( d_spec4D->isNoesy( DimZ, DimW ) && res[DimZ]->findLink( res[DimW] ) == 0 )
            cmd->add( new LinkSpinCmd( d_pro->getSpins(), res[DimZ], res[DimW] ) );
    }else
    {
        Dlg::SpinRanking l1 = d_pro->getMatcher()->findMatchingSpins( d_pro->getSpins(),
            d_spec2D->getColor( DimX ), d_pointMdl->getPos( DimX ), d_spec2D );
        Dlg::SpinRanking l2 = d_pro->getMatcher()->findMatchingSpins( d_pro->getSpins(),
            d_spec2D->getColor( DimY ), d_pointMdl->getPos( DimY ), d_spec2D );
        res = Dlg::selectSpinPair( this,
            l1, l2, "Select Matching Spins" );
        if( res.empty() )
            return;
        if( res[DimX]->findLink( res[DimY] ) == 0 ) // Ref == target zul�sig wegen Diagonaler
            cmd->add( new LinkSpinCmd( d_pro->getSpins(), res[DimX], res[DimY] ) );
    }

    if( cmd->empty() )
    {
        Root::ReportToUser::alert( this, "Propose New Peak",
            "The selected spins are already linked!" );
        return;
    }
    cmd->handle( getAgent() );
}

void FourDScope3::handleSelectSpec4D()
{
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    CHECKED_IF( true, d_spec4D && d_spec4D == a->property( "0" ).value<Spec::Spectrum*>() );

    setSpec4D( a->property( "0" ).value<Spec::Spectrum*>() );
    selectCurSystem();
}

void FourDScope3::handlePickLabel4D()
{
    SpinSystem* sys = 0;
    if( !d_cur.isZero() )
        sys = d_cur[ DimX ]->getSystem();
    ENABLED_IF( !d_spec4D.isNull() && sys );

    SpinLabelPoints pairs;
    SpinLabelPoint pair;
	NmrExperiment* e = d_pro->getRepository()->getTypes()->inferExperiment2( d_spec4D->getType(), sys, d_spec4D );
    if( e )
    {
        PathTable::Path filter;
        filter[ d_spec4D->mapToType( DimX ) ] = d_cur[ DimX ]->getLabel();
        filter[ d_spec4D->mapToType( DimY ) ] = d_cur[ DimY ]->getLabel();
        // wir wollen nur Spalten, welche die Labels der in der Plane selektierten Spins enthalten
        pairs = e->getTuples( filter, d_spec4D->mapToType( DimW ), d_spec4D->mapToType( DimZ ) );
    }
    // pairs enthalten hier alle grundstzlich mglichen Labels, unabhngig davon, ob diese bereits gepickt wurden
    // wenn ein System ausgewhlt ist, behalte nur die noch freien Labels
    pairs = sys->getAcceptables( pairs, 2 );
    if( !Dlg::getLabelPoint( this, pairs, pair, 2, "Pick Labels" ) )
        return;

	for( int i = 0; i < pair.maxSize(); i++ )
		pair[i].setAssigned(); // werde Wildcard los

	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Pick Labels" );
    if( sys->isAcceptable( pair[1] ) )
        cmd->add( new PickSystemSpinLabelCmd( d_pro->getSpins(),
                                              sys, d_spec4D->getColor( DimZ ), d_pointMdl->getPos( DimZ ), pair[1], 0 ) );
    if( sys->isAcceptable( pair[0] ) )
        cmd->add( new PickSystemSpinLabelCmd( d_pro->getSpins(),
                                              sys, d_spec4D->getColor( DimW ), d_pointMdl->getPos( DimW ), pair[0], 0 ) );
    if( !cmd->empty() )
        cmd->handle( getAgent() );

    d_ortho->getPointView()->selectPoint( d_pointMdl->getPos( DimX ), d_pointMdl->getPos( DimY ) );
}

void FourDScope3::handlePickSpin4D()
{
    if( !d_src4D->showNulls() )
    {
        handlePickLabel4D();
        return;
    }

    const Dimension ref = DimX;
    ENABLED_IF( !d_spec4D.isNull() && !d_cur.isZero() );

    Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Pick Spins" );
    // Pick generisches Spektrum
    if( d_cur[ ref ]->getSystem() )
    {
        Root::Ref<PickSystemSpinCmd> c2 = new PickSystemSpinCmd( d_pro->getSpins(),
            d_cur[ ref ]->getSystem(), d_spec4D->getColor( DimZ ), d_pointMdl->getPos( DimZ ), 0 );
        cmd->add( c2 );
        c2->execute();
        Root::Ref<PickSystemSpinCmd> c3 = new PickSystemSpinCmd( d_pro->getSpins(),
            d_cur[ ref ]->getSystem(), d_spec4D->getColor( DimW ), d_pointMdl->getPos( DimW ), 0 );
        cmd->add( c3 );
        c3->execute();
    }else
    {
        Root::Ref<PickSpinCmd> c2 = new PickSpinCmd( d_pro->getSpins(),
            d_spec4D->getColor( DimZ ), d_pointMdl->getPos( DimZ ), 0 );
        cmd->add( c2 );
        c2->execute();
        Root::Ref<PickSpinCmd> c3 = new PickSpinCmd( d_pro->getSpins(),
            d_spec4D->getColor( DimW ), d_pointMdl->getPos( DimW ), 0 );
        cmd->add( c3 );
        c3->execute();
    }

    cmd->handle( getAgent() );
    d_ortho->getPointView()->selectPoint( d_pointMdl->getPos( DimW ), d_pointMdl->getPos( DimZ ) );
}

void FourDScope3::handleMoveSpin4D()
{
    ENABLED_IF( d_ortho->getPointView()->getSel().size() == 1 );

    SpinPoint tuple = *d_ortho->getPointView()->getSel().begin();
    Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Move Spins" );
    // Move generisches Spektrum
    cmd->add( new MoveSpinCmd( d_pro->getSpins(), tuple[ DimX ], d_pointMdl->getPos( DimW ), 0 ) );
    cmd->add( new MoveSpinCmd( d_pro->getSpins(), tuple[ DimY ], d_pointMdl->getPos( DimZ ), 0 ) );
    cmd->handle( getAgent() );
}

void FourDScope3::handleMoveSpinAlias4D()
{
    ENABLED_IF( d_ortho->getPointView()->getSel().size() == 1 );

    SpinPoint tuple = *d_ortho->getPointView()->getSel().begin();
    Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Move Spin Aliasses" );
    cmd->add( new MoveSpinCmd( d_pro->getSpins(), tuple[ DimX ], d_pointMdl->getPos( DimW ), d_spec4D ) );
    cmd->add( new MoveSpinCmd( d_pro->getSpins(), tuple[ DimY ], d_pointMdl->getPos( DimZ ), d_spec4D ) );
    cmd->handle( getAgent() );
}

void FourDScope3::handleDeleteSpins4D()
{
    ENABLED_IF( !d_spec4D.isNull() && !d_ortho->getPointView()->getSel().empty() );

    SpinPointView::Selection sel = d_ortho->getPointView()->getSel();
    SpinPointView::Selection::const_iterator p;
    Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Delete Spins" );
    std::set<Spin*> test;
    for( p = sel.begin(); p != sel.end(); ++p )
    {
        if( test.count( (*p)[ DimX ] ) == 0 )
        {
            cmd->add( new DeleteSpinCmd( d_pro->getSpins(), (*p)[ DimX ] ) );
            test.insert( (*p)[ DimX ] );
        }
        if( test.count( (*p)[ DimY ] ) == 0 )
        {
            cmd->add( new DeleteSpinCmd( d_pro->getSpins(), (*p)[ DimY ] ) );
            test.insert( (*p)[ DimY ] );
        }
    }
    cmd->handle( getAgent() );
}

void FourDScope3::handleLabelSpin4D()
{
    ENABLED_IF( !d_spec4D.isNull() && d_ortho->getPointView()->getSel().size() == 1 );

    SpinPoint tuple = *d_ortho->getPointView()->getSel().begin();

    SpinLabel x = tuple[ DimX ]->getLabel();
    SpinLabel y = tuple[ DimY ]->getLabel();
    SpinLabelSet lx;
    SpinLabelSet ly;
    NmrExperiment* e = d_pro->getRepository()->getTypes()->
		inferExperiment2( d_spec4D->getType(), tuple[ DimX ]->getSystem(), d_spec4D );
    if( e )
        e->getColumn( d_spec4D->mapToType( DimW ), lx );
    e = d_pro->getRepository()->getTypes()->
		inferExperiment2( d_spec4D->getType(), tuple[ DimY ]->getSystem(), d_spec4D );
    if( e )
        e->getColumn( d_spec4D->mapToType( DimZ ), ly );
    if( !Dlg::getLabels( this, tuple[ DimX ]->getId(), tuple[ DimY ]->getId(), x, y, lx, ly ) )
        return;

    const bool yOk = tuple[ DimY ]->getLabel() == y || // wenn Tag und Offset gleich, kann problemlos gendert werden
            tuple[ DimY ]->getSystem() != 0 && tuple[ DimY ]->getSystem()->isAcceptable( y );
    const bool xOk = tuple[ DimX ]->getLabel() == x ||
            tuple[ DimX ]->getSystem() != 0 && tuple[ DimX ]->getSystem()->isAcceptable( x );
    if( !yOk || !xOk )
    {
        Root::ReportToUser::alert( this, "Label Spin", "One or both labels are not acceptable" );
        return;
    }

    Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Label Spins" );
    if( xOk && !tuple[ DimX ]->getLabel().equals( x ) )
        cmd->add( new LabelSpinCmd( d_pro->getSpins(), tuple[ DimX ], x ) );
    if( yOk && !tuple[ DimY ]->getLabel().equals( y ) )
        cmd->add( new LabelSpinCmd( d_pro->getSpins(), tuple[ DimY ], y ) );
    if( !cmd->empty() )
        cmd->handle( getAgent() );
    if( x.isNull() || y.isNull() )
        d_src4D->showNulls( true );
}

void FourDScope3::handleSetPeakWidth()
{
    ENABLED_IF( true );
    Spectrum* spec = d_spec2D;
	if( spec == 0 || ( d_ortho->getPlaneViewer()->hasFocus() && !d_spec4D.isNull() ) )
        spec = d_spec4D;

    PpmPoint w;
    QStringList labels;
    for( Dimension d = 0; d < spec->getDimCount(); d++ )
    {
        w.push_back( d_pro->inferPeakWidth( d, spec ) );
        labels.append( spec->getDimName(d) );
    }
    if( Dlg::getPpmPoint( this, w, "Set Peak Width", labels ) )
    {
        for( int d = 0; d < spec->getDimCount(); d++ )
        {
            if( w[0] < 0.0 )
            {
                QMessageBox::critical( this, tr("Set Peak Width"), tr("Invalid peak width in dimension %d!").arg(d) );
                return;
            }
            d_pro->setPeakWidth( d, w[d], spec );
        }
    }
}

void FourDScope3::handleOrthoShowOffs()
{
    CHECKED_IF( true, d_src4D->showOffs() );

    d_src4D->showOffs( !d_src4D->showOffs() );
}

void FourDScope3::setShow4dPlane(bool on)
{
    if( on )
    {
        if( d_spec4D )
        {
            d_plane->setSpectrum( new SpecProjector( d_spec4D, d_pointMdl, DimX, DimY ), 0 );
            d_peaks2D->setRefSpec( d_spec4D );
            //d_autoHide = false;
        }
        d_plane->setPoints( d_range2D );
    }else
    {
        d_plane->setSpectrum( 0, 0, true, false );
        d_peaks2D->setRefSpec( d_spec2D );
        d_plane->setPoints( d_rot2D );
        //d_autoHide = true;
    }
    d_show4DPlane = on;
    update4dPlaneContour();
    updatePlaneLabel();
    specChanged( true );
}

void FourDScope3::handleShowPathSim()
{
    ENABLED_IF( d_ortho->getPlaneViewer()->hasFocus() && !d_spec4D.isNull() ||
                d_plane->getPlaneViewer()->hasFocus() );

    PathSimDlg dlg( (d_ortho->getPlaneViewer()->hasFocus())?d_spec4D.deref():getSpec2(), this );
    if( d_cur[DimX] && d_cur[DimX]->getSystem() && d_cur[DimX]->getSystem()->getAssig() )
        dlg.setResiType( d_cur[DimX]->getSystem()->getAssig()->getType() );
    dlg.exec();
}

void FourDScope3::handleShowOrthoPlane()
{
    // Einschalten nur wenn 4D vorhanden; ausschalten nur wenn 2D vorhanden
    CHECKED_IF( !d_show4DPlane && d_spec4D || d_show4DPlane && d_spec2D, d_show4DPlane );
    setShow4dPlane( !d_show4DPlane );
}

void FourDScope3::handleAutoHide()
{
    CHECKED_IF( true, d_autoHide );

    d_autoHide = !d_autoHide;
    if( d_spec4D )
    {
        show4dPlaneSlice(!d_autoHide );
    }
    //d_slices[ DimX ].d_viewer->update();
    //d_slices[ DimY ].d_viewer->update();
}

void FourDScope3::handleStripCalibrate()
{
    ENABLED_IF( d_ortho->getPointView()->getSel().size() == 1 );

    SpinPoint tuple = *d_ortho->getPointView()->getSel().begin();

    PpmPoint p( 0, 0, 0, 0 );
    p[ DimZ ] = tuple[ DimY ]->getShift( d_spec4D ) - d_pointMdl->getPos( DimZ );
    p[ DimW ] = tuple[ DimX ]->getShift( d_spec4D ) - d_pointMdl->getPos( DimW );

    QApplication::setOverrideCursor(Qt::WaitCursor);
    Root::Ref<SpecCalibrateCmd> cmd = new SpecCalibrateCmd( d_spec4D, p );
    cmd->handle( getAgent() );
    QApplication::restoreOverrideCursor();
}

void FourDScope3::handleProposeSpin4D()
{
    ENABLED_IF( !d_spec4D.isNull() && !d_cur.isZero() && d_spec4D->hasNoesy() );

    Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Propose Spins" );
    Dlg::SpinTuple res;

    Dlg::SpinRanking l3 = d_pro->getMatcher()->findMatchingSpins(
                d_pro->getSpins(), d_spec4D->getColor( DimZ ), d_pointMdl->getPos( DimZ ), d_spec4D );
    Dlg::SpinRanking l4 = d_pro->getMatcher()->findMatchingSpins(
                d_pro->getSpins(), d_spec4D->getColor( DimW ), d_pointMdl->getPos( DimW ), d_spec4D );
    res = Dlg::selectSpinPair( this, l4, l3, "Select Matching Spins" );
    if( res.empty() )
        return;
    const Dimension DW = DimX;
    const Dimension DZ = DimY;

    if( d_spec4D->isNoesy( DimX, DimZ ) && d_cur[ DimX ]->findLink( res[DZ] ) == 0 )
        cmd->add( new LinkSpinCmd( d_pro->getSpins(), d_cur[ DimX ], res[DZ] ) );

    if( d_spec4D->isNoesy( DimY, DimZ ) && d_cur[ DimY ]->findLink( res[DZ] ) == 0 )
        cmd->add( new LinkSpinCmd( d_pro->getSpins(), d_cur[ DimY ], res[DZ] ) );

    if( d_spec4D->isNoesy( DimX, DimW ) && d_cur[ DimX ]->findLink( res[DW] ) == 0 )
        cmd->add( new LinkSpinCmd( d_pro->getSpins(), d_cur[ DimX ], res[DW] ) );

    if( d_spec4D->isNoesy( DimY, DimW ) && d_cur[ DimY ]->findLink( res[DW] ) == 0 )
        cmd->add( new LinkSpinCmd( d_pro->getSpins(), d_cur[ DimY ], res[DW] ) );

    if( d_spec4D->isNoesy( DimZ, DimW ) && res[DZ]->findLink( res[DW] ) == 0 )
        cmd->add( new LinkSpinCmd( d_pro->getSpins(), res[DZ], res[DW] ) );

    if( cmd->empty() )
    {
        Root::ReportToUser::alert( this, "Propose Spins",
            "The selected spins are already linked!" );
        return;
    }
    if( !d_src4D->showLinks() ) // Fred 2015-01-09 12:39
        d_src4D->showLinks( true );

    cmd->handle( getAgent() );
}

void FourDScope3::handlePlaneSpinXAttr()
{
    ENABLED_IF( d_plane->getPointView()->getSel().size() == 1 );
    SpinPoint tuple = *d_plane->getPointView()->getSel().begin();

    DynValueEditor::edit( this,
        d_pro->getRepository()->findObjectDef( Repository::keySpin ), tuple[ DimX ] );
}

void FourDScope3::handlePlaneSpinYAttr()
{
    ENABLED_IF( d_plane->getPointView()->getSel().size() == 1 );
    SpinPoint tuple = *d_plane->getPointView()->getSel().begin();

    DynValueEditor::edit( this,
        d_pro->getRepository()->findObjectDef( Repository::keySpin ), tuple[ DimY ] );
}

void FourDScope3::handlePlaneLinkAttr()
{
    if( d_plane->getPointView()->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane->getPointView()->getSel().begin();
    SpinLink* l = tuple[ DimX ]->findLink( tuple[ DimY ] );
    ENABLED_IF( l );

    DynValueEditor::edit( this,
        d_pro->getRepository()->findObjectDef( Repository::keyLink ), l );
}

void FourDScope3::handlePlaneSysXAttr()
{
    if( d_plane->getPointView()->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane->getPointView()->getSel().begin();
    SpinSystem* s = tuple[ DimX ]->getSystem();
    ENABLED_IF( s );

    DynValueEditor::edit( this,
        d_pro->getRepository()->findObjectDef( Repository::keySpinSystem ), s );
}

void FourDScope3::handlePlaneSysYAttr()
{
    if( d_plane->getPointView()->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane->getPointView()->getSel().begin();
    SpinSystem* s = tuple[ DimY ]->getSystem();
    ENABLED_IF( s );

    DynValueEditor::edit( this,
        d_pro->getRepository()->findObjectDef( Repository::keySpinSystem ), s );
}

void FourDScope3::handleOrthoSpinXAttr()
{
    ENABLED_IF( d_ortho->getPointView()->getSel().size() == 1 );
    SpinPoint tuple = *d_ortho->getPointView()->getSel().begin();

    DynValueEditor::edit( this,
        d_pro->getRepository()->findObjectDef( Repository::keySpin ), tuple[ DimX ] );
}

void FourDScope3::handleOrthoSpinYAttr()
{
    ENABLED_IF( d_ortho->getPointView()->getSel().size() == 1 );
    SpinPoint tuple = *d_ortho->getPointView()->getSel().begin();

    DynValueEditor::edit( this,
        d_pro->getRepository()->findObjectDef( Repository::keySpin ), tuple[ DimY ] );
}

void FourDScope3::handleGotoSystem()
{
    ENABLED_IF( true );

    Gui2::UiFunction* a = Gui2::UiFunction::me();
    QString id;
    if( a->property( "0" ).isNull() )
    {
        bool ok	= FALSE;
        id	= QInputDialog::getText( "Goto System",
            "Please	enter a spin system id:", QLineEdit::Normal, "", &ok, this );
        if( !ok )
            return;
    }else
        id = a->property( "0" ).toString();

    SpinSystem* sys = d_pro->getSpins()->findSystem( id.toLatin1() );
    if( sys == 0 )
    {
        setStatusMessage( "Goto System: unknown system" );
        return;
    }
    gotoTuple( sys, 0, 0, true );
}

void FourDScope3::handleNextSpec4D()
{
    ENABLED_IF( d_sort4D.size() > 1 );
    stepSpec4D( true );
}

void FourDScope3::handlePrevSpec4D()
{
    ENABLED_IF( d_sort4D.size() > 1 );
    stepSpec4D( false );
}

void FourDScope3::handleNextSpec2D()
{
    ENABLED_IF( d_sort2D.size() > 1 );
    stepSpec2D( true );
}

void FourDScope3::handlePrevSpec2D()
{
    ENABLED_IF( d_sort2D.size() > 1 );
    stepSpec2D( false );
}

void FourDScope3::handlePlaneShowOffs()
{
    CHECKED_IF( true, d_src2D->showOffs() );

    d_src2D->showOffs( !d_src2D->showOffs() );
    d_plane->getPlaneViewer()->update();
}

void FourDScope3::handlePlaneShowLinks()
{
    CHECKED_IF( true, d_src2D->showLinks() );

    d_src2D->showLinks( !d_src2D->showLinks() );
    d_plane->getPlaneViewer()->update();
}

void FourDScope3::handleOrthoShowLinks()
{
    CHECKED_IF( true, d_src4D->showLinks() );

    d_src4D->showLinks( !d_src4D->showLinks() );
    d_plane->getPlaneViewer()->update();
}

void FourDScope3::handleDeleteLinks()
{
    ENABLED_IF( !d_plane->getPointView()->getSel().empty() );

    SpinPointView::Selection sel = d_plane->getPointView()->getSel();
    SpinPointView::Selection::const_iterator p;
    SpinBase* base = d_pro->getSpins();
    Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Delete Links" );
    std::set<SpinLink*> test;
    SpinLink* l;
    for( p = sel.begin(); p != sel.end(); ++p )
    {
        l = (*p)[ DimX ]->findLink( (*p)[ DimY ] );
        if( l && test.count( l ) == 0 )
        {
            cmd->add( new UnlinkSpinCmd( base, (*p)[ DimX ], (*p)[ DimY ] ) );
            test.insert( l );
        }
    }
    cmd->handle( getAgent() );
}

void FourDScope3::handleLabelVerti()
{
    ENABLED_IF( d_plane->getPointView()->getSel().size() == 1 );

    SpinPoint tuple = *d_plane->getPointView()->getSel().begin();

    Gui2::UiFunction* a = Gui2::UiFunction::me();
    SpinLabel l;
    if( !SpinLabel::parse( a->property( "0" ).toString(), l ) )
    {
        Root::ReportToUser::alert( this, "Label Vertical Spin", "Invalid spin label syntax!" );
        return;
    }
    Root::Ref<LabelSpinCmd> cmd =
        new LabelSpinCmd( d_pro->getSpins(), tuple[ DimY ], l );
    cmd->handle( getAgent() );
    if( l.isNull())
        d_src2D->showNulls( true );
}

void FourDScope3::handleLabelHori()
{
    ENABLED_IF( d_plane->getPointView()->getSel().size() == 1 );

    SpinPoint tuple = *d_plane->getPointView()->getSel().begin();

    Gui2::UiFunction* a = Gui2::UiFunction::me();
    SpinLabel l;
    if( !SpinLabel::parse( a->property( "0" ).toString(), l ) )
    {
        Root::ReportToUser::alert( this, "Label Horizontal Spin", "Invalid spin label syntax!" );
        return;
    }
    Root::Ref<LabelSpinCmd> cmd =
        new LabelSpinCmd( d_pro->getSpins(), tuple[ DimX ], l );
    cmd->handle( getAgent() );
    if( l.isNull() )
        d_src2D->showNulls( true );
}

void FourDScope3::handleSetCandidates()
{
    if( d_plane->getPointView()->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane->getPointView()->getSel().begin();
    ENABLED_IF( tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() );

    SpinSystem* sys = tuple[ DimX ]->getSystem();

    CandidateDlg dlg( this, d_pro->getRepository() );
    dlg.setTitle( sys );
    if( dlg.exec() )
        d_pro->getSpins()->setCands( sys, dlg.d_cands );
}

void FourDScope3::handlePlaneShowInfered()
{
    CHECKED_IF( true, d_src2D->showInferred() );

    d_src2D->showInferred( !d_src2D->showInferred() );
    d_plane->getPlaneViewer()->update();
}

void FourDScope3::handleOrthoShowInfered()
{
    CHECKED_IF( true, d_src4D->showInferred() );

    d_src4D->showInferred( !d_src4D->showInferred() );
    d_ortho->getPlaneViewer()->update();
}

void FourDScope3::handlePlaneShowUnlabeled()
{
    CHECKED_IF( true, d_src2D->showNulls() );

    d_src2D->showNulls( !d_src2D->showNulls() );
    d_plane->getPlaneViewer()->update();
}

void FourDScope3::handleOrthoShowUnlabeled()
{
    CHECKED_IF( true, d_src4D->showNulls() );

    d_src4D->showNulls( !d_src4D->showNulls() );
    d_plane->getPlaneViewer()->update();
}

void FourDScope3::handlePlaneShowUnknown()
{
    CHECKED_IF( true, d_src2D->showUnknown() );

    d_src2D->showUnknown( !d_src2D->showUnknown() );
    d_plane->getPlaneViewer()->update();
}

void FourDScope3::handleOrthoShowUnknown()
{
    CHECKED_IF( true, d_src4D->showUnknown() );

    d_src4D->showUnknown( !d_src4D->showUnknown() );
    d_plane->getPlaneViewer()->update();
}

void FourDScope3::handleCreateLinks()
{
    CHECKED_IF( true, false );
}

void FourDScope3::handleDoPlanePathSim()
{
    CHECKED_IF( true, d_src2D->doPathsim() );

    d_src2D->doPathsim( !d_src2D->doPathsim() );
    d_plane->getPlaneViewer()->update();
}

void FourDScope3::handleDoOrthoPathSim()
{
    CHECKED_IF( true, d_src4D->doPathsim() );

    d_src4D->doPathsim( !d_src4D->doPathsim() );
    d_ortho->getPlaneViewer()->update();
}

void FourDScope3::handleDeleteLinks4D()
{
    ENABLED_IF( !d_spec4D.isNull() && !d_ortho->getPointView()->getSel().empty() );

    SpinPointView::Selection sel = d_ortho->getPointView()->getSel();
    SpinPointView::Selection::const_iterator p;
    Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Delete Links" );
    SpinLink* l;
    std::set<SpinLink*> test;
    for( p = sel.begin(); p != sel.end(); ++p )
    {
        l = (*p)[ DimX ]->findLink( (*p)[ DimY ] );
        if( l && test.count( l ) == 0 )
        {
            cmd->add( new UnlinkSpinCmd( d_pro->getSpins(), (*p)[ DimX ], (*p)[ DimY ] ) );
            test.insert( l );
        }
    }
    cmd->handle( getAgent() );
}

void FourDScope3::handleOrthoShowGhosts()
{
    CHECKED_IF( true, d_range2D->showGhosts() );

    d_range2D->showGhosts( !d_range2D->showGhosts() );
    d_range4D->showGhosts( d_range2D->showGhosts() );
}

void FourDScope3::handleAutoHold()
{
    CHECKED_IF( true, d_autoHold );
    d_autoHold = !d_autoHold;
}

void FourDScope3::handleHidePeak4D()
{
    if( d_ortho->getPointView()->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_ortho->getPointView()->getSel().begin();
    SpinLink* link = tuple[ DimX ]->findLink( tuple[ DimY ] );
    ENABLED_IF( link );
    Root::Ref<HideSpinLinkCmd> cmd = new HideSpinLinkCmd( d_pro->getSpins(), link, d_spec4D );
    cmd->handle( getAgent() );
    // TODO: Plural
}

void FourDScope3::handleGotoSpin()
{
    ENABLED_IF( d_spec4D );

    Gui2::UiFunction* a = Gui2::UiFunction::me();
    QString id;
    if( a->property( "0" ).isNull() )
    {
        bool ok	= FALSE;
        id	= QInputDialog::getText( "Goto Peak",
            "Please	enter a spin system id:",QLineEdit::Normal, "", &ok, this );
        if( !ok )
            return;
    }else
        id = a->property( "0" ).toString();

    SpinSystem* sys = d_pro->getSpins()->findSystem( id );
    if( sys == 0 )
    {
        setStatusMessage( "Goto Peak: unknown system" );
        return;
    }
    gotoTuple( sys, 0, 0, false );
}

void FourDScope3::handleOrthoSysAttr()
{
    ENABLED_IF( !d_spec4D.isNull() && !d_cur.isZero() && d_cur[DimX]->getSystem() );

    DynValueEditor::edit( this,
        d_pro->getRepository()->findObjectDef( Repository::keySpinSystem ),
        d_cur[DimX]->getSystem() );
}

void FourDScope3::handleOrthoLinkAttr()
{
    if( d_ortho->getPointView()->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_ortho->getPointView()->getSel().begin();
    SpinLink* l = tuple[ DimX ]->findLink( tuple[ DimY ] );
    ENABLED_IF( l );

    DynValueEditor::edit( this,
        d_pro->getRepository()->findObjectDef( Repository::keyLink ), l );
}

void FourDScope3::handleSetupPlaneOverlay()
{
    ENABLED_IF( true );

    OverlayManagerDlg dlg( d_plane, d_pro );
    dlg.exec();
}

void FourDScope3::handleSetupOrthoOverlay()
{
    ENABLED_IF( true );

    OverlayManagerDlg dlg( d_ortho, d_pro );
    dlg.exec();
}

void FourDScope3::handlePlaneUseLinkColors()
{
    CHECKED_IF( true, d_plane->getPointView()->getColors() );
    if( d_plane->getPointView()->getColors() )
        d_plane->getPointView()->setColors( 0 );
    else
        d_plane->getPointView()->setColors( d_pro->getRepository()->getColors() );
}

void FourDScope3::handleOrthoUseLinkColors()
{
    CHECKED_IF( true, d_ortho->getPointView()->getColors() );

    if( d_ortho->getPointView()->getColors() )
    {
        d_ortho->getPointView()->setColors( 0 );
    }else
    {
        d_ortho->getPointView()->setColors( d_pro->getRepository()->getColors() );
    }
}

void FourDScope3::handleSetLinkParams()
{
    if( d_plane->getPointView()->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane->getPointView()->getSel().begin();
    SpinLink* l = tuple[ DimX ]->findLink( tuple[ DimY ] );
    ENABLED_IF( l );
    const SpinLink::Alias& al = l->getAlias( getSpec2() );
    Dlg::LinkParams2 par;
    par.d_rating = al.d_rating;
    par.d_code = al.d_code;
    par.d_visible = al.d_visible;
    if( Dlg::getLinkParams2( this, par ) )
        d_pro->getSpins()->setAlias( l, getSpec2(),
                                     par.d_rating, par.d_code, par.d_visible );
}

void FourDScope3::handleSetLinkParams4D()
{
    if( d_spec4D.isNull() || d_ortho->getPointView()->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_ortho->getPointView()->getSel().begin();
    SpinLink* l = tuple[ DimX ]->findLink( tuple[ DimY ] );
    ENABLED_IF( l );
    const SpinLink::Alias& al = l->getAlias( d_spec4D );
    Dlg::LinkParams2 par;
    par.d_rating = al.d_rating;
    par.d_code = al.d_code;
    par.d_visible = al.d_visible;
    if( Dlg::getLinkParams2( this, par ) )
        d_pro->getSpins()->setAlias( l, d_spec4D,
        par.d_rating, par.d_code, par.d_visible );
}

void FourDScope3::handleGotoPoint()
{
    ENABLED_IF( true );

    PpmPoint orig = d_pointMdl->getPpmPoint(DimCount);
    GotoDlg dlg( this );
    Spectrum* spec = d_spec2D;
    if( d_spec4D )
        spec = d_spec4D;
    PpmCube cube;
    spec->getCube( cube );
    Dimension d;
    for( d = 0; d <	spec->getDimCount(); d++	)
        dlg.addDimension( cube[	d ], spec->getColor( d ),
            orig[ d ] );
    if( dlg.exec() )
    {
        for( d = 0; d <	spec->getDimCount(); d++ )
        {
            orig[ d ] = dlg.getValue( d );
        }
        setCursor( orig );
    }
}

void FourDScope3::handleNewPeakList()
{
    ENABLED_IF( true );
    if( !askToClosePeaklist() )
        return;

    Spectrum* spec = d_spec2D;
    if( d_spec4D && d_show4DPlane )
        spec = d_spec4D;
    setPeakList( new PeakList( spec ) );
}

void FourDScope3::handleOpenPeakList()
{
    ENABLED_IF( true );

    PeakList* pl = Dlg::selectPeakList( this, d_pro );
    if( pl == 0 )
        return;
    setPeakList( pl );
}

void FourDScope3::handleSavePeakList()
{
    ENABLED_IF( d_pl && d_pl->getId() == 0 );

    savePeakList();
}

void FourDScope3::handleMapPeakList()
{
    ENABLED_IF( d_pl );

    RotateDlg dlg( this, "Peaklist", "Spectrum" );
    Rotation rot( d_pl->getDimCount() );
    Spectrum* spec = d_spec2D;
    if( d_spec4D && d_show4DPlane )
        spec = d_spec4D;
    for( Dimension d = 0; d < d_pl->getDimCount(); d++ )
    {
        dlg.addDimension( d_pl->getAtomType( d ).getIsoLabel(),
        ( d < spec->getDimCount() )?spec->getColor( d ).getIsoLabel():"" );
        rot[ d ] = d;
    }

    if( dlg.rotate( rot ) )
        d_pl->setRotation( rot );
}

void FourDScope3::handlePickPlPeak()
{
    ENABLED_IF( d_pl );

    Q_ASSERT( d_pl );
    PeakSpace::PeakData pd;
    PeakModel::Params pp;
    pd.d_pos[ DimX ] = d_pointMdl->getPos( DimX );
    pd.d_pos[ DimY ] = d_pointMdl->getPos( DimY );
    pd.d_pos[ DimZ ] = d_pointMdl->getPos( DimZ );
    pd.d_pos[ DimW ] = d_pointMdl->getPos( DimW );
    Spectrum* spec = d_spec2D;
    if( d_spec4D && d_show4DPlane )
        spec = d_spec4D;
    PpmPoint p;
    p.assign( spec->getDimCount(), 0 );
    for( int i = 0; i < p.size(); i++ )
        p[i] = d_pointMdl->getPos(i);
    pd.d_amp = spec->getAt( p, true, d_plane->getFolding() );
    try
    {
        COP cop( true, true );
        d_pl->addPeak( pd, spec, cop );
        cop.d_done->registerForUndo( getAgent() );
    }catch( Root::Exception& e )
    {
        Root::ReportToUser::alert( this, "Pick Peak", e.what() );
    }
}

void FourDScope3::handleMovePlPeak()
{
    ENABLED_IF( d_pl && d_plane->getPeakView()->getSel().size() == 1 );

    Root::Index peak = *d_plane->getPeakView()->getSel().begin();
    PeakPos pos;
    PeakModel::Params pp;
    pos[ DimX ] = d_pointMdl->getPos( DimX );
    pos[ DimY ] = d_pointMdl->getPos( DimY );
    pos[ DimZ ] = d_pointMdl->getPos( DimZ );
    pos[ DimW ] = d_pointMdl->getPos( DimW );
    Spectrum* spec = d_spec2D;
    if( d_spec4D && d_show4DPlane )
        spec = d_spec4D;
    PpmPoint p;
    p.assign( spec->getDimCount(), 0 );
    for( int i = 0; i < p.size(); i++ )
        p[i] = d_pointMdl->getPos(i);
    Amplitude amp = spec->getAt( p, true, d_plane->getFolding() );
    try
    {
        COP cop( true, true );
        d_pl->setPos( peak, pos, amp, 0, cop );
        cop.d_done->registerForUndo( getAgent() );
    }catch( Root::Exception& e )
    {
        Root::ReportToUser::alert( this, "Move Peak", e.what() );
    }
}

void FourDScope3::handleMovePlAlias()
{
    ENABLED_IF( d_pl && d_plane->getPeakView()->getSel().size() == 1 );

    Root::Index peak = *d_plane->getPeakView()->getSel().begin();
    PeakPos pos;
    PeakModel::Params pp;
    pos[ DimX ] = d_pointMdl->getPos( DimX );
    pos[ DimY ] = d_pointMdl->getPos( DimY );
    pos[ DimZ ] = d_pointMdl->getPos( DimZ );
    pos[ DimW ] = d_pointMdl->getPos( DimW );
    Spectrum* spec = d_spec2D;
    if( d_spec4D && d_show4DPlane )
        spec = d_spec4D;
    PpmPoint p;
    p.assign( spec->getDimCount(), 0 );
    for( int i = 0; i < p.size(); i++ )
        p[i] = d_pointMdl->getPos(i);
    Amplitude amp = spec->getAt( p, true, d_plane->getFolding() );
    try
    {
        COP cop( true, true );
        d_pl->setPos( peak, pos, amp, spec, cop );
        cop.d_done->registerForUndo( getAgent() );
    }catch( Root::Exception& e )
    {
        Root::ReportToUser::alert( this, "Move Peak Alias", e.what() );
    }
}

void FourDScope3::handleLabelPlPeak()
{
    ENABLED_IF( d_pl && d_plane->getPeakView()->getSel().size() == 1 );

    Root::Index peak = ( *d_plane->getPeakView()->getSel().begin() );
    bool ok	= FALSE;
    QString res	= QInputDialog::getText( "Set Peak Label",
        "Please	enter a label:", QLineEdit::Normal, d_pl->getTag( peak ).data(), &ok, this );
    if( !ok )
        return;
    try
    {
        COP cop( true, true );
        d_pl->setTag( peak, res.toLatin1(), cop );
        cop.d_done->registerForUndo( getAgent() );
    }catch( Root::Exception& e )
    {
        Root::ReportToUser::alert( this, "Label Peak", e.what() );
    }
}

void FourDScope3::handleDeletePlPeaks()
{
    ENABLED_IF( d_pl && !d_plane->getPeakView()->getSel().empty() );

    Root::Ref<Root::MakroTransaction> cmd = new Root::MakroTransaction( "Delete Peaks" );
    try
    {
        PeakPlaneView::Selection sel = d_plane->getPeakView()->getSel();
        PeakPlaneView::Selection::const_iterator p;
        for( p = sel.begin(); p != sel.end(); ++p )
        {
            COP cop( true, true );
            d_pl->removePeak( (*p), cop );
            cmd->add( cop.d_done );
        }
    }catch( Root::Exception& e )
    {
        cmd->unexecute();
        Root::ReportToUser::alert( this, "Delete Peaks", e.what() );
        return;
    }
    cmd->registerForUndo( getAgent() );
}

void FourDScope3::handleEditPlPeakAtts()
{
    ENABLED_IF( d_pl && d_plane->getPeakView()->getSel().size() == 1 );

    Root::Index id = ( *d_plane->getPeakView()->getSel().begin() );
    Peak* peak = d_pl->getPeakList()->getPeak( id );
    DynValueEditor::edit( this,
        d_pro->getRepository()->findObjectDef( Repository::keyPeak ), peak );
}

void FourDScope3::handleDeleteAliasPeak()
{
    ENABLED_IF( !d_plane->getPointView()->getSel().empty() );

    SpinPointView::Selection sel = d_plane->getPointView()->getSel();
    SpinPointView::Selection::const_iterator p;
    Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Delete Spins" );
    std::set<Spin*> test;
    for( p = sel.begin(); p != sel.end(); ++p )
    {
        if( test.count( (*p)[ DimX ] ) == 0 )
        {
            cmd->add( new MoveSpinCmd( d_pro->getSpins(), (*p)[ DimX ],
            (*p)[ DimX ]->getShift(), // auf Home schieben l�cht Alias
            getSpec2() ) );
            test.insert( (*p)[ DimX ] );
        }
        if( test.count( (*p)[ DimY ] ) == 0 )
        {
            cmd->add( new MoveSpinCmd( d_pro->getSpins(), (*p)[ DimY ],
            (*p)[ DimY ]->getShift(), // auf Home schieben l�cht Alias
            getSpec2() ) );
            test.insert( (*p)[ DimY ] );
        }
    }
    cmd->handle( getAgent() );
}

void FourDScope3::handleGotoPlPeak()
{
    ENABLED_IF( d_pl );

    Gui2::UiFunction* a = Gui2::UiFunction::me();
    Root::Index id;
    if( a->property( "0" ).isNull() )
    {
        bool ok	= FALSE;
        id	= QInputDialog::getInteger( "Goto Peak",
            "Please	enter peak id:",
            0, -999999, 999999, 1, &ok, this );
        if( !ok )
            return;
    }else
        id = a->property( "0" ).toInt();

    try
    {
        PeakPos pos;
        d_pl->getPos( id, pos, (d_spec4D && d_show4DPlane)?d_spec4D:d_spec2D );
        PpmPoint p;
        p.assign( Root::Math::min( d_pl->getDimCount(), (d_show4DPlane)?DimCount:2 ), 0 );
        for( Dimension d = 0; d < p.size(); d++ )
            p[d] = pos[d];
        setCursor( p );
        if( p.size() > 2 )
            d_ortho->ensureCursorVisible();
    }catch( Root::Exception& e )
    {
        Root::ReportToUser::alert( this, "Goto Peak", e.what() );
    }
}

void FourDScope3::updateCaption()
{
	QString str;
	str.sprintf( "FourDScope - %s", getSpec()->getName() );
    setCaption( str );
}

void FourDScope3::buildMenus()
{
    /////// File
    Gui2::AutoMenu* menuFile = new Gui2::AutoMenu( tr("&File"), this );
    menuFile->addCommand( tr("Save"), this, SLOT(handleSave()), tr( "CTRL+S" ) );
    menuFile->addCommand( tr("&Make Overlay Colors Persistent"), this,SLOT(handleSaveColors()) );
    menuFile->addSeparator();
    menuFile->addCommand( tr("Print Plane..."), this, SLOT(handlePrintPlane()), tr("CTRL+P") );
    menuFile->addCommand( tr("Print Ortho..."), this, SLOT(handlePrintOrtho()));
    menuFile->addSeparator();
    menuFile->addCommand( tr("Export &Plane Peaklist..."), this, SLOT(handleExportPlanePeaks()) );
    menuFile->addCommand( tr("Export &Ortho Peaklist..."), this, SLOT(handleExportOrthoPeaks()) );
    menuFile->addSeparator();
    menuFile->addCommand( tr("Show Plane in MonoScope..."), this, SLOT(handlePlaneToMonoScope()) );
    menuFile->addCommand( tr("Show Ortho in MonoScope..."), this, SLOT(handleOrthoToMonoScope()) );
    menuFile->addSeparator();
    menuFile->addCommand( tr("Close"), this, SLOT(handleClose()), tr("CTRL+W") );

    /////// Edit
    Gui2::AutoMenu* menuEdit = new Gui2::AutoMenu( tr("&Edit"), this );
    menuEdit->addCommand( tr("Undo"), this, SLOT(handleUndo()), tr("CTRL+Z") );
    menuEdit->addCommand( tr("Redo"), this, SLOT(handleRedo()), tr("CTRL+Y") );
    menuEdit->addSeparator();
    menuEdit->addCommand( tr("&Calibrate Plane"), this, SLOT(handleSpecCalibrate()) );
    menuEdit->addCommand( tr("Calibrate Ortho"), this, SLOT(handleStripCalibrate()) );
    menuEdit->addSeparator();
    menuEdit->addCommand( tr("Set Peak Width..."), this, SLOT(handleSetPeakWidth()) );
    menuEdit->addCommand( tr("Set Spin Matcher Tolerance..."), this,SLOT(handleSetMatcherTol()) );
    menuEdit->addCommand( tr("Setup Link Colors..."), this, SLOT(handleSetPeakColors() ) );

    /////// Navigate
    Gui2::AutoMenu* menuNavi = new Gui2::AutoMenu( tr("&Navigate"), this );
    menuNavi->addCommand( tr("Backward"), this, SLOT(handleBackward()), tr("CTRL+B") );
    menuNavi->addCommand( tr("Forward"), this,SLOT(handleForward()) );
    menuNavi->addSeparator();
    menuNavi->addCommand( tr("Goto System..."), this,SLOT(handleGotoSystem()), tr("CTRL+G") );
    menuNavi->addCommand( tr("Goto Point..."), this,SLOT(handleGotoPoint()));
    menuNavi->addSeparator();
    menuNavi->addCommand( "Hold Reference", this,SLOT(handleHoldReference()) );
    menuNavi->addCommand( "Auto Hold", this,SLOT(handleAutoHold()) );
    menuNavi->addSeparator();

    menuNavi->addCommand( tr("Center to Peak"), this,SLOT(handleAutoCenter()) );
    menuNavi->addCommand( tr("Plane Global Zoom Sync"), d_plane, SLOT(handleGlobalRangeSync()) );
    menuNavi->addCommand( tr("Ortho Global Zoom Sync"), d_ortho, SLOT(handleGlobalRangeSync()) );
    menuNavi->addCommand( tr("Plane Global Cursor Sync"), d_plane, SLOT(handleGlobalCursorSync()) );
    menuNavi->addCommand( tr("Ortho Global Cursor Sync"), d_ortho, SLOT(handleGlobalCursorSync()) );
    menuNavi->addSeparator();
    menuNavi->addCommand( tr("Show System List"), this, SLOT(handleShowSysList()) );
    menuNavi->addCommand( tr("Show Hori. Spin in List"), this, SLOT( handleSyncHori() ) );
    menuNavi->addCommand( tr("Show Verti. Spin in List"), this, SLOT( handleSyncVerti() ) );
    menuNavi->addSeparator();
    menuNavi->addCommand( tr("Show Plane Point List"), this, SLOT(handleShowPlanePointList()) );
    menuNavi->addCommand( tr("Show Ortho Point List"), this, SLOT(handleShowOrthoPointList()) );

    /////// Top Plane View
    Gui2::AutoMenu* menuView = new Gui2::AutoMenu( tr("Plane &View"), this );
    menuView->addMenu( d_popSpec2D );
    menuView->addCommand( tr("Fit Window"), d_plane, SLOT(handleFitWindow()), tr("CTRL+Home") );
    menuView->addSeparator();
    menuView->addCommand( tr("Show Intensity"), d_plane, SLOT(handleShowIntensity()) );
    menuView->addCommand( tr("Adjust Intensity"), d_plane, SLOT( handleAdjustIntensity() ) );
    menuView->addCommand( tr("Show Contour"), d_plane, SLOT(handleShowContour()) );
    menuView->addCommand( tr("Set Contour Parameters..."), d_plane, SLOT(handleContourParams() ) );
    menuView->addCommand( tr("Setup &Overlays..."), this,SLOT(handleSetupPlaneOverlay()) );
    menuView->addSeparator();
    menuView->addCommand( tr("Set Resolution..."), d_plane, SLOT(handleSetResolution()) );
    menuView->addCommand( tr("Show Low Resolution"), d_plane, SLOT(handleShowLowRes()) );
    menuView->addCommand( tr("Show Folded"), d_plane, SLOT(handleShowFolded()) );
    menuView->addSeparator();
    menuView->addCommand( tr("Do Pathway Simulation"), this, SLOT(handleDoPlanePathSim()) );
    menuView->addCommand( tr("Show Infered Peaks"), this, SLOT(handlePlaneShowInfered()) );
    menuView->addCommand( tr("Resolve Projected Spins"), this, SLOT(handlePlaneShowOffs()) );
    menuView->addCommand( tr("Show Unlabeled Peaks"), this, SLOT(handlePlaneShowUnlabeled()) );
	menuView->addCommand( tr("Show Peaks with foreign Labels"), this, SLOT(handlePlaneShowUnknown()) );
    menuView->addCommand( tr("Show Ghost Labels"), d_plane, SLOT(handleGhostLabels()) );
    Gui2::AutoMenu* sub = new Gui2::AutoMenu( tr("Spin Label Format"), this, false );
    menuView->addMenu( sub );
    for( int i = SpinPointView3::None; i < SpinPointView3::End; i++ )
        sub->addCommand( SpinPointView3::menuText[ i ], d_plane, SLOT(handleViewLabels() ) )->setProperty("0", i );
    menuView->addSeparator();
    menuView->addCommand( tr("Show Spin Links"), this, SLOT(handlePlaneShowLinks()) );
    menuView->addCommand( tr("Show Hidden Links"), d_plane, SLOT(handleShowHiddenLinks()) );
    menuView->addCommand( tr("Use Link Color Codes"), this, SLOT(handlePlaneUseLinkColors()) );
    menuView->addSeparator();
    menuView->addCommand( tr("Add &Horizontal Ruler"), d_plane, SLOT(handleAddRulerVerti()) );
    menuView->addCommand( tr("Add &Vertical Ruler"), d_plane, SLOT(handleAddRulerHori()) );
    menuView->addCommand( tr("Auto Set Rulers to Reference"), this, SLOT(handleAutoRuler()) );
    menuView->addCommand( tr("Remove Selected Rulers"), d_plane, SLOT(handleRemoveRulers()));
    menuView->addCommand( tr("Remove All Rulers"), d_plane, SLOT(handleRemoveAllRulers()) );
    menuView->addSeparator();
    menuView->addCommand( tr("Auto Hide Ortho Slices"), this, SLOT(handleAutoHide()) );
    menuView->addCommand( tr("Show 4D D1/D2 Plane"), this,SLOT(handleShowOrthoPlane()), tr("CTRL+4") );
    menuView->addCommand( tr("Set 4D Plane Colors"), this, SLOT( handle4dPlaneColors() ) );

    ////// Top Plane Spins
    Gui2::AutoMenu* menuPlane = new Gui2::AutoMenu( tr("&Plane Spins"), this );
    menuPlane->addCommand( tr("&Pick New System"), this, SLOT(handlePickSystem() ) );
    menuPlane->addCommand( tr("Propose System..."), this, SLOT(handleProposePeak()) );
    menuPlane->addCommand( tr("Extend Horizontally"), this, SLOT(handlePickHori()) );
    menuPlane->addCommand( tr("Propose Horizontally..."), this, SLOT(handleProposeHori()) );
    menuPlane->addCommand( tr("Extend Vertically"), this, SLOT(handlePickVerti()) );
    menuPlane->addCommand( tr("Propose Vertically..."), this, SLOT(handleProposeVerti()) );
    menuPlane->addCommand( tr("&Move Spins"), this, SLOT(handleMovePeak()) );
    menuPlane->addCommand( tr("Move Spin &Aliases"), this, SLOT(handleMovePeakAlias()) );
    menuPlane->addCommand( tr("&Label Spins..."), this, SLOT(handleLabelPeak()) );
    menuPlane->addSeparator();
    menuPlane->addCommand( tr("Hide/Show Link"), this, SLOT(handleHidePeak()) );
    menuPlane->addCommand( tr("Set Link Params..."), this, SLOT(handleSetLinkParams()) );
    menuPlane->addSeparator();
    menuPlane->addCommand( tr("Un-Alias Peaks"), this,SLOT(handleDeleteAliasPeak()) );
    menuPlane->addCommand( tr("Delete Peaks"), this,SLOT(handleDeletePeak()) );
    menuPlane->addCommand( tr("Delete Spin Links"), this,SLOT(handleDeleteLinks()) );
    menuPlane->addCommand( tr("Delete Horizontal Spin"), this,SLOT(handleDeleteSpinX()) );
    menuPlane->addCommand( tr("Delete Vertical Spin"), this,SLOT(handleDeleteSpinY()) );
    menuPlane->addSeparator();
    menuPlane->addCommand( tr("Horizontal Spin Atts..."), this,SLOT(handlePlaneSpinXAttr()) );
    menuPlane->addCommand( tr("Vertical Spin Atts..."), this,SLOT(handlePlaneSpinYAttr()) );
    menuPlane->addCommand( tr("Horizontal System Atts..."), this,SLOT(handlePlaneSysXAttr()) );
    menuPlane->addCommand( tr("Vertical System Atts..."), this,SLOT(handlePlaneSysYAttr()) );
    menuPlane->addCommand( tr("Spin Link Atts..."), this,SLOT(handlePlaneLinkAttr()) );

    /////// Top Assignment
    Gui2::AutoMenu* menuAssig = new Gui2::AutoMenu( tr("&Assignment"), this );
    menuAssig->addCommand( tr("&Link Systems..."), this, SLOT(handleLinkSystems()) );
    menuAssig->addCommand( tr("Unlink &Predecessor"), this, SLOT(handleUnlinkPred()) );
    menuAssig->addCommand( tr("Unlink &Successor"), this, SLOT(handleUnlinkSucc()) );
    menuAssig->addSeparator();
    menuAssig->addCommand( tr("&Assign System..."), this, SLOT(handleAssign()) );
    menuAssig->addCommand( tr("&Unassign System"), this, SLOT(handleUnassign()) );
    menuAssig->addCommand( tr("Set System &Type..."), this,SLOT(handleSetSystemType()) );
    menuAssig->addCommand( tr("Set Assig. Candidates..."), this,SLOT(handleSetCandidates()) );
    menuAssig->addCommand( tr("Show Ali&gnment..."), this,SLOT(handleShowAlignment()) );

    ////// Top Plane Peaks
    Gui2::AutoMenu* menuPeaks = new Gui2::AutoMenu( tr("Plane &Peaks"), this );
    menuPeaks->addCommand( tr("&New Peaklist"),this,SLOT(handleNewPeakList()) );
    menuPeaks->addCommand( tr("&Open Peaklist..."),this,SLOT(handleOpenPeakList()) );
    menuPeaks->addCommand( tr("&Add to Repository..."),this,SLOT(handleSavePeakList()) );
    menuPeaks->addCommand( tr("Map to Spectrum..."),this,SLOT(handleMapPeakList()) );
    menuPeaks->addCommand( tr("Select Color..."),d_plane,SLOT(handleSetPlColor()) );
    sub = new Gui2::AutoMenu( tr("Peak Label Format"), this, false );
    menuPeaks->addMenu( sub );
    for( int i = PeakPlaneView3::NONE; i < PeakPlaneView3::END; i++ )
    {
        sub->addCommand( PeakPlaneView3::menuText[ i ], d_plane,
                         SLOT(handleViewPlLabels()) )->setProperty("0", i );
    }
    menuPeaks->addSeparator();
    menuPeaks->addCommand( tr("&Pick Peak"),this,SLOT(handlePickPlPeak()) );
    menuPeaks->addCommand( tr("&Move Peak"),this,SLOT(handleMovePlPeak()) );
    menuPeaks->addCommand( tr("&Move Peak Alias"),this,SLOT(handleMovePlAlias()) );
    menuPeaks->addCommand( tr("&Label Peak..."),this,SLOT(handleLabelPlPeak()) );
    menuPeaks->addCommand( tr("&Delete Peaks"),this,SLOT(handleDeletePlPeaks()) );
    menuPeaks->addCommand( tr("&Edit Attributes..."),this,SLOT(handleEditPlPeakAtts()) );

    /////// Top Ortho View
    menuView = new Gui2::AutoMenu( tr("Ortho View"), this );
    menuView->addMenu( d_popSpec4D );
    menuView->addCommand( tr("Fit Window"), d_ortho, SLOT(handleFitWindow()), tr("CTRL+SHIFT+Home") );
    menuView->addSeparator();
    menuView->addCommand( tr("Show Intensity"), d_ortho, SLOT(handleShowIntensity()) );
    menuView->addCommand( tr("Adjust Intensity"), d_ortho, SLOT( handleAdjustIntensity() ) );
    menuView->addCommand( tr("Show Contour"), d_ortho, SLOT(handleShowContour()) );
    menuView->addCommand( tr("Set Contour Parameters..."), d_ortho, SLOT(handleContourParams() ) );
    menuView->addCommand( tr("Setup &Overlays..."), this, SLOT(handleSetupOrthoOverlay()) );
    menuView->addSeparator();
    menuView->addCommand( tr("Set Resolution..."), d_ortho, SLOT(handleSetResolution()) );
    menuView->addCommand( tr("Show Low Resolution"), d_ortho, SLOT(handleShowLowRes()) );
    menuView->addCommand( tr("Show Folded"), d_ortho, SLOT(handleShowFolded()) );
    menuView->addSeparator();
    menuView->addCommand( tr("Do Pathway Simulation"), this, SLOT(handleDoOrthoPathSim()) );
    menuView->addCommand( tr("Show Infered Peaks"), this, SLOT(handleOrthoShowInfered()) );
    menuView->addCommand( tr("Resolve Projected Spins"), this, SLOT(handleOrthoShowOffs()) );
    menuView->addCommand( tr("Show Unlabeled Peaks"), this, SLOT(handleOrthoShowUnlabeled()) );
	menuView->addCommand( tr("Show Peaks with foreign Labels"), this, SLOT(handleOrthoShowUnknown()) );
    menuView->addCommand( tr("Show Ghost Peaks"), this, SLOT(handleOrthoShowGhosts()) );
    menuView->addCommand( tr("Show Ghost Labels"), d_ortho, SLOT(handleGhostLabels()) );
	menuView->addCommand( tr("Filter by selected System"), this, SLOT(handleFilterBySystem()) );
    sub = new Gui2::AutoMenu( tr("Spin Label Format"), this, false );
    menuView->addMenu( sub );
    for( int i = SpinPointView3::None; i < SpinPointView3::End; i++ )
        sub->addCommand( SpinPointView3::menuText[ i ], d_ortho, SLOT(handleViewLabels() ) )->setProperty("0", i );
    menuView->addSeparator();
    menuView->addCommand( tr("Show Spin Links"), this, SLOT(handleOrthoShowLinks()) );
    menuView->addCommand( tr("Show Hidden Links"), d_ortho, SLOT(handleShowHiddenLinks()) );
    menuView->addCommand( tr("Use Link Color Codes"), this, SLOT(handleOrthoUseLinkColors()) );
    menuView->addSeparator();
    menuView->addCommand( tr("Add &Horizontal Ruler"), d_ortho,SLOT(handleAddRulerVerti()) );
    menuView->addCommand( tr("Add &Vertical Ruler"), d_ortho,SLOT(handleAddRulerHori()) );
    menuView->addCommand( tr("Remove Selected Rulers"), d_ortho,SLOT(handleRemoveRulers()));
    menuView->addCommand( tr("Remove All Rulers"), d_ortho,SLOT(handleRemoveAllRulers()) );
	menuView->addSeparator();
	menuView->addCommand( tr("Hold Slices"), this,SLOT(handleHoldOrthoCur() ), tr("CTRL+0"), true );

    ////// Top Ortho Spins
    Gui2::AutoMenu* menuStrip = new Gui2::AutoMenu( tr("&Ortho Spins"), this );
    menuStrip->addCommand( tr("&Pick Spins"),this,SLOT(handlePickSpin4D()) );
    menuStrip->addCommand( tr("Pick Labels..."),this,SLOT(handlePickLabel4D()) );
    menuStrip->addCommand( tr("Propose &Spins..."),this,SLOT(handleProposeSpin4D()) );
    menuStrip->addCommand( tr("&Move Spins"),this,SLOT(handleMoveSpin4D()) );
    menuStrip->addCommand( tr("&Move Spin Aliasses"),this,SLOT(handleMoveSpinAlias4D()) );
    menuStrip->addCommand( tr("&Label Spins..."),this,SLOT(handleLabelSpin4D()) );
    menuStrip->addSeparator();
    menuStrip->addCommand( tr("Hide/Show Link"),this,SLOT(handleHidePeak4D()) );
    menuStrip->addCommand( tr("Set Link Params..."),this,SLOT(handleSetLinkParams4D()) );
    menuStrip->addSeparator();
    menuStrip->addCommand( tr("&Delete Spins"),this,SLOT(handleDeleteSpins4D()) );
    menuStrip->addCommand( tr("Delete Spin &Links"),this,SLOT(handleDeleteLinks4D()) );
    menuStrip->addSeparator();
    menuStrip->addCommand( tr("Horizontal Spin Atts..."),this,SLOT(handleOrthoSpinXAttr()) );
    menuStrip->addCommand( tr("Vertical Spin..."),this,SLOT(handleOrthoSpinYAttr()) );
    menuStrip->addCommand( tr("System..."),this,SLOT(handleOrthoSysAttr()) );
    menuStrip->addCommand( tr("Spin Link..."),this,SLOT(handleOrthoLinkAttr()) );

    Gui2::AutoMenu* menuHelp = new Gui2::AutoMenu( tr("&?"), this );
    menuHelp->addCommand( tr("About..."), this, SLOT(handleHelpAbout()) );

}

void FourDScope3::buildViews()
{
    QSplitter* split = new QSplitter( Qt::Horizontal, this );
    split->setOpaqueResize(false);
    setCentralWidget( split );

    split->addWidget( d_plane );
    split->addWidget( d_ortho );
    split->setHandleWidth( d_plane->handleWidth() );

    QDockWidget* dock = new QDockWidget( tr("Spin Systems"), this );
    dock->setAllowedAreas( Qt::AllDockWidgetAreas );
    dock->setFeatures( QDockWidget::AllDockWidgetFeatures );
    dock->setFloating( false );
    dock->setVisible( false );
    addDockWidget( Qt::RightDockWidgetArea, dock );

    d_sysList = new StripListGadget2( dock, getAgent(), d_pro, true );
    dock->setWidget( d_sysList );
    connect( d_sysList,SIGNAL(returnPressed(Gui::ListViewItem*)),this,SLOT(onListActivate()) );
    connect( d_sysList,SIGNAL(doubleClicked(Gui::ListViewItem*)), this, SLOT(onListActivate()) );

    dock = new QDockWidget( tr("Plane Points"), this );
    dock->setAllowedAreas( Qt::AllDockWidgetAreas );
    dock->setFeatures( QDockWidget::AllDockWidgetFeatures );
    dock->setFloating( false );
    dock->setVisible( false );
    addDockWidget( Qt::RightDockWidgetArea, dock );

    d_planePointList = new SpinPointList(dock);
    d_planePointList->setSpace( d_plane->getPointView()->getModel() );
    connect( d_planePointList, SIGNAL(doubleClicked(Spec::SpinSpace::Element)),
             this, SLOT(onGoto(Spec::SpinSpace::Element)) );
    connect( d_planePointList, SIGNAL(singleClicked(Spec::SpinSpace::Element)),
             this, SLOT(onShowPlanePoint(Spec::SpinSpace::Element)) );
    dock->setWidget( d_planePointList );

    dock = new QDockWidget( tr("Ortho Points"), this );
    dock->setAllowedAreas( Qt::AllDockWidgetAreas );
    dock->setFeatures( QDockWidget::AllDockWidgetFeatures );
    dock->setFloating( false );
    dock->setVisible( false );
    addDockWidget( Qt::RightDockWidgetArea, dock );

    d_orthoPointList = new SpinPointList(dock);
    d_orthoPointList->setSpace( d_ortho->getPointView()->getModel() );
    connect( d_orthoPointList, SIGNAL(doubleClicked(Spec::SpinSpace::Element)),
             this, SLOT(onGoto(Spec::SpinSpace::Element)) );
    connect( d_orthoPointList, SIGNAL(singleClicked(Spec::SpinSpace::Element)),
             this, SLOT(onShowOrthoPoint(Spec::SpinSpace::Element)) );
    dock->setWidget( d_orthoPointList );

    Gui2::AutoMenu* menu = new Gui2::AutoMenu( d_sysList, true );
    menu->addCommand( "Goto...", this, SLOT(handleGoto()) );
    menu->addCommand( "Use 4D Navigation", this, SLOT(handleGoto4D()) );
    menu->addCommand( "Show 4D Values", this, SLOT(handleUse4DSpec()) );
    menu->addSeparator();
    d_sysList->addCommands( menu );

    if( d_use4D )
        d_sysList->setSpec( d_spec4D );
    else
        d_sysList->setSpec( d_spec2D );
}

Root::Ref<PointSet> FourDScope3::createStripPeaks()
{
    Spectrum* spec = d_spec4D;
	PpmPoint p( 0, 0, 0, 0 );
	PointSet::Assig a;
	Root::Index id;
	Root::Ref<PointSet> ps = Factory::createEasyPeakList( spec );

    LinkFilterRotSpace::Iterator i, _end = d_rot4D->end();
    LinkFilterRotSpace::Element e;
    SpinPointView3::LabelType l = d_ortho->getPointView()->getLabelType();
	SpinLink* link;
    for( i = d_rot4D->begin(); i != _end; ++i )
	{
        d_rot4D->fetch( i, e );
		if( e.isGhost() || !e.isInfer() && e.isHidden() )
			continue;
		p[ DimX ] = e.d_point[ DimX ]->getShift( spec );
		p[ DimY ] = e.d_point[ DimY ]->getShift( spec );
		p[ DimZ ] = e.d_point[ DimZ ]->getShift( spec );
        p[ DimW ] = e.d_point[ DimW ]->getShift( spec );
		a[ DimX ] = e.d_point[ DimX ]->getId();
		a[ DimY ] = e.d_point[ DimY ]->getId();
		a[ DimZ ] = e.d_point[ DimZ ]->getId();
        a[ DimW ] = e.d_point[ DimW ]->getId();
		id = ps->getNextId();
		ps->setPoint( id, p );
		ps->setAssig( id, a );
		if( link = e.d_point[ DimX ]->findLink( e.d_point[ DimY ] ) )
			ps->setCode( id, link->getAlias( spec ).d_code );
		else if( link = e.d_point[ DimX ]->findLink( e.d_point[ DimZ ] ) )
			ps->setCode( id, link->getAlias( spec ).d_code );
        else if( link = e.d_point[ DimX ]->findLink( e.d_point[ DimW ] ) )
			ps->setCode( id, link->getAlias( spec ).d_code );
        else if( link = e.d_point[ DimZ ]->findLink( e.d_point[ DimW ] ) )
			ps->setCode( id, link->getAlias( spec ).d_code );

        ps->setComment( id, SpinPointView3::formatLabel( e.d_point, l, DimZ ) );
	}
	return ps;
}

bool FourDScope3::askToCloseWindow() const
{
    return const_cast<FourDScope3*>(this)->askToClosePeaklist();
}

void FourDScope3::handle(Root::Message& msg)
{
    if( d_lock )
        return;
    BEGIN_HANDLER();
    MESSAGE( ViewAreaMdl3::UpdRange, a, msg )
    {
        Locker l(d_lock);
        if( a->getOrigin() == d_plane->getPlaneViewer()->getArea() )
        {
            planeAreaUpdated(a);
            msg.consume();
        }
    }
    MESSAGE( PointMdl::Updated, a, msg )
    {
        Locker l(d_lock);
        const bool hasX = a->hasDim(DimX);
        const bool hasY = a->hasDim(DimY);
        const bool hasW = a->hasDim(DimW);
        const bool hasZ = a->hasDim(DimZ);
        if( hasX || hasY )
        {
            show4dPlaneSlice( false ); // der andere
            selectCurSystem();
            d_ortho->ensureCursorVisible(); // da in Ortho die Message nicht relevant ist da nur x und y.
        }
        if( hasW || hasZ )
        {
            show4dPlaneSlice( true );
        }
        notifyCursor( hasX || hasY );
        msg.consume();
    }
    MESSAGE( SpectrumPeer::Added, a, msg )
    {
        Q_UNUSED(a)
        Locker l(d_lock);
        updateSpecPop2D();
        updateSpecPop4D();
    }
    MESSAGE( SpectrumPeer::Removed, a, msg )
    {
        Q_UNUSED(a)
        Locker l(d_lock);
        updateSpecPop2D();
        updateSpecPop4D();
    }
    MESSAGE( Spin::Update, a, msg )
    {
        Locker l(d_lock);
        switch( a->getType() )
        {
        case Spin::Update::Delete:
            {
                for( int i = 0; i < d_ref.maxSize(); i++ )
                {
                    if( d_ref[ i ] == a->getSpin() )
                    {
                        d_ref.zero();
                        updateRef();
                        break;
                    }
                }
            }
            break;
        case Spin::Update::Label:
        case Spin::Update::System:
            {
                for( int i = 0; i < d_ref.maxSize(); i++ )
                {
                    if( d_ref[ i ] == a->getSpin() )
                    {
                        updateRef();
                        break;
                    }
                }
            }
            break;
        case Spin::Update::All:
            d_ref.zero();
            updateRef();
            break;
        default:
            break;
        }
        msg.consume();
    }HANDLE_ELSE()
    {
        GenericScope::handle( msg );
    }
    END_HANDLER();
}

void FourDScope3::handleShowSysList()
{
    CHECKED_IF( true, d_sysList->parentWidget()->isVisible() );

    d_sysList->parentWidget()->setVisible( !d_sysList->parentWidget()->isVisible() );
}

void FourDScope3::handleShowPlanePointList()
{
    CHECKED_IF( true, d_planePointList->parentWidget()->isVisible() );

    d_planePointList->parentWidget()->setVisible( !d_planePointList->parentWidget()->isVisible() );
}

void FourDScope3::handleShowOrthoPointList()
{
    CHECKED_IF( true, d_orthoPointList->parentWidget()->isVisible() );

    d_orthoPointList->parentWidget()->setVisible( !d_orthoPointList->parentWidget()->isVisible() );
}

void FourDScope3::handleSetTolerance()
{
    ENABLED_IF( true );

    Gui2::UiFunction* a = Gui2::UiFunction::me();
    AtomType t = AtomType::parseLabel( a->property( "0" ).toByteArray() );
	if( t.isNone() )
	{
        setStatusMessage( "Set Tolerance: invalid atom type" );
		return;
	}
    PPM p = a->property( "1" ).toDouble();
    d_pro->getMatcher()->setTol( t, p );
}

void FourDScope3::handleExportPlanePeaks()
{
    ENABLED_IF( true );

    QString fileName = QFileDialog::getSaveFileName( this, "Export 2D Pealist",
                                                     Root::AppAgent::getCurrentDir(),
                                                     "*.peaks", 0, QFileDialog::DontConfirmOverwrite );
	if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );
	if( info.extension( false ).upper() != "PEAKS" )
		fileName += ".peaks";
	info.setFile( fileName );
	if( info.exists() )
	{
        if( QMessageBox::warning( this, "Save As",
			"This file already exists. Do you want to overwrite it?",
			"&OK", "&Cancel" ) != 0 )
			return;
	}
	Root::AppAgent::setCurrentDir( info.dirPath( true ) );
	try
	{
		Root::Ref<PointSet> ps = createPlanePeaks();
		ps->saveToFile( fileName.toAscii().data() );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Error Exporting Peaklist", e.what() );
	}
}

Root::Ref<PointSet> FourDScope3::createPlanePeaks()
{
    Spectrum* spec = getSpec2();
	PpmPoint p( 0, 0 );
	PointSet::Assig a;
	Root::Index id;
	Root::Ref<PointSet> ps = Factory::createEasyPeakList( spec );
    LinkFilterRotSpace::Iterator i, _end = d_rot2D->end();
    LinkFilterRotSpace::Element e;
    SpinPointView3::LabelType l = d_plane->getPointView()->getLabelType();
	SpinLink* link;
    for( i = d_rot2D->begin(); i != _end; ++i )
	{
        d_rot2D->fetch( i, e );
		if( e.isGhost() || e.isHidden() )
			continue;
		p[ DimX ] = e.d_point[ DimX ]->getShift( spec );
		p[ DimY ] = e.d_point[ DimY ]->getShift( spec );
		a[ DimX ] = e.d_point[ DimX ]->getId();
		a[ DimY ] = e.d_point[ DimY ]->getId();
		id = ps->getNextId();
		ps->setPoint( id, p );
		ps->setAssig( id, a );
        if( ( link = e.d_point[ DimX ]->findLink( e.d_point[ DimY ] ) ) )
			ps->setCode( id, link->getAlias( spec ).d_code );
	
        QByteArray buf = SpinPointView3::formatLabel( e.d_point, l, DimY );
		ps->setComment( id, buf );
	}
	return ps;
}

void FourDScope3::handleSetMatcherTol()
{
    ENABLED_IF( true );

    Spectrum* spec = d_spec2D;
    if( !d_spec4D.isNull() )
        spec = d_spec4D;

    std::set<AtomType> atoms;
    for( Dimension d = 0; d < spec->getDimCount(); d++ )
        atoms.insert( spec->getColor( d ) );
    PpmPoint p;
    QStringList labels;
    QList<AtomType> order;
    for( std::set<AtomType>::const_iterator i = atoms.begin(); i != atoms.end(); ++i )
    {
        const AtomType a = (*i);
        p.push_back( d_pro->getMatcher()->getTol( a ) );
        labels.append( QString("%1:").arg(a.getIsoLabel()) );
        order.append( a );
    }
    if( Dlg::getPpmPoint( this, p, tr("Set Spin Matcher Tolerance (PPM)"), labels, true ) )
    {
        // RK 31.10.03: tol <= 0 neu erlaubt
        for( int i = 0; i < p.size(); i++ )
            d_pro->getMatcher()->setTol( order[i], p[i] );
    }
}

void FourDScope3::handleExportOrthoPeaks()
{
    Spectrum* spec = d_spec4D;
    ENABLED_IF( spec );

    QString fileName = QFileDialog::getSaveFileName( this, "Export 4D Pealist",
                                                     Root::AppAgent::getCurrentDir(),
                                                     "*.peaks", 0, QFileDialog::DontConfirmOverwrite );
	if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );

	if( info.extension( false ).upper() != "PEAKS" )
		fileName += ".peaks";
	info.setFile( fileName );
	if( info.exists() )
	{
        if( QMessageBox::warning( this, "Save As",
			"This file already exists. Do you want to overwrite it?",
			"&OK", "&Cancel" ) != 0 )
			return;
	}
	Root::AppAgent::setCurrentDir( info.dirPath( true ) );
	try
	{
		Root::Ref<PointSet> ps = createStripPeaks();
		if( ps.isNull() )
			throw Root::Exception( "Cannot create Peaklist" );
		ps->saveToFile( fileName.toAscii().data() );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Error Exporting Peaklist", e.what() );
	}catch( ... )
	{
		Root::ReportToUser::alert( this, "Error Exporting Peaklist",
			"Unknown Exception" );
	}
}

extern void createMonoScope( Root::Agent* a, Spec::Spectrum* s, 
					 Spec::Project* p, PeakList* l, const Rotation& r );

void FourDScope3::handleOrthoToMonoScope()
{
    Spectrum* spec = d_spec4D;
    ENABLED_IF( spec );

	try
	{
		Root::Ref<PointSet> ps = createStripPeaks();
		if( ps.isNull() )
			throw Root::Exception( "Cannot create Peaklist" );
		Root::Ref<PeakList> pl = new PeakList( spec );
		pl->append( ps ); // pl ist nun eine identische Kopie von ps.
		pl->setName( "Export from FourDScope3" );
        createMonoScope( getAgent()->getParent(), spec, d_pro, pl, Rotation() );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Error Exporting Peaklist", e.what() );
	}catch( ... )
	{
		Root::ReportToUser::alert( this, "Error Exporting Peaklist",
			"Unknown Exception" );
	}
}

void FourDScope3::handleGoto()
{
    ENABLED_IF( d_sysList );
    gotoTuple( d_sysList->getSelectedStrip(), d_sysList->getSelectedSpin(),
        d_sysList->getSelectedLink(), !d_goto4D );
    d_plane->getPlaneViewer()->setFocus();
}

void FourDScope3::handleSyncHori()
{
    Spin* spin = getSel( true );
    ENABLED_IF( d_sysList && spin );
    d_sysList->gotoSpin( spin );
}

void FourDScope3::handleSyncVerti()
{
    Spin* spin = getSel( false );
    ENABLED_IF( d_sysList && spin );
    d_sysList->gotoSpin( spin );
}

void FourDScope3::handleGotoResidue()
{
    ENABLED_IF( true );

    Gui2::UiFunction* a = Gui2::UiFunction::me();
    Root::Index id;
    if( a->property( "0" ).isNull() )
	{
		bool ok	= FALSE;
		id	= QInputDialog::getInteger( "Goto Residue", 
			"Please	enter a residue number:", 
            0, -999999, 999999, 1, &ok, this );
		if( !ok )
			return;
	}else
        id = a->property( "0" ).toInt();

    Residue* resi = d_pro->getSequence()->getResidue( id );
	if( resi == 0 || resi->getSystem() == 0 )
	{
        setStatusMessage( "Goto Residue: unknown or unassigned residue" );
		return;
	}

    a->setProperty( "0", Root::Int32(resi->getSystem()->getId() ) );
    handleGotoSystem();
}

void FourDScope3::handleUse4DSpec()
{
    CHECKED_IF( true, d_use4D );
	d_use4D = !d_use4D;
	if( d_use4D )
        d_sysList->setSpec( d_spec4D );
	else
        d_sysList->setSpec( d_spec2D );
}

void FourDScope3::handleGoto4D()
{
    CHECKED_IF( true, d_goto4D );
	d_goto4D = !d_goto4D;
}

void FourDScope3::handleSaveColors()
{
    ENABLED_IF( true );
    Repository::SlotColors clr;
    for( int i = -1; i < d_plane->getOverlayCount(); i++ )
	{
        ContourView3::Params p = d_plane->getContourParams(i);
        if( p.d_pos.isValid() && p.d_neg.isValid() )
            clr.append( Repository::SpecColor( p.d_pos, p.d_neg ) );
	}
    d_pro->getRepository()->setScreenClr( clr );
}

void FourDScope3::handlePlaneToMonoScope()
{
    Spectrum* spec = getSpec2();
    ENABLED_IF( spec );

	try
	{
		Root::Ref<PointSet> ps = createPlanePeaks();
		Root::Ref<PeakList> pl = new PeakList( spec );
		pl->append( ps ); // pl ist nun eine identische Kopie von ps.
		pl->setName( "Export from HomoScope2" );
        createMonoScope( getAgent()->getParent(), spec, d_pro, pl, Rotation() );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Error Exporting Peaklist", e.what() );
	}catch( ... )
	{
		Root::ReportToUser::alert( this, "Error Exporting Peaklist", "Unknown Exception" );
	}
}
