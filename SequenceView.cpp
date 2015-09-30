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

#include "SequenceView.h"
#include <stdio.h>
#include <ctype.h>
#include <qinputdialog.h>
#include <qmessagebox.h>
#include <QFileDialog>
#include <QApplication>
#include <Dlg.h>
#include <Root/Application.h>
#include <Spec/ResidueBase.h>
#include <Spec/SequenceFile.h>
#include <SpecView/DynValueEditor.h>
#include <SpecView/ObjectListView.h>
#include "LuaCaraExplorer.h"
using namespace Spec;

static char s_buf[64];
static const int s_off = 10000; // Damit Sortierung auch fr Negative stimmt.

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr SequenceView::AddValue = "AddValue";
Root::Action::CmdStr SequenceView::RemoveValue = "RemoveValue";
Root::Action::CmdStr SequenceView::ChangeValue = "ChangeValue";
Root::Action::CmdStr SequenceView::ImportValues = "ImportValues";
Root::Action::CmdStr SequenceView::EditAtts = "EditAtts";
Root::Action::CmdStr SequenceView::ShowTable = "ShowTable";
Root::Action::CmdStr SequenceView::ExportValues = "ExportValues";
Root::Action::CmdStr SequenceView::SetNumber = "SetNumber";
Root::Action::CmdStr SequenceView::SetChain = "SetChain";
Root::Action::CmdStr SequenceView::AddChain = "AddChain";
Root::Action::CmdStr SequenceView::RemoveChain = "RemoveChain";
Root::Action::CmdStr SequenceView::RemoveResidue = "RemoveResidue";
Root::Action::CmdStr SequenceView::SetNumberFrom = "SetNumberFrom";
Root::Action::CmdStr SequenceView::ExportChain = "ExportChain";

ACTION_SLOTS_BEGIN( SequenceView )
    { SequenceView::ExportChain, &SequenceView::handleExportChain },
    { SequenceView::SetNumberFrom, &SequenceView::handleSetNumberFrom },
    { SequenceView::RemoveChain, &SequenceView::handleRemoveChain },
    { SequenceView::RemoveResidue, &SequenceView::handleRemoveResidue },
    { SequenceView::AddChain, &SequenceView::handleAddChain },
    { SequenceView::SetNumber, &SequenceView::handleSetNumber },
    { SequenceView::SetChain, &SequenceView::handleSetChain },
    { SequenceView::ExportValues, &SequenceView::handleExportValues },
    { SequenceView::ShowTable, &SequenceView::handleShowTable },
    { SequenceView::EditAtts, &SequenceView::handleEditAtts },
    { SequenceView::AddValue, &SequenceView::handleAddValue },
    { SequenceView::RemoveValue, &SequenceView::handleRemoveValue },
    { SequenceView::ChangeValue, &SequenceView::handleChangeValue },
    { SequenceView::ImportValues, &SequenceView::handleImportValues },
ACTION_SLOTS_END( SequenceView )

//////////////////////////////////////////////////////////////////////

class _ResidueItem : public Gui::ListViewItem
{
public:
	Root::ExRef<Residue> d_res;
	_ResidueItem( Gui::ListView* p, Residue* res ):Gui::ListViewItem(p),d_res( res ) {}

	QString text( int f ) const 
	{ 
		switch( f )
		{
		case 0:
			return QString().setNum( d_res->getId() );
		case 1:
			if( d_res->getChain().isNull() )
				::sprintf( s_buf, "%d", d_res->getNr() );
			else
				::sprintf( s_buf, "%s:%d", d_res->getChain().data(), d_res->getNr() );
			return s_buf;
		case 2:
			return d_res->getType()->getShort().data();
		case 5:
			if( d_res->getSystem() )
				::sprintf( s_buf, "%d", d_res->getSystem()->getId() );
			else
				return "";
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
			::sprintf( s_buf, "%08d", d_res->getId() + s_off );
			return s_buf;
		case 1:
			::sprintf( s_buf, "%s%09d", d_res->getChain().data(), 
					d_res->getNr() + s_off );
			return s_buf;
		case 2:
			return d_res->getType()->getShort().data();
		case 5:
			if( d_res->getSystem() )
				::sprintf( s_buf, "%08d", d_res->getSystem()->getId() );
			else
				return "";
			return s_buf;
		default:
			return "";
		}
		return s_buf;
	}
	// const QPixmap *pixmap( int i ) const { return s_pixRepository; }
};

