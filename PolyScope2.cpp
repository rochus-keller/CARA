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

#include "PolyScope2.h"
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

#include <PolyScopeAgent2.h>
#include <Dlg.h>
#include <StripListGadget.h>

using namespace Spec;

//////////////////////////////////////////////////////////////////////
// Entkopplung; nutzbar ohne include

void createPolyScope2(Root::Agent* a, Spec::Spectrum* s, Spec::Project* p)
{ 
	new PolyScope2( a, s, p );
}

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr PolyScope2::ShowList = "ShowList";
Root::Action::CmdStr PolyScope2::ShowAlignment = "ShowAlignment";
Root::Action::CmdStr PolyScope2::UnlinkSucc = "UnlinkSucc";
Root::Action::CmdStr PolyScope2::UnlinkPred = "UnlinkPred";
Root::Action::CmdStr PolyScope2::AssignStrip = "AssignStrip";
Root::Action::CmdStr PolyScope2::UnassignStrip = "UnassignStrip";
Root::Action::CmdStr PolyScope2::SetCandidates = "SetCandidates";
Root::Action::CmdStr PolyScope2::SetTolerance = "SetTolerance";
Root::Action::CmdStr PolyScope2::ExportPeaklist = "ExportPeaklist";
Root::Action::CmdStr PolyScope2::SetTolHori = "SetTolHori";
Root::Action::CmdStr PolyScope2::SetTolVerti = "SetTolVerti";
Root::Action::CmdStr PolyScope2::SetTolStrip = "SetTolStrip";
Root::Action::CmdStr PolyScope2::ExportStripPeaks = "ExportStripPeaks";
Root::Action::CmdStr PolyScope2::ToMonoScope = "ToMonoScope";
Root::Action::CmdStr PolyScope2::ToMonoScope2D = "ToMonoScope2D";
Root::Action::CmdStr PolyScope2::SyncHori = "SyncHori";
Root::Action::CmdStr PolyScope2::SyncVerti = "SyncVerti";
Root::Action::CmdStr PolyScope2::Goto = "Goto";
Root::Action::CmdStr PolyScope2::GotoResidue = "GotoResidue";
Root::Action::CmdStr PolyScope2::SetWidthFactor = "SetWidthFactor";
Root::Action::CmdStr PolyScope2::Use3DSpec = "Use3DSpec";
Root::Action::CmdStr PolyScope2::Goto3D = "Goto3D";
Root::Action::CmdStr PolyScope2::SaveColors = "SaveColors";

ACTION_SLOTS_BEGIN( PolyScope2 )
    { PolyScope2::SaveColors, &PolyScope2::handleSaveColors },
    { PolyScope2::Goto3D, &PolyScope2::handleGoto3D },
    { PolyScope2::Use3DSpec, &PolyScope2::handleUse3DSpec },
    { PolyScope2::SetWidthFactor, &PolyScope2::handleSetWidthFactor },
    { PolyScope2::GotoResidue, &PolyScope2::handleGotoResidue },
    { PolyScope2::SyncHori, &PolyScope2::handleSyncHori },
    { PolyScope2::SyncVerti, &PolyScope2::handleSyncVerti },
    { PolyScope2::Goto, &PolyScope2::handleGoto },
    { PolyScope2::ToMonoScope, &PolyScope2::handleToMonoScope },
    { PolyScope2::ToMonoScope2D, &PolyScope2::handleToMonoScope2D },
    { PolyScope2::SetTolStrip, &PolyScope2::handleSetTolStrip },
    { PolyScope2::ExportStripPeaks, &PolyScope2::handleExportStripPeaks },
    { PolyScope2::SetTolHori, &PolyScope2::handleSetTolHori },
    { PolyScope2::SetTolVerti, &PolyScope2::handleSetTolVerti },
    { PolyScope2::ExportPeaklist, &PolyScope2::handleExportPeaklist },
    { PolyScope2::SetTolerance, &PolyScope2::handleSetTolerance },
    { PolyScope2::ShowList, &PolyScope2::handleShowList },
ACTION_SLOTS_END( PolyScope2 )

//////////////////////////////////////////////////////////////////////

