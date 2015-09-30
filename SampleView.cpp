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

#include "SampleView.h"
#include <stdio.h>
#include <ctype.h>
#include <qinputdialog.h>
#include <qmessagebox.h>
#include <QApplication>
#include <QComboBox>
#include <Dlg.h>
#include <Root/Application.h>
#include <Spec/Repository.h>
#include <SpecView/DynValueEditor.h>
#include <SpecView/ObjectListView.h>
#include <Gui/InputDlg.h>
#include "LuaCaraExplorer.h"
using namespace Spec;

static char s_buf[64];
static const int s_off = 10000; // Damit Sortierung auch fr Negative stimmt.

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr SampleView::EditAtts = "EditAtts";
Root::Action::CmdStr SampleView::ShowTable = "ShowTable";
Root::Action::CmdStr SampleView::RemoveRange = "RemoveRange";
Root::Action::CmdStr SampleView::AddRange = "AddRange";
Root::Action::CmdStr SampleView::RemoveSample = "RemoveSample";
Root::Action::CmdStr SampleView::AddSample = "AddSample";
Root::Action::CmdStr SampleView::RenameSample = "RenameSample";

ACTION_SLOTS_BEGIN( SampleView )
    { SampleView::RemoveRange, &SampleView::handleRemoveRange },
    { SampleView::AddRange, &SampleView::handleAddRange },
    { SampleView::RemoveSample, &SampleView::handleRemoveSample },
    { SampleView::RenameSample, &SampleView::handleRenameSample },
    { SampleView::AddSample, &SampleView::handleAddSample },
    { SampleView::ShowTable, &SampleView::handleShowTable },
    { SampleView::EditAtts, &SampleView::handleEditAtts },
ACTION_SLOTS_END( SampleView )

//////////////////////////////////////////////////////////////////////

class _SampleItem : public Gui::ListViewItem
{
public:
	Root::ExRef<BioSample> d_samp;
	_SampleItem( Gui::ListView* p, BioSample* res ):Gui::ListViewItem(p),d_samp( res ) {}

	QString text( int f ) const 
	{ 
		switch( f )
		{
		case 0:
			return d_samp->getName();
		case 1:
			return QString::number( d_samp->getId() );
		default:
			return "";
		}
	}
	QString key( int f, bool ascending  ) const
	{
		switch( f )
		{
		case 1:
			::sprintf( s_buf, "%08d", d_samp->getId() + s_off );
			return s_buf;
		default:
			return text( f );
		}
		return s_buf;
	}
	// const QPixmap *pixmap( int i ) const { return s_pixRepository; }
};

class _SampleRangeItem : public Gui::ListViewItem
{
public:
	BioSample::Range d_range;
	_SampleRangeItem( Gui::ListViewItem* p, const BioSample::Range& dp ):
	  Gui::ListViewItem(p),d_range( dp ) {}

	QString text( int f ) const 
	{ 
		switch( f )
		{
		case 0:
			if( d_range.d_schema == 0 )
				return "Default";
			else
			{
				_SampleItem* a = (_SampleItem*) parent();
				LabelingScheme* s = a->d_samp->getOwner()->getOwner()->getTypes()->
					getLabelingScheme( d_range.d_schema );
				if( s )
					return s->getName();
				else
					return "?"; 
			}
		case 1:
			return QString::number( d_range.d_schema );
		case 2:
			return QString::number( d_range.d_start );
		case 3:
			return QString::number( d_range.d_end );
		default:
			return "";
		}
	}
	QString key( int f, bool ascending  ) const
	{
		switch( f )
		{
		case 1:
			::sprintf( s_buf, "%08d", d_range.d_schema );
			return s_buf;
		case 2:
			::sprintf( s_buf, "%08d", d_range.d_start );
			return s_buf;
		case 3:
			::sprintf( s_buf, "%08d", d_range.d_end );
			return s_buf;
		default:
			return text( f );
		}
		return s_buf;
	}
	// const QPixmap *pixmap( int i ) const { return s_pixRepository; }
};

SampleView::SampleView(QWidget* p,Spec::Project* pro):
	Gui::ListView( p ), d_pro( pro )
{
	setShowSortIndicator( true );
	setRootIsDecorated( true );
	setAllColumnsShowFocus( true );
	addColumn( "Name" );
	addColumn( "ID" );
	addColumn( "From" );
	addColumn( "To" );
	refill();
	d_pro->addObserver( this );
}

