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

#include "StripListGadget.h"
#include <Gui/ListView.h> 
#include <QPixmap>
#include <stdio.h>
#include <QInputDialog> 
#include <QMessageBox>
#include <QPainter>
#include <QItemDelegate>
#include <QApplication>
#include <QtDebug>

#include <Root/MakroCommand.h>
#include <Root/Message.h>

#include <Lexi/Handler.h>
#include <Lexi/LexiListView.h>
#include <Gui/Menu.h>

#include <Spec/Stripper.h>
#include <Spec/Repository.h>

#include <SpecView/DynValueEditor.h>
#include <SpecView/CandidateDlg.h>
#include <SingleAlignmentView.h>
#include <SpecView/ObjectListView.h>
#include <Dlg.h>
#include "LuaCaraExplorer.h"
using namespace Spec;
using namespace Lexi;
using Root::Index;

//////////////////////////////////////////////////////////////////////

static const char*  c_strip[] = {
"22 17 2 1",
" 	c None",
".	c #000000",
"                      ",
"                      ",
"      .............   ",
"      .           .   ",
"      .           .   ",
"      .           .   ",
"  ..  .       ..  .   ",
" .  ...      .  ...   ",
" .           .        ",
" .  ...      .  ...   ",
"  ..  .       ..  .   ",
"      .           .   ",
"      .           .   ",
"      .           .   ",
"      .............   ",
"                      ",
"                      "};
static const char*  c_peak[] = {
"22 17 2 1",
" 	c None",
".	c #000000",
"                      ",
"                      ",
"    ............      ",
"    .          .      ",
"    .          .      ",
"    .          .      ",
"    .          .      ",
"    .          .      ",
"    ............      ",
"    .                 ",
"    .                 ",
"    .                 ",
"    .                 ",
"    .                 ",
"    .                 ",
"                      ",
"                      "};
static const char*  c_left[] = {
"22 17 2 1",
" 	c None",
".	c #000000",
"                      ",
"                      ",
" . . ....             ",
"        .             ",
"        .             ",
"        .             ",
"    ..  .             ",
"   .  ...             ",
"   .                  ",
"   .  ...             ",
"    ..  .             ",
"        .             ",
"        .             ",
"        .             ",
" . . ....             ",
"                      ",
"                      "};
static const char*  c_right[] = {
"22 17 2 1",
" 	c None",
".	c #000000",
"                      ",
"                      ",
"              .... .  ",
"              .       ",
"              .       ",
"              .       ",
"          ..  .       ",
"         .  ...       ",
"         .            ",
"         .  ...       ",
"          ..  .       ",
"              .       ",
"              .       ",
"              .       ",
"              .... .  ",
"                      ",
"                      "};
static const char*  c_link[] = {
"22 17 2 1",
" 	c None",
".	c #000000",
"                      ",
"                      ",
"                      ",
"                      ",
"                      ",
"                      ",
"  ...          ...    ",
" .   .        .   .   ",
" .   ..........   .   ",
" .	  .        .   .   ",
"  ...          ...    ",
"                      ",
"                      ",
"                      ",
"                      ",
"                      ",
"                      "};

static char g_buffer[32];
static PeakColorList* s_cl; // RISK Trick um Speicher in LinkItem zu sparen
static const int s_off = 10000; // Wegen Negativsortierung

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr StripListGadget::ShowLinks = "ShowLinks";
Root::Action::CmdStr StripListGadget::CreateLink = "CreateLink";
Root::Action::CmdStr StripListGadget::EatSystem = "EatSystem";
Root::Action::CmdStr StripListGadget::CreateSystem = "CreateSystem";
Root::Action::CmdStr StripListGadget::ForceLabel = "ForceLabel";
Root::Action::CmdStr StripListGadget::CreateSpin = "CreateSpin";
Root::Action::CmdStr StripListGadget::LabelSpin = "LabelSpin";
Root::Action::CmdStr StripListGadget::MoveSpin = "MoveSpin";
Root::Action::CmdStr StripListGadget::MoveSpinAlias = "MoveSpinAlias";
Root::Action::CmdStr StripListGadget::SetSysType = "SetSysType";
Root::Action::CmdStr StripListGadget::Delete = "Delete";
Root::Action::CmdStr StripListGadget::LinkThis = "LinkThis";
Root::Action::CmdStr StripListGadget::ShowAlignment = "ShowAlignment";
Root::Action::CmdStr StripListGadget::UnlinkSucc = "UnlinkSucc";
Root::Action::CmdStr StripListGadget::UnlinkPred = "UnlinkPred";
Root::Action::CmdStr StripListGadget::Assign = "Assign";
Root::Action::CmdStr StripListGadget::Unassign = "Unassign";
Root::Action::CmdStr StripListGadget::RunStripper = "RunStripper";
Root::Action::CmdStr StripListGadget::StrictStripMatch = "StrictStripMatch";
Root::Action::CmdStr StripListGadget::UnlabeledStripMatch = "UnlabeledStripMatch";
Root::Action::CmdStr StripListGadget::SetSpinTol = "SetSpinTol";
Root::Action::CmdStr StripListGadget::SetCandidates = "SetCandidates";
Root::Action::CmdStr StripListGadget::EditAtts = "EditAtts";
Root::Action::CmdStr StripListGadget::GotoOther = "GotoOther";
Root::Action::CmdStr StripListGadget::FindSpin = "FindSpin";
Root::Action::CmdStr StripListGadget::AcceptLabel = "AcceptLabel";
Root::Action::CmdStr StripListGadget::ShowTable = "ShowTable";
Root::Action::CmdStr StripListGadget::OpenAll = "OpenAll";
Root::Action::CmdStr StripListGadget::CloseAll = "CloseAll";
Root::Action::CmdStr StripListGadget::LinkParams = "LinkParams";

ACTION_SLOTS_BEGIN( StripListGadget )
    { StripListGadget::LinkParams, &StripListGadget::handleLinkParams },
    { StripListGadget::OpenAll, &StripListGadget::handleOpenAll },
    { StripListGadget::CloseAll, &StripListGadget::handleCloseAll },
    { StripListGadget::ShowTable, &StripListGadget::handleShowTable },
    { StripListGadget::AcceptLabel, &StripListGadget::handleAcceptLabel },
    { StripListGadget::FindSpin, &StripListGadget::handleFindSpin },
    { StripListGadget::GotoOther, &StripListGadget::handleGotoOther },
    { StripListGadget::ShowLinks, &StripListGadget::handleShowLinks },
    { StripListGadget::CreateLink, &StripListGadget::handleCreateLink },
    { StripListGadget::EatSystem, &StripListGadget::handleEatSystem },
    { StripListGadget::CreateSystem, &StripListGadget::handleCreateSystem },
    { StripListGadget::ForceLabel, &StripListGadget::handleForceLabel },
    { StripListGadget::CreateSpin, &StripListGadget::handleCreateSpin },
    { StripListGadget::LabelSpin, &StripListGadget::handleLabelSpin },
    { StripListGadget::MoveSpin, &StripListGadget::handleMoveSpin },
    { StripListGadget::MoveSpinAlias, &StripListGadget::handleMoveSpinAlias },
    { StripListGadget::SetSysType, &StripListGadget::handleSetSysType },
    { StripListGadget::Delete, &StripListGadget::handleDelete },
    { StripListGadget::LinkThis, &StripListGadget::handleLinkThis },
    { StripListGadget::ShowAlignment, &StripListGadget::handleShowAlignment },
    { StripListGadget::UnlinkSucc, &StripListGadget::handleUnlinkSucc },
    { StripListGadget::UnlinkPred, &StripListGadget::handleUnlinkPred },
    { StripListGadget::Assign, &StripListGadget::handleAssign },
    { StripListGadget::Unassign, &StripListGadget::handleUnassign },
    { StripListGadget::RunStripper, &StripListGadget::handleRunStripper },
    { StripListGadget::UnlabeledStripMatch, &StripListGadget::handleUnlabeledStripMatch },
    { StripListGadget::StrictStripMatch, &StripListGadget::handleStrictStripMatch },
    { StripListGadget::SetSpinTol, &StripListGadget::handleSetSpinTol },
    { StripListGadget::SetCandidates, &StripListGadget::handleSetCandidates },
    { StripListGadget::EditAtts, &StripListGadget::handleEditAtts },
