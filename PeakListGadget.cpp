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

#include "PeakListGadget.h"

#include <QMessageBox>
#include <QFileInfo> 
#include <QApplication>
#include <Gui/ListView.h>
#include <QHeaderView>
#include <Spec/Spectrum.h>
#include <QFileDialog>
#include <Spec/Factory.h> 
#include <Spec/Project.h> 
#include <Spec/Repository.h> 
#include <fstream>
#include <QInputDialog>

//* Lexi
#include <Lexi/ContextMenu.h>
#include <Lexi/MainWindow.h>
#include <Lexi/CommandLine.h>
#include <Lexi/LexiListView.h>
#include <SpecView/ObjectListView.h>
#include <SpecView/SpinPointView.h>
using namespace Lexi;
using namespace Spec;

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr PeakListGadget::AddColumn = "AddColumn";
Root::Action::CmdStr PeakListGadget::RemoveCols = "RemoveCols";

ACTION_SLOTS_BEGIN( PeakListGadget )
    { PeakListGadget::RemoveCols, &PeakListGadget::handleRemoveCols },
    { PeakListGadget::AddColumn, &PeakListGadget::handleAddColumn },
ACTION_SLOTS_END( PeakListGadget )

//////////////////////////////////////////////////////////////////////

static char s_buffer[ 32 ];

static QString g_lastDir; // = QString::null;

static std::vector<Root::SymbolString> s_cols;

static const int s_top = 1;
static const int s_dimInvar = 5;

class _PeakItemImp : public Gui::ListViewItem
{
public:
	_PeakItemImp( Gui::ListView* parent, Peak* peak, _PeakController* home ):
		Gui::ListViewItem( parent ), d_peak( peak ), d_home( home ) 
		{
		}

	QString text( int col ) const;
	QString key( int col, bool ) const;
	Root::ExRef<Peak> d_peak;
	Root::Ptr<_PeakController> d_home;
};

class _PeakGuessItem : public Gui::ListViewItem
{
public:
	_PeakGuessItem( _PeakItemImp* parent, Peak::Guess* guess ):
		Gui::ListViewItem( parent ), d_guess( guess ) {}

	QString text( int col ) const;
	QString key( int col, bool ) const
	{
		_PeakItemImp* p = (_PeakItemImp*) parent();
		const Dimension dim = p->d_peak->getDimCount();
		if( col == 0 )
		{
			::sprintf( s_buffer, "%06d", d_guess->getId() );
			return s_buffer;
		}else if( col < dim + 1 )
		{
			if( d_guess->getAssig()[ col - 1 ] != 0 )
				::sprintf( s_buffer, "%04d", d_guess->getAssig()[ col - 1 ] );
			else
				*s_buffer = 0;
			return s_buffer;
		}else if( col == dim + 5 )
		{
			::sprintf( s_buffer, "%02.2f", d_guess->getProb() );
			return s_buffer;
		}
		else
		{
			int id = col - dim - s_top - s_dimInvar;
            if( id >= 0 && id < int(s_cols.size()) )
			{
				Root::Object* o = d_guess;
				Root::Any a;
				o->getFieldValue( s_cols[ id ], a );
				if( a.isNull() )
					return "";
				else if( a.isNumber() )
				{
					::sprintf( s_buffer, "%08.3f", a.getDouble() );
					return s_buffer;
				}else
					return a.getCStr();
			}
		}
		return "";
	}
	Root::ExRef<Peak::Guess> d_guess;
};

class _PeakController : public Lexi::ListView
{
public:
	Root::Ref<PeakList> d_model;
	Root::Ref<Spectrum> d_spec;
	Root::Ref<Project> d_pro;
	Root::Ref<ContextMenu> d_kontext;
	QObject* d_main;
	bool d_locked;
	bool d_showAssig;

	_PeakController(Lexi::Viewport* vp,PeakList* m, Spectrum* spec, QObject* main, 
		ContextMenu* popup, bool showVol, Project* pro ):
		Lexi::ListView( vp->getWindow() ), d_kontext( popup ), d_model( m ),
		d_main( main ), d_locked( false ), d_spec( spec ), d_pro( pro )
	{
		d_showAssig = false;
		getImp()->setRootIsDecorated( true );
		getImp()->setShowSortIndicator( true );
		
		if( d_model )
			d_model->addObserver( this );
	}