class _ResidueParamItem : public Gui::ListViewItem
{
public:
	Root::ExRef<Residue> d_res;
	Root::SymbolString d_name;
	_ResidueParamItem( Gui::ListViewItem* p, Root::SymbolString name, Residue* r ):
	  Gui::ListViewItem(p),d_name( name ), d_res(r) {}

	QString text( int f ) const 
	{ 
		switch( f )
		{
		case 0:
			return d_name.data();
		case 3:
			return QString().setNum( d_res->getDisPar(d_name).d_mean );
		case 4:
			return QString().setNum( d_res->getDisPar(d_name).d_dev );
		default:
			return "";
		}
	}
	QString key( int f, bool ascending  ) const
	{
		switch( f )
		{
		case 0:
			return d_name.data();
		case 3:
			::sprintf( s_buf, "%8.3f", d_res->getDisPar(d_name).d_mean );
			return s_buf;
		case 4:
			::sprintf( s_buf, "%8.3f", d_res->getDisPar(d_name).d_dev );
			return s_buf;
		default:
			return "";
		}
		return s_buf;
	}
	// const QPixmap *pixmap( int i ) const { return s_pixRepository; }
};

SequenceView::SequenceView(QWidget* p,Spec::Project* pro):
	Gui::ListView( p ), d_pro( pro )
{
	setShowSortIndicator( true );
	setRootIsDecorated( true );
	setAllColumnsShowFocus( true );
	addColumn( "ID" );
	addColumn( "Chain/Nr." );
	addColumn( "Type" );
	addColumn( "Mean" );
	addColumn( "Std.Dev." );
	addColumn( "Assig." );
	refill();
	d_pro->addObserver( this );
}

SequenceView::~SequenceView()
{
	d_pro->removeObserver( this );
}

void SequenceView::onCurrentChanged()
{
	if( _ResidueItem* i = dynamic_cast<_ResidueItem*>( currentItem() ) )
	{
		LuaCaraExplorer::setCurrentResidue( i->d_res );
	}else if( _ResidueParamItem* i = dynamic_cast<_ResidueParamItem*>( currentItem() ) )
	{
		_ResidueItem* a = (_ResidueItem*) i->parent();
		LuaCaraExplorer::setCurrentResidue( a->d_res );
	}
}

Gui::Menu* SequenceView::createPopup()
{
	Gui::Menu* pop = new Gui::Menu();
	Gui::Menu::item( pop,  "Set Number...", SetNumber, false );
	Gui::Menu::item( pop,  "Renumber from here...", SetNumberFrom, false );
	Gui::Menu::item( pop,  "Set Chain...", SetChain, false );
	Gui::Menu::item( pop,  "Append Chain...", AddChain, false );
	Gui::Menu::item( pop,  "Export Chain...", ExportChain, false );
	pop->insertSeparator();
	Gui::Menu::item( pop,  "Remove Residue...", RemoveResidue, false );
	Gui::Menu::item( pop,  "Remove Chain...", RemoveChain, false );
	pop->insertSeparator();
	Gui::Menu::item( pop,  "Add Value...", AddValue, false );
	Gui::Menu::item( pop,  "Remove Value", RemoveValue, false );
	Gui::Menu::item( pop,  "Change Value...", ChangeValue, false );
	Gui::Menu::item( pop,  "Import Values...", ImportValues, false );
	Gui::Menu::item( pop,  "Export Values...", ExportValues, false );
	pop->insertSeparator();
	Gui::Menu::item( pop,  "Edit Attributes...", EditAtts, false );
	Gui::Menu::item( pop,  "Open Object Table...", ShowTable, false );
	return pop;
}

static void _fillItem( _ResidueItem* i, Residue* r )
{
	const Residue::Parameters& par = r->getParams();
	Residue::Parameters::const_iterator p2;
	for( p2 = par.begin(); p2 != par.end(); ++p2 )
	{
		if( (*p2).second.isValid() )
			new _ResidueParamItem( i, (*p2).first, r );
	}
}

void SequenceView::refill()
{
	clear();
	const Sequence::ResidueMap& seq = d_pro->getSequence()->getResi();
	Sequence::ResidueMap::const_iterator p;
	for( p = seq.begin(); p != seq.end(); ++p )
	{
		_ResidueItem* i1 = new _ResidueItem( this, (*p).second );
		_fillItem( i1, (*p).second );
	}
}

