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

#include "StripListGadget2.h"
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

#include <Spec/Stripper.h>
#include <Spec/Repository.h>

#include <SpecView/DynValueEditor.h>
#include <SpecView/CandidateDlg.h>
#include <SingleAlignmentView.h>
#include <SpecView/ObjectListView.h>
#include <Dlg.h>
#include "LuaCaraExplorer.h"
using namespace Spec;
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

struct _SpinLinkItem3 : public Gui::ListViewItem
{
    Root::Ref<Spin> d_spin;
    Root::Ref<SpinLink> d_link;

    _SpinLinkItem3( _PeakLabelItem2 * parent, Spin* s, SpinLink* l );
	QString text( int f ) const;
	QString key( int f, bool ) const;
	QVariant pixmap( int i ) const;
};

struct _StripLinkItem2 : public Gui::ListViewItem
{
    _StripLinkItem2( Gui::ListViewItem * parent, SpinSystem* sys, Fitness f ):
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

struct _PeakLabelItem2 : public Gui::ListViewItem
{
    _PeakLabelItem2( Gui::ListViewItem * parent, StripListGadget2* l, Spin* s ):
		Gui::ListViewItem( parent ), d_spin( s ), d_list( l ) {}
    Root::Ref<Spin> d_spin;
    StripListGadget2* d_list;