SampleView::~SampleView()
{
	d_pro->removeObserver( this );
}

void SampleView::onCurrentChanged()
{
	if( _SampleItem* item = dynamic_cast<_SampleItem*>( currentItem() ) )
	{
		LuaCaraExplorer::setCurrentSample( item->d_samp );
	}else if( _SampleRangeItem* item = dynamic_cast<_SampleRangeItem*>( currentItem() ) )
	{
		_SampleItem* a = (_SampleItem*) item->parent();
		LuaCaraExplorer::setCurrentSample( a->d_samp );
	}
}

Gui::Menu* SampleView::createPopup()
{
	Gui::Menu* pop = new Gui::Menu();
	Gui::Menu::item( pop,  "Add Sample...", AddSample, false );
	Gui::Menu::item( pop,  "Rename Sample...", RenameSample, false );
	Gui::Menu::item( pop,  "Remove Sample...", RemoveSample, false );
	pop->insertSeparator();
	Gui::Menu::item( pop,  "Add Range...", AddRange, false );
	Gui::Menu::item( pop,  "Remove Range", RemoveRange, false );
	pop->insertSeparator();
	Gui::Menu::item( pop,  "Edit Attributes...", EditAtts, false );
	Gui::Menu::item( pop,  "Open Object Table...", ShowTable, false );
	return pop;
}

void SampleView::refill()
{
	clear();
	const Project::SampleMap& sm = d_pro->getSamples();
	Project::SampleMap::const_iterator p;
	for( p = sm.begin(); p != sm.end(); ++p )
	{
		_SampleItem* i1 = new _SampleItem( this, (*p).second );
		const BioSample::Ranges& par = (*p).second->getRanges();
		BioSample::Ranges::const_iterator p2;
		for( p2 = par.begin(); p2 != par.end(); ++p2 )
		{
			new _SampleRangeItem( i1, (*p2) );
		}
	}
}

void SampleView::handle(Root::Message & msg)
{
	BEGIN_HANDLER();
	MESSAGE( BioSample::Added, a, msg )
	{
		_SampleItem* i1 = new _SampleItem( this, a->sender() );
		const BioSample::Ranges& par = a->sender()->getRanges();
		BioSample::Ranges::const_iterator p2;
		for( p2 = par.begin(); p2 != par.end(); ++p2 )
		{
			new _SampleRangeItem( i1, (*p2) );
		}
		i1->setCurrent();
	}
	MESSAGE( BioSample::Removed, a, msg )
	{
		for( int i = 0; i < count(); i++ )
			if( static_cast<_SampleItem*>( child(i) )->d_samp == a->sender() )
			{
				child(i)->removeMe();
				break;
			}
	}
	MESSAGE( BioSample::Changed, a, msg )
	{
		switch( a->d_hint )
		{
		case BioSample::AddRange:
		case BioSample::EraseRange:
		case BioSample::Schema:
		case BioSample::Assignable:
			for( int i = 0; i < count(); i++ )
				if( static_cast<_SampleItem*>( child(i) )->d_samp == a->sender() )
				{
					child(i)->clearChildren();
					const BioSample::Ranges& par = a->sender()->getRanges();
					BioSample::Ranges::const_iterator p2;
					for( p2 = par.begin(); p2 != par.end(); ++p2 )
					{
						new _SampleRangeItem( static_cast<_SampleItem*>( child(i) ), (*p2) );
					}
					break;
				}
			break;
        default:
            break;
		}
	}
	MESSAGE( Root::Action, a, msg )
	{
		EXECUTE_ACTION( SampleView, *a );
	}
	END_HANDLER();
}

void SampleView::handleEditAtts(Root::Action & a)
{
	_SampleItem* item = dynamic_cast<_SampleItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	DynValueEditor::edit( this, 
		d_pro->getRepository()->findObjectDef( Repository::keySample ),
		item->d_samp );
}

void SampleView::handleShowTable(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );

	int i = 0;
	const Project::SampleMap& sm = d_pro->getSamples();
	Project::SampleMap::const_iterator p;
	ObjectListView::ObjectList o( sm.size() );
	for( p = sm.begin(); p != sm.end(); ++p, ++i)
	{
		o[ i ] = (*p).second;
	}
	ObjectListView::edit( this, d_pro->getRepository()
		->findObjectDef( Repository::keySample ), o );
}

