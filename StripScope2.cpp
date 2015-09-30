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

#include "StripScope2.h"
// Qt
#include <qinputdialog.h> 
#include <qmessagebox.h>
#include <qmenubar.h>
#include <QFileDialog>

#include <Root/ActionHandler.h>
#include <Root/Command.h>
#include <Root/Application.h>
#include <Root/UpstreamFilter.h>
using namespace Root;

#include <Lexi/LayoutKit.h>
#include <QColor>
#include <Lexi/CommandLine.h>
#include <Lexi/Splitter.h>
#include <Lexi/Redirector.h>
#include <Lexi/Background.h>
#include <Lexi/Bevel.h>
#include <Lexi/Shapes.h>
#include <Lexi/Interactor.h>
#include <Gui/Menu.h>
#include <Lexi/ContextMenu.h>
#include <Lexi/Printer.h>
#include <Dlg.h>
#include <SpecView/CandidateDlg.h>
#include <SingleAlignmentView.h>
#include <Spec/FragmentAssignment.h>
#include <Spec/PointSet.h>
#include <Spec/Factory.h>
#include <Spec/Repository.h>
#include <SpecView/DynValueEditor.h>

using namespace Lexi;
using namespace Spec;

static QColor g_frameClr = Qt::lightGray;

//////////////////////////////////////////////////////////////////////
// Entkopplung; nutzbar ohne include

void createStripScope2(Root::Agent* a, Spec::Project* p, Spec::Spectrum* s )
{ 
	new StripScope2( a, p, s );
}

//////////////////////////////////////////////////////////////////////

Action::CmdStr StripScope2::SetLaneCount = "SetLaneCount";
Action::CmdStr StripScope2::ShowRulers = "ShowRulers";
Action::CmdStr StripScope2::SelectRuler = "SelectRuler";
Action::CmdStr StripScope2::ShowSucc = "ShowSucc";
Action::CmdStr StripScope2::ShowPred = "ShowPred";
Action::CmdStr StripScope2::ShowAllSucc = "AllShowSucc";
Action::CmdStr StripScope2::ShowAllPred = "AllShowPred";
Action::CmdStr StripScope2::ShowAllBest = "ShowAllBest";
Action::CmdStr StripScope2::ShowFrag = "ShowFrag";
Action::CmdStr StripScope2::RunStripper = "RunStripper";
Action::CmdStr StripScope2::StrictStripMatch = "StrictStripMatch";
Action::CmdStr StripScope2::RunMapper = "RunMapper";
Action::CmdStr StripScope2::MapGeometric = "MapGeometric";
Action::CmdStr StripScope2::MapExclusive = "MapExclusive";
Action::CmdStr StripScope2::MapExpand = "MapExpand";
Action::CmdStr StripScope2::MinFragLen = "MinFragLen";
Action::CmdStr StripScope2::MapMatchCount = "MapMatchCount";
Action::CmdStr StripScope2::SetStripWidth = "SetStripWidth";
Action::CmdStr StripScope2::SetStripTol = "SetStripTol";
Action::CmdStr StripScope2::ExportAnchorPeaks = "ExportAnchorPeaks";
Action::CmdStr StripScope2::ExportStripPeaks = "ExportStripPeaks";
Action::CmdStr StripScope2::ShowSpinFits = "ShowSpinFits";
Action::CmdStr StripScope2::EditAtts = "EditAtts";
Root::Action::CmdStr StripScope2::Goto = "Goto";
Root::Action::CmdStr StripScope2::GotoResidue = "GotoResidue";
Action::CmdStr StripScope2::SetWidthFactor = "SetWidthFactor";
Action::CmdStr StripScope2::ShowSystem = "ShowSystem";
Action::CmdStr StripScope2::UnlabeledStripMatch = "UnlabeledStripMatch";

ACTION_SLOTS_BEGIN( StripScope2 )
    { StripScope2::UnlabeledStripMatch, &StripScope2::handleUnlabeledStripMatch },
    { StripScope2::ShowSystem, &StripScope2::handleShowSystem },
    { StripScopeAgent2::NextSpec, &StripScope2::handleNextSpec },
    { StripScopeAgent2::PrevSpec, &StripScope2::handlePrevSpec },
    { StripScopeAgent2::SelectSpec, &StripScope2::handleSelectSpec },
    { StripScopeAgent2::SetDepth, &StripScope2::handleSetDepth },
    { StripScope2::SetWidthFactor, &StripScope2::handleSetWidthFactor },
    { StripScope2::GotoResidue, &StripScope2::handleGotoResidue },
    { StripScope2::Goto, &StripScope2::handleGoto },
    { StripScope2::ShowSpinFits, &StripScope2::handleShowSpinFits },
    { StripScope2::ShowAllBest, &StripScope2::handleShowAllBest },
    { StripScope2::ExportAnchorPeaks, &StripScope2::handleExportAnchorPeaks },
    { StripScope2::ExportStripPeaks, &StripScope2::handleExportStripPeaks },
    { StripScope2::SetStripWidth, &StripScope2::handleSetStripWidth },
    { StripScope2::SetStripTol, &StripScope2::handleSetStripTol },
    { StripScope2::RunStripper, &StripScope2::handleRunStripper },
    { StripScope2::StrictStripMatch, &StripScope2::handleStrictStripMatch },
    { StripScope2::ShowAllSucc, &StripScope2::handleShowAllSucc },
    { StripScope2::ShowAllPred, &StripScope2::handleShowAllPred },
    { StripScope2::ShowFrag, &StripScope2::handleShowFrag },
    { StripScope2::ShowPred, &StripScope2::handleShowPred },
    { StripScope2::ShowSucc, &StripScope2::handleShowSucc },
    { StripScope2::SetLaneCount, &StripScope2::handleSetLaneCount },
    { StripScope2::ShowRulers, &StripScope2::handleShowRulers },
    { StripScope2::SelectRuler, &StripScope2::handleSelectRuler },
