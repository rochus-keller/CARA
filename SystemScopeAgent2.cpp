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

#include "SystemScopeAgent2.h"
#include <qtextstream.h> 
#include <qinputdialog.h> 
#include <qmessagebox.h>
#include <Root/Any.h>
#include <Root/MakroCommand.h>
#include <Lexi/MainWindow.h>
#include <Lexi/Background.h>
#include <Lexi/Label.h>
#include <Lexi/ContextMenu.h>
#include <Spec/SpecProjector.h>
#include <Spec/SpecRotator.h>
#include <Spec/SpectrumType.h>
#include <Spec/SpectrumPeer.h>
#include <Spec/Repository.h>
#include <Spec/SpinPointSpace.h>
#include <Spec/FragmentAssignment.h>
#include <SpecView/CursorMdl.h>
#include <SpecView/ViewAreaMdl.h>
#include <SpecView/SpecViewer.h>
#include <SpecView/FocusCtrl.h>
#include <SpecView/ContourView.h>
#include <SpecView/CursorView.h>
#include <SpecView/CursorCtrl.h>
#include <SpecView/ScrollCtrl.h>
#include <SpecView/ZoomCtrl.h>
#include <SpecView/SelectZoomCtrl.h>
#include <SpecView/SliceView.h>
#include <SpecView/CenterLine.h>
#include <SpecView/SpinLineView.h>
#include <SpecView/PointSelectCtrl.h>
#include <SpecView/FoldingView.h>
#include <SpecView/SelectRulerCtr.h>
#include <Spec/GlobalCursor.h>
#include <Dlg.h>
#include <SpecView/DynValueEditor.h>
#include "ProposeSpinDlg.h"
using namespace Spec;

static const int BACKGROUND = 0;
static const int INTENSITY = 1;
static const int CONTOUR = 2;
static const int FOLDING = 3;
static const int LABEL1 = 4;
static const int LABEL2 = 5;
static const int PEAKS = 6;
static const int CURSORS = 7;
static const int TUPLES = 8;
static const int VIEWCOUNT = 9;

static QColor g_clrHold = Qt::cyan;
static QColor g_clrLabel = Qt::yellow;

#define MW ((Lexi::MainWindow*) getParent())->getQt()

//////////////////////////////////////////////////////////////////////

Action::CmdStr SystemScopeAgent2::ContourParams = "ContourParams";
Action::CmdStr SystemScopeAgent2::AutoContour = "AutoContour";
Action::CmdStr SystemScopeAgent2::FitWindow = "FitWindow";
Action::CmdStr SystemScopeAgent2::DeleteSpins = "DeleteSpins";
Action::CmdStr SystemScopeAgent2::PickSpin = "PickSpin";
Action::CmdStr SystemScopeAgent2::MoveSpin = "MoveSpin";
Action::CmdStr SystemScopeAgent2::MoveSpinAlias = "MoveSpinAlias";
Action::CmdStr SystemScopeAgent2::LabelSpin = "LabelSpin";
Action::CmdStr SystemScopeAgent2::ForceLabelSpin = "ForceLabelSpin";
Action::CmdStr SystemScopeAgent2::SelectSpec = "SelectSpec";
Action::CmdStr SystemScopeAgent2::SetResolution = "SetResolution";
Action::CmdStr SystemScopeAgent2::ShowLowRes = "ShowLowRes";
Action::CmdStr SystemScopeAgent2::AutoCenter = "AutoCenter";
Action::CmdStr SystemScopeAgent2::ShowFolded = "ShowFolded";
Action::CmdStr SystemScopeAgent2::ShowOrthogonal = "ShowOrthogonal";
Action::CmdStr SystemScopeAgent2::PickOrtho = "PickOrtho";
Action::CmdStr SystemScopeAgent2::MoveOrtho = "MoveOrtho";
Action::CmdStr SystemScopeAgent2::MoveOrthoAlias = "MoveOrthoAlias";
Action::CmdStr SystemScopeAgent2::LabelOrtho = "LabelOrtho";
Action::CmdStr SystemScopeAgent2::ForceLabelOrtho = "ForceLabelOrtho";
Action::CmdStr SystemScopeAgent2::DeleteOrthos = "DeleteOrthos";
Action::CmdStr SystemScopeAgent2::ShowVCursor = "ShowVCursor";
Action::CmdStr SystemScopeAgent2::StripCalibrate = "StripCalibrate";
Action::CmdStr SystemScopeAgent2::OrthoCalibrate = "OrthoCalibrate";
Action::CmdStr SystemScopeAgent2::ShowDepth = "ShowDepth";
Action::CmdStr SystemScopeAgent2::SetWidth = "SetWidth";
Action::CmdStr SystemScopeAgent2::ShowWithOff = "ShowWithOff";
Action::CmdStr SystemScopeAgent2::EditAtts = "EditAtts";
Action::CmdStr SystemScopeAgent2::EditAttsOrtho = "EditAttsOrtho";
Action::CmdStr SystemScopeAgent2::CursorSync = "CursorSync";
Root::Action::CmdStr SystemScopeAgent2::NextSpec = "NextSpec";
Root::Action::CmdStr SystemScopeAgent2::PrevSpec = "PrevSpec";
Root::Action::CmdStr SystemScopeAgent2::FitOrtho = "FitOrtho";
Action::CmdStr SystemScopeAgent2::AutoGain = "AutoGain";
Action::CmdStr SystemScopeAgent2::ShowGhosts = "ShowGhosts";
Action::CmdStr SystemScopeAgent2::DoPathSim = "DoPathSim";
Action::CmdStr SystemScopeAgent2::ShowUnlabeled = "ShowUnlabeled";
Action::CmdStr SystemScopeAgent2::ShowInfered = "ShowInfered";
Action::CmdStr SystemScopeAgent2::ShowLinks = "ShowLinks";
Action::CmdStr SystemScopeAgent2::ProposeSpin = "ProposeSpin";
Action::CmdStr SystemScopeAgent2::ProposeOrtho = "ProposeOrtho";
Action::CmdStr SystemScopeAgent2::PickLabel = "PickLabel";
Action::CmdStr SystemScopeAgent2::PickLabelOrtho = "PickLabelOrtho";
Action::CmdStr SystemScopeAgent2::ViewLabels = "ViewLabels";
Action::CmdStr SystemScopeAgent2::SetDepth = "SetDepth";
Action::CmdStr SystemScopeAgent2::GhostLabels = "GhostLabels";
Action::CmdStr SystemScopeAgent2::ViewLabels2 = "ViewLabels2";
Action::CmdStr SystemScopeAgent2::PickBounds = "PickBounds";
Action::CmdStr SystemScopeAgent2::SetBounds = "SetBounds";
Action::CmdStr SystemScopeAgent2::AutoSlice = "AutoSlice";
Action::CmdStr SystemScopeAgent2::PickBoundsSym = "PickBoundsSym";
Action::CmdStr SystemScopeAgent2::DeleteLinks = "DeleteLinks";
Action::CmdStr SystemScopeAgent2::ShowAllPeaks = "ShowAllPeaks";
Action::CmdStr SystemScopeAgent2::HidePeak = "HidePeak";
Action::CmdStr SystemScopeAgent2::EditAttsSys = "EditAttsSys";
Action::CmdStr SystemScopeAgent2::EditAttsLink = "EditAttsLink";
Action::CmdStr SystemScopeAgent2::FitOrthoX = "FitOrthoX";
Action::CmdStr SystemScopeAgent2::ShowUnknown = "ShowUnknown";

ACTION_SLOTS_BEGIN( SystemScopeAgent2 )
    { SystemScopeAgent2::ShowUnknown, &SystemScopeAgent2::handleShowUnknown },
    { SystemScopeAgent2::FitOrthoX, &SystemScopeAgent2::handleFitOrthoX },
    { SystemScopeAgent2::EditAttsLink, &SystemScopeAgent2::handleEditAttsLink },
    { SystemScopeAgent2::EditAttsSys, &SystemScopeAgent2::handleEditAttsSys },
    { SystemScopeAgent2::HidePeak, &SystemScopeAgent2::handleHidePeak },
    { SystemScopeAgent2::ShowAllPeaks, &SystemScopeAgent2::handleShowAllPeaks },
    { SystemScopeAgent2::DeleteLinks, &SystemScopeAgent2::handleDeleteLinks },
    { SystemScopeAgent2::PickBounds, &SystemScopeAgent2::handlePickBounds },
    { SystemScopeAgent2::SetBounds, &SystemScopeAgent2::handleSetBounds },
    { SystemScopeAgent2::AutoSlice, &SystemScopeAgent2::handleAutoSlice },
    { SystemScopeAgent2::PickBoundsSym, &SystemScopeAgent2::handlePickBoundsSym },
    { SystemScopeAgent2::ViewLabels2, &SystemScopeAgent2::handleViewLabels2 },
    { SystemScopeAgent2::GhostLabels, &SystemScopeAgent2::handleGhostLabels },
    { SystemScopeAgent2::SetDepth, &SystemScopeAgent2::handleSetDepth },
    { SystemScopeAgent2::ViewLabels, &SystemScopeAgent2::handleViewLabels },
    { SystemScopeAgent2::PickLabelOrtho, &SystemScopeAgent2::handlePickLabelOrtho },
    { SystemScopeAgent2::PickLabel, &SystemScopeAgent2::handlePickLabel },
    { SystemScopeAgent2::ProposeSpin, &SystemScopeAgent2::handleProposeSpin },
    { SystemScopeAgent2::ProposeOrtho, &SystemScopeAgent2::handleProposeOrtho },
    { SystemScopeAgent2::ShowGhosts, &SystemScopeAgent2::handleShowGhosts },
    { SystemScopeAgent2::DoPathSim, &SystemScopeAgent2::handleDoPathSim },
    { SystemScopeAgent2::ShowUnlabeled, &SystemScopeAgent2::handleShowUnlabeled },
    { SystemScopeAgent2::ShowInfered, &SystemScopeAgent2::handleShowInfered },
    { SystemScopeAgent2::ShowLinks, &SystemScopeAgent2::handleShowLinks },
    { SystemScopeAgent2::AutoGain, &SystemScopeAgent2::handleAutoGain },
    { SystemScopeAgent2::FitOrtho, &SystemScopeAgent2::handleFitOrtho },
    { SystemScopeAgent2::NextSpec, &SystemScopeAgent2::handleNextSpec },
    { SystemScopeAgent2::PrevSpec, &SystemScopeAgent2::handlePrevSpec },
    { SystemScopeAgent2::CursorSync, &SystemScopeAgent2::handleCursorSync },
    { SystemScopeAgent2::EditAtts, &SystemScopeAgent2::handleEditAtts },
    { SystemScopeAgent2::EditAttsOrtho, &SystemScopeAgent2::handleEditAttsOrtho },
    { SystemScopeAgent2::ShowWithOff, &SystemScopeAgent2::handleShowWithOff },
    { SystemScopeAgent2::SetWidth, &SystemScopeAgent2::handleSetWidth },
    { SystemScopeAgent2::OrthoCalibrate, &SystemScopeAgent2::handleOrthoCalibrate },
    { SystemScopeAgent2::StripCalibrate, &SystemScopeAgent2::handleStripCalibrate },
    { SystemScopeAgent2::ShowVCursor, &SystemScopeAgent2::handleShowVCursor },
    { SystemScopeAgent2::PickOrtho, &SystemScopeAgent2::handlePickOrtho },
    { SystemScopeAgent2::MoveOrtho, &SystemScopeAgent2::handleMoveOrtho },
    { SystemScopeAgent2::MoveOrthoAlias, &SystemScopeAgent2::handleMoveOrthoAlias },
    { SystemScopeAgent2::LabelOrtho, &SystemScopeAgent2::handleLabelOrtho },
    { SystemScopeAgent2::ForceLabelOrtho, &SystemScopeAgent2::handleForceLabelOrtho },
    { SystemScopeAgent2::DeleteOrthos, &SystemScopeAgent2::handleDeleteOrthos },
    { SystemScopeAgent2::ForceLabelSpin, &SystemScopeAgent2::handleForceLabelSpin },
    { SystemScopeAgent2::ShowFolded, &SystemScopeAgent2::handleShowFolded },
    { SystemScopeAgent2::AutoCenter, &SystemScopeAgent2::handleAutoCenter },
    { SystemScopeAgent2::SelectSpec, &SystemScopeAgent2::handleSelectSpec },
    { SystemScopeAgent2::LabelSpin, &SystemScopeAgent2::handleLabelSpin },
    { SystemScopeAgent2::DeleteSpins, &SystemScopeAgent2::handleDeleteSpins },
    { SystemScopeAgent2::PickSpin, &SystemScopeAgent2::handlePickSpin },
    { SystemScopeAgent2::MoveSpin, &SystemScopeAgent2::handleMoveSpin },
    { SystemScopeAgent2::MoveSpinAlias, &SystemScopeAgent2::handleMoveSpinAlias },
    { SystemScopeAgent2::FitWindow, &SystemScopeAgent2::handleFitWindow },
    { SystemScopeAgent2::ContourParams, &SystemScopeAgent2::handleContourParams },
    { SystemScopeAgent2::AutoContour, &SystemScopeAgent2::handleAutoContour },
    { SystemScopeAgent2::ShowLowRes, &SystemScopeAgent2::handleShowLowRes },
    { SystemScopeAgent2::SetResolution, &SystemScopeAgent2::handleSetResolution },
