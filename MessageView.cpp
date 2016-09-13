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

#include "MessageView.h"
#include <QTreeView>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog> 
#include <QTextEdit>
#include <Root/Any.h>
#include <Root/MessageLog.h>
#include <Root/Application.h>
#include <stdio.h>
#include <QFile>
#include <Root/Vector.h>
using namespace Root;

//static char s_buf[16];

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr MessageView::Clear = "Clear";
Root::Action::CmdStr MessageView::SaveTo = "SaveTo";

ACTION_SLOTS_BEGIN( MessageView )
    { MessageView::Clear, &MessageView::handleClear },
    { MessageView::SaveTo, &MessageView::handleSaveTo },
ACTION_SLOTS_END( MessageView )

//////////////////////////////////////////////////////////////////////

class MessageViewMdl : public QAbstractItemModel, Root::Messenger
{
public:
	MessageViewMdl( QObject* p ):QAbstractItemModel(p)
	{
		MessageLog::inst()->addObserver( this );
	}
	~MessageViewMdl()
	{
		MessageLog::inst()->removeObserver( this );
	}
    void handle( Message& msg )
	{
		BEGIN_HANDLER();
        MESSAGE( Root::MessageLog::Update, a, msg )
		{
			switch( a->getType() )
			{
			case Root::MessageLog::Update::All:
				reset();
				break;
			case Root::MessageLog::Update::Add:
				beginInsertRows( QModelIndex(), a->getNr(), a->getNr() );
				endInsertRows();
				break;
			}
		}
		END_HANDLER();
	}
	int columnCount( const QModelIndex & parent ) const 
	{ 
		return 4;
	}
	QVariant headerData( int section, Qt::Orientation orientation, int role) const 
	{
		if( role == Qt::DisplayRole )
		{
			switch(section)
			{
			case 0:
				return tr("Nr.");
			case 1:
				return tr("Kind");
			case 2:
				return tr("Source");
            case 3:
                return tr("Message");
			}
		}
		return QVariant();
	}
	QVariant data( const QModelIndex & index, int role) const
	{
		if( role == Qt::DisplayRole )
		{
			const MessageLog::Entry& e = MessageLog::inst()->getEntry( index.row() );
			switch( index.column() )
			{
			case 0:
				return index.row();
			case 1:
				return MessageLog::s_pretty[ e.d_kind ];
			case 2:
				return e.d_src.data();
            case 3:
				return e.d_msg.simplified();
			}
		}
		return QVariant();
	}
	QModelIndex index ( int row, int column, const QModelIndex & parent ) const
	{
		return createIndex( row, column, row ); 
	}
	QModelIndex parent(const QModelIndex &) const { return QModelIndex(); }
	int rowCount( const QModelIndex & parent ) const
	{
		if(	!parent.isValid() )
			return MessageLog::inst()->getCount();
		else
			return 0;
	}
	Qt::ItemFlags flags(const QModelIndex &index) const
	{
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}
};

//////////////////////////////////////////////////////////////////////

MessageView::MessageView(QWidget* p):
	QSplitter( Qt::Vertical, p )
{
	d_list = new QTreeView( this );
	d_list->setAllColumnsShowFocus(true);
	d_list->setRootIsDecorated(false);
	d_list->setModel( new MessageViewMdl( d_list ) );
	d_msg = new QTextEdit( this );
	d_msg->setReadOnly( true );

	connect( d_list->selectionModel(), 
		SIGNAL( currentChanged( const QModelIndex &, const QModelIndex & )  ), 
		this, SLOT( handleSelected(const QModelIndex &) ) );

	Gui::Menu* pop = new Gui::Menu( d_list, true );
	Gui::Menu::item( pop, this, "Clear Log", Clear, false );
	Gui::Menu::item( 
	pop, this, "Save Log...", SaveTo, false );

}

MessageView::~MessageView()
{
}

void MessageView::handle(Root::Message & msg)
{
	BEGIN_HANDLER();
	MESSAGE( Root::Action, a, msg )
	{
		EXECUTE_ACTION( MessageView, *a );
	}
	END_HANDLER();
}

void MessageView::handleSelected(const QModelIndex & idx)
{
	if( idx.isValid() )
		d_msg->setText( MessageLog::inst()->getEntry( idx.sibling(idx.row(), 0).data().toInt() ).d_msg );
	else
		d_msg->setText( "" );
}

void MessageView::handleClear(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );

	Root::MessageLog::inst()->clearLog();
}

void MessageView::handleSaveTo(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );

	QString fileName = QFileDialog::getSaveFileName( this, 
			"Save Message Log", Root::AppAgent::getCurrentDir(), 
			"Message Log (*.log)" );
	if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );

	if( info.extension( false ).upper() != "LOG" )
		fileName += ".log";
	info.setFile( fileName );
	if( info.exists() )
	{
		if( QMessageBox::warning( this, "Save As",
			"This file already exists. Do you want to overwrite it?",
			"&OK", "&Cancel" ) != 0 )
			return;
	}
	Root::AppAgent::setCurrentDir( info.dirPath( true ) );

    QFile out( fileName );
    if( !out.open( QFile::Append ) )
	{
		QMessageBox::critical( this, "Save Message Log", 
			"Cannot write to selected file!", "&Cancel" );
		return;
	}
	Root::MessageLog::inst()->saveToStream( &out );
}