ACTION_SLOTS_END( StripListGadget )

//////////////////////////////////////////////////////////////////////

struct _SystemItem;
struct _PeakLabelItem;
struct _SpinLinkItem;
struct _StripLinkItem;

struct _StripListView : public Lexi::ListView
{
	Root::Ref<Project> d_pro;

	Root::Ptr<Root::Agent> d_main;
	Root::Ref<Spectrum> d_spec;
	bool d_showLinks;
	
    QHash<SpinSystem*, _SystemItem*> d_map;

	void handle(Root::Message& msg );
	void handleReturn(Gui::ListViewItem * i);
	void handleDblClick(Gui::ListViewItem * i);
	void handleSelection();

	void loadAllStrips();
    void updateItem( _SystemItem * s, bool repaint = true );
    _PeakLabelItem* createSpinView(_SystemItem * s, Spin* spin);
    _SystemItem* createSysView( Gui::ListView* v, SpinSystem* sys );
	_SpinLinkItem* createLinkView( _PeakLabelItem * p, SpinLink* l);
	SpinSystem* getSys() const;
	Spin* getLink() const;
    void clearMatches( _SystemItem* s );
    void fillMatches( _SystemItem* s );

	_StripListView(Lexi::Viewport* vp, Project* pro, Root::Agent* main, bool links );
	~_StripListView();
};

//////////////////////////////////////////////////////////////////////

struct _SpinLinkItem : public Gui::ListViewItem
{
	Root::ExRef<Spin> d_spin;
	Root::ExRef<SpinLink> d_link;

	_SpinLinkItem( _PeakLabelItem * parent, Spin* s, SpinLink* l );
	QString text( int f ) const;
	QString key( int f, bool ) const;
	QVariant pixmap( int i ) const;
};

struct _StripLinkItem : public Gui::ListViewItem
{
	_StripLinkItem( Gui::ListViewItem * parent, SpinSystem* sys, Fitness f ):
		Gui::ListViewItem( parent ), d_sys(sys), d_fit( f ) {}
	SpinSystem* d_sys;
	Fitness d_fit;

	QString text( int f ) const
	{
		switch( f )
		{
		case 0:
			::sprintf( g_buffer, "%d", d_sys->getId() );
			return g_buffer;
		case 2:
			::sprintf( g_buffer, "%.2f", ::fabs( d_fit ) );
			return g_buffer;
		}
		return "";
	}
	QString key( int f, bool ) const
	{
		switch( f )
		{
		case 0:
			::sprintf( g_buffer, "%d%06d", (d_fit>0.0)?1:0, d_sys->getId() );
			return g_buffer;
		case 2:
			::sprintf( g_buffer, "%06.2f", ::fabs( d_fit ) );
			return g_buffer;
		}
		return "";
	}
	QVariant pixmap( int i ) const
	{
		if( i != 0 )
			return QVariant();
		if( d_fit > 0.0 )
			return QPixmap( c_right ); // Item ist potentieller Successor
		else
			return QPixmap( c_left );  // Item ist potentieller Predecessor
	}
};

struct _PeakLabelItem : public Gui::ListViewItem
{
	_PeakLabelItem( Gui::ListViewItem * parent, _StripListView* l, Spin* s ):
		Gui::ListViewItem( parent ), d_spin( s ), d_list( l ) {}
	Root::ExRef<Spin> d_spin;
	_StripListView* d_list;

	QString text( int f ) const
	{
		switch( f )
		{
		case 0:
			d_spin->getLabel().format( g_buffer, sizeof( g_buffer ) );
			return g_buffer;
		case 1:
			::sprintf( g_buffer, "%.3f %s", d_spin->getShift( d_list->d_spec ),
				d_spin->getAtom().getIsoLabel() );
			return g_buffer;
		case 8:
			::sprintf( g_buffer, "%d", d_spin->getId() );
			return g_buffer;
		}
		return "";
	}
	QString key( int f, bool ) const
	{
		double shift;
		switch( f )
		{
		case 0:
			d_spin->getLabel().format( g_buffer, sizeof( g_buffer ) );
			return g_buffer;
		case 1:
			shift = d_spin->getShift( d_list->d_spec );
			if( shift < 0.0 )
				::sprintf( g_buffer, "A%+08.3f", shift );
			else
				::sprintf( g_buffer, "B%+08.3f", shift );
			return g_buffer;
		case 8:
			::sprintf( g_buffer, "%08d", d_spin->getId() );
			return g_buffer;
		}
		return "";
	}
	QVariant pixmap( int i ) const
	{
		if( i != 0 )
			return QVariant();
		return QPixmap( c_peak );
	}

	_SpinLinkItem* findLink( SpinLink* l ) const
	{
		_SpinLinkItem* item = 0;
		for( int i = 0; i < count(); i++ )
		{
			item = static_cast<_SpinLinkItem*>( child( i ) );
			if( item && item->d_link == l )
				return item;
		}
		return 0;
	}
	_SpinLinkItem* findLink( Spin* other ) const
	{
		_SpinLinkItem* item = 0;
		for( int i = 0; i < count(); i++ )
		{
			item = static_cast<_SpinLinkItem*>( child( i ) );
			if( item && item->d_spin == other )
				return item;
		}
		return 0;
	}
};

struct _SystemItem : public Gui::ListViewItem
{
    _SystemItem( Gui::ListView * parent, SpinSystem* sys ):
		Gui::ListViewItem( parent ), d_sys( sys ), d_excl( false ) 
		{ assert( sys ); }
	Root::ExRef<SpinSystem> d_sys;
	bool d_excl;

	QString text( int f ) const
	{
		switch( f )
		{
		case 0:
			::sprintf( g_buffer, "%d", d_sys->getId() );
			return g_buffer;
		case 3:
			if( d_sys->getPred() != 0 )
				::sprintf( g_buffer, "%d", d_sys->getPred()->getId() );
			else
				g_buffer[ 0 ] = 0;
			return g_buffer;
		case 4:
			if( d_sys->getSucc() != 0 )
				::sprintf( g_buffer, "%d", d_sys->getSucc()->getId() );
			else
				g_buffer[ 0 ] = 0;
			return g_buffer;
		case 5:
			if( d_sys->getAssig() != 0 )
			{
				d_sys->formatLabel( g_buffer, sizeof(g_buffer) );
			}else if( d_excl )
			{
				g_buffer[ 0 ] = '*';
				g_buffer[ 1 ] = 0;
			}else
				g_buffer[ 0 ] = 0;
			return g_buffer;
		case 6:
			d_sys->formatCands( g_buffer, sizeof( g_buffer ) );
			return g_buffer;
		case 7:
			if( d_sys->getSysType() )
				return d_sys->getSysType()->getName().data();
			else
				return "";
		}
		return "";
	}
	QString key( int f, bool ) const
	{
		switch( f )
		{
		case 0:
			::sprintf( g_buffer, "%06d", d_sys->getId() );
			return g_buffer;
		case 3:
			if( d_sys->getPred() != 0 )
				::sprintf( g_buffer, "%06d", d_sys->getPred()->getId() );
			else
				g_buffer[ 0 ] = 0;
			return g_buffer;
		case 4:
			if( d_sys->getSucc() != 0 )
				::sprintf( g_buffer, "%06d", d_sys->getSucc()->getId() );
			else
				g_buffer[ 0 ] = 0;
			return g_buffer;
		case 5:
			if( d_sys->getAssig() != 0 )
			{
				::sprintf( g_buffer, "%s%09d", d_sys->getAssig()->getChain().data(),
					d_sys->getAssig()->getNr() + s_off );
				return g_buffer;
			}else
				return "";
		default:
			return text( f );
		}
		return "";
	}
	QVariant pixmap( int i ) const
	{
		if( i != 0 )
			return QVariant();
		return QPixmap( c_strip );
	}

