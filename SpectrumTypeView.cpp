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

#include "SpectrumTypeView.h"
#include <qinputdialog.h>
#include <QFileDialog>
#include <qfileinfo.h> 
#include <qmessagebox.h>
#include <qlabel.h>
#include <qlineedit.h> 
#include <qlayout.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qcheckbox.h> 
#include <q3multilineedit.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <Q3GridLayout>
#include <Q3Frame>
#include <Q3VBoxLayout>
#include <QApplication>
#include <Root/Any.h>
#include <stdio.h>
#include <Root/Application.h>
#include <SpecView/DynValueEditor.h>
#include <SpecView/ObjectListView.h>
#include <Spec/NmrExperiment.h>
#include <Spec/SpinPointSpace.h>
#include <strstream>
TODO( strstream ersetzen );
// NOTE <sstream> bzw. ostringstream auf Irix nicht vorhanden!
#include <Root/Vector.h>
#include <Dlg.h>
#include "LuaCaraExplorer.h"
#include <SpecView/PathSimDlg.h>
using namespace Spec;
 
static char s_buf[64];

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr SpectrumTypeView::AddType = "AddType";
Root::Action::CmdStr SpectrumTypeView::AddLabel = "AddLabel";
Root::Action::CmdStr SpectrumTypeView::RemoveLabel = "RemoveLabel";
Root::Action::CmdStr SpectrumTypeView::EditOrder = "EditOrder";
Root::Action::CmdStr SpectrumTypeView::RemoveType = "RemoveType";
Root::Action::CmdStr SpectrumTypeView::RenameType = "RenameType";
Root::Action::CmdStr SpectrumTypeView::DuplicateType = "DuplicateType";
Root::Action::CmdStr SpectrumTypeView::ShowPath = "ShowPath";
Root::Action::CmdStr SpectrumTypeView::EditProc = "EditProc";
Root::Action::CmdStr SpectrumTypeView::EditAtts = "EditAtts";
Root::Action::CmdStr SpectrumTypeView::ShowTable = "ShowTable";
Root::Action::CmdStr SpectrumTypeView::PeakWidth = "PeakWidth";
Root::Action::CmdStr SpectrumTypeView::ReverseProc = "ReverseProc";
Root::Action::CmdStr SpectrumTypeView::ImportTypes = "ImportTypes";

ACTION_SLOTS_BEGIN( SpectrumTypeView )
    { SpectrumTypeView::ImportTypes, &SpectrumTypeView::handleImportTypes },
    { SpectrumTypeView::ReverseProc, &SpectrumTypeView::handleReverseProc },
    { SpectrumTypeView::PeakWidth, &SpectrumTypeView::handlePeakWidth },
    { SpectrumTypeView::ShowTable, &SpectrumTypeView::handleShowTable },
    { SpectrumTypeView::EditAtts, &SpectrumTypeView::handleEditAtts },
    { SpectrumTypeView::EditProc, &SpectrumTypeView::handleEditProc },
    { SpectrumTypeView::ShowPath, &SpectrumTypeView::handleShowPath },
    { SpectrumTypeView::DuplicateType, &SpectrumTypeView::handleDuplicateType },
    { SpectrumTypeView::RemoveType, &SpectrumTypeView::handleRemoveType },
    { SpectrumTypeView::RenameType, &SpectrumTypeView::handleRenameType },
    { SpectrumTypeView::AddType, &SpectrumTypeView::handleAddType },
    { SpectrumTypeView::AddLabel, &SpectrumTypeView::handleAddLabel },
    { SpectrumTypeView::RemoveLabel, &SpectrumTypeView::handleRemoveLabel },
    { SpectrumTypeView::EditOrder, &SpectrumTypeView::handleEditOrder },
ACTION_SLOTS_END( SpectrumTypeView )

//////////////////////////////////////////////////////////////////////

class _SpectrumTypeItem : public Gui::ListViewItem
{
public:
	_SpectrumTypeItem( Gui::ListView* p, SpectrumType* spec ):
		Gui::ListViewItem(p),d_type( spec ) {}

	Root::ExRef<SpectrumType> d_type;

	QString text( int f ) const 
	{ 
		switch( f )
		{
		case 0:
			return d_type->getName().data();
		default:
			return "";
		}
	}
};