static _ResidueItem* _find( Gui::ListView* v, Residue* r )
{
	for( int i = 0; i < v->count(); i++ )
		if( static_cast<_ResidueItem*>( v->child(i) )->d_res == r )
		{
			return static_cast<_ResidueItem*>( v->child(i) );
		}
	return 0;
}

static _ResidueParamItem* _find( _ResidueItem* item, Root::SymbolString n )
{
	if( item == 0 )
		return 0;
	for( int i = 0; i < item->count(); i++ )
		if( static_cast<_ResidueParamItem*>( item->child(i) )->d_name == n )
		{
			return static_cast<_ResidueParamItem*>( item->child(i) );
		}
	return 0;
}

void SequenceView::handle(Root::Message & msg)
{
	BEGIN_HANDLER();
	MESSAGE( Residue::Added, a, msg )
	{
		_ResidueItem* i1 = new _ResidueItem( this, a->sender());
		_fillItem( i1, a->sender() );
		i1->setCurrent();
	}
	MESSAGE( Residue::Removed, a, msg )
	{
		_ResidueItem* r = _find( this, a->sender() );
		if( r )
			r->removeMe();
	}
	MESSAGE( Residue::Changed, a, msg )
	{
		if( a->d_hint == Residue::SetParam )
		{
			_ResidueItem* r = _find( this, a->sender() );
			_ResidueParamItem* i = _find( r, a->d_value );
			if( i == 0 && r != 0 )
				new _ResidueParamItem( r, a->d_value, r->d_res );
		}else if( a->d_hint == Residue::RemoveParam )
		{
			_ResidueParamItem* i = _find( _find( this, a->sender() ), a->d_value );
			if( i )
				i->removeMe();
		}
	}
	MESSAGE( Root::Action, a, msg )
	{
		EXECUTE_ACTION( SequenceView, *a );
	}
	END_HANDLER();
}

void SequenceView::handleAddValue(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );

	Dlg::Value v;
	if( Dlg::getValue( this, v, true ) )
	{
		Sequence* seq = d_pro->getSequence();
		_ResidueItem* i = (_ResidueItem*)currentItem();
		if( i->d_res->hasParam( v.d_name ) )
		{
			QMessageBox::critical( this, "Add Value",
					"This label already has a value!", "&Cancel" );
			return;
		}
		seq->setDisPar( i->d_res, v.d_name, v.d_dp );
	}
}

void SequenceView::handleRemoveValue(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() != 0 );

	_ResidueItem* p = (_ResidueItem*)currentItem()->parent();
	_ResidueParamItem* i = (_ResidueParamItem*)currentItem();
	Sequence* seq = d_pro->getSequence();
	seq->removeDisPar( p->d_res, i->d_name );
}

void SequenceView::handleChangeValue(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() != 0 );

	_ResidueItem* p = (_ResidueItem*)currentItem()->parent();
	_ResidueParamItem* i = (_ResidueParamItem*)currentItem();
	Dlg::Value v;
	v.d_name = i->d_name;
	v.d_dp = i->d_res->getDisPar( i->d_name );
	if( Dlg::getValue( this, v, false ) )
	{
		Sequence* seq = d_pro->getSequence();
		seq->setDisPar( p->d_res, v.d_name, v.d_dp );
		i->repaint();
	}
}

void SequenceView::handleImportValues(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );

	QString fileName = QFileDialog::getOpenFileName( this, "Select Residue Library File",
                                                     Root::AppAgent::getCurrentDir(),
		"Residue Library (*.rel);;BMRB Stats File (*.stats)" );
    if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );
	Root::AppAgent::setCurrentDir( info.dirPath( true ) );
	ResidueBase db;
	QApplication::setOverrideCursor( Qt::waitCursor );
	if( info.extension(false).upper() == "REL" )
	{
		if( !db.read( fileName, d_pro->getRepository(), d_pro->getSequence() ) )
		{
			QMessageBox::critical( this, "Error Import Values",
					db.d_result.data(), "&Cancel" );
		}
	}else if( info.extension(false).upper() == "STATS" )
	{
		if( !db.readStats( fileName, d_pro->getRepository(), d_pro->getSequence() ) )
		{
			QMessageBox::critical( this, "Error Import Values",
					db.d_result.data(), "&Cancel" );
		}
	}else
	{
		QMessageBox::critical( this, "Error Import Values",
				"Invalid File Format", "&Cancel" );
	}
	QApplication::restoreOverrideCursor();
}