static void buildCommands( CommandLine* cl )
{
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::AdjustIntensity, "CW", "Adjust Intensity", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::PickPlPeak, "PP", "Pick Peak", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::MovePlPeak, "MP", "Move Peak", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::MovePlAlias, "MA", "Move Peak Alias", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::DeletePlPeaks, "DP", "Delete Peaks", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::LabelPlPeak, "LP", "Label Peak", true ) );

	cl->registerCommand( new ActionCommand( PolyScopeAgent2::DeleteAliasPeak, "UA", "Un-Alias Spins", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::GotoPoint, "GT", "Goto", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::PickSystem, "PN", "Pick New System", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::SetLinkParams, "PA", 
		"Set Link Params", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::SetLinkParams3D, "ZPA", 
		"Set Link Params", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::SetSystemType, "SY", "Set System Type", true ) );
	cl->registerCommand( new ActionCommand( PolyScope2::SyncHori, "SH", "Sync. Hori. Spin", true ) );
	cl->registerCommand( new ActionCommand( PolyScope2::SyncVerti, "SV", "Sync. Verti. Spin", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::RemoveRulers, "RS", "Remove Selected Rulers", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::RemoveAllRulers, "RA", "Remove All Rulers", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::AddRulerVerti, "RH", "Add Horizontal Ruler", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::AddRulerHori, "RV", "Add Vertical Ruler", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::HoldReference, "HR", "Hold Reference", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::ForwardPlane, "FP", "Forward Plane", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::BackwardPlane, "BP", "Backward Plane", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::ShowFolded, "SF", "Show Folded", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::AutoContour2, "ZAC", "Auto Contour Level", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::AutoContour, "AC", "Auto Contour Level", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::Show3dPlane, "3D", "Show 3D Plane", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::FitWindow3D, "ZWW", "Fit To Window", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::Backward, "BB", "Backward", true ) );
	cl->registerCommand( new ActionCommand( PolyScope2::ShowList, "SL", "Show List", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::CursorSync, "GC", "Sync To Global Cursor", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::FitWindowX, "WX", "Fit X To Window", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::FitWindowY, "WY", "Fit Y To Window", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::FitWindow, "WW", "Fit To Window", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::AutoCenter, "CC", "Center To Peak", true ) );
	cl->registerCommand( new ActionCommand( Root::Action::EditUndo, "ZZ", "Undo", true ) );
	cl->registerCommand( new ActionCommand( Root::Action::EditRedo, "YY", "Redo", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::ShowContour, "SC", "Show Contour", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::ShowIntensity, "SI", "Show Intensity", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::ShowLowRes, 
		"LR", "Low Resolution", true ) ); // TODO: Argument
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::ContourParams, 
		"CP", "Contour Params", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::ContourParams2, 
		"ZCP", "Contour Params", true ) );

	ActionCommand* cmd = new ActionCommand( 
		PolyScope2::SetTolerance, "ST", "Set Tolerance", false );
	cmd->registerParameter( Any::CStr, true, "Atom Type" ); 
	cmd->registerParameter( Any::Float, true, "Tolerance (ppm)" ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::LinkSystems, "LK", "Link Systems", false );
	cmd->registerParameter( Any::ULong, false, "Predecessor" ); // 1..n Pred, Succ oder Nil
	cmd->registerParameter( Any::ULong, false, "Successor" ); // 1..n Succ oder Nil
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::ViewLabels, 
		"LL", "Label Format", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::ViewPlLabels, 
		"LF", "Label Format", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::ViewLabels3D, 
		"ZLL", "Label Format", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cl->registerCommand( new ActionCommand( PolyScopeAgent2::PickSystem, 
		"PY", "Pick System", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::ProposePeak, 
		"PR", "Propose Spins", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::MovePeak, 
		"MS", "Move Spins", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::MovePeakAlias, 
		"AY", "Move System Alias", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::PickHori, 
		"EH", "Extend Horizontally", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::ProposeHori, 
		"PH", "Propose Horizontally", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::PickVerti, 
		"EV", "Extend Vertically", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::ProposeVerti, 
		"PV", "Propose Vertically", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::LabelPeak, 
		"LS", "Label Spins", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::DeletePeak, 
		"DS", "Delete Spins", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::DeleteLinks, 
		"DL", "Delete Spin Links", true ) );

	cl->registerCommand( new ActionCommand( PolyScopeAgent2::PickSpin3D, 
		"PI", "Pick Spin", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::ProposeSpin, 
		"RI", "Propose Spin", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::MoveSpin3D, 
		"MI", "Move Spin", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::MoveSpinAlias3D, 
		"AI", "Move Spin Alias", true ) );

	cl->registerCommand( new ActionCommand( PolyScopeAgent2::ShowWithOff2, 
		"RP", "Resolve Projected Spins", true ) );

	cmd = new ActionCommand( PolyScopeAgent2::ForceLabelSpin3D, 
		"LI", "Force Spin Label", false );
	cmd->registerParameter( Any::CStr, false ); // Label oder leer
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::GotoSystem, 
		"GY", "Goto System", false );
	cmd->registerParameter( Any::CStr, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::GotoPeak, 
		"GS", "Goto Spins", false );
	cmd->registerParameter( Any::CStr, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::GotoPlPeak, 
		"GP", "Goto Peak", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::OverlayCount, 
		"OC", "Overlay Count", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder Count
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::ActiveOverlay, 
		"AO", "Active Overlay", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::OverlaySpec, 
		"YS", "Overlay Spectrum", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cl->registerCommand( new ActionCommand( PolyScopeAgent2::SetPosColor, "PC", "Set Positive Color", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::SetNegColor, "NC", "Set Negative Color", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::ComposeLayers, "CL", "Compose Layers", true ) );

	cmd = new ActionCommand( PolyScopeAgent2::AddLayer, 
		"AL", "Add Overlay Layer", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::CntThreshold, 
		"CT", "Contour Threshold", false );
	cmd->registerParameter( Any::Float, true ); 
	cmd->registerParameter( Any::CStr, false ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::CntFactor, 
		"CF", "Contour Factor", false );
	cmd->registerParameter( Any::Float, true ); 
	cmd->registerParameter( Any::CStr, false ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::CntOption, 
		"CO", "Contour Options", false );
	cmd->registerParameter( Any::CStr, false ); // Leer oder + oder -
	cmd->registerParameter( Any::CStr, false ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScope2::GotoResidue, 
		"GR", "Goto Residue", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cl->registerCommand( new ActionCommand( PolyScopeAgent2::NextSpec2D, 
		"NS", "Next Spectrum", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::PrevSpec2D, 
		"PS", "Prev. Spectrum", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::NextSpec3D, 
		"NT", "Next Spectrum", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::PrevSpec3D, 
		"PT", "Prev. Spectrum", true ) );

	cmd = new ActionCommand( PolyScopeAgent2::LabelHori, 
		"LH", "Label Horizontal Spin", false );
	cmd->registerParameter( Any::CStr, false ); // Label oder leer
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::LabelVerti, 
		"LV", "Label Vertical Spin", false );
	cmd->registerParameter( Any::CStr, false ); // Label oder leer
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::PickLabel3D, 
		"PL", "Pick Label", false );
	cmd->registerParameter( Any::CStr, false ); // Label oder leer
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::LabelSpin3D, 
		"LZ", "Label Strip Spin", false );
	cmd->registerParameter( Any::CStr, false ); // Label oder leer
	cl->registerCommand( cmd );

	cmd = new ActionCommand( Root::Action::ExecuteLine, "LUA", "Lua", false );
	cmd->registerParameter( Any::Memo, true ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::AutoGain, 
		"AG", "Set Auto Contour Gain", false );
	cmd->registerParameter( Any::Float, true ); 
	cmd->registerParameter( Any::CStr, false ); // Label oder leer
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::AutoGain3D, 
		"ZAG", "Set Auto Contour Gain", false );
	cmd->registerParameter( Any::Float, true ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScope2::SetWidthFactor, 
		"WF", "Set Strip Width Factor", false );
	cmd->registerParameter( Any::Float, false ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::SetDepth, 
		"SD", "Set Peak Depth", false );
	cmd->registerParameter( Any::Float, false ); 
	cl->registerCommand( cmd );

}