class _TypeDimItem : public Gui::ListViewItem
{
public:
	_TypeDimItem( Gui::ListViewItem* p, SpectrumType* spec, Dimension d ):
		Gui::ListViewItem(p),d_type( spec ), d_dim( d ) {}

	Root::ExRef<SpectrumType> d_type;
	Dimension d_dim;

	QString text( int f ) const 
	{ 
		switch( f )
		{
		case 0:
			::sprintf( s_buf, "Dim. %s: %s", getDimSymbolLetter( d_dim ), 
				d_type->getName( d_dim ).data() );
			return s_buf;
		case 1:
			return d_type->getColor( d_dim ).getIsoLabel();
		default:
			return "";
		}
	}
};

class _DimLabelItem : public Gui::ListViewItem
{
public:
	_DimLabelItem( Gui::ListViewItem* p, const SpinLabel& l ):
		Gui::ListViewItem(p),d_lbl( l ) {}
	SpinLabel d_lbl;

	QString text( int f ) const 
	{ 
		switch( f )
		{
		case 0:
			d_lbl.format( s_buf, sizeof( s_buf ) );
			return s_buf;
		default:
			return "";
		}
	}
};

//////////////////////////////////////////////////////////////////////

SpectrumTypeView::SpectrumTypeView(QWidget* p,Spec::Repository* r):
	Gui::ListView( p ), d_rep( r )
{
	setShowSortIndicator( true );
	setRootIsDecorated( true );
	setAllColumnsShowFocus( true );
	addColumn( "Name" );
	addColumn( "Type" );
	refill();
	d_rep->addObserver( this );
}

SpectrumTypeView::~SpectrumTypeView()
{
	d_rep->removeObserver( this );
}

void SpectrumTypeView::onCurrentChanged()
{
	if( _SpectrumTypeItem* i = dynamic_cast<_SpectrumTypeItem*>( currentItem() ) )
		LuaCaraExplorer::setCurrentSpectrumType( i->d_type );
	else if( _TypeDimItem* i = dynamic_cast<_TypeDimItem*>( currentItem() ) )
		LuaCaraExplorer::setCurrentSpectrumType( i->d_type );
	else if( _DimLabelItem* i = dynamic_cast<_DimLabelItem*>( currentItem() ) )
	{
		_TypeDimItem* t = (_TypeDimItem*) i->parent();
		LuaCaraExplorer::setCurrentSpectrumType( t->d_type );
	}
}

Gui::Menu* SpectrumTypeView::createPopup()
{
    Gui::Menu* pop = new Gui::Menu();
	Gui::Menu::item( pop, "Add Type...", AddType, false );
	Gui::Menu::item( pop,  "Import Types...", ImportTypes, false );
	Gui::Menu::item( pop,  "Duplicate Type...", DuplicateType, false );
	Gui::Menu::item( pop,  "Edit Type...", EditOrder, false );
	Gui::Menu::item( pop,  "Rename Type...", RenameType, false );
	Gui::Menu::item( pop,  "Delete Type...", RemoveType, false );
	Gui::Menu::item( pop,  "Edit Attributes...", EditAtts, false );
	Gui::Menu::item( pop,  "Open Object Table...", ShowTable, false );
	pop->insertSeparator();
	Gui::Menu::item( pop,  "Edit Procedure...", EditProc, false );
	Gui::Menu::item( pop,  "Reverse Steps...", ReverseProc, false );
	Gui::Menu::item( pop,  "Show Experiment Path...", ShowPath, false );
	pop->insertSeparator();
	Gui::Menu::item( pop,  "Set Peak Width...", PeakWidth, false );
	Gui::Menu::item( pop,  "Add Label...", AddLabel, false );
	Gui::Menu::item( pop,  "Remove Label", RemoveLabel, false );
    return pop;
}

static void _fill( _TypeDimItem* i, SpectrumType* st, Dimension d )
{
	const SpinLabelSet& sls = st->getLabels( d );
	SpinLabelSet::const_iterator p2;
	for( p2 = sls.begin(); p2 != sls.end(); ++p2 )
		new _DimLabelItem( i, (*p2) );
}