void SequenceView::handleEditAtts(Root::Action & a)
{
	_ResidueItem* item = dynamic_cast<_ResidueItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	DynValueEditor::edit( this, 
		d_pro->getRepository()->findObjectDef( Repository::keyResidue ),
		item->d_res );
}

void SequenceView::handleShowTable(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );

	int i = 0;
	const Sequence::ResidueMap& sm = d_pro->getSequence()->getResi();
	Sequence::ResidueMap::const_iterator p;
	ObjectListView::ObjectList o( sm.size() );
	for( p = sm.begin(); p != sm.end(); ++p, ++i)
	{
		o[ i ] = (*p).second;
	}
	ObjectListView::edit( this, d_pro->getRepository()
		->findObjectDef( Repository::keyResidue ), o );
}

void SequenceView::handleExportValues(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );

	QString fileName = QFileDialog::getSaveFileName( this, "Export Residue Library File",
                                                      Root::AppAgent::getCurrentDir(),
                                                        "Residue Library (*.rel)" );
	if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );

	if( info.extension( false ).upper() != "REL" )
		fileName += ".rel";
	info.setFile( fileName );
	if( info.exists() )
	{
		if( QMessageBox::warning( this, "Save As",
			"This file already exists. Do you want to overwrite it?",
			"&OK", "&Cancel" ) != 0 )
			return;
	}
	Root::AppAgent::setCurrentDir( info.dirPath( true ) );
	ResidueBase db;
	if( !db.write( fileName, d_pro->getSequence() ) )
	{
		QMessageBox::critical( this, "Error Export Values",
				db.d_result.data(), "&Cancel" );
	}
}

void SequenceView::handleSetNumber(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );
	Sequence* seq = d_pro->getSequence();
	_ResidueItem* i = (_ResidueItem*)currentItem();

	bool ok = false;
	int res = QInputDialog::getInteger( "Set Residue Number",
		"Enter an arbitrary residue number:", i->d_res->getNr(), -999999, 99999999, 1, &ok, this );
	if( !ok )
		return;
	Root::SymbolString chain = i->d_res->getChain();
	res = res - i->d_res->getNr();
	Sequence::ResidueMap::const_iterator j;
	for( j = seq->getResi().begin(); j != seq->getResi().end(); ++j )
	{
		if( (*j).second->getChain() == chain )
		{
			seq->setNr( (*j).second, (*j).second->getNr() + res );
		}
	}
	viewport()->update();
}

bool SequenceView::isValidChain(Root::SymbolString c)
{
	Sequence* seq = d_pro->getSequence();
	const char* s = c.data();
	for( ; *s != 0; s++ )
	{
		if( !::isalnum( *s ) )
		{
			QMessageBox::critical( this, "Error Setting Chain",
					"The id is not alpha-numeric!", "&Cancel" );
			return false;
		}
	}
	Sequence::ResidueMap::const_iterator j;
	for( j = seq->getResi().begin(); j != seq->getResi().end(); ++j )
	{
		if( (*j).second->getChain() == c )
		{
			QMessageBox::critical( this, "Error Setting Chain",
					"The selected chain id is not unique!", "&Cancel" );
			return false;
		}
	}
	return true;
}

void SequenceView::handleSetChain(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );
	Sequence* seq = d_pro->getSequence();
	_ResidueItem* i = (_ResidueItem*)currentItem();

	bool ok	= false;
	QString res = QInputDialog::getText( "Set Chain", 
		"Please enter a unique chain id (alpha-numeric):", QLineEdit::Normal, 
		i->d_res->getChain().data(), &ok, this );
	if( !ok )
		return;
	Root::SymbolString c = res.stripWhiteSpace().toLatin1();
	if( !isValidChain( c ) )
		return;
	Root::SymbolString old = i->d_res->getChain();
	Sequence::ResidueMap::const_iterator j;
	for( j = seq->getResi().begin(); j != seq->getResi().end(); ++j )
	{
		if( (*j).second->getChain() == old )
		{
			seq->setChain( (*j).second, c );
		}
	}
	viewport()->update();
}

