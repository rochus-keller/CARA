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

#include "SynchroScope2.h"

// Qt
#include <qinputdialog.h> 
#include <qmessagebox.h>
#include <qmenubar.h>

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
#include <Lexi/ContextMenu.h>
#include <Lexi/Printer.h>
#include <Spec/SpecProjector.h>
#include <Spec/SpecRotator.h>
#include <Spec/Factory.h>
#include <SpecView/IntensityView.h>
#include <SpecView/FocusCtrl.h>
#include <SpecView/OverviewCtrl.h>
#include <SpecView/PeakPlaneView.h>
#include <Dlg.h>
#include <PolyScopeAgent2.h>
using namespace Lexi;
using namespace Spec;

static QColor g_frameClr = Qt::lightGray;

//////////////////////////////////////////////////////////////////////
// Entkopplung; nutzbar ohne include

void createSynchroScope2(Root::Agent* a, Spec::Spectrum* spec, Spec::Project* p)
{ 
	Rotation rot;
	if( SpectrumType::rotateToUniqueKeys( spec, rot ) )
		spec = new SpecRotator( spec, rot );
	new SynchroScope2( a, spec, p );
}

ACTION_SLOTS_BEGIN( SynchroScope2 )
ACTION_SLOTS_END( SynchroScope2 )

//////////////////////////////////////////////////////////////////////