ACTION_SLOTS_END( SystemScopeAgent2 )

//////////////////////////////////////////////////////////////////////

SystemScopeAgent2::SystemScopeAgent2(Root::Agent* parent,Project* pro,Spectrum* s):
	Root::Agent( parent ), d_pro( pro ), d_spec( s ), d_lock( false ),
	d_curOrtho( -1 ), d_showVCur( false ), d_ol( SpinPointView::None )
{

	assert( pro );
	d_pro->addObserver( this );
	assert( s && s->getDimCount() == 3 );

	d_orig = s;

	d_src = new SpinPointSpace( pro->getSpins(),
		pro->getRepository()->getTypes(), true, true, false );
	d_src->setSpecType( s->getType() );
	d_mdl = new LinkFilterRotSpace( d_src, s );
	Rotation rot( 2 );
	rot[ DimX ] = s->mapToType( DimX );
	rot[ DimY ] = s->mapToType( DimZ );
	d_anchs = new AnchorSpace( pro->getSpins(),
		pro->getRepository()->getTypes(), rot, true );
	d_anchs->setSpecType( s->getType() );
	d_anchs->addObserver( this );

	d_cursor.assign( 3, 0 );	// RISK

	initParams();
	buildPopup();
	buildViews();
}

SystemScopeAgent2::~SystemScopeAgent2()
{
	d_pro->removeObserver( this );
	
	delete d_popLabel;
	delete d_popLabelOrtho;
	delete d_popSpec;
	delete d_popStrip;
	delete d_popOrtho;

	d_anchs->removeObserver( this );

	GlobalCursor::removeObserver( this );
}

void SystemScopeAgent2::updateSpecPop()
{
	d_popSpec->purge();
	ColorMap a, b;
	d_spec->getColors( a );
	Project::SpecSet l;
	Spectrum* spec = 0;
	a[ DimY ] = AtomType(); // Joker

	const Project::SpectrumMap& sm = d_pro->getSpectra();
	Project::SpectrumMap::const_iterator p;
	Rotation rot;
	l.insert( d_orig );
	for( p = sm.begin(); p != sm.end(); ++p )
	{
		spec = (*p).second;
		if( spec->getDimCount() == 3 && spec->getId() != d_orig->getId() )
		{
			spec->getColors( b );
			if( a[ DimX ] == b[ DimX ] && a[ DimZ ] == b[ DimZ ] )
				l.insert( spec );
			else
			{
				// TEST qDebug( "id=%d name=%s", spec->getId(), spec->getName() );
				if( SpectrumType::autoRotate( a, spec, rot, false ) ) // Keine Auflsungsoptimierung
				{
					l.insert( new SpecRotator( spec, rot ) );
				}
			}
		}
	}
	Project::SpecSet::const_iterator p1;
	Sort::const_iterator pp1;
	for( p1 = l.begin(); p1 != l.end(); ++p1 )
		d_sort[ (*p1)->getName() ] = (*p1);
	for( pp1 = d_sort.begin(); pp1 != d_sort.end(); ++pp1 )
	{
		Gui::Menu::item( d_popSpec, this, (*pp1).first.data(),  
			SystemScopeAgent2::SelectSpec, true )->addParam( Root::Any( (*pp1).second ) );
	}
}

