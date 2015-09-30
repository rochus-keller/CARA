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

#include "LabelingSchemeView.h"
#include <qinputdialog.h>
#include <qmessagebox.h>
#include <qlabel.h>
#include <qlineedit.h> 
#include <qlayout.h>
#include <qpushbutton.h>
#include <Root/Application.h>
#include <Root/Any.h>
#include <SpecView/DynValueEditor.h>
#include <SpecView/ObjectListView.h>
#include <Dlg.h>
#include <SpecView/CandidateDlg.h>
using namespace Spec;

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr LabelingSchemeView::Add = "Add";
Root::Action::CmdStr LabelingSchemeView::Rename = "Rename";
Root::Action::CmdStr LabelingSchemeView::Remove = "Remove";
Root::Action::CmdStr LabelingSchemeView::EditAtts = "EditAtts";
Root::Action::CmdStr LabelingSchemeView::ShowTable = "ShowTable";

ACTION_SLOTS_BEGIN( LabelingSchemeView )
    { LabelingSchemeView::ShowTable, &LabelingSchemeView::handleShowTable },
    { LabelingSchemeView::EditAtts, &LabelingSchemeView::handleEditAtts },
    { LabelingSchemeView::Remove, &LabelingSchemeView::handleRemove },
    { LabelingSchemeView::Rename, &LabelingSchemeView::handleRename },
    { LabelingSchemeView::Add, &LabelingSchemeView::handleNew },
ACTION_SLOTS_END( LabelingSchemeView )

//////////////////////////////////////////////////////////////////////

class _LabelingSchemeItem : public Gui::ListViewItem
{
	Root::ExRef<LabelingScheme> d_scheme;
public:
	_LabelingSchemeItem( Gui::ListView* p, LabelingScheme* rt ):Gui::ListViewItem(p),d_scheme( rt ) {}
	LabelingScheme* getScheme() const { return d_scheme; }

	QString text( int f ) const 
	{ 
		switch( f )
		{
		case 0:
			return QString::number( d_scheme->getId() );
		case 1:
			return d_scheme->getName();
		default:
			return "";
		}
	}
	// const QPixmap *pixmap( int i ) const { return s_pixRepository; }
};

//////////////////////////////////////////////////////////////////////

LabelingSchemeView::LabelingSchemeView(QWidget* p,Spec::Repository* r):
	Gui::ListView( p ), d_rep( r )
{
	setShowSortIndicator( true );
	//setRootIsDecorated( true );
	setAllColumnsShowFocus( true );
	addColumn( "ID" );
	addColumn( "Name" );
	refill();
	d_rep->addObserver( this );
}

LabelingSchemeView::~LabelingSchemeView()
{
	d_rep->removeObserver( this );
}

void LabelingSchemeView::refill()
{
	clear();

	TypeBase::LabelingSchemeMap::const_iterator p;
	const TypeBase::LabelingSchemeMap& sm = d_rep->getTypes()->getLabelingSchemes();
	for( p = sm.begin(); p != sm.end(); ++p )
	{
        new _LabelingSchemeItem( this, (*p).second );
	}
}

Gui::Menu* LabelingSchemeView::createPopup()
{
    Gui::Menu* pop = new Gui::Menu();
	Gui::Menu::item( pop,  "New...", Add, false );
	Gui::Menu::item( pop,  "Rename...", Rename, false );
	Gui::Menu::item( pop,  "Delete...", Remove, false );
	pop->insertSeparator();
	Gui::Menu::item( pop,  "Edit Attributes...", EditAtts, false );
	Gui::Menu::item( pop, "Open Object Table...", ShowTable, false );
    return pop;
}

void LabelingSchemeView::handle(Root::Message & msg)
{
	BEGIN_HANDLER();
	MESSAGE( LabelingScheme::Added, a, msg )
	{
		_LabelingSchemeItem* i = new _LabelingSchemeItem( this, a->sender() );
		i->setCurrent();
	}
	MESSAGE( LabelingScheme::Removed, a, msg )
	{
		for( int i = 0; i < count(); i++ )
			if( static_cast<_LabelingSchemeItem*>( child( i ) )->getScheme() == a->sender() )
				child(i)->removeMe();
	}
	MESSAGE( Root::Action, a, msg )
	{
		EXECUTE_ACTION( LabelingSchemeView, *a );
	}
	END_HANDLER();
}

void LabelingSchemeView::handleNew(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );

	bool ok;
	int id = 1;
	TypeBase::LabelingSchemeMap::const_iterator p;
	const TypeBase::LabelingSchemeMap& sm = d_rep->getTypes()->getLabelingSchemes();
	for( p = sm.begin(); p != sm.end(); ++p )
	{
		if( (*p).second->getId() >= id )
			id = (*p).second->getId() + 1;
	}
restart:
	id = QInputDialog::getInteger( "New Labeling Scheme", 
		"Please select a unique ID:", id, 1, 2147483647, 1, &ok, this );
	if( !ok )
		return;
	if( id == 0 || sm.find( id ) != sm.end() )
	{
		QMessageBox::critical( this, "New Labeling Scheme", 
			"The selected id is not unique!", "&Cancel" );
		goto restart;
	}

	QString res = QInputDialog::getText( "New Labeling Scheme", 
		"Please select a name:", QLineEdit::Normal, "", &ok, this );
	if( !ok )
		return;
	res = res.stripWhiteSpace();

	for( p = sm.begin(); p != sm.end(); ++p )
	{
		if( res == (*p).second->getName() )
		{
			QMessageBox::critical( this, "New Labeling Scheme", 
				"The selected name is not unique!", "&Cancel" );
			return;
		}
	}
	d_rep->getTypes()->addLabelingScheme( id, res );
}

void LabelingSchemeView::handleRemove(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );

	_LabelingSchemeItem* item = (_LabelingSchemeItem*) currentItem();

	if( QMessageBox::warning( this, "Delete Labeling Scheme",
		"Do you really want to delete this scheme (cannot be undone)?",
		"&OK", "&Cancel" ) != 0 )
		return;
	if( !d_rep->getTypes()->remove( item->getScheme() ) )
		QMessageBox::critical( this, "Delete Labeling Scheme", 
			"Cannot delete scheme since it is in use!", "&Cancel" );
}

void LabelingSchemeView::handleRename(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );

	_LabelingSchemeItem* item = (_LabelingSchemeItem*) currentItem();
	bool ok;
	QString res = QInputDialog::getText( "Rename Labeling Scheme", 
		"Please choose a name:", QLineEdit::Normal, 
		item->getScheme()->getName(), &ok, this );
	if( !ok )
		return;
	res = res.stripWhiteSpace();
	d_rep->getTypes()->rename( item->getScheme(), res );
}

void LabelingSchemeView::handleEditAtts(Root::Action & a)
{
	_LabelingSchemeItem* item = dynamic_cast<_LabelingSchemeItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	DynValueEditor::edit( this, d_rep->findObjectDef( Repository::keyLabelingScheme ),
		item->getScheme() );
}

void LabelingSchemeView::handleShowTable(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );

	int i = 0;
	TypeBase::LabelingSchemeMap::const_iterator p;
	const TypeBase::LabelingSchemeMap& sm = d_rep->getTypes()->getLabelingSchemes();
	ObjectListView::ObjectList o( sm.size() );
	for( p = sm.begin(); p != sm.end(); ++p, ++i)
	{
		o[ i ] = (*p).second;
	}
	ObjectListView::edit( this, d_rep->findObjectDef( Repository::keyLabelingScheme ), o );
}

