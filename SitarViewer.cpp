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

#include "SitarViewer.h"
#include <qinputdialog.h> 
#include <qmessagebox.h>
#include <qfileinfo.h> 
#include <qdir.h> 
#include <qmenubar.h>
#include <qcolordialog.h>
#include <qtextstream.h> 
#include <Root/UpstreamFilter.h>
#include <Lexi/FocusManager.h>
#include <Lexi/LayoutKit.h>
#include <QColor>
#include <Lexi/CommandLine.h>
#include <Lexi/Splitter.h>
#include <Lexi/Redirector.h>
#include <Lexi/Background.h>
#include <Lexi/Bevel.h>
#include <Lexi/Interactor.h>
#include <Gui/Menu.h>
#include <Lexi/ContextMenu.h>
#include <Lexi/Printer.h>
#include <Lexi/Shapes.h>
#include <Lexi/SelectorCmd.h>
#include <Lexi/Label.h>
#include <Spec/SitarSpectrum.h>
#include <Spec/SpecProjector.h>
#include <Spec/SpecRotator.h>
#include <SpecView/CursorMdl.h>
#include <SpecView/ViewAreaMdl.h>
#include <SpecView/SpecViewer.h>
#include <SpecView/IntensityView.h>
#include <SpecView/FocusCtrl.h>
#include <SpecView/ContourView.h>
#include <SpecView/CursorView.h>
#include <SpecView/CursorCtrl.h>
#include <SpecView/ScrollCtrl.h>
#include <SpecView/ZoomCtrl.h>
#include <SpecView/SelectZoomCtrl.h>
#include <SpecView/SelectRulerCtr.h>
#include <SpecView/SliceView.h>
#include <SpecView/FoldingView.h>
#include <Spec/GlobalCursor.h>
#include <SpecView/CenterLine.h>
#include <SpecView/OverviewCtrl.h>
#include <SpecView/SpinPointView.h>
#include <Dlg.h>
using namespace Spec;
using namespace Root;
using namespace Lexi;

static const int BACKGROUND = 0;
static const int INTENSITY = 1;
static const int CONTOUR = 2;
static const int FOLDING = 3;
static const int CURSORS = 4;
static const int PEAKS = 5;
static const int LABEL1 = 6;
static const int LABEL2 = 7;
static const int VIEWCOUNT = 8;

static const float s_contourFactor = 1.4f;
static const ContourView::Option s_contourOption = ContourView::Both;
static const float s_gain = 2.0;
static const bool s_autoContour = true;
static QColor g_frameClr = Qt::lightGray;
static QColor g_posClr = Qt::blue;
static QColor g_negClr = Qt::cyan;
static QColor g_curClr = Qt::darkYellow;

//////////////////////////////////////////////////////////////////////

void createSitarViewer( Root::Agent* a, Spec::Spectrum* s )
{
	assert( a );
	SitarSpectrum* sitar = dynamic_cast<SitarSpectrum*>( s );
	if( sitar == 0 )
		Root::ReportToUser::alert( a, "Error opening spectrum",
			"This is not a sitar spectrum!" );
    else
        new SitarViewer( a, s );
}

//////////////////////////////////////////////////////////////////////

static const Action::CmdStr AutoContour = "AutoContour";
static const Action::CmdStr ContourParams = "ContourParams";
static const Action::CmdStr AutoContour2 = "AutoContour2";
static const Action::CmdStr ContourParams2 = "ContourParams2";
static const Action::CmdStr FitWindow2 = "FitWindow2";
static const Action::CmdStr FitWindow = "FitWindow";
static const Action::CmdStr SetPeakWidht = "SetPeakWidht";
static const Action::CmdStr RangeSync = "RangeSync";
static const Action::CmdStr CursorSync = "CursorSync";
static const Action::CmdStr ShowCurve = "ShowCurve";
static const Action::CmdStr MatchGrad = "MatchGrad";
static const Action::CmdStr Interpolate = "Interpolate";
static const Action::CmdStr Clip = "Clip";
static const Action::CmdStr LineSlices = "LineSlices";
static const Action::CmdStr SyncDepth = "SyncDepth";
static const Action::CmdStr ShowLhs = "ShowLhs";
static const Action::CmdStr ShowRhs = "ShowRhs";
static const Action::CmdStr Rectify = "Rectify";
static const Action::CmdStr OneOff = "OneOff";
static const Action::CmdStr ShowSumSlice = "ShowSumSlice";

ACTION_SLOTS_BEGIN( SitarViewer )
    { ShowSumSlice, &SitarViewer::handleShowSumSlice },
    { OneOff, &SitarViewer::handleOneOff },
    { Rectify, &SitarViewer::handleRectify },
    { ShowRhs, &SitarViewer::handleShowRhs },
    { ShowLhs, &SitarViewer::handleShowLhs },
    { AutoContour, &SitarViewer::handleAutoContour },
    { ContourParams, &SitarViewer::handleContourParams },
    { SyncDepth, &SitarViewer::handleSyncDepth },
    { LineSlices, &SitarViewer::handleLineSlices },
    { Clip, &SitarViewer::handleClip },
    { Interpolate, &SitarViewer::handleInterpolate },
    { MatchGrad, &SitarViewer::handleMatchGrad },
    { ShowCurve, &SitarViewer::handleShowCurve },
    { FitWindow, &SitarViewer::handleFitWindow },
    { RangeSync, &SitarViewer::handleRangeSync },
    { CursorSync, &SitarViewer::handleCursorSync },
    { SetPeakWidht, &SitarViewer::handleSetPeakWidht },
    { AutoContour2, &SitarViewer::handleAutoContour2 },
    { ContourParams2, &SitarViewer::handleContourParams2 },
    { FitWindow2, &SitarViewer::handleFitWindow2 },
ACTION_SLOTS_END( SitarViewer )

//////////////////////////////////////////////////////////////////////

class _Selected : public Root::Message
{
public:
	_Selected( PpmRange r ):d_range( r ) {}
	PpmRange d_range;
};

class _Stepped : public Root::Message
{
public:
	_Stepped():d_delta(0) {}
	PPM d_delta;
};

class _NSelectorCmd : public SelectorCmd
{
	Root::Ptr<SpecViewer> d_view;
public:
	_NSelectorCmd( Glyph* ctrl, SpecViewer* m ):
	  SelectorCmd( ctrl ), d_view( m ) 
	{
	}
	void execute()
	{
		Allocation rect;
		getAllocation( rect );
		if( rect.getWidth() != 0 )
		{
			ViewAreaMdl* v = d_view->getViewArea();
			Allocation a = v->getAllocation();
			PpmRange rx;
			rx.first = v->toPpm( rect.getLeft(), a.getLeft(), DimX );
			rx.second = v->toPpm( rect.getRight(), a.getLeft(), DimX );
			_Selected msg( rx );
			d_view->getViewport()->traverse( msg );
		}
		Command::execute();
	}
}; 

