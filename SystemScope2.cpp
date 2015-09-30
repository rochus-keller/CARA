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

#include "SystemScope2.h"
using namespace Spec;

// Qt
#include <qtextstream.h> 
#include <qinputdialog.h> 
#include <qmessagebox.h>
#include <qdir.h> 
#include <qmenubar.h>
#include <qsplitter.h> 
#include <q3vbox.h>
#include <qcombobox.h> 
#include <Gui/ListView.h> 
#include <q3listbox.h>
#include <Q3ComboBox>
#include <QHeaderView>

#include <Root/ActionHandler.h>
#include <Root/Command.h>
#include <Root/Application.h>
#include <Root/UpstreamFilter.h>
#include <Root/MakroCommand.h>
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
#include <Lexi/ContextMenu.h>
#include <Lexi/Printer.h>

#include <Spec/SpinSystem.h>
#include <Spec/Residue.h>
#include <Spec/Repository.h>
#include <Spec/NmrExperiment.h>

#include <SpecView/IntensityView.h>
#include <SpecView/FocusCtrl.h>
#include <SpecView/OverviewCtrl.h>
#include <SpecView/SpinLineView.h>
#include <SpecView/ResidueParamView.h>

#include <SystemScopeAgent2.h>
#include <Dlg.h>
#include <SpecView/MoleculeViewer.h>
#include <SpecView/DynValueEditor.h>

using namespace Lexi;
using namespace Spec;

//////////////////////////////////////////////////////////////////////
// Entkopplung; nutzbar ohne include

void createSystemScope2(Root::Agent* a, Spec::Project* p, Spec::Spectrum* s )
{ 
	new SystemScope2( a, p, s );
}

//////////////////////////////////////////////////////////////////////

Action::CmdStr SystemScope2::StripOrder = "StripOrder";
Action::CmdStr SystemScope2::ShowStrip = "ShowStrip";
Action::CmdStr SystemScope2::ShowSpinPath = "ShowSpinPath";
Action::CmdStr SystemScope2::CreateSpin = "CreateSpin";
Action::CmdStr SystemScope2::DeleteSpin = "DeleteSpin";
Action::CmdStr SystemScope2::LabelSpin = "LabelSpin";
Action::CmdStr SystemScope2::ForceLabelSpin = "ForceLabelSpin";
Action::CmdStr SystemScope2::MoveSpin = "MoveSpin";
Action::CmdStr SystemScope2::MoveSpinAlias = "MoveSpinAlias";
Action::CmdStr SystemScope2::EditSpinAtts = "EditSpinAtts";
Action::CmdStr SystemScope2::EditSysAtts = "EditSysAtts";
Action::CmdStr SystemScope2::AcceptLabel = "AcceptLabel";
Action::CmdStr SystemScope2::GotoSystem = "GotoSystem";
Action::CmdStr SystemScope2::GotoResidue = "GotoResidue";
Action::CmdStr SystemScope2::SetWidthFactor = "SetWidthFactor";
Action::CmdStr SystemScope2::ShowRulers = "ShowRulers";
Action::CmdStr SystemScope2::SetTol = "SetTol";
Action::CmdStr SystemScope2::SetTolOrtho = "SetTolOrtho";

ACTION_SLOTS_BEGIN( SystemScope2 )
    { SystemScope2::SetTolOrtho, &SystemScope2::handleSetTolOrtho },
    { SystemScope2::SetTol, &SystemScope2::handleSetTol },
    { SystemScope2::ShowRulers, &SystemScope2::handleShowRulers },
    { SystemScope2::SetWidthFactor, &SystemScope2::handleSetWidthFactor },
    { SystemScope2::GotoResidue, &SystemScope2::handleGotoResidue },
    { SystemScope2::GotoSystem, &SystemScope2::handleGotoSystem },
    { SystemScope2::AcceptLabel, &SystemScope2::handleAcceptLabel },
    { SystemScope2::EditSpinAtts, &SystemScope2::handleEditSpinAtts },
    { SystemScope2::EditSysAtts, &SystemScope2::handleEditSysAtts },
    { SystemScopeAgent2::ShowDepth, &SystemScope2::handleShowDepth },
    { SystemScope2::MoveSpinAlias, &SystemScope2::handleMoveSpinAlias },
    { SystemScope2::CreateSpin, &SystemScope2::handleCreateSpin },
    { SystemScope2::DeleteSpin, &SystemScope2::handleDeleteSpin },
    { SystemScope2::ForceLabelSpin, &SystemScope2::handleForceLabelSpin },
    { SystemScope2::LabelSpin, &SystemScope2::handleLabelSpin },
    { SystemScope2::MoveSpin, &SystemScope2::handleMoveSpin },
    { SystemScope2::ShowStrip, &SystemScope2::handleShowStrip },
    { SystemScope2::ShowSpinPath, &SystemScope2::handleShowSpinPath },
    { SystemScope2::StripOrder, &SystemScope2::handleStripOrder },
    { SystemScopeAgent2::ShowOrthogonal, &SystemScope2::handleShowOrthogonal },
ACTION_SLOTS_END( SystemScope2 )

//////////////////////////////////////////////////////////////////////

