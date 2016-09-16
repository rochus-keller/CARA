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

#include "MonoScopeAgent.h"
#include <math.h>
#include <qtextstream.h> 
#include <qinputdialog.h> 
#include <qcolordialog.h>
#include <qmessagebox.h>
#include <Root/MessageLog.h>
#include <Lexi/MainWindow.h>
#include <Lexi/Background.h>
#include <Lexi/Label.h>
#include <Lexi/ContextMenu.h>
#include <Spec/SpecProjector.h>
#include <Spec/SpectrumType.h>
#include <Spec/SpectrumPeer.h>
#include <Spec/PeakListPeer.h>
#include <Spec/PeakProjector.h>
#include <Spec/Repository.h>
#include <Spec/SpecRotator.h>
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
#include <Spec/GlobalCursor.h>
#include <SpecView/DynValueEditor.h>

#include <SpecView/PeakModelTuner.h>
#include <SpecView/RotateDlg.h>
#include <Dlg.h>
#include <SpecView/GotoDlg.h>
#include <SpecView/PeakBatchCurve.h>
#include <ReportViewer.h>
#include <SpectrumListView.h>
#include <SpecView/SpecBatchList.h>

using namespace Spec;

static const int BACKGROUND = 0;
static const int INTENSITY = 1;
static const int CONTOUR = 2;
static const int FOLDING = 3;
static const int CURSORS = 4;
static const int PEAKS = 5;
static const int LABEL1 = 6;
static const int LABEL2 = 7;
static const int VIEWCOUNT = 8;

static const bool s_showContour = true;
static const float s_contourFactor = 1.4f;
static const ContourView::Option s_contourOption = ContourView::Both;
static const float s_gain = 2.0;
static const bool s_autoContour = true;

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr MonoScopeAgent::SetResolution = "SetResolution";
Root::Action::CmdStr MonoScopeAgent::ShowLowRes = "ShowLowRes";
Root::Action::CmdStr MonoScopeAgent::Forward = "Forward";
Root::Action::CmdStr MonoScopeAgent::Backward = "Backward";
Root::Action::CmdStr MonoScopeAgent::FitWindow = "FitWindow";
Root::Action::CmdStr MonoScopeAgent::FitWindowX = "FitWindowX";
Root::Action::CmdStr MonoScopeAgent::FitWindowY = "FitWindowY";
Root::Action::CmdStr MonoScopeAgent::PickPeak = "PickPeak";
Root::Action::CmdStr MonoScopeAgent::MovePeak = "MovePeak";
Root::Action::CmdStr MonoScopeAgent::MovePeakAlias = "MovePeakAlias";
Root::Action::CmdStr MonoScopeAgent::LabelPeak = "LabelPeak";
Root::Action::CmdStr MonoScopeAgent::ForceLabel = "ForceLabel";
Root::Action::CmdStr MonoScopeAgent::DeletePeaks = "DeletePeaks";
Root::Action::CmdStr MonoScopeAgent::PeakCalibrate = "PeakCalibrate";
Root::Action::CmdStr MonoScopeAgent::ShowFolded = "ShowFolded";
Root::Action::CmdStr MonoScopeAgent::ViewLabels = "ViewLabels";
Root::Action::CmdStr MonoScopeAgent::SpecCalibrate = "SpecCalibrate";
Root::Action::CmdStr MonoScopeAgent::AutoCenter = "AutoCenter";
Root::Action::CmdStr MonoScopeAgent::SpecRotate = "SpecRotate";
Root::Action::CmdStr MonoScopeAgent::Goto = "Goto";
Root::Action::CmdStr MonoScopeAgent::ShowContour = "ShowContour";
Root::Action::CmdStr MonoScopeAgent::ShowIntensity = "ShowIntensity";
Root::Action::CmdStr MonoScopeAgent::AutoContour = "AutoContour";
Root::Action::CmdStr MonoScopeAgent::ContourParams = "ContourParams";
Root::Action::CmdStr MonoScopeAgent::PeakRotate = "PeakRotate";
Root::Action::CmdStr MonoScopeAgent::MapPeakList = "MapPeakList";
Root::Action::CmdStr MonoScopeAgent::DeleteAliasPeaks = "DeleteAliasPeaks";

Root::Action::CmdStr MonoScopeAgent::ShowModel = "ShowModel";
Root::Action::CmdStr MonoScopeAgent::ShowBaseWidth = "ShowBaseWidth";
Root::Action::CmdStr MonoScopeAgent::TunePeakModel = "TunePeakModel";
Root::Action::CmdStr MonoScopeAgent::UpdateAllAmps = "UpdateAllAmps";
Root::Action::CmdStr MonoScopeAgent::IntegrateAll = "IntegrateAll";
Root::Action::CmdStr MonoScopeAgent::IntegrateSel = "IntegrateSel";
Root::Action::CmdStr MonoScopeAgent::ShowBackCalc = "ShowBackCalc";
Root::Action::CmdStr MonoScopeAgent::ShowDiff = "ShowDiff";
Root::Action::CmdStr MonoScopeAgent::ExactBackCalc = "ExactBackCalc";
Root::Action::CmdStr MonoScopeAgent::SelectSpec = "SelectSpec";
Root::Action::CmdStr MonoScopeAgent::BatchIntegrate = "BatchIntegrate";
Root::Action::CmdStr MonoScopeAgent::PeakCurve = "PeakCurve";

Root::Action::CmdStr MonoScopeAgent::PickBounds = "PickBounds";
Root::Action::CmdStr MonoScopeAgent::SetSliceMinMax = "SetSliceMinMax";
Root::Action::CmdStr MonoScopeAgent::SliceAutoScale = "SliceAutoScale";
Root::Action::CmdStr MonoScopeAgent::PickBoundsSym = "PickBoundsSym";

Root::Action::CmdStr MonoScopeAgent::ForwardPlane = "ForwardPlane";
Root::Action::CmdStr MonoScopeAgent::BackwardPlane = "BackwardPlane";
Root::Action::CmdStr MonoScopeAgent::CreateReport = "CreateReport";
Root::Action::CmdStr MonoScopeAgent::EditAtts = "EditAtts";
Root::Action::CmdStr MonoScopeAgent::SetColor = "SetColor";
Root::Action::CmdStr MonoScopeAgent::SelectPeaks = "SelectPeaks";
Root::Action::CmdStr MonoScopeAgent::CursorSync = "CursorSync";
Root::Action::CmdStr MonoScopeAgent::CalcMaximum = "CalcMaximum";
Root::Action::CmdStr MonoScopeAgent::AutoGain = "AutoGain";
Root::Action::CmdStr MonoScopeAgent::RangeSync = "RangeSync";
Root::Action::CmdStr MonoScopeAgent::SetDepth = "SetDepth";

Root::Action::CmdStr MonoScopeAgent::OverlayCount = "OverlayCount";
Root::Action::CmdStr MonoScopeAgent::ActiveOverlay = "ActiveOverlay";
Root::Action::CmdStr MonoScopeAgent::SetPosColor = "SetPosColor";
Root::Action::CmdStr MonoScopeAgent::SetNegColor = "SetNegColor";
Root::Action::CmdStr MonoScopeAgent::OverlaySpec = "OverlaySpec";
Root::Action::CmdStr MonoScopeAgent::CntFactor = "CntFactor";
Root::Action::CmdStr MonoScopeAgent::CntThreshold = "CntThreshold";
Root::Action::CmdStr MonoScopeAgent::CntOption = "CntOption";
Root::Action::CmdStr MonoScopeAgent::AddLayer = "AddLayer";
Root::Action::CmdStr MonoScopeAgent::ComposeLayers = "ComposeLayers";
Root::Action::CmdStr MonoScopeAgent::SyncDepth = "SyncDepth";
Root::Action::CmdStr MonoScopeAgent::AdjustIntensity = "AdjustIntensity";
Root::Action::CmdStr MonoScopeAgent::SetIntMeth = "SetIntMeth";

ACTION_SLOTS_BEGIN( MonoScopeAgent )
    { MonoScopeAgent::SetIntMeth, &MonoScopeAgent::handleSetIntMeth },
    { MonoScopeAgent::AdjustIntensity, &MonoScopeAgent::handleAdjustIntensity },
    { MonoScopeAgent::DeleteAliasPeaks, &MonoScopeAgent::handleDeleteAliasPeaks },
    { MonoScopeAgent::FitWindowX, &MonoScopeAgent::handleFitWindowX },
    { MonoScopeAgent::FitWindowY, &MonoScopeAgent::handleFitWindowY },
    { MonoScopeAgent::SyncDepth, &MonoScopeAgent::handleSyncDepth },
    { MonoScopeAgent::ComposeLayers, &MonoScopeAgent::handleComposeLayers },
    { MonoScopeAgent::AddLayer, &MonoScopeAgent::handleAddLayer },
    { MonoScopeAgent::CntFactor, &MonoScopeAgent::handleCntFactor },
    { MonoScopeAgent::CntThreshold, &MonoScopeAgent::handleCntThreshold },
    { MonoScopeAgent::CntOption, &MonoScopeAgent::handleCntOption },
    { MonoScopeAgent::OverlaySpec, &MonoScopeAgent::handleOverlaySpec },
    { MonoScopeAgent::OverlayCount, &MonoScopeAgent::handleOverlayCount },
    { MonoScopeAgent::ActiveOverlay, &MonoScopeAgent::handleActiveOverlay },
    { MonoScopeAgent::SetPosColor, &MonoScopeAgent::handleSetPosColor },
    { MonoScopeAgent::SetNegColor, &MonoScopeAgent::handleSetNegColor },
    { MonoScopeAgent::SetDepth, &MonoScopeAgent::handleSetDepth },
    { MonoScopeAgent::RangeSync, &MonoScopeAgent::handleRangeSync },
    { MonoScopeAgent::AutoGain, &MonoScopeAgent::handleAutoGain },
    { MonoScopeAgent::CalcMaximum, &MonoScopeAgent::handleCalcMaximum },
    { MonoScopeAgent::CursorSync, &MonoScopeAgent::handleCursorSync },
    { MonoScopeAgent::SetColor, &MonoScopeAgent::handleSetColor },
    { MonoScopeAgent::EditAtts, &MonoScopeAgent::handleEditAtts },
    { MonoScopeAgent::MovePeakAlias, &MonoScopeAgent::handleMovePeakAlias },
    { MonoScopeAgent::CreateReport, &MonoScopeAgent::handleCreateReport },
    { MonoScopeAgent::BackwardPlane, &MonoScopeAgent::handleBackwardPlane },
    { MonoScopeAgent::ForwardPlane, &MonoScopeAgent::handleForwardPlane },
    { MonoScopeAgent::PeakCurve, &MonoScopeAgent::handlePeakCurve },
    { MonoScopeAgent::PickBounds, &MonoScopeAgent::handlePickBounds },
    { MonoScopeAgent::SetSliceMinMax, &MonoScopeAgent::handleSetSliceMinMax },
    { MonoScopeAgent::SliceAutoScale, &MonoScopeAgent::handleSliceAutoScale },
    { MonoScopeAgent::PickBoundsSym, &MonoScopeAgent::handlePickBoundsSym },
    { MonoScopeAgent::BatchIntegrate, &MonoScopeAgent::handleBatchIntegrate },
    { MonoScopeAgent::ShowModel, &MonoScopeAgent::handleShowModel },
    { MonoScopeAgent::ShowBaseWidth, &MonoScopeAgent::handleShowBaseWidth },
    { MonoScopeAgent::TunePeakModel, &MonoScopeAgent::handleTunePeakModel },
    { MonoScopeAgent::UpdateAllAmps, &MonoScopeAgent::handleUpdateAllAmps },
    { MonoScopeAgent::IntegrateAll, &MonoScopeAgent::handleIntegrateAll },
    { MonoScopeAgent::IntegrateSel, &MonoScopeAgent::handleIntegrateSel },
    { MonoScopeAgent::ShowBackCalc, &MonoScopeAgent::handleShowBackCalc },
    { MonoScopeAgent::ShowDiff, &MonoScopeAgent::handleShowDiff },
    { MonoScopeAgent::ExactBackCalc, &MonoScopeAgent::handleExactBackCalc },
    { MonoScopeAgent::ForceLabel, &MonoScopeAgent::handleForceLabel },
    { MonoScopeAgent::MapPeakList, &MonoScopeAgent::handleMapPeakList },
    { MonoScopeAgent::PeakRotate, &MonoScopeAgent::handlePeakRotate },
    { MonoScopeAgent::Goto, &MonoScopeAgent::handleGoto },
    { MonoScopeAgent::ContourParams, &MonoScopeAgent::handleContourParams },
    { MonoScopeAgent::AutoContour, &MonoScopeAgent::handleAutoContour },
    { MonoScopeAgent::ShowIntensity, &MonoScopeAgent::handleShowIntensity },
    { MonoScopeAgent::ShowContour, &MonoScopeAgent::handleShowContour },
    { MonoScopeAgent::SpecRotate, &MonoScopeAgent::handleSpecRotate },
    { MonoScopeAgent::AutoCenter, &MonoScopeAgent::handleAutoCenter },
    { MonoScopeAgent::ShowFolded, &MonoScopeAgent::handleShowFolded },
    { MonoScopeAgent::ViewLabels, &MonoScopeAgent::handleViewLabels },
    { MonoScopeAgent::PeakCalibrate, &MonoScopeAgent::handlePeakCalibrate },
    { MonoScopeAgent::SpecCalibrate, &MonoScopeAgent::handleSpecCalibrate },
    { MonoScopeAgent::PickPeak, &MonoScopeAgent::handlePickPeak },
    { MonoScopeAgent::MovePeak, &MonoScopeAgent::handleMovePeak },
    { MonoScopeAgent::DeletePeaks, &MonoScopeAgent::handleDeletePeaks },
    { MonoScopeAgent::Forward, &MonoScopeAgent::handleForward },
    { MonoScopeAgent::Backward, &MonoScopeAgent::handleBackward },
    { MonoScopeAgent::ShowLowRes, &MonoScopeAgent::handleShowLowRes },
    { MonoScopeAgent::SetResolution, &MonoScopeAgent::handleSetResolution },
    { MonoScopeAgent::FitWindow, &MonoScopeAgent::handleFitWindow },
