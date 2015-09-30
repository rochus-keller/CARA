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

#include "HomoScope2.h"

// Qt
#include <qtextstream.h> 
#include <qinputdialog.h> 
#include <qmessagebox.h>
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
#include <Lexi/Printer.h>
#include <Lexi/Shapes.h>
using namespace Lexi;

//* Spec
#include <Spec/Factory.h>
#include <Spec/SpecProjector.h>
#include <Spec/SpectrumPeer.h>
#include <Spec/SpectrumType.h>
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
#include "PeakListGadget.h"
#include <SpecView/PeakPlaneView.h>

#include <Dlg.h>

using namespace Spec;

//////////////////////////////////////////////////////////////////////
// Entkopplung; nutzbar ohne include

void createHomoScope2(Root::Agent* a, Spec::Spectrum* s, Spec::Project* p)
{ 
	new HomoScope2( a, s, p );
}

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr HomoScope2::ShowList = "ShowList";

ACTION_SLOTS_BEGIN( HomoScope2 )
    { HomoScope2::ShowList, &HomoScope2::handleShowList },
ACTION_SLOTS_END( HomoScope2 )

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
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::SetSystemType, "SY", "Set System Type", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::RemoveRulers, "RS", "Remove Selected Rulers", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::RemoveAllRulers, "RA", "Remove All Rulers", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::AddRulerVerti, "RH", "Add Horizontal Ruler", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::AddRulerHori, "RV", "Add Vertical Ruler", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::HoldReference, "HR", "Hold Reference", true ) );
	cl->registerCommand( new ActionCommand( HomoScope2::SyncHori, "SH", "Sync. Hori. Spin", true ) );
	cl->registerCommand( new ActionCommand( HomoScope2::SyncVerti, "SV", "Sync. Verti. Spin", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::ShowFolded, "SF", "Show Folded", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::AutoContour, "AC", "Auto Contour Level", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::Backward, "BB", "Backward", true ) );
	cl->registerCommand( new ActionCommand( HomoScope2::ShowList, "SL", "Show List", true ) );
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

	ActionCommand* cmd = new ActionCommand( 
		HomoScope2::SetTolerance, "ST", "Set Tolerance", false );
	cmd->registerParameter( Any::CStr, true, "Atom Type" ); 
	cmd->registerParameter( Any::Float, true, "Tolerance (ppm)" ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::LinkSystems, "LK", "Link Systems", false );
	cmd->registerParameter( Any::ULong, false, "Predecessor" ); // 1..n Pred, Succ oder Nil
	cmd->registerParameter( Any::ULong, false, "Successor" ); // 1..n Succ oder Nil
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
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::SetLinkParams, 
		"PA", "Set Link Params", true ) );

	cmd = new ActionCommand( PolyScopeAgent2::GotoPeak, 
		"GS", "Goto Spins", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::GotoPeak, 
		"GY", "Goto Spins", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::GotoPlPeak, 
		"GP", "Goto Peak", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( HomoScope2::GotoResidue, 
		"GR", "Goto Residue", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::ViewLabels, 
		"LL", "Label Format", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::ViewPlLabels, 
		"LF", "Label Format", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cl->registerCommand( new ActionCommand( PolyScopeAgent2::NextSpec2D, 
		"NS", "Next Spectrum", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::PrevSpec2D, 
		"PS", "Prev. Spectrum", true ) );

	cmd = new ActionCommand( PolyScopeAgent2::LabelHori, 
		"LH", "Label Horizontal Spin", false );
	cmd->registerParameter( Any::CStr, false ); // Label oder leer
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::LabelVerti, 
		"LV", "Label Vertical Spin", false );
	cmd->registerParameter( Any::CStr, false ); // Label oder leer
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::AutoGain, 
		"AG", "Set Auto Contour Gain", false );
	cmd->registerParameter( Any::Float, true ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( Root::Action::ExecuteLine, "LUA", "Lua", false );
	cmd->registerParameter( Any::Memo, true ); 
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

}

