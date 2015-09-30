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

#include "MonoScope2.h"
// Qt
#include <QTextStream> 
#include <QInputDialog> 
#include <QMessageBox>
#include <QFileDialog> 
#include <QFileInfo> 
#include <QDir> 
#include <QMenuBar>
#include <QColorDialog>
#include <QColor>
#include <QHBoxLayout>
#include <QDockWidget>
#include <QSettings>

//* Root
#include <Root/ActionHandler.h>
#include <Root/Command.h>
#include <Root/Application.h>
#include <Root/UpstreamFilter.h>
#include <Root/MessageLog.h>
using namespace Root;

//* Lexi
#include <Lexi/LayoutKit.h>
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
#include <SpecBatchList.h>
#include <SpectrumListView.h>
#include <SpecView/DynValueEditor.h>
#include <SpecView/ObjectListView.h>
#include <Gui/SplitGrid.h>

using namespace Spec;

//////////////////////////////////////////////////////////////////////

void createMonoScope( Root::Agent* a, Spec::Spectrum* s, 
					 Spec::Project* p, PeakList* l, const Rotation& r )
{
	new MonoScope2( a, s, p, l, r );
}

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr MonoScope2::ShowSlices = "ShowSlices";
Root::Action::CmdStr MonoScope2::ShowList = "ShowList";
Root::Action::CmdStr MonoScope2::NewPeaklist = "NewPeaklist";
Root::Action::CmdStr MonoScope2::ImportPeaklist = "ImportPeaklist";
Root::Action::CmdStr MonoScope2::ExportPeaklist = "ExportPeaklist";
Root::Action::CmdStr MonoScope2::OpenPeaklist = "OpenPeaklist";
Root::Action::CmdStr MonoScope2::SavePeaklist = "SavePeaklist";
Root::Action::CmdStr MonoScope2::GotoPeak = "GotoPeak";
Root::Action::CmdStr MonoScope2::ExportIntegTable = "ExportIntegTable";
Root::Action::CmdStr MonoScope2::NextSpec = "NextSpec";
Root::Action::CmdStr MonoScope2::PrevSpec = "PrevSpec";
Root::Action::CmdStr MonoScope2::GotoSpec = "GotoSpec";
Root::Action::CmdStr MonoScope2::SetupBatch = "SetupBatch";
Root::Action::CmdStr MonoScope2::AdaptHome = "AdaptHome";
Root::Action::CmdStr MonoScope2::ShowAssig = "ShowAssig";
Root::Action::CmdStr MonoScope2::ImportAlias = "ImportAlias";
Root::Action::CmdStr MonoScope2::ImportLinks = "ImportLinks";
Root::Action::CmdStr MonoScope2::EditPeak = "EditPeak";
Root::Action::CmdStr MonoScope2::ShowPeak = "ShowPeak";
Root::Action::CmdStr MonoScope2::CheckPeakDoubles = "CheckPeakDoubles";
Root::Action::CmdStr MonoScope2::EditAtts = "EditAtts";
Root::Action::CmdStr MonoScope2::SetColor = "SetColor";
Root::Action::CmdStr MonoScope2::NextList = "NextList";
Root::Action::CmdStr MonoScope2::PrevList = "PrevList";
Root::Action::CmdStr MonoScope2::ShowTable = "ShowTable";
Root::Action::CmdStr MonoScope2::SetRgbColor = "SetRgbColor";
Root::Action::CmdStr MonoScope2::ExportCaraSpec = "ExportCaraSpec";
Root::Action::CmdStr MonoScope2::ExportEasySpec = "ExportEasySpec";
Root::Action::CmdStr MonoScope2::SyncPeak = "SyncPeak";
Root::Action::CmdStr MonoScope2::SaveColors = "SaveColors";
Root::Action::CmdStr MonoScope2::SetLabel = "SetLabel";
Root::Action::CmdStr MonoScope2::EditAssig = "EditAssig";
Root::Action::CmdStr MonoScope2::AddAssig = "AddAssig";
Root::Action::CmdStr MonoScope2::RemoveAssig = "RemoveAssig";
Root::Action::CmdStr MonoScope2::ExportTable = "ExportTable";

//////////////////////////////////////////////////////////////////////