void SystemScopeAgent2::buildPopup()
{
	d_popLabel = new Gui::Menu(); // Explizit lschen

	d_popSpec = new Gui::Menu(); 
	updateSpecPop();

	d_popStrip = new Gui::Menu();
	Gui::Menu::item( d_popStrip, this, "Show &Orthogonal", ShowOrthogonal, false );
	Gui::Menu::item( d_popStrip, this, "Show Depth", ShowDepth, false );
	d_popStrip->insertItem( "Select Spectrum", d_popSpec );
	d_popStrip->insertSeparator();
	Gui::Menu::item( d_popStrip, this, "&Pick Spin", PickSpin, false );
	Gui::Menu::item( d_popStrip, this, "Pick Label...", PickLabel, false );
	Gui::Menu::item( d_popStrip, this, "Propose Spin...", ProposeSpin, false );
	Gui::Menu::item( d_popStrip, this, "&Move Spin", MoveSpin, false );
	Gui::Menu::item( d_popStrip, this, "&Move Spin Alias", MoveSpinAlias, false );
	d_popStrip->insertItem( "Label Spin", d_popLabel );
	Gui::Menu::item( d_popStrip, this, "&Force Spin Label", ForceLabelSpin, false );
	Gui::Menu::item( d_popStrip, this, "&Delete Spins", DeleteSpins, false );
	Gui::Menu::item( d_popStrip, this, "Delete Links", DeleteLinks, false );
	Gui::Menu* menuAtts = new Gui::Menu( d_popStrip );
	Gui::Menu::item( menuAtts, this, "Spin...", EditAtts, false );
	Gui::Menu::item( menuAtts, this, "System...", EditAttsSys, false );
	Gui::Menu::item( menuAtts, this, "Spin Link...", EditAttsLink, false );
	d_popStrip->insertItem( "&Edit Attributes", menuAtts );
	d_popStrip->insertSeparator();
	Gui::Menu::item( d_popStrip, this, "Set Peak Width...", SetWidth, false );
	Gui::Menu::item( d_popStrip, this, "Set Peak Depth...", SetDepth, false );
	Gui::Menu::item( d_popStrip, this, "Fit Window", FitWindow, false, Qt::Key_Home );

	d_popLabelOrtho = new Gui::Menu();

	d_popOrtho = new Gui::Menu(); // Explizit lschen
	Gui::Menu::item( d_popOrtho, this, "&Pick Ortho. Spin", PickOrtho, false );
	Gui::Menu::item( d_popOrtho, this, "Pick Ortho. Label...", PickLabelOrtho, false );
	Gui::Menu::item( d_popOrtho, this, "Propose Ortho. Spin...", ProposeOrtho, false );
	Gui::Menu::item( d_popOrtho, this, "&Move Ortho. Spin", MoveOrtho, false );
	Gui::Menu::item( d_popOrtho, this, "&Move Ortho. Spin Alias", MoveOrthoAlias, false );
	d_popOrtho->insertItem( "Label Spin", d_popLabelOrtho );
	Gui::Menu::item( d_popOrtho, this, "&Force Ortho. Label", ForceLabelOrtho, false );
	Gui::Menu::item( d_popOrtho, this, "&Delete Ortho. Spins", DeleteOrthos, false );
	Gui::Menu::item( d_popOrtho, this, "Edit Attributes...", EditAttsOrtho, false );
	d_popOrtho->insertSeparator();
	Gui::Menu::item( d_popOrtho, this, "Fit Window", FitOrtho, false );
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

void SystemScopeAgent2::handle(Root::Message& msg)
{
	if( d_lock )
		return;
	d_lock = true;
	BEGIN_HANDLER();
	MESSAGE( ViewAreaMdl::Update, a, msg )
	{
		Lexi::Viewport::pushHourglass();
		if( d_slice.d_viewer->getViewArea() == a->getOrigin() )
			updateSlice( a );
		else if( d_strip.d_viewer->getViewArea() == a->getOrigin() )
			updateStrip( a );
		else
			for( int i = 0; i < d_orthos.size(); i++ )
				if( d_orthos[ i ].d_viewer->getViewArea() == a->getOrigin() )
				{
					updateOrtho( i, a );
					break;
				}

		Lexi::Viewport::popCursor();
		msg.consume();
	}
	MESSAGE( CursorMdl::Update, a, msg )
	{
		if( d_slice.d_cur == a->getOrigin() )
			updateSlice( a );
		else if( d_strip.d_cur == a->getOrigin() )
			updateStrip( a );
		else
			for( int i = 0; i < d_orthos.size(); i++ )
				if( d_orthos[ i ].d_cur == a->getOrigin() )
				{
					updateOrtho( i, a );
					break;
				}
		msg.consume();
	}
	MESSAGE( SpinSpace::Update, e, msg )
	{
		if( e->getOrigin() != d_anchs )
			return;
		e->consume();
		switch( e->getType() )
		{
		case SpinSpace::Update::Create:
		case SpinSpace::Update::Delete:
			// NOP
			break;
		case SpinSpace::Update::Move:
		case SpinSpace::Update::All:
			if( !d_anchor.isZero() && e->getElem().d_point == d_anchor )
			{
				d_cursor[ DimX ] = d_anchorX = e->getElem().d_point[ DimX ]->getShift( d_spec );
				d_cursor[ DimZ ] = d_anchorZ = e->getElem().d_point[ DimY ]->getShift( d_spec );
				d_strip.d_spec->setOrigin( d_cursor );
				d_strip.d_viewer->redraw();
				d_slice.d_spec->setOrigin( d_cursor );
				d_slice.d_viewer->redraw();
				d_strip.d_mdl->setOrigThick( d_anchorZ, 
					d_spec->getScale( DimZ ).getDelta(), true ); // RISK
				PpmRange rx = d_spec->getScale( DimX ).getRange();
				PPM w = d_pro->inferStripWidth( DimX, d_spec );
				_allocate( rx, d_cursor[ DimX ], w );
				d_strip.d_viewer->getViewArea()->setRange( DimX, rx );
			}
			break;
        default:
            break;
		}
	}
	MESSAGE( Project::Changed, a, msg )
	{
		msg.consume();
		if( a->d_hint == Project::WidthFactor )
		{
			if( !d_anchor.isZero() )
				setAnchor( d_anchor );
		}else if( a->d_hint == Project::Width )
		{
			if( !d_anchor.isZero() )
				setAnchor( d_anchor );
			PPM w = d_pro->inferPeakWidth( DimX, d_spec );
			for( int i = 0; i < d_orthos.size(); i++ )
                d_orthos[i].d_mdl->setGhostWidth( w );
		}
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
		for( int l = 0; l < d_orthos.size(); l++ )
		{
			if( d_orthos[ l ].d_viewer == a->getViewer() )
			{
				d_curOrtho = l;
				// updateOrthoLabel();
				break;
			}else
				d_curOrtho = -1;
		}
		msg.consume();
	}
	MESSAGE( Spin::Update, a, msg )
	{
		switch( a->getType() )
		{
		case Spin::Update::Shift:
			for( int i = 0; i < d_orthos.size(); i++ )
			{
				if( d_orthos[ i ].d_tuple[ DimY ] == a->getSpin() )
				{
					d_orthos[ i ].d_spec->setOrigin( DimX, 
						d_orthos[ i ].d_tuple[ DimY ]->getShift( d_spec ) );
					break;
				}
			}
			break;
        default:
            break;
		}
		msg.consume();
	}
	MESSAGE( GlobalCursor::UpdatePos, a, msg )
	{
		d_lock = false;
		d_cursorSync = false;
		if( d_strip.d_spec && 
			( a->getDim() == DimY || a->getDim() == DimUndefined ) &&
			d_strip.d_spec->getColor( DimY ) == a->getTy() )
		{
			d_strip.d_cur->setCursor( Dimension( DimY ), a->getY() );
		}
		if( !d_orthos.empty() && 
			( a->getDim() == DimX || a->getDim() == DimUndefined ) &&
			d_orthos[0].d_spec->getColor( DimX ) == a->getTy() )
		{
			d_orthos[0].d_cur->setCursor( Dimension( DimX ), a->getX() );
		}
		d_cursorSync = true;
		msg.consume();
	}
	MESSAGE( Root::Action, a, msg )
	{
		d_lock = false; // Kein Blocking fr Action-Ausfhrung
		EXECUTE_ACTION( SystemScopeAgent2, *a );
	}
	END_HANDLER();
	d_lock = false;
}

void SystemScopeAgent2::updateOrthoLabel()
{
	d_popLabelOrtho->purge();
	Gui::Menu::item( d_popLabelOrtho, this, "?", 
		LabelOrtho, true )->addParam( Root::Any() );
	Gui::Menu::item( d_popLabelOrtho, this, "?-1", 
		LabelOrtho, true )->addParam( new SpinLabelHolder( "?-1" ) );

	//if( d_curOrtho == -1 )
	//	return;
	if( d_anchor.isZero() )
		return;
	NmrExperiment* e = d_pro->getRepository()->getTypes()->
		inferExperiment2( d_spec->getType(), d_anchor[ DimX ]->getSystem(), d_spec );
	assert( e );
	SpinLabelSet l;
	e->getColumn( d_spec->mapToType( DimZ ), l );
	typedef std::map<QByteArray ,SpinLabel> Sort;
	Sort sort;
	SpinLabelSet::const_iterator p;
	for( p = l.begin(); p != l.end(); ++p )
		sort[ (*p).data() ] = (*p);
	Sort::const_iterator pp1;
	for( pp1 = sort.begin(); pp1 != sort.end(); ++pp1 )
	{
		Gui::Menu::item( d_popLabelOrtho, this, (*pp1).first.data(), 
			LabelOrtho, true )->addParam( Root::Any( new SpinLabelHolder( (*pp1).second ) ) );
	}
}

void SystemScopeAgent2::updateStrip( ViewAreaMdl::Update *msg)
{
	// In einem Strip wurde Ausschnitt gendert
	if( msg->getType() != ViewAreaMdl::Update::Range )
		return;

	d_strip.d_viewer->getViewArea()->setRange( DimY, 
		d_strip.d_viewer->getViewArea()->getRange( DimY ) );
	d_strip.d_viewer->redraw();

	d_slice.d_viewer->getViewArea()->setRange( DimY, 
		d_strip.d_viewer->getViewArea()->getRange( DimY ) );
	d_slice.d_viewer->redraw();

	for( int i = 0; i < d_orthos.size(); i++ )
	{
		d_orthos[ i ].d_viewer->getViewArea()->setRange( DimY, 
			d_strip.d_viewer->getViewArea()->getRange( DimY ) );
		d_orthos[ i ].d_viewer->redraw();
	}
}

void SystemScopeAgent2::updateStrip( CursorMdl::Update *msg)
{
	// In einem Strip wurde Cursor gendert

	if( msg->getDim() == DimY || msg->getDim() == DimUndefined )
	{
		PPM pos = msg->getY();

		if( d_autoCenter )
		{
			SpinPoint tuple = 
				d_strip.d_tuples->getHit( d_anchorX, pos );
			if( !tuple.isZero() )
			{
				pos = tuple[ DimY ]->getShift( d_spec ); 
				msg->override( msg->getX(), pos ); 
			}
		}

		d_cursor[ DimY ] = pos;
		if( !d_showVCur )
			d_cursor[ DimX ] = d_anchorX;
		// RISK: unntig d_slice.d_spec->setOrigin( d_cursor );
		for( int i = 0; i < d_orthos.size(); i++ )
			d_orthos[ i ].d_cur->setCursor( (Dimension)DimY, d_cursor[ DimY ] );
		if( d_cursorSync && d_strip.d_spec )
			GlobalCursor::setCursor( DimY, pos, d_strip.d_spec->getColor( DimY ) );
	}
	if( d_showVCur && ( msg->getDim() == DimX || msg->getDim() == DimUndefined ) )
	{
		d_cursor[ DimX ] = msg->getX();
		d_cursor[ DimZ ] = d_anchorZ;
		d_slice.d_spec->setOrigin( d_cursor );
	}
	d_slice.d_cur->setCursor( (Dimension)DimY, d_cursor[ DimY ] );
	notifyCursor();
}

void SystemScopeAgent2::updateSlice( ViewAreaMdl::Update *msg)
{
	if( msg->getType() != ViewAreaMdl::Update::Range )
		return;

	d_strip.d_viewer->getViewArea()->setRange( DimY, 
		d_slice.d_viewer->getViewArea()->getRange( DimY ) );
	d_strip.d_viewer->redraw();
	for( int i = 0; i < d_orthos.size(); i++ )
	{
		d_orthos[ i ].d_viewer->getViewArea()->setRange( DimY, 
			d_slice.d_viewer->getViewArea()->getRange( DimY ) );
		d_orthos[ i ].d_viewer->redraw();
	}
}

void SystemScopeAgent2::updateSlice( CursorMdl::Update *msg)
{
	d_cursor[ DimY ] = msg->getX();
	d_strip.d_cur->setCursor( (Dimension)DimY, d_cursor[ DimY ] );
	for( int i = 0; i < d_orthos.size(); i++ )
		d_orthos[ i ].d_cur->setCursor( (Dimension)DimY, d_cursor[ DimY ] );
	if( d_cursorSync && d_slice.d_spec )
		GlobalCursor::setCursor( (Dimension)DimY, msg->getX(), 
			d_slice.d_spec->getColor( DimX ) );
	notifyCursor();
}

void SystemScopeAgent2::updateOrtho(int i, ViewAreaMdl::Update * msg)
{
	if( msg->getType() != ViewAreaMdl::Update::Range )
		return;
	if( msg->getY() )
	{
		d_strip.d_viewer->getViewArea()->setRange( DimY, 
			d_orthos[ i ].d_viewer->getViewArea()->getRange( DimY ) );
		d_strip.d_viewer->redraw();

		d_slice.d_viewer->getViewArea()->setRange( DimY, 
			d_orthos[ i ].d_viewer->getViewArea()->getRange( DimY ) );
		d_slice.d_viewer->redraw();

		for( int j = 0; j < d_orthos.size(); j++ )
			if( i != j && d_orthos[ j ].d_viewer )
			{
				d_orthos[ j ].d_viewer->getViewArea()->setRange( DimY, 
					d_orthos[ i ].d_viewer->getViewArea()->getRange( DimY ) );
				d_orthos[ j ].d_viewer->redraw();
			}
	}
}

void SystemScopeAgent2::updateOrtho(int i, CursorMdl::Update * msg)
{
	PpmPoint pos( msg->getX(), msg->getY() );

	if( d_autoCenter && msg->getDim() == DimUndefined )
	{
		SpinPoint tuple = 
			d_orthos[ i ].d_tuples->getHit( pos[ DimX ], pos[ DimY ] );
		if( !tuple.isZero() )
		{
			pos[ DimX ] = tuple[ DimX ]->getShift( d_spec ); 
			pos[ DimY ] = tuple[ DimY ]->getShift( d_spec ); 
			msg->override( pos[ DimX ], pos[ DimY ] ); 
		}
	}

	if( msg->getDim() == DimY || msg->getDim() == DimUndefined )
	{
		d_cursor[ DimY ] = pos[ DimY ];
		d_strip.d_cur->setCursor( (Dimension)DimY, d_cursor[ DimY ] );
		d_slice.d_cur->setCursor( (Dimension)DimY, d_cursor[ DimY ] );
		for( int j = 0; j < d_orthos.size(); j++ )
			if( i != j )
				d_orthos[ j ].d_cur->setCursor( (Dimension)DimY, d_cursor[ DimY ] );
		if( d_cursorSync && !d_orthos.empty() )
			GlobalCursor::setCursor( (Dimension)DimY, pos[ DimY ], 
				d_orthos[0].d_spec->getColor( DimY ) );
	}
	if( msg->getDim() == DimX || msg->getDim() == DimUndefined )
	{
		d_cursor[ DimZ ] = pos[ DimX ];
		d_cursor[ DimX ] = d_orthos[ i ].d_spec->getOrigin()[ DimX ];
		d_slice.d_specOrtho->setOrigin( d_cursor );
		if( d_cursorSync && !d_orthos.empty() )
			GlobalCursor::setCursor( (Dimension)DimX, pos[ DimX ], 
				d_orthos[0].d_spec->getColor( DimX ) );
	}
	notifyCursor( true );
}

void SystemScopeAgent2::notifyCursor( bool ortho )
{
	QString str;
    QTextStream ts( &str, QIODevice::WriteOnly );

	ts.setf( QTextStream::fixed );
	ts.precision( 3 );

	if( d_spec )
	{
		ts << d_spec->getName();
		ts <<  "  Cursor:  ";

		for( Dimension d = 0; d < d_spec->getDimCount(); d++ )
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
			Amplitude val = d_spec->getAt( d_cursor, d_folding, d_folding );
			ts.setf( QTextStream::showpos );
			ts.precision( 0 );
			ts << "Level=" << val;
		}catch( ... )
		{
			ts << " Out of Spectrum";
		}
	}else
		ts << "No Spectrum";
	QByteArray  tmp;
	SpinPointView* spv = d_strip.d_tuples;
	if( ortho )
		for( int i = 0; i < d_orthos.size(); i++ )
			if( d_orthos[ i ].d_viewer->hasFocus() )
			{
				spv = d_orthos[ i ].d_tuples;
				break;
			}
	if( spv->formatSelection( tmp, SpinPointView::PairAll, 3 ) )
	{
		str += ",  ";
		str += tmp.data();
	}
	Lexi::ShowStatusMessage msg( str );
	traverseUp( msg );
}