ACTION_SLOTS_END( MonoScopeAgent )

//////////////////////////////////////////////////////////////////////

#define MW ((Lexi::MainWindow*)getParent())->getQt()

MonoScopeAgent::MonoScopeAgent(Root::Agent* parent, Spectrum* spec, 
							   Project* pro, const Rotation& rot ):
	Root::Agent( parent ),  
		d_lock( false ), d_pro( pro ), d_aol( 0 ), d_pp( 0 )
{
	assert( spec );
	d_rot = rot;
	if( d_rot.empty() )
	{
		d_main = spec;
	}else
	{
		assert( d_rot.size() == spec->getDimCount() );
		d_main = new SpecRotator( spec, d_rot );
	}
	d_spec = d_main;
	initParams();
	d_cursor.assign( spec->getDimCount(), 0 );
	d_back = new BackCalculation( d_spec );
	buildPopup();
}

void MonoScopeAgent::initParams()
{
	d_resol = 1;
	d_folding = false;
	d_lowResol = false;
#ifdef _DEBUG
	d_showSlices = true; 
#else
	d_showSlices = true; 
#endif
	d_autoCenter = true;
	d_showModel = false;
	d_showBase = false;
	d_showBack = false;
	d_cursorSync = false;
	d_syncDepth = false;
	d_rangeSync = false;
	d_intMeth = PeakList::LinEq;
	// TODO: diese Werte sollen ab Konfigurations-Record gelsen werden
}

MonoScopeAgent::~MonoScopeAgent()
{
	delete d_popSpec;
	delete d_popPlane;
	delete d_popHisto;


	GlobalCursor::removeObserver( this );
}

void MonoScopeAgent::buildPopup()
{
	d_popSpec = new Gui::Menu();
	d_popPlane = new Gui::Menu();
	d_popHisto = new Gui::Menu();

	Gui::Menu::item( d_popPlane, this, "Pick Peak", PickPeak, false );
	Gui::Menu::item( d_popPlane, this, "Move Peak", MovePeak, false );
	Gui::Menu::item( d_popPlane, this, "Move Peak Alias", MovePeakAlias, false );
	Gui::Menu::item( d_popPlane, this, "Force Peak Label", ForceLabel, false );
	Gui::Menu::item( d_popPlane, this, "Un-Alias Peaks", DeleteAliasPeaks, false );
	Gui::Menu::item( d_popPlane, this, "Delete Peaks", DeletePeaks, false );
	Gui::Menu::item( d_popPlane, this, "Set Color...", SetColor, false );
	Gui::Menu::item( d_popPlane, this, "Edit Attributes...", EditAtts, false );
	d_popPlane->insertSeparator();
	d_popPlane->insertItem( "Select Spectrum", d_popSpec );
	Gui::Menu::item( d_popPlane, this, "Show Model", ShowModel, true );
	Gui::Menu::item( d_popPlane, this, "Tune Model", TunePeakModel, false );
	d_popPlane->insertSeparator();
	Gui::Menu::item( d_popPlane, this, "Fit Window", FitWindow, false );

}

void MonoScopeAgent::reloadSpecPop()
{
	d_popSpec->purge();
	if( d_peaks.isNull() || d_pro.isNull() )
		return;
	PeakList::SpecList::const_iterator p1;
	Spectrum* spec;

	Gui::Menu::item( d_popSpec, this, d_main->getName(), 
		SelectSpec, true )->addParam( Root::Any( d_main ) );
	d_popSpec->insertSeparator();

	QString str;
	for( p1 = d_peaks->getSpecs().begin(); p1 != d_peaks->getSpecs().end(); ++p1 )
	{
		spec = d_pro->getSpec( (*p1) );
		if( spec && spec->getType() == d_main->getType() )	// RISK
		{
			str.sprintf( "%d %s", spec->getId(), spec->getName() );
			Gui::Menu::item( d_popSpec, this, str, 
				SelectSpec, true )->addParam( Root::Any( spec ) );
		}
	}

}

void MonoScopeAgent::allocate()
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

void MonoScopeAgent::initOverlay(int n)
{
	assert( n > 0 );
	if( d_plane.d_ol.size() == n )
		return;
	Lexi::Glyph* g = d_plane.d_viewer->getViews()->getComponent( CONTOUR );
	while( g->getCount() )
		g->remove( 0 );

	int i;
	if( n > d_plane.d_ol.size() )
	{
		int old = d_plane.d_ol.size();
		d_plane.d_ol.resize( n );
		const Repository::SlotColors& sc = d_pro->getRepository()->getScreenClr();
		for( i = old; i < n; i++ )
		{
			d_plane.d_ol[i].d_buf = new SpecBufferMdl( 
				d_plane.d_viewer->getViewArea() );
			d_plane.d_ol[i].d_buf->setFolding( d_folding );
			d_plane.d_ol[i].d_view = new ContourView( d_plane.d_ol[i].d_buf, 
				d_plane.d_ol[0].d_view->isAuto() );
			if( i < sc.size() )
			{
				d_plane.d_ol[i].d_view->setPosColor( sc[ i ].d_pos );
				d_plane.d_ol[i].d_view->setNegColor( sc[ i ].d_neg );
			}
			d_plane.d_ol[i].d_view->setVisi( d_plane.d_ol[0].d_view->isVisi() );
			d_plane.d_ol[i].d_view->createLevelsAuto( s_contourFactor, s_contourOption, s_gain );
		}
	}else
	{
		d_plane.d_ol.resize( n );
	}
	for( i = 0; i < d_plane.d_ol.size(); i++ )
		g->append( d_plane.d_ol[i].d_view );
	setActiveOverlay( 0 );
	// updatePlaneLabel();
}

void MonoScopeAgent::setActiveOverlay(int n)
{
	if( n == d_aol )
		return;
	d_plane.d_viewer->redraw();

	d_aol = n;
	// updatePlaneLabel();
}

int MonoScopeAgent::selectLayer()
{
	if( d_plane.d_ol.size() == 1 )
		return 0;

	Dlg::StringList l( d_plane.d_ol.size() + 1);
	l[ 0 ] = "&All";
	QString str;
	for( int i = 0; i < d_plane.d_ol.size(); i++ )
	{
		if( d_plane.d_ol[ i ].d_spec )
			str.sprintf( "&%d %s", i, d_plane.d_ol[ i ].d_spec->getName() );
		else
			str.sprintf( "&%d <empty>", i );
		l[ i + 1 ] = str.toLatin1();
	}
	int c = Dlg::getOption( MW, l, 
		"Select Layer", d_aol + 1 );
	if( c == -1 )
		return -2;
	else
		return c - 1;
}

bool MonoScopeAgent::isDirty() const
{
	if( d_peaks && d_peaks->isNew() && d_peaks->isDirty() )
		return true;

	Histo::const_iterator i;
	for( i = d_histo.begin(); i != d_histo.end(); ++i )
		if( (*i)->isNew() && (*i)->isDirty() )
			return true;
	return false;
}

SpecViewer* MonoScopeAgent::createPlaneViewer()
{
	d_plane.d_ol.assign( 1, PlaneSocket::Overlay() );

	d_plane.d_viewer = new SpecViewer( new ViewAreaMdl( true, true, true, true ), VIEWCOUNT );
	d_plane.d_viewer->getViewArea()->addObserver( this );

	// s_autoContour = d_spec->getDimCount() <= 2; 

	d_plane.d_ol[0].d_spec = new SpecProjector( d_spec, DimX, DimY );
	d_plane.d_ol[0].d_buf = 
		new SpecBufferMdl( d_plane.d_viewer->getViewArea(), d_plane.d_ol[0].d_spec, false );
	d_plane.d_ol[0].d_buf->setFolding( d_folding, false );

	d_plane.d_intens = new IntensityView( d_plane.d_ol[0].d_buf, false );
	d_plane.d_viewer->getViews()->replace( INTENSITY, d_plane.d_intens );

	d_plane.d_ol[0].d_view = new ContourView( d_plane.d_ol[0].d_buf, s_autoContour );
	Lexi::Layer* l = new Lexi::Layer();
	l->append( d_plane.d_ol[0].d_view );
	d_plane.d_viewer->getViews()->replace( CONTOUR, l );
	d_plane.d_ol[0].d_view->setVisi( s_showContour );
	if( d_pro )
	{
		const Repository::SlotColors& sc = d_pro->getRepository()->getScreenClr();
		if( !sc.empty() )
		{
			d_plane.d_ol[0].d_view->setPosColor( sc[ 0 ].d_pos );
			d_plane.d_ol[0].d_view->setNegColor( sc[ 0 ].d_neg );
		}
	}
	d_plane.d_ol[0].d_view->createLevelsAuto( s_contourFactor, s_contourOption, s_gain );

	d_plane.d_cur = new CursorMdl();
	d_plane.d_cur->addObserver( this );
	PeakColorList* clr = 0;
	if( d_pro )
		clr = d_pro->getRepository()->getColors();
	d_plane.d_peaks = new PeakPlaneView( d_plane.d_viewer, 0, Qt::white, 0, clr );
	d_plane.d_viewer->getViews()->replace( PEAKS, d_plane.d_peaks );
	CursorView* cv = new CursorView( d_plane.d_viewer, d_plane.d_cur );
	d_plane.d_viewer->getViews()->replace( CURSORS, cv );
	if( d_folding )
		d_plane.d_viewer->getViews()->replace( FOLDING, new FoldingView( d_plane.d_ol[0].d_buf ) );

	d_plane.d_viewer->getHandlers()->append( new ZoomCtrl( d_plane.d_viewer, true, true ) );
	d_plane.d_viewer->getHandlers()->append( new SelectZoomCtrl( d_plane.d_viewer, true, true ) );
	d_plane.d_viewer->getHandlers()->append( new ScrollCtrl( d_plane.d_viewer ) );
	d_plane.d_viewer->getHandlers()->append( new CursorCtrl( cv, false ) );
	d_plane.d_viewer->getHandlers()->append( new PeakSelectCtrl( d_plane.d_peaks, false ) );
	d_plane.d_viewer->getHandlers()->append( new Lexi::ContextMenu( d_popPlane, false ) );
	d_plane.d_viewer->getHandlers()->append( new FocusCtrl( d_plane.d_viewer ) );

	// d_plane.d_ol[0].d_buf->fitToArea();
	return d_plane.d_viewer;
}