static void buildCommands( CommandLine* cl )
{
	cl->registerCommand( new ActionCommand( MonoScopeAgent::AdjustIntensity, "CW", "Adjust Intensity", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::DeleteAliasPeaks, "UA", "Un-Alias Peaks", true ) );
	cl->registerCommand( new ActionCommand( MonoScope2::SyncPeak, "SP", "Sync. Peak", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::ShowFolded, "SF", "Show Folded", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::AutoContour, "AC", "Auto Contour Level", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::Backward, "BB", "Backward", true ) );
	cl->registerCommand( new ActionCommand( MonoScope2::ShowList, "SL", "Show List", true ) );
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
	cl->registerCommand( new ActionCommand( MonoScope2::NextSpec, "NS", "Next Spectrum", true ) );
	cl->registerCommand( new ActionCommand( MonoScope2::PrevSpec, "PS", "Prev. Spectrum", true ) );
	cl->registerCommand( new ActionCommand( MonoScope2::NextList, "NL", "Next Peaklist", true ) );
	cl->registerCommand( new ActionCommand( MonoScope2::PrevList, "PL", "Prev. Peaklist", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::PeakCurve, "PC", "Show Peak Curve", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::PeakCalibrate, "CL", "Calibrate Peaklist", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::ForwardPlane, 
		"FP", "Forward Plane", true ) );
	cl->registerCommand( new ActionCommand( MonoScopeAgent::BackwardPlane, 
		"BP", "Backward Plane", true ) );

	ActionCommand* cmd = new ActionCommand( MonoScope2::GotoPeak, 
		"GP", "Goto Peak", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( MonoScope2::GotoSpec, 
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

MonoScope2::MonoScope2(Root::Agent * a, Spectrum* s, Project* pro, 
					 PeakList* pl, const Rotation& rot ):
	Lexi::MainWindow( a, true, true ), d_ovCtrl( 0 ),
		d_list( 0 )
{
	assert( s );
	assert( s->getDimCount() > 1 );
	d_agent = new MonoScopeAgent( this, s, pro, rot );
	
	if( pl )
		d_agent->setAutoCenter( true );
	init();

	// RISK: immer ab initio eine leere Peakliste anbieten, die auf Spec passt.
	if( pl )
		d_agent->setPeaklist( pl, true );
	else
		d_agent->setPeaklist( new PeakList( s ) ); 
	if( d_list )
		d_list->setPeakList( d_agent->getPeaklist(), d_agent->getMainSpec() );
	if( pl )
		d_listWidget->parentWidget()->setVisible(true);
}

void MonoScope2::init()
{

	buildMenus();
	updateCaption();

	getQt()->setDockNestingEnabled( true );

	QDockWidget* sliceDock = new QDockWidget( "Depth Slices", getQt() );
	sliceDock->setObjectName( sliceDock->windowTitle() );
	sliceDock->setAllowedAreas( Qt::AllDockWidgetAreas );
	sliceDock->setFeatures( QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable );
	sliceDock->setVisible( false );
	d_depthPane = new QWidget( sliceDock );
	QHBoxLayout* box = new QHBoxLayout( d_depthPane );
	box->setMargin( 0 );
	box->setSpacing( 2 );
	sliceDock->setWidget( d_depthPane );
	getQt()->addDockWidget( Qt::RightDockWidgetArea, sliceDock );

	QDockWidget* peakDock = new QDockWidget( "Peaklist", getQt() );
	peakDock->setObjectName( peakDock->windowTitle() );
	peakDock->setAllowedAreas( Qt::AllDockWidgetAreas );
	peakDock->setFeatures( QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable );
	peakDock->setVisible( false );
	d_listWidget = new GlyphWidget( peakDock, 0, new UpstreamFilter( this, true ), true );
	peakDock->setWidget( d_listWidget );
	getQt()->addDockWidget( Qt::RightDockWidgetArea, peakDock );

	d_list = new PeakListGadget( d_listWidget->getViewport(), d_agent->getPeaklist(), d_agent->getMainSpec(), this, 
		0, true, d_agent->getPro() );
	Gui::Menu* menu = new Gui::Menu( d_list->getListView()->getImp(), true );
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
	d_listWidget->setBody( d_list );

	buildViews( true );
	getQt()->showMaximized();
	QMetaObject::invokeMethod( this, "restoreLayout", Qt::QueuedConnection );
}

void MonoScope2::restoreLayout()
{
	/* Restore ist nicht erwünscht
	QSettings set;
	QVariant state = set.value( "MonoScope/State" );
	if( !state.isNull() )
		getQt()->restoreState( state.toByteArray() );
		*/
	Gui::SplitGrid* grid = static_cast<Gui::SplitGrid*>( getQt()->centralWidget() );
	//QVariant v = set.value( "MonoScope/GridSizes" );
	if( false ) // v.isValid() )
	{
		//grid->setSizesCoded( v );
	}else
	{

		grid->setRowSizes( QList<int>() << ( grid->height() * 0.75 ) << ( grid->height() * 0.25 ) );
		grid->setColSizes( QList<int>() << ( grid->width() * 0.25 ) << ( grid->width() * 0.75 ) );
	}
	//d_depthPane->parentWidget()->resize(  
	d_depthPane->parentWidget()->setVisible( d_agent->getMainSpec()->getDimCount() > 2 );
}

MonoScope2::~MonoScope2()
{
}

void MonoScope2::updateCaption()
{
	QString str;
	str.sprintf( "MonoScope - %s", d_agent->getMainSpec()->getName() );
	getQt()->setCaption( str );
}

void MonoScope2::buildMenus()
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
	// Gui::Menu::item( menuView, d_agent, "Show Slices", ShowSlices, true );
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
		MonoScope2::SaveColors, false );
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
		MonoScope2::NextSpec, false, Qt::CTRL+Qt::Key_2 );
	Gui::Menu::item( menuSpec, d_agent, "&Previous Spectrum", 
		MonoScope2::PrevSpec, false, Qt::CTRL+Qt::Key_1 );
	Gui::Menu::item( menuSpec, d_agent, "&Setup Batch List...", 
		MonoScope2::SetupBatch, false );
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
		MonoScope2::CheckPeakDoubles, false ); 
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

