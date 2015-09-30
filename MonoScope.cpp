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

#include "MonoScope.h"
// Qt
#include <qtextstream.h> 
#include <qinputdialog.h> 
#include <qmessagebox.h>
#include <QFileDialog>
#include <qfileinfo.h> 
#include <qdir.h> 
#include <qmenubar.h>
#include <qcolordialog.h>

//* Root
#include <Root/ActionHandler.h>
#include <Root/Command.h>
#include <Root/Application.h>
#include <Root/UpstreamFilter.h>
#include <Root/MessageLog.h>
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
#include <Spec/PeakListPeer.h>
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
#include <SpecView/PeakPlaneView.h>

#include <Dlg.h>
#include <SpecView/SpecBatchList.h>
#include <SpectrumListView.h>
#include <SpecView/DynValueEditor.h>
#include <SpecView/ObjectListView.h>

using namespace Spec;

//////////////////////////////////////////////////////////////////////

void createMonoScope( Root::Agent* a, Spec::Spectrum* s, 
					 Spec::Project* p, PeakList* l, const Rotation& r )
{
	new MonoScope( a, s, p, l, r );
}

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr MonoScope::ShowSlices = "ShowSlices";
Root::Action::CmdStr MonoScope::ShowList = "ShowList";
Root::Action::CmdStr MonoScope::NewPeaklist = "NewPeaklist";
Root::Action::CmdStr MonoScope::ImportPeaklist = "ImportPeaklist";
Root::Action::CmdStr MonoScope::ExportPeaklist = "ExportPeaklist";
Root::Action::CmdStr MonoScope::OpenPeaklist = "OpenPeaklist";
Root::Action::CmdStr MonoScope::SavePeaklist = "SavePeaklist";
Root::Action::CmdStr MonoScope::GotoPeak = "GotoPeak";
Root::Action::CmdStr MonoScope::ExportIntegTable = "ExportIntegTable";
Root::Action::CmdStr MonoScope::NextSpec = "NextSpec";
Root::Action::CmdStr MonoScope::PrevSpec = "PrevSpec";
Root::Action::CmdStr MonoScope::GotoSpec = "GotoSpec";
Root::Action::CmdStr MonoScope::SetupBatch = "SetupBatch";
Root::Action::CmdStr MonoScope::AdaptHome = "AdaptHome";
Root::Action::CmdStr MonoScope::ShowAssig = "ShowAssig";
Root::Action::CmdStr MonoScope::ImportAlias = "ImportAlias";
Root::Action::CmdStr MonoScope::ImportLinks = "ImportLinks";
Root::Action::CmdStr MonoScope::EditPeak = "EditPeak";
Root::Action::CmdStr MonoScope::ShowPeak = "ShowPeak";
Root::Action::CmdStr MonoScope::CheckPeakDoubles = "CheckPeakDoubles";
Root::Action::CmdStr MonoScope::EditAtts = "EditAtts";
Root::Action::CmdStr MonoScope::SetColor = "SetColor";
Root::Action::CmdStr MonoScope::NextList = "NextList";
Root::Action::CmdStr MonoScope::PrevList = "PrevList";
Root::Action::CmdStr MonoScope::ShowTable = "ShowTable";
Root::Action::CmdStr MonoScope::SetRgbColor = "SetRgbColor";
Root::Action::CmdStr MonoScope::ExportCaraSpec = "ExportCaraSpec";
Root::Action::CmdStr MonoScope::ExportEasySpec = "ExportEasySpec";
Root::Action::CmdStr MonoScope::SyncPeak = "SyncPeak";
Root::Action::CmdStr MonoScope::SaveColors = "SaveColors";
Root::Action::CmdStr MonoScope::SetLabel = "SetLabel";
Root::Action::CmdStr MonoScope::EditAssig = "EditAssig";
Root::Action::CmdStr MonoScope::AddAssig = "AddAssig";
Root::Action::CmdStr MonoScope::RemoveAssig = "RemoveAssig";
Root::Action::CmdStr MonoScope::ExportTable = "ExportTable";

ACTION_SLOTS_BEGIN( MonoScope )
    { MonoScope::ExportTable, &MonoScope::handleExportTable },
    { MonoScope::EditAssig, &MonoScope::handleEditAssig },
    { MonoScope::AddAssig, &MonoScope::handleAddAssig },
    { MonoScope::RemoveAssig, &MonoScope::handleRemoveAssig },
    { MonoScope::SetLabel, &MonoScope::handleSetLabel },
    { MonoScope::SaveColors, &MonoScope::handleSaveColors },
    { MonoScope::SyncPeak, &MonoScope::handleSyncPeak },
    { MonoScope::ExportEasySpec, &MonoScope::handleExportEasySpec },
    { MonoScope::ExportCaraSpec, &MonoScope::handleExportCaraSpec },
    { MonoScope::SetRgbColor, &MonoScope::handleSetRgbColor },
    { MonoScope::ShowTable, &MonoScope::handleShowTable },
    { MonoScope::PrevList, &MonoScope::handlePrevList },
    { MonoScope::NextList, &MonoScope::handleNextList },
    { MonoScopeAgent::SelectPeaks, &MonoScope::handleSelectPeaks },
    { MonoScope::SetColor, &MonoScope::handleSetColor },
    { MonoScope::EditAtts, &MonoScope::handleEditAtts },
    { MonoScope::CheckPeakDoubles, &MonoScope::handleCheckPeakDoubles },
    { MonoScope::ShowPeak, &MonoScope::handleShowPeak },
    { MonoScope::EditPeak, &MonoScope::handleEditPeak },
    { MonoScope::ImportAlias, &MonoScope::handleImportAlias },
    { MonoScope::ImportLinks, &MonoScope::handleImportLinks },
    { MonoScope::ShowAssig, &MonoScope::handleShowAssig },
    { MonoScope::AdaptHome, &MonoScope::handleAdaptHome },
    { MonoScope::SetupBatch, &MonoScope::handleSetupBatch },
    { MonoScope::PrevSpec, &MonoScope::handlePrevSpec },
    { MonoScope::NextSpec, &MonoScope::handleNextSpec },
    { MonoScope::GotoSpec, &MonoScope::handleGotoSpec },
    { MonoScope::ExportIntegTable, &MonoScope::handleExportIntegTable },
    { MonoScopeAgent::SelectSpec, &MonoScope::handleSelectSpec },
    { MonoScope::GotoPeak, &MonoScope::handleGotoPeak },
    { MonoScope::SavePeaklist, &MonoScope::handleSavePeaklist },
    { MonoScope::OpenPeaklist, &MonoScope::handleOpenPeaklist },
    { MonoScope::ShowList, &MonoScope::handleShowList },
    { MonoScope::ExportPeaklist, &MonoScope::handleExportPeaklist },
    { MonoScope::ImportPeaklist, &MonoScope::handleImportPeaklist },
    { MonoScope::NewPeaklist, &MonoScope::handleNewPeaklist },
    { MonoScope::ShowSlices, &MonoScope::handleShowSlices },