SpecViewer* MonoScopeAgent::createSliceViewer(Dimension view, Dimension spec)
{
	d_slices[ spec ].d_viewer = new SpecViewer( 
		new ViewAreaMdl( view == DimX, view == DimY, view == DimX, view == DimY ), 6 );
	d_slices[ spec ].d_viewer->getViewArea()->addObserver( this );

	d_slices[ spec ].d_spec = new SpecProjector( d_spec, spec );
	d_slices[ spec ].d_buf = new SpecBufferMdl( d_slices[ spec ].d_viewer->getViewArea(), 
		d_slices[ spec ].d_spec, false );
	d_slices[ spec ].d_buf->setFolding( d_folding, false );

	SliceView* sv = new SliceView( d_slices[ spec ].d_buf );
	d_slices[ spec ].d_viewer->getViews()->replace( CONTOUR, sv );
	d_slices[ spec ].d_slice = sv;
	d_slices[ spec ].d_cur = new CursorMdl();
	d_slices[ spec ].d_cur->addObserver( this );
	d_slices[ spec ].d_peak = new PeakModelView( sv );
	d_slices[ spec ].d_viewer->getViews()->replace( PEAKS, d_slices[ spec ].d_peak );
	CursorView* cv = new CursorView( d_slices[ spec ].d_viewer,
		d_slices[ spec ].d_cur );
	d_slices[ spec ].d_viewer->getViews()->replace( CURSORS, cv );

	d_slices[ spec ].d_viewer->getHandlers()->append( new ZoomCtrl( d_slices[ spec ].d_viewer, view == DimX, view == DimY ) );
	d_slices[ spec ].d_viewer->getHandlers()->append( new SelectZoomCtrl( d_slices[ spec ].d_viewer, view == DimX, view == DimY ) );
	d_slices[ spec ].d_viewer->getHandlers()->append( new ScrollCtrl( d_slices[ spec ].d_viewer ) );
	d_slices[ spec ].d_viewer->getHandlers()->append( new SelectRulerCtr( d_slices[ spec ].d_viewer, true ) );
	d_slices[ spec ].d_viewer->getHandlers()->append( new CursorCtrl( cv, false ) );
	d_slices[ spec ].d_viewer->getHandlers()->append( new FocusCtrl( d_slices[ spec ].d_viewer ) );

	// d_slices[ spec ].d_buf->fitToArea();
	return d_slices[ spec ].d_viewer;
}

bool MonoScopeAgent::setSpec(Spectrum * spec)
{
	if( spec == 0 || d_spec->getId() == spec->getId() || 
		d_main->getType() != spec->getType() )
		return false;

	Lexi::Viewport::pushHourglass();
	if( d_rot.empty() || spec == d_main )
		d_spec = spec;
	else
	{
		assert( d_rot.size() == spec->getDimCount() );
		d_spec = new SpecRotator( spec, d_rot );
	}
	d_back->setSpec( d_spec );

	Spectrum* to = d_spec;
	if( d_showBack && 
		d_spec->getDimCount() == d_plane.d_peaks->getModel()->getDimCount() )
		to = d_back;
	d_plane.d_ol[0].d_spec = new SpecProjector( to, DimX, DimY, d_cursor );
	d_plane.d_ol[0].d_buf->setSpectrum( d_plane.d_ol[0].d_spec );
	d_plane.d_peaks->setSpec( to );
	if( d_showSlices )
	{
		for( Dimension d = 0; d < d_slices.size(); ++d )
		{
			d_slices[ d ].d_spec = new SpecProjector( to, d, d_cursor );
			d_slices[ d ].d_buf->setSpectrum( d_slices[ d ].d_spec );
			d_slices[ d ].d_peak->setCenter( d_cursor[ d ] );
			d_slices[ d ].d_viewer->redraw();
		}
	}

	QString str;
	str.sprintf( " %d %s", d_spec->getId(), d_spec->getName() );
	d_plane.d_viewer->getViews()->replace( LABEL1,
		new Lexi::Label( str, nil, d_plane.d_peaks->getColor(), 
		Lexi::AlignLeft, Lexi::AlignTop ) );
	Lexi::Viewport::popCursor();
	notifyCursor();
	return true;
}

void MonoScopeAgent::setPeaklist(PeakList * pl, bool autorot )
{
	d_peaks = pl;
	if( d_peaks )
	{
		if( d_histo.count( pl ) == 0 )
		{
			d_histo.insert( pl );
			Histo::const_iterator i;

			d_popHisto->purge();
			for( i = d_histo.begin(); i != d_histo.end(); ++i )
			{
				Gui::Menu::item( d_popHisto, this, (*i)->getName().data(), 
					SelectPeaks, true )->addParam( Root::Any( (*i) ) );
			}
		}

		const Dimension dim = d_peaks->getDimCount();
		const Dimension dimMin = Root::Math::min( d_peaks->getDimCount(), d_spec->getDimCount() );
		if( autorot )
		{
			ColorMap spec;
			d_spec->getColors( spec );
			// Bilde die Farben der PeakListe auf die Farben des Spektrums ab.
			// rot.size() <= d_peaks->getDimCount()
			Rotation rot = createRotation2( spec, d_peaks->getColors() );
			if( rot.empty() )
			{
				Root::ReportToUser::warn( this, "Mapping Peaklist",
					"Cannot automatically determine peaklist mapping. Please adjust manually." );
				rot.assign( dim, 0 );
				for( Dimension d = 0; d < rot.size(); d++ )
					rot[ d ] = d;
					// Versuche es mit einer 1:1-Abbildung
			}
			d_pp = new PeakProjector( new PeakListPeer( d_peaks ), rot );
		}else
			d_pp = new PeakProjector( new PeakListPeer( d_peaks ) );

        d_plane.d_peaks->setModel( new PeakProjector( d_pp, DimX, DimY ) );
		for( Dimension d = 2; d < dimMin; d++ )
		{
			// Initialisiere Plane-Level
			PPM w = d_spec->getPeakWidth( d );
			if( w <= 0.0 )
				w = d_spec->getScale( d ).getDelta();
			d_plane.d_peaks->getModel()->setOrigThick( d, d_cursor[ d ], w );
		}
		d_back->setMaster( d_pp );

		if( d_showSlices )
			for( Dimension d = 0; d < dimMin; d++ ) 
			{
				d_slices[ d ].d_peak->setModel( new PeakProjector( d_pp, d ) );
				d_slices[ d ].d_peak->showBaseWidth( d_showBase );
				d_slices[ d ].d_peak->show( d_showModel );
				// TODO: Rotation der Peakliste nachfhren.
			}

		d_plane.d_viewer->getViews()->replace( LABEL2,
			new Lexi::Label( d_peaks->getName().data(), nil, 
			d_plane.d_peaks->getColor(), 
			Lexi::AlignLeft, Lexi::AlignBottom ) );
		reloadSpecPop();
	}else
	{
		d_plane.d_viewer->getViews()->replace( LABEL2, 0 );
		d_plane.d_peaks->setModel( 0 );
		for( Dimension d = 0; d < d_slices.size(); d++ )
			d_slices[ d ].d_peak->setModel( 0, 0 );
		d_back->setMaster( 0 );
		d_pp = 0;
	}
}

void MonoScopeAgent::handle(Root::Message& msg)
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
				if( d_showSlices && 
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
				if( d_showSlices && d_slices[ d ].d_cur == a->getOrigin() )
				{
					updateSlice( d, a );
					break;
				}
		msg.consume();
	}
	MESSAGE( GlobalCursor::UpdatePos, a, msg )
	{
		d_lock = false;
		d_cursorSync = false;
		if( ( a->getDim() == DimY || a->getDim() == DimUndefined ) &&
			d_plane.d_ol[0].d_spec->getColor( DimY ) == a->getTy() )
			d_plane.d_cur->setCursor( Dimension( DimY ), a->getY() );
		if( ( a->getDim() == DimX || a->getDim() == DimUndefined ) &&
			d_plane.d_ol[0].d_spec->getColor( DimX ) == a->getTx() )
			d_plane.d_cur->setCursor( Dimension( DimX ), a->getX() );
		if( d_syncDepth && a->getDim() > 1 && a->getDim() < d_spec->getDimCount() &&
			d_spec->getColor( a->getDim() ) == a->getTx() )
		{
			if( d_showSlices )
			{
				d_cursor[ a->getDim() ] = a->getX();
				d_slices[ a->getDim() ].d_cur->setCursor( (Dimension)DimY, a->getX() );
				d_slices[ a->getDim() ].d_peak->setCenter( a->getX() );
				// updateSlice( )
			}
		}
		d_cursorSync = true;
		msg.consume();
	}
	MESSAGE( GlobalCursor::UpdateRange, a, msg )
	{
		d_lock = false;
		d_rangeSync = false;
		if( ( a->getDim() == DimY || a->getDim() == DimUndefined ) &&
			d_plane.d_ol[0].d_spec->getColor( DimY ) == a->getTy() )
			d_plane.d_viewer->getViewArea()->setRange( DimY, a->getY() );
		if( ( a->getDim() == DimX || a->getDim() == DimUndefined ) &&
			d_plane.d_ol[0].d_spec->getColor( DimX ) == a->getTx() )
			d_plane.d_viewer->getViewArea()->setRange( DimX, a->getX() );
		d_rangeSync = true;
		msg.consume();
	}
	MESSAGE( Root::Action, a, msg )
	{
		d_lock = false; // Kein Blocking fr Action-Ausfhrung
		EXECUTE_ACTION( MonoScopeAgent, *a );
	}
	END_HANDLER();
	d_lock = false;
}