	~_PeakController()
	{
		if( d_model )
		{
			d_model->removeObserver( this );
			d_model = 0;
			d_spec = 0;
			d_main = 0;
			d_pro = 0;
		}
	}

	void setModel(PeakList * mdl, Spectrum* spec = 0)
	{
		if( d_model )
			d_model->removeObserver( this );
		d_model = mdl;
		d_spec = spec;
		if( d_model )
			d_model->addObserver( this );
		//addColumns();
		load();
	}

	void handleRightClick(Gui::ListViewItem *, const QPoint & p)
	{
		if( d_kontext )
        {
            QPoint p2 = getImp()->mapToGlobal( p );
			d_kontext->show( p2 );
        }
	}

	void handleReturn(Gui::ListViewItem * i)
	{
		handleGoto( i );
	}

	void handleDblClick(Gui::ListViewItem * i)
	{
		handleGoto( i );
	}

	void handleGoto(Gui::ListViewItem * i)
	{
		if( i && d_main )
		{
			_PeakItemImp* item;
			_PeakGuessItem* g = dynamic_cast<_PeakGuessItem*>( i );
			if( g )
				item = (_PeakItemImp*) g->parent();
			else
				item = dynamic_cast<_PeakItemImp*>( i );
			if( item )
			{
				PeakListGadget::ActivatePeakMsg msg( item->d_peak->getId() );
				d_main->event( &msg );
			}
		}
	}

	void handle(Message& msg )
	{
		if( PeakList::Update* m = dynamic_cast<PeakList::Update*>( &msg ) )
		{
			if( d_locked )
				return;
			switch( m->getType() )
			{
			case PeakList::Update::Add: 
				{
					new _PeakItemImp( getImp(), m->getPeak(), this );
				}
				break;
			case PeakList::Update::Position:
			case PeakList::Update::Data:
				handleChange( m->getPeak() );
				break;
			case PeakList::Update::Delete:
				handleDelete( m->getPeak() );
				break;
			case PeakList::Update::Guess:
				handleReplace( m->getPeak() );
				break;
			case PeakList::Update::Everything:
				load();
				break;
            default:
                break;
			}
			m->consume();
		}else
			Handler::handle( msg );
	}

	void addColumns()
	{
		getImp()->clear();
		while( getImp()->columns() )
			getImp()->removeColumn( 0 );

		if( d_model == nil )
			return;

		// s_top = 1 Anzahl Spalten
		getImp()->addColumn( "ID" );
		for( int d = 0; d < d_model->getDimCount(); d++ )
		{
			QString str;
			str.sprintf( "%s", d_model->getColors()[d].getIsoLabel() );
			getImp()->addColumn( str );
		}

		// s_dimInvar = 5 Anzahl zustzlicher Spalten
		getImp()->addColumn( "Vol." );
		getImp()->addColumn( "Amp." );
		getImp()->addColumn( "Label" );
		getImp()->addColumn( "Color" );
		getImp()->addColumn( "Prob." );
        for( int i = 0; i < int(s_cols.size()); i++ )
			getImp()->addColumn( s_cols[ i ].data() );
	}

	void handleDelete(Peak* p)
	{

		Gui::ListViewItemIterator it( getImp() );
		// iterate through all items of the listview
		for ( ; it.current(); ++it ) 
		{
			_PeakItemImp* pi = dynamic_cast<_PeakItemImp*>( it.current() );
			if( pi && pi->d_peak == p )
			{
				pi->removeMe();
				return;
			}
		}
	}

	void handleChange(Peak* p)
	{

		Gui::ListViewItemIterator it( getImp() );
		// iterate through all items of the listview
		for ( ; it.current(); ++it ) 
		{
			_PeakItemImp* pi = dynamic_cast<_PeakItemImp*>( it.current() );
			if( pi && pi->d_peak == p )
			{
				pi->repaint();
				for( int i = 0; i < pi->count(); i++ )
				{
					pi->child(i)->repaint();
				}
				return;
			}
		}
	}

	void handleReplace(Peak* p)
	{

		Gui::ListViewItemIterator it( getImp() );
		// iterate through all items of the listview
		for ( ; it.current(); ++it ) 
		{
			_PeakItemImp* pi = dynamic_cast<_PeakItemImp*>( it.current() );
			if( pi && pi->d_peak == p )
			{
				_PeakItemImp* i = add( p );
				i->setOpen( pi->isOpen() );
				pi->removeMe();
				return;
			}
		}
	}