ACTION_SLOTS_END( MonoScope )

//////////////////////////////////////////////////////////////////////

static void buildCommands( CommandLine* cl )
{
	cl->registerCommand( new ActionCommand( MonoScopeAgent::AdjustIntensity, "CW", "Adjust Intensity", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::DeleteAliasPeaks, "UA", "Un-Alias Peaks", true ) );
	cl->registerCommand( new ActionCommand( MonoScope::SyncPeak, "SP", "Sync. Peak", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::ShowFolded, "SF", "Show Folded", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::AutoContour, "AC", "Auto Contour Level", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::Backward, "BB", "Backward", true ) );
	cl->registerCommand( new ActionCommand( MonoScope::ShowList, "SL", "Show List", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::CursorSync, "GC", "Sync To Global Cursor", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::FitWindowX, "WX", "Fit X To Window", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::FitWindowY, "WY", "Fit Y To Window", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::FitWindow, "WW", "Fit To Window", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::AutoCenter, "CC", "Center To Peak", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::PickPeak, "PP", "Pick Peak", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::MovePeak, "MP", "Move Peak", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::MovePeakAlias, "MA", "Move Peak Alias", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::DeletePeaks, "DP", "Delete Peaks", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::Goto, "GT", "Goto", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::ShowContour, "SC", "Show Contour", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::ShowIntensity, "SI", "Show Intensity", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::ContourParams, "CP", "Contour Params", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::Backward, "RZ", "Backward", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::ShowLowRes, "LO", "Low Resolution", true ) );
	cl->registerCommand( new ActionCommand( Root::Action::EditUndo, "ZZ", "Undo", true ) );
	cl->registerCommand( new ActionCommand( Root::Action::EditRedo, "YY", "Redo", true ) );
	cl->registerCommand( new ActionCommand( MonoScope::NextSpec, "NS", "Next Spectrum", true ) );
	cl->registerCommand( new ActionCommand( MonoScope::PrevSpec, "PS", "Prev. Spectrum", true ) );
	cl->registerCommand( new ActionCommand( MonoScope::NextList, "NL", "Next Peaklist", true ) );
	cl->registerCommand( new ActionCommand( MonoScope::PrevList, "PL", "Prev. Peaklist", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::PeakCurve, "PC", "Show Peak Curve", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::PeakCalibrate, "CL", "Calibrate Peaklist", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::ForwardPlane, 
		"FP", "Forward Plane", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::BackwardPlane, 
		"BP", "Backward Plane", true ) );

	ActionCommand* cmd = new ActionCommand( MonoScope::GotoPeak, 
		"GP", "Goto Peak", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( MonoScope::GotoSpec, 
		"GS", "Goto Spectrum", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder Position
	cl->registerCommand( cmd );

	cmd = new ActionCommand( MonoScopeAgent::AutoGain, 
		"AG", "Set Auto Contour Gain", false );
	cmd->registerParameter( Any::Float, true ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( Root::Action::ExecuteLine, "LUA", "Lua", false );
	cmd->registerParameter( Any::Memo, true ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( MonoScopeAgent::SetDepth, 
		"SD", "Set Peak Depth", false );
	cmd->registerParameter( Any::Float, false ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( MonoScopeAgent::OverlayCount, 
		"OC", "Overlay Count", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder Count
	cl->registerCommand( cmd );

	cmd = new ActionCommand( MonoScopeAgent::ActiveOverlay, 
		"AO", "Active Overlay", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( MonoScopeAgent::OverlaySpec, 
		"YS", "Overlay Spectrum", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cl->registerCommand( new ActionCommand( MonoScopeAgent::SetPosColor, "PC", "Set Positive Color", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::SetNegColor, "NC", "Set Negative Color", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::ComposeLayers, "CL", "Compose Layers", true ) );

	cmd = new ActionCommand( MonoScopeAgent::AddLayer, 
		"AL", "Add Overlay Layer", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( MonoScopeAgent::CntThreshold, 
		"CT", "Contour Threshold", false );
	cmd->registerParameter( Any::Float, true ); 
	cmd->registerParameter( Any::CStr, false ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( MonoScopeAgent::CntFactor, 
		"CF", "Contour Factor", false );
	cmd->registerParameter( Any::Float, true ); 
	cmd->registerParameter( Any::CStr, false ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( MonoScopeAgent::ViewLabels, 
		"LF", "Label Format", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( MonoScopeAgent::CntOption, 
		"CO", "Contour Options", false );
	cmd->registerParameter( Any::CStr, false ); // Leer oder + oder -
	cmd->registerParameter( Any::CStr, false ); 
	cl->registerCommand( cmd );
}

//////////////////////////////////////////////////////////////////////

static QColor g_frameClr = Qt::lightGray;

MonoScope::MonoScope(Root::Agent * a, Spectrum* s, Project* pro, 
					 PeakList* pl, const Rotation& rot ):
	Lexi::MainWindow( a, true, true ), d_showList( false ), d_ovCtrl( 0 ),
		d_list( 0 )
{
	assert( s );
	assert( s->getDimCount() > 1 );
	d_agent = new MonoScopeAgent( this, s, pro, rot );
	
	if( pl )
	{
		d_showList = true;
		d_agent->setAutoCenter( true );
	}
	init();

	// RISK: immer ab initio eine leere Peakliste anbieten, die auf Spec passt.
	if( pl )
		d_agent->setPeaklist( pl, true );
	else
		d_agent->setPeaklist( new PeakList( s ) ); 
	if( d_showList )
		d_list->setPeakList( d_agent->getPeaklist(), d_agent->getMainSpec() );
}

void MonoScope::init()
{
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
	buildViews( true );

}

MonoScope::~MonoScope()
{
}

void MonoScope::updateCaption()
{
	QString str;
	str.sprintf( "MonoScope - %s", d_agent->getMainSpec()->getName() );
	getQt()->setCaption( str );
}

void MonoScope::buildMenus()
{
	Gui::Menu* menuFile = new Gui::Menu( menuBar() );
    Gui::Menu::item( menuFile, this, Root::Action::FileSave, Qt::CTRL+Qt::Key_S );
	Gui::Menu::item( menuFile, d_agent, "Print Preview...", 
		MonoScopeAgent::CreateReport, false, Qt::CTRL+Qt::Key_P );
	menuFile->insertSeparator();
    Gui::Menu::item( menuFile, this, Root::Action::WindowClose, Qt::CTRL+Qt::Key_W );
	menuBar()->insertItem( "&File", menuFile );

	Gui::Menu* menuEdit = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuEdit, this, Root::Action::EditUndo, Qt::CTRL+Qt::Key_Z );
	Gui::Menu::item( menuEdit, this, Root::Action::EditRedo, Qt::CTRL+Qt::Key_Y );
	menuBar()->insertItem( "&Edit", menuEdit );

	Gui::Menu* menuView = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuView, d_agent, "Backward", 
		MonoScopeAgent::Backward, false );
	Gui::Menu::item( menuView, d_agent, "Forward", 
		MonoScopeAgent::Forward, false );
	Gui::Menu::item( menuView, d_agent, "Fit Window", 
		MonoScopeAgent::FitWindow, false, Qt::Key_Home );
	Gui::Menu::item( menuView, d_agent, "Goto...", 
		MonoScopeAgent::Goto, false );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Show Contour", 
		MonoScopeAgent::ShowContour, true );
	Gui::Menu::item( menuView, d_agent, "Show Intensity", 
		MonoScopeAgent::ShowIntensity, true );
	Gui::Menu::item( menuView, d_agent, "Auto Contour Level", 
		MonoScopeAgent::AutoContour, true );
	Gui::Menu::item( menuView, d_agent, "Set Contour Parameters...", 
		MonoScopeAgent::ContourParams, false );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Set Resolution...", 
		MonoScopeAgent::SetResolution, false );
	Gui::Menu::item( menuView, d_agent, "Show Low Resolution", 
		MonoScopeAgent::ShowLowRes, true );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Forward Plane", 
		MonoScopeAgent::ForwardPlane, false, Qt::SHIFT+Qt::Key_Next  );
	Gui::Menu::item( menuView, d_agent, "Backward Plane", 
		MonoScopeAgent::BackwardPlane, false, Qt::SHIFT+Qt::Key_Prior );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Show Slices", ShowSlices, true );
	Gui::Menu::item( menuView, d_agent, "Show List", ShowList, true );
	Gui::Menu::item( menuView, d_agent, "Show Folded", 
		MonoScopeAgent::ShowFolded, true );
	Gui::Menu::item( menuView, d_agent, "Detect Maximum Amplitudes...", 
		MonoScopeAgent::CalcMaximum, true, Qt::CTRL+Qt::Key_T );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Center to Peak", 
		MonoScopeAgent::AutoCenter, true );
	Gui::Menu* sub = new Gui::Menu( menuBar() );
	menuView->insertItem( "Show Labels", sub );
	short i;
	for( i = PeakPlaneView::NONE; i < PeakPlaneView::END; i++ )
	{
		Gui::Menu::item( sub, d_agent, PeakPlaneView::menuText[ i ], 
			MonoScopeAgent::ViewLabels, true )->addParam( i );
	}
	Gui::Menu::item( menuView, d_agent, "Sync to Global Zoom", 
		MonoScopeAgent::RangeSync, true );
	Gui::Menu::item( menuView, d_agent, "Sync to Global Cursor", 
		MonoScopeAgent::CursorSync, true );
	Gui::Menu::item( menuView, d_agent, "Sync to Depth", 
		MonoScopeAgent::SyncDepth, true );
	menuBar()->insertItem( "&View", menuView );

	Gui::Menu* menuOverlay = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuOverlay, d_agent, "Set Layer &Count...", 
		MonoScopeAgent::OverlayCount, false );
	Gui::Menu::item( menuOverlay, d_agent, "A&dd Overlay Layer...", 
		MonoScopeAgent::AddLayer, false );
	Gui::Menu::item( menuOverlay, d_agent, "Compose &Layers...", 
		MonoScopeAgent::ComposeLayers, false );
	Gui::Menu::item( menuOverlay, d_agent, "Set &Active Layer...", 
		MonoScopeAgent::ActiveOverlay, false );
	Gui::Menu::item( menuOverlay, d_agent, "Select &Spectrum...", 
		MonoScopeAgent::OverlaySpec, false );
	Gui::Menu::item( menuOverlay, d_agent, "Set &Positive Color...", 
		MonoScopeAgent::SetPosColor, false );
	Gui::Menu::item( menuOverlay, d_agent, "Set &Negative Color...", 
		MonoScopeAgent::SetNegColor, false );
	Gui::Menu::item( menuOverlay, d_agent, "&Make Colors Persistent", 
		MonoScope::SaveColors, false );
	menuBar()->insertItem( "&Overlay", menuOverlay );

	Gui::Menu* menuSpec = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuSpec, d_agent, "&Map to Type...", 
		MonoScopeAgent::SpecRotate, false );
	Gui::Menu::item( menuSpec, d_agent, "&Calibrate", 
		MonoScopeAgent::SpecCalibrate, false );
	Gui::Menu::item( menuSpec, d_agent, "Export CARA Spectrum...", ExportCaraSpec, false );
	Gui::Menu::item( menuSpec, d_agent, "Export EASY Spectrum...", ExportEasySpec, false );
	menuSpec->insertSeparator();
	menuSpec->insertItem( "Select Spectrum", d_agent->getPopSpec() );
	Gui::Menu::item( menuSpec, d_agent, "&Next Spectrum", 
		MonoScope::NextSpec, false, Qt::CTRL+Qt::Key_2 );
	Gui::Menu::item( menuSpec, d_agent, "&Previous Spectrum", 
		MonoScope::PrevSpec, false, Qt::CTRL+Qt::Key_1 );
	Gui::Menu::item( menuSpec, d_agent, "&Setup Batch List...", 
		MonoScope::SetupBatch, false );
	menuBar()->insertItem( "&Spectrum", menuSpec );

	Gui::Menu* menuPeaks = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuPeaks, this, "&New Peaklist", NewPeaklist, false );
	Gui::Menu::item( menuPeaks, this, "&Open Peaklist...", OpenPeaklist, false );
	menuPeaks->insertItem( "Select Peaklist", d_agent->getPopHisto() );
	Gui::Menu::item( menuPeaks, this, "&Add to Repository...", SavePeaklist, false );
	menuPeaks->insertSeparator();
	Gui::Menu::item( menuPeaks, this, "&Import Peaklist...", ImportPeaklist, false );
	Gui::Menu::item( menuPeaks, this, "&Export Peaklist...", ExportPeaklist, false );
	Gui::Menu::item( menuPeaks, this, "Export Integration &Table...", ExportIntegTable, false );
	menuPeaks->insertSeparator();
	Gui::Menu::item( menuPeaks, d_agent, "Rotate Peaklist...", 
		MonoScopeAgent::PeakRotate, false );
	Gui::Menu::item( menuPeaks, d_agent, "Map to Spectrum...", 
		MonoScopeAgent::MapPeakList, false );
	Gui::Menu::item( menuPeaks, d_agent, "Calibrate Peaklist...", 
		MonoScopeAgent::PeakCalibrate, false ); 
	Gui::Menu::item( menuPeaks, d_agent, "Report degenerate peaks...", 
		MonoScope::CheckPeakDoubles, false ); 
	Gui::Menu::item( menuPeaks, this, "Set Peaklist Owner...", AdaptHome, false ); 
	menuPeaks->insertSeparator();
	Gui::Menu::item( menuPeaks, this, "Import Alias Shifts...", ImportAlias, false ); 
	Gui::Menu::item( menuPeaks, this, "Import Spin Links...", ImportLinks, false ); 
	menuPeaks->insertSeparator();
	Gui::Menu::item( menuPeaks, d_agent, "&Pick Peak", 
		MonoScopeAgent::PickPeak, false );
	Gui::Menu::item( menuPeaks, d_agent, "&Move Peak", 
		MonoScopeAgent::MovePeak, false );
	Gui::Menu::item( menuPeaks, d_agent, "&Move Peak Alias", 
		MonoScopeAgent::MovePeakAlias, false );
	Gui::Menu::item( menuPeaks, d_agent, "&Force Label", 
		MonoScopeAgent::ForceLabel, false );
	Gui::Menu::item( menuPeaks, d_agent, "Un-Alias Peaks", 
		MonoScopeAgent::DeleteAliasPeaks, false );
	Gui::Menu::item( menuPeaks, d_agent, "&Delete Peaks", 
		MonoScopeAgent::DeletePeaks, false );
	Gui::Menu::item( menuPeaks, d_agent, "&Set Color...", 
		MonoScopeAgent::SetColor, false );
	Gui::Menu::item( menuPeaks, d_agent, "&Edit Attributes...", 
		MonoScopeAgent::EditAtts, false, Qt::CTRL+Qt::Key_Return );
	menuBar()->insertItem( "&Peaks", menuPeaks );

	Gui::Menu* menuSlices = new Gui::Menu( menuBar() );
 	Gui::Menu::item( menuSlices, d_agent, "Set Bounds...", 
		MonoScopeAgent::SetSliceMinMax, false );
 	Gui::Menu::item( menuSlices, d_agent, "Pick Bounds", 
		MonoScopeAgent::PickBounds, false );
 	Gui::Menu::item( menuSlices, d_agent, "Pick Bounds Sym.", 
		MonoScopeAgent::PickBoundsSym, false );
 	Gui::Menu::item( menuSlices, d_agent, "Auto Scale", 
		MonoScopeAgent::SliceAutoScale, true, Qt::CTRL+Qt::Key_A );
	menuBar()->insertItem( "&Slices", menuSlices );

	Gui::Menu* menuInt = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuInt, d_agent, "&Tune Peak Model...", 
		MonoScopeAgent::TunePeakModel, false, Qt::CTRL+Qt::Key_M  );
	Gui::Menu::item( menuInt, d_agent, "&Update All Amplitudes", 
		MonoScopeAgent::UpdateAllAmps, false );
	Gui::Menu::item( menuInt, d_agent, "&Integrate All", 
		MonoScopeAgent::IntegrateAll, false, Qt::CTRL+Qt::Key_I );
	Gui::Menu::item( menuInt, d_agent, "&Integrate Batch List", 
		MonoScopeAgent::BatchIntegrate, false );
	Gui::Menu::item( menuInt, d_agent, "&Show Peak Curve...", 
		MonoScopeAgent::PeakCurve, false, Qt::CTRL+Qt::Key_K );
	menuInt->insertSeparator();
	Gui::Menu::item( menuInt, d_agent, "Show Backcalculation", 
		MonoScopeAgent::ShowBackCalc, true, Qt::CTRL+Qt::Key_B );
	Gui::Menu::item( menuInt, d_agent, "Exact Calculation", 
		MonoScopeAgent::ExactBackCalc, true, Qt::CTRL+Qt::Key_E );
	Gui::Menu::item( menuInt, d_agent, "Show Difference", 
		MonoScopeAgent::ShowDiff, true, Qt::CTRL+Qt::Key_D );
	menuInt->insertSeparator();
	Gui::Menu::item( menuInt, d_agent, "Show Base Width", 
		MonoScopeAgent::ShowBaseWidth, true, Qt::CTRL+Qt::Key_H );
	Gui::Menu::item( menuInt, d_agent, "Show Peak Model", 
		MonoScopeAgent::ShowModel, true, Qt::CTRL+Qt::Key_L );
	sub = new Gui::Menu( menuBar() );
	menuInt->insertItem( "Integration Method", sub );
	for( i = PeakList::LinEq; i < PeakList::MAX_IntegrationMethod; i++ )
	{
		Gui::Menu::item( sub, d_agent, PeakList::integrationMethodName[ i ], 
			MonoScopeAgent::SetIntMeth, true )->addParam( i );
	}
	menuBar()->insertItem( "&Integrator", menuInt );

	Gui::Menu* menuHelp = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuHelp, this, Root::Action::HelpAbout );
	menuBar()->insertItem( "&?", menuHelp );

}

void MonoScope::buildViews( bool create )
{
	d_focus->setBody( 0 ); // Wichtig im Falle von erneutem Aufruf von buildViews.
	// Ansonsten gibt's Exception, da dann die Viewer mehrere Owner haben.

	if( create )
		d_agent->allocate();

	SpecViewer* plane = 0;
	if( create )
		plane = d_agent->createPlaneViewer();
	else
		plane = d_agent->getPlaneViewer();

	Splitter* split1 = new Splitter( 
		(d_agent->showSlices())?2:1, 
		(d_agent->showSlices())?2:1 );
	split1->setBarWidth( 80 );
	split1->setPane( plane, 0, 0 );

	d_focus->clear();
	d_focus->setCircle( !d_showList );
	d_focus->append( plane->getController() );

	Splitter* split0 = new Splitter( 1, (d_showList)?2:1 );
	split0->setBarWidth( 80 );
	split0->setPane( split1, 0, 0 );

	if( d_agent->showSlices() )
	{
		const Dimension dim = d_agent->getMainSpec()->getDimCount();
		SpecViewer* g = 0;
		if( create || d_agent->getSliceViewer( DimX ) == 0 )
			g = d_agent->createSliceViewer( DimX, DimX );
		else
			g = d_agent->getSliceViewer( DimX );
		split1->setPane( g, 1, 0 );
		d_focus->append( g->getController() );

		Glyph* box = LayoutKit::hbox();
		split1->setPane( box, 0, 1 );
		for( Dimension d = 1; d < dim; d++ )
		{
			if( create || d_agent->getSliceViewer( d ) == 0 )
				g = d_agent->createSliceViewer( DimY, d );
			else
				g = d_agent->getSliceViewer( d );
			box->append( g );
			d_focus->append( g->getController() );
			if( d < dim - 1 )
				box->append( new Rule( DimY, g_frameClr, 40 ) );
		}
		// Create Overview
		if( dim == 2 )
		{
			if( create || d_ovCtrl == 0 )
			{
				d_ov = new SpecViewer( new ViewAreaMdl( true, true, true, true ) );
				Root::Ref<SpecProjector> pro = new SpecProjector( d_agent->getMainSpec(), 
					DimX, DimY );
				Root::Ref<SpecBufferMdl> mdl = new SpecBufferMdl( d_ov->getViewArea(), 
					pro, false );
				d_ov->getViews()->append( new IntensityView( mdl ) );
				d_ovCtrl = new OverviewCtrl( mdl, plane->getViewArea() );
				d_ov->getHandlers()->append( d_ovCtrl );
				d_ov->getHandlers()->append( new FocusCtrl( d_ov ) );
				// mdl->copy( d_agent->getPlane().d_ol[0].d_buf );
				// mdl->setResolution( 1 );
			}else
				d_ovCtrl->setTarget( plane->getViewArea() );
			split1->setPane( d_ov, 1, 1 );
		}
	}
	if( d_showList )
	{
		d_list = new PeakListGadget( d_widget->getViewport(), 
			d_agent->getPeaklist(), d_agent->getMainSpec(), this, 
			0, true, d_agent->getPro() );
		Gui::Menu* menu = new Gui::Menu();
		Gui::Menu::item( menu,this, "Locate Peak", ShowPeak );
		Gui::Menu::item( menu,this, "Edit Peak...", EditPeak );
		Gui::Menu::item( menu,this, "Set Label...", SetLabel );
		Gui::Menu::item( menu,this, "Set Color...", SetColor );
		Gui::Menu::item( menu,this, "Select Code Color...", SetRgbColor );
		Gui::Menu::item( menu,this, "Report degenerate peaks...", CheckPeakDoubles );
		Gui::Menu::item( menu,this, "Edit Attributes...", EditAtts );
		Gui::Menu::item( menu,this, "Open Object Table...", ShowTable );
		menu->addSeparator();
		Gui::Menu::item( menu,this, "Show Assignments", ShowAssig, true );
		Gui::Menu::item( menu,this, "Edit Assignment...", EditAssig );
		Gui::Menu::item( menu,this, "Add Assignment...", AddAssig );
		Gui::Menu::item( menu,this, "Remove Assignment...", RemoveAssig );
		menu->addSeparator();
		Gui::Menu::item( menu,this, "New...", NewPeaklist );
		Gui::Menu::item( menu,this, "Open...", OpenPeaklist );
		Gui::Menu::item( menu,this, "Save...", SavePeaklist );
		menu->addSeparator();
		Gui::Menu::item( menu,this, "Import Peaklist...", ImportPeaklist );
		Gui::Menu::item( menu,this, "Export Peaklist...", ExportPeaklist );
		Gui::Menu::item( menu,this, "Export Table...", ExportTable );
		menu->addSeparator();
		Gui::Menu::item( menu,d_list, "Add Column...", PeakListGadget::AddColumn );
		Gui::Menu::item( menu,d_list, "Remove Columns", PeakListGadget::RemoveCols );

		d_list->setPopup( new ContextMenu( menu ) );
		split0->setPane( d_list, 0, 1 );
	}else
		d_list = 0;
	d_focus->setBody( split0 );
	if( create )
		d_agent->initOrigin();

	if( d_agent->showSlices() )
		split1->setRowPos( 1,  15 * d_widget->height() );
	if( d_showList && d_agent->showSlices() )
	{
		split1->setColumnPos( 1,  10 * d_widget->width() );
		split0->setColumnPos( 1,  15 * d_widget->width() );
	}else if( d_showList && !d_agent->showSlices() )
		split0->setColumnPos( 1,  15 * d_widget->width() );
	else if( d_agent->showSlices() && !d_showList )
		split1->setColumnPos( 1,  15 * d_widget->width() );

	if( create && d_agent->getPeaklist() )
	{
		Root::Ref<PeakList> tmp = d_agent->getPeaklist();
		d_agent->setPeaklist( tmp );	// NOTE: da wir sonst zuerst deref und dann ref
	}
	d_widget->reallocate();
	if( create )
	{
		d_agent->fitToView();
		d_agent->setCursor();
	}
	d_widget->setFocusGlyph( plane->getController() );
	if( d_ov )
	{ 
		if( d_agent->getMainSpec()->canDownsize() )
		{
			d_ovCtrl->getModel()->fitToArea();
		}else
		{
			d_ovCtrl->getModel()->copy( d_agent->getPlane().d_ol[0].d_buf );
		}
	}
	d_widget->getViewport()->damageAll();
}

void MonoScope::handleAction(Root::Action& a)
{
	if( !EXECUTE_ACTION( MonoScope, a ) )
		MainWindow::handleAction( a );
}

void MonoScope::handle(Root::Message& m)
{
	BEGIN_HANDLER();
	MESSAGE( PeakListGadget::ActivatePeakMsg, e, m )
	{
		d_agent->setCursor( e->getPeak() );
		d_widget->setFocusGlyph( d_agent->getPlane().d_viewer->getController() );
		m.consume();
	}HANDLE_ELSE()
		MainWindow::handle( m );
	END_HANDLER();
}

bool MonoScope::askToCloseWindow() const
{
	if( !d_agent->isDirty() )
		return true;
	bool done = true;
	MonoScopeAgent::Histo::const_iterator i;
	for( i = d_agent->getHisto().begin(); i != d_agent->getHisto().end(); ++i )
		if( (*i)->isNew() )
			done = false;
	if( done )
		return true;
	if( d_agent->getHisto().size() > 1 )
	{
		return ( QMessageBox::warning( getQt(), "About to close peaklist",
			"There are unsaved peaklists. Do you want to close MonoScope without saving?",
			"&OK", "&Cancel", QString::null, 1, 1 ) == 0 );
	}else
	{
		switch( QMessageBox::warning( getQt(), "About to close peaklist",
				"Do you want to save the peaklist in the repository before closing?",
											  "&Save", "&Don't Save", "&Cancel",
											  0,		// Enter == button 0
											  2 ) )		// Escape == button 2
		{
		case 0:
			return handleSave( d_agent->getPro() == 0 ); // Do it with action.
		case 1:
			return true;	// Do it without action
		default:
			return false;	// Don't do it.
		}
	}
}

bool MonoScope::handleSave(bool xp) const
{
	if( d_agent->getPeaklist() == 0 )
		return false;
	if( xp )
	{
		QString fileName;
		if( d_agent->getPeaklist()->getName().empty() )
			fileName = AppAgent::getCurrentDir();
		else
		{
			fileName = d_agent->getPeaklist()->getName().data();
			fileName += ".peaks";
		}
		fileName = QFileDialog::getSaveFileName( getQt(), "Export Pealist", fileName, "*.peaks" );
		if( fileName.isNull() ) 
			return false;

		QFileInfo info( fileName );

		if( info.extension( false ).upper() != "PEAKS" )
			fileName += ".peaks";
		info.setFile( fileName );
		if( info.exists() )
		{
			if( QMessageBox::warning( getQt(), "Save As",
				"This file already exists. Do you want to overwrite it?",
				"&OK", "&Cancel" ) != 0 )
				return false;
		}
		Root::AppAgent::setCurrentDir( info.dirPath( true ) );
		try
		{
			Root::Ref<PointSet> ps = Factory::createEasyPeakList( 
				d_agent->getPeaklist(), d_agent->getSpec() );
			ps->saveToFile( fileName.toAscii().data() );
		}catch( Root::Exception& e )
		{
			Root::ReportToUser::alert( const_cast<MonoScope*>(this), "Error Exporting Peaklist", e.what() );
		}
	}else
	{
		bool ok;
		QString res = QInputDialog::getText( "Name Peaklist", 
			"Please enter a unique short name:", QLineEdit::Normal, 
			d_agent->getPeaklist()->getName().data(), &ok, getQt() );
		if( !ok )
			return false;
		assert( d_agent->getPro() != 0 );
		if( res.isEmpty() || d_agent->getPro()->findPeakList( res.toLatin1() ) != 0 )
		{
			QMessageBox::critical( getQt(), "Save Peaklist",
					"This peaklist name is already used!", "&Cancel" );
			return false;
		}
		d_agent->getPeaklist()->setName( res.toLatin1() );
        // RISK: kann es vorkommen, dass die PeakList kein home hat?
		d_agent->getPro()->addPeakList( d_agent->getPeaklist() );
		d_agent->getPeaklist()->clearDirty();
	}
	return true;
}

void MonoScope::handleShowSlices(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_agent->showSlices() );

	d_agent->showSlices( !d_agent->showSlices() );
	Lexi::Viewport::pushHourglass();
	buildViews( false );
	Lexi::Viewport::popCursor();
}