static void buildCommands( CommandLine* cl )
{
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::AdjustIntensity, "CW", "Adjust Intensity", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::PickPlPeak, "PP", "Pick Peak", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::MovePlPeak, "MP", "Move Peak", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::MovePlAlias, "MA", "Move Peak Alias", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::DeletePlPeaks, "DP", "Delete Peaks", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::LabelPlPeak, "LP", "Label Peak", true ) );

	cl->registerCommand( new ActionCommand( PolyScopeAgent2::DeleteAliasPeak, "UA", "Un-Alias System", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::GotoPoint, "GT", "Goto", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::ShowFolded, "SF", "Show Folded", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::AutoContour2, "ZAC", "Auto Contour Level", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::AutoContour, "AC", "Auto Contour Level", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::Show3dPlane, "3D", "Show 3D Plane", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::FitWindow3D, "ZWW", "Fit To Window", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::Backward, "BB", "Backward", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::CursorSync, "GC", "Sync To Global Cursor", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::FitWindowX, "WX", "Fit X To Window", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::FitWindowY, "WY", "Fit Y To Window", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::FitWindow, "WW", "Fit To Window", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::AutoCenter, "CC", "Center To Peak", true ) );
	cl->registerCommand( new ActionCommand( Root::Action::EditUndo, "ZZ", "Undo", true ) );
	cl->registerCommand( new ActionCommand( Root::Action::EditRedo, "YY", "Redo", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::ShowContour, "SC", "Show Contour", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::ShowIntensity, "SI", "Show Intensity", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::ForwardPlane, 
		"FP", "Forward Plane", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::BackwardPlane, 
		"BP", "Backward Plane", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::ShowLowRes, 
		"LR", "Low Resolution", true ) ); // TODO: Argument
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::ContourParams, 
		"CP", "Contour Params", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::ContourParams2, 
		"ZCP", "Contour Params", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::Show3dPlane, 
		"SP", "Show 3D Plane", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::PickSystem, 
		"PY", "Pick System", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::MovePeak, 
		"MS", "Move System", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::MovePeakAlias, 
		"AS", "Move System Alias", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::DeletePeak, 
		"DS", "Delete Systems", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::PickSpin3D, 
		"PI", "Pick Spin", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::MoveSpin3D, 
		"MI", "Move Spin", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::MoveSpinAlias3D, 
		"AI", "Move Spin Alias", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::DeleteSpins3D, 
		"DI", "Delete Spins", true ) );

	ActionCommand* cmd = new ActionCommand( PolyScopeAgent2::ForceLabelSpin3D, 
		"LI", "Force Spin Label", false );
	cmd->registerParameter( Any::CStr, false ); // Label oder leer
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::SetResolution, 
		"SR", "Set Resolution", false );
	cmd->registerParameter( Any::UShort, false ); // Leer oder Auflsung
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::GotoSystem, 
		"GY", "Goto System", false );
	cmd->registerParameter( Any::CStr, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::GotoPlPeak, 
		"GP", "Goto Peak", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cl->registerCommand( new ActionCommand( PolyScopeAgent2::NextSpec2D, 
		"NS", "Next Spectrum", true ) );
	cl->registerCommand( new ActionCommand( PolyScopeAgent2::PrevSpec2D, 
		"PS", "Prev. Spectrum", true ) );

	cmd = new ActionCommand( PolyScopeAgent2::AutoGain, 
		"AG", "Set Auto Contour Gain", false );
	cmd->registerParameter( Any::Float, true ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::AutoGain3D, 
		"ZAG", "Set Auto Contour Gain", false );
	cmd->registerParameter( Any::Float, true ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( Root::Action::ExecuteLine, "LUA", "Lua", false );
	cmd->registerParameter( Any::Memo, true ); 
	cl->registerCommand( cmd );

	cmd = new ActionCommand( PolyScopeAgent2::ViewPlLabels, 
		"LF", "Label Format", false );
	cmd->registerParameter( Any::Long, false ); // Leer oder ID
	cl->registerCommand( cmd );

	cmd = new ActionCommand( SynchroScope2::SetWidthFactor, 
		"WF", "Set Strip Width Factor", false );
	cmd->registerParameter( Any::Float, false ); 
	cl->registerCommand( cmd );
}

//////////////////////////////////////////////////////////////////////

SynchroScope2::SynchroScope2(Root::Agent * a, Spectrum* spec, Project* pro ):
	PolyScope2( a )
{

	d_agent = new PolyScopeAgent2( this, spec, pro, false, true );

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

SynchroScope2::~SynchroScope2()
{
	d_focus->setBody( 0 );

	d_ovCtrl = 0;
}

void SynchroScope2::updateCaption()
{
	QString str;
	str.sprintf( "SynchroScope - %s", d_agent->getSpec2D()->getName() );
	getQt()->setCaption( str );
}

void SynchroScope2::buildMenus()
{
	Gui::Menu* menuFile = new Gui::Menu( menuBar() );
    Gui::Menu::item( menuFile, this, Root::Action::FileSave, Qt::CTRL+Qt::Key_S );
	Gui::Menu::item( menuFile, d_agent, "Print Preview...", 
		PolyScopeAgent2::CreateReport, false, Qt::CTRL+Qt::Key_P );
	Gui::Menu* sub = new Gui::Menu( menuFile );
	Gui::Menu::item( sub, this, "&Anchor Peaklist...", ExportPeaklist, false );
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
	Gui::Menu::item( menuView, d_agent, "Backward", 
		PolyScopeAgent2::Backward, false, Qt::CTRL+Qt::Key_B );
	Gui::Menu::item( menuView, d_agent, "Forward", 
		PolyScopeAgent2::Forward, false );
	Gui::Menu::item( menuView, d_agent, "Goto Point...", 
		PolyScopeAgent2::GotoPoint, false );
	Gui::Menu::item( menuView, d_agent, "Goto System...", 
		PolyScopeAgent2::GotoSystem, false );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Set Resolution...", 
		PolyScopeAgent2::SetResolution, false );
	Gui::Menu::item( menuView, d_agent, "Show Low Resolution", 
		PolyScopeAgent2::ShowLowRes, true );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Hide 3D Slices", 
		PolyScopeAgent2::AutoHide, true );
	Gui::Menu::item( menuView, d_agent, "Show Folded", 
		PolyScopeAgent2::ShowFolded, true );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, d_agent, "Center to Peak", 
		PolyScopeAgent2::AutoCenter, true );
	Gui::Menu::item( menuView, d_agent, "Sync to Global Cursor", 
		PolyScopeAgent2::CursorSync, true );
	Gui::Menu::item( menuView, d_agent, "Sync to Depth", 
		PolyScopeAgent2::SyncDepth, true );
	menuBar()->insertItem( "&View", menuView );

	Gui::Menu* menuPlane = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuPlane, d_agent, "Calibrate from System", 
		PolyScopeAgent2::SpecCalibrate, false );
	/* TODO
	Gui::Menu::item( menuPlane, d_agent, "Calibrate from Peak", 
		PolyScopeAgent2::SpecCalibrate, false );
		*/
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
	Gui::Menu::item( menuPlane, d_agent, "Pick System", 
		PolyScopeAgent2::PickSystem, false );
	Gui::Menu::item( menuPlane, d_agent, "Move System", 
		PolyScopeAgent2::MovePeak, false );
	Gui::Menu::item( menuPlane, d_agent, "Move System Alias", 
		PolyScopeAgent2::MovePeakAlias, false );
	Gui::Menu::item( menuPlane, d_agent, "Un-Alias Systems", 
		PolyScopeAgent2::DeleteAliasPeak, false );
	Gui::Menu::item( menuPlane, d_agent, "Delete Systems", 
		PolyScopeAgent2::DeletePeak, false );
	Gui::Menu::item( menuPlane, d_agent, "&Edit Attributes...", 
		PolyScopeAgent2::EditAttsSysH, false, Qt::CTRL+Qt::Key_Return );
	menuPlane->insertSeparator();
	Gui::Menu::item( menuPlane, d_agent, "Show 3D Plane", 
		PolyScopeAgent2::Show3dPlane, true, Qt::CTRL+Qt::Key_3 );
	Gui::Menu::item( menuPlane, d_agent, "Fit Window", 
		PolyScopeAgent2::FitWindow, false, Qt::CTRL+Qt::Key_Home );
	menuBar()->insertItem( "&Plane", menuPlane );

	Gui::Menu* menuStrip = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuStrip, d_agent, "Calibrate Strip", 
		PolyScopeAgent2::StripCalibrate, false );
	Gui::Menu::item( menuStrip, d_agent, "Auto Contour Level", 
		PolyScopeAgent2::AutoContour2, true );
	Gui::Menu::item( menuStrip, d_agent, "Set Contour Parameters...", 
		PolyScopeAgent2::ContourParams2, false );

	sub = new Gui::Menu( menuBar() );
	menuStrip->insertItem( "Select Spectrum", d_agent->getPopSpec3D() );
	menuStrip->insertSeparator();
	Gui::Menu::item( menuStrip, d_agent, "&Pick Spin", 
		PolyScopeAgent2::PickSpin3D, false );
	Gui::Menu::item( menuStrip, d_agent, "Pick Label...", 
		PolyScopeAgent2::PickLabel3D, false );
	Gui::Menu::item( menuStrip, d_agent, "&Move Spin", 
		PolyScopeAgent2::MoveSpin3D, false );
	Gui::Menu::item( menuStrip, d_agent, "&Move Spin Alias", 
		PolyScopeAgent2::MoveSpinAlias3D, false );

	menuStrip->insertItem( "Label Spin", d_agent->getPopLabel() );

	Gui::Menu::item( menuStrip, d_agent, "&Select Label...", 
		PolyScopeAgent2::LabelSpin3D, false );
	Gui::Menu::item( menuStrip, d_agent, "&Force Spin Label", 
		PolyScopeAgent2::ForceLabelSpin3D, false );
	Gui::Menu::item( menuStrip, d_agent, "&Delete Spins", 
		PolyScopeAgent2::DeleteSpins3D, false );
	Gui::Menu::item( menuStrip, d_agent, "&Edit Attributes...", 
		PolyScopeAgent2::EditAttsSpin3D, false );
	menuStrip->insertSeparator();
	Gui::Menu::item( menuStrip, d_agent, "Fit Window", 
		PolyScopeAgent2::FitWindow3D, false, Qt::Key_Home );
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
	for( short i = PeakPlaneView::NONE; i < PeakPlaneView::END; i++ )
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