ACTION_SLOTS_END( StripScope2 )

//////////////////////////////////////////////////////////////////////

static void buildCommands( CommandLine* cl )
{
	cl->registerCommand( new ActionCommand( StripScopeAgent2::KeepSpec, "HS", "Hold Reference Spectrum", true ) );
	cl->registerCommand( new ActionCommand( StripScopeAgent2::SyncStrip, "SH", "Sync to List", true ) );
	cl->registerCommand( new ActionCommand( StripScopeAgent2::ShowFolded, "SF", "Show Folded", true ) );
	cl->registerCommand( new ActionCommand( StripScopeAgent2::AutoContour, "AC", "Auto Contour Level", true ) );
	cl->registerCommand( new ActionCommand( StripScopeAgent2::CursorSync, "GC", "Sync To Global Cursor", true ) );
	cl->registerCommand( new ActionCommand( StripScopeAgent2::FitWindow, "WY", "Fit To Window", true ) );
	cl->registerCommand( new ActionCommand( StripScopeAgent2::FitWindow, "WW", "Fit To Window", true ) );
	cl->registerCommand( new ActionCommand( StripScopeAgent2::AutoCenter, "CC", "Center To Peak", true ) );
	cl->registerCommand( new ActionCommand( Root::Action::EditUndo, "ZZ", "Undo", true ) );
	cl->registerCommand( new ActionCommand( Root::Action::EditRedo, "YY", "Redo", true ) );
	cl->registerCommand( new ActionCommand( StripScopeAgent2::FitWindow, 
		"HH", "Fit Window", true ) );
	cl->registerCommand( new ActionCommand( StripScopeAgent2::ForwardStrip, 
		"FS", "Forward", true ) );
	cl->registerCommand( new ActionCommand( StripScopeAgent2::NextStrip, 
		"NN", "Next Strip", true ) );
	cl->registerCommand( new ActionCommand( StripScopeAgent2::BackwardStrip, 
		"BS", "Backward", true ) );
	cl->registerCommand( new ActionCommand( StripScopeAgent2::PrevStrip, 
		"PP", "Prev. Strip", true ) );
	cl->registerCommand( new ActionCommand( StripScopeAgent2::ShowLowRes, 
		"LR", "Low Resolution", true ) ); // TODO: Argument
	cl->registerCommand( new ActionCommand( StripScopeAgent2::ContourParams, 
		"CP", "Contour Params", true ) );
	cl->registerCommand( new ActionCommand( StripScopeAgent2::PickSpin, 
		"PI", "Pick Spin", true ) );
	cl->registerCommand( new ActionCommand( StripScopeAgent2::MoveSpin, 
		"MI", "Move Spin", true ) );
	cl->registerCommand( new ActionCommand( StripScopeAgent2::MoveSpinAlias, 
		"AI", "Move Spin Alias", true ) );
	cl->registerCommand( new ActionCommand( StripScopeAgent2::DeleteSpins, 
		"DI", "Delete Spins", true ) );
	cl->registerCommand( new ActionCommand( StripScopeAgent2::ShowAlignment, 
		"SA", "Show Alignment", true ) );
	cl->registerCommand( new ActionCommand( StripScopeAgent2::DeleteLinks, 
		"DL", "Delete Spin Links", true ) );

	cl->registerCommand( new ActionCommand( StripScopeAgent2::ShowWithOff, 
		"RP", "Resolve Projected Spins", true ) );

	ActionCommand* cmd = new ActionCommand( StripScopeAgent2::HoldStrip, 
		"SH", "Hold Strip", false );
	cmd->registerParameter( Any::UShort, false ); // 1..n Default Focus-Strip
	cl->registerCommand( cmd );

	cmd = new ActionCommand( StripScopeAgent2::GotoStrip, 
		"GY", "Goto System", false );
	cmd->registerParameter( Any::ULong, false ); // 1..n Default Focus-Strip
	cl->registerCommand( cmd );

	cmd = new ActionCommand( StripScopeAgent2::GotoStrip, 
		"GS", "Goto System", false );
	cmd->registerParameter( Any::ULong, false ); // 1..n Default Focus-Strip
	cl->registerCommand( cmd );

	cmd = new ActionCommand( StripScope2::GotoResidue, 
		"GR", "Goto Residue", false );
	cmd->registerParameter( Any::ULong, false ); // 1..n Default Focus-Strip
	cl->registerCommand( cmd );

	cmd = new ActionCommand( StripScopeAgent2::ReplaceStrip, 
		"RS", "Replace Strip", false );
	cmd->registerParameter( Any::ULong, false ); // 1..n Default Inputbox
	cl->registerCommand( cmd );

	cmd = new ActionCommand( StripScopeAgent2::ForceLabelSpin, 
		"LI", "Force Spin Label", false );
	cmd->registerParameter( Any::CStr, false ); // Label oder leer
	cl->registerCommand( cmd );

	cmd = new ActionCommand( StripScope2::ShowRulers, 
		"SR", "Show Rulers", false );
	cmd->registerParameter( Any::Short, false ); // Leer oder Auflsung
	cl->registerCommand( cmd );

	cmd = new ActionCommand( StripScopeAgent2::LinkStrip, "LK", "Link Strip", false );
	cmd->registerParameter( Any::ULong, false ); // 1..n Pred, Succ oder Nil
	cmd->registerParameter( Any::ULong, false ); // 1..n Succ oder Nil
	cl->registerCommand( cmd );

	cmd = new ActionCommand( StripScopeAgent2::AssignStrip, "AS", "Assign Strip", false );
	cmd->registerParameter( Any::ULong, true ); // 1..n Sequence
	cl->registerCommand( cmd );

	cmd = new ActionCommand( StripScope2::SetStripWidth, "SW", "Set Strip Width", false );
	cmd->registerParameter( Any::CStr, true ); 
	cmd->registerParameter( Any::Float, true ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( StripScope2::SetStripTol, "ST", "Set Strip Tolerance", false );
	cmd->registerParameter( Any::CStr, true ); 
	cmd->registerParameter( Any::Float, true ); 
	cl->registerCommand( cmd );

	cl->registerCommand( new ActionCommand( StripScopeAgent2::NextSpec, 
		"NS", "Next Spectrum", true ) );
	cl->registerCommand( new ActionCommand( StripScopeAgent2::PrevSpec, 
		"PS", "Prev. Spectrum", true ) );

	cmd = new ActionCommand( StripScopeAgent2::AutoGain, 
		"AG", "Set Auto Contour Gain", false );
	cmd->registerParameter( Any::Float, true ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( Root::Action::ExecuteLine, "LUA", "Lua", false );
	cmd->registerParameter( Any::Memo, true ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( StripScopeAgent2::PickLabel, 
		"PL", "Pick Label", false );
	cmd->registerParameter( Any::CStr, false ); // Label oder leer
	cl->registerCommand( cmd );

	cmd = new ActionCommand( StripScopeAgent2::ViewLabels, 
		"LL", "Label Format", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( StripScope2::SetWidthFactor, 
		"WF", "Set Strip Width Factor", false );
	cmd->registerParameter( Any::Float, false ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( StripScopeAgent2::SetDepth, 
		"SD", "Set Peak Depth", false );
	cmd->registerParameter( Any::Float, false ); 
	cl->registerCommand( cmd );


}

//////////////////////////////////////////////////////////////////////

StripScope2::StripScope2(Root::Agent* a,Project* pro, Spectrum* s):
	Lexi::MainWindow( a, true, true )
{
	assert( s );
	assert( pro );
	if( s->getDimCount() != 3 )	
		throw Root::Exception( "Invalid number of dimensions" );
	d_agent = new StripScopeAgent2( this, pro, s );

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
	d_widget->getViewport()->damageAll();
}

StripScope2::~StripScope2()
{
	d_widget->setBody( 0 );
	d_agent = 0;
}

void StripScope2::updateCaption()
{
	QString str;
	str.sprintf( "StripScope - %s", d_agent->getSpec()->getName() );
	getQt()->setCaption( str );
}

void StripScope2::buildMenus()
{
	Gui::Menu* menuFile = new Gui::Menu( menuBar() );
    Gui::Menu::item( menuFile, this, Root::Action::FileSave, Qt::CTRL+Qt::Key_S );
	Gui::Menu::item( menuFile, d_agent, "Print Preview...", 
		StripScopeAgent2::CreateReport, false, Qt::CTRL+Qt::Key_P );
	Gui::Menu* sub = new Gui::Menu( menuFile );
	Gui::Menu::item( sub, this, "&Anchor Peaklist...", ExportAnchorPeaks, false );
	Gui::Menu::item( sub, this, "&Strip Peaklist...", ExportStripPeaks, false );
	menuFile->insertItem( "Export", sub );
	menuFile->insertSeparator();
    Gui::Menu::item( menuFile, this, Root::Action::WindowClose, Qt::CTRL+Qt::Key_W );
	menuBar()->insertItem( "&File", menuFile );

	Gui::Menu* menuEdit = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuEdit, this, Root::Action::EditUndo, Qt::CTRL+Qt::Key_Z );
	Gui::Menu::item( menuEdit, this, Root::Action::EditRedo, Qt::CTRL+Qt::Key_Y );
	menuBar()->insertItem( "&Edit", menuEdit );

	Gui::Menu* menuView = new Gui::Menu( menuBar() );
	menuView->insertItem( "Select Spectrum", d_agent->getPopSpec() );
	Gui::Menu::item( menuView, d_agent, "Hold Reference Spectrum", 
		StripScopeAgent2::KeepSpec, true );
	Gui::Menu::item( menuView, d_agent, "Set Contour Parameters...", 
		StripScopeAgent2::ContourParams, false );
	Gui::Menu::item( menuView, d_agent, "Auto Contour Level", 
		StripScopeAgent2::AutoContour, true );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Set Resolution...", 
		StripScopeAgent2::SetResolution, false );
	Gui::Menu::item( menuView, d_agent, "Show Low Resolution", 
		StripScopeAgent2::ShowLowRes, true );
	menuView->insertSeparator();
	sub = new Gui::Menu( menuBar() );
	menuView->insertItem( "Page", sub );
	Gui::Menu::item( sub, d_agent, "Next Page", 
		StripScopeAgent2::ForwardStrip, false, Qt::CTRL+Qt::Key_R );
	Gui::Menu::item( sub, d_agent, "Previous Page", 
		StripScopeAgent2::BackwardStrip, false, Qt::CTRL+Qt::Key_E );
	Gui::Menu::item( sub, d_agent, "Next Strip", 
		StripScopeAgent2::NextStrip, false, Qt::CTRL+Qt::Key_Right );
	Gui::Menu::item( sub, d_agent, "Previous Strip", 
		StripScopeAgent2::PrevStrip, false, Qt::CTRL+Qt::Key_Left );

	sub = new Gui::Menu( menuBar() );
	menuView->insertItem( "Select Strips", sub );
	Gui::Menu::item( sub, d_agent, "Strict Strip Matching", 
		StripScopeAgent2::StrictStripMatch, true );
	Gui::Menu::item( sub, d_agent, "Unlabeled Strip Matching", 
		StripScopeAgent2::UnlabeledStripMatch, true );
	sub->insertSeparator();

	Gui::Menu::item( sub, d_agent, "Fit Selected Spins", StripScopeAgent2::SelectStrips, 
		true, Qt::CTRL+Qt::Key_F )->addParam( short( StripQuery2::SpinFit ) );

	Gui::Menu::item( sub, d_agent, "Fit All Spins", StripScopeAgent2::SelectStrips, 
		true )->addParam( short( StripQuery2::AllBest ) );

	Gui::Menu::item( sub, d_agent, "Select Manually...", StripScopeAgent2::SelectStrips, true, 
		Qt::CTRL+Qt::Key_M )->addParam( short( StripQuery2::Manual ) );

	Gui::Menu::item( sub, d_agent, "All Strips", StripScopeAgent2::SelectStrips, true, 
		Qt::CTRL+Qt::Key_A )->addParam( short( StripQuery2::All ) );

	Gui::Menu::item( sub, d_agent, "All Assigned Strips", StripScopeAgent2::SelectStrips, 
		true )->addParam( short( StripQuery2::AllAssigned ) );

	Gui::Menu::item( sub, d_agent, "Unpicked Strips", StripScopeAgent2::SelectStrips, 
		true )->addParam( short( StripQuery2::UnpickedStrips ) );

	Gui::Menu::item( sub, d_agent, "Unlinked Strips", StripScopeAgent2::SelectStrips, 
		true )->addParam( short( StripQuery2::UnlinkedStrips ) );

	Gui::Menu::item( sub, d_agent, "Fragment", StripScopeAgent2::SelectStrips, 
		true )->addParam( short( StripQuery2::Fragment ) );

	Gui::Menu::item( sub, d_agent, "Best free Successors", StripScopeAgent2::SelectStrips, 
		true )->addParam( short( StripQuery2::BestSucc ) );

	Gui::Menu::item( sub, d_agent, "Best free Predecessors", StripScopeAgent2::SelectStrips, 
		true )->addParam( short( StripQuery2::BestPred ) );

	Gui::Menu::item( sub, d_agent, "Best free Pred. & Succ.", StripScopeAgent2::SelectStrips, 
		true )->addParam( short( StripQuery2::BestFit ) );

	Gui::Menu::item( sub, d_agent, "All best Successors", StripScopeAgent2::SelectStrips, 
		true )->addParam( short( StripQuery2::AllBestSucc ) );

	Gui::Menu::item( sub, d_agent, "All best Predecessors", StripScopeAgent2::SelectStrips, 
		true )->addParam( short( StripQuery2::AllBestPred ) );

	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Resolve Projected Spins", 
		StripScopeAgent2::ShowWithOff, true );
	Gui::Menu::item( menuView, d_agent, "Show Spin Links", 
		StripScopeAgent2::ShowLinks, true );
	Gui::Menu::item( menuView, d_agent, "Show Infered Peaks", 
		StripScopeAgent2::ShowInfered, true );
	Gui::Menu::item( menuView, d_agent, "Show Unlabeled Peaks", 
		StripScopeAgent2::ShowUnlabeled, true );
	Gui::Menu::item( menuView, d_agent, "Show Peaks with unknown Labels", 
		StripScopeAgent2::ShowUnknown, true );
	sub = new Gui::Menu( menuBar() );
	menuView->insertItem( "Do Pathway Simulation", sub );
	Gui::Menu::item( sub, d_agent, "Strip Peaks", 
		StripScopeAgent2::DoPathSim, true );
	Gui::Menu::item( sub, d_agent, "Strip Anchors", 
		StripScopeAgent2::AnchPathSim, true );
	Gui::Menu::item( menuView, d_agent, "Show Ghost Peaks", 
		StripScopeAgent2::ShowGhosts, true );
	Gui::Menu::item( menuView, d_agent, "Show Ghost Labels", 
		StripScopeAgent2::GhostLabels, true );
	Gui::Menu::item( menuView, d_agent, "Show Hidden Links", 
		StripScopeAgent2::ShowAllPeaks, true );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Hold Reference Strip", 
		StripScopeAgent2::HoldStrip, true );
	Gui::Menu::item( menuView, this, "Select Ruler Residue...", 
		StripScope2::SelectRuler, false );
	Gui::Menu::item( menuView, this, "Show Rulers", 
		StripScope2::ShowRulers, true );
	Gui::Menu::item( menuView, d_agent, "Set Lane Count...", 
		StripScope2::SetLaneCount, false );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Show Folded", 
		StripScopeAgent2::ShowFolded, true );
	Gui::Menu::item( menuView, d_agent, "Center to Peak", 
		StripScopeAgent2::AutoCenter, true );
	sub = new Gui::Menu( menuBar() );
	menuView->insertItem( "Show Labels", sub );
	short i;
	for( i = SpinPointView::None; i < SpinPointView::End; i++ )
	{
		Gui::Menu::item( sub, d_agent, SpinPointView::menuText[ i ], 
			StripScopeAgent2::ViewLabels, true )->addParam( i );
	}
	Gui::Menu::item( menuView, d_agent, "Sync to Global Cursor", 
		StripScopeAgent2::CursorSync, true );
	Gui::Menu::item( menuView, d_agent, "Fit Window", 
		StripScopeAgent2::FitWindow, false, Qt::Key_Home );
	menuBar()->insertItem( "&View", menuView );

	Gui::Menu* menuStrip = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuStrip, d_agent, "Calibrate", 
		StripScopeAgent2::Calibrate, false );
	menuStrip->insertSeparator();
	Gui::Menu::item( menuStrip, d_agent, "&Pick Spin", 
		StripScopeAgent2::PickSpin, false );
	Gui::Menu::item( menuStrip, d_agent, "Pick Label...", 
		StripScopeAgent2::PickLabel, false );
	Gui::Menu::item( menuStrip, d_agent, "Propose Spin...", 
		StripScopeAgent2::ProposeSpin, false );
	Gui::Menu::item( menuStrip, d_agent, "Hide/Show Link", 
		StripScopeAgent2::HidePeak, false );
	Gui::Menu::item( menuStrip, d_agent, "&Move Spin", 
		StripScopeAgent2::MoveSpin, false );
	Gui::Menu::item( menuStrip, d_agent, "&Move Spin Alias", 
		StripScopeAgent2::MoveSpinAlias, false );

	menuStrip->insertItem( "Label Spin", d_agent->getPopLabel() );
	Gui::Menu::item( sub, d_agent, "?", 0, false );
	Gui::Menu::item( sub, d_agent, "?-1", 0, false );

	Gui::Menu::item( menuStrip, d_agent, "&Force Spin Label", 
		StripScopeAgent2::ForceLabelSpin, false );
	Gui::Menu::item( menuStrip, d_agent, "&Delete Spins", 
		StripScopeAgent2::DeleteSpins, false );
	Gui::Menu::item( menuStrip, d_agent, "Delete Spin Links", 
		StripScopeAgent2::DeleteLinks, false );
	Gui::Menu* menuAtts = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuAtts, d_agent, "Spin...", 
		StripScopeAgent2::EditAtts, false, Qt::CTRL+Qt::Key_Return );
	Gui::Menu::item( menuAtts, d_agent, "System...", 
		StripScopeAgent2::EditAttsSys, false );
	Gui::Menu::item( menuAtts, d_agent, "Spin Link...", 
		StripScopeAgent2::EditAttsLink, false );
	menuStrip->insertItem( "&Edit Attributes", menuAtts );
	menuStrip->insertSeparator();
	Gui::Menu::item( menuStrip, d_agent, "&Link to Reference", 
		StripScopeAgent2::LinkReference, false );
	Gui::Menu::item( menuStrip, d_agent, "&Link Spin Systems...", 
		StripScopeAgent2::LinkStrip, false );
	Gui::Menu::item( menuStrip, d_agent, "Unlink &Predecessor", 
		StripScopeAgent2::UnlinkPred, false );
	Gui::Menu::item( menuStrip, d_agent, "Unlink &Successor", 
		StripScopeAgent2::UnlinkSucc, false );
	menuStrip->insertSeparator();
	Gui::Menu::item( menuStrip, d_agent, "&Assign to Sequence...", 
		StripScopeAgent2::AssignStrip, false );
	Gui::Menu::item( menuStrip, d_agent, "&Unassign Spin Systems", 
		StripScopeAgent2::UnassignStrip, false );
	menuStrip->insertSeparator();
	Gui::Menu::item( menuStrip, d_agent, "Set Peak Width...", 
		StripScopeAgent2::SetWidth, false );
	Gui::Menu::item( menuStrip, d_agent, "Set Spin Tolerance...", 
		StripScopeAgent2::SetTol, false );
	Gui::Menu::item( menuStrip, d_agent, "Show Alignment...", 
		StripScopeAgent2::ShowAlignment, false );
	menuBar()->insertItem( "&Strips", menuStrip );

	Gui::Menu* menuSlices = new Gui::Menu( menuBar() );
 	Gui::Menu::item( menuSlices, d_agent, "Set Bounds...", 
		StripScopeAgent2::SetBounds, false );
 	Gui::Menu::item( menuSlices, d_agent, "Pick Bounds", 
		StripScopeAgent2::PickBounds, false );
 	Gui::Menu::item( menuSlices, d_agent, "Pick Bounds Sym.", 
		StripScopeAgent2::PickBoundsSym, false );
 	Gui::Menu::item( menuSlices, d_agent, "Auto Scale", 
		StripScopeAgent2::AutoSlice, true );
	menuBar()->insertItem( "S&lices", menuSlices );

	Gui::Menu* menuSetup = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuSetup, this, "Run Automatic Strip Matcher", StripScope2::RunStripper, true );
	Gui::Menu::item( menuSetup, this, "Strict Strip Matching", StripScope2::StrictStripMatch, true );
	Gui::Menu::item( menuSetup, this, "Unlabeled Strip Matching", StripScope2::UnlabeledStripMatch, true );
	menuSetup->insertSeparator();
	Gui::Menu::item( menuSetup, this, "Run Sequence Mapper", StripScope2::RunMapper, true );
	Gui::Menu::item( menuSetup, this, "Geometric Mapping", StripScope2::MapGeometric, true );
	Gui::Menu::item( menuSetup, this, "Exclude Assigneds", StripScope2::MapExclusive, true );
	Gui::Menu::item( menuSetup, this, "Use Neighbours", StripScope2::MapExpand, true );
	Gui::Menu::item( menuSetup, this, "Min. Frag. Length...", StripScope2::MinFragLen, false );
	Gui::Menu::item( menuSetup, this, "Max. Match Count...", StripScope2::MapMatchCount, false );
	menuBar()->insertItem( "&Auto Setup", menuSetup );

	Gui::Menu* menuHelp = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuHelp, this, Root::Action::HelpAbout );
	menuBar()->insertItem( "&?", menuHelp );

}

