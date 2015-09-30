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

#include "ResidueTypeView.h"
#include <qinputdialog.h>
#include <qmessagebox.h>
#include <QFileDialog>
#include <qlabel.h>
#include <qlineedit.h> 
#include <qlayout.h>
#include <qpushbutton.h>
#include <Q3HBoxLayout>
#include <Q3GridLayout>
#include <Q3VBoxLayout>
#include <QApplication>
#include <QComboBox>
#include <Root/Application.h>
#include <Root/Any.h>
#include <SpecView/MoleculeViewer.h>
#include <SpecView/DynValueEditor.h>
#include <SpecView/ObjectListView.h>
#include <Spec/ResidueBase.h>
#include <Dlg.h>
#include <Gui/InputDlg.h>
#include "LuaCaraExplorer.h"
using namespace Spec;

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr ResidueTypeView::OpenMolecule = "OpenMolecule";
Root::Action::CmdStr ResidueTypeView::NewType = "NewType";
Root::Action::CmdStr ResidueTypeView::SetDev = "SetDev";
Root::Action::CmdStr ResidueTypeView::SetMean = "SetMean";
Root::Action::CmdStr ResidueTypeView::RemoveType = "RemoveType";
Root::Action::CmdStr ResidueTypeView::RenameType = "RenameType";
Root::Action::CmdStr ResidueTypeView::SetSysType = "SetSysType";
Root::Action::CmdStr ResidueTypeView::EditAtts = "EditAtts";
Root::Action::CmdStr ResidueTypeView::ShowTable = "ShowTable";
Root::Action::CmdStr ResidueTypeView::Reload = "Reload";
Root::Action::CmdStr ResidueTypeView::SetGeneric = "SetGeneric";
Root::Action::CmdStr ResidueTypeView::SetTerminals = "SetTerminals";
Root::Action::CmdStr ResidueTypeView::ImportValues = "ImportValues";
Root::Action::CmdStr ResidueTypeView::ExportValues = "ExportValues";
Root::Action::CmdStr ResidueTypeView::ImportTypes = "ImportTypes";
Root::Action::CmdStr ResidueTypeView::AddIsotope = "AddIsotope";
Root::Action::CmdStr ResidueTypeView::RemoveIsotope = "RemoveIsotope";

ACTION_SLOTS_BEGIN( ResidueTypeView )
    { ResidueTypeView::RemoveIsotope, &ResidueTypeView::handleRemoveIsotope },
    { ResidueTypeView::AddIsotope, &ResidueTypeView::handleAddIsotope },
    { ResidueTypeView::ImportTypes, &ResidueTypeView::handleImportTypes },
    { ResidueTypeView::ExportValues, &ResidueTypeView::handleExportValues },
    { ResidueTypeView::ImportValues, &ResidueTypeView::handleImportValues },
    { ResidueTypeView::SetGeneric, &ResidueTypeView::handleSetGeneric },
    { ResidueTypeView::SetTerminals, &ResidueTypeView::handleSetTerminals },
    { ResidueTypeView::Reload, &ResidueTypeView::handleReload },
    { ResidueTypeView::ShowTable, &ResidueTypeView::handleShowTable },
    { ResidueTypeView::EditAtts, &ResidueTypeView::handleEditAtts },
    { ResidueTypeView::SetSysType, &ResidueTypeView::handleSetSysType },
    { ResidueTypeView::RemoveType, &ResidueTypeView::handleRemoveType },
    { ResidueTypeView::RenameType, &ResidueTypeView::handleRenameType },
    { ResidueTypeView::SetDev, &ResidueTypeView::handleSetDev },
    { ResidueTypeView::SetMean, &ResidueTypeView::handleSetMean },
    { ResidueTypeView::NewType, &ResidueTypeView::handleNewType },
    { ResidueTypeView::OpenMolecule, &ResidueTypeView::handleOpenMolecule },
ACTION_SLOTS_END( ResidueTypeView )