static void _fill( _SpectrumTypeItem* i, SpectrumType* st )
{
	for( Dimension d = 0; d < st->getDimCount(); d++ )
	{
		_TypeDimItem* di = new _TypeDimItem( i, st, d );
		_fill( di, st, d );
	}
}

void SpectrumTypeView::refill()
{
	clear();

	QString str;
	const Repository::SpectrumTypeMap& stm = d_rep->getSpecTypes();
	Repository::SpectrumTypeMap::const_iterator p1;
	for( p1 = stm.begin(); p1 != stm.end(); ++p1 )
	{
		_SpectrumTypeItem* sti = new _SpectrumTypeItem( this, (*p1).second );
		_fill( sti, (*p1).second );
	}
	QMetaObject::invokeMethod( this, "resizeFirstColumnToContents", Qt::QueuedConnection );
}

static _SpectrumTypeItem* _find( Gui::ListView* v, SpectrumType* st )
{
	for( int i = 0; i < v->count(); i++ )
		if( static_cast<_SpectrumTypeItem*>( v->child(i) )->d_type == st )
		{
			return static_cast<_SpectrumTypeItem*>( v->child(i) );
		}
	return 0;
}

static _TypeDimItem* _find( _SpectrumTypeItem* t, Dimension d )
{
	if( t == 0 )
		return 0;
	for( int i = 0; i < t->count(); i++ )
		if( static_cast<_TypeDimItem*>( t->child(i) )->d_dim == d )
		{
			return static_cast<_TypeDimItem*>( t->child(i) );
		}
	return 0;
}

void SpectrumTypeView::handle(Root::Message & msg)
{
	BEGIN_HANDLER();
	MESSAGE( SpectrumType::Added, a, msg )
	{
		_SpectrumTypeItem* sti = new _SpectrumTypeItem( this, a->sender() );
		_fill( sti, a->sender() );
		sti->setCurrent();
	}
	MESSAGE( SpectrumType::Removed, a, msg )
	{
		_SpectrumTypeItem* st = _find( this, a->sender() );
		if( st )
			st->removeMe();
	}
	MESSAGE( SpectrumType::Changed, a, msg )
	{
		if( a->d_hint == SpectrumType::Labels )
		{
			_TypeDimItem* t = _find( _find( this, a->sender() ), a->d_value );
			if( t )
			{
				t->clearChildren();
				_fill( t, a->sender(), a->d_value );
			}
		}
	}
	MESSAGE( Root::Action, a, msg )
	{
		EXECUTE_ACTION( SpectrumTypeView, *a );
	}
	END_HANDLER();
}

void SpectrumTypeView::handleAddType(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );

	const char* title = "New Spectrum Type";
	QDialog dlg( this, "", true );
	dlg.setCaption( title );

	const Dimension maxDim = 4;
	Root::Vector<QLineEdit*> _labels( maxDim );
	Root::Vector<QComboBox*> _atoms( maxDim );

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	Q3GridLayout* contents = new Q3GridLayout( 7, maxDim + 1 );

	contents->addWidget( new QLabel( "Name:", &dlg ), 0, 0 ); 
	QLineEdit* _name = new QLineEdit( &dlg );
	contents->addWidget( _name, 0, 1 );

	contents->addWidget( new QLabel( "Atom:", &dlg ), 2, 0 ); 
	contents->addWidget( new QLabel( "Dim. Name:", &dlg ), 3, 0 ); 

	QString str;
	for( Dimension d = 0; d < maxDim; d++ )
	{
		str.sprintf( "Dim. %s", getDimSymbolLetter( d ) );
		contents->addWidget( new QLabel( str, &dlg ), 1, d + 1 ); 

		_atoms[ d ] = new QComboBox( &dlg );
		for( int i = AtomType::None; i <= AtomType::Lr; i++ )
			_atoms[ d ]->insertItem( AtomType::s_labels[ i ] );
		_atoms[ d ]->setCurrentItem( 0 );
		contents->addWidget( _atoms[ d ], 2, d + 1 );

		_labels[ d ] = new QLineEdit( &dlg );
		contents->addWidget( _labels[ d ], 3, d + 1 );
	}

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
		Dimension count = 0;
		Dimension d;
		for( d = 0; d < maxDim; d++ )
		{
			if( _atoms[ d ]->currentItem() == 0 )
				break;
			else
				count = d + 1;
		}
		if( count == 0 )
		{
			QMessageBox::critical( this, title, 
				"Type needs at least one dimenstion!", "&Cancel" );
			continue;
		} 
		str = _name->text().stripWhiteSpace();
		if( str.isEmpty() )
		{
			QMessageBox::critical( this, title, 
				"Type needs a name!", "&Cancel" );
			continue;
		}
		Root::Ref<SpectrumType> t = new SpectrumType( str, count );
		for( d = 0; d < count; d++ )
		{
			t->setColor( d, AtomType::Isotope( _atoms[ d ]->currentItem() ) );
			t->setName( d, _labels[ d ]->text() );
		}
		if( !d_rep->addSpectrumType( t ) )
		{
			QMessageBox::critical( this, title, 
				"Cannot add this spectrum type to repository!", "&Cancel" );
			continue;
		}else
			break;
	}
}