static void buildCommands( CommandLine* cl )
{
	cl->registerCommand( new ActionCommand( SystemScope2::SetTol, "ST", "Set Strip Tolerance", true ) );
	cl->registerCommand( new ActionCommand( SystemScope2::SetTolOrtho, "OT", "Set Ortho. Tolerance", true ) );
	cl->registerCommand( new ActionCommand( SystemScopeAgent2::FitOrtho, "FF", "Fit To Window", true ) );
	cl->registerCommand( new ActionCommand( SystemScopeAgent2::FitOrthoX, "WX", "Fit To Window", true ) );
	cl->registerCommand( new ActionCommand( SystemScopeAgent2::ShowFolded, "SF", "Show Folded", true ) );
	cl->registerCommand( new ActionCommand( SystemScopeAgent2::AutoContour, "AC", "Auto Contour Level", true ) );
	cl->registerCommand( new ActionCommand( SystemScopeAgent2::CursorSync, "GC", "Sync To Global Cursor", true ) );
	cl->registerCommand( new ActionCommand( SystemScopeAgent2::FitWindow, "WY", "Fit To Window", true ) );
	cl->registerCommand( new ActionCommand( SystemScopeAgent2::FitOrtho, "WW", "Fit To Window", true ) );
	cl->registerCommand( new ActionCommand( SystemScopeAgent2::AutoCenter, "CC", "Center To Peak", true ) );
	cl->registerCommand( new ActionCommand( Root::Action::EditUndo, "ZZ", "Undo", true ) );
	cl->registerCommand( new ActionCommand( Root::Action::EditRedo, "YY", "Redo", true ) );
	cl->registerCommand( new ActionCommand( SystemScopeAgent2::ContourParams, "CP", "Contour Params", true ) );
	cl->registerCommand( new ActionCommand( SystemScopeAgent2::ShowLowRes, "LO", "Low Resolution", true ) );
	cl->registerCommand( new ActionCommand( SystemScopeAgent2::PickSpin, "PI", "Pick Spin", true ) );
	cl->registerCommand( new ActionCommand( SystemScopeAgent2::ProposeSpin, "RI", "Propose Spin", true ) );
	cl->registerCommand( new ActionCommand( SystemScopeAgent2::MoveSpin, "MI", "Move Spin", true ) );
	cl->registerCommand( new ActionCommand( SystemScopeAgent2::MoveSpin, "MS", "Move Spin", true ) );
	cl->registerCommand( new ActionCommand( SystemScopeAgent2::MoveSpinAlias, "AI", "Move Spin Alias", true ) );
	cl->registerCommand( new ActionCommand( SystemScopeAgent2::MoveOrtho, "MO", "Move Orthogonal", true ) );
	cl->registerCommand( new ActionCommand( SystemScopeAgent2::MoveOrthoAlias, "AO", "Move Orthogonal Alias", true ) );
	cl->registerCommand( new ActionCommand( SystemScopeAgent2::DeleteSpins, "DS", "Delete Spins", true ) );
	cl->registerCommand( new ActionCommand( SystemScopeAgent2::NextSpec, 
		"NS", "Next Spectrum", true ) );
	cl->registerCommand( new ActionCommand( SystemScopeAgent2::PrevSpec, 
		"PS", "Prev. Spectrum", true ) );
	cl->registerCommand( new ActionCommand( SystemScopeAgent2::DeleteLinks, 
		"DL", "Delete Spin Links", true ) );

	cl->registerCommand( new ActionCommand( SystemScopeAgent2::ShowWithOff, 
		"RP", "Resolve Projected Spins", true ) );

	ActionCommand* cmd = new ActionCommand( Root::Action::ExecuteLine, "LUA", "Lua", false );
	cmd->registerParameter( Any::Memo, true ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( SystemScopeAgent2::AutoGain, 
		"AG", "Set Auto Contour Gain", false );
	cmd->registerParameter( Any::Float, true ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( SystemScopeAgent2::PickLabel, 
		"PL", "Pick Label", false );
	cmd->registerParameter( Any::CStr, false ); // Label oder leer
	cl->registerCommand( cmd );

	cmd = new ActionCommand( SystemScope2::GotoSystem, 
		"GY", "Goto System", false );
	cmd->registerParameter( Any::CStr, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( SystemScope2::GotoSystem, 
		"GS", "Goto System", false );
	cmd->registerParameter( Any::CStr, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( SystemScope2::GotoResidue, 
		"GR", "Goto Residue", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( SystemScope2::ShowRulers, 
		"SR", "Show Rulers", true );
	cl->registerCommand( cmd );

	cmd = new ActionCommand( SystemScopeAgent2::ViewLabels, 
		"LL", "Label Format", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( SystemScope2::SetWidthFactor, 
		"WF", "Set Strip Width Factor", false );
	cmd->registerParameter( Any::Float, false ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( SystemScopeAgent2::SetDepth, 
		"SD", "Set Peak Depth", false );
	cmd->registerParameter( Any::Float, false ); 
	cl->registerCommand( cmd );



}

//////////////////////////////////////////////////////////////////////

static char g_buffer[ 64 ];

struct _SpinItem3 : public Gui::ListViewItem
{
	_SpinItem3( Gui::ListView * parent, const SpinPoint& t ):
		Gui::ListViewItem( parent ), d_tuple( t ) {}
	_SpinItem3( Gui::ListView * parent, Spin* s ):
		Gui::ListViewItem( parent ) { d_tuple[ DimX ] = s; }
	SpinPoint d_tuple;

	QString text( int f ) const
	{
		int t = f / 3;
		if( d_tuple[ t ] == 0 )
			return "<error>"; // TEST
		switch( f % 3 )
		{
		case 0:
			::sprintf( g_buffer, "%d", d_tuple[ t ]->getId() );
			return g_buffer;
		case 1:
			::sprintf( g_buffer, "%.2f %s", d_tuple[ t ]->getShift(),
				d_tuple[ t ]->getAtom().getIsoLabel() ); // RISK
			return g_buffer;
		case 2:
			d_tuple[ t ]->getLabel().format( g_buffer, sizeof(g_buffer) );
			return g_buffer;
		}
		return "";
	}
	QString key( int f, bool ascending  ) const
	{
		int t = f / 3;
		if( d_tuple[ t ] == 0 )
			return "<error>"; // TEST
		switch( f % 3 )
		{
		case 0:
			::sprintf( g_buffer, "%08d", d_tuple[ t ]->getId() );
			return g_buffer;
		case 1:
			::sprintf( g_buffer, "%08.2f", double( d_tuple[ t ]->getShift() ) ); // RISK
			return g_buffer;
		case 2:
			d_tuple[ t ]->getLabel().format( g_buffer, sizeof(g_buffer) );
			return g_buffer;
		}
		return "";
	}
};

//////////////////////////////////////////////////////////////////////

struct _SysSelectorItem2 : public Q3ListBoxText
{
	_SysSelectorItem2( Q3ListBox* p, SpinSystem* sys, bool stripOrder ):
		Q3ListBoxText( p ), d_sys( sys ), d_stripOrder( stripOrder ) {}
	Root::ExRef<SpinSystem> d_sys;
	bool d_stripOrder;
	QString text() const
	{
		if( d_sys.isNull() )
		{
			if( d_stripOrder )
				return "  (select spin system)";
			else
				return "  (select residue)";
		}
		QString str;
		Residue* ass;
		ass = d_sys->getAssig();
		if( ass != 0 )
		{
			d_sys->formatLabel( g_buffer, sizeof(g_buffer) );
			if( d_stripOrder )
				str.sprintf( "%03d -> %s %d", d_sys->getId(), g_buffer, ass->getId() );
			else
				str.sprintf( "%03d %s -> %d", ass->getId(), g_buffer, d_sys->getId() );
		}else
			str.sprintf( "%03d ", d_sys->getId() );	// Space wichtig
		return str;
	}
};

struct _TypeSelectorItem2 : public Q3ListBoxText
{
	_TypeSelectorItem2( Q3ListBox* p, ResidueType* rt ):
		Q3ListBoxText( p ), d_rt( rt ) {}
	Root::ExRef<ResidueType> d_rt;
	QString text() const
	{
		if( d_rt.isNull() )
			return "  (select residue type)";
		else
			return d_rt->getShort().data();
	}
};

//////////////////////////////////////////////////////////////////////
// Diese Klass ist ein Dirty-Trick um zu verhindern, dass BbSpinSystemScope
// als Event-Sink mit connect eingetragen werden muss. Sobald davon
// ein Meta-Objekt existiert, strtzt andauernd.

_Redirector2::_Redirector2( SystemScope2* parent ):
	  QObject( parent->getQt() ), d_parent( parent ) {}

void _Redirector2::handleSelectSys(int i)
{
	SelectSystem msg( i );
	d_parent->traverse( msg );
}
void _Redirector2::handleSelectType(int i)
{
	SelectType msg( i );
	d_parent->traverse( msg );
}
void _Redirector2::doubleClicked( Gui::ListViewItem * i ) 
{
	SelectAnchor msg;
	d_parent->traverse( msg );
}

//////////////////////////////////////////////////////////////////////

static QColor g_frameClr = Qt::lightGray;

SystemScope2::SystemScope2(Root::Agent* a,Project* pro,Spectrum* spec):
	Lexi::MainWindow( a, true, true ), d_curSystem( 0 ), d_curType( 0 ), d_agent( 0 )
{
	d_agent = new SystemScopeAgent2( this, pro, spec );
	pro->addObserver( this );
	d_agent->getModel()->addObserver( this );

	loadSetup();

	QSplitter* splitH = new QSplitter( getQt() );
	getQt()->setCentralWidget( splitH );
	Q3VBox* vbox = new Q3VBox( splitH );
	Q3HBox* hbox = new Q3HBox( vbox );
	d_sysSelector = new Q3ComboBox( false, hbox );
	d_typeSelector = new Q3ComboBox( false, hbox );

	d_spins = new Gui::ListView( vbox );	
	d_spins->header()->setResizeMode( QHeaderView::ResizeToContents );
	d_spins->setRootIsDecorated( false );
	d_spins->setAllColumnsShowFocus( true );
	d_spins->addColumn( "ID" );
	d_spins->addColumn( "Shift" );
	d_spins->addColumn( "Label" );

	d_bonds = new Gui::ListView( vbox );
	d_bonds->header()->setResizeMode( QHeaderView::ResizeToContents );
	d_bonds->setRootIsDecorated( false );
	d_bonds->setAllColumnsShowFocus( true );
	for( Dimension d = 0; d < d_agent->getModel()->getDimCount(); d++ )
	{
		d_bonds->addColumn( "ID" );
		d_bonds->addColumn( "Shift" );
		d_bonds->addColumn( "Label" );
	}

	// TODO: irgend ein Agent oberhalb soll Undo bieten: d_undo = new UndoManager( this );
	Root::UpstreamFilter* filter = new UpstreamFilter( this, true );

	d_focus = new FocusManager( nil, true );
	Redirector* redir = new Redirector( new Background( d_focus, Qt::black, true ) );
	CommandLine* cl = new CommandLine( d_agent );
	redir->setKeyHandler( cl );
	buildCommands( cl );

	d_widget = new GlyphWidget( splitH, new Bevel( redir, true, false ), filter, true );
	d_widget->setFocusGlyph( redir );

	buildMenus();
	buildPopups();

	// splitH->setResizeMode( vbox, QSplitter::FollowSizeHint );
	getQt()->resize( 600, 400 ); // RISK
	getQt()->showMaximized();

	QList<int> s;
	s << getQt()->width() * 0.2;
	s << getQt()->width() * 0.8;
	splitH->setSizes( s );
	d_widget->setMinimumWidth( getQt()->width() * 0.4 );
	splitH->setCollapsible( 1, false );
	splitH->setOpaqueResize(false);

	buildViews();

	_Redirector2* tmp = new _Redirector2( this );
	getQt()->connect( d_sysSelector, SIGNAL( activated(int) ), tmp, SLOT( handleSelectSys( int ) ) );
	getQt()->connect( d_typeSelector, SIGNAL( activated(int) ), tmp, SLOT( handleSelectType( int ) ) );

	getQt()->connect( d_bonds, SIGNAL( returnPressed ( Gui::ListViewItem * ) ), 
		tmp, SLOT( doubleClicked( Gui::ListViewItem * ) ) );
	getQt()->connect( d_bonds, SIGNAL( doubleClicked( Gui::ListViewItem * ) ), 
		tmp, SLOT( doubleClicked( Gui::ListViewItem * ) ) );

	reloadSystemSelector();
	d_widget->getViewport()->damageAll();
	d_spins->setSorting( 0, true );
	d_bonds->setSorting( 0, true );
	updateCaption();
}

SystemScope2::~SystemScope2()
{
	if( d_agent->getModel() )
		d_agent->getModel()->removeObserver( this );
	if( d_agent->getProject() )
		d_agent->getProject()->removeObserver( this );

	d_widget->setBody( 0 );
}

void SystemScope2::buildViews()
{
	Splitter* inner = new Splitter( 1, 3 );
	inner->setBarWidth( 80 );

	d_focus->clear();
	d_focus->setCircle( false );

	inner->setPane( d_agent->getSlice().d_viewer, 0, 0 );
	d_focus->append( d_agent->getSlice().d_viewer->getController() );

	Layer* strips = new Layer();
	inner->setPane( strips, 0, 1 );
	d_resiView = new ResidueParamView( d_agent->getStrip().d_viewer ); // RISK
	d_resiView->show( false );
	strips->append( d_agent->getStrip().d_viewer );
	strips->append( d_resiView );
	
	// Das funktioniert: inner->setPane( d_agent->getStrip().d_viewer, 0, 1 );

	d_focus->append( d_agent->getStrip().d_viewer->getController() );

	d_orthoBox = LayoutKit::hbox();
	inner->setPane( d_orthoBox, 0, 2 );

	d_focus->setBody( inner );
	inner->setColumnPos( 1,  4 * d_widget->width() );
	inner->setColumnPos( 2,  7 * d_widget->width() );
	d_widget->setFocusGlyph( d_agent->getStrip().d_viewer->getController() );
}

void SystemScope2::buildMenus()
{
	Gui::Menu* menuFile = new Gui::Menu( menuBar() );
    Gui::Menu::item( menuFile, this, Root::Action::FileSave, Qt::CTRL+Qt::Key_S );
	menuFile->insertSeparator();
    Gui::Menu::item( menuFile, this, Root::Action::WindowClose, Qt::CTRL+Qt::Key_W );
	menuBar()->insertItem( "&File", menuFile );

	Gui::Menu* menuEdit = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuEdit, this, Root::Action::EditUndo, Qt::CTRL+Qt::Key_Z );
	Gui::Menu::item( menuEdit, this, Root::Action::EditRedo, Qt::CTRL+Qt::Key_Y );
	menuEdit->insertSeparator();
	Gui::Menu::item( menuEdit, d_agent, "Set System Attributes...", 
		SystemScope2::EditSysAtts, false );
	menuBar()->insertItem( "&Edit", menuEdit );

	Gui::Menu* menuView = new Gui::Menu( menuBar() );
	menuView->insertItem( "Select Spectrum", d_agent->getPopSpec() );
	Gui::Menu::item( menuView, d_agent, "Set Contour Parameters...", 
		SystemScopeAgent2::ContourParams, false );
	Gui::Menu::item( menuView, d_agent, "Auto Contour Level", 
		SystemScopeAgent2::AutoContour, true );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Order by Strip", 
		SystemScope2::StripOrder, true );
	Gui::Menu::item( menuView, this, "Show Rulers", 
		SystemScope2::ShowRulers, true );
	Gui::Menu::item( menuView, d_agent, "Set Resolution...", 
		SystemScopeAgent2::SetResolution, false );
	Gui::Menu::item( menuView, d_agent, "Show Low Resolution", 
		SystemScopeAgent2::ShowLowRes, true );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Resolve Projected Spins", 
		SystemScopeAgent2::ShowWithOff, true );
	Gui::Menu::item( menuView, d_agent, "Show Spin Links", 
		SystemScopeAgent2::ShowLinks, true );
	Gui::Menu::item( menuView, d_agent, "Show Infered Peaks", 
		SystemScopeAgent2::ShowInfered, true );
	Gui::Menu::item( menuView, d_agent, "Show Unlabeled Peaks", 
		SystemScopeAgent2::ShowUnlabeled, true );
	Gui::Menu::item( menuView, d_agent, "Show Peaks with unknown Labels", 
		SystemScopeAgent2::ShowUnknown, true );
	Gui::Menu::item( menuView, d_agent, "Do Pathway Simulation", 
		SystemScopeAgent2::DoPathSim, true );
	Gui::Menu::item( menuView, d_agent, "Show Ghost Peaks", 
		SystemScopeAgent2::ShowGhosts, true );
	Gui::Menu::item( menuView, d_agent, "Show Ghost Labels", 
		SystemScopeAgent2::GhostLabels, true );
	Gui::Menu::item( menuView, d_agent, "Show Hidden Links", 
		SystemScopeAgent2::ShowAllPeaks, true );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Show Folded", 
		SystemScopeAgent2::ShowFolded, true );
	Gui::Menu::item( menuView, d_agent, "Show Vertical Cursor", 
		SystemScopeAgent2::ShowVCursor, true );
	Gui::Menu::item( menuView, d_agent, "Center to Peak", 
		SystemScopeAgent2::AutoCenter, true );
	Gui::Menu::item( menuView, d_agent, "Sync to Global Cursor", 
		SystemScopeAgent2::CursorSync, true );
	Gui::Menu::item( menuView, d_agent, "Fit Window", 
		SystemScopeAgent2::FitWindow, false, Qt::Key_Home );
	menuBar()->insertItem( "&View", menuView );

	Gui::Menu* menuStrip = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuStrip, d_agent, "Calibrate Strip", 
		SystemScopeAgent2::StripCalibrate, false );
	menuStrip->insertSeparator();
	Gui::Menu::item( menuStrip, d_agent, "&Pick Spin", 
		SystemScopeAgent2::PickSpin, false );
	Gui::Menu::item( menuStrip, d_agent, "Pick Label...", 
		SystemScopeAgent2::PickLabel, false );
	Gui::Menu::item( menuStrip, d_agent, "Propose Spin...", 
		SystemScopeAgent2::ProposeSpin, false );
	Gui::Menu::item( menuStrip, d_agent, "Hide/Show Link", 
		SystemScopeAgent2::HidePeak, false );
	Gui::Menu::item( menuStrip, d_agent, "Set Tolerance...", 
		SystemScope2::SetTol, false );
	Gui::Menu::item( menuStrip, d_agent, "&Move Spin", 
		SystemScopeAgent2::MoveSpin, false );
	Gui::Menu::item( menuStrip, d_agent, "&Move Spin Alias", 
		SystemScopeAgent2::MoveSpinAlias, false );

	menuStrip->insertItem( "Label Spin", d_agent->getPopLabel() );

	Gui::Menu::item( menuStrip, d_agent, "&Force Spin Label", 
		SystemScopeAgent2::ForceLabelSpin, false );
	Gui::Menu::item( menuStrip, d_agent, "&Delete Spins", 
		SystemScopeAgent2::DeleteSpins, false );
	Gui::Menu::item( menuStrip, d_agent, "Delete Spin Links", 
		SystemScopeAgent2::DeleteLinks, false );
	Gui::Menu* menuAtts = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuAtts, d_agent, "Spin...", 
		SystemScopeAgent2::EditAtts, false, Qt::CTRL+Qt::Key_Return );
	Gui::Menu::item( menuAtts, d_agent, "System...", 
		SystemScopeAgent2::EditAttsSys, false );
	Gui::Menu::item( menuAtts, d_agent, "Spin Link...", 
		SystemScopeAgent2::EditAttsLink, false );
	menuStrip->insertItem( "&Edit Attributes", menuAtts );
	menuStrip->insertSeparator();
	Gui::Menu::item( menuStrip, d_agent, "Show &Orthogonal", 
		SystemScopeAgent2::ShowOrthogonal, false );
	Gui::Menu::item( menuStrip, d_agent, "Show &Depth", 
		SystemScopeAgent2::ShowDepth, false );
	Gui::Menu* sub = new Gui::Menu( menuBar() );
	menuStrip->insertItem( "Show Labels", sub );
	short i;
	for( i = SpinPointView::None; i < SpinPointView::End; i++ )
	{
		Gui::Menu::item( sub, d_agent, SpinPointView::menuText[ i ], 
			SystemScopeAgent2::ViewLabels, true )->addParam( i );
	}
	menuBar()->insertItem( "&Strip", menuStrip );

	Gui::Menu* menuOrtho = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuOrtho, d_agent, "Calibrate Orthogonal", 
		SystemScopeAgent2::OrthoCalibrate, false );
	menuOrtho->insertSeparator();
	Gui::Menu::item( menuOrtho, d_agent, "&Pick Spin", 
		SystemScopeAgent2::PickOrtho, false );
	Gui::Menu::item( menuOrtho, d_agent, "Pick Label...", 
		SystemScopeAgent2::PickLabelOrtho, false );
	Gui::Menu::item( menuOrtho, d_agent, "Propose Spin...", 
		SystemScopeAgent2::ProposeOrtho, false );
	Gui::Menu::item( menuOrtho, d_agent, "Set Tolerance...", 
		SystemScope2::SetTolOrtho, false );
	Gui::Menu::item( menuOrtho, d_agent, "&Move Spin", 
		SystemScopeAgent2::MoveOrtho, false );
	Gui::Menu::item( menuOrtho, d_agent, "&Move Spin Alias", 
		SystemScopeAgent2::MoveOrthoAlias, false );

	menuOrtho->insertItem( "Label Spin", d_agent->getPopLabelOrtho() );

	Gui::Menu::item( menuOrtho, d_agent, "&Force Spin Label", 
		SystemScopeAgent2::ForceLabelOrtho, false );
	Gui::Menu::item( menuOrtho, d_agent, "&Delete Spins", 
		SystemScopeAgent2::DeleteOrthos, false );
	Gui::Menu::item( menuOrtho, d_agent, "Edit Attributes...", 
		SystemScopeAgent2::EditAttsOrtho, false );
	menuOrtho->insertSeparator();
	sub = new Gui::Menu( menuBar() );
	menuOrtho->insertItem( "Show Labels", sub );
	for( i = SpinPointView::None; i < SpinPointView::End; i++ )
	{
		Gui::Menu::item( sub, d_agent, SpinPointView::menuText[ i ], 
			SystemScopeAgent2::ViewLabels2, true )->addParam( i );
	}
	Gui::Menu::item( menuOrtho, d_agent, "Fit Window", 
		SystemScopeAgent2::FitOrtho, false );
	menuBar()->insertItem( "&Orthogonal", menuOrtho );

	Gui::Menu* menuSlices = new Gui::Menu( menuBar() );
 	Gui::Menu::item( menuSlices, d_agent, "Set Bounds...", 
		SystemScopeAgent2::SetBounds, false );
 	Gui::Menu::item( menuSlices, d_agent, "Pick Bounds", 
		SystemScopeAgent2::PickBounds, false );
 	Gui::Menu::item( menuSlices, d_agent, "Pick Bounds Sym.", 
		SystemScopeAgent2::PickBoundsSym, false );
 	Gui::Menu::item( menuSlices, d_agent, "Auto Scale", 
		SystemScopeAgent2::AutoSlice, true );
	menuBar()->insertItem( "&Slices", menuSlices );

	Gui::Menu* menuHelp = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuHelp, this, Root::Action::HelpAbout );
	menuBar()->insertItem( "&?", menuHelp );

}

