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

#include "SystemTypeView.h"
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
#include "LuaCaraExplorer.h"
using namespace Spec;

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr SystemTypeView::AddType = "AddType";
Root::Action::CmdStr SystemTypeView::RenameType = "RenameType";
Root::Action::CmdStr SystemTypeView::RemoveType = "RemoveType";
Root::Action::CmdStr SystemTypeView::EditAtts = "EditAtts";
Root::Action::CmdStr SystemTypeView::ShowTable = "ShowTable";
Root::Action::CmdStr SystemTypeView::SetModel = "SetModel";
Root::Action::CmdStr SystemTypeView::SetGeneric = "SetGeneric";
Root::Action::CmdStr SystemTypeView::SetTerminals = "SetTerminals";
Root::Action::CmdStr SystemTypeView::SetClass = "SetClass";
Root::Action::CmdStr SystemTypeView::SetCands = "SetCands";

ACTION_SLOTS_BEGIN( SystemTypeView )
    { SystemTypeView::SetGeneric, &SystemTypeView::handleSetGeneric },
    { SystemTypeView::SetTerminals, &SystemTypeView::handleSetTerminals },
    { SystemTypeView::SetClass, &SystemTypeView::handleSetClass },
    { SystemTypeView::SetCands, &SystemTypeView::handleSetCands },
    { SystemTypeView::SetModel, &SystemTypeView::handleSetModel },
    { SystemTypeView::ShowTable, &SystemTypeView::handleShowTable },
    { SystemTypeView::EditAtts, &SystemTypeView::handleEditAtts },
    { SystemTypeView::RemoveType, &SystemTypeView::handleRemoveType },
    { SystemTypeView::RenameType, &SystemTypeView::handleRenameType },
    { SystemTypeView::AddType, &SystemTypeView::handleNewType },
ACTION_SLOTS_END( SystemTypeView )

//////////////////////////////////////////////////////////////////////

class _SystemTypeItem : public Gui::ListViewItem
{
	Root::ExRef<SystemType> d_st;
public:
	_SystemTypeItem( Gui::ListView* p, SystemType* rt ):Gui::ListViewItem(p),d_st( rt ) {}
	SystemType* getSt() const { return d_st; }

	QString text( int f ) const 
	{ 
		switch( f )
		{
		case 0:
			return d_st->getName().data();
		case 1:
			return d_st->getResiType().data();
		case 2:
			return d_st->getClass().data();
		case 3:
			return d_st->getNTerm().data();
		case 4:
			return d_st->getCTerm().data();
		case 5:
			return d_st->getGenSysSym().data();
		case 6:
			if( !d_st->getMatches().empty() )
				return "x";
			else
				return "";
		default:
			return "";
		}
	}
	// const QPixmap *pixmap( int i ) const { return s_pixRepository; }
};

//////////////////////////////////////////////////////////////////////

SystemTypeView::SystemTypeView(QWidget* p,Spec::Repository* r):
	Gui::ListView( p ), d_rep( r )
{
	setShowSortIndicator( true );
	//setRootIsDecorated( true );
	setAllColumnsShowFocus( true );
	addColumn( "Name" );
	addColumn( "Model" );
	addColumn( "Class" );
	addColumn( "N-Term." );
	addColumn( "C-Term." );
	addColumn( "Gen.Neigh." );
	addColumn( "Cands." );
	refill();
	d_rep->addObserver( this );
}

SystemTypeView::~SystemTypeView()
{
	d_rep->removeObserver( this );
}

void SystemTypeView::onCurrentChanged()
{
	if( _SystemTypeItem* i = dynamic_cast<_SystemTypeItem*>( currentItem() ) )
		LuaCaraExplorer::setCurrentSystemType( i->getSt() );
}

void SystemTypeView::refill()
{
	clear();

	Repository::SystemTypeMap::const_iterator p;
	const Repository::SystemTypeMap& sm = d_rep->getSystemTypes();
	for( p = sm.begin(); p != sm.end(); ++p )
	{
        new _SystemTypeItem( this, (*p).second );
	}
}

