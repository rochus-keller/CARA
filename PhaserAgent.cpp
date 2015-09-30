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

#include "PhaserAgent.h"
#include <qtextstream.h> 
#include <qinputdialog.h> 
#include <qmessagebox.h>
#include <Lexi/MainWindow.h>
#include <Lexi/Background.h>
#include <Lexi/Label.h>
#include <Lexi/ContextMenu.h>
#include <QColor>
#include <Spec/SpecProjector.h>
#include <Spec/SpectrumType.h>
#include <Spec/SpectrumPeer.h>
#include <SpecView/CursorMdl.h>
#include <SpecView/ViewAreaMdl.h>
#include <SpecView/SpecViewer.h>
#include <SpecView/FocusCtrl.h>
#include <SpecView/ContourView.h>
#include <SpecView/IntensityView.h>
#include <SpecView/CursorView.h>
#include <SpecView/CursorCtrl.h>
#include <SpecView/ScrollCtrl.h>
#include <SpecView/ZoomCtrl.h>
#include <SpecView/SelectZoomCtrl.h>
#include <SpecView/SelectRulerCtr.h>
#include <SpecView/SliceView.h>
#include <SpecView/PeakPlaneView.h>
#include <SpecView/PeakSelectCtrl.h>
#include <SpecView/PeakModelView.h>
#include <SpecView/FoldingView.h>

#include <SpecView/PeakModelTuner.h>
#include <SpecView/RotateDlg.h>
#include <Dlg.h>
#include <SpecView/GotoDlg.h>
#include <SpecView/PeakBatchCurve.h>

using namespace Spec;
using Root::Index;

static const int BACKGROUND = 0;
static const int INTENSITY = 1;
static const int CONTOUR = 2;
static const int FOLDING = 3;
static const int PIVOT = 4;
static const int CURSORS = 5;
static const int LABEL1 = 6;
static const int LABEL2 = 7;
static const int VIEWCOUNT = 8;

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr PhaserAgent::SetResolution = "SetResolution";
Root::Action::CmdStr PhaserAgent::ShowLowRes = "ShowLowRes";
Root::Action::CmdStr PhaserAgent::Forward = "Forward";
Root::Action::CmdStr PhaserAgent::Backward = "Backward";
Root::Action::CmdStr PhaserAgent::FitWindow = "FitWindow";
Root::Action::CmdStr PhaserAgent::ShowFolded = "ShowFolded";
Root::Action::CmdStr PhaserAgent::SpecRotate = "SpecRotate";
Root::Action::CmdStr PhaserAgent::Goto = "Goto";
Root::Action::CmdStr PhaserAgent::ShowContour = "ShowContour";
Root::Action::CmdStr PhaserAgent::ShowIntensity = "ShowIntensity";
Root::Action::CmdStr PhaserAgent::AutoContour = "AutoContour";
Root::Action::CmdStr PhaserAgent::ContourParams = "ContourParams";

Root::Action::CmdStr PhaserAgent::PickBounds = "PickBounds";
Root::Action::CmdStr PhaserAgent::SetSliceMinMax = "SetSliceMinMax";
Root::Action::CmdStr PhaserAgent::SliceAutoScale = "SliceAutoScale";
Root::Action::CmdStr PhaserAgent::PickBoundsSym = "PickBoundsSym";

Root::Action::CmdStr PhaserAgent::ForwardPlane = "ForwardPlane";
Root::Action::CmdStr PhaserAgent::BackwardPlane = "BackwardPlane";
Root::Action::CmdStr PhaserAgent::SetPhase = "SetPhase";

Root::Action::CmdStr PhaserAgent::UseDimX = "UseDimX";
Root::Action::CmdStr PhaserAgent::UseDimY = "UseDimY";
Root::Action::CmdStr PhaserAgent::UseDimZ = "UseDimZ";
Root::Action::CmdStr PhaserAgent::SetPivot = "SetPivot";
Root::Action::CmdStr PhaserAgent::PointPivot = "PointPivot";
Root::Action::CmdStr PhaserAgent::AutoGain = "AutoGain";

ACTION_SLOTS_BEGIN( PhaserAgent )
    { PhaserAgent::AutoGain, &PhaserAgent::handleAutoGain },
    { PhaserAgent::PointPivot, &PhaserAgent::handlePointPivot },
    { PhaserAgent::SetPivot, &PhaserAgent::handleSetPivot },
    { PhaserAgent::UseDimX, &PhaserAgent::handleUseDimX },
    { PhaserAgent::UseDimY, &PhaserAgent::handleUseDimY },
    { PhaserAgent::UseDimZ, &PhaserAgent::handleUseDimZ },
    { PhaserAgent::SetPhase, &PhaserAgent::handleSetPhase },
    { PhaserAgent::BackwardPlane, &PhaserAgent::handleBackwardPlane },
    { PhaserAgent::ForwardPlane, &PhaserAgent::handleForwardPlane },
    { PhaserAgent::PickBounds, &PhaserAgent::handlePickBounds },
    { PhaserAgent::SetSliceMinMax, &PhaserAgent::handleSetSliceMinMax },
    { PhaserAgent::SliceAutoScale, &PhaserAgent::handleSliceAutoScale },
    { PhaserAgent::PickBoundsSym, &PhaserAgent::handlePickBoundsSym },
    { PhaserAgent::Goto, &PhaserAgent::handleGoto },
    { PhaserAgent::ContourParams, &PhaserAgent::handleContourParams },
    { PhaserAgent::AutoContour, &PhaserAgent::handleAutoContour },
    { PhaserAgent::ShowIntensity, &PhaserAgent::handleShowIntensity },
    { PhaserAgent::ShowContour, &PhaserAgent::handleShowContour },
    { PhaserAgent::SpecRotate, &PhaserAgent::handleSpecRotate },
    { PhaserAgent::ShowFolded, &PhaserAgent::handleShowFolded },
    { PhaserAgent::Forward, &PhaserAgent::handleForward },
    { PhaserAgent::Backward, &PhaserAgent::handleBackward },
    { PhaserAgent::ShowLowRes, &PhaserAgent::handleShowLowRes },
    { PhaserAgent::SetResolution, &PhaserAgent::handleSetResolution },
    { PhaserAgent::FitWindow, &PhaserAgent::handleFitWindow },