//////////////////////////////////////////////////////////////////////

static QColor g_frameClr = Qt::lightGray;


PolyScope2::PolyScope2(Root::Agent * a, Spectrum* spec, Project* pro ):
	Lexi::MainWindow( a, true, true ), d_showList( false ), d_ovCtrl( 0 ), 
	d_use3D( true ), d_goto3D( false ), d_agent(0)
{
	d_agent = new PolyScopeAgent2( this, spec, pro );
	
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

PolyScope2::PolyScope2(Root::Agent * a ):
	Lexi::MainWindow( a, true, true ), d_showList( false ), d_ovCtrl( 0 ), 
	d_use3D( true ), d_goto3D( false ), d_agent( 0 ), d_focus( 0 ), d_widget( 0 )
{

}

PolyScope2::~PolyScope2()
{
	d_widget->setBody( 0 );
}

void PolyScope2::updateCaption()
{
	QString str; // TODO
	str.sprintf( "PolyScope - %s", d_agent->getSpec()->getName() );
	getQt()->setCaption( str );
}

void PolyScope2::buildMenus()
{
	Gui::Menu* menuFile = new Gui::Menu( menuBar() );
    Gui::Menu::item( menuFile, this, Root::Action::FileSave, Qt::CTRL+Qt::Key_S );
	Gui::Menu::item( menuFile, d_agent, "Print Preview...", 
		PolyScopeAgent2::CreateReport, false, Qt::CTRL+Qt::Key_P );
	Gui::Menu* sub = new Gui::Menu( menuFile );
	Gui::Menu::item( sub, this, "&Anchor Peaklist...", ExportPeaklist, false );
	Gui::Menu::item( sub, this, "&Strip Peaklist...", ExportStripPeaks, false );
	Gui::Menu::item( sub, this, "Strip peaks to &MonoScope...", ToMonoScope, false );
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
		PolyScopeAgent2::Backward, false, Qt::CTRL+Qt::Key_B );
	Gui::Menu::item( menuView, d_agent, "Forward", 
		PolyScopeAgent2::Forward, false );

	Gui::Menu::item( menuView, d_agent, "Fit Window", 
		PolyScopeAgent2::FitWindow, false, Qt::CTRL+Qt::Key_Home );
	Gui::Menu::item( menuView, d_agent, "Goto System...", 
		PolyScopeAgent2::GotoSystem, false, Qt::CTRL+Qt::Key_G );
	Gui::Menu::item( menuView, d_agent, "Goto Point...", 
		PolyScopeAgent2::GotoPoint, false );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Set Resolution...", 
		PolyScopeAgent2::SetResolution, false );
	Gui::Menu::item( menuView, d_agent, "Show Low Resolution", 
		PolyScopeAgent2::ShowLowRes, true );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Do Pathway Simulation", 
		PolyScopeAgent2::ForceCross, true );
	Gui::Menu::item( menuView, d_agent, "Show Ghost Peaks", 
		PolyScopeAgent2::ShowGhosts, true );
	Gui::Menu::item( menuView, d_agent, "Show Ghost Labels", 
		PolyScopeAgent2::GhostLabels, true );
	Gui::Menu::item( menuView, d_agent, "Show Hidden Links", 
		PolyScopeAgent2::ShowAllPeaks, true );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Hide 3D Slices", 
		PolyScopeAgent2::AutoHide, true );
	Gui::Menu::item( menuView, d_agent, "Show Folded", 
		PolyScopeAgent2::ShowFolded, true );
	Gui::Menu::item( menuView, d_agent, "Sync to Global Zoom", 
		PolyScopeAgent2::RangeSync, true );
	Gui::Menu::item( menuView, d_agent, "Sync to Global Cursor", 
		PolyScopeAgent2::CursorSync, true );
	Gui::Menu::item( menuView, d_agent, "Sync to Depth", 
		PolyScopeAgent2::SyncDepth, true );
	Gui::Menu::item( menuView, d_agent, "Center to Peak", 
		PolyScopeAgent2::AutoCenter, true );
	Gui::Menu::item( menuView, d_agent, "Show List", ShowList, true );

	menuBar()->insertItem( "&View", menuView );

	Gui::Menu* menuPicking = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuPicking, d_agent, "&Pick New System", 
		PolyScopeAgent2::PickSystem, false );
	Gui::Menu::item( menuPicking, d_agent, "Propose System...", 
		PolyScopeAgent2::ProposePeak, false );
	Gui::Menu::item( menuPicking, d_agent, "Extend Horizontally", 
		PolyScopeAgent2::PickHori, false );
	Gui::Menu::item( menuPicking, d_agent, "Propose Horizontally...", 
		PolyScopeAgent2::ProposeHori, false );
	Gui::Menu::item( menuPicking, d_agent, "Extend Vertically", 
		PolyScopeAgent2::PickVerti, false );
	Gui::Menu::item( menuPicking, d_agent, "Propose Vertically...", 
		PolyScopeAgent2::ProposeVerti, false );
	Gui::Menu::item( menuPicking, d_agent, "&Move Spins", 
		PolyScopeAgent2::MovePeak, false );
	Gui::Menu::item( menuPicking, d_agent, "Move Spin &Aliases", 
		PolyScopeAgent2::MovePeakAlias, false );
	Gui::Menu::item( menuPicking, d_agent, "&Label Spins...", 
		PolyScopeAgent2::LabelPeak, false );
	Gui::Menu::item( menuPicking, d_agent, "Hide/Show Link", 
		PolyScopeAgent2::HidePeak, false );
	Gui::Menu::item( menuPicking, d_agent, "Set Link Params...", 
		PolyScopeAgent2::SetLinkParams, false );
	menuPicking->insertSeparator();
	Gui::Menu::item( menuPicking, this, "Set Verti. Tolerance...", 
		PolyScope2::SetTolVerti, false );
	Gui::Menu::item( menuPicking, this, "Set Hori. Tolerance...", 
		PolyScope2::SetTolHori, false );
	menuPicking->insertSeparator();
	Gui::Menu::item( menuPicking, d_agent, "Un-Alias Peaks", 
		PolyScopeAgent2::DeleteAliasPeak, false );
	Gui::Menu::item( menuPicking, d_agent, "Delete Peaks", 
		PolyScopeAgent2::DeletePeak, false );
	Gui::Menu::item( menuPicking, d_agent, "Delete Spin Links", 
		PolyScopeAgent2::DeleteLinks, false );
	Gui::Menu::item( menuPicking, d_agent, "Delete Horizontal Spin", 
		PolyScopeAgent2::DeleteSpinX, false );
	Gui::Menu::item( menuPicking, d_agent, "Delete Vertical Spin", 
		PolyScopeAgent2::DeleteSpinY, false );

	Gui::Menu* menuAssig = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuAssig, d_agent, "&Link Systems...", 
		PolyScopeAgent2::LinkSystems, false );
	Gui::Menu::item( menuAssig, d_agent, "Unlink &Predecessor", 
		PolyScopeAgent2::UnlinkPred, false );
	Gui::Menu::item( menuAssig, d_agent, "Unlink &Successor", 
		PolyScopeAgent2::UnlinkSucc, false );
	menuAssig->insertSeparator();
	Gui::Menu::item( menuAssig, d_agent, "&Assign System...", 
		PolyScopeAgent2::Assign, false );
	Gui::Menu::item( menuAssig, d_agent, "&Unassign System", 
		PolyScopeAgent2::Unassign, false );
	Gui::Menu::item( menuAssig, d_agent, "Set System &Type...", 
		PolyScopeAgent2::SetSystemType, false );
	Gui::Menu::item( menuAssig, d_agent, "Set Assig. Candidates...", 
		PolyScopeAgent2::SetCandidates, false );
	Gui::Menu::item( menuAssig, d_agent, "Show Ali&gnment...", 
		PolyScopeAgent2::ShowAlignment, false );

	Gui::Menu* menuRuler = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuRuler, d_agent, "Add &Horizontal Ruler", 
		PolyScopeAgent2::AddRulerVerti, false );
	Gui::Menu::item( menuRuler, d_agent, "Add &Vertical Ruler", 
		PolyScopeAgent2::AddRulerHori, false );
	Gui::Menu::item( menuRuler, d_agent, "Remove Selected Rulers", 
		PolyScopeAgent2::RemoveRulers, false );
	Gui::Menu::item( menuRuler, d_agent, "Remove All Rulers", 
		PolyScopeAgent2::RemoveAllRulers, false );
	Gui::Menu::item( menuRuler, d_agent, "Set Rulers to Reference", 
		PolyScopeAgent2::AutoRuler, true );

	Gui::Menu* menuAtts = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuAtts, d_agent, "Horizontal Spin...", 
		PolyScopeAgent2::EditAttsSpinH, false );
	Gui::Menu::item( menuAtts, d_agent, "Vertical Spin...", 
		PolyScopeAgent2::EditAttsSpinV, false );
	Gui::Menu::item( menuAtts, d_agent, "Horizontal System...", 
		PolyScopeAgent2::EditAttsSysH, false );
	Gui::Menu::item( menuAtts, d_agent, "Vertical System...", 
		PolyScopeAgent2::EditAttsSysV, false );
	Gui::Menu::item( menuAtts, d_agent, "Spin Link...", 
		PolyScopeAgent2::EditAttsLink, false );

	Gui::Menu* menuOverlay = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuOverlay, d_agent, "Set Layer &Count...", 
		PolyScopeAgent2::OverlayCount, false );
	Gui::Menu::item( menuOverlay, d_agent, "A&dd Overlay Layer...", 
		PolyScopeAgent2::AddLayer, false );
	Gui::Menu::item( menuOverlay, d_agent, "Compose &Layers...", 
		PolyScopeAgent2::ComposeLayers, false );
	Gui::Menu::item( menuOverlay, d_agent, "Set &Active Layer...", 
		PolyScopeAgent2::ActiveOverlay, false );
	Gui::Menu::item( menuOverlay, d_agent, "Select &Spectrum...", 
		PolyScopeAgent2::OverlaySpec, false );
	Gui::Menu::item( menuOverlay, d_agent, "Set &Positive Color...", 
		PolyScopeAgent2::SetPosColor, false );
	Gui::Menu::item( menuOverlay, d_agent, "Set &Negative Color...", 
		PolyScopeAgent2::SetNegColor, false );
	Gui::Menu::item( menuOverlay, d_agent, "&Make Colors Persistent", 
		PolyScope2::SaveColors, false );

	Gui::Menu* menuPlane = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuPlane, d_agent, "&Calibrate", 
		PolyScopeAgent2::SpecCalibrate, false );
	menuPlane->insertItem( "Select Spectrum", d_agent->getPopSpec2D() );
	menuPlane->insertItem( "&Overlay", menuOverlay );
	menuPlane->insertSeparator();
	Gui::Menu::item( menuPlane, d_agent, "Show Contour", 
		PolyScopeAgent2::ShowContour, true );
	Gui::Menu::item( menuPlane, d_agent, "Show Intensity", 
		PolyScopeAgent2::ShowIntensity, true );
	Gui::Menu::item( menuPlane, d_agent, "Auto Contour Level", 
		PolyScopeAgent2::AutoContour, true );
	Gui::Menu::item( menuPlane, d_agent, "Set Contour Parameters...", 
		PolyScopeAgent2::ContourParams, false );
	menuPlane->insertSeparator();
	Gui::Menu::item( menuPlane, d_agent, "Forward Plane", 
		PolyScopeAgent2::ForwardPlane, false, Qt::SHIFT+Qt::Key_Next );
	Gui::Menu::item( menuPlane, d_agent, "Backward Plane", 
		PolyScopeAgent2::BackwardPlane, false, Qt::SHIFT+Qt::Key_Prior );
	menuPlane->insertSeparator();
	menuPlane->insertItem( "&Picking", menuPicking );
	menuPlane->insertItem( "&Assignment", menuAssig );
	menuPlane->insertItem( "&Ruler", menuRuler );
	menuPlane->insertItem( "&Edit Attributes", menuAtts );
	menuPlane->insertSeparator();
	Gui::Menu::item( menuPlane, d_agent, "Hold Reference", 
		PolyScopeAgent2::HoldReference, false );
	Gui::Menu::item( menuPlane, d_agent, "Auto Hold", 
		PolyScopeAgent2::AutoHold, true );
	menuPlane->insertSeparator();
	Gui::Menu::item( menuPlane, d_agent, "Show 3D Plane", 
		PolyScopeAgent2::Show3dPlane, true, Qt::CTRL+Qt::Key_3 );
	Gui::Menu::item( menuPlane, d_agent, "Show Spin Links", 
		PolyScopeAgent2::ShowLinks, true );
	Gui::Menu::item( menuPlane, d_agent, "Show Infered Peaks", 
		PolyScopeAgent2::ShowInfered, true );
	Gui::Menu::item( menuPlane, d_agent, "Show Unlabeled Peaks", 
		PolyScopeAgent2::ShowUnlabeled, true );
	Gui::Menu::item( menuPlane, d_agent, "Show Peaks with unknown Labels", 
		PolyScopeAgent2::ShowUnknown, true );
	Gui::Menu::item( menuPlane, d_agent, "Resolve Projected Spins", 
		PolyScopeAgent2::ShowWithOff2, true );
	Gui::Menu::item( menuPlane, d_agent, "Use Link Color Codes", 
		PolyScopeAgent2::UseLinkColors, true );
	sub = new Gui::Menu( menuBar() );
	menuPlane->insertItem( "Show Labels", sub );
	short i;
	for( i = SpinPointView::None; i < SpinPointView::End; i++ )
	{
		Gui::Menu::item( sub, d_agent, SpinPointView::menuText[ i ], 
			PolyScopeAgent2::ViewLabels, true )->addParam( i );
	}
	menuBar()->insertItem( "&Plane", menuPlane );

	Gui::Menu* menuStrip = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuStrip, d_agent, "Calibrate Strip", 
		PolyScopeAgent2::StripCalibrate, false );
	Gui::Menu::item( menuStrip, d_agent, "Set Tolerance...", SetTolStrip, false );
	menuStrip->insertSeparator();
	Gui::Menu::item( menuStrip, d_agent, "Auto Contour Level", 
		PolyScopeAgent2::AutoContour2, true );
	Gui::Menu::item( menuStrip, d_agent, "Set Contour Parameters...", 
		PolyScopeAgent2::ContourParams2, false );

	menuStrip->insertItem( "Select Spectrum", d_agent->getPopSpec3D() );
	menuStrip->insertSeparator();
	Gui::Menu::item( menuStrip, d_agent, "&Pick Spin", 
		PolyScopeAgent2::PickSpin3D, false );
	Gui::Menu::item( menuStrip, d_agent, "Pick Label...", 
		PolyScopeAgent2::PickLabel3D, false );
	Gui::Menu::item( menuStrip, d_agent, "Propose &Spin...", 
		PolyScopeAgent2::ProposeSpin, false );
	Gui::Menu::item( menuStrip, d_agent, "&Move Spin", 
		PolyScopeAgent2::MoveSpin3D, false );
	Gui::Menu::item( menuStrip, d_agent, "&Move Spin Alias", 
		PolyScopeAgent2::MoveSpinAlias3D, false );

	Gui::Menu::item( menuStrip, d_agent, "&Label Spin...", 
		PolyScopeAgent2::LabelSpin3D, false );
	Gui::Menu::item( menuStrip, d_agent, "Hide/Show Link", 
		PolyScopeAgent2::HidePeak2, false );
	Gui::Menu::item( menuStrip, d_agent, "Set Link Params...", 
		PolyScopeAgent2::SetLinkParams3D, false );

	Gui::Menu::item( menuStrip, d_agent, "&Force Spin Label", 
		PolyScopeAgent2::ForceLabelSpin3D, false );
	Gui::Menu::item( menuStrip, d_agent, "&Delete Spins", 
		PolyScopeAgent2::DeleteSpins3D, false );
	Gui::Menu::item( menuStrip, d_agent, "Delete Spin &Links", 
		PolyScopeAgent2::DeleteLinks3D, false );
	menuAtts = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuAtts, d_agent, "Spin...", 
		PolyScopeAgent2::EditAttsSpin3D, false );
	Gui::Menu::item( menuAtts, d_agent, "System...", 
		PolyScopeAgent2::EditAttsSys3D, false );
	Gui::Menu::item( menuAtts, d_agent, "Spin Link...", 
		PolyScopeAgent2::EditAttsLink3D, false );
	menuStrip->insertItem( "&Edit Attributes", menuAtts );
	menuStrip->insertSeparator();
	Gui::Menu::item( menuStrip, d_agent, "Show Spin Links", 
		PolyScopeAgent2::ShowLinks2, true );
	Gui::Menu::item( menuStrip, d_agent, "Show Infered Peaks", 
		PolyScopeAgent2::ShowInfered2, true );
	Gui::Menu::item( menuStrip, d_agent, "Show Unlabeled Peaks", 
		PolyScopeAgent2::ShowUnlabeled2, true );
	Gui::Menu::item( menuStrip, d_agent, "Show Peaks with unknown Labels", 
		PolyScopeAgent2::ShowUnknown2, true );
	Gui::Menu::item( menuStrip, d_agent, "Resolve Projected Spins", 
		PolyScopeAgent2::ShowWithOff, true );
	Gui::Menu::item( menuStrip, d_agent, "Use Link Color Codes", 
		PolyScopeAgent2::UseLinkColors3D, true );
	Gui::Menu::item( menuStrip, d_agent, "Fit Window", 
		PolyScopeAgent2::FitWindow3D, false, Qt::Key_Home );
	sub = new Gui::Menu( menuBar() );
	menuStrip->insertItem( "Show Labels", sub );
	for( i = SpinPointView::None; i < SpinPointView::End; i++ )
	{
		Gui::Menu::item( sub, d_agent, SpinPointView::menuText[ i ], 
			PolyScopeAgent2::ViewLabels3D, true )->addParam( i );
	}
	menuBar()->insertItem( "&Strips", menuStrip );

	Gui::Menu* menuPeaks = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuPeaks, d_agent, "&New Peaklist", 
		PolyScopeAgent2::NewPeakList, false );
	Gui::Menu::item( menuPeaks, d_agent, "&Open Peaklist...", 
		PolyScopeAgent2::OpenPeakList, false );
	Gui::Menu::item( menuPeaks, d_agent, "&Add to Repository...", 
		PolyScopeAgent2::SavePeakList, false );
	Gui::Menu::item( menuPeaks, d_agent, "Map to Spectrum...", 
		PolyScopeAgent2::MapPeakList, false );
	Gui::Menu::item( menuPeaks, d_agent, "Select Color...", 
		PolyScopeAgent2::SetPlColor, false );
	sub = new Gui::Menu( menuBar() );
	menuPeaks->insertItem( "Show Labels", sub );
	for( i = PeakPlaneView::NONE; i < PeakPlaneView::END; i++ )
	{
		Gui::Menu::item( sub, d_agent, PeakPlaneView::menuText[ i ], 
			PolyScopeAgent2::ViewPlLabels, true )->addParam( i );
	}
	menuPeaks->insertSeparator();
	Gui::Menu::item( menuPeaks, d_agent, "&Pick Peak", 
		PolyScopeAgent2::PickPlPeak, false );
	Gui::Menu::item( menuPeaks, d_agent, "&Move Peak", 
		PolyScopeAgent2::MovePlPeak, false );
	Gui::Menu::item( menuPeaks, d_agent, "&Move Peak Alias", 
		PolyScopeAgent2::MovePlAlias, false );
	Gui::Menu::item( menuPeaks, d_agent, "&Label Peak...", 
		PolyScopeAgent2::LabelPlPeak, false );
	Gui::Menu::item( menuPeaks, d_agent, "&Delete Peaks", 
		PolyScopeAgent2::DeletePlPeaks, false );
	Gui::Menu::item( menuPeaks, d_agent, "&Edit Attributes...", 
		PolyScopeAgent2::EditPlPeakAtts, false );
	menuBar()->insertItem( "&Peaks", menuPeaks );

	Gui::Menu* menuHelp = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuHelp, this, Root::Action::HelpAbout );
	menuBar()->insertItem( "&?", menuHelp );

}