//////////////////////////////////////////////////////////////////////

class _ResidueTypeItem : public Gui::ListViewItem
{
	Root::ExRef<ResidueType> d_rt;
public:
	_ResidueTypeItem( Gui::ListView* p, ResidueType* rt ):Gui::ListViewItem(p),d_rt( rt ) {}
	ResidueType* getRt() const { return d_rt; }

	QString text( int f ) const 
	{ 
		switch( f )
		{
		case 0:
			return d_rt->getName().data();
		case 1:
			return d_rt->getShort().data();
		case 2:
			return d_rt->getLetter().data();
		case 8:
			if( d_rt->getSysType() )
				return d_rt->getSysType()->getName().data();
		default:
			return "";
		}
	}
	// const QPixmap *pixmap( int i ) const { return s_pixRepository; }
};

static char s_buf[64];

class _AtomItem : public Gui::ListViewItem
{
public:
	Root::ExRef<Atom> d_atom;
	_AtomItem( Gui::ListViewItem* p, Atom* a ):Gui::ListViewItem(p),d_atom( a ) {}

	QString text( int f ) const 
	{ 
		switch( f )
		{
		case 0:
			return d_atom->getName().data();
		case 3:
			return d_atom->getType().getIsoLabel();
		case 4:
			::sprintf( s_buf, "%d", d_atom->getNum() );
			return s_buf;
		case 5:
			if( d_atom->getMean() != DisPar::Undefined )
				::sprintf( s_buf, "%.3f", d_atom->getMean() );
			else
				return "";
			return s_buf;
		case 6:
			if( d_atom->getDev() != 0.0 )
				::sprintf( s_buf, "%.3f", d_atom->getDev() );
			else
				return "";
			return s_buf;
		case 7:
			if( d_atom->getGroup() )
				return d_atom->getGroup()->getName().data();
			else
				return "";
		default:
			return "";
		}
	}
	QString key( int f, bool ascending  ) const
	{
		switch( f )
		{
		case 4:
            ::sprintf( s_buf, "%8.3d", d_atom->getNum() );
			return s_buf;
		case 5:
			if( d_atom->getMean() != DisPar::Undefined )
				::sprintf( s_buf, "%08.3f", d_atom->getMean() );
			else
				return "";
			return s_buf;
		case 6:
			if( d_atom->getDev() != 0.0 )
				::sprintf( s_buf, "%08.3f", d_atom->getDev() );
			else
				return "";
			return s_buf;
		default:
			return text( f );
		}
		return s_buf;
	}
	// const QPixmap *pixmap( int i ) const { return s_pixRepository; }
};

class _IsotopeItem : public Gui::ListViewItem
{
public:
	Root::Index d_scheme;
	_IsotopeItem( Gui::ListViewItem* p, Root::Index s ):Gui::ListViewItem(p),d_scheme( s ) {}

	QString text( int f ) const 
	{ 
		_AtomItem* a = (_AtomItem*) parent();
		switch( f )
		{
		case 0:
			{
				LabelingScheme* s = a->d_atom->getOwner()->getOwner()->
					getLabelingScheme( d_scheme );
				if( s )
					return QString( "%1 %2" ).arg( d_scheme ).arg( s->getName() );
				else
					return QString( "%1 ?" ).arg( d_scheme ); 
			}
		case 3:
				return a->d_atom->getType( d_scheme ).getIsoLabel();
		default:
			return "";
		}
	}
	QString key( int f, bool ascending ) const 
	{ 
		switch( f )
		{
		case 0:
			return QString( "%1" ).arg( d_scheme, 8 ); 
		default:
			return text( f );
		}
	}
	// const QPixmap *pixmap( int i ) const { return s_pixRepository; }
};