ACTION_SLOTS_END( PhaserAgent )

//////////////////////////////////////////////////////////////////////

#define MW ((Lexi::MainWindow*)getParent())->getQt()

static	QColor g_color = Qt::darkGray;

class _PhaseCtr : public Lexi::Handler
{
	Root::Ptr<PhaserAgent> d_agent;
	float d_phi, d_psi;
	Dimension d_dim;
public:
	_PhaseCtr( PhaserAgent* a ):
	  d_agent(a), d_phi( 0 ), d_psi( 0 )
	{
		d_dim = DimUndefined;
	}
	void draw(Canvas & c, const Allocation & a)
	{
		Lexi::Twips x = a.getLeft() + a.getWidth() / 10;
		c.drawLine( x, a.getTop(), x, a.getBottom(), g_color );	
		Lexi::Twips y = a.getTop() + a.getHeight() / 10;
		c.drawLine( x, y, a.getRight(), y, g_color );	
	}
	bool handleMousePress( Viewport& v, const Allocation& a, const MouseEvent& e )
	{
		if( e.isLeft() && d_agent->getCurDim() != -1 )
		{
			d_phi = d_agent->getPhi();
			d_psi = d_agent->getPsi();
			if( e.getX() < ( a.getLeft() + a.getWidth() / 10 ) )
				d_dim = DimY;
			else if( e.getY() < ( a.getTop() + a.getHeight() / 10 ) )
				d_dim = DimX;
			else
				d_dim = DimUndefined;
			v.grab( this );
			return true;
		}else
			return false;
	}
	void update( const Allocation& a, Lexi::Twips w, Lexi::Twips h, bool all, bool shift )
	{
		if( d_dim == DimX || d_dim == DimUndefined )
			d_phi = double( w ) * 360.0 * ( (shift)?3.0:1.0 ) / 
				double( a.getWidth() ) + d_phi;
		if( d_dim == DimY || d_dim == DimUndefined )
			d_psi = double( h ) * 360.0 * ( (shift)?3.0:1.0 ) / 
				double( a.getHeight() ) + d_psi;
		d_agent->setPhiPsi( d_phi, d_psi, all );
	}
	bool handleMouseRelease( Viewport& v, const Allocation& a, const MouseEvent& e )
	{
		if( v.isGrabbing( this ) )
		{
			d_agent->setPhiPsi( d_phi, d_psi, true );
			v.ungrab();
			return true;
		}else
			return false;
	}
	bool handleMouseDrag( Viewport& v, const Allocation& a, const DragEvent& e )
	{
		if( v.isGrabbing( this ) )
		{
			update( a, e.getDeltaX(), -e.getDeltaY(), false, e.isShift() );
			return true;
		}else
			return false;
	}
	bool handleKeyPress( Viewport& v, const Allocation& a, const KeyEvent& e ) 
	{
		if( e.isUp() || e.isDown() || e.isLeft() || e.isRight() )
		{
			if( d_agent->getCurDim() == -1 )
				return false;
			const float off = 1;
			const float fac = 20;
			float dx = 0, dy = 0;
			if( e.isUp() )
				dy = off;
			else if( e.isDown() )
				dy = -off;
			else if( e.isLeft() )
				dx = -off;
			else if( e.isRight() )
				dx = off;
			if( e.isShift() )
			{
				dx *= fac;
				dy *= fac;
			}
			d_agent->setPhiPsi( d_agent->getPhi() + dx, d_agent->getPsi() + dy, true );
			return true;
		}else if( e.isPlainKey() )
		{
			switch( e.getChar() )	// Funktioniert nicht, keine Ahnung warum
			{
			case 'x':
			case 'y':
			case 'z':
				d_agent->setCurDim( e.getChar() - 'x' );
				return true;
			}
		}
		return false;
	}

};

//////////////////////////////////////////////////////////////////////

PhaserAgent::PhaserAgent(Root::Agent* parent, Spectrum* spec ):
	Root::Agent( parent ), d_lock( false ), d_curDim( -1 )
{
	assert( spec );
	d_spec = new PhasedSpec( spec );
	initParams();
	d_cursor.assign( spec->getDimCount(), 0 );
	buildPopup();
}

void PhaserAgent::initParams()
{
	d_resol = 1;
	d_folding = false;
	d_lowResol = false;
	d_autoContour = true;
	d_showIntens = false;
	d_showContour = true;
	d_contourFactor = 1.4f;
	d_contourOption = ContourView::Both;
	d_contourLevel = 400;
	d_gain = 2.0;
	// TODO: diese Werte sollen ab Konfigurations-Record gelsen werden
}