	_PeakLabelItem* findSpin( Spin* s ) const
	{
		_PeakLabelItem* item = 0;
		for( int i = 0; i < count(); i++ )
		{
			item = dynamic_cast<_PeakLabelItem*>( child( i ) );
			if( item && item->d_spin == s )
				return item;
		}
		return 0;
	}
	_SpinLinkItem* findLink( Spin* s, SpinLink* l ) const
	{
		_PeakLabelItem* i = findSpin( s );
		if( i )
			return i->findLink( l  );
		else
			return 0;
	}
	_SpinLinkItem* findLink( Spin* spin, Spin* other ) const
	{
		_PeakLabelItem* i = findSpin( spin );
		if( i )
			return i->findLink( other  );
		else
			return 0;
	}
	_StripLinkItem* findStripLink( SpinSystem* sys ) const 
	{
		_StripLinkItem* item;
		for( int i = 0; i < count(); i++ )
		{
			item = dynamic_cast<_StripLinkItem*>( child( i ) );
			if( item && item->d_sys == sys )
				return item;
		}
		return 0;
	}
};

_SpinLinkItem::_SpinLinkItem( _PeakLabelItem * parent, Spin* s, SpinLink* l ):
		Gui::ListViewItem( parent ), d_spin( s ), d_link( l ) {}

QString _SpinLinkItem::text( int f ) const
{
	_PeakLabelItem* _this = dynamic_cast<_PeakLabelItem*>( parent() );
	assert( _this );
	const SpinLink::Alias& alias = d_link->getAlias( _this->d_list->d_spec );
	switch( f )
	{
	case 0:
		d_spin->getLabel().format( g_buffer, sizeof( g_buffer ) );
		return g_buffer;
	case 1:
		::sprintf( g_buffer, "%.3f %s", d_spin->getShift( _this->d_list->d_spec ),
			d_spin->getAtom().getIsoLabel() );
		return g_buffer;
	case 2:
		if( alias.d_visible )
			::sprintf( g_buffer, "%d/%.1f", int( alias.d_code ), alias.d_rating );
		else
			::sprintf( g_buffer, "-" );
		return g_buffer;
	case 5:
		if( d_spin->getSystem() )
			d_spin->getSystem()->formatLabel( g_buffer, sizeof(g_buffer) );
		else
			g_buffer[ 0 ] = 0;
		return g_buffer;
	case 8:
		::sprintf( g_buffer, "%d", d_spin->getId() );
		return g_buffer;
	}
	return "";
}
QString _SpinLinkItem::key( int f, bool ) const
{
	double shift;
	_PeakLabelItem* _this = dynamic_cast<_PeakLabelItem*>( parent() );
	assert( _this );
	const SpinLink::Alias& alias = d_link->getAlias( _this->d_list->d_spec );
	switch( f )
	{
	case 0:
		d_spin->getLabel().format( g_buffer, sizeof( g_buffer ) );
		return g_buffer;
	case 1:
		shift = d_spin->getShift( _this->d_list->d_spec );
		if( shift < 0.0 )
			::sprintf( g_buffer, "A%+08.3f", shift );
		else
			::sprintf( g_buffer, "B%+08.3f", shift );
		return g_buffer;
	case 2:
		if( alias.d_visible )
			::sprintf( g_buffer, "%03d %08.1f", int( alias.d_code ), alias.d_rating );
		else
			::sprintf( g_buffer, "-" );
		return g_buffer;
	case 5:
		if( d_spin->getSystem() )
		{
			if( d_spin->getSystem()->getAssig() != 0 )
				::sprintf( g_buffer, "_%s%09d", 
					d_spin->getSystem()->getAssig()->getChain().data(),
					d_spin->getSystem()->getAssig()->getNr() + s_off );
			else
				::sprintf( g_buffer, "%09d", d_spin->getSystem()->getId() );
		}else
			g_buffer[ 0 ] = 0;
		return g_buffer;
	case 8:
		::sprintf( g_buffer, "%06d", d_spin->getId() );
		return g_buffer;
	default:
		return "";
	}
	return "";
}
QVariant _SpinLinkItem::pixmap( int i ) const
{
	if( i != 0 )
		return QVariant();
	return QPixmap( c_link );
}

////////////////////////////////////////////////////////////////////////////////

void _StripListView::handleReturn(Gui::ListViewItem * i)
{
	if( i && d_main )
	{
		StripListGadget::ActivateMsg msg;
		d_main->traverse( msg );
	}
}

void _StripListView::handleDblClick(Gui::ListViewItem * i)
{
	if( i && d_main )
	{
		StripListGadget::ActivateMsg msg;
		d_main->traverse( msg );
	}
}
	
_StripListView::_StripListView(Lexi::Viewport* vp, Project* pro,
		Root::Agent* main, bool links ):
		Lexi::ListView( vp->getWindow() ),
		d_main( main ), d_pro( pro ), d_showLinks( links )
{
	if( main == 0 )
		d_main = vp;
	s_cl = d_pro->getRepository()->getColors();
	d_pro->getStripper()->addObserver( this );
	d_pro->addObserver( this );
	getImp()->addColumn( "System" );
	getImp()->addColumn( "Shift" );
	getImp()->addColumn( "Fit" );
	getImp()->addColumn( "Pred" );
	getImp()->addColumn( "Succ" );
	getImp()->addColumn( "Ass" );
	getImp()->addColumn( "Cand." );
	getImp()->addColumn( "Type" );
	getImp()->addColumn( "Spin" );
	loadAllStrips();
    for( int col = 2; col < getImp()->columns(); col++ )
        getImp()->resizeColumnToContents(col);
}

_StripListView::~_StripListView()
{
	d_pro->removeObserver( this );
	d_pro->getStripper()->removeObserver( this );
}

void _StripListView::handleSelection()
{
    if( _SystemItem* i = dynamic_cast<_SystemItem*>( getImp()->currentItem() ) )
	{
		LuaCaraExplorer::setCurrentSystem( i->d_sys );
	}else if( _SpinLinkItem* i = dynamic_cast<_SpinLinkItem*>( getImp()->currentItem() ) )
	{
		LuaCaraExplorer::setCurrentSpinLink( i->d_link );
	}else if( _StripLinkItem* i = dynamic_cast<_StripLinkItem*>( getImp()->currentItem() ) )
	{
		LuaCaraExplorer::setCurrentSystem( i->d_sys );
	}else if( _PeakLabelItem* i = dynamic_cast<_PeakLabelItem*>( getImp()->currentItem() ) )
	{
		LuaCaraExplorer::setCurrentSpin( i->d_spin );
	}
}

//////////////////////////////////////////////////////////////////////

SpinSystem* _StripListView::getSys() const
{
    _SystemItem* i = dynamic_cast<_SystemItem*>( getImp()->currentItem() );
	if( i != nil )
		return i->d_sys;
	else
		return 0;
}
Spin* _StripListView::getLink() const
{
	_SpinLinkItem* i = dynamic_cast<_SpinLinkItem*>( getImp()->currentItem() );
	if( i != nil )
		return i->d_spin;
	else
		return 0;
}

void _StripListView::loadAllStrips()
{
	getImp()->clear();
	d_map.clear();
	const SpinBase::SpinSystemMap& strips = d_pro->getSpins()->getSystems();
	SpinBase::SpinSystemMap::const_iterator idx;
	for( idx = strips.begin(); idx != strips.end(); ++idx )
		createSysView( getImp(), (*idx).second );
}