void StripScope2::buildViews()
{
	Splitter* inner = new Splitter( 1, 3 );	// TODO: X/Z-Slices
	inner->setBarWidth( 80 );

	d_focus->clear();
	d_focus->setCircle( false );
	// TODO d_stripBox leeren?

	d_stripBox = LayoutKit::hbox();
	Layer* strips = new Layer();

	strips->append( d_stripBox );
	inner->setPane( strips, 0, 2 );

	d_list = new StripListGadget( d_widget->getViewport(), d_agent->getProject(),
		this );
	inner->setPane( d_list, 0, 0 );

	Gui::Menu* menu = new Gui::Menu( d_list->getListView()->getImp(), true );
	Gui::Menu::item( menu, this, "Goto...", Goto, false );
	Gui::Menu::item( menu, d_list, "Find Spin...", StripListGadget::FindSpin, false );
	menu->addSeparator();
	/* Weg damit, da Fred dadurch verwirrt wurde.
	Gui::Menu::item( menu, d_list, "Run Automatic Strip Matcher", 
		StripListGadget::RunStripper, 0, true );
	Gui::Menu::item( menu, d_list, "Strict Strip Matching", 
		StripListGadget::StrictStripMatch, 0, true );
	Gui::Menu::item( menu, d_list, "Unlabeled Strip Matching", 
		StripListGadget::UnlabeledStripMatch, 0, true );
	Gui::Menu::item( menu, d_list, "Set Spin Tolerance...", StripListGadget::SetSpinTol, false );
	menu->addSeparator();
	*/
	Gui::Menu::item( menu, this, "Show Free Successors", ShowSucc, false );
	Gui::Menu::item( menu, this, "Show Free Predecessors", ShowPred, false );
	Gui::Menu::item( menu, this, "Show All Successors", ShowAllSucc, false );
	Gui::Menu::item( menu, this, "Show All Predecessors", ShowAllPred, false );
	Gui::Menu::item( menu, this, "Show All Fits for all Spins", ShowAllBest, false );
	Gui::Menu::item( menu, this, "Show Fits for Seleced Spin", ShowSpinFits, false );
	Gui::Menu::item( menu, this, "Show Fragment", ShowFrag, false );
	Gui::Menu::item( menu, this, "Show System", ShowSystem, false );
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
	Gui::Menu::item( menu, d_list, "Set Spin Tolerance...", StripListGadget::SetSpinTol, false );

	d_agent->initViews( 5 ); // RISK
	// Slice
	inner->setPane( d_agent->getSlice().d_viewer, 0, 1 );
	d_focus->append( d_agent->getSlice().d_viewer->getController() );
	for( int i = 0; i < 5; i++ )
	{
		if( i != 0 )
			d_stripBox->append( new Rule( DimY, g_frameClr, 40 ) );
		d_stripBox->append( d_agent->getStrip( i ).d_viewer );
		d_focus->append( d_agent->getStrip( i ).d_viewer->getController() );
	}
	d_agent->setSpec( d_agent->getSpec() );
	d_resiView = new ResidueParamView( d_agent->getStrip( 0 ).d_viewer ); // RISK
	// TODO: Viewer wird bei neuem Aufruf von initViews ungltig!
	strips->append( d_resiView );
	
	d_focus->setBody( inner );
	inner->setColumnPos( 1,  5 * d_widget->width() );
	inner->setColumnPos( 2,  7 * d_widget->width() );
	d_widget->setFocusGlyph( d_agent->getStrip( 0 ).d_viewer->getController() );

	d_list->setSpec( d_agent->getSpec() );
}