PhaserAgent::~PhaserAgent()
{
	delete d_popPlane;
	delete d_popSlice;
	delete d_popPane;
}

void PhaserAgent::buildPopup()
{
	d_popPlane = new Gui::Menu();

	Gui::Menu::item( d_popPlane, this, "Pick Pivot", PointPivot, false );
	Gui::Menu::item( d_popPlane, this, "Fit Window", FitWindow, false );

	d_popSlice = new Gui::Menu();
	Gui::Menu::item( d_popSlice, this, "Set Phase...", SetPhase, false );
	Gui::Menu::item( d_popSlice, this, "Set Pivot...", SetPivot, false );
	d_popSlice->insertSeparator();
	Gui::Menu::item( d_popSlice, this, "Set Bounds...", SetSliceMinMax, false );
	Gui::Menu::item( d_popSlice, this, "Pick Bounds", PickBounds, false );
	Gui::Menu::item( d_popSlice, this, "Pick Bounds Sym.", PickBoundsSym, false );
	Gui::Menu::item( d_popSlice, this, "Auto Scale", SliceAutoScale, true );

	d_popPane = new Gui::Menu();
	Gui::Menu::item( d_popPane, this, "Set Phase...", SetPhase, false );
	Gui::Menu::item( d_popSlice, this, "Set Pivot...", SetPivot, false );
	d_popPane->insertSeparator();
	Gui::Menu::item( d_popPane, this, "Use Dim. X", UseDimX, true );
	Gui::Menu::item( d_popPane, this, "Use Dim. Y", UseDimY, true );
	Gui::Menu::item( d_popPane, this, "Use Dim. Z", UseDimZ, true );
}

void PhaserAgent::allocate()
{
	const Dimension dim = d_spec->getDimCount();
	if( d_plane.d_cur )
		d_plane.d_cur->removeObserver( this );
	if( d_plane.d_viewer )
		d_plane.d_viewer->getViewArea()->removeObserver( this );
	for( Dimension d = 0; d < d_slices.size(); d++ )
	{
		if( d_slices[d].d_cur )
			d_slices[d].d_cur->removeObserver( this );
		if( d_slices[d].d_viewer )
			d_slices[d].d_viewer->getViewArea()->removeObserver( this );
	}
	d_slices.assign( dim, SliceSocket() ); 
}

SpecViewer* PhaserAgent::createPlaneViewer()
{
	d_plane.d_viewer = new SpecViewer( new ViewAreaMdl( true, true, true, true ), VIEWCOUNT );
	d_plane.d_viewer->getViewArea()->addObserver( this );

	// d_autoContour = d_spec->getDimCount() <= 2; 

	d_plane.d_spec = new SpecProjector( d_spec, DimX, DimY );
	d_plane.d_buf = 
		new SpecBufferMdl( d_plane.d_viewer->getViewArea(), d_plane.d_spec );
	d_plane.d_buf->setFolding( d_folding );

	bool intens = d_showIntens;
	bool contour = d_showContour;
	showIntens( false, false );
	showContour( false, false );
	showIntens( intens, false );
	showContour( contour, false );

	d_plane.d_cur = new CursorMdl();
	d_plane.d_cur->addObserver( this );
	CursorView* cv = new CursorView( d_plane.d_viewer, d_plane.d_cur );
	d_plane.d_viewer->getViews()->replace( CURSORS, cv );

	d_plane.d_pivot = new CursorMdl();
	CursorView* pv = new CursorView( d_plane.d_viewer, d_plane.d_pivot );
	pv->setColor( Qt::darkYellow );
	d_plane.d_viewer->getViews()->replace( PIVOT, pv );

	if( d_folding )
		d_plane.d_viewer->getViews()->replace( FOLDING, new FoldingView( d_plane.d_buf ) );

	d_plane.d_viewer->getHandlers()->append( new ZoomCtrl( d_plane.d_viewer, true, true ) );
	d_plane.d_viewer->getHandlers()->append( new SelectZoomCtrl( d_plane.d_viewer, true, true ) );
	d_plane.d_viewer->getHandlers()->append( new ScrollCtrl( d_plane.d_viewer ) );
	d_plane.d_viewer->getHandlers()->append( new CursorCtrl( cv, false ) );
	d_plane.d_viewer->getHandlers()->append( new Lexi::ContextMenu( d_popPlane, false ) );
	d_plane.d_viewer->getHandlers()->append( new FocusCtrl( d_plane.d_viewer ) );

	d_plane.d_buf->fitToArea();
	return d_plane.d_viewer;
}

SpecViewer* PhaserAgent::createPhaser()
{
	d_phaser.d_viewer = new SpecViewer( new ViewAreaMdl( true, true, true, true ) );
	d_phaser.d_viewer->getViews()->append( new Lexi::Background( 0, Qt::gray ) );
	d_phaser.d_label = new Lexi::Label( "", 0, Qt::black, 0.5, 0.5 );
	d_phaser.d_viewer->getViews()->append( d_phaser.d_label );
	d_phaser.d_viewer->getHandlers()->append( new _PhaseCtr( this ) );
	d_phaser.d_viewer->getHandlers()->append( new Lexi::ContextMenu( d_popPane, false ) );
	d_phaser.d_viewer->getHandlers()->append( new FocusCtrl( d_phaser.d_viewer ) );
	return d_phaser.d_viewer;
}