void SystemScope2::buildPopups()
{
	Gui::Menu* popSpin = new Gui::Menu( d_spins, true );
	Gui::Menu::item( popSpin, this, "&Create Spin", CreateSpin, false );
	Gui::Menu::item( popSpin, this, "&Move Spin", MoveSpin, false );
	Gui::Menu::item( popSpin, this, "&Move Spin Alias", MoveSpinAlias, false );
	Gui::Menu::item( popSpin, this, "&Label Spin", LabelSpin, false );
	Gui::Menu::item( popSpin, this, "&Force Spin Label", ForceLabelSpin, false );
	Gui::Menu::item( popSpin, this, "&Accept Label", AcceptLabel, false );
	Gui::Menu::item( popSpin, this, "&Delete Spin", DeleteSpin, false );
	Gui::Menu::item( popSpin, this, "Edit Attributes...", EditSpinAtts, false );

	Gui::Menu* popBond = new Gui::Menu( d_bonds, true );
	Gui::Menu::item( popBond, this, "Show Strip", ShowStrip, false );
	Gui::Menu::item( popBond, this, "Show Spin Path...", ShowSpinPath, false );
	//popBond->insertSeparator();

}

void SystemScope2::reloadSystemSelector()
{
	Q3ListBox* lb = d_typeSelector->listBox();
	d_typeSelector->clear();
	const Repository::ResidueTypeMap& rtm = 
		d_agent->getProject()->getRepository()->getResidueTypes();
	Repository::ResidueTypeMap::const_iterator p1;
	new _TypeSelectorItem2( lb, 0 );
	for( p1 = rtm.begin(); p1 != rtm.end(); ++p1 )
	{
		new _TypeSelectorItem2( lb, (*p1).second );
	}
	lb->sort();

	d_sysSelector->clear();
	lb = d_sysSelector->listBox();
	new _SysSelectorItem2( lb, 0, d_stripOrder );
	if( d_stripOrder )
	{
		const SpinBase::SpinSystemMap& sel = 
			d_agent->getProject()->getSpins()->getSystems();
		SpinBase::SpinSystemMap::const_iterator pos;
		for( pos = sel.begin(); pos != sel.end(); ++pos )
		{
			new _SysSelectorItem2( lb, (*pos).second, d_stripOrder );
		}
		d_sysSelector->setCurrentItem( 0 );
	}else
	{
		const Sequence::ResidueMap& sel = 
			d_agent->getProject()->getSequence()->getResi();
		Sequence::ResidueMap::const_iterator pos;
		SpinSystem* ass;
		for( pos = sel.begin(); pos != sel.end(); ++pos )
		{
			ass = (*pos).second->getSystem();
			if( ass != 0 )
			{
				new _SysSelectorItem2( lb, ass, d_stripOrder );
			}
		}
		d_sysSelector->setCurrentItem( 0 );
	}
	d_curSystem = 0;
	curSystemSet();
}