//////////////////////////////////////////////////////////////////////

static QColor g_frameClr = Qt::lightGray;


HomoScope2::HomoScope2(Root::Agent * a, Spectrum* spec, Project* pro ):
	PolyScope2( a )
{
	d_agent = new PolyScopeAgent2( this, spec, pro, true );
	
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

HomoScope2::~HomoScope2()
{
	d_widget->setBody( 0 );
}

void HomoScope2::updateCaption()
{
	QString str;
	str.sprintf( "HomoScope2 - %s", d_agent->getSpec()->getName() );
	getQt()->setCaption( str );
}

void HomoScope2::buildMenus()
{
	Gui::Menu* menuFile = new Gui::Menu( menuBar() );
    Gui::Menu::item( menuFile, this, Root::Action::FileSave, Qt::CTRL+Qt::Key_S );
	Gui::Menu::item( menuFile, d_agent, "Print Preview...", 
		PolyScopeAgent2::CreateReport, false, Qt::CTRL+Qt::Key_P );
	Gui::Menu* sub = new Gui::Menu( menuFile );
	Gui::Menu::item( sub, this, "&Peaklist...", ExportPeaklist, false );
	Gui::Menu::item( sub, this, "Peaks to &MonoScope...", ToMonoScope2D, false );
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
	/*
	Gui::Menu::item( menuView, d_agent, "Forward", 
		PolyScopeAgent2::Forward, false );
	*/
	Gui::Menu::item( menuView, d_agent, "Fit Window", 
		PolyScopeAgent2::FitWindow, false, Qt::Key_Home );
	Gui::Menu::item( menuView, d_agent, "Goto Point...", 
		PolyScopeAgent2::GotoPoint, false );
	Gui::Menu::item( menuView, d_agent, "Goto System...", 
		PolyScopeAgent2::GotoSystem, false, Qt::CTRL+Qt::Key_G );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Show Contour", 
		PolyScopeAgent2::ShowContour, true );
	Gui::Menu::item( menuView, d_agent, "Show Intensity", 
		PolyScopeAgent2::ShowIntensity, true );
	Gui::Menu::item( menuView, d_agent, "Auto Contour Level", 
		PolyScopeAgent2::AutoContour, true );
	Gui::Menu::item( menuView, d_agent, "Set Contour Parameters...", 
		PolyScopeAgent2::ContourParams, false );
	Gui::Menu::item( menuView, d_agent, "Show Folded", 
		PolyScopeAgent2::ShowFolded, true );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Set Resolution...", 
		PolyScopeAgent2::SetResolution, false );
	Gui::Menu::item( menuView, d_agent, "Show Low Resolution", 
		PolyScopeAgent2::ShowLowRes, true );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Resolve Projected Spins", 
		PolyScopeAgent2::ShowWithOff, true );
	Gui::Menu::item( menuView, d_agent, "Show Spin Links", 
		PolyScopeAgent2::ShowLinks, true );
	Gui::Menu::item( menuView, d_agent, "Show Infered Peaks", 
		PolyScopeAgent2::ShowInfered, true );
	Gui::Menu::item( menuView, d_agent, "Show Unlabeled Peaks", 
		PolyScopeAgent2::ShowUnlabeled, true );
	Gui::Menu::item( menuView, d_agent, "Show Peaks with unknown Labels", 
		PolyScopeAgent2::ShowUnknown, true );
	Gui::Menu::item( menuView, d_agent, "Do Pathway Simulation", 
		PolyScopeAgent2::ForceCross, true );
	Gui::Menu::item( menuView, d_agent, "Show Hidden Links", 
		PolyScopeAgent2::ShowAllPeaks, true );
	Gui::Menu::item( menuView, d_agent, "Use Link Color Codes", 
		PolyScopeAgent2::UseLinkColors, true );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Sync to Global Zoom", 
		PolyScopeAgent2::RangeSync, true );
	Gui::Menu::item( menuView, d_agent, "Sync to Global Cursor", 
		PolyScopeAgent2::CursorSync, true );
	Gui::Menu::item( menuView, d_agent, "Center to Peak", 
		PolyScopeAgent2::AutoCenter, true );
	Gui::Menu::item( menuView, d_agent, "Show List", ShowList, true );

	sub = new Gui::Menu( menuBar() );
	menuView->insertItem( "Show Labels", sub );
	short i;
	for( i = SpinPointView::None; i < SpinPointView::End; i++ )
	{
		Gui::Menu::item( sub, d_agent, SpinPointView::menuText[ i ], 
			PolyScopeAgent2::ViewLabels, true )->addParam( i );
	}

	menuBar()->insertItem( "&View", menuView );

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
	menuBar()->insertItem( "&Overlay", menuOverlay );

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
	menuPicking->insertItem( "&Edit Attributes", menuAtts );

	menuPicking->insertSeparator();
	Gui::Menu::item( menuPicking, d_agent, "Hold Reference", 
		PolyScopeAgent2::HoldReference, false );
	Gui::Menu::item( menuPicking, d_agent, "Auto Hold", 
		PolyScopeAgent2::AutoHold, true );
	menuPicking->insertSeparator();
	Gui::Menu::item( menuPicking, this, "Set Verti. Tolerance...", 
		HomoScope2::SetTolVerti, false );
	Gui::Menu::item( menuPicking, this, "Set Hori. Tolerance...", 
		HomoScope2::SetTolHori, false );
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
	menuBar()->insertItem( "&Picking", menuPicking );

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
	menuBar()->insertItem( "&Assignment", menuAssig );

	Gui::Menu* menuSpec = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuSpec, d_agent, "&Calibrate", 
		PolyScopeAgent2::SpecCalibrate, false );
	menuSpec->insertSeparator();
	menuSpec->insertItem( "Select Spectrum", d_agent->getPopSpec2D() );
	menuBar()->insertItem( "&Spectrum", menuSpec );

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
	menuBar()->insertItem( "&Ruler", menuRuler );

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