void StripScope2::handleAction(Root::Action& a)
{
	if( !EXECUTE_ACTION( StripScope2, a ) )
		MainWindow::handleAction( a );
}

void StripScope2::handle(Root::Message& m)
{
	BEGIN_HANDLER();
	MESSAGE( StripListGadget::ActivateMsg, e, m )
	{
        Q_UNUSED(e)
		if( d_list )
			d_agent->gotoTuple( d_list->getSelectedStrip(), d_list->getSelectedSpin() );
		m.consume();
	}
	MESSAGE( StripScopeAgent2::AdjustSelection, e, m )
	{
		m.consume();
		d_list->showStrip( e->getPoint()[0]->getSystem() );
	}
	HANDLE_ELSE()
		MainWindow::handle( m );
	END_HANDLER();
}

void StripScope2::handleShowRulers(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_resiView->show() );

	if( !a.getParam( 0 ).isNull() )
	{
		Residue* r = d_agent->getProject()->getSequence()->getResidue( 
			a.getParam( 0 ).getShort() );
		if( r == 0 )
		{
			Lexi::ShowStatusMessage msg(  "Show Ruler: Unknown Residue" );
			traverseUp( msg );
			return;
		}
		d_resiView->setResidue( r );
	}else
		d_resiView->show( !d_resiView->show() );
	d_widget->getViewport()->damageAll(); // RISK
}