	_PeakItemImp* add( Peak* p )
	{
		_PeakItemImp* pi = new _PeakItemImp( getImp(), p, this );
		Peak::GuessMap::const_iterator i;
		for( i = p->getGuesses().begin(); i != p->getGuesses().end(); ++i )
			new _PeakGuessItem( pi, (*i).second );
		return pi;
	}
	void load()
	{
		if( d_model == nil )
			return;
		d_locked = false;
		addColumns();
		PeakList::Peaks::const_iterator i;
		for( i = d_model->getPeaks().begin(); i != d_model->getPeaks().end(); ++i )
		{
			add( (*i).second );
		}
		for( int i = 0; i < getImp()->columnCount(); i++ )
			getImp()->resizeColumnToContents( i );
	}

	void setSpec( Spectrum* spec )
	{
		d_spec = spec;
		load();
	}
	Spectrum* getSpec() const { return d_spec; }
};

QString _PeakItemImp::text( int col ) const
{
	Peak::Position pd;
	bool alias = false;
	d_peak->getPosition( pd, d_home->getSpec(), &alias );
	const Dimension dim = d_peak->getDimCount();
	if( col == dim + 1 )
	{
		if( pd.d_vol != 0.0 )
			::sprintf( s_buffer, "% 8.0f", pd.d_vol );
		else
			*s_buffer = 0;
		return s_buffer;
	}else if( col == dim + 2 )
	{
		if( pd.d_amp != 0.0 )
			::sprintf( s_buffer, "% 8.0f", pd.d_amp );
		else
			*s_buffer = 0;
		return s_buffer;
	}else if( col == dim + 3 )
	{
		return d_peak->getTag().data();
	}else if( col == dim + 4 )
	{
		if( d_peak->getColor() )
			::sprintf( s_buffer, "%d", d_peak->getColor() );
		else
			*s_buffer = 0;
		return s_buffer;
	}else if( col < dim + 1 )
	{
		switch( col )
		{
		case 0: // ID
			::sprintf( s_buffer, "%d", d_peak->getId() );
			return s_buffer;
		default:
			if( d_home->d_showAssig )
			{
				if( col - 1 < dim && d_peak->getAssig()[ col - 1 ] != 0 )
				{
					if( d_home->d_pro )
					{
						Spin* spin = d_home->d_pro->getSpins()->getSpin( 
							d_peak->getAssig()[ col - 1 ] );
						if( spin )
							SpinPointView::formatLabel( s_buffer, sizeof(s_buffer),
								spin, SpinPointView::SysOrResiTagAll, DimX );
						else
							::sprintf( s_buffer, "(%d)", d_peak->getAssig()[ col - 1 ] );
					}else
						::sprintf( s_buffer, "%d", d_peak->getAssig()[ col - 1 ] );
				}else
					*s_buffer = 0;
			}else
				::sprintf( s_buffer, "%.3f", pd.d_pos[ col - 1 ] );
			return s_buffer;
		}
	}else
	{
		int id = col - dim - s_top - s_dimInvar;
        if( id >= 0 && id < int(s_cols.size()) )
		{
			Root::Object* o = d_peak;
			Root::Any a;
			o->getFieldValue( s_cols[ id ], a );
			if( a.isNull() )
				return "";
			else
				return a.getCStr();
		}
	}
	return "";
}
QString _PeakItemImp::key( int col, bool ) const
{
	Peak::Position pd;
	bool alias = false;
	d_peak->getPosition( pd, d_home->getSpec(), &alias );
	const Dimension dim = d_peak->getDimCount();
	if( col == dim + 1 )
	{
		/* TODO: wieso?
		if( pd.isAlias() )
			return "";
			*/
		if( pd.d_vol != 0.0 )
			::sprintf( s_buffer, "%08.0f", pd.d_vol );
		else
			*s_buffer = 0;
		return s_buffer;
	}else if( col == dim + 2 )
	{
		/* TODO: wieso?
		if( pd.isAlias() )
			return "";
			*/
		if( pd.d_amp != 0.0 )
			::sprintf( s_buffer, "%08.0f", pd.d_amp );
		else
			*s_buffer = 0;
		return s_buffer;
	}else if( col == dim + 3 )
	{
		return d_peak->getTag().data();
	}else if( col == dim + 4 )
	{
		if( d_peak->getColor() )
			::sprintf( s_buffer, "%04d", d_peak->getColor() );
		else
			*s_buffer = 0;
		return s_buffer;
	}else if( col < dim + 1 )
		switch( col )
		{
		case 0: // ID
			::sprintf( s_buffer, "%06d", d_peak->getId() );
			return s_buffer;
		default:
			if( d_home->d_showAssig )
			{
				if( col - 1 < dim && d_peak->getAssig()[ col - 1 ] != 0 )
					::sprintf( s_buffer, "%04d", d_peak->getAssig()[ col - 1 ] );
				else
					*s_buffer = 0;
			}else
				::sprintf( s_buffer, "%06.3f", pd.d_pos[ col - 1 ] );
			return s_buffer;
		}
	else
	{
		int id = col - dim - s_top - s_dimInvar;
        if( id >= 0 && id < int(s_cols.size()) )
		{
			Root::Object* o = d_peak;
			Root::Any a;
			o->getFieldValue( s_cols[ id ], a );
			if( a.isNull() )
				return "";
			else if( a.isNumber() )
			{
				::sprintf( s_buffer, "%08.3f", a.getDouble() );
				return s_buffer;
			}else
				return a.getCStr();
		}
	}
	return "";
}
QString _PeakGuessItem::text( int col ) const
{
	_PeakItemImp* p = (_PeakItemImp*) parent();
	const Dimension dim = p->d_peak->getDimCount();
	if( col == 0 )
	{
		::sprintf( s_buffer, "%d", d_guess->getId() );
		return s_buffer;
	}else if( col < dim + 1 )
	{
		if( d_guess->getAssig()[ col - 1 ] != 0 )
		{
			if( p->d_home->d_pro )
			{
				Spin* spin = p->d_home->d_pro->getSpins()->getSpin( 
					d_guess->getAssig()[ col - 1 ] );
				if( spin )
					SpinPointView::formatLabel( s_buffer, sizeof(s_buffer),
						spin, SpinPointView::SysOrResiTagAll, DimX );
				else
					::sprintf( s_buffer, "(%d)", d_guess->getAssig()[ col - 1 ] );
			}else
				::sprintf( s_buffer, "%d", d_guess->getAssig()[ col - 1 ] );
		}else
			*s_buffer = 0;
		return s_buffer;
	}else if( col == dim + 5 )
	{
		::sprintf( s_buffer, "%0.2f", d_guess->getProb() );
		return s_buffer;
	}else
	{
		int id = col - dim - s_top - s_dimInvar;
        if( id >= 0 && id < int(s_cols.size()) )
		{
			Root::Object* o = d_guess;
			Root::Any a;
			o->getFieldValue( s_cols[ id ], a );
			if( a.isNull() )
				return "";
			else
				return a.getCStr();
		}
	}
	return "";
}