class _NSelector : public Lexi::GlyphCommander 
{
public:
	_NSelector(SpecViewer* v):d_view(v){}

	bool handleMousePress( Lexi::Viewport&, 
		const Lexi::Allocation&, const Lexi::MouseEvent& e )
	{
		if( e.isLeft() && !e.isCtrl() && e.isShift() && !e.isAlt() )
		{
			return installCommand( new _NSelectorCmd( this, d_view ) ); 
		}else
			return false;
	}
	bool handleKeyPress( Viewport& v, const Allocation&, const KeyEvent& e )
	{
		if( e.isShift() && !e.isCtrl() && !e.isAlt() &&
			( e.isLeft() || e.isRight() || e.isUp() || e.isDown() ) )
		{
			_Stepped msg;
			if( e.isLeft() )
				msg.d_delta = -20.0 / d_view->getViewArea()->getTwipPerPpm( DimX );
			else if( e.isRight() )
				msg.d_delta = +20.0 / d_view->getViewArea()->getTwipPerPpm( DimX );
			else if( e.isDown() )
				msg.d_delta = -200.0 / d_view->getViewArea()->getTwipPerPpm( DimX );
			else if( e.isUp() )
				msg.d_delta = +200.0 / d_view->getViewArea()->getTwipPerPpm( DimX );
			d_view->getViewport()->traverse( msg );
			return true;
		}else
			return false;
	}
private:
	Root::Ptr<SpecViewer> d_view;
};

//////////////////////////////////////////////////////////////////////

SitarViewer::SitarViewer(Root::Agent * a, Spectrum* s):
	Lexi::MainWindow( a, true, true ), d_lock( false ), 
	d_cursorSync( true ), d_rangeSync(false), d_syncDepth( true ),
	d_sliceSum(0), d_bufSum(0), d_specSum(0)
{

	d_spec = dynamic_cast<SitarSpectrum*>( s );
	assert( d_spec );
	d_cursor.assign( 3, 0 );
	d_off = 0.0;
	d_cursor[DimZ] = d_spec->getNFromHOff( d_off );
	d_pw[DimX] = 0.15f;
	d_pw[DimY] = 1.5f;

	buildPopups();
	Root::UpstreamFilter* filter = new UpstreamFilter( this, true );

	d_focus = new FocusManager( nil, true );
	Redirector* redir = new Redirector( new Background( d_focus,Qt::black, true ) );
	CommandLine* cl = new CommandLine( this );
	redir->setKeyHandler( cl );
	// buildCommands( cl );

	d_widget = new GlyphWidget( getQt(), new Bevel( redir, true, false ), filter, true );
	getQt()->setCentralWidget( d_widget );
	d_widget->setFocusGlyph( redir );

	buildMenus();
	getQt()->resize( 600, 400 ); // RISK
	getQt()->showMaximized();
	updateCaption();
	buildViews();

	adjustN();

	if( d_cursorSync )
		GlobalCursor::addObserver( this );	
}

SitarViewer::~SitarViewer()
{
	GlobalCursor::removeObserver( this );	
	d_widget->setBody( 0 );
}
 
void SitarViewer::buildMenus()
{
	Gui::Menu* menuFile = new Gui::Menu( getQt()->menuBar() );
    Gui::Menu::item( menuFile, this, Root::Action::FileSave, Qt::CTRL+Qt::Key_S );
	menuFile->insertSeparator();
    Gui::Menu::item( menuFile, this, Root::Action::WindowClose, Qt::CTRL+Qt::Key_W );
	menuBar()->insertItem( "&File", menuFile );

	Gui::Menu* menuEdit = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuEdit, this, Root::Action::EditUndo, Qt::CTRL+Qt::Key_Z );
	Gui::Menu::item( menuEdit, this, Root::Action::EditRedo, Qt::CTRL+Qt::Key_Y );
	menuBar()->insertItem( "&Edit", menuEdit );

	Gui::Menu* menuView = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuView, this, "Sync to Global Zoom", RangeSync, true );
	Gui::Menu::item( menuView, this, "Sync to Global Cursor", CursorSync, true );
	Gui::Menu::item( menuView, this, "Sync to Depth", SyncDepth, true );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, this, "Show Sum Slice", ShowSumSlice, true );
	Gui::Menu::item( menuView, this, "Show Line Slices", LineSlices, true );
	menuBar()->insertItem( "&View", menuView );

	Gui::Menu* menuPlane = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuPlane, this, "Auto Contour Level", AutoContour, true );
	Gui::Menu::item( menuPlane, this, "Set Contour Parameters...", ContourParams, false );
	menuPlane->insertSeparator();
	Gui::Menu::item( menuPlane, this, "Show Curve", ShowCurve, true, Qt::CTRL+Qt::Key_U );
	Gui::Menu::item( menuPlane, this, "Show Left Projection", ShowLhs, true, Qt::CTRL+Qt::Key_L );
	Gui::Menu::item( menuPlane, this, "Show Right Projection", ShowRhs, true, Qt::CTRL+Qt::Key_R );
	menuPlane->insertSeparator();
	Gui::Menu::item( menuPlane, this, "Fit Window", FitWindow, false );
	menuBar()->insertItem( "&Plane", menuPlane );

	Gui::Menu* menuStrip = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuStrip, this, "Auto Contour Level", AutoContour2, true );
	Gui::Menu::item( menuStrip, this, "Set Contour Parameters...", ContourParams2, false );
	Gui::Menu::item( menuStrip, this, "Set Peak Width...", SetPeakWidht, false );
	Gui::Menu::item( menuStrip, this, "Fit Window", FitWindow2, false, Qt::Key_Home );
	menuBar()->insertItem( "&Strips", menuStrip );

	Gui::Menu* menuSitar = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuSitar, this, "Interpolate", Interpolate, true );
	Gui::Menu::item( menuSitar, this, "Rectify Left and Right", Rectify, true );
	Gui::Menu::item( menuSitar, this, "Clip Other Sign", Clip, true );
	Gui::Menu::item( menuSitar, this, "Off By One", OneOff, true );
	Gui::Menu::item( menuSitar, this, "Match Gradient", MatchGrad, true );
	menuBar()->insertItem( "Si&tar", menuSitar );

	Gui::Menu* menuHelp = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuHelp, this, Root::Action::HelpAbout );
	menuBar()->insertItem( "&?", menuHelp );
}

void SitarViewer::buildPopups()
{

}