void MonoScopeAgent::updatePlane(CursorMdl::Update * msg)
{
	assert( d_slices.size() >= 2 );
	PeakPos pos( msg->getX(), msg->getY() );

	if( d_autoCenter && msg->getDim() == DimUndefined && d_plane.d_peaks->getModel() )
	{
		Root::Index peak = d_plane.d_peaks->getHit( pos[ DimX ], pos[ DimY ] );
		if( peak )
		{
			d_plane.d_peaks->getModel()->getPos( peak, pos, d_spec );
			msg->override( pos[ DimX ], pos[ DimY ] ); 
			// das geht, da View die Message erst nach Agent erhlt.
		}
	}

	// Der X-Slice zeigt den durch den Y-Cursor der Plane
	// reprsentierten Slice mit Origin Y (und umgekehrt)
	if( msg->getDim() == DimY || msg->getDim() == DimUndefined )
	{
		if( d_showSlices )
		{
			d_slices[ DimX ].d_spec->setOrigin( DimY, pos[ DimY ] );
			d_slices[ DimX ].d_viewer->redraw();
			d_slices[ DimY ].d_cur->setCursor( (Dimension)DimY, pos[ DimY ] );
			d_slices[ DimY ].d_peak->setCenter( pos[ DimY ] );
			for( Dimension d = 2; d < d_spec->getDimCount(); d++ )
			{
				d_slices[ d ].d_spec->setOrigin( DimY, pos[ DimY ] );
				d_slices[ d ].d_viewer->redraw();
			}
		}
		d_cursor[ DimY ] = pos[ DimY ];
		if( d_cursorSync )
			GlobalCursor::setCursor( DimY, pos[ DimY ], d_plane.d_ol[0].d_spec->getColor( DimY ) );
	}
	if( msg->getDim() == DimX || msg->getDim() == DimUndefined )
	{
		if( d_showSlices )
		{
			d_slices[ DimY ].d_spec->setOrigin( DimX, pos[ DimX ] );
			d_slices[ DimY ].d_viewer->redraw();
			d_slices[ DimX ].d_cur->setCursor( (Dimension)DimX, pos[ DimX ] );
			d_slices[ DimX ].d_peak->setCenter( pos[ DimX ] );
			for( Dimension d = 2; d < d_spec->getDimCount(); d++ )
			{
				d_slices[ d ].d_spec->setOrigin( DimX, pos[ DimX ] );
				d_slices[ d ].d_viewer->redraw();
			}
		}
		d_cursor[ DimX ] = pos[ DimX ];
		if( d_cursorSync )
			GlobalCursor::setCursor( DimX, pos[ DimX ], d_plane.d_ol[0].d_spec->getColor( DimX ) );
	}
	notifyCursor();
}

void MonoScopeAgent::updateSlice(Dimension dim, CursorMdl::Update *msg)
{
	d_cursor[ dim ] = msg->getX();
	
	if( d_showModel )
	{
		d_slices[ dim ].d_peak->setCenter( msg->getX() );
		d_slices[ dim ].d_viewer->redraw();
	}

	if( dim < 2 )
	{
		d_plane.d_cur->setCursor( dim, msg->getX() ); // Beide Dims gleich
		for( Dimension d = 2; d < d_spec->getDimCount(); d++ )
		{
			d_slices[ d ].d_spec->setOrigin( dim, msg->getX() );
			d_slices[ d ].d_spec->getOrigin().dump( QString( "Slice D%1" ).arg( d ).toLatin1() ); // TEST
			d_slices[ d ].d_viewer->redraw();
		}
		if( d_cursorSync )
			GlobalCursor::setCursor( dim, msg->getX(), 
				d_slices[ dim ].d_spec->getColor( DimX ) );
	}else // Depth
	{
		Lexi::Viewport::pushHourglass();
		registerPlane();
		for( Dimension d = 0; d < d_spec->getDimCount(); d++ )
		{
			d_slices[ d ].d_spec->setOrigin( dim, msg->getX() );
			//d_slices[ d ].d_spec->getOrigin().dump( QString( "Slice D%1" ).arg( d ).toLatin1() ); // TEST
			d_slices[ d ].d_viewer->redraw();
		}
		/* Old: funktioniert nicht bei 4D
		d_slices[ DimX ].d_spec->setOrigin( dim, msg->getX() );
		d_slices[ DimX ].d_viewer->redraw();
		d_slices[ DimY ].d_spec->setOrigin( dim, msg->getX() );
		d_slices[ DimY ].d_viewer->redraw();
		*/
		d_plane.d_ol[0].d_spec->setOrigin( dim, msg->getX() );
		if( d_plane.d_peaks->getModel() )
		{
			PPM w = d_spec->getPeakWidth( dim );
			if( w <= 0.0 )
				w = d_spec->getScale( dim ).getDelta();
			d_plane.d_peaks->getModel()->setOrigThick( dim, msg->getX(), w ); 
		}
		d_plane.d_viewer->damageMe();
		if( d_cursorSync && d_syncDepth )
			GlobalCursor::setCursor( dim, msg->getX(), d_spec->getColor( dim ) );
		Lexi::Viewport::popCursor();
	}
	notifyCursor();
}

void MonoScopeAgent::updatePlane(ViewAreaMdl::Update * msg)
{
	if( !d_showSlices || msg->getType() != ViewAreaMdl::Update::Range )
		return;
	registerPlane();
	if( msg->getX() && d_slices[ DimX ].d_viewer )
	{
		d_slices[ DimX ].d_viewer->getViewArea()->setRange( DimX, 
			d_plane.d_viewer->getViewArea()->getRange( DimX ) );
		d_slices[ DimX ].d_viewer->redraw();
		if( d_rangeSync )
			GlobalCursor::setRange( DimX, 
			d_plane.d_viewer->getViewArea()->getRange( DimX ), 
			d_plane.d_ol[0].d_spec->getColor( DimX ) );
	}
	if( msg->getY() && d_slices[ DimY ].d_viewer )
	{
		d_slices[ DimY ].d_viewer->getViewArea()->setRange( DimY, 
			d_plane.d_viewer->getViewArea()->getRange( DimY ) );
		d_slices[ DimY ].d_viewer->redraw();
		if( d_rangeSync )
			GlobalCursor::setRange( DimY, 
			d_plane.d_viewer->getViewArea()->getRange( DimY ), 
			d_plane.d_ol[0].d_spec->getColor( DimY ) );
	}
}

void MonoScopeAgent::updateSlice(Dimension dim, ViewAreaMdl::Update *msg)
{
	if( msg->getType() != ViewAreaMdl::Update::Range )
		return;
	if( dim < 2 )
	{
		d_plane.d_viewer->getViewArea()->setRange( dim, 
			d_slices[ dim ].d_viewer->getViewArea()->getRange( dim ) );
		d_plane.d_viewer->damageMe();
		if( d_rangeSync )
			GlobalCursor::setRange( dim, 
				d_slices[ dim ].d_viewer->getViewArea()->getRange( dim ), 
				d_slices[ dim ].d_spec->getColor( DimX ) );
	}
}

void MonoScopeAgent::registerPlane()
{
	PpmCube cube;
	cube.assign( d_spec->getDimCount(), PpmRange() );
	cube[ DimX ] = d_plane.d_viewer->getViewArea()->getRange( DimX );
	cube[ DimY ] = d_plane.d_viewer->getViewArea()->getRange( DimY );
	for( Dimension d = 2; d < d_slices.size(); ++d )
	{
		if( d_showSlices && d_slices[ d ].d_viewer )
			cube[ d ] = d_slices[ d ].d_viewer->getViewArea()->getRange( DimY );
		else
			cube[ d ] = d_spec->getScale( d ).getRange();
	}
	d_backward.push_back( std::make_pair( cube, d_cursor ) );
}

void MonoScopeAgent::initOrigin()
{
	d_cursor[ DimX ] = d_plane.d_viewer->getViewArea()->getRange( DimX ).first;
	d_cursor[ DimY ] = d_plane.d_viewer->getViewArea()->getRange( DimY ).first;
	d_plane.d_cur->setCursor( d_cursor[ DimX ], d_cursor[ DimY ] );
	for( Dimension d = 2; d < d_slices.size(); ++d )
	{
		d_cursor[ d ] = d_spec->getScale( d ).getRange().first;
		d_slices[ d ].d_cur->setCursor( (Dimension)DimY, d_cursor[ d ] );
		d_slices[ d ].d_peak->setCenter( d_cursor[ d ] );
	}
	if( d_showSlices )
	{
		for( Dimension d = 2; d < d_slices.size(); ++d )
		{
			d_slices[ d ].d_spec->setOrigin( d_cursor );
		}
	}
}

void MonoScopeAgent::notifyCursor()
{
	QString str;
    QTextStream ts( &str, QIODevice::WriteOnly );

	ts.setf( QTextStream::fixed );
	ts.precision( 3 );

	ts <<  "Cursor:  ";

	for( Dimension d = 0; d < d_cursor.size(); d++ )
	{
		ts << getDimLetter( d, false ) << ": ";
		ts << d_spec->getDimName( d ) << "=";
		ts << d_cursor[ d ];
		if( d_folding )
			ts << " (" << d_spec->getScale( d ).getRangeOffset( d_cursor[ d ] ) << ")  ";
		else
			ts << "  ";
	}

	try
	{
		Amplitude val = 0;
		if( d_showBack && d_back && d_back->getDimCount() == d_cursor.size() )
			val = ( (Spectrum*)d_back )->getAt( d_cursor, d_folding, d_folding );
		else
			val = d_spec->getAt( d_cursor, d_folding, d_folding );
		ts.setf( QTextStream::showpos );
		ts.precision( 0 );
		ts << "Level=" << val;
	}catch( ... )
	{
		ts << " Out of Spectrum";
	}
	QByteArray  tmp;
	if( d_plane.d_peaks->formatSelection( tmp, 3 ) )
	{
		ts << ",  ";
		ts << tmp.data();
	}
	Lexi::ShowStatusMessage msg( str );
	traverseUp( msg );
}

void MonoScopeAgent::setCursor(Root::Index id)
{
	PeakPos pos;
	d_pp->getPos( id, pos, d_spec );

	bool ac = d_autoCenter;
	d_autoCenter = false;

	Dimension d;
	for( d = 0; d < d_cursor.size(); d++ )
		d_cursor[ d ] = pos[ d ];
	d_plane.d_cur->setCursor( d_cursor[ DimX ], d_cursor[ DimY ] );
	for( d = 2; d < d_slices.size(); ++d )
	{
		if( d_showSlices )
		{
			d_slices[ d ].d_cur->setCursor( (Dimension)DimY, d_cursor[ d ] );
			d_slices[ d ].d_peak->setCenter( d_cursor[ d ] );
		}
	}
	d_plane.d_peaks->select( id );
	ViewAreaMdl* area = d_plane.d_viewer->getViewArea();
	if( !area->getRange( DimX ).contains( d_cursor[ DimX ] ) ||
		!area->getRange( DimY ).contains( d_cursor[ DimY ] ) )
	{
		area->centerPoint( d_cursor[ DimX ], d_cursor[ DimY ] );
		d_plane.d_viewer->damageMe();
	}
	if( d_cursor.size() >= 3 && d_showSlices )
	{
        for( d = 2; d < d_cursor.size(); d++ )
        {
            ViewAreaMdl* slice = d_slices[ d ].d_viewer->getViewArea();
            if( !slice->getRange( DimY ).contains( d_cursor[ d ] ) )
            {
                slice->centerPoint( slice->getRange( DimX ).getCenter(), d_cursor[ d ] );
                d_slices[ d ].d_viewer->damageMe();
            }
        }
    }
	d_autoCenter = ac;
}

void MonoScopeAgent::fitToView()
{
	d_plane.d_ol[0].d_buf->fitToArea();
	d_plane.d_viewer->damageMe();
	if( d_showSlices )
	{
		for( Dimension d = 0; d < d_slices.size(); ++d )
		{
			d_slices[ d ].d_buf->fitToArea();
			d_slices[ d ].d_viewer->redraw();
		}
	}
}

void MonoScopeAgent::updateContour( int i, bool redraw )
{
	if( !d_plane.d_ol[0].d_view->isVisi() )
		return;

	if( d_plane.d_ol[i].d_view->isAuto() )
	{
		d_plane.d_ol[i].d_view->createLevelsAuto();
	}else if( d_plane.d_ol[i].d_spec )
		d_plane.d_ol[i].d_view->createLevelsMin( d_plane.d_ol[i].d_spec->getThreshold() );
	if( redraw )
		d_plane.d_viewer->damageMe();
}

