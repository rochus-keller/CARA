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

#include "Dlg.h"
#include <qdialog.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qradiobutton.h> 
#include <q3buttongroup.h>
#include <qradiobutton.h>
#include <qmessagebox.h> 
#include <qlineedit.h> 
#include <qvalidator.h>
#include <qlayout.h>
#include <qcombobox.h>
#include <qcheckbox.h> 
#include <q3textview.h> 
#include <q3listbox.h> 
#include <q3hbox.h>
#include <Gui/ListView.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <Q3GridLayout>
#include <Q3VBoxLayout>
#include <Q3ComboBox>
#include <stdio.h>
#include <Spec/Project.h>
#include <Spec/Repository.h>
using namespace Spec;
using namespace Root;

static bool s_draft = false;
static char s_buf[ 64 ];

struct _TupleItem : public Gui::ListViewItem
{
	enum { COUNT = 3 };

	_TupleItem( Gui::ListView * parent, const SpinPoint& t ):
		Gui::ListViewItem( parent ), d_t( t ) {}
	SpinPoint d_t; // RISK: msste eigentlich ExRef sein.

	QString text( int f ) const
	{
		Spin* s = d_t[ f / COUNT ];
		f = f % COUNT;
		*s_buf = 0;
		assert( s );
		switch( f )
		{
		case 0:
			::sprintf( s_buf, "%.3f", s->getShift() ); 
			return s_buf;
		case 1:
			if( s->getLabel().isNull() )
				::sprintf( s_buf, "? %d", s->getId() );
			else
				s->getLabel().format( s_buf, sizeof( s_buf ) );
			return s_buf;
		case 2:
			if( s->getSystem() )
			{
				s->getSystem()->formatLabel( s_buf, sizeof(s_buf) );
			}
			return s_buf;
		}
		return "";
	}
	QString key( int f, bool ) const
	{
		Spin* s = d_t[ f / COUNT ];
		f = f % COUNT;
		*s_buf = 0;
		assert( s );
		switch( f )
		{
		case 0:
			::sprintf( s_buf, "%08.3f", s->getShift() ); 
			return s_buf;
		case 1:
			if( s->getLabel().isNull() )
				::sprintf( s_buf, "_%08d", s->getId() );
			else
				s->getLabel().format( s_buf, sizeof( s_buf ) );
			return s_buf;
		case 2:
			if( s->getSystem() )
			{
				if( s->getSystem()->getAssig() != 0 )
					::sprintf( s_buf, "_%08d", s->getSystem()->getAssig()->getId() );
				else
					::sprintf( s_buf, "%08d", s->getSystem()->getId() ); 
			}
			return s_buf;
		}
		return "";
	}
};

struct _LabelPairItem : public Gui::ListViewItem
{
    _LabelPairItem( Gui::ListView * parent, const SpinLabelPoint& pair ):
        Gui::ListViewItem( parent ), d_pair( pair ) {}
    SpinLabelPoint d_pair;

    QString text( int f ) const
    {
        if( f < SpinLabelPoint::MAX_DIM )
        {
            d_pair[f].format( s_buf, sizeof( s_buf ) );
            return s_buf;
        }else
            return "";
    }
};

Dlg::SpinItem::SpinItem( Gui::ListView * parent, Spin* lhs, float f ):
	Gui::ListViewItem( parent ), d_spin( lhs ), d_fit( f ) {}

QString Dlg::SpinItem::text( int f ) const
{
	*s_buf = 0;
	switch( f )
	{
	case 0:
		::sprintf( s_buf, "%.2f", d_fit ); 
		return s_buf;
	case 1:
		::sprintf( s_buf, "%.3f", d_spin->getShift() ); 
		return s_buf;
	case 2:
		if( d_spin->getLabel().isNull() )
			::sprintf( s_buf, "? %d", d_spin->getId() );
		else
			d_spin->getLabel().format( s_buf, sizeof( s_buf ) );
		return s_buf;
	case 3:
		if( d_spin->getSystem() )
		{
			d_spin->getSystem()->formatLabel( s_buf, sizeof(s_buf) );
		}
		return s_buf;
	}
	return "";
}
QString Dlg::SpinItem::key( int f, bool ) const
{
	*s_buf = 0;
	switch( f )
	{
	case 0:
		::sprintf( s_buf, "%03.6f", d_fit ); 
		return s_buf;
	case 1:
		::sprintf( s_buf, "%08.3f", d_spin->getShift() ); 
		return s_buf;
	case 2:
		if( d_spin->getLabel().isNull() )
			::sprintf( s_buf, "_%08d", d_spin->getId() );
		else
			d_spin->getLabel().format( s_buf, sizeof( s_buf ) );
		return s_buf;
	case 3:
		if( d_spin->getSystem() )
		{
			if( d_spin->getSystem()->getAssig() != 0 )
				::sprintf( s_buf, "_%08d", d_spin->getSystem()->getAssig()->getId() );
			else
				::sprintf( s_buf, "%08d", d_spin->getSystem()->getId() ); 
		}
		return s_buf;
	}
	return "";
}

bool Dlg::setParams(QWidget *parent, Dlg::ContourParams & p)
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( "Set Contour Parameters" );

	QString str;

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	Q3GridLayout* contents = new Q3GridLayout( 3, 2 );

	contents->addWidget( new QLabel( "Factor:", &dlg ), 0, 0 ); 
	QLineEdit* factor = new QLineEdit( &dlg );
	contents->addWidget( factor, 0, 1 );
	str.setNum( p.d_factor );
	factor->setText( str );

	contents->addWidget( new QLabel( "Threshold:", &dlg ), 1, 0 ); 
	QLineEdit* threshold = new QLineEdit( &dlg );
	contents->addWidget( threshold, 1, 1 );
	str.setNum( p.d_threshold );
	threshold->setText( str );

	contents->addWidget( new QLabel( "Option:", &dlg ), 2, 0 ); 
	Q3ComboBox* option = new Q3ComboBox( &dlg );
	option->insertItem( "Positive" );
	option->insertItem( "Negative" );
	option->insertItem( "Both" );
	contents->addWidget( option, 2, 1 );
	option->setCurrentItem( p.d_option );


	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( contents );
	top->addLayout( buttons );

	while( true )
	{
		if( dlg.exec() == QDialog::Accepted )
		{
			p.d_factor = factor->text().toFloat();
			p.d_threshold = threshold->text().toFloat();
			p.d_option = (ContourView::Option) option->currentItem();
			if( p.d_factor <= 1.0 || p.d_factor > 10.0 )
			{
				if( QMessageBox::critical( parent, "Set Contour Parameters", 
					"Factor must be > 1.0 and < 10.0", "&OK", "&Cancel" ) != 0 )
					return false;
			}else if( p.d_threshold < 0.0 )
			{
				if( QMessageBox::critical( parent, "Set Contour Parameters", 
					"Threshold must be >= 0.0", "&OK", "&Cancel" ) != 0 )
					return false;
			}else
				return true;
		}else
			return false;
	}
}

void Dlg::showInfo(QWidget * parent, const char *title, const char *text)
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( title );
	dlg.resize( 640, 480 );
	QString str;

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();

	Q3TextView* tv = new Q3TextView( &dlg );
	tv->setText( text );
	top->addWidget( tv );

	QPushButton* ok = new QPushButton( "&Done", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	top->addLayout( buttons );

	dlg.exec();
}

