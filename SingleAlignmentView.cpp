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

#include "SingleAlignmentView.h"
#include <QSpinBox>
#include <QLayout>
#include <QLabel> 
#include <QAction> 
#include <QPainter> 
#include <QHBoxLayout>
#include <QFrame>
#include <QVBoxLayout>
#include <QItemDelegate>
#include <QHeaderView>
#include <QScrollBar>
#include <Spec/Residue.h>
#include <Spec/SpinSystem.h>
#include <Gui/Menu.h>
using namespace Spec;

static QString g_temp;
static char s_buf[64];
static int g_textWidth = 0;

class _FragViewImp : public QFrame 
{
	const SpinSystemString& d_frag;
	int d_off;
public:
	void setOff( int x ) { d_off = x; }
	int getOff() const { return d_off; }
	void drawContents( QPainter * );
	void paintEvent(QPaintEvent *)
	{
		QPainter painter(this);
		drawFrame(&painter);
		drawContents( &painter );
	}
	_FragViewImp( QWidget* p, int h, const SpinSystemString& f ):QFrame( p ), d_frag( f )
	{
		setFrameShadow( QFrame::Sunken );
		setFrameShape( QFrame::WinPanel );
		d_off = 0;
	}
	QSize sizeHint() const
	{
		const int w = frameWidth() * 2;
        return QSize( g_textWidth * ( d_frag.size() + 2 ) + 4 + w, fontMetrics().height() + 6 + w );
	}
};

class _SingleAlignmentList : public Gui::ListView
{
public:
	_SingleAlignmentList( SingleAlignmentView* p ):ListView( p ) {}

protected:
	void scrollContentsBy ( int dx, int dy )
	{
		ListView::scrollContentsBy( dx, dy );
		SingleAlignmentView* v = static_cast<SingleAlignmentView*>( parent() );
		assert( horizontalScrollBar() != 0 );
		v->handleMove( horizontalScrollBar()->value() );
	}
};

class _MatchItemImp : public Gui::ListViewItem 
{
public:
	_MatchItemImp( Gui::ListView* l, const FragmentAssignment::Match& m ):
	  Gui::ListViewItem( l ), d_match( m ) {}

	const FragmentAssignment::Match& d_match;
	void paintAss( QPainter * p, int width );
	void paintFit( QPainter*, int width );
	void paintSeq( QPainter*, int width );
	QString key( int column, bool ascending ) const;
	QString text( int i ) const;
	QVariant size( int col ) const
	{
		if( col == 0 )
			return QSize( ( d_match.d_fits.size() + 2 ) * g_textWidth, listView()->fontMetrics().height() + 3 );
		return QVariant();
	}
};

struct _SingleAlignmentViewDeleg : public QItemDelegate
{
	_SingleAlignmentViewDeleg( Gui::ListView* p ):QItemDelegate(p) {}
	Gui::ListView* listView() const { return static_cast<Gui::ListView*>( parent() ); }

	void paint( QPainter * p, const QStyleOptionViewItem & option, const QModelIndex & index ) const
	{
		if( _MatchItemImp* item = dynamic_cast<_MatchItemImp*>( listView()->indexToItem( index ) ) )
		{
			p->save();
			p->translate( option.rect.topLeft() );
			// width-1, height()-1 is the bottom right pixel in the cell
			if( index.column() == 0 )
				item->paintSeq( p, option.rect.width() );
			else if( index.column() == 1 )
				item->paintFit( p, option.rect.width() );
			else if( index.column() == 2 )
				item->paintAss( p, option.rect.width() );
			p->restore();
		}else
			QItemDelegate::paint( p, option, index );
	}

};

//////////////////////////////////////////////////////////////////////