void SitarViewer::buildViews()
{
	d_lock = true;
	createPlane();
	d_slices.assign( 3, SliceSocket() );
	createSlice( DimX, DimX );
	d_slices[ DimX ].d_specL = new SpecProjector( d_spec->getLhs(), DimX );
	d_slices[ DimX ].d_bufL->setSpectrum( d_slices[ DimX ].d_specL, false );
	d_slices[ DimX ].d_specR = new SpecProjector( d_spec->getRhs(), DimX );
	d_slices[ DimX ].d_bufR->setSpectrum( d_slices[ DimX ].d_specR, false );

	d_slices[ DimX ].d_lhs = new CursorView( d_slices[ DimX ].d_viewer, 0 );
	d_slices[ DimX ].d_lhs->setColor( g_curClr );
	d_slices[ DimX ].d_viewer->getViews()->replace( PEAKS, d_slices[ DimX ].d_lhs );

	d_slices[ DimX ].d_rhs = new CursorView( d_slices[ DimX ].d_viewer, 0 );
	d_slices[ DimX ].d_rhs->setColor( g_curClr );
	d_slices[ DimX ].d_viewer->getViews()->replace( FOLDING, d_slices[ DimX ].d_rhs );

	d_specSum = new SpecProjector( d_spec, DimX );
	d_bufSum = new SpecBufferMdl( d_slices[ DimX ].d_viewer->getViewArea(), 0, false );
	d_sliceSum = new SliceView( d_bufSum );
	d_sliceSum->setColor( g_frameClr );
	d_slices[ DimX ].d_viewer->getViews()->replace( LABEL2, d_sliceSum );

	createSlice( DimY, DimY );
	d_slices[ DimY ].d_specL = new SpecProjector( d_spec->getLhs(), DimY );
	d_slices[ DimY ].d_bufL->setSpectrum( d_slices[ DimY ].d_specL, false );
	d_slices[ DimY ].d_specR = new SpecProjector( d_spec->getRhs(), DimY );
	d_slices[ DimY ].d_bufR->setSpectrum( d_slices[ DimY ].d_specR, false );

	d_strips.assign( 2, StripSocket() );
	createSlice( DimY, DimZ );
	createStrip( DimX );
	createStrip( DimY );

	d_slices[ DimZ ].d_specL = new SpecProjector( d_spec, DimY );
	d_slices[ DimZ ].d_bufL->setSpectrum( d_slices[ DimZ ].d_specL, false );
	d_strips[ DimX ].d_spec = new SpecProjector( d_spec, DimX, DimY );
	d_strips[ DimX ].d_buf->setSpectrum( d_strips[ DimX ].d_spec, false );
	d_strips[ DimY ].d_spec = new SpecProjector( d_spec, DimZ, DimY );
	d_strips[ DimY ].d_buf->setSpectrum( d_strips[ DimY ].d_spec, false );

	syncStripsToCur();
	d_lock = false;

	Splitter* inner = new Splitter( 2, 2 );
	inner->setBarWidth( 80 );

	d_focus->clear();
	d_focus->setCircle( true );

	Splitter* outer = new Splitter( 1, 2 );
	outer->setBarWidth( 80 );
	outer->setPane( inner, 0, 0 );

	inner->setPane( d_plane.d_viewer, 0, 1 );

	inner->setPane( d_slices[ DimX ].d_viewer, 1, 1 );

	inner->setPane( d_slices[ DimY ].d_viewer, 0, 0 );

	Glyph* box = LayoutKit::hbox();
	outer->setPane( box, 0, 1 );

	box->append( d_slices[ DimZ ].d_viewer );
	box->append( new Rule( DimensionY, g_frameClr, 40 ) );
	box->append( d_strips[ DimX ].d_viewer );
	box->append( new Rule( DimensionY, g_frameClr, 40 ) );
	box->append( d_strips[ DimY ].d_viewer );

	d_ov = new SpecViewer( new ViewAreaMdl( true, true, true, true ) );
	Root::Ref<SpecProjector> pro = new SpecProjector( d_spec->getLhs(), DimX, DimY );
	Root::Ref<SpecBufferMdl> mdl = new SpecBufferMdl( d_ov->getViewArea(), pro );
	d_ov->getViews()->append( new IntensityView( mdl ) );
	d_ovCtrl = new OverviewCtrl( mdl, d_plane.d_viewer->getViewArea() );
	d_ov->getHandlers()->append( d_ovCtrl );
	d_ov->getHandlers()->append( new FocusCtrl( d_ov ) );
	inner->setPane( d_ov, 1, 0 );

	d_focus->append( d_slices[ DimY ].d_viewer->getController() );
	d_focus->append( d_plane.d_viewer->getController() );
	d_focus->append( d_slices[ DimX ].d_viewer->getController() );
	d_focus->append( d_slices[ DimZ ].d_viewer->getController() );
	d_focus->append( d_strips[ DimX ].d_viewer->getController() );
	d_focus->append( d_strips[ DimY ].d_viewer->getController() );

	d_focus->setBody( outer );


	inner->setRowPos( 1,  17 * d_widget->height() );
	inner->setColumnPos( 1,  3 * d_widget->width() );
	outer->setColumnPos( 1,  15 * d_widget->width() );

	d_widget->reallocate();
	d_plane.d_bufL->fitToArea();
	// TODO d_agent->setCursor();
	d_widget->setFocusGlyph( d_plane.d_viewer->getController() );

	if( d_ov )
	{ 
		if( d_spec->getLhs()->canDownsize() )
		{
			d_ovCtrl->getModel()->fitToArea();
		}else
		{
			d_ovCtrl->getModel()->copy( d_plane.d_bufL );
		}
	}
	d_widget->getViewport()->damageAll();
}

void SitarViewer::updateCaption()
{
	QString str;
	str.sprintf( "SitarViewer - %s", d_spec->getName() );
	getQt()->setCaption( str );
}