void MonoScopeAgent::showIntens(bool on )
{
	if( d_plane.d_intens->isVisi() == on )
		return;
	Lexi::Viewport::pushHourglass();
	d_plane.d_intens->setVisi( on );
	Lexi::Viewport::popCursor();
}

void MonoScopeAgent::setCursor( PpmPoint p)
{
	if( p.size() < d_spec->getDimCount() )
	{
		p.assign( d_spec->getDimCount(), 0 );
		for( int i = 0; i < p.size(); i++ )
			p[ i ] = d_spec->getScale( i ).getIdxN();
	}
	d_cursor = p;
	d_plane.d_cur->setCursor( d_cursor[ DimX ], d_cursor[ DimY ] );
	for( Dimension d = 2; d < d_slices.size(); ++d )
	{
		if( d_showSlices )
		{
			d_slices[ d ].d_cur->setCursor( (Dimension)DimY, d_cursor[ d ] );
			d_slices[ d ].d_peak->setCenter( d_cursor[ d ] );
		}
	}
}

void MonoScopeAgent::handleSetResolution(Action & a)
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
		d_plane.d_ol[0].d_buf->setResolution( d_resol );
		d_plane.d_viewer->damageMe();
		Viewport::popCursor();
	}
}

void MonoScopeAgent::handleShowLowRes(Action &a)
{
	ACTION_CHECKED_IF( a, true, d_lowResol );

	Viewport::pushHourglass();
	d_lowResol = !d_lowResol;
	if( d_lowResol )
		d_plane.d_ol[0].d_buf->setResolution( d_resol );
	else
		d_plane.d_ol[0].d_buf->setScaling( false );
	d_plane.d_viewer->damageMe();
	Viewport::popCursor();
}

void MonoScopeAgent::handleFitWindow(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	fitToView();
}

void MonoScopeAgent::handleForward(Action & a)
{
	ACTION_ENABLED_IF( a, d_forward.size()	> 0 );

	Viewport::pushHourglass();
	d_backward.push_back( d_forward.back() );
	const PpmCube& cube = d_forward.back().first;
	d_cursor = d_backward.back().second;
	d_plane.d_viewer->getViewArea()->setRange( DimX, cube[ DimX ] );
	d_plane.d_viewer->getViewArea()->setRange( DimY, cube[ DimY ] );
	d_plane.d_cur->setCursor( d_cursor[ DimX ], d_cursor[ DimY ] );
	if( d_showSlices )
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

void MonoScopeAgent::handleBackward(Action & a)
{ 
	ACTION_ENABLED_IF( a, d_backward.size() > 1 );

	Viewport::pushHourglass();
	d_forward.push_back( d_backward.back() );
	const PpmCube& cube = d_backward.back().first;
	d_cursor = d_forward.back().second;
	d_plane.d_viewer->getViewArea()->setRange( DimX, cube[ DimX ] );
	d_plane.d_viewer->getViewArea()->setRange( DimY, cube[ DimY ] );
	d_plane.d_cur->setCursor( d_cursor[ DimX ], d_cursor[ DimY ] );
	if( d_showSlices )
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

static Amplitude _getAt(const PeakPos & p, bool f, Spectrum * s, const PeakModel::Params& m )
{
	assert( s );
	Dimension d;

	PpmCube c;
	c.assign( s->getDimCount(), PpmRange() );
	for( d = 0; d < c.size(); d++ )
		c[ d ].allocate( p[ d ], 0.0 );

	for( d = 0; d < c.size(); d++ )
	{
		c[ d ].expand( m.d_width[ d ] * m.d_tol[ d ] ); 
		// Setzt auch Width auf Null, falls ntig
	}
	try
	{
		return s->getAt( c, f );
	}catch( ... )
	{
		return 0;
	}
}

void MonoScopeAgent::handlePickPeak(Action & a)
{
	ACTION_ENABLED_IF( a, d_peaks->getDimCount() <= d_spec->getDimCount() );

	assert( d_pp );
	PeakSpace::PeakData pd;
	PeakModel::Params pp;
	pd.d_pos = d_cursor;
	// d_pp->getPeakSpace()->getParams( 0, pp );
	d_pp->getParams( 0, pp );
	pd.d_amp = _getAt( d_cursor, d_folding, d_spec, pp ); 
	try
	{
		COP cop( true, true );
		d_pp->addPeak( pd, d_spec, cop );
		cop.d_done->registerForUndo( this );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Pick Peak", e.what() );
	}
}

void MonoScopeAgent::handleMovePeak(Action & a)
{
	// NOTE: Peaks werden immer fr ein konkretes Spektrum gesetzt. Move generiert
	// immer ein Alias. Wichtig, da Amplitude Teil des Peak-Alias.

	ACTION_ENABLED_IF( a, !d_peaks.isNull() && d_plane.d_peaks->getSel().size() == 1 &&
		d_peaks->getDimCount() <= d_spec->getDimCount() );

	Root::Index peak = *d_plane.d_peaks->getSel().begin();
	PeakPos pos;
	pos.nirwana();
	PeakModel::Params pp;
	d_plane.d_peaks->getModel()->getPos( peak, pos, 0 ); // Initialisiere alle Dim mit aktueller Pos
	// Nur X,Y werden verschoben.
	pos[ DimX ] = d_cursor[ DimX ];
	pos[ DimY ] = d_cursor[ DimY ];
	for( Dimension d = DimZ; d < d_cursor.size(); d++ )
		pos[ d ] = d_cursor[ d ];
	d_pp->getParams( 0, pp );
	Amplitude amp = _getAt( d_cursor, d_folding, d_spec, pp ); 
	try
	{
		COP cop( true, true );
		d_pp->setPos( peak, pos, amp, 0, cop );
		cop.d_done->registerForUndo( this );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Move Peak", e.what() );
	}
}

void MonoScopeAgent::handleMovePeakAlias(Action & a)
{
	// NOTE: Peaks werden immer fr ein konkretes Spektrum gesetzt. Move generiert
	// immer ein Alias. Wichtig, da Amplitude Teil des Peak-Alias.

	ACTION_ENABLED_IF( a, !d_peaks.isNull() && d_plane.d_peaks->getSel().size() == 1 &&
		d_peaks->getDimCount() <= d_spec->getDimCount() );

	Root::Index peak = *d_plane.d_peaks->getSel().begin();
	PeakPos pos;
	PeakModel::Params pp;
	d_plane.d_peaks->getModel()->getPos( peak, pos, d_spec ); // Initialisiere alle Dim mit aktueller Pos
	// Nur X,Y werden verschoben.
	pos[ DimX ] = d_cursor[ DimX ];
	pos[ DimY ] = d_cursor[ DimY ];
	for( Dimension d = DimZ; d < d_cursor.size(); d++ )
		pos[ d ] = d_cursor[ d ];
	d_pp->getParams( 0, pp );
	Amplitude amp = _getAt( d_cursor, d_folding, d_spec, pp ); 
	try
	{
		COP cop( true, true );
		d_pp->setPos( peak, pos, amp, d_spec, cop );
		cop.d_done->registerForUndo( this );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Move Peak Alias", e.what() );
	}
}

void MonoScopeAgent::handleDeletePeaks(Action & a)
{
	ACTION_ENABLED_IF( a, !d_peaks.isNull() && !d_plane.d_peaks->getSel().empty() );

	Root::Ref<Root::MakroTransaction> cmd = new Root::MakroTransaction( "Delete Peaks" );
	try
	{
		PeakPlaneView::Selection sel = d_plane.d_peaks->getSel();
		PeakPlaneView::Selection::const_iterator p;
		for( p = sel.begin(); p != sel.end(); ++p )
		{
			COP cop( true, true );
			d_pp->removePeak( (*p), cop );
			cmd->add( cop.d_done );
		}
	}catch( Root::Exception& e )
	{
		cmd->unexecute();
		Root::ReportToUser::alert( this, "Delete Peaks", e.what() );
		return;
	}
	cmd->registerForUndo( this );
}

void MonoScopeAgent::handleShowFolded(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_folding );

	Viewport::pushHourglass();
	d_folding = !d_folding;
	d_plane.d_ol[0].d_buf->setFolding( d_folding );
	if( d_folding )
		d_plane.d_viewer->getViews()->replace( FOLDING, new FoldingView( d_plane.d_ol[0].d_buf ) );
	else
		d_plane.d_viewer->getViews()->replace( FOLDING, 0 );
	d_plane.d_viewer->damageMe();
	if( d_showSlices )
		for( Dimension d = 0; d < d_slices.size(); ++d )
		{
			d_slices[ d ].d_buf->setFolding( d_folding );
			d_slices[ d ].d_viewer->redraw();
		}
	Viewport::popCursor();
}

void MonoScopeAgent::handleViewLabels(Action & a)
{
	if( a.getParamCount() == 0 )
		return;

	PeakPlaneView::Label q = (PeakPlaneView::Label) a.getParam( 0 ).getShort();
	if( q < PeakPlaneView::NONE || q >= PeakPlaneView::END )
		return;

	ACTION_CHECKED_IF( a, true,
		d_plane.d_peaks->getLabel() == q );
	
	d_plane.d_peaks->setLabel( q );
	d_plane.d_viewer->redraw();
}

void MonoScopeAgent::handlePeakCalibrate(Action & a)
{
	ACTION_ENABLED_IF( a, !d_peaks.isNull() &&  // RISK: auch fr gespeicherte zulassen
		d_plane.d_peaks->getSel().size() <= 1 );

	PpmPoint off;
	Root::Index peak = 0;
	if( d_plane.d_peaks->getSel().empty() )
	{
		bool ok;
		int res = QInputDialog::getInteger( "Calibrate Peaklist", 
			"Please enter the id of the reference peak:", 
			0, 0, 999999, 1, &ok, MW );
		if( !ok )
			return;
		PeakList::Peaks::const_iterator pos = d_peaks->getPeaks().find( res );
		if( pos == d_peaks->getPeaks().end() )
		{
			QMessageBox::critical( MW, "Calibrate Peaklist",
					"A peak with this id doesn't exist!", "&Cancel" );
			return;
		}else
			peak = (*pos).second->getId();
	}else
		peak = *d_plane.d_peaks->getSel().begin();

	PeakPos p;
	d_pp->getPos( peak, p, d_spec );
	const Rotation& rot = d_pp->getRot();
	std::set<Dimension> test;
	Dimension d;
	for( d = 0; d < d_cursor.size() && d < rot.size(); d++ )
	{
		test.insert( rot[ d ] ); 
		p[ rot[ d ] ] = d_cursor[ d ] - p[ d ];
	}
	for( d = 0; d < d_cursor.size(); d++ )
	{
		if( test.count( d ) == 0 ) 
			// Alle von Rot nicht berhrten Dim mssen Offset 0 haben
			p[ d ] = 0;
	}

	Viewport::pushHourglass();
	Root::Ref<PeakCalibrateCmd> cmd = new PeakCalibrateCmd( d_peaks, p );
	cmd->handle( this );
	Viewport::popCursor();
}

void MonoScopeAgent::handleSpecCalibrate(Action & a)
{
	ACTION_ENABLED_IF( a, !d_peaks.isNull() && 
		d_plane.d_peaks->getSel().size() <= 1 );

	PpmPoint off;
	Root::Index peak = 0;
	if( d_plane.d_peaks->getSel().empty() )
	{
		bool ok;
		int res = QInputDialog::getInteger( "Calibrate Spectrum", 
			"Please enter the id of the reference peak:", 
			0, 0, 999999, 1, &ok, MW );
		if( !ok )
			return;
		PeakList::Peaks::const_iterator pos = d_peaks->getPeaks().find( res );
		if( pos == d_peaks->getPeaks().end() )
		{
			QMessageBox::critical( MW, "Calibrate Spectrum",
					"A peak with this id doesn't exist!", "&Cancel" );
			return;
		}else
			peak = (*pos).second->getId();
	}else
		peak = *d_plane.d_peaks->getSel().begin();

	PeakPos p;
	d_plane.d_peaks->getModel()->getPos( peak, p, d_spec );
	for( Dimension d = 0; d < d_cursor.size(); d++ )
		p[ d ] = p[ d ] - d_cursor[ d ];
	PpmPoint q;
	q.assign( d_cursor.size(), 0 );
	p.fillPoint( q );

	Viewport::pushHourglass();
	Root::Ref<SpecCalibrateCmd> cmd = new SpecCalibrateCmd( d_spec, q );
	cmd->handle( this );
	Viewport::popCursor();
}

void MonoScopeAgent::handleAutoCenter(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_autoCenter );

	d_autoCenter = !d_autoCenter;
}