void MonoScope::handleNewPeaklist(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	//if( !askToClosePeaklist() )
	//	return;

	d_agent->setPeaklist( new PeakList( d_agent->getMainSpec() ) );
	if( d_showList )
		d_list->setPeakList( d_agent->getPeaklist(), d_agent->getMainSpec() );
}

void MonoScope::handleImportPeaklist(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	//if( !askToClosePeaklist() )
	//	return;
	QString fileName = QFileDialog::getOpenFileName( getQt(), "Import Peaklist", AppAgent::getCurrentDir(),
			"*.peaks" );
	if( fileName.isNull() ) 
		return;

	try
	{
		QFileInfo info( fileName );
		Root::Ref<PointSet> ps = Factory::createEasyPeakList( fileName );
		ColorMap psClr;
		ps->getColors( psClr );
		PeakList* pl = new PeakList( psClr, d_agent->getMainSpec() );
		pl->append( ps ); // pl ist nun eine identische Kopie von ps.
		pl->setName( info.completeBaseName().toLatin1() );

		ColorMap specClr;
		d_agent->getMainSpec()->getColors( specClr );
		if( specClr != psClr )
		{
			// pl passt noch nicht auf spec
			Rotation rot = createRotation( specClr, psClr );
			if( rot.size() == psClr.size() )
			{
				pl->rotateAll( rot ); // eine Rotation ist mglich.
			}
			d_agent->setPeaklist( pl, rot.size() != psClr.size() );
		}else
			// pl passt bereits auf spec
			d_agent->setPeaklist( pl, false );
		if( d_showList )
			d_list->setPeakList( d_agent->getPeaklist(), d_agent->getMainSpec() );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Error Importing Peaklist", e.what() );
	}
}

