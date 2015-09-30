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

#include "SliceScopeAgent.h"
#include <qtextstream.h> 
#include <qinputdialog.h> 
#include <qmessagebox.h>
#include <Root/Any.h>
#include <Root/MakroCommand.h>
#include <Lexi/MainWindow.h>
#include <Lexi/Background.h>
#include <Gui/Menu.h>
#include <Lexi/Label.h>
#include <Lexi/ContextMenu.h>
#include <Spec/SpecProjector.h>
#include <Spec/SpectrumType.h>
#include <Spec/SpectrumPeer.h>
#include <Spec/Repository.h>
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
#include <SpecView/SliceView.h>
#include <SpecView/CenterLine.h>
#include <SpecView/SelectRulerCtr.h>
#include <Spec/GlobalCursor.h>
#include <SpecView/PeakSelectCtrl.h>

#include <SpecView/RotateDlg.h>
#include <Dlg.h>
#include <SpecView/GotoDlg.h>
using namespace Spec;

static const int BACKGROUND = 0;
static const int INTENSITY = 1;
static const int CONTOUR = 2;
static const int LABEL1 = 3;
static const int LABEL2 = 4;
static const int CURSORS = 5;
static const int PEAKS = 6;
static const int TUPLES = 7;
static const int VIEWCOUNT = 8;

//////////////////////////////////////////////////////////////////////

Action::CmdStr SliceScopeAgent::AddSpec = "AddSpec";
Action::CmdStr SliceScopeAgent::RemoveSpec = "RemoveSpec";
Action::CmdStr SliceScopeAgent::FitToWindow = "FitToWindow";
Action::CmdStr SliceScopeAgent::CursorSyncX = "CursorSyncX";
Action::CmdStr SliceScopeAgent::CursorSyncY = "CursorSyncY";
Action::CmdStr SliceScopeAgent::ShowSpins = "ShowSpins";
Action::CmdStr SliceScopeAgent::PickBounds = "PickBounds";
Action::CmdStr SliceScopeAgent::SetSliceMinMax = "SetSliceMinMax";
Action::CmdStr SliceScopeAgent::SliceAutoScale = "SliceAutoScale";
Action::CmdStr SliceScopeAgent::PickBoundsSym = "PickBoundsSym";
Action::CmdStr SliceScopeAgent::CreateReport = "CreateReport";
Action::CmdStr SliceScopeAgent::AutoCenter = "AutoCenter";
Action::CmdStr SliceScopeAgent::DeleteSpins = "DeleteSpins";
Action::CmdStr SliceScopeAgent::PickSpin = "PickSpin";
Action::CmdStr SliceScopeAgent::MoveSpin = "MoveSpin";
Action::CmdStr SliceScopeAgent::MoveSpinAlias = "MoveSpinAlias";
Action::CmdStr SliceScopeAgent::LabelSpin = "LabelSpin";
Action::CmdStr SliceScopeAgent::ForceLabelSpin = "ForceLabelSpin";
Action::CmdStr SliceScopeAgent::ShowFolded = "ShowFolded";
Action::CmdStr SliceScopeAgent::EditAtts = "EditAtts";
Action::CmdStr SliceScopeAgent::Calibrate = "Calibrate";

ACTION_SLOTS_BEGIN( SliceScopeAgent )
    { SliceScopeAgent::Calibrate, &SliceScopeAgent::handleCalibrate },
    { SliceScopeAgent::EditAtts, &SliceScopeAgent::handleEditAtts },
    { SliceScopeAgent::ShowFolded, &SliceScopeAgent::handleShowFolded },
    { SliceScopeAgent::ForceLabelSpin, &SliceScopeAgent::handleForceLabelSpin },
    { SliceScopeAgent::LabelSpin, &SliceScopeAgent::handleLabelSpin },
    { SliceScopeAgent::MoveSpinAlias, &SliceScopeAgent::handleMoveSpinAlias },
    { SliceScopeAgent::MoveSpin, &SliceScopeAgent::handleMoveSpin },
    { SliceScopeAgent::PickSpin, &SliceScopeAgent::handlePickSpin },
    { SliceScopeAgent::DeleteSpins, &SliceScopeAgent::handleDeleteSpins },
    { SliceScopeAgent::AutoCenter, &SliceScopeAgent::handleAutoCenter },
    { SliceScopeAgent::CreateReport, &SliceScopeAgent::handleCreateReport },
    { SliceScopeAgent::PickBoundsSym, &SliceScopeAgent::handlePickBoundsSym },
    { SliceScopeAgent::SliceAutoScale, &SliceScopeAgent::handleSliceAutoScale },
    { SliceScopeAgent::SetSliceMinMax, &SliceScopeAgent::handleSetSliceMinMax },
    { SliceScopeAgent::PickBounds, &SliceScopeAgent::handlePickBounds },
    { SliceScopeAgent::ShowSpins, &SliceScopeAgent::handleShowSpins },
    { SliceScopeAgent::CursorSyncX, &SliceScopeAgent::handleCursorSyncX },
    { SliceScopeAgent::CursorSyncY, &SliceScopeAgent::handleCursorSyncY },
    { SliceScopeAgent::FitToWindow, &SliceScopeAgent::handleFitToWindow },
