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

#include "SpinLinkListView.h"
#include <qinputdialog.h>
#include <qmessagebox.h>
#include <qlabel.h>
#include <qlineedit.h> 
#include <qlayout.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qcheckbox.h> 
#include <Root/Any.h>
#include <Spec/Repository.h>
#include <AidaCentral.h>
#include <Dlg.h>
#include <SpecView/DynValueEditor.h>
#include <SpecView/ObjectListView.h>
#include <Root/Vector.h>
#include <stdio.h>
#include "LuaCaraExplorer.h"
using namespace Spec;
 
static char s_buf[64];

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr SpinLinkListView::ShowTable = "ShowTable";
Root::Action::CmdStr SpinLinkListView::EditAtts = "EditAtts";
Root::Action::CmdStr SpinLinkListView::Delete = "Delete";
Root::Action::CmdStr SpinLinkListView::Create = "Create";

ACTION_SLOTS_BEGIN( SpinLinkListView )
    { SpinLinkListView::Delete, &SpinLinkListView::handleDelete },
    { SpinLinkListView::Create, &SpinLinkListView::handleCreate },
    { SpinLinkListView::ShowTable, &SpinLinkListView::handleShowTable },
    { SpinLinkListView::EditAtts, &SpinLinkListView::handleEditAtts },
ACTION_SLOTS_END( SpinLinkListView )

//////////////////////////////////////////////////////////////////////

static const int s_count = 6; 
static const int s_off = 10000; // Wegen Sortierung

struct _SpinLinkItem2 : public Gui::ListViewItem
{
	_SpinLinkItem2( Gui::ListView * parent, Spin* lhs, Spin* rhs, SpinLink* l ):
		Gui::ListViewItem( parent ), d_lhs( lhs ), d_rhs( rhs ), d_link( l ) {}
	Root::ExRef<Spin> d_lhs;
	Root::ExRef<Spin> d_rhs;
	Root::Ref<SpinLink> d_link;

	QString text( int f ) const
	{
		Spin* s = d_lhs;
		if( f >= s_count )
		{
			f = f % s_count;
			s = d_rhs;
		}
		*s_buf = 0;
		switch( f )
		{
		case 0:
			::sprintf( s_buf, "%d", s->getId() );
			return s_buf;
		case 1:
			::sprintf( s_buf, "%s", s->getAtom().getIsoLabel() ); 
			return s_buf;
		case 2:
			s->getLabel().format( s_buf, sizeof( s_buf ) );
			return s_buf;
		case 3:
			::sprintf( s_buf, "%.3f", s->getShift() ); 
			return s_buf;
		case 4:
			if( s->getSystem() )
				::sprintf( s_buf, "%d", s->getSystem()->getId() ); 
			return s_buf;
		case 5:
			if( s->getSystem() && s->getSystem()->getAssig() != 0 )
				s->getSystem()->formatLabel( s_buf, sizeof(s_buf) );
			return s_buf;
		}
		return "";
	}
	QString key( int f, bool ) const
	{
		Spin* s = d_lhs;
		int ff = f;
		if( f >= s_count )
		{
			ff = f % s_count;
			s = d_rhs;
		}
		*s_buf = 0;
		switch( ff )
		{
		case 0:
			::sprintf( s_buf, "%08d", s->getId() );
			return s_buf;
		case 3:
			if( s->getShift() < 0.0 )
				::sprintf( s_buf, "A%08.3f", s->getShift() ); 
			else
				::sprintf( s_buf, "B%08.3f", s->getShift() ); 
			return s_buf;
		case 4:
			if( s->getSystem() )
				::sprintf( s_buf, "%08d", s->getSystem()->getId() ); 
			return s_buf;
		case 5:
			if( s->getSystem() && s->getSystem()->getAssig() != 0 )
				::sprintf( s_buf, "%s%09d", s->getSystem()->getAssig()->getChain().data(),
					s->getSystem()->getAssig()->getNr() + s_off );
			return s_buf;
		default:
			return text( f );
		}
		return "";
	}
};

SpinLinkListView::SpinLinkListView(QWidget* p, AidaCentral* par, Spec::Project* r):
	Gui::ListView( p ), d_pro( r ), d_parent( par )
{
	setShowSortIndicator( true );
	setRootIsDecorated( true );
	setAllColumnsShowFocus( true );
	addColumn( "Spin L" );
	addColumn( "Atom" );
	addColumn( "Label" );
	addColumn( "Shift" );
	addColumn( "Sys." );
	addColumn( "Assig." );
	addColumn( "Spin R" );
	addColumn( "Atom" );
	addColumn( "Label" );
	addColumn( "Shift" );
	addColumn( "Sys." );
	addColumn( "Assig." );
	refill();

	d_pro->addObserver( this );
}

SpinLinkListView::~SpinLinkListView()
{
	d_pro->removeObserver( this );
}

void SpinLinkListView::onCurrentChanged()
{
	if( _SpinLinkItem2* item = dynamic_cast<_SpinLinkItem2*>( currentItem() ) )
		LuaCaraExplorer::setCurrentSpinLink( item->d_link );
}

