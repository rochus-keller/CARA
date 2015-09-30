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

#include "Phaser.h"
// Qt
#include <qtextstream.h> 
#include <qinputdialog.h> 
#include <qmessagebox.h>
#include <QFileDialog>
#include <qfileinfo.h> 
#include <qdir.h> 
#include <qmenubar.h>
#include <qapplication.h>

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
#include <Gui/Menu.h>
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
#include <SpecView/OverviewCtrl.h>
#include <SpecView/PeakPlaneView.h>

#include <Dlg.h>

using namespace Spec;

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr Phaser::LoadImagX = "LoadImagX";
Root::Action::CmdStr Phaser::LoadImagY = "LoadImagY";
Root::Action::CmdStr Phaser::LoadImagZ = "LoadImagZ";

ACTION_SLOTS_BEGIN( Phaser )
    { Phaser::LoadImagX, &Phaser::handleLoadImagX },
    { Phaser::LoadImagY, &Phaser::handleLoadImagY },
    { Phaser::LoadImagZ, &Phaser::handleLoadImagZ },
ACTION_SLOTS_END( Phaser )

//////////////////////////////////////////////////////////////////////
// Entkopplung; nutzbar ohne include

void createPhaser( Root::Agent* a, Spec::Spectrum* s )
{
	new Spec::Phaser( a, s );
}

//////////////////////////////////////////////////////////////////////

static void buildCommands( CommandLine* cl )
{
	cl->registerCommand( new ActionCommand( PhaserAgent::AutoContour, "AC", "Auto Contour Level", true ) );
	cl->registerCommand( new ActionCommand( PhaserAgent::FitWindow, "WW", "Fit To Window", true ) );
	cl->registerCommand( new ActionCommand( PhaserAgent::Goto, "GT", "Goto", true ) );
	cl->registerCommand( new ActionCommand( PhaserAgent::ShowContour, "SC", "Show Contour", true ) );
	cl->registerCommand( new ActionCommand( PhaserAgent::ShowIntensity, "SI", "Show Intensity", true ) );
	cl->registerCommand( new ActionCommand( PhaserAgent::ContourParams, "CP", "Contour Params", true ) );
	cl->registerCommand( new ActionCommand( PhaserAgent::Backward, "RZ", "Backward", true ) );
	cl->registerCommand( new ActionCommand( PhaserAgent::ShowLowRes, "LO", "Low Resolution", true ) );
	cl->registerCommand( new ActionCommand( Root::Action::EditUndo, "ZZ", "Undo", true ) );
	cl->registerCommand( new ActionCommand( Root::Action::EditRedo, "YY", "Redo", true ) );
	cl->registerCommand( new ActionCommand( PhaserAgent::ForwardPlane, 
		"FP", "Forward Plane", true ) );
	cl->registerCommand( new ActionCommand( PhaserAgent::BackwardPlane, 
		"BP", "Backward Plane", true ) );

	ActionCommand* cmd = new ActionCommand( Root::Action::ExecuteLine, "LUA", "Lua", false );
	cmd->registerParameter( Any::Memo, true ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PhaserAgent::AutoGain, 
		"AG", "Set Auto Contour Gain", false );
	cmd->registerParameter( Any::Float, true ); 
	cl->registerCommand( cmd );

}

//////////////////////////////////////////////////////////////////////

static QColor g_frameClr = Qt::lightGray;

Phaser::Phaser(Root::Agent * a, Spectrum* spec ):
	Lexi::MainWindow( a, true, true )
{

	assert( spec && spec->getDimCount() >= 2 );
	d_agent = new PhaserAgent( this, spec );
	
	init();

	// RISK: immer ab initio eine leere Peakliste anbieten, die auf Spec passt.
}