bool Dlg::getBounds(QWidget * parent, Amplitude &min, Amplitude &max,
			const char* title )
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( title );

	QString str;

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	Q3GridLayout* contents = new Q3GridLayout( 3, 2 );

	contents->addWidget( new QLabel( "Upper:", &dlg ), 0, 0 ); 
	QLineEdit* upper = new QLineEdit( &dlg );
	contents->addWidget( upper, 0, 1 );
	str.sprintf( "%.0f", max );
	upper->setText( str );

	contents->addWidget( new QLabel( "Lower:", &dlg ), 1, 0 ); 
	QLineEdit* lower = new QLineEdit( &dlg );
	contents->addWidget( lower, 1, 1 );
	str.sprintf( "%.0f", min );
	lower->setText( str );

	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( contents );
	top->addLayout( buttons );

	while( true )
	{
		if( dlg.exec() == QDialog::Accepted )
		{
			Amplitude u = upper->text().toFloat();
			Amplitude l = lower->text().toFloat();
			if( u < 0.0 )
			{
				if( QMessageBox::critical( parent, title, 
					"Upper must be equal or greater than zero!", "&OK", "&Cancel" ) != 0 )
					return false;
			}if(  l > 0.0 )
			{
				if( QMessageBox::critical( parent, title, 
					"Lower must be equal or smaller than zero!", "&OK", "&Cancel" ) != 0 )
					return false;
			}else if( u == 0.0 && l == 0.0 )
			{
				if( QMessageBox::critical( parent, title, 
					"Set at least one bound not zero!", "&OK", "&Cancel" ) != 0 )
					return false;
			}else
			{
				min = l;
				max = u;
				return true;
			}
		}else
			return false;
	}
}

bool Dlg::getPhase(QWidget * parent, float &phi, float &psi)
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( "Set Phase" );

	QString str;

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	Q3GridLayout* contents = new Q3GridLayout( 3, 2 );

	contents->addWidget( new QLabel( "Phi 0:", &dlg ), 0, 0 ); 
	QLineEdit* upper = new QLineEdit( &dlg );
	contents->addWidget( upper, 0, 1 );
	str.sprintf( "%0.1f", phi );
	upper->setText( str );

	contents->addWidget( new QLabel( "Phi 1:", &dlg ), 1, 0 ); 
	QLineEdit* lower = new QLineEdit( &dlg );
	contents->addWidget( lower, 1, 1 );
	str.sprintf( "%0.1f", psi );
	lower->setText( str );

	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( contents );
	top->addLayout( buttons );

	bool done;
	while( dlg.exec() == QDialog::Accepted )
	{
		float u = upper->text().toFloat( &done );
		if( !done )
		{
			if( QMessageBox::critical( parent, "Set Phase", 
				"Invalid value for phi 0!", "&OK", "&Cancel" ) != 0 )
				continue;
		}			
		float l = lower->text().toFloat( &done );
		if( !done )
		{
			if( QMessageBox::critical( parent, "Set Phase", 
				"Invalid value for phi 1!", "&OK", "&Cancel" ) != 0 )
				continue;
		}			
		phi = u;
		psi = l;
		return true;
	}
	return false;
}

bool Dlg::getValue(QWidget * parent, Value& val, bool setName)
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( "Set Value" );

	QString str;

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	Q3GridLayout* contents = new Q3GridLayout( 4, 2 );

	contents->addWidget( new QLabel( "Label:", &dlg ), 0, 0 ); 
	QLineEdit* tag = new QLineEdit( &dlg );
	if( !setName )
		tag->setReadOnly( true );
	contents->addWidget( tag, 0, 1 );
	tag->setText( val.d_name.data() );

	contents->addWidget( new QLabel( "Mean:", &dlg ), 1, 0 ); 
	QLineEdit* mean = new QLineEdit( &dlg );
	contents->addWidget( mean, 1, 1 );
	str.setNum( val.d_dp.d_mean );
	if( !setName )
		mean->setText( str );

	contents->addWidget( new QLabel( "Std.Dev.:", &dlg ), 2, 0 ); 
	QLineEdit* dev = new QLineEdit( &dlg );
	contents->addWidget( dev, 2, 1 );
	str.setNum( val.d_dp.d_dev );
	if( !setName )
		dev->setText( str );

	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( contents );
	top->addLayout( buttons );

	while( true )
	{
		if( dlg.exec() == QDialog::Accepted )
		{
			bool ok1, ok2;
			val.d_dp.d_mean = mean->text().toFloat( &ok1 );
			val.d_dp.d_dev = dev->text().toFloat( &ok2 );
			if( !ok1 || !ok2 )
			{
				QMessageBox::critical( &dlg, "Get Value",
						"Invalid ppm value!", "&Cancel" );
				continue;
			}
			if( setName )
				val.d_name = tag->text().toLatin1();
			return true;
		}else
			return false;
	}
}

bool Dlg::selectStrings(QWidget * parent, const char* title, StringSet & sl, bool multi )
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( title );
	dlg.resize( 200, 400 );
	QString str;

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();

	Q3ListBox* lb = new Q3ListBox( &dlg );
	if( multi )
		lb->setSelectionMode( Q3ListBox::Extended );
	StringSet::iterator pos;
	for( pos = sl.begin(); pos != sl.end(); ++pos )
		lb->insertItem( (*pos).data() );
	lb->sort();
	top->addWidget( lb );
	QObject::connect( lb, SIGNAL( doubleClicked ( Q3ListBoxItem * ) ), &dlg, SLOT( accept() ) );

	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( buttons );

	if( dlg.exec() == QDialog::Accepted )
	{
		sl.clear();
        for( int i = 0; i < int(lb->count()); i++ )
		{
			if( lb->isSelected( i ) )
				sl.insert( lb->text( i ).toLatin1() );
		}
		return true;
	}else
		return false;
}

PeakList* Dlg::selectPeakList(QWidget * parent, Project * pro)
{
	assert( pro );
	QDialog dlg( parent, "", true );
	dlg.setCaption( "Select Peaklist" );

	QString str;

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();

	Q3ListBox* lb = new Q3ListBox( &dlg );
	const Project::PeakListMap& pm = pro->getPeakLists();
	Project::PeakListMap::const_iterator pos;
	for( pos = pm.begin(); pos != pm.end(); ++pos )
		lb->insertItem( (*pos).second->getName().data() );

	lb->sort();
	top->addWidget( lb );
	QObject::connect( lb, SIGNAL( doubleClicked ( Q3ListBoxItem * ) ), &dlg, SLOT( accept() ) );

	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( buttons );

	if( dlg.exec() == QDialog::Accepted )
	{
		return pro->findPeakList( lb->currentText().toLatin1() );
	}else
		return 0;
}

bool Dlg::getPredSucc(QWidget* parent, int &pred, int &succ)
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( "Enter the Spin Systems to Link" );

	QString str;

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	Q3GridLayout* contents = new Q3GridLayout( 4, 2 );

	contents->addWidget( new QLabel( "Predecessor:", &dlg ), 1, 0 ); 
	QLineEdit* mean = new QLineEdit( &dlg );
	contents->addWidget( mean, 1, 1 );
	mean->setText( str );

	contents->addWidget( new QLabel( "Successor:", &dlg ), 2, 0 ); 
	QLineEdit* dev = new QLineEdit( &dlg );
	contents->addWidget( dev, 2, 1 );
	dev->setText( str );

	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( contents );
	top->addLayout( buttons );

	if( dlg.exec() == QDialog::Accepted )
	{
		pred = mean->text().toUInt();
		succ = dev->text().toUInt();
		return true;
	}else
		return false;
}