Gui::Menu* SpinLinkListView::createPopup()
{
	Gui::Menu* pop = new Gui::Menu();
	Gui::Menu::item( pop, "Create Link...", Create, false );
	Gui::Menu::item( pop, "Delete Link", Delete, false );
	pop->insertSeparator();
	Gui::Menu::item( pop, "Edit Attributes...", EditAtts, false );
	Gui::Menu::item( pop, "Open Object Table...", ShowTable, false );
	return pop;
}

void SpinLinkListView::refill()
{
	clear();
	const SpinBase::SpinLinkSet& sm = d_pro->getSpins()->getLinks();
	SpinBase::SpinLinkSet::const_iterator p;
	Spin* lhs;
	Spin* rhs;
	for( p = sm.begin(); p != sm.end(); ++p )
	{
		lhs = d_pro->getSpins()->getSpin( (*p)->getLhs() );
		rhs = d_pro->getSpins()->getSpin( (*p)->getRhs() );
		if( lhs && rhs )
			new _SpinLinkItem2( this , lhs, rhs, (*p) );
	}
}

static _SpinLinkItem2* _findItem( Gui::ListView* lv, Spin* spin )
{

    Gui::ListViewItemIterator it( lv );
    // iterate through all items of the listview
	_SpinLinkItem2* i = 0;
    for ( ; it.current(); ++it ) 
	{
		i = dynamic_cast<_SpinLinkItem2*>( it.current() );
		if( i && ( i->d_lhs == spin || i->d_rhs == spin ) )
			return i;
    }
	return 0;
}

static _SpinLinkItem2* _findItem( Gui::ListView* lv, Spin* lhs, Spin* rhs )
{

    Gui::ListViewItemIterator it( lv );
    // iterate through all items of the listview
	_SpinLinkItem2* i = 0;
    for ( ; it.current(); ++it ) 
	{
		i = dynamic_cast<_SpinLinkItem2*>( it.current() );
        if( i && ( ( i->d_lhs == lhs && i->d_rhs == rhs ) ||
            ( i->d_lhs == rhs && i->d_rhs == lhs ) ) )
			return i;
    }
	return 0;
}

void SpinLinkListView::handle(Root::Message & msg)
{
	if( parent() == 0 )
		return;
	BEGIN_HANDLER();
	MESSAGE( Spin::Update, a, msg )
	{
		switch( a->getType() )
		{
		case Spin::Update::Delete:
			{
				_SpinLinkItem2* i = _findItem( this, a->getSpin() );
				if( i )
					i->removeMe();
			}
			break;
		case Spin::Update::Shift:
			{
				_SpinLinkItem2* i = _findItem( this, a->getSpin() );
				if( i )
					i->repaint();
			}
			break;
		case Spin::Update::Label:
		case Spin::Update::System:
			{
				_SpinLinkItem2* i = _findItem( this, a->getSpin() );
				if( i )
					i->repaint();
			}
			break;
		case Spin::Update::All:
			refill();
			break;
        default:
            break;
		}
		msg.consume();
	}
	MESSAGE( SpinLink::Update, a, msg )
	{
		switch( a->getType() )
		{
		case SpinLink::Update::Link:
			new _SpinLinkItem2( this , a->getLhs(), a->getRhs(), a->getLink() );
			break;
		case SpinLink::Update::Unlink:
			{
				_SpinLinkItem2* i = _findItem( this, a->getLhs(), a->getRhs() );
				if( i )
					i->removeMe();
			}
			break;
		case SpinLink::Update::All:
			refill();
			break;
        default:
            break;
		}
		msg.consume();
	}
	MESSAGE( Root::Action, a, msg )
	{
		EXECUTE_ACTION( SpinLinkListView, *a );
	}
	END_HANDLER();
}

void SpinLinkListView::handleEditAtts(Root::Action & a)
{
	_SpinLinkItem2* item = dynamic_cast<_SpinLinkItem2*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	DynValueEditor::edit( this, d_parent->getRep()->findObjectDef( Repository::keyLink ),
		item->d_lhs->findLink( item->d_rhs ) );
}

void SpinLinkListView::handleShowTable(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );

	int i = 0;
	const SpinBase::SpinLinkSet& sm = d_pro->getSpins()->getLinks();
	SpinBase::SpinLinkSet::const_iterator p;
	ObjectListView::ObjectList o( sm.size() );
	for( p = sm.begin(); p != sm.end(); ++p, ++i )
		o[ i ] = (*p);
	ObjectListView::edit( this, d_pro->getRepository()
		->findObjectDef( Repository::keyLink ), o );
}

void SpinLinkListView::handleDelete(Root::Action & a)
{
	_SpinLinkItem2* item = dynamic_cast<_SpinLinkItem2*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	Root::Ref<UnlinkSpinCmd> cmd = new UnlinkSpinCmd( d_pro->getSpins(), 
		item->d_lhs, item->d_rhs );
	cmd->handle( d_parent );
}

void SpinLinkListView::handleCreate(Root::Action & a)
{
	// TODO
}

/* TODO: Freds Wünsche
select spin links involving only one system.
(e.g. Sys# 3: any spinlink including atoms from this
system are listed)

remove all displayed spinlinks
import & export spinlink function would be nice.

special commands to clean up spinlink lists:
delete spinlinks between atom types X and Y. (e.g. H & N)
delete spinlinks within residues 
*/