void SitarViewer::handle(Root::Message& msg)
{
	if( d_lock )
		return;
	d_lock = true;
	BEGIN_HANDLER();
	MESSAGE( ViewAreaMdl::Update, a, msg )
	{
		Lexi::Viewport::pushHourglass();
		if( a->getOrigin() == d_plane.d_viewer->getViewArea() )
			updatePlane( a );
		else
		{
			Dimension d;
			for( d = 0; d < d_slices.size(); d++ )
				if( d_slices[ d ].d_viewer->getViewArea() == a->getOrigin() )
				{
					updateSlice( d, a );
					break;
				}
			for( d = 0; d < d_strips.size(); d++ )
				if( d_strips[ d ].d_viewer->getViewArea() == a->getOrigin() )
				{
					updateStrip( d, a );
					break;
				}
		}

		Lexi::Viewport::popCursor();
		msg.consume();
	}
	MESSAGE( _Selected, a, msg )
	{
        //PPM c = a->d_range.getCenter();
		PPM w = a->d_range.getWidth();
		d_cursor[DimZ] = d_spec->getNFromHOff( w );
		d_off = w / 2.0;
		setOffCursor( d_cursor[DimX] );
		adjustN();
		msg.consume();
	}
	MESSAGE( _Stepped, a, msg )
	{
		d_off += a->d_delta;
		if( d_off < 0.0 )
			d_off = 0.0;
		d_cursor[DimZ] = d_spec->getNFromHOff( 2.0 * d_off );
		setOffCursor( d_cursor[DimX] );
		adjustN();
		msg.consume();
	}
	MESSAGE( CursorMdl::Update, a, msg )
	{
		if( a->getOrigin() == d_plane.d_cur )
			updatePlane( a );
		else
		{
			Dimension d;
			for( d = 0; d < d_slices.size(); d++ )
				if( d_slices[ d ].d_cur == a->getOrigin() )
				{
					updateSlice( d, a );
					break;
				}
			for( d = 0; d < d_strips.size(); d++ )
				if( d_strips[ d ].d_cur == a->getOrigin() )
				{
					updateStrip( d, a );
					break;
				}
		}
		if( a->getDim() == DimX || a->getDim() == DimUndefined )
		{
			setOffCursor( a->getX() );
		}
		msg.consume();
	}
	MESSAGE( GlobalCursor::UpdatePos, a, msg )
	{
		d_lock = false;
		d_cursorSync = false;
		if( ( a->getDim() == DimY || a->getDim() == DimUndefined ) &&
			d_plane.d_specL->getColor( DimY ) == a->getTy() )
			d_plane.d_cur->setCursor( Dimension( DimY ), a->getY() );
		if( ( a->getDim() == DimY || a->getDim() == DimUndefined ) &&
			d_spec->getColor( DimZ ) == a->getTy() )
		{
			d_cursor[DimZ] = a->getY();
			d_off = d_spec->getHOffFromN( d_cursor[DimZ] ) * 0.5;
			setOffCursor( d_cursor[DimX] );
			adjustN();
		}
		if( ( a->getDim() == DimX || a->getDim() == DimUndefined ) &&
			d_plane.d_specL->getColor( DimX ) == a->getTx() )
			d_plane.d_cur->setCursor( Dimension( DimX ), a->getX() );

		if( d_syncDepth && ( a->getDim() == DimY ) &&
			d_spec->getColor( DimY ) == a->getTy() )
		{
			d_plane.d_cur->setCursor( Dimension( DimY ), a->getY() );
			centerY();
		}
		if( d_plane.d_curve->isVisible() )
			d_plane.d_viewer->redraw();
		d_cursorSync = true;
		msg.consume();
	}
	MESSAGE( GlobalCursor::UpdateRange, a, msg )
	{
		d_lock = false;
		d_rangeSync = false;
		if( ( a->getDim() == DimY || a->getDim() == DimUndefined ) &&
			d_plane.d_specL->getColor( DimY ) == a->getTy() )
			d_plane.d_viewer->getViewArea()->setRange( DimY, a->getY() );
		if( ( a->getDim() == DimX || a->getDim() == DimUndefined ) &&
			d_plane.d_specL->getColor( DimX ) == a->getTx() )
			d_plane.d_viewer->getViewArea()->setRange( DimX, a->getX() );
		d_rangeSync = true;
		msg.consume();
	}
	MESSAGE( Root::Action, a, msg )
	{
		d_lock = false; // Kein Blocking fr Action-Ausfhrung
		EXECUTE_ACTION( SitarViewer, *a );
	}
	HANDLE_ELSE()
		MainWindow::handle( msg );
	END_HANDLER();
	d_lock = false;
}

void SitarViewer::createPlane()
{
	d_plane.d_viewer = new SpecViewer( new ViewAreaMdl( true, true, true, true ), VIEWCOUNT );
	d_plane.d_viewer->getViewArea()->addObserver( this );

	d_plane.d_specL = new SpecProjector( d_spec->getLhs(), DimX, DimY );
	d_plane.d_bufL = 
		new SpecBufferMdl( d_plane.d_viewer->getViewArea(), d_plane.d_specL, false );
	d_plane.d_viewL = new ContourView( d_plane.d_bufL, s_autoContour );
	d_plane.d_viewer->getViews()->replace( CONTOUR, d_plane.d_viewL );
	d_plane.d_viewL->setVisi( true );
	d_plane.d_viewL->createLevelsAuto( s_contourFactor, s_contourOption, s_gain );

	d_plane.d_specR = new SpecProjector( d_spec->getRhs(), DimX, DimY );
	d_plane.d_bufR = 
		new SpecBufferMdl( d_plane.d_viewer->getViewArea(), d_plane.d_specR, false );
	d_plane.d_viewR = new ContourView( d_plane.d_bufR, s_autoContour );
	d_plane.d_viewer->getViews()->replace( INTENSITY, d_plane.d_viewR );
	d_plane.d_viewR->setVisi( true );
	d_plane.d_viewR->setPosColor( g_posClr );
	d_plane.d_viewR->setNegColor( g_negClr );
	d_plane.d_viewR->createLevelsAuto( s_contourFactor, s_contourOption, s_gain );

	d_plane.d_lhs = new CursorView( d_plane.d_viewer, 0 );
	d_plane.d_lhs->setColor( g_curClr );
	d_plane.d_viewer->getViews()->replace( PEAKS, d_plane.d_lhs );
	d_plane.d_rhs = new CursorView( d_plane.d_viewer, 0 );
	d_plane.d_rhs->setColor( g_curClr );
	d_plane.d_viewer->getViews()->replace( FOLDING, d_plane.d_rhs );

	d_plane.d_cur = new CursorMdl();
	d_plane.d_cur->addObserver( this );
	CursorView* cv = new CursorView( d_plane.d_viewer, d_plane.d_cur );
	d_plane.d_viewer->getViews()->replace( CURSORS, cv );

	d_plane.d_curve = new SitarCurveView( d_plane.d_viewer, d_spec, cv );
	d_plane.d_viewer->getViews()->replace( LABEL1, d_plane.d_curve );

	d_plane.d_viewer->getHandlers()->append( new ZoomCtrl( d_plane.d_viewer, true, true ) );
	d_plane.d_viewer->getHandlers()->append( new SelectZoomCtrl( d_plane.d_viewer, true, true ) );
	d_plane.d_viewer->getHandlers()->append( new ScrollCtrl( d_plane.d_viewer ) );
	d_plane.d_viewer->getHandlers()->append( new CursorCtrl( cv, false ) );
	d_plane.d_viewer->getHandlers()->append( new _NSelector( d_plane.d_viewer ) );
	// d_plane.d_viewer->getHandlers()->append( new Lexi::ContextMenu( d_popPlane, false ) );
	d_plane.d_viewer->getHandlers()->append( new FocusCtrl( d_plane.d_viewer ) );

}