static GlyphWidget* _create( MonoScope2* w, CommandLine* cl, SpecViewer* v )
{
	return new GlyphWidget( w->getQt(), new Redirector( cl, new Background( v, Qt::black, true ) ),
		new UpstreamFilter( w, true ), true );
}

void MonoScope2::buildViews( bool create )
{
	// Hier wird für jede Pane ein GlyphWidget verwendet, so dass Layout und Fokus neu Sache von Qt ist.
	if( getQt()->centralWidget() )
		delete getQt()->centralWidget();

	Gui::SplitGrid* grid = new Gui::SplitGrid( getQt() );
	getQt()->setCentralWidget( grid );
	QVector<Gui::SplitGrid::Rows> matrix; // Spalten x Zeilen
	if( true ) // d_agent->showSlices() )
	{
		matrix.resize( 2 );
		matrix[0].resize( 2 );
		matrix[1].resize( 2 );
	}else
	{
		matrix.resize( 1 );
		matrix[0].resize( 1 );
	}

	CommandLine* cl = new CommandLine( d_agent );
	buildCommands( cl );

	if( create )
		d_agent->allocate();

	SpecViewer* plane = 0;
	if( create )
		plane = d_agent->createPlaneViewer();
	else
		plane = d_agent->getPlaneViewer();

	if( true ) // d_agent->showSlices() )
	{
		d_mainPane = _create( this, cl, plane );
		matrix[1][0] = d_mainPane;

		const Dimension dim = d_agent->getMainSpec()->getDimCount();
		SpecViewer* g = 0;
		if( create || d_agent->getSliceViewer( DimX ) == 0 )
			g = d_agent->createSliceViewer( DimX, DimX );
		else
			g = d_agent->getSliceViewer( DimX );
		matrix[1][1] = _create( this, cl, g );

		if( create || d_agent->getSliceViewer( DimY ) == 0 )
			g = d_agent->createSliceViewer( DimY, DimY );
		else
			g = d_agent->getSliceViewer( DimY );
		matrix[0][0] = _create( this, cl, g );

		// Create Overview
		if( create || d_ovCtrl == 0 )
		{
			d_ov = new SpecViewer( new ViewAreaMdl( true, true, true, true ) );
			Root::Ref<SpecProjector> pro = new SpecProjector( d_agent->getMainSpec(), DimX, DimY );
			Root::Ref<SpecBufferMdl> mdl = new SpecBufferMdl( d_ov->getViewArea(), pro, false );
			d_ov->getViews()->append( new IntensityView( mdl ) );
			d_ovCtrl = new OverviewCtrl( mdl, plane->getViewArea() );
			d_ov->getHandlers()->append( d_ovCtrl );
			d_ov->getHandlers()->append( new FocusCtrl( d_ov ) );
		}else
			d_ovCtrl->setTarget( plane->getViewArea() );

		matrix[0][1] = _create( this, cl, d_ov );
	}else
	{
		d_mainPane = _create( this, cl, plane );
		matrix[0][0] = d_mainPane;
	}

	if( create )
	{
		const Dimension dim = d_agent->getMainSpec()->getDimCount();
		SpecViewer* g = 0;
		for( Dimension d = 2; d < dim; d++ )
		{
			QLayout* box = d_depthPane->layout();
			if( create || d_agent->getSliceViewer( d ) == 0 )
				g = d_agent->createSliceViewer( DimY, d );
			else
				g = d_agent->getSliceViewer( d );

			box->addWidget( new GlyphWidget( d_depthPane, new Redirector( cl, new Background( g, Qt::black, true ) ), 
				new UpstreamFilter( this, true ), true ) );
		}
	}
	for( int i = 0; i < matrix.size(); i++ )
		grid->addColumn( matrix[i] );
	if( create )
		d_agent->initOrigin();

	if( true ) // d_agent->showSlices() )
	{
		grid->setColStretchFactor( 1, 6 );
		grid->setColStretchFactor( 0, 1 );
		grid->setRowStretchFactor( 1, 4 );
		grid->setRowStretchFactor( 0, 1 );
	}

	if( create && d_agent->getPeaklist() )
	{
		Root::Ref<PeakList> tmp = d_agent->getPeaklist();
		d_agent->setPeaklist( tmp );	// NOTE: da wir sonst zuerst deref und dann ref
	}
	if( create )
	{
		d_agent->fitToView();
		d_agent->setCursor();
	}
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
}