void MonoScope::handleExportPeaklist(Action & a)
{
	ACTION_ENABLED_IF( a, d_agent->getPeaklist() );

	handleSave( true );
}

void MonoScope::handleShowList(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_showList );

	d_showList = !d_showList;
	Lexi::Viewport::pushHourglass();
	buildViews( false );
	Lexi::Viewport::popCursor();
}

void MonoScope::handleOpenPeaklist(Action & a)
{
	ACTION_ENABLED_IF( a, d_agent->getPro() != 0 );

	PeakList* pl = Dlg::selectPeakList( getQt(), d_agent->getPro() );
	if( pl == 0 )
		return;
	d_agent->setPeaklist( pl, true );
	if( d_showList )
		d_list->setPeakList( d_agent->getPeaklist(), d_agent->getMainSpec() );
}

void MonoScope::handleSavePeaklist(Action & a)
{
	ACTION_ENABLED_IF( a, d_agent->getPeaklist() && 
		d_agent->getPeaklist()->isNew() && d_agent->getPro() != 0 );

	handleSave( false );
}

void MonoScope::handleGotoPeak(Action & a)
{
	ACTION_ENABLED_IF( a, d_agent->getPeaklist() );

	Root::Index id;
	if( a.getParam( 0 ).isNull() )
	{
		bool ok	= FALSE;
		id	= QInputDialog::getInteger( "Goto Peak", 
			"Please	enter peak id:", 
			0, -999999, 999999, 1, &ok, getQt() );
		if( !ok )
			return;
	}else
		id = a.getParam( 0 ).getLong();

	if( d_agent->getPeaklist()->getPeak( id ) == 0 )
	{
		Root::ReportToUser::alert( this, "Goto Peak", "Unknown Peak!" );
		return;
	}
	d_agent->setCursor( id );
}