void PolyScope2::buildViews( bool reuse )
{
	d_focus->setBody( 0 );

	Splitter* inner = new Splitter( 2, 2 );
	inner->setBarWidth( 80 );

	d_focus->clear();
	d_focus->setCircle( !d_showList );

	Splitter* outer = new Splitter( 1, (d_showList)?3:2 );
	outer->setBarWidth( 80 );
	outer->setPane( inner, 0, 0 );

	inner->setPane( d_agent->getPlane().d_viewer, 0, 1 );

	inner->setPane( d_agent->getSlice( DimX ).d_viewer, 1, 1 );

	inner->setPane( d_agent->getSlice( DimY ).d_viewer, 0, 0 );

	Glyph* box = LayoutKit::hbox();
	outer->setPane( box, 0, 1 );

	box->append( d_agent->getSlice( DimZ ).d_viewer );
	box->append( new Rule( DimensionY, g_frameClr, 40 ) );
	box->append( d_agent->getStrip( DimX ).d_viewer );
	box->append( new Rule( DimensionY, g_frameClr, 40 ) );
	box->append( d_agent->getStrip( DimY ).d_viewer );

	if( d_showList )
	{
		d_list = new StripListGadget( d_widget->getViewport(), 
			d_agent->getProject(), this, 0, true );
		outer->setPane( d_list, 0, 2 );
		Gui::Menu* menu = new Gui::Menu( d_list->getListView()->getImp(), true );
		Gui::Menu::item( menu, this, "Goto...", Goto, false );
		Gui::Menu::item( menu, this, "Use 3D Navigation", Goto3D, true );
		Gui::Menu::item( menu, this, "Show 3D Values", Use3DSpec, true );
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
		if( d_use3D )
			d_list->setSpec( d_agent->getSpec3D() );
		else
			d_list->setSpec( d_agent->getSpec2D() );
	}else
		d_list = 0;

	if( reuse && d_ovCtrl )
	{
		d_ovCtrl->setTarget( d_agent->getPlane().d_viewer->getViewArea() );
	}else if( d_agent->getSpec2D()->getDimCount() == 2 )
	{
		d_ov = new SpecViewer( new ViewAreaMdl( true, true, true, true ) );
		Root::Ref<SpecProjector> pro = new SpecProjector( d_agent->getSpec(), DimX, DimY );
		Root::Ref<SpecBufferMdl> mdl = new SpecBufferMdl( d_ov->getViewArea(), pro );
		d_ov->getViews()->append( new IntensityView( mdl ) );
		d_ovCtrl = new OverviewCtrl( mdl, d_agent->getPlane().d_viewer->getViewArea() );
		d_ov->getHandlers()->append( d_ovCtrl );
		d_ov->getHandlers()->append( new FocusCtrl( d_ov ) );
		// mdl->copy( d_agent->getPlane().d_ol[0].d_buf );
	}
	inner->setPane( d_ov, 1, 0 );

	d_focus->append( d_agent->getSlice( DimY ).d_viewer->getController() );
	d_focus->append( d_agent->getPlane().d_viewer->getController() );
	d_focus->append( d_agent->getSlice( DimX ).d_viewer->getController() );
	d_focus->append( d_agent->getSlice( DimZ ).d_viewer->getController() );
	d_focus->append( d_agent->getStrip( DimX ).d_viewer->getController() );
	d_focus->append( d_agent->getStrip( DimY ).d_viewer->getController() );

	d_focus->setBody( outer );


	if( d_showList )
	{
		inner->setRowPos( 1,  18 * d_widget->height() );
		inner->setColumnPos( 1,  2 * d_widget->width() );
		outer->setColumnPos( 1,  12 * d_widget->width() );
		outer->setColumnPos( 2,  17 * d_widget->width() );
	}else
	{
		inner->setRowPos( 1,  17 * d_widget->height() );
		inner->setColumnPos( 1,  3 * d_widget->width() );
		outer->setColumnPos( 1,  15 * d_widget->width() );
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

Root::Ref<PointSet> PolyScope2::createStripPeaks()
{
	Spectrum* spec = d_agent->getSpec3D();
	PpmPoint p( 0, 0, 0 );
	PointSet::Assig a;
	char buf[32];
	Root::Index id;
	Root::Ref<PointSet> ps = Factory::createEasyPeakList( spec );

	LinkFilterRotSpace* mdl = d_agent->getMdl3D();
	LinkFilterRotSpace::Iterator i, _end = mdl->end();
	LinkFilterRotSpace::Element e;
	SpinPointView::Label l = d_agent->getStrip( DimX ).d_tuples->getLabel();
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

bool PolyScope2::askToCloseWindow() const
{
	return d_agent->askToClosePeaklist();
}

void PolyScope2::handle(Root::Message& m)
{
	BEGIN_HANDLER();
	MESSAGE( StripListGadget::ActivateMsg, e, m )
	{
        Q_UNUSED(e)
		if( d_list )
		{
			d_agent->gotoTuple( d_list->getSelectedStrip(), d_list->getSelectedSpin(),
				d_list->getSelectedLink(), !d_goto3D );
			d_widget->setFocusGlyph( d_agent->getPlane().d_viewer->getController() );
		}
		m.consume();
	}MESSAGE( PolyScopeAgent2::SpecChanged, e, m )
	{
		if( d_list && d_agent )
		{
			if( d_use3D )
				d_list->setSpec( d_agent->getSpec3D() );
			else
				d_list->setSpec( d_agent->getSpec2D() );
		}
		if( d_ovCtrl && !e->d_threeD )
		{
			d_ovCtrl->getModel()->setSpectrum( 
				new SpecProjector( d_agent->getSpec(), DimX, DimY ) );
			d_ov->redraw();
		}
		m.consume();
	}MESSAGE( Root::Action, a, m )
	{
		if( !EXECUTE_ACTION( PolyScope2, *a ) )
			MainWindow::handle( m );
	}HANDLE_ELSE()
		MainWindow::handle( m );
	END_HANDLER();
}

void PolyScope2::handleShowList(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_showList );

	d_showList = !d_showList;
	Lexi::Viewport::pushHourglass();
	buildViews( true );
	Lexi::Viewport::popCursor();
}

void PolyScope2::handleSetTolerance(Action & a)
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

void PolyScope2::handleExportPeaklist(Action & a)
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

Root::Ref<PointSet> PolyScope2::createPlanePeaks()
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

void PolyScope2::handleSetTolHori(Action & a)
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

void PolyScope2::handleSetTolVerti(Action & a)
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

void PolyScope2::handleSetTolStrip(Action & a)
{
	ACTION_ENABLED_IF( a, d_agent->getSpec3D() );

	AtomType atom;
	atom = d_agent->getSpec3D()->getColor( DimZ );

	bool ok	= FALSE;
	QString res;
	res.sprintf( "%0.3f", d_agent->getProject()->getMatcher()->getTol( atom ) );
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

	d_agent->getProject()->getMatcher()->setTol( atom, w );
}

void PolyScope2::handleExportStripPeaks(Action & a)
{
	Spectrum* spec = d_agent->getSpec3D();
	ACTION_ENABLED_IF( a, spec );

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

void PolyScope2::handleToMonoScope(Action & a)
{
	Spectrum* spec = d_agent->getSpec3D();
	ACTION_ENABLED_IF( a, spec );

	try
	{
		Root::Ref<PointSet> ps = createStripPeaks();
		if( ps.isNull() )
			throw Root::Exception( "Cannot create Peaklist" );
		Root::Ref<PeakList> pl = new PeakList( spec );
		pl->append( ps ); // pl ist nun eine identische Kopie von ps.
		pl->setName( "Export from PolyScope2" );
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

void PolyScope2::handleGoto(Action & a)
{
	ACTION_ENABLED_IF( a, d_list );
	d_agent->gotoTuple( d_list->getSelectedStrip(), d_list->getSelectedSpin(),
		d_list->getSelectedLink(), !d_goto3D );
	d_widget->setFocusGlyph( d_agent->getPlane().d_viewer->getController() );
}

void PolyScope2::handleSyncHori(Action & a)
{
	Spin* spin = d_agent->getSel( true );
	ACTION_ENABLED_IF( a, d_list && spin );
	d_list->gotoSpin( spin );
}

void PolyScope2::handleSyncVerti(Action & a)
{
	Spin* spin = d_agent->getSel( false );
	ACTION_ENABLED_IF( a, d_list && spin );
	d_list->gotoSpin( spin );
}

void PolyScope2::handleGotoResidue(Action & a)
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
		PolyScopeAgent2::GotoSystem, false );
	action.addParam( Root::Int32(resi->getSystem()->getId()) );
	action.execute();
}

void PolyScope2::handleSetWidthFactor(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	float g;
	if( a.getParam( 0 ).isNull() )
	{
		bool ok	= FALSE;
		QString res;
		res.sprintf( "%0.2f", d_agent->getProject()->getWidthFactor() );
		res	= QInputDialog::getText( "Set Strip Width Factor", 
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

void PolyScope2::handleUse3DSpec(Action & a)
{
	ACTION_CHECKED_IF( a, d_list && d_agent, d_use3D );
	d_use3D = !d_use3D;
	if( d_use3D )
		d_list->setSpec( d_agent->getSpec3D() );
	else
		d_list->setSpec( d_agent->getSpec2D() );
}

void PolyScope2::handleGoto3D(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_goto3D );
	d_goto3D = !d_goto3D;
}

void PolyScope2::handleSaveColors(Action & a)
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

void PolyScope2::handleToMonoScope2D(Action & a)
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

