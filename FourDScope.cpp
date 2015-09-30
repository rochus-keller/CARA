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

#include "FourDScope.h"
// Qt
#include <qtextstream.h> 
#include <qinputdialog.h> 
#include <qmessagebox.h>
#include <QFileDialog>
#include <qfileinfo.h> 
#include <qdir.h> 
#include <qmenubar.h>

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
using namespace Lexi;

//* Spec
#include <Spec/Factory.h>
#include <Spec/SpecProjector.h>
#include <Spec/SpectrumPeer.h>
#include <Spec/SpectrumType.h>
#include <Spec/Repository.h>
#include <SpecView/SpecViewer.h>
#include <SpecView/FocusCtrl.h>
#include <SpecView/ContourView.h>
#include <SpecView/IntensityView.h>
#include <SpecView/CursorView.h>
#include <SpecView/CursorCtrl.h>
#include <SpecView/ScrollCtrl.h>
#include <SpecView/ZoomCtrl.h>
#include <SpecView/SelectZoomCtrl.h>
#include <SpecView/SliceView.h>
#include <SpecView/OverviewCtrl.h>
#include <SpecView/PeakPlaneView.h>

#include <FourDScopeAgent.h>
#include <Dlg.h>
#include <StripListGadget.h>

using namespace Spec;

GCC_IGNORE(-Wparentheses);

//////////////////////////////////////////////////////////////////////
// Entkopplung; nutzbar ohne include

void createFourDScope(Root::Agent* a, Spec::Spectrum* s, Spec::Project* p)
{ 
	new FourDScope( a, s, p );
}

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr FourDScope::ShowList = "ShowList";
Root::Action::CmdStr FourDScope::ShowAlignment = "ShowAlignment";
Root::Action::CmdStr FourDScope::UnlinkSucc = "UnlinkSucc";
Root::Action::CmdStr FourDScope::UnlinkPred = "UnlinkPred";
Root::Action::CmdStr FourDScope::AssignStrip = "AssignStrip";
Root::Action::CmdStr FourDScope::UnassignStrip = "UnassignStrip";
Root::Action::CmdStr FourDScope::SetCandidates = "SetCandidates";
Root::Action::CmdStr FourDScope::SetTolerance = "SetTolerance";
Root::Action::CmdStr FourDScope::ExportPeaklist = "ExportPeaklist";
Root::Action::CmdStr FourDScope::SetTolHori = "SetTolHori";
Root::Action::CmdStr FourDScope::SetTolVerti = "SetTolVerti";
Root::Action::CmdStr FourDScope::SetTolStrip = "SetTolStrip";
Root::Action::CmdStr FourDScope::ExportStripPeaks = "ExportStripPeaks";
Root::Action::CmdStr FourDScope::ToMonoScope = "ToMonoScope";
Root::Action::CmdStr FourDScope::ToMonoScope2D = "ToMonoScope2D";
Root::Action::CmdStr FourDScope::SyncHori = "SyncHori";
Root::Action::CmdStr FourDScope::SyncVerti = "SyncVerti";
Root::Action::CmdStr FourDScope::Goto = "Goto";
Root::Action::CmdStr FourDScope::GotoResidue = "GotoResidue";
Root::Action::CmdStr FourDScope::SetWidthFactor = "SetWidthFactor";
Root::Action::CmdStr FourDScope::Use4DSpec = "Use4DSpec";
Root::Action::CmdStr FourDScope::Goto4D = "Goto4D";
Root::Action::CmdStr FourDScope::SaveColors = "SaveColors";

ACTION_SLOTS_BEGIN( FourDScope )
    { FourDScope::SaveColors, &FourDScope::handleSaveColors },
    { FourDScope::Goto4D, &FourDScope::handleGoto4D },
    { FourDScope::Use4DSpec, &FourDScope::handleUse4DSpec },
    { FourDScope::SetWidthFactor, &FourDScope::handleSetWidthFactor },
    { FourDScope::GotoResidue, &FourDScope::handleGotoResidue },
    { FourDScope::SyncHori, &FourDScope::handleSyncHori },
    { FourDScope::SyncVerti, &FourDScope::handleSyncVerti },
    { FourDScope::Goto, &FourDScope::handleGoto },
    { FourDScope::ToMonoScope, &FourDScope::handleToMonoScope },
    { FourDScope::ToMonoScope2D, &FourDScope::handleToMonoScope2D },
    { FourDScope::SetTolStrip, &FourDScope::handleSetTolStrip },
    { FourDScope::ExportStripPeaks, &FourDScope::handleExportStripPeaks },
    { FourDScope::SetTolHori, &FourDScope::handleSetTolHori },
    { FourDScope::SetTolVerti, &FourDScope::handleSetTolVerti },
    { FourDScope::ExportPeaklist, &FourDScope::handleExportPeaklist },
    { FourDScope::SetTolerance, &FourDScope::handleSetTolerance },
    { FourDScope::ShowList, &FourDScope::handleShowList },
ACTION_SLOTS_END( FourDScope )

//////////////////////////////////////////////////////////////////////