void SpectrumTypeView::handleAddLabel(Root::Action & a)
{
	_TypeDimItem* i = dynamic_cast<_TypeDimItem*>( currentItem() );
	ACTION_ENABLED_IF( a, i );

	bool ok	= FALSE;
	QString res;
	res.sprintf( "Please enter a valid label (%s):", SpinLabel::s_syntax );
	res = QInputDialog::getText( "Add Label", res, QLineEdit::Normal, "", &ok, this );
	if( !ok )
		return;

	SpinLabel l;
	if( !SpinLabel::parse( res, l ) )
	{
		QMessageBox::critical( this, "Add Label", 
			"Invalid label syntax!", "&Cancel" );
		return;
	}
	i->d_type->addLabel( i->d_dim, l );
	i->setOpen( true );
}

void SpectrumTypeView::handleRemoveLabel(Root::Action & a)
{
	_DimLabelItem* i = dynamic_cast<_DimLabelItem*>( currentItem() );
	ACTION_ENABLED_IF( a, i );
	_TypeDimItem* t = (_TypeDimItem*) i->parent();
	t->d_type->removeLabel( t->d_dim, i->d_lbl );
}

void SpectrumTypeView::handleEditOrder(Root::Action & a)
{
	_SpectrumTypeItem* item = dynamic_cast<_SpectrumTypeItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	const char* title = "Edit Spectrum Type";
	QDialog dlg( this, "", true );
	dlg.setCaption( title );

	SpectrumType* t = item->d_type;
	const Dimension maxDim = item->d_type->getDimCount();
	Root::Vector<QLineEdit*> _labels( maxDim );

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	Q3GridLayout* contents = new Q3GridLayout( 7, maxDim + 1 );

	contents->addWidget( new QLabel( "Dim. Name:", &dlg ), 3, 0 ); 

	QString str;
	for( Dimension d = 0; d < maxDim; d++ )
	{
		str.sprintf( "Dim. %s", getDimSymbolLetter( d ) );
		contents->addWidget( new QLabel( str, &dlg ), 1, d + 1 ); 

		_labels[ d ] = new QLineEdit( &dlg );
		_labels[ d ]->setText( t->getName( d ).data() ); 
		contents->addWidget( _labels[ d ], 3, d + 1 );
	}

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
		for( Dimension d = 0; d < maxDim; d++ )
		{
			t->setName( d, _labels[ d ]->text() );
		}
		break;
	}
}

void SpectrumTypeView::handleRemoveType(Root::Action & a)
{
	_SpectrumTypeItem* item = dynamic_cast<_SpectrumTypeItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	if( QMessageBox::warning( this, "Delete Spectrum Type",
		"Do you really want to delete this type (cannot be undone)?",
		"&OK", "&Cancel" ) != 0 )
		return;
	if( !d_rep->remove( item->d_type ) )
		QMessageBox::critical( this, "Delete Spectrum Type", 
			"Cannot delete type since it is in use!", "&Cancel" );
}

void SpectrumTypeView::handleRenameType(Root::Action & a)
{
	_SpectrumTypeItem* item = dynamic_cast<_SpectrumTypeItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	bool ok;
	QString res = QInputDialog::getText( "Rename Spectrum Type", 
		"Please choose another name:", QLineEdit::Normal, item->d_type->getName().data(), &ok, this );
	if( !ok )
		return;
	if( !d_rep->rename( item->d_type, res ) )
		QMessageBox::critical( this, "Rename Spectrum Type", 
			"Cannot rename since name is already in use!", "&Cancel" );
}