ACTION_SLOTS_END( SliceScopeAgent )

//////////////////////////////////////////////////////////////////////
#define MW ((Lexi::MainWindow*)getParent())->getQt()

static QColor g_clrLabel = Qt::yellow;

SliceScopeAgent::SliceScopeAgent(Root::Agent* parent, 
				Spectrum* s, Project* pro, PeakList* pl ):
	Root::Agent( parent ), d_lock( false ), d_lane( -1 ), d_pl( pl )
{
	assert( s );
	if( pro )
	{
		d_pro = pro;
		d_pro->addObserver( this );
	}

	initParams();
	buildPopup();
	addSpec( s );
	d_slices[ 0 ].d_buf->fitToArea();	// TODO
	updateSpecPop();
}

SliceScopeAgent::~SliceScopeAgent()
{
	if( d_pro )
		d_pro->removeObserver( this );
	d_pro = 0;
	
	delete d_popSpec;
	delete d_popLabel;
	delete d_popSlice;

	GlobalCursor::removeObserver( this );
}

void SliceScopeAgent::buildPopup()
{
	d_popLabel = new Gui::Menu(); // Explizit lschen

	d_popSpec = new Gui::Menu();

	d_popSlice = new Gui::Menu();
	d_popSlice->insertItem( "Add Spectrum", d_popSpec );
	Gui::Menu::item( d_popSlice, this, "Remove Spectrum", RemoveSpec, false );
	Gui::Menu::item( d_popSlice, this, "Fit to Window", FitToWindow, false );
}

void SliceScopeAgent::createSlice(int i)
{
	SpecViewer* slice = new SpecViewer( 
		new ViewAreaMdl( true, false, true, false ), VIEWCOUNT );
	d_slices[ i ].d_viewer = slice;
	slice->getViewArea()->addObserver( this );

	d_slices[ i ].d_buf = new SpecBufferMdl( slice->getViewArea() );
	d_slices[ i ].d_buf->setFolding( d_folding );
	slice->getViews()->replace( INTENSITY, new SliceView( d_slices[ i ].d_buf ) );

	d_slices[ i ].d_cur = new CursorMdl();
	d_slices[ i ].d_cur->addObserver( this );
	CursorView* cv = new CursorView( slice, d_slices[ i ].d_cur );
	slice->getViews()->replace( CURSORS, cv );
	PeakColorList* clr = 0;
	if( d_pro )
		clr = d_pro->getRepository()->getColors();
	d_slices[ i ].d_peaks = new PeakSliceView( d_slices[ i ].d_viewer, 0, Qt::white, 0, clr );
	d_slices[ i ].d_viewer->getViews()->replace( PEAKS, d_slices[ i ].d_peaks );

	slice->getHandlers()->append( new ZoomCtrl( slice, true, false ) );
	slice->getHandlers()->append( new SelectZoomCtrl( slice, true, false ) );
	slice->getHandlers()->append( new ScrollCtrl( slice ) );
	slice->getHandlers()->append( new PeakSelect1DCtrl( d_slices[ i ].d_peaks, true ) );
	slice->getHandlers()->append( new CursorCtrl( cv, false ) );
	slice->getHandlers()->append( new Lexi::ContextMenu( d_popSlice, false ) );
	slice->getHandlers()->append( new FocusCtrl( slice, this ) );
}

void SliceScopeAgent::setSpec(int i, Spectrum * s, bool fit )
{
	assert( s && s->getDimCount() == 1 );
	d_specs.insert( s );

	d_slices[ i ].d_spec = s;
	d_slices[ i ].d_buf->setSpectrum( d_slices[ i ].d_spec );
	if( fit )
		d_slices[ i ].d_buf->fitToArea();	// TODO
}