SingleAlignmentView::SingleAlignmentView(Root::Agent * a, Spec::FragmentAssignment* ass ):
	QWidget( 0 ), d_threshold( 0 ), d_parent( a )
{
	setAttribute(Qt::WA_DeleteOnClose);
	assert( a );
	QVBoxLayout* vbox = new QVBoxLayout(this);
	vbox->setMargin( 1 );
	vbox->setSpacing( 1 );
	QHBoxLayout* hbox = new QHBoxLayout();
	hbox->setMargin( 0 );
	hbox->setSpacing( 0 );

	d_edit = new QSpinBox( 0, 100, 1, this );
	d_list = new _SingleAlignmentList( this );
	d_list->setAllColumnsShowFocus( true );
	d_list->setShowSortIndicator( true );
	d_list->setRootIsDecorated( false );
	d_list->addColumn( "Sequence/Fragment" );
	d_list->addColumn( "Fitness" );
	d_list->addColumn( "Assigned" );
	d_list->setItemDelegate( new _SingleAlignmentViewDeleg( d_list ) );
	d_list->setUniformRowHeights( true );
	d_list->setItemsExpandable( false );
	d_list->header()->setStretchLastSection( false );
	d_list->header()->setMovable( false );

	g_textWidth = d_list->fontMetrics().width( "(W000)" );	// RISK
	if( ass->getMatchCount() > 0 )
	{
		ass->getMatch(0).d_start->formatLabel( s_buf, sizeof(s_buf) );
		int w = d_list->fontMetrics().width( s_buf );
		if( w > g_textWidth )
			g_textWidth = w + w * 0.5;
	}
	
	d_label = new QLabel( this );
	d_label->setFrameStyle( QFrame::WinPanel | QFrame::Sunken );

	hbox->addWidget( d_label );
	QLabel* lbl = new QLabel( "Threshold:", this );
	lbl->setFrameStyle( QFrame::WinPanel | QFrame::Sunken );
	hbox->addWidget( lbl );

	hbox->addWidget( d_edit );
	vbox->addLayout( hbox );
	d_header = 0;
	if( ass )
	{
		d_header = new _FragViewImp( this, d_label->minimumHeight(), ass->getFragment() );
		vbox->addWidget( d_header );
	}
	vbox->addWidget( d_list );

	d_ass = ass;
	load();

	d_list->setSorting( 1, false );

	QString str = "Fragment Alignment";
	if( ass )
		str.sprintf( "Fragment Alignment: #%d - #%d", 
		ass->getFragment().front()->getId(), ass->getFragment().back()->getId() );
	setCaption( str );

    d_list->header()->setResizeMode( 0, QHeaderView::Fixed );
    d_list->setColumnWidth(0, d_header->sizeHint().width() );
    d_list->resizeColumnToContents( 1 );
    d_list->resizeColumnToContents( 2 );

	connect( d_edit, SIGNAL( valueChanged ( int ) ), this, SLOT( handleValueChanged ( int ) ) );

	QMenu* pop = new Gui::Menu( d_list, true ); 
	pop->insertItem( "Assign", this, SLOT( handleAssign() ) );
	d_geom = new QAction( this ); 
	d_geom->setCheckable( true );
	d_geom->setText( "Geometric Mapping" );
	d_geom->setOn( true );
	connect( d_geom, SIGNAL( toggled ( bool ) ), this, SLOT( handleGeom(bool) ) );
	d_geom->addTo( pop );
	d_exp = new QAction( this ); 
	d_exp->setText( "Use Neighbours" );
	d_exp->setCheckable( true );
	d_exp->setOn( true );
	connect( d_exp, SIGNAL( toggled ( bool ) ), this, SLOT( handleExp(bool) ) );
	d_exp->addTo( pop );
	d_strict = new QAction( this ); 
	d_strict->setText( "Drop Zero-Peaks" );
	d_strict->setCheckable( true );
	d_strict->setOn( true );
	connect( d_strict, SIGNAL( toggled ( bool ) ), this, SLOT( handleStrict(bool) ) );
	d_strict->addTo( pop );
	pop->insertItem( "Update", this, SLOT( handleUpdate() ) );
}