void SystemScope2::selectAnchor()
{
	Allocation aa;
	d_orthoBox->allocation( aa );
	while( d_orthoBox->getCount() )
		d_orthoBox->remove( 0 );
	d_widget->getViewport()->damage( aa );
	d_agent->killOrthos();
	// TODO Focus
	_SpinItem3* item = dynamic_cast<_SpinItem3*>( d_bonds->currentItem() );
	assert( item );
	d_agent->setAnchor( item->d_tuple );
}

void SystemScope2::loadSetup()
{
	d_stripOrder = true;
}

void SystemScope2::adjustTol( Dimension dim )
{
	AtomType atom;
	atom = d_agent->getSpec()->getColor( dim );

	bool ok	= FALSE;
	QString res;
	res.sprintf( "%0.3f", d_agent->getProject()->getMatcher()->getTol( atom ) );
	QString str;
	str.sprintf( "Please enter atom positive PPM value for %s:", atom.getIsoLabel() );
	res	= QInputDialog::getText( "Set Spin Tolerance", str, QLineEdit::Normal, res, &ok, getQt() );
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

void SystemScope2::reloadSpins()
{
	d_spins->clear();
	if( d_curSystem == 0 )
		return;
	const SpinSystem::Spins& sel = d_curSystem->getSpins();
	SpinSystem::Spins::const_iterator pos;
	SpinPoint t;
	for( pos = sel.begin(); pos != sel.end(); ++pos )
	{
		t[ DimX ] = (*pos);
		new _SpinItem3( d_spins, t );
	}
}

void SystemScope2::reloadAnchors()
{
	d_bonds->clear();
	SpinSpace* space = d_agent->getModel();
	SpinSpace::Element e;
	SpinSpace::Iterator pos;

	for( pos = space->begin(); pos != space->end(); ++pos )
	{
		space->fetch( pos, e );
		if( !e.isGhost() && !e.isHidden() )
			new _SpinItem3( d_bonds, e.d_point );
	}
}

void SystemScope2::curSystemSet()
{
	d_curType = 0;
	if( d_curSystem && d_curSystem->getAssig() )
		d_curType = d_curSystem->getAssig()->getType();
	d_agent->getModel()->setSystem( d_curSystem, d_curType );

	Q3ListBox* lb = d_typeSelector->listBox();
	_TypeSelectorItem2* item = 0;
    for( int i = 0; i < int(lb->count()); i++ )
	{
		item = static_cast<_TypeSelectorItem2*>( lb->item( i ) );
		if( item->d_rt == d_curType )
		{
			d_typeSelector->setCurrentItem( i );
			break;
		}
	}

	if( d_curSystem && d_curSystem->getAssig() )
		d_resiView->setResidue( d_curSystem->getAssig(), false );
	else
		d_resiView->setType( d_curType, false );
	d_agent->getStrip().d_viewer->redraw(); 

	reloadSpins();
	reloadAnchors();
}

static _SysSelectorItem2* findItem( Q3ListBox* lb, SpinSystem* sys )
{
	_SysSelectorItem2* item = 0;
    for( int i = 0; i < int(lb->count()); i++ )
	{
		item = static_cast<_SysSelectorItem2*>( lb->item( i ) );
		if( item->d_sys == sys )
			return item;
	}
	return 0;
}

static _SpinItem3* findItem( Gui::ListView* lv, Spin* spin )
{

    Gui::ListViewItemIterator it( lv );
    // iterate through all items of the listview
	_SpinItem3* i = 0;
    for ( ; it.current(); ++it ) 
	{
		i = static_cast<_SpinItem3*>( it.current() );
		if( i && i->d_tuple[ 0 ] == spin )
			return i;
    }
	return 0;
}

void SystemScope2::handle(Root::Message& msg)
{
	BEGIN_HANDLER();
	MESSAGE( _Redirector2::SelectSystem, e, msg )
	{
		msg.consume();
		Q3ListBox* lb = d_sysSelector->listBox();
		_SysSelectorItem2* i = static_cast<_SysSelectorItem2*>( lb->item( e->d_sel ) );
		if( i )
			d_curSystem = i->d_sys;
		curSystemSet();
	}
	MESSAGE( _Redirector2::SelectType, e, msg )
	{
		msg.consume();
		Q3ListBox* lb = d_typeSelector->listBox();
		_TypeSelectorItem2* i = static_cast<_TypeSelectorItem2*>( lb->item( e->d_sel ) );
		if( i )
		{
			d_curType = i->d_rt;
			d_agent->getModel()->setSystem( d_curSystem, d_curType );
			d_resiView->setType( d_curType, false );
			d_agent->getStrip().d_viewer->redraw(); 
			reloadAnchors();
		}
	}
	MESSAGE( _Redirector2::SelectAnchor, e, msg )
	{
        Q_UNUSED(e)
        msg.consume();
		selectAnchor();
	}
	MESSAGE( Residue::Added, e, msg )
    {
        Q_UNUSED(e)
        reloadSystemSelector();
    }
	MESSAGE( Residue::Removed, e, msg )
    {
        Q_UNUSED(e)
        reloadSystemSelector();
    }
	MESSAGE( Residue::Changed, e, msg )
    {
        Q_UNUSED(e)
        reloadSystemSelector();
    }
	MESSAGE( Spin::Update, a, msg )
	{
		msg.consume();
		if( a->getType() == Spin::Update::All )
				reloadSpins();
		else if( a->getSpin()->getSystem() == d_curSystem )
		{
			d_spins->repaint();
			switch( a->getType() )
			{
			case Spin::Update::Shift:
			case Spin::Update::Label:
				{
					_SpinItem3* i = findItem( d_spins, a->getSpin() );
					if( i )
						i->repaint();
				}
				break;
            default:
                break;
			}
		}
	}
	MESSAGE( SpinSystem::Update, a, msg )
	{
		msg.consume();
		if( a->getType() == SpinSystem::Update::All )
			reloadSystemSelector(); 
		else if( a->getType() == SpinSystem::Update::Create )
		{
			if( d_stripOrder || a->getSystem()->getAssig() )
				new _SysSelectorItem2( d_sysSelector->listBox(), a->getSystem(), d_stripOrder );
		}else if( a->getSystem() == d_curSystem )
		{
			switch( a->getType() )
			{
			case SpinSystem::Update::Delete:
				{
					_SysSelectorItem2* i = 
						findItem( d_sysSelector->listBox(), a->getSystem() );
					if( i && i->d_sys == d_curSystem )
					{
						d_curSystem = 0;
						curSystemSet();
					}
					delete i;
				}
				break;
			case SpinSystem::Update::Assig:
				d_sysSelector->repaint();
				break;
			case SpinSystem::Update::Add:
				new _SpinItem3( d_spins, a->getSpin() );
				break;
			case SpinSystem::Update::Remove:
				{
					_SpinItem3* i = findItem( d_spins, a->getSpin() );
					if( i )
						i->removeMe();
				}
				break;
            default:
                break;
			}
		}
	}
	MESSAGE( SpinSpace::Update, a, msg )
	{
		if( a->getOrigin() == d_agent->getModel() )
		{
			msg.consume();
			reloadAnchors(); // RISK: ev. eingrenzen
		}
	}
	MESSAGE( Root::Action, a, msg )
	{
		if( !EXECUTE_ACTION( SystemScope2, *a ) )
			MainWindow::handle( msg );
	}HANDLE_ELSE()
		MainWindow::handle( msg );
	END_HANDLER();
}

void SystemScope2::updatePopup()
{
}

void SystemScope2::updateCaption()
{
	QString str;
	str.sprintf( "SystemScope - %s", d_agent->getSpec()->getName() );
	getQt()->setCaption( str );
}

void SystemScope2::handleStripOrder(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_stripOrder );
	d_stripOrder = !d_stripOrder;
	reloadSystemSelector();
}