SpecViewer* PhaserAgent::createSliceViewer(Dimension view, Dimension spec)
{
	d_slices[ spec ].d_viewer = new SpecViewer( 
		new ViewAreaMdl( view == DimX, view == DimY, view == DimX, view == DimY ), 6 );
	d_slices[ spec ].d_viewer->getViewArea()->addObserver( this );

	d_slices[ spec ].d_spec = new SpecProjector( d_spec, spec );
	d_slices[ spec ].d_buf = new SpecBufferMdl( d_slices[ spec ].d_viewer->getViewArea(), 
		d_slices[ spec ].d_spec );
	d_slices[ spec ].d_buf->setFolding( d_folding );

	SliceView* sv = new SliceView( d_slices[ spec ].d_buf );
	d_slices[ spec ].d_viewer->getViews()->replace( CONTOUR, sv );
	d_slices[ spec ].d_slice = sv;
	d_slices[ spec ].d_cur = new CursorMdl();
	d_slices[ spec ].d_cur->addObserver( this );
	CursorView* cv = new CursorView( d_slices[ spec ].d_viewer,
		d_slices[ spec ].d_cur );
	d_slices[ spec ].d_viewer->getViews()->replace( CURSORS, cv );

	d_slices[ spec ].d_pivot = new CursorMdl();
	CursorView* pv = new CursorView( d_slices[ spec ].d_viewer, d_slices[ spec ].d_pivot );
	pv->setColor( ( Qt::darkYellow ) );
	d_slices[ spec ].d_viewer->getViews()->replace( PIVOT, pv );

	d_slices[ spec ].d_viewer->getHandlers()->append( new ZoomCtrl( d_slices[ spec ].d_viewer, view == DimX, view == DimY ) );
	d_slices[ spec ].d_viewer->getHandlers()->append( new SelectZoomCtrl( d_slices[ spec ].d_viewer, view == DimX, view == DimY ) );
	d_slices[ spec ].d_viewer->getHandlers()->append( new ScrollCtrl( d_slices[ spec ].d_viewer ) );
	d_slices[ spec ].d_viewer->getHandlers()->append( new SelectRulerCtr( d_slices[ spec ].d_viewer, true ) );
	d_slices[ spec ].d_viewer->getHandlers()->append( new CursorCtrl( cv, false ) );
	d_slices[ spec ].d_viewer->getHandlers()->append( new Lexi::ContextMenu( d_popSlice, false ) );
	d_slices[ spec ].d_viewer->getHandlers()->append( new FocusCtrl( d_slices[ spec ].d_viewer, this ) );
	d_slices[ spec ].d_buf->fitToArea();

	return d_slices[ spec ].d_viewer;
}

void PhaserAgent::handle(Root::Message& msg)
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
			for( Dimension d = 0; d < d_slices.size(); d++ )
				if( true && 
					d_slices[ d ].d_viewer->getViewArea() == a->getOrigin() )
				{
					updateSlice( d, a );
					break;
				}

		Lexi::Viewport::popCursor();
		msg.consume();
	}
	MESSAGE( CursorMdl::Update, a, msg )
	{
		if( a->getOrigin() == d_plane.d_cur )
			updatePlane( a );
		else
			for( Dimension d = 0; d < d_slices.size(); d++ )
				if( true && d_slices[ d ].d_cur == a->getOrigin() )
				{
					updateSlice( d, a );
					break;
				}
		msg.consume();
	}
	MESSAGE( Root::Action, a, msg )
	{
		d_lock = false; // Kein Blocking fr Action-Ausfhrung
		EXECUTE_ACTION( PhaserAgent, *a );
	}
	MESSAGE( FocusCtrl::FocusIn, a, msg )
	{
		for( int l = 0; l < d_slices.size(); l++ )
		{
			if( d_slices[ l ].d_spec && d_slices[ l ].d_viewer == a->getViewer() )
			{
				d_curDim = l;
				updatePane();
				break;
			}else
				d_curDim = -1;
		}
		msg.consume();
	}
	END_HANDLER();
	d_lock = false;
}


void PhaserAgent::updatePlane(CursorMdl::Update * msg)
{
	assert( d_slices.size() >= 2 );
	PpmPoint pos( msg->getX(), msg->getY() );

	// Der X-Slice zeigt den durch den Y-Cursor der Plane
	// reprsentierten Slice mit Origin Y (und umgekehrt)
	if( msg->getDim() == DimY || msg->getDim() == DimUndefined )
	{
		if( true )
		{
			d_slices[ DimX ].d_spec->setOrigin( DimY, pos[ DimY ] );
			d_slices[ DimX ].d_viewer->redraw();
			d_slices[ DimY ].d_cur->setCursor( (Dimension)DimY, pos[ DimY ] );
			for( Dimension d = 2; d < d_spec->getDimCount(); d++ )
			{
				d_slices[ d ].d_spec->setOrigin( DimY, pos[ DimY ] );
				d_slices[ d ].d_viewer->redraw();
			}
		}
		d_cursor[ DimY ] = pos[ DimY ];
	}
	if( msg->getDim() == DimX || msg->getDim() == DimUndefined )
	{
		if( true )
		{
			d_slices[ DimY ].d_spec->setOrigin( DimX, pos[ DimX ] );
			d_slices[ DimY ].d_viewer->redraw();
			d_slices[ DimX ].d_cur->setCursor( (Dimension)DimX, pos[ DimX ] );
			for( Dimension d = 2; d < d_spec->getDimCount(); d++ )
			{
				d_slices[ d ].d_spec->setOrigin( DimX, pos[ DimX ] );
				d_slices[ d ].d_viewer->redraw();
			}
		}
		d_cursor[ DimX ] = pos[ DimX ];
	}
	notifyCursor();
}