void SystemScopeAgent2::createStrip()
{
	d_strip.d_viewer = new SpecViewer( 
		new ViewAreaMdl( true, true, false, true ), VIEWCOUNT );
	d_strip.d_viewer->getViewArea()->addObserver( this );

	d_strip.d_buf = new SpecBufferMdl( d_strip.d_viewer->getViewArea() );
	d_strip.d_buf->setFolding( d_folding );
	// d_strip.d_buf->setResolution( d_resol );
	// d_strip.d_buf->setScaling( d_lowResol );

	d_strip.d_viewer->getViews()->replace( BACKGROUND, new Lexi::Background() );
	createContour();

	d_strip.d_viewer->getViews()->replace( INTENSITY, 
		new CenterLine( d_strip.d_viewer ) );

	d_strip.d_cur = new CursorMdl();
	d_strip.d_cur->addObserver( this );
	d_strip.d_tuples = new SpinPointView( d_strip.d_viewer );
	d_strip.d_tuples->setLabel( SpinPointView::SysOrResiTagAll, DimY );
	d_strip.d_viewer->getViews()->replace( TUPLES, d_strip.d_tuples );
	CursorView* cv = new CursorView( d_strip.d_viewer, 
		d_strip.d_cur, d_showVCur, true );
	d_strip.d_viewer->getViews()->replace( CURSORS, cv );
	d_strip.d_mdl = new RangeFilterSpace( d_mdl, DimZ );
	d_strip.d_tuples->setModel( d_strip.d_mdl );
	if( d_folding )
		d_strip.d_viewer->getViews()->replace( FOLDING, new FoldingView( d_strip.d_buf ) );
	else
		d_strip.d_viewer->getViews()->replace( FOLDING, 0 );

	d_strip.d_viewer->getHandlers()->append( new ZoomCtrl( d_strip.d_viewer, false, true ) );
	d_strip.d_viewer->getHandlers()->append( new SelectZoomCtrl( d_strip.d_viewer, false, true ) );
	d_strip.d_viewer->getHandlers()->append( new ScrollCtrl( d_strip.d_viewer, false, true ) );
	d_strip.d_curCtl = new CursorCtrl( cv, false, d_showVCur, true );
	d_strip.d_viewer->getHandlers()->append( d_strip.d_curCtl );
	d_strip.d_viewer->getHandlers()->append( new PointSelectCtrl( d_strip.d_tuples, false ) );
	d_strip.d_viewer->getHandlers()->append( new Lexi::ContextMenu( d_popStrip, false ) );
	d_strip.d_viewer->getHandlers()->append( 
		new FocusCtrl( d_strip.d_viewer, this ) );
}

void SystemScopeAgent2::createSlice()
{
	SpecViewer* slice = new SpecViewer( 
		new ViewAreaMdl( false, true, false, true ) );
	d_slice.d_viewer = slice;
	slice->getViewArea()->addObserver( this );

	d_slice.d_buf = new SpecBufferMdl( slice->getViewArea() );
	d_slice.d_buf->setFolding( d_folding );
	d_slice.d_slice = new SliceView( d_slice.d_buf );
	slice->getViews()->append( d_slice.d_slice );

	d_slice.d_bufOrtho = new SpecBufferMdl( slice->getViewArea() );
	d_slice.d_bufOrtho->setFolding( d_folding );
	d_slice.d_sliceOrtho = new SliceView( d_slice.d_bufOrtho, g_clrHold );
	slice->getViews()->append( d_slice.d_sliceOrtho );

	d_slice.d_cur = new CursorMdl();
	d_slice.d_cur->addObserver( this );
	CursorView* cv = new CursorView( slice, d_slice.d_cur );
	slice->getViews()->append( cv );

	slice->getHandlers()->append( new ZoomCtrl( slice, false, true ) );
	slice->getHandlers()->append( new SelectZoomCtrl( slice, false, true ) );
	slice->getHandlers()->append( new ScrollCtrl( slice ) );
	slice->getHandlers()->append( new SelectRulerCtr( slice, true ) );
	slice->getHandlers()->append( new CursorCtrl( cv, false ) );
	slice->getHandlers()->append( new FocusCtrl( slice ) );
}

void SystemScopeAgent2::createOrthos( bool depthOnly )
{
	SpinPointView::Selection sel = d_strip.d_tuples->getSel();

	if( depthOnly )
	{
		sel.clear();
		SpinPoint tt = d_anchor;
		tt[ DimY ] = tt[ DimX ];
		sel.insert( tt );
	}

	PpmPoint orig = d_cursor;

	killOrthos();
	d_orthos.assign( sel.size(), OrthoSocket() );

	PPM w = d_pro->inferPeakWidth( DimX, d_spec );
	Rotation rot( 3 );
	rot[ DimX ] = DimZ;
	rot[ DimY ] = DimY;
	rot[ DimZ ] = DimX;

	if( d_area0.isNull() )
	{
		d_area0 = new ViewAreaMdl( true, true, true, true );
		d_area0->setRange( DimX, 
				d_spec->getScale( DimZ ).getRange(), false );
		d_area0->setRange( DimY, 
			d_strip.d_viewer->getViewArea()->getRange( DimY ), true );
	}

	int i = 0;
	char buf[ 64 ];
	SpinPointView::Selection::const_iterator t;
	for( t = sel.begin(); t != sel.end(); ++t, i++ )
	{
		d_orthos[i].d_tuple = (*t);
		if( i == 0 )
			d_orthos[i].d_viewer = new SpecViewer( d_area0, VIEWCOUNT );
		else
			d_orthos[i].d_viewer = new SpecViewer( 
				new ViewAreaMdl( true, true, true, true ), VIEWCOUNT );
		d_orthos[i].d_viewer->getViewArea()->addObserver( this );

		d_orthos[i].d_buf = new SpecBufferMdl( d_orthos[i].d_viewer->getViewArea() );
		d_orthos[i].d_buf->setFolding( d_folding );
		d_orthos[i].d_buf->setResolution( d_resol );
		d_orthos[i].d_buf->setScaling( d_lowResol );

		d_orthos[i].d_viewer->getViews()->replace( BACKGROUND, new Lexi::Background() );

		ContourView* v = new ContourView( d_orthos[i].d_buf, d_autoContour );
		d_orthos[i].d_viewer->getViews()->replace( CONTOUR, v );
		if( d_autoContour ) // Wirkt, sobald Orthos neu konstruiert.
			v->createLevelsAuto( d_contourFactor, d_contourOption, d_gain );
		else
			v->createLevelsMin( d_contourFactor, d_spec->getThreshold(), d_contourOption );

		d_orthos[i].d_cur = new CursorMdl();
		d_orthos[i].d_cur->addObserver( this );

		d_orthos[i].d_mdl = new RangeFilterSpace( d_mdl, DimX );
		d_orthos[i].d_mdl->showGhosts( d_strip.d_mdl->showGhosts() );
		d_orthos[i].d_tuples = new SpinPointView( d_orthos[i].d_viewer, 
			new RotatedSpace( d_orthos[i].d_mdl, rot ) );
		d_orthos[ i ].d_tuples->setLabel( d_ol, DimX );
		d_orthos[i].d_tuples->ghostLabel( d_strip.d_tuples->ghostLabel() );
		d_orthos[i].d_mdl->setSys( d_anchor[ DimX ]->getSystem() );
        d_orthos[i].d_mdl->setOrigThick( (*t)[ DimY ]->getShift( d_spec ),
			d_spec->getScale( DimX ).getDelta(), true ); // RISK
        d_orthos[i].d_mdl->setGhostWidth( w );

		d_orthos[i].d_viewer->getViews()->replace( PEAKS, 
			new SpinLineView2( d_orthos[i].d_viewer, d_orthos[i].d_tuples->getModel() ) );
		d_orthos[i].d_viewer->getViews()->replace( TUPLES, d_orthos[i].d_tuples );

		CursorView* cv = new CursorView( d_orthos[i].d_viewer, 
			d_orthos[i].d_cur, true, true );
		d_orthos[i].d_viewer->getViews()->replace( CURSORS, cv );
		if( d_folding )
			d_orthos[i].d_viewer->getViews()->replace( FOLDING, new FoldingView( d_orthos[i].d_buf ) );

		d_orthos[i].d_viewer->getHandlers()->append( new ZoomCtrl( d_orthos[i].d_viewer, true, true ) );
		d_orthos[i].d_viewer->getHandlers()->append( new SelectZoomCtrl( d_orthos[i].d_viewer, true, true ) );
		d_orthos[i].d_viewer->getHandlers()->append( new ScrollCtrl( d_orthos[i].d_viewer, true, true ) );
		d_orthos[i].d_viewer->getHandlers()->append( new CursorCtrl( cv, false, true, true ) );
		d_orthos[i].d_viewer->getHandlers()->append( new 
			PointSelectCtrl( d_orthos[i].d_tuples, false ) );
		d_orthos[i].d_viewer->getHandlers()->append( new Lexi::ContextMenu( d_popOrtho, false ) );
		d_orthos[i].d_viewer->getHandlers()->append( 
			new FocusCtrl( d_orthos[i].d_viewer, this ) );

		if( d_spec->getColor( DimY ) == d_spec->getColor( DimX ) )
			orig[ DimX ] = (*t)[ DimY ]->getShift( d_spec );
		d_orthos[ i ].d_spec = new SpecProjector( d_spec, DimZ, DimY, orig );
		d_orthos[ i ].d_buf->setSpectrum( d_orthos[ i ].d_spec );
		if( i != 0 )
			d_orthos[ i ].d_viewer->getViewArea()->setRange( DimX, 
				d_spec->getScale( DimZ ).getRange(), false );
		d_orthos[ i ].d_viewer->getViewArea()->setRange( DimY, 
			d_strip.d_viewer->getViewArea()->getRange( DimY ), true );
		d_orthos[ i ].d_buf->reload();
		
		SpinPointView::formatLabel( buf, sizeof( buf ), (*t), 
			SpinPointView::SysOrResiTagAll, DimY );
		d_orthos[ i ].d_viewer->getViews()->replace( LABEL2,
			new Lexi::Label( buf, nil, g_clrLabel, Lexi::AlignLeft, Lexi::AlignBottom ) );
	}
	d_slice.d_specOrtho = new SpecProjector( d_spec, DimY, orig );
	d_slice.d_bufOrtho->setSpectrum( d_slice.d_specOrtho );
	d_slice.d_viewer->redraw();
	d_curOrtho = -1;
}