static void buildCommands( CommandLine* cl )
{
	cl->registerCommand( new ActionCommand( FourDScopeAgent::AdjustIntensity, "CW", "Adjust Intensity", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::PickPlPeak, "PP", "Pick Peak", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::MovePlPeak, "MP", "Move Peak", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::MovePlAlias, "MA", "Move Peak Alias", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::DeletePlPeaks, "DP", "Delete Peaks", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::LabelPlPeak, "LP", "Label Peak", true ) );

	cl->registerCommand( new ActionCommand( FourDScopeAgent::DeleteAliasPeak, "UA", "Un-Alias Spins", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::GotoPoint, "GT", "Goto", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::PickSystem, "PN", "Pick New System", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::SetLinkParams, "PA", 
		"Set Plane Link Params", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::SetLinkParams4D, "ZPA",
		"Set Ortho Link Params", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::SetSystemType, "SY", "Set System Type", true ) );
	cl->registerCommand( new ActionCommand( FourDScope::SyncHori, "SH", "Sync. Hori. Spin", true ) );
	cl->registerCommand( new ActionCommand( FourDScope::SyncVerti, "SV", "Sync. Verti. Spin", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::RemoveRulers, "RS", "Remove Selected Rulers", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::RemoveAllRulers, "RA", "Remove All Rulers", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::AddRulerVerti, "RH", "Add Horizontal Ruler", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::AddRulerHori, "RV", "Add Vertical Ruler", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::HoldReference, "HR", "Hold Reference", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::ShowFolded, "SF", "Show Folded", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::AutoContour2, "ZAC", "Auto Contour Level", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::AutoContour, "AC", "Auto Contour Level", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::Show4dPlane, "4D", "Show 4D Plane", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::FitWindow4D, "ZWW", "Ortho Fit To Window", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::Backward, "BB", "Backward", true ) );
	cl->registerCommand( new ActionCommand( FourDScope::ShowList, "SL", "Show List", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::CursorSync, "GC", "Sync To Global Cursor", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::FitWindowX, "WX", "Fit X To Window", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::FitWindowY, "WY", "Fit Y To Window", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::FitWindow, "WW", "Plane Fit To Window", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::AutoCenter, "CC", "Center To Peak", true ) );
	cl->registerCommand( new ActionCommand( Root::Action::EditUndo, "ZZ", "Undo", true ) );
	cl->registerCommand( new ActionCommand( Root::Action::EditRedo, "YY", "Redo", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::ShowContour, "SC", "Show Contour", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::ShowIntensity, "SI", "Show Intensity", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::ShowLowRes, 
		"LR", "Low Resolution", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::ContourParams, 
		"CP", "Contour Params", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::ContourParams2, 
		"ZCP", "Contour Params", true ) );

	ActionCommand* cmd = new ActionCommand( 
		FourDScope::SetTolerance, "ST", "Set Tolerance", false );
	cmd->registerParameter( Any::CStr, true, "Atom Type" ); 
	cmd->registerParameter( Any::Float, true, "Tolerance (ppm)" ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( FourDScopeAgent::LinkSystems, "LK", "Link Systems", false );
	cmd->registerParameter( Any::ULong, false, "Predecessor" ); // 1..n Pred, Succ oder Nil
	cmd->registerParameter( Any::ULong, false, "Successor" ); // 1..n Succ oder Nil
	cl->registerCommand( cmd );

	cmd = new ActionCommand( FourDScopeAgent::ViewLabels, 
		"LL", "Plane Label Format", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( FourDScopeAgent::ViewPlLabels, 
		"LF", "Label Format", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( FourDScopeAgent::ViewLabels4D,
		"ZLL", "Ortho Label Format", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cl->registerCommand( new ActionCommand( FourDScopeAgent::PickSystem, 
		"PY", "Plane Pick System", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::ProposePeak, 
		"PR", "Plane Propose Spins", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::MovePeak, 
		"MS", "Plane Move Spins", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::MovePeakAlias, 
		"AY", "Plane Move System Alias", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::PickHori, 
		"EH", "Plane Extend Horizontally", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::ProposeHori, 
		"PH", "Propose Horizontally", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::PickVerti, 
		"EV", "Plane Extend Vertically", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::ProposeVerti, 
		"PV", "Propose Vertically", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::LabelPeak, 
		"LS", "Label Spins", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::DeletePeak, 
		"DS", "Delete Spins", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::DeleteLinks, 
		"DL", "Delete Spin Links", true ) );

	cl->registerCommand( new ActionCommand( FourDScopeAgent::PickSpin4D,
        "PO", "Ortho Pick Peak", true ) );
    cl->registerCommand( new ActionCommand( FourDScopeAgent::ProposeSpin,
		"RI", "Ortho Propose Spins", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::MoveSpin4D,
        "MO", "Ortho Move Peak", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::MoveSpinAlias4D,
        "OA", "Ortho Move Peak Alias", true ) );
    cl->registerCommand( new ActionCommand( FourDScopeAgent::PickHori4D,
		"ZEH", "Ortho Extend Horizontally", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::PickVerti4D,
		"ZEV", "Ortho Extend Vertically", true ) );

	cl->registerCommand( new ActionCommand( FourDScopeAgent::ShowWithOff2, 
		"RP", "Resolve Projected Spins", true ) );

	cmd = new ActionCommand( FourDScopeAgent::GotoSystem, 
		"GY", "Goto System", false );
	cmd->registerParameter( Any::CStr, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( FourDScopeAgent::GotoPeak, 
		"GS", "Goto Spins", false );
	cmd->registerParameter( Any::CStr, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( FourDScopeAgent::GotoPlPeak, 
		"GP", "Goto Peak", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( FourDScopeAgent::OverlayCount, 
		"OC", "Overlay Count", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder Count
	cl->registerCommand( cmd );

	cmd = new ActionCommand( FourDScopeAgent::ActiveOverlay, 
        "AO", "Active Overlay", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( FourDScopeAgent::OverlaySpec, 
		"YS", "Overlay Spectrum", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cl->registerCommand( new ActionCommand( FourDScopeAgent::SetPosColor, "PC", "Set Positive Color", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::SetNegColor, "NC", "Set Negative Color", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::ComposeLayers, "CL", "Compose Layers", true ) );

	cmd = new ActionCommand( FourDScopeAgent::AddLayer, 
		"AL", "Add Overlay Layer", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( FourDScopeAgent::CntThreshold, 
		"CT", "Contour Threshold", false );
	cmd->registerParameter( Any::Float, true ); 
	cmd->registerParameter( Any::CStr, false ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( FourDScopeAgent::CntFactor, 
		"CF", "Contour Factor", false );
	cmd->registerParameter( Any::Float, true ); 
	cmd->registerParameter( Any::CStr, false ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( FourDScopeAgent::CntOption, 
		"CO", "Contour Options", false );
	cmd->registerParameter( Any::CStr, false ); // Leer oder + oder -
	cmd->registerParameter( Any::CStr, false ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( FourDScope::GotoResidue, 
		"GR", "Goto Residue", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cl->registerCommand( new ActionCommand( FourDScopeAgent::NextSpec2D, 
		"NS", "Next Spectrum", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::PrevSpec2D, 
		"PS", "Prev. Spectrum", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::NextSpec4D,
		"NT", "Ortho Next Spectrum", true ) );
	cl->registerCommand( new ActionCommand( FourDScopeAgent::PrevSpec4D,
		"PT", "Ortho Prev. Spectrum", true ) );

	cmd = new ActionCommand( FourDScopeAgent::LabelHori, 
		"LH", "Label Horizontal Spin", false );
	cmd->registerParameter( Any::CStr, false ); // Label oder leer
	cl->registerCommand( cmd );

	cmd = new ActionCommand( FourDScopeAgent::LabelVerti, 
		"LV", "Label Vertical Spin", false );
	cmd->registerParameter( Any::CStr, false ); // Label oder leer
	cl->registerCommand( cmd );

	cmd = new ActionCommand( FourDScopeAgent::PickLabel4D,
        "PL", "Ortho Pick Labels", true );
	cl->registerCommand( cmd );

	cmd = new ActionCommand( FourDScopeAgent::LabelSpin4D,
        "LZ", "Label Ortho Spins", true );
	cl->registerCommand( cmd );

	cmd = new ActionCommand( Root::Action::ExecuteLine, "LUA", "Lua", false );
	cmd->registerParameter( Any::Memo, true ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( FourDScopeAgent::AutoGain, 
		"AG", "Set Auto Contour Gain", false );
	cmd->registerParameter( Any::Float, true ); 
	cmd->registerParameter( Any::CStr, false ); // Label oder leer
	cl->registerCommand( cmd );

	cmd = new ActionCommand( FourDScopeAgent::AutoGain4D,
		"ZAG", "Set Ortho Auto Contour Gain", false );
	cmd->registerParameter( Any::Float, true ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( FourDScope::SetWidthFactor, 
		"WF", "Set Ortho Width Factor", false );
	cmd->registerParameter( Any::Float, false ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( FourDScopeAgent::SetWidth,
		"SW", "Set Peak Width", false );
	cmd->registerParameter( Any::Float, false ); 
	cl->registerCommand( cmd );

    cl->registerCommand( new ActionCommand( FourDScopeAgent::ShowPathSim, "SP", "Show Path Simulation", true ) );
}

//////////////////////////////////////////////////////////////////////

static QColor g_frameClr = Qt::lightGray;


FourDScope::FourDScope(Root::Agent * a, Spectrum* spec, Project* pro ):
	Lexi::MainWindow( a, true, true ), d_showList( false ), d_ovCtrl( 0 ), 
	d_use4D( true ), d_goto4D( false ), d_agent(0)
{
	d_agent = new FourDScopeAgent( this, spec, pro );
	
	Root::UpstreamFilter* filter = new UpstreamFilter( this, true );

	d_focus = new FocusManager( nil, true );
	Redirector* redir = new Redirector( new Background( d_focus, Qt::black, true ) );
	CommandLine* cl = new CommandLine( d_agent );
	redir->setKeyHandler( cl );
	buildCommands( cl );

	d_widget = new GlyphWidget( getQt(), new Bevel( redir, true, false ), filter, true );
	getQt()->setCentralWidget( d_widget );
	d_widget->setFocusGlyph( redir );

	buildMenus();
	getQt()->resize( 600, 400 ); // RISK
	getQt()->showMaximized();
	updateCaption();
	buildViews();
}

FourDScope::FourDScope(Root::Agent * a ):
	Lexi::MainWindow( a, true, true ), d_showList( false ), d_ovCtrl( 0 ), 
	d_use4D( true ), d_goto4D( false ), d_agent( 0 ), d_focus( 0 ), d_widget( 0 )
{

}

FourDScope::~FourDScope()
{
	d_widget->setBody( 0 );
}

void FourDScope::updateCaption()
{
	QString str;
	str.sprintf( "FourDScope - %s", d_agent->getSpec()->getName() );
	getQt()->setCaption( str );
}

void FourDScope::buildMenus()
{
	Gui::Menu* menuFile = new Gui::Menu( menuBar() );
    Gui::Menu::item( menuFile, this, Root::Action::FileSave, Qt::CTRL+Qt::Key_S );
	Gui::Menu::item( menuFile, d_agent, "Print Preview...", 
		FourDScopeAgent::CreateReport, false, Qt::CTRL+Qt::Key_P );
	Gui::Menu* sub = new Gui::Menu( menuFile );
	Gui::Menu::item( sub, this, "&Anchor Peaklist...", ExportPeaklist, false );
	Gui::Menu::item( sub, this, "&Ortho Peaklist...", ExportStripPeaks, false );
	Gui::Menu::item( sub, this, "Ortho peaks to &MonoScope...", ToMonoScope, false );
	Gui::Menu::item( sub, this, "Plane peaks to &MonoScope...", ToMonoScope2D, false );
	menuFile->insertItem( "Export", sub );
	menuFile->insertSeparator();
    Gui::Menu::item( menuFile, this, Root::Action::WindowClose, Qt::CTRL+Qt::Key_W );
	menuBar()->insertItem( "&File", menuFile );

	Gui::Menu* menuEdit = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuEdit, this, Root::Action::EditUndo, Qt::CTRL+Qt::Key_Z );
	Gui::Menu::item( menuEdit, this, Root::Action::EditRedo, Qt::CTRL+Qt::Key_Y );
	menuBar()->insertItem( "&Edit", menuEdit );

	Gui::Menu* menuView = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuView, d_agent, "Backward", 
		FourDScopeAgent::Backward, false, Qt::CTRL+Qt::Key_B );
	Gui::Menu::item( menuView, d_agent, "Forward", 
		FourDScopeAgent::Forward, false );

	Gui::Menu::item( menuView, d_agent, "Fit Window", 
		FourDScopeAgent::FitWindow, false, Qt::CTRL+Qt::Key_Home );
	Gui::Menu::item( menuView, d_agent, "Goto System...", 
		FourDScopeAgent::GotoSystem, false, Qt::CTRL+Qt::Key_G );
	Gui::Menu::item( menuView, d_agent, "Goto Point...", 
		FourDScopeAgent::GotoPoint, false );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Set Resolution...", 
		FourDScopeAgent::SetResolution, false );
	Gui::Menu::item( menuView, d_agent, "Show Low Resolution", 
		FourDScopeAgent::ShowLowRes, true );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Do Pathway Simulation", 
		FourDScopeAgent::ForceCross, true );
	Gui::Menu::item( menuView, d_agent, "Show Ghost Peaks", 
		FourDScopeAgent::ShowGhosts, true );
	Gui::Menu::item( menuView, d_agent, "Show Ghost Labels", 
		FourDScopeAgent::GhostLabels, true );
	Gui::Menu::item( menuView, d_agent, "Show Hidden Links", 
		FourDScopeAgent::ShowAllPeaks, true );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Hide 4D Slices",
		FourDScopeAgent::AutoHide, true );
	Gui::Menu::item( menuView, d_agent, "Show Folded", 
		FourDScopeAgent::ShowFolded, true );
	Gui::Menu::item( menuView, d_agent, "Sync to Global Zoom", 
		FourDScopeAgent::RangeSync, true );
	Gui::Menu::item( menuView, d_agent, "Sync to Global Cursor", 
		FourDScopeAgent::CursorSync, true );
	Gui::Menu::item( menuView, d_agent, "Sync to Depth", 
		FourDScopeAgent::SyncDepth, true );
	Gui::Menu::item( menuView, d_agent, "Center to Peak", 
		FourDScopeAgent::AutoCenter, true );
	Gui::Menu::item( menuView, d_agent, "Show List", ShowList, true );

	menuBar()->insertItem( "&View", menuView );

	Gui::Menu* menuPicking = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuPicking, d_agent, "&Pick New System", 
		FourDScopeAgent::PickSystem, false );
	Gui::Menu::item( menuPicking, d_agent, "Propose System...", 
		FourDScopeAgent::ProposePeak, false );
	Gui::Menu::item( menuPicking, d_agent, "Extend Horizontally", 
		FourDScopeAgent::PickHori, false );
	Gui::Menu::item( menuPicking, d_agent, "Propose Horizontally...", 
		FourDScopeAgent::ProposeHori, false );
	Gui::Menu::item( menuPicking, d_agent, "Extend Vertically", 
		FourDScopeAgent::PickVerti, false );
	Gui::Menu::item( menuPicking, d_agent, "Propose Vertically...", 
		FourDScopeAgent::ProposeVerti, false );
	Gui::Menu::item( menuPicking, d_agent, "&Move Spins", 
		FourDScopeAgent::MovePeak, false );
	Gui::Menu::item( menuPicking, d_agent, "Move Spin &Aliases", 
		FourDScopeAgent::MovePeakAlias, false );
	Gui::Menu::item( menuPicking, d_agent, "&Label Spins...", 
		FourDScopeAgent::LabelPeak, false );
	Gui::Menu::item( menuPicking, d_agent, "Hide/Show Link", 
		FourDScopeAgent::HidePeak, false );
	Gui::Menu::item( menuPicking, d_agent, "Set Link Params...", 
		FourDScopeAgent::SetLinkParams, false );
	menuPicking->insertSeparator();
	Gui::Menu::item( menuPicking, this, "Set Verti. Tolerance...", 
		FourDScope::SetTolVerti, false );
	Gui::Menu::item( menuPicking, this, "Set Hori. Tolerance...", 
		FourDScope::SetTolHori, false );
	menuPicking->insertSeparator();
	Gui::Menu::item( menuPicking, d_agent, "Un-Alias Peaks", 
		FourDScopeAgent::DeleteAliasPeak, false );
	Gui::Menu::item( menuPicking, d_agent, "Delete Peaks", 
		FourDScopeAgent::DeletePeak, false );
	Gui::Menu::item( menuPicking, d_agent, "Delete Spin Links", 
		FourDScopeAgent::DeleteLinks, false );
	Gui::Menu::item( menuPicking, d_agent, "Delete Horizontal Spin", 
		FourDScopeAgent::DeleteSpinX, false );
	Gui::Menu::item( menuPicking, d_agent, "Delete Vertical Spin", 
		FourDScopeAgent::DeleteSpinY, false );

	Gui::Menu* menuAssig = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuAssig, d_agent, "&Link Systems...", 
		FourDScopeAgent::LinkSystems, false );
	Gui::Menu::item( menuAssig, d_agent, "Unlink &Predecessor",
		FourDScopeAgent::UnlinkPred, false );
	Gui::Menu::item( menuAssig, d_agent, "Unlink &Successor", 
		FourDScopeAgent::UnlinkSucc, false );
	menuAssig->insertSeparator();
	Gui::Menu::item( menuAssig, d_agent, "&Assign System...", 
		FourDScopeAgent::Assign, false );
	Gui::Menu::item( menuAssig, d_agent, "&Unassign System",
		FourDScopeAgent::Unassign, false );
	Gui::Menu::item( menuAssig, d_agent, "Set System &Type...",
		FourDScopeAgent::SetSystemType, false );
	Gui::Menu::item( menuAssig, d_agent, "Set Assig. Candidates...",
		FourDScopeAgent::SetCandidates, false );
	Gui::Menu::item( menuAssig, d_agent, "Show Ali&gnment...", 
		FourDScopeAgent::ShowAlignment, false );

	Gui::Menu* menuRuler = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuRuler, d_agent, "Add &Horizontal Ruler", 
		FourDScopeAgent::AddRulerVerti, false );
	Gui::Menu::item( menuRuler, d_agent, "Add &Vertical Ruler", 
		FourDScopeAgent::AddRulerHori, false );
	Gui::Menu::item( menuRuler, d_agent, "Remove Selected Rulers", 
		FourDScopeAgent::RemoveRulers, false );
	Gui::Menu::item( menuRuler, d_agent, "Remove All Rulers", 
		FourDScopeAgent::RemoveAllRulers, false );
	Gui::Menu::item( menuRuler, d_agent, "Set Rulers to Reference", 
		FourDScopeAgent::AutoRuler, true );

	Gui::Menu* menuAtts = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuAtts, d_agent, "Horizontal Spin...", 
		FourDScopeAgent::EditAttsSpinH, false );
	Gui::Menu::item( menuAtts, d_agent, "Vertical Spin...", 
		FourDScopeAgent::EditAttsSpinV, false );
	Gui::Menu::item( menuAtts, d_agent, "Horizontal System...", 
		FourDScopeAgent::EditAttsSysH, false );
	Gui::Menu::item( menuAtts, d_agent, "Vertical System...", 
		FourDScopeAgent::EditAttsSysV, false );
	Gui::Menu::item( menuAtts, d_agent, "Spin Link...", 
		FourDScopeAgent::EditAttsLink, false );

	Gui::Menu* menuOverlay = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuOverlay, d_agent, "Set Layer &Count...", 
		FourDScopeAgent::OverlayCount, false );
	Gui::Menu::item( menuOverlay, d_agent, "A&dd Overlay Layer...", 
		FourDScopeAgent::AddLayer, false );
	Gui::Menu::item( menuOverlay, d_agent, "Compose &Layers...", 
		FourDScopeAgent::ComposeLayers, false );
	Gui::Menu::item( menuOverlay, d_agent, "Set &Active Layer...", 
		FourDScopeAgent::ActiveOverlay, false );
	Gui::Menu::item( menuOverlay, d_agent, "Select &Spectrum...", 
		FourDScopeAgent::OverlaySpec, false );
	Gui::Menu::item( menuOverlay, d_agent, "Set &Positive Color...", 
		FourDScopeAgent::SetPosColor, false );
	Gui::Menu::item( menuOverlay, d_agent, "Set &Negative Color...", 
		FourDScopeAgent::SetNegColor, false );
	Gui::Menu::item( menuOverlay, d_agent, "&Make Colors Persistent", 
		FourDScope::SaveColors, false );

	Gui::Menu* menuPlane = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuPlane, d_agent, "&Calibrate", 
		FourDScopeAgent::SpecCalibrate, false );
	menuPlane->insertItem( "Select Spectrum", d_agent->getPopSpec2D() );
	menuPlane->insertItem( "&Overlay", menuOverlay );
	menuPlane->insertSeparator();
	Gui::Menu::item( menuPlane, d_agent, "Show Contour", 
		FourDScopeAgent::ShowContour, true );
	Gui::Menu::item( menuPlane, d_agent, "Show Intensity", 
		FourDScopeAgent::ShowIntensity, true );
	Gui::Menu::item( menuPlane, d_agent, "Auto Contour Level", 
		FourDScopeAgent::AutoContour, true );
	Gui::Menu::item( menuPlane, d_agent, "Set Contour Parameters...", 
		FourDScopeAgent::ContourParams, false );
	menuPlane->insertSeparator();
	menuPlane->insertItem( "&Picking", menuPicking );
	menuPlane->insertItem( "&Assignment", menuAssig );
	menuPlane->insertItem( "&Ruler", menuRuler );
	menuPlane->insertItem( "&Edit Attributes", menuAtts );
	menuPlane->insertSeparator();
	Gui::Menu::item( menuPlane, d_agent, "Hold Reference", 
		FourDScopeAgent::HoldReference, false );
	Gui::Menu::item( menuPlane, d_agent, "Auto Hold", 
		FourDScopeAgent::AutoHold, true );
	menuPlane->insertSeparator();
	Gui::Menu::item( menuPlane, d_agent, "Show 4D Plane",
		FourDScopeAgent::Show4dPlane, true, Qt::CTRL+Qt::Key_3 );
	Gui::Menu::item( menuPlane, d_agent, "Show Spin Links", 
		FourDScopeAgent::ShowLinks, true );
	Gui::Menu::item( menuPlane, d_agent, "Show Infered Peaks", 
		FourDScopeAgent::ShowInfered, true );
	Gui::Menu::item( menuPlane, d_agent, "Show Unlabeled Peaks", 
		FourDScopeAgent::ShowUnlabeled, true );
	Gui::Menu::item( menuPlane, d_agent, "Show Peaks with foreign Labels",
		FourDScopeAgent::ShowUnknown, true );
	Gui::Menu::item( menuPlane, d_agent, "Resolve Projected Spins", 
		FourDScopeAgent::ShowWithOff2, true );
	Gui::Menu::item( menuPlane, d_agent, "Use Link Color Codes", 
		FourDScopeAgent::UseLinkColors, true );
	sub = new Gui::Menu( menuBar() );
	menuPlane->insertItem( "Show Labels", sub );
	short i;
	for( i = SpinPointView::None; i < SpinPointView::End; i++ )
	{
		Gui::Menu::item( sub, d_agent, SpinPointView::menuText[ i ], 
			FourDScopeAgent::ViewLabels, true )->addParam( i );
	}
	menuBar()->insertItem( "&Plane", menuPlane );

	Gui::Menu* menuStrip = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuStrip, d_agent, "Calibrate Orthogonal",
		FourDScopeAgent::StripCalibrate, false );
	Gui::Menu::item( menuStrip, d_agent, "Set Tolerance...", SetTolStrip, false );
	menuStrip->insertSeparator();
	Gui::Menu::item( menuStrip, d_agent, "Auto Contour Level", 
		FourDScopeAgent::AutoContour2, true );
	Gui::Menu::item( menuStrip, d_agent, "Set Contour Parameters...", 
		FourDScopeAgent::ContourParams2, false );

	menuStrip->insertItem( "Select Spectrum", d_agent->getPopSpec4D() );
	menuStrip->insertSeparator();
	Gui::Menu::item( menuStrip, d_agent, "&Pick Spins",
		FourDScopeAgent::PickSpin4D, false );
	Gui::Menu::item( menuStrip, d_agent, "Pick Labels...",
		FourDScopeAgent::PickLabel4D, false );
	Gui::Menu::item( menuStrip, d_agent, "Propose &Spins...",
		FourDScopeAgent::ProposeSpin, false );
	Gui::Menu::item( menuStrip, d_agent, "&Move Spins",
		FourDScopeAgent::MoveSpin4D, false );
	Gui::Menu::item( menuStrip, d_agent, "&Move Spin Aliasses",
		FourDScopeAgent::MoveSpinAlias4D, false );

	Gui::Menu::item( menuStrip, d_agent, "&Label Spins...",
		FourDScopeAgent::LabelSpin4D, false );
	Gui::Menu::item( menuStrip, d_agent, "Hide/Show Link", 
		FourDScopeAgent::HidePeak4D, false );
	Gui::Menu::item( menuStrip, d_agent, "Set Link Params...", 
		FourDScopeAgent::SetLinkParams4D, false );

	Gui::Menu::item( menuStrip, d_agent, "&Delete Spins", 
		FourDScopeAgent::DeleteSpins4D, false );
	Gui::Menu::item( menuStrip, d_agent, "Delete Spin &Links", 
		FourDScopeAgent::DeleteLinks4D, false );

	menuAtts = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuAtts, d_agent, "Horizontal Spin...",
		FourDScopeAgent::EditAttsSpinX4D, false );
    Gui::Menu::item( menuAtts, d_agent, "Vertical Spin...",
		FourDScopeAgent::EditAttsSpinY4D, false );
	Gui::Menu::item( menuAtts, d_agent, "System...",
		FourDScopeAgent::EditAttsSys4D, false );
	Gui::Menu::item( menuAtts, d_agent, "Spin Link...", 
		FourDScopeAgent::EditAttsLink4D, false );
	menuStrip->insertItem( "&Edit Attributes", menuAtts );
	menuStrip->insertSeparator();
	Gui::Menu::item( menuStrip, d_agent, "Show Spin Links", 
		FourDScopeAgent::ShowLinks2, true );
	Gui::Menu::item( menuStrip, d_agent, "Show Infered Peaks",
		FourDScopeAgent::ShowInfered2, true );
	Gui::Menu::item( menuStrip, d_agent, "Show Unlabeled Peaks", 
		FourDScopeAgent::ShowUnlabeled2, true );
	Gui::Menu::item( menuStrip, d_agent, "Show Peaks with foreign Labels",
		FourDScopeAgent::ShowUnknown2, true );
	Gui::Menu::item( menuStrip, d_agent, "Resolve Projected Spins", 
		FourDScopeAgent::ShowWithOff, true );
	Gui::Menu::item( menuStrip, d_agent, "Use Link Color Codes", 
		FourDScopeAgent::UseLinkColors4D, true );
	Gui::Menu::item( menuStrip, d_agent, "Fit Window", 
		FourDScopeAgent::FitWindow4D, false, Qt::Key_Home );
	sub = new Gui::Menu( menuBar() );
	menuStrip->insertItem( "Show Labels", sub );
	for( i = SpinPointView::None; i < SpinPointView::End; i++ )
	{
		Gui::Menu::item( sub, d_agent, SpinPointView::menuText[ i ], 
			FourDScopeAgent::ViewLabels4D, true )->addParam( i );
	}
	menuBar()->insertItem( "&Orthogonal", menuStrip );

	Gui::Menu* menuPeaks = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuPeaks, d_agent, "&New Peaklist", 
		FourDScopeAgent::NewPeakList, false );
	Gui::Menu::item( menuPeaks, d_agent, "&Open Peaklist...", 
		FourDScopeAgent::OpenPeakList, false );
	Gui::Menu::item( menuPeaks, d_agent, "&Add to Repository...", 
		FourDScopeAgent::SavePeakList, false );
	Gui::Menu::item( menuPeaks, d_agent, "Map to Spectrum...", 
		FourDScopeAgent::MapPeakList, false );
	Gui::Menu::item( menuPeaks, d_agent, "Select Color...", 
		FourDScopeAgent::SetPlColor, false );
	sub = new Gui::Menu( menuBar() );
	menuPeaks->insertItem( "Show Labels", sub );
	for( i = PeakPlaneView::NONE; i < PeakPlaneView::END; i++ )
	{
		Gui::Menu::item( sub, d_agent, PeakPlaneView::menuText[ i ], 
			FourDScopeAgent::ViewPlLabels, true )->addParam( i );
	}
	menuPeaks->insertSeparator();
	Gui::Menu::item( menuPeaks, d_agent, "&Pick Peak", 
		FourDScopeAgent::PickPlPeak, false );
	Gui::Menu::item( menuPeaks, d_agent, "&Move Peak", 
		FourDScopeAgent::MovePlPeak, false );
	Gui::Menu::item( menuPeaks, d_agent, "&Move Peak Alias", 
		FourDScopeAgent::MovePlAlias, false );
	Gui::Menu::item( menuPeaks, d_agent, "&Label Peak...", 
		FourDScopeAgent::LabelPlPeak, false );
	Gui::Menu::item( menuPeaks, d_agent, "&Delete Peaks", 
		FourDScopeAgent::DeletePlPeaks, false );
	Gui::Menu::item( menuPeaks, d_agent, "&Edit Attributes...", 
		FourDScopeAgent::EditPlPeakAtts, false );
	menuBar()->insertItem( "&Peaks", menuPeaks );

	Gui::Menu* menuHelp = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuHelp, this, Root::Action::HelpAbout );
	menuBar()->insertItem( "&?", menuHelp );

}

void FourDScope::buildViews( bool reuse )
{
	d_focus->setBody( 0 );

	Splitter* planeSplit = new Splitter( 2, 4 ); // 0:slice, 1:plane, 2:slice, 3:ortho
	planeSplit->setBarWidth( 80 );

	d_focus->clear();
	d_focus->setCircle( !d_showList );

	Splitter* topSplit = new Splitter( 1, (d_showList)?2:1 );
	topSplit->setBarWidth( 80 );
	topSplit->setPane( planeSplit, 0, 0 );

	planeSplit->setPane( d_agent->getPlane().d_viewer, 0, 1 );

	planeSplit->setPane( d_agent->getSlice( DimX ).d_viewer, 1, 1 );

	planeSplit->setPane( d_agent->getSlice( DimY ).d_viewer, 0, 0 );

    planeSplit->setPane( d_agent->getOrtho().d_viewer, 0, 3 );

	planeSplit->setPane( d_agent->getSlice( DimW ).d_viewer, 1, 3 );

	planeSplit->setPane( d_agent->getSlice( DimZ ).d_viewer, 0, 2 );

	if( d_showList )
	{
		d_list = new StripListGadget( d_widget->getViewport(), 
			d_agent->getProject(), this, 0, true );
		topSplit->setPane( d_list, 0, 1 );
		Gui::Menu* menu = new Gui::Menu( d_list->getListView()->getImp(), true );
		Gui::Menu::item( menu, this, "Goto...", Goto, false );
		Gui::Menu::item( menu, this, "Use 4D Navigation", Goto4D, true );
		Gui::Menu::item( menu, this, "Show 4D Values", Use4DSpec, true );
		Gui::Menu::item( menu, d_list, "Find Spin...", StripListGadget::FindSpin, false );
		Gui::Menu::item( menu, d_list, "Find Link Partner...", StripListGadget::GotoOther, false );
		menu->addSeparator();
		Gui::Menu::item( menu, d_list, "Run Automatic Strip Matcher", StripListGadget::RunStripper, true );
		Gui::Menu::item( menu, d_list, "Strict Strip Matching", StripListGadget::StrictStripMatch, true );
		Gui::Menu::item( menu, d_list, "Set Spin Tolerance...", StripListGadget::SetSpinTol, false );
		menu->addSeparator();
		Gui::Menu::item( menu, d_list, "Create System", StripListGadget::CreateSystem, false );
		Gui::Menu::item( menu, d_list, "Eat System...", StripListGadget::EatSystem, false );
		Gui::Menu::item( menu, d_list, "Set Assig. Candidates...", StripListGadget::SetCandidates, false );
		Gui::Menu::item( menu, d_list, "Set System Type...", StripListGadget::SetSysType, false );
		Gui::Menu::item( menu, d_list, "Show Alignment...", StripListGadget::ShowAlignment, false );
		menu->addSeparator();
		Gui::Menu::item( menu, d_list, "Link This", StripListGadget::LinkThis, false );
		Gui::Menu::item( menu, d_list, "Unlink Successor", StripListGadget::UnlinkSucc, false );
		Gui::Menu::item( menu, d_list, "Unlink Predecessor", StripListGadget::UnlinkPred, false );
		menu->addSeparator();
		Gui::Menu::item( menu, d_list, "Create Spin...", StripListGadget::CreateSpin, false );
		Gui::Menu::item( menu, d_list, "Move Spin...", StripListGadget::MoveSpin, false );
		Gui::Menu::item( menu, d_list, "Label Spin...", StripListGadget::LabelSpin, false );
		Gui::Menu::item( menu, d_list, "Force Label...", StripListGadget::ForceLabel, false );
		Gui::Menu::item( menu, d_list, "Accept Label", StripListGadget::AcceptLabel, false );
		menu->addSeparator();
		Gui::Menu::item( menu, d_list, "Assign to...", StripListGadget::Assign, false );
		Gui::Menu::item( menu, d_list, "Unassign", StripListGadget::Unassign, false );
		Gui::Menu::item( menu, d_list, "Delete", StripListGadget::Delete, false );
		Gui::Menu::item( menu, d_list, "Edit Attributes...", StripListGadget::EditAtts, false );
		menu->addSeparator();
		Gui::Menu::item( menu, d_list, "Show Spin Links", StripListGadget::ShowLinks, true );
		Gui::Menu::item( menu, d_list, "Create Link...", StripListGadget::CreateLink, false );
		Gui::Menu::item( menu, d_list, "Set Link Params...", StripListGadget::LinkParams, false );
		menu->addSeparator();
		Gui::Menu::item( menu, d_list, "Open All", StripListGadget::OpenAll, false );
		Gui::Menu::item( menu, d_list, "Close All", StripListGadget::CloseAll, false );
		if( d_use4D )
			d_list->setSpec( d_agent->getSpec4D() );
		else
			d_list->setSpec( d_agent->getSpec2D() );
	}else
		d_list = 0;

	if( reuse && d_ovCtrl )
	{
		d_ovCtrl->setTarget( d_agent->getPlane().d_viewer->getViewArea() );
	}else if( d_agent->getSpec2D() )
	{
		d_ov = new SpecViewer( new ViewAreaMdl( true, true, true, true ) );
		Root::Ref<SpecProjector> pro = new SpecProjector( d_agent->getSpec2D(), DimX, DimY );
		Root::Ref<SpecBufferMdl> mdl = new SpecBufferMdl( d_ov->getViewArea(), pro );
		d_ov->getViews()->append( new IntensityView( mdl ) );
		d_ovCtrl = new OverviewCtrl( mdl, d_agent->getPlane().d_viewer->getViewArea() );
		d_ov->getHandlers()->append( d_ovCtrl );
		d_ov->getHandlers()->append( new FocusCtrl( d_ov ) );
		// mdl->copy( d_agent->getPlane().d_ol[0].d_buf );
	}
	planeSplit->setPane( d_ov, 1, 0 );

	d_focus->append( d_agent->getSlice( DimY ).d_viewer->getController() );
	d_focus->append( d_agent->getPlane().d_viewer->getController() );
	d_focus->append( d_agent->getSlice( DimX ).d_viewer->getController() );
	d_focus->append( d_agent->getSlice( DimZ ).d_viewer->getController() );
    d_focus->append( d_agent->getSlice( DimW ).d_viewer->getController() );
	d_focus->append( d_agent->getOrtho().d_viewer->getController() );

	d_focus->setBody( topSplit );


	if( d_showList )
	{
        // 0:slice, 1:plane, 2:slice, 3:ortho, 1 Twip == 20 Point
		planeSplit->setRowPos( 1,  18 * d_widget->height() );
		planeSplit->setColumnPos( 1,  2 * d_widget->width() );
        planeSplit->setColumnPos( 2,  10 * d_widget->width() );
        planeSplit->setColumnPos( 3,  12 * d_widget->width() );
		topSplit->setColumnPos( 1,  17 * d_widget->width() );
	}else
	{
		planeSplit->setRowPos( 1,  17 * d_widget->height() );
        planeSplit->setColumnPos( 1,  2 * d_widget->width() );
        planeSplit->setColumnPos( 2,  10 * d_widget->width() );
        planeSplit->setColumnPos( 3,  12 * d_widget->width() );
	}
	d_widget->reallocate();
	if( !reuse )
	{
		d_agent->getPlane().d_ol[0].d_buf->fitToArea();
		d_agent->setCursor();
	}
	d_widget->setFocusGlyph( d_agent->getPlane().d_viewer->getController() );

	if( d_ov )
	{ 
		if( d_agent->getSpec2D()->canDownsize() )
		{
			d_ovCtrl->getModel()->fitToArea();
		}else
		{
			d_ovCtrl->getModel()->copy( d_agent->getPlane().d_ol[0].d_buf );
		}
	}
	d_widget->getViewport()->damageAll();
}

Root::Ref<PointSet> FourDScope::createStripPeaks()
{
	Spectrum* spec = d_agent->getSpec4D();
	PpmPoint p( 0, 0, 0, 0 );
	PointSet::Assig a;
	char buf[32];
	Root::Index id;
	Root::Ref<PointSet> ps = Factory::createEasyPeakList( spec );

	LinkFilterRotSpace* mdl = d_agent->getMdl4D();
	LinkFilterRotSpace::Iterator i, _end = mdl->end();
	LinkFilterRotSpace::Element e;
	SpinPointView::Label l = d_agent->getOrtho().d_tuples->getLabel();
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
        if( ( link = e.d_point[ DimX ]->findLink( e.d_point[ DimY ] ) ) )
			ps->setCode( id, link->getAlias( spec ).d_code );
        else if( ( link = e.d_point[ DimX ]->findLink( e.d_point[ DimZ ] ) ) )
			ps->setCode( id, link->getAlias( spec ).d_code );
        else if( ( link = e.d_point[ DimX ]->findLink( e.d_point[ DimW ] ) ) )
			ps->setCode( id, link->getAlias( spec ).d_code );
        else if( ( link = e.d_point[ DimZ ]->findLink( e.d_point[ DimW ] ) ) )
			ps->setCode( id, link->getAlias( spec ).d_code );

		SpinPointView::formatLabel( buf, sizeof(buf), e.d_point, l, DimZ );
		ps->setComment( id, buf );
	}
	return ps;
}

bool FourDScope::askToCloseWindow() const
{
	return d_agent->askToClosePeaklist();
}

void FourDScope::handle(Root::Message& m)
{
	BEGIN_HANDLER();
	MESSAGE( StripListGadget::ActivateMsg, e, m )
	{
        Q_UNUSED(e)
		if( d_list )
		{
			d_agent->gotoTuple( d_list->getSelectedStrip(), d_list->getSelectedSpin(),
				d_list->getSelectedLink(), !d_goto4D );
			d_widget->setFocusGlyph( d_agent->getPlane().d_viewer->getController() );
		}
		m.consume();
	}MESSAGE( FourDScopeAgent::SpecChanged, e, m )
	{
		if( d_list && d_agent )
		{
			if( d_use4D )
				d_list->setSpec( d_agent->getSpec4D() );
			else
				d_list->setSpec( d_agent->getSpec2D() );
		}
		if( d_ovCtrl && !e->d_threeD )
		{
			d_ovCtrl->getModel()->setSpectrum( 
				new SpecProjector( d_agent->getSpec2D(), DimX, DimY ) );
			d_ov->redraw();
		}
		m.consume();
	}MESSAGE( Root::Action, a, m )
	{
		if( !EXECUTE_ACTION( FourDScope, *a ) )
			MainWindow::handle( m );
	}HANDLE_ELSE()
		MainWindow::handle( m );
	END_HANDLER();
}

void FourDScope::handleShowList(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_showList );

	d_showList = !d_showList;
	Lexi::Viewport::pushHourglass();
	buildViews( true );
	Lexi::Viewport::popCursor();
}

void FourDScope::handleSetTolerance(Action & a)
{
    ACTION_ENABLED_IF( a, a.getParamCount() == 2 );

	AtomType t = AtomType::parseLabel( a.getParam( 0 ).getCStr() );
	if( t.isNone() )
	{
		Lexi::ShowStatusMessage msg( "Set Tolerance: invalid atom type" );
		traverseUp( msg );
		return;
	}
	PPM p = a.getParam( 1 ).getFloat();
	d_agent->getProject()->getMatcher()->setTol( t, p );
}

void FourDScope::handleExportPeaklist(Action & a)
{
    ACTION_ENABLED_IF( a, true );

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

Root::Ref<PointSet> FourDScope::createPlanePeaks()
{
	Spectrum* spec = d_agent->getSpec();
	PpmPoint p( 0, 0 );
	PointSet::Assig a;
	Root::Index id;
	Root::Ref<PointSet> ps = Factory::createEasyPeakList( spec );
	LinkFilterRotSpace* mdl = d_agent->getMdl2D();
	LinkFilterRotSpace::Iterator i, _end = mdl->end();
	LinkFilterRotSpace::Element e;
	char buf[32];
	SpinPointView::Label l = d_agent->getPlane().d_tuples->getLabel();
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

void FourDScope::handleSetTolHori(Action & a)
{
    ACTION_ENABLED_IF( a, true );

	AtomType atom;
	atom = d_agent->getSpec()->getColor( DimX );

	bool ok	= FALSE;
	QString res;
	QString str;
	str.sprintf( "Please enter atom positive PPM value for %s:", atom.getIsoLabel() );
	res.sprintf( "%0.3f", d_agent->getProject()->getMatcher()->getTol( atom ) );
	res	= QInputDialog::getText( "Set Matching Tolerance", str, QLineEdit::Normal, res, &ok, getQt() );
	if( !ok )
		return;
	PPM w = res.toFloat( &ok );
	if( !ok ) // RK 31.10.03: w <= 0 neu erlaubt
	{
		QMessageBox::critical( getQt(), "Set Spin Tolerance", "Invalid tolerance!", "&Cancel" );
		return;
	}

	d_agent->getProject()->getMatcher()->setTol( atom, w );
}

void FourDScope::handleSetTolVerti(Action & a)
{
    ACTION_ENABLED_IF( a, true );

	AtomType atom;
	atom = d_agent->getSpec()->getColor( DimY );

	bool ok	= FALSE;
	QString res;
	res.sprintf( "%0.3f", d_agent->getProject()->getMatcher()->getTol( atom ) );
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

	d_agent->getProject()->getMatcher()->setTol( atom, w );
}

void FourDScope::handleSetTolStrip(Action & a)
{
    ACTION_ENABLED_IF( a, d_agent->getSpec4D() );

	AtomType atom;
	atom = d_agent->getSpec4D()->getColor( DimZ );

	bool ok	= FALSE;
	QString res;
	res.sprintf( "%0.3f", d_agent->getProject()->getMatcher()->getTol( atom ) );
	QString str;
	str.sprintf( "Please enter atom positive PPM value for %s:", atom.getIsoLabel() );
	res	= QInputDialog::getText( "Set Orthogonal Tolerance", str, QLineEdit::Normal, res, &ok, getQt() );
	if( !ok )
		return;
	PPM w = res.toFloat( &ok );
	if( !ok || w < 0.0 )
	{
		QMessageBox::critical( getQt(), "Set Orthogonal Tolerance", "Invalid tolerance!", "&Cancel" );
		return;
	}

	d_agent->getProject()->getMatcher()->setTol( atom, w );
}

void FourDScope::handleExportStripPeaks(Action & a)
{
	Spectrum* spec = d_agent->getSpec4D();
    ACTION_ENABLED_IF( a, spec );

	QString fileName = QFileDialog::getSaveFileName( getQt(), "Export 4D Pealist",
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

void FourDScope::handleToMonoScope(Action & a)
{
	Spectrum* spec = d_agent->getSpec4D();
    ACTION_ENABLED_IF( a, spec );

	try
	{
		Root::Ref<PointSet> ps = createStripPeaks();
		if( ps.isNull() )
			throw Root::Exception( "Cannot create Peaklist" );
		Root::Ref<PeakList> pl = new PeakList( spec );
		pl->append( ps ); // pl ist nun eine identische Kopie von ps.
		pl->setName( "Export from FourDScope" );
		createMonoScope( getParent(), spec, d_agent->getProject(), pl, Rotation() );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Error Exporting Peaklist", e.what() );
	}catch( ... )
	{
		Root::ReportToUser::alert( this, "Error Exporting Peaklist",
			"Unknown Exception" );
	}
}

void FourDScope::handleGoto(Action & a)
{
    ACTION_ENABLED_IF( a, d_list );
	d_agent->gotoTuple( d_list->getSelectedStrip(), d_list->getSelectedSpin(),
		d_list->getSelectedLink(), !d_goto4D );
	d_widget->setFocusGlyph( d_agent->getPlane().d_viewer->getController() );
}

void FourDScope::handleSyncHori(Action & a)
{
	Spin* spin = d_agent->getSel( true );
    ACTION_ENABLED_IF( a, d_list && spin );
	d_list->gotoSpin( spin );
}

void FourDScope::handleSyncVerti(Action & a)
{
	Spin* spin = d_agent->getSel( false );
    ACTION_ENABLED_IF( a, d_list && spin );
	d_list->gotoSpin( spin );
}

void FourDScope::handleGotoResidue(Action & a)
{
    ACTION_ENABLED_IF( a, true );

	Root::Index id;
	if( a.getParam( 0 ).isNull() )
	{
		bool ok	= FALSE;
		id	= QInputDialog::getInteger( "Goto Residue", 
			"Please	enter a residue number:", 
			0, -999999, 999999, 1, &ok, getQt() );
		if( !ok )
			return;
	}else
		id = a.getParam( 0 ).getLong();

	Residue* resi = d_agent->getProject()->getSequence()->getResidue( id );
	if( resi == 0 || resi->getSystem() == 0 )
	{
		Lexi::ShowStatusMessage msg( "Goto Residue: unknown or unassigned residue" );
		traverse( msg );
		return;
	}

	Root::MenuParamAction action( d_agent, "Goto System",
		FourDScopeAgent::GotoSystem, false );
	action.addParam( Root::Int32(resi->getSystem()->getId()) );
	action.execute();
}

void FourDScope::handleSetWidthFactor(Action & a)
{
    ACTION_ENABLED_IF( a, true );

	float g;
	if( a.getParam( 0 ).isNull() )
	{
		bool ok	= FALSE;
		QString res;
		res.sprintf( "%0.2f", d_agent->getProject()->getWidthFactor() );
		res	= QInputDialog::getText( "Set Orthogonal Width Factor",
			"Please	enter a positive factor:", QLineEdit::Normal, res, &ok, getQt() );
		if( !ok )
			return;
		g = res.toFloat( &ok );
	}else
		g = a.getParam( 0 ).getFloat();
	if( g <= 0.0 )
	{
		Root::ReportToUser::alert( this, "Set Width Factor", "Invalid Factor Value" );
		return;
	}
	d_agent->getProject()->setWidthFactor( g );
}

void FourDScope::handleUse4DSpec(Action & a)
{
	ACTION_CHECKED_IF( a, d_list && d_agent, d_use4D );
	d_use4D = !d_use4D;
	if( d_use4D )
		d_list->setSpec( d_agent->getSpec4D() );
	else
		d_list->setSpec( d_agent->getSpec2D() );
}

void FourDScope::handleGoto4D(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_goto4D );
	d_goto4D = !d_goto4D;
}

void FourDScope::handleSaveColors(Action & a)
{
    ACTION_ENABLED_IF( a, true );
	Repository::SlotColors clr( d_agent->getPlane().d_ol.size() );
	for( int i = 0; i < clr.size(); i++ )
	{
		clr[ i ].d_pos = d_agent->getPlane().d_ol[i].d_view->getPosColor();
		clr[ i ].d_neg = d_agent->getPlane().d_ol[i].d_view->getNegColor();
	}
	d_agent->getProject()->getRepository()->setScreenClr( clr );
}

void FourDScope::handleToMonoScope2D(Action & a)
{
	Spectrum* spec = d_agent->getSpec();
    ACTION_ENABLED_IF( a, spec );

	try
	{
		Root::Ref<PointSet> ps = createPlanePeaks();
		Root::Ref<PeakList> pl = new PeakList( spec );
		pl->append( ps ); // pl ist nun eine identische Kopie von ps.
		pl->setName( "Export from HomoScope2" );
		createMonoScope( getParent(), spec, d_agent->getProject(), pl, Rotation() );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Error Exporting Peaklist", e.what() );
	}catch( ... )
	{
		Root::ReportToUser::alert( this, "Error Exporting Peaklist", "Unknown Exception" );
	}
}