void StripScope2::handleSelectRuler(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	bool ok	= FALSE;
	Index res;
	if( a.getParamCount() == 0 )
	{
		res	= QInputDialog::getInteger( "Select Ruler Residue", 
			"Please	enter ID of the residue:", 
			0, -99999, 99999, 1, &ok, d_widget );
		if( !ok )
			return;
	}else
	{
		res = a.getParam( 0 ).getShort();	
	}
	Residue* r = d_agent->getProject()->getSequence()->getResidue( res );
	if( r == 0 )
	{
		Lexi::ShowStatusMessage msg(  "Select Ruler Residue: Unknown Residue" );
		traverseUp( msg );
		return;
	}
	d_resiView->setResidue( r );
	d_widget->getViewport()->damageAll(); // RISK
}

void StripScope2::handleSetLaneCount(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	bool ok	= FALSE;
	int res	= QInputDialog::getInteger( "Set strip count", 
		"Please	enter the number of strips to display:", 
		d_agent->getLaneCount(), 1, 10,	1, &ok,	d_widget );
	if( !ok )
		return;

	d_focus->clear();
	while( d_stripBox->getCount() )
		d_stripBox->remove( 0 );
	d_agent->initViews( res, false ); 
	for( int i = 0; i < res; i++ )
	{
		if( i != 0 )
			d_stripBox->append( new Rule( DimY, g_frameClr, 40 ) );
		d_stripBox->append( d_agent->getStrip( i ).d_viewer );
		d_focus->append( d_agent->getStrip( i ).d_viewer->getController() );
	}
	d_widget->getViewport()->damageAll();
}