_SpinLinkItem* _StripListView::createLinkView(_PeakLabelItem * p, SpinLink* l)
{
	Spin* other;
	if( l->getLhs() == p->d_spin->getId() )
		other = d_pro->getSpins()->getSpin( l->getRhs() );
	else
		other = d_pro->getSpins()->getSpin( l->getLhs() );
	if( other )
	{
		return new _SpinLinkItem( p, other, l );
	}else
		return 0;
}

_PeakLabelItem* _StripListView::createSpinView(_SystemItem * s, Spin* spin)
{
	_PeakLabelItem* p = new _PeakLabelItem( s, this, spin );
	if( d_showLinks )
	{
		Spin::Links::const_iterator p1;
		for( p1 = spin->getLinks().begin(); p1 != spin->getLinks().end(); ++p1 )
			createLinkView( p, (*p1) );
	}
	return p;
}

_SystemItem* _StripListView::createSysView( Gui::ListView* v, SpinSystem* sys )
{
    _SystemItem* s = new _SystemItem( v, sys );
	d_map[ s->d_sys ] = s;

	const SpinSystem::Spins& spins = s->d_sys->getSpins();
	SpinSystem::Spins::const_iterator idx;

	if( d_pro->getStripper()->isOn() )
        fillMatches( s );

	for( idx = spins.begin(); idx != spins.end(); ++idx )
		createSpinView( s, (*idx) );
	s->repaint();
	return s;
}

void _StripListView::handle(Root::Message & msg )
{
	BEGIN_HANDLER();
	MESSAGE( SpinLink::Update, a, msg )
	{
		switch( a->getType() )
		{
		case SpinLink::Update::Link:
			if( d_showLinks && a->getRhs()->getSystem() )
			{
                _SystemItem* s = d_map.value( a->getRhs()->getSystem() );
				assert( s != 0 );
				createLinkView( s->findSpin( a->getRhs() ), a->getLink() );
			}
			if( d_showLinks && a->getLhs()->getSystem() )
			{
                _SystemItem* s = d_map.value( a->getLhs()->getSystem() );
				assert( s != 0 );
				createLinkView( s->findSpin( a->getLhs() ), a->getLink() );
			}
			break;
		case SpinLink::Update::Unlink:
			if( d_showLinks && a->getRhs()->getSystem() )
			{
                _SystemItem* s = d_map.value( a->getRhs()->getSystem() );
				assert( s != 0 );
				_SpinLinkItem* l = s->findLink( a->getRhs(), a->getLhs() ); // getLink liefert hier 0
				assert( l != 0 );
				l->removeMe();
			}
			if( d_showLinks && a->getLhs()->getSystem() )
			{
                _SystemItem* s = d_map.value( a->getLhs()->getSystem() );
				assert( s != 0 );
				_SpinLinkItem* l = s->findLink( a->getLhs(), a->getRhs() ); // dito
				assert( l != 0 );
				l->removeMe();
			}
			break;
		case SpinLink::Update::All:
			if( d_showLinks )
				loadAllStrips();
			break;
        default:
            break;
		}
		msg.consume();
	}
	MESSAGE( Spin::Update, a, msg )
	{
		if( a->getSpin()->getSystem() )
		{
			switch( a->getType() )
			{
			case Spin::Update::Shift:
			case Spin::Update::Label:
				{
					Gui::ListViewItem* i = d_map[ a->getSpin()->getSystem() ];
					_PeakLabelItem* p;
					for( int j = 0; i && j < i->count(); j++ )
					{
						p = dynamic_cast<_PeakLabelItem*>( i->child( j ) );
						if( p && p->d_spin == a->getSpin() )
						{
							p->repaint();
							break;
						}
					}
				}
				break;
            default:
                break;
			}
		}
		msg.consume();
	}
	MESSAGE( SpinSystem::Update, a, msg )
	{
		switch( a->getType() )
		{
			// Ignore Link/Unlink
		case SpinSystem::Update::Pred:
		case SpinSystem::Update::Succ:
		case SpinSystem::Update::Assig:
		case SpinSystem::Update::Candidates:
			updateItem( d_map[ a->getSystem() ] );
			break;
		case SpinSystem::Update::Create:
			createSysView( getImp(), a->getSystem() );
			break;
		case SpinSystem::Update::Delete:
			d_map[ a->getSystem() ]->removeMe();
			d_map.remove( a->getSystem() );
			break;
		case SpinSystem::Update::Add:
			createSpinView( d_map[ a->getSystem() ], a->getSpin() );
			break;
		case SpinSystem::Update::Remove:
			{
                _SystemItem* s = d_map.value( a->getSystem() );
				if( s )
					s->findSpin( a->getSpin() )->removeMe();
			}
			break;
		case SpinSystem::Update::All:
			loadAllStrips();
			break;
        default:
            break;
		}
		msg.consume();
	}
	MESSAGE( Stripper::Update, a, msg )
	{
		msg.consume();
		switch( a->getType() ) 
		{
		case Stripper::Update::Match:
			{
                _SystemItem* s = d_map.value( a->getSys() );
				if( s )
				{
//                    clearMatches(s);
//                    fillMatches(s);
                    _StripLinkItem* l = s->findStripLink( a->getOther() );
                    if( l )
                        l->d_fit = a->getFit();
                    else
                    {
                        new _StripLinkItem( s, a->getOther(), a->getFit() );
                        getImp()->commit();
                        // Da sonst bei unmittelbarem Unmatch der Link noch nicht im parent ist
                    }
				}
			}
			break;
		case Stripper::Update::Unmatch:
			{
                _SystemItem* s = d_map.value( a->getSys() );
				if( s )
				{
//                    clearMatches(s);
//                    fillMatches(s);
                    _StripLinkItem* l = s->findStripLink( a->getOther() );
                   //assert( l != 0 );
                    if( l != 0 )
                        l->removeMe();
                    else
                        // TODO: das passiert wenn man Spins lscht und mit Undo wieder einfgt
                        qDebug() << "_StripListView::handle Stripper::Update::Unmatch Sys" << a->getSys()->getId() << "Other" << a->getOther()->getId();
				}
			}
			break;
		case Stripper::Update::MatchAll:
            // Auch bei Stripper On aufgerufen
			if( getImp()->count() == 0 )
				loadAllStrips();
			else
				for( int i = 0; i < getImp()->count(); i++ )
				{
                    _SystemItem* s = static_cast<_SystemItem*>( getImp()->child( i ) );
                    fillMatches( s );
                }
			break;
		case Stripper::Update::Clear:
			//loadAllStrips(); 
            // Auch bei Stripper Off aufgerufen
			for( int i = 0; i < getImp()->count(); i++ )
			{
                _SystemItem* s = static_cast<_SystemItem*>( getImp()->child( i ) );
                clearMatches( s );
			}
			break;
		}
	}
	END_HANDLER();
}

void _StripListView::clearMatches( _SystemItem* s )
{
    QList<_StripLinkItem*> sl;
    for( int j = 0; j < s->count(); j++ )
    {
        _StripLinkItem* l = dynamic_cast<_StripLinkItem*>( s->child( j ) );
        if( l )
            sl.append( l );
    }
    for( int k = 0; k < sl.size(); k++ )
    {
        sl[k]->removeMe();
        // QApplication::processEvents();
    }
}

void _StripListView::fillMatches( _SystemItem* s )
{
    Stripper::MatchList l = d_pro->getStripper()->getMatches( s->d_sys );
    for( int j = 0; j < l.size(); j++ )
    {
        new _StripLinkItem( s, l[j].first, l[j].second );
    }
    getImp()->commit();
}