SingleAlignmentView::~SingleAlignmentView()
{
}

void SingleAlignmentView::handleMove( int x )
{
	if( d_header && d_header->getOff() != -x )
	{
		d_header->setOff( -x );
		d_header->repaint();
	}
}

void SingleAlignmentView::load()
{
	d_list->clear();
	if( d_ass.isNull() )
	{
		d_label->setText( "No Assignment" );
		return;
	}

	int count = 0;
	float thres = d_threshold / 100.0;

	for( int i = 0; i < d_ass->getMatchCount(); i++ )
	{
		if( d_ass->getMatch( i ).d_total > thres )
		{
			new _MatchItemImp( d_list, d_ass->getMatch( i ) );
			count++;
		}
	}

	g_temp.sprintf( "%d Matches out of %d possible", count, // d_ass->getMatchCount(), 
		d_ass->getBase()->getSeq()->getResi().size() - d_ass->getFragment().size() + 1 );
	d_label->setText( g_temp );
	// nein, neu Fixed: d_list->resizeColumnToContents( 0 );
}

QString _MatchItemImp::text(int i) const
{
	return "";
}

QString _MatchItemImp::key(int column, bool ascending) const
{
	if( column == 1 )
	{
		g_temp.sprintf( "%03d", int( 100.0 * d_match.d_total ) );
		return g_temp;
	}else if( column == 0 )
	{
		g_temp.sprintf( "%s%04d%04d", d_match.d_start->getChain().data(),
			d_match.d_start->getNr(), d_match.d_start->getId() );
		return g_temp;
	}else if( column == 2 )
	{
		return (d_match.d_assigned)?"1":"0";
	}
	return "";
}

void _MatchItemImp::paintSeq(QPainter * p, int width)
{
	const int w = g_textWidth;
	const int ww = width; // Root::Math::max( width, ( d_match.d_fits.size() + 1 ) * w );
	const int h = height();
	int x = 0;

	p->fillRect( 0, 0, ww, h , Qt::lightGray );

	if( d_match.d_start->getPred() && d_match.d_lJoker != -1.0 )
	{
		int dy = h * d_match.d_lJoker;
		p->fillRect( x, h - dy, w, dy , Qt::green );

		d_match.d_start->getPred()->formatLabel( s_buf, sizeof(s_buf) );
		p->setPen( Qt::black );
		p->drawText( x, 0, w, h, Qt::AlignCenter, s_buf );
		if( d_match.d_start->getPred()->getSystem() != 0 )
			p->fillRect( x, 0, 3, 3, Qt::black );
	}
	p->setPen( Qt::white );
	p->drawLine( x + w - 1, 0, x + w - 1, h - 1 );
	x += w;

	Residue* resi = d_match.d_start;
	for( int i = 0; i < d_match.d_fits.size(); i++ )
	{
		assert( resi );
		int dy = h * d_match.d_fits[ i ];
		p->fillRect( x, h - dy, w, dy , Qt::green );

		resi->formatLabel( s_buf, sizeof(s_buf) );
		p->setPen( Qt::black );
		p->drawText( x, 0, w, h, Qt::AlignCenter, s_buf );
		if( resi->getSystem() != 0 )
			p->fillRect( x, 0, 3, 3, Qt::black );

		p->setPen( Qt::white );
		p->drawLine( x + w - 1, 0, x + w - 1, h - 1 );
		x += w;
		resi = resi->getSucc();
	}

	if( resi && d_match.d_rJoker != -1.0 )
	{
		int dy = h * d_match.d_rJoker;
		p->fillRect( x, h - dy, w, dy , Qt::green );

		resi->formatLabel( s_buf, sizeof(s_buf) );
		p->setPen( Qt::black );
		p->drawText( x, 0, w, h, Qt::AlignCenter, s_buf );
		if( resi->getSystem() )
			p->fillRect( x, 0, 3, 3, Qt::black );
		p->setPen( Qt::white );
		p->drawLine( x + w - 1, 0, x + w - 1, h - 1 );
	}

	p->setPen( Qt::white );
	p->drawLine( 0, h - 1, ww - 1, h - 1 );
}