void PhaserAgent::updateSlice(Dimension dim, CursorMdl::Update *msg)
{
	d_cursor[ dim ] = msg->getX();
	
	if( dim < 2 )
	{
		d_plane.d_cur->setCursor( dim, msg->getX() ); // Beide Dims gleich
		for( Dimension d = 2; d < d_spec->getDimCount(); d++ )
		{
			d_slices[ d ].d_spec->setOrigin( dim, msg->getX() );
			d_slices[ d ].d_viewer->redraw();
		}
	}else
	{
		registerPlane();
		d_slices[ DimX ].d_spec->setOrigin( dim, msg->getX() );
		d_slices[ DimX ].d_viewer->redraw();
		d_slices[ DimY ].d_spec->setOrigin( dim, msg->getX() );
		d_slices[ DimY ].d_viewer->redraw();
		Lexi::Viewport::pushHourglass();
		d_plane.d_spec->setOrigin( dim, msg->getX() );
		d_plane.d_viewer->damageMe();
		Lexi::Viewport::popCursor();
	}
	notifyCursor();
}

void PhaserAgent::updatePlane(ViewAreaMdl::Update * msg)
{
	if( !true || msg->getType() != ViewAreaMdl::Update::Range )
		return;
	registerPlane();
	if( msg->getX() && d_slices[ DimX ].d_viewer )
	{
		d_slices[ DimX ].d_viewer->getViewArea()->setRange( DimX, 
			d_plane.d_viewer->getViewArea()->getRange( DimX ) );
		d_slices[ DimX ].d_viewer->redraw();
	}
	if( msg->getY() && d_slices[ DimY ].d_viewer )
	{
		d_slices[ DimY ].d_viewer->getViewArea()->setRange( DimY, 
			d_plane.d_viewer->getViewArea()->getRange( DimY ) );
		d_slices[ DimY ].d_viewer->redraw();
	}
}

void PhaserAgent::updateSlice(Dimension dim, ViewAreaMdl::Update *msg)
{
	if( msg->getType() != ViewAreaMdl::Update::Range )
		return;
	if( dim < 2 )
	{
		d_plane.d_viewer->getViewArea()->setRange( dim, 
			d_slices[ dim ].d_viewer->getViewArea()->getRange( dim ) );
		d_plane.d_viewer->damageMe();
	}
}

void PhaserAgent::registerPlane()
{
	PpmCube cube;
	cube.assign( d_spec->getDimCount(), PpmRange() );
	cube[ DimX ] = d_plane.d_viewer->getViewArea()->getRange( DimX );
	cube[ DimY ] = d_plane.d_viewer->getViewArea()->getRange( DimY );
	for( Dimension d = 2; d < d_slices.size(); ++d )
	{
		if( true && d_slices[ d ].d_viewer )
			cube[ d ] = d_slices[ d ].d_viewer->getViewArea()->getRange( DimY );
		else
			cube[ d ] = d_spec->getScale( d ).getRange();
	}
	d_backward.push_back( std::make_pair( cube, d_cursor ) );
}

void PhaserAgent::initOrigin()
{
	d_cursor[ DimX ] = d_plane.d_viewer->getViewArea()->getRange( DimX ).first;
	d_cursor[ DimY ] = d_plane.d_viewer->getViewArea()->getRange( DimY ).first;
	d_plane.d_cur->setCursor( d_cursor[ DimX ], d_cursor[ DimY ] );
	for( Dimension d = 2; d < d_slices.size(); ++d )
	{
		d_cursor[ d ] = d_spec->getScale( d ).getRange().first;
		d_slices[ d ].d_cur->setCursor( (Dimension)DimY, d_cursor[ d ] );
	}
}

void PhaserAgent::notifyCursor()
{
	QString str;
    QTextStream ts( &str, QIODevice::WriteOnly );

	ts.setf( QTextStream::fixed );
	ts.precision( 3 );

	ts <<  "Cursor:  ";

	for( Dimension d = 0; d < d_cursor.size(); d++ )
	{
		ts << char( 'x' + d ) << ": ";
		ts << d_spec->getDimName( d ) << "=";
		ts << d_cursor[ d ];
		if( d_folding )
			ts << " (" << d_spec->getScale( d ).getRangeOffset( d_cursor[ d ] ) << ")  ";
		else
			ts << "  ";
	}

	try
	{
		Amplitude val = d_spec->Spectrum::getAt( d_cursor, d_folding, d_folding );
		ts.setf( QTextStream::showpos );
		ts.precision( 0 );
		ts << "Level=" << val;
	}catch( ... )
	{
		ts << " Out of Spectrum";
	}
	Lexi::ShowStatusMessage msg( str );
	traverseUp( msg );
}

