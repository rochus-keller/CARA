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

#include "FourDScope2.h"
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
#include <Spec/GlobalCursor.h>

// SpecView
#include <SpecView/DynValueEditor.h>

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
#include <StripListGadget.h>
#include <SpecView3/CommandLine2.h>
#include "StripListGadget2.h"

using namespace Spec;

GCC_IGNORE(-Wparentheses);

//////////////////////////////////////////////////////////////////////
// Entkopplung; nutzbar ohne include

void createFourDScope2(Root::Agent* a, Spec::Spectrum* s, Spec::Project* p)
{ 
	new FourDScope2( a, s, p );
}

//////////////////////////////////////////////////////////////////////

static const int BACKGROUND = 0;
static const int INTENSITY = 1;
static const int CONTOUR = 2;
static const int FOLDING = 3;
static const int LABEL1 = 4;
static const int LABEL2 = 5;
static const int LABEL3 = 6;
static const int RULER1 = 7;
static const int RULER2 = 8;
static const int CURSORS = 9;
static const int TUPLES = 10;
static const int PEAKS = 11;
static const int VIEWCOUNT = 12;

static const bool s_showContour = true;
static const float s_contourFactor = 1.4f;
static const ContourView::Option s_contourOption = ContourView::Both;
static const ContourView3::Option s_contourOption2 = ContourView3::Both;
static const float s_gain = 2.0;

static QColor g_clrLabel = Qt::yellow;
static QColor g_clrSlice4D = Qt::cyan;
static QColor g_clrPeak = Qt::yellow;

//////////////////////////////////////////////////////////////////////

static void buildCommands( FourDScope2* me, CommandLine2* cl )
{
    cl->addCommand( me, SLOT( handleAdjustIntensity() ), "CW", "Adjust Intensity", true );
    cl->addCommand( me, SLOT( handlePickPlPeak() ), "PP", "Pick Peak", true );
    cl->addCommand( me, SLOT( handleMovePlPeak() ), "MP", "Move Peak", true );
    cl->addCommand( me, SLOT( handleMovePlAlias() ), "MA", "Move Peak Alias", true );
    cl->addCommand( me, SLOT( handleDeletePlPeaks() ), "DP", "Delete Peaks", true );
    cl->addCommand( me, SLOT( handleLabelPlPeak() ), "LP", "Label Peak", true );

    cl->addCommand( me, SLOT( handleDeleteAliasPeak() ), "UA", "Un-Alias Spins", true );
    cl->addCommand( me, SLOT( handleGotoPoint() ), "GT", "Goto", true );
    cl->addCommand( me, SLOT( handlePickSystem() ), "PN", "Pick New System", true );
    cl->addCommand( me, SLOT( handleSetLinkParams() ), "PA", "Set Plane Link Params", true );
    cl->addCommand( me, SLOT( handleSetLinkParams4D() ), "ZPA", "Set Ortho Link Params", true );
    cl->addCommand( me, SLOT( handleSetSystemType() ), "SY", "Set System Type", true );
    cl->addCommand( me, SLOT( handleSyncHori() ), "SH", "Sync. Hori. Spin", true );
    cl->addCommand( me, SLOT( handleSyncVerti() ), "SV", "Sync. Verti. Spin", true );
    cl->addCommand( me, SLOT( handleRemoveRulers() ), "RS", "Remove Selected Rulers", true );
    cl->addCommand( me, SLOT( handleRemoveAllRulers() ), "RA", "Remove All Rulers", true );
    cl->addCommand( me, SLOT( handleAddRulerVerti() ), "RH", "Add Horizontal Ruler", true );
    cl->addCommand( me, SLOT( handleAddRulerHori() ), "RV", "Add Vertical Ruler", true );
    cl->addCommand( me, SLOT( handleHoldReference() ), "HR", "Hold Reference", true );
    cl->addCommand( me, SLOT( handleShowFolded() ), "SF", "Show Folded", true );
    cl->addCommand( me, SLOT( handleAutoContour2() ), "ZAC", "Auto Contour Level", true );
    cl->addCommand( me, SLOT( handleAutoContour() ), "AC", "Auto Contour Level", true );
    cl->addCommand( me, SLOT( handleShow4dPlane() ), "4D", "Show 4D Plane", true );
    cl->addCommand( me, SLOT( handleFitWindow4D() ), "ZWW", "Ortho Fit To Window", true );
    cl->addCommand( me, SLOT( handleBackward() ), "BB", "Backward", true );
    cl->addCommand( me, SLOT( handleShowList() ), "SL", "Show List", true );
    cl->addCommand( me, SLOT( handleCursorSync() ), "GC", "Sync To Global Cursor", true );
    cl->addCommand( me, SLOT( handleFitWindowX() ), "WX", "Fit X To Window", true );
    cl->addCommand( me, SLOT( handleFitWindowY() ), "WY", "Fit Y To Window", true );
    cl->addCommand( me, SLOT( handleFitWindow() ), "WW", "Plane Fit To Window", true );
    cl->addCommand( me, SLOT( handleAutoCenter() ), "CC", "Center To Peak", true );
    cl->addCommand( me, SLOT( handleUndo() ), "ZZ", "Undo", true );
    cl->addCommand( me, SLOT( handleRedo() ), "YY", "Redo", true );
    cl->addCommand( me, SLOT( handleShowContour() ), "SC", "Show Contour", true );
    cl->addCommand( me, SLOT( handleShowIntensity() ), "SI", "Show Intensity", true );
    cl->addCommand( me, SLOT( handleShowLowRes() ), "LR", "Low Resolution", true );
    cl->addCommand( me, SLOT( handleContourParams() ), "CP", "Contour Params", true );
    cl->addCommand( me, SLOT( handleContourParams2() ), "ZCP", "Contour Params", true );

    CommandLine2::Command* cmd = cl->addCommand( me, SLOT( handleSetTolerance() ), "ST", "Set Tolerance", false );
    cmd->registerParameter( Any::CStr, true, "Atom Type" );
    cmd->registerParameter( Any::Float, true, "Tolerance (ppm)" );

    cmd = cl->addCommand( me, SLOT( handleLinkSystems() ), "LK", "Link Systems", false );
    cmd->registerParameter( Any::ULong, false, "Predecessor" ); // 1..n Pred, Succ oder Nil
    cmd->registerParameter( Any::ULong, false, "Successor" ); // 1..n Succ oder Nil

    cmd = cl->addCommand( me, SLOT( handleViewLabels() ), "LL", "Plane Label Format", false );
    cmd->registerParameter( Any::Long, false ); // Leer oder ID

    cmd = cl->addCommand( me, SLOT( handleViewPlLabels() ), "LF", "Label Format", false );
    cmd->registerParameter( Any::Long, false ); // Leer oder ID

    cmd = cl->addCommand( me, SLOT( handleViewLabels4D() ), "ZLL", "Ortho Label Format", false );
    cmd->registerParameter( Any::Long, false ); // Leer oder ID

    cl->addCommand( me, SLOT( handlePickSystem() ), "PY", "Plane Pick System", true );
    cl->addCommand( me, SLOT( handleProposePeak() ), "PR", "Plane Propose Spins", true );
    cl->addCommand( me, SLOT( handleMovePeak() ), "MS", "Plane Move Spins", true );
    cl->addCommand( me, SLOT( handleMovePeakAlias() ), "AY", "Plane Move System Alias", true );
    cl->addCommand( me, SLOT( handlePickHori() ), "EH", "Plane Extend Horizontally", true );
    cl->addCommand( me, SLOT( handleProposeHori() ), "PH", "Propose Horizontally", true );
    cl->addCommand( me, SLOT( handlePickVerti() ), "EV", "Plane Extend Vertically", true );
    cl->addCommand( me, SLOT( handleProposeVerti() ), "PV", "Propose Vertically", true );
    cl->addCommand( me, SLOT( handleLabelPeak() ), "LS", "Label Spins", true );
    cl->addCommand( me, SLOT( handleDeletePeak() ), "DS", "Delete Spins", true );
    cl->addCommand( me, SLOT( handleDeleteLinks() ), "DL", "Delete Spin Links", true );

    cl->addCommand( me, SLOT( handlePickSpin4D() ), "PO", "Ortho Pick Peak", true );
    cl->addCommand( me, SLOT( handleProposeSpin() ), "RI", "Ortho Propose Spins", true );
    cl->addCommand( me, SLOT( handleMoveSpin4D() ), "MO", "Ortho Move Peak", true );
    cl->addCommand( me, SLOT( handleMoveSpinAlias4D() ), "OA", "Ortho Move Peak Alias", true );
    cl->addCommand( me, SLOT( handlePickHori4D() ), "ZEH", "Ortho Extend Horizontally", true );
    cl->addCommand( me, SLOT( handlePickVerti4D() ), "ZEV", "Ortho Extend Vertically", true );

    cl->addCommand( me, SLOT( handleShowWithOff2() ), "RP", "Resolve Projected Spins", true );

    cmd = cl->addCommand( me, SLOT( handleGotoSystem() ), "GY", "Goto System", false );
    cmd->registerParameter( Any::CStr, false ); // Leer oder ID

    cmd = cl->addCommand( me, SLOT( handleGotoPeak() ), "GS", "Goto Spins", false );
    cmd->registerParameter( Any::CStr, false ); // Leer oder ID

    cmd = cl->addCommand( me, SLOT( handleGotoPlPeak() ), "GP", "Goto Peak", false );
    cmd->registerParameter( Any::Long, false ); // Leer oder ID

    cmd = cl->addCommand( me, SLOT( handleOverlayCount() ), "OC", "Overlay Count", false );
    cmd->registerParameter( Any::Long, false ); // Leer oder Count

    cmd = cl->addCommand( me, SLOT( handleActiveOverlay() ), "AO", "Active Overlay", false );
    cmd->registerParameter( Any::Long, false ); // Leer oder ID

    cmd = cl->addCommand( me, SLOT( handleOverlaySpec() ), "YS", "Overlay Spectrum", false );
    cmd->registerParameter( Any::Long, false ); // Leer oder ID

    cl->addCommand( me, SLOT( handleSetPosColor() ), "PC", "Set Positive Color", true );
    cl->addCommand( me, SLOT( handleSetNegColor() ), "NC", "Set Negative Color", true );
    cl->addCommand( me, SLOT( handleComposeLayers() ), "CL", "Compose Layers", true );

    cmd = cl->addCommand( me, SLOT( handleAddLayer() ), "AL", "Add Overlay Layer", false );
    cmd->registerParameter( Any::Long, false ); // Leer oder ID

    cmd = cl->addCommand( me, SLOT( handleCntThreshold() ), "CT", "Contour Threshold", false );
    cmd->registerParameter( Any::Float, true );
    cmd->registerParameter( Any::CStr, false );

    cmd = cl->addCommand( me, SLOT( handleCntFactor() ), "CF", "Contour Factor", false );
    cmd->registerParameter( Any::Float, true );
    cmd->registerParameter( Any::CStr, false );

    cmd = cl->addCommand( me, SLOT( handleCntOption() ), "CO", "Contour Options", false );
    cmd->registerParameter( Any::CStr, false ); // Leer oder + oder -
    cmd->registerParameter( Any::CStr, false );


    cmd = cl->addCommand( me, SLOT( handleGotoResidue() ), "GR", "Goto Residue", false );
    cmd->registerParameter( Any::Long, false ); // Leer oder ID

    cl->addCommand( me, SLOT( handleNextSpec2D() ), "NS", "Next Spectrum", true );
    cl->addCommand( me, SLOT( handlePrevSpec2D() ), "PS", "Prev. Spectrum", true );
    cl->addCommand( me, SLOT( handleNextSpec4D() ), "NT", "Ortho Next Spectrum", true );
    cl->addCommand( me, SLOT( handlePrevSpec4D() ), "PT", "Ortho Prev. Spectrum", true );

    cmd = cl->addCommand( me, SLOT( handleLabelHori() ), "LH", "Label Horizontal Spin", false );
    cmd->registerParameter( Any::CStr, false ); // Label oder leer

    cmd = cl->addCommand( me, SLOT( handleLabelVerti() ), "LV", "Label Vertical Spin", false );
    cmd->registerParameter( Any::CStr, false ); // Label oder leer

    cmd = cl->addCommand( me, SLOT( handlePickLabel4D() ), "PL", "Ortho Pick Labels", true );

    cmd = cl->addCommand( me, SLOT( handleLabelSpin4D() ),"LZ", "Label Ortho Spins", true );

    cmd = cl->addCommand( me, SLOT( handleExecuteLine() ), "LUA", "Lua", false );
    cmd->registerParameter( Any::Memo, true );

    cmd = cl->addCommand( me, SLOT( handleAutoGain() ), "AG", "Set Auto Contour Gain", false );
    cmd->registerParameter( Any::Float, true );
    cmd->registerParameter( Any::CStr, false ); // Label oder leer

    cmd = cl->addCommand( me, SLOT( handleAutoGain4D() ), "ZAG", "Set Ortho Auto Contour Gain", false );
    cmd->registerParameter( Any::Float, true );

    cmd = cl->addCommand( me, SLOT( handleSetWidthFactor() ),"WF", "Set Ortho Width Factor", false );
    cmd->registerParameter( Any::Float, false );

    cmd = cl->addCommand( me, SLOT( handleSetWidth() ), "SW", "Set Peak Width", false );
    cmd->registerParameter( Any::Float, false );

    cl->addCommand( me, SLOT( handleShowPathSim() ), "SP", "Show Path Simulation", true );
}

//////////////////////////////////////////////////////////////////////

struct Locker
{
    Locker( bool& l ):lock(l) { l = true; }
    ~Locker() { lock = false; }
    bool& lock;
};


static QColor g_frameClr = Qt::lightGray;


FourDScope2::FourDScope2(Root::Agent * supervisor, Spectrum* spec, Project* pro ):
    GenericScope( supervisor ), d_showList( false ), d_ovCtrl( 0 ),
    d_use4D( true ), d_goto4D( false ), d_lock( false ), d_autoRuler( false ), d_aol( 0 ),
      d_popSpec2D( 0 ), d_pl(0),
      d_popSpec4D( 0 ), d_popOrtho(0), d_popPlane(0)
{
    Q_ASSERT( spec && ( spec->getDimCount() == 2 || spec->getDimCount() == DimCount ) );
    Q_ASSERT( pro );
    d_pro = pro;
    d_pro->addObserver( this );

    d_orig = spec;

    d_src2D = new SpinPointSpace( pro->getSpins(),
                                 pro->getRepository()->getTypes(), false,
                                 true, true, false );
    d_src4D = new SpinPointSpace( pro->getSpins(), pro->getRepository()->getTypes(), false,
                                 true, true, false );
    d_plane.d_mdl2D = new LinkFilterRotSpace( d_src2D );
    d_ortho.d_rot = new LinkFilterRotSpace( d_src4D );

    d_cl = new CommandLine2( this );
    buildCommands( this, d_cl );
    installEventFilter( d_cl );
    initParams();
    createPopup();
    initViews();

	buildMenus();
    showMaximized();
	buildViews();
    if( spec->getDimCount() == 2 )
    {
        setSpec2D( spec );
        d_pointMdl->setPos( DimX, spec->getScale( DimX ).getRange().first, DimY, spec->getScale( DimY ).getRange().first );
    }else
    {
        d_show4DPlane = true;
        setSpec4D( spec );
        d_pointMdl->setPos( DimZ, spec->getScale( DimZ ).getRange().first, DimW, spec->getScale( DimW ).getRange().first );
    }
    updateSpecPop2D();
    updateSpecPop4D();
    updateCaption();
    d_plane.d_ol[0].d_iso->getBuf()->fitToArea();
    setCursor();
    QMetaObject::invokeMethod( this, "restoreLayout", Qt::QueuedConnection );
}

FourDScope2::~FourDScope2()
{
    d_pro->removeObserver( this );
    d_pro = 0;

    if( d_popSpec2D )
        delete d_popSpec2D;
    if( d_popSpec4D )
        delete d_popSpec4D;
    if( d_popPlane )
        delete d_popPlane;
    if( d_popOrtho )
        delete d_popOrtho;

    GlobalCursor::removeObserver( this );
}

void FourDScope2::restoreLayout()
{
    d_planeGrid->setRowSizes( QList<int>() << ( height() * 0.75 ) << ( height() * 0.25 ) );
    d_orthoGrid->setRowSizes( d_planeGrid->getRowSizes() );
    d_planeGrid->setColSizes( QList<int>() << ( width() * 0.25 ) << ( width() * 0.75 ) );
    d_orthoGrid->setColSizes( d_planeGrid->getColSizes() );
}

void FourDScope2::initViews()
{
    Locker l(d_lock);
    d_slices.resize( DimCount );

    d_pointMdl = new PointMdl();
    d_pointMdl->setNotifyUpdating(d_autoCenter);
    d_pointMdl->addObserver( this );

    d_cubeMdl = new CubeMdl();
    d_cubeMdl->addObserver( this );

    createPlane();
    createSlice( DimX, DimX );
    createSlice( DimY, DimY );
    createSlice( DimY, DimZ );
    createSlice( DimX, DimW );
    createOrtho();

    // d_plane.d_ol[0].d_iso->getBuf()->fitToArea(); wird verzgert gemacht
    updatePlaneLabel();
}