struct _StripListGadgetDeleg : public QItemDelegate
{
	_StripListGadgetDeleg( Gui::ListView* p ):QItemDelegate(p) {}
	Gui::ListView* listView() const { return static_cast<Gui::ListView*>( parent() ); }

	void paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
	{
		if( _SpinLinkItem* item = dynamic_cast<_SpinLinkItem*>( listView()->indexToItem( index ) ) )
		{
			if( index.column() == 0 )
			{
				_PeakLabelItem* _this = dynamic_cast<_PeakLabelItem*>( item->parent() );
				assert( _this );
				const SpinLink::Alias& alias = item->d_link->getAlias( _this->d_list->d_spec );
				assert( s_cl );
				QColor clr = s_cl->getColor( alias.d_code );
				QPixmap icon( c_link );
				if( alias.d_visible && alias.d_code && clr.isValid() )
				{
					painter->save();
					painter->setCompositionMode( QPainter::CompositionMode_SourceIn );
					painter->fillRect( option.rect.left(), option.rect.top(), icon.width(), item->height(), clr );
					painter->restore();
				}
				painter->drawPixmap( option.rect.left(), option.rect.top(), icon.width(), icon.height(), icon );
			}else 
				QItemDelegate::paint( painter, option, index );
		}else
			QItemDelegate::paint( painter, option, index );
	}

};

//////////////////////////////////////////////////////////////////////

StripListGadget::StripListGadget(Lexi::Viewport* vp, 
		Spec::Project* pro, 
		Root::Agent* main, Lexi::Glyph* popup, bool links, bool border )
{
	assert( vp );
	d_this = new _StripListView( vp, pro, main, links );
	d_this->addRef();
	d_this->getImp()->setRootIsDecorated( true );
	d_this->getImp()->setShowSortIndicator( true );
	if( !border )
		d_this->getImp()->setLineWidth( 0 );

	d_this->getImp()->setItemDelegate( new _StripListGadgetDeleg( d_this->getImp() ) );

	setBody( d_this );
	setController( d_this );
	d_this->getImp()->show();
}

StripListGadget::~StripListGadget()
{
	if( d_this )
		d_this->release();
	d_this = 0;
} 

Lexi::ListView* StripListGadget::getListView() const
{
	return d_this;
}

Gui::Menu* StripListGadget::createPopup()
{
	Gui::Menu* menu = new Gui::Menu();
	Gui::Menu::item( menu,  "Find Spin...", StripListGadget::FindSpin, false );
	Gui::Menu::item( menu,  "Find Link Partner...", StripListGadget::GotoOther, false );
	menu->addSeparator();
	Gui::Menu::item( menu,  "Run Automatic Strip Matcher", StripListGadget::RunStripper, true );
	Gui::Menu::item( menu,  "Strict Strip Matching", StripListGadget::StrictStripMatch, true );
	Gui::Menu::item( menu,  "Unlabeled Strip Matching", StripListGadget::UnlabeledStripMatch, true );
	Gui::Menu::item( menu,  "Set Spin Tolerance...", StripListGadget::SetSpinTol, false );
	menu->addSeparator();
	Gui::Menu::item( menu,  "Create System", StripListGadget::CreateSystem, false );
	Gui::Menu::item( menu,  "Eat System...", StripListGadget::EatSystem, false );
	Gui::Menu::item( menu,  "Set Assig. Candidates...", StripListGadget::SetCandidates, false );
	Gui::Menu::item( menu,  "Set System Type...", StripListGadget::SetSysType, false );
	Gui::Menu::item( menu,  "Show Alignment...", StripListGadget::ShowAlignment, false );
	menu->addSeparator();
	Gui::Menu::item( menu,  "Link This", StripListGadget::LinkThis, false );
	Gui::Menu::item( menu,  "Unlink Successor", StripListGadget::UnlinkSucc, false );
	Gui::Menu::item( menu,  "Unlink Predecessor", StripListGadget::UnlinkPred, false );
	menu->addSeparator();
	Gui::Menu::item( menu,  "Create Spin...", StripListGadget::CreateSpin, false );
	Gui::Menu::item( menu,  "Move Spin...", StripListGadget::MoveSpin, false );
	Gui::Menu::item( menu,  "Label Spin...", StripListGadget::LabelSpin, false );
	Gui::Menu::item( menu,  "Force Label...", StripListGadget::ForceLabel, false );
	Gui::Menu::item( menu,  "Accept Label", StripListGadget::AcceptLabel, false );
	menu->addSeparator();
	Gui::Menu::item( menu,  "Assign to...", StripListGadget::Assign, false );
	Gui::Menu::item( menu,  "Unassign", StripListGadget::Unassign, false );
	Gui::Menu::item( menu,  "Delete", StripListGadget::Delete, false );
	Gui::Menu::item( menu,  "Edit Attributes...", StripListGadget::EditAtts, false );
	Gui::Menu::item( menu,  "Open System Table...", StripListGadget::ShowTable, false );
	menu->addSeparator();
	Gui::Menu::item( menu,  "Show Spin Links", StripListGadget::ShowLinks, true );
	Gui::Menu::item( menu,  "Create Link...", StripListGadget::CreateLink, false );
	Gui::Menu::item( menu,  "Set Link Params...", StripListGadget::LinkParams, false );
	menu->addSeparator();
	Gui::Menu::item( menu,  "Open All", StripListGadget::OpenAll, false );
	Gui::Menu::item( menu,  "Close All", StripListGadget::CloseAll, false );
	return menu;
}

bool StripListGadget::gotoSpin(Spin * spin)
{
	if( spin == 0 || spin->getSystem() == 0 )
		return false;
    _SystemItem* s = d_this->d_map[ spin->getSystem() ];
	if( s == 0 )
		return false;
	_PeakLabelItem* p;
	for( int i = 0; i < s->count(); i++ )
	{
		p = dynamic_cast<_PeakLabelItem*>( s->child( i ) );
		if( p && p->d_spin == spin )
		{
			d_this->getImp()->ensureItemVisible( p );
			d_this->getImp()->setSelected( p, true );
			return true;
		}
	}
	return false;
}

void _StripListView::updateItem(_SystemItem *s, bool repaint )
{
	// s->d_excl = !d_strips->getExcludes( s->d_id ).empty();
	if( repaint )
		s->repaint();
}

SpinSystem* StripListGadget::getSelectedStrip()
{
    _SystemItem* i = dynamic_cast<_SystemItem*>( d_this->getImp()->currentItem() );
	if( i != nil )
		return i->d_sys;
	_StripLinkItem* a = dynamic_cast<_StripLinkItem*>( d_this->getImp()->currentItem() );
	if( a )
        i = dynamic_cast<_SystemItem*>( a->parent() );
	if( i != nil )
		return i->d_sys;
	_PeakLabelItem* b = dynamic_cast<_PeakLabelItem*>( d_this->getImp()->currentItem() );	
	if( b )
        i = dynamic_cast<_SystemItem*>( b->parent() );
	if( i != nil )
		return i->d_sys;
	else
		return 0;
}

SpinSystem* StripListGadget::getCandSucc() const
{
	_StripLinkItem* a = dynamic_cast<_StripLinkItem*>( d_this->getImp()->currentItem() );
	if( a != nil && a->d_fit > 0.0 )
		return a->d_sys;
	else
		return 0;
}

SpinSystem* StripListGadget::getCandPred() const
{
	_StripLinkItem* a = dynamic_cast<_StripLinkItem*>( d_this->getImp()->currentItem() );
	if( a != nil && a->d_fit < 0.0 )
		return a->d_sys;
	else
		return 0;
}

void StripListGadget::showStrip(SpinSystem* i)
{
    _SystemItem* s = d_this->d_map[ i ];
	if( s == 0 )
		return;
	d_this->getImp()->setSelected( s, true );
	d_this->getImp()->setOpen( s, true );
	d_this->getImp()->ensureItemVisible( s );
}