void MonoScope2::handleAction(Root::Action& a)
{
	if( !EXECUTE_ACTION( MonoScope2, a ) )
		MainWindow::handleAction( a );
}

void MonoScope2::handle(Root::Message& m)
{
	BEGIN_HANDLER();
	MESSAGE( PeakListGadget::ActivatePeakMsg, e, m )
	{
		d_agent->setCursor( e->getPeak() );
		d_mainPane->QWidget::setFocus();
		m.consume();
	}HANDLE_ELSE()
		MainWindow::handle( m );
	END_HANDLER();
}

bool MonoScope2::askToCloseWindow() const
{
	QSettings set;
	set.setValue("MonoScope/State", getQt()->saveState() );
	set.setValue("MonoScope/GridSizes", static_cast<Gui::SplitGrid*>( getQt()->centralWidget() )->getSizesCoded() );

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

bool MonoScope2::handleSave(bool xp) const
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
			fileName = d_agent->getPeaklist()->getName().c_str();
			fileName += ".peaks";
		}
		fileName = QFileDialog::getSaveFileName( getQt(), "Export Pealist", fileName, 
				"*.peaks" );
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
			ps->saveToFile( fileName.ascii() );
		}catch( Root::Exception& e )
		{
			Root::ReportToUser::alert( const_cast<MonoScope2*>(this), "Error Exporting Peaklist", e.what() );
		}
	}else
	{
		bool ok;
		QString res = QInputDialog::getText( "Name Peaklist", 
			"Please enter a unique short name:", QLineEdit::Normal, 
			d_agent->getPeaklist()->getName().c_str(), &ok, getQt() );
		if( !ok )
			return false;
		assert( d_agent->getPro() != 0 );
		if( res.isEmpty() || d_agent->getPro()->findPeakList( res.latin1() ) != 0 )	
		{
			QMessageBox::critical( getQt(), "Save Peaklist",
					"This peaklist name is already used!", "&Cancel" );
			return false;
		}
		d_agent->getPeaklist()->setName( res.latin1() );
		d_agent->getPro()->addPeakList( d_agent->getPeaklist() );
		d_agent->getPeaklist()->clearDirty();
	}
	return true;
}

void MonoScope2::handleShowSlices(Action & a)
{
	ENABLED_CHECKED_IF( a, true, d_agent->showSlices() );

	d_agent->showSlices( !d_agent->showSlices() );
	Lexi::Viewport::pushHourglass();
	buildViews( false );
	Lexi::Viewport::popCursor();
}

void MonoScope2::handleNewPeaklist(Action & a)
{
	ENABLED_IF( a, true );
	//if( !askToClosePeaklist() )
	//	return;

	d_agent->setPeaklist( new PeakList( d_agent->getMainSpec() ) );
	if( d_list )
		d_list->setPeakList( d_agent->getPeaklist(), d_agent->getMainSpec() );
}