void SitarViewer::createStrip(Dimension d)
{
	d_strips[ d ].d_viewer = new SpecViewer( 
		new ViewAreaMdl( true, true, false, true ), VIEWCOUNT );
	d_strips[ d ].d_viewer->getViewArea()->addObserver( this );

	d_strips[ d ].d_buf = new SpecBufferMdl( d_strips[ d ].d_viewer->getViewArea(), 0, false );

	d_strips[ d ].d_view = new ContourView( d_strips[ d ].d_buf, true );	// Immer auto
	d_strips[ d ].d_viewer->getViews()->replace( BACKGROUND, new Lexi::Background() );
	d_strips[ d ].d_viewer->getViews()->replace( CONTOUR, d_strips[ d ].d_view );
	d_strips[ d ].d_view->createLevelsAuto( s_contourFactor, s_contourOption, s_gain );
	d_strips[ d ].d_viewer->getViews()->replace( INTENSITY, 
		new CenterLine( d_strips[ d ].d_viewer ) );

	d_strips[ d ].d_cur = new CursorMdl();
	d_strips[ d ].d_cur->addObserver( this );
	CursorView* cv = new CursorView( d_strips[ d ].d_viewer, 
		d_strips[ d ].d_cur, false, true );
	d_strips[ d ].d_viewer->getViews()->replace( CURSORS, cv );

	d_strips[ d ].d_viewer->getHandlers()->append( new ZoomCtrl( d_strips[ d ].d_viewer, false, true ) );
	d_strips[ d ].d_viewer->getHandlers()->append( new SelectZoomCtrl( d_strips[ d ].d_viewer, false, true ) );
	d_strips[ d ].d_viewer->getHandlers()->append( new ScrollCtrl( d_strips[ d ].d_viewer, false, true ) );
	d_strips[ d ].d_viewer->getHandlers()->append( new CursorCtrl( cv, false, false, true ) );
	// d_strips[ d ].d_viewer->getHandlers()->append( new Lexi::ContextMenu( d_popStrip, false ) );
	d_strips[ d ].d_viewer->getHandlers()->append( new FocusCtrl( d_strips[ d ].d_viewer ) );
}

void SitarViewer::createSlice(Dimension view, Dimension spec )
{
	d_slices[ spec ].d_viewer = new SpecViewer( 
		new ViewAreaMdl( view == DimX, view == DimY, view == DimX, view == DimY ), VIEWCOUNT );
	d_slices[ spec ].d_viewer->getViewArea()->addObserver( this );

	d_slices[ spec ].d_bufL = new SpecBufferMdl( d_slices[ spec ].d_viewer->getViewArea(), 
		0, false );
	d_slices[ spec ].d_bufR = new SpecBufferMdl( d_slices[ spec ].d_viewer->getViewArea(), 
		0, false );

	SliceView* sv = new SliceView( d_slices[ spec ].d_bufL );
	d_slices[ spec ].d_viewer->getViews()->replace( CONTOUR, sv );
	d_slices[ spec ].d_sliceL = sv;
	sv = new SliceView( d_slices[ spec ].d_bufR );
	sv->setColor( g_negClr );
	d_slices[ spec ].d_viewer->getViews()->replace( INTENSITY, sv );
	d_slices[ spec ].d_sliceR = sv;

	d_slices[ spec ].d_cur = new CursorMdl();
	d_slices[ spec ].d_cur->addObserver( this );
	CursorView* cv = new CursorView( d_slices[ spec ].d_viewer,
		d_slices[ spec ].d_cur );
	d_slices[ spec ].d_viewer->getViews()->replace( CURSORS, cv );

	d_slices[ spec ].d_viewer->getHandlers()->append( new ZoomCtrl( d_slices[ spec ].d_viewer, view == DimX, view == DimY ) );
	d_slices[ spec ].d_viewer->getHandlers()->append( new SelectZoomCtrl( d_slices[ spec ].d_viewer, view == DimX, view == DimY ) );
	d_slices[ spec ].d_viewer->getHandlers()->append( new ScrollCtrl( d_slices[ spec ].d_viewer ) );
	d_slices[ spec ].d_viewer->getHandlers()->append( new SelectRulerCtr( d_slices[ spec ].d_viewer, true ) );
	d_slices[ spec ].d_viewer->getHandlers()->append( new CursorCtrl( cv, false ) );
	d_slices[ spec ].d_viewer->getHandlers()->append( new FocusCtrl( d_slices[ spec ].d_viewer ) );
}

void SitarViewer::updatePlane(CursorMdl::Update * msg)
{
	// Auf der Plane wurde der Cursor ge�dert
	assert( d_slices.size() >= 2 );
	PpmPoint pos( msg->getX(), msg->getY() );

	// Der X-Slice zeigt den durch den Y-Cursor der Plane
	// repr�entierten Slice mit Origin Y (und umgekehrt)
	if( msg->getDim() == DimY || msg->getDim() == DimUndefined )
	{
		d_cursor[ DimY ] = pos[ DimY ];
		d_slices[ DimY ].d_cur->setCursor( (Dimension)DimY, pos[ DimY ] );
		d_slices[ DimX ].d_specL->setOrigin( DimY, pos[ DimY ] );
		d_slices[ DimX ].d_specR->setOrigin( DimY, pos[ DimY ] );
		d_slices[ DimX ].d_viewer->redraw();
		d_strips[ DimX ].d_cur->setCursor( (Dimension)DimY, d_cursor[ DimY ] );
		d_strips[ DimY ].d_cur->setCursor( (Dimension)DimY, d_cursor[ DimY ] );
		d_slices[ DimY ].d_cur->setCursor( (Dimension)DimY, d_cursor[ DimY ] );
		d_slices[ DimZ ].d_cur->setCursor( (Dimension)DimY, d_cursor[ DimY ] );
		if( d_cursorSync )
			GlobalCursor::setCursor( DimY, pos[ DimY ], d_plane.d_specL->getColor( DimY ) );
	}
	if( msg->getDim() == DimX || msg->getDim() == DimUndefined )
	{
		d_cursor[ DimX ] = pos[ DimX ];
		d_slices[ DimX ].d_cur->setCursor( (Dimension)DimX, pos[ DimX ] );
		d_slices[ DimY ].d_specL->setOrigin( DimX, pos[ DimX ] + d_off );
		d_slices[ DimY ].d_specR->setOrigin( DimX, pos[ DimX ] - d_off );
		d_slices[ DimY ].d_viewer->redraw();
		syncStripsToCur();
		if( d_cursorSync )
			GlobalCursor::setCursor( DimX, pos[ DimX ], d_plane.d_specL->getColor( DimX ) );
		if( d_plane.d_curve->isVisible() )
			d_plane.d_viewer->redraw();
	}
	// sync3dZSliceToCur();
	// syncStripsToCur();
	notifyCursor();
}