Spin* StripListGadget::getSelectedSpin() const
{
	_PeakLabelItem* b = dynamic_cast<_PeakLabelItem*>( d_this->getImp()->currentItem() );	
	if( b )
		return b->d_spin;
	_SpinLinkItem* a = dynamic_cast<_SpinLinkItem*>( d_this->getImp()->currentItem() );
	if( a )
		b = dynamic_cast<_PeakLabelItem*>( a->parent() );
	if( b != nil )
		return b->d_spin;
	else
		return 0;
}

Spin* StripListGadget::getSelectedLink() const
{
	return d_this->getLink();
}

void StripListGadget::setSpec(Spectrum * spec)
{
	if( d_this->d_spec == spec )
		return;
	d_this->d_spec = spec;
	d_this->getImp()->viewport()->update();
}

Spectrum* StripListGadget::getSpec() const
{
	return d_this->d_spec;
}

void StripListGadget::handle(Root::Message & msg)
{
	BEGIN_HANDLER();
	MESSAGE( Root::Action, a, msg )
	{
		EXECUTE_ACTION( StripListGadget, *a );
	}
	END_HANDLER();
}

void StripListGadget::handleEditAtts(Root::Action& a)
{
	Spin* spin = getSelectedSpin();
	SpinSystem* strip = d_this->getSys();
	Spin* link = d_this->getLink();
	ACTION_ENABLED_IF( a, spin != 0 || strip != 0 || link != 0 );
	if( link )
		DynValueEditor::edit( d_this->getImp(), d_this->d_pro->getRepository()
			->findObjectDef( Repository::keyLink ), spin->findLink( link ) );
	else if( spin )
		DynValueEditor::edit( d_this->getImp(), d_this->d_pro->getRepository()
			->findObjectDef( Repository::keySpin ), spin );
	else if( strip )
		DynValueEditor::edit( d_this->getImp(), d_this->d_pro->getRepository()
			->findObjectDef( Repository::keySpinSystem ), strip );
}

void StripListGadget::handleSetCandidates(Root::Action& a)
{
	SpinSystem* sys = d_this->getSys();
	ACTION_ENABLED_IF( a, sys != 0);

	CandidateDlg dlg( d_this->getImp(), d_this->d_pro->getRepository() );
	dlg.setTitle( sys );
	if( dlg.exec() )
		d_this->d_pro->getSpins()->setCands( sys, dlg.d_cands );
}

void StripListGadget::handleSetSpinTol(Root::Action& a)
{
	Spin* spin = getSelectedSpin();
	ACTION_ENABLED_IF( a, spin != 0 );

	AtomType t = spin->getAtom();
	bool ok	= FALSE;
	QString res;
	res.sprintf( "%0.3f", d_this->d_pro->getMatcher()->getTol( t ) );
	res	= QInputDialog::getText( "Set Spin Tolerance", 
		"Please	enter a positive PPM value:", QLineEdit::Normal, 
		res, &ok, d_this->getImp() );
	if( !ok )
		return;
	PPM w = res.toFloat( &ok );
	if( !ok || w <= 0.0 )
	{
		QMessageBox::critical( d_this->getImp(), "Set Spin Tolerance",
				"Invalid tolerance!", "&Cancel" );
		return;
	}
	d_this->d_pro->getMatcher()->setTol( t, w );
	d_this->d_pro->getStripper()->recalcAll();
}

void StripListGadget::handleStrictStripMatch(Root::Action& a)
{
	Stripper* s = d_this->d_pro->getStripper();
	ACTION_CHECKED_IF( a, true, s->isStrict() );

	s->setStrict( !s->isStrict() );
}

void StripListGadget::handleUnlabeledStripMatch(Root::Action& a)
{
	Stripper* s = d_this->d_pro->getStripper();
	ACTION_CHECKED_IF( a, true, s->isUnlabeled() );

	s->setUnlabeled( !s->isUnlabeled() );
	if( s->isUnlabeled() )
		s->setStrict( false );
}

void StripListGadget::handleRunStripper(Root::Action& a)
{
	Stripper* s = d_this->d_pro->getStripper();
	ACTION_CHECKED_IF( a, true, s->isOn() );

	s->setOn( !s->isOn() );
}

void StripListGadget::handleUnassign(Root::Action& a)
{
	Spin* spin = getSelectedSpin();
	SpinSystem* sys = d_this->getSys();
    ACTION_ENABLED_IF( a, ( spin != 0 && spin->getSystem() ) ||
        ( sys != 0 && sys->getAssig() != 0 ) );
	if( spin )
	{
		Root::Ref<UnassignSpinCmd> cmd = new UnassignSpinCmd( 
			d_this->d_pro->getSpins(), spin ); 
		cmd->handle( d_this->d_main );
	}else
	{
		Root::Ref<UnassignSystemCmd> cmd =
			new UnassignSystemCmd( d_this->d_pro->getSpins(), sys );
		cmd->handle( d_this->d_main );
	}
}

void StripListGadget::handleAssign(Root::Action& a)
{
	Spin* spin = getSelectedSpin();
	SpinSystem* sys = d_this->getSys();
	ACTION_ENABLED_IF( a, spin != 0 || sys != 0 );
	if( spin )
	{
		bool ok = false;
		int res = QInputDialog::getInteger( "Select Spin System",
			"Enter system id:", 0, 1, 99999999, 1, &ok, d_this->getImp() );
		if( !ok )
			return;
		SpinSystem* target = d_this->d_pro->getSpins()->getSystem( res );
		if( target == 0 )
		{
			if( QMessageBox::warning( d_this->getImp(), "Assign Spin",
					  "The selected spin system doesn't exist. Do you want to create it?",
					  "&OK", "&Cancel" ) != 0 )	
					  return;
			target = d_this->d_pro->getSpins()->addSystem( res );
		}else if( target == sys )
		{
			QMessageBox::critical( d_this->getImp(), "Assign Spin", 
				"Spin is already part of this system!", "&Cancel" );
			return;
		}
		Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Reassign Spin" );
		cmd->add( new UnassignSpinCmd( d_this->d_pro->getSpins(), spin ) ); 
		cmd->add( new AssignSpinCmd( d_this->d_pro->getSpins(), spin, target ) ); 
		cmd->handle( d_this->d_main );
	}else
	{
		bool ok;
		QString str;
		str.sprintf( "Assignment for spin system %d:", sys->getId() );
		int r = QInputDialog::getInteger( "Assign Spin System", 
			str, (sys->getAssig())?sys->getAssig()->getId():0, 
			-999999, 999999, 1, &ok, d_this->getImp() );
		if( !ok )
			return;

		Residue* res = d_this->d_pro->getSequence()->getResidue( r );
		if( res == 0 )
		{
			QMessageBox::critical( d_this->getImp(), "Assign Spin System", 
				"Unknown residue number!", "&Cancel" );
			return;
		}
		Root::Ref<AssignSystemCmd> cmd =
			new AssignSystemCmd( d_this->d_pro->getSpins(), sys, res );
		cmd->handle( d_this->d_main );
	}
}

void StripListGadget::handleUnlinkPred(Root::Action& a)
{
	SpinSystem* id = d_this->getSys();
	if( id == 0 )
		return;
	SpinSystem* other = id->getPred();

	ACTION_ENABLED_IF( a, id != 0 && other != 0 );


	Root::Ref<UnlinkSystemCmd> cmd =
		new UnlinkSystemCmd( d_this->d_pro->getSpins(), other, id );
	cmd->handle( d_this->d_main );
}

void StripListGadget::handleUnlinkSucc(Root::Action& a)
{
	SpinSystem* id = d_this->getSys();
	if( id == 0 )
		return;
	SpinSystem* other = id->getSucc();

	ACTION_ENABLED_IF( a, id != 0 && other != 0 );


	Root::Ref<UnlinkSystemCmd> cmd =
		new UnlinkSystemCmd( d_this->d_pro->getSpins(), id, other );
	cmd->handle( d_this->d_main );
}