Gui::Menu* SystemTypeView::createPopup()
{
	Gui::Menu* pop = new Gui::Menu();
	Gui::Menu::item( pop,  "New Type...", AddType, false );
	Gui::Menu::item( pop,  "Rename Type...", RenameType, false );
	Gui::Menu::item( pop,  "Delete Type...", RemoveType, false );
	pop->insertSeparator();
	Gui::Menu::item( pop,  "Set Model...", SetModel, false );
	Gui::Menu::item( pop,  "Set Class...", SetClass, false );
	Gui::Menu::item( pop,  "Set Candidates...", SetCands, false );
	pop->insertSeparator();
	Gui::Menu::item( pop,  "Set Terminals...", SetTerminals, false );
	Gui::Menu::item( pop,  "Set Generic Neighbors...", SetGeneric, false );
	pop->insertSeparator();
	Gui::Menu::item( pop,  "Edit Attributes...", EditAtts, false );
	Gui::Menu::item( pop,  "Open Object Table...", ShowTable, false );
	return pop;
}

void SystemTypeView::handle(Root::Message & msg)
{
	BEGIN_HANDLER();
	MESSAGE( SystemType::Added, a, msg )
	{
		_SystemTypeItem* i = new _SystemTypeItem( this, a->sender() );
		i->setCurrent();
	}
	MESSAGE( SystemType::Removed, a, msg )
	{
		for( int i = 0; i < count(); i++ )
			if( static_cast<_SystemTypeItem*>( child(i) )->getSt() == a->sender() )
			{
				child(i)->removeMe();
				break;
			}
	}
	MESSAGE( Root::Action, a, msg )
	{
		EXECUTE_ACTION( SystemTypeView, *a );
	}
	END_HANDLER();
}

void SystemTypeView::handleNewType(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );

	bool ok;
	QString res = QInputDialog::getText( "New Spin System Type", 
		"Please select a unique name:", QLineEdit::Normal, "", &ok, this );
	if( !ok )
		return;
	res = res.stripWhiteSpace();
	if( res.isEmpty() )
	{
		QMessageBox::critical( this, "New Spin System Type", 
			"System type must have a name!", "&Cancel" );
		return;
	}

	Repository::SystemTypeMap::const_iterator p;
	const Repository::SystemTypeMap& sm = d_rep->getSystemTypes();
	for( p = sm.begin(); p != sm.end(); ++p )
	{
		if( res == (*p).second->getName().data() )
		{
			QMessageBox::critical( this, "New Spin System Type", 
				"The selected name is not unique!", "&Cancel" );
			return;
		}
	}
	d_rep->addSystemType( res.toLatin1() );
}

void SystemTypeView::handleRemoveType(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );

	_SystemTypeItem* item = (_SystemTypeItem*) currentItem();

	if( QMessageBox::warning( this, "Delete Spin System Type",
		"Do you really want to delete this type (cannot be undone)?",
		"&OK", "&Cancel" ) != 0 )
		return;
	if( !d_rep->remove( item->getSt() ) )
		QMessageBox::critical( this, "Delete Spin System Type", 
			"Cannot delete type since it is in use!", "&Cancel" );
}

void SystemTypeView::handleRenameType(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );

	_SystemTypeItem* item = (_SystemTypeItem*) currentItem();
	bool ok;
	QString res = QInputDialog::getText( "Rename Spin System Type", 
		"Please choose a new unique name:", QLineEdit::Normal, item->getSt()->getName().data(), &ok, this );
	if( !ok )
		return;
	res = res.stripWhiteSpace();
	if( res.isEmpty() )
	{
		QMessageBox::critical( this, "Rename Spin System Type", 
			"System type must have a name!", "&Cancel" );
		return;
	}
	if( !d_rep->rename( item->getSt(), res ) )
		QMessageBox::critical( this, "Rename Spin System Type", 
			"Cannot rename since name is already in use!", "&Cancel" );
}

void SystemTypeView::handleEditAtts(Root::Action & a)
{
	_SystemTypeItem* item = dynamic_cast<_SystemTypeItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	DynValueEditor::edit( this, d_rep->findObjectDef( Repository::keySystemType ),
		item->getSt() );
}

void SystemTypeView::handleShowTable(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );

	int i = 0;
	Repository::SystemTypeMap::const_iterator p;
	const Repository::SystemTypeMap& sm = d_rep->getSystemTypes();
	ObjectListView::ObjectList o( sm.size() );
	for( p = sm.begin(); p != sm.end(); ++p, ++i)
	{
		o[ i ] = (*p).second;
	}
	ObjectListView::edit( this, d_rep->findObjectDef( Repository::keySystemType ), o );
}