void SystemScopeAgent2::updateOrthoContour()
{
	for( int i = 0; i < d_orthos.size(); i++ )
	{
		ContourView* v = (ContourView*) d_orthos[i].d_viewer->getViews()->getComponent( CONTOUR );
		if( d_autoContour ) 
			v->createLevelsAuto( d_contourFactor, d_contourOption, d_gain );
		else
			v->createLevelsMin( d_contourFactor, d_spec->getThreshold(), d_contourOption );
		d_orthos[i].d_viewer->redraw();
	}
}

void SystemScopeAgent2::killOrthos()
{
	for( int i = 0; i < d_orthos.size(); i++ )
		d_orthos[i].d_viewer->getViewArea()->removeObserver( this );

	d_orthos.clear();
	d_curOrtho = -1;
	d_slice.d_specOrtho = 0;
	d_slice.d_bufOrtho->setSpectrum( 0 );
	d_slice.d_viewer->redraw();
}

void SystemScopeAgent2::buildViews()
{
	createStrip();
	createSlice();

}

void SystemScopeAgent2::setSpec(Spectrum * spec)
{
	Spectrum* old = d_spec;
	d_spec = spec;
	assert( spec );
	if( d_mdl->getSpec() != spec )
	{
		d_mdl->setSpec( 0 );
		d_src->setSpec( spec );
		Rotation rot( 2 );
		rot[ DimX ] = spec->mapToType( DimX );
		rot[ DimY ] = spec->mapToType( DimZ );
		d_anchs->setSpec( rot, spec );
		d_mdl->setSpec( d_spec );

		if( d_strip.d_spec )
			d_strip.d_spec->setSpectrum( d_spec );
		if( d_slice.d_spec )
			d_slice.d_spec->setSpectrum( d_spec );
		if( d_slice.d_specOrtho )
			d_slice.d_specOrtho->setSpectrum( d_spec );
		for( int i = 0; i < d_orthos.size(); i++ )
			d_orthos[ i ].d_spec->setSpectrum( d_spec );
		if( d_strip.d_spec )
			d_strip.d_viewer->getViews()->replace( LABEL1,
				new Lexi::Label( d_strip.d_spec->getName(), nil, g_clrLabel, 
					Lexi::AlignLeft, Lexi::AlignTop ) );
		if( spec && spec->getType() && spec->getType()->getProc().empty() )
		{
			Lexi::ShowStatusMessage msg(  "Warning: no spins visible because spectrum type has no procedure!" );
			traverseUp( msg );
		}else
		{
			Lexi::ShowStatusMessage msg(  "Spectrum changed" );
			traverseUp( msg );
		}
		PpmRange rx = d_spec->getScale( DimX ).getRange();
		PPM w = d_pro->inferStripWidth( DimX, d_spec );
		_allocate( rx, d_cursor[ DimX ], w );
		d_strip.d_viewer->getViewArea()->setRange( DimX, rx );

		if( old == 0 || old->getColor( DimY ) != spec->getColor( DimY ) )
		{
			// Nur FitWindow, wenn andere Farbe
			PpmRange r = d_spec->getScale( DimY ).getRange();	
			r.invert();
			d_strip.d_viewer->getViewArea()->setRange( DimY, r );
			d_slice.d_viewer->getViewArea()->setRange( DimY, r );
			for( int i = 0; i < d_orthos.size(); i++ )
				d_orthos[ i ].d_viewer->getViewArea()->setRange( DimY, r );
		}

		createContour();
		d_strip.d_viewer->redraw();
		updateOrthoContour();
		updateLabelPop();
		updateOrthoLabel();
	}
}

void SystemScopeAgent2::stepSpec(bool next)
{
	if( d_sort.size() < 2 )
		return;
	Sort::const_iterator p = d_sort.find( d_spec->getName() );
	if( p == d_sort.end() )
	{
		if( next )
			p = d_sort.begin();
		else
			--p;
	}else
	{
		if( next )
		{
			++p;
			if( p == d_sort.end() )
				p = d_sort.begin();
		}else if( p == d_sort.begin() )
		{
			p = d_sort.end();
			--p;
		}else
			--p;
	}
	assert( p != d_sort.end() );
	Lexi::Viewport::pushHourglass();
	setSpec( (*p).second );
	Lexi::Viewport::popCursor();
}

void SystemScopeAgent2::setAnchor(const SpinPoint & tuple)
{
	d_anchor = tuple;

	// Anchor berechnen und zu 3D ausdehnen
	d_cursor[ DimX ] = d_anchorX = tuple[ DimX ]->getShift( d_spec );
	d_cursor[ DimZ ] = d_anchorZ = tuple[ DimY ]->getShift( d_spec );
	PpmRange rx = d_spec->getScale( DimX ).getRange();
	PPM w = d_pro->inferStripWidth( DimX, d_spec );
	_allocate( rx, d_cursor[ DimX ], w );
	PpmRange ry = d_strip.d_viewer->getViewArea()->getRange( DimY );
	if( ry.empty() )
	{
		ry = d_spec->getScale( DimY ).getRange();	
		ry.invert();
	}

	d_strip.d_spec = new SpecProjector( d_spec, DimX, DimY, d_cursor );
	d_strip.d_buf->setSpectrum( d_strip.d_spec );
	d_strip.d_viewer->getViewArea()->setRange( DimX, rx );
	d_strip.d_viewer->getViewArea()->setRange( DimY, ry );
	d_strip.d_viewer->redraw();

	if( d_slice.d_buf )
	{
		d_slice.d_spec = new SpecProjector( d_spec, DimY, d_cursor );
		d_slice.d_buf->setSpectrum( d_slice.d_spec );
		d_slice.d_viewer->getViewArea()->setRange( DimY, ry );
		d_slice.d_viewer->redraw();
	}

	// Strip setzen
	d_strip.d_mdl->setSys( d_anchor[ DimX ]->getSystem() );
	d_strip.d_mdl->setOrigThick( d_anchorZ, d_spec->getScale( DimZ ).getDelta(), true ); // RISK
	w = d_pro->inferPeakWidth( DimZ, d_spec );
	d_strip.d_mdl->setGhostWidth( w );

	updateLabelPop();
	updateOrthoLabel();

	char buf[ 64 ];
	SpinPointView::formatLabel( buf, sizeof( buf ), d_anchor, 
		SpinPointView::PairIdLabelSysOrResi, DimY );
	d_strip.d_viewer->getViews()->replace( LABEL2,
		new Lexi::Label( buf, nil, g_clrLabel, Lexi::AlignLeft, Lexi::AlignBottom ) );
	d_strip.d_viewer->getViews()->replace( LABEL1,
		new Lexi::Label( d_strip.d_spec->getName(), nil, g_clrLabel, 
			Lexi::AlignLeft, Lexi::AlignTop ) );
}

void SystemScopeAgent2::updateLabelPop()
{
	// Label-Popup setzen
	if( d_popLabel )
	{
		d_popLabel->purge();
		Gui::Menu::item( d_popLabel, this, "?", 
			LabelSpin, true )->addParam( Root::Any() );
		Gui::Menu::item( d_popLabel, this, "?-1", 
			LabelSpin, true )->addParam( new SpinLabelHolder( "?-1" ) );

		if( d_anchor.isZero() )
			return;
		NmrExperiment* e = d_pro->getRepository()->getTypes()->
			inferExperiment2( d_spec->getType(), d_anchor[ DimX ]->getSystem(), d_spec );
		assert( e );
		SpinLabelSet l;
        PathTable::Path filter;
		filter[ d_spec->mapToType( DimX ) ] = d_anchor[ DimX ]->getLabel();
		filter[ d_spec->mapToType( DimZ ) ] = d_anchor[ DimY ]->getLabel();
		e->getColumn( filter, d_spec->mapToType( DimY ), l );
		typedef std::map<QByteArray ,SpinLabel> Sort;
		Sort sort;
		SpinLabelSet::const_iterator p;
		for( p = l.begin(); p != l.end(); ++p )
			sort[ (*p).data() ] = (*p);
		Sort::const_iterator pp1;
		for( pp1 = sort.begin(); pp1 != sort.end(); ++pp1 )
		{
			Gui::Menu::item( d_popLabel, this, (*pp1).first.data(), 
				LabelSpin, true )->addParam( Root::Any( new SpinLabelHolder( (*pp1).second ) ) );
		}
	}
}

void SystemScopeAgent2::createContour()
{
	ContourView* v = new ContourView( d_strip.d_buf, d_autoContour );
	d_strip.d_viewer->getViews()->replace( CONTOUR, v );
	if( d_autoContour )
	{
		v->createLevelsAuto( d_contourFactor, d_contourOption, d_gain );
	}else
		v->createLevelsMin( d_contourFactor, d_spec->getThreshold(), d_contourOption );
}

void SystemScopeAgent2::initParams()
{
	d_resol = 1;
	d_lowResol = false;
	d_autoContour = true;
	d_autoCenter = true;
	d_contourFactor = 1.4f;
	d_contourOption = ContourView::Both;
	d_folding = false;
	d_cursorSync = false;
	d_gain = 2.0;
	// TODO: diese Werte sollen ab Konfigurations-Record gelesen werden
}

void SystemScopeAgent2::handleContourParams(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	Dlg::ContourParams p;
	p.d_factor = d_contourFactor;
	p.d_threshold =	d_spec->getThreshold();
	p.d_option = d_contourOption;
	if( Dlg::setParams( MW, p ) )
	{
		d_spec->setThreshold( p.d_threshold );
		d_contourOption = p.d_option;
		d_contourFactor = p.d_factor;
		d_autoContour = false;
		createContour();
		d_strip.d_viewer->redraw();
		updateOrthoContour();
	}
}

void SystemScopeAgent2::handleAutoContour(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_autoContour );
	
	d_autoContour = !d_autoContour;
	createContour();
	d_strip.d_viewer->redraw();
	updateOrthoContour();
}