void MonoScope::handleSelectSpec(Action & a)
{
	Spectrum* spec = dynamic_cast<Spectrum*>( a.getParam( 0 ).getObject() );
	assert( spec );
	ACTION_CHECKED_IF( a, d_agent->getPeaklist() != 0, 
		d_agent->getSpec()->getId() == spec->getId() );
	if( d_agent->getSpec()->getId() == spec->getId() )
		return;
	if( d_agent->setSpec( spec ) && d_showList )
		d_list->setSpec( d_agent->getSpec() );
}

void MonoScope::handleExportIntegTable(Action & a)
{
	ACTION_ENABLED_IF( a, d_agent->getPro() && d_agent->getPeaklist() );

	QString fileName = QFileDialog::getSaveFileName(  getQt(), "Export Integration Table",
                                                      AppAgent::getCurrentDir(), "*.*" );
	if( fileName.isNull() ) 
		return;
	QFileInfo info( fileName );

	if( info.exists() )
	{
		if( QMessageBox::warning( getQt(), "Save As",
			"This file already exists. Do you want to overwrite it?",
			"&OK", "&Cancel" ) != 0 )
			return;
	}
	Root::AppAgent::setCurrentDir( info.dirPath( true ) );
	d_agent->getPro()->saveIntegTable( d_agent->getPeaklist(), fileName );
}