void FourDScope2::createPopup()
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
    d_popPlane->addCommand( "Add Vertical Ruler", this, SLOT( handleAddRulerHori()) );
    d_popPlane->addCommand( "Add Horizontal Ruler", this, SLOT( handleAddRulerVerti() ) );
    d_popPlane->addCommand( "Remove All Rulers", this, SLOT( handleRemoveAllRulers() ) );
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
    menuAtts->addCommand( "Horizontal Spin...", this, SLOT( handleEditAttsSpinH() ) );
    menuAtts->addCommand( "Vertical Spin...", this, SLOT( handleEditAttsSpinV() ) );
    menuAtts->addCommand( "Horizontal System...", this, SLOT( handleEditAttsSysH() ) );
    menuAtts->addCommand( "Vertical System...", this, SLOT( handleEditAttsSysV() ) );
    menuAtts->addCommand( "Spin Link...", this, SLOT( handleEditAttsLink() ) );
    d_popPlane->addMenu( menuAtts );

    d_popPlane->addSeparator();
    d_popPlane->addCommand( "Un-Alias Peaks", this, SLOT( handleDeleteAliasPeak()) );
    d_popPlane->addCommand( "Delete Peaks", this, SLOT( handleDeletePeak()) );
    d_popPlane->addCommand( "Delete Spin Links", this, SLOT( handleDeleteLinks()) );
    d_popPlane->addCommand( "Delete Horizontal Spin", this, SLOT( handleDeleteSpinX()) );
    d_popPlane->addCommand( "Delete Vertical Spin", this, SLOT( handleDeleteSpinY()) );
    d_popPlane->addSeparator();

    // ->addCommand( "Forward", Forward, false );
    d_popPlane->addCommand( "Set Peak Width...", this, SLOT( handleSetWidth() ) );
    d_popPlane->addCommand( "Show Alignment...", this, SLOT( handleShowAlignment() ) );
    d_popPlane->addCommand( "Backward", this, SLOT( handleBackward()) );
    d_popPlane->addCommand( "Show 4D Plane", this, SLOT( handleShow4dPlane()) );
    d_popPlane->addCommand( "Fit Window", this, SLOT( handleFitWindow()));

    d_popOrtho = new Gui2::AutoMenu("Select Spectrum");

    d_popOrtho->addMenu( d_popSpec4D );
    d_popOrtho->addMenu( menuPeaks );
    d_popOrtho->addSeparator();
    d_popOrtho->addCommand( "&Pick Spins", this, SLOT( handlePickSpin4D() ) );
    d_popOrtho->addCommand( "Pick Labels...", this, SLOT( handlePickLabel4D() ) );
    d_popOrtho->addCommand( "Extend Horizontally...", this, SLOT( handlePickHori4D() ) );
    d_popOrtho->addCommand( "Extend Vertically...", this, SLOT( handlePickVerti4D() ) );
    d_popOrtho->addCommand( "&Propose Spins...", this, SLOT( handleProposeSpin() ) );
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
    menuAtts->addCommand( "Horizontal Spin...", this, SLOT( handleEditAttsSpinX4D() ) );
    menuAtts->addCommand( "Vertical Spin...", this, SLOT( handleEditAttsSpinY4D() ) );
    menuAtts->addCommand( "System...", this, SLOT( handleEditAttsSys4D() ) );
    menuAtts->addCommand( "Spin Link...", this, SLOT( handleEditAttsLink4D() ) );
    d_popOrtho->addMenu( menuAtts );
    d_popOrtho->addSeparator();
    d_popOrtho->addCommand( "Set Peak Width...", this, SLOT( handleSetWidth() ) );
    d_popOrtho->addCommand( "Show 4D Plane", this, SLOT( handleShow4dPlane() ) );
    d_popOrtho->addCommand( "Show Path Simulation...", this, SLOT( handleShowPathSim() ) );
    d_popOrtho->addCommand( "Fit Window", this, SLOT( handleFitWindow4D() ) );
}

void FourDScope2::updateSpecPop2D()
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

void FourDScope2::updateSpecPop4D()
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

void FourDScope2::createPlane()
{
    d_plane.d_ol.resize( 1 );

    Root::Ref<ViewAreaMdl3> vam = new ViewAreaMdl3( d_cubeMdl, DimX, DimY );
    d_plane.d_viewer = new SpecViewer3( this, VIEWCOUNT );
    d_plane.d_viewer->installEventFilter( d_cl );
    d_plane.d_viewer->setArea( vam );
    vam->addObserver( this );

    Ref<Spectrum> spec;
    if( d_spec2D )
        spec = new SpecProjector( d_spec2D, d_pointMdl, DimX, DimY );
    Ref<SpecBufferMdl3> buf = new SpecBufferMdl3( d_plane.d_viewer->getArea(), spec, false );
    buf->setFolding( d_folding, false );

    d_plane.d_intens = new IntensityView3( vam, buf, false );
    d_plane.d_intens->setVisible(false);
    d_plane.d_viewer->setView( INTENSITY, d_plane.d_intens );

    d_plane.d_ol[0].d_iso = new ContourView3( vam, buf, true );
    Root::Ref<ViewStack> l = new ViewStack();
    l->addView( d_plane.d_ol[0].d_iso );
    d_plane.d_viewer->setView( CONTOUR, l );
    d_plane.d_ol[0].d_iso->setVisible( s_showContour );
    ContourView3::Params p;
    p.d_factor = s_contourFactor;
    p.d_option = s_contourOption2;
    p.d_gain = s_gain;
    p.d_auto = true;
    const Repository::SlotColors& sc = d_pro->getRepository()->getScreenClr();
    if( !sc.empty() )
    {
        p.d_pos = sc[ 0 ].d_pos;
        p.d_neg = sc[ 0 ].d_neg;
    }
    d_plane.d_ol[0].d_iso->setParams( p );

    d_plane.d_mdlND = new RangeFilterSpaceND( d_ortho.d_rot, d_pointMdl, DimZ, DimW );
    d_plane.d_points = new SpinPointView3( vam, d_plane.d_mdl2D );
    d_plane.d_points->setLabelType( SpinPointView3::PairLabelSysOrResi );
    d_plane.d_points->adjustPositions( d_pointMdl, DimX, DimY );
    d_plane.d_viewer->setView( TUPLES, d_plane.d_points );
    CursorView3* cv = new CursorView3( vam, d_pointMdl, DimX, DimY );
    d_plane.d_viewer->setView( CURSORS, cv );
    if( d_folding )
        d_plane.d_viewer->setView( FOLDING, new FoldingView3( d_plane.d_ol[0].d_iso->getBuf() ) );
    d_plane.d_hRuler = new SpinPoint1DView3( vam, DimY, 0, Qt::darkYellow );
    d_plane.d_hRulerMdl = new ManualSpinSpace( 1 );
    d_plane.d_hRuler->setModel( d_plane.d_hRulerMdl );
    d_plane.d_hRuler->setLabelType( SpinPoint1DView3::SysTagAll );
    d_plane.d_viewer->setView( RULER1, d_plane.d_hRuler );
    d_plane.d_vRuler = new SpinPoint1DView3( vam, DimX, 0, Qt::darkYellow );
    d_plane.d_vRulerMdl = new ManualSpinSpace( 1 );
    d_plane.d_vRuler->setModel( d_plane.d_vRulerMdl );
    d_plane.d_vRuler->setLabelType( SpinPoint1DView3::SysTagAll );
    d_plane.d_viewer->setView( RULER2, d_plane.d_vRuler );

    d_plane.d_peakMdl = new PeakSubSpaceND( new PeakSpaceDummy( 2 ), DimX, DimY, d_pointMdl, DimVector( DimZ, DimW ) );
    d_plane.d_peaks = new PeakPlaneView3( vam, d_plane.d_peakMdl );
    d_plane.d_peaks->setColor( g_clrPeak );
    d_plane.d_peaks->adjustPositions( d_pointMdl, DimX, DimY );
    d_plane.d_viewer->setView( PEAKS, d_plane.d_peaks );

    d_plane.d_viewer->addView( new ZoomCtrl3( vam, true, true ) );
    d_plane.d_viewer->addView( new SelectZoomCtrl3( vam, true, true ) );
    d_plane.d_viewer->addView( new ScrollCtrl3( vam ) );
    d_plane.d_viewer->addView( new CursorCtrl3( cv, false ) );
    d_plane.d_viewer->addView( new PointSelect1DCtrl3( vam, d_plane.d_hRuler, false ) );
    d_plane.d_viewer->addView( new PointSelect1DCtrl3( vam, d_plane.d_vRuler, false ) );
    // NOTE: Select muss vor Cursor kommen, da sonst Selection zu spt passiert.
    Root::Ref<PointSelectCtrl3> pointSelector = new PointSelectCtrl3( vam, d_plane.d_points, false );
    d_plane.d_viewer->addView( pointSelector );
    Root::Ref<PeakSelectCtrl3> peakSelector = new PeakSelectCtrl3( vam, d_plane.d_peaks, false );
    peakSelector->propagateTo( pointSelector );
    d_plane.d_viewer->addView( peakSelector );
    d_popPlane->connectPopup( d_plane.d_viewer );
}

void FourDScope2::initOverlay(int n)
{
    Q_ASSERT( n > 0 );
    if( d_plane.d_ol.size() == n )
        return;
    ViewStack* vStack = static_cast<ViewStack*>(d_plane.d_viewer->getView( CONTOUR ));
    vStack->clear();

    int i;
    if( n > d_plane.d_ol.size() )
    {
        int old = d_plane.d_ol.size();
        d_plane.d_ol.resize( n );
        const Repository::SlotColors& sc = d_pro->getRepository()->getScreenClr();
        for( i = old; i < n; i++ )
        {
            Ref<SpecBufferMdl3> buf = new SpecBufferMdl3( d_plane.d_viewer->getArea() );
            buf->setFolding( d_folding );
            d_plane.d_ol[i].d_iso = new ContourView3( d_plane.d_viewer->getArea(), buf, true );
            if( i < sc.size() )
            {
                d_plane.d_ol[i].d_iso->setPosColor( sc[ i ].d_pos );
                d_plane.d_ol[i].d_iso->setNegColor( sc[ i ].d_neg );
            }
            ContourView3::Params p;
            p.d_factor = s_contourFactor;
            p.d_option = s_contourOption2;
            p.d_gain = s_gain;
            p.d_auto = true;
            d_plane.d_ol[i].d_iso->setVisible( d_plane.d_ol[0].d_iso->isVisible() );
            d_plane.d_ol[i].d_iso->setParams(p);
        }
    }else
    {
        d_plane.d_ol.resize( n );
    }
    for( i = 0; i < d_plane.d_ol.size(); i++ )
        vStack->addView( d_plane.d_ol[i].d_iso );
    setActiveOverlay( 0 );
    updatePlaneLabel();
}

void FourDScope2::setActiveOverlay(int n)
{
    if( n == d_aol )
        return;
    d_plane.d_viewer->update();

    d_aol = n;
    updatePlaneLabel();
}

int FourDScope2::selectLayer()
{
    if( d_plane.d_ol.size() == 1 )
        return 0;

    Dlg::StringList l( d_plane.d_ol.size() + 1);
    l[ 0 ] = "&All";
    QString str;
    for( int i = 0; i < d_plane.d_ol.size(); i++ )
    {
        if( d_plane.d_ol[ i ].d_iso->getBuf()->getSpectrum() )
            str.sprintf( "&%d %s", i, d_plane.d_ol[ i ].d_iso->getBuf()->getSpectrum()->getName() );
        else
            str.sprintf( "&%d <empty>", i );
        l[ i + 1 ] = str.toLatin1();
    }
    int c = Dlg::getOption( this, l,
        "Select Layer", d_aol + 1 );
    if( c == -1 )
        return -2;
    else
        return c - 1;
}

void FourDScope2::createOrtho()
{
    Root::Ref<ViewAreaMdl3> vam = new ViewAreaMdl3( d_cubeMdl, DimW, DimZ );
    d_ortho.d_viewer = new SpecViewer3( this, VIEWCOUNT );
    d_ortho.d_viewer->installEventFilter( d_cl );
    d_ortho.d_viewer->setArea( vam );
    vam->addObserver( this );

    Ref<SpecBufferMdl3> buf = new SpecBufferMdl3( vam, 0, false );
    buf->setFolding( d_folding, false );

    d_ortho.d_iso = new ContourView3( vam, buf, true );	// Immer auto
    d_ortho.d_viewer->setView( CONTOUR, d_ortho.d_iso );
    ContourView3::Params p;
    p.d_factor = s_contourFactor;
    p.d_option = s_contourOption2;
    p.d_gain = s_gain;
    p.d_auto = true;
    d_ortho.d_iso->setParams( p );

    d_ortho.d_range = new RangeFilterSpaceND( d_ortho.d_rot, d_pointMdl, DimX, DimY );
    d_ortho.d_points = new SpinPointView3( vam, new RotatedSpace( d_ortho.d_range,
                                                                  Rotation( DimW, DimZ, DimX, DimY )) );
    d_ortho.d_points->adjustPositions( d_pointMdl, DimW, DimZ );
    d_ortho.d_points->setLabelType( SpinPointView3::PairLabelSysOrResi );
    d_ortho.d_viewer->setView( TUPLES, d_ortho.d_points );
    CursorView3* cv = new CursorView3( vam, d_pointMdl, DimW, DimZ );
    d_ortho.d_viewer->setView( CURSORS, cv );
    if( d_folding )
        d_ortho.d_viewer->setView( FOLDING, new FoldingView3( d_ortho.d_iso->getBuf() ) );

    d_ortho.d_peakMdl = new PeakSubSpaceND( new PeakSpaceDummy( 2 ), DimW, DimZ, d_pointMdl, DimVector( DimX, DimY ) );
    d_ortho.d_peaks = new PeakPlaneView3( vam, d_ortho.d_peakMdl );
    d_ortho.d_peaks->adjustPositions( d_pointMdl, DimW, DimZ );
    d_ortho.d_peaks->setColor( g_clrPeak );
    d_ortho.d_viewer->setView( PEAKS, d_ortho.d_peaks );

    d_ortho.d_viewer->addView( new ZoomCtrl3( vam, true, true ) );
    d_ortho.d_viewer->addView( new SelectZoomCtrl3( vam, true, true ) );
    d_ortho.d_viewer->addView( new ScrollCtrl3( vam, true, true ) );
    d_ortho.d_viewer->addView( new CursorCtrl3( cv, false, true, true ) );
    Root::Ref<PointSelectCtrl3> pointSelector = new PointSelectCtrl3( vam, d_ortho.d_points, false );
    d_ortho.d_viewer->addView( pointSelector );
    Root::Ref<PeakSelectCtrl3> peakSelector = new PeakSelectCtrl3( vam, d_ortho.d_peaks, false );
    peakSelector->propagateTo( pointSelector );
    d_ortho.d_viewer->addView( peakSelector );
    d_popOrtho->connectPopup( d_ortho.d_viewer );
}

void FourDScope2::createSlice(Dimension view, Dimension spec)
{
    Root::Ref<ViewAreaMdl3> vam = new ViewAreaMdl3( view, d_cubeMdl, spec );
    SpecViewer3* slice = new SpecViewer3( this );
    slice->installEventFilter( d_cl );
    slice->setArea( vam );
    d_slices[ spec ].d_viewer = slice;
    vam->addObserver( this );

    d_slices[ spec ].d_buf2D = new SpecBufferMdl3( vam, 0, false );
    d_slices[ spec ].d_buf2D->setFolding( d_folding, false );
	slice->addView( new SliceView3( d_slices[ spec ].d_buf2D ) );

    d_slices[ spec ].d_bufND = new SpecBufferMdl3( vam, 0, false );
    d_slices[ spec ].d_bufND->setFolding( d_folding, false );
	Root::Ref<SliceView3> sv = new SliceView3( d_slices[ spec ].d_bufND );
	sv->setColor(g_clrSlice4D);
	slice->addView( sv );

    CursorView3* cv = new CursorView3( vam, d_pointMdl, (view==DimX)?spec:Dimension(DimNone),
                                       (view==DimY)?spec:Dimension(DimNone) );
    slice->addView( cv );

    slice->addView( new ZoomCtrl3( vam, view == DimX, view == DimY ) );
    slice->addView( new SelectZoomCtrl3( vam, view == DimX, view == DimY ) );
    slice->addView( new ScrollCtrl3( vam ) );
    slice->addView( new SelectRulerCtrl3( vam, true ) );
    slice->addView( new CursorCtrl3( cv, false ) );
}

void FourDScope2::updateContour( int i, bool redraw )
{
    if( !d_plane.d_ol[0].d_iso->isVisible() )
        return;

    if( d_show4DPlane && i == 0 )
    {
        ContourView3::Params p = d_ortho.d_iso->getParams();
        if( d_spec4D )
            p.d_threshold = d_spec4D->getThreshold();
        d_plane.d_ol[0].d_iso->setParams( p );
    }else
    {
        d_plane.d_ol[i].d_iso->recalcLevels();
    }
    if( redraw )
        d_plane.d_viewer->update();
}

void FourDScope2::showIntens(bool on )
{
    if( d_plane.d_intens->isVisible() == on )
        return;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    d_plane.d_intens->setVisible( on );
    QApplication::restoreOverrideCursor();
}

void FourDScope2::initParams()
{
    d_resol = 1;
    d_lowResol = false;
    d_autoCenter = true;
    d_folding = false;
    d_show4DPlane = false;
    d_autoHide = true;
    d_cursorSync = false;
    d_rangeSync = false;
    d_autoHold = false;
    d_syncDepth = true;
    // TODO: diese Werte sollen ab Konfigurations-Record gelesen werden
}

void FourDScope2::centerToCursor(bool threeD)
{
    if( !threeD )
    {
        ViewAreaMdl3* area = d_plane.d_viewer->getArea();
        if( !area->getRange( DimX ).contains( d_pointMdl->getPos(DimX) ) ||
            !area->getRange( DimY ).contains( d_pointMdl->getPos(DimY) ) )
        {
            area->centerPoint( d_pointMdl->getPos(DimX), d_pointMdl->getPos(DimY) );
            d_plane.d_viewer->update();
        }
    }else
    {
        ViewAreaMdl3* area = d_ortho.d_viewer->getArea();
        if( !area->getRange( DimY ).contains( d_pointMdl->getPos(DimZ) ) ||
            !area->getRange( DimX ).contains( d_pointMdl->getPos(DimW) ) )
        {
            area->centerPoint( d_pointMdl->getPos(DimW), d_pointMdl->getPos(DimZ) );
            d_ortho.d_viewer->update();
        }
    }
}

bool FourDScope2::askToClosePeaklist()
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

bool FourDScope2::savePeakList()
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

void FourDScope2::setCursor( PpmPoint p)
{
    if( p.size() == 0 )
    {
        p.assign( getSpec()->getDimCount(), 0 );
        for( int i = 0; i < p.size(); i++ )
            p[ i ] = getSpec()->getScale( i ).getIdxN();
    }
    for( Dimension d = 0; d < p.size(); d++ )
        d_pointMdl->setPos( d, p[ d ] );

    centerToCursor( false ); // RISK
    show4dPlaneSlice( DimX, true );
    show4dPlaneSlice( DimY, true );
    selectCurSystem();
    // TODO: Selection
    notifyCursor( true );
}

void FourDScope2::planeAreaUpdated(ViewAreaMdl3::UpdRange *msg)
{
    // In Plane wurde Ausschnitt ge�dert
    PpmCube cube;
    cube.assign( 2, PpmRange() );
    cube[ DimX ] = d_plane.d_viewer->getArea()->getRange( DimX );
    cube[ DimY ] = d_plane.d_viewer->getArea()->getRange( DimY );
    d_backward.push_back( std::make_pair( cube, d_pointMdl->getPpmPoint(DimCount) ) );
}