void SystemScopeAgent2::handleFitWindow(Action & a)
{
	ACTION_ENABLED_IF( a, true ); 

	PpmRange r = d_spec->getScale( DimY ).getRange();	
	r.invert();
	d_slice.d_viewer->getViewArea()->setRange( DimY, r );
	d_slice.d_viewer->redraw();
	d_strip.d_viewer->getViewArea()->setRange( DimY, r );
	d_strip.d_viewer->redraw();
}

void SystemScopeAgent2::handleDeleteSpins(Action & a)
{
	ACTION_ENABLED_IF( a, !d_anchor.isZero() &&
		!d_strip.d_tuples->getSel().empty() &&
		d_strip.d_viewer->hasFocus() );

	SpinPointView::Selection sel = d_strip.d_tuples->getSel();

	SpinPointView::Selection::const_iterator p;
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Delete Spins" );
	std::set<Spin*> test;
	for( p = sel.begin(); p != sel.end(); ++p )
	{
		if( test.count( (*p)[ DimY ] ) == 0 )
		{
			cmd->add( new DeleteSpinCmd( d_pro->getSpins(), (*p)[ DimY ] ) );
			test.insert( (*p)[ DimY ] );
		}
	}
	cmd->handle( this );
}

void SystemScopeAgent2::handlePickSpin(Action & a)
{
	if( !d_src->showNulls() )
	{
		handlePickLabel( a );
		return;
	}
	ACTION_ENABLED_IF( a, !d_anchor.isZero() );

	Root::Ref<PickSystemSpinCmd> cmd = new PickSystemSpinCmd( d_pro->getSpins(), 
		d_anchor[0]->getSystem(), 
		d_spec->getColor( DimY ), d_cursor[ DimY ], 0 ); 
		// Pick generisches Spektrum
	cmd->handle( this );
	d_strip.d_tuples->selectPeak( d_anchorX, d_cursor[ DimY ] );
}

void SystemScopeAgent2::handleMoveSpin(Action & a)
{
	ACTION_ENABLED_IF( a, !d_anchor.isZero() &&
		d_strip.d_tuples->getSel().size() == 1 &&
		d_strip.d_viewer->hasFocus() );

	SpinPoint tuple = *d_strip.d_tuples->getSel().begin();
	if( d_showVCur )
	{
		Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Move Spin System" );
		for( Dimension d = 0; d < 2; d++ )
		{
			cmd->add( new MoveSpinCmd( d_pro->getSpins(), tuple[ d ], 
				d_cursor[ d ], 0 ) ); // Move generisches Spektrum
		}
		cmd->handle( this );
	}else
	{
		Root::Ref<MoveSpinCmd> cmd =
			new MoveSpinCmd( d_pro->getSpins(), tuple[ DimY ], d_cursor[ DimY ], 0 );
			// Move generisches Spektrum
		cmd->handle( this );
	}
}

void SystemScopeAgent2::handleMoveSpinAlias(Action & a)
{
	ACTION_ENABLED_IF( a, !d_anchor.isZero() &&
		d_strip.d_tuples->getSel().size() == 1 &&
		d_strip.d_viewer->hasFocus() );

	SpinPoint tuple = *d_strip.d_tuples->getSel().begin();
	if( d_showVCur )
	{
		Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Move Spin System" );
		for( Dimension d = 0; d < 2; d++ )
		{
			cmd->add( new MoveSpinCmd( d_pro->getSpins(), tuple[ d ], 
				d_cursor[ d ], d_spec ) );
		}
		cmd->handle( this );
	}else
	{
		Root::Ref<MoveSpinCmd> cmd =
			new MoveSpinCmd( d_pro->getSpins(), tuple[ DimY ], d_cursor[ DimY ], d_spec );
		cmd->handle( this );
	}
}

void SystemScopeAgent2::handleLabelSpin(Action & a)
{
	if( d_anchor.isZero() )
		return;

	SpinLabelHolder* lh = dynamic_cast<SpinLabelHolder*>( a.getParam( 0 ).getObject() );
	SpinLabel l;
	if( lh )
		l = lh->d_label;
	ACTION_ENABLED_IF( a,
		d_strip.d_tuples->getSel().size() == 1 &&
		d_strip.d_viewer->hasFocus() &&
		d_anchor[ DimX ]->getSystem()->isAcceptable( l ) );

	SpinPoint tuple = *d_strip.d_tuples->getSel().begin();
	Root::Ref<LabelSpinCmd> cmd =
		new LabelSpinCmd( d_pro->getSpins(), tuple[ DimY ], l );
	cmd->handle( this );
	if( l.isNull() )
		d_src->showNulls( true );
}

void SystemScopeAgent2::handleForceLabelSpin(Action & a)
{
	ACTION_ENABLED_IF( a, !d_anchor.isZero() &&
		d_strip.d_tuples->getSel().size() == 1 &&
		d_strip.d_viewer->hasFocus() );

	SpinPoint tuple = *d_strip.d_tuples->getSel().begin();

	bool ok	= FALSE;
	QString res;
	if( a.getParam( 0 ).isNull() )
	{
		res.sprintf( "Please enter a valid label (%s):", SpinLabel::s_syntax );
		res = QInputDialog::getText( "Force Spin Label", res, QLineEdit::Normal, 
			tuple[ DimY ]->getLabel().data(), &ok, MW );
		if( !ok )
			return;
	}else
		res = a.getParam( 0 ).getCStr();

	SpinLabel l;
	if( !SpinLabel::parse( res, l ) )
	{
		Root::ReportToUser::alert( this, "Force Spin Label", "Invalid spin label syntax!" );
		return;
	}

	Root::Ref<LabelSpinCmd> cmd =
		new LabelSpinCmd( d_pro->getSpins(), tuple[ DimY ], l );
	cmd->handle( this );
	if( l.isNull() )
		d_src->showNulls( true );
}

void SystemScopeAgent2::handleSelectSpec(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_spec == a.getParam( 0 ).getObject() );

	Lexi::Viewport::pushHourglass();
	setSpec( dynamic_cast<Spectrum*>( a.getParam( 0 ).getObject() ) );
	Lexi::Viewport::popCursor();
}

void SystemScopeAgent2::handleSetResolution(Action & a)
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
		d_strip.d_buf->setResolution( d_resol );
		d_strip.d_viewer->redraw();
		for( int i = 0; i < d_orthos.size(); i++ )
		{
			d_orthos[i].d_buf->setResolution( d_resol );
			d_orthos[i].d_viewer->redraw();
		}
		Viewport::popCursor();
	}
}

void SystemScopeAgent2::handleShowLowRes(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_lowResol );

	Viewport::pushHourglass();
	d_lowResol = !d_lowResol;
	if( d_lowResol )
	{
		d_strip.d_buf->setResolution( d_resol );
		d_strip.d_viewer->redraw();
		for( int i = 0; i < d_orthos.size(); i++ )
		{
			d_orthos[i].d_buf->setResolution( d_resol );
			d_orthos[i].d_viewer->redraw();
		}
	}else
	{
		d_strip.d_buf->setScaling( false );
		d_strip.d_viewer->redraw();
		for( int i = 0; i < d_orthos.size(); i++ )
		{
			d_orthos[i].d_buf->setScaling( false );
			d_orthos[i].d_viewer->redraw();
		}
	}
	Viewport::popCursor();
}

void SystemScopeAgent2::handleAutoCenter(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_autoCenter );

	d_autoCenter = !d_autoCenter;
}

void SystemScopeAgent2::handleShowFolded(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_folding );

	Viewport::pushHourglass();
	d_folding = !d_folding;
	d_strip.d_buf->setFolding( d_folding );
	if( d_folding )
		d_strip.d_viewer->getViews()->replace( FOLDING, new FoldingView( d_strip.d_buf ) );
	else
		d_strip.d_viewer->getViews()->replace( FOLDING, 0 );
	d_strip.d_viewer->redraw();
	d_slice.d_buf->setFolding( d_folding );
	d_slice.d_bufOrtho->setFolding( d_folding );
	d_slice.d_viewer->redraw();
	for( int i = 0; i < d_orthos.size(); i++ )
	{
		d_orthos[i].d_buf->setFolding( d_folding );
		if( d_folding )
			d_orthos[i].d_viewer->getViews()->replace( FOLDING, new FoldingView( d_orthos[i].d_buf ) );
		else
			d_orthos[i].d_viewer->getViews()->replace( FOLDING, 0 );
		d_orthos[i].d_viewer->redraw();
	}
	Viewport::popCursor();
}

void SystemScopeAgent2::handlePickOrtho(Action & a)
{
	if( !d_src->showNulls() )
	{
		handlePickLabelOrtho( a );
		return;
	}
	ACTION_ENABLED_IF( a, d_curOrtho != -1 &&
		d_orthos[ d_curOrtho ].d_viewer->hasFocus() );

	Root::Ref<PickSystemSpinCmd> cmd = new PickSystemSpinCmd( d_pro->getSpins(), 
		d_anchor[0]->getSystem(), 
		d_spec->getColor( DimZ ), d_cursor[ DimZ ], 0 ); 
		// Pick generisches Spektrum
	cmd->handle( this );
	d_orthos[ d_curOrtho ].d_tuples->selectPeak( d_cursor[ DimZ ], d_cursor[ DimY ] );
}

void SystemScopeAgent2::handleMoveOrtho(Action & a)
{
	ACTION_ENABLED_IF( a, d_curOrtho != -1 &&
		d_orthos[ d_curOrtho ].d_tuples->getSel().size() == 1 &&
		d_orthos[ d_curOrtho ].d_viewer->hasFocus() );

	SpinPoint spin = *d_orthos[ d_curOrtho ].d_tuples->getSel().begin();
	Root::Ref<MoveSpinCmd> cmd =
		new MoveSpinCmd( d_pro->getSpins(), spin[ DimX ], d_cursor[ DimZ ], 0 );
		// Move generisches Spektrum
	cmd->handle( this );
}

void SystemScopeAgent2::handleMoveOrthoAlias(Action & a)
{
	ACTION_ENABLED_IF( a, d_curOrtho != -1 &&
		d_orthos[ d_curOrtho ].d_tuples->getSel().size() == 1 &&
		d_orthos[ d_curOrtho ].d_viewer->hasFocus() );

	SpinPoint spin = *d_orthos[ d_curOrtho ].d_tuples->getSel().begin();
	Root::Ref<MoveSpinCmd> cmd =
		new MoveSpinCmd( d_pro->getSpins(), spin[ DimX ], d_cursor[ DimZ ], d_spec );
		// Move generisches Spektrum
	cmd->handle( this );
}

