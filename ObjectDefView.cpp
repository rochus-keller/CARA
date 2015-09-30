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

#include "ObjectDefView.h"
#include <qinputdialog.h>
#include <qlabel.h>
#include <qlineedit.h> 
#include <qlayout.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <q3multilineedit.h>
#include <qmessagebox.h>
#include <Q3ComboBox>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <Q3VBoxLayout>
#include <Root/Application.h>
#include <Root/Any.h>
using namespace Spec;

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr ObjectDefView::AddAtt = "AddAtt";
Root::Action::CmdStr ObjectDefView::RemoveAtt = "RemoveAtt";
Root::Action::CmdStr ObjectDefView::EditAtt = "EditAtt";

ACTION_SLOTS_BEGIN( ObjectDefView )
    { ObjectDefView::AddAtt, &ObjectDefView::handleAddAtt },
    { ObjectDefView::RemoveAtt, &ObjectDefView::handleRemoveAtt },
    { ObjectDefView::EditAtt, &ObjectDefView::handleEditAtt },
ACTION_SLOTS_END( ObjectDefView )

//////////////////////////////////////////////////////////////////////

class _ObjectItem : public Gui::ListViewItem
{
public:
	_ObjectItem( Gui::ListView* p, Root::SymbolString cls, 
		const ObjectDef::Attribute& a ):
		Gui::ListViewItem(p),d_class( cls ), d_att( a ) {}
	Root::SymbolString d_class;
	ObjectDef::Attribute d_att;

	QString text( int f ) const 
	{ 
		switch( f )
		{
		case 0:
			return d_class.data();
		case 1:
			return Root::Any::getTypeName( d_att.d_type );
		case 2:
			return d_att.d_description.data();
		default:
			return "";
		}
	}
	// const QPixmap *pixmap( int i ) const { return s_pixRepository; }
};

//////////////////////////////////////////////////////////////////////

ObjectDefView::ObjectDefView(QWidget* p,Spec::ObjectDef* d):
	Gui::ListView( p ), d_obj( d )
{
	setShowSortIndicator( true );
	setRootIsDecorated( false );
	setAllColumnsShowFocus( true );
	addColumn( "Name" );
	addColumn( "Type" );
	addColumn( "Description" );
	refill();
}

ObjectDefView::~ObjectDefView()
{

}

void ObjectDefView::refill()
{
	clear();

	ObjectDef::Attributes::const_iterator p;
	const ObjectDef::Attributes& sm = d_obj->getAttributes();
	for( p = sm.begin(); p != sm.end(); ++p )
	{
        new _ObjectItem( this, (*p).first, (*p).second );
	}
}
Gui::Menu* ObjectDefView::createPopup()
{
    Gui::Menu* pop = new Gui::Menu();
	Gui::Menu::item( pop, "New Attribute...", AddAtt, false );
	Gui::Menu::item( pop, "Edit Attribute...", EditAtt, false );
	Gui::Menu::item( pop, "Delete Attribute...", RemoveAtt, false );
    return pop;
}

void ObjectDefView::handle(Root::Message & msg)
{
	BEGIN_HANDLER();
	MESSAGE( Root::Action, a, msg )
	{
		EXECUTE_ACTION( ObjectDefView, *a );
	}
	END_HANDLER();
}

void ObjectDefView::handleAddAtt(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );

	QDialog dlg( this, "", true );
	dlg.setCaption( "New Attribute" );
	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 5);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	top->addWidget( new QLabel( "Name:", &dlg ) );
	QLineEdit* name = new QLineEdit( &dlg );
	top->addWidget( name );
	Q3ComboBox* t = new Q3ComboBox( &dlg );
	t->setSizeLimit( 20 );
	for( int i = Root::Any::Boolean; i <= Root::Any::OID; i++ )
		t->insertItem( Root::Any::getTypeName( (Root::Any::Type)i ) );
	top->addWidget( t );
	top->addWidget( new QLabel( "Description:", &dlg ) );
	Q3MultiLineEdit* desc = new Q3MultiLineEdit( &dlg );
	desc->setWordWrap( Q3MultiLineEdit::WidgetWidth );
	top->addWidget( desc );
	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );	// ok_callback
	ok->setDefault( true );
	buttons->addWidget( ok );
	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );	// ok_callback
	buttons->addWidget( cancel );
	top->addLayout( buttons );

	if( dlg.exec() != QDialog::Accepted )
		return;

	if( name->text().isEmpty() )
	{
		QMessageBox::critical( this, "New Attribute", 
			"Please enter a name for the Attribute!", "&Cancel" );
		return;
	}
	ObjectDef::Attributes::const_iterator p;
	const ObjectDef::Attributes& sm = d_obj->getAttributes();
	for( p = sm.begin(); p != sm.end(); ++p )
	{
		if( name->text() == (*p).first.data() )
		{
			QMessageBox::critical( this, "New Attribute", 
				"The selected name is not unique!", "&Cancel" );
			return;
		}
	}
	ObjectDef::Attribute att;
	att.d_type = (Root::Any::Type) ( t->currentItem() + 1 );
	att.d_description = desc->text().toLatin1();
	d_obj->setField( name->text().toLatin1(), att.d_type, att.d_description );
	//_ObjectItem* rti = new _ObjectItem( this, name->text().latin1(), att );
	refill();
}

void ObjectDefView::handleRemoveAtt(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );

	_ObjectItem* item = (_ObjectItem*) currentItem();

	if( QMessageBox::warning( this, "Remove Attribute",
		"Do you really want to remove this attribute (cannot be undone)?",
		"&OK", "&Cancel" ) != 0 )
		return;
	d_obj->removeField( item->d_class );
	item->removeMe();
}

void ObjectDefView::handleEditAtt(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );

	_ObjectItem* item = (_ObjectItem*) currentItem();

	QDialog dlg( this, "", true );
	dlg.setCaption( "Edit Attribute" );
	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 5);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	Q3ComboBox* t = new Q3ComboBox( &dlg );
	t->setSizeLimit( 20 );
	for( int i = Root::Any::Boolean; i <= Root::Any::OID; i++ )
		t->insertItem( Root::Any::getTypeName( (Root::Any::Type)i ) );
	t->setCurrentItem( item->d_att.d_type - 1 );
	top->addWidget( t );
	top->addWidget( new QLabel( "Description:", &dlg ) );
	Q3MultiLineEdit* desc = new Q3MultiLineEdit( &dlg );
	desc->setWordWrap( Q3MultiLineEdit::WidgetWidth );
	desc->setText( item->d_att.d_description.data() );
	top->addWidget( desc );
	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );	// responsevalues
	ok->setDefault( true );
	buttons->addWidget( ok );
	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );	// ok_callback
	buttons->addWidget( cancel );
	top->addLayout( buttons );

	if( dlg.exec() != QDialog::Accepted )
		return;

	ObjectDef::Attribute att;
	att.d_type = (Root::Any::Type) ( t->currentItem() + 1 );
	att.d_description = desc->text().toLatin1();
	d_obj->setField( item->d_class, att.d_type, att.d_description );
	refill();
}