void PhaserAgent::fitToView()
{
	d_plane.d_buf->fitToArea();
	d_plane.d_viewer->damageMe();
	if( true )
	{
		for( Dimension d = 0; d < d_slices.size(); ++d )
		{
			d_slices[ d ].d_buf->fitToArea();
			d_slices[ d ].d_viewer->redraw();
		}
	}
	initOrigin(); // Wichtig, damit richtige Plane dargestellt wird.
}

void PhaserAgent::setCurDim(Dimension d)
{
	if( d_curDim == d )
		return;
	if( d < 0 || d >= d_spec->getDimCount() )
		return;
	d_curDim = d;
	updatePane();
}

void PhaserAgent::showContour(bool on, bool redraw )
{
	if( d_showContour == on )
		return;
	d_showContour = on;
	if( d_showContour )
	{
		ContourView* v = new ContourView( d_plane.d_buf, d_autoContour );
		d_plane.d_viewer->getViews()->replace( BACKGROUND, new Lexi::Background() );
		d_plane.d_viewer->getViews()->replace( CONTOUR, v );
		if( d_autoContour )
		{
			v->createLevelsAuto( d_contourFactor, d_contourOption, d_gain );
		}else
			v->createLevelsMin( d_contourFactor, d_contourLevel, d_contourOption );
	}else
	{
		d_plane.d_viewer->getViews()->replace( BACKGROUND, 0 );
		d_plane.d_viewer->getViews()->replace( CONTOUR, 0 );
	}
	if( redraw )
		d_plane.d_viewer->damageMe();
}

void PhaserAgent::showIntens(bool on, bool redraw )
{
	if( d_showIntens == on )
		return;
	d_showIntens = on;
	if( d_showIntens )
	{
		IntensityView* v = new IntensityView( d_plane.d_buf );
		Lexi::Viewport::pushHourglass();
		v->reload();
		Lexi::Viewport::popCursor();
		d_plane.d_viewer->getViews()->replace( INTENSITY, v );
	}else
	{
		d_plane.d_viewer->getViews()->replace( INTENSITY, 0 );
	}
	if( redraw )
		d_plane.d_viewer->damageMe();
}

void PhaserAgent::setContourParams(Amplitude a, float f, ContourView::Option o)
{
	d_contourLevel = a;
	d_contourOption = o;
	d_contourFactor = f;
	d_autoContour = false;
	showContour( false );
	showContour( true );
}

void PhaserAgent::setAutoContour(bool on)
{
	if( d_autoContour == on )
		return;
	d_autoContour = on;
	showContour( false );
	showContour( true );
}

void PhaserAgent::setCursor(const PpmPoint & p)
{
	assert( p.size() == d_spec->getDimCount() );
	d_cursor = p;
	d_plane.d_cur->setCursor( d_cursor[ DimX ], d_cursor[ DimY ] );
	for( Dimension d = 2; d < d_slices.size(); ++d )
	{
		d_slices[ d ].d_cur->setCursor( (Dimension)DimY, d_cursor[ d ] );
	}
}

void PhaserAgent::updatePane()
{
	// TODO
	QString str;
	if( d_curDim != -1 )
	{
		str.sprintf( "Dim. %c\nPhi 0: %0.1f\nPhi 1: %0.1f\nPivot: %0.1f",
			char( 'x' + d_curDim ), 
			d_spec->getPhi( d_curDim ), d_spec->getPsi( d_curDim ),
			d_spec->getPivot( d_curDim ) );
	}
	d_phaser.d_label->setText( str );
	d_phaser.d_viewer->redraw();
}

void PhaserAgent::setPhiPsi(float phi, float psi, bool all)
{
	d_spec->setPhiPsi( d_curDim, phi, psi, all );
	if( !all )
	{
		d_slices[ d_curDim ].d_buf->reload();
		d_slices[ d_curDim ].d_viewer->redraw();
	}
	updatePane();
}

void PhaserAgent::handleSetResolution(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	bool ok	= FALSE;
	int res	= QInputDialog::getInteger( "Set Resolution", 
		"Please	enter the minimal number of pixels per sample:", 
		d_resol, 1, 20, 1,	&ok, MW );
	if( ok )
	{
		d_resol = res;
		d_lowResol = true;
		Viewport::pushHourglass();
		d_plane.d_buf->setResolution( d_resol );
		d_plane.d_viewer->damageMe();
		Viewport::popCursor();
	}
}

void PhaserAgent::handleShowLowRes(Action &a)
{
	ACTION_CHECKED_IF( a, true, d_lowResol );

	Viewport::pushHourglass();
	d_lowResol = !d_lowResol;
	if( d_lowResol )
		d_plane.d_buf->setResolution( d_resol );
	else
		d_plane.d_buf->setScaling( false );
	d_plane.d_viewer->damageMe();
	Viewport::popCursor();
}

void PhaserAgent::handleFitWindow(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	fitToView();
}

void PhaserAgent::handleForward(Action & a)
{
	ACTION_ENABLED_IF( a, d_forward.size()	> 0 );

	Viewport::pushHourglass();
	d_backward.push_back( d_forward.back() );
	const PpmCube& cube = d_forward.back().first;
	d_cursor = d_backward.back().second;
	d_plane.d_viewer->getViewArea()->setRange( DimX, cube[ DimX ] );
	d_plane.d_viewer->getViewArea()->setRange( DimY, cube[ DimY ] );
	d_plane.d_cur->setCursor( d_cursor[ DimX ], d_cursor[ DimY ] );
	if( true )
	{
		for( Dimension d = 0; d < d_slices.size(); ++d )
		{
			d_slices[ d ].d_cur->setCursor( (Dimension)DimY, d_cursor[ d ] );
			d_slices[ d ].d_viewer->getViewArea()->setRange( 
				d_slices[ d ].d_viewer->getViewArea()->getDim(), cube[ d ] );
			d_slices[ d ].d_viewer->redraw();
		}
	}
	d_forward.pop_back();
	d_plane.d_viewer->damageMe();
	Viewport::popCursor();
}