void StripScope2::handleShowSucc(Action & a)
{
	ACTION_ENABLED_IF( a, d_list->getSelectedStrip() != 0 );
	d_agent->resetFirst(); 
	SpinPoint tuple;
	if( !d_agent->findTuple( d_list->getSelectedStrip(),
		d_list->getSelectedSpin(), tuple ) )
		return;
	d_agent->updateSpecType();
	d_agent->getQuery()->query( StripQuery2::BestSucc, tuple );
	d_agent->fillPage();
}

void StripScope2::handleShowPred(Action & a)
{
	ACTION_ENABLED_IF( a, d_list->getSelectedStrip() != 0 );
	d_agent->resetFirst(); 
	SpinPoint tuple;
	if( !d_agent->findTuple( d_list->getSelectedStrip(),
		d_list->getSelectedSpin(), tuple ) )
		return;
	d_agent->updateSpecType();
	d_agent->getQuery()->query( StripQuery2::BestPred, tuple );
	d_agent->fillPage();
}

void StripScope2::handleShowAllSucc(Action & a)
{
	ACTION_ENABLED_IF( a, d_list->getSelectedStrip() != 0 );
	d_agent->resetFirst(); 
	SpinPoint tuple;
	if( !d_agent->findTuple( d_list->getSelectedStrip(),
		d_list->getSelectedSpin(), tuple ) )
		return;
	d_agent->updateSpecType();
	d_agent->getQuery()->query( StripQuery2::AllBestSucc, tuple );
	d_agent->fillPage();
}