bool Dlg::getFrameSize( QWidget* parent, float& w, float& h )
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( "Set Frame Size" );

	QString str;

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	Q3GridLayout* contents = new Q3GridLayout( 4, 2 );

	str.sprintf( "%0.0f", w );
	contents->addWidget( new QLabel( "Width (mm):", &dlg ), 1, 0 ); 
	QLineEdit* width = new QLineEdit( &dlg );
	contents->addWidget( width, 1, 1 );
	width->setText( str );

	str.sprintf( "%0.0f", h );
	contents->addWidget( new QLabel( "Height (mm):", &dlg ), 2, 0 ); 
	QLineEdit* height = new QLineEdit( &dlg );
	contents->addWidget( height, 2, 1 );
	height->setText( str );

	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( contents );
	top->addLayout( buttons );

	if( dlg.exec() == QDialog::Accepted )
	{
		w = width->text().toFloat();
		h = height->text().toFloat();
		return true;
	}else
		return false;
}

bool Dlg::getPpmRange( QWidget* parent, PpmRange& w, PpmRange& h, const char* title )
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( title );

	QString str;

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	Q3GridLayout* contents = new Q3GridLayout( 4, 3 );

	str.sprintf( "%0.3f", w.first );
	contents->addWidget( new QLabel( "Horizontal (ppm/ppm):", &dlg ), 1, 0 ); 
	QLineEdit* w1 = new QLineEdit( &dlg );
	contents->addWidget( w1, 1, 1 );
	w1->setText( str );

	str.sprintf( "%0.3f", w.second );
	QLineEdit* w2 = new QLineEdit( &dlg );
	contents->addWidget( w2, 1, 2 );
	w2->setText( str );

	if( w.first == 0.0 && w.second == 0.0 )
	{
		w1->setReadOnly( true );
		w2->setReadOnly( true );
	}

	str.sprintf( "%0.3f", h.first );
	contents->addWidget( new QLabel( "Vertical (ppm/ppm):", &dlg ), 2, 0 ); 
	QLineEdit* h1 = new QLineEdit( &dlg );
	contents->addWidget( h1, 2, 1 );
	h1->setText( str );

	str.sprintf( "%0.3f", h.second );
	QLineEdit* h2 = new QLineEdit( &dlg );
	contents->addWidget( h2, 2, 2 );
	h2->setText( str );

	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( contents );
	top->addLayout( buttons );

	if( dlg.exec() == QDialog::Accepted )
	{
		w.first = w1->text().toFloat();
		h.first = h1->text().toFloat();
		w.second = w2->text().toFloat();
		h.second = h2->text().toFloat();
		return true;
	}else
		return false;
}

bool Dlg::getResolution( QWidget* parent, Root::Long& sh, Root::UInt8& ch,
						Root::Long& sv, Root::UInt8& cv )
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( "Set Ruler Resolution" );

	QString str;

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	Q3GridLayout* contents = new Q3GridLayout( 4, 3 );

	str.sprintf( "%d", sh );
	contents->addWidget( new QLabel( "Horizontal (TWIP/count):", &dlg ), 1, 0 ); 
	QLineEdit* w1 = new QLineEdit( &dlg );
	contents->addWidget( w1, 1, 1 );
	w1->setText( str );

	str.sprintf( "%d", int( ch ) );
	QLineEdit* w2 = new QLineEdit( &dlg );
	contents->addWidget( w2, 1, 2 );
	w2->setText( str );

	str.sprintf( "%d", sv );
	contents->addWidget( new QLabel( "Vertical (TWIP/count):", &dlg ), 2, 0 ); 
	QLineEdit* h1 = new QLineEdit( &dlg );
	contents->addWidget( h1, 2, 1 );
	h1->setText( str );

	str.sprintf( "%d", int( cv ) );
	QLineEdit* h2 = new QLineEdit( &dlg );
	contents->addWidget( h2, 2, 2 );
	h2->setText( str );

	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( contents );
	top->addLayout( buttons );

	bool done = true;
	if( dlg.exec() == QDialog::Accepted )
	{
		bool _ok; 
		int _sh, _sv, _ch, _cv;
		_sh = w1->text().toULong( &_ok ); done = done && _ok;
		_sv = h1->text().toUShort( &_ok ); done = done && _ok;
		_ch = w2->text().toULong( &_ok ); done = done && _ok;
		_cv = h2->text().toUShort( &_ok ); done = done && _ok;
		if( !done || _sh <= 0 || _sv <= 0 || _ch <= 0 || _cv <= 0 )
		{
			QMessageBox::critical( &dlg, "Set Ruler Resolution",
					"Invalid value entries!", "&Cancel" );
			return false;
		}
		sh = _sh;
		sv = _sv;
		ch = _ch;
		cv = _cv;
		return true;
	}else
		return false;
}