void SampleView::handleAddSample(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );

	bool ok;
	QString res = QInputDialog::getText( "New Sample", 
		"Please select a name:", QLineEdit::Normal, "", &ok, this );
	if( !ok )
		return;
	res = res.stripWhiteSpace();

	const Project::SampleMap& sm = d_pro->getSamples();
	Project::SampleMap::const_iterator p;
	for( p = sm.begin(); p != sm.end(); ++p )
	{
		if( res == (*p).second->getName() )
		{
			QMessageBox::critical( this, "New Sample", 
				"The selected name is not unique!", "&Cancel" );
			return;
		}
	}
	Root::Ref<BioSample> s = new BioSample();
	d_pro->addSample( s );
	s->setName( res.toLatin1() );
}

void SampleView::handleRenameSample(Root::Action & a)
{
	_SampleItem* item = dynamic_cast<_SampleItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	bool ok;
	QString res = QInputDialog::getText( "Rename Sample", 
		"Please choose a name:", QLineEdit::Normal, 
		item->d_samp->getName(), &ok, this );
	if( !ok )
		return;
	res = res.stripWhiteSpace();
	item->d_samp->setName( res.toLatin1() );
}

void SampleView::handleRemoveSample(Root::Action & a)
{
	_SampleItem* item = dynamic_cast<_SampleItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	if( QMessageBox::warning( this, "Delete Sample",
		"Do you really want to delete this sample (cannot be undone)?",
		"&OK", "&Cancel" ) != 0 )
		return;
	if( !d_pro->removeSample( item->d_samp ) )
		QMessageBox::critical( this, "Delete Sample", 
			"Cannot delete sample since it is in use!", "&Cancel" );
}

void SampleView::handleAddRange(Root::Action & a)
{
	_SampleItem* item = dynamic_cast<_SampleItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );
	Gui::InputDlg dlg( this, "Add Range" );

	TypeBase::LabelingSchemeMap::const_iterator l;
	const TypeBase::LabelingSchemeMap& ls = d_pro->getOwner()->getTypes()->getLabelingSchemes();
	dlg.addLabel( "Labeling Scheme:", 0, 0 );
	QComboBox _scheme;
	for( l = ls.begin(); l != ls.end(); ++l )
		_scheme.addItem( (*l).second->getName(), (*l).first );
	dlg.add( &_scheme, 0, 1 );
	QComboBox _from;
	QComboBox _to;
	const Sequence::ResidueMap& sm = d_pro->getSequence()->getResi();
	Sequence::ResidueMap::const_iterator r;
	QString str;
	for( r = sm.begin(); r != sm.end(); ++r )
	{
		str.sprintf( "%d/%s:%s%d", (*r).first, (*r).second->getChain().data(),
			(*r).second->getType()->getLetter().data(), (*r).second->getNr() );
		_from.addItem( str, (*r).first );
		_to.addItem( str, (*r).first );
	}
	dlg.addLabel( "From:", 1, 0 );
	dlg.add( &_from, 1, 1 );
	dlg.addLabel( "To:", 2, 0 );
	dlg.add( &_to, 2, 1 );
	if( dlg.exec() )
	{
		int i = _scheme.currentIndex();
		assert( i != -1 );
		Root::Index scheme = _scheme.itemData( i ).toInt();
		i = _from.currentIndex();
		assert( i != -1 );
		Root::Index from = _from.itemData( i ).toInt();
		i = _to.currentIndex();
		assert( i != -1 );
		Root::Index to = _to.itemData( i ).toInt();
		if( !item->d_samp->addRange( from, to, scheme, false ) )
			QMessageBox::critical( this, "Add Range", 
				"Cannot create range due to overlap or From>To!", "&Cancel" );
	}
}

void SampleView::handleRemoveRange(Root::Action & a)
{
	_SampleRangeItem* item = dynamic_cast<_SampleRangeItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	_SampleItem* p = dynamic_cast<_SampleItem*>( item->parent() );
	if( QMessageBox::warning( this, "Remove Range",
		"Do you really want to delete this range (cannot be undone)?",
		"&OK", "&Cancel" ) != 0 )
		return;
	if( !p->d_samp->eraseRange( item->d_range.d_start ) )
		QMessageBox::critical( this, "Remove Range", 
			"Error deleting range!", "&Cancel" );
}