ResidueTypeView::ResidueTypeView(QWidget* p,Spec::Repository* r):
	Gui::ListView( p ), d_rep( r )
{
	setShowSortIndicator( true );
	setRootIsDecorated( true );
	setAllColumnsShowFocus( true );
	addColumn( "Name" );
	addColumn( "Short" );
	addColumn( "Letter" );
	addColumn( "Atom" );
	addColumn( "Magnitude" );
	addColumn( "Mean" );
	addColumn( "Std.Dev." );
	addColumn( "Group" );
	addColumn( "Type" );
	refill();
	d_rep->addObserver( this );
}

ResidueTypeView::~ResidueTypeView()
{
	d_rep->removeObserver( this );
}

static void _fillAtom( _AtomItem* a )
{
	MetaAtom::Types::const_iterator i;
	for( i = a->d_atom->getTypes().begin(); i != a->d_atom->getTypes().end(); ++i )
		new _IsotopeItem( a, (*i).first );
}

void ResidueTypeView::addItem( ResidueType* rt )
{
	_ResidueTypeItem* rti = new _ResidueTypeItem( this, rt );
	const ResidueType::AtomMap& am = rt->getAtoms();
	ResidueType::AtomMap::const_iterator p2;
	for( p2 = am.begin(); p2 != am.end(); ++p2 )
	{
		_AtomItem* ai = new _AtomItem( rti, (*p2).second );
		_fillAtom( ai );
	}
	rti->setCurrent();
}

void ResidueTypeView::refill()
{
	clear();

	const Repository::ResidueTypeMap& rtm = d_rep->getResidueTypes();
	Repository::ResidueTypeMap::const_iterator p1;
	for( p1 = rtm.begin(); p1 != rtm.end(); ++p1 )
	{
		addItem( (*p1).second );
	}
}

void ResidueTypeView::onCurrentChanged()
{
	if( _AtomItem* i = dynamic_cast<_AtomItem*>( currentItem() ) )
	{
		LuaCaraExplorer::setCurrentResidueType( i->d_atom->getOwner() );
	}else if( _ResidueTypeItem* i = dynamic_cast<_ResidueTypeItem*>( currentItem() ) )
	{
		LuaCaraExplorer::setCurrentResidueType( i->getRt() );
	}else if( _IsotopeItem* i = dynamic_cast<_IsotopeItem*>( currentItem() ) )
	{
		_AtomItem* a = (_AtomItem*) i->parent();
		LuaCaraExplorer::setCurrentResidueType( a->d_atom->getOwner() );
	}
}

void ResidueTypeView::handleOpenMolecule(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );

	_ResidueTypeItem* i = (_ResidueTypeItem*) currentItem();
	QApplication::setOverrideCursor( Qt::waitCursor );
	MoleculeViewer* mv = new MoleculeViewer( 0, i->getRt() );
	mv->show();
	QApplication::restoreOverrideCursor();

}

Gui::Menu* ResidueTypeView::createPopup()
{
	Gui::Menu* pop = new Gui::Menu();
	Gui::Menu::item( pop,  "Open Molecule Viewer...", OpenMolecule, false );
	pop->insertSeparator();
	Gui::Menu::item( pop,  "New Residue Type...", NewType, false );
	Gui::Menu::item( pop,  "Import Types...", ImportTypes, false );
	Gui::Menu::item( pop,  "Rename Type...", RenameType, false );
	Gui::Menu::item( pop,  "Set Spin System Type...", SetSysType, false );
	Gui::Menu::item( pop,  "Delete Type...", RemoveType, false );
	Gui::Menu::item( pop,  "Import Values...", ImportValues, false );
	Gui::Menu::item( pop,  "Export Values...", ExportValues, false );
	Gui::Menu::item( pop,  "Edit Attributes...", EditAtts, false );
	Gui::Menu::item( pop,  "Open Object Table...", ShowTable, false );
	pop->insertSeparator();
	Gui::Menu::item( pop,  "Set Terminals...", SetTerminals, false );
	Gui::Menu::item( pop,  "Generic Residue Type", SetGeneric, true );
	pop->insertSeparator();
	Gui::Menu::item( pop,  "Set Mean...", SetMean, false );
	Gui::Menu::item( pop,  "Set Deviation...", SetDev, false );
	pop->insertSeparator();
	Gui::Menu::item( pop,  "Add Isotope...", AddIsotope, false );
	Gui::Menu::item( pop,  "Remove Isotope", RemoveIsotope, false );
	pop->insertSeparator();
	Gui::Menu::item( pop,  "Reload", Reload, false );
	return pop;
}