void SystemScopeAgent2::handleLabelOrtho(Action & a)
{
	ACTION_ENABLED_IF( a, d_curOrtho != -1 &&
		d_orthos[ d_curOrtho ].d_tuples->getSel().size() == 1 &&
		d_orthos[ d_curOrtho ].d_viewer->hasFocus() );

	SpinLabelHolder* lh = dynamic_cast<SpinLabelHolder*>( a.getParam( 0 ).getObject() );
	SpinLabel l;
	if( lh )
		l = lh->d_label;
	SpinPoint spin = *d_orthos[ d_curOrtho ].d_tuples->getSel().begin();
	Root::Ref<LabelSpinCmd> cmd =
		new LabelSpinCmd( d_pro->getSpins(), spin[ DimX ], l );
	cmd->handle( this );
	if( l.isNull() )
		d_src->showNulls( true );
}

void SystemScopeAgent2::handleForceLabelOrtho(Action & a)
{
	ACTION_ENABLED_IF( a, d_curOrtho != -1 &&
		d_orthos[ d_curOrtho ].d_tuples->getSel().size() == 1 &&
		d_orthos[ d_curOrtho ].d_viewer->hasFocus() );

	SpinPoint spin = *d_orthos[ d_curOrtho ].d_tuples->getSel().begin();

	bool ok	= FALSE;
	QString res;
	if( a.getParam( 0 ).isNull() )
	{
		res.sprintf( "Please enter a valid label (%s):", SpinLabel::s_syntax );
		res = QInputDialog::getText( "Force Spin Label", res, QLineEdit::Normal, 
			spin[ DimX ]->getLabel().data(), &ok, MW );
		if( !ok )
			return;
	}else
		res = a.getParam( 0 ).getCStr();

	SpinLabel l;
	if( !SpinLabel::parse( res, l ) )
	{
		Root::ReportToUser::alert( this, "Force Spin Label", "Invalid spin label syntax!" );
		return;
	}

	Root::Ref<LabelSpinCmd> cmd =
		new LabelSpinCmd( d_pro->getSpins(), spin[ DimX ], l );
	cmd->handle( this );
	if( l.isNull() )
		d_src->showNulls( true );
}

void SystemScopeAgent2::handleDeleteOrthos(Action & a)
{
	ACTION_ENABLED_IF( a, d_curOrtho != -1 &&
		!d_orthos[ d_curOrtho ].d_tuples->getSel().empty() &&
		d_orthos[ d_curOrtho ].d_viewer->hasFocus() );

	SpinPointView::Selection sel = d_orthos[ d_curOrtho ].d_tuples->getSel();

	SpinPointView::Selection::const_iterator p;
	std::set<Spin*> test;
	for( p = sel.begin(); p != sel.end(); ++p )
		test.insert( (*p)[ DimX ] );

	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Delete Spins" );
	std::set<Spin*>::iterator i;
	for( i = test.begin(); i != test.end(); ++i )
	{
		cmd->add( new DeleteSpinCmd( d_pro->getSpins(), (*i) ) );
	}
	cmd->handle( this );
}

void SystemScopeAgent2::handleShowVCursor(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_showVCur );

	d_showVCur = !d_showVCur;
	d_strip.d_curCtl->use( DimX, d_showVCur );
	d_strip.d_curCtl->getView()->use( DimX, d_showVCur );
	if( !d_showVCur )
	{
		d_cursor[ DimX ] = d_anchorX;
		d_cursor[ DimZ ] = d_anchorZ;
		d_slice.d_spec->setOrigin( d_cursor );
	}
}

void SystemScopeAgent2::handleOrthoCalibrate(Action & a)
{
	ACTION_ENABLED_IF( a, d_curOrtho != -1 &&
		d_orthos[ d_curOrtho ].d_tuples->getSel().size() == 1 &&
		d_orthos[ d_curOrtho ].d_viewer->hasFocus() );

	SpinPoint spin = *d_orthos[ d_curOrtho ].d_tuples->getSel().begin();

	PpmPoint off( 0, 0, 0 );
	off[ DimZ ] = spin[ DimX ]->getShift( d_spec ) - d_cursor[ DimZ ];

	Viewport::pushHourglass();
	Root::Ref<SpecCalibrateCmd> cmd = new SpecCalibrateCmd( d_spec, off );
	cmd->handle( this );
	Viewport::popCursor();
}

void SystemScopeAgent2::handleStripCalibrate(Action & a)
{
	ACTION_ENABLED_IF( a, !d_anchor.isZero() &&
		d_strip.d_tuples->getSel().size() == 1 &&
		d_strip.d_viewer->hasFocus() );

	SpinPoint tuple = *d_strip.d_tuples->getSel().begin();

	PpmPoint off = d_cursor;
	off[ DimZ ] = 0;
	if( !d_showVCur )
		off[ DimX ] = d_anchorX;
	for( Dimension d = 0; d < 2; d++ )
		off[ d ] = tuple[ d ]->getShift( d_spec ) - off[ d ];

	Viewport::pushHourglass();
	Root::Ref<SpecCalibrateCmd> cmd = new SpecCalibrateCmd( d_spec, off );
	cmd->handle( this );
	Viewport::popCursor();
}

void SystemScopeAgent2::handleSetWidth(Action & a)
{
	ACTION_ENABLED_IF( a, !d_anchor.isZero() &&
		d_strip.d_viewer->hasFocus() );

	bool ok	= FALSE;
	QString res;
	res.sprintf( "%0.3f", d_pro->inferPeakWidth( DimX, d_strip.d_spec ) );
	res	= QInputDialog::getText( "Set Peak Width", 
		"Please	enter a positive PPM value:", QLineEdit::Normal, 
		res, &ok, MW );
	if( !ok )
		return;
	PPM w = res.toFloat( &ok );
	if( !ok || w < 0.0 )
	{
		QMessageBox::critical( MW, "Set Peak Width",
				"Invalid peak width!", "&Cancel" );
		return;
	}

	d_pro->setPeakWidth( DimX, w, d_strip.d_spec );
}

void SystemScopeAgent2::handleEditAtts(Action & a)
{
	ACTION_ENABLED_IF( a, !d_anchor.isZero() &&
		d_strip.d_tuples->getSel().size() == 1 &&
		d_strip.d_viewer->hasFocus() );

	SpinPoint tuple = *d_strip.d_tuples->getSel().begin();
	DynValueEditor::edit( MW, 
		d_pro->getRepository()->findObjectDef( Repository::keySpin ), tuple[ DimY ] );
}

void SystemScopeAgent2::handleEditAttsOrtho(Action & a)
{
	ACTION_ENABLED_IF( a, d_curOrtho != -1 &&
		d_orthos[ d_curOrtho ].d_tuples->getSel().size() == 1 &&
		d_orthos[ d_curOrtho ].d_viewer->hasFocus() );

	SpinPoint spin = *d_orthos[ d_curOrtho ].d_tuples->getSel().begin();
	DynValueEditor::edit( MW, 
		d_pro->getRepository()->findObjectDef( Repository::keySpin ), spin[ DimX ] );
}

void SystemScopeAgent2::handleCursorSync(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_cursorSync );
	
	d_cursorSync = !d_cursorSync;
	if( d_cursorSync )
		GlobalCursor::addObserver( this );	// TODO: preset Cursor
	else
		GlobalCursor::removeObserver( this );
}

void SystemScopeAgent2::handleNextSpec(Action & a)
{
	ACTION_ENABLED_IF( a, d_sort.size() > 1 ); 
	stepSpec( true );
}

void SystemScopeAgent2::handlePrevSpec(Action & a)
{
	ACTION_ENABLED_IF( a, d_sort.size() > 1 ); 
	stepSpec( false );
}

void SystemScopeAgent2::handleFitOrtho(Action & a)
{
	ACTION_ENABLED_IF( a, true ); 

	handleFitWindow( a );
	if( d_curOrtho != -1 &&	d_orthos[ d_curOrtho ].d_viewer->hasFocus() )
	{
		d_orthos[ d_curOrtho ].d_buf->fitToDim( DimX );
		d_orthos[ d_curOrtho ].d_viewer->redraw();
	}
}

void SystemScopeAgent2::handleAutoGain(Action & a)
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
	createContour();
	d_strip.d_viewer->redraw();
	updateOrthoContour();
}

void SystemScopeAgent2::handleShowWithOff(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_src->showOffs() );
	
	d_src->showOffs( !d_src->showOffs() );
}

void SystemScopeAgent2::handleShowLinks(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_src->showLinks() );
	
	d_src->showLinks( !d_src->showLinks() );
}

void SystemScopeAgent2::handleShowInfered(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_src->showInferred() );
	
	d_src->showInferred( !d_src->showInferred() );
}

void SystemScopeAgent2::handleShowUnlabeled(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_src->showNulls() );
	
	d_src->showNulls( !d_src->showNulls() );
}

void SystemScopeAgent2::handleShowUnknown(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_src->showUnknown() );
	
	d_src->showUnknown( !d_src->showUnknown() );
}

void SystemScopeAgent2::handleDoPathSim(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_src->doPathsim() );
	
	d_src->doPathsim( !d_src->doPathsim() );
}

void SystemScopeAgent2::handleShowGhosts(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_strip.d_mdl->showGhosts() );

	d_strip.d_mdl->showGhosts( !d_strip.d_mdl->showGhosts() );
	for( int i = 0; i < d_orthos.size(); i++ )
		d_orthos[ i ].d_mdl->showGhosts( d_strip.d_mdl->showGhosts() );
}

void SystemScopeAgent2::handleProposeSpin(Action & a)
{
	ACTION_ENABLED_IF( a, !d_anchor.isZero() && d_spec->isNoesy( DimY ) );

	Spin* orig = 0;
	if( d_spec->isNoesy( DimX, DimY ) )
		orig = d_anchor[ DimX ];
	else
		orig = d_anchor[ DimY ];

	ProposeSpinDlg dlg( MW, d_pro, d_spec->getColor( DimY ), d_cursor[ DimY ], 
		d_spec,	"Select Possible Partner" );
	dlg.setAnchor( DimX, d_anchor[ DimX ] );
	dlg.setAnchor( DimZ, d_anchor[ DimY ] );
	if( !dlg.exec() || dlg.getSpin() == 0 )
		return;

	if( orig->findLink( dlg.getSpin() ) == 0 ) // Ref == target zulssig wegen Diagonaler
	{
		Root::Ref<LinkSpinCmd> c1 = new LinkSpinCmd( d_pro->getSpins(), orig, dlg.getSpin() ); 
		if( c1->handle( this ) )
			d_strip.d_tuples->selectPeak( d_cursor[ DimX ], d_cursor[ DimY ] );
	}else
		Root::ReportToUser::alert( this, "Propose Spin", 
			"The selected spins are already linked!" );
}