void SystemScope2::handleShowDepth(Action & a)
{
	ACTION_ENABLED_IF( a, !d_agent->getAnchor().isZero() &&
		d_agent->getStrip().d_viewer->hasFocus() );

	Viewport::pushHourglass();
	Allocation aa;
	d_orthoBox->allocation( aa );
	while( d_orthoBox->getCount() )
		d_orthoBox->remove( 0 );
	d_agent->createOrthos( true );
	const SystemScopeAgent2::Orthos& os = d_agent->getOrthos();
	for( int i = 0; i < os.size(); i++ )
	{
		if( i != 0 )
			d_orthoBox->append( new Rule( DimY, g_frameClr, 40 ) );
		d_orthoBox->append( os[ i ].d_viewer );
		// TODO d_focus->append( os[ i ].d_viewer->getController() );
	}
	d_widget->getViewport()->damage( aa );
	Viewport::popCursor();
}

void SystemScope2::handleShowOrthogonal(Action & a)
{
	ACTION_ENABLED_IF( a, !d_agent->getAnchor().isZero() &&
		!d_agent->getStrip().d_tuples->getSel().empty() &&
		d_agent->getStrip().d_viewer->hasFocus() &&
		d_agent->getSpec()->getColor( DimY ) == d_agent->getSpec()->getColor( DimX ) );

	Viewport::pushHourglass();
	Allocation aa;
	d_orthoBox->allocation( aa );
	while( d_orthoBox->getCount() )
		d_orthoBox->remove( 0 );
	d_agent->createOrthos();
	const SystemScopeAgent2::Orthos& os = d_agent->getOrthos();
	for( int i = 0; i < os.size(); i++ )
	{
		if( i != 0 )
			d_orthoBox->append( new Rule( DimY, g_frameClr, 40 ) );
		d_orthoBox->append( os[ i ].d_viewer );
		// TODO d_focus->append( os[ i ].d_viewer->getController() );
	}
	d_widget->getViewport()->damage( aa );
	Viewport::popCursor();
}

