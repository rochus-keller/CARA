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

#include "PeakListListView.h"
#include <qinputdialog.h>
#include <qmessagebox.h>
#include <stdio.h>
#include <QApplication>
#include <Root/Application.h>
#include <Root/Any.h>
#include <Lexi/GlyphWidget.h>
#include <AidaCentral.h>
#include <Spec/PeakListPeer.h>
#include <Spec/PeakProjector.h>
#include <SpecView/DynValueEditor.h>
#include "PeakListGadget.h"
#include <SpecView/ObjectListView.h>
#include "LuaCaraExplorer.h"
using namespace Spec;

static char s_buf[64];


//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr PeakListListView::OpenMonoScope = "OpenMonoScope";
Root::Action::CmdStr PeakListListView::Remove = "Remove";
Root::Action::CmdStr PeakListListView::EditAtts = "EditAtts";
Root::Action::CmdStr PeakListListView::ShowTable = "ShowTable";

ACTION_SLOTS_BEGIN( PeakListListView )
    { PeakListListView::ShowTable, &PeakListListView::handleShowTable },
    { PeakListListView::EditAtts, &PeakListListView::handleEditAtts },
    { PeakListListView::Remove, &PeakListListView::handleRemove },
    { PeakListListView::OpenMonoScope, &PeakListListView::handleOpenMonoScope },
ACTION_SLOTS_END( PeakListListView )

//////////////////////////////////////////////////////////////////////

class _PeakListListItem : public Gui::ListViewItem
{
	Root::ExRef<PeakList> d_pl;
public:
	_PeakListListItem( Gui::ListView* p, PeakList* pl ):Gui::ListViewItem(p),d_pl( pl ) {}

	PeakList* getList() const { return d_pl; }

	QString text( int f ) const 
	{ 
		switch( f )
		{
		case 0:
			return d_pl->getName().data();
		case 1:
			::sprintf( s_buf, "%d", d_pl->getId() );
			return s_buf;
		case 2:
			::sprintf( s_buf, "%d", d_pl->getDimCount() );
			return s_buf;
		case 3:
			::sprintf( s_buf, "%d", d_pl->getHome() );
			return s_buf;
		default:
			return "";
		}
	}
	QString key( int f, bool ascending  ) const
	{
		switch( f )
		{
		case 0:
			return d_pl->getName().data();
		case 1:
			::sprintf( s_buf, "%8d", d_pl->getId() );
			return s_buf;
		case 2:
			::sprintf( s_buf, "%8d", d_pl->getDimCount() );
			return s_buf;
		case 3:
			::sprintf( s_buf, "%8d", d_pl->getHome() );
			return s_buf;
		default:
			return "";
		}
	}
	// const QPixmap *pixmap( int i ) const { return s_pixRepository; }
};

struct _PeakListListItemLabel : public Gui::ListViewItem
{
	_PeakListListItemLabel( Gui::ListViewItem* p, AtomType at ):Gui::ListViewItem(p),d_at( at ) {}
	AtomType d_at;
	QString text( int f ) const 
	{ 
		return d_at.getIsoLabel();
	}
};

//////////////////////////////////////////////////////////////////////


PeakListListView::PeakListListView(QWidget* v,AidaCentral* c, Spec::Repository* r,Spec::Project* p):
	Gui::ListView( v ), d_rep( r ), d_pro( p ), d_central( c )
{
	setShowSortIndicator( true );
	setRootIsDecorated( true );
	setAllColumnsShowFocus( true );
	addColumn( "Name" );
	addColumn( "ID" );
	addColumn( "Dim" );
	addColumn( "Owner" );
	refill();
	d_pro->addObserver( this );
}

PeakListListView::~PeakListListView()
{
	d_pro->removeObserver( this );
}