void SitarViewer::updatePlane(ViewAreaMdl::Update * msg)
{
	if( msg->getType() != ViewAreaMdl::Update::Range )
		return;
	if( msg->getX() )
	{
		PpmRange r = d_plane.d_viewer->getViewArea()->getRange( DimX );
		d_slices[ DimX ].d_viewer->getViewArea()->setRange( DimX, r, true );
		d_slices[ DimX ].d_viewer->redraw();
		if( d_rangeSync )
			GlobalCursor::setRange( DimX, 
			d_plane.d_viewer->getViewArea()->getRange( DimX ), 
				d_plane.d_specL->getColor( DimX ) );
	}
	if( msg->getY() )
	{
		PpmRange r = d_plane.d_viewer->getViewArea()->getRange( DimY );
		d_slices[ DimY ].d_viewer->getViewArea()->setRange( DimY, r, true );
		d_slices[ DimY ].d_viewer->redraw();
		d_strips[ DimX ].d_viewer->getViewArea()->setRange( DimY, r, true );
		d_strips[ DimX ].d_viewer->redraw();
		d_strips[ DimY ].d_viewer->getViewArea()->setRange( DimY, r, true );
		d_strips[ DimY ].d_viewer->redraw();
		d_slices[ DimZ ].d_viewer->getViewArea()->setRange( DimY, r, true );
		d_slices[ DimZ ].d_viewer->redraw();
		if( d_rangeSync )
			GlobalCursor::setRange( DimY, 
			d_plane.d_viewer->getViewArea()->getRange( DimY ), 
				d_plane.d_specL->getColor( DimY ) );
	}
}

void SitarViewer::updateSlice(Dimension dim, CursorMdl::Update *msg)
{
	// Auf einem Slice wurde der Cursor ge�dert
	bool twoD = true;
	if( dim == DimZ )
	{
		dim = DimY;
		twoD = false;
	}

	d_cursor[ dim ] = msg->getX();
	d_plane.d_cur->setCursor( dim, msg->getX() ); // Beide Dims gleich
	if( d_plane.d_curve->isVisible() )
		d_plane.d_viewer->redraw();
	notifyCursor(twoD);
	if( d_cursorSync )
		GlobalCursor::setCursor( dim, msg->getX(), 
			d_slices[ dim ].d_specL->getColor( DimX ) );
	if( dim == DimY )
	{
		d_strips[ DimX ].d_cur->setCursor( (Dimension)DimY, d_cursor[ dim ] );
		d_strips[ DimY ].d_cur->setCursor( (Dimension)DimY, d_cursor[ dim ] );
		d_slices[ DimY ].d_cur->setCursor( (Dimension)DimY, d_cursor[ dim ] );
		d_slices[ DimZ ].d_cur->setCursor( (Dimension)DimY, d_cursor[ dim ] );
	}
	if( dim == DimX )
		syncStripsToCur();
}

void SitarViewer::updateSlice(Dimension dim, ViewAreaMdl::Update *msg)
{
	if( msg->getType() != ViewAreaMdl::Update::Range )
		return;

	PpmRange r = d_slices[ dim ].d_viewer->getViewArea()->getRange( 
		d_slices[ dim ].d_viewer->getViewArea()->getDim() );

	if( dim == DimZ )
		dim = DimY;

	d_plane.d_viewer->getViewArea()->setRange( dim, r, true );
	d_plane.d_viewer->redraw();

	if( dim == DimY )
	{
		d_strips[ DimX ].d_viewer->getViewArea()->setRange( DimY, r, true );
		d_strips[ DimX ].d_viewer->redraw();
		d_strips[ DimY ].d_viewer->getViewArea()->setRange( DimY, r, true );
		d_strips[ DimY ].d_viewer->redraw();
		d_slices[ DimZ ].d_viewer->getViewArea()->setRange( DimY, r, true );
		d_slices[ DimZ ].d_viewer->redraw();
		d_slices[ DimY ].d_viewer->getViewArea()->setRange( DimY, r, true );
		d_slices[ DimY ].d_viewer->redraw();
	}
}

void SitarViewer::updateStrip(Dimension dim, ViewAreaMdl::Update *msg)
{
	// In einem Strip wurde Ausschnitt ge�dert

	if( msg->getType() != ViewAreaMdl::Update::Range )
		return;

	if( msg->getY() )
	{
		PpmRange r = d_strips[ dim ].d_viewer->getViewArea()->getRange( DimY );
		d_slices[ DimY ].d_viewer->getViewArea()->setRange( DimY, r, true );
		d_slices[ DimY ].d_viewer->redraw();
		d_slices[ DimZ ].d_viewer->getViewArea()->setRange( DimY, r, true );
		d_slices[ DimZ ].d_viewer->redraw();
		d_plane.d_viewer->getViewArea()->setRange( DimY, r, true );
		d_plane.d_viewer->redraw();
		if( dim != DimX )
			d_strips[ DimX ].d_viewer->getViewArea()->setRange( DimY, r, true );
		if( dim != DimY )
			d_strips[ DimY ].d_viewer->getViewArea()->setRange( DimY, r, true );
	}
}

void SitarViewer::updateStrip(Dimension dim, CursorMdl::Update *msg)
{
	// In einem Strip wurde Cursor ge�dert
	d_plane.d_cur->setCursor( (Dimension)DimY, msg->getX() ); // Beide Dims gleich
	updatePlane( msg );
	notifyCursor(false);
}

void SitarViewer::notifyCursor(bool plane)
{
	QString str;
    QTextStream ts( &str, QIODevice::WriteOnly );

	ts.setf( QTextStream::fixed );
	ts.precision( 3 );

	ts <<  "Cursor:  ";

	Spectrum* spec = 0;
	if( plane )
		spec = d_spec->getLhs();
	else
		spec = d_spec;

	for( Dimension d = 0; d < spec->getDimCount(); d++ )
	{
		ts << char( 'x' + d ) << ": ";
		// ts << spec->getColor( d ).getLabel() << "=";
		ts << spec->getDimName( d ) << "=";	// wegen Fred
		ts << d_cursor[ d ];
		ts << "  ";
	}

	try
	{
		ts.setf( QTextStream::showpos );
		ts.precision( 0 );
		if( plane )
		{
			PpmPoint p;
			for( Dimension d = 0; d < spec->getDimCount(); d++ )
				p.push_back( d_cursor[ d ] );
			ts << "Level L=" << d_spec->getLhs()->getAt( p, false, false );
			ts << " Level R=" << d_spec->getRhs()->getAt( p, false, false );
		}else 
			ts << "Level=" << spec->getAt( d_cursor, false, false );
	}catch( ... )
	{
		ts << " Out of Spectrum";
	}
	QByteArray  tmp;
	Lexi::ShowStatusMessage msg( str );
	MainWindow::handle( msg );
}