void SpectrumTypeView::handleDuplicateType(Root::Action & a)
{
	_SpectrumTypeItem* item = dynamic_cast<_SpectrumTypeItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	bool ok;
	QString res = QInputDialog::getText( "Duplicate Spectrum Type", 
		"Please choose a new name:", QLineEdit::Normal, "", &ok, this );
	if( !ok || res.isEmpty() )
		return;

	Root::Ref<SpectrumType> t = new SpectrumType( res, *item->d_type );
	if( !d_rep->addSpectrumType( t ) )
	{
		QMessageBox::critical( this, "Duplicate Spectrum Type", 
			"Cannot add this spectrum type to repository!", "&Cancel" );
	}
}

void SpectrumTypeView::handleShowPath(Root::Action & a)
{
	_SpectrumTypeItem* item = dynamic_cast<_SpectrumTypeItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );
	SpectrumType* t = item->d_type;

    PathSimDlg dlg( t, this );
    dlg.exec();
    return;

    // old version:
//	const Repository::ResidueTypeMap& rm = d_rep->getResidueTypes();
//	Repository::ResidueTypeMap::const_iterator p;
//	QStringList l;
//	for( p = rm.begin(); p != rm.end(); ++p )
//		l.append( (*p).second->getShort().data() );
//	l.sort();
//	bool ok;
//	QString res = QInputDialog::getItem( "Show Experiment Path",
//		"Select a residue type to execute:",
//		l, 0, false, &ok, this );
//	if( !ok )
//		return;
//	ResidueType* rt = d_rep->findResidueType( res );
//	assert( rt );
//	Root::Ref<NmrExperiment> nmr = d_rep->getTypes()->inferExperiment2( t, 0, rt );
//	std::ostrstream out;

//	nmr->printTable( out );
//	Q3MultiLineEdit* w = new Q3MultiLineEdit();
//	QString str;
//	str.sprintf( "Experiment Path: %s on %s", t->getName().data(), rt->getShort().data() );
//	w->setCaption( str );
//	w->setText( QByteArray ( out.str(), out.pcount() ).data() );
//	out.rdbuf()->freeze(false);
//	w->setReadOnly( true );
//	w->show();
}