void SystemScope2::handleShowStrip(Action & a)
{
	ACTION_ENABLED_IF( a, d_bonds->currentItem() );
	selectAnchor();
}

void SystemScope2::handleShowSpinPath(Action & a)
{
	ACTION_ENABLED_IF( a, d_curType && d_bonds->currentItem() );

	_SpinItem3* item = dynamic_cast<_SpinItem3*>( d_bonds->currentItem() );
	assert( item );

	MoleculeViewer* mv = new MoleculeViewer( 0, d_curType, false );
	SpinSpace* ss = d_agent->getSpace();
	SpectrumType* t = ss->getSpecType();
    Q_ASSERT( t != 0 );
	const int y_nr = t->getStepNr( ss->getSpec()->mapToType( DimY ) );
	const int x_nr = t->getStepNr( ss->getSpec()->mapToType( DimX ) );
	const int z_nr = t->getStepNr( ss->getSpec()->mapToType( DimZ ) );
	const int xy = Root::Math::abs( y_nr - x_nr );
	const int zy = Root::Math::abs( y_nr - z_nr );
	Dimension other = DimUndefined;
	if( xy == 1 && zy == 1 )
	{
		if( x_nr < y_nr )
			other = DimX;
		else if( z_nr < y_nr )
			other = DimY; // Y weil wir Bond auslesen
	}else if( xy == 1 )
		other = DimX;
	else if( zy == 1 )
		other = DimY;

	if( other != DimUndefined )
	{
		NmrExperiment* nmr = d_agent->getProject()->getRepository()->getTypes()->inferExperiment4( t, d_curType );
		assert( nmr );
		SpinLabelSet res;
        PathTable::Path path;
		assert( item->d_tuple[ other ] );
		// other * 2 ist ein Dirty Trick damit aus DimY DimZ wird.
		path[ ss->getSpec()->mapToType( other * 2 ) ] = 
			item->d_tuple[ other ]->getLabel();
		nmr->getColumn( path, ss->getSpec()->mapToType( DimY ), res );
		mv->select( item->d_tuple[ other ]->getLabel(), res );
	}
	mv->show();
}

