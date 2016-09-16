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

#include "PolyScope3.h"
// Qt
#include <qtextstream.h> 
#include <qinputdialog.h> 
#include <qmessagebox.h>
#include <QFileDialog>
#include <qfileinfo.h> 
#include <qdir.h> 
#include <qmenubar.h>
#include <QColorDialog>
#include <QDockWidget>

//* Root
#include <Root/ActionHandler.h>
#include <Root/Command.h>
#include <Root/Application.h>
#include <Root/UpstreamFilter.h>
using namespace Root;

//* Lexi
#include <Lexi/LayoutKit.h>
#include <QColor>
#include <Lexi/CommandLine.h>
#include <Lexi/Splitter.h>
#include <Lexi/Redirector.h>
#include <Lexi/Background.h>
#include <Lexi/Bevel.h>
#include <Lexi/Interactor.h>
#include <Lexi/ContextMenu.h>
#include <Lexi/Printer.h>
#include <Lexi/Shapes.h>
#include <Lexi/Label.h>
using namespace Lexi;

//* Spec
#include <Spec/Factory.h>
#include <Spec/SpecProjector.h>
#include <Spec/SpectrumPeer.h>
#include <Spec/SpectrumType.h>
#include <Spec/Repository.h>
#include <Spec/GlobalCursor.h>
#include <Spec/SpecRotator.h>
#include <Spec/SpinPointSpace.h>
#include <Spec/PeakListPeer.h>
#include <Spec/FragmentAssignment.h>

// SpecView
#include <SpecView/SpecViewer.h>
#include <SpecView/FocusCtrl.h>
#include <SpecView/ContourView.h>
#include <SpecView/IntensityView.h>
#include <SpecView/CursorView.h>
#include <SpecView/CursorCtrl.h>
#include <SpecView/ScrollCtrl.h>
#include <SpecView/ZoomCtrl.h>
#include <SpecView/DynValueEditor.h>
#include <SpecView/SelectZoomCtrl.h>
#include <SpecView/SliceView.h>
#include <SpecView/OverviewCtrl.h>
#include <SpecView/PeakPlaneView.h>
#include <SpecView/FoldingView.h>
#include <SpecView/PointSelectCtrl.h>
#include <SpecView/PeakSelectCtrl.h>
#include <SpecView/CenterLine.h>
#include <SpecView/SelectRulerCtr.h>
#include <SpecView/CandidateDlg.h>
#include <SpecView/SpecBatchList.h>
#include <SpecView/GotoDlg.h>
#include <SpecView/RotateDlg.h>
#include <SpecView3/CommandLine2.h>

#include "PolyScope3.h"
#include "Dlg.h"
#include "StripListGadget2.h"
#include "ProposeSpinDlg.h"
#include "SingleAlignmentView.h"
#include "ReportViewer.h"

using namespace Spec;

//////////////////////////////////////////////////////////////////////
// RISK: Dirty Trick