void StripScope2::handleShowAllPred(Action & a)
{
	ACTION_ENABLED_IF( a, d_list->getSelectedStrip() != 0 );
	d_agent->resetFirst(); 
	SpinPoint tuple;
	if( !d_agent->findTuple( d_list->getSelectedStrip(),
		d_list->getSelectedSpin(), tuple ) )
		return;
	d_agent->updateSpecType();
	d_agent->getQuery()->query( StripQuery2::AllBestPred, tuple );
	d_agent->fillPage();
}

void StripScope2::handleShowFrag(Action & a)
{
	ACTION_ENABLED_IF( a, d_list->getSelectedStrip() != 0 &&
		d_agent->getQuery()->canQuery( StripQuery2::Fragment ) );
	d_agent->resetFirst(); 
	if( !d_agent->getQuery()->setReference( d_list->getSelectedStrip() ) )
	{
		Lexi::ShowStatusMessage msg( "Show Fragment: Spin Labels don't fit Spectrum" );
		traverse( msg );
		return;
	}
	d_agent->updateSpecType();
	d_agent->getQuery()->query( StripQuery2::Fragment, 
		d_agent->getQuery()->getReference() );
	d_agent->fillPage();
}

void StripScope2::handleSetStripWidth(Action & a)
{
	ACTION_ENABLED_IF( a, a.getParamCount() == 2 );

	AtomType t = AtomType::parseLabel( a.getParam( 0 ).getCStr() );
	if( t.isNone() )
	{
		Lexi::ShowStatusMessage msg( "Set Strip Width: invalid atom type" );
		traverseUp( msg );
		return;
	}
	PPM p = a.getParam( 1 ).getFloat();
	d_agent->getProject()->setStripWidth( t, p );
}

void StripScope2::handleSetStripTol(Action & a)
{
	ACTION_ENABLED_IF( a, a.getParamCount() == 2 );

	AtomType t = AtomType::parseLabel( a.getParam( 0 ).getCStr() );
	if( t.isNone() )
	{
		Lexi::ShowStatusMessage msg( "Set Strip Tolerance: invalid atom type" );
		traverseUp( msg );
		return;
	}
	PPM p = a.getParam( 1 ).getFloat();
	d_agent->getProject()->getMatcher()->setTol( t, p );
	d_agent->getQuery()->requery();
	d_agent->getProject()->getStripper()->recalcAll();
}

void StripScope2::handleRunStripper(Action & a)
{
	Stripper* s = d_agent->getProject()->getStripper();
	ACTION_CHECKED_IF( a, true, s->isOn() );

	s->setOn( !s->isOn() );
}

void StripScope2::handleUnlabeledStripMatch(Action & a)
{
	Stripper* s = d_agent->getProject()->getStripper();
	ACTION_CHECKED_IF( a, true, s->isUnlabeled() );

	s->setUnlabeled( !s->isUnlabeled() );
	if( s->isUnlabeled() )
		s->setStrict( false );
}

void StripScope2::handleStrictStripMatch(Action & a)
{
	Stripper* s = d_agent->getProject()->getStripper();
	ACTION_CHECKED_IF( a, true, s->isStrict() );

	s->setStrict( !s->isStrict() );
}

void StripScope2::handleExportAnchorPeaks(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	QString fileName = QFileDialog::getSaveFileName( getQt(), "Export Anchor Pealist",
                                                     Root::AppAgent::getCurrentDir(),
                                                      "*.peaks" );
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
		Spectrum* spec = d_agent->getSpec();
		PpmPoint p( 0, 0 );
		PointSet::Assig a;
		Root::Index id;
		ColorMap cm;
		cm.push_back( spec->getColor( DimX ) );
		cm.push_back( spec->getColor( DimZ ) );
		Root::Ref<PointSet> ps = Factory::createEasyPeakList( cm );
		SpinSpace* mdl = d_agent->getMdl2D();
		SpinSpace::Iterator i, _end = mdl->end();
		SpinSpace::Element e;
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
		}
		ps->saveToFile( fileName.toAscii().data() );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Error Exporting Peaklist", e.what() );
	}
}