void SystemScope2::handleCreateSpin(Action & a)
{
	ACTION_ENABLED_IF( a, d_curSystem );

	Dlg::SpinParams sp;
	if( !Dlg::getSpinParams( getQt(), sp ) )
		return;
	Root::Ref<PickSystemSpinLabelCmd> cmd = new PickSystemSpinLabelCmd( 
		d_agent->getProject()->getSpins(), 
		d_curSystem, 
		sp.d_type, sp.d_shift, sp.d_label, 0 ); 
		// Pick generisches Spektrum
	cmd->handle( this );
}

void SystemScope2::handleDeleteSpin(Action & a)
{
	ACTION_ENABLED_IF( a, d_spins->currentItem() );

	_SpinItem3* item = dynamic_cast<_SpinItem3*>( d_spins->currentItem() );
	assert( item );

	Root::Ref<DeleteSpinCmd> cmd = new DeleteSpinCmd( 
		d_agent->getProject()->getSpins(), item->d_tuple[ DimX ] );
	cmd->handle( this );
}

void SystemScope2::handleForceLabelSpin(Action & a)
{
	ACTION_ENABLED_IF( a, d_spins->currentItem() );

	_SpinItem3* item = dynamic_cast<_SpinItem3*>( d_spins->currentItem() );
	assert( item );
	bool ok	= FALSE;
	QString res;
	res.sprintf( "Please enter a valid label (%s):", SpinLabel::s_syntax );
	res = QInputDialog::getText( "Force Spin Label", res, QLineEdit::Normal, 
			item->d_tuple[ DimX ]->getLabel().data(), &ok, getQt() );
		if( !ok )
			return;

	SpinLabel l;
	if( !SpinLabel::parse( res, l ) )
	{
		Root::ReportToUser::alert( this, "Force Spin Label", "Invalid spin label syntax!" );
		return;
	}

	Root::Ref<LabelSpinCmd> cmd =
		new LabelSpinCmd( d_agent->getProject()->getSpins(), item->d_tuple[ DimX ], l );
	cmd->handle( this );
}