void createPolyScope3(Root::Agent* a, Spec::Spectrum* s, Spec::Project* p)
{ 
	new PolyScope3( a, s, p );
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
static const float s_gain = 2.0;

static QColor g_clrLabel = Qt::yellow;
static QColor g_clrSlice3D = Qt::cyan;
static QColor g_clrPeak = Qt::yellow;
static QColor g_frameClr = Qt::lightGray;

static const Dimension s_defDim = 2;

//////////////////////////////////////////////////////////////////////


PolyScope3::PolyScope3(Root::Agent * a, Spectrum* spec, Project* pro ):
    GenericScope( a ), d_ovCtrl( 0 ),
    d_use3D( true ), d_goto3D( false ), d_lock( false ), d_autoRuler( false ), d_aol( 0 ),
    d_popLabel( 0 ), d_popSpec2D( 0 ), d_pl(0), d_popSpec3D( 0 ), d_popStrip(0), d_popPlane(0)
{	
	assert( spec && spec->getDimCount() >= 2 && spec->getDimCount() <= 3 );
	assert( pro );
	d_pro = pro;
	d_pro->addObserver( this );

	d_spec2D = spec;
	d_orig = spec;

    d_src2D = new SpinPointSpace( pro->getSpins(),
                                  pro->getRepository()->getTypes(), 0,
                                  true, true, false );
    d_src2D->setSpec( spec );
    d_src3D = new SpinPointSpace( pro->getSpins(), pro->getRepository()->getTypes(), 0,
                                  true, true, false );

    d_mdl2D = new LinkFilterRotSpace( d_src2D, spec );
	d_mdl3D = new LinkFilterRotSpace( d_src3D );

	initParams();
    buildPopup();
    initViews( false );
	if( d_spec2D->getDimCount() > 2 )
	{
		setSpec3D( d_spec2D );
		d_cursor[ DimZ ] = d_spec2D->getScale( DimZ ).getRange().first;
		d_slices[ DimZ ].d_cur->setCursor( (Dimension)DimY, d_cursor[ DimZ ] );
	}
	updateSpecPop2D();
	updateSpecPop3D();

    Root::UpstreamFilter* filter = new UpstreamFilter( getAgent(), true );

	d_focus = new FocusManager( nil, true );    
    d_cl = new CommandLine2( this );
    installEventFilter( d_cl );
    buildCommands();

    d_widget = new GlyphWidget( getQt(), new Bevel( new Background( d_focus, Qt::black, true ), true, false ), filter, true );
	getQt()->setCentralWidget( d_widget );
    d_widget->setFocusGlyph( d_focus );
    d_widget->installEventFilter( d_cl );

	buildMenus();
	getQt()->resize( 600, 400 ); // RISK
	getQt()->showMaximized();
	updateCaption();
	buildViews();
}

PolyScope3::~PolyScope3()
{
    d_widget->getViewport()->setParent(0); // damit Imp nicht zweimal gelöscht wird, sondern nur von GlyphWidget

	d_pro->removeObserver( this );
	d_pro = 0;
	
	if( d_popSpec2D )
		delete d_popSpec2D;
	if( d_popSpec3D )
		delete d_popSpec3D;
	if( d_popLabel )
		delete d_popLabel;
	if( d_popPlane )
		delete d_popPlane;
	if( d_popStrip )
		delete d_popStrip;

	GlobalCursor::removeObserver( this );
}

void PolyScope3::updateCaption()
{
	QString str; // TODO
    str.sprintf( "PolyScope - %s", getSpec()->getName() );
	getQt()->setCaption( str );
}

void PolyScope3::buildCommands()
{
    d_cl->addCommand( this, SLOT( handleAdjustIntensity() ), "CW", "Adjust Intensity", true );
    d_cl->addCommand( this, SLOT( handlePickPlPeak() ), "PP", "Pick Peak", true );
    d_cl->addCommand( this, SLOT( handleMovePlPeak() ), "MP", "Move Peak", true );
    d_cl->addCommand( this, SLOT( handleMovePlAlias() ), "MA", "Move Peak Alias", true );
    d_cl->addCommand( this, SLOT( handleDeletePlPeaks() ), "DP", "Delete Peaks", true );
    d_cl->addCommand( this, SLOT( handleLabelPlPeak() ), "LP", "Label Peak", true );

    d_cl->addCommand( this, SLOT( handleDeleteAliasPeak() ), "UA", "Un-Alias Spins", true );
    d_cl->addCommand( this, SLOT( handleGotoPoint() ), "GT", "Goto", true );
    d_cl->addCommand( this, SLOT( handlePickSystem() ), "PN", "Pick New System", true );
    d_cl->addCommand( this, SLOT( handleSetLinkParams() ), "PA", "Set Link Params", true );
    d_cl->addCommand( this, SLOT( handleSetLinkParams3D() ), "ZPA", "Set Link Params", true );
    d_cl->addCommand( this, SLOT( handleSetSystemType() ), "SY", "Set System Type", true );
    d_cl->addCommand( this, SLOT( handleSyncHori() ), "SH", "Sync. Hori. Spin", true );
    d_cl->addCommand( this, SLOT( handleSyncVerti() ), "SV", "Sync. Verti. Spin", true );
    d_cl->addCommand( this, SLOT( handleRemoveRulers() ), "RS", "Remove Selected Rulers", true );
    d_cl->addCommand( this, SLOT( handleRemoveAllRulers() ), "RA", "Remove All Rulers", true );
    d_cl->addCommand( this, SLOT( handleAddRulerVerti() ), "RH", "Add Horizontal Ruler", true );
    d_cl->addCommand( this, SLOT( handleAddRulerHori() ), "RV", "Add Vertical Ruler", true );
    d_cl->addCommand( this, SLOT( handleHoldReference() ), "HR", "Hold Reference", true );
    d_cl->addCommand( this, SLOT( handleForwardPlane() ), "FP", "Forward Plane", true );
    d_cl->addCommand( this, SLOT( handleBackwardPlane() ), "BP", "Backward Plane", true );
    d_cl->addCommand( this, SLOT( handleShowFolded() ), "SF", "Show Folded", true );
    d_cl->addCommand( this, SLOT( handleAutoContour2() ), "ZAC", "Auto Contour Level", true );
    d_cl->addCommand( this, SLOT( handleAutoContour() ), "AC", "Auto Contour Level", true );
    d_cl->addCommand( this, SLOT( handleShow3dPlane() ), "3D", "Show 3D Plane", true );
    d_cl->addCommand( this, SLOT( handleFitWindow3D() ), "ZWW", "Fit To Window", true );
    d_cl->addCommand( this, SLOT( handleBackward() ), "BB", "Backward", true );
    d_cl->addCommand( this, SLOT( handleShowList() ), "SL", "Show List", true );
    d_cl->addCommand( this, SLOT( handleCursorSync() ), "GC", "Sync To Global Cursor", true );
    d_cl->addCommand( this, SLOT( handleFitWindowX() ), "WX", "Fit X To Window", true );
    d_cl->addCommand( this, SLOT( handleFitWindowY() ), "WY", "Fit Y To Window", true );
    d_cl->addCommand( this, SLOT( handleFitWindow() ), "WW", "Fit To Window", true );
    d_cl->addCommand( this, SLOT( handleAutoCenter() ), "CC", "Center To Peak", true );
    d_cl->addCommand( this, SLOT( handleUndo() ), "ZZ", "Undo", true );
    d_cl->addCommand( this, SLOT( handleRedo() ), "YY", "Redo", true );
    d_cl->addCommand( this, SLOT( handleShowContour() ), "SC", "Show Contour", true );
    d_cl->addCommand( this, SLOT( handleShowIntensity() ), "SI", "Show Intensity", true );
    d_cl->addCommand( this, SLOT( handleShowLowRes() ), "LR", "Low Resolution", true ); // TODO: Argument
    d_cl->addCommand( this, SLOT( handleContourParams() ), "CP", "Contour Params", true );
    d_cl->addCommand( this, SLOT( handleContourParams2() ), "ZCP", "Contour Params", true );

    CommandLine2::Command* cmd = d_cl->addCommand( this, SLOT(handleSetTolerance()), "ST", "Set Tolerance", false );
    cmd->registerParameter( Any::CStr, true, "Atom Type" );
    cmd->registerParameter( Any::Float, true, "Tolerance (ppm)" );

    cmd = d_cl->addCommand( this, SLOT( handleLinkSystems() ), "LK", "Link Systems", false );
    cmd->registerParameter( Any::ULong, false, "Predecessor" ); // 1..n Pred, Succ oder Nil
    cmd->registerParameter( Any::ULong, false, "Successor" ); // 1..n Succ oder Nil

    cmd = d_cl->addCommand( this, SLOT( handleViewLabels() ), "LL", "Label Format", false );
    cmd->registerParameter( Any::Long, false ); // Leer oder ID

    cmd = d_cl->addCommand( this, SLOT( handleViewPlLabels() ), "LF", "Label Format", false );
    cmd->registerParameter( Any::Long, false ); // Leer oder ID

    cmd = d_cl->addCommand( this, SLOT( handleViewLabels3D() ), "ZLL", "Label Format", false );
    cmd->registerParameter( Any::Long, false ); // Leer oder ID

    d_cl->addCommand( this, SLOT( handlePickSystem() ), "PY", "Pick System", true );
    d_cl->addCommand( this, SLOT( handleProposePeak() ), "PR", "Propose Spins", true );
    d_cl->addCommand( this, SLOT( handleMovePeak() ), "MS", "Move Spins", true );
    d_cl->addCommand( this, SLOT( handleMovePeakAlias() ), "AY", "Move System Alias", true );
    d_cl->addCommand( this, SLOT( handlePickHori() ),  "EH", "Extend Horizontally", true );
    d_cl->addCommand( this, SLOT( handleProposeHori() ), "PH", "Propose Horizontally", true );
    d_cl->addCommand( this, SLOT( handlePickVerti() ), "EV", "Extend Vertically", true );
    d_cl->addCommand( this, SLOT( handleProposeVerti() ), "PV", "Propose Vertically", true );
    d_cl->addCommand( this, SLOT( handleLabelPeak() ), "LS", "Label Spins", true );
    d_cl->addCommand( this, SLOT( handleDeletePeak() ), "DS", "Delete Spins", true );
    d_cl->addCommand( this, SLOT( handleDeleteLinks() ), "DL", "Delete Spin Links", true );

    d_cl->addCommand( this, SLOT( handlePickSpin3D() ), "PI", "Pick Spin", true );
    d_cl->addCommand( this, SLOT( handleProposeSpin() ), "RI", "Propose Spin", true );
    d_cl->addCommand( this, SLOT( handleMoveSpin3D() ), "MI", "Move Spin", true );
    d_cl->addCommand( this, SLOT( handleMoveSpinAlias3D() ), "AI", "Move Spin Alias", true );

    d_cl->addCommand( this, SLOT( handleShowWithOff2() ), "RP", "Resolve Projected Spins", true );

    cmd = d_cl->addCommand( this, SLOT( handleForceLabelSpin3D() ), "LI", "Force Spin Label", false );
    cmd->registerParameter( Any::CStr, false ); // Label oder leer

    cmd = d_cl->addCommand( this, SLOT( handleGotoSystem() ), "GY", "Goto System", false );
    cmd->registerParameter( Any::CStr, false ); // Leer oder ID

    cmd = d_cl->addCommand( this, SLOT( handleGotoPeak() ), "GS", "Goto Spins", false );
    cmd->registerParameter( Any::CStr, false ); // Leer oder ID

    cmd = d_cl->addCommand( this, SLOT( handleGotoPlPeak() ), "GP", "Goto Peak", false );
    cmd->registerParameter( Any::Long, false ); // Leer oder ID

    cmd = d_cl->addCommand( this, SLOT( handleOverlayCount() ), "OC", "Overlay Count", false );
    cmd->registerParameter( Any::Long, false ); // Leer oder Count

    cmd = d_cl->addCommand( this, SLOT( handleActiveOverlay() ), "AO", "Active Overlay", false );
    cmd->registerParameter( Any::Long, false ); // Leer oder ID

    cmd = d_cl->addCommand( this, SLOT( handleOverlaySpec() ), "YS", "Overlay Spectrum", false );
    cmd->registerParameter( Any::Long, false ); // Leer oder ID

    d_cl->addCommand( this, SLOT( handleSetPosColor() ), "PC", "Set Positive Color", true );
    d_cl->addCommand( this, SLOT( handleSetNegColor() ), "NC", "Set Negative Color", true );
    d_cl->addCommand( this, SLOT( handleComposeLayers() ), "CL", "Compose Layers", true );

    cmd = d_cl->addCommand( this, SLOT( handleAddLayer() ), "AL", "Add Overlay Layer", false );
    cmd->registerParameter( Any::Long, false ); // Leer oder ID

    cmd = d_cl->addCommand( this, SLOT( handleCntThreshold() ), "CT", "Contour Threshold", false );
    cmd->registerParameter( Any::Float, true );
    cmd->registerParameter( Any::CStr, false );

    cmd = d_cl->addCommand( this, SLOT( handleCntFactor() ), "CF", "Contour Factor", false );
    cmd->registerParameter( Any::Float, true );
    cmd->registerParameter( Any::CStr, false );

    cmd = d_cl->addCommand( this, SLOT( handleCntOption() ), "CO", "Contour Options", false );
    cmd->registerParameter( Any::CStr, false ); // Leer oder + oder -
    cmd->registerParameter( Any::CStr, false );

    cmd = d_cl->addCommand( this, SLOT( handleGotoResidue() ), "GR", "Goto Residue", false );
    cmd->registerParameter( Any::Long, false ); // Leer oder ID

    d_cl->addCommand( this, SLOT( handleNextSpec2D() ), "NS", "Next Spectrum", true );
    d_cl->addCommand( this, SLOT( handlePrevSpec2D() ), "PS", "Prev. Spectrum", true );
    d_cl->addCommand( this, SLOT( handleNextSpec3D() ), "NT", "Next Spectrum", true );
    d_cl->addCommand( this, SLOT( handlePrevSpec3D() ), "PT", "Prev. Spectrum", true );

    cmd = d_cl->addCommand( this, SLOT( handleLabelHori() ), "LH", "Label Horizontal Spin", false );
    cmd->registerParameter( Any::CStr, false ); // Label oder leer

    cmd = d_cl->addCommand( this, SLOT( handleLabelVerti() ), "LV", "Label Vertical Spin", false );
    cmd->registerParameter( Any::CStr, false ); // Label oder leer

    cmd = d_cl->addCommand( this, SLOT( handlePickLabel3D() ), "PL", "Pick Label", false );
    cmd->registerParameter( Any::CStr, false ); // Label oder leer

    cmd = d_cl->addCommand( this, SLOT( handleLabelSpin3D() ), "LZ", "Label Strip Spin", false );
    cmd->registerParameter( Any::CStr, false ); // Label oder leer

    cmd = d_cl->addCommand( this, SLOT(handleExecuteLine()), "LUA", "Lua", false );
    cmd->registerParameter( Any::Memo, true );

    cmd = d_cl->addCommand( this, SLOT( handleAutoGain() ), "AG", "Set Auto Contour Gain", false );
    cmd->registerParameter( Any::Float, true );
    cmd->registerParameter( Any::CStr, false ); // Label oder leer

    cmd = d_cl->addCommand( this, SLOT( handleAutoGain3D() ), "ZAG", "Set Auto Contour Gain", false );
    cmd->registerParameter( Any::Float, true );

    cmd = d_cl->addCommand( this, SLOT( handleSetWidthFactor() ), "WF", "Set Strip Width Factor", false );
    cmd->registerParameter( Any::Float, false );

    cmd = d_cl->addCommand( this, SLOT( handleSetDepth() ), "SD", "Set Peak Depth", false );
    cmd->registerParameter( Any::Float, false );
}

void PolyScope3::buildMenus()
{
    Gui2::AutoMenu* menuFile = new Gui2::AutoMenu( tr("&File"), this );
    menuFile->addCommand( "Save", this, SLOT(handleSave()), Qt::CTRL+Qt::Key_S );
    menuFile->addCommand( "Print Preview...", this, SLOT(handleCreateReport()), false, Qt::CTRL+Qt::Key_P );
    Gui2::AutoMenu* sub = new Gui2::AutoMenu( tr("Export"), menuFile );
    sub->addCommand( "&Anchor Peaklist...", this, SLOT(handleExportPeaklist()), false );
    sub->addCommand( "&Strip Peaklist...", this, SLOT(handleExportStripPeaks()), false );
    sub->addCommand( "Strip peaks to &MonoScope...", this, SLOT(handleToMonoScope()), false );
    sub->addCommand( "Plane peaks to &MonoScope...", this, SLOT(handleToMonoScope2D()), false );
    menuFile->addMenu( sub );
    menuFile->addSeparator();
    menuFile->addCommand( "Close", this, SLOT(handleClose()), Qt::CTRL+Qt::Key_W );

    Gui2::AutoMenu* menuEdit = new Gui2::AutoMenu( menuBar() );
    menuEdit->addCommand( "Undo", this, SLOT(handleUndo()), Qt::CTRL+Qt::Key_Z );
    menuEdit->addCommand( "Redo", this, SLOT(handleRedo()), Qt::CTRL+Qt::Key_Y );
	menuBar()->insertItem( "&Edit", menuEdit );

    Gui2::AutoMenu* menuView = new Gui2::AutoMenu( menuBar() );
    menuView->addCommand( "Backward", this, SLOT(handleBackward()), false, Qt::CTRL+Qt::Key_B );
    menuView->addCommand( "Forward", this, SLOT(handleForward()), false );

    menuView->addCommand( "Fit Window", this, SLOT(handleFitWindow()), false, Qt::CTRL+Qt::Key_Home );
    menuView->addCommand( "Goto System...", this, SLOT(handleGotoSystem()), false, Qt::CTRL+Qt::Key_G );
    menuView->addCommand( "Goto Point...", this, SLOT(handleGotoPoint()), false );
    menuView->addSeparator();
    menuView->addCommand( "Set Resolution...", this, SLOT(handleSetResolution()), false );
    menuView->addCommand( "Show Low Resolution", this, SLOT(handleShowLowRes()), true );
    menuView->addSeparator();
    menuView->addCommand( "Do Pathway Simulation", this, SLOT(handleForceCross()), true );
    menuView->addCommand( "Show Ghost Peaks", this, SLOT(handleShowGhosts()), true );
    menuView->addCommand( "Show Ghost Labels", this, SLOT(handleGhostLabels()), true );
    menuView->addCommand( "Show Hidden Links", this, SLOT(handleShowAllPeaks()), true );
    menuView->addSeparator();
    menuView->addCommand( "Hide 3D Slices", this, SLOT(handleAutoHide()), true );
    menuView->addCommand( "Show Folded", this, SLOT(handleShowFolded()), true );
    menuView->addCommand( "Sync to Global Zoom", this, SLOT(handleRangeSync()), true );
    menuView->addCommand( "Sync to Global Cursor", this, SLOT(handleCursorSync()), true );
    menuView->addCommand( "Sync to Depth", this, SLOT(handleSyncDepth()), true );
    menuView->addCommand( "Center to Peak", this, SLOT(handleAutoCenter()), true );
    menuView->addCommand( "Show List", this, SLOT(handleShowList()), true );

	menuBar()->insertItem( "&View", menuView );

    Gui2::AutoMenu* menuPicking = new Gui2::AutoMenu( menuBar() );
    menuPicking->addCommand( "&Pick New System", this, SLOT(handlePickSystem()), false );
    menuPicking->addCommand( "Propose System...", this, SLOT(handleProposePeak()), false );
    menuPicking->addCommand( "Extend Horizontally", this, SLOT(handlePickHori()), false );
    menuPicking->addCommand( "Propose Horizontally...", this, SLOT(handleProposeHori()), false );
    menuPicking->addCommand( "Extend Vertically", this, SLOT(handlePickVerti()), false );
    menuPicking->addCommand( "Propose Vertically...", this, SLOT(handleProposeVerti()), false );
    menuPicking->addCommand( "&Move Spins", this, SLOT(handleMovePeak()), false );
    menuPicking->addCommand( "Move Spin &Aliases", this, SLOT(handleMovePeakAlias()), false );
    menuPicking->addCommand( "&Label Spins...", this, SLOT(handleLabelPeak()), false );
    menuPicking->addCommand( "Hide/Show Link", this, SLOT(handleHidePeak()), false );
    menuPicking->addCommand( "Set Link Params...", this, SLOT(handleSetLinkParams()), false );
    menuPicking->addSeparator();
    menuPicking->addCommand( "Set Verti. Tolerance...", this, SLOT(handleSetTolVerti()), false );
    menuPicking->addCommand( "Set Hori. Tolerance...", this, SLOT(handleSetTolHori()), false );
    menuPicking->addSeparator();
    menuPicking->addCommand( "Un-Alias Peaks", this, SLOT(handleDeleteAliasPeak()), false );
    menuPicking->addCommand( "Delete Peaks", this, SLOT(handleDeletePeak()), false );
    menuPicking->addCommand( "Delete Spin Links", this, SLOT(handleDeleteLinks()), false );
    menuPicking->addCommand( "Delete Horizontal Spin", this, SLOT(handleDeleteSpinX()), false );
    menuPicking->addCommand( "Delete Vertical Spin", this, SLOT(handleDeleteSpinY()), false );

    Gui2::AutoMenu* menuAssig = new Gui2::AutoMenu( menuBar() );
    menuAssig->addCommand( "&Link Systems...", this, SLOT(handleLinkSystems()), false );
    menuAssig->addCommand( "Unlink &Predecessor", this, SLOT(handleUnlinkPred()), false );
    menuAssig->addCommand( "Unlink &Successor", this, SLOT(handleUnlinkSucc()), false );
    menuAssig->addSeparator();
    menuAssig->addCommand( "&Assign System...", this, SLOT(handleAssign()), false );
    menuAssig->addCommand( "&Unassign System", this, SLOT(handleUnassign()), false );
    menuAssig->addCommand( "Set System &Type...", this, SLOT(handleSetSystemType()), false );
    menuAssig->addCommand( "Set Assig. Candidates...", this, SLOT(handleSetCandidates()), false );
    menuAssig->addCommand( "Show Ali&gnment...", this, SLOT(handleShowAlignment()), false );

    Gui2::AutoMenu* menuRuler = new Gui2::AutoMenu( menuBar() );
    menuRuler->addCommand( "Add &Horizontal Ruler", this, SLOT(handleAddRulerVerti()), false );
    menuRuler->addCommand( "Add &Vertical Ruler", this, SLOT(handleAddRulerHori()), false );
    menuRuler->addCommand( "Remove Selected Rulers", this, SLOT(handleRemoveRulers()), false );
    menuRuler->addCommand( "Remove All Rulers", this, SLOT(handleRemoveAllRulers()), false );
    menuRuler->addCommand( "Set Rulers to Reference", this, SLOT(handleAutoRuler()), true );

    Gui2::AutoMenu* menuAtts = new Gui2::AutoMenu( menuBar() );
    menuAtts->addCommand( "Horizontal Spin...", this, SLOT(handleEditAttsSpinH()), false );
    menuAtts->addCommand( "Vertical Spin...", this, SLOT(handleEditAttsSpinV()), false );
    menuAtts->addCommand( "Horizontal System...", this, SLOT(handleEditAttsSysH()), false );
    menuAtts->addCommand( "Vertical System...", this, SLOT(handleEditAttsSysV()), false );
    menuAtts->addCommand( "Spin Link...", this, SLOT(handleEditAttsLink()), false );

    Gui2::AutoMenu* menuOverlay = new Gui2::AutoMenu( menuBar() );
    menuOverlay->addCommand( "Set Layer &Count...", this, SLOT(handleOverlayCount()), false );
    menuOverlay->addCommand( "A&dd Overlay Layer...", this, SLOT(handleAddLayer()), false );
    menuOverlay->addCommand( "Compose &Layers...", this, SLOT(handleComposeLayers()), false );
    menuOverlay->addCommand( "Set &Active Layer...", this, SLOT(handleActiveOverlay()), false );
    menuOverlay->addCommand( "Select &Spectrum...", this, SLOT(handleOverlaySpec()), false );
    menuOverlay->addCommand( "Set &Positive Color...", this, SLOT(handleSetPosColor()), false );
    menuOverlay->addCommand( "Set &Negative Color...", this, SLOT(handleSetNegColor()), false );
    menuOverlay->addCommand( "&Make Colors Persistent", this, SLOT(handleSaveColors()), false );

    Gui2::AutoMenu* menuPlane = new Gui2::AutoMenu( menuBar() );
    menuPlane->addCommand( "&Calibrate", this, SLOT(handleSpecCalibrate()), false );
    menuPlane->insertItem( "Select Spectrum", getPopSpec2D() );
	menuPlane->insertItem( "&Overlay", menuOverlay );
    menuPlane->addSeparator();
    menuPlane->addCommand( "Show Contour", this, SLOT(handleShowContour()), true );
    menuPlane->addCommand( "Show Intensity", this, SLOT(handleShowIntensity()), true );
    menuPlane->addCommand( "Auto Contour Level", this, SLOT(handleAutoContour()), true );
    menuPlane->addCommand( "Set Contour Parameters...", this, SLOT(handleContourParams()), false );
    menuPlane->addSeparator();
    menuPlane->addCommand( "Forward Plane", this, SLOT(handleForwardPlane()), false, Qt::SHIFT+Qt::Key_Next );
    menuPlane->addCommand( "Backward Plane", this, SLOT(handleBackwardPlane()), false, Qt::SHIFT+Qt::Key_Prior );
    menuPlane->addSeparator();
	menuPlane->insertItem( "&Picking", menuPicking );
	menuPlane->insertItem( "&Assignment", menuAssig );
	menuPlane->insertItem( "&Ruler", menuRuler );
	menuPlane->insertItem( "&Edit Attributes", menuAtts );
    menuPlane->addSeparator();
    menuPlane->addCommand( "Hold Reference", this, SLOT(handleHoldReference()), false );
    menuPlane->addCommand( "Auto Hold", this, SLOT(handleAutoHold()), true );
    menuPlane->addSeparator();
    menuPlane->addCommand( "Show 3D Plane", this, SLOT(handleShow3dPlane()), true, Qt::CTRL+Qt::Key_3 );
    menuPlane->addCommand( "Show Spin Links", this, SLOT(handleShowLinks()), true );
    menuPlane->addCommand( "Show Infered Peaks", this, SLOT(handleShowInfered()), true );
    menuPlane->addCommand( "Show Unlabeled Peaks", this, SLOT(handleShowUnlabeled()), true );
	menuPlane->addCommand( "Show Peaks with foreign Labels", this, SLOT(handleShowUnknown()), true );
    menuPlane->addCommand( "Resolve Projected Spins", this, SLOT(handleShowWithOff2()), true );
    menuPlane->addCommand( "Use Link Color Codes", this, SLOT(handleUseLinkColors()), true );
    sub = new Gui2::AutoMenu( menuBar() );
	menuPlane->insertItem( "Show Labels", sub );
	short i;
	for( i = SpinPointView::None; i < SpinPointView::End; i++ )
        sub->addCommand( SpinPointView::menuText[ i ], this, SLOT(handleViewLabels()), true )->setProperty( "0", i );
	menuBar()->insertItem( "&Plane", menuPlane );

    Gui2::AutoMenu* menuStrip = new Gui2::AutoMenu( menuBar() );
    menuStrip->addCommand( "Calibrate Strip", this, SLOT(handleStripCalibrate()), false );
    menuStrip->addCommand( "Set Tolerance...", this, SLOT(handleSetTolStrip()), false );
    menuStrip->addSeparator();
    menuStrip->addCommand( "Auto Contour Level", this, SLOT(handleAutoContour2()), true );
    menuStrip->addCommand( "Set Contour Parameters...", this, SLOT(handleContourParams2()), false );

    menuStrip->insertItem( "Select Spectrum", getPopSpec3D() );
    menuStrip->addSeparator();
    menuStrip->addCommand( "&Pick Spin", this, SLOT(handlePickSpin3D()), false );
    menuStrip->addCommand( "Pick Label...", this, SLOT(handlePickLabel3D()), false );
    menuStrip->addCommand( "Propose &Spin...", this, SLOT(handleProposeSpin()), false );
    menuStrip->addCommand( "&Move Spin", this, SLOT(handleMoveSpin3D()), false );
    menuStrip->addCommand( "&Move Spin Alias", this, SLOT(handleMoveSpinAlias3D()), false );

    menuStrip->addCommand( "&Label Spin...", this, SLOT(handleLabelSpin3D()), false );
    menuStrip->addCommand( "Hide/Show Link", this, SLOT(handleHidePeak2()), false );
    menuStrip->addCommand( "Set Link Params...", this, SLOT(handleSetLinkParams3D()), false );

    menuStrip->addCommand( "&Force Spin Label", this, SLOT(handleForceLabelSpin3D()), false );
    menuStrip->addCommand( "&Delete Spins", this, SLOT(handleDeleteSpins3D()), false );
    menuStrip->addCommand( "Delete Spin &Links", this, SLOT(handleDeleteLinks3D()), false );
    menuAtts = new Gui2::AutoMenu( menuBar() );
    menuAtts->addCommand( "Spin...", this, SLOT(handleEditAttsSpin3D()), false );
    menuAtts->addCommand( "System...", this, SLOT(handleEditAttsSys3D()), false );
    menuAtts->addCommand( "Spin Link...", this, SLOT(handleEditAttsLink3D()), false );
	menuStrip->insertItem( "&Edit Attributes", menuAtts );
    menuStrip->addSeparator();
    menuStrip->addCommand( "Show Spin Links", this, SLOT(handleShowLinks2()), true );
    menuStrip->addCommand( "Show Infered Peaks", this, SLOT(handleShowInfered2()), true );
    menuStrip->addCommand( "Show Unlabeled Peaks", this, SLOT(handleShowUnlabeled2()), true );
	menuStrip->addCommand( "Show Peaks with foreign Labels", this, SLOT(handleShowUnknown2()), true );
    menuStrip->addCommand( "Resolve Projected Spins", this, SLOT(handleShowWithOff()), true );
    menuStrip->addCommand( "Use Link Color Codes", this, SLOT(handleUseLinkColors3D()), true );
    menuStrip->addCommand( "Fit Window", this, SLOT(handleFitWindow3D()), false, Qt::Key_Home );
    sub = new Gui2::AutoMenu( menuBar() );
	menuStrip->insertItem( "Show Labels", sub );
	for( i = SpinPointView::None; i < SpinPointView::End; i++ )
        sub->addCommand( SpinPointView::menuText[ i ], this, SLOT(handleViewLabels3D()), true )
                ->setProperty("0", i );
	menuBar()->insertItem( "&Strips", menuStrip );

    Gui2::AutoMenu* menuPeaks = new Gui2::AutoMenu( menuBar() );
    menuPeaks->addCommand( "&New Peaklist", this, SLOT(handleNewPeakList()), false );
    menuPeaks->addCommand( "&Open Peaklist...", this, SLOT(handleOpenPeakList()), false );
    menuPeaks->addCommand( "&Add to Repository...", this, SLOT(handleSavePeakList()), false );
    menuPeaks->addCommand( "Map to Spectrum...", this, SLOT(handleMapPeakList()), false );
    menuPeaks->addCommand( "Select Color...", this, SLOT(handleSetPlColor()), false );
    sub = new Gui2::AutoMenu( menuBar() );
	menuPeaks->insertItem( "Show Labels", sub );
	for( i = PeakPlaneView::NONE; i < PeakPlaneView::END; i++ )
        sub->addCommand( PeakPlaneView::menuText[ i ], this, SLOT(handleViewPlLabels()), true )
                ->setProperty( "0", i );
    menuPeaks->addSeparator();
    menuPeaks->addCommand( "&Pick Peak", this, SLOT(handlePickPlPeak()), false );
    menuPeaks->addCommand( "&Move Peak", this, SLOT(handleMovePlPeak()), false );
    menuPeaks->addCommand( "&Move Peak Alias", this, SLOT(handleMovePlAlias()), false );
    menuPeaks->addCommand( "&Label Peak...", this, SLOT(handleLabelPlPeak()), false );
    menuPeaks->addCommand( "&Delete Peaks", this, SLOT(handleDeletePlPeaks()), false );
    menuPeaks->addCommand( "&Edit Attributes...", this, SLOT(handleEditPlPeakAtts()), false );
	menuBar()->insertItem( "&Peaks", menuPeaks );

    Gui2::AutoMenu* menuHelp = new Gui2::AutoMenu( menuBar() );
    menuHelp->addCommand( "About...", this, SLOT(handleHelpAbout()) );
	menuBar()->insertItem( "&?", menuHelp );

}

void PolyScope3::buildViews()
{
	d_focus->setBody( 0 );

	Splitter* inner = new Splitter( 2, 2 );
	inner->setBarWidth( 80 );

	d_focus->clear();
    d_focus->setCircle( true );

    Splitter* outer = new Splitter( 1, 2 );
	outer->setBarWidth( 80 );
	outer->setPane( inner, 0, 0 );

    inner->setPane( getPlane().d_viewer, 0, 1 );

    inner->setPane( getSlice( DimX ).d_viewer, 1, 1 );

    inner->setPane( getSlice( DimY ).d_viewer, 0, 0 );

	Glyph* box = LayoutKit::hbox();
	outer->setPane( box, 0, 1 );

    box->append( getSlice( DimZ ).d_viewer );
	box->append( new Rule( DimensionY, g_frameClr, 40 ) );
    box->append( getStrip( DimX ).d_viewer );
	box->append( new Rule( DimensionY, g_frameClr, 40 ) );
    box->append( getStrip( DimY ).d_viewer );

    if( true )
	{
        QDockWidget* dock = new QDockWidget( tr("Spin Systems"), this );
        dock->setAllowedAreas( Qt::AllDockWidgetAreas );
        dock->setFeatures( QDockWidget::AllDockWidgetFeatures );
        dock->setFloating( false );
        dock->setVisible( false );
        addDockWidget( Qt::RightDockWidgetArea, dock );

        d_list = new StripListGadget2( d_widget, getAgent(), getProject(), true );
        dock->setWidget( d_list );
        connect( d_list,SIGNAL(returnPressed(Gui::ListViewItem*)),this,SLOT(onListActivate()) );
        connect( d_list,SIGNAL(doubleClicked(Gui::ListViewItem*)), this, SLOT(onListActivate()) );

        Gui2::AutoMenu* menu = new Gui2::AutoMenu( d_list, true );
        menu->addCommand( "Goto...", this, SLOT(handleGoto()), false );
        menu->addCommand( "Use 3D Navigation", this, SLOT(handleGoto3D()), true );
        menu->addCommand( "Show 3D Values", this, SLOT(handleUse3DSpec()), true );
        menu->addCommand( "Find Spin...", d_list, SLOT(handleFindSpin()), false );
        menu->addCommand( "Find Link Partner...", d_list, SLOT(handleGotoOther()), false );
		menu->addSeparator();
        menu->addCommand( "Run Automatic Strip Matcher", d_list, SLOT(handleRunStripper()), true );
        menu->addCommand( "Strict Strip Matching", d_list, SLOT(handleStrictStripMatch()), true );
        menu->addCommand( "Set Spin Tolerance...", d_list, SLOT(handleSetSpinTol()), false );
		menu->addSeparator();
        menu->addCommand( "Create System", d_list, SLOT(handleCreateSystem()), false );
        menu->addCommand( "Eat System...", d_list, SLOT(handleEatSystem()), false );
        menu->addCommand( "Set Assig. Candidates...", d_list, SLOT(handleSetCandidates()), false );
        menu->addCommand( "Set System Type...", d_list, SLOT(handleSetSysType()), false );
        menu->addCommand( "Show Alignment...", d_list, SLOT(handleShowAlignment()), false );
		menu->addSeparator();
        menu->addCommand( "Link This", d_list, SLOT(handleLinkThis()), false );
        menu->addCommand( "Unlink Successor", d_list, SLOT(handleUnlinkSucc()), false );
        menu->addCommand( "Unlink Predecessor", d_list, SLOT(handleUnlinkPred()), false );
		menu->addSeparator();
        menu->addCommand( "Create Spin...", d_list, SLOT(handleCreateSpin()), false );
        menu->addCommand( "Move Spin...", d_list, SLOT(handleMoveSpin()), false );
        menu->addCommand( "Label Spin...", d_list, SLOT(handleLabelSpin()), false );
        menu->addCommand( "Force Label...", d_list, SLOT(handleForceLabel()), false );
        menu->addCommand( "Accept Label", d_list, SLOT(handleAcceptLabel()), false );
		menu->addSeparator();
        menu->addCommand( "Assign to...", d_list, SLOT(handleAssign()), false );
        menu->addCommand( "Unassign", d_list, SLOT(handleUnassign()), false );
        menu->addCommand( "Delete", d_list, SLOT(handleDelete()), false );
        menu->addCommand( "Edit Attributes...", d_list, SLOT(handleEditAtts()), false );
		menu->addSeparator();
        menu->addCommand( "Show Spin Links", d_list, SLOT(handleShowLinks()), true );
        menu->addCommand( "Create Link...", d_list, SLOT(handleCreateLink()), false );
        menu->addCommand( "Set Link Params...", d_list, SLOT(handleLinkParams()), false );
		menu->addSeparator();
        menu->addCommand( "Open All", d_list, SLOT(handleOpenAll()), false );
        menu->addCommand( "Close All", d_list, SLOT(handleCloseAll()), false );
		if( d_use3D )
            d_list->setSpec( getSpec3D() );
		else
            d_list->setSpec( getSpec2D() );
	}else
		d_list = 0;

    if( getSpec2D()->getDimCount() == 2 )
	{
		d_ov = new SpecViewer( new ViewAreaMdl( true, true, true, true ) );
        Root::Ref<SpecProjector> pro = new SpecProjector( getSpec(), DimX, DimY );
		Root::Ref<SpecBufferMdl> mdl = new SpecBufferMdl( d_ov->getViewArea(), pro );
		d_ov->getViews()->append( new IntensityView( mdl ) );
        d_ovCtrl = new OverviewCtrl( mdl, getPlane().d_viewer->getViewArea() );
		d_ov->getHandlers()->append( d_ovCtrl );
		d_ov->getHandlers()->append( new FocusCtrl( d_ov ) );
        // mdl->copy( getPlane().d_ol[0].d_buf );
	}
	inner->setPane( d_ov, 1, 0 );

    d_focus->append( getSlice( DimY ).d_viewer->getController() );
    d_focus->append( getPlane().d_viewer->getController() );
    d_focus->append( getSlice( DimX ).d_viewer->getController() );
    d_focus->append( getSlice( DimZ ).d_viewer->getController() );
    d_focus->append( getStrip( DimX ).d_viewer->getController() );
    d_focus->append( getStrip( DimY ).d_viewer->getController() );

	d_focus->setBody( outer );


    inner->setRowPos( 1,  17 * d_widget->height() );
    inner->setColumnPos( 1,  3 * d_widget->width() );
    outer->setColumnPos( 1,  15 * d_widget->width() );
    d_widget->reallocate();
    getPlane().d_ol[0].d_buf->fitToArea();
    setCursor();
    d_widget->setFocusGlyph( getPlane().d_viewer->getController() );

	if( d_ov )
	{ 
        if( getSpec2D()->canDownsize() )
		{
			d_ovCtrl->getModel()->fitToArea();
		}else
		{
            d_ovCtrl->getModel()->copy( getPlane().d_ol[0].d_buf );
		}
	}
	d_widget->getViewport()->damageAll();
}

Root::Ref<PointSet> PolyScope3::createStripPeaks()
{
    Spectrum* spec = getSpec3D();
	PpmPoint p( 0, 0, 0 );
	PointSet::Assig a;
	char buf[32];
	Root::Index id;
	Root::Ref<PointSet> ps = Factory::createEasyPeakList( spec );

    LinkFilterRotSpace* mdl = getMdl3D();
	LinkFilterRotSpace::Iterator i, _end = mdl->end();
	LinkFilterRotSpace::Element e;
    SpinPointView::Label l = getStrip( DimX ).d_tuples->getLabel();
	SpinLink* link;
	for( i = mdl->begin(); i != _end; ++i )
	{
		mdl->fetch( i, e );
        if( e.isGhost() || ( !e.isInfer() && e.isHidden() ) )
			continue;
		p[ DimX ] = e.d_point[ DimX ]->getShift( spec );
		p[ DimY ] = e.d_point[ DimY ]->getShift( spec );
		p[ DimZ ] = e.d_point[ DimZ ]->getShift( spec );
		a[ DimX ] = e.d_point[ DimX ]->getId();
		a[ DimY ] = e.d_point[ DimY ]->getId();
		a[ DimZ ] = e.d_point[ DimZ ]->getId();
		id = ps->getNextId();
		ps->setPoint( id, p );
		ps->setAssig( id, a );
        if( ( link = e.d_point[ DimX ]->findLink( e.d_point[ DimY ] ) ) )
			ps->setCode( id, link->getAlias( spec ).d_code );
        else if( ( link = e.d_point[ DimX ]->findLink( e.d_point[ DimZ ] ) ) )
			ps->setCode( id, link->getAlias( spec ).d_code );

		SpinPointView::formatLabel( buf, sizeof(buf), e.d_point, l, DimZ );
		ps->setComment( id, buf );
	}
	return ps;
}

bool PolyScope3::askToCloseWindow()
{
    return askToClosePeaklist();
}

void PolyScope3::handleShowList()
{
    CHECKED_IF( true, d_list->parentWidget()->isVisible() );

    d_list->parentWidget()->setVisible( !d_list->parentWidget()->isVisible() );
}

void PolyScope3::handleSetTolerance()
{
    ENABLED_IF( true );

    Gui2::UiFunction* a = Gui2::UiFunction::me();
    AtomType t = AtomType::parseLabel( a->property("0").toByteArray() );
	if( t.isNone() )
	{
		Lexi::ShowStatusMessage msg( "Set Tolerance: invalid atom type" );
        getAgent()->traverseUp( msg );
		return;
	}
    PPM p = a->property( "1" ).toDouble();
    getProject()->getMatcher()->setTol( t, p );
}

void PolyScope3::handleExportPeaklist()
{
    ENABLED_IF( true );

	QString fileName = QFileDialog::getSaveFileName( getQt(), "Export 2D Pealist",
                                                     Root::AppAgent::getCurrentDir(), "*.peaks" );
	if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );
	if( info.extension( false ).upper() != "PEAKS" )
		fileName += ".peaks";
	info.setFile( fileName );
	if( info.exists() )
	{
		if( QMessageBox::warning( getQt(), "Save As",
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

Root::Ref<PointSet> PolyScope3::createPlanePeaks()
{
    Spectrum* spec = getSpec();
	PpmPoint p( 0, 0 );
	PointSet::Assig a;
	Root::Index id;
	Root::Ref<PointSet> ps = Factory::createEasyPeakList( spec );
    LinkFilterRotSpace* mdl = getMdl2D();
	LinkFilterRotSpace::Iterator i, _end = mdl->end();
	LinkFilterRotSpace::Element e;
	char buf[32];
    SpinPointView::Label l = getPlane().d_tuples->getLabel();
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
	
		SpinPointView::formatLabel( buf, sizeof(buf), e.d_point, l, DimY );
		ps->setComment( id, buf );
	}
	return ps;
}

void PolyScope3::handleSetTolHori()
{
    ENABLED_IF( true );

	AtomType atom;
    atom = getSpec()->getColor( DimX );

	bool ok	= FALSE;
	QString res;
	QString str;
	str.sprintf( "Please enter atom positive PPM value for %s:", atom.getIsoLabel() );
    res.sprintf( "%0.3f", getProject()->getMatcher()->getTol( atom ) );
	res	= QInputDialog::getText( "Set Matching Tolerance", str, QLineEdit::Normal, res, &ok, getQt() );
	if( !ok )
		return;
	PPM w = res.toFloat( &ok );
	if( !ok ) // RK 31.10.03: w <= 0 neu erlaubt
	{
		QMessageBox::critical( getQt(), "Set Spin Tolerance", "Invalid tolerance!", "&Cancel" );
		return;
	}

    getProject()->getMatcher()->setTol( atom, w );
}

void PolyScope3::handleSetTolVerti()
{
    ENABLED_IF( true );

	AtomType atom;
    atom = getSpec()->getColor( DimY );

	bool ok	= FALSE;
	QString res;
    res.sprintf( "%0.3f", getProject()->getMatcher()->getTol( atom ) );
	QString str;
	str.sprintf( "Please enter atom positive PPM value for %s:", atom.getIsoLabel() );
	res	= QInputDialog::getText( "Set Vertical Spin Tolerance", str, QLineEdit::Normal, res, &ok, getQt() );
	if( !ok )
		return;
	PPM w = res.toFloat( &ok );
	if( !ok ) // RK 31.10.03: w <= 0 neu erlaubt
	{
		QMessageBox::critical( getQt(), "Set Spin Tolerance", "Invalid tolerance!", "&Cancel" );
		return;
	}

    getProject()->getMatcher()->setTol( atom, w );
}

void PolyScope3::handleSetTolStrip()
{
    ENABLED_IF( getSpec3D() );

	AtomType atom;
    atom = getSpec3D()->getColor( DimZ );

	bool ok	= FALSE;
	QString res;
    res.sprintf( "%0.3f", getProject()->getMatcher()->getTol( atom ) );
	QString str;
	str.sprintf( "Please enter atom positive PPM value for %s:", atom.getIsoLabel() );
	res	= QInputDialog::getText( "Set Strip Tolerance", str, QLineEdit::Normal, res, &ok, getQt() );
	if( !ok )
		return;
	PPM w = res.toFloat( &ok );
	if( !ok || w < 0.0 )
	{
		QMessageBox::critical( getQt(), "Set Strip Tolerance", "Invalid tolerance!", "&Cancel" );
		return;
	}

    getProject()->getMatcher()->setTol( atom, w );
}

void PolyScope3::handleExportStripPeaks()
{
    Spectrum* spec = getSpec3D();
    ENABLED_IF( spec );

	QString fileName = QFileDialog::getSaveFileName( getQt(), "Export 3D Pealist",
                                                     Root::AppAgent::getCurrentDir(), "*.peaks" );
	if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );

	if( info.extension( false ).upper() != "PEAKS" )
		fileName += ".peaks";
	info.setFile( fileName );
	if( info.exists() )
	{
		if( QMessageBox::warning( getQt(), "Save As",
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

void PolyScope3::handleToMonoScope()
{
    Spectrum* spec = getSpec3D();
    ENABLED_IF( spec );

	try
	{
		Root::Ref<PointSet> ps = createStripPeaks();
		if( ps.isNull() )
			throw Root::Exception( "Cannot create Peaklist" );
		Root::Ref<PeakList> pl = new PeakList( spec );
		pl->append( ps ); // pl ist nun eine identische Kopie von ps.
		pl->setName( "Export from PolyScope3" );
        createMonoScope( getAgent()->getParent(), spec, getProject(), pl, Rotation() );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Error Exporting Peaklist", e.what() );
	}catch( ... )
	{
		Root::ReportToUser::alert( this, "Error Exporting Peaklist",
			"Unknown Exception" );
	}
}

void PolyScope3::handleGoto()
{
    ENABLED_IF( d_list );
    gotoTuple( d_list->getSelectedStrip(), d_list->getSelectedSpin(),
		d_list->getSelectedLink(), !d_goto3D );
    d_widget->setFocusGlyph( getPlane().d_viewer->getController() );
}

void PolyScope3::handleSyncHori()
{
    Spin* spin = getSel( true );
    ENABLED_IF( d_list && spin );
	d_list->gotoSpin( spin );
}

void PolyScope3::handleSyncVerti()
{
    Spin* spin = getSel( false );
    ENABLED_IF( d_list && spin );
	d_list->gotoSpin( spin );
}

void PolyScope3::handleGotoResidue()
{
    ENABLED_IF( true );

	Root::Index id;
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    if( a->property( "0" ).isNull() )
	{
		bool ok	= FALSE;
		id	= QInputDialog::getInteger( "Goto Residue", 
			"Please	enter a residue number:", 
			0, -999999, 999999, 1, &ok, getQt() );
		if( !ok )
			return;
	}else
        id = a->property( "0" ).toInt();

    Residue* resi = getProject()->getSequence()->getResidue( id );
	if( resi == 0 || resi->getSystem() == 0 )
	{
		Lexi::ShowStatusMessage msg( "Goto Residue: unknown or unassigned residue" );
		traverse( msg );
		return;
	}

    a->setProperty( "0", Root::Int32(resi->getSystem()->getId() ) );
    handleGotoSystem();
}

void PolyScope3::handleSetWidthFactor()
{
    ENABLED_IF( true );

	float g;
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    if( a->property( "0" ).isNull() )
	{
		bool ok	= FALSE;
		QString res;
        res.sprintf( "%0.2f", getProject()->getWidthFactor() );
		res	= QInputDialog::getText( "Set Strip Width Factor", 
			"Please	enter a positive factor:", QLineEdit::Normal, res, &ok, getQt() );
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
    getProject()->setWidthFactor( g );
}

void PolyScope3::handleUse3DSpec()
{
    CHECKED_IF( d_list && this, d_use3D );
	d_use3D = !d_use3D;
	if( d_use3D )
        d_list->setSpec( getSpec3D() );
	else
        d_list->setSpec( getSpec2D() );
}

void PolyScope3::handleGoto3D()
{
    CHECKED_IF( true, d_goto3D );
	d_goto3D = !d_goto3D;
}

void PolyScope3::handleSaveColors()
{
    ENABLED_IF( true );
    Repository::SlotColors clr( getPlane().d_ol.size() );
	for( int i = 0; i < clr.size(); i++ )
	{
        clr[ i ].d_pos = getPlane().d_ol[i].d_view->getPosColor();
        clr[ i ].d_neg = getPlane().d_ol[i].d_view->getNegColor();
	}
    getProject()->getRepository()->setScreenClr( clr );
}

void PolyScope3::handleToMonoScope2D()
{
    Spectrum* spec = getSpec();
    ENABLED_IF( spec );

	try
	{
		Root::Ref<PointSet> ps = createPlanePeaks();
		Root::Ref<PeakList> pl = new PeakList( spec );
		pl->append( ps ); // pl ist nun eine identische Kopie von ps.
		pl->setName( "Export from HomoScope2" );
        createMonoScope( getAgent()->getParent(), spec, getProject(), pl, Rotation() );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Error Exporting Peaklist", e.what() );
	}catch( ... )
	{
		Root::ReportToUser::alert( this, "Error Exporting Peaklist", "Unknown Exception" );
	}
}

void PolyScope3::initViews( bool homo )
{
	d_lock = true;
	d_cursor.assign( 3, 0 );
	d_slices.assign( (homo)?2:3, SliceSocket() );

	createPlane();
	createSlice( DimX, DimX );
	d_slices[ DimX ].d_spec2D = new SpecProjector( d_spec2D, DimX );
	d_slices[ DimX ].d_buf2D->setSpectrum( d_slices[ DimX ].d_spec2D, false );
	createSlice( DimY, DimY );
	d_slices[ DimY ].d_spec2D = new SpecProjector( d_spec2D, DimY );
	d_slices[ DimY ].d_buf2D->setSpectrum( d_slices[ DimY ].d_spec2D, false );
	if( !homo )
	{
		d_strips.assign( 2, StripSocket() );
		createSlice( DimY, DimZ );
		createStrip( DimX );
		createStrip( DimY );
	}
	d_lock = false;

	// d_plane.d_ol[0].d_buf->fitToArea(); wird verzï¿½ert gemacht
	updatePlaneLabel();
}

void PolyScope3::buildPopup()
{
    d_popSpec2D = new Gui2::AutoMenu(0);
    d_popSpec3D = new Gui2::AutoMenu(0);

    Gui2::AutoMenu* menuPeaks = new Gui2::AutoMenu( d_popPlane );
    menuPeaks->addCommand( "&Pick Peak", this, SLOT(handlePickPlPeak()), false );
    menuPeaks->addCommand( "&Move Peak", this, SLOT(handleMovePlPeak()), false );
    menuPeaks->addCommand( "&Move Peak Alias", this, SLOT(handleMovePlAlias()), false );
    menuPeaks->addCommand( "&Label Peak...", this, SLOT(handleLabelPlPeak()), false );
    menuPeaks->addCommand( "&Delete Peaks", this, SLOT(handleDeletePlPeaks()), false );
    menuPeaks->addCommand( "&Edit Attributes...", this, SLOT(handleEditPlPeakAtts()), false );
    menuPeaks->addSeparator();
    menuPeaks->addCommand( "&Open Peaklist...", this, SLOT(handleOpenPeakList()), false );

    d_popPlane = new Gui2::AutoMenu(0);
    d_popPlane->addCommand( "Hold Reference", this, SLOT(handleHoldReference()), true );
    d_popPlane->addCommand( "Add Vertical Ruler", this, SLOT(handleAddRulerHori()), false );
    d_popPlane->addCommand( "Add Horizontal Ruler", this, SLOT(handleAddRulerVerti()), false );
    d_popPlane->addCommand( "Remove All Rulers", this, SLOT(handleRemoveAllRulers()), false );
	d_popPlane->insertItem( "Select Spectrum", d_popSpec2D );
	d_popPlane->insertItem( "Peaks", menuPeaks );
    d_popPlane->addSeparator();
    d_popPlane->addCommand( "&Pick New System", this, SLOT(handlePickSystem()), false );
    d_popPlane->addCommand( "Propose System...", this, SLOT(handleProposePeak()), false );
    d_popPlane->addCommand( "Extend Horizontally", this, SLOT(handlePickHori()), false );
    d_popPlane->addCommand( "Propose Horizontally...", this, SLOT(handleProposeHori()), false );
    d_popPlane->addCommand( "Extend Vertically", this, SLOT(handlePickVerti()), false );
    d_popPlane->addCommand( "Propose Vertically...", this, SLOT(handleProposeVerti()), false );
    d_popPlane->addCommand( "&Move Spins", this, SLOT(handleMovePeak()), false );
    d_popPlane->addCommand( "Move Spin &Aliases", this, SLOT(handleMovePeakAlias()), false );
    d_popPlane->addCommand( "&Label Spins...", this, SLOT(handleLabelPeak()), false );
    d_popPlane->addCommand( "Hide/Show Link", this, SLOT(handleHidePeak()), false );
    d_popPlane->addCommand( "Set System &Type...", this, SLOT(handleSetSystemType()), false );
    d_popPlane->addCommand( "Set Link Params...", this, SLOT(handleSetLinkParams()), false );

    Gui2::AutoMenu* menuAtts = new Gui2::AutoMenu( d_popPlane );
    menuAtts->addCommand( "Horizontal Spin...", this, SLOT(handleEditAttsSpinH()), false );
    menuAtts->addCommand( "Vertical Spin...", this, SLOT(handleEditAttsSpinV()), false );
    menuAtts->addCommand( "Horizontal System...", this, SLOT(handleEditAttsSysH()), false );
    menuAtts->addCommand( "Vertical System...", this, SLOT(handleEditAttsSysV()), false );
    menuAtts->addCommand( "Spin Link...", this, SLOT(handleEditAttsLink()), false );
    d_popPlane->insertItem( "&Edit Attributes", menuAtts );

    d_popPlane->addSeparator();
    d_popPlane->addCommand( "Un-Alias Peaks", this, SLOT(handleDeleteAliasPeak()), false );
    d_popPlane->addCommand( "Delete Peaks", this, SLOT(handleDeletePeak()), false );
    d_popPlane->addCommand( "Delete Spin Links", this, SLOT(handleDeleteLinks()), false );
    d_popPlane->addCommand( "Delete Horizontal Spin", this, SLOT(handleDeleteSpinX()), false );
    d_popPlane->addCommand( "Delete Vertical Spin", this, SLOT(handleDeleteSpinY()), false );
    d_popPlane->addSeparator();
    // d_popPlane->addCommand( "Forward", Forward, false );
    d_popPlane->addCommand( "Set Peak Depth...", this, SLOT(handleSetDepth()), false );
    d_popPlane->addCommand( "Show Alignment...", this, SLOT(handleShowAlignment()), false );
    d_popPlane->addCommand( "Backward", this, SLOT(handleBackward()), false );
    d_popPlane->addCommand( "Show 3D Plane", this, SLOT(handleShow3dPlane()), true );
    d_popPlane->addCommand( "Fit Window", this, SLOT(handleFitWindow()), false );

    d_popStrip = new Gui2::AutoMenu(0);
    d_popStrip->insertItem( "Select Spectrum", d_popSpec3D );
	d_popStrip->insertItem( "Peaks", menuPeaks );
    d_popStrip->addSeparator();
    d_popStrip->addCommand( "&Pick Spin", this, SLOT(handlePickSpin3D()), false );
    d_popStrip->addCommand( "Pick Label...", this, SLOT(handlePickLabel3D()), false );
    d_popStrip->addCommand( "&Propose Spin...", this, SLOT(handleProposeSpin()), false );
    d_popStrip->addCommand( "&Move Spin", this, SLOT(handleMoveSpin3D()), false );
    d_popStrip->addCommand( "&Move Spin Alias", this, SLOT(handleMoveSpinAlias3D()), false );
    d_popStrip->addCommand( "Label Spin...", this, SLOT(handleLabelSpin3D()), false );
    d_popStrip->addCommand( "&Force Spin Label", this, SLOT(handleForceLabelSpin3D()), false );
    d_popStrip->addCommand( "&Delete Spins", this, SLOT(handleDeleteSpins3D()), false );
    d_popStrip->addCommand( "Delete Links", this, SLOT(handleDeleteLinks3D()), false );
    d_popStrip->addCommand( "Set Link Params...", this, SLOT(handleSetLinkParams3D()), false );
    menuAtts = new Gui2::AutoMenu( d_popStrip );
    menuAtts->addCommand( "Spin...", this, SLOT(handleEditAttsSpin3D()), false );
    menuAtts->addCommand( "System...", this, SLOT(handleEditAttsSys3D()), false );
    menuAtts->addCommand( "Spin Link...", this, SLOT(handleEditAttsLink3D()), false );
	d_popStrip->insertItem( "&Edit Attributes", menuAtts );
    d_popStrip->addSeparator();
    d_popStrip->addCommand( "Set Peak Width...", this, SLOT(handleSetWidth()), false );
    d_popStrip->addCommand( "Show 3D Plane", this, SLOT(handleShow3dPlane()), true );
    d_popStrip->addCommand( "Fit Window", this, SLOT(handleFitWindow3D()), false );
}

void PolyScope3::buildPopup2()
{
    d_popSpec3D = new Gui2::AutoMenu(0);
    d_popLabel = new Gui2::AutoMenu(0); // Explizit lschen

    d_popPlane = new Gui2::AutoMenu(0);
    d_popPlane->addCommand( "Pick System", this, SLOT(handlePickSystem()), false );
    d_popPlane->addCommand( "Move System", this, SLOT(handleMovePeak()), false );
    d_popPlane->addCommand( "Move System Alias", this, SLOT(handleMovePeakAlias()), false );
    d_popPlane->addCommand( "Delete Systems", this, SLOT(handleDeletePeak()), false );
    d_popPlane->addCommand( "Edit Attributes...", this, SLOT(handleEditAttsSysH()), false );
	/* TODO
    d_popPlane->addSeparator();
    d_popPlane->addCommand( "&Pick Peak", PickPeak, false );
    d_popPlane->addCommand( "&Move Peak", MovePeak, false );
    d_popPlane->addCommand( "&Force Peak Label", ForceLabelPeak, false );
    d_popPlane->addCommand( "&Delete Peaks", DeletePeaks, false );
    d_popPlane->addCommand( "Edit Peak Attributes...", EditPeakAtts, false );
	*/
    d_popPlane->addSeparator();
    d_popPlane->addCommand( "Calibrate from System", this, SLOT(handleSpecCalibrate()), false );
    d_popPlane->addCommand( "Show 3D Plane", this, SLOT(handleShow3dPlane()), true );
    d_popPlane->addCommand( "Fit Window", this, SLOT(handleFitWindow()), false );


    d_popStrip = new Gui2::AutoMenu(0);
	d_popStrip->insertItem( "Select Spectrum", d_popSpec3D );
    d_popStrip->addSeparator();
    d_popStrip->addCommand( "&Pick Spin", this, SLOT(handlePickSpin3D()), false );
    d_popStrip->addCommand( "Pick Label...", this, SLOT(handlePickLabel3D()), false );
    d_popStrip->addCommand( "&Move Spin", this, SLOT(handleMoveSpin3D()), false );
    d_popStrip->addCommand( "&Move Spin Alias", this, SLOT(handleMoveSpinAlias3D()), false );
	d_popStrip->insertItem( "Label Spin", d_popLabel );
    d_popStrip->addCommand( "&Force Spin Label", this, SLOT(handleForceLabelSpin3D()), false );
    d_popStrip->addCommand( "&Delete Spins", this, SLOT(handleDeleteSpins3D()), false );
    d_popStrip->addCommand( "Edit Attributes...", this, SLOT(handleEditAttsSpin3D()), false );
    d_popStrip->addSeparator();
    d_popStrip->addCommand( "Set Peak Width...", this, SLOT(handleSetWidth()), false );
    d_popStrip->addCommand( "Show 3D Plane", this, SLOT(handleShow3dPlane()), true );
    d_popStrip->addCommand( "Fit Window", this, SLOT(handleFitWindow3D()), false );
}

void PolyScope3::updateSpecPop2D()
{
	if( d_popSpec2D == 0 )
		return;

    d_popSpec2D->clear();
	ColorMap a, b;
	d_spec2D->getColors( a );
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
	if( d_spec2D == d_spec3D )
	{
        d_popSpec2D->addCommand( d_spec3D->getName(), this, SLOT(handleSelectSpec2D()), true )->
                setProperty("0", QVariant::fromValue(d_spec3D.deref() ) );
    }
	Sort::const_iterator pp1;
	Project::SpecSet::const_iterator p1;
	for( p1 = l.begin(); p1 != l.end(); ++p1 )
		d_sort2D[ (*p1)->getName() ] = (*p1);
	for( pp1 = d_sort2D.begin(); pp1 != d_sort2D.end(); ++pp1 )
	{
        d_popSpec2D->addCommand( (*pp1).first.data(), this, SLOT(handleSelectSpec2D()) )->
                setProperty( "0", QVariant::fromValue((*pp1).second.deref()) );
	}
}

void PolyScope3::updateSpecPop3D()
{
    d_popSpec3D->clear();
	ColorMap a, b;
	d_spec2D->getColors( a );
	Project::SpecSet l;
	Spectrum* spec = 0;
	if( a.size() == 3 )
		a[ DimZ ] = AtomType(); // Joker
	else
		a.push_back( AtomType() );

	if( d_synchro )
	{
		KeyLabels keys( 3 ); // 3, da letzte Dimension Don't care, aber 3D Spec gesucht.
		keys[ DimX ] = d_spec2D->getKeyLabel( DimX );
		keys[ DimY ] = d_spec2D->getKeyLabel( DimY );

		l = d_pro->findSpectra( keys );
	}else
	{
		const Project::SpectrumMap& sm = d_pro->getSpectra();
		Project::SpectrumMap::const_iterator p;
		Rotation rot;
		if( d_orig->getDimCount() == 3 )
			l.insert( d_orig );
		for( p = sm.begin(); p != sm.end(); ++p )
		{
			spec = (*p).second;
			if( spec->getDimCount() == 3 && spec->getId() != d_orig->getId() )
			{
				spec->getColors( b );
				if( a[ DimX ] == b[ DimX ] && a[ DimY ] == b[ DimY ] )
					l.insert( spec );
				else
				{
					// TEST qDebug( "id=%d name=%s", spec->getId(), spec->getName() );
					if( SpectrumType::autoRotate( a, spec, rot, false ) ) // Keine Auflï¿½ungsoptimierung
					{
						l.insert( new SpecRotator( spec, rot ) );
					}
				}
			}
		}
	}
	Project::SpecSet::const_iterator p1;
	Sort::const_iterator pp1;
	for( p1 = l.begin(); p1 != l.end(); ++p1 )
		d_sort3D[ (*p1)->getName() ] = (*p1);
	for( pp1 = d_sort3D.begin(); pp1 != d_sort3D.end(); ++pp1 )
	{
        d_popSpec3D->addCommand( (*pp1).first.data(), this, SLOT(handleSelectSpec3D()) )->
                setProperty( "0", QVariant::fromValue((*pp1).second.deref()) );
	}
}

void PolyScope3::createPlane()
{
	d_plane.d_ol.assign( 1, PlaneSocket::Overlay() );

	d_plane.d_viewer = new SpecViewer( new ViewAreaMdl( true, true, true, true ), VIEWCOUNT );
	d_plane.d_viewer->getViewArea()->addObserver( this );

	d_plane.d_ol[0].d_spec = new SpecProjector( d_spec2D, DimX, DimY );
	d_plane.d_ol[0].d_buf = new SpecBufferMdl( 
		d_plane.d_viewer->getViewArea(), d_plane.d_ol[0].d_spec, false );
	d_plane.d_ol[0].d_buf->setFolding( d_folding, false );

	d_plane.d_intens = new IntensityView( d_plane.d_ol[0].d_buf, false );
	d_plane.d_viewer->getViews()->replace( INTENSITY, d_plane.d_intens );

	d_plane.d_ol[0].d_view = new ContourView( d_plane.d_ol[0].d_buf, d_autoContour );
	Lexi::Layer* l = new Lexi::Layer();
	l->append( d_plane.d_ol[0].d_view );
	d_plane.d_viewer->getViews()->replace( CONTOUR, l );
	d_plane.d_ol[0].d_view->setVisi( s_showContour );
	const Repository::SlotColors& sc = d_pro->getRepository()->getScreenClr();
	if( !sc.empty() )
	{
		d_plane.d_ol[0].d_view->setPosColor( sc[ 0 ].d_pos );
		d_plane.d_ol[0].d_view->setNegColor( sc[ 0 ].d_neg );
	}
	d_plane.d_ol[0].d_view->createLevelsAuto( s_contourFactor, s_contourOption, s_gain );
	
	d_plane.d_cur = new CursorMdl();
	d_plane.d_cur->addObserver( this );
	d_plane.d_mdl3D = new RangeFilterSpace( d_mdl3D, DimZ );
	d_plane.d_tuples = new SpinPointView( d_plane.d_viewer, d_mdl2D );
	d_plane.d_tuples->setLabel( SpinPointView::PairLabelSysOrResi );
	d_plane.d_viewer->getViews()->replace( TUPLES, d_plane.d_tuples );
	CursorView* cv = new CursorView( d_plane.d_viewer, d_plane.d_cur );
	d_plane.d_viewer->getViews()->replace( CURSORS, cv );
	if( d_folding )
		d_plane.d_viewer->getViews()->replace( FOLDING, new FoldingView( d_plane.d_ol[0].d_buf ) );
	d_plane.d_hRuler = new SpinPoint1DView( d_plane.d_viewer, 
		DimY, 0, ( Qt::darkYellow) );
    d_plane.d_hRulerMdl = new ManualSpinSpace( 1 );
	d_plane.d_hRuler->setModel( d_plane.d_hRulerMdl );
	d_plane.d_hRuler->setLabel( SpinPoint1DView::SysTagAll );
	d_plane.d_viewer->getViews()->replace( RULER1, d_plane.d_hRuler );
	d_plane.d_vRuler = new SpinPoint1DView( d_plane.d_viewer, 
		DimX, 0, ( Qt::darkYellow) );
    d_plane.d_vRulerMdl = new ManualSpinSpace( 1 );
	d_plane.d_vRuler->setModel( d_plane.d_vRulerMdl );
	d_plane.d_vRuler->setLabel( SpinPoint1DView::SysTagAll );
	d_plane.d_viewer->getViews()->replace( RULER2, d_plane.d_vRuler );

	d_plane.d_pp = new PeakSubSpace( new PeakSpaceDummy( s_defDim ), DimX, DimY );
	d_plane.d_peaks = new PeakPlaneView( d_plane.d_viewer, d_plane.d_pp );
	d_plane.d_peaks->setColor( g_clrPeak );
	d_plane.d_viewer->getViews()->replace( PEAKS, d_plane.d_peaks );


	d_plane.d_viewer->getHandlers()->append( new ZoomCtrl( d_plane.d_viewer, true, true ) );
	d_plane.d_viewer->getHandlers()->append( new SelectZoomCtrl( d_plane.d_viewer, true, true ) );
	d_plane.d_viewer->getHandlers()->append( new ScrollCtrl( d_plane.d_viewer ) );
	d_plane.d_viewer->getHandlers()->append( new CursorCtrl( cv, false ) );
	d_plane.d_viewer->getHandlers()->append( 
			new PointSelect1DCtrl( d_plane.d_hRuler, false ) );
	d_plane.d_viewer->getHandlers()->append( 
			new PointSelect1DCtrl( d_plane.d_vRuler, false ) );
	// NOTE: Select muss vor Cursor kommen, da sonst Selection zu spï¿½ passiert.
	d_plane.d_viewer->getHandlers()->append( 
		new PointSelectCtrl( d_plane.d_tuples, false ) );
	d_plane.d_viewer->getHandlers()->append( 
		new PeakSelectCtrl( d_plane.d_peaks, false, false ) ); // Kein Drag-Select
	d_plane.d_viewer->getHandlers()->append( new Lexi::ContextMenu( d_popPlane, false ) );
	d_plane.d_viewer->getHandlers()->append( new FocusCtrl( d_plane.d_viewer ) );
}

void PolyScope3::initOverlay(int n)
{
	assert( n > 0 );
	if( d_plane.d_ol.size() == n )
		return;
	Lexi::Glyph* g = d_plane.d_viewer->getViews()->getComponent( CONTOUR );
	while( g->getCount() )
		g->remove( 0 );

	int i;
	if( n > d_plane.d_ol.size() )
	{
		int old = d_plane.d_ol.size();
		d_plane.d_ol.resize( n );
		const Repository::SlotColors& sc = d_pro->getRepository()->getScreenClr();
		for( i = old; i < n; i++ )
		{
			d_plane.d_ol[i].d_buf = new SpecBufferMdl( 
				d_plane.d_viewer->getViewArea() );
			d_plane.d_ol[i].d_buf->setFolding( d_folding );
			d_plane.d_ol[i].d_view = new ContourView( d_plane.d_ol[i].d_buf, d_autoContour );
			if( i < sc.size() )
			{
				d_plane.d_ol[i].d_view->setPosColor( sc[ i ].d_pos );
				d_plane.d_ol[i].d_view->setNegColor( sc[ i ].d_neg );
			}
			d_plane.d_ol[i].d_view->setVisi( d_plane.d_ol[0].d_view->isVisi() );
			d_plane.d_ol[i].d_view->createLevelsAuto( s_contourFactor, s_contourOption, s_gain );
		}
	}else
	{
		d_plane.d_ol.resize( n );
	}
	for( i = 0; i < d_plane.d_ol.size(); i++ )
		g->append( d_plane.d_ol[i].d_view );
	setActiveOverlay( 0 );
	updatePlaneLabel();
}

void PolyScope3::setActiveOverlay(int n)
{
	if( n == d_aol )
		return;
	d_plane.d_viewer->redraw();

	d_aol = n;
	updatePlaneLabel();
}

int PolyScope3::selectLayer()
{
	if( d_plane.d_ol.size() == 1 )
		return 0;

	Dlg::StringList l( d_plane.d_ol.size() + 1);
	l[ 0 ] = "&All";
	QString str;
	for( int i = 0; i < d_plane.d_ol.size(); i++ )
	{
		if( d_plane.d_ol[ i ].d_spec )
			str.sprintf( "&%d %s", i, d_plane.d_ol[ i ].d_spec->getName() );
		else
			str.sprintf( "&%d <empty>", i );
		l[ i + 1 ] = str.toLatin1();
	}
    int c = Dlg::getOption( getQt(), l,
		"Select Layer", d_aol + 1 );
	if( c == -1 )
		return -2;
	else
		return c - 1;
}

void PolyScope3::createStrip(Dimension d)
{
	d_strips[ d ].d_viewer = new SpecViewer( 
		new ViewAreaMdl( true, true, false, true ), VIEWCOUNT );
	d_strips[ d ].d_viewer->getViewArea()->addObserver( this );

	d_strips[ d ].d_buf = new SpecBufferMdl( d_strips[ d ].d_viewer->getViewArea(), 0, false );
	d_strips[ d ].d_buf->setFolding( d_folding, false );

	d_strips[ d ].d_view = new ContourView( d_strips[ d ].d_buf, true );	// Immer auto
	d_strips[ d ].d_viewer->getViews()->replace( BACKGROUND, new Lexi::Background() );
	d_strips[ d ].d_viewer->getViews()->replace( CONTOUR, d_strips[ d ].d_view );
	d_strips[ d ].d_view->createLevelsAuto( s_contourFactor, s_contourOption, s_gain );
	d_strips[ d ].d_viewer->getViews()->replace( INTENSITY, 
		new CenterLine( d_strips[ d ].d_viewer ) );

	d_strips[ d ].d_cur = new CursorMdl();
	d_strips[ d ].d_cur->addObserver( this );
	Rotation rot( 3 );
	const Dimension other = (d==DimX)?DimY:DimX;
	rot[ DimX ] = d;
	rot[ DimY ] = DimZ;
	rot[ DimZ ] = other;
	d_strips[ d ].d_mdl = new RangeFilterSpace( d_mdl3D, other );
	d_strips[ d ].d_tuples = new SpinPointView( d_strips[ d ].d_viewer, 
		new RotatedSpace( d_strips[ d ].d_mdl, rot ) );
	d_strips[ d ].d_tuples->setLabel( SpinPointView::SysOrResiTagAll, DimY );
	d_strips[ d ].d_viewer->getViews()->replace( TUPLES, d_strips[ d ].d_tuples );
	CursorView* cv = new CursorView( d_strips[ d ].d_viewer, 
		d_strips[ d ].d_cur, false, true );
	d_strips[ d ].d_viewer->getViews()->replace( CURSORS, cv );
	if( d_folding )
		d_strips[ d ].d_viewer->getViews()->replace( FOLDING, 
			new FoldingView( d_strips[ d ].d_buf ) );

	d_strips[ d ].d_pp = new PeakSubSpace( new PeakSpaceDummy( s_defDim ), d, DimZ );
	d_strips[ d ].d_peaks = new PeakPlaneView( d_strips[ d ].d_viewer, d_strips[ d ].d_pp );
	d_strips[ d ].d_peaks->setColor( g_clrPeak );
	d_strips[ d ].d_viewer->getViews()->replace( PEAKS, d_strips[ d ].d_peaks );

	d_strips[ d ].d_viewer->getHandlers()->append( new ZoomCtrl( d_strips[ d ].d_viewer, false, true ) );
	d_strips[ d ].d_viewer->getHandlers()->append( new SelectZoomCtrl( d_strips[ d ].d_viewer, false, true ) );
	d_strips[ d ].d_viewer->getHandlers()->append( new ScrollCtrl( d_strips[ d ].d_viewer, false, true ) );
	d_strips[ d ].d_viewer->getHandlers()->append( new CursorCtrl( cv, false, false, true ) );
	d_strips[ d ].d_viewer->getHandlers()->append( new 
		PointSelectCtrl( d_strips[ d ].d_tuples, false ) );
	d_strips[ d ].d_viewer->getHandlers()->append( 
		new PeakSelectCtrl( d_strips[ d ].d_peaks, false, false ) ); // Kein Drag-Select
	d_strips[ d ].d_viewer->getHandlers()->append( new Lexi::ContextMenu( d_popStrip, false ) );
	d_strips[ d ].d_viewer->getHandlers()->append( new FocusCtrl( d_strips[ d ].d_viewer ) );
}

void PolyScope3::createSlice(Dimension view, Dimension spec)
{
	SpecViewer* slice = new SpecViewer( 
		new ViewAreaMdl( view == DimX, view == DimY, view == DimX, view == DimY ) );
	d_slices[ spec ].d_viewer = slice;
	slice->getViewArea()->addObserver( this );

	d_slices[ spec ].d_buf2D = new SpecBufferMdl( slice->getViewArea(), 0, false );
	d_slices[ spec ].d_buf2D->setFolding( d_folding, false );
	slice->getViews()->append( new SliceView( d_slices[ spec ].d_buf2D ) );

	d_slices[ spec ].d_buf3D = new SpecBufferMdl( slice->getViewArea(), 0, false );
	d_slices[ spec ].d_buf3D->setFolding( d_folding, false );
	slice->getViews()->append( new SliceView( d_slices[ spec ].d_buf3D, g_clrSlice3D ) );

	d_slices[ spec ].d_cur = new CursorMdl();
	d_slices[ spec ].d_cur->addObserver( this );
	CursorView* cv = new CursorView( slice, d_slices[ spec ].d_cur );
	slice->getViews()->append( cv );

	slice->getHandlers()->append( new ZoomCtrl( slice, view == DimX, view == DimY ) );
	slice->getHandlers()->append( new SelectZoomCtrl( slice, view == DimX, view == DimY ) );
	slice->getHandlers()->append( new ScrollCtrl( slice ) );
	slice->getHandlers()->append( new SelectRulerCtr( slice, true ) );
	slice->getHandlers()->append( new CursorCtrl( cv, false ) );
	slice->getHandlers()->append( new FocusCtrl( slice ) );
}

void PolyScope3::updateContour( int i, bool redraw )
{
	if( !d_plane.d_ol[0].d_view->isVisi() )
		return;

	if( d_show3DPlane && i == 0 )
	{
		if( d_strips[DimX].d_view->isAuto() )
		{
			d_plane.d_ol[0].d_view->createLevelsAuto( d_strips[DimX].d_view->getFactor(), 
				d_strips[DimX].d_view->getOption(), d_strips[DimX].d_view->getGain() );
		}else
			d_plane.d_ol[0].d_view->createLevelsMin( d_strips[DimX].d_view->getFactor(), 
				(d_spec3D)?d_spec3D->getThreshold():0.0, d_strips[DimX].d_view->getOption() );
	}else
	{
		if( d_plane.d_ol[i].d_view->isAuto() )
		{
			d_plane.d_ol[i].d_view->createLevelsAuto();
		}else if( d_plane.d_ol[i].d_spec )
			d_plane.d_ol[i].d_view->createLevelsMin( d_plane.d_ol[i].d_spec->getThreshold() );
	}
	if( redraw )
		d_plane.d_viewer->damageMe();
}

void PolyScope3::showIntens(bool on )
{
	if( d_plane.d_intens->isVisi() == on )
		return;
	Lexi::Viewport::pushHourglass();
	d_plane.d_intens->setVisi( on );
	Lexi::Viewport::popCursor();
}

void PolyScope3::initParams()
{
	d_resol = 1;
	d_lowResol = false;
	d_autoContour = true;
	d_autoCenter = true;
	d_contourFactor = 1.4f;
	d_contourOption = ContourView::Both;
	d_folding = false;
	d_show3DPlane = false;
	d_autoHide = true;
	d_cursorSync = false;
	d_gain = 2.0;
	d_rangeSync = false;
	d_autoHold = false;
	d_syncDepth = true;
	// TODO: diese Werte sollen ab Konfigurations-Record gelesen werden
}

void PolyScope3::centerToCursor(bool threeD)
{
	if( !threeD )
	{
		ViewAreaMdl* area = d_plane.d_viewer->getViewArea();
		if( !area->getRange( DimX ).contains( d_cursor[ DimX ] ) ||
			!area->getRange( DimY ).contains( d_cursor[ DimY ] ) )
		{
			area->centerPoint( d_cursor[ DimX ], d_cursor[ DimY ] );
			d_plane.d_viewer->damageMe();
		}
	}else if( !d_strips.empty() )
	{
		ViewAreaMdl* area = d_strips[0].d_viewer->getViewArea();
		if( !area->getRange( DimY ).contains( d_cursor[ DimZ ] ) )
		{
			area->centerPointDim( DimY, d_cursor[ DimZ ] );
			d_strips[0].d_viewer->damageMe();
		}
	}
}

void PolyScope3::handle(Root::Message& msg)
{
	if( d_lock )
		return;
	d_lock = true;
	BEGIN_HANDLER();
	MESSAGE( ViewAreaMdl::Update, a, msg )
	{
		Lexi::Viewport::pushHourglass();
		if( a->getOrigin() == d_plane.d_viewer->getViewArea() )
			updatePlane( a );
		else
		{
			Dimension d;
			for( d = 0; d < d_slices.size(); d++ )
				if( d_slices[ d ].d_viewer->getViewArea() == a->getOrigin() )
				{
					updateSlice( d, a );
					break;
				}
			for( d = 0; d < d_strips.size(); d++ )
				if( d_strips[ d ].d_viewer->getViewArea() == a->getOrigin() )
				{
					updateStrip( d, a );
					break;
				}
		}

		Lexi::Viewport::popCursor();
		msg.consume();
	}
	MESSAGE( CursorMdl::Update, a, msg )
	{
		if( a->getOrigin() == d_plane.d_cur )
			updatePlane( a );
		else
		{
			Dimension d;
			for( d = 0; d < d_slices.size(); d++ )
				if( d_slices[ d ].d_cur == a->getOrigin() )
				{
					updateSlice( d, a );
					break;
				}
			for( d = 0; d < d_strips.size(); d++ )
				if( d_strips[ d ].d_cur == a->getOrigin() )
				{
					updateStrip( d, a );
					break;
				}
		}
		msg.consume();
	}
	MESSAGE( GlobalCursor::UpdatePos, a, msg )
	{
		d_lock = false;
		d_cursorSync = false;
		bool threeD = false;
		if( ( a->getDim() == DimY || a->getDim() == DimUndefined ) &&
			d_plane.d_ol[0].d_spec->getColor( DimY ) == a->getTy() )
			d_plane.d_cur->setCursor( Dimension( DimY ), a->getY() );
		if( ( a->getDim() == DimX || a->getDim() == DimUndefined ) &&
			d_plane.d_ol[0].d_spec->getColor( DimX ) == a->getTx() )
			d_plane.d_cur->setCursor( Dimension( DimX ), a->getX() );
		if( d_syncDepth && ( a->getDim() == DimY ) && d_spec3D &&
			d_spec3D->getColor( DimZ ) == a->getTy() )
		{
			d_cursor[ DimZ ] = a->getY();
			if( !d_strips.empty() )
			{
				d_slices[ DimZ ].d_cur->setCursor( (Dimension)DimY, a->getY() );
				d_strips[ DimX ].d_cur->setCursor( (Dimension)DimY, a->getY() );
				d_strips[ DimY ].d_cur->setCursor( (Dimension)DimY, a->getY() );
			}
			threeD = true;
		}
		d_cursorSync = true;
		if( !threeD )
		{
			selectCurSystem();
			if( !d_rangeSync )
				centerToCursor( threeD );
		}else
			centerToCursor( threeD );
		msg.consume();
	}
	MESSAGE( GlobalCursor::UpdateRange, a, msg )
	{
		d_lock = false;
		d_rangeSync = false;
		if( ( a->getDim() == DimY || a->getDim() == DimUndefined ) &&
			d_plane.d_ol[0].d_spec->getColor( DimY ) == a->getTy() )
			d_plane.d_viewer->getViewArea()->setRange( DimY, a->getY() );
		if( ( a->getDim() == DimX || a->getDim() == DimUndefined ) &&
			d_plane.d_ol[0].d_spec->getColor( DimX ) == a->getTx() )
			d_plane.d_viewer->getViewArea()->setRange( DimX, a->getX() );
		d_rangeSync = true;
		msg.consume();
    }MESSAGE( Project::Changed, a, msg )
	{
		msg.consume();
		if( a->d_hint == Project::Width )
		{
			if( d_spec3D )
				d_plane.d_mdl3D->setGhostWidth( d_pro->inferPeakWidth( DimZ, d_spec3D ) );
			if( d_show3DPlane )
				d_plane.d_pp->setRange( DimZ, d_pro->inferPeakWidth( DimZ, d_spec3D ) );
			syncStripsToCur();
		}else if( a->d_hint == Project::WidthFactor )
		{
			syncStripsToCur();
		}
	}
	MESSAGE( SpectrumPeer::Added, a, msg )
	{
        Q_UNUSED(a)
		updateSpecPop2D();
		updateSpecPop3D();
	}
	MESSAGE( SpectrumPeer::Removed, a, msg )
	{
        Q_UNUSED(a)
		updateSpecPop2D();
		updateSpecPop3D();
	}
	MESSAGE( Spin::Update, a, msg )
	{
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
    }MESSAGE( PolyScope3::SpecChanged, e, msg )
    {
        if( d_list && this )
        {
            if( d_use3D )
                d_list->setSpec( getSpec3D() );
            else
                d_list->setSpec( getSpec2D() );
        }
        if( d_ovCtrl && !e->d_threeD )
        {
            d_ovCtrl->getModel()->setSpectrum(
                new SpecProjector( getSpec(), DimX, DimY ) );
            d_ov->redraw();
        }
        msg.consume();
    }HANDLE_ELSE()
        GenericScope::handle( msg );
    END_HANDLER();
	d_lock = false;
}

bool PolyScope3::askToClosePeaklist()
{
	if( d_pl == 0 || 
        ( d_pl && ( d_pl->getId() != 0 || !d_pl->getPeakList()->isDirty() ) ) )
		return true;
    switch( QMessageBox::warning( getQt(), "About to close peaklist",
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

bool PolyScope3::savePeakList()
{
	bool ok;
	QString res = QInputDialog::getText( "Name Peaklist", 
		"Please enter a unique short name:", QLineEdit::Normal, 
        d_pl->getName().data(), &ok, getQt() );
	if( !ok )
		return false;
	if( res.isEmpty() || d_pro->findPeakList( res.toLatin1() ) != 0 )	
	{
        QMessageBox::critical( getQt(), "Save Peaklist",
				"This peaklist name is already used!", "&Cancel" );
		return false;
	}
	d_pl->getPeakList()->setName( res.toLatin1() );
	d_pro->addPeakList( d_pl->getPeakList() );
	d_pl->getPeakList()->clearDirty();
	return true;
}

void PolyScope3::updatePlane(CursorMdl::Update * msg)
{
	// Auf der Plane wurde der Cursor geï¿½dert
	assert( d_slices.size() >= 2 );
	PpmPoint pos( msg->getX(), msg->getY() );

	if( d_autoCenter && msg->getDim() == DimUndefined )
	{
		Spectrum* spec = d_spec2D;
		if( d_spec3D && d_show3DPlane )
			spec = d_spec3D;
		SpinPoint tuple = 
			d_plane.d_tuples->getHit( pos[ DimX ], pos[ DimY ] );
		if( !tuple.isZero() )
		{
			pos[ DimX ] = tuple[ DimX ]->getShift( spec ); 
			pos[ DimY ] = tuple[ DimY ]->getShift( spec ); 
			msg->override( pos[ DimX ], pos[ DimY ] ); 
		}else
		{
			Root::Index peak = d_plane.d_peaks->getHit( pos[ DimX ], pos[ DimY ] );
			if( peak )
			{
				PeakPos p;
				d_plane.d_peaks->getModel()->getPos( peak, p, spec );
				msg->override( p[ DimX ], p[ DimY ] ); 
				pos[ DimX ] = p[ DimX ];
				pos[ DimY ] = p[ DimY ];
				// das geht, da View die Message erst nach Agent erhï¿½t.
			}
		}
	}

	// Der X-Slice zeigt den durch den Y-Cursor der Plane
	// reprï¿½entierten Slice mit Origin Y (und umgekehrt)
	if( msg->getDim() == DimY || msg->getDim() == DimUndefined )
	{
		d_cursor[ DimY ] = pos[ DimY ];
		d_slices[ DimY ].d_cur->setCursor( (Dimension)DimY, pos[ DimY ] );
		d_slices[ DimX ].d_spec2D->setOrigin( DimY, pos[ DimY ] );
		d_slices[ DimX ].d_viewer->redraw();
		sync3dXySliceToCur( DimX, false );
		if( d_cursorSync )
			GlobalCursor::setCursor( DimY, pos[ DimY ], d_plane.d_ol[0].d_spec->getColor( DimY ) );
	}
	if( msg->getDim() == DimX || msg->getDim() == DimUndefined )
	{
		d_cursor[ DimX ] = pos[ DimX ];
		d_slices[ DimX ].d_cur->setCursor( (Dimension)DimX, pos[ DimX ] );
		d_slices[ DimY ].d_spec2D->setOrigin( DimX, pos[ DimX ] );
		d_slices[ DimY ].d_viewer->redraw();
		sync3dXySliceToCur( DimY, false );
		if( d_cursorSync )
			GlobalCursor::setCursor( DimX, pos[ DimX ], d_plane.d_ol[0].d_spec->getColor( DimX ) );
	}
	sync3dZSliceToCur();
	syncStripsToCur();
	notifyCursor();
}

void PolyScope3::updateSlice(Dimension dim, CursorMdl::Update *msg)
{
	// Auf einem Slice wurde der Cursor geï¿½dert
	d_cursor[ dim ] = msg->getX();
	if( dim < 2 )
	{
		// X/Y-Slice gecklickt
		d_plane.d_cur->setCursor( dim, msg->getX() ); // Beide Dims gleich
		syncStripsToCur();
		sync3dZSliceToCur();
		notifyCursor();
		if( d_cursorSync )
			GlobalCursor::setCursor( dim, msg->getX(), 
				d_slices[ dim ].d_spec2D->getColor( DimX ) );
	}else if( d_spec3D && dim == DimZ )
	{
		// Z-Slice geklickt
		// registerPlane();
		sync3dXySliceToCur( DimX, true );
		sync3dXySliceToCur( DimY, true );
		sync2dPlaneSliceToCur();
		d_strips[ DimX ].d_cur->setCursor( (Dimension)DimY, d_cursor[ dim ] );
		d_strips[ DimY ].d_cur->setCursor( (Dimension)DimY, d_cursor[ dim ] );
		notifyCursor( false );
		if( d_cursorSync && d_syncDepth )
			GlobalCursor::setCursor( DimZ, msg->getX(), d_spec3D->getColor( DimZ ) );
	}
}

void PolyScope3::setCursor( PpmPoint p)
{
	if( p.size() == 0 )
	{
		p.assign( d_spec2D->getDimCount(), 0 );
		for( int i = 0; i < p.size(); i++ )
			p[ i ] = d_spec2D->getScale( i ).getIdxN();
	}
	for( Dimension d = 0; d < p.size(); d++ )
		d_cursor[ d ] = p[ d ];
	d_plane.d_cur->setCursor( d_cursor[ DimX ], d_cursor[ DimY ] );
    centerToCursor( false ); // RISK

	sync3dXySliceToCur( DimX, true );
	sync3dXySliceToCur( DimY, true );
	sync2dPlaneSliceToCur();
	if( p.size() == d_cursor.size() && !d_strips.empty() )
	{
		d_slices[ DimZ ].d_cur->setCursor( (Dimension)DimY, d_cursor[ DimZ ] );
		d_strips[ DimX ].d_cur->setCursor( (Dimension)DimY, d_cursor[ DimZ ] );
		d_strips[ DimY ].d_cur->setCursor( (Dimension)DimY, d_cursor[ DimZ ] );
	}
	// TODO: Selection
	notifyCursor( true );
}

void PolyScope3::sync2dPlaneSliceToCur()
{
	if( d_spec3D.isNull() )
		return;
	d_slices[ DimX ].d_spec3D->setOrigin( d_cursor );
	d_slices[ DimY ].d_spec3D->setOrigin( d_cursor );

	d_slices[ DimX ].d_viewer->redraw();
	d_slices[ DimY ].d_viewer->redraw();
	if( d_show3DPlane || d_spec2D->getDimCount() > 2 )
	{
		Lexi::Viewport::pushHourglass();
		d_plane.d_ol[0].d_spec->setOrigin( d_cursor );
		d_plane.d_viewer->redraw();
		Lexi::Viewport::popCursor();
	}
	assert( d_spec3D );
	Lexi::Viewport::pushHourglass();
	PPM w = d_spec3D->getScale( DimZ ).getDelta();
	d_plane.d_mdl3D->setOrigThick( d_cursor[ DimZ ], w );
	if( d_show3DPlane )
		d_plane.d_pp->setOrigThick( DimZ, d_cursor[ DimZ ], w );
	Lexi::Viewport::popCursor();
}

void PolyScope3::updateSlice(Dimension dim, ViewAreaMdl::Update *msg)
{
	// In Slice wurde Ausschnitt geï¿½dert
	if( msg->getType() != ViewAreaMdl::Update::Range )
		return;
	if( dim < 2 )
	{
		// X/Y-Slice verï¿½dert
		d_plane.d_viewer->getViewArea()->setRange( dim, 
			d_slices[ dim ].d_viewer->getViewArea()->getRange( dim ) );
		d_plane.d_viewer->redraw();
		if( d_rangeSync )
			GlobalCursor::setRange( dim, 
				d_slices[ dim ].d_viewer->getViewArea()->getRange( dim ), 
				d_slices[ dim ].d_spec2D->getColor( DimX ) );
	}else if( d_spec3D && dim == DimZ )
	{
		// Z-Slice verï¿½der
		d_strips[ DimX ].d_viewer->getViewArea()->setRange( DimY, 
			d_slices[ DimZ ].d_viewer->getViewArea()->getRange( DimY ) );
		d_strips[ DimX ].d_viewer->redraw();
		d_strips[ DimY ].d_viewer->getViewArea()->setRange( DimY, 
			d_slices[ DimZ ].d_viewer->getViewArea()->getRange( DimY ) );
		d_strips[ DimY ].d_viewer->redraw();
	}
}

void PolyScope3::updatePlane(ViewAreaMdl::Update * msg)
{
	// In Plane wurde Ausschnitt geï¿½dert
	if( msg->getType() != ViewAreaMdl::Update::Range )
		return;
	PpmCube cube;
	cube.assign( d_spec2D->getDimCount(), PpmRange() );
	cube[ DimX ] = d_plane.d_viewer->getViewArea()->getRange( DimX );
	cube[ DimY ] = d_plane.d_viewer->getViewArea()->getRange( DimY );
	d_backward.push_back( std::make_pair( cube, d_cursor ) );

	if( msg->getX() )
	{
		d_slices[ DimX ].d_viewer->getViewArea()->setRange( DimX, 
			d_plane.d_viewer->getViewArea()->getRange( DimX ) );
		d_slices[ DimX ].d_viewer->redraw();
		if( d_rangeSync )
			GlobalCursor::setRange( DimX, 
			d_plane.d_viewer->getViewArea()->getRange( DimX ), 
			d_plane.d_ol[0].d_spec->getColor( DimX ) );
	}
	if( msg->getY() )
	{
		d_slices[ DimY ].d_viewer->getViewArea()->setRange( DimY, 
			d_plane.d_viewer->getViewArea()->getRange( DimY ) );
		d_slices[ DimY ].d_viewer->redraw();
		if( d_rangeSync )
			GlobalCursor::setRange( DimY, 
			d_plane.d_viewer->getViewArea()->getRange( DimY ), 
			d_plane.d_ol[0].d_spec->getColor( DimY ) );
	}
}

void PolyScope3::updateStrip(Dimension dim, ViewAreaMdl::Update *msg)
{
	// In einem Strip wurde Ausschnitt geï¿½dert

	if( d_spec3D.isNull() || msg->getType() != ViewAreaMdl::Update::Range )
		return;
	//registerPlane();

	d_slices[ DimZ ].d_viewer->getViewArea()->setRange( DimY, 
			d_strips[ dim ].d_viewer->getViewArea()->getRange( DimY ) );
	d_slices[ DimZ ].d_viewer->redraw();

	Dimension other = !dim; // Hack zum X/Y-Swap
	d_strips[ other ].d_viewer->getViewArea()->setRange( DimY, 
			d_strips[ dim ].d_viewer->getViewArea()->getRange( DimY ) );
	d_strips[ other ].d_viewer->redraw();
}

void PolyScope3::updateStrip(Dimension dim, CursorMdl::Update *msg)
{
	// In einem Strip wurde Cursor geï¿½dert

	if( d_spec3D.isNull() )
		return;

	PPM pos = msg->getX();
	if( d_autoCenter )
	{
		SpinPoint tuple = 
			d_strips[ dim ].d_tuples->getHit( d_cursor[ dim ], pos );
		if( !tuple.isZero() )
		{
			pos = tuple[ DimY ]->getShift( d_spec3D ); 
			msg->override( pos, pos ); 
		}else if( d_pl && d_pl->getDimCount() > 2 )
		{
			Root::Index peak = d_strips[ dim ].d_peaks->getHit( d_cursor[ dim ], pos );
			if( peak )
			{
				PeakPos p;
				d_strips[ dim ].d_peaks->getModel()->getPos( peak, p, d_spec3D );
				msg->override( p[ DimY ], p[ DimY ] ); 
				pos = p[ DimY ];
				// das geht, da View die Message erst nach Agent erhï¿½t.
			}
		}
	}

	// registerPlane();
	d_cursor[ DimZ ] = pos;
	sync3dXySliceToCur( DimX, true );
	sync3dXySliceToCur( DimY, true );
	sync2dPlaneSliceToCur();
	d_slices[ DimZ ].d_cur->setCursor( (Dimension)DimY, d_cursor[ DimZ ] );
	Dimension other = !dim; // Hack zum X/Y-Swap
	d_strips[ other ].d_cur->setCursor( (Dimension)DimY, d_cursor[ DimZ ] );
	if( d_show3DPlane )
		updateContour( 0, true );
	notifyCursor( false );
	if( d_cursorSync && d_syncDepth )
		GlobalCursor::setCursor( DimY, d_cursor[ DimZ ], d_spec3D->getColor( DimZ ) );
}

void PolyScope3::updatePlaneLabel()
{
	Spectrum* spec = d_plane.d_ol[d_aol].d_spec;
	if( !d_spec3D.isNull() && d_show3DPlane )
		spec = d_spec3D;
	QString str;
	if( d_plane.d_ol.size() > 1 )
	{
		if( spec == 0 )
			str.sprintf( " %d <empty>", d_aol );
		else
			str.sprintf( " %d %s  %s / %s", d_aol, spec->getName(), spec->getDimName( DimX ),
				spec->getDimName( DimY ) );
	}else
		str.sprintf( " %s  %s / %s", spec->getName(), spec->getDimName( DimX ),
			spec->getDimName( DimY ) );
	d_plane.d_viewer->getViews()->replace( LABEL1,
		new Lexi::Label( str, nil, 
		(d_show3DPlane)?g_clrSlice3D:d_plane.d_tuples->getColor(), 
		Lexi::AlignLeft, Lexi::AlignTop ) );
}

void PolyScope3::setSpec3D(Spectrum * spec)
{
	if( d_spec3D == spec )
		return;

	// Ist egal: assert( spec == 0 || d_specList.count( spec ) > 0 ); 
	// Nur vorher evaluierte Spektren zulï¿½sig.

	Lexi::Viewport::pushHourglass();
	Spectrum* old = d_spec3D;
	d_spec3D = spec;
	if( !d_spec3D.isNull() )
	{
		d_slices[ DimZ ].d_spec3D = new SpecProjector( d_spec3D, DimZ );
		d_slices[ DimZ ].d_spec3D->setOrigin( d_cursor );

		d_strips[ DimX ].d_spec = new SpecProjector( d_spec3D, DimX, DimZ );
		d_strips[ DimY ].d_spec = new SpecProjector( d_spec3D, DimY, DimZ );

		d_slices[ DimZ ].d_buf3D->setSpectrum( d_slices[ DimZ ].d_spec3D, true );
		d_slices[ DimZ ].d_spec2D = 0;
		d_slices[ DimZ ].d_buf2D->setSpectrum( 0 );

		if( old == 0 || old->getColor( DimZ ) != spec->getColor( DimZ ) ||
			!old->getScale( DimZ ).getRange().intersects( spec->getScale( DimZ ).getRange() ) )
		{
			// Nur FitWindow, wenn andere Farbe oder Bereich nicht berlappend
			PpmRange r = d_spec3D->getScale( DimZ ).getRange();	
			r.invert();
			d_strips[ DimX ].d_viewer->getViewArea()->setRange( DimY, r );
			d_strips[ DimY ].d_viewer->getViewArea()->setRange( DimY, r );
			d_slices[ DimZ ].d_viewer->getViewArea()->setRange( DimY, r );
		}

		d_mdl3D->setSpec( 0 );
		d_src3D->setSpec( d_spec3D );
		d_mdl3D->setSpec( d_spec3D );
		d_plane.d_mdl3D->setGhostWidth( d_pro->inferPeakWidth( DimZ, d_spec3D ) );
		syncStripsToCur();

		d_strips[ DimX ].d_viewer->getViews()->replace( LABEL1,
			new Lexi::Label( d_spec3D->getName(), nil, g_clrSlice3D, 
			Lexi::AlignLeft, Lexi::AlignTop ) );
		updateStripLabelPop();
	}else
	{
		d_slices[ DimZ ].d_spec3D = 0;
		d_strips[ DimX ].d_spec = 0;
		d_strips[ DimY ].d_spec = 0;
		d_slices[ DimZ ].d_buf3D->setSpectrum( 0 );
		if( d_spec2D->getDimCount() > 2 )
		{
			d_slices[ DimZ ].d_spec2D = new SpecProjector( d_spec2D, DimZ );
			d_slices[ DimZ ].d_buf2D->setSpectrum( d_slices[ DimZ ].d_spec2D );
			d_slices[ DimZ ].d_buf2D->fitToArea();
		}
		d_strips[ DimX ].d_viewer->getViews()->replace( LABEL1, 0 );
		d_mdl3D->setSpec( 0 );
		d_src3D->setSpecType( 0 );
		if( d_popLabel )
		{
            d_popLabel->clear();
            d_popLabel->addCommand("?", this, SLOT(handleLabelSpin3D()) )->setProperty( "0", "?" );
            d_popLabel->addCommand( "?-1", this, SLOT(handleLabelSpin3D()) )->setProperty( "0", "?-1" );
        }
	}

	d_slices[ DimZ ].d_viewer->redraw();
	d_strips[ DimX ].d_buf->setSpectrum( d_strips[ DimX ].d_spec );
	d_strips[ DimY ].d_buf->setSpectrum( d_strips[ DimY ].d_spec );
	syncStripWidth();

	if( d_show3DPlane )
	{
		update3dPlane();
		updatePlaneLabel();
	}

	sync3dXySliceToCur( DimX, true );
	sync3dXySliceToCur( DimY, true );
	selectCurSystem( true );
	Lexi::Viewport::popCursor();
	SpecChanged msg( true );
    getAgent()->traverseUp( msg );
}

void PolyScope3::updateStripLabelPop()
{
	if( d_popLabel )
	{
        d_popLabel->clear();
        d_popLabel->addCommand("?", this, SLOT(handleLabelSpin3D()) )->setProperty( "0", "?" );
        d_popLabel->addCommand( "?-1", this, SLOT(handleLabelSpin3D()) )->setProperty( "0", "?-1" );
        const SpinLabelSet& sls = d_spec3D->getType()->getLabels( d_spec3D->mapToType( DimZ ) );
		SpinLabelSet::const_iterator p1;
		typedef std::map<QByteArray ,SpinLabel> Sort;
		Sort sort;
		Sort::const_iterator q1;
		for( p1 = sls.begin(); p1 != sls.end(); ++p1 )
		{
			sort[ (*p1).data() ] = (*p1);
		}
		for( q1 = sort.begin(); q1 != sort.end(); ++q1 )
		{
            d_popLabel->addCommand( (*q1).first.data(), this, SLOT(handleLabelSpin3D()) )
                    ->setProperty( "0", (*q1).second.data() );
		}
	}
}

void PolyScope3::stepSpec2D(bool next)
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
	assert( p != d_sort2D.end() );
	setSpec2D( (*p).second );
}

void PolyScope3::stepSpec3D(bool next)
{
	if( d_sort3D.size() < 2 )
		return;
	Sort::const_iterator p = d_sort3D.end();
	if( d_spec3D )
		p = d_sort3D.find( d_spec3D->getName() );
	if( p == d_sort3D.end() )
	{
		if( next )
			p = d_sort3D.begin();
		else
			--p;
	}else
	{
		if( next )
		{
			++p;
			if( p == d_sort3D.end() )
				p = d_sort3D.begin();
		}else if( p == d_sort3D.begin() )
		{
			p = d_sort3D.end();
			--p;
		}else
			--p;
	}
	assert( p != d_sort3D.end() );
	setSpec3D( (*p).second );
}

void PolyScope3::setSpec2D(Spectrum * spec )
{
	if( d_spec2D == spec )
		return;
	Lexi::Viewport::pushHourglass();
	d_spec2D = spec;

	d_plane.d_ol[0].d_spec = new SpecProjector( d_spec2D, DimX, DimY );
	d_plane.d_ol[0].d_buf->setSpectrum( d_plane.d_ol[0].d_spec );

	d_mdl2D->setSpec( 0 );
	d_src2D->setSpec( d_spec2D );
	d_mdl2D->setSpec( d_spec2D );
	if( d_spec2D->getDimCount() > 2 )
	{
		d_plane.d_tuples->setModel( d_plane.d_mdl3D );
		d_plane.d_ol[0].d_spec->setOrigin( d_cursor );
	}else
		d_plane.d_tuples->setModel( d_mdl2D );

	d_plane.d_vRulerMdl->setSpec( d_plane.d_ol[0].d_spec );
	d_plane.d_hRulerMdl->setSpec( d_plane.d_ol[0].d_spec );

	d_slices[ DimX ].d_spec2D = new SpecProjector( d_spec2D, DimX );
	d_slices[ DimX ].d_buf2D->setSpectrum( d_slices[ DimX ].d_spec2D );
	d_slices[ DimX ].d_buf2D->setFolding( d_folding );
	d_slices[ DimY ].d_spec2D = new SpecProjector( d_spec2D, DimY );
	d_slices[ DimY ].d_buf2D->setSpectrum( d_slices[ DimY ].d_spec2D );
	d_slices[ DimY ].d_buf2D->setFolding( d_folding );
	d_plane.d_peaks->setSpec( d_spec2D );

	if( d_spec2D->getDimCount() > 2 )
	{
		d_slices[ DimX ].d_spec2D->setOrigin( d_cursor );
		d_slices[ DimY ].d_spec2D->setOrigin( d_cursor );
	}else
	{
		d_slices[ DimX ].d_spec2D->setOrigin( DimY, d_cursor[ DimY ] );
		d_slices[ DimY ].d_spec2D->setOrigin( DimX, d_cursor[ DimX ] );
	}

	updateContour( 0, true );
	updatePlaneLabel();
	d_plane.d_viewer->redraw();
	Lexi::Viewport::popCursor();
	SpecChanged msg( false );
    getAgent()->traverseUp( msg );
}

void PolyScope3::setPeakList(PeakList * pl)
{
	if( pl )
	{
		d_pl = new PeakListPeer( pl );
		d_plane.d_pp->setPeakSpace( d_pl );
		if( !d_strips.empty() )
		{
			d_strips[DimX].d_pp->setPeakSpace( d_pl );
			d_strips[DimY].d_pp->setPeakSpace( d_pl );
		}
		QString str;
		str.sprintf( "%dD %s", d_pl->getDimCount(), pl->getName().data() );
		d_plane.d_viewer->getViews()->replace( LABEL3,
			new Lexi::Label( str, nil, 
			d_plane.d_peaks->getColor(), 
			Lexi::AlignLeft, Lexi::AlignBottom ) );
	}else
	{
		d_pl = 0;
		d_plane.d_pp->setPeakSpace( new PeakSpaceDummy( s_defDim ) );
		if( !d_strips.empty() )
		{
			d_strips[DimX].d_pp->setPeakSpace( new PeakSpaceDummy( s_defDim ) );
			d_strips[DimY].d_pp->setPeakSpace( new PeakSpaceDummy( s_defDim ) );
		}
		d_plane.d_viewer->getViews()->replace( LABEL3, 0 );
	}
}

void PolyScope3::extendSystem(Dimension source, Dimension target )
{
	// Ausser spec2D identisch mit HomoScope
	Spin* ref = 0;
	if( d_ref[ source ] != 0 )
		ref = d_ref[ source ];
	else if( d_plane.d_tuples->getSel().size() == 1 )
		ref = ( *d_plane.d_tuples->getSel().begin() )[ source ];
	else
	{
		// Der User kann Extend auch ausfhren, wenn kein Peak selektiert wurde.
		// In diesem Fall schlagen wir Peaks in der Region der Cursordimension vor.
        ProposeSpinDlg dlg( getQt(), d_pro, d_spec2D->getColor( source ), d_cursor[ source ],
			d_spec2D,	"Select Reference Spin" );
		dlg.setAnchor( source, ref );
		if( !dlg.exec() || dlg.getSpin() == 0 )
			return;
		ref = dlg.getSpin();
	}
	pickSpin( target, ref, ref->getSystem() );
}

void PolyScope3::pickSpin(Dimension d, Spin *other, SpinSystem *owner)
{
	SpinLabel l = d_spec2D->getKeyLabel( d );

	// 26.6.05: immer anzeigen if( !d_src2D->showNulls() )
	{
		SpinLabelSet ly = d_spec2D->getType()->getLabels( d_spec2D->mapToType( d ) );
			// Ich lasse das vorerst so, da nicht sicher ist, ob Inference Keys enthï¿½t.

		NmrExperiment* e = d_pro->getRepository()->getTypes()->
			inferExperiment2( d_spec2D->getType(), owner, d_spec2D );
		if( e )
			e->getColumn( d_spec2D->mapToType( d ), ly, owner );
		if( owner )
			ly = owner->getAcceptables( ly );

        if( !Dlg::getLabel( getQt(), l, ly ) )
			return;
		if( owner && !owner->isAcceptable( l ) )
		{
			Root::ReportToUser::alert( this, "Pick Label", "Label is not acceptable" );
			return;
		}
	}

	Root::Ref<PickSystemSpinLabelCmd> cmd = new PickSystemSpinLabelCmd( d_pro->getSpins(), 
		owner, d_spec2D->getColor( d ), d_cursor[ d ], l, 0 ); 
    if( cmd->handle( getAgent() ) )
	{
		if( l.isNull() )
			d_src2D->showNulls( true );
		d_plane.d_tuples->selectPeak( d_cursor[ DimX ], d_cursor[ DimY ] );
	}
}

Spin* PolyScope3::getSelectedSpin()
{
	Dimension ref = DimX;
	if(	d_strips[ DimX ].d_viewer->hasFocus() )
		ref = DimX;
	else if( d_strips[ DimY ].d_viewer->hasFocus() )
		ref = DimY;
	if( d_strips[ ref ].d_tuples->getSel().size() != 1 )
		return 0;
	return ( *d_strips[ ref ].d_tuples->getSel().begin() )[ DimY ];
}

void PolyScope3::gotoTuple(SpinSystem * sys, Spin * spin, Spin * link, bool twoD )
{
	SpinPoint tuple;
	Dimension dim;
	SpinSpace::Result tuples;
	if( !twoD && d_spec3D )
	{
		if( link )
			d_mdl3D->find( tuples, spin, link );
		else if( spin )
			d_mdl3D->find( tuples, spin );
		else
			d_mdl3D->find( tuples, sys );
		dim = 3;
	}else
	{
		if( link )
			d_mdl2D->find( tuples, spin, link );
		else if( spin )
			d_mdl2D->find( tuples, spin );
		else
			d_mdl2D->find( tuples, sys );
		dim = 2;
	}
	if( tuples.empty() )
	{
		Lexi::ShowStatusMessage msg( "Goto peak: element not found in inferred base" );
        getAgent()->traverseUp( msg );
		return;
	}
	if( tuples.hasOne() || d_synchro )
	{
		tuple = tuples.first().d_point;
	}else
	{
        tuple = Dlg::selectTuple( getQt(), tuples,
			dim, "Select Spin Tuple" );
		if( tuple.isZero() )
			return;
	}
	PpmPoint orig;
	for( Dimension d = 0; d < dim; d++ )
		orig.push_back( tuple[ d ]->getShift(  
			( dim == 3 )?d_spec3D:d_spec2D ) );
	bool ac = d_autoCenter;
	d_autoCenter = false;
	d_plane.d_cur->setCursor( orig[ DimX ], orig[ DimY ] );
	d_plane.d_tuples->select( tuple );
	if( dim == 3 )
	{
		d_cursor[ DimZ ] = orig[ DimZ ];
		sync3dXySliceToCur( DimX, true );
		sync3dXySliceToCur( DimY, true );
		sync2dPlaneSliceToCur();
		d_strips[ DimX ].d_cur->setCursor( (Dimension)DimY, orig[ DimZ ] );
		d_strips[ DimY ].d_cur->setCursor( (Dimension)DimY, orig[ DimZ ] );
	}
	// TODO: select in Plane and Strips
	selectCurSystem();
	centerToCursor();
	if( dim == 3 )
	{
		ViewAreaMdl* area = d_strips[ DimX ].d_viewer->getViewArea();
		if( !area->getRange( DimY ).contains( d_cursor[ DimZ ] ) )
		{
			area->centerPoint( area->getRange( DimX ).getCenter(), d_cursor[ DimZ ] );
			d_strips[ DimX ].d_viewer->damageMe();
		}
	}
	notifyCursor( dim == 2 );
	d_autoCenter = ac;
}

Spin* PolyScope3::getSel(bool hori) const
{
	// Kopie von HomoScope
	if( d_plane.d_tuples->getSel().size() != 1 )
		return 0;

	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	if( hori )
		return tuple[ DimX ];
	else
		return tuple[ DimY ];
}

void PolyScope3::notifyCursor(bool plane)
{
	QString str;
    QTextStream ts( &str, QIODevice::WriteOnly );

	ts.setf( QTextStream::fixed );
	ts.precision( 3 );

	ts <<  "Cursor:  ";

	Spectrum* spec = 0;
	if( plane )
		spec = d_spec2D;
	else
		spec = d_spec3D;

	for( Dimension d = 0; d < spec->getDimCount(); d++ )
	{
		ts << char( 'x' + d ) << ": ";
		// ts << spec->getColor( d ).getLabel() << "=";
		ts << spec->getDimName( d ) << "=";	// wegen Fred
		ts << d_cursor[ d ];
		if( d_folding )
			ts << " (" << spec->getScale( d ).getRangeOffset( d_cursor[ d ] ) << ")  ";
		else
			ts << "  ";
	}

	try
	{
		Amplitude val = 0;
		if( plane )
		{
			PpmPoint p;
			for( Dimension d = 0; d < d_spec2D->getDimCount(); d++ )
				p.push_back( d_cursor[ d ] );
			val = d_spec2D->getAt( p, d_folding, d_folding );
		}else 
			val = d_spec3D->getAt( d_cursor, d_folding, d_folding );
		ts.setf( QTextStream::showpos );
		ts.precision( 0 );
		ts << "Level=" << val;
	}catch( ... )
	{
		ts << " Out of Spectrum";
	}
	QByteArray  tmp;
	SpinPointView* tv = 0;
	if( plane )
		tv = d_plane.d_tuples;
	else if( d_strips[ DimX ].d_viewer->hasFocus() )
		tv = d_strips[ DimX ].d_tuples;
	else if( d_strips[ DimY ].d_viewer->hasFocus() )
		tv = d_strips[ DimY ].d_tuples;
	if( tv && tv->formatSelection( tmp, SpinPointView::PairAll, 3 ) )
	{
		str += ",  ";
		str += tmp.data();
	}
	Lexi::ShowStatusMessage msg( str );
    getAgent()->traverseUp( msg );
}

void PolyScope3::sync3dZSliceToCur()
{
	if( d_spec3D.isNull() )
		return;

	d_slices[ DimZ ].d_spec3D->setOrigin( d_cursor );
	d_slices[ DimZ ].d_viewer->redraw();
}

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

void PolyScope3::syncStripWidth()
{
	if( d_spec3D.isNull() )
		return;
	PpmRange r = d_spec2D->getScale( DimX ).getRange();	
	PPM w = d_pro->inferStripWidth( DimX, d_spec3D );
	_allocate( r, d_cursor[ DimX ], w );
	d_strips[ DimX ].d_viewer->getViewArea()->setRange( DimX, r );
	w = d_pro->inferPeakWidth( DimX, d_spec3D );
    d_strips[ DimY ].d_mdl->setGhostWidth( w );
	d_strips[ DimY ].d_pp->setRange( DimX, w );

	r = d_spec2D->getScale( DimY ).getRange();
	w = d_pro->inferStripWidth( DimY, d_spec3D );
	_allocate( r, d_cursor[ DimY ], w );
	d_strips[ DimY ].d_viewer->getViewArea()->setRange( DimX, r );
	w = d_pro->inferPeakWidth( DimY, d_spec3D );
    d_strips[ DimX ].d_mdl->setGhostWidth( w );
	d_strips[ DimX ].d_pp->setRange( DimY, w );

	d_strips[ DimX ].d_viewer->redraw();
	d_strips[ DimY ].d_viewer->redraw();
}

void PolyScope3::syncStripsToCur()
{
	if( d_spec3D.isNull() )
		return;
	d_lock = true;

	d_strips[ DimX ].d_spec->setOrigin( d_cursor );
	d_strips[ DimY ].d_spec->setOrigin( d_cursor );

    d_strips[ DimX ].d_mdl->setOrigThick( d_cursor[ DimY ],
		d_spec3D->getScale( DimY ).getDelta(), true );	// RISK
    d_strips[ DimY ].d_mdl->setOrigThick( d_cursor[ DimX ],
		d_spec3D->getScale( DimX ).getDelta(), true );

	d_strips[ DimX ].d_pp->setOrigThick( DimY, d_cursor[ DimY ], 
		d_spec3D->getScale( DimY ).getDelta() );
	d_strips[ DimY ].d_pp->setOrigThick( DimX, d_cursor[ DimX ], 
		d_spec3D->getScale( DimX ).getDelta() );

	syncStripWidth();
	selectCurSystem();

	d_lock = false;
}

void PolyScope3::sync3dXySliceToCur( Dimension dim, bool show )
{
	assert( dim == DimX || dim == DimY );

	Dimension d = dim; // TODO * 2;

	if( ( !d_autoHide || show ) && !d_spec3D.isNull() )
	{
		// RISK: wieso? if( d_slices[ dim ].d_spec3D.isNull() )
			d_slices[ dim ].d_spec3D = new SpecProjector( d_spec3D, d );
		d_slices[ dim ].d_spec3D->setOrigin( d_cursor );
	}else
	{
		d_slices[ dim ].d_spec3D = 0;
	}
	if( d_spec2D->getDimCount() > 2 )
		d_slices[ dim ].d_spec2D->setOrigin( d_cursor );

	d_slices[ dim ].d_buf3D->setSpectrum( d_slices[ dim ].d_spec3D );
	d_slices[ dim ].d_viewer->redraw();
}

void PolyScope3::updateContour2( Dimension d, bool ac )
{
	if( ac )
	{
		d_strips[d].d_view->createLevelsAuto();
	}else
		d_strips[d].d_view->createLevelsMin( (d_spec3D)?d_spec3D->getThreshold():0.0 );
	d_strips[d].d_viewer->damageMe();
}

void PolyScope3::updateRef()
{
	if( !d_ref.isZero() )
	{
		char buf[ 64 ];
		d_plane.d_tuples->formatLabel( buf, sizeof( buf ), d_ref );
		QString str;
		str.sprintf( "Reference: %s", buf );
		d_plane.d_viewer->getViews()->replace( LABEL2,
			new Lexi::Label( str, nil, g_clrLabel, 
				Lexi::AlignRight, Lexi::AlignTop ) );
	}else
		d_plane.d_viewer->getViews()->replace( LABEL2, 0 );
	d_plane.d_viewer->redraw(); // TODO: eingrenzen
}

void PolyScope3::selectCurSystem( bool force )
{
	if( d_spec3D && ( d_plane.d_tuples->getSel().size() == 1 ||
		!d_ref.isZero() ) )
	{
		SpinPoint tuple;
		if( !d_ref.isZero() )
			tuple = d_ref;
		else
			tuple = *d_plane.d_tuples->getSel().begin();
		if( !force && d_cur == tuple )
			return; // Bereits korrekt
		d_cur = tuple;

		// H, N
		// H, N, C
		QString str;
		if( d_synchro && d_cur[0]->getSystem() )
			str.sprintf( " Sys. %d", d_cur[0]->getSystem()->getId() );
		else
		{
			char buf[32];
			SpinPointView::formatLabel( buf, sizeof(buf), d_cur, 
				SpinPointView::PairIdLabelSysOrResi, DimY );
			str = buf;
		}
		d_strips[ DimY ].d_viewer->getViews()->replace( LABEL1,
			new Lexi::Label( str, nil, g_clrSlice3D, 
			Lexi::AlignLeft, Lexi::AlignTop ) );

			str.sprintf( " %s/%s", 
				d_spec3D->getDimName( DimX ), d_spec3D->getDimName( DimZ ) );

		d_strips[ DimX ].d_viewer->getViews()->replace( LABEL2,
			new Lexi::Label( str, nil, g_clrSlice3D, 
			Lexi::AlignLeft, Lexi::AlignBottom ) );
		str.sprintf( " %s/%s", 
			d_spec3D->getDimName( DimY ), d_spec3D->getDimName( DimZ ) );
		d_strips[ DimY ].d_viewer->getViews()->replace( LABEL2,
			new Lexi::Label( str, nil, g_clrSlice3D, 
			Lexi::AlignLeft, Lexi::AlignBottom ) );

		d_strips[ DimX ].d_mdl->setSys( d_cur[ DimX ]->getSystem() );
		d_strips[ DimY ].d_mdl->setSys( d_cur[ DimY ]->getSystem() );
	}else
	{
		d_cur.zero();
		if( !d_strips.empty() )
		{
			d_strips[ DimY ].d_viewer->getViews()->replace( LABEL1, 0 );
			d_strips[ DimX ].d_viewer->getViews()->replace( LABEL2, 0 );
			d_strips[ DimY ].d_viewer->getViews()->replace( LABEL2, 0 );
			d_strips[ DimX ].d_mdl->setSys( 0 );
			d_strips[ DimY ].d_mdl->setSys( 0 );
		}
	}
}

void PolyScope3::handleSetResolution()
{
    ENABLED_IF( true );

	bool ok	= FALSE;
	int res	= QInputDialog::getInteger( "Set Resolution", 
		"Please	enter the minimal number of pixels per sample:", 
        d_resol, 1, 20, 1,	&ok, getQt() );
	if( ok )
	{
		d_resol = res;
		d_lowResol = true;
		Viewport::pushHourglass();
		d_plane.d_ol[0].d_buf->setResolution( d_resol );
		d_plane.d_viewer->damageMe();
		Viewport::popCursor();
    }
}

void PolyScope3::onListActivate()
{
    gotoTuple( d_list->getSelectedStrip(), d_list->getSelectedSpin(),
        d_list->getSelectedLink(), !d_goto3D );
    d_widget->setFocusGlyph( getPlane().d_viewer->getController() );
}

void PolyScope3::handleShowLowRes()
{
    CHECKED_IF( true, d_lowResol );

	Viewport::pushHourglass();
	d_lowResol = !d_lowResol;
	if( d_lowResol )
		d_plane.d_ol[0].d_buf->setResolution( d_resol );
	else
		d_plane.d_ol[0].d_buf->setScaling( false );
	d_plane.d_viewer->damageMe();
	Viewport::popCursor();
}

void PolyScope3::handleForward()
{
	return;
    ENABLED_IF( false ); // TODO d_forward.size()	> 0 );

	Viewport::pushHourglass();
	d_backward.push_back( d_forward.back() );
	const PpmCube& cube = d_forward.back().first;
	d_cursor = d_backward.back().second;
	d_plane.d_viewer->getViewArea()->setRange( DimX, cube[ DimX ] );
	d_plane.d_viewer->getViewArea()->setRange( DimY, cube[ DimY ] );
	d_plane.d_cur->setCursor( d_cursor[ DimX ], d_cursor[ DimY ] );
	for( Dimension d = 0; d < d_slices.size(); ++d )
	{
		d_slices[ d ].d_cur->setCursor( (Dimension)DimY, d_cursor[ d ] );
		d_slices[ d ].d_viewer->getViewArea()->setRange( 
			d_slices[ d ].d_viewer->getViewArea()->getDim(), cube[ d ] );
		d_slices[ d ].d_viewer->redraw();
	}
	d_forward.pop_back();
	d_plane.d_viewer->damageMe();
	Viewport::popCursor();
}

void PolyScope3::handleBackward()
{
	return;
    ENABLED_IF( false ); // TODO d_backward.size() > 1 );

	Viewport::pushHourglass();
	d_forward.push_back( d_backward.back() );
	d_backward.pop_back();
	const PpmCube& cube = d_backward.back().first;
	d_cursor = d_forward.back().second;
	d_plane.d_viewer->getViewArea()->setRange( DimX, cube[ DimX ] );
	d_plane.d_viewer->getViewArea()->setRange( DimY, cube[ DimY ] );
	d_plane.d_cur->setCursor( d_cursor[ DimX ], d_cursor[ DimY ] );
	for( Dimension d = 0; d < d_slices.size(); ++d )
	{
		d_slices[ d ].d_cur->setCursor( (Dimension)DimY, d_cursor[ d ] );
		d_slices[ d ].d_viewer->getViewArea()->setRange( 
			d_slices[ d ].d_viewer->getViewArea()->getDim(), cube[ d ] );
		d_slices[ d ].d_viewer->redraw();
	}
	d_backward.pop_back();
	d_plane.d_viewer->damageMe();
	Viewport::popCursor();
}

void PolyScope3::handleFitWindow()
{
    ENABLED_IF( true );
	d_plane.d_ol[0].d_buf->fitToArea();
	/*
	d_autoContour = true;	// Wegen Pascal
	updateContour( 0, true );
	*/
	d_plane.d_viewer->damageMe();
	for( Dimension d = 0; d < d_slices.size(); ++d )
	{
		d_slices[ d ].d_buf2D->fitToArea();
		d_slices[ d ].d_viewer->redraw();
	}
}

void PolyScope3::handleShowFolded()
{
    CHECKED_IF( true, d_folding );

	Viewport::pushHourglass();
	d_folding = !d_folding;
	for( int i = 0; i < d_plane.d_ol.size(); i++ )
		d_plane.d_ol[i].d_buf->setFolding( d_folding );
	if( d_folding )
		d_plane.d_viewer->getViews()->replace( FOLDING, new FoldingView( d_plane.d_ol[0].d_buf ) );
	else
		d_plane.d_viewer->getViews()->replace( FOLDING, 0 );
	d_plane.d_viewer->damageMe();
	Dimension d;
	for( d = 0; d < d_slices.size(); ++d )
	{
		d_slices[ d ].d_buf2D->setFolding( d_folding );
		d_slices[ d ].d_buf3D->setFolding( d_folding );
		d_slices[ d ].d_viewer->redraw();
	}
	for( d = 0; d < d_strips.size(); d++ )
	{
		d_strips[ d ].d_buf->setFolding( d_folding );
		if( d_folding )
			d_strips[ d ].d_viewer->getViews()->replace( FOLDING, 
				new FoldingView( d_strips[ d ].d_buf ) );
		else
			d_strips[ d ].d_viewer->getViews()->replace( FOLDING, 0 );
		d_strips[ d ].d_viewer->redraw();
	}
	Viewport::popCursor();
}

void PolyScope3::handleSpecCalibrate()
{
    ENABLED_IF( d_plane.d_tuples->getSel().size() == 1 );

	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();

	Spectrum* spec = d_spec2D;
	if( d_spec3D && d_show3DPlane )
		spec = d_spec3D;

	PpmPoint p( 0, 0 );
	for( Dimension d = 0; d < 2; d++ )
		p[ d ] = tuple[ d ]->getShift( spec ) - d_cursor[ d ];

	Viewport::pushHourglass();
	Root::Ref<SpecCalibrateCmd> cmd = new SpecCalibrateCmd( spec, p );
    cmd->handle( getAgent() );
	Viewport::popCursor();
}

void PolyScope3::handleAutoCenter()
{
    CHECKED_IF( true, d_autoCenter );

	d_autoCenter = !d_autoCenter;
}

void PolyScope3::handleShowContour()
{
    CHECKED_IF( true, d_plane.d_ol[0].d_view->isVisi() );

	bool visi = !d_plane.d_ol[0].d_view->isVisi();
	for( int i = 0; i < d_plane.d_ol.size(); i++ )
		d_plane.d_ol[i].d_view->setVisi( visi );
	if( d_plane.d_ol[0].d_view->isVisi() )
		d_plane.d_viewer->getViews()->replace( BACKGROUND, new Lexi::Background() );
	else
		d_plane.d_viewer->getViews()->replace( BACKGROUND, 0 );
	updateContour( 0, true );
}

void PolyScope3::handleShowIntensity()
{
    CHECKED_IF( true, d_plane.d_intens->isVisi() );

	showIntens( !d_plane.d_intens->isVisi() );
}

void PolyScope3::handleAutoContour()
{
    CHECKED_IF( ( d_show3DPlane && d_aol == 0 ) || d_plane.d_ol[d_aol].d_spec,
        ( d_show3DPlane && d_strips[ DimX ].d_view->isAuto() ) ||
        ( !d_show3DPlane && d_plane.d_ol[d_aol].d_view->isAuto() ) );
	
	if( d_show3DPlane && d_aol == 0 )
        handleAutoContour2();
	else
	{
		if( d_plane.d_ol[d_aol].d_view->isAuto() )
			d_plane.d_ol[d_aol].d_view->createLevelsMin( d_plane.d_ol[d_aol].d_spec->getThreshold() );
		else
			d_plane.d_ol[d_aol].d_view->createLevelsAuto();
		d_plane.d_viewer->redraw();
	}
}

void PolyScope3::handleContourParams()
{
    ENABLED_IF( ( d_show3DPlane && d_aol == 0 ) || d_plane.d_ol[d_aol].d_spec );

	if( d_show3DPlane && d_aol == 0 )
	{
        handleContourParams2();
		return;
	}
	Dlg::ContourParams p;
	p.d_factor = d_plane.d_ol[d_aol].d_view->getFactor();
	p.d_threshold =	d_plane.d_ol[d_aol].d_spec->getThreshold();
	p.d_option = d_plane.d_ol[d_aol].d_view->getOption();
    if( Dlg::setParams( getQt(), p ) )
	{
		d_plane.d_ol[d_aol].d_spec->setThreshold( p.d_threshold );
		d_plane.d_ol[d_aol].d_view->setOption( p.d_option );
		d_plane.d_ol[d_aol].d_view->setFactor( p.d_factor );
		d_plane.d_ol[d_aol].d_view->setVisi( true );
		showIntens( false );
		d_plane.d_ol[d_aol].d_view->createLevelsMin( d_plane.d_ol[d_aol].d_spec->getThreshold() );
		d_plane.d_viewer->damageMe();
	}
}

void PolyScope3::handlePickSystem()
{
    ENABLED_IF( true );

	PpmPoint p( d_cursor[ DimX ], d_cursor[ DimY ] );
	bool ok = true;
	if( !d_show3DPlane && d_spec2D->getDimCount() == 2 && !d_synchro )
	{
		// Kann kein System picken in einem 3D ohne dritte Dim auch zu picken.
		/* 26.6.05: ab nun immer Dialog
		if( d_src2D->showNulls() )
		{
			Root::Ref<CreateSystemPairCmd> c1 = 
				new CreateSystemPairCmd( d_pro->getSpins(),	p, d_spec2D );
			ok = c1->handle( this ); // Darstellung von ? mï¿½lich.
		}else
		*/
		{
			Dlg::LP lp;
			// Darstellung von ? nicht mï¿½lich. Picke mit Label.
			lp.d_x = d_spec2D->getKeyLabel( DimX );
			lp.d_y = d_spec2D->getKeyLabel( DimY );

            if( !Dlg::getLabelsSysType( getQt(), lp, d_pro->getRepository(), d_spec2D->getType(),
				d_spec2D->mapToType( DimX ), d_spec2D->mapToType( DimY ) ) )
				return;
			Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Pick System" );
			Root::Ref<CreateSystemPairCmd> c1 = 
				new CreateSystemPairCmd( d_pro->getSpins(),	p, d_spec2D, lp.d_sys );
			c1->execute();
			cmd->add( c1 );
			if( !c1->getSpin( DimX )->getLabel().equals( lp.d_x ) )
				cmd->add( new LabelSpinCmd( d_pro->getSpins(), c1->getSpin( DimX ), lp.d_x ) ); 
			if( !c1->getSpin( DimY )->getLabel().equals( lp.d_y ) )
				cmd->add( new LabelSpinCmd( d_pro->getSpins(), c1->getSpin( DimY ), lp.d_y ) ); 
            ok = cmd->handle( getAgent() );
			if( lp.d_x.isNull() || lp.d_y.isNull() )
				d_src2D->showNulls( true );
		}
	}else if( d_synchro )
	{
		Root::Ref<CreateSystemPairCmd> c1 = 
			new CreateSystemPairCmd( d_pro->getSpins(),	p, d_spec2D );
        ok = c1->handle( getAgent() );
	}else // if( d_show3DPlane )
	{
		Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Pick System" );
		Root::Ref<CreateSystemPairCmd> c1 = 
			new CreateSystemPairCmd( d_pro->getSpins(),	p, d_spec2D );
		c1->execute();
		cmd->add( c1 );
		Root::Ref<PickSystemSpinCmd> c2 = new PickSystemSpinCmd( d_pro->getSpins(), 
			c1->getSystem(), d_spec3D->getColor( DimZ ), d_cursor[ DimZ ], 0 ); 
		cmd->add( c2 );
		c2->execute();
        ok = cmd->handle( getAgent() );
	}
	if( ok )
	{
		d_plane.d_tuples->selectPeak( d_cursor[ DimX ], d_cursor[ DimY ] );
		if( d_autoHold && !d_plane.d_tuples->getSel().empty() )
			d_ref = *d_plane.d_tuples->getSel().begin();
		updateRef();
		selectCurSystem();
		if( !d_strips.empty() )
		{
			d_strips[ DimX ].d_tuples->selectPeak( d_cursor[ DimX ], d_cursor[ DimZ ] );
			d_strips[ DimY ].d_tuples->selectPeak( d_cursor[ DimY ], d_cursor[ DimZ ] );
		}
	}
}

void PolyScope3::handlePickHori()
{
    ENABLED_IF( !d_show3DPlane );
	extendSystem( DimY, DimX );
}

void PolyScope3::handlePickVerti()
{
    ENABLED_IF( !d_show3DPlane );
	extendSystem( DimX, DimY );
}

void PolyScope3::handleMovePeak()
{
    ENABLED_IF( d_plane.d_tuples->getSel().size() == 1 );

	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Move Spin System" );
	for( Dimension d = 0; d < 2; d++ )
	{
		cmd->add( new MoveSpinCmd( d_pro->getSpins(), tuple[ d ], 
			d_cursor[ d ], 0 ) ); // Move generisches Spektrum
	}
    cmd->handle( getAgent() );
	selectCurSystem( true );
}

void PolyScope3::handleMovePeakAlias()
{
    ENABLED_IF( d_plane.d_tuples->getSel().size() == 1 );

	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Move Spin System" );
	Spectrum* spec = (d_show3DPlane && d_spec3D)?d_spec3D:d_spec2D;
	for( Dimension d = 0; d < 2; d++ )
	{
		if( d_cursor[ d ] != tuple[ d ]->getShift( spec ) )
			cmd->add( new MoveSpinCmd( d_pro->getSpins(), tuple[ d ], 
				d_cursor[ d ], spec ) );
	}
    cmd->handle( getAgent() );
	selectCurSystem( true );
}

void PolyScope3::handleLabelPeak()
{
    ENABLED_IF( d_plane.d_tuples->getSel().size() == 1 );

	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();

	Spectrum* spec = d_spec2D;
	if( d_show3DPlane && d_spec3D )
		spec = d_spec3D;
	SpinLabel x = tuple[ DimX ]->getLabel();
	SpinLabel y = tuple[ DimY ]->getLabel();
	SpinLabelSet lx = spec->getType()->getLabels( spec->mapToType( DimX ) );
	SpinLabelSet ly = spec->getType()->getLabels( spec->mapToType( DimY ) );
		// lx/y soll vorerst alle statischen Labels auch sehen. Wenn man das weglï¿½st,
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
    if( !Dlg::getLabels( getQt(),
		tuple[ DimX ]->getId(), tuple[ DimY ]->getId(), x, y, lx, ly ) )
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

void PolyScope3::handleHidePeak()
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	SpinLink* link = tuple[ DimX ]->findLink( tuple[ DimY ] );
    ENABLED_IF( link );
	Root::Ref<HideSpinLinkCmd> cmd = new HideSpinLinkCmd( d_pro->getSpins(), 
		link, d_spec2D );
    cmd->handle( getAgent() );
	// TODO: Plural
}

void PolyScope3::handleDeletePeak()
{
    ENABLED_IF( !d_plane.d_tuples->getSel().empty() );

	SpinPointView::Selection sel = d_plane.d_tuples->getSel();
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

void PolyScope3::handleDeleteSpinX()
{
    ENABLED_IF( d_plane.d_tuples->getSel().size() == 1 );
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();

	Root::Ref<DeleteSpinCmd> cmd = new DeleteSpinCmd( d_pro->getSpins(), tuple[ DimX ] );
    cmd->handle( getAgent() );
}

void PolyScope3::handleDeleteSpinY()
{
    ENABLED_IF( d_plane.d_tuples->getSel().size() == 1 );
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();

	Root::Ref<DeleteSpinCmd> cmd = new DeleteSpinCmd( d_pro->getSpins(), tuple[ DimY ] );
    cmd->handle( getAgent() );
}

void PolyScope3::handleShowAllPeaks()
{
    CHECKED_IF( true, d_mdl2D->showAll() );
	d_mdl2D->showAll( !d_mdl2D->showAll() );
	d_mdl3D->showAll( d_mdl2D->showAll() );
}

void PolyScope3::handleShowAlignment()
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
    ENABLED_IF( tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() );

	SpinSystem* sys = tuple[ DimX ]->getSystem();
	
	SpinSystemString fra;
	d_pro->getSpins()->fillString( sys, fra );

	FragmentAssignment* f = new FragmentAssignment( d_pro->getSpins(),
		d_pro->getMatcher(), fra );
    SingleAlignmentView* v = new SingleAlignmentView( getAgent(), f );
	v->show();
}

void PolyScope3::handleAssign()
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
    ENABLED_IF( tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() );

	SpinSystem* sys = tuple[ DimX ]->getSystem();
	bool ok;
	QString str;
	str.sprintf( "Assignment for spin system %d:", sys->getId() );
	int r = QInputDialog::getInteger( "Assign Strips", 
		str, (sys->getAssig())?sys->getAssig()->getId():0, 
        -999999, 999999, 1, &ok, getQt() );
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

void PolyScope3::handleUnassign()
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
    ENABLED_IF( tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() );

	SpinSystem* sys = tuple[ DimX ]->getSystem();

	Root::Ref<UnassignSystemCmd> cmd =
		new UnassignSystemCmd( d_pro->getSpins(), sys );
    cmd->handle( getAgent() );
}

void PolyScope3::handleSetSystemType()
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
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
        l, cur, false, &ok, getQt() );
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

void PolyScope3::handleViewLabels()
{
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    if( a->property("0").isNull() )
		return;

    SpinPointView::Label q = (SpinPointView::Label) a->property("0").toInt();
	if( q < SpinPointView::None || q >= SpinPointView::End )
		return;

    CHECKED_IF( true,
		d_plane.d_tuples->getLabel() == q );
	
	d_plane.d_tuples->setLabel( q );
	d_plane.d_viewer->redraw();
}

void PolyScope3::handleSelectSpec2D()
{
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    CHECKED_IF( !d_show3DPlane, d_spec2D == a->property( "0" ).value<Spec::Spectrum*>() );

    setSpec2D( a->property( "0" ).value<Spec::Spectrum*>() );
}

void PolyScope3::handleLinkSystems()
{
    ENABLED_IF( true );

	int pred, succ;
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    if( !a->property( "0" ).isNull() && !a->property( "1" ).isNull() )
	{
        pred = a->property( "0" ).toInt();
        succ = a->property( "1" ).toInt();
    }else if( !Dlg::getPredSucc( getQt(), pred, succ ) )
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

void PolyScope3::handleUnlinkPred()
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
    ENABLED_IF( tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() &&
		tuple[ DimX ]->getSystem()->getPred() != 0 );

	SpinSystem* other = tuple[ DimX ]->getSystem()->getPred();
	Root::Ref<UnlinkSystemCmd> cmd =
		new UnlinkSystemCmd( d_pro->getSpins(), other, tuple[ DimX ]->getSystem() );
    cmd->handle( getAgent() );
}

void PolyScope3::handleUnlinkSucc()
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
    ENABLED_IF( tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() &&
		tuple[ DimX ]->getSystem()->getSucc() != 0 );

	SpinSystem* other = tuple[ DimX ]->getSystem()->getSucc();
	Root::Ref<UnlinkSystemCmd> cmd =
		new UnlinkSystemCmd( d_pro->getSpins(), tuple[ DimX ]->getSystem(), other );
    cmd->handle( getAgent() );
}

void PolyScope3::handleUnhidePeak()
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

void PolyScope3::handleHoldReference()
{
    CHECKED_IF(
		!d_ref.isZero() || d_plane.d_tuples->getSel().size() == 1, !d_ref.isZero() );
	
	if( !d_ref.isZero() && d_plane.d_tuples->getSel().empty() )
		d_ref.zero(); // Hebe Zustand auf
	else if( !d_plane.d_tuples->getSel().empty() )
	{
		d_ref = *d_plane.d_tuples->getSel().begin();
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

void PolyScope3::handleCreateReport()
{
    ENABLED_IF( true );

	Dlg::ContourParams p;
	p.d_factor = d_plane.d_ol[0].d_view->getFactor();
	p.d_threshold =	d_spec2D->getThreshold();
	p.d_option = d_plane.d_ol[0].d_view->getOption();
    ReportViewer* rv = ReportViewer::getViewer( getAgent()->getParent()->getParent(), p,
		d_plane.d_ol[0].d_view->getGain(), d_plane.d_ol[0].d_view->isAuto(), d_folding,
		(d_plane.d_ol.size()>1)?d_pro->getRepository():0 );
	ReportViewer::Spector vec;
	for( int i = 0; i < d_plane.d_ol.size(); i++ )
		if( d_plane.d_ol[i].d_spec )
			vec.push_back( d_plane.d_ol[i].d_spec );
	rv->showPlane( d_plane.d_viewer->getViewArea(), 
		vec, new PeakProjector( *d_plane.d_pp ), d_plane.d_tuples->getModel() );
}

void PolyScope3::handleAddRulerVerti()
{
	// Erzeuge horizontal verlaufender Ruler an Position des Y-Spins
    ENABLED_IF( !d_plane.d_tuples->getSel().empty() );

	SpinPointView::Selection sel = d_plane.d_tuples->getSel();
	SpinPointView::Selection::const_iterator p;
	SpinPoint t;
	for( p = sel.begin(); p != sel.end(); ++ p )
	{
		t[ 0 ] = (*p)[ DimY ];
		d_plane.d_hRulerMdl->addPoint( t );
	}
}

void PolyScope3::handleAddRulerHori()
{
	// Erzeuge vertikal verlaufender Ruler an Position des X-Spins
    ENABLED_IF( !d_plane.d_tuples->getSel().empty() );

	SpinPointView::Selection sel = d_plane.d_tuples->getSel();
	SpinPointView::Selection::const_iterator p;
	SpinPoint t;
	for( p = sel.begin(); p != sel.end(); ++ p )
	{
		t[ 0 ] = (*p)[ DimX ];
		d_plane.d_vRulerMdl->addPoint( t );
	}
}

void PolyScope3::handleRemoveRulers()
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

void PolyScope3::handleRemoveAllRulers()
{
    ENABLED_IF( !d_plane.d_hRulerMdl->isEmpty() ||
		!d_plane.d_vRulerMdl->isEmpty() );
	d_plane.d_hRulerMdl->removeAll();
	d_plane.d_vRulerMdl->removeAll();
}

void PolyScope3::handleAutoRuler()
{
    CHECKED_IF( true, d_autoRuler );

	d_autoRuler = !d_autoRuler;
}

void PolyScope3::handleProposeHori()
{
	if( d_ref.isZero() && d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = d_ref;
	if( d_ref[ DimX ] == 0 )
		tuple = *d_plane.d_tuples->getSel().begin();

	Spin* ref = tuple[ DimY ];
    ENABLED_IF( !d_show3DPlane && ref->getSystem() && d_spec2D->hasNoesy() );

    ProposeSpinDlg dlg( getQt(), d_pro, d_spec2D->getColor( DimX ), d_cursor[ DimX ],
		d_spec2D, "Select Horizontal Partner" );
	dlg.setAnchor( DimY, ref );
	if( !dlg.exec() || dlg.getSpin() == 0 )
		return;

	// TODO: wenn target kein SpinSystem hat, dann ins ref einfgen.
	// TODO: wenn target und ref System haben, einen SysLink vorschlagen (Seite?)

	if( ref->findLink( dlg.getSpin() ) == 0 )	// Ref == target zulï¿½sig wegen Diagonaler
	{
		Root::Ref<LinkSpinCmd> cmd = new LinkSpinCmd( d_pro->getSpins(), ref, dlg.getSpin() ); 
        cmd->handle( getAgent() );
	}else
		Root::ReportToUser::alert( this, "Propose Horizontal Extension", 
			"The selected spins are already linked!" );

}

void PolyScope3::handleProposeVerti()
{
	if( d_ref.isZero() && d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = d_ref;
	if( d_ref[ DimX ] == 0 )
		tuple = *d_plane.d_tuples->getSel().begin();

	Spin* ref = tuple[ DimX ];
    ENABLED_IF( !d_show3DPlane && ref->getSystem() && d_spec2D->hasNoesy()  );

    ProposeSpinDlg dlg( getQt(), d_pro, d_spec2D->getColor( DimY ), d_cursor[ DimY ],
		d_spec2D, "Select Vertical Partner" );
	dlg.setAnchor( DimX, ref );
	if( !dlg.exec() || dlg.getSpin() == 0 )
		return;

	// TODO: wenn target kein SpinSystem hat, dann ins ref einfgen.
	// TODO: wenn target und ref System haben, einen SysLink vorschlagen (Seite?)

	if( ref->findLink( dlg.getSpin() ) == 0 ) // Ref == target zulï¿½sig wegen Diagonaler
	{
		Root::Ref<LinkSpinCmd> cmd = new LinkSpinCmd( d_pro->getSpins(), ref, dlg.getSpin() ); 
        cmd->handle( getAgent() );
	}else
		Root::ReportToUser::alert( this, "Propose Vertical Extension", 
			"The selected spins are already linked!" );
}

void PolyScope3::handleProposePeak()
{
    ENABLED_IF( ( d_show3DPlane && d_spec3D && d_spec3D->hasNoesy() ) ||
        ( !d_show3DPlane && d_spec2D->hasNoesy() ) );

	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Propose Peak" );
	Dlg::SpinTuple res;
	if( d_show3DPlane && d_spec3D )
	{
		Dlg::SpinRanking l1 = d_pro->getMatcher()->findMatchingSpins( d_pro->getSpins(),
			d_spec3D->getColor( DimX ), d_cursor[ DimX ], d_spec3D );
		Dlg::SpinRanking l2 = d_pro->getMatcher()->findMatchingSpins( d_pro->getSpins(),
			d_spec3D->getColor( DimY ), d_cursor[ DimY ], d_spec3D );
		Dlg::SpinRanking l3 = d_pro->getMatcher()->findMatchingSpins( d_pro->getSpins(),
			d_spec3D->getColor( DimZ ), d_cursor[ DimZ ], d_spec3D );
        res = Dlg::selectSpinTriple( getQt(),
			l1, l2, l3, "Select Matching Spins" );
		if( res.empty() )
			return;

		if( d_spec3D->isNoesy( DimX, DimY ) && res[DimX]->findLink( res[DimY] ) == 0 )
			cmd->add( new LinkSpinCmd( d_pro->getSpins(), res[DimX], res[DimY] ) ); 

		if( d_spec3D->isNoesy( DimX, DimZ ) && res[DimX]->findLink( res[DimZ] ) == 0 )
			cmd->add( new LinkSpinCmd( d_pro->getSpins(), res[DimX], res[DimZ] ) ); 

		if( d_spec3D->isNoesy( DimY, DimZ ) && res[DimY]->findLink( res[DimZ] ) == 0 )
			cmd->add( new LinkSpinCmd( d_pro->getSpins(), res[DimY], res[DimZ] ) ); 
	}else
	{
		Dlg::SpinRanking l1 = d_pro->getMatcher()->findMatchingSpins( d_pro->getSpins(),
			d_spec2D->getColor( DimX ), d_cursor[ DimX ], d_spec2D );
		Dlg::SpinRanking l2 = d_pro->getMatcher()->findMatchingSpins( d_pro->getSpins(),
			d_spec2D->getColor( DimY ), d_cursor[ DimY ], d_spec2D );
        res = Dlg::selectSpinPair( getQt(),
			l1, l2, "Select Matching Spins" );
		if( res.empty() )
			return;
		if( res[DimX]->findLink( res[DimY] ) == 0 ) // Ref == target zulï¿½sig wegen Diagonaler
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

void PolyScope3::handleSelectSpec3D()
{
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    CHECKED_IF( true, d_spec3D == a->property( "0" ).value<Spec::Spectrum*>() );

    setSpec3D( a->property( "0" ).value<Spec::Spectrum*>() );
	selectCurSystem();
}

void PolyScope3::handlePickSpin3D()
{
	if( !d_src3D->showNulls() )
	{
        handlePickLabel3D();
		return;
	}

	Dimension ref = DimX;	// RISK
	if(	d_strips[ DimX ].d_viewer->hasFocus() )
		ref = DimX;
	else if( d_strips[ DimY ].d_viewer->hasFocus() )
		ref = DimY;
    ENABLED_IF( ref != DimUndefined && !d_spec3D.isNull() && !d_cur.isZero() );

	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Pick Spin" );
	// Pick generisches Spektrum
	if( d_cur[ ref ]->getSystem() )
	{
		Root::Ref<PickSystemSpinCmd> c2 = new PickSystemSpinCmd( d_pro->getSpins(), 
			d_cur[ ref ]->getSystem(), d_spec3D->getColor( DimZ ), d_cursor[ DimZ ], 0 ); 
		cmd->add( c2 );
		c2->execute();
	}else
	{
		Root::Ref<PickSpinCmd> c2 = new PickSpinCmd( d_pro->getSpins(), 
			d_spec3D->getColor( DimZ ), d_cursor[ DimZ ], 0 ); 
		cmd->add( c2 );
		c2->execute();
	}
	
    cmd->handle( getAgent() );
	d_strips[ DimX ].d_tuples->selectPeak( d_cursor[ DimX ], d_cursor[ DimZ ] );
	d_strips[ DimY ].d_tuples->selectPeak( d_cursor[ DimY ], d_cursor[ DimZ ] );
}

void PolyScope3::handleMoveSpin3D()
{
	Dimension ref = DimUndefined;
	if(	d_strips[ DimX ].d_viewer->hasFocus() &&
		d_strips[ DimX ].d_tuples->getSel().size() == 1 )
		ref = DimX;
	else if( d_strips[ DimY ].d_viewer->hasFocus() &&
		d_strips[ DimY ].d_tuples->getSel().size() == 1 )
		ref = DimY;

    ENABLED_IF( ref != DimUndefined && !d_spec3D.isNull() && !d_cur.isZero() );

	SpinPoint tuple;
	if( ref == DimX )
		tuple = *d_strips[ DimX ].d_tuples->getSel().begin();
	else
		tuple = *d_strips[ DimY ].d_tuples->getSel().begin();
	Root::Ref<MoveSpinCmd> cmd =
		new MoveSpinCmd( d_pro->getSpins(), tuple[ DimY ], d_cursor[ DimZ ], 0 );
		// Move generisches Spektrum
    cmd->handle( getAgent() );
}

void PolyScope3::handleMoveSpinAlias3D()
{
	Dimension ref = DimUndefined;
	if(	d_strips[ DimX ].d_viewer->hasFocus() &&
		d_strips[ DimX ].d_tuples->getSel().size() == 1 )
		ref = DimX;
	else if( d_strips[ DimY ].d_viewer->hasFocus() &&
		d_strips[ DimY ].d_tuples->getSel().size() == 1 )
		ref = DimY;

    ENABLED_IF( ref != DimUndefined && !d_spec3D.isNull() && !d_cur.isZero() );

	SpinPoint tuple;
	if( ref == DimX )
		tuple = *d_strips[ DimX ].d_tuples->getSel().begin();
	else
		tuple = *d_strips[ DimY ].d_tuples->getSel().begin();
	Root::Ref<MoveSpinCmd> cmd =
		new MoveSpinCmd( d_pro->getSpins(), tuple[ DimY ], d_cursor[ DimZ ], d_spec3D );
    cmd->handle( getAgent() );
}

void PolyScope3::handleDeleteSpins3D()
{
	Dimension ref = DimUndefined;
	if(	d_strips[ DimX ].d_viewer->hasFocus() &&
		!d_strips[ DimX ].d_tuples->getSel().empty() )
		ref = DimX;
	else if( d_strips[ DimY ].d_viewer->hasFocus() &&
		!d_strips[ DimY ].d_tuples->getSel().empty() )
		ref = DimY;

    ENABLED_IF( ref != DimUndefined && !d_spec3D.isNull() && !d_cur.isZero() );

	SpinPointView::Selection sel;
	if( ref == DimX )
		sel = d_strips[ DimX ].d_tuples->getSel();
	else
		sel = d_strips[ DimY ].d_tuples->getSel();
	SpinPointView::Selection::const_iterator p;
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Delete Spins" );
	std::set<Spin*> test;
	for( p = sel.begin(); p != sel.end(); ++p )
	{
		if( test.count( (*p)[ DimY ] ) == 0 )
		{
			cmd->add( new DeleteSpinCmd( d_pro->getSpins(), (*p)[ DimY ] ) );
			test.insert( (*p)[ DimY ] );
		}
	}
    cmd->handle( getAgent() );
}

void PolyScope3::handleLabelSpin3D()
{
	if( d_spec3D.isNull() || d_cur.isZero() )
		return;
	Dimension ref = DimUndefined;
	if(	d_strips[ DimX ].d_viewer->hasFocus() &&
		d_strips[ DimX ].d_tuples->getSel().size() == 1 )
		ref = DimX;
	else if( d_strips[ DimY ].d_viewer->hasFocus() &&
		d_strips[ DimY ].d_tuples->getSel().size() == 1 )
		ref = DimY;

    ENABLED_IF( ref != DimUndefined );

	SpinPoint tuple;
	if( ref == DimX )
		tuple = *d_strips[ DimX ].d_tuples->getSel().begin();
	else
		tuple = *d_strips[ DimY ].d_tuples->getSel().begin();

	SpinLabel y = tuple[ DimY ]->getLabel();
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    if( a->property("0").isNull() || !SpinLabel::parse( a->property("0").toByteArray(), y ) )
	{
		SpinLabelSet ly = d_spec3D->getType()->getLabels( d_spec3D->mapToType( DimZ ) );
		// ly soll vorerst alle statischen Labels auch sehen. Wenn man das weglï¿½st,
		// verschwinden zur Zeit noch CB aus der Liste von HNCACB von Freds Template.
		// Zu grosse Auswirkungen auf bestehende Projekte.
		NmrExperiment* e = d_pro->getRepository()->getTypes()->
			inferExperiment2( d_spec3D->getType(), tuple[ DimY ]->getSystem(), d_spec3D );
		if( e )
		{
			// e->getColumn( d_spec3D->mapToType( DimZ ), ly );
            PathTable::Path filter;
			filter[ d_spec3D->mapToType( DimX ) ] = d_cur[ DimX ]->getLabel();
			filter[ d_spec3D->mapToType( DimY ) ] = d_cur[ DimY ]->getLabel();
			e->getColumn( filter, d_spec3D->mapToType( DimZ ), ly );
		}
		if( tuple[ DimY ]->getSystem() )
			ly = tuple[ DimY ]->getSystem()->getAcceptables( ly );
        if( !Dlg::getLabel( getQt(), y, ly ) )
			return;
	}

	Root::Ref<LabelSpinCmd> cmd =
		new LabelSpinCmd( d_pro->getSpins(), tuple[ DimY ], y );
    cmd->handle( getAgent() );
	if( y.isNull() )
		d_src3D->showNulls( true );
}

void PolyScope3::handleForceLabelSpin3D()
{
	Dimension ref = DimUndefined;
	if(	d_strips[ DimX ].d_viewer->hasFocus() &&
		d_strips[ DimX ].d_tuples->getSel().size() == 1 )
		ref = DimX;
	else if( d_strips[ DimY ].d_viewer->hasFocus() &&
		d_strips[ DimY ].d_tuples->getSel().size() == 1 )
		ref = DimY;

    ENABLED_IF( ref != DimUndefined && !d_spec3D.isNull() && !d_cur.isZero() );

	SpinPoint tuple;
	if( ref == DimX )
		tuple = *d_strips[ DimX ].d_tuples->getSel().begin();
	else
		tuple = *d_strips[ DimY ].d_tuples->getSel().begin();

	bool ok	= FALSE;
	QString res;
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    if( a->property("0").isNull() )
	{
		res.sprintf( "Please enter a valid label (%s):", SpinLabel::s_syntax );
		res = QInputDialog::getText( "Force Spin Label", res, QLineEdit::Normal, 
            tuple[ DimY ]->getLabel().data(), &ok, getQt() );
		if( !ok )
			return;
	}else
        res = a->property("0").toByteArray();

	SpinLabel l;
	if( !SpinLabel::parse( res, l ) )
	{
		Root::ReportToUser::alert( this, "Force Spin Label", "Invalid spin label syntax!" );
		return;
	}

	Root::Ref<LabelSpinCmd> cmd =
		new LabelSpinCmd( d_pro->getSpins(), tuple[ DimY ], l );
    cmd->handle( getAgent() );
	if( l.isNull() )
		d_src3D->showNulls( true );
}

void PolyScope3::handleSetWidth()
{
    ENABLED_IF( !d_spec3D.isNull() &&
		( d_strips[ DimX ].d_viewer->hasFocus() ||
		  d_strips[ DimY ].d_viewer->hasFocus() ) );

	Dimension dim;
	if( d_strips[ DimX ].d_viewer->hasFocus() )
	{
		dim = DimX;
	}else
	{
		dim = DimY;
	}
	bool ok	= FALSE;
	QString res;
	res.sprintf( "%0.3f", d_pro->inferPeakWidth( dim, d_spec3D ) );
	res	= QInputDialog::getText( "Set Peak Width", 
		"Please	enter atom positive PPM value:",QLineEdit::Normal,  
        res, &ok, getQt() );
	if( !ok )
		return;
	PPM w = res.toFloat( &ok );
	if( !ok || w < 0.0 )
	{
        QMessageBox::critical( getQt(), "Set Peak Width",
				"Invalid peak width!", "&Cancel" );
		return;
	}

	d_pro->setPeakWidth( dim, w, d_spec3D );
}

void PolyScope3::handleFitWindow3D()
{
    ENABLED_IF( !d_spec3D.isNull() );
	if( d_spec3D.isNull() )
		return;	// QT-BUG
	PpmRange r = d_spec3D->getScale( DimZ ).getRange();	
	r.invert();
	d_strips[ DimX ].d_viewer->getViewArea()->setRange( DimY, r );
	d_strips[ DimY ].d_viewer->getViewArea()->setRange( DimY, r );
	d_slices[ DimZ ].d_viewer->getViewArea()->setRange( DimY, r );	
	d_strips[ DimX ].d_viewer->redraw();
	d_strips[ DimY ].d_viewer->redraw();
}

void PolyScope3::handleAutoContour2()
{
    CHECKED_IF( true, d_strips[ DimX ].d_view->isAuto() );
	
	bool ac = !d_strips[ DimX ].d_view->isAuto();
	updateContour2( DimX, ac );
	updateContour2( DimY, ac );
	if( d_show3DPlane )
		updateContour( 0, true );
}

void PolyScope3::handleContourParams2()
{
    ENABLED_IF( d_spec3D );

	Dlg::ContourParams p;
	p.d_factor = d_strips[ DimX ].d_view->getFactor();
	p.d_threshold =	d_spec3D->getThreshold();
	p.d_option = d_strips[ DimX ].d_view->getOption();
    if( Dlg::setParams( getQt(), p ) )
	{
		d_spec3D->setThreshold( p.d_threshold );
		d_strips[ DimX ].d_view->setOption( p.d_option );
		d_strips[ DimX ].d_view->setFactor( p.d_factor );
		d_strips[ DimY ].d_view->setOption( p.d_option );
		d_strips[ DimY ].d_view->setFactor( p.d_factor );
		updateContour2( DimX, false );
		updateContour2( DimY, false );
		if( d_show3DPlane )
		{
			showIntens( false );
			updateContour( 0, true );
		}
	}
}

void PolyScope3::handleShowWithOff()
{
    CHECKED_IF( true, d_src3D->showOffs() );
	
	d_src3D->showOffs( !d_src3D->showOffs() );
}

void PolyScope3::update3dPlane()
{
	if( d_show3DPlane )
	{
		d_plane.d_ol[0].d_spec = new SpecProjector( d_spec3D, DimX, DimY );
		d_plane.d_ol[0].d_spec->setOrigin( d_cursor );
		d_plane.d_ol[0].d_buf->setSpectrum( d_plane.d_ol[0].d_spec );
		// TODO if( !d_synchro )
			d_plane.d_tuples->setModel( d_plane.d_mdl3D );
		d_plane.d_pp->setOrigThick( DimZ, d_cursor[ DimZ ], d_spec3D->getScale( DimZ ).getDelta() );
		d_plane.d_pp->setRange( DimZ, d_plane.d_mdl3D->getGhostWidth() );
		d_plane.d_peaks->setSpec( d_spec3D );
	}else
	{
		d_plane.d_ol[0].d_spec = new SpecProjector( d_spec2D, DimX, DimY );
		d_plane.d_ol[0].d_buf->setSpectrum( d_plane.d_ol[0].d_spec );
		if( d_spec2D->getDimCount() > 2 )
		{
			d_plane.d_ol[0].d_spec->setOrigin( d_cursor );
			d_slices[ DimX ].d_spec2D->setOrigin( d_cursor );
			d_slices[ DimY ].d_spec2D->setOrigin( d_cursor );
		}
		// TODO if( !d_synchro )
			d_plane.d_tuples->setModel( d_mdl2D );
		d_plane.d_pp->setOrigThick( DimZ, d_cursor[ DimZ ], 0 );
		d_plane.d_pp->setRange( -1, 0 );
		d_plane.d_peaks->setSpec( d_spec2D );
	}
	d_plane.d_vRulerMdl->setSpec( d_plane.d_ol[0].d_spec );
	d_plane.d_hRulerMdl->setSpec( d_plane.d_ol[0].d_spec );
	updateContour( 0, true );
	updatePlaneLabel();
	d_plane.d_viewer->redraw();
}

void PolyScope3::handleShow3dPlane()
{
    CHECKED_IF( d_spec3D, d_show3DPlane );
	d_show3DPlane = !d_show3DPlane;
	if( d_show3DPlane )
	{
		d_contourFactor = d_plane.d_ol[0].d_view->getFactor();
		d_contourOption = d_plane.d_ol[0].d_view->getOption();
		d_gain = d_plane.d_ol[0].d_view->getGain();
		d_autoContour = d_plane.d_ol[0].d_view->isAuto();
	}else
	{
		if( d_autoContour )
		{
			d_plane.d_ol[0].d_view->createLevelsAuto( d_contourFactor, 
				d_contourOption, d_gain );
		}else
			d_plane.d_ol[0].d_view->createLevelsMin( d_contourFactor, 
				d_spec2D->getThreshold(), d_contourOption );
	}
	update3dPlane();
	SpecChanged msg( true );
    getAgent()->traverseUp( msg );
}

void PolyScope3::handleAutoHide()
{
    CHECKED_IF( true, d_autoHide );

	d_autoHide = !d_autoHide;
	if( d_spec3D )
	{
		sync3dXySliceToCur( DimX, !d_autoHide );
		sync3dXySliceToCur( DimY, !d_autoHide );
	}
	d_slices[ DimX ].d_viewer->redraw();
	d_slices[ DimY ].d_viewer->redraw();
}

void PolyScope3::handleStripCalibrate()
{
	Dimension ref = DimUndefined;
	if(	d_strips[ DimX ].d_viewer->hasFocus() &&
		d_strips[ DimX ].d_tuples->getSel().size() == 1 )
		ref = DimX;
	else if( d_strips[ DimY ].d_viewer->hasFocus() &&
		d_strips[ DimY ].d_tuples->getSel().size() == 1 )
		ref = DimY;

    ENABLED_IF( ref != DimUndefined && !d_spec3D.isNull() && !d_cur.isZero() );


	SpinPoint tuple;
	if( ref == DimX )
		tuple = *d_strips[ DimX ].d_tuples->getSel().begin();
	else
		tuple = *d_strips[ DimY ].d_tuples->getSel().begin();

	PpmPoint p( 0, 0, 0 );
	p[ DimZ ] = tuple[ DimY ]->getShift( d_spec3D ) - d_cursor[ DimZ ];

	Viewport::pushHourglass();
	Root::Ref<SpecCalibrateCmd> cmd = new SpecCalibrateCmd( d_spec3D, p );
    cmd->handle( getAgent() );
	Viewport::popCursor();
}

void PolyScope3::handleProposeSpin()
{
    ENABLED_IF( !d_spec3D.isNull() && !d_cur.isZero() &&
		d_spec3D->isNoesy( DimZ ) );

	Spin* orig = 0;
	if( d_spec3D->isNoesy( DimX, DimZ ) )
		orig = d_cur[ DimX ];
	else
		orig = d_cur[ DimY ];

    ProposeSpinDlg dlg( getQt(), d_pro, d_spec3D->getColor( DimZ ), d_cursor[ DimZ ],
		d_spec3D, "Select Vertical Partner" );
	dlg.setAnchor( DimX, d_cur[ DimX ] );
	dlg.setAnchor( DimZ, d_cur[ DimY ] );
	if( !dlg.exec() || dlg.getSpin() == 0 )
		return;

	if( orig->findLink( dlg.getSpin() ) == 0 ) // Ref == target zulssig wegen Diagonaler
	{
		Root::Ref<LinkSpinCmd> c1 = new LinkSpinCmd( d_pro->getSpins(), orig, dlg.getSpin() ); 
        if( c1->handle( getAgent() ) )
		{
			d_strips[ DimX ].d_tuples->selectPeak( d_cursor[ DimX ], d_cursor[ DimZ ] );
			d_strips[ DimY ].d_tuples->selectPeak( d_cursor[ DimY ], d_cursor[ DimZ ] );
		}
	}else
		Root::ReportToUser::alert( this, "Propose Vertical Extension", 
			"The selected spins are already linked!" );
}

void PolyScope3::handleEditAttsSpinH()
{
    ENABLED_IF( d_plane.d_tuples->getSel().size() == 1 );
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();

    DynValueEditor::edit( getQt(),
		d_pro->getRepository()->findObjectDef( Repository::keySpin ), tuple[ DimX ] );
}

void PolyScope3::handleEditAttsSpinV()
{
    ENABLED_IF( d_plane.d_tuples->getSel().size() == 1 );
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();

    DynValueEditor::edit( getQt(),
		d_pro->getRepository()->findObjectDef( Repository::keySpin ), tuple[ DimY ] );
}

void PolyScope3::handleEditAttsLink()
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	SpinLink* l = tuple[ DimX ]->findLink( tuple[ DimY ] );
    ENABLED_IF( l );

    DynValueEditor::edit( getQt(),
		d_pro->getRepository()->findObjectDef( Repository::keyLink ), l );
}

void PolyScope3::handleEditAttsSysH()
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	SpinSystem* s = tuple[ DimX ]->getSystem();
    ENABLED_IF( s );

    DynValueEditor::edit( getQt(),
		d_pro->getRepository()->findObjectDef( Repository::keySpinSystem ), s );
}

void PolyScope3::handleEditAttsSysV()
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	SpinSystem* s = tuple[ DimY ]->getSystem();
    ENABLED_IF( s );

    DynValueEditor::edit( getQt(),
		d_pro->getRepository()->findObjectDef( Repository::keySpinSystem ), s );
}

void PolyScope3::handleEditAttsSpin3D()
{
	Spin* spin = getSelectedSpin();
    ENABLED_IF( !d_spec3D.isNull() && !d_cur.isZero() && spin);

    DynValueEditor::edit( getQt(),
		d_pro->getRepository()->findObjectDef( Repository::keySpin ), spin );
}

void PolyScope3::handleCursorSync()
{
    CHECKED_IF( true, d_cursorSync );
	
	d_cursorSync = !d_cursorSync;
	if( d_cursorSync )
		GlobalCursor::addObserver( this );	// TODO: preset Cursor
	else if( !d_rangeSync )
		GlobalCursor::removeObserver( this );
}

void PolyScope3::handleGotoSystem()
{
    ENABLED_IF( true );

	QString id;
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    if( a->property("0").isNull() )
	{
		bool ok	= FALSE;
		id	= QInputDialog::getText( "Goto System", 
            "Please	enter a spin system id:", QLineEdit::Normal, "", &ok, getQt() );
		if( !ok )
			return;
	}else
        id = a->property("0").toByteArray();

	SpinSystem* sys = d_pro->getSpins()->findSystem( id );
	if( sys == 0 )
	{
		Lexi::ShowStatusMessage msg( "Goto System: unknown system" );
        getAgent()->traverseUp( msg );
		return;
	}
	gotoTuple( sys, 0, 0, true );
}

void PolyScope3::handleNextSpec3D()
{
    ENABLED_IF( d_sort3D.size() > 1 );
	stepSpec3D( true );
}

void PolyScope3::handlePrevSpec3D()
{
    ENABLED_IF( d_sort3D.size() > 1 );
	stepSpec3D( false );
}

void PolyScope3::handleNextSpec2D()
{
    ENABLED_IF( d_sort2D.size() > 1 );
	stepSpec2D( true );
}

void PolyScope3::handlePrevSpec2D()
{
    ENABLED_IF( d_sort2D.size() > 1 );
	stepSpec2D( false );
}

void PolyScope3::handleForwardPlane()
{
	if( d_spec3D.isNull() )
		return;
    ENABLED_IF( !d_spec3D.isNull() );

	d_slices[ DimZ ].d_cur->setCursor( (Dimension)DimY, 
		d_cursor[ DimZ ] + d_spec3D->getScale( DimZ ).getDelta() );  
}

void PolyScope3::handleBackwardPlane()
{
	if( d_spec3D.isNull() )
		return;
    ENABLED_IF( !d_spec3D.isNull() );

	d_slices[ DimZ ].d_cur->setCursor( (Dimension)DimY, 
		d_cursor[ DimZ ] - d_spec3D->getScale( DimZ ).getDelta() );  
}

void PolyScope3::handleShowWithOff2()
{
    CHECKED_IF( !d_show3DPlane, d_src2D->showOffs() );
	
	d_src2D->showOffs( !d_src2D->showOffs() );
	d_plane.d_viewer->redraw();
}

void PolyScope3::handleShowLinks()
{
    CHECKED_IF( !d_show3DPlane, d_src2D->showLinks() );
	
	d_src2D->showLinks( !d_src2D->showLinks() );
	d_plane.d_viewer->redraw();
}

void PolyScope3::handleShowLinks2()
{
    CHECKED_IF( true, d_src3D->showLinks() );
	
	d_src3D->showLinks( !d_src3D->showLinks() );
	d_plane.d_viewer->redraw();
}

void PolyScope3::handleDeleteLinks()
{
    ENABLED_IF( !d_plane.d_tuples->getSel().empty() );

	SpinPointView::Selection sel = d_plane.d_tuples->getSel();
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

void PolyScope3::handleLabelVerti()
{
    ENABLED_IF( d_plane.d_tuples->getSel().size() == 1 );

	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();

	SpinLabel l;
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    if( !SpinLabel::parse( a->property("0").toByteArray(), l ) )
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

void PolyScope3::handleLabelHori()
{
    ENABLED_IF( d_plane.d_tuples->getSel().size() == 1 );

	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();

	SpinLabel l;
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    if( !SpinLabel::parse( a->property("0").toByteArray(), l ) )
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

void PolyScope3::handleSetCandidates()
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
    ENABLED_IF( tuple[ DimX ]->getSystem() == tuple[ DimY ]->getSystem() );

	SpinSystem* sys = tuple[ DimX ]->getSystem();

    CandidateDlg dlg( getQt(), d_pro->getRepository() );
	dlg.setTitle( sys );
	if( dlg.exec() )
		d_pro->getSpins()->setCands( sys, dlg.d_cands );
}

void PolyScope3::handleShowInfered()
{
    CHECKED_IF( !d_show3DPlane, d_src2D->showInferred() );
	
	d_src2D->showInferred( !d_src2D->showInferred() );
	d_plane.d_viewer->redraw();
}

void PolyScope3::handleShowInfered2()
{
    CHECKED_IF( true, d_src3D->showInferred() );
	
	d_src3D->showInferred( !d_src3D->showInferred() );
	d_plane.d_viewer->redraw();
}

void PolyScope3::handleShowUnlabeled()
{
    CHECKED_IF( !d_show3DPlane, d_src2D->showNulls() );
	
	d_src2D->showNulls( !d_src2D->showNulls() );
	d_plane.d_viewer->redraw();
}

void PolyScope3::handleShowUnlabeled2()
{
    CHECKED_IF( true, d_src3D->showNulls() );
	
	d_src3D->showNulls( !d_src3D->showNulls() );
	d_plane.d_viewer->redraw();
}

void PolyScope3::handleShowUnknown()
{
    CHECKED_IF( !d_show3DPlane, d_src2D->showUnknown() );
	
	d_src2D->showUnknown( !d_src2D->showUnknown() );
	d_plane.d_viewer->redraw();
}

void PolyScope3::handleShowUnknown2()
{
    CHECKED_IF( true, d_src3D->showUnknown() );
	
	d_src3D->showUnknown( !d_src3D->showUnknown() );
	d_plane.d_viewer->redraw();
}

void PolyScope3::handleCreateLinks()
{
    CHECKED_IF( true, false );
}

void PolyScope3::handleForceCross()
{
    CHECKED_IF( true, d_src2D->doPathsim() );
	
	d_src2D->doPathsim( !d_src2D->doPathsim() );
	d_src3D->doPathsim( d_src2D->doPathsim() );
	d_plane.d_viewer->redraw();
}

void PolyScope3::handleDeleteLinks3D()
{
	Dimension ref = DimUndefined;
	if(	d_strips[ DimX ].d_viewer->hasFocus() &&
		!d_strips[ DimX ].d_tuples->getSel().empty() )
		ref = DimX;
	else if( d_strips[ DimY ].d_viewer->hasFocus() &&
		!d_strips[ DimY ].d_tuples->getSel().empty() )
		ref = DimY;

    ENABLED_IF( ref != DimUndefined && !d_spec3D.isNull() && !d_cur.isZero() );

	SpinPointView::Selection sel;
	if( ref == DimX )
		sel = d_strips[ DimX ].d_tuples->getSel();
	else
		sel = d_strips[ DimY ].d_tuples->getSel();
	SpinPointView::Selection::const_iterator p;
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Delete Links" );
	SpinLink* l;
	std::set<SpinLink*> test;
	for( p = sel.begin(); p != sel.end(); ++p )
	{
		// TODO statt (*p)[ DimX ] ev. d_cur[ DimX ] verwenden? Identisch?
		l = (*p)[ DimX ]->findLink( (*p)[ DimY ] );
		if( l && test.count( l ) == 0 )
		{
			cmd->add( new UnlinkSpinCmd( d_pro->getSpins(), (*p)[ DimX ], (*p)[ DimY ] ) );
			test.insert( l );
		}
	}
    cmd->handle( getAgent() );
}

void PolyScope3::handleViewLabels3D()
{
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    if( a->property("0").isNull() )
		return;

    SpinPointView::Label q = (SpinPointView::Label) a->property("0").toInt();
	if( q < SpinPointView::None || q >= SpinPointView::End )
		return;

    CHECKED_IF( true,
		d_strips[ DimX ].d_tuples->getLabel() == q );
	
	for( Dimension d = 0; d < d_strips.size(); d++ )
	{
		d_strips[ d ].d_tuples->setLabel( q, DimY );
		d_strips[ d ].d_viewer->redraw();
	}
}

void PolyScope3::handleAutoGain()
{
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    ENABLED_IF( !a->property("0").isNull() && !d_show3DPlane );

    float g = a->property("0").toDouble();
	if( g <= 0.0 )
	{
		Root::ReportToUser::alert( this, "Set Auto Gain", "Invalid Gain Value" );
		return;
	}
	int l = d_aol;
    if( a->property("1").isNull() )
		l = selectLayer();
    else if( ::strcmp( a->property("1").toByteArray(), "*" ) == 0 )
		l = -1;
	else
        l = a->property("1").toInt();
	if( l == -2 )
		return;
	else if( l == -1 ) // All
	{
		for( l = 0; l < d_plane.d_ol.size(); l++ )
		{
			d_plane.d_ol[l].d_view->setVisi( true );
			d_plane.d_ol[l].d_view->setGain( g );
			d_plane.d_ol[l].d_view->createLevelsAuto();
		}
	}else if( l >= 0 && l < d_plane.d_ol.size() )
	{
		d_plane.d_ol[l].d_view->setVisi( true );
		d_plane.d_ol[l].d_view->setGain( g );
		d_plane.d_ol[l].d_view->createLevelsAuto();
	}
	d_plane.d_viewer->damageMe();
}

void PolyScope3::handleAutoGain3D()
{
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    ENABLED_IF( !a->property("0").isNull() );
    float g = a->property("0").toDouble();
	if( g <= 0.0 )
	{
		Root::ReportToUser::alert( this, "Set Auto Gain", "Invalid Gain Value" );
		return;
	}
	d_strips[ DimX ].d_view->setGain( g );
	d_strips[ DimY ].d_view->setGain( g );
	updateContour2( DimX, true );
	updateContour2( DimY, true );
}

void PolyScope3::handleShowGhosts()
{
    CHECKED_IF( true, d_plane.d_mdl3D->showGhosts() );

	d_plane.d_mdl3D->showGhosts( !d_plane.d_mdl3D->showGhosts() );
	d_strips[ DimX ].d_mdl->showGhosts( d_plane.d_mdl3D->showGhosts() );
	d_strips[ DimY ].d_mdl->showGhosts( d_plane.d_mdl3D->showGhosts() );
}

void PolyScope3::handleAutoHold()
{
    CHECKED_IF( true, d_autoHold );
	d_autoHold = !d_autoHold;
}

void PolyScope3::handlePickLabel3D()
{
	Dimension ref = DimX;
	if(	d_strips[ DimX ].d_viewer->hasFocus() )
		ref = DimX;
	else if( d_strips[ DimY ].d_viewer->hasFocus() )
		ref = DimY;
    ENABLED_IF( ref != DimUndefined && !d_spec3D.isNull() && !d_cur.isZero() &&
		d_cur[ ref ]->getSystem() );

	SpinLabelSet ly = d_spec3D->getType()->getLabels( d_spec3D->mapToType( DimZ ) );
		// ly soll vorerst alle statischen Labels auch sehen. Wenn man das weglï¿½st,
		// verschwinden zur Zeit noch CB aus der Liste von HNCACB von Freds Template.
		// Zu grosse Auswirkungen auf bestehende Projekte.
	NmrExperiment* e = d_pro->getRepository()->getTypes()->
		inferExperiment2( d_spec3D->getType(), d_cur[ ref ]->getSystem(), d_spec3D );
	if( e )
	{
		// e->getColumn( d_spec3D->mapToType( DimZ ), ly );
        PathTable::Path filter;
		filter[ d_spec3D->mapToType( DimX ) ] = d_cur[ DimX ]->getLabel();
		filter[ d_spec3D->mapToType( DimY ) ] = d_cur[ DimY ]->getLabel();
		e->getColumn( filter, d_spec3D->mapToType( DimZ ), ly );
	}
	if( d_cur[ ref ]->getSystem() )
		ly = d_cur[ ref ]->getSystem()->getAcceptables( ly );
	SpinLabel y;
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    if( a->property("0").isNull() ||
        !SpinLabel::parse( a->property("0").toByteArray(), y ) )
        if( !Dlg::getLabel( getQt(), y, ly ) )
			return;
	if( !d_cur[ ref ]->getSystem()->isAcceptable( y ) )
	{
		Root::ReportToUser::alert( this, "Pick Label", "Label is not acceptable" );
		return;
	}

	Root::Ref<PickSystemSpinLabelCmd> cmd = new PickSystemSpinLabelCmd( d_pro->getSpins(), 
		d_cur[ ref ]->getSystem(), d_spec3D->getColor( DimZ ), d_cursor[ DimZ ], y, 0 ); 
    cmd->handle( getAgent() );

	if( y.isNull() )
		d_src3D->showNulls( true );

	d_strips[ DimX ].d_tuples->selectPeak( d_cursor[ DimX ], d_cursor[ DimZ ] );
	d_strips[ DimY ].d_tuples->selectPeak( d_cursor[ DimY ], d_cursor[ DimZ ] );
}

void PolyScope3::handleSetDepth()
{
    ENABLED_IF( d_spec3D );

	float g;
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    if( a->property("0").isNull() )
	{
		bool ok	= FALSE;
		QString res;
		res.sprintf( "%f", d_pro->inferPeakWidth( DimZ, d_spec3D ) );
		res	= QInputDialog::getText( "Set Peak Depth", 
			"Please	enter a positive ppm value:", QLineEdit::Normal, res, &ok, 
            getQt() );
		if( !ok )
			return;
		g = res.toFloat( &ok );
	}else
        g = a->property("0").toDouble();
	if( g < 0.0 )
	{
		Root::ReportToUser::alert( this, "Set Peak Depth", "Invalid PPM Value" );
		return;
	}
	d_pro->setPeakWidth( DimZ, g, d_spec3D );
}

void PolyScope3::handleGhostLabels()
{
    CHECKED_IF( true, d_plane.d_tuples->ghostLabel() );

	d_plane.d_tuples->ghostLabel( !d_plane.d_tuples->ghostLabel() );
	d_strips[ DimX ].d_tuples->ghostLabel( d_plane.d_tuples->ghostLabel() );
	d_strips[ DimY ].d_tuples->ghostLabel( d_plane.d_tuples->ghostLabel() );
}

void PolyScope3::handleHidePeak2()
{
	Dimension ref = DimUndefined;
	if(	d_strips[ DimX ].d_viewer->hasFocus() &&
		d_strips[ DimX ].d_tuples->getSel().size() == 1 )
		ref = DimX;
	else if( d_strips[ DimY ].d_viewer->hasFocus() &&
		d_strips[ DimY ].d_tuples->getSel().size() == 1 )
		ref = DimY;

    ENABLED_IF( ref != DimUndefined && !d_spec3D.isNull() && !d_cur.isZero() );

	SpinPoint tuple;
	if( ref == DimX )
		tuple = *d_strips[ DimX ].d_tuples->getSel().begin();
	else
		tuple = *d_strips[ DimY ].d_tuples->getSel().begin();
	SpinLink* link = tuple[ DimX ]->findLink( tuple[ DimY ] );
    ENABLED_IF( link );
	Root::Ref<HideSpinLinkCmd> cmd = new HideSpinLinkCmd( d_pro->getSpins(), 
		link, d_spec3D );
    cmd->handle( getAgent() );
	// TODO: Plural
}

void PolyScope3::handleGotoPeak()
{
    ENABLED_IF( d_spec3D );

	QString id;
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    if( a->property("0").isNull() )
	{
		bool ok	= FALSE;
		id	= QInputDialog::getText( "Goto Peak", 
            "Please	enter a spin system id:",QLineEdit::Normal, "", &ok, getQt() );
		if( !ok )
			return;
	}else
        id = a->property("0").toByteArray();

	SpinSystem* sys = d_pro->getSpins()->findSystem( id );
	if( sys == 0 )
	{
		Lexi::ShowStatusMessage msg( "Goto Peak: unknown system" );
        getAgent()->traverseUp( msg );
		return;
	}
	gotoTuple( sys, 0, 0, false );
}

void PolyScope3::handleRangeSync()
{
    CHECKED_IF( true, d_rangeSync );
	
	d_rangeSync = !d_rangeSync;
	if( d_rangeSync )
		GlobalCursor::addObserver( this );	// TODO: preset Cursor
	else if( !d_cursorSync )
		GlobalCursor::removeObserver( this );
}

void PolyScope3::handleEditAttsSys3D()
{
	Spin* spin = getSelectedSpin();
    ENABLED_IF( !d_spec3D.isNull() && !d_cur.isZero() && spin && spin->getSystem() );

    DynValueEditor::edit( getQt(),
		d_pro->getRepository()->findObjectDef( Repository::keySpinSystem ), 
		spin->getSystem() );
}

void PolyScope3::handleEditAttsLink3D()
{
	Spin* spin = getSelectedSpin();
	SpinLink* l = 0;
	if( d_cur[ DimX ] )
        l = d_cur[ DimX ]->findLink( spin );
    ENABLED_IF( !d_spec3D.isNull() && !d_cur.isZero() && l );

    DynValueEditor::edit( getQt(),
		d_pro->getRepository()->findObjectDef( Repository::keyLink ), l );
}

void PolyScope3::handleOverlayCount()
{
    ENABLED_IF( true );

	Root::Index c;
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    if( a->property("0").isNull() )
	{
		bool ok	= FALSE;
		c = QInputDialog::getInteger( "Set Overlay Count", 
			"Please	enter a positive number:", 
            d_plane.d_ol.size(), 1, 9, 1, &ok, getQt() );
		if( !ok )
			return;
	}else
        c = a->property("0").toInt();
	if( c < 1 )
	{
        QMessageBox::critical( getQt(), "Set Overlay Count",
				"Invalid Count!", "&Cancel" );
		return;
	}
	initOverlay( c );
}

void PolyScope3::handleActiveOverlay()
{
    ENABLED_IF( true );

	Root::Index c;
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    if( a->property("0").isNull() )
	{
		Dlg::StringList l( d_plane.d_ol.size() );
		QString str;
		for( int i = 0; i < d_plane.d_ol.size(); i++ )
		{
			if( d_plane.d_ol[ i ].d_spec )
				str.sprintf( "&%d %s", i, d_plane.d_ol[ i ].d_spec->getName() );
			else
				str.sprintf( "&%d <empty>", i );
			l[ i ] = str.toLatin1();
		}
        c = Dlg::getOption( getQt(), l,
			"Select Active Overlay", d_aol );
		if( c == -1 )
			return;
	}else
        c = a->property("0").toInt();
	if( c < 0 || c >= d_plane.d_ol.size() )
	{
        QMessageBox::critical( getQt(), "Set Active Overlay",
				"Invalid Overlay Number!", "&Cancel" );
		return;
	}
	setActiveOverlay( c );
}

void PolyScope3::handleSetPosColor()
{
    ENABLED_IF( true );
	
	QColor clr = QColorDialog::getColor( d_plane.d_ol[d_aol].d_view->getPosColor(), 
        getQt() );
	if( clr.isValid() )
	{
		d_plane.d_ol[d_aol].d_view->setPosColor( ( clr ) );
		d_plane.d_viewer->redraw();
	}
}

void PolyScope3::handleSetNegColor()
{
    ENABLED_IF( true );

	QColor clr = QColorDialog::getColor( d_plane.d_ol[d_aol].d_view->getNegColor(), 
        getQt() );
	if( clr.isValid() )
	{
		d_plane.d_ol[d_aol].d_view->setNegColor( ( clr ) );
		d_plane.d_viewer->redraw();
	}
}

void PolyScope3::handleOverlaySpec()
{
    ENABLED_IF( true );

	Spectrum* spec = d_plane.d_ol[d_aol].d_spec;
	Root::Index c = 0;
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    if( a->property("0").isNull() )
	{
		Dlg::StringSet s;
		Project::SpectrumMap::const_iterator i;
		s.insert( "" ); // empty
		for( i = d_pro->getSpectra().begin(); i != d_pro->getSpectra().end(); ++i )
			if( (*i).second->getDimCount() == 2 )
				s.insert( (*i).second->getName() );
        if( !Dlg::selectStrings( getQt(),
			"Select Overlay Spectrum", s, false ) || s.empty() )
			return;
		spec = d_pro->findSpectrum( (*s.begin()).data() );
	}else
	{
        c = a->property("0").toInt();

		spec = d_pro->getSpec( c );
		if( spec == 0 && c != 0 )
		{
            QMessageBox::critical( getQt(), "Set Overlay Spectrum",
					"Invalid spectrum ID!", "&Cancel" );
			return;
		}
		if( spec && spec->getDimCount() != 2 )
		{
            QMessageBox::critical( getQt(), "Set Overlay Spectrum",
					"Invalid number of dimensions!", "&Cancel" );
			return;
		}
	}
	if( d_aol == 0 && spec == 0 )
	{
        QMessageBox::critical( getQt(), "Set Overlay Spectrum",
				"Cannot remove spectrum of layer 0!", "&Cancel" );
		return;
	}
	if( d_aol == 0 )
		setSpec2D( spec );
	else if( spec )
	{
		d_plane.d_ol[d_aol].d_spec = new SpecProjector( spec, DimX, DimY );
		d_plane.d_ol[d_aol].d_buf->setSpectrum( d_plane.d_ol[d_aol].d_spec );
		d_plane.d_viewer->redraw();
	}else
	{
		d_plane.d_ol[d_aol].d_spec = 0;
		d_plane.d_ol[d_aol].d_buf->setSpectrum( 0 );
		d_plane.d_viewer->redraw();
	}
	updatePlaneLabel();
}

void PolyScope3::handleCntFactor()
{
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    ENABLED_IF( !d_show3DPlane && !a->property("0").isNull() );

    float g = a->property("0").toDouble();
	if( g <= 1.0 || g > 10.0 )
	{
		Root::ReportToUser::alert( this, "Set Contour Factor", "Invalid Factor Value" );
		return;
	}
	int l = d_aol;
    if( a->property("1").isNull() )
		l = selectLayer();
    else if( ::strcmp( a->property("1").toByteArray(), "*" ) == 0 )
		l = -1;
	else
        l = a->property("1").toInt();
	if( l == -2 )
		return;
	else if( l == -1 ) // All
	{
		for( l = 0; l < d_plane.d_ol.size(); l++ )
		{
			d_plane.d_ol[l].d_view->setFactor( g );
			d_plane.d_ol[l].d_view->setVisi( true );
			updateContour( l, false );
		}
		d_plane.d_viewer->damageMe();
	}else if( l >= 0 && l < d_plane.d_ol.size() )
	{
		d_plane.d_ol[l].d_view->setFactor( g );
		d_plane.d_ol[l].d_view->setVisi( true );
		updateContour( l, true );
	}
}

void PolyScope3::handleCntThreshold()
{
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    ENABLED_IF( !d_show3DPlane && !a->property("0").isNull() );

    float g = a->property("0").toDouble();
	if( g < 0.0 )
	{
		Root::ReportToUser::alert( this, "Set Spectrum Threshold", "Invalid Threshold Value" );
		return;
	}
	int l = d_aol;
    if( a->property("1").isNull() )
		l = selectLayer();
    else if( ::strcmp( a->property("1").toByteArray(), "*" ) == 0 )
		l = -1;
	else
        l = a->property("1").toInt();
	if( l == -2 )
		return;
	else if( l == -1 ) // All
	{
		for( l = 0; l < d_plane.d_ol.size(); l++ )
		{
			if( d_plane.d_ol[l].d_spec )
				d_plane.d_ol[l].d_spec->setThreshold( g );
			d_plane.d_ol[l].d_view->setVisi( true );
			d_plane.d_ol[l].d_view->createLevelsMin( g );
		}
	}else if( l >= 0 && l < d_plane.d_ol.size() && d_plane.d_ol[l].d_spec )
	{
		d_plane.d_ol[l].d_spec->setThreshold( g );
		d_plane.d_ol[l].d_view->setVisi( true );
		d_plane.d_ol[l].d_view->createLevelsMin( g );
	}
	d_plane.d_viewer->damageMe();
}

void PolyScope3::handleCntOption()
{
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    ENABLED_IF( !d_show3DPlane && !a->property("0").isNull() );

	ContourView::Option o = ContourView::Both;
    if( strcmp( a->property("0").toByteArray(), "+" ) == 0 )
		o = ContourView::Positive;
    else if( strcmp( a->property("0").toByteArray(), "-" ) == 0 )
		o = ContourView::Negative;
	
	int l = d_aol;
    if( a->property("1").isNull() )
		l = selectLayer();
    else if( ::strcmp( a->property("1").toByteArray(), "*" ) == 0 )
		l = -1;
	else
        l = a->property("1").toInt();
	if( l == -2 )
		return;
	else if( l == -1 ) // All
	{
		for( l = 0; l < d_plane.d_ol.size(); l++ )
		{
			d_plane.d_ol[l].d_view->setOption( o );
			d_plane.d_ol[l].d_view->setVisi( true );
			updateContour( l, false );
		}
		d_plane.d_viewer->damageMe();
	}else if( l >= 0 && l < d_plane.d_ol.size() )
	{
		d_plane.d_ol[l].d_view->setOption( o );
		d_plane.d_ol[l].d_view->setVisi( true );
		updateContour( l, true );
	}
}

void PolyScope3::handleAddLayer()
{
    ENABLED_IF( true );

	initOverlay( d_plane.d_ol.size() + 1 );
	setActiveOverlay( d_plane.d_ol.size() - 1 );
    handleOverlaySpec();
}

void PolyScope3::handleComposeLayers()
{
    ENABLED_IF( true );

	ColorMap cm( 2 );
	Root::Ref<PeakList> pl = new PeakList( cm );
	PeakList::SpecList l;
	for( int i = 0; i < d_plane.d_ol.size(); i++ )
		if( d_plane.d_ol[i].d_spec )
			l.push_back( d_plane.d_ol[i].d_spec->getId() );
	pl->setSpecs( l );
    SpecBatchList dlg( getQt(), pl, d_pro );
	dlg.setCaption( "Compose Layers" );
	if( dlg.doit() && !pl->getSpecs().empty() )
	{
		const PeakList::SpecList& s = pl->getSpecs();
		initOverlay( s.size() );
		Spectrum* spec;
		for( int i = 0; i < s.size(); i++ )
		{
			spec = d_pro->getSpec( s[ i ] );
			assert( spec );
			d_plane.d_ol[i].d_spec = new SpecProjector( spec, DimX, DimY );
			d_plane.d_ol[i].d_buf->setSpectrum( d_plane.d_ol[i].d_spec );
		}
		d_plane.d_viewer->redraw();
		updatePlaneLabel();
	}
}

void PolyScope3::handleUseLinkColors()
{
    CHECKED_IF( true, d_plane.d_tuples->getColors() );
	if( d_plane.d_tuples->getColors() )
		d_plane.d_tuples->setColors( 0 );
	else
		d_plane.d_tuples->setColors( d_pro->getRepository()->getColors() );
}

void PolyScope3::handleUseLinkColors3D()
{
    CHECKED_IF( true, d_strips[DimX].d_tuples->getColors() );

	if( d_strips[DimX].d_tuples->getColors() )
	{	
		d_strips[DimX].d_tuples->setColors( 0 );
		d_strips[DimY].d_tuples->setColors( 0 );
	}else
	{
		d_strips[DimX].d_tuples->setColors( d_pro->getRepository()->getColors() );
		d_strips[DimY].d_tuples->setColors( d_pro->getRepository()->getColors() );
	}
}

void PolyScope3::handleSetLinkParams()
{
	if( d_plane.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_plane.d_tuples->getSel().begin();
	SpinLink* l = tuple[ DimX ]->findLink( tuple[ DimY ] );
    ENABLED_IF( l );
	const SpinLink::Alias& al = l->getAlias( d_spec2D );
	Dlg::LinkParams2 par;
	par.d_rating = al.d_rating;
	par.d_code = al.d_code;
	par.d_visible = al.d_visible;
    if( Dlg::getLinkParams2( getQt(), par ) )
		d_pro->getSpins()->setAlias( l, d_spec2D, 
		par.d_rating, par.d_code, par.d_visible );
}

void PolyScope3::handleSetLinkParams3D()
{
	Spin* spin = getSelectedSpin();
	SpinLink* l = 0;
	if( d_cur[ DimX ] )
        l = d_cur[ DimX ]->findLink( spin );
    ENABLED_IF( !d_spec3D.isNull() && !d_cur.isZero() && l );

	const SpinLink::Alias& al = l->getAlias( d_spec3D );
	Dlg::LinkParams2 par;
	par.d_rating = al.d_rating;
	par.d_code = al.d_code;
	par.d_visible = al.d_visible;
    if( Dlg::getLinkParams2( getQt(), par ) )
		d_pro->getSpins()->setAlias( l, d_spec3D, 
		par.d_rating, par.d_code, par.d_visible );
}

void PolyScope3::handleGotoPoint()
{
    ENABLED_IF( true );

	PpmPoint orig = d_cursor;
    GotoDlg dlg( getQt() );
	Spectrum* spec = d_spec2D;
	if( d_spec3D )
		spec = d_spec3D;
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

void PolyScope3::handleNewPeakList()
{
    ENABLED_IF( true );
	if( !askToClosePeaklist() )
		return;

	Spectrum* spec = d_spec2D;
	if( d_spec3D && d_show3DPlane )
		spec = d_spec3D;
	setPeakList( new PeakList( spec ) );
}

void PolyScope3::handleOpenPeakList()
{
    ENABLED_IF( true );

    PeakList* pl = Dlg::selectPeakList( getQt(), d_pro );
	if( pl == 0 )
		return;
	setPeakList( pl );
}

void PolyScope3::handleSavePeakList()
{
    ENABLED_IF( d_pl && d_pl->getId() == 0 );

	savePeakList();
}

void PolyScope3::handleMapPeakList()
{
    ENABLED_IF( d_pl );

    RotateDlg dlg( getQt(), "Peaklist", "Spectrum" );
	Rotation rot( d_pl->getDimCount() );
	Spectrum* spec = d_spec2D;
	if( d_spec3D && d_show3DPlane )
		spec = d_spec3D;
	for( Dimension d = 0; d < d_pl->getDimCount(); d++ )
	{
		dlg.addDimension( d_pl->getAtomType( d ).getIsoLabel(), 
		( d < spec->getDimCount() )?spec->getColor( d ).getIsoLabel():"" );
		rot[ d ] = d;
	}

	if( dlg.rotate( rot ) )
		d_pl->setRotation( rot );
}

void PolyScope3::handlePickPlPeak()
{
    ENABLED_IF( d_pl );

	assert( d_pl );
	PeakSpace::PeakData pd;
	PeakModel::Params pp;
	pd.d_pos[ DimX ] = d_cursor[ DimX ];
	pd.d_pos[ DimY ] = d_cursor[ DimY ];
	pd.d_pos[ DimZ ] = d_cursor[ DimZ ];
	Spectrum* spec = d_spec2D;
	if( d_spec3D && d_show3DPlane )
		spec = d_spec3D;
	PpmPoint p;
	p.assign( spec->getDimCount(), 0 );
	for( int i = 0; i < p.size(); i++ )
		p[i] = d_cursor[i];
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

void PolyScope3::handleMovePlPeak()
{
    ENABLED_IF( d_pl && d_plane.d_peaks->getSel().size() == 1 );

	Root::Index peak = *d_plane.d_peaks->getSel().begin();
	PeakPos pos;
	PeakModel::Params pp;
	pos[ DimX ] = d_cursor[ DimX ];
	pos[ DimY ] = d_cursor[ DimY ];
	pos[ DimZ ] = d_cursor[ DimZ ];
	Spectrum* spec = d_spec2D;
	if( d_spec3D && d_show3DPlane )
		spec = d_spec3D;
	PpmPoint p;
	p.assign( spec->getDimCount(), 0 );
	for( int i = 0; i < p.size(); i++ )
		p[i] = d_cursor[i];
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

void PolyScope3::handleMovePlAlias()
{
    ENABLED_IF( d_pl && d_plane.d_peaks->getSel().size() == 1 );

	Root::Index peak = *d_plane.d_peaks->getSel().begin();
	PeakPos pos;
	PeakModel::Params pp;
	pos[ DimX ] = d_cursor[ DimX ];
	pos[ DimY ] = d_cursor[ DimY ];
	pos[ DimZ ] = d_cursor[ DimZ ];
	Spectrum* spec = d_spec2D;
	if( d_spec3D && d_show3DPlane )
		spec = d_spec3D;
	PpmPoint p;
	p.assign( spec->getDimCount(), 0 );
	for( int i = 0; i < p.size(); i++ )
		p[i] = d_cursor[i];
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

void PolyScope3::handleLabelPlPeak()
{
    ENABLED_IF( d_pl && d_plane.d_peaks->getSel().size() == 1 );

	Root::Index peak = ( *d_plane.d_peaks->getSel().begin() );
	bool ok	= FALSE;
	QString res	= QInputDialog::getText( "Set Peak Label", 
        "Please	enter a label:", QLineEdit::Normal, d_pl->getTag( peak ).data(), &ok, getQt() );
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

void PolyScope3::handleDeletePlPeaks()
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

void PolyScope3::handleEditPlPeakAtts()
{
    ENABLED_IF( d_pl && d_plane.d_peaks->getSel().size() == 1 );

	Root::Index id = ( *d_plane.d_peaks->getSel().begin() );
	Peak* peak = d_pl->getPeakList()->getPeak( id );
    DynValueEditor::edit( getQt(),
		d_pro->getRepository()->findObjectDef( Repository::keyPeak ), peak );
}

void PolyScope3::handleSetPlColor()
{
    ENABLED_IF( true );

	QColor clr = QColorDialog::getColor( d_plane.d_peaks->getColor(), 
        getQt() );
	if( clr.isValid() )
	{
		d_plane.d_peaks->setColor( ( clr ) );
		d_plane.d_viewer->redraw();
		for( int i = 0; i < d_strips.size(); i++ )
		{
			d_strips[i].d_peaks->setColor( d_plane.d_peaks->getColor() );
			d_strips[i].d_viewer->redraw();
		}
	}
}

void PolyScope3::handleDeleteAliasPeak()
{
    ENABLED_IF( !d_plane.d_tuples->getSel().empty() );

	SpinPointView::Selection sel = d_plane.d_tuples->getSel();
	SpinPointView::Selection::const_iterator p;
    d_pro->getSpins();
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Delete Spins" );
	std::set<Spin*> test;
	for( p = sel.begin(); p != sel.end(); ++p )
	{
		if( test.count( (*p)[ DimX ] ) == 0 )
		{
			cmd->add( new MoveSpinCmd( d_pro->getSpins(), (*p)[ DimX ], 
			(*p)[ DimX ]->getShift(), // auf Home schieben lï¿½cht Alias
			d_spec2D ) );
			test.insert( (*p)[ DimX ] );
		}
		if( test.count( (*p)[ DimY ] ) == 0 )
		{
			cmd->add( new MoveSpinCmd( d_pro->getSpins(), (*p)[ DimY ], 
			(*p)[ DimY ]->getShift(), // auf Home schieben lï¿½cht Alias
			d_spec2D ) );
			test.insert( (*p)[ DimY ] );
		}
	}
    cmd->handle( getAgent() );
}

void PolyScope3::handleFitWindowX()
{
    ENABLED_IF( true );
	d_plane.d_ol[0].d_buf->fitToDim( DimX );
	d_plane.d_viewer->damageMe();
	if( !d_slices.empty() )
	{
		d_slices[ DimX ].d_buf2D->fitToArea();
		d_slices[ DimX ].d_viewer->redraw();
	}
}

void PolyScope3::handleFitWindowY()
{
    ENABLED_IF( true );
	d_plane.d_ol[0].d_buf->fitToDim( DimY );
	d_plane.d_viewer->damageMe();
	if( !d_slices.empty() )
	{
		d_slices[ DimY ].d_buf2D->fitToArea();
		d_slices[ DimY ].d_viewer->redraw();
	}
}

void PolyScope3::handleGotoPlPeak()
{
    ENABLED_IF( d_pl );

	Root::Index id;
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    if( a->property("0").isNull() )
	{
		bool ok	= FALSE;
		id	= QInputDialog::getInteger( "Goto Peak", 
			"Please	enter peak id:", 
            0, -999999, 999999, 1, &ok, getQt() );
		if( !ok )
			return;
	}else
        id = a->property("0").toInt();

	try
	{
		PeakPos pos;
		d_pl->getPos( id, pos, (d_spec3D && d_show3DPlane)?d_spec3D:d_spec2D );
		PpmPoint p;
		p.assign( Root::Math::min( d_pl->getDimCount(), (d_show3DPlane)?3:2 ), 0 );
		for( Dimension d = 0; d < p.size(); d++ )
			p[d] = pos[d];
		setCursor( p );
        centerToCursor( p.size() > 2 );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Goto Peak", e.what() );
	}
}

void PolyScope3::handleViewPlLabels()
{
    Gui2::UiFunction* a = Gui2::UiFunction::me();
    if( a->property("0").isNull() )
		return;

    PeakPlaneView::Label q = (PeakPlaneView::Label) a->property("0").toInt();
	if( q < PeakPlaneView::NONE || q >= PeakPlaneView::END )
		return;

    CHECKED_IF( true, d_plane.d_peaks->getLabel() == q );
	
	d_plane.d_peaks->setLabel( q );
	for( int i = 0; i < d_strips.size(); i++ )
	{
		d_strips[i].d_peaks->setLabel( q );
		d_strips[i].d_viewer->redraw();
	}
	d_plane.d_viewer->redraw();
}

void PolyScope3::handleSyncDepth()
{
    CHECKED_IF( true, d_syncDepth );
	
	d_syncDepth = !d_syncDepth;
	// Unterfeature von SyncCursor
}

void PolyScope3::handleAdjustIntensity()
{
    ENABLED_IF( true );
	
    Dlg::adjustIntensity( getQt(), d_plane.d_intens );
}