void SliceScopeAgent::addSpec(Spectrum * s)
{
	assert( s && s->getDimCount() == 1 );
	d_lock = true;
	d_specs.insert( s );
	d_slices.push_back( SliceSocket() );
	createSlice( d_slices.size() - 1 );
	d_slices.back().d_spec = s;
	d_slices.back().d_buf->setSpectrum( d_slices.back().d_spec );
	d_slices.back().d_buf->fitToArea();	// TODO
	d_slices.back().d_viewer->getViews()->replace( LABEL1,
		new Lexi::Label( s->getName(), nil, g_clrLabel, 
		Lexi::AlignLeft, Lexi::AlignTop ) );
	for( int l = 0; l < d_slices.size(); l++ )
		d_slices[ l ].d_cur->setCursor( (Dimension)DimX, d_cursor );
	d_lock = false;
}

void SliceScopeAgent::removeSpec(int lane)
{
	d_lock = true;
	d_slices.erase( d_slices.begin() + lane );
	d_lock = false;
}

void SliceScopeAgent::initParams()
{
	d_folding = false;
	d_cursorSyncX = false;
	d_cursorSyncY = false;
}

void SliceScopeAgent::updateSpecPop()
{
	if( d_pro.isNull() )
		return;
	ColorMap a, b;
	( *d_specs.begin() )->getColors( a );
	Spectrum* spec = 0;
	const Project::SpectrumMap& sm = d_pro->getSpectra();
	Project::SpectrumMap::const_iterator p;
	AtomType atom = ( *d_specs.begin() )->getColor( DimX );
	for( p = sm.begin(); p != sm.end(); ++p )
	{
		spec = (*p).second;

		if( spec->getDimCount() == 1 && spec->getColor( DimX ) == atom )
		{
			Gui::Menu::item( d_popSpec, this, spec->getName(), 
				SliceScopeAgent::AddSpec, true )->addParam( Root::Any( spec ) );
		}
	}
}

void SliceScopeAgent::handle(Root::Message& msg)
{
	if( d_lock )
		return;
	d_lock = true;
	BEGIN_HANDLER();
	MESSAGE( ViewAreaMdl::Update, a, msg )
	{
		Lexi::Viewport::pushHourglass();
		Dimension d;
		for( d = 0; d < d_slices.size(); d++ )
			if( d_slices[ d ].d_viewer->getViewArea() == a->getOrigin() )
			{
				updateSlice( d, a );
				break;
			}

		Lexi::Viewport::popCursor();
		msg.consume();
	}
	MESSAGE( CursorMdl::Update, a, msg )
	{
		Dimension d;
		for( d = 0; d < d_slices.size(); d++ )
			if( d_slices[ d ].d_cur == a->getOrigin() )
			{
				updateSlice( d, a );
				break;
			}
		msg.consume();
	}
	MESSAGE( GlobalCursor::UpdatePos, a, msg )
	{
		d_lock = false;
		if( ( a->getDim() == DimY || a->getDim() == DimUndefined ) &&
			d_slices[ 0 ].d_spec->getColor( DimX ) == a->getTy() && d_cursorSyncY )
		{
			d_cursorSyncY = false;
			d_slices[ 0 ].d_cur->setCursor( Dimension( DimX ), a->getY() );
			d_cursorSyncY = true;
		}
		if( ( a->getDim() == DimX || a->getDim() == DimUndefined ) &&
			d_slices[ 0 ].d_spec->getColor( DimX ) == a->getTx() && d_cursorSyncX )
		{
			d_cursorSyncX = false;
			d_slices[ 0 ].d_cur->setCursor( Dimension( DimX ), a->getX() );
			d_cursorSyncX = true;
		}
		msg.consume();
	}
	MESSAGE( SpectrumPeer::Added, a, msg )
	{
        Q_UNUSED(a)
		updateSpecPop();
	}
	MESSAGE( SpectrumPeer::Removed, a, msg )
	{
        Q_UNUSED(a)
		updateSpecPop();
	}
	MESSAGE( FocusCtrl::FocusIn, a, msg )
	{
		for( int l = 0; l < d_slices.size(); l++ )
		{
			if( d_slices[ l ].d_spec && d_slices[ l ].d_viewer == a->getViewer() )
			{
				d_lane = l;
				break;
			}else
				d_lane = -1;
		}
		msg.consume();
	}
	MESSAGE( Root::Action, a, msg )
	{
		d_lock = false; // Kein Blocking fr Action-Ausfhrung
		EXECUTE_ACTION( SliceScopeAgent, *a );
	}
	END_HANDLER();
	d_lock = false;
}