void MonoScope2::handleImportPeaklist(Action & a)
{
	ENABLED_IF( a, true );
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
		pl->setName( info.baseName().latin1() );

		ColorMap specClr;
		d_agent->getMainSpec()->getColors( specClr );
		if( specClr != psClr )
		{
			// pl passt noch nicht auf spec
			Rotation rot = createRotation( specClr, psClr );
			if( rot.size() == psClr.size() )
			{
				pl->rotateAll( rot ); // eine Rotation ist möglich.
			}
			d_agent->setPeaklist( pl, rot.size() != psClr.size() );
		}else
			// pl passt bereits auf spec
			d_agent->setPeaklist( pl, false );
		if( d_list )
			d_list->setPeakList( d_agent->getPeaklist(), d_agent->getMainSpec() );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Error Importing Peaklist", e.what() );
	}
}

void MonoScope2::handleExportPeaklist(Action & a)
{
	ENABLED_IF( a, d_agent->getPeaklist() );

	handleSave( true );
}

void MonoScope2::handleShowList(Action & a)
{
	ENABLED_CHECKED_IF( a, true, d_listWidget->parentWidget()->isVisible() );

	d_listWidget->parentWidget()->setVisible( !d_listWidget->parentWidget()->isVisible() );
}

void MonoScope2::handleOpenPeaklist(Action & a)
{
	ENABLED_IF( a, d_agent->getPro() != 0 );

	PeakList* pl = Dlg::selectPeakList( getQt(), d_agent->getPro() );
	if( pl == 0 )
		return;
	d_agent->setPeaklist( pl, true );
	if( d_list )
		d_list->setPeakList( d_agent->getPeaklist(), d_agent->getMainSpec() );
}

void MonoScope2::handleSavePeaklist(Action & a)
{
	ENABLED_IF( a, d_agent->getPeaklist() && 
		d_agent->getPeaklist()->isNew() && d_agent->getPro() != 0 );

	handleSave( false );
}