PeakListGadget::PeakListGadget(Viewport* v, PeakList* ps, Spectrum* spec,
		QObject* main, Glyph* popup, bool showVol, Project* pro )
{
	d_this = new _PeakController( v, ps, spec, main,
		dynamic_cast<ContextMenu*>( popup ), showVol, pro );
	d_this->addRef();

	setBody( d_this );
	setController( d_this );
	d_this->getImp()->show();
	
	d_this->addColumns();
	d_this->load();
}

PeakListGadget::~PeakListGadget()
{
	d_this->release();
	if( getBody() )
	{
		setController( 0 );
		setBody( 0 );
		d_this = 0;
	}
}

void PeakListGadget::setPopup( Lexi::Glyph * g)
{
	d_this->d_kontext = dynamic_cast<ContextMenu*>( g );
}

void PeakListGadget::handle(Root::Message & msg)
{
	BEGIN_HANDLER();
	MESSAGE( Root::Action, a, msg )
	{
		EXECUTE_ACTION( PeakListGadget, *a );
	}
	END_HANDLER();
}

void PeakListGadget::setPeakList(PeakList * ps, Spectrum* s)
{
	d_this->setModel( ps, s );
}

Root::Index PeakListGadget::getSelected() const
{
	ListView* lv = dynamic_cast<ListView*>( getBody() );
	assert( lv );
	_PeakItemImp* pi = dynamic_cast<_PeakItemImp*>( lv->getImp()->currentItem() );
	if( pi == 0 )
		return 0;
	else
		return pi->d_peak->getId();
}