Gui::ListViewItem* PeakListListView::addItem( PeakList * sp)
{
	_PeakListListItem* i = new _PeakListListItem( this, sp );
	for( Dimension d = 0; d < sp->getDimCount(); d++ )
	{
		new _PeakListListItemLabel( i, sp->getColors()[ d ] );
	}
	i->setCurrent();
	return i;
}

void PeakListListView::refill()
{
	clear();
	const Project::PeakListMap& sm = d_pro->getPeakLists();
	Project::PeakListMap::const_iterator p;
	for( p = sm.begin(); p != sm.end(); ++p )
	{
		addItem( (*p).second );
	}
}

Gui::Menu* PeakListListView::createPopup()
{
	Gui::Menu* pop = new Gui::Menu();
	Gui::Menu::item( pop, "&Open MonoScope...", OpenMonoScope, false );
	Gui::Menu::item( pop, "Remove Peaklist...", Remove, false );
	Gui::Menu::item( pop, "Edit Attributes...", EditAtts, false );
	Gui::Menu::item( pop, "Open Object Table...", ShowTable, false );
	return pop;
}

void PeakListListView::handle(Root::Message & msg)
{
	BEGIN_HANDLER();
	MESSAGE( PeakList::Added, a, msg )
	{
		addItem( a->sender() );
	}
	MESSAGE( PeakList::Removed, a, msg )
	{
		for( int i = 0; i < count(); i++ )
			if( static_cast<_PeakListListItem*>( child( i ) )->getList() == a->sender() )
			{
				child(i)->removeMe();
				break;
			}
	}
	MESSAGE( Root::Action, a, msg )
	{
		EXECUTE_ACTION( PeakListListView, *a );
	}
	END_HANDLER();
}

void createMonoScope( Root::Agent* a, Spec::Spectrum* s, 
					 Spec::Project* p, PeakList* l, const Rotation& r );

void PeakListListView::handleOpenMonoScope(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );

	_PeakListListItem* i = (_PeakListListItem*) currentItem();
	PeakList* l = i->getList();
	Spectrum* s = d_pro->getSpec( l->getHome() );
	if( s == 0 )
	{
		QMessageBox::critical( this, "Open MonoScope", 
			"Invalid home spectrum", "&Cancel" );
		return;
	}
	QApplication::setOverrideCursor( Qt::waitCursor );
	createMonoScope( d_central, s, d_pro, l, Rotation() );
	QApplication::restoreOverrideCursor();
}

void PeakListListView::handleRemove(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );

	_PeakListListItem* i = (_PeakListListItem*) currentItem();
	PeakList* l = i->getList();
	if( QMessageBox::warning( this, "Remove Peaklist",
		"Do you really want to remove the peaklist (cannot be undone)?",
		"&OK", "&Cancel" ) != 0 )
		return;

	d_pro->removePeakList( l );
}

void PeakListListView::handleEditAtts(Root::Action & a)
{
	_PeakListListItem* item = dynamic_cast<_PeakListListItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	DynValueEditor::edit( this, d_rep->findObjectDef( Repository::keyPeaklist ),
		item->getList() );
}

void PeakListListView::onCurrentChanged()
{
	if( currentItem() )
	{
		if( currentItem()->parent() == 0 )
			LuaCaraExplorer::setCurrentPeakList( dynamic_cast<_PeakListListItem*>( currentItem() )->getList() );
		else
			LuaCaraExplorer::setCurrentPeakList( dynamic_cast<_PeakListListItem*>( currentItem()->parent() )->getList() );
	}
}

void PeakListListView::handleShowTable(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );

	int i = 0;
	const Project::PeakListMap& sm = d_pro->getPeakLists();
	Project::PeakListMap::const_iterator p;
	ObjectListView::ObjectList o( sm.size() );
	for( p = sm.begin(); p != sm.end(); ++p, ++i)
	{
		o[ i ] = (*p).second;
	}
	ObjectListView::edit( this, d_rep->findObjectDef( Repository::keyPeaklist ), o );
}