void SystemScopeAgent2::handleProposeOrtho(Action & a)
{
	ACTION_ENABLED_IF( a, d_curOrtho != -1 &&
		d_orthos[ d_curOrtho ].d_viewer->hasFocus() && d_spec->isNoesy( DimZ ) );

	Spin* orig = 0;
	if( d_spec->isNoesy( DimX, DimZ ) )
		orig = d_orthos[ d_curOrtho ].d_tuple[ DimX ];
	else
		orig = d_orthos[ d_curOrtho ].d_tuple[ DimY ];

	ProposeSpinDlg dlg( MW, d_pro, d_spec->getColor( DimZ ), d_cursor[ DimZ ], 
		d_spec,	"Select Possible Partner" );
	dlg.setAnchor( DimX, d_orthos[ d_curOrtho ].d_tuple[ DimX ] );
	dlg.setAnchor( DimZ, d_orthos[ d_curOrtho ].d_tuple[ DimY ] );
	if( !dlg.exec() || dlg.getSpin() == 0 )
		return;

	if( orig->findLink( dlg.getSpin() ) == 0 ) // Ref == target zulssig wegen Diagonaler
	{
		Root::Ref<LinkSpinCmd> c1 = new LinkSpinCmd( d_pro->getSpins(), orig, dlg.getSpin() ); 
		if( c1->handle( this ) )
			d_orthos[ d_curOrtho ].d_tuples->selectPeak( d_cursor[ DimZ ], d_cursor[ DimY ] );
	}else
		Root::ReportToUser::alert( this, "Propose Spin", 
			"The selected spins are already linked!" );
}

void SystemScopeAgent2::handlePickLabel(Action & a)
{
	ACTION_ENABLED_IF( a, !d_anchor.isZero() );

	SpinLabelSet ly = d_spec->getType()->getLabels( d_spec->mapToType( DimY ) );
	NmrExperiment* e = d_pro->getRepository()->getTypes()->
		inferExperiment2( d_spec->getType(), d_anchor[0]->getSystem(), d_spec );
	if( e )
		e->getColumn( d_spec->mapToType( DimY ), ly );
	if( d_anchor[0]->getSystem() )
		ly = d_anchor[0]->getSystem()->getAcceptables( ly );
	SpinLabel y;
	if( a.getParamCount() == 0 || a.getParam( 0 ).isNull() || 
		!SpinLabel::parse( a.getParam( 0 ).getCStr(), y ) )
		if( !Dlg::getLabel( MW, y, ly ) )
			return;
	if( !d_anchor[0]->getSystem()->isAcceptable( y ) )
	{
		Root::ReportToUser::alert( this, "Pick Label", "Label is not acceptable" );
		return;
	}

	Root::Ref<PickSystemSpinLabelCmd> cmd = new PickSystemSpinLabelCmd( d_pro->getSpins(), 
		d_anchor[0]->getSystem(), d_spec->getColor( DimY ), d_cursor[ DimY ], y, 0 ); 
	cmd->handle( this );

	if( y.isNull() )
		d_src->showNulls( true );
	d_strip.d_tuples->selectPeak( d_anchorX, d_cursor[ DimY ] );
}

void SystemScopeAgent2::handlePickLabelOrtho(Action & a)
{
	ACTION_ENABLED_IF( a, d_curOrtho != -1 &&
		d_orthos[ d_curOrtho ].d_viewer->hasFocus() );

	SpinLabelSet ly = d_spec->getType()->getLabels( d_spec->mapToType( DimZ ) );
	NmrExperiment* e = d_pro->getRepository()->getTypes()->
		inferExperiment2( d_spec->getType(), d_anchor[0]->getSystem(), d_spec );
	if( e )
		e->getColumn( d_spec->mapToType( DimZ ), ly );
	if( d_anchor[0]->getSystem() )
		ly = d_anchor[0]->getSystem()->getAcceptables( ly );
	SpinLabel y;
	if( a.getParam( 0 ).isNull() || !SpinLabel::parse( a.getParam( 0 ).getCStr(), y ) )
		if( !Dlg::getLabel( MW, y, ly ) )
			return;
	if( !d_anchor[0]->getSystem()->isAcceptable( y ) )
	{
		Root::ReportToUser::alert( this, "Pick Label", "Label is not acceptable" );
		return;
	}

	Root::Ref<PickSystemSpinLabelCmd> cmd = new PickSystemSpinLabelCmd( d_pro->getSpins(), 
		d_anchor[0]->getSystem(), d_spec->getColor( DimZ ), d_cursor[ DimZ ], y, 0 ); 
	cmd->handle( this );
	
	if( y.isNull() )
		d_src->showNulls( true );
	d_orthos[ d_curOrtho ].d_tuples->selectPeak( d_cursor[ DimZ ], d_cursor[ DimY ] );
}

void SystemScopeAgent2::handleViewLabels(Action & a)
{
	if( a.getParamCount() == 0 )
		return;

	SpinPointView::Label q = (SpinPointView::Label) a.getParam( 0 ).getShort();
	if( q < SpinPointView::None || q >= SpinPointView::End )
		return;

	ACTION_CHECKED_IF( a, true,
		d_strip.d_tuples->getLabel() == q );
	
	d_strip.d_tuples->setLabel( q, DimY );
	d_strip.d_viewer->redraw();
}

void SystemScopeAgent2::handleSetDepth(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	float g;
	if( a.getParam( 0 ).isNull() )
	{
		bool ok	= FALSE;
		QString res;
		res.sprintf( "%f", d_pro->inferPeakWidth( DimZ, d_spec ) );
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
	d_pro->setPeakWidth( DimZ, g, d_spec );
}

void SystemScopeAgent2::handleGhostLabels(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_strip.d_tuples->ghostLabel() );

	d_strip.d_tuples->ghostLabel( !d_strip.d_tuples->ghostLabel() );
	for( int i = 0; i < d_orthos.size(); i++ )
		d_orthos[i].d_tuples->ghostLabel( d_strip.d_tuples->ghostLabel() );
}

void SystemScopeAgent2::handleViewLabels2(Action & a)
{
	if( a.getParamCount() == 0 )
		return;

	SpinPointView::Label q = (SpinPointView::Label) a.getParam( 0 ).getShort();
	if( q < SpinPointView::None || q >= SpinPointView::End )
		return;

	ACTION_CHECKED_IF( a, true, d_ol == q );
	
	d_ol = q;
	for( int i = 0; i < d_orthos.size(); i++ )
	{
		d_orthos[ i ].d_tuples->setLabel( q, DimX );
		d_orthos[ i ].d_viewer->redraw();
	}
}

void SystemScopeAgent2::handlePickBounds(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	try
	{
		PpmPoint p( d_anchorX, d_cursor[ DimY ], d_anchorZ );
		Amplitude val = d_spec->getAt( p, d_folding, d_folding );
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
		d_slice.d_slice->setMinMax( _min, _max );	
		d_slice.d_sliceOrtho->setMinMax( _min, _max );	
		d_slice.d_viewer->redraw();
	}catch( ... )
	{
	}
}

void SystemScopeAgent2::handleSetBounds(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	
	Amplitude _min, _max;
	_min = d_slice.d_slice->getMin();
	_max = d_slice.d_slice->getMax();
	if( Dlg::getBounds( MW, _min, _max ) )
	{
		d_slice.d_slice->setMinMax( _min, _max );	
		d_slice.d_sliceOrtho->setMinMax( _min, _max );	
		d_slice.d_viewer->redraw();
	}
}

void SystemScopeAgent2::handleAutoSlice(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_slice.d_slice->isAutoScale() );

	bool on = !d_slice.d_slice->isAutoScale();
	d_slice.d_slice->setAutoScale( on );	
	d_slice.d_sliceOrtho->setAutoScale( on );	
	d_slice.d_viewer->redraw();
}

void SystemScopeAgent2::handlePickBoundsSym(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	try
	{
		PpmPoint p( d_cursor[ DimX ], d_cursor[ DimY ], d_anchorZ );
		Amplitude val = d_spec->getAt( p, d_folding, d_folding );
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
		d_slice.d_slice->setMinMax( _min, _max );	
		d_slice.d_sliceOrtho->setMinMax( _min, _max );	
		d_slice.d_viewer->redraw();
	}catch( ... )
	{
	}
}

void SystemScopeAgent2::handleDeleteLinks(Action & a)
{
	ACTION_ENABLED_IF( a, !d_anchor.isZero() &&
		!d_strip.d_tuples->getSel().empty() &&
		d_strip.d_viewer->hasFocus() );

	SpinPointView::Selection sel = d_strip.d_tuples->getSel();

	SpinPointView::Selection::const_iterator p;
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Delete Links" );
	SpinLink* l;
	std::set<SpinLink*> test;
	for( p = sel.begin(); p != sel.end(); ++p )
	{
		l = (*p)[ DimX ]->findLink( (*p)[ DimY ] );
		if( l && test.count( l ) == 0 )
		{
			cmd->add( new UnlinkSpinCmd( d_pro->getSpins(), (*p)[ DimX ], (*p)[ DimY ] ) );
			test.insert( l );
		}
	}
	cmd->handle( this );
}

void SystemScopeAgent2::handleShowAllPeaks(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_mdl->showAll() );
	d_mdl->showAll( !d_mdl->showAll() );
}

void SystemScopeAgent2::handleHidePeak(Action & a)
{
	if( d_strip.d_tuples->getSel().size() != 1 )
		return;
	SpinPoint tuple = *d_strip.d_tuples->getSel().begin();
	SpinLink* link = tuple[ DimX ]->findLink( tuple[ DimY ] );
	ACTION_ENABLED_IF( a, link );
	Root::Ref<HideSpinLinkCmd> cmd = new HideSpinLinkCmd( d_pro->getSpins(), 
		link, d_spec );
	cmd->handle( this );
	// TODO: Plural
}

void SystemScopeAgent2::handleEditAttsSys(Action & a)
{
	ACTION_ENABLED_IF( a, !d_anchor.isZero() &&
		d_strip.d_tuples->getSel().size() == 1 &&
		d_strip.d_viewer->hasFocus() );

	SpinPoint tuple = *d_strip.d_tuples->getSel().begin();
	DynValueEditor::edit( MW, 
		d_pro->getRepository()->findObjectDef( Repository::keySpinSystem ), 
		tuple[ DimY ]->getSystem() );
}

void SystemScopeAgent2::handleEditAttsLink(Action & a)
{
	if( d_anchor.isZero() ||
		d_strip.d_tuples->getSel().size() != 1 ||
		!d_strip.d_viewer->hasFocus() )
		return;

	SpinPoint tuple = *d_strip.d_tuples->getSel().begin();
	SpinLink* l = tuple[ DimX ]->findLink( tuple[ DimY ] );
	ACTION_ENABLED_IF( a, l );

	DynValueEditor::edit( MW, 
		d_pro->getRepository()->findObjectDef( Repository::keyLink ), l );
}

void SystemScopeAgent2::handleFitOrthoX(Action & a)
{
	ACTION_ENABLED_IF( a, d_curOrtho != -1 &&
		d_orthos[ d_curOrtho ].d_viewer->hasFocus() ); 

	d_orthos[ d_curOrtho ].d_buf->fitToDim( DimX );
	d_orthos[ d_curOrtho ].d_viewer->redraw();
}