void PhaserAgent::handleBackward(Action & a)
{ 
	ACTION_ENABLED_IF( a, d_backward.size() > 1 );

	Viewport::pushHourglass();
	d_forward.push_back( d_backward.back() );
	const PpmCube& cube = d_backward.back().first;
	d_cursor = d_forward.back().second;
	d_plane.d_viewer->getViewArea()->setRange( DimX, cube[ DimX ] );
	d_plane.d_viewer->getViewArea()->setRange( DimY, cube[ DimY ] );
	d_plane.d_cur->setCursor( d_cursor[ DimX ], d_cursor[ DimY ] );
	if( true )
	{
		for( Dimension d = 0; d < d_slices.size(); ++d )
		{
			d_slices[ d ].d_cur->setCursor( (Dimension)DimY, d_cursor[ d ] );
			d_slices[ d ].d_viewer->getViewArea()->setRange( 
				d_slices[ d ].d_viewer->getViewArea()->getDim(), cube[ d ] );
			d_slices[ d ].d_viewer->redraw();
		}
	}
	d_backward.pop_back();
	d_plane.d_viewer->damageMe();
	Viewport::popCursor();
}

void PhaserAgent::handleShowFolded(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_folding );

	Viewport::pushHourglass();
	d_folding = !d_folding;
	d_plane.d_buf->setFolding( d_folding );
	if( d_folding )
		d_plane.d_viewer->getViews()->replace( FOLDING, new FoldingView( d_plane.d_buf ) );
	else
		d_plane.d_viewer->getViews()->replace( FOLDING, 0 );
	d_plane.d_viewer->damageMe();
	if( true )
		for( Dimension d = 0; d < d_slices.size(); ++d )
		{
			d_slices[ d ].d_buf->setFolding( d_folding );
			d_slices[ d ].d_viewer->redraw();
		}
	Viewport::popCursor();
}


void PhaserAgent::handleGoto(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	PpmPoint orig = getCursor();
	GotoDlg dlg( MW );
	PpmCube cube;
	d_spec->getCube( cube );
	Dimension d;
	for( d = 0; d <	d_spec->getDimCount(); d++	)
		dlg.addDimension( cube[	d ], d_spec->getColor( d ), 
			orig[ d ] );
	if( dlg.exec() )
	{
		for( d = 0; d <	d_spec->getDimCount(); d++ )
		{
			orig[ d ] = dlg.getValue( d );
		}
		setCursor( orig );
	}
}

void PhaserAgent::handleShowContour(Action & a)
{
	ACTION_CHECKED_IF( a, true, showContour() );

	showContour( !showContour() );
}

void PhaserAgent::handleShowIntensity(Action & a)
{
	ACTION_CHECKED_IF( a, true, showIntens() );

	showIntens( !showIntens() );
}

void PhaserAgent::handleAutoContour(Action & a)
{
	ACTION_CHECKED_IF( a, true, isAutoContour() );
	
	setAutoContour( !isAutoContour() );
}

void PhaserAgent::handleContourParams(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	Dlg::ContourParams p;
	p.d_factor = getContourFactor();
	p.d_threshold =	getContourLevel();
	p.d_option = getContourOption();
	if( Dlg::setParams( MW, p ) )
	{
		showIntens( false );
		setContourParams( p.d_threshold, p.d_factor, p.d_option );
	}
}

void PhaserAgent::handleSpecRotate(Root::Action & a)
{
	SpectrumPeer* spec = dynamic_cast<SpectrumPeer*>( d_spec.deref() );
	ACTION_ENABLED_IF( a, spec );

	RotateDlg dlg( MW, "Original", "Mapped" );
	Rotation rot = spec->getRotation();
	SpectrumType* st = spec->getType();
	assert( st );
	QString s1, s2;
	for( Dimension d = 0; d < spec->getDimCount(); d++ )
	{
		s1.sprintf( "%s (%s)", 
			spec->getSpectrum()->getScale( d ).getColor().getIsoLabel(), 
			spec->getSpectrum()->getLabel( d ) );
		s2.sprintf( "%s (%s)", st->getColor( d ).getIsoLabel(), st->getName( d ).data() );
		dlg.addDimension( s1, s2 );
	}
	if( dlg.rotate( rot ) )
	{
		spec->setRotation( rot );
		fitToView();	// TODO: ev. Ausschnittsgrsse beibehalten.
	}
}

void PhaserAgent::handlePickBounds(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	try
	{
		Amplitude val = 0;
		val = d_spec->Spectrum::getAt( d_cursor, d_folding, d_folding );
		Amplitude _min, _max;
		if( val > 0.0 )
		{
			_max = val;
			_min = 0;
		}else if( val < 0.0 )
		{
			_max = 0;
			_min = val;
		}else
			return;
		for( Index i = 0; i < d_slices.size(); i++ )
		{
			d_slices[ i ].d_slice->setMinMax( _min, _max );
			d_slices[ i ].d_viewer->redraw();
		}
	}catch( ... )
	{
	}
}