void MonoScope::handleGotoSpec(Action & a)
{
	ACTION_ENABLED_IF( a, d_agent->getPro() && d_agent->getPeaklist() );

	long i = a.getParam( 0 ).getLong();
	const PeakList::SpecList& specs = d_agent->getPeaklist()->getSpecs();

	Spectrum* spec = 0;
	if( i == 0 )
		spec = d_agent->getMainSpec();
	else if( i > 0 && i <= specs.size() )
		spec = d_agent->getPro()->getSpec( specs[ i - 1 ] );

	if( spec && d_agent->setSpec( spec ) && d_showList )
		d_list->setSpec( d_agent->getSpec() );
}

static int _findPos( const PeakList::SpecList& s, Spectrum* spec )
{
	for( int i = 0; i < s.size(); i++ )
		if( s[ i ] == spec->getId() )
			return i;
	return -1;
}

void MonoScope::handleNextSpec(Action & a)
{
	if( d_agent->getPro() == 0 || d_agent->getPeaklist() == 0 )
		return;
	const PeakList::SpecList& specs = d_agent->getPeaklist()->getSpecs();
	int i = _findPos( specs, d_agent->getSpec() );
	i++;
	ACTION_ENABLED_IF( a, i < specs.size() );
	if( i >= specs.size() )
		return; // NOTE: QT-BUG
	Spectrum* spec =  d_agent->getPro()->getSpec( specs[ i ] );

	if( spec && d_agent->setSpec( spec ) && d_showList )
		d_list->setSpec( d_agent->getSpec() );
}