bool Dlg::getPpmPoint( QWidget* parent, PpmPoint& p, const QString& title, const QStringList& labels, bool onlyLabels )
{
    Q_ASSERT( labels.isEmpty() || labels.size() == p.size() );

	QDialog dlg( parent, "", true );
	dlg.setCaption( title );

	QString str;

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
    Q3GridLayout* contents = new Q3GridLayout( p.size(), 2 );

	Root::Vector<QLineEdit*> e( p.size() );

	Dimension d;
	for( d = 0; d < p.size(); d++ )
	{
        if( !labels.isEmpty() && onlyLabels )
            str = labels[d];
        else if( !labels.isEmpty() )
            str.sprintf( "Dim. %s (%s):", getDimSymbol( d ), labels[d].toLatin1().data() );
        else
            str.sprintf( "Dim. %s (%s):", getDimLetter( d, false ), getDimSymbol( d, true ) );
        contents->addWidget( new QLabel( str, &dlg ), d, 0 );
		str.sprintf( "%0.3f", p[ d ] );
		e[ d ] = new QLineEdit( &dlg );
		contents->addWidget( e[ d ], d, 1 );
		e[ d ]->setText( str );
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
	int err = 0;
	while( dlg.exec() == QDialog::Accepted )
	{
		err = 0;
		for( d = 0; d < p.size(); d++ )
		{
			p[ d ] = e[ d ]->text().toFloat( &_ok );
			if( !_ok )
				err++;
		}
		if( err )
		{
			QMessageBox::critical( &dlg, title,
					"Invalid PPM value!", "&Cancel" );
			continue;
		}
		return true;
	}
	return false;
}

bool Dlg::getLocation( QWidget* parent, Location& p, const char* title, bool r )
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( title );

	QString str;

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	Q3GridLayout* contents = new Q3GridLayout( 4, 2 );

	Root::Vector<QLineEdit*> e( 4 );

	Dimension d;
	for( d = 0; d < 3; d++ )
	{
		str.sprintf( "Dim. %s:", getDimLetter( d, false ) );
		contents->addWidget( new QLabel( str, &dlg ), d, 0 ); 
		str.sprintf( "%0.3f", p.d_pos[ d ] );
		e[ d ] = new QLineEdit( &dlg );
		contents->addWidget( e[ d ], d, 1 );
		e[ d ]->setText( str );
	}
	if( r )
	{
		contents->addWidget( new QLabel( "R-Value:", &dlg ), d, 0 ); 
		str.sprintf( "%0.3f", p.d_dev );
		e[ 3 ] = new QLineEdit( &dlg );
		contents->addWidget( e[ d ], 3, 1 );
		e[ 3 ]->setText( str );
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
	int err = 0;
	while( dlg.exec() == QDialog::Accepted )
	{
		err = 0;
		for( d = 0; d < 3; d++ )
		{
			p.d_pos[ d ] = e[ d ]->text().toFloat( &_ok );
			if( !_ok )
				err++;
		}
		if( err )
		{
			QMessageBox::critical( &dlg, title,
					"Invalid coordinate!", "&Cancel" );
			continue;
		}
		if( r )
		{
			p.d_dev = e[ 3 ]->text().toFloat( &_ok );
			if( !_ok )
			{
				QMessageBox::critical( &dlg, title,
						"Invalid R-Value!", "&Cancel" );
				continue;
			}
		}
		return true;
	}
	return false;
}

bool Dlg::getSubMatCount( QWidget* parent, Spectrum* s, Root::Extension& p, double& fac )
{
	const char* title = "Create XEASY Spectrum";
	assert( s && s->getDimCount() == p.size() );
	QDialog dlg( parent, "", true );
	dlg.setCaption( title );

	QString str;

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	Q3GridLayout* contents = new Q3GridLayout( p.size(), 2 );

	QLineEdit* f;
	top->addWidget( new QLabel( "Please	enter an amplitude multiplication factor:\n"
			"(default complies with the allowed EASY amplitude range)", &dlg ) ); 
	str.sprintf( "%f", fac );
	f = new QLineEdit( &dlg );
	top->addWidget( f );
	f->setText( str );

	Root::Vector<QLineEdit*> e( p.size() );

	top->addWidget( new QLabel( "Please	enter the number of submatrices:\n"
		"(range 1..#points, where 1 corresponds to no submatrix)", &dlg ) ); 
	Dimension d;
	for( d = 0; d < p.size(); d++ )
	{
		str.sprintf( "Dim. %s (%s, %d points):", getDimLetter( d, false ),
			s->getScale( d ).getColor().getIsoLabel(), 
			s->getScale( d ).getSampleCount() );
		contents->addWidget( new QLabel( str, &dlg ), d, 0 ); 
		str.sprintf( "%d", p[ d ] );
		e[ d ] = new QLineEdit( &dlg );
		contents->addWidget( e[ d ], d, 1 );
		e[ d ]->setText( str );
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
	int err = 0;
	int warn = 0;
	while( dlg.exec() == QDialog::Accepted )
	{
		err = 0;
		warn = 0;
		fac = f->text().toDouble( &_ok );
		if( !_ok )
		{
			QMessageBox::critical( &dlg, title,
					"Invalid floating point value!", "&Cancel" );
			continue;
		}

		for( d = 0; d < p.size(); d++ )
		{
			p[ d ] = e[ d ]->text().toUInt( &_ok );
			if( !_ok || p[ d ] == 0 || p[ d ] >= s->getScale( d ).getSampleCount() )
				err++;
			if( s->getScale( d ).getSampleCount() % p[ d ] != 0 )
				warn++;
		}
		if( err )
		{
			QMessageBox::critical( &dlg, title,
					"Submatrix count must be within 1 and the number of sample points!", "&Cancel" );
			continue;
		}
		if( warn )
		{
			switch( QMessageBox::warning( &dlg, title,
					"The spectrum will be shortened using the given number of submatrices!",
												  "&Accept", "&Change", "&Cancel",
												  0,		// Enter == button 0
												  2 ) )		// Escape == button 2
			{
			case 0:
				return true;
			case 1:
				continue;
			default:
				return false;	// Don't do it.
			}
		}
		return true;
	}
	return false;
}

bool Dlg::getSpinParams(QWidget * parent, SpinParams & sp)
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( "Get Spin Parameters" );

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	Q3GridLayout* contents = new Q3GridLayout( 4, 2 );

	contents->addWidget( new QLabel( "Shift:", &dlg ), 1, 0 ); 
	QLineEdit* _shift = new QLineEdit( &dlg );
	contents->addWidget( _shift, 1, 1 );

	contents->addWidget( new QLabel( "Label:", &dlg ), 2, 0 ); 
	QLineEdit* _lbl = new QLineEdit( &dlg );
	contents->addWidget( _lbl, 2, 1 );

	contents->addWidget( new QLabel( "Atom:", &dlg ), 3, 0 ); 
	Q3ComboBox* _atom = new Q3ComboBox( &dlg );
	for( int i = AtomType::H1; i < AtomType::MaxIsotope; i++ )
		_atom->insertItem( AtomType::s_labels[ i ] );
	contents->addWidget( _atom, 3, 1 );
	_atom->setCurrentItem( 0 );

	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( contents );
	top->addLayout( buttons );

	bool doit;
	while( dlg.exec() == QDialog::Accepted )
	{
		sp.d_type = AtomType::Isotope( _atom->currentItem() + 1 );
		sp.d_shift = _shift->text().toFloat( &doit );
		if( !doit )
		{
			QMessageBox::critical( &dlg, "Get Spin Parameters",
					"Invalid ppm value!", "&Cancel" );
		}else if( !SpinLabel::parse( _lbl->text(), sp.d_label )  )
		{
			QMessageBox::critical( &dlg, "Get Spin Parameters",
					"Invalid spin label syntax!", "&Cancel" );
		}else
		{
			return true;
		}
	}
	return false;
}

bool Dlg::getLabels( QWidget* parent, Root::Index x, Root::Index y, 
			SpinLabel& lx, SpinLabel& ly, const SpinLabelSet& sx,
			const SpinLabelSet& sy )
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( "Edit Spin Labels" );

	QString str;
	SpinLabelSet::const_iterator i;

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	Q3GridLayout* contents = new Q3GridLayout( 4, 2 );

	if( x )
		str.sprintf( "Dim. x: Spin %d", x );
	else
        str.sprintf( "Dim. x: New Spin" );
	contents->addWidget( new QLabel( str, &dlg ), 1, 0 ); 
	Q3ComboBox* _x = new Q3ComboBox( true, &dlg );
	for( i = sx.begin(); i != sx.end(); ++i )
		_x->insertItem( (*i).data() );
	_x->listBox()->sort();
	contents->addWidget( _x, 1, 1 );
	_x->setEditText( lx.data() );

	if( y )
		str.sprintf( "Dim. y: Spin %d", y );
	else
        str.sprintf( "Dim. y: New Spin" );
	contents->addWidget( new QLabel( str, &dlg ), 2, 0 ); 
	Q3ComboBox* _y = new Q3ComboBox( true, &dlg );
	for( i = sy.begin(); i != sy.end(); ++i )
		_y->insertItem( (*i).data() );
	_y->listBox()->sort();
	contents->addWidget( _y, 2, 1 );
	_y->setEditText( ly.data() );

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
		if( !SpinLabel::parse( _x->currentText(), lx ) )
		{
			QMessageBox::critical( &dlg, "Edit Spin Labels",
					"Invalid x label!", "&Cancel" );
			continue;
		}
		if( !SpinLabel::parse( _y->currentText(), ly ) )
		{
			QMessageBox::critical( &dlg, "Edit Spin Labels",
					"Invalid y label!", "&Cancel" );
			continue;
		}
		return true;
	}
	return false;
}