void SliceScopeAgent::updateSlice(int lane, CursorMdl::Update *msg)
{
	// Auf einem Slice wurde der Cursor gendert
	d_cursor = msg->getX();
	for( int l = 0; l < d_slices.size(); l++ )
		if( l != lane )
		{
			d_slices[ l ].d_cur->setCursor( (Dimension)DimX, d_cursor );
		}
	notifyCursor();
	if( d_cursorSyncX )
		GlobalCursor::setCursor( DimX, msg->getX(), 
			d_slices[ lane ].d_spec->getColor( DimX ) );
	if( d_cursorSyncY )
		GlobalCursor::setCursor( DimY, msg->getX(), 
			d_slices[ lane ].d_spec->getColor( DimX ) );
}

void SliceScopeAgent::updateSlice(int lane, ViewAreaMdl::Update *msg)
{
	// In Slice wurde Ausschnitt gendert
	if( msg->getType() != ViewAreaMdl::Update::Range )
		return;

	for( int l = 0; l < d_slices.size(); l++ )
		if( l != lane )
		{
			d_slices[ l ].d_viewer->getViewArea()->setRange( DimX, 
				d_slices[ lane ].d_viewer->getViewArea()->getRange( DimX ) );
			d_slices[ l ].d_viewer->redraw();
		}
}

void SliceScopeAgent::notifyCursor()
{
	QString str;
    QTextStream ts( &str, QIODevice::WriteOnly );

	ts.setf( QTextStream::fixed );
	ts.precision( 3 );

	ts <<  "Cursor:  ";

	Spectrum* spec = *d_specs.begin();	// TODO

	ts << char( 'x' + DimX ) << ": ";
	ts << spec->getDimName( DimX ) << "=";
	ts << d_cursor << "  ";
	if( d_folding )
		ts << " (" << spec->getScale( DimX ).getRangeOffset( d_cursor ) << ")  ";
	else
		ts << "  ";

	try
	{
		Amplitude val = 0;
		PpmPoint p( d_cursor );
		val = spec->getAt( p, d_folding, d_folding );
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

void SliceScopeAgent::handleFitToWindow(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	d_slices.front().d_buf->fitToArea();	

	for( int l = 0; l < d_slices.size(); l++ )
		d_slices[ l ].d_viewer->redraw();
}

void SliceScopeAgent::handleCursorSyncX(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_cursorSyncX );
	
	d_cursorSyncX = !d_cursorSyncX;
	if( d_cursorSyncX )
		GlobalCursor::addObserver( this );	// TODO: preset Cursor
	else if( !d_cursorSyncY )
		GlobalCursor::removeObserver( this );
}

void SliceScopeAgent::handleCursorSyncY(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_cursorSyncY );
	
	d_cursorSyncY = !d_cursorSyncY;
	if( d_cursorSyncY )
		GlobalCursor::addObserver( this );	// TODO: preset Cursor
	else if( !d_cursorSyncX )
		GlobalCursor::removeObserver( this );
}

void SliceScopeAgent::handleShowSpins(Action &)
{

}

void SliceScopeAgent::handlePickBounds(Action &)
{

}

void SliceScopeAgent::handleSetSliceMinMax(Action &)
{

}

void SliceScopeAgent::handleSliceAutoScale(Action &)
{

}

void SliceScopeAgent::handlePickBoundsSym(Action &)
{

}

void SliceScopeAgent::handleCreateReport(Action &)
{

}

void SliceScopeAgent::handleAutoCenter(Action &)
{

}

void SliceScopeAgent::handleDeleteSpins(Action &)
{

}

void SliceScopeAgent::handlePickSpin(Action &)
{

}

void SliceScopeAgent::handleMoveSpin(Action &)
{

}

void SliceScopeAgent::handleMoveSpinAlias(Action &)
{

}

void SliceScopeAgent::handleLabelSpin(Action &)
{

}

void SliceScopeAgent::handleForceLabelSpin(Action &)
{

}

void SliceScopeAgent::handleShowFolded(Action &)
{

}

void SliceScopeAgent::handleEditAtts(Action &)
{

}

void SliceScopeAgent::handleCalibrate(Action &)
{

}