void MonoScope2::handleGotoPeak(Action & a)
{
	ENABLED_IF( a, d_agent->getPeaklist() );

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

void MonoScope2::handleSelectSpec(Action & a)
{
	Spectrum* spec = dynamic_cast<Spectrum*>( a.getParam( 0 ).getObject() );
	assert( spec );
	ENABLED_CHECKED_IF( a, d_agent->getPeaklist() != 0, 
		d_agent->getSpec()->getId() == spec->getId() );
	if( d_agent->getSpec()->getId() == spec->getId() )
		return;
	if( d_agent->setSpec( spec ) && d_list )
		d_list->setSpec( d_agent->getSpec() );
}

void MonoScope2::handleExportIntegTable(Action & a)
{
	ENABLED_IF( a, d_agent->getPro() && d_agent->getPeaklist() );

	QString fileName = QFileDialog::getSaveFileName( getQt(), "Export Integration Table" , 
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

void MonoScope2::handleGotoSpec(Action & a)
{
	ENABLED_IF( a, d_agent->getPro() && d_agent->getPeaklist() );

	long i = a.getParam( 0 ).getLong();
	const PeakList::SpecList& specs = d_agent->getPeaklist()->getSpecs();

	Spectrum* spec = 0;
	if( i == 0 )
		spec = d_agent->getMainSpec();
	else if( i > 0 && i <= specs.size() )
		spec = d_agent->getPro()->getSpec( specs[ i - 1 ] );

	if( spec && d_agent->setSpec( spec ) && d_list )
		d_list->setSpec( d_agent->getSpec() );
}

static int _findPos( const PeakList::SpecList& s, Spectrum* spec )
{
	for( int i = 0; i < s.size(); i++ )
		if( s[ i ] == spec->getId() )
			return i;
	return -1;
}

void MonoScope2::handleNextSpec(Action & a)
{
	if( d_agent->getPro() == 0 || d_agent->getPeaklist() == 0 )
		return;
	const PeakList::SpecList& specs = d_agent->getPeaklist()->getSpecs();
	int i = _findPos( specs, d_agent->getSpec() );
	i++;
	ENABLED_IF( a, i < specs.size() );
	if( i >= specs.size() )
		return; // NOTE: QT-BUG
	Spectrum* spec =  d_agent->getPro()->getSpec( specs[ i ] );

	if( spec && d_agent->setSpec( spec ) && d_list )
		d_list->setSpec( d_agent->getSpec() );
}

void MonoScope2::handlePrevSpec(Action & a)
{
	if( d_agent->getPro() == 0 || d_agent->getPeaklist() == 0 )
		return;
	const PeakList::SpecList& specs = d_agent->getPeaklist()->getSpecs();
	int i = _findPos( specs, d_agent->getSpec() );
	if( i == -1 )
		i = specs.size();
	i--;
	ENABLED_IF( a, i >= 0 && i < specs.size() );
	if( i < 0 )
		return; // NOTE: QT-BUG
	Spectrum* spec =  d_agent->getPro()->getSpec( specs[ i ] );

	if( spec && d_agent->setSpec( spec ) && d_list )
		d_list->setSpec( d_agent->getSpec() );
}

void MonoScope2::handleSetupBatch(Action & a)
{
	ENABLED_IF( a, d_agent->getPro() && d_agent->getPeaklist() );

	SpecBatchList dlg( getQt(), d_agent->getPeaklist(), d_agent->getPro() );
	if( dlg.doit() )
		d_agent->reloadSpecPop();
}

void MonoScope2::handleAdaptHome(Action & a)
{
	PeakList* pl = d_agent->getPeaklist();
	ENABLED_IF( a, pl && pl->getHome() != d_agent->getSpec()->getId() );

	if( QMessageBox::warning( getQt(), "Set Peaklist Owner", 
		"Do you really want this spectrum own the current peaklist?", 
		"&OK", "&Cancel", QString::null, 1, 1 ) != 0 )		
		return;	// Cancel
	if( !pl->setHome( d_agent->getSpec() ) )
		QMessageBox::critical( getQt(), "Set Peaklist Owner", 
		"Peaklist not compatible with spectrum!", "&Cancel" );
}

void MonoScope2::handleShowAssig(Action & a)
{
	ENABLED_CHECKED_IF( a, d_list, d_list && d_list->showAssig() );

	d_list->showAssig( !d_list->showAssig() );
}

void MonoScope2::handleImportAlias(Action & a)
{
	ENABLED_IF( a, d_agent->getPeaklist() );

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

void MonoScope2::handleImportLinks(Action & a)
{
	ENABLED_IF( a, d_agent->getPeaklist() );

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

void MonoScope2::handleEditPeak(Action & a)
{
	ENABLED_IF( a, d_list && d_list->getSelected() );

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

void MonoScope2::handleShowPeak(Action & a)
{
	ENABLED_IF( a, d_list && d_list->getSelected() );

	d_agent->setCursor( d_list->getSelected() );
}

void MonoScope2::handleCheckPeakDoubles(Action & a)
{
	ENABLED_IF( a, d_agent->getPeaklist() );

	PeakList* pl = d_agent->getPeaklist();
	Spectrum* spec = d_agent->getSpec();
	PeakList::SelectionSet res = pl->findDoubles( spec );

	QString str;
	str.sprintf( "Peaklist %s contains the following groups of degenerate peaks "
		"in spectrum %s (one group per line):\n",
		pl->getName().c_str(), spec->getName() );
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
		Root::MessageLog::inst()->warning( "Report Degenerate Peaks", str );
		str.sprintf( "The peaklist contains %d degenerate peaks. "
			"Check message log for details.", res.size() );
		QMessageBox::information( getQt(), "Report Degenerate Peaks", str, "&OK" );
	}else
		QMessageBox::information( getQt(), "Report Degenerate Peaks", 
			"The peaklist contains no degenerate peaks for the selected spectrum.", "&OK" );
}

void MonoScope2::handleEditAtts(Action & a)
{
	Peak* p = (d_list)?d_list->getPeak():0;
	ENABLED_IF( a, d_agent->getPro() && p );

	Peak::Guess* g = d_list->getGuess();
	if( g )
		DynValueEditor::edit( getQt(), d_agent->getPro()->getRepository()
			->findObjectDef( Repository::keyGuess ), g );
	else
		DynValueEditor::edit( getQt(), d_agent->getPro()->getRepository()
			->findObjectDef( Repository::keyPeak ), p );
}

void MonoScope2::handleSetColor(Action & a)
{
	ENABLED_IF( a, d_list && d_list->getSelected() );

	Peak* p = d_agent->getPeaklist()->getPeak( d_list->getSelected() );

	bool ok;
	int res = QInputDialog::getInteger( "Set Color", 
		"Please enter a valid color code:", 
		p->getColor(), 0, 255, 1, &ok, getQt() );
	if( !ok )
		return;
	d_agent->getPeaklist()->setColor( p, res );	// TODO: undo
}

void MonoScope2::handleSelectPeaks(Action & a)
{
	PeakList* pl = dynamic_cast<PeakList*>( a.getParam( 0 ).getObject() );
	ENABLED_CHECKED_IF( a, true, d_agent->getPeaklist() == pl );
	if( d_agent->getPeaklist() == pl )
		return;
	d_agent->setPeaklist( pl );
	if( d_list )
		d_list->setPeakList( pl, d_agent->getMainSpec() );
}

void MonoScope2::handlePrevList(Action & a)
{
	const MonoScopeAgent::Histo& h = d_agent->getHisto();
	ENABLED_IF( a, h.size() > 1 );
	MonoScopeAgent::Histo::const_iterator i = h.find( d_agent->getPeaklist() );
	if( i == h.begin() )
		i = h.end();
	--i;
	d_agent->setPeaklist( (*i) );
	if( d_list )
		d_list->setPeakList( (*i), d_agent->getMainSpec() );
}

void MonoScope2::handleNextList(Action & a)
{
	const MonoScopeAgent::Histo& h = d_agent->getHisto();
	ENABLED_IF( a, h.size() > 1 );
	MonoScopeAgent::Histo::const_iterator i = h.find( d_agent->getPeaklist() );
	i++;
	if( i == h.end() )
		i = h.begin();
	d_agent->setPeaklist( (*i) );
	if( d_list )
		d_list->setPeakList( (*i), d_agent->getMainSpec() );
}

void MonoScope2::handleShowTable(Root::Action & a)
{
	PeakList* pl = d_agent->getPeaklist();
	ENABLED_IF( a, pl );

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

void MonoScope2::handleSetRgbColor(Action & a)
{
	ENABLED_IF( a, d_list && d_list->getSelected() );

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

void MonoScope2::handleExportCaraSpec(Action & a)
{
	Spectrum* spec = d_agent->getSpec();
	ENABLED_IF( a, spec );
	SpectrumListView::saveCaraSpectrum( getQt(), spec );
}

void MonoScope2::handleExportEasySpec(Action & a)
{
	Spectrum* spec = d_agent->getSpec();
	ENABLED_IF( a, spec );
	SpectrumListView::saveEasySpectrum( getQt(), spec );
}

void MonoScope2::handleSyncPeak(Action & a)
{
	if( d_list == 0 || d_agent->getPlane().d_peaks == 0 ||
		d_agent->getPlane().d_peaks->getSel().size() != 1 )
		return;

	ENABLED_IF( a, true );
	d_list->gotoPeak( *d_agent->getPlane().d_peaks->getSel().begin() );
}

void MonoScope2::handleSaveColors(Action & a)
{
	ENABLED_IF( a, d_agent->getPro() );
	Repository::SlotColors clr( d_agent->getPlane().d_ol.size() );
	for( int i = 0; i < clr.size(); i++ )
	{
		clr[ i ].d_pos = d_agent->getPlane().d_ol[i].d_view->getPosColor();
		clr[ i ].d_neg = d_agent->getPlane().d_ol[i].d_view->getNegColor();
	}
	d_agent->getPro()->getRepository()->setScreenClr( clr );
}

void MonoScope2::handleSetLabel(Action & a)
{
	ENABLED_IF( a, d_list && d_list->getSelected() );

	Peak* p = d_agent->getPeaklist()->getPeak( d_list->getSelected() );

	bool ok	= FALSE;
	QString res	= QInputDialog::getText( "Force Peak Label", 
		"Please	enter a label:", QLineEdit::Normal, 
		p->getTag().c_str(), &ok, getQt() );
	if( !ok )
		return;
	d_agent->getPeaklist()->setTag( p, res.latin1() );	// TODO: undo
}

void MonoScope2::handleEditAssig(Action & a)
{
	Peak* p = (d_list)?d_list->getPeak():0;
	ENABLED_IF( a, d_agent->getPro() &&  p );

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

void MonoScope2::handleAddAssig(Action & a)
{
	Peak* p = (d_list)?d_list->getPeak():0;
	ENABLED_IF( a, d_agent->getPro() && p );
	Dlg::Assig ass;;
	float f = 0.0;
	if( Dlg::getAssig( getQt(), p->getDimCount(), ass, &f, "Add Assignment" ) )
	{
		d_agent->getPeaklist()->addGuess( p, ass, f );
	}
}

void MonoScope2::handleRemoveAssig(Action & a)
{
	Peak::Guess* g = (d_list)?d_list->getGuess():0;
	ENABLED_IF( a, d_agent->getPro() && g );
	d_agent->getPeaklist()->deleteGuess( d_list->getPeak(), g->getId() );
}

void MonoScope2::handleExportTable(Action & a)
{
	ENABLED_IF( a, d_agent->getPeaklist() );

	QString fileName;
	if( d_agent->getPeaklist()->getName().empty() )
		fileName = AppAgent::getCurrentDir();
	else
	{
		fileName = d_agent->getPeaklist()->getName().c_str();
		fileName += ".txt";
	}
	fileName = QFileDialog::getSaveFileName( getQt(), "Export Peak Table", fileName, "*.txt" );
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

//////////////////////////////////////////////////////////////////////
// Muss am Schluss stehen, da sonst ClassViewer spinnt.

ACTION_SLOTS_BEGIN( MonoScope2 )
	{ MonoScope2::ExportTable, &MonoScope2::handleExportTable },
	{ MonoScope2::EditAssig, &MonoScope2::handleEditAssig },
	{ MonoScope2::AddAssig, &MonoScope2::handleAddAssig },
	{ MonoScope2::RemoveAssig, &MonoScope2::handleRemoveAssig },
	{ MonoScope2::SetLabel, &MonoScope2::handleSetLabel },
	{ MonoScope2::SaveColors, &MonoScope2::handleSaveColors },
	{ MonoScope2::SyncPeak, &MonoScope2::handleSyncPeak },
	{ MonoScope2::ExportEasySpec, &MonoScope2::handleExportEasySpec },
	{ MonoScope2::ExportCaraSpec, &MonoScope2::handleExportCaraSpec },
	{ MonoScope2::SetRgbColor, &MonoScope2::handleSetRgbColor },
	{ MonoScope2::ShowTable, &MonoScope2::handleShowTable },
	{ MonoScope2::PrevList, &MonoScope2::handlePrevList },
	{ MonoScope2::NextList, &MonoScope2::handleNextList },
	{ MonoScopeAgent::SelectPeaks, &MonoScope2::handleSelectPeaks },
	{ MonoScope2::SetColor, &MonoScope2::handleSetColor },
	{ MonoScope2::EditAtts, &MonoScope2::handleEditAtts },
	{ MonoScope2::CheckPeakDoubles, &MonoScope2::handleCheckPeakDoubles },
	{ MonoScope2::ShowPeak, &MonoScope2::handleShowPeak },
	{ MonoScope2::EditPeak, &MonoScope2::handleEditPeak },
	{ MonoScope2::ImportAlias, &MonoScope2::handleImportAlias },
	{ MonoScope2::ImportLinks, &MonoScope2::handleImportLinks },
	{ MonoScope2::ShowAssig, &MonoScope2::handleShowAssig },
	{ MonoScope2::AdaptHome, &MonoScope2::handleAdaptHome },
	{ MonoScope2::SetupBatch, &MonoScope2::handleSetupBatch },
	{ MonoScope2::PrevSpec, &MonoScope2::handlePrevSpec },
	{ MonoScope2::NextSpec, &MonoScope2::handleNextSpec },
	{ MonoScope2::GotoSpec, &MonoScope2::handleGotoSpec },
	{ MonoScope2::ExportIntegTable, &MonoScope2::handleExportIntegTable },
	{ MonoScopeAgent::SelectSpec, &MonoScope2::handleSelectSpec },
	{ MonoScope2::GotoPeak, &MonoScope2::handleGotoPeak },
	{ MonoScope2::SavePeaklist, &MonoScope2::handleSavePeaklist },
	{ MonoScope2::OpenPeaklist, &MonoScope2::handleOpenPeaklist },
	{ MonoScope2::ShowList, &MonoScope2::handleShowList },
	{ MonoScope2::ExportPeaklist, &MonoScope2::handleExportPeaklist },
	{ MonoScope2::ImportPeaklist, &MonoScope2::handleImportPeaklist },
	{ MonoScope2::NewPeaklist, &MonoScope2::handleNewPeaklist },
	// { MonoScope2::ShowSlices, &MonoScope2::handleShowSlices },
ACTION_SLOTS_END( MonoScope2 )

//////////////////////////////////////////////////////////////////////