void SystemTypeView::handleSetModel(Root::Action & a)
{
	_SystemTypeItem* item = dynamic_cast<_SystemTypeItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	Repository::ResidueTypeMap::const_iterator p;
	const Repository::ResidueTypeMap& sm = d_rep->getResidueTypes();

	int cur = 0, i = 0;
	QStringList l;
	l.append( "" );
	for( p = sm.begin(); p != sm.end(); ++p, i++ )
	{
		l.append( (*p).second->getShort().data() ); // NOTE: Name sollte Unique sein.
	}
	l.sort();
	if( item->getSt()->getRt() )
		cur = l.findIndex( item->getSt()->getRt()->getShort().data() );

	bool ok;
	QString res = QInputDialog::getItem( "Set Model", "Select a residue type:", 
		l, cur, false, &ok, this );
	if( !ok )
		return;

	ResidueType* sst = 0;
	for( p = sm.begin(); p != sm.end(); ++p )
		if( res == (*p).second->getShort().data() )
		{
			sst = (*p).second;
			break;
		}
	d_rep->getTypes()->setResiType( item->getSt(), sst );
}

void SystemTypeView::handleSetGeneric(Root::Action & a)
{
	_SystemTypeItem* item = dynamic_cast<_SystemTypeItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	Repository::ResidueTypeMap::const_iterator p;
	const Repository::ResidueTypeMap& sm = d_rep->getResidueTypes();

	int cur = 0, i = 0;
	QStringList l;
	l.append( "" );
	for( p = sm.begin(); p != sm.end(); ++p, i++ )
	{
		l.append( (*p).second->getShort().data() ); // NOTE: Name sollte Unique sein.
	}
	l.sort();
	if( item->getSt()->getGenSys() )
		cur = l.findIndex( item->getSt()->getGenSys()->getShort().data() );

	bool ok;
	QString res = QInputDialog::getItem( "Set Generic Neighbors", "Select a residue type:", 
		l, cur, false, &ok, this );
	if( !ok )
		return;

	ResidueType* sst = 0;
	for( p = sm.begin(); p != sm.end(); ++p )
		if( res == (*p).second->getShort().data() )
		{
			sst = (*p).second;
			break;
		}
	d_rep->getTypes()->setGenSys( item->getSt(), sst );
}

void SystemTypeView::handleSetTerminals(Root::Action & a)
{
	_SystemTypeItem* item = dynamic_cast<_SystemTypeItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	Root::SymbolString n = item->getSt()->getNTerm();
	Root::SymbolString c = item->getSt()->getCTerm();
	if( Dlg::getLabelPair( this, n, c, "Select Terminals", 
		"N-Terminal (i-1):", "C-Terminal (i+1):" ) )
	{
		try
		{
			d_rep->getTypes()->setTerms( item->getSt(), n, c );
		}catch( Root::Exception& e )
		{
			QMessageBox::critical( this, "Set Terminals", e.what(), "&Cancel" );
			return;
		}
	}
}

void SystemTypeView::handleSetClass(Root::Action & a)
{
	_SystemTypeItem* item = dynamic_cast<_SystemTypeItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	bool ok;
	QString res = QInputDialog::getText( "Set Type Classifier", 
		"Please enter an arbitrary name:", QLineEdit::Normal, item->getSt()->getClass().data(), &ok, this );
	if( !ok )
		return;
	res = res.stripWhiteSpace();
	d_rep->getTypes()->setClass( item->getSt(), res.toLatin1() );
}

void SystemTypeView::handleSetCands(Root::Action & a)
{
	_SystemTypeItem* item = dynamic_cast<_SystemTypeItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	CandidateDlg dlg( this, d_rep );
	dlg.setTitle( item->getSt() );
	if( dlg.exec() )
	{
		SystemType::ResiTypeMatches res;
		SpinSystem::Candidates::const_iterator i;
		for( i = dlg.d_cands.begin(); i != dlg.d_cands.end(); ++i )
			res.insert( (*i)->getId() );
		d_rep->getTypes()->setMatches( item->getSt(), res );
	}
}