void HomoScope2::buildViews( bool reuse )
{
	d_focus->setBody( 0 );

	Splitter* inner = new Splitter( 2, 2 );
	inner->setBarWidth( 80 );

	d_focus->clear();
	d_focus->setCircle( !d_showList );

	Splitter* outer = new Splitter( 1, (d_showList)?2:1 );
	outer->setBarWidth( 80 );
	outer->setPane( inner, 0, 0 );

	inner->setPane( d_agent->getPlane().d_viewer, 0, 1 );

	inner->setPane( d_agent->getSlice( DimX ).d_viewer, 1, 1 );

	inner->setPane( d_agent->getSlice( DimY ).d_viewer, 0, 0 );

	if( d_showList )
	{
		d_list = new StripListGadget( d_widget->getViewport(), 
			d_agent->getProject(), this, 0, true );
		outer->setPane( d_list, 0, 1 );
		Gui::Menu* pop = new Gui::Menu( d_list->getListView()->getImp(), true );
		Gui::Menu::item( pop, this, "Goto...", Goto, false );
		Gui::Menu::item( pop, d_list, "Find Spin...", StripListGadget::FindSpin, false );
		Gui::Menu::item( pop, d_list, "Find Link Partner...", StripListGadget::GotoOther, false );
		pop->addSeparator();
		Gui::Menu::item( pop, d_list, "Run Automatic Strip Matcher", StripListGadget::RunStripper, true );
		Gui::Menu::item( pop, d_list, "Strict Strip Matching", StripListGadget::StrictStripMatch, true );
		Gui::Menu::item( pop, d_list, "Set Spin Tolerance...", StripListGadget::SetSpinTol, false );
		pop->addSeparator();
		Gui::Menu::item( pop, d_list, "Create System", StripListGadget::CreateSystem, false );
		Gui::Menu::item( pop, d_list, "Eat System...", StripListGadget::EatSystem, false );
		Gui::Menu::item( pop, d_list, "Set Assig. Candidates...", StripListGadget::SetCandidates, false );
		Gui::Menu::item( pop, d_list, "Set System Type...", StripListGadget::SetSysType, false );
		Gui::Menu::item( pop, d_list, "Show Alignment...", StripListGadget::ShowAlignment, false );
		pop->addSeparator();
		Gui::Menu::item( pop, d_list, "Link This", StripListGadget::LinkThis, false );
		Gui::Menu::item( pop, d_list, "Unlink Successor", StripListGadget::UnlinkSucc, false );
		Gui::Menu::item( pop, d_list, "Unlink Predecessor", StripListGadget::UnlinkPred, false );
		pop->addSeparator();
		Gui::Menu::item( pop, d_list, "Create Spin...", StripListGadget::CreateSpin, false );
		Gui::Menu::item( pop, d_list, "Move Spin...", StripListGadget::MoveSpin, false );
		Gui::Menu::item( pop, d_list, "Label Spin...", StripListGadget::LabelSpin, false );
		Gui::Menu::item( pop, d_list, "Force Label...", StripListGadget::ForceLabel, false );
		Gui::Menu::item( pop, d_list, "Accept Label", StripListGadget::AcceptLabel, false );
		pop->addSeparator();
		Gui::Menu::item( pop, d_list, "Assign to...", StripListGadget::Assign, false );
		Gui::Menu::item( pop, d_list, "Unassign", StripListGadget::Unassign, false );
		Gui::Menu::item( pop, d_list, "Delete", StripListGadget::Delete, false );
		Gui::Menu::item( pop, d_list, "Edit Attributes...", StripListGadget::EditAtts, false );
		pop->addSeparator();
		Gui::Menu::item( pop, d_list, "Show Spin Links", StripListGadget::ShowLinks, true );
		Gui::Menu::item( pop, d_list, "Create Link...", StripListGadget::CreateLink, false );
		Gui::Menu::item( pop, d_list, "Set Link Params...", StripListGadget::LinkParams, false );
		pop->addSeparator();
		Gui::Menu::item( pop, d_list, "Open All", StripListGadget::OpenAll, false );
		Gui::Menu::item( pop, d_list, "Close All", StripListGadget::CloseAll, false );
		d_list->setSpec( d_agent->getSpec() );
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

	d_focus->setBody( outer );

	inner->setRowPos( 1,  17 * d_widget->height() );
	inner->setColumnPos( 1,  3 * d_widget->width() );
	outer->setColumnPos( 1,  15 * d_widget->width() );
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

bool HomoScope2::askToCloseWindow() const
{
	return d_agent->askToClosePeaklist();
}

void HomoScope2::handle(Root::Message& m)
{
	BEGIN_HANDLER();
	MESSAGE( PolyScopeAgent2::SpecChanged, e, m )
	{
		if( d_list && d_agent )
		{
			d_list->setSpec( d_agent->getSpec2D() );
		}
		if( d_ovCtrl && !e->d_threeD )
		{
			d_ovCtrl->getModel()->setSpectrum( 
				new SpecProjector( d_agent->getSpec(), DimX, DimY ) );
			d_ov->redraw();
		}
		m.consume();
	}
	MESSAGE( Root::Action, a, m )
	{
		if( !EXECUTE_ACTION( HomoScope2, *a ) )
			PolyScope2::handle( m );
	}HANDLE_ELSE()
		PolyScope2::handle( m );
	END_HANDLER();
}

void HomoScope2::handleShowList(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_showList );

	d_showList = !d_showList;
	Lexi::Viewport::pushHourglass();
	buildViews( true );
	Lexi::Viewport::popCursor();
}