bool Dlg::getLabelPoint(QWidget * parent, const SpinLabelPoints& pairs, SpinLabelPoint& res, Dimension count, const char *title)
{
    QDialog dlg( parent, "", true );
    dlg.setCaption( title );

    Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
    Q3HBoxLayout* buttons = new Q3HBoxLayout();

    Gui::ListView* lb = new Gui::ListView( &dlg );
    lb->setAllColumnsShowFocus( true );
    lb->setShowSortIndicator( true );
    lb->setRootIsDecorated(false);
    for( int d = 0; d < count; d++ )
        lb->addColumn( Spec::getDimSymbol( d ) );

    foreach( SpinLabelPoint p, pairs )
    {
        bool ok = true;
        for( int d = 0; d < count; d++ )
            if( false ) // p[d].isNull() )
            {
                ok = false;
                break;
            }
        if( ok )
            new _LabelPairItem( lb, p );
    }

    top->addWidget( lb );
    QCheckBox* draft = new QCheckBox( "&Draft", &dlg );
    top->addWidget( draft );
    QObject::connect( lb, SIGNAL( doubleClicked ( Gui::ListViewItem * ) ),
        &dlg, SLOT( accept() ) );

    QPushButton* ok = new QPushButton( "&OK", &dlg );
    QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
    buttons->addWidget( ok );
    ok->setDefault( true );

    QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
    QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
    buttons->addWidget( cancel );

    top->addLayout( buttons );
    //dlg.resize( 800, 600 ); // RISK

    QMetaObject::invokeMethod( lb, "resizeAllColumnsToContents", Qt::QueuedConnection );
    if( dlg.exec() == QDialog::Accepted && lb->selectedItem() )
    {
        _LabelPairItem* i = (_LabelPairItem*)lb->selectedItem();
        res = i->d_pair;
        if( draft->isChecked() )
        {
            for( int i = 0; i < count; i++ )
                res[i].setDraft();
        }
        return true;
    }else
        return false;
}