	QString text( int f ) const
	{
		switch( f )
		{
		case 0:
			d_spin->getLabel().format( g_buffer, sizeof( g_buffer ) );
			return g_buffer;
		case 1:
            ::sprintf( g_buffer, "%.3f %s", d_spin->getShift( d_list->getSpec() ),
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
            shift = d_spin->getShift( d_list->getSpec() );
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

    _SpinLinkItem3* findLink( SpinLink* l ) const
	{
        _SpinLinkItem3* item = 0;
		for( int i = 0; i < count(); i++ )
		{
            item = static_cast<_SpinLinkItem3*>( child( i ) );
			if( item && item->d_link == l )
				return item;
		}
		return 0;
	}
    _SpinLinkItem3* findLink( Spin* other ) const
	{
        _SpinLinkItem3* item = 0;
		for( int i = 0; i < count(); i++ )
		{
            item = static_cast<_SpinLinkItem3*>( child( i ) );
			if( item && item->d_spin == other )
				return item;
		}
		return 0;
	}
};

struct _SystemItem2 : public Gui::ListViewItem
{
    _SystemItem2( Gui::ListView * parent, SpinSystem* sys ):
		Gui::ListViewItem( parent ), d_sys( sys ), d_excl( false ) 
		{ assert( sys ); }
    Root::Ref<SpinSystem> d_sys;
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

    _PeakLabelItem2* findSpin( Spin* s ) const
	{
        _PeakLabelItem2* item = 0;
		for( int i = 0; i < count(); i++ )
		{
            item = dynamic_cast<_PeakLabelItem2*>( child( i ) );
			if( item && item->d_spin == s )
				return item;
		}
		return 0;
	}
    _SpinLinkItem3* findLink( Spin* s, SpinLink* l ) const
	{
        _PeakLabelItem2* i = findSpin( s );
		if( i )
			return i->findLink( l  );
		else
			return 0;
	}
    _SpinLinkItem3* findLink( Spin* spin, Spin* other ) const
	{
        _PeakLabelItem2* i = findSpin( spin );
		if( i )
			return i->findLink( other  );
		else
			return 0;
	}
    _StripLinkItem2* findStripLink( SpinSystem* sys ) const
	{
        _StripLinkItem2* item;
		for( int i = 0; i < count(); i++ )
		{
            item = dynamic_cast<_StripLinkItem2*>( child( i ) );
			if( item && item->d_sys == sys )
				return item;
		}
		return 0;
	}
};

_SpinLinkItem3::_SpinLinkItem3( _PeakLabelItem2 * parent, Spin* s, SpinLink* l ):
		Gui::ListViewItem( parent ), d_spin( s ), d_link( l ) {}

QString _SpinLinkItem3::text( int f ) const
{
    _PeakLabelItem2* _this = dynamic_cast<_PeakLabelItem2*>( parent() );
	assert( _this );
    const SpinLink::Alias& alias = d_link->getAlias( _this->d_list->getSpec() );
	switch( f )
	{
	case 0:
		d_spin->getLabel().format( g_buffer, sizeof( g_buffer ) );
		return g_buffer;
	case 1:
        ::sprintf( g_buffer, "%.3f %s", d_spin->getShift( _this->d_list->getSpec() ),
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
QString _SpinLinkItem3::key( int f, bool ) const
{
	double shift;
    _PeakLabelItem2* _this = dynamic_cast<_PeakLabelItem2*>( parent() );
	assert( _this );
    const SpinLink::Alias& alias = d_link->getAlias( _this->d_list->getSpec() );
	switch( f )
	{
	case 0:
		d_spin->getLabel().format( g_buffer, sizeof( g_buffer ) );
		return g_buffer;
	case 1:
        shift = d_spin->getShift( _this->d_list->getSpec() );
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
QVariant _SpinLinkItem3::pixmap( int i ) const
{
	if( i != 0 )
		return QVariant();
	return QPixmap( c_link );
}

////////////////////////////////////////////////////////////////////////////////


SpinSystem* StripListGadget2::getSys() const
{
    _SystemItem2* i = dynamic_cast<_SystemItem2*>( currentItem() );
	if( i != nil )
		return i->d_sys;
	else
		return 0;
}
Spin* StripListGadget2::getLink() const
{
    _SpinLinkItem3* i = dynamic_cast<_SpinLinkItem3*>( currentItem() );
	if( i != nil )
		return i->d_spin;
	else
		return 0;
}

void StripListGadget2::loadAllStrips()
{
    clear();
	d_map.clear();
	const SpinBase::SpinSystemMap& strips = d_pro->getSpins()->getSystems();
	SpinBase::SpinSystemMap::const_iterator idx;
	for( idx = strips.begin(); idx != strips.end(); ++idx )
        createSysView( this, (*idx).second );
}

_SpinLinkItem3* StripListGadget2::createLinkView(_PeakLabelItem2 * p, SpinLink* l)
{
	Spin* other;
	if( l->getLhs() == p->d_spin->getId() )
		other = d_pro->getSpins()->getSpin( l->getRhs() );
	else
		other = d_pro->getSpins()->getSpin( l->getLhs() );
	if( other )
	{
        return new _SpinLinkItem3( p, other, l );
	}else
		return 0;
}

_PeakLabelItem2* StripListGadget2::createSpinView(_SystemItem2 * s, Spin* spin)
{
    if( s == 0 )
    {
        qWarning() << "StripListGadget2::createSpinView s == 0";
        return 0;
    }
    _PeakLabelItem2* p = new _PeakLabelItem2( s, this, spin );
	if( d_showLinks )
	{
		Spin::Links::const_iterator p1;
		for( p1 = spin->getLinks().begin(); p1 != spin->getLinks().end(); ++p1 )
			createLinkView( p, (*p1) );
	}
	return p;
}

_SystemItem2* StripListGadget2::createSysView( Gui::ListView* v, SpinSystem* sys )
{
    if( d_map.contains( sys->getId() ) )
    {
        qWarning() << "StripListGadget2::createSysView: a system with this id already created" << sys->getId();
        return d_map.value( sys->getId() );
    }
    _SystemItem2* s = new _SystemItem2( v, sys );
    d_map[ sys->getId() ] = s;

	const SpinSystem::Spins& spins = s->d_sys->getSpins();
	SpinSystem::Spins::const_iterator idx;

	if( d_pro->getStripper()->isOn() )
        fillMatches( s );

	for( idx = spins.begin(); idx != spins.end(); ++idx )
		createSpinView( s, (*idx) );
	s->repaint();
	return s;
}

void StripListGadget2::clearMatches( _SystemItem2* s )
{
    QList<_StripLinkItem2*> sl;
    for( int j = 0; j < s->count(); j++ )
    {
        _StripLinkItem2* l = dynamic_cast<_StripLinkItem2*>( s->child( j ) );
        if( l )
            sl.append( l );
    }
    for( int k = 0; k < sl.size(); k++ )
    {
        sl[k]->removeMe();
        // QApplication::processEvents();
    }
}

void StripListGadget2::fillMatches( _SystemItem2* s )
{
    Stripper::MatchList l = d_pro->getStripper()->getMatches( s->d_sys );
    for( int j = 0; j < l.size(); j++ )
    {
        new _StripLinkItem2( s, l[j].first, l[j].second );
    }
    commit();
}

struct _StripListGadgetDeleg2 : public QItemDelegate
{
    _StripListGadgetDeleg2( Gui::ListView* p ):QItemDelegate(p) {}
	Gui::ListView* listView() const { return static_cast<Gui::ListView*>( parent() ); }

	void paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
	{
        if( _SpinLinkItem3* item = dynamic_cast<_SpinLinkItem3*>( listView()->indexToItem( index ) ) )
		{
			if( index.column() == 0 )
			{
                _PeakLabelItem2* _this = dynamic_cast<_PeakLabelItem2*>( item->parent() );
				assert( _this );
                const SpinLink::Alias& alias = item->d_link->getAlias( _this->d_list->getSpec() );
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

StripListGadget2::StripListGadget2(QWidget *parent,
        Root::Agent *agent, Project * pro, bool links ):Gui::ListView(parent),
    d_agent( agent ), d_pro( pro ), d_showLinks( links )
{
    setRootIsDecorated( true );
    setShowSortIndicator( true );

    setItemDelegate( new _StripListGadgetDeleg2( this ) );

    s_cl = d_pro->getRepository()->getColors();
    d_pro->getStripper()->addObserver( this );
    d_pro->addObserver( this );
    addColumn( "System" );
    addColumn( "Shift" );
    addColumn( "Fit" );
    addColumn( "Pred" );
    addColumn( "Succ" );
    addColumn( "Ass" );
    addColumn( "Cand." );
    addColumn( "Type" );
    addColumn( "Spin" );
    loadAllStrips();
    for( int col = 2; col < columns(); col++ )
        resizeColumnToContents(col);
}

StripListGadget2::~StripListGadget2()
{
    d_pro->removeObserver( this );
    d_pro->getStripper()->removeObserver( this );
}

void StripListGadget2::addCommands( Gui2::AutoMenu* menu )
{
    menu->addCommand( "Find Spin...",  this, SLOT( handleFindSpin()) );
    menu->addCommand( "Find Link Partner...",  this, SLOT( handleGotoOther()));
	menu->addSeparator();
    menu->addCommand( "Run Automatic Strip Matcher",  this, SLOT( handleRunStripper()));
    menu->addCommand( "Strict Strip Matching",  this, SLOT( handleStrictStripMatch()));
    menu->addCommand( "Unlabeled Strip Matching",  this, SLOT( handleUnlabeledStripMatch()));
    menu->addCommand( "Set Spin Tolerance...",  this, SLOT( handleSetSpinTol()));
	menu->addSeparator();
    menu->addCommand( "Create System",  this, SLOT( handleCreateSystem()));
    menu->addCommand( "Eat System...",  this, SLOT( handleEatSystem()));
    menu->addCommand( "Set Assig. Candidates...",  this, SLOT( handleSetCandidates()));
    menu->addCommand( "Set System Type...",  this, SLOT( handleSetSysType()));
    menu->addCommand( "Show Alignment...",  this, SLOT( handleShowAlignment()));
	menu->addSeparator();
    menu->addCommand( "Link This",  this, SLOT( handleLinkThis()));
    menu->addCommand( "Unlink Successor",  this, SLOT( handleUnlinkSucc()));
    menu->addCommand( "Unlink Predecessor",  this, SLOT( handleUnlinkPred()));
	menu->addSeparator();
    menu->addCommand( "Create Spin...",  this, SLOT( handleCreateSpin()));
    menu->addCommand( "Move Spin...",  this, SLOT( handleMoveSpin()));
    menu->addCommand( "Label Spin...",  this, SLOT( handleLabelSpin()));
    menu->addCommand( "Force Label...",  this, SLOT( handleForceLabel()));
    menu->addCommand( "Accept Label",  this, SLOT( handleAcceptLabel()));
	menu->addSeparator();
    menu->addCommand( "Assign to...",  this, SLOT( handleAssign()));
    menu->addCommand( "Unassign",  this, SLOT( handleUnassign()));
    menu->addCommand( "Delete",  this, SLOT( handleDelete()));
    menu->addCommand( "Edit Attributes...",  this, SLOT( handleEditAtts()));
    menu->addCommand( "Open System Table...",  this, SLOT( handleShowTable()));
	menu->addSeparator();
    menu->addCommand( "Show Spin Links",  this, SLOT( handleShowLinks()));
    menu->addCommand( "Create Link...",  this, SLOT( handleCreateLink()));
    menu->addCommand( "Set Link Params...",  this, SLOT( handleLinkParams()));
	menu->addSeparator();
    menu->addCommand( "Open All",  this, SLOT( handleOpenAll()));
    menu->addCommand( "Close All",  this, SLOT( handleCloseAll()));
}

bool StripListGadget2::gotoSpin(Spin * spin)
{
	if( spin == 0 || spin->getSystem() == 0 )
		return false;
    _SystemItem2* s = d_map[ spin->getSystem()->getId() ];
	if( s == 0 )
		return false;
    _PeakLabelItem2* p;
	for( int i = 0; i < s->count(); i++ )
	{
        p = dynamic_cast<_PeakLabelItem2*>( s->child( i ) );
		if( p && p->d_spin == spin )
		{
            ensureItemVisible( p );
            setSelected( p, true );
			return true;
		}
	}
	return false;
}

void StripListGadget2::updateItem(_SystemItem2 *s, bool repaint )
{
	// s->d_excl = !d_strips->getExcludes( s->d_id ).empty();
	if( repaint )
		s->repaint();
}

SpinSystem* StripListGadget2::getSelectedStrip()
{
    _SystemItem2* i = dynamic_cast<_SystemItem2*>( currentItem() );
	if( i != nil )
		return i->d_sys;
    _StripLinkItem2* a = dynamic_cast<_StripLinkItem2*>( currentItem() );
	if( a )
        i = dynamic_cast<_SystemItem2*>( a->parent() );
	if( i != nil )
		return i->d_sys;
    _PeakLabelItem2* b = dynamic_cast<_PeakLabelItem2*>( currentItem() );
	if( b )
        i = dynamic_cast<_SystemItem2*>( b->parent() );
	if( i != nil )
		return i->d_sys;
	else
		return 0;
}

SpinSystem* StripListGadget2::getCandSucc() const
{
    _StripLinkItem2* a = dynamic_cast<_StripLinkItem2*>( currentItem() );
	if( a != nil && a->d_fit > 0.0 )
		return a->d_sys;
	else
		return 0;
}

SpinSystem* StripListGadget2::getCandPred() const
{
    _StripLinkItem2* a = dynamic_cast<_StripLinkItem2*>( currentItem() );
	if( a != nil && a->d_fit < 0.0 )
		return a->d_sys;
	else
		return 0;
}

void StripListGadget2::showStrip(SpinSystem* i)
{
    _SystemItem2* s = d_map[ i->getId() ];
	if( s == 0 )
		return;
    setSelected( s, true );
    setOpen( s, true );
    ensureItemVisible( s );
}

Spin* StripListGadget2::getSelectedSpin() const
{
    _PeakLabelItem2* b = dynamic_cast<_PeakLabelItem2*>( currentItem() );
	if( b )
		return b->d_spin;
    _SpinLinkItem3* a = dynamic_cast<_SpinLinkItem3*>( currentItem() );
	if( a )
        b = dynamic_cast<_PeakLabelItem2*>( a->parent() );
	if( b != nil )
		return b->d_spin;
	else
		return 0;
}

Spin* StripListGadget2::getSelectedLink() const
{
    return getLink();
}

void StripListGadget2::setSpec(Spectrum * spec)
{
    if( d_spec == spec )
		return;
    d_spec = spec;
    viewport()->update();
}

void StripListGadget2::handle(Root::Message & msg)
{
	BEGIN_HANDLER();
    MESSAGE( SpinLink::Update, a, msg )
    {
        switch( a->getType() )
        {
        case SpinLink::Update::Link:
            if( d_showLinks && a->getRhs()->getSystem() )
            {
                _SystemItem2* s = d_map.value( a->getRhs()->getSystem()->getId() );
                assert( s != 0 );
                createLinkView( s->findSpin( a->getRhs() ), a->getLink() );
            }
            if( d_showLinks && a->getLhs()->getSystem() )
            {
                _SystemItem2* s = d_map.value( a->getLhs()->getSystem()->getId() );
                assert( s != 0 );
                createLinkView( s->findSpin( a->getLhs() ), a->getLink() );
            }
            break;
        case SpinLink::Update::Unlink:
            if( d_showLinks && a->getRhs()->getSystem() )
            {
                _SystemItem2* s = d_map.value( a->getRhs()->getSystem()->getId() );
                assert( s != 0 );
                _SpinLinkItem3* l = s->findLink( a->getRhs(), a->getLhs() ); // getLink liefert hier 0
                assert( l != 0 );
                l->removeMe();
            }
            if( d_showLinks && a->getLhs()->getSystem() )
            {
                _SystemItem2* s = d_map.value( a->getLhs()->getSystem()->getId() );
                assert( s != 0 );
                _SpinLinkItem3* l = s->findLink( a->getLhs(), a->getRhs() ); // dito
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
                    Gui::ListViewItem* i = d_map[ a->getSpin()->getSystem()->getId() ];
                    _PeakLabelItem2* p;
                    for( int j = 0; i && j < i->count(); j++ )
                    {
                        p = dynamic_cast<_PeakLabelItem2*>( i->child( j ) );
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
            updateItem( d_map[ a->getSystem()->getId() ] );
            break;
        case SpinSystem::Update::Create:
            createSysView( this, a->getSystem() );
            break;
        case SpinSystem::Update::Delete:
            d_map[ a->getSystem()->getId() ]->removeMe();
            d_map.remove( a->getSystem()->getId() );
            break;
        case SpinSystem::Update::Add:
            createSpinView( d_map[ a->getSystem()->getId() ], a->getSpin() );
            break;
        case SpinSystem::Update::Remove:
            {
                _SystemItem2* s = d_map.value( a->getSystem()->getId() );
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
                _SystemItem2* s = d_map.value( a->getSys()->getId() );
                if( s )
                {
//                    clearMatches(s);
//                    fillMatches(s);
                    _StripLinkItem2* l = s->findStripLink( a->getOther() );
                    if( l )
                        l->d_fit = a->getFit();
                    else
                    {
                        new _StripLinkItem2( s, a->getOther(), a->getFit() );
                        commit();
                        // Da sonst bei unmittelbarem Unmatch der Link noch nicht im parent ist
                    }
                }
            }
            break;
        case Stripper::Update::Unmatch:
            {
                _SystemItem2* s = d_map.value( a->getSys()->getId() );
                if( s )
                {
//                    clearMatches(s);
//                    fillMatches(s);
                    _StripLinkItem2* l = s->findStripLink( a->getOther() );
                   //assert( l != 0 );
                    if( l != 0 )
                        l->removeMe();
                    else
                        // TODO: das passiert wenn man Spins lscht und mit Undo wieder einfgt
                        qDebug() << "StripListGadget2::handle Stripper::Update::Unmatch Sys" << a->getSys()->getId() << "Other" << a->getOther()->getId();
                }
            }
            break;
        case Stripper::Update::MatchAll:
            // Auch bei Stripper On aufgerufen
            if( count() == 0 )
                loadAllStrips();
            else
                for( int i = 0; i < count(); i++ )
                {
                    _SystemItem2* s = static_cast<_SystemItem2*>( child( i ) );
                    fillMatches( s );
                }
            break;
        case Stripper::Update::Clear:
            //loadAllStrips();
            // Auch bei Stripper Off aufgerufen
            for( int i = 0; i < count(); i++ )
            {
                _SystemItem2* s = static_cast<_SystemItem2*>( child( i ) );
                clearMatches( s );
            }
            break;
        }
    }
    END_HANDLER();
}

void StripListGadget2::handleEditAtts()
{
	Spin* spin = getSelectedSpin();
    SpinSystem* strip = getSys();
    Spin* link = getLink();
    ENABLED_IF(spin != 0 || strip != 0 || link != 0 );
	if( link )
        DynValueEditor::edit( this, d_pro->getRepository()
			->findObjectDef( Repository::keyLink ), spin->findLink( link ) );
	else if( spin )
        DynValueEditor::edit( this, d_pro->getRepository()
			->findObjectDef( Repository::keySpin ), spin );
	else if( strip )
        DynValueEditor::edit( this, d_pro->getRepository()
			->findObjectDef( Repository::keySpinSystem ), strip );
}

void StripListGadget2::handleSetCandidates()
{
    SpinSystem* sys = getSys();
    ENABLED_IF(sys != 0);

    CandidateDlg dlg( this, d_pro->getRepository() );
	dlg.setTitle( sys );
	if( dlg.exec() )
        d_pro->getSpins()->setCands( sys, dlg.d_cands );
}

void StripListGadget2::handleSetSpinTol()
{
	Spin* spin = getSelectedSpin();
    ENABLED_IF(spin != 0 );

	AtomType t = spin->getAtom();
	bool ok	= FALSE;
	QString res;
    res.sprintf( "%0.3f", d_pro->getMatcher()->getTol( t ) );
	res	= QInputDialog::getText( "Set Spin Tolerance", 
		"Please	enter a positive PPM value:", QLineEdit::Normal, 
        res, &ok, this );
	if( !ok )
		return;
	PPM w = res.toFloat( &ok );
	if( !ok || w <= 0.0 )
	{
        QMessageBox::critical( this, "Set Spin Tolerance",
				"Invalid tolerance!", "&Cancel" );
		return;
	}
    d_pro->getMatcher()->setTol( t, w );
    d_pro->getStripper()->recalcAll();
}

void StripListGadget2::handleStrictStripMatch()
{
    Stripper* s = d_pro->getStripper();
    CHECKED_IF(true, s->isStrict() );

	s->setStrict( !s->isStrict() );
}

void StripListGadget2::handleUnlabeledStripMatch()
{
    Stripper* s = d_pro->getStripper();
    CHECKED_IF(true, s->isUnlabeled() );

	s->setUnlabeled( !s->isUnlabeled() );
	if( s->isUnlabeled() )
		s->setStrict( false );
}

void StripListGadget2::handleRunStripper()
{
    Stripper* s = d_pro->getStripper();
    CHECKED_IF(true, s->isOn() );

	s->setOn( !s->isOn() );
}

void StripListGadget2::handleUnassign()
{
	Spin* spin = getSelectedSpin();
    SpinSystem* sys = getSys();
    ENABLED_IF( (spin != 0 && spin->getSystem() ) ||
        ( sys != 0 && sys->getAssig() != 0 ) );
	if( spin )
	{
		Root::Ref<UnassignSpinCmd> cmd = new UnassignSpinCmd( 
            d_pro->getSpins(), spin );
        cmd->handle( d_agent );
	}else
	{
		Root::Ref<UnassignSystemCmd> cmd =
            new UnassignSystemCmd( d_pro->getSpins(), sys );
        cmd->handle( d_agent );
	}
}

void StripListGadget2::handleAssign()
{
	Spin* spin = getSelectedSpin();
    SpinSystem* sys = getSys();
    ENABLED_IF(spin != 0 || sys != 0 );
	if( spin )
	{
		bool ok = false;
		int res = QInputDialog::getInteger( "Select Spin System",
            "Enter system id:", 0, 1, 99999999, 1, &ok, this );
		if( !ok )
			return;
        SpinSystem* target = d_pro->getSpins()->getSystem( res );
		if( target == 0 )
		{
            if( QMessageBox::warning( this, "Assign Spin",
					  "The selected spin system doesn't exist. Do you want to create it?",
					  "&OK", "&Cancel" ) != 0 )	
					  return;
            target = d_pro->getSpins()->addSystem( res );
		}else if( target == sys )
		{
            QMessageBox::critical( this, "Assign Spin",
				"Spin is already part of this system!", "&Cancel" );
			return;
		}
		Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Reassign Spin" );
        cmd->add( new UnassignSpinCmd( d_pro->getSpins(), spin ) );
        cmd->add( new AssignSpinCmd( d_pro->getSpins(), spin, target ) );
        cmd->handle( d_agent );
	}else
	{
		bool ok;
		QString str;
		str.sprintf( "Assignment for spin system %d:", sys->getId() );
		int r = QInputDialog::getInteger( "Assign Spin System", 
			str, (sys->getAssig())?sys->getAssig()->getId():0, 
            -999999, 999999, 1, &ok, this );
		if( !ok )
			return;

        Residue* res = d_pro->getSequence()->getResidue( r );
		if( res == 0 )
		{
            QMessageBox::critical( this, "Assign Spin System",
				"Unknown residue number!", "&Cancel" );
			return;
		}
		Root::Ref<AssignSystemCmd> cmd =
            new AssignSystemCmd( d_pro->getSpins(), sys, res );
        cmd->handle( d_agent );
	}
}

void StripListGadget2::handleUnlinkPred()
{
    SpinSystem* id = getSys();
	if( id == 0 )
		return;
	SpinSystem* other = id->getPred();

    ENABLED_IF(id != 0 && other != 0 );


	Root::Ref<UnlinkSystemCmd> cmd =
        new UnlinkSystemCmd( d_pro->getSpins(), other, id );
    cmd->handle( d_agent );
}

void StripListGadget2::handleUnlinkSucc()
{
    SpinSystem* id = getSys();
	if( id == 0 )
		return;
	SpinSystem* other = id->getSucc();

    ENABLED_IF(id != 0 && other != 0 );


	Root::Ref<UnlinkSystemCmd> cmd =
        new UnlinkSystemCmd( d_pro->getSpins(), id, other );
    cmd->handle( d_agent );
}

void StripListGadget2::handleShowAlignment()
{
    SpinSystem* id = getSys();
	SpinSystem* p = getCandPred();
	SpinSystem* s = getCandSucc();
    ENABLED_IF(p != 0 || s != 0 || id != 0 );

	SpinSystemString fra;
	if( id != 0 ) // das selektierte Item ist ein Strip
        d_pro->getSpins()->fillString( id, fra );
	else if( p != 0 )
        d_pro->getSpins()->fillString( p, getSelectedStrip(), fra );
	else 
        d_pro->getSpins()->fillString( getSelectedStrip(), s, fra );

	FragmentAssignment* f = new FragmentAssignment( 
        d_pro->getSpins(), d_pro->getMatcher(), fra );
    SingleAlignmentView* v = new SingleAlignmentView( d_agent, f );
	v->show();
}

void StripListGadget2::handleLinkThis()
{
    Spin* link = getLink();
    ENABLED_IF(getCandSucc() != 0 || getCandPred() != 0 || link );

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
        int res = Dlg::getOption( this, l, "Select Direction" );
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
            new LinkSystemCmd( d_pro->getSpins(), pred, succ );
        cmd->handle( d_agent );
    }catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Link This", e.what() );
	}
}

void StripListGadget2::handleDelete()
{
	Spin* spin = getSelectedSpin();
    SpinSystem* sys = getSys();
    Spin* link = getLink();
    ENABLED_IF(spin != 0 || sys != 0 || link != 0 );
	if( link )
	{
		Root::Ref<UnlinkSpinCmd> cmd = new UnlinkSpinCmd( 
            d_pro->getSpins(), spin, link );
        cmd->handle( d_agent );
	}else if( spin )
	{
        Root::Ref<DeleteSpinCmd> cmd = new DeleteSpinCmd( d_pro->getSpins(), spin );
        cmd->handle( d_agent );
	}else
	{
		if( sys->getPred() == 0 && sys->getSucc() == 0 && sys->getAssig() == 0 )
		{
			Root::Ref<DeleteSystemCmd> cmd = new DeleteSystemCmd( 
                d_pro->getSpins(), sys );
            cmd->handle( d_agent );
		}else
            QMessageBox::critical( this, "Delete Spin System",
				"Cannot delete this system because there are still assignments!", "&Cancel" );
	}
}

void StripListGadget2::handleSetSysType()
{
    SpinSystem* sys = getSys();
    ENABLED_IF(sys );

	Repository::SystemTypeMap::const_iterator p;
    const Repository::SystemTypeMap& sm = d_pro->getRepository()->getSystemTypes();

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
        l, cur, false, &ok, this );
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
        new ClassifySystemCmd( d_pro->getSpins(), sys, sst );
    cmd->handle( d_agent );
}

void StripListGadget2::handleMoveSpinAlias()
{
	// TODO
}

void StripListGadget2::handleMoveSpin()
{
	Spin* spin = getSelectedSpin();
    ENABLED_IF(spin );

	bool ok;
	QString res;
	res.sprintf( "%f", spin->getShift() );
	res = QInputDialog::getText( "Move Spin", 
		"Please	enter a new PPM value for spin:", QLineEdit::Normal, 
        res, &ok, this );
	if( !ok )
		return;
	PPM p = res.toFloat( &ok );
	if( !ok )
	{
        QMessageBox::critical( this, "Move Spin", "Invalid ppm value!", "&Cancel" );
		return;
	}

	Root::Ref<MoveSpinCmd> cmd =
        new MoveSpinCmd( d_pro->getSpins(), spin, p, 0 );
		// Move generisches Spektrum
    cmd->handle( d_agent );
}

void StripListGadget2::handleLabelSpin()
{
	Spin* spin = getSelectedSpin();
    ENABLED_IF(spin );

	SpinLabel l = spin->getLabel();
    ResidueType* r = d_pro->inferResiType( spin->getSystem() );
	SpinLabelSet ll;
	if( r )
		ll = r->findAll( spin->getAtom() );
    if( !Dlg::getLabel( this, l, ll ) )
		return;
	Root::Ref<LabelSpinCmd> cmd =
        new LabelSpinCmd( d_pro->getSpins(), spin, l );
    cmd->handle( d_agent );
}

void StripListGadget2::handleCreateSpin()
{
	SpinSystem* sys = getSelectedStrip();
    ENABLED_IF(sys );

	Dlg::SpinParams sp;
    if( !Dlg::getSpinParams( this, sp ) )
		return;
	Root::Ref<PickSystemSpinLabelCmd> cmd = new PickSystemSpinLabelCmd( 
        d_pro->getSpins(), sys, // Darf null sein.
		sp.d_type, sp.d_shift, sp.d_label, 0 ); 
		// Pick generisches Spektrum
    cmd->handle( d_agent );
}

void StripListGadget2::handleForceLabel()
{
	Spin* spin = getSelectedSpin();
    ENABLED_IF(spin );

	bool ok	= FALSE;
	QString res;
	res.sprintf( "Please enter a valid label (%s):", SpinLabel::s_syntax );
	res = QInputDialog::getText( "Force Spin Label", res, QLineEdit::Normal, 
            spin->getLabel().data(), &ok, this );
	if( !ok )
		return;

	SpinLabel l;
	if( !SpinLabel::parse( res, l ) )
	{
        QMessageBox::critical( this, "Force Spin Label",
			"Invalid spin label syntax!", "&Cancel" );
		return;
	}

	Root::Ref<LabelSpinCmd> cmd =
        new LabelSpinCmd( d_pro->getSpins(), spin, l );
    cmd->handle( d_agent );
}

void StripListGadget2::handleCreateSystem()
{
    ENABLED_IF(true );
	Root::Ref<CreateSystemOnlyCmd> cmd =
        new CreateSystemOnlyCmd( d_pro->getSpins() );
    cmd->handle( d_agent );
    Gui::ListViewItem* item = d_map[ cmd->getSystem()->getId() ];
    setSelected( item, true );
    ensureItemVisible( item );
}

void StripListGadget2::handleEatSystem()
{
    SpinSystem* sys = getSys();
    ENABLED_IF(sys );
	bool ok = false;
	int res = QInputDialog::getInteger( "Select Spin System",
        "Enter the ID of the spin system to eat:", 0, 1, 99999999, 1, &ok, this );
	if( !ok )
		return;
    SpinSystem* source = d_pro->getSpins()->getSystem( res );
	if( source == 0 )
	{
        QMessageBox::critical( this, "Eat Spin System",
			"Spin system does not exist!", "&Cancel" );
		return;
	}
	if( source->getPred() != 0 || source->getSucc() != 0 || source->getAssig() != 0 )
	{
        QMessageBox::critical( this, "Eat Spin System",
			"Cannot delete source system because there are still assignments!", "&Cancel" );
		return;
	}
	SpinSystem::Spins::const_iterator p1;
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Eat Spin System" );
	for( p1 = source->getSpins().begin(); p1 != source->getSpins().end(); ++p1 )
	{
		if( !sys->isAcceptable( (*p1)->getLabel() ) )
		{
            QMessageBox::critical( this, "Eat Spin System",
				"One of the spins in the source system is not acceptable by the target system!", "&Cancel" );
			return;
		}
        cmd->add( new UnassignSpinCmd( d_pro->getSpins(), (*p1) ) );
        cmd->add( new AssignSpinCmd( d_pro->getSpins(), (*p1), sys ) );
	}
    cmd->add( new DeleteSystemCmd( d_pro->getSpins(), source ) );
    cmd->handle( d_agent );
}

void StripListGadget2::handleCreateLink()
{
	// TODO
	Spin* spin = getSelectedSpin();
    ENABLED_IF(spin );
	bool ok = false;
	int res = QInputDialog::getInteger( "Link Spin",
        "Enter the ID of the spin to link to:", 0, 1, 99999999, 1, &ok, this );
	if( !ok )
		return;
    Spin* to = d_pro->getSpins()->getSpin( res );
	if( to == 0 )
	{
        QMessageBox::critical( this, "Link Spin",
			"Target spin does not exist!", "&Cancel" );
		return;
	}else if( to == spin )
	{
        QMessageBox::critical( this, "Link Spin",
			"Cannot link to itself!", "&Cancel" );
		return;
	}else if( to->findLink( spin ) != 0 )
	{
        QMessageBox::critical( this, "Link Spin",
			"Spins are already linked!", "&Cancel" );
		return;
	}
	Root::Ref<LinkSpinCmd> cmd =
        new LinkSpinCmd( d_pro->getSpins(), spin, to );
    cmd->handle( d_agent );
}

void StripListGadget2::handleShowLinks()
{
    CHECKED_IF(true, d_showLinks );
    d_showLinks = !d_showLinks;
    loadAllStrips();
}

void StripListGadget2::handleFindSpin()
{
    ENABLED_IF(true );
	bool ok = false;
	int res = QInputDialog::getInteger( "Find Spin",
        "Enter the ID of the spin to find:", 0, 1, 99999999, 1, &ok, this );
	if( !ok )
		return;
    Spin* to = d_pro->getSpins()->getSpin( res );
	if( to == 0 )
	{
        QMessageBox::critical( this, "Find Spin",
			"Spin does not exist!", "&Cancel" );
		return;
	}
	gotoSpin( to );
}

void StripListGadget2::handleGotoOther()
{
    Spin* spin = getLink();
    ENABLED_IF(spin );
	gotoSpin( spin );
}

void StripListGadget2::handleAcceptLabel()
{
	Spin* spin = getSelectedSpin();
    ENABLED_IF(spin && spin->getSystem() &&
		!spin->getLabel().isNull() &&
		!spin->getLabel().isFinal() );

	SpinSystem* sys = spin->getSystem();
	SpinSystem::Spins::const_iterator i;
	SpinLabel l;
	Root::Ref<Root::MakroCommand> cmd = new Root::MakroCommand( "Accept Label" ); 
	for( i = sys->getSpins().begin(); i != sys->getSpins().end(); i++ )
	{
		if( (*i) != spin && (*i)->getLabel() == spin->getLabel() )
            cmd->add( new LabelSpinCmd( d_pro->getSpins(), (*i), l ) );
	}
	l = spin->getLabel();
	l.setFinal();
    cmd->add( new LabelSpinCmd( d_pro->getSpins(), spin, l ) );
    cmd->handle( d_agent );
}

void StripListGadget2::handleShowTable()
{
    ENABLED_IF(true );

	int i = 0;
    const SpinBase::SpinSystemMap& sm = d_pro->getSpins()->getSystems();
	SpinBase::SpinSystemMap::const_iterator p;
	ObjectListView::ObjectList o( sm.size() );
	for( p = sm.begin(); p != sm.end(); ++p, ++i )
		o[ i ] = (*p).second;
    ObjectListView::edit( this, d_pro->getRepository()
		->findObjectDef( Repository::keySpinSystem ), o );
}

void StripListGadget2::handleOpenAll()
{
    ENABLED_IF(true );
    Gui::ListViewItemIterator it( this );
    for( ; it.current(); ++it )
        it.current()->setOpen( true );
}

void StripListGadget2::handleCloseAll()
{
    ENABLED_IF(true );
    Gui::ListViewItemIterator it( this );
    for( ; it.current(); ++it )
        it.current()->setOpen( false );
}

void StripListGadget2::handleLinkParams()
{
	Spin* spin = getSelectedSpin();
    Spin* link = getLink();
    ENABLED_IF(link != 0 );

	SpinLink* sl = spin->findLink( link );
	assert( sl );
    const SpinLink::Alias& al = sl->getAlias( d_spec );
	Dlg::LinkParams2 par;
	par.d_rating = al.d_rating;
	par.d_code = al.d_code;
	par.d_visible = al.d_visible;
    if( Dlg::getLinkParams2( this, par ) )
        d_pro->getSpins()->setAlias( sl, d_spec,
		par.d_rating, par.d_code, par.d_visible );
	// TODO: Undo
}

void StripListGadget2::onCurrentChanged()
{
    if( _SystemItem2* i = dynamic_cast<_SystemItem2*>( currentItem() ) )
    {
        LuaCaraExplorer::setCurrentSystem( i->d_sys );
    }else if( _SpinLinkItem3* i = dynamic_cast<_SpinLinkItem3*>( currentItem() ) )
    {
        LuaCaraExplorer::setCurrentSpinLink( i->d_link );
    }else if( _StripLinkItem2* i = dynamic_cast<_StripLinkItem2*>( currentItem() ) )
    {
        LuaCaraExplorer::setCurrentSystem( i->d_sys );
    }else if( _PeakLabelItem2* i = dynamic_cast<_PeakLabelItem2*>( currentItem() ) )
    {
        LuaCaraExplorer::setCurrentSpin( i->d_spin );
    }
}