void ResidueTypeView::handle(Root::Message & msg)
{
	BEGIN_HANDLER();
	MESSAGE( ResidueType::Added, a, msg )
	{
		addItem( a->sender() );
	}
	MESSAGE( ResidueType::Removed, a , msg )
	{
		Gui::ListViewItem* i = findItem( a->sender() );
		if( i )
			i->removeMe();
	}
	MESSAGE( Atom::Added, a, msg )
	{
		Gui::ListViewItem* i = findItem( a->sender()->getOwner() );
		_AtomItem* ai = new _AtomItem( i, static_cast<Atom*>( a->sender() ) );
		_fillAtom( ai );
	}
	MESSAGE( Atom::Removed, a, msg )
	{
		Gui::ListViewItem* i = findItem( a->sender()->getOwner() );
		for( int j = 0; j < i->count(); j++ )
		{
			if( static_cast<_AtomItem*>( i->child( j ) )->d_atom == a->sender() )
			{
				i->child( j )->removeMe();
				break;
			}
		}
	}
	MESSAGE( MetaAtom::Changed, a, msg )
	{
		if( a->d_hint == MetaAtom::AtomTypes )
		{
			Gui::ListViewItem* i = findItem( a->sender()->getOwner() );
			if( i )
			{
				for( int j = 0; j < i->count(); j++ )
				{
					_AtomItem* ai = static_cast<_AtomItem*>( i->child( j ) );
					if( ai->d_atom == a->sender() )
					{
						ai->clearChildren();
						_fillAtom( ai );
						break;
					}
				}
			}
		}
	}
	MESSAGE( Root::Action, a, msg )
	{
		EXECUTE_ACTION( ResidueTypeView, *a );
	}
	END_HANDLER();
}

Gui::ListViewItem* ResidueTypeView::findItem( ResidueType* rt ) const
{
	for( int i = 0; i < count(); i++ )
		if( static_cast<_ResidueTypeItem*>( child(i) )->getRt() == rt )
			return child(i);
	return 0;
}

void ResidueTypeView::handleNewType(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );
	QDialog dlg( this, "", true );
	dlg.setCaption( "New Residue Type" );

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	Q3GridLayout* contents = new Q3GridLayout( 4, 2 );

	contents->addWidget( new QLabel( "Name:", &dlg ), 1, 0 ); 
	QLineEdit* _name = new QLineEdit( &dlg );
	contents->addWidget( _name, 1, 1 );

	contents->addWidget( new QLabel( "Short:", &dlg ), 2, 0 ); 
	QLineEdit* _short = new QLineEdit( &dlg );
	contents->addWidget( _short, 2, 1 );

	contents->addWidget( new QLabel( "Letter:", &dlg ), 3, 0 ); 
	QLineEdit* _let = new QLineEdit( &dlg );
	contents->addWidget( _let, 3, 1 );

	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( contents );
	top->addLayout( buttons );

	while( dlg.exec() == QDialog::Accepted )
	{
		ResidueType* rt = d_rep->addResidueType( _name->text().toLatin1(),
			_short->text().toLatin1(), _let->text().toLatin1() );
		if( rt == 0 )
		{
			QMessageBox::critical( this, "New Residue Type",
					"Non unique or empty identifiers!", "&Cancel" );
		}else
			return;
	}
}