void MonoScopeAgent::handlePeakRotate(Action & a)
{
	PeakList* pl = d_peaks;
	ACTION_ENABLED_IF( a, pl && pl->isNew() );

	RotateDlg dlg( MW, "Original", "Rotated" );
	Rotation rot;
	rot.assign( pl->getDimCount(), 0 );
	QString s1;
	for( Dimension d = 0; d < pl->getDimCount(); d++ )
	{
		rot[ d ] = d;
		s1.sprintf( "%s", pl->getColors()[ d ].getIsoLabel() );
		dlg.addDimension( s1, "" );
	}
	if( dlg.rotate( rot ) )
	{
		pl->rotateAll( rot );
	}
}

void MonoScopeAgent::handleGoto(Action & a)
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

void MonoScopeAgent::handleShowContour(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_plane.d_ol[0].d_view->isVisi() );

	bool visi = !d_plane.d_ol[0].d_view->isVisi();
	for( int i = 0; i < d_plane.d_ol.size(); i++ )
		d_plane.d_ol[i].d_view->setVisi( visi );
	if( d_plane.d_ol[0].d_view->isVisi() )
		d_plane.d_viewer->getViews()->replace( BACKGROUND, new Lexi::Background() );
	else
		d_plane.d_viewer->getViews()->replace( BACKGROUND, 0 );
	updateContour( 0, true );
}

void MonoScopeAgent::handleShowIntensity(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_plane.d_intens->isVisi() );

	showIntens( !d_plane.d_intens->isVisi() );
}

void MonoScopeAgent::handleAutoContour(Action & a)
{
	ACTION_CHECKED_IF( a, d_plane.d_ol[d_aol].d_spec, 
		d_plane.d_ol[d_aol].d_view->isAuto() );
	
	if( d_plane.d_ol[d_aol].d_view->isAuto() )
		d_plane.d_ol[d_aol].d_view->createLevelsMin( d_plane.d_ol[d_aol].d_spec->getThreshold() );
	else
		d_plane.d_ol[d_aol].d_view->createLevelsAuto();
	d_plane.d_viewer->redraw();
}

void MonoScopeAgent::handleContourParams(Action & a)
{
	ACTION_ENABLED_IF( a, d_plane.d_ol[d_aol].d_spec );

	Dlg::ContourParams p;
	p.d_factor = d_plane.d_ol[d_aol].d_view->getFactor();
	p.d_threshold =	d_plane.d_ol[d_aol].d_spec->getThreshold();
	p.d_option = d_plane.d_ol[d_aol].d_view->getOption();
	if( Dlg::setParams( MW, p ) )
	{
		d_plane.d_ol[d_aol].d_spec->setThreshold( p.d_threshold );
		d_plane.d_ol[d_aol].d_view->setOption( p.d_option );
		d_plane.d_ol[d_aol].d_view->setFactor( p.d_factor );
		d_plane.d_ol[d_aol].d_view->setVisi( true );
		showIntens( false );
		d_plane.d_ol[d_aol].d_view->createLevelsMin( d_plane.d_ol[d_aol].d_spec->getThreshold() );
		d_plane.d_viewer->damageMe();
	}
}

void MonoScopeAgent::handleSpecRotate(Root::Action & a)
{
	SpectrumPeer* spec = dynamic_cast<SpectrumPeer*>( d_spec.deref() );
	ACTION_ENABLED_IF( a, spec && spec->getType() );

	if( SpectrumListView::mapToType( MW, 
		spec->getType(), spec ) )
	{
		fitToView();	// TODO: ev. Ausschnittsgrsse beibehalten.
		initOrigin(); // Wichtig, damit richtige Plane dargestellt wird.
	}
}
 
void MonoScopeAgent::handleMapPeakList(Action & a)
{
	ACTION_ENABLED_IF( a, d_peaks );

	RotateDlg dlg( MW, "Peaklist", "Spectrum" );
	const Dimension dim = d_peaks->getDimCount();
	Rotation rot( dim );
	for( Dimension d = 0; d < dim; d++ )
	{
		dlg.addDimension( 
			( d < d_peaks->getDimCount() )?d_peaks->getColors()[d].getIsoLabel():"", 
			( d < d_spec->getDimCount() )?d_spec->getColor( d ).getIsoLabel():"" );
		rot[ d ] = ( d < d_pp->getDimCount() )?d_pp->getRot()[ d ]:d;
	}

	if( dlg.rotate( rot ) )
	{
		assert( d_pp );
		d_pp->setRot( rot );
	}
}

void MonoScopeAgent::handleForceLabel(Action & a)
{
	ACTION_ENABLED_IF( a, !d_peaks.isNull() && d_plane.d_peaks->getSel().size() == 1 );

	Root::Index peak = *d_plane.d_peaks->getSel().begin();
	bool ok	= FALSE;
	QString res	= QInputDialog::getText( "Force Peak Label", 
		"Please	enter a label:", QLineEdit::Normal, 
		d_pp->getTag( peak ).data(), &ok, MW );
	if( !ok )
		return;
	try
	{
		COP cop( true, true );
		d_pp->setTag( peak, res.toLatin1(), cop );
		cop.d_done->registerForUndo( this );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Label Peak", e.what() );
	}
}

void MonoScopeAgent::handleShowModel(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_showModel );

	d_showModel = !d_showModel;
	if( d_showSlices )
		for( Dimension d = 0; d < d_slices.size(); d++ )
			d_slices[ d ].d_peak->show( d_showModel );
}

void MonoScopeAgent::handleShowBaseWidth(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_showBase );

	d_showBase = !d_showBase;
	if( d_showSlices )
		for( Dimension d = 0; d < d_slices.size(); d++ )
			d_slices[ d ].d_peak->showBaseWidth( d_showBase );
}

void MonoScopeAgent::handleTunePeakModel(Action & a)
{
	ACTION_ENABLED_IF( a, d_peaks );
	
	if( !d_showModel )
		handleShowModel( a );
	PeakModelTuner dlg( MW,
		d_peaks->getModel(), d_pp->getRot(), 0.5 );
	dlg.exec();
	d_back->loadModels();
}

void MonoScopeAgent::handleUpdateAllAmps(Action & a)
{
	ACTION_ENABLED_IF( a, d_peaks && d_peaks->getDimCount() == d_spec->getDimCount() );
	Lexi::Viewport::pushHourglass();
	d_peaks->updateAllAmps( d_spec, d_pp->getRot(), d_folding );
	Lexi::Viewport::popCursor();
}

void MonoScopeAgent::handleIntegrateAll(Action & a)
{
	ACTION_ENABLED_IF( a, d_peaks && d_peaks->getDimCount() == d_spec->getDimCount() );
	try
	{
		Lexi::Viewport::pushHourglass();
		bool hasDoubles = d_peaks->hasDoubles( d_spec );
		Lexi::Viewport::popCursor();
		if( hasDoubles )
		{
			if( QMessageBox::warning( MW, "Integrate Peak List",
				"This peaklist contains degenerate peaks for the given spectrum.\n"
				"If you continue, the volume will be diveded among the degenerate peaks.\n"
				"If this is not intended, cancel and use the 'Report degenerate peaks' command.",
				"&Continue", "&Cancel" ) != 0 )
				return;
		}
		Lexi::Viewport::pushHourglass();
		d_peaks->integrateAll( d_spec, d_intMeth );
		Lexi::Viewport::popCursor();
	}catch( Root::Exception& e )
	{
		Lexi::Viewport::popCursor();
		Root::ReportToUser::alert( this, "Integrate All", e.what() );
	}
}

void MonoScopeAgent::handleIntegrateSel(Action &)
{
	// TODO
}

void MonoScopeAgent::handleShowBackCalc(Action & a)
{
	ACTION_CHECKED_IF( a, !d_peaks.isNull() && 
		d_spec->getDimCount() == d_plane.d_peaks->getModel()->getDimCount(), 
		d_showBack );

	d_showBack = !d_showBack;

	Lexi::Viewport::pushHourglass();
	Spectrum* to = d_spec;
	if( d_showBack )
		to = d_back;
	d_plane.d_ol[0].d_spec = new SpecProjector( to, DimX, DimY, d_cursor );
	d_plane.d_ol[0].d_buf->setSpectrum( d_plane.d_ol[0].d_spec );
	d_plane.d_viewer->redraw();
	if( d_showSlices )
	{
		for( Dimension d = 0; d < d_slices.size(); ++d )
		{
			d_slices[ d ].d_spec = new SpecProjector( to, d, d_cursor );
			d_slices[ d ].d_buf->setSpectrum( d_slices[ d ].d_spec );
			d_slices[ d ].d_viewer->redraw();
		}
	}
	Lexi::Viewport::popCursor();
	notifyCursor();
}

void MonoScopeAgent::handleShowDiff(Action & a)
{
	ACTION_CHECKED_IF( a, !d_peaks.isNull(), d_back->getDiff() );
	Lexi::Viewport::pushHourglass();
	d_back->setDiff( !d_back->getDiff() );
	Lexi::Viewport::popCursor();
	notifyCursor();
}

void MonoScopeAgent::handleExactBackCalc(Action & a)
{
	ACTION_CHECKED_IF( a, !d_peaks.isNull(), d_back->getExact() );
	Lexi::Viewport::pushHourglass();
	d_back->setExact( !d_back->getExact() );
	Lexi::Viewport::popCursor();
	notifyCursor();
}

void MonoScopeAgent::handleBatchIntegrate(Action & a)
{
	ACTION_ENABLED_IF( a, !d_peaks.isNull() );

	PeakList::SpecList::const_iterator p1;
	SpecRef<Spectrum> spec;
	
	Lexi::Viewport::pushHourglass();
	try
	{
		QString str;
		SpecRotator* sr;
		if( d_rot.empty() )
			sr = new SpecRotator( d_spec );
		else
			sr = new SpecRotator( d_spec, d_rot );
		for( p1 = d_peaks->getSpecs().begin(); p1 != d_peaks->getSpecs().end(); ++p1 )
		{
			spec = d_pro->getSpec( (*p1) );
			if( spec )
			{
				sr->setSpectrum( spec );
				str.sprintf( "Detecting Levels: %s", spec->getName() );
				Lexi::ShowStatusMessage msg1( str );
				traverseUp( msg1 );
				d_peaks->updateAllAmps( sr, d_pp->getRot(), d_folding );
				str.sprintf( "Integrating: %s", spec->getName() );
				Lexi::ShowStatusMessage msg2( str );
				traverseUp( msg2 );
				d_peaks->integrateAll( sr, d_intMeth );
			}
		}
		Lexi::Viewport::popCursor();
		Lexi::ShowStatusMessage msg2( "Done" );
		traverseUp( msg2 );
	}catch( Root::Exception& e )
	{
		Lexi::Viewport::popCursor();
		Root::ReportToUser::alert( this, "Integrate All", e.what() );
	}
}