void StripListGadget::handleShowAlignment(Root::Action& a)
{
	SpinSystem* id = d_this->getSys();
	SpinSystem* p = getCandPred();
	SpinSystem* s = getCandSucc();
	ACTION_ENABLED_IF( a, p != 0 || s != 0 || id != 0 );

	SpinSystemString fra;
	if( id != 0 ) // das selektierte Item ist ein Strip
		d_this->d_pro->getSpins()->fillString( id, fra );
	else if( p != 0 )
		d_this->d_pro->getSpins()->fillString( p, getSelectedStrip(), fra );
	else 
		d_this->d_pro->getSpins()->fillString( getSelectedStrip(), s, fra );

	FragmentAssignment* f = new FragmentAssignment( 
		d_this->d_pro->getSpins(), d_this->d_pro->getMatcher(), fra );
	SingleAlignmentView* v = new SingleAlignmentView( d_this->d_main, f );
	v->show();
}

void StripListGadget::handleLinkThis(Root::Action& a)
{
	Spin* link = d_this->getLink();
	ACTION_ENABLED_IF( a, getCandSucc() != 0 || getCandPred() != 0 || link );

	SpinSystem* pred;
	SpinSystem* succ;
	if( getCandPred() != 0 )
	{
		pred = getCandPred();
		succ = getSelectedStrip();
	}else if( getCandSucc() != 0 )
	{
		pred = getSelectedStrip();
		succ = getCandSucc();
	}else if( link )
	{
		Spin* spin = getSelectedSpin();
		assert( spin );
		pred = spin->getSystem();
		succ = link->getSystem();
		assert( pred );
		assert( succ );
		Dlg::StringList l( 2 );
		QString str;
		str.sprintf( "Pred. = %d / Succ.= %d", pred->getId(), succ->getId() );
		l[ 0 ] = str.toLatin1().data();
		str.sprintf( "Pred. = %d / Succ. = %d", succ->getId(), pred->getId() );
		l[ 1 ] = str.toLatin1().data();
		int res = Dlg::getOption( d_this->getImp(), l, "Select Direction" );
		if( res == -1 )
			return;
		else if( res == 1 )
		{
			SpinSystem* tmp = succ;
			succ = pred;
			pred = tmp;
		}
	}else
		return;

    try
	{
        Root::Ref<LinkSystemCmd> cmd =
            new LinkSystemCmd( d_this->d_pro->getSpins(), pred, succ );
        cmd->handle( d_this->d_main );
    }catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Link This", e.what() );
	}
}

void StripListGadget::handleDelete(Root::Action& a)
{
	Spin* spin = getSelectedSpin();
	SpinSystem* sys = d_this->getSys();
	Spin* link = d_this->getLink();
	ACTION_ENABLED_IF( a, spin != 0 || sys != 0 || link != 0 );
	if( link )
	{
		Root::Ref<UnlinkSpinCmd> cmd = new UnlinkSpinCmd( 
			d_this->d_pro->getSpins(), spin, link );
		cmd->handle( d_this->d_main );
	}else if( spin )
	{
		Root::Ref<DeleteSpinCmd> cmd = new DeleteSpinCmd( d_this->d_pro->getSpins(), spin );
		cmd->handle( d_this->d_main );
	}else
	{
		if( sys->getPred() == 0 && sys->getSucc() == 0 && sys->getAssig() == 0 )
		{
			Root::Ref<DeleteSystemCmd> cmd = new DeleteSystemCmd( 
				d_this->d_pro->getSpins(), sys );
			cmd->handle( d_this->d_main );
		}else
			QMessageBox::critical( d_this->getImp(), "Delete Spin System", 
				"Cannot delete this system because there are still assignments!", "&Cancel" );
	}
}

void StripListGadget::handleSetSysType(Root::Action& a)
{
	SpinSystem* sys = d_this->getSys();
	ACTION_ENABLED_IF( a, sys );

	Repository::SystemTypeMap::const_iterator p;
	const Repository::SystemTypeMap& sm = d_this->d_pro->getRepository()->getSystemTypes();

	int cur = 0, i = 0;
	QStringList l;
	l.append( "" );
	for( p = sm.begin(); p != sm.end(); ++p, i++ )
	{
		l.append( (*p).second->getName().data() ); // NOTE: Name sollte Unique sein.
	}
	l.sort();
	if( sys->getSysType() )
		cur = l.findIndex( sys->getSysType()->getName().data() );

	bool ok;
	QString res = QInputDialog::getItem( "Set System Type", "Select a spin system type:", 
		l, cur, false, &ok, d_this->getImp() );
	if( !ok )
		return;

	SystemType* sst = 0;
	for( p = sm.begin(); p != sm.end(); ++p )
	{
		if( res == (*p).second->getName().data() )
		{
			sst = (*p).second;
			break;
		}
	}

	Root::Ref<ClassifySystemCmd> cmd =
		new ClassifySystemCmd( d_this->d_pro->getSpins(), sys, sst );
	cmd->handle( d_this->d_main );
}

void StripListGadget::handleMoveSpinAlias(Root::Action& a)
{
	// TODO
}

void StripListGadget::handleMoveSpin(Root::Action& a)
{
	Spin* spin = getSelectedSpin();
	ACTION_ENABLED_IF( a, spin );

	bool ok;
	QString res;
	res.sprintf( "%f", spin->getShift() );
	res = QInputDialog::getText( "Move Spin", 
		"Please	enter a new PPM value for spin:", QLineEdit::Normal, 
		res, &ok, d_this->getImp() );
	if( !ok )
		return;
	PPM p = res.toFloat( &ok );
	if( !ok )
	{
		QMessageBox::critical( d_this->getImp(), "Move Spin", "Invalid ppm value!", "&Cancel" );
		return;
	}

	Root::Ref<MoveSpinCmd> cmd =
		new MoveSpinCmd( d_this->d_pro->getSpins(), spin, p, 0 );
		// Move generisches Spektrum
	cmd->handle( d_this->d_main );
}

void StripListGadget::handleLabelSpin(Root::Action& a)
{
	Spin* spin = getSelectedSpin();
	ACTION_ENABLED_IF( a, spin );

	SpinLabel l = spin->getLabel();
	ResidueType* r = d_this->d_pro->inferResiType( spin->getSystem() );
	SpinLabelSet ll;
	if( r )
		ll = r->findAll( spin->getAtom() );
	if( !Dlg::getLabel( d_this->getImp(), l, ll ) )
		return;
	Root::Ref<LabelSpinCmd> cmd =
		new LabelSpinCmd( d_this->d_pro->getSpins(), spin, l );
	cmd->handle( d_this->d_main );
}

void StripListGadget::handleCreateSpin(Root::Action& a)
{
	SpinSystem* sys = getSelectedStrip();
	ACTION_ENABLED_IF( a, sys );

	Dlg::SpinParams sp;
	if( !Dlg::getSpinParams( d_this->getImp(), sp ) )
		return;
	Root::Ref<PickSystemSpinLabelCmd> cmd = new PickSystemSpinLabelCmd( 
		d_this->d_pro->getSpins(), sys, // Darf null sein.
		sp.d_type, sp.d_shift, sp.d_label, 0 ); 
		// Pick generisches Spektrum
	cmd->handle( d_this->d_main );
}