void ResidueTypeView::handleSetDev(Root::Action & a)
{
	_AtomItem* i = dynamic_cast<_AtomItem*>( currentItem() );
	ACTION_ENABLED_IF( a, i );

	_ResidueTypeItem* rt = (_ResidueTypeItem*) i->parent();

	bool ok;

	DisPar dp = i->d_atom->getDisPar();
	QString	res;
	if( dp.d_dev != 0.0 )
		res.setNum( dp.d_dev );
	else
		res = "-";
	res = QInputDialog::getText( "Set Deviation", 
		"Please insert a new deviation value or '-'", QLineEdit::Normal, res, &ok, this );
	if( !ok )
		return;
	if( res == "-" )
	{
		dp.d_dev = 0.0;
	}else
	{
		dp.d_dev = res.toFloat( &ok );
		if( !ok || dp.d_dev < 0.0 )
		{
			QMessageBox::critical( this, "Set Deviation",
					"Invalid deviation value!", "&Cancel" );
			return;
		}
	}
	rt->getRt()->setDisPar( i->d_atom, dp );
}

void ResidueTypeView::handleSetMean(Root::Action & a)
{
	_AtomItem* i = dynamic_cast<_AtomItem*>( currentItem() );
	ACTION_ENABLED_IF( a, i );

	_ResidueTypeItem* rt = (_ResidueTypeItem*) i->parent();

	bool ok;

	DisPar dp = i->d_atom->getDisPar();
	QString	res;
	if( dp.d_mean != DisPar::Undefined )
		res.setNum( dp.d_mean );
	else
		res = "-";
	res = QInputDialog::getText( "Set Mean", 
		"Please insert a new mean value or '-'", QLineEdit::Normal, res, &ok, this );
	if( !ok )
		return;
	if( res == "-" )
	{
		dp.d_mean = DisPar::Undefined;
	}else
	{
		dp.d_mean = res.toFloat( &ok );
		if( !ok )
		{
			QMessageBox::critical( this, "Set Mean",
					"Invalid mean value!", "&Cancel" );
			return;
		}
	}
	rt->getRt()->setDisPar( i->d_atom, dp );
}

void ResidueTypeView::handleRemoveType(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );

	_ResidueTypeItem* item = (_ResidueTypeItem*) currentItem();

	if( QMessageBox::warning( this, "Delete Residue Type",
		"Do you really want to delete this type (cannot be undone)?",
		"&OK", "&Cancel" ) != 0 )
		return;
	if( !d_rep->remove( item->getRt() ) )
		QMessageBox::critical( this, "Delete Residue Type", 
			"Cannot delete type since it is in use!", "&Cancel" );
}

void ResidueTypeView::handleRenameType(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );

	_ResidueTypeItem* item = (_ResidueTypeItem*) currentItem();
	bool ok;
	QString res = QInputDialog::getText( "Rename Residue Type", 
		"Please choose another name:", QLineEdit::Normal, item->getRt()->getName().data(), &ok, this );
	if( !ok )
		return;
	if( !d_rep->rename( item->getRt(), res ) )
		QMessageBox::critical( this, "Rename Residue Type", 
			"Cannot rename since name is already in use!", "&Cancel" );
}

void ResidueTypeView::handleSetSysType(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );

	_ResidueTypeItem* item = (_ResidueTypeItem*) currentItem();

	Repository::SystemTypeMap::const_iterator p;
	const Repository::SystemTypeMap& sm = d_rep->getSystemTypes();

	int cur = 0, i = 0;
	QStringList l;
	l.append( "" );
	for( p = sm.begin(); p != sm.end(); ++p, i++ )
	{
		l.append( (*p).second->getName().data() ); // NOTE: Name sollte Unique sein.
	}
	l.sort();
	if( item->getRt()->getSysType() )
		cur = l.findIndex( item->getRt()->getSysType()->getName().data() );

	bool ok;
	QString res = QInputDialog::getItem( "Set System Type", "Select a spin system type:", 
		l, cur, false, &ok, this );
	if( !ok )
		return;

	SystemType* sst = 0;
	for( p = sm.begin(); p != sm.end(); ++p )
		if( res == (*p).second->getName().data() )
		{
			sst = (*p).second;
			break;
		}
	d_rep->getTypes()->setSysType( item->getRt(), sst );
}