void Phaser::init()
{
	Root::UpstreamFilter* filter = new UpstreamFilter( this, true );

	d_focus = new FocusManager( nil, true );
	Redirector* redir = new Redirector( d_focus );
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

Phaser::~Phaser()
{
	d_widget->setBody( 0 );
	d_agent = 0;
}

void Phaser::updateCaption()
{
	QString str;
	str.sprintf( "Phaser - %s", d_agent->getSpec()->getName() );
	getQt()->setCaption( str );
}

void Phaser::buildMenus()
{
	Gui::Menu* menuFile = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuFile, this, "Open Imag. Dim. X", Phaser::LoadImagX, true );
	Gui::Menu::item( menuFile, this, "Open Imag. Dim. Y", Phaser::LoadImagY, true );
	Gui::Menu::item( menuFile, this, "Open Imag. Dim. Z", Phaser::LoadImagZ, true );
	menuFile->insertSeparator();
    Gui::Menu::item( menuFile, this, Root::Action::WindowClose, Qt::CTRL+Qt::Key_W );
	menuBar()->insertItem( "&File", menuFile );

	Gui::Menu* menuEdit = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuEdit, this, Root::Action::EditUndo, Qt::CTRL+Qt::Key_Z );
	Gui::Menu::item( menuEdit, this, Root::Action::EditRedo, Qt::CTRL+Qt::Key_Y );
	menuBar()->insertItem( "&Edit", menuEdit );

	Gui::Menu* menuView = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuView, d_agent, "Backward", 
		PhaserAgent::Backward, false );
	Gui::Menu::item( menuView, d_agent, "Forward", 
		PhaserAgent::Forward, false );
	Gui::Menu::item( menuView, d_agent, "Fit Window", 
		PhaserAgent::FitWindow, false, Qt::Key_Home );
	Gui::Menu::item( menuView, d_agent, "Goto...", 
		PhaserAgent::Goto, false );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Show Contour", 
		PhaserAgent::ShowContour, true );
	Gui::Menu::item( menuView, d_agent, "Show Intensity", 
		PhaserAgent::ShowIntensity, true );
	Gui::Menu::item( menuView, d_agent, "Auto Contour Level", 
		PhaserAgent::AutoContour, true );
	Gui::Menu::item( menuView, d_agent, "Set Contour Parameters...", 
		PhaserAgent::ContourParams, false );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Set Resolution...", 
		PhaserAgent::SetResolution, false );
	Gui::Menu::item( menuView, d_agent, "Show Low Resolution", 
		PhaserAgent::ShowLowRes, true );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Show Folded", 
		PhaserAgent::ShowFolded, true );
	menuBar()->insertItem( "&View", menuView );

	Gui::Menu* menuSpec = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuSpec, d_agent, "&Rotate...", 
		PhaserAgent::SpecRotate, false );
	menuBar()->insertItem( "&Spectrum", menuSpec );

	Gui::Menu* menuSlices = new Gui::Menu( menuBar() );
 	Gui::Menu::item( menuSlices, d_agent, "Set Phase...", 
		PhaserAgent::SetPhase, false );
 	Gui::Menu::item( menuSlices, d_agent, "Set Pivot...", 
		PhaserAgent::SetPivot, false );
	menuSlices->insertSeparator();
	Gui::Menu::item( menuSlices, d_agent, "Set Bounds...", 
		PhaserAgent::SetSliceMinMax, false );
 	Gui::Menu::item( menuSlices, d_agent, "Pick Bounds", 
		PhaserAgent::PickBounds, false );
 	Gui::Menu::item( menuSlices, d_agent, "Pick Bounds Sym.", 
		PhaserAgent::PickBoundsSym, false );
 	Gui::Menu::item( menuSlices, d_agent, "Auto Scale", 
		PhaserAgent::SliceAutoScale, true, Qt::CTRL+Qt::Key_A );
	menuSlices->insertSeparator();
	Gui::Menu::item( menuSlices, d_agent, "Use Dim. X", 
		PhaserAgent::UseDimX, true );
	Gui::Menu::item( menuSlices, d_agent, "Use Dim. Y", 
		PhaserAgent::UseDimY, true );
	Gui::Menu::item( menuSlices, d_agent, "Use Dim. Z", 
		PhaserAgent::UseDimZ, true );
	menuBar()->insertItem( "&Slices", menuSlices );

	Gui::Menu* menuHelp = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuHelp, this, Root::Action::HelpAbout );
	menuBar()->insertItem( "&?", menuHelp );

}