void MonoScope::handlePrevSpec(Action & a)
{
	if( d_agent->getPro() == 0 || d_agent->getPeaklist() == 0 )
		return;
	const PeakList::SpecList& specs = d_agent->getPeaklist()->getSpecs();
	int i = _findPos( specs, d_agent->getSpec() );
	if( i == -1 )
		i = specs.size();
	i--;
	ACTION_ENABLED_IF( a, i >= 0 && i < specs.size() );
	if( i < 0 )
		return; // NOTE: QT-BUG
	Spectrum* spec =  d_agent->getPro()->getSpec( specs[ i ] );

	if( spec && d_agent->setSpec( spec ) && d_showList )
		d_list->setSpec( d_agent->getSpec() );
}

void MonoScope::handleSetupBatch(Action & a)
{
	ACTION_ENABLED_IF( a, d_agent->getPro() && d_agent->getPeaklist() );

	SpecBatchList dlg( getQt(), d_agent->getPeaklist(), d_agent->getPro() );
	if( dlg.doit() )
		d_agent->reloadSpecPop();
}

void MonoScope::handleAdaptHome(Action & a)
{
	PeakList* pl = d_agent->getPeaklist();
	ACTION_ENABLED_IF( a, pl && pl->getHome() != d_agent->getSpec()->getId() );

	if( QMessageBox::warning( getQt(), "Set Peaklist Owner", 
		"Do you really want this spectrum own the current peaklist?", 
		"&OK", "&Cancel", QString::null, 1, 1 ) != 0 )		
		return;	// Cancel
	if( !pl->setHome( d_agent->getSpec() ) )
		QMessageBox::critical( getQt(), "Set Peaklist Owner", 
		"Peaklist not compatible with spectrum!", "&Cancel" );
}

void MonoScope::handleShowAssig(Action & a)
{
	ACTION_CHECKED_IF( a, d_list, d_list && d_list->showAssig() );

	d_list->showAssig( !d_list->showAssig() );
}

void MonoScope::handleImportAlias(Action & a)
{
	ACTION_ENABLED_IF( a, d_agent->getPeaklist() );

	Root::Ref<PointSet> ps;
	try
	{
		ps = Factory::createEasyPeakList( 
			d_agent->getPeaklist(), d_agent->getSpec() );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Error Importing Peaklist", e.what() );
	}
	SpectrumListView::importAlias( getQt(), ps, 
		d_agent->getPro(), d_agent->getSpec() );
}

void MonoScope::handleImportLinks(Action & a)
{
	ACTION_ENABLED_IF( a, d_agent->getPeaklist() );

	Root::Ref<PointSet> ps;
	try
	{
		ps = Factory::createEasyPeakList( 
			d_agent->getPeaklist(), d_agent->getSpec() );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Error Importing Peaklist", e.what() );
	}
	SpectrumListView::importLinks( getQt(), ps, d_agent->getPro(), d_agent->getSpec() );
}

void MonoScope::handleEditPeak(Action & a)
{
	ACTION_ENABLED_IF( a, d_list && d_list->getSelected() );

	Peak* p = d_agent->getPeaklist()->getPeak( d_list->getSelected() );

	PeakPos peak = p->getPos( d_agent->getSpec() );
	PpmPoint pp;
	pp.assign( p->getDimCount() );
	peak.fillPoint( pp );
	if( !Dlg::getPpmPoint( getQt(), pp ) )
		return;
	peak = pp;
	Amplitude val = p->getAmp( d_agent->getSpec() );
	Root::Ref<MovePeakCmd> cmd = new MovePeakCmd( 
		p, peak, val, d_agent->getSpec() );
	cmd->handle( this );
}

void MonoScope::handleShowPeak(Action & a)
{
	ACTION_ENABLED_IF( a, d_list && d_list->getSelected() );

	d_agent->setCursor( d_list->getSelected() );
}

void MonoScope::handleCheckPeakDoubles(Action & a)
{
	ACTION_ENABLED_IF( a, d_agent->getPeaklist() );

	PeakList* pl = d_agent->getPeaklist();
	Spectrum* spec = d_agent->getSpec();
	PeakList::SelectionSet res = pl->findDoubles( spec );

	QString str;
	str.sprintf( "Peaklist %s contains the following groups of degenerate peaks "
		"in spectrum %s (one group per line):\n",
		pl->getName().data(), spec->getName() );
	PeakList::SelectionSet::const_iterator j;
	PeakList::Selection::const_iterator k;
	for( j = res.begin(); j != res.end(); ++j )
	{
		for( k = (*j).begin(); k != (*j).end(); ++k )
		{
			str += QString().setNum( (*k)->getId() );
			str += " ";
		}
		str += "\n";
	}
	if( !res.empty() )
	{
		Root::MessageLog::inst()->warning( "Report Degenerate Peaks", str.toLatin1() );
		str.sprintf( "The peaklist contains %d degenerate peaks. "
			"Check message log for details.", res.size() );
		QMessageBox::information( getQt(), "Report Degenerate Peaks", str.toLatin1(), "&OK" );
	}else
		QMessageBox::information( getQt(), "Report Degenerate Peaks", 
			"The peaklist contains no degenerate peaks for the selected spectrum.", "&OK" );
}

void MonoScope::handleEditAtts(Action & a)
{
	Peak* p = (d_list)?d_list->getPeak():0;
	ACTION_ENABLED_IF( a, d_agent->getPro() && p );

	Peak::Guess* g = d_list->getGuess();
	if( g )
		DynValueEditor::edit( getQt(), d_agent->getPro()->getRepository()
			->findObjectDef( Repository::keyGuess ), g );
	else
		DynValueEditor::edit( getQt(), d_agent->getPro()->getRepository()
			->findObjectDef( Repository::keyPeak ), p );
}

void MonoScope::handleSetColor(Action & a)
{
	ACTION_ENABLED_IF( a, d_list && d_list->getSelected() );

	Peak* p = d_agent->getPeaklist()->getPeak( d_list->getSelected() );

	bool ok;
	int res = QInputDialog::getInteger( "Set Color", 
		"Please enter a valid color code:", 
		p->getColor(), 0, 255, 1, &ok, getQt() );
	if( !ok )
		return;
	d_agent->getPeaklist()->setColor( p, res );	// TODO: undo
}

void MonoScope::handleSelectPeaks(Action & a)
{
	PeakList* pl = dynamic_cast<PeakList*>( a.getParam( 0 ).getObject() );
	ACTION_CHECKED_IF( a, true, d_agent->getPeaklist() == pl );
	if( d_agent->getPeaklist() == pl )
		return;
	d_agent->setPeaklist( pl );
	if( d_showList )
		d_list->setPeakList( pl, d_agent->getMainSpec() );
}