void ResidueTypeView::handleEditAtts(Root::Action & a)
{
	_ResidueTypeItem* item = dynamic_cast<_ResidueTypeItem*>( currentItem() );
	_AtomItem* ai = dynamic_cast<_AtomItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item || ai );

	if( item )
		DynValueEditor::edit( this, d_rep->findObjectDef( Repository::keyResidueType ),
			item->getRt() );
	else
		DynValueEditor::edit( this, d_rep->findObjectDef( Repository::keyAtom ),
			ai->d_atom );
}

void ResidueTypeView::handleShowTable(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );

	int i = 0;
	_AtomItem* ai = dynamic_cast<_AtomItem*>( currentItem() );
	if( ai == 0 )
	{
		const Repository::ResidueTypeMap& sm = d_rep->getResidueTypes();
		Repository::ResidueTypeMap::const_iterator p;
		ObjectListView::ObjectList o( sm.size() );
		for( p = sm.begin(); p != sm.end(); ++p, ++i)
		{
			o[ i ] = (*p).second;
		}
		ObjectListView::edit( this, d_rep->findObjectDef( Repository::keyResidueType ), o );
	}else
	{
		const ResidueType::AtomMap& am = ai->d_atom->getOwner()->getAtoms();
		ResidueType::AtomMap::const_iterator p;
		ObjectListView::ObjectList o( am.size() );
		for( p = am.begin(); p != am.end(); ++p, ++i)
		{
			o[ i ] = (*p).second;
		}
		ObjectListView::edit( this, d_rep->findObjectDef( Repository::keyAtom ), o );
	}
}

void ResidueTypeView::handleReload(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );
	refill();
}

void ResidueTypeView::handleSetGeneric(Root::Action & a)
{
	_ResidueTypeItem* item = dynamic_cast<_ResidueTypeItem*>( currentItem() );
	ACTION_CHECKED_IF( a, item, item && d_rep->getTypes()->getGenFromType( 0 ) == item->getRt() );

	if( d_rep->getTypes()->getGenFromType( 0 ) == item->getRt() )
		d_rep->getTypes()->setGenSys( 0 );
	else
		d_rep->getTypes()->setGenSys( item->getRt() );
}

void ResidueTypeView::handleSetTerminals(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );
	Root::SymbolString n = d_rep->getTypes()->getNTerm( 0 );
	Root::SymbolString c = d_rep->getTypes()->getCTerm( 0 );
	if( Dlg::getLabelPair( this, n, c, "Select Terminals", 
		"N-Terminal (i-1):", "C-Terminal (i+1):" ) )
	{
		try
		{
			d_rep->getTypes()->setTerms( n, c );
		}catch( Root::Exception& e )
		{
			QMessageBox::critical( this, "Set Terminals", e.what(), "&Cancel" );
			return;
		}
	}
}

void ResidueTypeView::handleImportValues(Root::Action & a)
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
		if( !db.read( fileName, d_rep ) )
		{
			QMessageBox::critical( this, "Error Import Values",
					db.d_result.data(), "&Cancel" );
		}else
			refill();
	}else if( info.extension(false).upper() == "STATS" )
	{
		if( !db.readStats( fileName, d_rep ) )
		{
			QMessageBox::critical( this, "Error Import Values",
					db.d_result.data(), "&Cancel" );
		}else
			refill();
	}else
	{
		QMessageBox::critical( this, "Error Import Values",
				"Invalid File Format", "&Cancel" );
	}
	QApplication::restoreOverrideCursor();
}

void ResidueTypeView::handleExportValues(Root::Action & a)
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
	if( !db.write( fileName, d_rep->getTypes() ) )
	{
		QMessageBox::critical( this, "Error Export Values",
				db.d_result.data(), "&Cancel" );
	}
}