void FourDScope2::updateGlobalCursor(PointMdl::Updated *msg)
{
    if( !d_cursorSync )
        return;

    const bool hasX = msg->hasDim(DimX);
    const bool hasY = msg->hasDim(DimY);
    const bool hasW = msg->hasDim(DimW);
    const bool hasZ = msg->hasDim(DimZ);
    if( hasX && hasY )
        GlobalCursor::setCursor( d_pointMdl->getPos(DimX), d_pointMdl->getPos(DimY),
                                 d_plane.d_ol[0].d_iso->getBuf()->getSpectrum()->getColor( DimX ), d_plane.d_ol[0].d_iso->getBuf()->getSpectrum()->getColor( DimY ) );
    else if( hasX )
        GlobalCursor::setCursor( DimX, d_pointMdl->getPos(DimX), d_slices[ DimX ].d_buf2D->getSpectrum()->getColor( DimX ) );
    else if( hasY )
        GlobalCursor::setCursor( DimY, d_pointMdl->getPos(DimY), d_slices[ DimY ].d_buf2D->getSpectrum()->getColor( DimX ) );
    if( d_syncDepth && !d_spec4D.isNull() )
    {
        if( hasW && hasZ )
            GlobalCursor::setCursor( d_pointMdl->getPos(DimW), d_pointMdl->getPos(DimZ),
                                     d_spec4D->getColor( DimW ), d_spec4D->getColor( DimZ )  );
        else if( hasW )
            GlobalCursor::setCursor( DimX, d_pointMdl->getPos(DimW), d_spec4D->getColor(DimW) );
        else if( hasZ )
            GlobalCursor::setCursor( DimY, d_pointMdl->getPos(DimZ), d_spec4D->getColor(DimZ) );
    }
}

void FourDScope2::updateGlobalCursor(CubeMdl::Updated *msg)
{
    if( !d_rangeSync )
        return;
    if( msg->is2D() )
    {
        // Entweder X/Y oder W/Z
        if( msg->hasDim(DimX) )
        {
            GlobalCursor::setRange( d_cubeMdl->getRange(DimX), d_cubeMdl->getRange(DimY),
                                     d_plane.d_ol[0].d_iso->getBuf()->getSpectrum()->getColor( DimX ), d_plane.d_ol[0].d_iso->getBuf()->getSpectrum()->getColor( DimY ) );
        }else if( d_syncDepth && !d_spec4D.isNull() )
        {
            GlobalCursor::setRange( d_cubeMdl->getRange(DimW), d_cubeMdl->getRange(DimZ),
                                     d_spec4D->getColor( DimW ), d_spec4D->getColor( DimZ )  );
        }
    }else
    {
        if( msg->getDim() == DimX || msg->getDim() == DimY )
            GlobalCursor::setRange( msg->getDim(), d_cubeMdl->getRange(msg->getDim()),
                                     d_slices[ msg->getDim() ].d_buf2D->getSpectrum()->getColor( DimX ) );
        else if( d_syncDepth && !d_spec4D.isNull() )
        {
            if( msg->getDim() == DimZ )
                GlobalCursor::setRange( DimY, d_cubeMdl->getRange(msg->getDim()), d_spec4D->getColor( msg->getDim() ) );
            else if( msg->getDim() == DimW )
                GlobalCursor::setRange( DimX, d_cubeMdl->getRange(msg->getDim()), d_spec4D->getColor( msg->getDim() ) );
        }
    }
}

void FourDScope2::updatePlaneLabel()
{
    Spectrum* spec = d_plane.d_ol[d_aol].d_iso->getBuf()->getSpectrum();
    if( !d_spec4D.isNull() && d_show4DPlane )
        spec = d_spec4D;
    QString str;
    if( spec == 0 )
        str.sprintf( " %d <empty>", d_aol );
    else if( d_plane.d_ol.size() > 1 )
        str.sprintf( " %d %s  %s / %s", d_aol, spec->getName(), spec->getDimName( DimX ),
                     spec->getDimName( DimY ) );
    else
        str.sprintf( " %s  %s / %s", spec->getName(), spec->getDimName( DimX ),
            spec->getDimName( DimY ) );
    d_plane.d_viewer->setView( LABEL1,
        new LabelView( d_plane.d_viewer->getArea(), str, 0,
        (d_show4DPlane)?g_clrSlice4D:d_plane.d_points->getColor(),
        Qt::AlignLeft | Qt::AlignTop ) );
}

void FourDScope2::setSpec4D(Spectrum * spec)
{
    Q_ASSERT( spec == 0 || spec->getDimCount() == DimCount );

    if( d_spec4D == spec )
        return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    Spectrum* old = d_spec4D;
    d_spec4D = spec;
    if( !d_spec4D.isNull() )
    {
        d_ortho.d_iso->getBuf()->setSpectrum( new SpecProjector( d_spec4D, d_pointMdl, DimW, DimZ ) );

        d_slices[ DimZ ].d_bufND->setSpectrum( new SpecProjector( d_spec4D, d_pointMdl, DimZ ), true );
        Q_ASSERT( d_slices[ DimZ ].d_buf2D->getSpectrum() == 0 );

        d_slices[ DimW ].d_bufND->setSpectrum( new SpecProjector( d_spec4D, d_pointMdl, DimW ), true );
        Q_ASSERT( d_slices[ DimW ].d_buf2D->getSpectrum() == 0 );

        if( old == 0 || old->getColor( DimZ ) != spec->getColor( DimZ ) ||
            !old->getScale( DimZ ).getRange().intersects( spec->getScale( DimZ ).getRange() ) ||
            old->getColor( DimW ) != spec->getColor( DimW ) ||
            !old->getScale( DimW ).getRange().intersects( spec->getScale( DimW ).getRange()) )
        {
            // Nur FitWindow, wenn andere Farbe oder Bereich nicht berlappend
            PpmRange r = d_spec4D->getScale( DimZ ).getRange();
            r.invert();
            d_slices[ DimZ ].d_viewer->getArea()->setRange( DimY, r );
            d_slices[ DimW ].d_viewer->getArea()->setRange( DimX, d_spec4D->getScale( DimW ).getRange() );
            d_ortho.d_viewer->getArea()->setRange( d_spec4D->getScale( DimW ).getRange(), r );
        }

        d_ortho.d_rot->setSpec( 0 );
        d_src4D->setSpec( d_spec4D );
        d_ortho.d_rot->setSpec( d_spec4D );
        d_plane.d_mdlND->setGhostWidth( d_pro->inferPeakWidth( DimZ, d_spec4D ),
                                    d_pro->inferPeakWidth( DimW, d_spec4D ) );
        d_plane.d_mdlND->setThickness( d_spec4D->getScale( DimZ ).getDelta(),
                                   d_spec4D->getScale( DimW ).getDelta() );
        PeakPos thick;
        thick[DimZ] = d_spec4D->getScale( DimZ ).getDelta();
        thick[DimW] = d_spec4D->getScale( DimW ).getDelta();
        thick[DimX] = d_spec4D->getScale( DimX ).getDelta();
        thick[DimY] = d_spec4D->getScale( DimY ).getDelta();
        d_plane.d_peakMdl->setThickness( thick );
        d_ortho.d_range->setThickness( d_spec4D->getScale( DimX ).getDelta(),
                                 d_spec4D->getScale( DimY ).getDelta() );
        d_ortho.d_peakMdl->setThickness( thick );
        const PPM wx = d_pro->inferPeakWidth( DimX, d_spec4D );
        const PPM wy = d_pro->inferPeakWidth( DimY, d_spec4D );
        d_ortho.d_range->setGhostWidth( wx, wy );
        d_ortho.d_peakMdl->setGhostWidth( wx, wy );

        d_ortho.d_viewer->setView( LABEL1,
                                   new LabelView( d_ortho.d_viewer->getArea(),
                                                  d_spec4D->getName(), nil, g_clrSlice4D, Qt::AlignLeft | Qt::AlignTop ) );
    }else // if d_spec4D.isNull()
    {
        d_ortho.d_iso->getBuf()->setSpectrum( 0 );
        d_slices[ DimZ ].d_bufND->setSpectrum( 0 );
        d_slices[ DimW ].d_bufND->setSpectrum( 0 );
        d_ortho.d_viewer->setView( LABEL1, 0 );
        d_ortho.d_rot->setSpec( 0 );
        d_src4D->setSpecType( 0 );
    }
    setShow4dPlane( d_show4DPlane );
    selectCurSystem( true );
    QApplication::restoreOverrideCursor();
    specChanged( true );
}

void FourDScope2::stepSpec2D(bool next)
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

void FourDScope2::stepSpec4D(bool next)
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

void FourDScope2::setSpec2D(Spectrum * spec )
{
    Q_ASSERT( spec == 0 || spec->getDimCount() == 2 );

    if( d_spec2D == spec )
        return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    d_spec2D = spec;

    d_plane.d_mdl2D->setSpec( 0 );
    d_src2D->setSpec( d_spec2D );
    d_plane.d_mdl2D->setSpec( d_spec2D );

    setShow4dPlane( false );

    if( d_spec2D )
        d_slices[ DimX ].d_buf2D->setSpectrum( new SpecProjector( d_spec2D, d_pointMdl, DimX ) );
    else
        d_slices[ DimX ].d_buf2D->setSpectrum( 0 );
    d_slices[ DimX ].d_buf2D->setFolding( d_folding );

    if( d_spec2D )
        d_slices[ DimY ].d_buf2D->setSpectrum( new SpecProjector( d_spec2D, d_pointMdl, DimY ) );
    else
        d_slices[ DimY ].d_buf2D->setSpectrum( 0 );
    d_slices[ DimY ].d_buf2D->setFolding( d_folding );

    if( d_spec2D )
    {
        d_ovCtrl->getBuf()->setSpectrum( d_spec2D );
        if( d_spec2D->canDownsize() )
            d_ovCtrl->getBuf()->fitToArea();
        else
            d_ovCtrl->getBuf()->copy( d_plane.d_ol[0].d_iso->getBuf() );
    }else
        d_ovCtrl->getBuf()->setSpectrum( 0 );

    QApplication::restoreOverrideCursor();
}

void FourDScope2::setPeakList(PeakList * pl)
{
    if( pl )
    {
        d_pl = new PeakListPeer( pl );
        d_plane.d_peakMdl->setPeakSpace( d_pl );
        d_ortho.d_peakMdl->setPeakSpace( d_pl );
        QString str;
        str.sprintf( "%dD %s", d_pl->getDimCount(), pl->getName().data() );
        d_plane.d_viewer->setView( LABEL3,
            new LabelView( d_plane.d_viewer->getArea(), str, 0,
            d_plane.d_peaks->getColor(), Qt::AlignLeft | Qt::AlignBottom ) );
    }else
    {
        d_pl = 0;
        d_plane.d_peakMdl->setPeakSpace( new PeakSpaceDummy( 2 ) );
        d_ortho.d_peakMdl->setPeakSpace( new PeakSpaceDummy( 2 ) );
        d_plane.d_viewer->setView( LABEL3, 0 );
    }
}

void FourDScope2::extendSystem(Dimension source, Dimension target )
{
    // Ausser spec2D identisch mit HomoScope
    Spin* ref = 0;
    if( d_ref[ source ] != 0 )
        ref = d_ref[ source ];
    else if( d_plane.d_points->getSel().size() == 1 )
        ref = ( *d_plane.d_points->getSel().begin() )[ source ];
    else
    {
        // Der User kann Extend auch ausfhren, wenn kein Peak selektiert wurde.
        // In diesem Fall schlagen wir Peaks in der Region der Cursordimension vor.
        ProposeSpinDlg dlg( this, d_pro, getSpec()->getColor( source ), d_pointMdl->getPos(source),
                            getSpec(),	"Select Reference Spin" );
        dlg.setAnchor( source, ref );
        if( !dlg.exec() || dlg.getSpin() == 0 )
            return;
        ref = dlg.getSpin();
    }
    pickSpin( target, ref, ref->getSystem() );
}

void FourDScope2::pickSpin(Dimension d, Spin *other, SpinSystem *owner)
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
        d_plane.d_points->selectPoint( d_pointMdl->getPos(DimX), d_pointMdl->getPos(DimY) );
    }
}

void FourDScope2::gotoTuple(SpinSystem * sys, Spin * spin, Spin * link, bool twoD )
{
    SpinPoint tuple;
    Dimension dim;
    SpinSpace::Result tuples;
    if( !twoD && d_spec4D )
    {
        if( link )
            d_ortho.d_rot->find( tuples, spin, link );
        else if( spin )
            d_ortho.d_rot->find( tuples, spin );
        else
            d_ortho.d_rot->find( tuples, sys );
        dim = DimCount;
    }else
    {
        if( link )
            d_plane.d_mdl2D->find( tuples, spin, link );
        else if( spin )
            d_plane.d_mdl2D->find( tuples, spin );
        else
            d_plane.d_mdl2D->find( tuples, sys );
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
    d_plane.d_points->select( tuple );
    if( dim == DimCount )
    {
        show4dPlaneSlice( DimX, true );
        show4dPlaneSlice( DimY, true );
    }
    selectCurSystem();
    centerToCursor();
    if( dim == DimCount )
    {
        ViewAreaMdl3* area = d_ortho.d_viewer->getArea();
        if( !area->getRange( DimY ).contains( d_pointMdl->getPos(DimZ) ) ||
            !area->getRange( DimX ).contains( d_pointMdl->getPos(DimW) ) )
        {
            area->centerPoint( d_pointMdl->getPos(DimW), d_pointMdl->getPos(DimZ) );
            d_ortho.d_viewer->update();
        }
    }
    notifyCursor( dim == 2 );
}

Spin* FourDScope2::getSel(bool hori) const
{
    // Kopie von HomoScope
    if( d_plane.d_points->getSel().size() != 1 )
        return 0;

    SpinPoint tuple = *d_plane.d_points->getSel().begin();
    if( hori )
        return tuple[ DimX ];
    else
        return tuple[ DimY ];
}

void FourDScope2::notifyCursor(bool plane)
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
        if( d_folding )
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
            val = getSpec()->getAt( p, d_folding, d_folding );
        }else
            val = d_spec4D->getAt( d_pointMdl->getPpmPoint(DimCount), d_folding, d_folding );
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
        d_plane.d_points->formatSelection( tmp, SpinPointView3::PairAll, 3 );
        str += QLatin1String(",  ") + QString::fromLatin1( tmp );
    }else if( d_ortho.d_viewer->hasFocus() )
    {
        d_ortho.d_points->formatSelection( tmp, SpinPointView3::PairAll, 3 );
        str += QLatin1String(",  ") + QString::fromLatin1( tmp );
    }
    setStatusMessage( str );
}

#ifdef __unused
static void _allocate( PpmRange& r, PPM point, PPM width ) // TODO: Bug Release-Mode
{
    if( r.first <= r.second )
    {
        r.first = point - width / 2.0;
        r.second = point + width / 2.0;
    }else
    {
        r.first = point + width / 2.0;
        r.second = point - width / 2.0;
    }
}
#endif

void FourDScope2::show4dPlaneSlice( Dimension dim, bool show )
{
    Q_ASSERT( dim == DimX || dim == DimY );

    if( ( !d_autoHide || show || d_show4DPlane ) )
    {
        if( !d_spec4D.isNull() && d_slices[ dim ].d_bufND->getSpectrum() == 0 )
            d_slices[ dim ].d_bufND->setSpectrum( new SpecProjector( d_spec4D, d_pointMdl, dim ) );
    }else
        d_slices[ dim ].d_bufND->setSpectrum( 0 );
}

void FourDScope2::updateContour4D( bool ac )
{
    d_ortho.d_iso->recalcLevels();
    d_ortho.d_viewer->update();
}

void FourDScope2::updateRef()
{
    if( !d_ref.isZero() )
    {
        QByteArray buf = d_plane.d_points->formatLabel( d_ref );
        QString str;
        str.sprintf( "Reference: %s", buf.data() );
        d_plane.d_viewer->setView( LABEL2,
            new LabelView( d_plane.d_viewer->getArea(), str, 0, g_clrLabel,
                Qt::AlignRight | Qt::AlignTop ) );
    }else
        d_plane.d_viewer->setView( LABEL2, 0 );
    d_plane.d_viewer->update();
}

void FourDScope2::selectCurSystem( bool force )
{
    if( d_spec4D && ( d_plane.d_points->getSel().size() == 1 || !d_ref.isZero() ) )
    {
        SpinPoint tuple;
        if( !d_ref.isZero() )
            tuple = d_ref;
        else
            tuple = *d_plane.d_points->getSel().begin();
        if( !force && d_cur == tuple )
            return; // Bereits korrekt
        d_cur = tuple;

        QString str;
        char buf[32];
        SpinPointView::formatLabel( buf, sizeof(buf), d_cur,
                                    SpinPointView::PairIdLabelSysOrResi, DimUndefined );
        str = buf;
        d_ortho.d_viewer->setView( LABEL1,
            new LabelView( d_ortho.d_viewer->getArea(), str, nil, g_clrSlice4D,
            Qt::AlignLeft | Qt::AlignTop ) );

        str.sprintf( " %s/%s", d_spec4D->getDimName( DimW ),
                     d_spec4D->getDimName( DimZ ) );

        d_ortho.d_viewer->setView( LABEL2,
            new LabelView( d_ortho.d_viewer->getArea(), str, nil, g_clrSlice4D,
            Qt::AlignLeft | Qt::AlignBottom ) );

        d_ortho.d_range->setSys( d_cur[ DimX ]->getSystem() );
    }else
    {
        d_cur.zero();
        d_ortho.d_viewer->setView( LABEL2, 0 );
        d_ortho.d_viewer->setView( LABEL1, 0 );
        d_ortho.d_range->setSys( 0 );
    }
}

void FourDScope2::handleSetResolution()
{
    ENABLED_IF( true );

    bool ok	= FALSE;
    int res	= QInputDialog::getInteger( "Set Resolution",
        "Please	enter the minimal number of pixels per sample:",
        d_resol, 1, 20, 1,	&ok, this );
    if( ok )
    {
        d_resol = res;
        d_lowResol = true;
        Viewport::pushHourglass();
        d_plane.d_ol[0].d_iso->getBuf()->setResolution( d_resol );
        d_plane.d_viewer->update();
        Viewport::popCursor();
    }
}

void FourDScope2::handleSave()
{
    Root::MenuAction a( getAgent(), Root::Action::FileSave );
    ENABLED_IF( a.checkEnabled() );
    a.execute();
}