void SequenceView::handleAddChain(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );
	try
	{
		const Root::Index next = d_pro->getSequence()->getNextId();
		Root::Ref<SequenceFile> sf;
		QString fileName = QFileDialog::getOpenFileName( this, "Select Sequence File",
                                                          Root::AppAgent::getCurrentDir(),
                                                            "Sequence (*.seq)" );
		if( !fileName.isNull() ) 
		{
			sf = new SequenceFile();
			sf->loadFromFile( d_pro->getRepository(), fileName, next );

			QFileInfo info( fileName );
			Root::AppAgent::setCurrentDir( info.dirPath( true ) );
		}else
			return;

		bool ok	= false;
		QString res = QInputDialog::getText( "Append Chain", 
			"Please enter a unique chain id (alpha-numeric):", QLineEdit::Normal, "", &ok, this );
		if( !ok )
			return;
		Root::SymbolString c = res.stripWhiteSpace().toLatin1();
		if( !isValidChain( c ) )
			return;

		d_pro->getSequence()->addChain( sf, c );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Append Chain", e.what(), "&Cancel" );
		return;
	}

}

void SequenceView::handleRemoveChain(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );
	Sequence* seq = d_pro->getSequence();
	_ResidueItem* i = (_ResidueItem*)currentItem();

	if( QMessageBox::warning( this, "Remove Chain",
		"Do you really want to remove the selected chain (cannot be undone)?",
		"&OK", "&Cancel" ) != 0 )
		return;

	if( !seq->removeChain( i->d_res->getChain() ) )
		QMessageBox::critical( this, "Remove Chain",
			"Chain could not be removed because some residues are assigned", "&Cancel" );
}

void SequenceView::handleRemoveResidue(Root::Action & a)
{
	if( currentItem() == 0 || currentItem()->parent() != 0 )
		return;
	_ResidueItem* i = (_ResidueItem*)currentItem();
	ACTION_ENABLED_IF( a, i && i->d_res->getSystem() == 0 );
	Sequence* seq = d_pro->getSequence();

	if( QMessageBox::warning( this, "Remove Residue",
		"Do you really want to remove the selected residue (cannot be undone)?",
		"&OK", "&Cancel" ) != 0 )
		return;

	seq->removeResi( i->d_res );
}

void SequenceView::handleSetNumberFrom(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );
	Sequence* seq = d_pro->getSequence();
	_ResidueItem* i = (_ResidueItem*)currentItem();

	bool ok = false;
	int res = QInputDialog::getInteger( "Set Residue Number",
		"Enter an arbitrary residue number:", i->d_res->getNr(), -999999, 99999999, 1, &ok, this );
	if( !ok )
		return;
	Root::SymbolString chain = i->d_res->getChain();
	int off = res - i->d_res->getNr();
	Sequence::ResidueMap::const_iterator j = seq->getResi().find( i->d_res->getId() );
	if( j != seq->getResi().begin() )
	{
		--j;
		if( (*j).second->getChain() == chain && (*j).second->getNr() >= res )
		{
			QMessageBox::critical( this, "Set Residue Number",
				"Residue numbers must be in ascending order and may not overlap", "&Cancel" );
			return;
		}
	}

	for( j = seq->getResi().begin(); j != seq->getResi().end(); ++j )
	{
		if( (*j).second->getChain() == chain && (*j).second->getId() >= i->d_res->getId() )
		{
			seq->setNr( (*j).second, (*j).second->getNr() + off );
		}
	}
	viewport()->update();
}

void SequenceView::handleExportChain(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );
	Sequence* seq = d_pro->getSequence();
	_ResidueItem* i = (_ResidueItem*)currentItem();

	QString fileName = QFileDialog::getSaveFileName( this, "Export Sequence File",
                                                     Root::AppAgent::getCurrentDir(),
                                                    "Sequence File (*.seq)" );
	if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );

	if( info.extension( false ).upper() != "SEQ" )
		fileName += ".seq";
	info.setFile( fileName );
	if( info.exists() )
	{
		if( QMessageBox::warning( this, "Save As",
			"This file already exists. Do you want to overwrite it?",
			"&OK", "&Cancel" ) != 0 )
			return;
	}

	Root::Ref<SequenceFile> sf = new SequenceFile();
	Root::SymbolString chain = i->d_res->getChain();
	Sequence::ResidueMap::const_iterator j;
	for( j = seq->getResi().begin(); j != seq->getResi().end(); ++j )
	{
		if( (*j).second->getChain() == chain )
		{
			sf->addResidue( (*j).second, 
						( (*j).second->getSystem() )?(*j).second->getSystem()->getId():0 );
		}
	}

	try
	{
		sf->writeToFile( fileName, true );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Export Sequence File", e.what(), "&Cancel" );
		return;
	}
}