void ResidueTypeView::handleImportTypes(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );

	QString fileName = QFileDialog::getOpenFileName( this, "Select Repository",
                                                     Root::AppAgent::getCurrentDir(),
                                                    "Repository (*.cara)" );
    if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );
	Root::AppAgent::setCurrentDir( info.dirPath( true ) );

	try
	{
		QApplication::setOverrideCursor( Qt::waitCursor );
		Root::Ref<Repository> temp = new Repository();
		temp->load( fileName, true );
		Dlg::StringList l;
		{
			const Repository::ResidueTypeMap& rt = temp->getResidueTypes();
			Repository::ResidueTypeMap::const_iterator i;
			for( i = rt.begin(); i != rt.end(); ++i )
				l.push_back( (*i).first.data() );
		}
		QApplication::restoreOverrideCursor();
		if( !Dlg::getStrings( this, l, "Select Residue Types" ) )
			return;

		typedef std::map<Root::SymbolString,Root::SymbolString> Trans;
		Trans t;
		{
			bool ok;
            for( int j = 0; j < int(l.size()); j++ )
			{
				t[ l[ j ] ] = l[ j ];
				while( d_rep->getTypes()->findResidueType( l[ j ].data() ) )
				{
					QString res = QInputDialog::getText( "Import Residue Types", 
						"Please choose another name:", QLineEdit::Normal, 
						l[ j ].data(), &ok, this );
					if( !ok )
						return;
					t[ l[ j ] ] = res.toLatin1();
					l[ j ] = res.toLatin1();
				}
			}
		}
		Trans::const_iterator k;
		ResidueType* rt;
		for( k = t.begin(); k != t.end(); ++k )
		{
			rt = temp->getTypes()->findResidueType( (*k).first );
			d_rep->getTypes()->addResidueType( new ResidueType( (*k).second, *rt ) );
			temp->getTypes()->remove( rt );
		}
		refill();
	}catch( Root::Exception& e )
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::critical( this, "Error Importing Types", e.what(), "&Cancel" );
	}
}

void ResidueTypeView::handleRemoveIsotope(Root::Action & t)
{
	_IsotopeItem* item = dynamic_cast<_IsotopeItem*>( currentItem() );
	ACTION_ENABLED_IF( t, item );

	_AtomItem* a = (_AtomItem*)item->parent();
	_ResidueTypeItem* rt = (_ResidueTypeItem*)a->parent();
	rt->getRt()->removeAtomType( a->d_atom, item->d_scheme );
}

void ResidueTypeView::handleAddIsotope(Root::Action & a)
{
	_AtomItem* item = dynamic_cast<_AtomItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	_ResidueTypeItem* rt = (_ResidueTypeItem*)item->parent();

	Gui::InputDlg dlg( this, "Add Isotope" );

	TypeBase::LabelingSchemeMap::const_iterator l;
	const TypeBase::LabelingSchemeMap& ls = rt->getRt()->getOwner()->getLabelingSchemes();
	QComboBox _scheme;
	for( l = ls.begin(); l != ls.end(); ++l )
		_scheme.addItem( (*l).second->getName(), (*l).first );
	QComboBox _atom;
	for( int i = AtomType::H1; i < AtomType::MaxIsotope; i++ )
		_atom.addItem( AtomType::s_labels[ i ] );
	dlg.addLabel( "Labeling Scheme:", 0, 0 );
	dlg.add( &_scheme, 0, 1 );
	dlg.addLabel( "Isotope:", 1, 0 );
	dlg.add( &_atom, 1, 1 );
	if( dlg.exec() )
	{
		int i = _scheme.currentIndex();
		assert( i != -1 );
		Root::Index scheme = _scheme.itemData( i ).toInt();
		if( !rt->getRt()->addAtomType( item->d_atom, scheme, 
			AtomType::parseLabel( _atom.currentText().toLatin1() ) ) )
				QMessageBox::critical( this, "Adding Isotope", "Please use another Labeling Scheme!" );
	}
}