_LabelSysTypeDlg::_LabelSysTypeDlg( QWidget* parent, Repository* rep, SpectrumType* st, 
	Dimension x, Dimension y ):
	QDialog( parent, "", true ), d_rep( rep ), d_st( st ), d_x( x ), d_y( y )
{
	setCaption( "Pick System" );

	QString str;
	SpinLabelSet::const_iterator i;

	Q3VBoxLayout* top = new Q3VBoxLayout( this, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	Q3GridLayout* contents = new Q3GridLayout( 5, 2 );

	str.sprintf( "Dim. x: New Spin" );
	contents->addWidget( new QLabel( str, this ), 1, 0 ); 
	d_lx = new Q3ComboBox( true, this );
	contents->addWidget( d_lx, 1, 1 );

	str.sprintf( "Dim. y: New Spin" );
	contents->addWidget( new QLabel( str, this ), 2, 0 ); 
	d_ly = new Q3ComboBox( true, this );
	contents->addWidget( d_ly, 2, 1 );

	str.sprintf( "System Type:" );
	contents->addWidget( new QLabel( str, this ), 3, 0 ); 

	Repository::SystemTypeMap::const_iterator p;
	const Repository::SystemTypeMap& sm = d_rep->getSystemTypes();
	d_z = new Q3ComboBox( false, this );
	d_z->insertItem( "" );
	for( p = sm.begin(); p != sm.end(); ++p )
		d_z->insertItem( (*p).second->getName().data() );
	d_z->listBox()->sort();
	contents->addWidget( d_z, 3, 1 );

	contents->addWidget( new QLabel( "Remember", this ), 4, 0 ); 
	d_r = new QCheckBox( this );
	d_r->setChecked( s_remember );
	contents->addWidget( d_r, 4, 1 );

	QPushButton* ok = new QPushButton( "&OK", this );
	QObject::connect( ok, SIGNAL( clicked() ), this, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", this );
	QObject::connect( cancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( contents );
	top->addLayout( buttons );
	connect( d_z, SIGNAL( activated ( int ) ), this, SLOT( handleSysType() ) );
}

void _LabelSysTypeDlg::setSysType( SystemType* s )
{
	if( !s_remember )
		s_sys = s;
}

bool _LabelSysTypeDlg::select( SpinLabel& lx, SpinLabel& ly )
{
	if( s_remember && s_sys )
	{
		for( int i = 0; i < d_z->count(); i++ )
		{
			if( d_z->text( i ) == s_sys->getName().data() )
			{
				d_z->setCurrentItem( i );
				break;
			}
		}
	}
	handleSysType();

	if( !s_remember )
	{
		d_lx->setEditText( lx.data() );
		d_ly->setEditText( ly.data() );
	}else
	{
		d_lx->setEditText( s_lx.data() );
		d_ly->setEditText( s_ly.data() );
	}
	if( lx.isNull() && d_lx->count() == 2 )
		d_lx->setEditText( d_lx->text( 1 ) );
	if( ly.isNull() && d_ly->count() == 2 )
		d_ly->setEditText( d_ly->text( 1 ) );

	while( exec() == QDialog::Accepted )
	{
		if( !SpinLabel::parse( d_lx->currentText(), lx ) )
		{
			QMessageBox::critical( this, "Edit Spin Labels",
					"Invalid x label!", "&Cancel" );
			continue;
		}
		if( !SpinLabel::parse( d_ly->currentText(), ly ) )
		{
			QMessageBox::critical( this, "Edit Spin Labels",
					"Invalid y label!", "&Cancel" );
			continue;
		}
		s_remember = d_r->isChecked();
		s_lx = lx;
		s_ly = ly;
		return true;
	}
	return false;
}
void _LabelSysTypeDlg::handleSysType()
{
	QString lx = d_lx->currentText();
	QString ly = d_ly->currentText();

	SpinLabelSet::const_iterator i;
	s_sys = d_rep->getTypes()->findSystemType( d_z->currentText().toLatin1() );
	SpinLabelSet sx = d_st->getLabels( d_x );
	SpinLabelSet sy = d_st->getLabels( d_y );
	NmrExperiment* e;
	e = d_rep->getTypes()->inferExperiment1( d_st, s_sys );
	if( e )
		e->getColumn( d_x, sx );
	e = d_rep->getTypes()->inferExperiment1( d_st, s_sys );
	if( e )
		e->getColumn( d_y, sy );
	d_lx->clear();
	d_lx->insertItem( "?" );
	for( i = sx.begin(); i != sx.end(); ++i )
		d_lx->insertItem( (*i).data() );
	d_lx->listBox()->sort();
	d_ly->clear();
	d_ly->insertItem( "?" );
	for( i = sy.begin(); i != sy.end(); ++i )
		d_ly->insertItem( (*i).data() );
	d_ly->listBox()->sort();

	if( lx == "?" && d_lx->count() == 2 )
		d_lx->setEditText( d_lx->text( 1 ) );
	else
		d_lx->setEditText( lx );
	if( ly == "?" && d_ly->count() == 2 )
		d_ly->setEditText( d_ly->text( 1 ) );
	else
		d_ly->setEditText( ly );
}

SystemType* _LabelSysTypeDlg::s_sys = 0;
bool _LabelSysTypeDlg::s_remember = false;
SpinLabel _LabelSysTypeDlg::s_lx, _LabelSysTypeDlg::s_ly;

bool Dlg::getLabelsSysType( QWidget* parent, LP& lp, 
	Repository* rep, SpectrumType* st, Dimension x, Dimension y )
{
	_LabelSysTypeDlg dlg( parent, rep, st, x, y );

	dlg.setSysType( lp.d_sys );
	bool res = dlg.select( lp.d_x, lp.d_y );
	lp.d_sys = dlg.getSysType();
	return res;
}

bool Dlg::getLabel( QWidget* parent, SpinLabel& lx, 
			const SpinLabelSet& sx )
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( "Select Spin Label" );
	dlg.setMinimumSize( 200, 200 );	// RISK

	QString str;
	SpinLabelSet::const_iterator i;

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	Q3VBoxLayout* contents = new Q3VBoxLayout(&dlg);

	Q3ListBox* _x = new Q3ListBox( &dlg );
	_x->setColumnMode( 5 );
	// _x->setRowMode( QListBox::FitToHeight );
	_x->setRowMode( Q3ListBox::Variable );
	_x->insertItem( "?" );
	if( lx.isNull() )
		_x->setSelected( _x->count() - 1, true );
	_x->insertItem( "?-1" );
	for( i = sx.begin(); i != sx.end(); ++i )
	{
		_x->insertItem( (*i).data() );
		if( (*i) == lx )
			_x->setSelected( _x->count() - 1, true );
	}
	contents->addWidget( _x );
	_x->sort();

	Q3HBox* box = new Q3HBox( &dlg );
	QCheckBox* cb = new QCheckBox( "&Draft (?)", box );
	cb->setChecked( (lx.isNull())?s_draft:(!lx.isFinal()) );
	new QLabel( "    Selected: ", box );
	QLineEdit* _z = new QLineEdit( box );
	_z->setMaxLength( 10 );
	_z->setText( _x->currentText() );
	contents->addWidget( box );
	QObject::connect( _x, SIGNAL( highlighted( const QString & ) ), _z, SLOT( setText( const QString& ) ) );

	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	QObject::connect( _x, SIGNAL( doubleClicked( Q3ListBoxItem * ) ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( contents );
	top->addLayout( buttons );

	while( dlg.exec() == QDialog::Accepted )
	{
		if( !SpinLabel::parse( _z->text(), lx ) )
		{
			QMessageBox::critical( &dlg, "Select Spin Label",
					"Invalid label!", "&Cancel" );
			continue;
		}
		s_draft = cb->isChecked();
		lx.setFinal( !s_draft );
		return true;
	}
	return false;
}

bool Dlg::getLabelPair( QWidget* parent, Root::SymbolString& CA, 
						  Root::SymbolString& CB,
						  const char* title, const char* up, const char* lo )
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( title );

	QString str;

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	Q3GridLayout* contents = new Q3GridLayout( 4, 2 );

	contents->addWidget( new QLabel( up, &dlg ), 1, 0 ); 
	QLineEdit* _ca = new QLineEdit( &dlg );
	contents->addWidget( _ca, 1, 1 );
	_ca->setText( CA.data() );

	contents->addWidget( new QLabel( lo, &dlg ), 2, 0 ); 
	QLineEdit* _cb = new QLineEdit( &dlg );
	contents->addWidget( _cb, 2, 1 );
	_cb->setText( CB.data() );

	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( contents );
	top->addLayout( buttons );

	if( dlg.exec() == QDialog::Accepted )
	{
		CA = _ca->text().toLatin1();
		CB = _cb->text().toLatin1();
		return true;
	}else
		return false;
}

static Gui::ListView* createList( QDialog* parent, const Dlg::SpinRanking & spins )
{
	Gui::ListView* lb = new Gui::ListView( parent );
	lb->setAllColumnsShowFocus( true );
	lb->setShowSortIndicator( true );
    lb->setRootIsDecorated(false);
    lb->addColumn( "Fit" );
	lb->addColumn( "Shift" );
	lb->addColumn( "Spin" );
	lb->addColumn( "Sys." );

	Dlg::SpinRanking::const_iterator p;
	for( p = spins.begin(); p != spins.end(); ++p )
		new Dlg::SpinItem( lb, (*p).first, (*p).second );
    lb->resizeAllColumnsToContents();
	return lb;
}

Spin* Dlg::selectSpin(QWidget * parent, const SpinRanking & spins, const char *title)
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( title );
	// dlg.resize( 200, 400 );

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();

	Gui::ListView* lb = createList( &dlg, spins );
	lb->setSorting( 0, false );
	top->addWidget( lb );
	QObject::connect( lb, SIGNAL( doubleClicked ( Gui::ListViewItem * ) ), &dlg, SLOT( accept() ) );

	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( buttons );

	if( dlg.exec() == QDialog::Accepted && lb->selectedItem() )
	{
		SpinItem* i = (SpinItem*)lb->selectedItem();
		return i->getSpin();
	}else
		return 0;
}

Dlg::SpinTuple Dlg::selectSpinPair( QWidget* parent, const SpinRanking& lhs, 
							   const SpinRanking& rhs, const char* title )
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( title );
	// dlg.resize( 400, 400 );

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* hbox = new Q3HBoxLayout(&dlg);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();

	Gui::ListView* lb1 = createList( &dlg, lhs );
	lb1->setSorting( 0, false );
	Gui::ListView* lb2 = createList( &dlg, rhs );
	lb2->setSorting( 0, false );

	hbox->addWidget( new QLabel( "X:", &dlg ) );
	hbox->addWidget( lb1 );
	hbox->addWidget( new QLabel( " Y:", &dlg ) );
	hbox->addWidget( lb2 );

	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( hbox );
	top->addLayout( buttons );

	if( dlg.exec() == QDialog::Accepted && lb1->selectedItem() &&
		lb2->selectedItem() )
	{
		SpinTuple res( 2 );
		res[ DimX ] = ((SpinItem*)lb1->selectedItem())->getSpin();
		res[ DimY ] = ((SpinItem*)lb2->selectedItem())->getSpin();
		return res;
	}else
		return SpinTuple();
}

Dlg::SpinTuple Dlg::selectSpinTriple( QWidget* parent, const SpinRanking& l1, 
								 const SpinRanking& l2, const SpinRanking& l3, const char* title )
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( title );
	// dlg.resize( 600, 400 );

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* hbox = new Q3HBoxLayout(&dlg);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();

	Gui::ListView* lb1 = createList( &dlg, l1 );
	lb1->setSorting( 0, false );
	Gui::ListView* lb2 = createList( &dlg, l2 );
	lb2->setSorting( 0, false );
	Gui::ListView* lb3 = createList( &dlg, l3 );
	lb3->setSorting( 0, false );

	hbox->addWidget( new QLabel( "X:", &dlg ) );
	hbox->addWidget( lb1 );
	hbox->addWidget( new QLabel( " Y:", &dlg ) );
	hbox->addWidget( lb2 );
	hbox->addWidget( new QLabel( " Z:", &dlg ) );
	hbox->addWidget( lb3 );

	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( hbox );
	top->addLayout( buttons );

	if( dlg.exec() == QDialog::Accepted && lb1->selectedItem() &&
		lb2->selectedItem() && lb3->selectedItem() )
	{
		SpinTuple res( 3 );
		res[ DimX ] = ((SpinItem*)lb1->selectedItem())->getSpin();
		res[ DimY ] = ((SpinItem*)lb2->selectedItem())->getSpin();
		res[ DimZ ] = ((SpinItem*)lb3->selectedItem())->getSpin();
		return res;
	}else
		return SpinTuple();
}