void SynchroScope2::buildViews()
{
	d_focus->setBody( 0 );

	Splitter* inner = new Splitter( 2, 2 );
	inner->setBarWidth( 80 );

	d_focus->clear();
	d_focus->setCircle( true );

	Splitter* outer = new Splitter( 1, 2 );
	outer->setBarWidth( 80 );
	outer->setPane( inner, 0, 0 );

	inner->setPane( d_agent->getPlane().d_viewer, 0, 1 );

	inner->setPane( d_agent->getSlice( DimX ).d_viewer, 1, 1 );

	inner->setPane( d_agent->getSlice( DimY ).d_viewer, 0, 0 );

	d_agent->getPlane().d_tuples->setLabel( SpinPointView::SystemId );

	Glyph* box = LayoutKit::hbox();
	outer->setPane( box, 0, 1 );

	box->append( d_agent->getSlice( DimZ ).d_viewer );
	box->append( new Rule( DimensionY, g_frameClr, 40 ) );
	box->append( d_agent->getStrip( DimX ).d_viewer );
	box->append( new Rule( DimensionY, g_frameClr, 40 ) );
	box->append( d_agent->getStrip( DimY ).d_viewer );

	if( d_agent->getSpec2D()->getDimCount() == 2 )
	{
		SpecViewer* ov = new SpecViewer( new ViewAreaMdl( true, true, true, true ) );
		Root::Ref<SpecProjector> pro = new SpecProjector( d_agent->getSpec(), DimX, DimY );
		Root::Ref<SpecBufferMdl> mdl = new SpecBufferMdl( ov->getViewArea(), pro );
		ov->getViews()->append( new IntensityView( mdl ) );
		d_ovCtrl = new OverviewCtrl( mdl, d_agent->getPlane().d_viewer->getViewArea() );
		ov->getHandlers()->append( d_ovCtrl );
		ov->getHandlers()->append( new FocusCtrl( ov ) );
		// mdl->copy( d_agent->getPlane().d_buf );
		// mdl->setResolution( 1 );
		inner->setPane( ov, 1, 0 );
	}else
		d_ovCtrl = 0;

	d_focus->append( d_agent->getSlice( DimY ).d_viewer->getController() );
	d_focus->append( d_agent->getPlane().d_viewer->getController() );
	d_focus->append( d_agent->getSlice( DimX ).d_viewer->getController() );
	d_focus->append( d_agent->getSlice( DimZ ).d_viewer->getController() );
	d_focus->append( d_agent->getStrip( DimX ).d_viewer->getController() );
	d_focus->append( d_agent->getStrip( DimY ).d_viewer->getController() );

	d_agent->getStrip( DimX ).d_tuples->setLabel( SpinPointView::TagAll, DimY );
	d_agent->getStrip( DimY ).d_tuples->setLabel( SpinPointView::TagAll, DimY );

	d_focus->setBody( outer );

	inner->setRowPos( 1,  17 * d_widget->height() );
	inner->setColumnPos( 1,  3 * d_widget->width() );
	outer->setColumnPos( 1,  15 * d_widget->width() );
	// TODO d_agent->postInit();

	d_widget->reallocate();
	d_agent->getPlane().d_ol[0].d_buf->fitToArea();
	d_agent->setCursor();
	d_widget->setFocusGlyph( d_agent->getPlane().d_viewer->getController() );
	if( d_ovCtrl )
	{ 
		if( d_agent->getSpec2D()->canDownsize() )
		{
			d_ovCtrl->getModel()->fitToArea();
		}else
		{
			d_ovCtrl->getModel()->copy( d_agent->getPlane().d_ol[0].d_buf );
		}
	}
}

bool SynchroScope2::askToCloseWindow() const
{
	return d_agent->askToClosePeaklist();
}

void SynchroScope2::handle(Root::Message& m)
{
	BEGIN_HANDLER();
	MESSAGE( PolyScopeAgent2::SpecChanged, e, m )
	{
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
		if( !EXECUTE_ACTION( SynchroScope2, *a ) )
			PolyScope2::handle( m );
	}HANDLE_ELSE()
		PolyScope2::handle( m );
	END_HANDLER();
}