void SpectrumTypeView::handleEditProc(Root::Action & a)
{
	_SpectrumTypeItem* item = dynamic_cast<_SpectrumTypeItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	const char* title = "Edit Experiment Procedure";
	QDialog dlg( this, "", true );
	dlg.setCaption( title );

	SpectrumType* t = item->d_type;
	const Dimension maxStep = 6;			// RISK
	Root::Vector<QComboBox*> _atoms( maxStep );
	Root::Vector<QLineEdit*> _names( maxStep );
	Root::Vector<QComboBox*> _dims( maxStep );
	Root::Vector<QLineEdit*> _hops( maxStep );
	Root::Vector<QCheckBox*> _rep( maxStep );
	Root::Vector<QLineEdit*> _means( maxStep );
	Root::Vector<QLineEdit*> _devs( maxStep );

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	Q3GridLayout* contents = new Q3GridLayout( 9, maxStep + 1 );

	std::ostrstream out;
	out << "Spectrum Type " << t->getName().data() << ":" << std::endl;
	int d;
	for( d = 0; d < t->getDimCount(); d++ )
	{
		if( d != 0 )
			out << std::endl;
		out << "Dim. " << getDimSymbolLetter( d ) << ": " << t->getColor( d ).getIsoLabel() <<
		" (" << t->getName( d ).data() << ") ";
	}
	QLabel* lbl = new QLabel( QByteArray ( out.str(), out.pcount() ).data(), &dlg );
	out.rdbuf()->freeze(false);
	lbl->setFrameStyle( Q3Frame::Panel );
	lbl->setFrameShadow( Q3Frame::Sunken );
	contents->addMultiCellWidget( lbl, 0, 0, 0, maxStep ); 

	contents->addWidget( new QLabel( "Target:", &dlg ), 2, 0 ); 
	contents->addWidget( new QLabel( "Hops:", &dlg ), 3, 0 ); 
	contents->addWidget( new QLabel( "Repeat:", &dlg ), 4, 0 ); 
	contents->addWidget( new QLabel( "*Mean:", &dlg ), 5, 0 ); 
	contents->addWidget( new QLabel( "*Deviation:", &dlg ), 6, 0 ); 
	contents->addWidget( new QLabel( "*Dimension:", &dlg ), 7, 0 ); 
	contents->addWidget( new QLabel( "*Comment:", &dlg ), 8, 0 ); 

	QString str;
	int i;
	for( d = 0; d < maxStep; d++ )
	{
		str.sprintf( "Step %d", d + 1 );
		contents->addWidget( new QLabel( str, &dlg ), 1, d + 1 ); 

		_atoms[ d ] = new QComboBox( &dlg );
		for( i = AtomType::None; i <= AtomType::Lr; i++ )
			_atoms[ d ]->insertItem( AtomType::s_labels[ i ] );
		contents->addWidget( _atoms[ d ], 2, d + 1 );

		_hops[ d ] = new QLineEdit( &dlg );
		contents->addWidget( _hops[ d ], 3, d + 1 );

		_rep[ d ] = new QCheckBox( &dlg );
		contents->addWidget( _rep[ d ], 4, d + 1 );

		_means[ d ] = new QLineEdit( &dlg );
		contents->addWidget( _means[ d ], 5, d + 1 );

		_devs[ d ] = new QLineEdit( &dlg );
		contents->addWidget( _devs[ d ], 6, d + 1 );

		_dims[ d ] = new QComboBox( &dlg );
		contents->addWidget( _dims[ d ], 7, d + 1 );
		_dims[ d ]->insertItem( "" );
		for( i = 0; i < t->getDimCount(); i++ )
			_dims[ d ]->insertItem( getDimSymbol( i ) );

		_names[ d ] = new QLineEdit( &dlg );
		contents->addWidget( _names[ d ], 8, d + 1 );

	}
	const SpectrumType::Procedure& proc = t->getProc();
	for( d = 0; d < proc.size() && d < maxStep; d++ )	// RISK
	{
		const SpectrumType::Step& s = proc[ d ];
		_atoms[ d ]->setCurrentItem( s.d_atom.getIsoType() );
		_hops[ d ]->setText( str.setNum( s.d_hops ) );
		_rep[ d ]->setChecked( s.d_repeat );
		if( !s.d_range.isNull() )
		{
			_means[ d ]->setText( str.setNum( s.d_range.d_mean ) );
			_devs[ d ]->setText( str.setNum( s.d_range.d_dev ) );
		}
		_dims[ d ]->setCurrentItem( s.d_dim + 1 );
		_names[ d ]->setText( s.d_text.data() ); 
	}

	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( contents );
	top->addLayout( buttons );

	bool _ok;
	while( dlg.exec() == QDialog::Accepted )
	{
		_ok = true;
		Dimension count = 0;
		Dimension d;
		for( d = 0; d < maxStep; d++ )
		{
			if( _atoms[ d ]->currentItem() == 0 )
				break;
			else
				count = d + 1;
		}
		if( count != 0 )
		{
			for( d = count; d < maxStep; d++ )
				if( _atoms[ d ]->currentItem() != 0 )
				{
					QMessageBox::critical( this, title, 
						"Step definitions must be continuous!", "&Cancel" );
					_ok = false;
					break;
				}
			if( !_ok )
				continue;

		}
		if( count == 0 )
		{
			t->setProc( SpectrumType::Procedure() );
			return;
		}
		SpectrumType::Procedure p( count );
		int done = 1;
		for( d = 0; d < count; d++ )
		{
			SpectrumType::Step& s = p[ d ];
			s.d_atom = (AtomType::Isotope) _atoms[ d ]->currentItem();
			s.d_hops = _hops[ d ]->text().toShort( &_ok ); done *= _ok;
			s.d_repeat = _rep[ d ]->isChecked();
			if( !_means[ d ]->text().isEmpty() )
				s.d_range.d_mean = _means[ d ]->text().toFloat( &_ok ); done *= _ok;
			if( !_devs[ d ]->text().isEmpty() )
				s.d_range.d_dev = _devs[ d ]->text().toFloat( &_ok ); done *= _ok;
			s.d_dim = _dims[ d ]->currentItem() - 1;
			if( !_names[ d ]->text().isEmpty() )
				s.d_text = _names[ d ]->text().toLatin1().data(); 
		}
		if( !done )
		{
			QMessageBox::critical( this, title, 
				"Invalid field values!", "&Cancel" );
			continue;
		}
		try
		{
			t->checkValid( p );
			t->setProc( p );
		}catch( Root::Exception& e )
		{
			QMessageBox::critical( this, title, e.what(), "&Cancel" );
			continue;
		}
		break;
	}
}