void FourDScope2::handleClose()
{
    Root::MenuAction a( getAgent(), Root::Action::WindowClose );
    ENABLED_IF( a.checkEnabled() );
    a.execute();
}

void FourDScope2::handleUndo()
{
    Root::MenuAction a( getAgent(), Root::Action::EditUndo );
    ENABLED_IF( a.checkEnabled() );
    a.execute();
}

void FourDScope2::handleRedo()
{
    Root::MenuAction a( getAgent(), Root::Action::EditRedo );
    ENABLED_IF( a.checkEnabled() );
    a.execute();
}

void FourDScope2::handleHelpAbout()
{
    Root::MenuAction a( getAgent(), Root::Action::HelpAbout );
    ENABLED_IF( a.checkEnabled() );
    a.execute();
}

void FourDScope2::handleExecuteLine()
{
    Gui2::UiFunction* f = Gui2::UiFunction::me();
    Root::MenuParamAction a( getAgent(), Root::Action::ExecuteLine );
    a.addParam( f->property("0").toByteArray().data() );
    ENABLED_IF( a.checkEnabled() );
    a.execute();
}

void FourDScope2::onListActivate()
{
    gotoTuple( d_list->getSelectedStrip(), d_list->getSelectedSpin(),
        d_list->getSelectedLink(), !d_goto4D );
    d_plane.d_viewer->setFocus();
}

void FourDScope2::handleShowLowRes()
{
    CHECKED_IF( true, d_lowResol );

    Viewport::pushHourglass();
    d_lowResol = !d_lowResol;
    if( d_lowResol )
        d_plane.d_ol[0].d_iso->getBuf()->setResolution( d_resol );
    else
        d_plane.d_ol[0].d_iso->getBuf()->setScaling( false );
    d_plane.d_viewer->update();
    Viewport::popCursor();
}

void FourDScope2::handleForward()
{
    ENABLED_IF( d_forward.size() > 0 );

    Viewport::pushHourglass();
    d_backward.push_back( d_forward.back() );
    const PpmCube& cube = d_forward.back().first;
    for( Dimension d = 0; d < DimCount; d++ )
        d_pointMdl->setPos( d, d_backward.back().second[d] );
    d_plane.d_viewer->getArea()->setRange( DimX, cube[ DimX ] );
    d_plane.d_viewer->getArea()->setRange( DimY, cube[ DimY ] );
    for( Dimension d = 0; d < d_slices.size(); ++d )
    {
        d_slices[ d ].d_viewer->getArea()->setRange(
            d_slices[ d ].d_viewer->getArea()->getDim(), cube[ d ] );
        d_slices[ d ].d_viewer->update();
    }
    d_forward.pop_back();
    d_plane.d_viewer->update();
    Viewport::popCursor();
}

void FourDScope2::handleBackward()
{
    ENABLED_IF( d_backward.size() > 1 );

    Viewport::pushHourglass();
    d_forward.push_back( d_backward.back() );
    d_backward.pop_back();
    const PpmCube& cube = d_backward.back().first;
    for( Dimension d = 0; d < DimCount; d++ )
        d_pointMdl->setPos( d, d_forward.back().second[d] );
    d_plane.d_viewer->getArea()->setRange( DimX, cube[ DimX ] );
    d_plane.d_viewer->getArea()->setRange( DimY, cube[ DimY ] );
    for( Dimension d = 0; d < d_slices.size(); ++d )
    {
        d_slices[ d ].d_viewer->getArea()->setRange(
            d_slices[ d ].d_viewer->getArea()->getDim(), cube[ d ] );
        d_slices[ d ].d_viewer->update();
    }
    d_backward.pop_back();
    d_plane.d_viewer->update();
    Viewport::popCursor();
}

void FourDScope2::handleFitWindow()
{
    ENABLED_IF( true );
    d_plane.d_ol[0].d_iso->getBuf()->fitToArea();
    /*
    d_autoContour = true;	// Wegen Pascal
    updateContour( 0, true );
    */
    d_plane.d_viewer->update();
    for( Dimension d = 0; d < d_slices.size(); ++d )
    {
        d_slices[ d ].d_buf2D->fitToArea();
        d_slices[ d ].d_viewer->update();
    }
}

void FourDScope2::handleShowFolded()
{
    CHECKED_IF( true, d_folding );

    Viewport::pushHourglass();
    d_folding = !d_folding;
    for( int i = 0; i < d_plane.d_ol.size(); i++ )
        d_plane.d_ol[i].d_iso->getBuf()->setFolding( d_folding );
    if( d_folding )
        d_plane.d_viewer->setView( FOLDING, new FoldingView3( d_plane.d_ol[0].d_iso->getBuf() ) );
    else
        d_plane.d_viewer->setView( FOLDING, 0 );
    d_plane.d_viewer->update();
    Dimension d;
    for( d = 0; d < d_slices.size(); ++d )
    {
        d_slices[ d ].d_buf2D->setFolding( d_folding );
        d_slices[ d ].d_bufND->setFolding( d_folding );
        d_slices[ d ].d_viewer->update();
    }
    d_ortho.d_iso->getBuf()->setFolding( d_folding );
    if( d_folding )
        d_ortho.d_viewer->setView( FOLDING, new FoldingView3( d_ortho.d_iso->getBuf() ) );
    else
        d_ortho.d_viewer->setView( FOLDING, 0 );
    d_ortho.d_viewer->update();
    Viewport::popCursor();
}

void FourDScope2::handleSpecCalibrate()
{
    ENABLED_IF( d_plane.d_points->getSel().size() == 1 );

    SpinPoint tuple = *d_plane.d_points->getSel().begin();

    Spectrum* spec = d_spec2D;
    if( d_spec4D && d_show4DPlane )
        spec = d_spec4D;

    PpmPoint p( 0, 0 );
    for( Dimension d = 0; d < 2; d++ )
        p[ d ] = tuple[ d ]->getShift( spec ) - d_pointMdl->getPos(d);

    Viewport::pushHourglass();
    Root::Ref<SpecCalibrateCmd> cmd = new SpecCalibrateCmd( spec, p );
    cmd->handle( getAgent() );
    Viewport::popCursor();
}

void FourDScope2::handleAutoCenter()
{
    CHECKED_IF( true, d_autoCenter );

    d_autoCenter = !d_autoCenter;
    d_pointMdl->setNotifyUpdating(d_autoCenter);
}

void FourDScope2::handleShowContour()
{
    CHECKED_IF( true, d_plane.d_ol[0].d_iso->isVisible() );

    const bool visi = !d_plane.d_ol[0].d_iso->isVisible();
    for( int i = 0; i < d_plane.d_ol.size(); i++ )
        d_plane.d_ol[i].d_iso->setVisible( visi );
    updateContour( 0, true );
}

void FourDScope2::handleShowIntensity()
{
    CHECKED_IF( true, d_plane.d_intens->isVisible() );

    showIntens( !d_plane.d_intens->isVisible() );
}

void FourDScope2::handleAutoContour()
{
    CHECKED_IF( d_show4DPlane && d_aol == 0 || d_plane.d_ol[d_aol].d_iso->getBuf()->getSpectrum(),
        d_show4DPlane && d_ortho.d_iso->isAutoContour() ||
        !d_show4DPlane && d_plane.d_ol[d_aol].d_iso->isAutoContour() );

    if( d_show4DPlane && d_aol == 0 )
        handleAutoContour2();
    else
    {
        d_plane.d_ol[d_aol].d_iso->setAutoContour( !d_plane.d_ol[d_aol].d_iso->isAutoContour() );
    }
}

void FourDScope2::handleContourParams()
{
    ENABLED_IF( d_show4DPlane && d_aol == 0 || d_plane.d_ol[d_aol].d_iso->getBuf()->getSpectrum() );

    if( d_show4DPlane && d_aol == 0 )
    {
        handleContourParams2();
        return;
    }

    ContourView3::Params p = d_plane.d_ol[d_aol].d_iso->getParams();
    if( ContourParamDlg::setParams( this, p ) )
    {
        d_plane.d_ol[d_aol].d_iso->setParams( p );
        d_plane.d_ol[d_aol].d_iso->setVisible( true );
        showIntens( false );
    }
}

void FourDScope2::handlePickSystem()
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
        d_plane.d_points->selectPoint( d_pointMdl->getPos( DimX ), d_pointMdl->getPos( DimY ) );
        if( d_autoHold && !d_plane.d_points->getSel().empty() )
            d_ref = *d_plane.d_points->getSel().begin();
        updateRef();
        selectCurSystem();
        d_ortho.d_points->selectPoint( d_pointMdl->getPos( DimW ), d_pointMdl->getPos( DimZ ) );
    }
}

void FourDScope2::handlePickHori()
{
    ENABLED_IF( !d_show4DPlane && !d_spec2D.isNull() || d_show4DPlane && !d_spec4D.isNull() );
    if( !d_show4DPlane )
        extendSystem( DimY, DimX );
    else
        extendSystem4D( DimY, DimX, true );
}

void FourDScope2::extendSystem4D( Dimension main, Dimension ortho, bool plane )
{
    Q_ASSERT( d_spec4D );
    Spin* ref = 0;
    const Dimension dim = ( ortho== DimY || ortho == DimZ )?DimY:DimX;
    if( d_ref[ ortho ] != 0 )
        ref = d_ref[ ortho ];
    else if( !plane && d_ortho.d_points->getSel().size() == 1 )
        ref = ( *d_ortho.d_points->getSel().begin() )[ dim ];
    else if( plane && d_plane.d_points->getSel().size() == 1 )
        ref = ( *d_plane.d_points->getSel().begin() )[ dim ];
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
        d_ortho.d_points->selectPoint( d_pointMdl->getPos( DimW ), d_pointMdl->getPos( DimZ ) );
    }
}

void FourDScope2::specChanged(bool fourD)
{
    if( d_use4D )
        d_list->setSpec( d_spec4D );
    else
        d_list->setSpec( d_spec2D );
    if( d_ovCtrl && !fourD )
        d_ovCtrl->getBuf()->setSpectrum( d_spec2D );
}

void FourDScope2::handlePickHori4D()
{
    ENABLED_IF( d_spec4D );
    extendSystem4D( DimW, DimZ, false );
}

void FourDScope2::handlePickVerti4D()
{
    ENABLED_IF( d_spec4D );
    extendSystem4D( DimZ, DimW, false );
}

void FourDScope2::handlePickVerti()
{
    ENABLED_IF( !d_show4DPlane && !d_spec2D.isNull() || d_show4DPlane && !d_spec4D.isNull() );
    if( !d_show4DPlane )
        extendSystem( DimX, DimY );
    else
        extendSystem4D( DimX, DimY, true );
}

void FourDScope2::handleMovePeak()
{
    // Verschiebt nur den 2D-Tuple
    ENABLED_IF( d_plane.d_points->getSel().size() == 1 );

    SpinPoint tuple = *d_plane.d_points->getSel().begin();
    Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Move Spin System" );
    for( Dimension d = 0; d < 2; d++ )
    {
        cmd->add( new MoveSpinCmd( d_pro->getSpins(), tuple[ d ],
            d_pointMdl->getPos( d ), 0 ) ); // Move generisches Spektrum
    }
    cmd->handle( getAgent() );
    selectCurSystem( true );
}

void FourDScope2::handleMovePeakAlias()
{
    // Verschiebt nur den 2D-Tuple
    ENABLED_IF( d_plane.d_points->getSel().size() == 1 );

    SpinPoint tuple = *d_plane.d_points->getSel().begin();
    Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Move Spin System" );
    Spectrum* spec = getSpec();
    for( Dimension d = 0; d < 2; d++ )
    {
        if( d_pointMdl->getPos( d ) != tuple[ d ]->getShift( spec ) )
            cmd->add( new MoveSpinCmd( d_pro->getSpins(), tuple[ d ], d_pointMdl->getPos( d ), spec ) );
    }
    cmd->handle( getAgent() );
    selectCurSystem( true );
}