Dlg::SpinTuple Dlg::selectSpinQuadruple( QWidget* parent, const SpinRanking& l1,
								 const SpinRanking& l2, const SpinRanking& l3,
                                         const SpinRanking& l4, const char* title )
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( title );
	// dlg.resize( 600, 400 );

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* hbox = new Q3HBoxLayout(&dlg);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();

	Gui::ListView* lb1 = createList( &dlg, l1 );
	lb1->setSorting( 0, false );
	Gui::ListView* lb2 = createList( &dlg, l2 );
	lb2->setSorting( 0, false );
	Gui::ListView* lb3 = createList( &dlg, l3 );
	lb3->setSorting( 0, false );
    Gui::ListView* lb4 = createList( &dlg, l4 );
	lb4->setSorting( 0, false );

	hbox->addWidget( new QLabel( "X:", &dlg ) );
	hbox->addWidget( lb1 );
	hbox->addWidget( new QLabel( " Y:", &dlg ) );
	hbox->addWidget( lb2 );
	hbox->addWidget( new QLabel( " Z:", &dlg ) );
	hbox->addWidget( lb3 );
    hbox->addWidget( new QLabel( " W:", &dlg ) );
	hbox->addWidget( lb4 );

	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( hbox );
	top->addLayout( buttons );

	if( dlg.exec() == QDialog::Accepted && lb1->selectedItem() &&
		lb2->selectedItem() && lb3->selectedItem() && lb4->selectedItem() )
	{
		SpinTuple res( 4 );
		res[ DimX ] = ((SpinItem*)lb1->selectedItem())->getSpin();
		res[ DimY ] = ((SpinItem*)lb2->selectedItem())->getSpin();
		res[ DimZ ] = ((SpinItem*)lb3->selectedItem())->getSpin();
        res[ DimW ] = ((SpinItem*)lb3->selectedItem())->getSpin();
		return res;
	}else
		return SpinTuple();
}


bool Dlg::getSpectrumFormat( QWidget* parent, int& res, 
							Spec::Amplitude& pmax, Spec::Amplitude& nmax )
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( "Saving CARA Spectrum" );

	QString str;

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();

	Q3ButtonGroup* box = new Q3ButtonGroup( 1, Qt::Horizontal, "Select Format", &dlg );
	top->addWidget( box );
	box->setExclusive( true );

	QRadioButton* r = new QRadioButton( "8 Bit Symmetric Compression", box );
	box->insert( r, 0 );
	r = new QRadioButton( "8 Bit Symmetric Compression, Clipped", box );
	box->insert( r, 100 );
	r = new QRadioButton( "8 Bit Adaptive Compression", box );
	box->insert( r, 5 );
	r = new QRadioButton( "8 Bit Adaptive Compression, Clipped", box );
	box->insert( r, 105 );
	// r = new QRadioButton( "8 Bit Positive Side Compression", box );
	// box->insert( r, 6 );
	// r = new QRadioButton( "8 Bit Negative Side Compression", box );
	// box->insert( r, 7 );
	// r = new QRadioButton( "8 Bit Uncompressed", box );
	// box->insert( r, 1 );
	r = new QRadioButton( "8 Bit Uncompressed, Clipped", box );
	box->insert( r, 101 );
	r = new QRadioButton( "16 Bit Uncompressed", box );
	box->insert( r, 2 );
	r = new QRadioButton( "16 Bit Uncompressed, Clipped", box );
	box->insert( r, 102 );
	r = new QRadioButton( "32 Bit Uncompressed", box );
	box->insert( r, 3 );

	box->setButton( 2 );

	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( buttons );

	while( dlg.exec() == QDialog::Accepted )
	{
		res = box->id( box->selected() );
		if( res >= 100 )
			return getBounds( parent, nmax, pmax, "Set Maximum Amplitudes" );
		return true;
	}
	return false;
}

int Dlg::getOption(QWidget * parent, const StringList& l, const char* title, int init )
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( title );

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();

	Root::Vector<QRadioButton*> b( l.size() );
	Q3ButtonGroup* box = new Q3ButtonGroup( 1, Qt::Horizontal, "Select Option", &dlg );
	top->addWidget( box );
    for( int i = 0; i < int(l.size()); i++ )
	{
		b[ i ] = new QRadioButton( l[ i ].data(), box );
		box->insert( b[ i ], i );
	}
	box->setExclusive( true );
	// Weder fr ButtonGroup noch RadioButton gibt es ein DblClck-Signal

	if( init != -1 )
		box->setButton( init );
	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( buttons );

	if( dlg.exec() == QDialog::Accepted )
	{
		return box->id( box->selected() );
	}
	return -1;
}

bool Dlg::getStrings(QWidget * parent, StringList& l, const char* title )
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( title );

	QString str;

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();

	Q3ListBox* box = new Q3ListBox( &dlg );
	box->setSelectionMode( Q3ListBox::Extended );
	top->addWidget( box );
    for( int i = 0; i < int(l.size()); i++ )
	{
		box->insertItem( l[ i ].data() );
	}
	box->sort();
	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( buttons );

	if( dlg.exec() == QDialog::Accepted )
	{
		l.clear();
		Q3ListBoxItem* i = box->firstItem();
		while( i )
		{
			if( i->selected() )
				l.push_back( i->text().toLatin1() );
			i = i->next();
		}
		return true;
	}else
		return false;
}

SpinPoint Dlg::selectTuple(QWidget * parent, const SpinSpace::Result & tuples, 
						   Dimension dim, const char *title)
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( title );
	// dlg.resize( 400, 400 );

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();

	Gui::ListView* lb = new Gui::ListView( &dlg );
	lb->setAllColumnsShowFocus( true );
	lb->setShowSortIndicator( true );
    lb->setRootIsDecorated(false);
    lb->addColumn( "Shift X" );
	lb->addColumn( "Spin X" );
	lb->addColumn( "Sys. X" );
	if( dim > 1 )
	{
		lb->addColumn( "Shift Y" );
		lb->addColumn( "Spin Y" );
		lb->addColumn( "Sys. Y" );
	}
	if( dim > 2  )
	{
		lb->addColumn( "Shift Z" );
		lb->addColumn( "Spin Z" );
		lb->addColumn( "Sys. Z" );
	}
    if( dim > 3  )
	{
		lb->addColumn( "Shift W" );
		lb->addColumn( "Spin W" );
		lb->addColumn( "Sys. W" );
	}
	SpinSpace::Result::const_iterator p;
	for( p = tuples.begin(); p != tuples.end(); ++p )
	{
		if( !(*p).isGhost() && !(*p).isHidden() )
			new _TupleItem( lb, (*p).d_point );
	}

    lb->resizeAllColumnsToContents();
	top->addWidget( lb );
	QObject::connect( lb, SIGNAL( doubleClicked ( Gui::ListViewItem * ) ), 
		&dlg, SLOT( accept() ) );

	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( buttons );
    dlg.resize( 800, 600 ); // RISK

	if( dlg.exec() == QDialog::Accepted && lb->selectedItem() )
	{
		_TupleItem* i = (_TupleItem*)lb->selectedItem();
		return i->d_t;
	}else
		return SpinPoint();
}