void StripScope2::handleExportStripPeaks(Action & a)
{
	Spectrum* spec = d_agent->getSpec();
	ACTION_ENABLED_IF( a, spec );

	QString fileName = QFileDialog::getSaveFileName( getQt(), "Export Strip Pealist",
                                                     Root::AppAgent::getCurrentDir(),
                                                        "*.peaks" );
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
		PpmPoint p( 0, 0, 0 );
		PointSet::Assig a;
		char buf[32];
		Root::Index id;
		Root::Ref<PointSet> ps = Factory::createEasyPeakList( spec );
		SpinSpace* mdl = d_agent->getMdl3D();
		SpinSpace::Iterator i, _end = mdl->end();
		SpinSpace::Element e;
		SpinPointView::Label l = d_agent->getStrip( 0 ).d_tuples->getLabel();
		for( i = mdl->begin(); i != _end; ++i )
		{
			mdl->fetch( i, e );
			if( e.isGhost() || e.isHidden() )
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

			SpinPointView::formatLabel( buf, sizeof(buf), e.d_point, l, DimY );
			ps->setComment( id, buf );
		}
		ps->saveToFile( fileName.toAscii().data() );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Error Exporting Peaklist", e.what() );
	}
}

void StripScope2::handleShowAllBest(Action & a)
{
	ACTION_ENABLED_IF( a, d_list->getSelectedStrip() != 0 );
	d_agent->resetFirst(); 
	if( !d_agent->getQuery()->setReference( d_list->getSelectedStrip() ) )
	{
		Lexi::ShowStatusMessage msg( "Show All Best: Spin Labels don't fit Spectrum" );
		traverse( msg );
		return;
	}
	d_agent->updateSpecType();
	d_agent->getQuery()->query( StripQuery2::AllBest );
	d_agent->fillPage();
}

void StripScope2::handleShowSpinFits(Action & a)
{
	ACTION_ENABLED_IF( a, d_list->getSelectedSpin() != 0 );
	d_agent->resetFirst(); 
	if( !d_agent->getQuery()->setReference( d_list->getSelectedStrip() ) )
	{
		Lexi::ShowStatusMessage msg( "Show All Best: Spin Labels don't fit Spectrum" );
		traverse( msg );
		return;
	}
	d_agent->updateSpecType();
	d_agent->getQuery()->query( StripQuery2::SpinFit, d_list->getSelectedSpin() );
	d_agent->fillPage();
}

void StripScope2::handleGoto(Action & a)
{
	ACTION_ENABLED_IF( a, d_list );
	d_agent->gotoTuple( d_list->getSelectedStrip(), d_list->getSelectedSpin() );
}

void StripScope2::handleGotoResidue(Action & a)
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
		StripScopeAgent2::GotoStrip, false );
	action.addParam( Root::Int32(resi->getSystem()->getId()) );
	action.execute();
}

void StripScope2::handleSetWidthFactor(Action & a)
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

void StripScope2::handleSetDepth(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	float g;
	if( a.getParam( 0 ).isNull() )
	{
		bool ok	= FALSE;
		QString res;
		res.sprintf( "%f", d_agent->getProject()->inferPeakWidth( DimZ, 
			d_agent->getSpec() ) );
		res	= QInputDialog::getText( "Set Peak Depth", 
			"Please	enter a positive ppm value:", QLineEdit::Normal, res, &ok, getQt() );
		if( !ok )
			return;
		g = res.toFloat( &ok );
	}else
		g = a.getParam( 0 ).getFloat();
	if( g < 0.0 )
	{
		Root::ReportToUser::alert( this, "Set Peak Depth", "Invalid PPM Value" );
		return;
	}
	d_agent->getProject()->setPeakWidth( DimZ, g, d_agent->getSpec() );
}

void StripScope2::handleSelectSpec(Action & a)
{
	Spectrum* spec = dynamic_cast<Spectrum*>( a.getParam( 0 ).getObject() );
	ACTION_CHECKED_IF( a, spec, spec && d_agent->getSpec()->getId() == spec->getId() );

	Lexi::Viewport::pushHourglass();
	d_agent->setSpec( spec );
	assert( d_list );
	d_list->setSpec( spec );
	Lexi::Viewport::popCursor();
}

void StripScope2::handleNextSpec(Action & a)
{
	ACTION_ENABLED_IF( a, d_agent->getSort().size() > 1 ); 
	d_agent->stepSpec( true );
	assert( d_list );
	d_list->setSpec( d_agent->getSpec() );
}

void StripScope2::handlePrevSpec(Action & a)
{
	ACTION_ENABLED_IF( a, d_agent->getSort().size() > 1 ); 
	d_agent->stepSpec( false );
	assert( d_list );
	d_list->setSpec( d_agent->getSpec() );
}

void StripScope2::handleShowSystem(Action & a)
{
	ACTION_ENABLED_IF( a, d_list->getSelectedStrip() != 0 );

	d_agent->resetFirst(); 
	StripQuery2::Result f;
	SpinSpace::Result tuples;
	SpinSpace::Result::const_iterator i;
	d_agent->getMdl2D()->find( tuples, d_list->getSelectedStrip() );
	if( tuples.empty() )
	{
		Lexi::ShowStatusMessage msg( 
			"Select Strips: System not represented in spectrum" );
		traverse( msg );
		return;
	}
	for( i = tuples.begin(); i != tuples.end(); ++i )
		f.push_back( (*i).d_point );

	d_agent->getQuery()->setManual( f, true );
	d_agent->updateSpecType();
	d_agent->fillPage();
}