void MonoScopeAgent::handlePickBounds(Action & a)
{
	ACTION_ENABLED_IF( a, d_showSlices );

	try
	{
		Amplitude val = 0;
		/* TODO if( d_peaks )
			val = d_plane.d_peaks->getModel()->getAt( d_cursor, d_folding );
		else */
			val = d_spec->getAt( d_cursor, d_folding, d_folding );
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

void MonoScopeAgent::handleSetSliceMinMax(Action & a)
{
	ACTION_ENABLED_IF( a, d_showSlices );
	
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

void MonoScopeAgent::handleSliceAutoScale(Action & a)
{
	ACTION_CHECKED_IF( a, d_showSlices, 
		d_showSlices && d_slices[ 0 ].d_slice->isAutoScale() );

	bool on = !d_slices[ 0 ].d_slice->isAutoScale();
	for( Index i = 0; i < d_slices.size(); i++ )
	{
		d_slices[ i ].d_slice->setAutoScale( on );
		d_slices[ i ].d_viewer->redraw();
	}
}

void MonoScopeAgent::handlePickBoundsSym(Action & a)
{
	ACTION_ENABLED_IF( a, d_showSlices );

	try
	{
		Amplitude val = 0;
		/* TODO if( d_peaks )
			val = d_plane.d_peaks->getModel()->getAt( d_cursor, d_folding );
		else */
			val = d_spec->getAt( d_cursor, d_folding, d_folding );
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

void MonoScopeAgent::handlePeakCurve(Action & a)
{
	ACTION_ENABLED_IF( a, !d_peaks.isNull() && d_plane.d_peaks->getSel().size() == 1 &&
		d_peaks->getDimCount() <= d_spec->getDimCount() );

	if( d_plane.d_peaks->getSel().empty() )	// QT-BUG
		return;

	PeakBatchCurve* v = new PeakBatchCurve( d_peaks, 
		d_peaks->getPeak( *d_plane.d_peaks->getSel().begin() ), d_pro );
	v->show();
}

void MonoScopeAgent::handleBackwardPlane(Action & a)
{
	if( d_spec->getDimCount() != 3 )
		return;
	ACTION_ENABLED_IF( a, true ); 

	d_slices[ DimZ ].d_cur->setCursor( (Dimension)DimY, 
		d_cursor[ DimZ ] - d_spec->getScale( DimZ ).getDelta() );  
}

void MonoScopeAgent::handleForwardPlane(Action & a)
{
	if( d_spec->getDimCount() != 3 )
		return;
	ACTION_ENABLED_IF( a, true ); 

	d_slices[ DimZ ].d_cur->setCursor( (Dimension)DimY, 
		d_cursor[ DimZ ] + d_spec->getScale( DimZ ).getDelta() );  
}

void MonoScopeAgent::handleCreateReport(Action &a)
{
	ACTION_ENABLED_IF( a, true );

	Dlg::ContourParams p;
	p.d_factor = d_plane.d_ol[0].d_view->getFactor();
	p.d_threshold =	d_spec->getThreshold();
	p.d_option = d_plane.d_ol[0].d_view->getOption();
	ReportViewer* rv = ReportViewer::getViewer( getParent()->getParent(), p, 
		d_plane.d_ol[0].d_view->getGain(), d_plane.d_ol[0].d_view->isAuto(), d_folding,
		(d_pro && d_plane.d_ol.size()>1)?d_pro->getRepository():0 );
	ReportViewer::Spector vec;
	for( int i = 0; i < d_plane.d_ol.size(); i++ )
		if( d_plane.d_ol[i].d_spec )
			vec.push_back( d_plane.d_ol[i].d_spec );
	PeakProjector* pp = 0;
	if( d_pp )
	{
		pp = new PeakProjector( d_pp, DimX, DimY );
		for( Dimension d = 2; d < d_spec->getDimCount(); d++ )
		{
			// Erst ab 2 beginnen, da x und y nicht eingeschrnkt werden sollen.
			// cursor und spec haben gleiche Rot. Peaks werden auf diese abgebildet.
			PPM w = d_spec->getPeakWidth( d );
			if( w <= 0.0 )
				w = d_spec->getScale( d ).getDelta();
			pp->setOrigThick( d, d_cursor[ d ], w );
		}
	}
	rv->showPlane( d_plane.d_viewer->getViewArea(), vec, pp );
}

void MonoScopeAgent::handleEditAtts(Action & a)
{
	ACTION_ENABLED_IF( a, d_pro && !d_peaks.isNull() && d_plane.d_peaks->getSel().size() == 1 &&
		d_peaks->getDimCount() <= d_spec->getDimCount() );

	Peak* p = d_peaks->getPeak( *d_plane.d_peaks->getSel().begin() );

	DynValueEditor::edit( MW, 
		d_pro->getRepository()->findObjectDef( Repository::keyPeak ), p );
}

void MonoScopeAgent::handleSetColor(Action & a)
{
	ACTION_ENABLED_IF( a, !d_peaks.isNull() && !d_plane.d_peaks->getSel().empty() );

	Peak* p = 0;
	if( d_plane.d_peaks->getSel().size() == 1 )
		p = d_peaks->getPeak( *d_plane.d_peaks->getSel().begin() );
	bool ok;
	int res = QInputDialog::getInteger( "Set Color", 
		"Please enter a valid color code:", 
		(p)?p->getColor():0, 0, 255, 1, &ok, MW );
	if( !ok )
		return;
	Root::Ref<Root::MakroTransaction> cmd = new Root::MakroTransaction( "Set Peak Color" );
	try
	{
		PeakPlaneView::Selection::const_iterator i;
		for( i = d_plane.d_peaks->getSel().begin(); i != d_plane.d_peaks->getSel().end(); ++i )
		{
			COP cop( true, true );
			d_pp->setColor( (*i), res, cop );
			cmd->add( cop.d_done );
		}
	}catch( Root::Exception& e )
	{
		cmd->unexecute();
		Root::ReportToUser::alert( this, "Set Peak Color", e.what() );
		return;
	}
	cmd->registerForUndo( this );
}

void MonoScopeAgent::handleCursorSync(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_cursorSync );
	
	d_cursorSync = !d_cursorSync;
	if( d_cursorSync )
		GlobalCursor::addObserver( this );	// TODO: preset Cursor
	else if( !d_rangeSync )
		GlobalCursor::removeObserver( this );
}

void MonoScopeAgent::handleCalcMaximum(Action & a)
{
	ACTION_ENABLED_IF( a, d_spec );

	Lexi::Viewport::pushHourglass();
	Spectrum::Levels l = d_spec->getLevels();
	Amplitude pmax, nmax, pmean, nmean;
	d_plane.d_ol[0].d_buf->getBuffer().calcMeanMinMax( nmean, pmean, nmax, pmax );
	const double mean = ( pmean + nmean ) * 0.5;
	const Buffer::Cells& data = d_plane.d_ol[0].d_buf->getBuffer().getRawData();
	const double count = data.size();
	double pint = 0.0, nint = 0.0;
	double sum = 0.0;
	for( int i = 0; i < data.size(); i++ )
	{
		sum += ::pow( data[ i ] - mean, 2.0 ) / count;
		if( data[ i ] > 0.0 )
			pint += data[ i ];
		else
			nint += data[ i ];
	}
	sum = ::sqrt( sum );
	Lexi::Viewport::popCursor();
	QString str;
	str.sprintf( "Maxima of whole spectrum:\n"
		"Maximum pos.: %.0f\n"
		"Maximum neg.: %.0f\n\n"
		"Maxima and noise of selected plane area: \n"
		"Number of points: %d\n"
		"Maximum pos.: %.0f\n"
		"Maximum neg.: %.0f\n"
		"Integral pos.: %.0f\n"
		"Integral neg.: %.0f\n"
		"Mean pos.: %.0f\n"
		"Mean neg.: %.0f\n"
		"Mean avg.: %.2f\n"
		"Noise [ sqrt( sum( ( amp(i) - Mean avg. )^2 ) / num. of pts. ) ) ]: %.2f\n", 
		l.d_pMax, l.d_nMax, 
		data.size(), pmax, nmax, pint, nint, pmean, nmean, mean, sum );
	Root::MessageLog::inst()->info( "Show Maximum Amplitudes", str.toLatin1() );
	Dlg::showInfo( MW, "Show Maximum Amplitudes", str );
}

void MonoScopeAgent::handleAutoGain(Action & a)
{
	ACTION_ENABLED_IF( a, !a.getParam( 0 ).isNull() );

	float g = a.getParam( 0 ).getFloat();
	if( g <= 0.0 )
	{
		Root::ReportToUser::alert( this, "Set Auto Gain", "Invalid Gain Value" );
		return;
	}
	int l = d_aol;
	if( a.getParam( 1 ).isNull() )
		l = selectLayer();
	else if( ::strcmp( a.getParam( 1 ).getCStr(), "*" ) == 0 )
		l = -1;
	else
		l = a.getParam( 1 ).getLong();
	if( l == -2 )
		return;
	else if( l == -1 ) // All
	{
		for( l = 0; l < d_plane.d_ol.size(); l++ )
		{
			d_plane.d_ol[l].d_view->setVisi( true );
			d_plane.d_ol[l].d_view->setGain( g );
			d_plane.d_ol[l].d_view->createLevelsAuto();
		}
	}else if( l >= 0 && l < d_plane.d_ol.size() )
	{
		d_plane.d_ol[l].d_view->setVisi( true );
		d_plane.d_ol[l].d_view->setGain( g );
		d_plane.d_ol[l].d_view->createLevelsAuto();
	}
	d_plane.d_viewer->damageMe();
}

void MonoScopeAgent::handleRangeSync(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_rangeSync );
	
	d_rangeSync = !d_rangeSync;
	if( d_rangeSync )
		GlobalCursor::addObserver( this );	// TODO: preset Cursor
	else if( !d_cursorSync )
		GlobalCursor::removeObserver( this );
}

void MonoScopeAgent::handleSetDepth(Action & a)
{
	ACTION_ENABLED_IF( a, d_spec->getDimCount() > 2 );

	float g;
	if( a.getParam( 0 ).isNull() )
	{
		bool ok	= FALSE;
		QString res;
		res.sprintf( "%f", d_spec->getPeakWidth( DimZ ) );
		res	= QInputDialog::getText( "Set Peak Depth", 
			"Please	enter a positive ppm value:", QLineEdit::Normal, res, &ok, 
			MW );
		if( !ok )
			return;
		g = res.toFloat( &ok );
	}else
		g = a.getParam( 0 ).getFloat();
	if( g < 0.0 )
	{
		Root::ReportToUser::alert( this, "Set Peak Depth", "Invalid PPM Value" );
		return;
	}
	d_spec->setPeakWidth( DimZ, g );
	if( d_plane.d_peaks->getModel() )
		d_plane.d_peaks->getModel()->setOrigThick( DimZ, d_cursor[ DimZ ], g );
}

void MonoScopeAgent::handleOverlayCount(Action & a)
{
	ACTION_ENABLED_IF( a, true ); 

	Root::Index c;
	if( a.getParam( 0 ).isNull() )
	{
		bool ok	= FALSE;
		c = QInputDialog::getInteger( "Set Overlay Count", 
			"Please	enter a positive number:", 
			d_plane.d_ol.size(), 1, 9, 1, &ok, MW );
		if( !ok )
			return;
	}else
		c = a.getParam( 0 ).getLong();
	if( c < 1 )
	{
		QMessageBox::critical( MW, "Set Overlay Count",
				"Invalid Count!", "&Cancel" );
		return;
	}
	initOverlay( c );
}

void MonoScopeAgent::handleActiveOverlay(Action & a)
{
	ACTION_ENABLED_IF( a, true ); 

	Root::Index c;
	if( a.getParam( 0 ).isNull() )
	{
		Dlg::StringList l( d_plane.d_ol.size() );
		QString str;
		for( int i = 0; i < d_plane.d_ol.size(); i++ )
		{
			if( d_plane.d_ol[ i ].d_spec )
				str.sprintf( "&%d %s", i, d_plane.d_ol[ i ].d_spec->getName() );
			else
				str.sprintf( "&%d <empty>", i );
			l[ i ] = str.toLatin1();
		}
		c = Dlg::getOption( MW, l, 
			"Select Active Overlay", d_aol );
		if( c == -1 )
			return;
	}else
		c = a.getParam( 0 ).getLong();
	if( c < 0 || c >= d_plane.d_ol.size() )
	{
		QMessageBox::critical( MW, "Set Active Overlay",
				"Invalid Overlay Number!", "&Cancel" );
		return;
	}
	setActiveOverlay( c );
}

void MonoScopeAgent::handleSetPosColor(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	
	QColor clr = QColorDialog::getColor( d_plane.d_ol[d_aol].d_view->getPosColor(), 
		MW );
	if( clr.isValid() )
	{
		d_plane.d_ol[d_aol].d_view->setPosColor( ( clr ) );
		d_plane.d_viewer->redraw();
	}
}

void MonoScopeAgent::handleSetNegColor(Action & a)
{
	ACTION_ENABLED_IF( a, true ); 

	QColor clr = QColorDialog::getColor( d_plane.d_ol[d_aol].d_view->getNegColor(), 
		MW );
	if( clr.isValid() )
	{
		d_plane.d_ol[d_aol].d_view->setNegColor( ( clr ) );
		d_plane.d_viewer->redraw();
	}
}

void MonoScopeAgent::handleOverlaySpec(Action & a)
{
	ACTION_ENABLED_IF( a, d_pro ); 

	Spectrum* spec = d_plane.d_ol[d_aol].d_spec;
	Root::Index c = 0;
	if( a.getParam( 0 ).isNull() )
	{
		Dlg::StringSet s;
		Project::SpectrumMap::const_iterator i;
		s.insert( "" ); // empty
		for( i = d_pro->getSpectra().begin(); i != d_pro->getSpectra().end(); ++i )
			if( (*i).second->getDimCount() == 2 )
				s.insert( (*i).second->getName() );
		if( !Dlg::selectStrings( MW, 
			"Select Overlay Spectrum", s, false ) || s.empty() )
			return;
		spec = d_pro->findSpectrum( (*s.begin()).data() );
	}else
	{
		c = a.getParam( 0 ).getLong();

		spec = d_pro->getSpec( c );
		if( spec == 0 && c != 0 )
		{
			QMessageBox::critical( MW, "Set Overlay Spectrum",
					"Invalid spectrum ID!", "&Cancel" );
			return;
		}
		if( spec && spec->getDimCount() != 2 )
		{
			QMessageBox::critical( MW, "Set Overlay Spectrum",
					"Invalid number of dimensions!", "&Cancel" );
			return;
		}
	}
	if( d_aol == 0 && spec == 0 )
	{
		QMessageBox::critical( MW, "Set Overlay Spectrum",
				"Cannot remove spectrum of layer 0!", "&Cancel" );
		return;
	}
	if( d_aol == 0 )
		setSpec( spec );
	else if( spec )
	{
		d_plane.d_ol[d_aol].d_spec = new SpecProjector( spec, DimX, DimY );
		d_plane.d_ol[d_aol].d_buf->setSpectrum( d_plane.d_ol[d_aol].d_spec );
		d_plane.d_viewer->redraw();
	}else
	{
		d_plane.d_ol[d_aol].d_spec = 0;
		d_plane.d_ol[d_aol].d_buf->setSpectrum( 0 );
		d_plane.d_viewer->redraw();
	}
	// updatePlaneLabel();
}

void MonoScopeAgent::handleCntFactor(Action & a)
{
	ACTION_ENABLED_IF( a, !a.getParam( 0 ).isNull() );

	float g = a.getParam( 0 ).getFloat();
	if( g <= 1.0 || g > 10.0 )
	{
		Root::ReportToUser::alert( this, "Set Contour Factor", "Invalid Factor Value" );
		return;
	}
	int l = d_aol;
	if( a.getParam( 1 ).isNull() )
		l = selectLayer();
	else if( ::strcmp( a.getParam( 1 ).getCStr(), "*" ) == 0 )
		l = -1;
	else
		l = a.getParam( 1 ).getLong();
	if( l == -2 )
		return;
	else if( l == -1 ) // All
	{
		for( l = 0; l < d_plane.d_ol.size(); l++ )
		{
			d_plane.d_ol[l].d_view->setFactor( g );
			d_plane.d_ol[l].d_view->setVisi( true );
			updateContour( l, false );
		}
		d_plane.d_viewer->damageMe();
	}else if( l >= 0 && l < d_plane.d_ol.size() )
	{
		d_plane.d_ol[l].d_view->setFactor( g );
		d_plane.d_ol[l].d_view->setVisi( true );
		updateContour( l, true );
	}
}

void MonoScopeAgent::handleCntThreshold(Action & a)
{
	ACTION_ENABLED_IF( a, !a.getParam( 0 ).isNull() );

	float g = a.getParam( 0 ).getFloat();
	if( g < 0.0 )
	{
		Root::ReportToUser::alert( this, "Set Spectrum Threshold", "Invalid Threshold Value" );
		return;
	}
	int l = d_aol;
	if( a.getParam( 1 ).isNull() )
		l = selectLayer();
	else if( ::strcmp( a.getParam( 1 ).getCStr(), "*" ) == 0 )
		l = -1;
	else
		l = a.getParam( 1 ).getLong();
	if( l == -2 )
		return;
	else if( l == -1 ) // All
	{
		for( l = 0; l < d_plane.d_ol.size(); l++ )
		{
			if( d_plane.d_ol[l].d_spec )
				d_plane.d_ol[l].d_spec->setThreshold( g );
			d_plane.d_ol[l].d_view->setVisi( true );
			d_plane.d_ol[l].d_view->createLevelsMin( g );
		}
	}else if( l >= 0 && l < d_plane.d_ol.size() && d_plane.d_ol[l].d_spec )
	{
		d_plane.d_ol[l].d_spec->setThreshold( g );
		d_plane.d_ol[l].d_view->setVisi( true );
		d_plane.d_ol[l].d_view->createLevelsMin( g );
	}
	d_plane.d_viewer->damageMe();
}

void MonoScopeAgent::handleCntOption(Action & a)
{
	ACTION_ENABLED_IF( a, !a.getParam( 0 ).isNull() );

	ContourView::Option o = ContourView::Both;
	if( strcmp( a.getParam(0).getCStr(), "+" ) == 0 )
		o = ContourView::Positive;
	else if( strcmp( a.getParam(0).getCStr(), "-" ) == 0 )
		o = ContourView::Negative;
	
	int l = d_aol;
	if( a.getParam( 1 ).isNull() )
		l = selectLayer();
	else if( ::strcmp( a.getParam( 1 ).getCStr(), "*" ) == 0 )
		l = -1;
	else
		l = a.getParam( 1 ).getLong();
	if( l == -2 )
		return;
	else if( l == -1 ) // All
	{
		for( l = 0; l < d_plane.d_ol.size(); l++ )
		{
			d_plane.d_ol[l].d_view->setOption( o );
			d_plane.d_ol[l].d_view->setVisi( true );
			updateContour( l, false );
		}
		d_plane.d_viewer->damageMe();
	}else if( l >= 0 && l < d_plane.d_ol.size() )
	{
		d_plane.d_ol[l].d_view->setOption( o );
		d_plane.d_ol[l].d_view->setVisi( true );
		updateContour( l, true );
	}
}

void MonoScopeAgent::handleAddLayer(Action & a)
{
	ACTION_ENABLED_IF( a, d_pro ); 

	initOverlay( d_plane.d_ol.size() + 1 );
	setActiveOverlay( d_plane.d_ol.size() - 1 );
	handleOverlaySpec( a );
}

void MonoScopeAgent::handleComposeLayers(Action & a)
{
	ACTION_ENABLED_IF( a, d_pro ); 

	ColorMap cm( 2 );
	Root::Ref<PeakList> pl = new PeakList( cm );
	PeakList::SpecList l;
	for( int i = 0; i < d_plane.d_ol.size(); i++ )
		if( d_plane.d_ol[i].d_spec )
			l.push_back( d_plane.d_ol[i].d_spec->getId() );
	pl->setSpecs( l );
	SpecBatchList dlg( MW, pl, d_pro );
	dlg.setCaption( "Compose Layers" );
	if( dlg.doit() && !pl->getSpecs().empty() )
	{
		const PeakList::SpecList& s = pl->getSpecs();
		initOverlay( s.size() );
		Spectrum* spec;
		for( int i = 0; i < s.size(); i++ )
		{
			spec = d_pro->getSpec( s[ i ] );
			assert( spec );
			d_plane.d_ol[i].d_spec = new SpecProjector( spec, DimX, DimY );
			d_plane.d_ol[i].d_buf->setSpectrum( d_plane.d_ol[i].d_spec );
		}
		d_plane.d_viewer->redraw();
		// updatePlaneLabel();
	}
}

void MonoScopeAgent::handleSyncDepth(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_syncDepth );
	
	d_syncDepth = !d_syncDepth;
}

void MonoScopeAgent::handleFitWindowX(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	d_plane.d_ol[0].d_buf->fitToDim(DimX);
	d_plane.d_viewer->damageMe();
	if( d_showSlices )
	{
		d_slices[ DimX ].d_buf->fitToArea();
		d_slices[ DimX ].d_viewer->redraw();
	}
}

void MonoScopeAgent::handleFitWindowY(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	d_plane.d_ol[0].d_buf->fitToDim(DimY);
	d_plane.d_viewer->damageMe();
	if( d_showSlices )
	{
		d_slices[ DimY ].d_buf->fitToArea();
		d_slices[ DimY ].d_viewer->redraw();
	}
}

void MonoScopeAgent::handleDeleteAliasPeaks(Action & a)
{
	ACTION_ENABLED_IF( a, !d_peaks.isNull() && !d_plane.d_peaks->getSel().empty() );

	Root::Ref<Root::MakroTransaction> cmd = new Root::MakroTransaction( "Delete Alias Peaks" );
	try
	{
		PeakPlaneView::Selection sel = d_plane.d_peaks->getSel();
		PeakPlaneView::Selection::const_iterator p;
		PeakSpace::PeakData orig;
		PeakSpace::PeakData home;
		for( p = sel.begin(); p != sel.end(); ++p )
		{
			COP cop( true, true );
			d_pp->getPeak( (*p), orig, d_spec );
			if( orig.isAlias() )
			{
				d_pp->getPeak( (*p), home );
				d_pp->setPos( (*p), home.d_pos, home.d_amp, d_spec, cop );
				cmd->add( cop.d_done );
			}
		}
	}catch( Root::Exception& e )
	{
		cmd->unexecute();
		Root::ReportToUser::alert( this, "Delete Alias Peaks", e.what() );
		return;
	}
	cmd->registerForUndo( this );
}

void MonoScopeAgent::handleAdjustIntensity(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	
	Dlg::adjustIntensity( MW, d_plane.d_intens );
}

void MonoScopeAgent::handleSetIntMeth(Action & a)
{
	if( a.getParamCount() == 0 )
		return;
	const short v = a.getParam( 0 ).getShort();
	if( v < 0 || v >= PeakList::MAX_IntegrationMethod )
		return;
	ACTION_CHECKED_IF( a, true, d_intMeth == v );

	d_intMeth = (PeakList::IntegrationMethod) v;
	
}