bool Dlg::getLinkParams(QWidget * parent, int count, LinkParams & params)
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( "Import Spin Links" );

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	Q3GridLayout* contents = new Q3GridLayout( 5, 2 );

	contents->addWidget( new QLabel( "Inter-Residuals Only:", &dlg ), 1, 0 ); 
	QCheckBox* _inter = new QCheckBox( "", &dlg );
	_inter->setChecked( params.d_onlyInter );
	contents->addWidget( _inter, 1, 1 );

	contents->addWidget( new QLabel( "Apply Shift Aliasses:", &dlg ), 2, 0 ); 
	QCheckBox* _shift = new QCheckBox( &dlg );
	_shift->setChecked( params.d_useShifts );
	contents->addWidget( _shift, 2, 1 );

	contents->addWidget( new QLabel( "Hide Other Links:", &dlg ), 3, 0 ); 
	QCheckBox* _hide = new QCheckBox( &dlg );
	_hide->setChecked( params.d_hideOthers );
	contents->addWidget( _hide, 3, 1 );

	contents->addWidget( new QLabel( "Restrict Links:", &dlg ), 4, 0 ); 
	Q3ComboBox* _atom = new Q3ComboBox( &dlg );
	_atom->insertItem( "" );
	int cur = 0;
	for( int i = AtomType::H1; i < AtomType::MaxIsotope; i++ )
	{
		_atom->insertItem( AtomType::s_labels[ i ] );
		if( params.d_atom == AtomType( (AtomType::Isotope) i ) )
			cur = i;
	}
	contents->addWidget( _atom, 4, 1 );
	_atom->setCurrentItem( cur );

	QPushButton* ok = new QPushButton( "&OK", &dlg );
	QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", &dlg );
	QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
	buttons->addWidget( cancel );

	QString str;
	str.sprintf( "Do you really want to import %d spin links? This cannot be undone!", count );
	top->addWidget( new QLabel( str, &dlg ) );
	top->addLayout( contents );
	top->addLayout( buttons );

	if( dlg.exec() == QDialog::Accepted )
	{
		params.d_atom = AtomType::Isotope( _atom->currentItem() );
		params.d_onlyInter = _inter->isChecked();
		params.d_useShifts = _shift->isChecked();
		params.d_hideOthers = _hide->isChecked();
		return true;
	}
	return false;
}

bool Dlg::getLinkParams2(QWidget * parent, LinkParams2 & params)
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( "Set Link Parameters" );

	QString str;

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	Q3GridLayout* contents = new Q3GridLayout( 4, 2 );

	contents->addWidget( new QLabel( "Visible:", &dlg ), 0, 0 ); 
	QCheckBox* visi = new QCheckBox( &dlg );
	visi->setChecked( params.d_visible );
	contents->addWidget( visi, 0, 1 );

	contents->addWidget( new QLabel( "Rating:", &dlg ), 1, 0 ); 
	QLineEdit* rating = new QLineEdit( &dlg );
	contents->addWidget( rating, 1, 1 );
	str.sprintf( "%.2f", params.d_rating );
	rating->setText( str );

	contents->addWidget( new QLabel( "Code:", &dlg ), 2, 0 ); 
	QLineEdit* code = new QLineEdit( &dlg );
	contents->addWidget( code, 2, 1 );
	str.setNum( int( params.d_code ) );
	code->setText( str );

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
	int res;
	while( true )
	{
		if( dlg.exec() == QDialog::Accepted )
		{
			params.d_rating = rating->text().toFloat( &_ok );
			if( !_ok )
			{
				QMessageBox::critical( &dlg, "Get Link Parameters",
						"Invalid rating value!", "&Cancel" );
				continue;
			}
			res = code->text().toShort( &_ok );
			if( !_ok || res < 0 || res > 255 )
			{
				QMessageBox::critical( &dlg, "Get Link Parameters",
						"Invalid code value!", "&Cancel" );
				continue;
			}
			params.d_code = res;
			params.d_visible = visi->isChecked();
			return true;
		}else
			return false;
	}
}

bool Dlg::getAssig( QWidget* parent, Dimension dim, Assig& ass, float* p, const char* title )
{
	QDialog dlg( parent, "", true );
	dlg.setCaption( title );

	QString str;

	Q3VBoxLayout* top = new Q3VBoxLayout(&dlg, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	Q3GridLayout* contents = new Q3GridLayout( dim + 1, 2 );

	Root::Vector<QLineEdit*> e( dim );
	QLineEdit* f;
	Dimension d;
	for( d = 0; d < dim; d++ )
	{
		str.sprintf( "Dim. %s (%s):", getDimLetter( d, false ), getDimSymbol( d, true ) );
		contents->addWidget( new QLabel( str, &dlg ), d, 0 ); 
		str.sprintf( "%d", ass[ d ] );
		e[ d ] = new QLineEdit( &dlg );
		contents->addWidget( e[ d ], d, 1 );
		e[ d ]->setText( str );
	}
	contents->addWidget( new QLabel( "Probability:", &dlg ), dim, 0 ); 
	f = new QLineEdit( &dlg );
	if( p == 0 )
	{
		f->setReadOnly( true );
		f->setText( "1.0" );
	}else
	{
		str.sprintf( "%0.2f", *p );
		f->setText( str );
	}
	contents->addWidget( f, dim, 1 );

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
	int err = 0;
	while( dlg.exec() == QDialog::Accepted )
	{
		err = 0;
		for( d = 0; d < dim; d++ )
		{
			ass[ d ] = e[ d ]->text().toInt( &_ok );
			if( !_ok )
				err++;
		}
		if( p )
		{
			*p = f->text().toFloat( &_ok );
			if( !_ok || *p < 0.0 || *p > 1.0 )
				err++;
		}
		if( err )
		{
			QMessageBox::critical( &dlg, title,
					"Invalid value!", "&Cancel" );
			continue;
		}
		return true;
	}
	return false;
}


ColorEdit::ColorEdit(QWidget* parent, IntensityView* iv):
	QDialog( parent )
{
	d_iv = iv;
	assert( d_iv );
	d_intens = d_iv->getIntens();
	d_thres = d_iv->getThres();

	setModal( true );
	setCaption( "Intensity Color Adjustments" );

	Q3VBoxLayout* top = new Q3VBoxLayout(this, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();
	Q3GridLayout* contents = new Q3GridLayout( 2, 2 );

	QPushButton* ok = new QPushButton( "&OK", this );
	QObject::connect( ok, SIGNAL( clicked() ), this, SLOT( accept() ) );	// responsevalues
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", this );
	QObject::connect( cancel, SIGNAL( clicked() ), this, SLOT( reject() ) );	// ok_callback
	buttons->addWidget( cancel );

	contents->addWidget( new QLabel( "Intensity", this ), 0, 0 ); 
	contents->addWidget( new QLabel( "Threshold", this ), 1, 0 ); 
	QSlider* s = new QSlider( 0, 47, 10, d_intens, Qt::Horizontal, this );
	s->setTracking( false );
	connect( s, SIGNAL( valueChanged ( int ) ), 
		this, SLOT( handleIntensity( int ) ) );
	contents->addWidget( s, 0, 1 );
	s = new QSlider( 0, 47, 10, d_thres, Qt::Horizontal, this );
	s->setTracking( false );
	connect( s, SIGNAL( valueChanged ( int ) ), 
		this, SLOT( handleThreshold( int ) ) );
	contents->addWidget( s, 1, 1 );
	top->addLayout( contents );
	top->addLayout( buttons );
}
bool ColorEdit::run()
{
	if( exec() != QDialog::Accepted )
	{
		d_iv->setIntensThres( d_intens, d_thres );
		return false;
	}else
		return true;
}
void ColorEdit::handleIntensity( int v )
{
	d_iv->setIntensThres( v, d_iv->getThres() );
}
void ColorEdit::handleThreshold( int v )
{
	d_iv->setIntensThres( d_iv->getIntens(), v );
}

bool Dlg::adjustIntensity( QWidget* parent, IntensityView* iv )
{
	ColorEdit dlg( parent, iv );
	return dlg.run();
}

