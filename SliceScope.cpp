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

#include "SliceScope.h"
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

static void buildCommands( CommandLine* cl )
{
	cl->registerCommand( new ActionCommand( SliceScopeAgent::CursorSyncX, "GCX", "Sync To Global X Cursor", true ) );
	cl->registerCommand( new ActionCommand( SliceScopeAgent::CursorSyncY, "GCY", "Sync To Global Y Cursor", true ) );
	cl->registerCommand( new ActionCommand( SliceScopeAgent::FitToWindow, "WW", "Fit To Window", true ) );

	ActionCommand* cmd = new ActionCommand( Root::Action::ExecuteLine, "LUA", "Lua", false );
	cmd->registerParameter( Any::Memo, true ); 
	cl->registerCommand( cmd );
}

ACTION_SLOTS_BEGIN( SliceScope )
    { SliceScopeAgent::RemoveSpec, &SliceScope::handleRemoveSpec },
    { SliceScopeAgent::AddSpec, &SliceScope::handleAddSpec },
ACTION_SLOTS_END( SliceScope )

//////////////////////////////////////////////////////////////////////

static QColor g_frameClr = Qt::lightGray;

SliceScope::SliceScope(Root::Agent * a, Spectrum* s, Project* pro, PeakList* pl ):
	Lexi::MainWindow( a, true, true )
{
	assert( s );
	assert( s->getDimCount() == 1 );
	d_agent = new SliceScopeAgent( this, s, pro, pl );
	
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
	getQt()->show();
	buildViews();
	d_agent->setSpec( 0, s, true );
	updateCaption();
	d_widget->getViewport()->damageAll();
}

SliceScope::~SliceScope()
{
	d_widget->setBody( 0 );
	d_agent = 0;
}

void SliceScope::updateCaption()
{
	QString str;
	str.sprintf( "SliceScope - %s", ( *d_agent->getSpecs().begin() )->getName() );
	getQt()->setCaption( str );
}

void SliceScope::buildMenus()
{
	Gui::Menu* menuFile = new Gui::Menu( menuBar() );
    Gui::Menu::item( menuFile, this, Root::Action::FileSave, Qt::CTRL+Qt::Key_S );
	menuFile->insertSeparator();
    Gui::Menu::item( menuFile, this, Root::Action::WindowClose, Qt::CTRL+Qt::Key_W );
	menuBar()->insertItem( "&File", menuFile );

	Gui::Menu* menuEdit = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuEdit, this, Root::Action::EditUndo, Qt::CTRL+Qt::Key_Z );
	Gui::Menu::item( menuEdit, this, Root::Action::EditRedo, Qt::CTRL+Qt::Key_Y );
	menuBar()->insertItem( "&Edit", menuEdit );

	Gui::Menu* menuView = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuView, d_agent, "Sync to Global Cursor X", 
		SliceScopeAgent::CursorSyncX, true );
	Gui::Menu::item( menuView, d_agent, "Sync to Global Cursor Y", 
		SliceScopeAgent::CursorSyncY, true );
	menuBar()->insertItem( "&View", menuView );

	Gui::Menu* menuHelp = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuHelp, this, Root::Action::HelpAbout );
	menuBar()->insertItem( "&?", menuHelp );
}

void SliceScope::buildViews()
{
	d_focus->clear();
	d_focus->setCircle( true );

	d_box = LayoutKit::vbox();
	d_focus->setBody( d_box );

	addSpecLane();
}

void SliceScope::addSpecLane()
{
	if( !d_agent->getSpecs().empty() )
		d_box->append( new Rule( DimX, g_frameClr, 40 ) );
	d_box->append( d_agent->getSlices().back().d_viewer );
	d_focus->append( d_agent->getSlices().back().d_viewer->getController() );
}

void SliceScope::handle(Root::Message& m)
{
	BEGIN_HANDLER();
	MESSAGE( Root::Action, a, m )
	{
		if( !EXECUTE_ACTION( SliceScope, *a ) )
			MainWindow::handle( m );
	}HANDLE_ELSE()
		MainWindow::handle( m );
	END_HANDLER();
}

void SliceScope::handleAddSpec(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	Lexi::Viewport::pushHourglass();
	d_agent->addSpec( dynamic_cast<Spectrum*>( a.getParam( 0 ).getObject() ) );
	addSpecLane();
	d_widget->getViewport()->damageAll();
	Lexi::Viewport::popCursor();
}

void SliceScope::handleRemoveSpec(Action & a)
{
	ACTION_ENABLED_IF( a, d_agent->getLane() != -1 && d_agent->getSlices().size() > 1 );

	int lane = d_agent->getLane();
	d_focus->remove( lane );
	d_agent->removeSpec( lane );
	lane *= 2;
	d_box->remove( lane );
	if( lane < d_box->getCount() )
		d_box->remove( lane );	// Entferne Ruler
	d_widget->getViewport()->damageAll();
}