void SystemScope2::handleLabelSpin(Action & a)
{
	ACTION_ENABLED_IF( a, d_spins->currentItem() );

	_SpinItem3* item = dynamic_cast<_SpinItem3*>( d_spins->currentItem() );
	assert( item );
	Spin* spin = item->d_tuple[ DimX ];
	SpinLabel l = spin->getLabel();

	ResidueType* r = d_agent->getProject()->inferResiType( spin->getSystem() );
	SpinLabelSet ll;
	if( r )
		ll = r->findAll( spin->getAtom() );
	if( !Dlg::getLabel( d_spins, l, ll ) )
		return;
	Root::Ref<LabelSpinCmd> cmd =
		new LabelSpinCmd( d_agent->getProject()->getSpins(), spin, l );
	cmd->handle( this );
}

void SystemScope2::handleMoveSpin(Action & a)
{
	ACTION_ENABLED_IF( a, d_spins->currentItem() );

	_SpinItem3* item = dynamic_cast<_SpinItem3*>( d_spins->currentItem() );
	assert( item );
	bool ok;
	QString res;
	res.sprintf( "%f", item->d_tuple[ DimX ]->getShift() );
	res = QInputDialog::getText( "Move Spin", 
		"Please	enter a new ppm value for spin:", QLineEdit::Normal, 
		res, &ok, d_widget );
	if( !ok )
		return;
	PPM p = res.toFloat( &ok );
	if( !ok )
	{
		Root::ReportToUser::alert( this, "Move Spin", "Invalid ppm value!" );
		return;
	}

	Root::Ref<MoveSpinCmd> cmd =
		new MoveSpinCmd( d_agent->getProject()->getSpins(), item->d_tuple[ DimX ], p, 0 );
		// Move generisches Spektrum
	cmd->handle( this );
}

void SystemScope2::handleMoveSpinAlias(Action & a)
{
	ACTION_ENABLED_IF( a, d_spins->currentItem() );

	_SpinItem3* item = dynamic_cast<_SpinItem3*>( d_spins->currentItem() );
	assert( item );
	bool ok;
	QString res;
	res.sprintf( "%f", item->d_tuple[ DimX ]->getShift( d_agent->getSpec() ) );
	res = QInputDialog::getText( "Move Spin Alias", 
		"Please	enter a new ppm value for spin alias:", QLineEdit::Normal, 
		res, &ok, d_widget );
	if( !ok )
		return;
	PPM p = res.toFloat( &ok );
	if( !ok )
	{
		Root::ReportToUser::alert( this, "Move Spin Alias", "Invalid ppm value!" );
		return;
	}

	Root::Ref<MoveSpinCmd> cmd =
		new MoveSpinCmd( d_agent->getProject()->getSpins(), 
			item->d_tuple[ DimX ], p, d_agent->getSpec() );
	cmd->handle( this );
}

void SystemScope2::handleEditSpinAtts(Action & a)
{
	ACTION_ENABLED_IF( a, d_spins->currentItem() );

	_SpinItem3* item = dynamic_cast<_SpinItem3*>( d_spins->currentItem() );
	assert( item );

	DynValueEditor::edit( getQt(), 
		d_agent->getProject()->getRepository()->findObjectDef( Repository::keySpin ), 
		item->d_tuple[ DimX ] );
}

void SystemScope2::handleEditSysAtts(Action & a)
{
	ACTION_ENABLED_IF( a, d_curSystem );

	DynValueEditor::edit( getQt(), 
		d_agent->getProject()->getRepository()->findObjectDef( 
		Repository::keySpinSystem ), d_curSystem );
}

void SystemScope2::handleAcceptLabel(Action & a)
{
	if( d_spins->currentItem() == 0 )
		return;
	_SpinItem3* item = dynamic_cast<_SpinItem3*>( d_spins->currentItem() );
	ACTION_ENABLED_IF( a, item && item->d_tuple[ 0 ]->getSystem() &&
		!item->d_tuple[ 0 ]->getLabel().isNull() &&
		!item->d_tuple[ 0 ]->getLabel().isFinal() );

	SpinSystem* sys = item->d_tuple[ 0 ]->getSystem();
	SpinSystem::Spins::const_iterator i;
	SpinLabel l;
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Accept Label" ); 
	for( i = sys->getSpins().begin(); i != sys->getSpins().end(); i++ )
	{
		if( (*i) != item->d_tuple[ 0 ] && 
			(*i)->getLabel() == item->d_tuple[ 0 ]->getLabel() )
			cmd->add( new LabelSpinCmd( d_agent->getProject()->getSpins(), (*i), l ) );
	}
	l = item->d_tuple[ 0 ]->getLabel();
	l.setFinal();
	cmd->add( new LabelSpinCmd( d_agent->getProject()->getSpins(), item->d_tuple[ 0 ], l ) );
	cmd->handle( this );
}

void SystemScope2::handleGotoSystem(Action & a)
{
	ACTION_ENABLED_IF( a, true ); 

	QString id;
	if( a.getParam( 0 ).isNull() )
	{
		bool ok	= FALSE;
		id	= QInputDialog::getText( "Goto System", 
			"Please	enter a spin system id:", QLineEdit::Normal, "", &ok, getQt() );
		if( !ok )
			return;
	}else
		id = a.getParam( 0 ).getCStr();

	SpinSystem* sys = d_agent->getProject()->getSpins()->findSystem( id );
	if( sys == 0 )
	{
		Lexi::ShowStatusMessage msg( "Goto System: unknown system" );
		traverse( msg );
		return;
	}

	Q3ListBox* lb = d_sysSelector->listBox();
	_SysSelectorItem2* item;
    for( int i = 0; i < int(lb->count()); i++ )
	{
		item = (_SysSelectorItem2*) lb->item( i );
		if( item && item->d_sys == sys )
		{
			d_sysSelector->setCurrentItem( i );
			d_curSystem = sys;
			curSystemSet();
			break;
		}
	}
}

void SystemScope2::handleGotoResidue(Action & a)
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
	SpinSystem* sys = resi->getSystem();

	Q3ListBox* lb = d_sysSelector->listBox();
	_SysSelectorItem2* item;
    for( int i = 0; i < int(lb->count()); i++ )
	{
		item = (_SysSelectorItem2*) lb->item( i );
		if( item && item->d_sys == sys )
		{
			d_sysSelector->setCurrentItem( i );
			d_curSystem = sys;
			curSystemSet();
			break;
		}
	}
}

void SystemScope2::handleSetWidthFactor(Action & a)
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

void SystemScope2::handleShowRulers(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_resiView->show() );

	d_resiView->show( !d_resiView->show() );
	d_agent->getStrip().d_viewer->redraw(); 
}

void SystemScope2::handleSetTolOrtho(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	adjustTol( DimZ );
}

void SystemScope2::handleSetTol(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	adjustTol( DimY );
}