void Phaser::buildViews( bool rebuild )
{
	if( rebuild )
		d_agent->allocate();

	SpecViewer* plane = 0;
	if( rebuild )
		plane = d_agent->createPlaneViewer();
	else
		plane = d_agent->getPlaneViewer();

	Splitter* split1 = new Splitter( 2, 2 );
	split1->setBarWidth( 80 );
	split1->setPane( new Background( plane, Qt::black, true ), 0, 0 );

	d_focus->clear();
	d_focus->setCircle( true );
	d_focus->append( plane->getController() );

	Splitter* split0 = new Splitter( 1, 1 );
	split0->setBarWidth( 80 );
	split0->setPane( split1, 0, 0 );

	const Dimension dim = d_agent->getSpec()->getDimCount();
	SpecViewer* g = 0;
	if( rebuild || d_agent->getSliceViewer( DimX ) == 0 )
		g = d_agent->createSliceViewer( DimX, DimX );
	else
		g = d_agent->getSliceViewer( DimX );
	split1->setPane( new Background( g, Qt::black, true ), 1, 0 );
	d_focus->append( g->getController() );

	Glyph* box = LayoutKit::hbox();
	split1->setPane( new Background( box, Qt::black, true ), 0, 1 );
	for( Dimension d = 1; d < dim; d++ )
	{
		if( rebuild || d_agent->getSliceViewer( d ) == 0 )
			g = d_agent->createSliceViewer( DimY, d );
		else
			g = d_agent->getSliceViewer( d );
		box->append( g );
		d_focus->append( g->getController() );
		if( d < dim - 1 )
			box->append( new Rule( DimY, g_frameClr, 40 ) );
	}
	
	g = d_agent->createPhaser();
	split1->setPane( g, 1, 1 );
	d_focus->append( g->getController() );

	d_focus->setBody( split0 );
	d_agent->initOrigin();

	split1->setRowPos( 1,  15 * d_widget->height() );
	split1->setColumnPos( 1,  15 * d_widget->width() );

	d_widget->getViewport()->damageAll();
}

void Phaser::openImag(Dimension d)
{
	QString fileName;
	fileName.sprintf( "Select Imaginary Part Spectrum Dim. %c", char( 'X' + d ) );
	fileName= QFileDialog::getOpenFileName( getQt(), fileName, AppAgent::getCurrentDir(),
        Spectrum::s_fileFilter );
    if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );
	AppAgent::setCurrentDir( info.dirPath( true ) );

	try
	{
		QApplication::setOverrideCursor( Qt::waitCursor );
		Spectrum* spec = Factory::createSpectrum( fileName );
		if( spec == 0 )
			Root::ReportToUser::alert( this, "Error opening spectrum",
				"Unknown spectrum format" );
		else
			d_agent->getSpec()->setImag( d, spec );
		QApplication::restoreOverrideCursor();
	}catch( Root::Exception& e )
	{
		QApplication::restoreOverrideCursor();
		Root::ReportToUser::alert( this, "Error Opening Spectrum", e.what() );
	}
}

void Phaser::handleAction(Root::Action& a)
{
	if( !EXECUTE_ACTION( Phaser, a ) )
		MainWindow::handleAction( a );
}

void Phaser::handle(Root::Message& m)
{
	BEGIN_HANDLER();
	HANDLE_ELSE()
		MainWindow::handle( m );
	END_HANDLER();
}

void Phaser::handleLoadImagX(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_agent->getSpec()->getImag( DimX ) );

	openImag( DimX );
}

void Phaser::handleLoadImagY(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_agent->getSpec()->getImag( DimY ) );

	openImag( DimY );
}

void Phaser::handleLoadImagZ(Action & a)
{
	ACTION_CHECKED_IF( a, d_agent->getSpec()->getDimCount() > 2, 
		d_agent->getSpec()->getDimCount() > 2 && d_agent->getSpec()->getImag( DimZ ) );

	openImag( DimZ );
}