void _MatchItemImp::paintFit(QPainter * p, int width )
{
	const int w = Root::Math::max( width, g_textWidth );
	const int h = height();

	p->fillRect( 0, 0, w, h , Qt::lightGray );
	p->fillRect( 0, 0, w * d_match.d_total, h, Qt::red );
	g_temp.sprintf( "%d%%", int( 100.0 * d_match.d_total ) );
	p->setPen( Qt::black );
	p->drawText( 0, 0, w, h, Qt::AlignCenter, g_temp );
	p->setPen( Qt::white );
	p->drawLine( 0, h - 1, w - 1, h - 1 );
}

void _MatchItemImp::paintAss(QPainter * p, int width )
{
	const int w = Root::Math::max( width, g_textWidth );
	const int h = height();
	const int x = 2 * h / 3;

	// p->fillRect( 0, 0, w, h , Qt::lightGray );
	if( d_match.d_assigned )
		p->fillRect( w / 2 - x / 2, h / 2 - x / 2, x, x , Qt::black );
	p->setPen( Qt::white );
	// p->drawLine( 0, h - 1, w - 1, h - 1 );
}

void _FragViewImp::drawContents(QPainter * p )
{
    QRect cr = contentsRect();
	const int w = g_textWidth;
	const int ww = ( d_frag.size() + 2 ) * w;
	const int h = cr.height();
	int x = 0 + d_off + cr.left();

	p->fillRect( cr.left(), cr.top(), ww, h , Qt::lightGray );

	g_temp.sprintf( "(#%d)", d_frag.front()->getId() );
	p->setPen( Qt::black );
	p->drawText( x, cr.top(), w, h, Qt::AlignCenter, g_temp );

	p->setPen( Qt::white );
	p->drawLine( x + w - 1, cr.top(), x + w - 1, h - 1 );
	x += w;

	for( int i = 0; i < d_frag.size(); i++ )
	{
		g_temp.sprintf( "#%d", d_frag[ i ]->getId() );
		p->setPen( Qt::black );
		p->drawText( x, cr.top(), w, h, Qt::AlignCenter, g_temp );

		p->setPen( Qt::white );
		p->drawLine( x + w - 1, cr.top(), x + w - 1, h - 1 );
		x += w;
	}

	g_temp.sprintf( "(#%d)", d_frag.back()->getId() );
	p->setPen( Qt::black );
	p->drawText( x, cr.top(), w, h, Qt::AlignCenter, g_temp );
	p->setPen( Qt::white );
	p->drawLine( x + w - 1, cr.top(), x + w - 1, h - 1 );
}

void SingleAlignmentView::handleValueChanged(int value)
{
	d_threshold = value;
	load();
}

void SingleAlignmentView::handleAssign()
{
	const SpinSystemString& fra = d_ass->getFragment();
	_MatchItemImp* item = dynamic_cast<_MatchItemImp*>( d_list->selectedItem() );
	if( item == 0 )
		return;

	Root::Ref<AssignSystemCmd> cmd =
		new AssignSystemCmd( d_ass->getBase(), fra[ 0 ], item->d_match.d_start );
	cmd->handle( d_parent );
	handleUpdate();
}

void SingleAlignmentView::handleGeom( bool on )
{
	d_ass->setGeometric( on );
	load();
}

void SingleAlignmentView::handleUpdate()
{
	d_ass->recalc();
	load();
}

void SingleAlignmentView::handleExp(bool on)
{
	d_ass->setExpand( on );
	load();
}

void SingleAlignmentView::handleStrict(bool on)
{
	d_ass->setStrict( on );
	load();
}