void PeakListGadget::lock()
{
	d_this->d_locked = true;
}

void PeakListGadget::reload()
{
	d_this->load();
}

void PeakListGadget::setSpec(Spectrum * spec)
{
	d_this->setSpec( spec );
}

bool PeakListGadget::showAssig() const
{
	_PeakController* p = dynamic_cast<_PeakController*>( getController() );
	assert( p );
	return p->d_showAssig;
}

void PeakListGadget::showAssig(bool on)
{
	d_this->d_showAssig = on;
	d_this->getImp()->viewport()->update();
}

bool PeakListGadget::gotoPeak(Root::Index peak)
{
	if( peak == 0 )
		return false;
	_PeakItemImp* j;
	for( int i = 0; i < d_this->getImp()->count(); i++ )
	{
		j = dynamic_cast<_PeakItemImp*>( d_this->getImp()->child( i ) );
		if( peak && j && j->d_peak->getId() == peak )
		{
			d_this->getImp()->ensureItemVisible( j );
			d_this->getImp()->setSelected( j, true );
			return true;
		}
	}
	return false;
}

Peak* PeakListGadget::getPeak( bool recursive ) const
{
	ListView* lv = dynamic_cast<ListView*>( getBody() );
	assert( lv );
	_PeakItemImp* pi = dynamic_cast<_PeakItemImp*>( lv->getImp()->currentItem() );
	if( pi )
		return pi->d_peak;
	_PeakGuessItem* g = dynamic_cast<_PeakGuessItem*>( lv->getImp()->currentItem() );
	if( recursive && g )
	{
		pi = (_PeakItemImp*) g->parent();
		return pi->d_peak;
	}
	return 0;
}

Lexi::ListView* PeakListGadget::getListView() const
{
	return d_this;
}

Peak::Guess* PeakListGadget::getGuess() const
{
	ListView* lv = dynamic_cast<ListView*>( getBody() );
	assert( lv );
	_PeakGuessItem* g = dynamic_cast<_PeakGuessItem*>( lv->getImp()->currentItem() );
	if( g )
		return g->d_guess;
	return 0;
}

void PeakListGadget::saveTable(const char * path)
{
	ListView* lv = dynamic_cast<ListView*>( getBody() );
	assert( lv );

	std::ofstream out( path );
	if( !out )
		return;
	QHeaderView* h = lv->getImp()->header();
    Gui::ListViewItemIterator it( lv->getImp() );
    // iterate through all items of the listview
    for( ; it.current(); ++it ) 
	{
		for( int i = 0; i < h->count(); i++ )
		{
			if( i != 0 )
				 out << "\t";
            out << it.current()->text( h->logicalIndex( i ) ).toLatin1().data();
		}
		out << std::endl;
    }
	
}

void PeakListGadget::handleAddColumn(Root::Action & a)
{
	ACTION_ENABLED_IF( a, d_this->d_pro );

	ObjectDef* od = d_this->d_pro->getRepository()->findObjectDef( Repository::keyPeak );
	QStringList l;
	ObjectDef::Attributes::const_iterator i;
	for( i = od->getAttributes().begin(); i != od->getAttributes().end(); ++i )
		l.append( (*i).first.data() ); 
	od = d_this->d_pro->getRepository()->findObjectDef( Repository::keyGuess );
	for( i = od->getAttributes().begin(); i != od->getAttributes().end(); ++i )
		l.append( (*i).first.data() ); 

	l.sort();

	bool ok;
	QString res = QInputDialog::getItem( "Add Column", "Select an attribute:", 
		l, 0, false, &ok, d_this->getImp() );
	if( !ok || res.isEmpty() )
		return;
	s_cols.push_back( res.toLatin1() );
	d_this->getImp()->addColumn( res );
}

void PeakListGadget::handleRemoveCols(Root::Action & a)
{
	ACTION_ENABLED_IF( a, !s_cols.empty() );

    for( int i = 0; i < int(s_cols.size()); i++ )
		d_this->getImp()->removeColumn( d_this->getImp()->columns() - 1 );
	s_cols.clear();
}