void SitarViewer::adjustN()
{
	QString str;
	str.sprintf( "N=%0.3f", d_cursor[DimZ] );
	d_strips[ DimY ].d_viewer->getViews()->replace( LABEL1,
		new Lexi::Label( str, nil, g_curClr, 
		Lexi::AlignLeft, Lexi::AlignTop ) );

	str.sprintf( " %s/%s", 
		d_spec->getDimName( DimX ), d_spec->getDimName( DimY ) );
	d_strips[ DimX ].d_viewer->getViews()->replace( LABEL2,
		new Lexi::Label( str, nil, g_curClr, 
		Lexi::AlignLeft, Lexi::AlignBottom ) );
	str.sprintf( " %s/%s", 
		d_spec->getDimName( DimZ ), d_spec->getDimName( DimY ) );
	d_strips[ DimY ].d_viewer->getViews()->replace( LABEL2,
		new Lexi::Label( str, nil, g_curClr, 
		Lexi::AlignLeft, Lexi::AlignBottom ) );
	syncStripsToCur();
	if( d_cursorSync )
		GlobalCursor::setCursor( DimY, d_cursor[DimZ], d_spec->getColor( DimZ ) );

}

static void _allocate( PpmRange& r, PPM point, PPM width ) // TODO: Bug Release-Mode
{
	if( r.first <= r.second )
	{
		r.first = point - width / 2.0;
		r.second = point + width / 2.0;
	}else
	{
		r.first = point + width / 2.0;
		r.second = point - width / 2.0;
	}
}

void SitarViewer::syncStripsToCur()
{
	d_strips[ DimX ].d_spec->setOrigin( d_cursor );
	d_strips[ DimY ].d_spec->setOrigin( d_cursor );

	PpmRange r = d_spec->getScale( DimX ).getRange();	
	_allocate( r, d_cursor[ DimX ], d_pw[DimX] );
	d_strips[ DimX ].d_viewer->getViewArea()->setRange( DimX, r );

	r = d_spec->getScale( DimZ ).getRange();
	_allocate( r, d_cursor[ DimZ ], d_pw[DimY] );
	d_strips[ DimY ].d_viewer->getViewArea()->setRange( DimX, r );

	d_strips[ DimX ].d_viewer->redraw();
	d_strips[ DimY ].d_viewer->redraw();

	d_slices[ DimZ ].d_specL->setOrigin( d_cursor );
	d_slices[ DimZ ].d_viewer->redraw();

	if( d_bufSum->getSpectrum() )
		d_specSum->setOrigin( d_cursor );
}


void SitarViewer::handleAutoContour2(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_strips[ DimX ].d_view->isAuto() );
	
	bool ac = !d_strips[ DimX ].d_view->isAuto();
	updateContour2( DimX, ac );
	updateContour2( DimY, ac );
}

void SitarViewer::handleContourParams2(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	Dlg::ContourParams p;
	p.d_factor = d_strips[ DimX ].d_view->getFactor();
	p.d_threshold =	d_spec->getThreshold();
	p.d_option = d_strips[ DimX ].d_view->getOption();
	if( Dlg::setParams( getQt(), p ) )
	{
		d_spec->setThreshold( p.d_threshold );
		d_strips[ DimX ].d_view->setOption( p.d_option );
		d_strips[ DimX ].d_view->setFactor( p.d_factor );
		d_strips[ DimY ].d_view->setOption( p.d_option );
		d_strips[ DimY ].d_view->setFactor( p.d_factor );
		updateContour2( DimX, false );
		updateContour2( DimY, false );
	}
}

void SitarViewer::handleFitWindow2(Action & a)
{
	ACTION_ENABLED_IF( a, true ); 
	PpmRange r = d_spec->getScale( DimY ).getRange();	
	r.invert();
	d_strips[ DimX ].d_viewer->getViewArea()->setRange( DimY, r );
	d_strips[ DimY ].d_viewer->getViewArea()->setRange( DimY, r );
	d_slices[ DimZ ].d_viewer->getViewArea()->setRange( DimY, r );	
	d_strips[ DimX ].d_viewer->redraw();
	d_strips[ DimY ].d_viewer->redraw();
}

void SitarViewer::handleSetPeakWidht(Action & a)
{
	ACTION_ENABLED_IF( a, d_strips[ DimX ].d_viewer->hasFocus() ||
		  d_strips[ DimY ].d_viewer->hasFocus() );

	Dimension dim;
	if( d_strips[ DimX ].d_viewer->hasFocus() )
	{
		dim = DimX;
	}else
	{
		dim = DimY;
	}
	bool ok	= FALSE;
	QString res;
	res.sprintf( "%0.3f", d_pw[dim] );
	res	= QInputDialog::getText( "Set Peak Width", 
		"Please	enter atom positive PPM value:", QLineEdit::Normal, 
		res, &ok, getQt() );
	if( !ok )
		return;
	PPM w = res.toFloat( &ok );
	if( !ok || w < 0.0 )
	{
		QMessageBox::critical( getQt(), "Set Peak Width",
				"Invalid peak width!", "&Cancel" );
		return;
	}
	d_pw[dim] = w;
	syncStripsToCur();
}

void SitarViewer::updateContour2(Dimension d, bool ac)
{
	if( ac )
	{
		d_strips[d].d_view->createLevelsAuto();
	}else
		d_strips[d].d_view->createLevelsMin( d_spec->getThreshold() );
	d_strips[d].d_viewer->damageMe();
}

void SitarViewer::setOffCursor(PPM p)
{
	d_plane.d_lhs->setCursor( (Dimension)DimX, p + d_off );
	d_plane.d_rhs->setCursor( (Dimension)DimX, p - d_off );
	d_slices[DimX].d_lhs->setCursor( (Dimension)DimX, p + d_off );
	d_slices[DimX].d_rhs->setCursor( (Dimension)DimX, p - d_off );
}

void SitarViewer::handleCursorSync(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_cursorSync );
	
	d_cursorSync = !d_cursorSync;
	if( d_cursorSync )
		GlobalCursor::addObserver( this );	// TODO: preset Cursor
	else if( !d_rangeSync )
		GlobalCursor::removeObserver( this );
}

void SitarViewer::handleRangeSync(Action &a)
{
	ACTION_CHECKED_IF( a, true, d_rangeSync );
	
	d_rangeSync = !d_rangeSync;
	if( d_rangeSync )
		GlobalCursor::addObserver( this );	// TODO: preset Cursor
	else if( !d_cursorSync )
		GlobalCursor::removeObserver( this );
}

void SitarViewer::handleFitWindow(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	d_plane.d_bufL->fitToArea();
	d_plane.d_viewer->damageMe();
	/*
	for( Dimension d = 0; d < 2; ++d )
	{
		d_slices[ d ].d_buf2D->fitToArea();
		d_slices[ d ].d_viewer->redraw();
	}
	*/
}