void MonoScope::handlePrevList(Action & a)
{
	const MonoScopeAgent::Histo& h = d_agent->getHisto();
	ACTION_ENABLED_IF( a, h.size() > 1 );
	MonoScopeAgent::Histo::const_iterator i = h.find( d_agent->getPeaklist() );
	if( i == h.begin() )
		i = h.end();
	--i;
	d_agent->setPeaklist( (*i) );
	if( d_showList )
		d_list->setPeakList( (*i), d_agent->getMainSpec() );
}

void MonoScope::handleNextList(Action & a)
{
	const MonoScopeAgent::Histo& h = d_agent->getHisto();
	ACTION_ENABLED_IF( a, h.size() > 1 );
	MonoScopeAgent::Histo::const_iterator i = h.find( d_agent->getPeaklist() );
	i++;
	if( i == h.end() )
		i = h.begin();
	d_agent->setPeaklist( (*i) );
	if( d_showList )
		d_list->setPeakList( (*i), d_agent->getMainSpec() );
}

void MonoScope::handleShowTable(Root::Action & a)
{
	PeakList* pl = d_agent->getPeaklist();
	ACTION_ENABLED_IF( a, pl );

	int i = 0;
	Peak::Guess* g = (d_list)?d_list->getGuess():0;
	if( g )
	{
		Peak* p = d_list->getPeak();
		Peak::GuessMap::const_iterator j;
		ObjectListView::ObjectList o( p->getGuesses().size() );
		for( j = p->getGuesses().begin(); j != p->getGuesses().end(); ++j, ++i )
			o[ i ] = (*j).second;

		ObjectListView::edit( getQt(), d_agent->getPro()->getRepository()
			->findObjectDef( Repository::keyGuess ), o );
	}else
	{
		const PeakList::Peaks& sm = pl->getPeaks();
		PeakList::Peaks::const_iterator p;
		ObjectListView::ObjectList o( sm.size() );
		for( p = sm.begin(); p != sm.end(); ++p, ++i)
		{
			o[ i ] = (*p).second;
		}
		ObjectListView::edit( getQt(), d_agent->getPro()->getRepository()
			->findObjectDef( Repository::keyPeak ), o );
	}
}

void MonoScope::handleSetRgbColor(Action & a)
{
	ACTION_ENABLED_IF( a, d_list && d_list->getSelected() );

	Peak* p = d_agent->getPeaklist()->getPeak( d_list->getSelected() );

	QColor clr = d_agent->getPro()->getRepository()->
		getColors()->getColor( p->getColor() );


	QColor res = QColorDialog::getColor( clr, getQt() );
	if( res.isValid() )
	{
		d_agent->getPro()->getRepository()->
			getColors()->setColor( p->getColor(), res.rgb() );
	}
}

void MonoScope::handleExportCaraSpec(Action & a)
{
	Spectrum* spec = d_agent->getSpec();
	ACTION_ENABLED_IF( a, spec );
	SpectrumListView::saveCaraSpectrum( getQt(), spec );
}

void MonoScope::handleExportEasySpec(Action & a)
{
	Spectrum* spec = d_agent->getSpec();
	ACTION_ENABLED_IF( a, spec );
	SpectrumListView::saveEasySpectrum( getQt(), spec );
}

void MonoScope::handleSyncPeak(Action & a)
{
	if( !d_showList || d_agent->getPlane().d_peaks == 0 ||
		d_agent->getPlane().d_peaks->getSel().size() != 1 )
		return;

	ACTION_ENABLED_IF( a, true );
	d_list->gotoPeak( *d_agent->getPlane().d_peaks->getSel().begin() );
}

void MonoScope::handleSaveColors(Action & a)
{
	ACTION_ENABLED_IF( a, d_agent->getPro() );
	Repository::SlotColors clr( d_agent->getPlane().d_ol.size() );
	for( int i = 0; i < clr.size(); i++ )
	{
		clr[ i ].d_pos = d_agent->getPlane().d_ol[i].d_view->getPosColor();
		clr[ i ].d_neg = d_agent->getPlane().d_ol[i].d_view->getNegColor();
	}
	d_agent->getPro()->getRepository()->setScreenClr( clr );
}

void MonoScope::handleSetLabel(Action & a)
{
	ACTION_ENABLED_IF( a, d_list && d_list->getSelected() );

	Peak* p = d_agent->getPeaklist()->getPeak( d_list->getSelected() );

	bool ok	= FALSE;
	QString res	= QInputDialog::getText( "Force Peak Label", 
		"Please	enter a label:", QLineEdit::Normal, 
		p->getTag().data(), &ok, getQt() );
	if( !ok )
		return;
	d_agent->getPeaklist()->setTag( p, res.toLatin1() );	// TODO: undo
}

void MonoScope::handleEditAssig(Action & a)
{
	Peak* p = (d_list)?d_list->getPeak():0;
	ACTION_ENABLED_IF( a, d_agent->getPro() &&  p );

	Dlg::Assig ass;
	Peak::Guess* g = d_list->getGuess();
	if( g )
	{
		ass = g->getAssig();
		float f = g->getProb();
		if( Dlg::getAssig( getQt(), p->getDimCount(), ass, &f ) )
		{
			d_agent->getPeaklist()->setGuess( p, g->getId(), ass, f );
		}
	}else
	{
		ass = p->getAssig();
		if( Dlg::getAssig( getQt(), p->getDimCount(), ass, 0 ) )
		{
			d_agent->getPeaklist()->setAssig( p, ass );
		}
	}
}

void MonoScope::handleAddAssig(Action & a)
{
	Peak* p = (d_list)?d_list->getPeak():0;
	ACTION_ENABLED_IF( a, d_agent->getPro() && p );
	Dlg::Assig ass;;
	float f = 0.0;
	if( Dlg::getAssig( getQt(), p->getDimCount(), ass, &f, "Add Assignment" ) )
	{
		d_agent->getPeaklist()->addGuess( p, ass, f );
	}
}

void MonoScope::handleRemoveAssig(Action & a)
{
	Peak::Guess* g = (d_list)?d_list->getGuess():0;
	ACTION_ENABLED_IF( a, d_agent->getPro() && g );
	d_agent->getPeaklist()->deleteGuess( d_list->getPeak(), g->getId() );
}

void MonoScope::handleExportTable(Action & a)
{
	ACTION_ENABLED_IF( a, d_agent->getPeaklist() );

	QString fileName;
	if( d_agent->getPeaklist()->getName().empty() )
		fileName = AppAgent::getCurrentDir();
	else
	{
		fileName = d_agent->getPeaklist()->getName().data();
		fileName += ".txt";
	}
	fileName = QFileDialog::getSaveFileName( getQt(), "Export Peak Table", fileName,
			"*.txt" );
	if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );

	if( info.extension( false ).upper() != "TXT" )
		fileName += ".txt";
	info.setFile( fileName );
	if( info.exists() )
	{
		if( QMessageBox::warning( getQt(), "Save As",
			"This file already exists. Do you want to overwrite it?",
			"&OK", "&Cancel" ) != 0 )
			return;
	}
	Root::AppAgent::setCurrentDir( info.dirPath( true ) );
	d_list->saveTable( fileName );
}