void PhaserAgent::handleSetSliceMinMax(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	
	Amplitude _min, _max;
	_min = d_slices[ 0 ].d_slice->getMin();
	_max = d_slices[ 0 ].d_slice->getMax();
	if( Dlg::getBounds( MW, _min, _max ) )
		for( Index i = 0; i < d_slices.size(); i++ )
		{
			d_slices[ i ].d_slice->setMinMax( _min, _max );	
			d_slices[ i ].d_viewer->redraw();
		}
}

void PhaserAgent::handleSliceAutoScale(Action & a)
{
	ACTION_CHECKED_IF( a, true, 
		true && d_slices[ 0 ].d_slice->isAutoScale() );

	bool on = !d_slices[ 0 ].d_slice->isAutoScale();
	for( Index i = 0; i < d_slices.size(); i++ )
	{
		d_slices[ i ].d_slice->setAutoScale( on );
		d_slices[ i ].d_viewer->redraw();
	}
}

void PhaserAgent::handlePickBoundsSym(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	try
	{
		Amplitude val = 0;
		val = d_spec->Spectrum::getAt( d_cursor, d_folding, d_folding );
		Amplitude _min, _max;
		if( val > 0.0 )
		{
			_max = val;
			_min = -val;
		}else if( val < 0.0 )
		{
			_max = -val;
			_min = val;
		}else
			return;
		for( Index i = 0; i < d_slices.size(); i++ )
		{
			d_slices[ i ].d_slice->setMinMax( _min, _max );	
			d_slices[ i ].d_viewer->redraw();
		}
	}catch( ... )
	{
	}
}

void PhaserAgent::handleBackwardPlane(Action & a)
{
	ACTION_ENABLED_IF( a, d_spec->getDimCount() == 3 ); 

	d_slices[ DimZ ].d_cur->setCursor( (Dimension)DimY, 
		d_cursor[ DimZ ] - d_spec->getScale( DimZ ).getDelta() );  
}

void PhaserAgent::handleForwardPlane(Action & a)
{
	ACTION_ENABLED_IF( a, d_spec->getDimCount() == 3 ); 

	d_slices[ DimZ ].d_cur->setCursor( (Dimension)DimY, 
		d_cursor[ DimZ ] + d_spec->getScale( DimZ ).getDelta() );  
}

void PhaserAgent::handleSetPhase(Action & a)
{
	ACTION_ENABLED_IF( a, d_curDim != -1 ); 

	float phi = d_spec->getPhi( d_curDim );
	float psi = d_spec->getPsi( d_curDim );
	if( Dlg::getPhase( MW, phi, psi) )
	{
		d_spec->setPhiPsi( d_curDim, phi, psi, true );
		updatePane();
	}
}

void PhaserAgent::handleUseDimX(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_curDim == DimX );

	setCurDim( DimX );
}

void PhaserAgent::handleUseDimY(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_curDim == DimY );

	setCurDim( DimY );
}

void PhaserAgent::handleUseDimZ(Action & a)
{
	ACTION_CHECKED_IF( a, d_spec->getDimCount() > 2, d_curDim == DimZ );

	setCurDim( DimZ );
}

void PhaserAgent::handleSetPivot(Action & a)
{
	ACTION_ENABLED_IF( a, d_curDim != -1 ); 

	PPM pivot = d_spec->getPivot( d_curDim );
	bool ok	= FALSE;
	QString res;
	res.sprintf( "%0.3f", double( d_cursor[ d_curDim ] ) );
	res	= QInputDialog::getText( "Set Pivot", 
		"Please	enter a PPM value:", QLineEdit::Normal, 
		res, &ok, MW );
	if( !ok )
		return;
	pivot = res.toFloat( &ok );
	if( !ok || pivot <= 0.0 )
	{
		QMessageBox::critical( MW, "Set Pivot",
				"Invalid PPM value!", "&Cancel" );
		return;
	}
	d_spec->setPivot( d_curDim, pivot, true );
	Dimension d = DimX;
	if( d_curDim > DimX )
		d = DimY;
	d_slices[ d_curDim ].d_pivot->setCursor( d, pivot );
	if( d_curDim < DimZ )
		d_plane.d_pivot->setCursor( d, pivot );
	updatePane();
}

void PhaserAgent::handlePointPivot(Action & a)
{
	ACTION_ENABLED_IF( a, true ); 

	Dimension dd;
	for( Dimension d = 0; d < d_cursor.size(); d++ )
	{
		d_spec->setPivot( d, d_cursor[ d ], true );
		if( d > DimX )
			dd = DimY;
		d_slices[ d ].d_pivot->setCursor( dd, d_cursor[ d ] );
		if( d < DimZ )
			d_plane.d_pivot->setCursor( d, d_cursor[ d ] );
	}
	updatePane();
}

void PhaserAgent::handleAutoGain(Action & a)
{
	ACTION_ENABLED_IF( a, !a.getParam( 0 ).isNull() );

	float g = a.getParam( 0 ).getFloat();
	if( g <= 0.0 )
	{
		Root::ReportToUser::alert( this, "Set Auto Gain", "Invalid Gain Value" );
		return;
	}
	d_gain = g;
	d_autoContour = true;
	showContour( false, false );
	showContour( true, true );
}