SitarCurveView::SitarCurveView(SpecViewer* vr, SitarSpectrum* s, CursorView* m ):
	SpecView( vr ), d_spec( s ), d_cur( m ), d_visi( false )
{
		d_clr = ( Qt::white );
}

void SitarCurveView::setVisible(bool on)
{
	d_visi = on;
}

void SitarCurveView::setColor(QColor c)
{
	d_clr = c;
}

void SitarCurveView::draw( Canvas& v, const Allocation& a)
{
	if( !d_visi )
		return;

	Canvas& c = v;

	ViewAreaMdl* area = getViewArea();
	Twips p, x, y;

	p = area->toTwip( d_cur->getCursor( DimX ), a.getLeft(), DimX );
	y = a.getTop();
	const Scale& s = d_spec->getScale( DimZ );
	float fac;
	PPM off;
	PPM n;
	while( y < a.getBottom() )
	{
		fac = ( y - a.getTop() ) / float( a.getHeight() );
		n = s.getIdxN() + fac * s.getWidth();
		off = d_spec->getHOffFromN( n );
		x = area->getTwipPerPpm( DimX ) * off * 0.5;
 		c.drawPoint( p + x, y, d_clr );
 		c.drawPoint( p - x, y, d_clr );

		// TEST
		/*
		n = d_spec->getNFromHOff( off ) - n;
		fac = n / s.getWidth();
		x = x * ( 1.0 + fac );
		*/

		/*
		n = d_spec->getNFromHOff( off );
		off = d_spec->getHOffFromN( n );
		x = area->getTwipPerPpm( DimX ) * off * 0.5;
		*/

 		// c.drawPoint( p - x, y, d_cur->getColor(0) );
		// END TEST

		y += 20;
	}
}

void SitarViewer::handleShowCurve(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_plane.d_curve->isVisible() );

	d_plane.d_curve->setVisible( !d_plane.d_curve->isVisible() );
	d_plane.d_viewer->redraw();
}

void SitarViewer::handleMatchGrad(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_spec->matchGradient() );
	d_spec->matchGradient( !d_spec->matchGradient() );
}

void SitarViewer::handleInterpolate(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_spec->interpol() );
	d_spec->interpol( !d_spec->interpol() );
}

void SitarViewer::handleClip(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_spec->clip() );
	d_spec->clip( !d_spec->clip() );
}

void SitarViewer::handleLineSlices(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_slices[ 0 ].d_sliceL->isLineSpec() );
	
	bool on = !d_slices[ 0 ].d_sliceL->isLineSpec();
	d_sliceSum->setLineSpec( on );
	for( int i = 0; i < d_slices.size(); i++ )
	{
		d_slices[ i ].d_sliceL->setLineSpec( on );
		d_slices[ i ].d_sliceR->setLineSpec( on );
		d_slices[ i ].d_viewer->redraw();
	}
}

void SitarViewer::handleSyncDepth(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_syncDepth );
	
	d_syncDepth = !d_syncDepth;
}

void SitarViewer::centerY()
{
	ViewAreaMdl* area = d_plane.d_viewer->getViewArea();
	if( !area->getRange( DimY ).contains( d_cursor[ DimY ] ) )
	{
		area->centerPointDim( DimY, d_cursor[ DimY ] );
		d_plane.d_viewer->damageMe();
	}
}

void SitarViewer::handleAutoContour(Action & a)
{
	ACTION_CHECKED_IF( a, d_plane.d_specL, d_plane.d_viewL->isAuto() );

	if( d_plane.d_viewL->isAuto() )
	{
		d_plane.d_viewL->createLevelsMin( d_plane.d_specL->getThreshold() );
		d_plane.d_viewR->createLevelsMin( d_plane.d_specR->getThreshold() );
	}else
	{
		d_plane.d_viewL->createLevelsAuto();
		d_plane.d_viewR->createLevelsAuto();
	}
	d_plane.d_viewer->redraw();
}

void SitarViewer::handleContourParams(Action & a)
{
	ACTION_ENABLED_IF( a, d_plane.d_specL );

	Dlg::ContourParams p;
	p.d_factor = d_plane.d_viewL->getFactor();
	p.d_threshold =	d_plane.d_specL->getThreshold();
	p.d_option = d_plane.d_viewL->getOption();
	if( p.d_option != ContourView::Both && !d_spec->lhsPos() )
	{
		p.d_option = (p.d_option==ContourView::Positive)?
			ContourView::Negative:ContourView::Positive;
	}
	if( Dlg::setParams( getQt(), p ) )
	{
		if( p.d_option != ContourView::Both && !d_spec->lhsPos() )
		{
			p.d_option = (p.d_option==ContourView::Positive)?
				ContourView::Negative:ContourView::Positive;
		}
		d_plane.d_specL->setThreshold( p.d_threshold );
		d_plane.d_viewL->setOption( p.d_option );
		d_plane.d_viewL->setFactor( p.d_factor );
		d_plane.d_viewL->setVisi( true );
		d_plane.d_viewL->createLevelsMin( d_plane.d_specL->getThreshold() );

		if( p.d_option != ContourView::Both && d_spec->lhsPos() != d_spec->rhsPos() )
		{
			p.d_option = (p.d_option==ContourView::Positive)?
				ContourView::Negative:ContourView::Positive;
		}
		d_plane.d_specR->setThreshold( p.d_threshold );
		d_plane.d_viewR->setOption( p.d_option );
		d_plane.d_viewR->setFactor( p.d_factor );
		d_plane.d_viewR->setVisi( true );
		d_plane.d_viewR->createLevelsMin( d_plane.d_specR->getThreshold() );

		d_plane.d_viewer->damageMe();
	}
}

void SitarViewer::handleShowRhs(Action & a)
{
	ACTION_CHECKED_IF( a, d_plane.d_specR, d_plane.d_viewR->isVisi() );
	d_plane.d_viewR->setVisi( !d_plane.d_viewR->isVisi() );
}

void SitarViewer::handleShowLhs(Action & a)
{
	ACTION_CHECKED_IF( a, d_plane.d_specL, d_plane.d_viewL->isVisi() );
	d_plane.d_viewL->setVisi( !d_plane.d_viewL->isVisi() );
}

void SitarViewer::handleRectify(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_spec->rectify() );
	d_spec->rectify( !d_spec->rectify() );
}

void SitarViewer::handleOneOff(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_spec->oneOff() );
	d_spec->oneOff( !d_spec->oneOff() );
}

void SitarViewer::handleShowSumSlice(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_bufSum->getSpectrum() );

	if( d_bufSum->getSpectrum() )
		d_bufSum->setSpectrum( 0 );
	else
	{
		d_bufSum->setSpectrum( d_specSum );
		d_specSum->setOrigin( d_cursor );
	}
}