void StripListGadget::handleForceLabel(Root::Action& a)
{
	Spin* spin = getSelectedSpin();
	ACTION_ENABLED_IF( a, spin );

	bool ok	= FALSE;
	QString res;
	res.sprintf( "Please enter a valid label (%s):", SpinLabel::s_syntax );
	res = QInputDialog::getText( "Force Spin Label", res, QLineEdit::Normal, 
			spin->getLabel().data(), &ok, d_this->getImp() );
	if( !ok )
		return;

	SpinLabel l;
	if( !SpinLabel::parse( res, l ) )
	{
		QMessageBox::critical( d_this->getImp(), "Force Spin Label", 
			"Invalid spin label syntax!", "&Cancel" );
		return;
	}

	Root::Ref<LabelSpinCmd> cmd =
		new LabelSpinCmd( d_this->d_pro->getSpins(), spin, l );
	cmd->handle( d_this->d_main );
}

void StripListGadget::handleCreateSystem(Root::Action& a)
{
	ACTION_ENABLED_IF( a, true );
	Root::Ref<CreateSystemOnlyCmd> cmd =
		new CreateSystemOnlyCmd( d_this->d_pro->getSpins() );
	cmd->handle( d_this->d_main );
	Gui::ListViewItem* item = d_this->d_map[ cmd->getSystem() ];
	d_this->getImp()->setSelected( item, true );
	d_this->getImp()->ensureItemVisible( item );
}

void StripListGadget::handleEatSystem(Root::Action& a)
{
	SpinSystem* sys = d_this->getSys();
	ACTION_ENABLED_IF( a, sys );
	bool ok = false;
	int res = QInputDialog::getInteger( "Select Spin System",
		"Enter the ID of the spin system to eat:", 0, 1, 99999999, 1, &ok, d_this->getImp() );
	if( !ok )
		return;
	SpinSystem* source = d_this->d_pro->getSpins()->getSystem( res );
	if( source == 0 )
	{
		QMessageBox::critical( d_this->getImp(), "Eat Spin System", 
			"Spin system does not exist!", "&Cancel" );
		return;
	}
	if( source->getPred() != 0 || source->getSucc() != 0 || source->getAssig() != 0 )
	{
		QMessageBox::critical( d_this->getImp(), "Eat Spin System", 
			"Cannot delete source system because there are still assignments!", "&Cancel" );
		return;
	}
	SpinSystem::Spins::const_iterator p1;
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Eat Spin System" );
	for( p1 = source->getSpins().begin(); p1 != source->getSpins().end(); ++p1 )
	{
		if( !sys->isAcceptable( (*p1)->getLabel() ) )
		{
			QMessageBox::critical( d_this->getImp(), "Eat Spin System", 
				"One of the spins in the source system is not acceptable by the target system!", "&Cancel" );
			return;
		}
		cmd->add( new UnassignSpinCmd( d_this->d_pro->getSpins(), (*p1) ) ); 
		cmd->add( new AssignSpinCmd( d_this->d_pro->getSpins(), (*p1), sys ) ); 
	}
	cmd->add( new DeleteSystemCmd( d_this->d_pro->getSpins(), source ) );
	cmd->handle( d_this->d_main );
}

void StripListGadget::handleCreateLink(Root::Action& a)
{
	// TODO
	Spin* spin = getSelectedSpin();
	ACTION_ENABLED_IF( a, spin );
	bool ok = false;
	int res = QInputDialog::getInteger( "Link Spin",
		"Enter the ID of the spin to link to:", 0, 1, 99999999, 1, &ok, d_this->getImp() );
	if( !ok )
		return;
	Spin* to = d_this->d_pro->getSpins()->getSpin( res );
	if( to == 0 )
	{
		QMessageBox::critical( d_this->getImp(), "Link Spin", 
			"Target spin does not exist!", "&Cancel" );
		return;
	}else if( to == spin )
	{
		QMessageBox::critical( d_this->getImp(), "Link Spin", 
			"Cannot link to itself!", "&Cancel" );
		return;
	}else if( to->findLink( spin ) != 0 )
	{
		QMessageBox::critical( d_this->getImp(), "Link Spin", 
			"Spins are already linked!", "&Cancel" );
		return;
	}
	Root::Ref<LinkSpinCmd> cmd =
		new LinkSpinCmd( d_this->d_pro->getSpins(), spin, to );
	cmd->handle( d_this->d_main );
}

void StripListGadget::handleShowLinks(Root::Action& a)
{
	ACTION_CHECKED_IF( a, true, d_this->d_showLinks );
	d_this->d_showLinks = !d_this->d_showLinks;
	d_this->loadAllStrips();
}

void StripListGadget::handleFindSpin(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	bool ok = false;
	int res = QInputDialog::getInteger( "Find Spin",
		"Enter the ID of the spin to find:", 0, 1, 99999999, 1, &ok, d_this->getImp() );
	if( !ok )
		return;
	Spin* to = d_this->d_pro->getSpins()->getSpin( res );
	if( to == 0 )
	{
		QMessageBox::critical( d_this->getImp(), "Find Spin", 
			"Spin does not exist!", "&Cancel" );
		return;
	}
	gotoSpin( to );
}

void StripListGadget::handleGotoOther(Action & a)
{
	Spin* spin = d_this->getLink();
	ACTION_ENABLED_IF( a, spin );
	gotoSpin( spin );
}

void StripListGadget::handleAcceptLabel(Root::Action & a)
{
	Spin* spin = getSelectedSpin();
	ACTION_ENABLED_IF( a, spin && spin->getSystem() &&
		!spin->getLabel().isNull() &&
		!spin->getLabel().isFinal() );

	SpinSystem* sys = spin->getSystem();
	SpinSystem::Spins::const_iterator i;
	SpinLabel l;
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Accept Label" ); 
	for( i = sys->getSpins().begin(); i != sys->getSpins().end(); i++ )
	{
		if( (*i) != spin && (*i)->getLabel() == spin->getLabel() )
			cmd->add( new LabelSpinCmd( d_this->d_pro->getSpins(), (*i), l ) );
	}
	l = spin->getLabel();
	l.setFinal();
	cmd->add( new LabelSpinCmd( d_this->d_pro->getSpins(), spin, l ) );
	cmd->handle( d_this->d_main );
}

void StripListGadget::handleShowTable(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );

	int i = 0;
	const SpinBase::SpinSystemMap& sm = d_this->d_pro->getSpins()->getSystems();
	SpinBase::SpinSystemMap::const_iterator p;
	ObjectListView::ObjectList o( sm.size() );
	for( p = sm.begin(); p != sm.end(); ++p, ++i )
		o[ i ] = (*p).second;
	ObjectListView::edit( d_this->getImp(), d_this->d_pro->getRepository()
		->findObjectDef( Repository::keySpinSystem ), o );
}

void StripListGadget::handleOpenAll(Action & a)
{
	ACTION_ENABLED_IF( a, true );
    Gui::ListViewItemIterator it( d_this->getImp() );
    for( ; it.current(); ++it )
        it.current()->setOpen( true );
}

void StripListGadget::handleCloseAll(Action & a)
{
	ACTION_ENABLED_IF( a, true );
    Gui::ListViewItemIterator it( d_this->getImp() );
    for( ; it.current(); ++it )
        it.current()->setOpen( false );
}

void StripListGadget::handleLinkParams(Action & a)
{
	Spin* spin = getSelectedSpin();
	Spin* link = d_this->getLink();
	ACTION_ENABLED_IF( a, link != 0 );

	SpinLink* sl = spin->findLink( link );
	assert( sl );
	const SpinLink::Alias& al = sl->getAlias( d_this->d_spec );
	Dlg::LinkParams2 par;
	par.d_rating = al.d_rating;
	par.d_code = al.d_code;
	par.d_visible = al.d_visible;
	if( Dlg::getLinkParams2( d_this->getImp(), par ) )
		d_this->d_pro->getSpins()->setAlias( sl, d_this->d_spec, 
		par.d_rating, par.d_code, par.d_visible );
	// TODO: Undo
}

void StripListGadget::pick( Twips x, Twips y, const Allocation& a, Trace& t )
{
    if( a.isHit( x, y ) )
		t.push_back( this );
}