void FourDScope2::handleLabelPeak()
{
    ENABLED_IF( d_plane.d_points->getSel().size() == 1 );

    SpinPoint tuple = *d_plane.d_points->getSel().begin();

    Spectrum* spec = getSpec();
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

void FourDScope2::handleHidePeak()
{
    if( d_plane.d_points->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane.d_points->getSel().begin();
    SpinLink* link = tuple[ DimX ]->findLink( tuple[ DimY ] );
    ENABLED_IF( link );
    Root::Ref<HideSpinLinkCmd> cmd = new HideSpinLinkCmd( d_pro->getSpins(), link, getSpec() );
    cmd->handle( getAgent() );
    // TODO: Plural
}

void FourDScope2::handleDeletePeak()
{
    ENABLED_IF( !d_plane.d_points->getSel().empty() );

    SpinPointView::Selection sel = d_plane.d_points->getSel();
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

void FourDScope2::handleDeleteSpinX()
{
    ENABLED_IF( d_plane.d_points->getSel().size() == 1 );
    SpinPoint tuple = *d_plane.d_points->getSel().begin();

    Root::Ref<DeleteSpinCmd> cmd = new DeleteSpinCmd( d_pro->getSpins(), tuple[ DimX ] );
    cmd->handle( getAgent() );
}

void FourDScope2::handleDeleteSpinY()
{
    ENABLED_IF( d_ortho.d_points->getSel().size() == 1 );
    SpinPoint tuple = *d_ortho.d_points->getSel().begin();

    Root::Ref<DeleteSpinCmd> cmd = new DeleteSpinCmd( d_pro->getSpins(), tuple[ DimY ] );
    cmd->handle( getAgent() );
}

void FourDScope2::handleDeleteSpinX4D()
{
    ENABLED_IF( d_ortho.d_points->getSel().size() == 1 );
    SpinPoint tuple = *d_ortho.d_points->getSel().begin();

    Root::Ref<DeleteSpinCmd> cmd = new DeleteSpinCmd( d_pro->getSpins(), tuple[ DimX ] );
    cmd->handle( getAgent() );
}

void FourDScope2::handleDeleteSpinY4D()
{
    ENABLED_IF( d_plane.d_points->getSel().size() == 1 );
    SpinPoint tuple = *d_plane.d_points->getSel().begin();

    Root::Ref<DeleteSpinCmd> cmd = new DeleteSpinCmd( d_pro->getSpins(), tuple[ DimY ] );
    cmd->handle( getAgent() );
}

void FourDScope2::handleShowAllPeaks()
{
    CHECKED_IF( true, d_plane.d_mdl2D->showAll() );
    d_plane.d_mdl2D->showAll( !d_plane.d_mdl2D->showAll() );
    d_ortho.d_rot->showAll( d_plane.d_mdl2D->showAll() );
}

void FourDScope2::handleShowAlignment()
{
    if( d_plane.d_points->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane.d_points->getSel().begin();
    ENABLED_IF( tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() );

    SpinSystem* sys = tuple[ DimX ]->getSystem();

    SpinSystemString fra;
    d_pro->getSpins()->fillString( sys, fra );

    FragmentAssignment* f = new FragmentAssignment( d_pro->getSpins(),
        d_pro->getMatcher(), fra );
    SingleAlignmentView* v = new SingleAlignmentView( getAgent(), f );
    v->show();
}

void FourDScope2::handleAssign()
{
    if( d_plane.d_points->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane.d_points->getSel().begin();
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

void FourDScope2::handleUnassign()
{
    if( d_plane.d_points->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane.d_points->getSel().begin();
    ENABLED_IF( tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() );

    SpinSystem* sys = tuple[ DimX ]->getSystem();

    Root::Ref<UnassignSystemCmd> cmd =
        new UnassignSystemCmd( d_pro->getSpins(), sys );
    cmd->handle( getAgent() );
}

void FourDScope2::handleSetSystemType()
{
    if( d_plane.d_points->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane.d_points->getSel().begin();
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

void FourDScope2::handleViewLabels()
{
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    SpinPointView3::LabelType q = (SpinPointView3::LabelType) a->property("0").toInt();
    if( q < SpinPointView3::None || q >= SpinPointView3::End )
        return;

    CHECKED_IF( true,
        d_plane.d_points->getLabelType() == q );

    d_plane.d_points->setLabelType( q );
    d_plane.d_viewer->update();
}

void FourDScope2::handleSelectSpec2D()
{
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    CHECKED_IF( true, d_spec2D && d_spec2D == a->property( "0" ).value<Spec::Spectrum*>() );

    setSpec2D( a->property( "0" ).value<Spec::Spectrum*>() );
}

void FourDScope2::handleLinkSystems()
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

void FourDScope2::handleUnlinkPred()
{
    if( d_plane.d_points->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane.d_points->getSel().begin();
    ENABLED_IF( tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() &&
        tuple[ DimX ]->getSystem()->getPred() != 0 );

    SpinSystem* other = tuple[ DimX ]->getSystem()->getPred();
    Root::Ref<UnlinkSystemCmd> cmd =
        new UnlinkSystemCmd( d_pro->getSpins(), other, tuple[ DimX ]->getSystem() );
    cmd->handle( getAgent() );
}

void FourDScope2::handleUnlinkSucc()
{
    if( d_plane.d_points->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane.d_points->getSel().begin();
    ENABLED_IF( tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() &&
        tuple[ DimX ]->getSystem()->getSucc() != 0 );

    SpinSystem* other = tuple[ DimX ]->getSystem()->getSucc();
    Root::Ref<UnlinkSystemCmd> cmd =
        new UnlinkSystemCmd( d_pro->getSpins(), tuple[ DimX ]->getSystem(), other );
    cmd->handle( getAgent() );
}

void FourDScope2::handleUnhidePeak()
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

void FourDScope2::handleHoldReference()
{
    CHECKED_IF( !d_ref.isZero() || d_plane.d_points->getSel().size() == 1, !d_ref.isZero() );

    if( !d_ref.isZero() && d_plane.d_points->getSel().empty() )
        d_ref.zero(); // Hebe Zustand auf
    else if( !d_plane.d_points->getSel().empty() )
    {
        d_ref = *d_plane.d_points->getSel().begin();
        if( d_autoRuler )
        {
            d_plane.d_hRulerMdl->removeAll();
            d_plane.d_vRulerMdl->removeAll();
            SpinPoint t;
            t[ 0 ] = d_ref[ DimY ];
            d_plane.d_hRulerMdl->addPoint( t );
            t[ 0 ] = d_ref[ DimX ];
            d_plane.d_vRulerMdl->addPoint( t );
        }
    }else
        return;

    updateRef();
    selectCurSystem();
}

void FourDScope2::handleCreateReport()
{
    ENABLED_IF( true );

    Dlg::ContourParams p;
    p.d_factor = d_plane.d_ol[0].d_iso->getFactor();
    p.d_threshold =	d_spec2D->getThreshold();
    p.d_option = (Spec::ContourView::Option)d_plane.d_ol[0].d_iso->getOption();
    Q_ASSERT( getAgent()->getParent() );
    Q_ASSERT( getAgent()->getParent()->getParent() );
    ReportViewer* rv = ReportViewer::getViewer( getAgent()->getParent()->getParent(), p,
        d_plane.d_ol[0].d_iso->getGain(), d_plane.d_ol[0].d_iso->isAutoContour(), d_folding,
        (d_plane.d_ol.size()>1)?d_pro->getRepository():0 );
    ReportViewer::Spector vec;
    for( int i = 0; i < d_plane.d_ol.size(); i++ )
        if( d_plane.d_ol[i].d_iso->getBuf()->getSpectrum() )
            vec.push_back( static_cast<SpecProjector*>( d_plane.d_ol[i].d_iso->getBuf()->getSpectrum() ) );
    Root::Ref<ViewAreaMdl> vam = new ViewAreaMdl(true,true,true,true);
    vam->setRange( d_plane.d_viewer->getArea()->getRange(DimX), d_plane.d_viewer->getArea()->getRange(DimY) );
    rv->showPlane( vam, vec, new PeakProjector( *d_plane.d_peakMdl ), d_plane.d_points->getModel() );
}

void FourDScope2::handleAddRulerVerti()
{
    // Erzeuge horizontal verlaufender Ruler an Position des Y-Spins
    ENABLED_IF( !d_plane.d_points->getSel().empty() );

    SpinPointView::Selection sel = d_plane.d_points->getSel();
    SpinPointView::Selection::const_iterator p;
    SpinPoint t;
    for( p = sel.begin(); p != sel.end(); ++ p )
    {
        t[ 0 ] = (*p)[ DimY ];
        d_plane.d_hRulerMdl->addPoint( t );
    }
}

void FourDScope2::handleAddRulerHori()
{
    // Erzeuge vertikal verlaufender Ruler an Position des X-Spins
    ENABLED_IF( !d_plane.d_points->getSel().empty() );

    SpinPointView::Selection sel = d_plane.d_points->getSel();
    SpinPointView::Selection::const_iterator p;
    SpinPoint t;
    for( p = sel.begin(); p != sel.end(); ++ p )
    {
        t[ 0 ] = (*p)[ DimX ];
        d_plane.d_vRulerMdl->addPoint( t );
    }
}

void FourDScope2::handleRemoveRulers()
{
    ENABLED_IF( !d_plane.d_hRuler->getSel().empty() ||
        !d_plane.d_vRuler->getSel().empty() );

    SpinPoint1DView::Selection sel = d_plane.d_hRuler->getSel();
    SpinPoint1DView::Selection::const_iterator p;
    for( p = sel.begin(); p != sel.end(); ++p )
    {
        d_plane.d_hRulerMdl->removePoint( (*p) );
    }
    sel = d_plane.d_vRuler->getSel();
    for( p = sel.begin(); p != sel.end(); ++p )
    {
        d_plane.d_vRulerMdl->removePoint( (*p) );
    }
}

void FourDScope2::handleRemoveAllRulers()
{
    ENABLED_IF( !d_plane.d_hRulerMdl->isEmpty() ||
        !d_plane.d_vRulerMdl->isEmpty() );
    d_plane.d_hRulerMdl->removeAll();
    d_plane.d_vRulerMdl->removeAll();
}

void FourDScope2::handleAutoRuler()
{
    CHECKED_IF( true, d_autoRuler );

    d_autoRuler = !d_autoRuler;
}

void FourDScope2::handleProposeHori()
{
    if( d_ref.isZero() && d_plane.d_points->getSel().size() != 1 )
        return;
    SpinPoint tuple = d_ref;
    if( d_ref[ DimX ] == 0 )
        tuple = *d_plane.d_points->getSel().begin();

    Spin* ref = tuple[ DimY ];
    ENABLED_IF( !d_show4DPlane && ref->getSystem() && getSpec()->hasNoesy() );

    ProposeSpinDlg dlg( this, d_pro, getSpec()->getColor( DimX ), d_pointMdl->getPos( DimX ),
                        getSpec(), "Select Horizontal Partner" );
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

void FourDScope2::handleProposeVerti()
{
    if( d_ref.isZero() && d_plane.d_points->getSel().size() != 1 )
        return;
    SpinPoint tuple = d_ref;
    if( d_ref[ DimX ] == 0 )
        tuple = *d_plane.d_points->getSel().begin();

    Spin* ref = tuple[ DimX ];
    ENABLED_IF( !d_show4DPlane && ref->getSystem() && getSpec()->hasNoesy()  );

    ProposeSpinDlg dlg( this, d_pro, getSpec()->getColor( DimY ), d_pointMdl->getPos( DimY ),
        getSpec(), "Select Vertical Partner" );
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

void FourDScope2::handleProposePeak()
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

void FourDScope2::handleSelectSpec4D()
{
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    CHECKED_IF( true, d_spec4D && d_spec4D == a->property( "0" ).value<Spec::Spectrum*>() );

    setSpec4D( a->property( "0" ).value<Spec::Spectrum*>() );
    selectCurSystem();
}

void FourDScope2::handlePickLabel4D()
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

    Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Pick Labels" );
    if( sys->isAcceptable( pair[1] ) )
        cmd->add( new PickSystemSpinLabelCmd( d_pro->getSpins(),
                                              sys, d_spec4D->getColor( DimZ ), d_pointMdl->getPos( DimZ ), pair[1], 0 ) );
    if( sys->isAcceptable( pair[0] ) )
        cmd->add( new PickSystemSpinLabelCmd( d_pro->getSpins(),
                                              sys, d_spec4D->getColor( DimW ), d_pointMdl->getPos( DimW ), pair[0], 0 ) );
    if( !cmd->empty() )
        cmd->handle( getAgent() );

    d_ortho.d_points->selectPoint( d_pointMdl->getPos( DimX ), d_pointMdl->getPos( DimY ) );
}

void FourDScope2::handlePickSpin4D()
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
    d_ortho.d_points->selectPoint( d_pointMdl->getPos( DimW ), d_pointMdl->getPos( DimZ ) );
}

void FourDScope2::handleMoveSpin4D()
{
    ENABLED_IF( d_ortho.d_points->getSel().size() == 1 );

    SpinPoint tuple = *d_ortho.d_points->getSel().begin();
    Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Move Spins" );
    // Move generisches Spektrum
    cmd->add( new MoveSpinCmd( d_pro->getSpins(), tuple[ DimX ], d_pointMdl->getPos( DimW ), 0 ) );
    cmd->add( new MoveSpinCmd( d_pro->getSpins(), tuple[ DimY ], d_pointMdl->getPos( DimZ ), 0 ) );
    cmd->handle( getAgent() );
}

void FourDScope2::handleMoveSpinAlias4D()
{
    ENABLED_IF( d_ortho.d_points->getSel().size() == 1 );

    SpinPoint tuple = *d_ortho.d_points->getSel().begin();
    Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Move Spin Aliasses" );
    cmd->add( new MoveSpinCmd( d_pro->getSpins(), tuple[ DimX ], d_pointMdl->getPos( DimW ), d_spec4D ) );
    cmd->add( new MoveSpinCmd( d_pro->getSpins(), tuple[ DimY ], d_pointMdl->getPos( DimZ ), d_spec4D ) );
    cmd->handle( getAgent() );
}

void FourDScope2::handleDeleteSpins4D()
{
    ENABLED_IF( !d_spec4D.isNull() && !d_ortho.d_points->getSel().empty() );

    SpinPointView::Selection sel = d_ortho.d_points->getSel();
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

void FourDScope2::handleLabelSpin4D()
{
    ENABLED_IF( !d_spec4D.isNull() && d_ortho.d_points->getSel().size() == 1 );

    SpinPoint tuple = *d_ortho.d_points->getSel().begin();

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

void FourDScope2::handleSetWidth()
{
    ENABLED_IF( !d_spec4D.isNull() && d_ortho.d_viewer->hasFocus() );

    PpmPoint wxy( d_pro->inferPeakWidth( DimX, d_spec4D ), d_pro->inferPeakWidth( DimY, d_spec4D ),
                  d_pro->inferPeakWidth( DimZ, d_spec4D ),
                  d_pro->inferPeakWidth( DimW, d_spec4D ) );
    if( Dlg::getPpmPoint( this, wxy, "Set Peak Width", QStringList() << d_spec4D->getDimName(DimX) <<
                          d_spec4D->getDimName(DimY) << d_spec4D->getDimName(DimZ) << d_spec4D->getDimName(DimW ) ) )
    {
        if( wxy[0] < 0.0 || wxy[1] < 0.0 )
        {
            QMessageBox::critical( this, "Set Peak Width",
                    "Invalid peak width!", "&Cancel" );
            return;
        }
        d_pro->setPeakWidth( DimX, wxy[0], d_spec4D );
        d_pro->setPeakWidth( DimY, wxy[1], d_spec4D );
        d_pro->setPeakWidth( DimZ, wxy[2], d_spec4D );
        d_pro->setPeakWidth( DimW, wxy[3], d_spec4D );

    }
}

void FourDScope2::handleFitWindow4D()
{
    ENABLED_IF( !d_spec4D.isNull() );
    if( d_spec4D.isNull() )
        return;	// QT-BUG
    d_ortho.d_iso->getBuf()->fitToArea();
    d_slices[ DimZ ].d_bufND->fitToArea();
    d_slices[ DimZ ].d_viewer->update();
    d_slices[ DimW ].d_bufND->fitToArea();
    d_slices[ DimW ].d_viewer->update();

//    PpmRange r = d_spec4D->getScale( DimZ ).getRange();
//	r.invert();
//	d_ortho.d_viewer->getArea()->setRange( DimY, r );
//	d_slices[ DimZ ].d_viewer->getArea()->setRange( DimY, r );
//    r = d_spec4D->getScale( DimW ).getRange();
//	// TODO r.invert();
//	d_ortho.d_viewer->getArea()->setRange( DimX, r );
//	d_slices[ DimW ].d_viewer->getArea()->setRange( DimX, r );
    d_ortho.d_viewer->update();
}

void FourDScope2::handleAutoContour2()
{
    CHECKED_IF( true, d_ortho.d_iso->isAutoContour() );

    d_ortho.d_iso->setAutoContour( !d_ortho.d_iso->isAutoContour() );
    if( d_show4DPlane )
        updateContour( 0, true );
}

void FourDScope2::handleContourParams2()
{
    ENABLED_IF( d_spec4D );

    ContourView3::Params p = d_ortho.d_iso->getParams();
    if( ContourParamDlg::setParams( this, p ) )
    {
        d_ortho.d_iso->setParams( p );
        d_ortho.d_iso->setVisible( true );
        if( d_show4DPlane )
        {
            showIntens( false );
            updateContour( 0, true );
        }
    }
}

void FourDScope2::handleShowWithOff()
{
    CHECKED_IF( true, d_src4D->showOffs() );

    d_src4D->showOffs( !d_src4D->showOffs() );
}

void FourDScope2::setShow4dPlane(bool on)
{
    if( on )
    {
        if( d_show4DPlane != on )
            d_contourParamsTmp = d_plane.d_ol[0].d_iso->getParams();
        if( d_spec4D )
            d_plane.d_ol[0].d_iso->getBuf()->setSpectrum( new SpecProjector( d_spec4D, d_pointMdl, DimX, DimY ) );
        else
            d_plane.d_ol[0].d_iso->getBuf()->setSpectrum( 0 );
        d_plane.d_points->setModel( d_plane.d_mdlND );
        d_plane.d_peakMdl->setGhostWidth( d_plane.d_mdlND->getGhostWidth()[0], d_plane.d_mdlND->getGhostWidth()[1] );
        d_plane.d_peaks->setSpec( d_spec4D );
    }else
    {
        if( d_spec2D )
            d_plane.d_ol[0].d_iso->getBuf()->setSpectrum( new SpecProjector( d_spec2D, d_pointMdl, DimX, DimY ) );
        else
            d_plane.d_ol[0].d_iso->getBuf()->setSpectrum( 0 );
        if( d_show4DPlane != on )
            d_plane.d_ol[0].d_iso->setParams(d_contourParamsTmp);
        d_plane.d_points->setModel( d_plane.d_mdl2D );
        d_plane.d_peakMdl->setGhostWidth( 0, 0 );
        d_plane.d_peaks->setSpec( d_spec2D );
    }
    d_show4DPlane = on;
    d_plane.d_vRulerMdl->setSpec( d_plane.d_ol[0].d_iso->getBuf()->getSpectrum() );
    d_plane.d_hRulerMdl->setSpec( d_plane.d_ol[0].d_iso->getBuf()->getSpectrum() );
    updateContour( 0, true );
    updatePlaneLabel();
    d_plane.d_viewer->update();
    specChanged( true );
}

void FourDScope2::handleShowPathSim()
{
    ENABLED_IF( d_ortho.d_viewer->hasFocus() && !d_spec4D.isNull() ||
                d_plane.d_viewer->hasFocus() );

    PathSimDlg dlg( (d_ortho.d_viewer->hasFocus())?d_spec4D.deref():getSpec(), this );
    if( d_cur[DimX] && d_cur[DimX]->getSystem() && d_cur[DimX]->getSystem()->getAssig() )
        dlg.setResiType( d_cur[DimX]->getSystem()->getAssig()->getType() );
    dlg.exec();
}

void FourDScope2::handleShow4dPlane()
{
    // Einschalten nur wenn 4D vorhanden; ausschalten nur wenn 2D vorhanden
    CHECKED_IF( !d_show4DPlane && d_spec4D || d_show4DPlane && d_spec2D, d_show4DPlane );
    setShow4dPlane( !d_show4DPlane );
}

void FourDScope2::handleAutoHide()
{
    CHECKED_IF( true, d_autoHide );

    d_autoHide = !d_autoHide;
    if( d_spec4D )
    {
        show4dPlaneSlice( DimX, !d_autoHide );
        show4dPlaneSlice( DimY, !d_autoHide );
    }
    d_slices[ DimX ].d_viewer->update();
    d_slices[ DimY ].d_viewer->update();
}

void FourDScope2::handleStripCalibrate()
{
    ENABLED_IF( d_ortho.d_points->getSel().size() == 1 );

    SpinPoint tuple = *d_ortho.d_points->getSel().begin();

    PpmPoint p( 0, 0, 0, 0 );
    p[ DimZ ] = tuple[ DimY ]->getShift( d_spec4D ) - d_pointMdl->getPos( DimZ );
    p[ DimW ] = tuple[ DimX ]->getShift( d_spec4D ) - d_pointMdl->getPos( DimW );

    Viewport::pushHourglass();
    Root::Ref<SpecCalibrateCmd> cmd = new SpecCalibrateCmd( d_spec4D, p );
    cmd->handle( getAgent() );
    Viewport::popCursor();
}

void FourDScope2::handleProposeSpin()
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
    cmd->handle( getAgent() );
}

void FourDScope2::handleEditAttsSpinH()
{
    ENABLED_IF( d_plane.d_points->getSel().size() == 1 );
    SpinPoint tuple = *d_plane.d_points->getSel().begin();

    DynValueEditor::edit( this,
        d_pro->getRepository()->findObjectDef( Repository::keySpin ), tuple[ DimX ] );
}

void FourDScope2::handleEditAttsSpinV()
{
    ENABLED_IF( d_plane.d_points->getSel().size() == 1 );
    SpinPoint tuple = *d_plane.d_points->getSel().begin();

    DynValueEditor::edit( this,
        d_pro->getRepository()->findObjectDef( Repository::keySpin ), tuple[ DimY ] );
}

void FourDScope2::handleEditAttsLink()
{
    if( d_plane.d_points->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane.d_points->getSel().begin();
    SpinLink* l = tuple[ DimX ]->findLink( tuple[ DimY ] );
    ENABLED_IF( l );

    DynValueEditor::edit( this,
        d_pro->getRepository()->findObjectDef( Repository::keyLink ), l );
}

void FourDScope2::handleEditAttsSysH()
{
    if( d_plane.d_points->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane.d_points->getSel().begin();
    SpinSystem* s = tuple[ DimX ]->getSystem();
    ENABLED_IF( s );

    DynValueEditor::edit( this,
        d_pro->getRepository()->findObjectDef( Repository::keySpinSystem ), s );
}

void FourDScope2::handleEditAttsSysV()
{
    if( d_plane.d_points->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane.d_points->getSel().begin();
    SpinSystem* s = tuple[ DimY ]->getSystem();
    ENABLED_IF( s );

    DynValueEditor::edit( this,
        d_pro->getRepository()->findObjectDef( Repository::keySpinSystem ), s );
}

void FourDScope2::handleEditAttsSpinX4D()
{
    ENABLED_IF( d_ortho.d_points->getSel().size() == 1 );
    SpinPoint tuple = *d_ortho.d_points->getSel().begin();

    DynValueEditor::edit( this,
        d_pro->getRepository()->findObjectDef( Repository::keySpin ), tuple[ DimX ] );
}

void FourDScope2::handleEditAttsSpinY4D()
{
    ENABLED_IF( d_ortho.d_points->getSel().size() == 1 );
    SpinPoint tuple = *d_ortho.d_points->getSel().begin();

    DynValueEditor::edit( this,
        d_pro->getRepository()->findObjectDef( Repository::keySpin ), tuple[ DimY ] );
}

void FourDScope2::handleCursorSync()
{
    CHECKED_IF( true, d_cursorSync );

    d_cursorSync = !d_cursorSync;
    if( d_cursorSync )
        GlobalCursor::addObserver( this );	// TODO: preset Cursor
    else if( !d_rangeSync )
        GlobalCursor::removeObserver( this );
}

void FourDScope2::handleGotoSystem()
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

void FourDScope2::handleNextSpec4D()
{
    ENABLED_IF( d_sort4D.size() > 1 );
    stepSpec4D( true );
}

void FourDScope2::handlePrevSpec4D()
{
    ENABLED_IF( d_sort4D.size() > 1 );
    stepSpec4D( false );
}

void FourDScope2::handleNextSpec2D()
{
    ENABLED_IF( d_sort2D.size() > 1 );
    stepSpec2D( true );
}

void FourDScope2::handlePrevSpec2D()
{
    ENABLED_IF( d_sort2D.size() > 1 );
    stepSpec2D( false );
}

void FourDScope2::handleShowWithOff2()
{
    CHECKED_IF( !d_show4DPlane, d_src2D->showOffs() );

    d_src2D->showOffs( !d_src2D->showOffs() );
    d_plane.d_viewer->update();
}

void FourDScope2::handleShowLinks()
{
    CHECKED_IF( !d_show4DPlane, d_src2D->showLinks() );

    d_src2D->showLinks( !d_src2D->showLinks() );
    d_plane.d_viewer->update();
}

void FourDScope2::handleShowLinks2()
{
    CHECKED_IF( true, d_src4D->showLinks() );

    d_src4D->showLinks( !d_src4D->showLinks() );
    d_plane.d_viewer->update();
}

void FourDScope2::handleDeleteLinks()
{
    ENABLED_IF( !d_plane.d_points->getSel().empty() );

    SpinPointView::Selection sel = d_plane.d_points->getSel();
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

void FourDScope2::handleLabelVerti()
{
    ENABLED_IF( d_plane.d_points->getSel().size() == 1 );

    SpinPoint tuple = *d_plane.d_points->getSel().begin();

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

void FourDScope2::handleLabelHori()
{
    ENABLED_IF( d_plane.d_points->getSel().size() == 1 );

    SpinPoint tuple = *d_plane.d_points->getSel().begin();

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

void FourDScope2::handleSetCandidates()
{
    if( d_plane.d_points->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane.d_points->getSel().begin();
    ENABLED_IF( tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() );

    SpinSystem* sys = tuple[ DimX ]->getSystem();

    CandidateDlg dlg( this, d_pro->getRepository() );
    dlg.setTitle( sys );
    if( dlg.exec() )
        d_pro->getSpins()->setCands( sys, dlg.d_cands );
}

void FourDScope2::handleShowInfered()
{
    CHECKED_IF( !d_show4DPlane, d_src2D->showInferred() );

    d_src2D->showInferred( !d_src2D->showInferred() );
    d_plane.d_viewer->update();
}

void FourDScope2::handleShowInfered2()
{
    CHECKED_IF( true, d_src4D->showInferred() );

    d_src4D->showInferred( !d_src4D->showInferred() );
    d_plane.d_viewer->update();
}

void FourDScope2::handleShowUnlabeled()
{
    CHECKED_IF( !d_show4DPlane, d_src2D->showNulls() );

    d_src2D->showNulls( !d_src2D->showNulls() );
    d_plane.d_viewer->update();
}

void FourDScope2::handleShowUnlabeled2()
{
    CHECKED_IF( true, d_src4D->showNulls() );

    d_src4D->showNulls( !d_src4D->showNulls() );
    d_plane.d_viewer->update();
}

void FourDScope2::handleShowUnknown()
{
    CHECKED_IF( !d_show4DPlane, d_src2D->showUnknown() );

    d_src2D->showUnknown( !d_src2D->showUnknown() );
    d_plane.d_viewer->update();
}

void FourDScope2::handleShowUnknown2()
{
    CHECKED_IF( true, d_src4D->showUnknown() );

    d_src4D->showUnknown( !d_src4D->showUnknown() );
    d_plane.d_viewer->update();
}

void FourDScope2::handleCreateLinks()
{
    CHECKED_IF( true, false );
}

void FourDScope2::handleForceCross()
{
    CHECKED_IF( true, d_src2D->doPathsim() );

    d_src2D->doPathsim( !d_src2D->doPathsim() );
    d_src4D->doPathsim( d_src2D->doPathsim() );
    d_plane.d_viewer->update();
}

void FourDScope2::handleDeleteLinks4D()
{
    ENABLED_IF( !d_spec4D.isNull() && !d_ortho.d_points->getSel().empty() );

    SpinPointView::Selection sel = d_ortho.d_points->getSel();
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

void FourDScope2::handleViewLabels4D()
{
    Gui2::UiFunction* a = Gui2::UiFunction::me();

    SpinPointView3::LabelType q = (SpinPointView3::LabelType) a->property( "0" ).toInt();
    if( q < SpinPointView3::None || q >= SpinPointView3::End )
        return;

    CHECKED_IF( true,
        d_ortho.d_points->getLabelType() == q );

    d_ortho.d_points->setLabelType( q, DimY );
    d_ortho.d_viewer->update();
}

void FourDScope2::handleAutoGain()
{
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    ENABLED_IF( !a->property( "0" ).isNull() && !d_show4DPlane );

    double g = a->property( "0" ).toDouble();
    if( g <= 0.0 )
    {
        Root::ReportToUser::alert( this, "Set Auto Gain", "Invalid Gain Value" );
        return;
    }
    int l = d_aol;
    if( a->property( "1" ).isNull() )
        l = selectLayer();
    else if( a->property( "1" ).toByteArray() == "*" )
        l = -1;
    else
        l = a->property( "1" ).toInt();
    if( l == -2 )
        return;
    else if( l == -1 ) // All
    {
        for( l = 0; l < d_plane.d_ol.size(); l++ )
        {
            d_plane.d_ol[l].d_iso->setVisible( true );
            d_plane.d_ol[l].d_iso->setGain( g, true );
        }
    }else if( l >= 0 && l < d_plane.d_ol.size() )
    {
        d_plane.d_ol[l].d_iso->setVisible( true );
        d_plane.d_ol[l].d_iso->setGain( g, true );
    }
    d_plane.d_viewer->update();
}

void FourDScope2::handleAutoGain4D()
{
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    ENABLED_IF( !a->property( "0" ).isNull() );
    double g = a->property( "0" ).toDouble();
    if( g <= 0.0 )
    {
        Root::ReportToUser::alert( this, "Set Auto Gain", "Invalid Gain Value" );
        return;
    }
    d_ortho.d_iso->setGain( g );
    updateContour4D( true );
}

void FourDScope2::handleShowGhosts()
{
    CHECKED_IF( true, d_plane.d_mdlND->showGhosts() );

    d_plane.d_mdlND->showGhosts( !d_plane.d_mdlND->showGhosts() );
    d_ortho.d_range->showGhosts( d_plane.d_mdlND->showGhosts() );
}

void FourDScope2::handleAutoHold()
{
    CHECKED_IF( true, d_autoHold );
    d_autoHold = !d_autoHold;
}

void FourDScope2::handleGhostLabels()
{
    CHECKED_IF( true, d_plane.d_points->getGhostLabel() );

    d_plane.d_points->setGhostLabel( !d_plane.d_points->getGhostLabel() );
    d_ortho.d_points->setGhostLabel( d_plane.d_points->getGhostLabel() );
}

void FourDScope2::handleHidePeak4D()
{
    if( d_ortho.d_points->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_ortho.d_points->getSel().begin();
    SpinLink* link = tuple[ DimX ]->findLink( tuple[ DimY ] );
    ENABLED_IF( link );
    Root::Ref<HideSpinLinkCmd> cmd = new HideSpinLinkCmd( d_pro->getSpins(), link, d_spec4D );
    cmd->handle( getAgent() );
    // TODO: Plural
}

void FourDScope2::handleGotoPeak()
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

void FourDScope2::handleRangeSync()
{
    CHECKED_IF( true, d_rangeSync );

    d_rangeSync = !d_rangeSync;
    if( d_rangeSync )
        GlobalCursor::addObserver( this );	// TODO: preset Cursor
    else if( !d_cursorSync )
        GlobalCursor::removeObserver( this );
}

void FourDScope2::handleEditAttsSys4D()
{
    ENABLED_IF( !d_spec4D.isNull() && !d_cur.isZero() && d_cur[DimX]->getSystem() );

    DynValueEditor::edit( this,
        d_pro->getRepository()->findObjectDef( Repository::keySpinSystem ),
        d_cur[DimX]->getSystem() );
}

void FourDScope2::handleEditAttsLink4D()
{
    if( d_ortho.d_points->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_ortho.d_points->getSel().begin();
    SpinLink* l = tuple[ DimX ]->findLink( tuple[ DimY ] );
    ENABLED_IF( l );

    DynValueEditor::edit( this,
        d_pro->getRepository()->findObjectDef( Repository::keyLink ), l );
}

void FourDScope2::handleOverlayCount()
{
    ENABLED_IF( true );

    Gui2::UiFunction* a = Gui2::UiFunction::me();
    Root::Index c;
    if( a->property( "0" ).isNull() )
    {
        bool ok	= FALSE;
        c = QInputDialog::getInteger( "Set Overlay Count",
            "Please	enter a positive number:",
            d_plane.d_ol.size(), 1, 9, 1, &ok, this );
        if( !ok )
            return;
    }else
        c = a->property( "0" ).toInt();
    if( c < 1 )
    {
        QMessageBox::critical( this, "Set Overlay Count",
                "Invalid Count!", "&Cancel" );
        return;
    }
    initOverlay( c );
}

void FourDScope2::handleActiveOverlay()
{
    ENABLED_IF( true );

    Gui2::UiFunction* a = Gui2::UiFunction::me();
    Root::Index c;
    if( a->property( "0" ).isNull() )
    {
        Dlg::StringList l( d_plane.d_ol.size() );
        QString str;
        for( int i = 0; i < d_plane.d_ol.size(); i++ )
        {
            if( d_plane.d_ol[ i ].d_iso->getBuf()->getSpectrum() )
                str.sprintf( "&%d %s", i, d_plane.d_ol[ i ].d_iso->getBuf()->getSpectrum()->getName() );
            else
                str.sprintf( "&%d <empty>", i );
            l[ i ] = str.toLatin1();
        }
        c = Dlg::getOption( this, l,
            "Select Active Overlay", d_aol );
        if( c == -1 )
            return;
    }else
        c = a->property( "0" ).toInt();
    if( c < 0 || c >= d_plane.d_ol.size() )
    {
        QMessageBox::critical( this, "Set Active Overlay",
                "Invalid Overlay Number!", "&Cancel" );
        return;
    }
    setActiveOverlay( c );
}

void FourDScope2::handleSetPosColor()
{
    ENABLED_IF( true );

    QColor clr = QColorDialog::getColor( d_plane.d_ol[d_aol].d_iso->getPosColor(),
        this );
    if( clr.isValid() )
    {
        d_plane.d_ol[d_aol].d_iso->setPosColor( ( clr ) );
        d_plane.d_viewer->update();
    }
}

void FourDScope2::handleSetNegColor()
{
    ENABLED_IF( true );

    QColor clr = QColorDialog::getColor( d_plane.d_ol[d_aol].d_iso->getNegColor(),
        this );
    if( clr.isValid() )
    {
        d_plane.d_ol[d_aol].d_iso->setNegColor( ( clr ) );
        d_plane.d_viewer->update();
    }
}

void FourDScope2::handleOverlaySpec()
{
    ENABLED_IF( true );

    Spectrum* spec = d_plane.d_ol[d_aol].d_iso->getBuf()->getSpectrum();
    Root::Index c = 0;
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    if( a->property( "0" ).isNull() )
    {
        Dlg::StringSet s;
        Project::SpectrumMap::const_iterator i;
        s.insert( "" ); // empty
        for( i = d_pro->getSpectra().begin(); i != d_pro->getSpectra().end(); ++i )
            if( (*i).second->getDimCount() == 2 )
                s.insert( (*i).second->getName() );
        if( !Dlg::selectStrings( this,
            "Select Overlay Spectrum", s, false ) || s.empty() )
            return;
        spec = d_pro->findSpectrum( (*s.begin()).data() );
    }else
    {
        c = a->property( "0" ).toInt();

        spec = d_pro->getSpec( c );
        if( spec == 0 && c != 0 )
        {
            QMessageBox::critical( this, "Set Overlay Spectrum",
                    "Invalid spectrum ID!", "&Cancel" );
            return;
        }
        if( spec && spec->getDimCount() != 2 )
        {
            QMessageBox::critical( this, "Set Overlay Spectrum",
                    "Invalid number of dimensions!", "&Cancel" );
            return;
        }
    }
    if( d_aol == 0 && spec == 0 )
    {
        QMessageBox::critical( this, "Set Overlay Spectrum",
                "Cannot remove spectrum of layer 0!", "&Cancel" );
        return;
    }
    if( d_aol == 0 )
        setSpec2D( spec );
    else if( spec )
    {
        d_plane.d_ol[d_aol].d_iso->getBuf()->setSpectrum( new SpecProjector( spec, d_pointMdl, DimX, DimY ) );
        d_plane.d_viewer->update();
    }else
    {
        d_plane.d_ol[d_aol].d_iso->getBuf()->setSpectrum( 0 );
        d_plane.d_viewer->update();
    }
    updatePlaneLabel();
}

void FourDScope2::handleCntFactor()
{
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    ENABLED_IF( !d_show4DPlane && !a->property( "0" ).isNull() );

    double g = a->property( "0" ).toDouble();
    if( g <= 1.0 || g > 10.0 )
    {
        Root::ReportToUser::alert( this, "Set Contour Factor", "Invalid Factor Value" );
        return;
    }
    int l = d_aol;
    if( a->property( "1" ).isNull() )
        l = selectLayer();
    else if( a->property( "1" ).toByteArray() == "*" )
        l = -1;
    else
        l = a->property( "1" ).toInt();
    if( l == -2 )
        return;
    else if( l == -1 ) // All
    {
        for( l = 0; l < d_plane.d_ol.size(); l++ )
        {
            d_plane.d_ol[l].d_iso->setFactor( g );
            d_plane.d_ol[l].d_iso->setVisible( true );
            updateContour( l, false );
        }
        d_plane.d_viewer->update();
    }else if( l >= 0 && l < d_plane.d_ol.size() )
    {
        d_plane.d_ol[l].d_iso->setFactor( g );
        d_plane.d_ol[l].d_iso->setVisible( true );
        updateContour( l, true );
    }
}

void FourDScope2::handleCntThreshold()
{
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    ENABLED_IF( !d_show4DPlane && !a->property( "0" ).isNull() );

    double g = a->property( "0" ).toDouble();
    if( g < 0.0 )
    {
        Root::ReportToUser::alert( this, "Set Spectrum Threshold", "Invalid Threshold Value" );
        return;
    }
    int l = d_aol;
    if( a->property( "1" ).isNull() )
        l = selectLayer();
    else if( a->property( "1" ).toByteArray() == "*" )
        l = -1;
    else
        l = a->property( "1" ).toInt();
    if( l == -2 )
        return;
    else if( l == -1 ) // All
    {
        for( l = 0; l < d_plane.d_ol.size(); l++ )
        {
            d_plane.d_ol[l].d_iso->setVisible( true );
            d_plane.d_ol[l].d_iso->setThreshold( g, true );
        }
    }else if( l >= 0 && l < d_plane.d_ol.size() && d_plane.d_ol[l].d_iso->getBuf()->getSpectrum() )
    {
        d_plane.d_ol[l].d_iso->setVisible( true );
        d_plane.d_ol[l].d_iso->setThreshold( g, true );
    }
    d_plane.d_viewer->update();
}

void FourDScope2::handleCntOption()
{
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    ENABLED_IF( !d_show4DPlane && !a->property( "0" ).isNull() );

    ContourView::Option o = ContourView::Both;
    if( a->property( "0" ).toByteArray() == "+" )
        o = ContourView::Positive;
    else if( a->property( "0" ).toByteArray() == "-" )
        o = ContourView::Negative;

    int l = d_aol;
    if( a->property( "1" ).isNull() )
        l = selectLayer();
    else if( a->property( "1" ).toByteArray() == "*" )
        l = -1;
    else
        l = a->property( "1" ).toInt();
    if( l == -2 )
        return;
    else if( l == -1 ) // All
    {
        for( l = 0; l < d_plane.d_ol.size(); l++ )
        {
            d_plane.d_ol[l].d_iso->setOption( (ContourView3::Option)o );
            d_plane.d_ol[l].d_iso->setVisible( true );
            updateContour( l, false );
        }
        d_plane.d_viewer->update();
    }else if( l >= 0 && l < d_plane.d_ol.size() )
    {
        d_plane.d_ol[l].d_iso->setOption( (ContourView3::Option)o );
        d_plane.d_ol[l].d_iso->setVisible( true );
        updateContour( l, true );
    }
}

void FourDScope2::handleAddLayer()
{
    ENABLED_IF( true );

    initOverlay( d_plane.d_ol.size() + 1 );
    setActiveOverlay( d_plane.d_ol.size() - 1 );
    handleOverlaySpec();
}

void FourDScope2::handleComposeLayers()
{
    ENABLED_IF( true );

    ColorMap cm( 2 );
    Root::Ref<PeakList> pl = new PeakList( cm );
    PeakList::SpecList l;
    for( int i = 0; i < d_plane.d_ol.size(); i++ )
        if( d_plane.d_ol[i].d_iso->getBuf()->getSpectrum() )
            l.push_back( d_plane.d_ol[i].d_iso->getBuf()->getSpectrum()->getId() );
    pl->setSpecs( l );
    SpecBatchList dlg( this, pl, d_pro );
    dlg.setCaption( "Compose Layers" );
    if( dlg.doit() && !pl->getSpecs().empty() )
    {
        const PeakList::SpecList& s = pl->getSpecs();
        initOverlay( s.size() );
        Spectrum* spec;
        for( int i = 0; i < s.size(); i++ )
        {
            spec = d_pro->getSpec( s[ i ] );
            Q_ASSERT( spec );
            d_plane.d_ol[i].d_iso->getBuf()->setSpectrum( new SpecProjector( spec, d_pointMdl, DimX, DimY ) );
        }
        d_plane.d_viewer->update();
        updatePlaneLabel();
    }
}

void FourDScope2::handleUseLinkColors()
{
    CHECKED_IF( true, d_plane.d_points->getColors() );
    if( d_plane.d_points->getColors() )
        d_plane.d_points->setColors( 0 );
    else
        d_plane.d_points->setColors( d_pro->getRepository()->getColors() );
}

void FourDScope2::handleUseLinkColors4D()
{
    CHECKED_IF( true, d_ortho.d_points->getColors() );

    if( d_ortho.d_points->getColors() )
    {
        d_ortho.d_points->setColors( 0 );
    }else
    {
        d_ortho.d_points->setColors( d_pro->getRepository()->getColors() );
    }
}

void FourDScope2::handleSetLinkParams()
{
    if( d_plane.d_points->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_plane.d_points->getSel().begin();
    SpinLink* l = tuple[ DimX ]->findLink( tuple[ DimY ] );
    ENABLED_IF( l );
    const SpinLink::Alias& al = l->getAlias( getSpec() );
    Dlg::LinkParams2 par;
    par.d_rating = al.d_rating;
    par.d_code = al.d_code;
    par.d_visible = al.d_visible;
    if( Dlg::getLinkParams2( this, par ) )
        d_pro->getSpins()->setAlias( l, getSpec(),
        par.d_rating, par.d_code, par.d_visible );
}

void FourDScope2::handleSetLinkParams4D()
{
    if( d_spec4D.isNull() || d_ortho.d_points->getSel().size() != 1 )
        return;
    SpinPoint tuple = *d_ortho.d_points->getSel().begin();
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

void FourDScope2::handleGotoPoint()
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

void FourDScope2::handleNewPeakList()
{
    ENABLED_IF( true );
    if( !askToClosePeaklist() )
        return;

    Spectrum* spec = d_spec2D;
    if( d_spec4D && d_show4DPlane )
        spec = d_spec4D;
    setPeakList( new PeakList( spec ) );
}

void FourDScope2::handleOpenPeakList()
{
    ENABLED_IF( true );

    PeakList* pl = Dlg::selectPeakList( this, d_pro );
    if( pl == 0 )
        return;
    setPeakList( pl );
}

void FourDScope2::handleSavePeakList()
{
    ENABLED_IF( d_pl && d_pl->getId() == 0 );

    savePeakList();
}

void FourDScope2::handleMapPeakList()
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

void FourDScope2::handlePickPlPeak()
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
    pd.d_amp = spec->getAt( p, true, d_folding );
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

void FourDScope2::handleMovePlPeak()
{
    ENABLED_IF( d_pl && d_plane.d_peaks->getSel().size() == 1 );

    Root::Index peak = *d_plane.d_peaks->getSel().begin();
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
    Amplitude amp = spec->getAt( p, true, d_folding );
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

void FourDScope2::handleMovePlAlias()
{
    ENABLED_IF( d_pl && d_plane.d_peaks->getSel().size() == 1 );

    Root::Index peak = *d_plane.d_peaks->getSel().begin();
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
    Amplitude amp = spec->getAt( p, true, d_folding );
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

void FourDScope2::handleLabelPlPeak()
{
    ENABLED_IF( d_pl && d_plane.d_peaks->getSel().size() == 1 );

    Root::Index peak = ( *d_plane.d_peaks->getSel().begin() );
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

void FourDScope2::handleDeletePlPeaks()
{
    ENABLED_IF( d_pl && !d_plane.d_peaks->getSel().empty() );

    Root::Ref<Root::MakroTransaction> cmd = new Root::MakroTransaction( "Delete Peaks" );
    try
    {
        PeakPlaneView::Selection sel = d_plane.d_peaks->getSel();
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

void FourDScope2::handleEditPlPeakAtts()
{
    ENABLED_IF( d_pl && d_plane.d_peaks->getSel().size() == 1 );

    Root::Index id = ( *d_plane.d_peaks->getSel().begin() );
    Peak* peak = d_pl->getPeakList()->getPeak( id );
    DynValueEditor::edit( this,
        d_pro->getRepository()->findObjectDef( Repository::keyPeak ), peak );
}

void FourDScope2::handleSetPlColor()
{
    ENABLED_IF( true );

    QColor clr = QColorDialog::getColor( d_plane.d_peaks->getColor(),
        this );
    if( clr.isValid() )
    {
        d_plane.d_peaks->setColor( ( clr ) );
        d_plane.d_viewer->update();
        d_ortho.d_peaks->setColor( d_plane.d_peaks->getColor() );
        d_ortho.d_viewer->update();
    }
}

void FourDScope2::handleDeleteAliasPeak()
{
    ENABLED_IF( !d_plane.d_points->getSel().empty() );

    SpinPointView::Selection sel = d_plane.d_points->getSel();
    SpinPointView::Selection::const_iterator p;
    Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Delete Spins" );
    std::set<Spin*> test;
    for( p = sel.begin(); p != sel.end(); ++p )
    {
        if( test.count( (*p)[ DimX ] ) == 0 )
        {
            cmd->add( new MoveSpinCmd( d_pro->getSpins(), (*p)[ DimX ],
            (*p)[ DimX ]->getShift(), // auf Home schieben l�cht Alias
            getSpec() ) );
            test.insert( (*p)[ DimX ] );
        }
        if( test.count( (*p)[ DimY ] ) == 0 )
        {
            cmd->add( new MoveSpinCmd( d_pro->getSpins(), (*p)[ DimY ],
            (*p)[ DimY ]->getShift(), // auf Home schieben l�cht Alias
            getSpec() ) );
            test.insert( (*p)[ DimY ] );
        }
    }
    cmd->handle( getAgent() );
}

void FourDScope2::handleFitWindowX()
{
    ENABLED_IF( true );
    d_plane.d_ol[0].d_iso->getBuf()->fitToDim( DimX );
    d_plane.d_viewer->update();
    if( !d_slices.empty() )
    {
        d_slices[ DimX ].d_buf2D->fitToArea();
        d_slices[ DimX ].d_viewer->update();
    }
}

void FourDScope2::handleFitWindowY()
{
    ENABLED_IF( true );
    d_plane.d_ol[0].d_iso->getBuf()->fitToDim( DimY );
    d_plane.d_viewer->update();
    if( !d_slices.empty() )
    {
        d_slices[ DimY ].d_buf2D->fitToArea();
        d_slices[ DimY ].d_viewer->update();
    }
}

void FourDScope2::handleGotoPlPeak()
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
        centerToCursor( p.size() > 2 );
    }catch( Root::Exception& e )
    {
        Root::ReportToUser::alert( this, "Goto Peak", e.what() );
    }
}

void FourDScope2::handleViewPlLabels()
{
    Gui2::UiFunction* a = Gui2::UiFunction::me();

    PeakPlaneView3::Label q = (PeakPlaneView3::Label) a->property( "0" ).toInt();
    if( q < PeakPlaneView3::NONE || q >= PeakPlaneView3::END )
        return;

    CHECKED_IF( true,
        d_plane.d_peaks->getLabel() == q );

    d_plane.d_peaks->setLabel( q );
    d_ortho.d_peaks->setLabel( q );
    d_ortho.d_viewer->update();
    d_plane.d_viewer->update();
}

void FourDScope2::handleSyncDepth()
{
    CHECKED_IF( true, d_syncDepth );

    d_syncDepth = !d_syncDepth;
    // Unterfeature von SyncCursor
}

void FourDScope2::handleAdjustIntensity()
{
    ENABLED_IF( true );

    // TODO Dlg::adjustIntensity( this, d_plane.d_intens );
}

void FourDScope2::updateCaption()
{
	QString str;
    str.sprintf( "FourDScope2 - %s", getSpec()->getName() );
    setCaption( str );
}

void FourDScope2::buildMenus()
{
    Gui2::AutoMenu* menuFile = new Gui2::AutoMenu( "&File", this );
    menuFile->addCommand( "Save", this, SLOT(handleSave()), tr( "CTRL+S" ) );
    menuFile->addCommand( "Print Preview...", this, SLOT(handleCreateReport()), tr("CTRL+P") );
    Gui2::AutoMenu* sub = new Gui2::AutoMenu( "Export", menuFile );
    sub->addCommand( "&Anchor Peaklist...", this, SLOT(handleExportPeaklist()) );
    sub->addCommand( "&Ortho Peaklist...", this, SLOT(handleExportStripPeaks()) );
    sub->addCommand( "Ortho peaks to &MonoScope...", this, SLOT(handleToMonoScope()) );
    sub->addCommand( "Plane peaks to &MonoScope...", this, SLOT(handleToMonoScope2D()) );
    menuFile->addMenu( sub );
    menuFile->addSeparator();
    menuFile->addCommand( "Close", this, SLOT(handleClose()), tr("CTRL+W") );

    Gui2::AutoMenu* menuEdit = new Gui2::AutoMenu( "&Edit", this );
    menuEdit->addCommand( "Undo", this, SLOT(handleUndo()), tr("CTRL+Z") );
    menuEdit->addCommand( "Redo", this, SLOT(handleRedo()), tr("CTRL+Y") );

    Gui2::AutoMenu* menuView = new Gui2::AutoMenu( "&View", this );
    menuView->addCommand( "Backward", this, SLOT(handleBackward()), Qt::CTRL+Qt::Key_B );
    menuView->addCommand( "Forward", this,SLOT(handleForward()) );

    menuView->addCommand( "Fit Window", this,SLOT(handleFitWindow()), Qt::CTRL+Qt::Key_Home );
    menuView->addCommand( "Goto System...", this,SLOT(handleGotoSystem()), Qt::CTRL+Qt::Key_G );
    menuView->addCommand( "Goto Point...", this,SLOT(handleGotoPoint()));
    menuView->addSeparator();
    menuView->addCommand( "Set Resolution...", this,SLOT(handleSetResolution()) );
    menuView->addCommand( "Show Low Resolution", this,SLOT(handleShowLowRes()) );
    menuView->addSeparator();
    menuView->addCommand( "Do Pathway Simulation", this,SLOT(handleForceCross()) );
    menuView->addCommand( "Show Ghost Peaks", this,SLOT(handleShowGhosts()) );
    menuView->addCommand( "Show Ghost Labels", this, SLOT(handleGhostLabels()) );
    menuView->addCommand( "Show Hidden Links", this,SLOT(handleShowAllPeaks()) );
    menuView->addSeparator();
    menuView->addCommand( "Hide 4D Slices", this, SLOT(handleAutoHide()) );
    menuView->addCommand( "Show Folded", this, SLOT(handleShowFolded()) );
    menuView->addCommand( "Sync to Global Zoom", this, SLOT(handleRangeSync()) );
    menuView->addCommand( "Sync to Global Cursor", this, SLOT(handleCursorSync()) );
    menuView->addCommand( "Sync to Depth", this,SLOT(handleSyncDepth()) );
    menuView->addCommand( "Center to Peak", this,SLOT(handleAutoCenter()) );
    menuView->addCommand( "Show List", this, SLOT(handleShowList()) );

    Gui2::AutoMenu* menuPicking = new Gui2::AutoMenu( "&Picking", this, false );
    menuPicking->addCommand( "&Pick New System", this, SLOT(handlePickSystem() ) );
    menuPicking->addCommand( "Propose System...", this, SLOT(handleProposePeak()) );
    menuPicking->addCommand( "Extend Horizontally", this,SLOT(handlePickHori()) );
    menuPicking->addCommand( "Propose Horizontally...", this,SLOT(handleProposeHori()) );
    menuPicking->addCommand( "Extend Vertically", this,SLOT(handlePickVerti()) );
    menuPicking->addCommand( "Propose Vertically...", this,SLOT(handleProposeVerti()) );
    menuPicking->addCommand( "&Move Spins", this,SLOT(handleMovePeak()) );
    menuPicking->addCommand( "Move Spin &Aliases", this,SLOT(handleMovePeakAlias()) );
    menuPicking->addCommand( "&Label Spins...", this,SLOT(handleLabelPeak()) );
    menuPicking->addCommand( "Hide/Show Link", this,SLOT(handleHidePeak()) );
    menuPicking->addCommand( "Set Link Params...", this,SLOT(handleSetLinkParams()) );
    menuPicking->addSeparator();
    menuPicking->addCommand( "Set Verti. Tolerance...", this,SLOT(handleSetTolVerti()) );
    menuPicking->addCommand( "Set Hori. Tolerance...", this,SLOT(handleSetTolHori()) );
    menuPicking->addSeparator();
    menuPicking->addCommand( "Un-Alias Peaks", this,SLOT(handleDeleteAliasPeak()) );
    menuPicking->addCommand( "Delete Peaks", this,SLOT(handleDeletePeak()) );
    menuPicking->addCommand( "Delete Spin Links", this,SLOT(handleDeleteLinks()) );
    menuPicking->addCommand( "Delete Horizontal Spin", this,SLOT(handleDeleteSpinX()) );
    menuPicking->addCommand( "Delete Vertical Spin", this,SLOT(handleDeleteSpinY()) );

    Gui2::AutoMenu* menuAssig = new Gui2::AutoMenu( "&Assignment", this, false );
    menuAssig->addCommand( "&Link Systems...", this, SLOT(handleLinkSystems()) );
    menuAssig->addCommand( "Unlink &Predecessor", this, SLOT(handleUnlinkPred()) );
    menuAssig->addCommand( "Unlink &Successor", this, SLOT(handleUnlinkSucc()) );
    menuAssig->addSeparator();
    menuAssig->addCommand( "&Assign System...", this, SLOT(handleAssign()) );
    menuAssig->addCommand( "&Unassign System", this, SLOT(handleUnassign()) );
    menuAssig->addCommand( "Set System &Type...", this,SLOT(handleSetSystemType()) );
    menuAssig->addCommand( "Set Assig. Candidates...", this,SLOT(handleSetCandidates()) );
    menuAssig->addCommand( "Show Ali&gnment...", this,SLOT(handleShowAlignment()) );

    Gui2::AutoMenu* menuRuler = new Gui2::AutoMenu( "&Ruler", this, false );
    menuRuler->addCommand( "Add &Horizontal Ruler", this,SLOT(handleAddRulerVerti()) );
    menuRuler->addCommand( "Add &Vertical Ruler", this,SLOT(handleAddRulerHori()) );
    menuRuler->addCommand( "Remove Selected Rulers", this,SLOT(handleRemoveRulers()));
    menuRuler->addCommand( "Remove All Rulers", this,SLOT(handleRemoveAllRulers()) );
    menuRuler->addCommand( "Set Rulers to Reference", this,SLOT(handleAutoRuler()) );

    Gui2::AutoMenu* menuAtts = new Gui2::AutoMenu( "&Edit Attributes", this, false );
    menuAtts->addCommand( "Horizontal Spin...", this,SLOT(handleEditAttsSpinH()) );
    menuAtts->addCommand( "Vertical Spin...", this,SLOT(handleEditAttsSpinV()) );
    menuAtts->addCommand( "Horizontal System...", this,SLOT(handleEditAttsSysH()) );
    menuAtts->addCommand( "Vertical System...", this,SLOT(handleEditAttsSysV()) );
    menuAtts->addCommand( "Spin Link...", this,SLOT(handleEditAttsLink()) );

    Gui2::AutoMenu* menuOverlay = new Gui2::AutoMenu( "&Overlay", this, false );
    menuOverlay->addCommand( "Set Layer &Count...", this,SLOT(handleOverlayCount()) );
    menuOverlay->addCommand( "A&dd Overlay Layer...", this,SLOT(handleAddLayer()) );
    menuOverlay->addCommand( "Compose &Layers...", this,SLOT(handleComposeLayers()) );
    menuOverlay->addCommand( "Set &Active Layer...", this,SLOT(handleActiveOverlay()) );
    menuOverlay->addCommand( "Select &Spectrum...", this,SLOT(handleOverlaySpec()) );
    menuOverlay->addCommand( "Set &Positive Color...", this,SLOT(handleSetPosColor()) );
    menuOverlay->addCommand( "Set &Negative Color...", this,SLOT(handleSetNegColor()) );
    menuOverlay->addCommand( "&Make Colors Persistent", this,SLOT(handleSaveColors()) );

    Gui2::AutoMenu* menuPlane = new Gui2::AutoMenu( "&Plane", this );
    menuPlane->addCommand( "&Calibrate", this,SLOT(handleSpecCalibrate()) );
    menuPlane->addMenu( d_popSpec2D );
    menuPlane->addMenu( menuOverlay );
    menuPlane->addSeparator();
    menuPlane->addCommand( "Show Contour", this,SLOT(handleShowContour()) );
    menuPlane->addCommand( "Show Intensity", this,SLOT(handleShowIntensity()) );
    menuPlane->addCommand( "Auto Contour Level", this,SLOT(handleAutoContour()) );
    menuPlane->addCommand( "Set Contour Parameters...", this,SLOT(handleContourParams() ) );
    menuPlane->addSeparator();
    menuPlane->addMenu( menuPicking );
    menuPlane->addMenu( menuAssig );
    menuPlane->addMenu( menuRuler );
    menuPlane->addMenu( menuAtts );
    menuPlane->addSeparator();
    menuPlane->addCommand( "Hold Reference", this,SLOT(handleHoldReference()) );
    menuPlane->addCommand( "Auto Hold", this,SLOT(handleAutoHold()) );
    menuPlane->addSeparator();
    menuPlane->addCommand( "Show 4D Plane", this,SLOT(handleShow4dPlane()), Qt::CTRL+Qt::Key_3 );
    menuPlane->addCommand( "Show Spin Links", this,SLOT(handleShowLinks()) );
    menuPlane->addCommand( "Show Infered Peaks", this,SLOT(handleShowInfered()) );
    menuPlane->addCommand( "Show Unlabeled Peaks", this,SLOT(handleShowUnlabeled()) );
    menuPlane->addCommand( "Show Peaks with unknown Labels", this,SLOT(handleShowUnknown()) );
    menuPlane->addCommand( "Resolve Projected Spins", this,SLOT(handleShowWithOff2()) );
    menuPlane->addCommand( "Use Link Color Codes", this,SLOT(handleUseLinkColors()) );
    sub = new Gui2::AutoMenu( "Show Labels", this, false );
    menuPlane->addMenu( sub );
	short i;
	for( i = SpinPointView::None; i < SpinPointView::End; i++ )
	{
        sub->addCommand( SpinPointView::menuText[ i ], this, SLOT(handleViewLabels() ) )->setProperty("0", i );
	}

    Gui2::AutoMenu* menuStrip = new Gui2::AutoMenu( "&Orthogonal", this );
    menuStrip->addCommand( "Calibrate Orthogonal",this,SLOT(handleStripCalibrate()) );
    menuStrip->addCommand( "Set Tolerance...", this, SLOT(handleSetTolStrip()) );
    menuStrip->addSeparator();
    menuStrip->addCommand( "Auto Contour Level",this,SLOT(handleAutoContour2()) );
    menuStrip->addCommand( "Set Contour Parameters...",this,SLOT(handleContourParams2()) );

    menuStrip->addMenu( d_popOrtho );
    menuStrip->addSeparator();
    menuStrip->addCommand( "&Pick Spins",this,SLOT(handlePickSpin4D()) );
    menuStrip->addCommand( "Pick Labels...",this,SLOT(handlePickLabel4D()) );
    menuStrip->addCommand( "Propose &Spins...",this,SLOT(handleProposeSpin()) );
    menuStrip->addCommand( "&Move Spins",this,SLOT(handleMoveSpin4D()) );
    menuStrip->addCommand( "&Move Spin Aliasses",this,SLOT(handleMoveSpinAlias4D()) );

    menuStrip->addCommand( "&Label Spins...",this,SLOT(handleLabelSpin4D()) );
    menuStrip->addCommand( "Hide/Show Link",this,SLOT(handleHidePeak4D()) );
    menuStrip->addCommand( "Set Link Params...",this,SLOT(handleSetLinkParams4D()) );

    menuStrip->addCommand( "&Delete Spins",this,SLOT(handleDeleteSpins4D()) );
    menuStrip->addCommand( "Delete Spin &Links",this,SLOT(handleDeleteLinks4D()) );

    menuAtts = new Gui2::AutoMenu( "&Edit Attributes", this, false );
    menuAtts->addCommand( "Horizontal Spin...",this,SLOT(handleEditAttsSpinX4D()) );
    menuAtts->addCommand( "Vertical Spin...",this,SLOT(handleEditAttsSpinY4D()) );
    menuAtts->addCommand( "System...",this,SLOT(handleEditAttsSys4D()) );
    menuAtts->addCommand( "Spin Link...",this,SLOT(handleEditAttsLink4D()) );
    menuStrip->addMenu( menuAtts );
    menuStrip->addSeparator();
    menuStrip->addCommand( "Show Spin Links",this,SLOT(handleShowLinks2()) );
    menuStrip->addCommand( "Show Infered Peaks",this,SLOT(handleShowInfered2()) );
    menuStrip->addCommand( "Show Unlabeled Peaks",this,SLOT(handleShowUnlabeled2()) );
    menuStrip->addCommand( "Show Peaks with unknown Labels",this,SLOT(handleShowUnknown2()) );
    menuStrip->addCommand( "Resolve Projected Spins",this,SLOT(handleShowWithOff()) );
    menuStrip->addCommand( "Use Link Color Codes",this,SLOT(handleUseLinkColors4D()) );
    menuStrip->addCommand( "Fit Window",this,SLOT(handleFitWindow4D()), Qt::Key_Home );
    sub = new Gui2::AutoMenu( this );
	menuStrip->insertItem( "Show Labels", sub );
	for( i = SpinPointView::None; i < SpinPointView::End; i++ )
	{
        sub->addCommand( SpinPointView::menuText[ i ],this, SLOT(handleViewLabels4D()) )->setProperty("0", i );
	}

    Gui2::AutoMenu* menuPeaks = new Gui2::AutoMenu( "&Peaks", this );
    menuPeaks->addCommand( "&New Peaklist",this,SLOT(handleNewPeakList()) );
    menuPeaks->addCommand( "&Open Peaklist...",this,SLOT(handleOpenPeakList()) );
    menuPeaks->addCommand( "&Add to Repository...",this,SLOT(handleSavePeakList()) );
    menuPeaks->addCommand( "Map to Spectrum...",this,SLOT(handleMapPeakList()) );
    menuPeaks->addCommand( "Select Color...",this,SLOT(handleSetPlColor()) );
    sub = new Gui2::AutoMenu( "Show Labels", this, false );
    menuPeaks->addMenu( sub );
	for( i = PeakPlaneView::NONE; i < PeakPlaneView::END; i++ )
	{
        sub->addCommand( PeakPlaneView::menuText[ i ], this,
                         SLOT(handleViewPlLabels()) )->setProperty("0", i );
	}
    menuPeaks->addSeparator();
    menuPeaks->addCommand( "&Pick Peak",this,SLOT(handlePickPlPeak()) );
    menuPeaks->addCommand( "&Move Peak",this,SLOT(handleMovePlPeak()) );
    menuPeaks->addCommand( "&Move Peak Alias",this,SLOT(handleMovePlAlias()) );
    menuPeaks->addCommand( "&Label Peak...",this,SLOT(handleLabelPlPeak()) );
    menuPeaks->addCommand( "&Delete Peaks",this,SLOT(handleDeletePlPeaks()) );
    menuPeaks->addCommand( "&Edit Attributes...",this,SLOT(handleEditPlPeakAtts()) );

    Gui2::AutoMenu* menuHelp = new Gui2::AutoMenu( "&?", this );
    menuHelp->addCommand( "About...", this, SLOT(handleHelpAbout()) );

}

void FourDScope2::buildViews()
{
    QSplitter* split = new QSplitter( Qt::Horizontal, this );
    split->setOpaqueResize(false);
    setCentralWidget( split );

    d_planeGrid = new Gui2::SplitGrid2( this, 2, 2 );
    d_planeGrid->setCell( d_plane.d_viewer, 0, 1 );
    d_planeGrid->setCell( d_slices[ DimX ].d_viewer, 1, 1 );
    d_planeGrid->setCell( d_slices[ DimY ].d_viewer, 0, 0 );
    //d_planeGrid->setColStretchFactors( QList<int>() << 1 << 6 );
    //d_planeGrid->setRowStretchFactors( QList<int>() << 4 << 1 );

    d_orthoGrid = new Gui2::SplitGrid2( this, 2, 2 );
    d_orthoGrid->setCell( d_ortho.d_viewer, 0, 1 );
    d_orthoGrid->setCell( d_slices[ DimW ].d_viewer, 1, 1 );
    d_orthoGrid->setCell( d_slices[ DimZ ].d_viewer, 0, 0 );
    d_orthoGrid->setCell( new SpecViewer3(this), 1, 0 );

    split->addWidget( d_planeGrid );
    split->addWidget( d_orthoGrid );
    split->setHandleWidth( d_planeGrid->handleWidth() );

    QDockWidget* dock = new QDockWidget( tr("Strip List"), this );
    dock->setAllowedAreas( Qt::AllDockWidgetAreas );
    dock->setFeatures( QDockWidget::AllDockWidgetFeatures );
    dock->setFloating( false );
    dock->setVisible( d_showList );
    addDockWidget( Qt::RightDockWidgetArea, dock );

    d_list = new StripListGadget2( dock, getAgent(), d_pro, true );
    dock->setWidget( d_list );
    connect( d_list,SIGNAL(returnPressed(Gui::ListViewItem*)),this,SLOT(onListActivate()) );
    connect( d_list,SIGNAL(doubleClicked(Gui::ListViewItem*)), this, SLOT(onListActivate()) );

    Gui2::AutoMenu* menu = new Gui2::AutoMenu( d_list, true );
    menu->addCommand( "Goto...", this, SLOT(handleGoto()) );
    menu->addCommand( "Use 4D Navigation", this, SLOT(handleGoto4D()) );
    menu->addCommand( "Show 4D Values", this, SLOT(handleUse4DSpec()) );
    menu->addSeparator();
    d_list->addCommands( menu );

    if( d_use4D )
        d_list->setSpec( d_spec4D );
    else
        d_list->setSpec( d_spec2D );

    SpecViewer3* ov = new SpecViewer3( this );
    ov->installEventFilter( d_cl );
    ov->setArea( new ViewAreaMdl3( true, true, true, true ) );
    Ref<SpecBufferMdl3> mdl = new SpecBufferMdl3( ov->getArea() );
    ov->addView( new IntensityView3( ov->getArea(), mdl ) );
    d_ovCtrl = new OverviewCtrl3( mdl, d_plane.d_viewer->getArea() );
    ov->addView( d_ovCtrl );
    d_planeGrid->setCell( ov, 1, 0 );
}

Root::Ref<PointSet> FourDScope2::createStripPeaks()
{
    Spectrum* spec = d_spec4D;
	PpmPoint p( 0, 0, 0, 0 );
	PointSet::Assig a;
	Root::Index id;
	Root::Ref<PointSet> ps = Factory::createEasyPeakList( spec );

    LinkFilterRotSpace* mdl = d_ortho.d_rot;
    LinkFilterRotSpace::Iterator i, _end = mdl->end();
    LinkFilterRotSpace::Element e;
    SpinPointView3::LabelType l = d_ortho.d_points->getLabelType();
	SpinLink* link;
	for( i = mdl->begin(); i != _end; ++i )
	{
		mdl->fetch( i, e );
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

bool FourDScope2::askToCloseWindow() const
{
    return const_cast<FourDScope2*>(this)->askToClosePeaklist();
}

void FourDScope2::handle(Root::Message& msg)
{
    if( d_lock )
        return;
    BEGIN_HANDLER();
    MESSAGE( ViewAreaMdl3::UpdRange, a, msg )
    {
        Locker l(d_lock);
        if( a->getOrigin() == d_plane.d_viewer->getArea() )
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
            if( hasY )
                show4dPlaneSlice( DimX, false ); // der andere
            if( hasX )
                show4dPlaneSlice( DimY, false ); // der andere
            selectCurSystem();
        }
        if( hasW || hasZ )
        {
            show4dPlaneSlice( DimX, true );
            show4dPlaneSlice( DimY, true );
            if( d_show4DPlane )
                updateContour( 0, true );
        }
        notifyCursor( hasX || hasY );
        updateGlobalCursor( a );
        msg.consume();
    }
    MESSAGE( CubeMdl::Updated, a, msg )
    {
        updateGlobalCursor( a );
        msg.consume();
    }
    MESSAGE( GlobalCursor::UpdatePos, a, msg )
    {
        d_cursorSync = false;
        bool fourD = false;
        if( ( a->getDim() == DimY || a->getDim() == DimUndefined ) &&
            d_plane.d_ol[0].d_iso->getBuf()->getSpectrum()->getColor( DimY ) == a->getTy() )
            d_pointMdl->setPos( DimY, a->getY() );
        if( ( a->getDim() == DimX || a->getDim() == DimUndefined ) &&
            d_plane.d_ol[0].d_iso->getBuf()->getSpectrum()->getColor( DimX ) == a->getTx() )
            d_pointMdl->setPos( DimX, a->getX() );
        if( d_syncDepth && ( a->getDim() == DimY || a->getDim() == DimUndefined ) && d_spec4D &&
            d_spec4D->getColor( DimZ ) == a->getTy() )
        {
            d_pointMdl->setPos( DimZ, a->getY() );
            fourD = true;
        }
        if( d_syncDepth && ( a->getDim() == DimX || a->getDim() == DimUndefined ) && d_spec4D &&
            d_spec4D->getColor( DimW ) == a->getTx() )
        {
            d_pointMdl->setPos( DimW, a->getX() );
            fourD = true;
        }
        d_cursorSync = true;
        if( !fourD )
        {
            selectCurSystem();
            if( !d_rangeSync )
                centerToCursor( fourD );
        }else
            centerToCursor( fourD );
        msg.consume();
    }
    MESSAGE( GlobalCursor::UpdateRange, a, msg )
    {
        d_rangeSync = false;
        if( ( a->getDim() == DimY || a->getDim() == DimUndefined ) &&
            d_plane.d_ol[0].d_iso->getBuf()->getSpectrum()->getColor( DimY ) == a->getTy() )
            d_plane.d_viewer->getArea()->setRange( DimY, a->getY() );
        if( ( a->getDim() == DimX || a->getDim() == DimUndefined ) &&
            d_plane.d_ol[0].d_iso->getBuf()->getSpectrum()->getColor( DimX ) == a->getTx() )
            d_plane.d_viewer->getArea()->setRange( DimX, a->getX() );
        d_rangeSync = true;
        msg.consume();
    }
    MESSAGE( Project::Changed, a, msg )
    {
        Locker l(d_lock);
        msg.consume();
        if( a->d_hint == Project::Width || a->d_hint == Project::WidthFactor  )
        {
            if( d_spec4D )
            {
                d_plane.d_mdlND->setGhostWidth( d_pro->inferPeakWidth( DimZ, d_spec4D ),
                                            d_pro->inferPeakWidth( DimW, d_spec4D ) );
                const PPM wx = d_pro->inferPeakWidth( DimX, d_spec4D );
                const PPM wy = d_pro->inferPeakWidth( DimY, d_spec4D );
                d_ortho.d_range->setGhostWidth( wx, wy );
                d_ortho.d_peakMdl->setGhostWidth( wx, wy );
            }
            if( d_show4DPlane )
                d_plane.d_peakMdl->setGhostWidth( d_pro->inferPeakWidth( DimZ, d_spec4D ),
                                        d_pro->inferPeakWidth( DimW, d_spec4D ) );
            selectCurSystem();
        }
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

void FourDScope2::handleShowList()
{
    CHECKED_IF( true, d_showList );

	d_showList = !d_showList;
    d_list->parentWidget()->setVisible( d_showList );
}

void FourDScope2::handleSetTolerance()
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

void FourDScope2::handleExportPeaklist()
{
    ENABLED_IF( true );

    QString fileName = QFileDialog::getSaveFileName( this, "Export 2D Pealist",
                                                     Root::AppAgent::getCurrentDir(), "*.peaks" );
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

Root::Ref<PointSet> FourDScope2::createPlanePeaks()
{
    Spectrum* spec = getSpec();
	PpmPoint p( 0, 0 );
	PointSet::Assig a;
	Root::Index id;
	Root::Ref<PointSet> ps = Factory::createEasyPeakList( spec );
    LinkFilterRotSpace* mdl = d_plane.d_mdl2D;
    LinkFilterRotSpace::Iterator i, _end = mdl->end();
    LinkFilterRotSpace::Element e;
    SpinPointView3::LabelType l = d_plane.d_points->getLabelType();
	SpinLink* link;
	for( i = mdl->begin(); i != _end; ++i )
	{
		mdl->fetch( i, e );
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

void FourDScope2::handleSetTolHori()
{
    ENABLED_IF( true );

	AtomType atom;
    atom = getSpec()->getColor( DimX );

	bool ok	= FALSE;
	QString res;
	QString str;
	str.sprintf( "Please enter atom positive PPM value for %s:", atom.getIsoLabel() );
    res.sprintf( "%0.3f", d_pro->getMatcher()->getTol( atom ) );
    res	= QInputDialog::getText( "Set Matching Tolerance", str, QLineEdit::Normal, res, &ok, this );
	if( !ok )
		return;
	PPM w = res.toFloat( &ok );
	if( !ok ) // RK 31.10.03: w <= 0 neu erlaubt
	{
        QMessageBox::critical( this, "Set Spin Tolerance", "Invalid tolerance!", "&Cancel" );
		return;
	}

    d_pro->getMatcher()->setTol( atom, w );
}

void FourDScope2::handleSetTolVerti()
{
    ENABLED_IF( true );

	AtomType atom;
    atom = getSpec()->getColor( DimY );

	bool ok	= FALSE;
	QString res;
    res.sprintf( "%0.3f", d_pro->getMatcher()->getTol( atom ) );
	QString str;
	str.sprintf( "Please enter atom positive PPM value for %s:", atom.getIsoLabel() );
    res	= QInputDialog::getText( "Set Vertical Spin Tolerance", str, QLineEdit::Normal, res, &ok, this );
	if( !ok )
		return;
	PPM w = res.toFloat( &ok );
	if( !ok ) // RK 31.10.03: w <= 0 neu erlaubt
	{
        QMessageBox::critical( this, "Set Spin Tolerance", "Invalid tolerance!", "&Cancel" );
		return;
	}

    d_pro->getMatcher()->setTol( atom, w );
}

void FourDScope2::handleSetTolStrip()
{
    ENABLED_IF( d_spec4D );

	AtomType atom;
    atom = d_spec4D->getColor( DimZ );

	bool ok	= FALSE;
	QString res;
    res.sprintf( "%0.3f", d_pro->getMatcher()->getTol( atom ) );
	QString str;
	str.sprintf( "Please enter atom positive PPM value for %s:", atom.getIsoLabel() );
    res	= QInputDialog::getText( "Set Orthogonal Tolerance", str, QLineEdit::Normal, res, &ok, this );
	if( !ok )
		return;
	PPM w = res.toFloat( &ok );
	if( !ok || w < 0.0 )
	{
        QMessageBox::critical( this, "Set Orthogonal Tolerance", "Invalid tolerance!", "&Cancel" );
		return;
	}

    d_pro->getMatcher()->setTol( atom, w );
}

void FourDScope2::handleExportStripPeaks()
{
    Spectrum* spec = d_spec4D;
    ENABLED_IF( spec );

    QString fileName = QFileDialog::getSaveFileName( this, "Export 4D Pealist",
                                                     Root::AppAgent::getCurrentDir(), "*.peaks" );
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

void FourDScope2::handleToMonoScope()
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
		pl->setName( "Export from FourDScope2" );
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

void FourDScope2::handleGoto()
{
    ENABLED_IF( d_list );
    gotoTuple( d_list->getSelectedStrip(), d_list->getSelectedSpin(),
		d_list->getSelectedLink(), !d_goto4D );
    d_plane.d_viewer->setFocus();
}

void FourDScope2::handleSyncHori()
{
    Spin* spin = getSel( true );
    ENABLED_IF( d_list && spin );
	d_list->gotoSpin( spin );
}

void FourDScope2::handleSyncVerti()
{
    Spin* spin = getSel( false );
    ENABLED_IF( d_list && spin );
	d_list->gotoSpin( spin );
}

void FourDScope2::handleGotoResidue()
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

void FourDScope2::handleSetWidthFactor()
{
    ENABLED_IF( true );

    Gui2::UiFunction* a = Gui2::UiFunction::me();
    double g;
    if( a->property( "0" ).isNull() )
	{
		bool ok	= FALSE;
		QString res;
        res.sprintf( "%0.2f", d_pro->getWidthFactor() );
		res	= QInputDialog::getText( "Set Orthogonal Width Factor",
            "Please	enter a positive factor:", QLineEdit::Normal, res, &ok, this );
		if( !ok )
			return;
		g = res.toFloat( &ok );
	}else
        g = a->property( "0" ).toDouble();
	if( g <= 0.0 )
	{
		Root::ReportToUser::alert( this, "Set Width Factor", "Invalid Factor Value" );
		return;
	}
    d_pro->setWidthFactor( g );
}

void FourDScope2::handleUse4DSpec()
{
    CHECKED_IF( true, d_use4D );
	d_use4D = !d_use4D;
	if( d_use4D )
        d_list->setSpec( d_spec4D );
	else
        d_list->setSpec( d_spec2D );
}

void FourDScope2::handleGoto4D()
{
    CHECKED_IF( true, d_goto4D );
	d_goto4D = !d_goto4D;
}

void FourDScope2::handleSaveColors()
{
    ENABLED_IF( true );
    Repository::SlotColors clr( d_plane.d_ol.size() );
	for( int i = 0; i < clr.size(); i++ )
	{
        clr[ i ].d_pos = d_plane.d_ol[i].d_iso->getPosColor();
        clr[ i ].d_neg = d_plane.d_ol[i].d_iso->getNegColor();
	}
    d_pro->getRepository()->setScreenClr( clr );
}

void FourDScope2::handleToMonoScope2D()
{
    Spectrum* spec = getSpec();
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