void SpectrumTypeView::handleEditAtts(Root::Action & a)
{
	_SpectrumTypeItem* item = dynamic_cast<_SpectrumTypeItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	DynValueEditor::edit( this, d_rep->findObjectDef( Repository::keySpectrumType ),
		item->d_type );
}

void SpectrumTypeView::handleShowTable(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );

	int i = 0;
	const Repository::SpectrumTypeMap& sm = d_rep->getSpecTypes();
	Repository::SpectrumTypeMap::const_iterator p;
	ObjectListView::ObjectList o( sm.size() );
	for( p = sm.begin(); p != sm.end(); ++p, ++i)
	{
		o[ i ] = (*p).second;
	}
	ObjectListView::edit( this, d_rep->findObjectDef( Repository::keySpectrumType ), o );
}

void SpectrumTypeView::handlePeakWidth(Root::Action & a)
{
	_TypeDimItem* i = dynamic_cast<_TypeDimItem*>( currentItem() );
	ACTION_ENABLED_IF( a, i );
	bool ok	= FALSE;
	QString res;
	res.sprintf( "%0.3f", i->d_type->getPeakWidth( i->d_dim ) );
	res	= QInputDialog::getText( "Set Peak Width", 
		"Please enter a PPM value >= 0:", QLineEdit::Normal, res, &ok, this );
	if( !ok )
		return;
	PPM w = res.toFloat( &ok );
	if( !ok || w < 0.0 )
	{
		QMessageBox::critical( this, "Set Peak Width", "Invalid peak width!", "&Cancel" );
		return;
	}
	i->d_type->setPeakWidth( i->d_dim, w );
}

void SpectrumTypeView::handleReverseProc(Root::Action & a)
{
	_SpectrumTypeItem* item = dynamic_cast<_SpectrumTypeItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	if( QMessageBox::warning( this, "Reverse Steps",
		"Do you really want to reverse the steps (cannot be undone)?",
		"&OK", "&Cancel" ) != 0 )
		return;

	SpectrumType::Procedure proc = item->d_type->getProc();
	for( int i = 0; i < proc.size(); i++ )
		proc[ i ] = item->d_type->getProc()[ proc.size() - i - 1 ];

	try
	{
		item->d_type->checkValid( proc );
		item->d_type->setProc( proc );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Reverse Steps", e.what(), "&Cancel" );
	}
}

void SpectrumTypeView::handleImportTypes(Root::Action & a)
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
			const Repository::SpectrumTypeMap& rt = temp->getSpecTypes();
			Repository::SpectrumTypeMap::const_iterator i;
			for( i = rt.begin(); i != rt.end(); ++i )
				l.push_back( (*i).first.data() );
		}
		QApplication::restoreOverrideCursor();
		if( !Dlg::getStrings( this, l, "Select Spectrum Types" ) )
			return;

		typedef std::map<Root::SymbolString,Root::SymbolString> Trans;
		Trans t;
		{
			bool ok;
            for( int j = 0; j < int(l.size()); j++ )
			{
				t[ l[ j ] ] = l[ j ];
				while( d_rep->getTypes()->findSpectrumType( l[ j ].data() ) )
				{
					QString res = QInputDialog::getText( "Import Spectrum Types", 
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
		for( k = t.begin(); k != t.end(); ++k )
			d_rep->getTypes()->addSpectrumType( 
				new SpectrumType( (*k).second, 
					*temp->getTypes()->findSpectrumType( (*k).first ) ) );
	}catch( Root::Exception& e )
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::critical( this, "Error Importing Types", e.what(), "&Cancel" );
	}
}



