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

#include "ScriptView.h"
#include <QListWidget>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QToolButton>
#include <Root/Application.h>
#include <Gui/Menu.h>
#include <SpecView/DynValueEditor.h>
#include <fstream>
#include "ScriptEditor3.h"
using namespace Spec;

ACTION_SLOTS_BEGIN( ScriptView )
{ "ExportBinary", &ScriptView::handleExportBinary },
{ "RunFromFile", &ScriptView::handleRunFromFile },
{ "Duplicate", &ScriptView::handleDuplicate },
{ "ImportScript", &ScriptView::handleImportScript },
{ "ExportScript", &ScriptView::handleExportScript },
{ "ShowScript", &ScriptView::handleOpenScript },
{ "EditScript", &ScriptView::handleEditScript },
{ "EditAtts", &ScriptView::handlEditAtts },
{ "CreateScript", &ScriptView::handleCreateScript },
{ "DeleteScript", &ScriptView::handleDeleteScript },
{ "RenameScript", &ScriptView::handleRenameScript },
{ "CheckScript", &ScriptView::handleCheckScript },
{ "RunScript", &ScriptView::handleRunScript },
{ "ShowFile", &ScriptView::handleShowFile },
ACTION_SLOTS_END( ScriptView )

static const int s_fixed = 1;

struct _ScriptViewItem : public Gui::ListViewItem
{
	_ScriptViewItem( Gui::ListView* p, Spec::Script* s ):ListViewItem( p ),d_script(s),d_edit(0) {}
	Root::Ref<Spec::Script> d_script;
    ScriptEditor3* d_edit;
	QString text( int f ) const 
	{ 
		if( f == 0 )
			return d_script->getName().data();
		return QString();
	}
};

static _ScriptViewItem* _findScript( Gui::ListView* list, Script* script )
{
    for( int i = 0; i < list->count(); i++ )
    {
        _ScriptViewItem* item = static_cast<_ScriptViewItem*>( list->child( i ) );
        if( item->d_script == script )
            return item;
    }
    return 0;
}

ScriptView::ScriptView(QWidget* p, Root::Agent* a, Lua::Engine* e, Spec::Repository* r):
	QSplitter( p ), d_lua( e ), d_rep( r ), d_parent( a )
{
	d_rep->addObserver( this );
    d_lua->addObserver( this );
    d_scriptList = new Gui::ListView( this );
	d_scriptList->addColumn( "Scripts" );
	d_scriptList->setMinimumHeight( 200 );
	d_scriptList->setShowSortIndicator( true );
	d_scriptList->setRootIsDecorated( false );
	Repository::ScriptMap::const_iterator p1;
	for( p1 = d_rep->getScripts().begin(); p1 != d_rep->getScripts().end(); ++p1 )
		new _ScriptViewItem( d_scriptList, (*p1).second );
	d_scriptList->setSorting( 0 );
	connect( d_scriptList, SIGNAL(  doubleClicked( Gui::ListViewItem * ) ), 
		this, SLOT( onItemDoubleClicked( Gui::ListViewItem * ) ) );
	connect( d_scriptList, SIGNAL(  returnPressed( Gui::ListViewItem * ) ), 
		this, SLOT( onItemDoubleClicked( Gui::ListViewItem * ) ) );
    addWidget( d_scriptList );

	d_tab = new QTabWidget( this );
	d_tab->setIconSize( QSize(10,10) );
	d_tab->setElideMode( Qt::ElideRight );
	connect( d_tab, SIGNAL( currentChanged ( int ) ), this, SLOT( onCurrentTabChanged(int) ) );

	d_closer = new QToolButton( d_tab );
	d_closer->setEnabled(false);
	d_closer->setPixmap( QPixmap( ":/cara/images/close.png" ) );
	d_closer->setAutoRaise( true );
	connect( d_closer, SIGNAL( clicked() ), this, SLOT( onClose() ) );
	d_tab->setCornerWidget( d_closer );
    addWidget( d_tab );

	addEditor( 0 );
    ScriptEditor3* edit = static_cast<ScriptEditor3*>( d_tab->widget(0) );
	edit->setReadOnly( false );
	d_tab->setTabIcon( 0, QIcon(":/cara/images/pen.png") );
    setStretchFactor( 0, 3 );
    setStretchFactor( 1, 7 );

}

ScriptView::~ScriptView()
{
    d_lua->removeObserver( this );
    d_rep->removeObserver( this );
}

Gui::Menu* ScriptView::createPopup()
{
    Gui::Menu* pop = new Gui::Menu();
	Gui::Menu::item( pop, "Show...", "ShowScript", false );
	Gui::Menu::item( pop, "Edit...", "EditScript", false );
	Gui::Menu::item( pop, "Check Syntax...", "CheckScript", false );
	Gui::Menu::item( pop, "Execute...", "RunScript", false );
	pop->insertSeparator();
	Gui::Menu::item( pop, "New...", "CreateScript", false );
	Gui::Menu::item( pop, "Duplicate...", "Duplicate", false );
	Gui::Menu::item( pop, "Rename...", "RenameScript", false );
	Gui::Menu::item( pop, "Delete...", "DeleteScript", false );
	Gui::Menu::item( pop, "Edit Attributes...", "EditAtts", false );
	pop->insertSeparator();
	Gui::Menu::item( pop, "Import...", "ImportScript", false );
    Gui::Menu::item( pop, "Show File...", "ShowFile", false );
    Gui::Menu::item( pop, "Run from File...", "RunFromFile", false );
	Gui::Menu::item( pop, "Export...", "ExportScript", false );
	Gui::Menu::item( pop, "Export Binary...", "ExportBinary", false );
    return pop;
}

void ScriptView::onItemDoubleClicked( Gui::ListViewItem * item )
{
	if( item )
	{
		_ScriptViewItem* svi = static_cast<_ScriptViewItem*>( item);
		if( svi->d_edit == 0 )
		{
			svi->d_edit = addEditor( svi->d_script );
		}else
		{
            const int i = tabIndexOf( svi->d_edit );
			assert( i != -1 );
			d_tab->setCurrentIndex( i );
		}
	}
}

void ScriptView::onCurrentTabChanged(int i)
{
	d_closer->setEnabled( i >= s_fixed );
}

void ScriptView::onClose()
{
	if( d_tab->currentIndex() < s_fixed )
		return;
	Script* cur = 0;
    ScriptEditor3* obj = static_cast<ScriptEditor3*>( d_tab->widget( d_tab->currentIndex() ) );
	for( int i = 0; i < d_scriptList->count(); i++ )
	{
		_ScriptViewItem* svi = static_cast<_ScriptViewItem*>( d_scriptList->child( i ) );
		if( svi->d_edit == obj )
		{
			svi->d_edit = 0;
			cur = svi->d_script;
			break;
		}
	}
    if( cur != 0 && obj->document()->isModified() )
		cur->setCode( obj->text().toLatin1() );
	d_tab->removeTab( d_tab->currentIndex() );
}

void ScriptView::handle( Root::Message& msg )
{
	BEGIN_HANDLER();
    MESSAGE( Script::Added, a, msg )
	{
		new _ScriptViewItem( d_scriptList, a->sender() );
	}
    MESSAGE( Script::Removed, a, msg )
	{
		for( int i = 0; i < d_scriptList->count(); i++ )
		{
			_ScriptViewItem* svi = static_cast<_ScriptViewItem*>( d_scriptList->child(i) );
			if( svi->d_script == a->sender() )
			{
				if( svi->d_edit )
				{
                    const int j = tabIndexOf( svi->d_edit );
					if( j != -1 )
					{
						delete d_tab->widget(j);
						d_tab->removeTab(j);
					}
				}
				svi->removeMe();
				break;
			}
		}
	}
    MESSAGE( Script::Changed, a, msg )
	{
        Q_UNUSED(a)
        // TODO: was ist, wenn Script gerade gendert wird?
	}
    MESSAGE( Repository::Changed, a, msg )
	{
		if( a->d_hint == Repository::AboutToSave )
		{
			for( int i = 0; i < d_scriptList->count(); i++ )
			{
				_ScriptViewItem* svi = static_cast<_ScriptViewItem*>( d_scriptList->child(i) );
                if( svi->d_edit && svi->d_edit->document()->isModified() )
				{
					svi->d_script->setCode( svi->d_edit->text().toLatin1() );
                    const int j = tabIndexOf( svi->d_edit );
					assert( j != -1 );
					d_tab->setTabText( j, svi->d_script->getName().data() );
				}
			}
		}
	}
    MESSAGE( Lua::Engine::Update, a, msg )
    {
        switch( a->getType() )
        {
		case Lua::Engine::Print:
		case Lua::Engine::Error:
            {
                Root::ReportStatus msg( a->d_val1 );
                msg.sendTo(d_parent);
            }
            break;
		case Lua::Engine::LineHit:
		case Lua::Engine::BreakHit:
            doDebugHit(a->d_val1, a->d_val2);
            break;
		case Lua::Engine::ActiveLevel:
            doActiveLevel(a->d_val1, a->d_val2);
            break;
        default:
            break;
        }
        //m.consume();
    }
    MESSAGE( Root::Action, a, msg )
	{
		EXECUTE_ACTION( ScriptView, *a );
	}
	END_HANDLER();
}

static _ScriptViewItem* _find( Gui::ListView* l, Script* s )
{
	for( int i = 0; i < l->count(); i++ )
		if( static_cast<_ScriptViewItem*>( l->child(i) )->d_script == s )
		{
			return static_cast<_ScriptViewItem*>( l->child(i) );
		}
	return 0;
}

void ScriptView::handleCreateScript(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );

	bool ok	= false;
	QString res = QInputDialog::getText( "New Script", 
			"Please enter a unique name:", QLineEdit::Normal, "", &ok, this );
	if( !ok )
		return;

	Script* s = d_rep->addScript( res.toLatin1() );
	if( s == 0 )
	{
		QMessageBox::critical( this, "New Script", 
			"The selected name was not unique!", "&Cancel" );
		return;
	}
	d_scriptList->onNotifyInsert();
	_ScriptViewItem* svi = _find( d_scriptList, s );
	assert( svi );
	svi->setCurrent();
    ScriptEditor3* editor = addEditor( s );
    editor->setReadOnly( false );
    editor->setFocus();
    svi->d_edit = editor;
    d_tab->setTabIcon( d_tab->currentIndex(), QIcon(":/cara/images/pen.png") );
}

void ScriptView::handleShowFile(Root::Action & a)
{
    ACTION_ENABLED_IF( a, true );
    QString fileName = QFileDialog::getOpenFileName(this, tr("Show File"), Root::AppAgent::getCurrentDir(),
        "Lua Script (*.lua)");
    if( fileName.isNull() )
        return;

    QFileInfo info( fileName );
    Root::AppAgent::setCurrentDir( info.dirPath( true ) );

    ScriptEditor3* e = findEditorInTab( fileName );
    if( e != 0 )
    {
        d_tab->setCurrentWidget( e );
    }else
    {
        // File öffnen, allenfalls dekompilieren
		createViewFromFile( fileName.toLatin1(), false ); // ohne @, damit Debugger eigene Version hat.
    }
}

ScriptEditor3* ScriptView::addEditor( Spec::Script* s )
{
    ScriptEditor3* e = new ScriptEditor3( d_tab, d_lua );
	e->installDefaultPopup();
	QString title = "<Default>";
	if( s )
    {
        title = QString::fromLatin1(s->getName());
        e->setText( QString::fromLatin1( s->getCode() ) );
        e->setName( QChar(':') + title );
        e->updateBreakpoints();
    }
    e->setFocus();
    e->document()->setModified( false );
    e->setReadOnly( true );
    const int i = d_tab->addTab( e, title );
	d_tab->setCurrentIndex( i );
	connect( e, SIGNAL(textChanged()), this, SLOT( onTextChanged() ) );
    return e;
}

ScriptEditor3 *ScriptView::createViewFromFile(const QByteArray &name, bool meetLineNumbers)
{
    QFileInfo info( QString::fromLatin1( (name.startsWith('@'))?name.mid(1):name ) );
    if( !info.exists() )
        return 0;
    ScriptEditor3* e = new ScriptEditor3( d_tab, d_lua );
    e->installDefaultPopup();
    e->loadFromFile( info.filePath(), meetLineNumbers );
    e->setName( QString::fromLatin1(name) );
    e->updateBreakpoints();
    e->setReadOnly( true );
    const int i = d_tab->addTab( e, info.fileName() );
    d_tab->setCurrentIndex( i );
    e->setFocus();
    return e;
}

void ScriptView::onTextChanged()
{
    const int i = tabIndexOf( sender() );
	if( i != -1 && !d_tab->tabText(i).startsWith( "*" ) )
	{
		d_tab->setTabText( i, "*" + d_tab->tabText(i) );
		Root::UpdateMessage msg( d_rep );
		d_rep->traverse( msg );
	}
}

void ScriptView::handleDeleteScript(Root::Action & a)
{
	ACTION_ENABLED_IF( a, d_scriptList->currentItem() );
	_ScriptViewItem* svi = static_cast<_ScriptViewItem*>( d_scriptList->currentItem() );

	if( QMessageBox::information( this, "Delete Script",
			  "Do you really want to delete this script (cannot be undone)?",
			  "&Delete", "&Cancel" ) != 0 )	
		return;

	d_rep->remove( svi->d_script );
}

int ScriptView::tabIndexOf( QObject* obj )
{
	for( int i = 1; i < d_tab->count(); i++ )
	{
		if( d_tab->widget(i) == obj )
			return i;
	}
    return -1;
}

ScriptEditor3 *ScriptView::findEditorInTab(const QString &name)
{
    for( int i = 1; i < d_tab->count(); i++ )
    {
        ScriptEditor3* e = static_cast<ScriptEditor3*>( d_tab->widget(i) );
		if( e->getName() == name )
            return e;
    }
    return 0;
}

void ScriptView::doDebugHit(const QByteArray& script, int line)
{
    Root::ReportStatus msg;
    ScriptEditor3* curEdit = findEditorByName( script, true );
    if( curEdit )
    {
        curEdit->setPositionMarker( line - 1 );
        msg.setMessage( QString("Break at line %1 of '%2'" ).arg( line).
                arg( d_tab->tabText( d_tab->currentIndex() ) ).toLatin1() );
    }else
    {
        setFocus(); // ansonsten findet sendToQt kein Target
        msg.setMessage( QString("Break at line %1 of unknown script").arg( line).toLatin1() );
    }
    msg.sendTo(d_parent); // bei sendToQt findet man nicht immer ein aktives Objekt

    while( d_lua->isWaiting() )
    {
        QApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents );
        QApplication::flush();
    }
    if( curEdit )
        curEdit->setPositionMarker( -1 );
}

void ScriptView::doActiveLevel(const QByteArray &script, int line)
{
    ScriptEditor3* curEdit = findEditorByName( script, true );
    if( curEdit )
        curEdit->setCursorPosition( line - 1, 0 );
}

ScriptEditor3 *ScriptView::findEditorByName(const QByteArray &name, bool open)
{
    if( name.startsWith( ':' ) )
    {
        // Es handelt sich um ein Repository-Script
        Repository::ScriptMap::const_iterator i = d_rep->getScripts().find( name.mid( 1 ) );
        Q_ASSERT( i != d_rep->getScripts().end() );
        _ScriptViewItem* item = _findScript( d_scriptList, (*i).second );
        Q_ASSERT( item != 0 );
        if( open )
            onItemDoubleClicked( item ); // erzeuge und/oder setTop
        return item->d_edit;
    }else if( name.startsWith( '@' ) )
    {
        // Es handelt sich um ein Script aus einer Datei - Plugin oder Run from File
        ScriptEditor3* e = findEditorInTab( name );
        if( e != 0 )
        {
            if( open )
                d_tab->setCurrentWidget( e );
        }else if( open )
        {
            // File öffnen, allenfalls dekompilieren
			e = createViewFromFile( name, true );
        }
        return e;
	}else if( name.startsWith( '#' ) )
	{
		ScriptEditor3* e = findEditorInTab( name );
		if( e != 0 )
		{
			if( open )
				d_tab->setCurrentWidget( e );
		}else if( open )
		{
			// File öffnen, allenfalls dekompilieren
			e = new ScriptEditor3( d_tab, d_lua );
			e->installDefaultPopup();
			e->updateBreakpoints();
			e->loadFromString( d_lua->getCurBinary() );
			e->setName( QString::fromLatin1(name) );
			e->setReadOnly( true );
			d_tab->setCurrentIndex( d_tab->addTab( e, name ) );
			e->setFocus();
		}
		return e;
	}else if( name.isEmpty() )
    {
        Root::ReportStatus msg( "Error: script has no name" );
        msg.sendTo(d_parent);
    }
    return 0;
}

void ScriptView::handleRenameScript(Root::Action & a)
{
	ACTION_ENABLED_IF( a, d_scriptList->currentItem() );

	_ScriptViewItem* svi = static_cast<_ScriptViewItem*>( d_scriptList->currentItem() );
	bool ok	= false;
	QString res = QInputDialog::getText( "Rename Script", 
		"Please enter a unique name:", QLineEdit::Normal, svi->d_script->getName().data(), &ok, this );
	if( !ok )
		return;
	if( !d_rep->rename( svi->d_script, res ) )
	{
		QMessageBox::critical( this, "Rename Script", 
			"The selected name was not unique!", "&Cancel" );
		return;
	}
    const int i = tabIndexOf( svi->d_edit );
	if( i != -1 )
	{
        svi->d_edit->setName( QChar(':') + res );
        if( svi->d_edit->document()->isModified() )
			res = "*" + res;
		d_tab->setTabText( i, res );
	}
}

void ScriptView::handleCheckScript(Root::Action & a)
{
	ACTION_ENABLED_IF( a, d_scriptList->currentItem() );

	_ScriptViewItem* svi = static_cast<_ScriptViewItem*>( d_scriptList->currentItem() );
    if( d_lua->pushFunction( svi->d_script->getCode(), ":" + svi->d_script->getName() ) )
    {
        d_lua->pop();
        QMessageBox::information( this, tr("Checking Syntax"), tr("No errors found.") );
    }else
        QMessageBox::critical( this, tr("Checking Syntax"), d_lua->getLastError() );
}

void ScriptView::handlEditAtts(Root::Action & a)
{
	ACTION_ENABLED_IF( a, d_scriptList->currentItem() );
	_ScriptViewItem* svi = static_cast<_ScriptViewItem*>( d_scriptList->currentItem() );

	DynValueEditor::edit( this, d_rep->findObjectDef( Repository::keyScript ), svi->d_script );
}
void ScriptView::handleOpenScript(Root::Action & a)
{
	ACTION_ENABLED_IF( a, d_scriptList->currentItem() );

	onItemDoubleClicked( d_scriptList->currentItem() );
}

void ScriptView::handleEditScript(Root::Action & a)
{
	ACTION_ENABLED_IF( a, d_scriptList->currentItem() );

	onItemDoubleClicked( d_scriptList->currentItem() );
    ScriptEditor3* e = static_cast<ScriptEditor3*>( d_tab->currentWidget() );
	if( e )
	{
		e->setReadOnly( false );
		d_tab->setTabIcon( d_tab->currentIndex(), QIcon(":/cara/images/pen.png") );
	}
}

void ScriptView::handleImportScript(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );
	QString fileName = QFileDialog::getOpenFileName( this, tr("Import Lua Script"), 
		Root::AppAgent::getCurrentDir(), "Lua Script (*.lua)" );
	if( fileName.isEmpty() )
		return;

	QFileInfo info( fileName );
	Root::AppAgent::setCurrentDir( info.dirPath( true ) );

	bool ok	= false;
	QString res = QInputDialog::getText( "Import Script", 
			"Please enter a unique name:", QLineEdit::Normal, info.baseName(), &ok, this );
	if( !ok )
		return;

	QFile in( fileName );
	if( !in.open( QIODevice::ReadOnly ) )
	{
		QMessageBox::critical( this, "Open File", 
			"Cannot read from selected file!", "&Cancel" );
		return;
	}
	QByteArray code = in.readAll();
	in.close();
	if( code.length() > 4 && code[ 0 ] == char(0x1b) && code[ 1 ] == 'L' &&
		code[ 2 ] == 'u' && code[ 3 ] == 'a' )
	{
		QMessageBox::critical( this, "Import Script", 
			"Cannot import binary Lua scripts!", "&Cancel" );
		return;
	}
	Script* s = d_rep->addScript( res.toLatin1() );
	if( s == 0 )
	{
		QMessageBox::critical( this, "Import Script", 
			"The selected name was not unique!", "&Cancel" );
		return;
	}
	s->setCode( code );
	d_scriptList->onNotifyInsert();
	_ScriptViewItem* svi = _find( d_scriptList, s );
	assert( svi );
	svi->d_edit = addEditor( s );
	svi->setCurrent();
}

void ScriptView::handleExportScript(Root::Action & a)
{
	ACTION_ENABLED_IF( a, d_scriptList->currentItem() );
	_ScriptViewItem* svi = static_cast<_ScriptViewItem*>( d_scriptList->currentItem() );

	QDir dir( Root::AppAgent::getCurrentDir() );
	QString fileName = dir.absoluteFilePath( svi->d_script->getName().data() );
	fileName = QFileDialog::getSaveFileName( this, tr("Export Lua Script"), fileName, 
        "Lua Script (*.lua)", 0, QFileDialog::DontConfirmOverwrite );
	if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );

	if( info.extension( false ).upper() != "LUA" )
		fileName += ".lua";
	info.setFile( fileName );
	if( info.exists() )
	{
		if( QMessageBox::warning( this, "Save As",
			"This file already exists. Do you want to overwrite it?",
			"&OK", "&Cancel" ) != 0 )
			return;
	}
	Root::AppAgent::setCurrentDir( info.dirPath( true ) );
	std::ofstream out( fileName );
	if( !out )
	{
		QMessageBox::critical( this, "Save As", 
			"Cannot write to selected file!", "&Cancel" );
		return;
	}
	out << svi->d_script->getCode();
}

void ScriptView::handleDuplicate(Root::Action & a)
{
	ACTION_ENABLED_IF( a, d_scriptList->currentItem() );
	_ScriptViewItem* svi = static_cast<_ScriptViewItem*>( d_scriptList->currentItem() );

	bool ok	= false;
	QString res = QInputDialog::getText( "Duplicate Script", 
		"Please enter a unique name:", QLineEdit::Normal, svi->d_script->getName().data(), &ok, this );
	if( !ok )
		return;

	Script* s = d_rep->addScript( res.toLatin1() );
	if( s == 0 )
	{
		QMessageBox::critical( this, "Duplicate Script", 
			"The selected name was not unique!", "&Cancel" );
		return;
	}
	s->setCode( svi->d_script->getCode() );
	d_scriptList->onNotifyInsert();
	svi = _find( d_scriptList, s );
	assert( svi );
	svi->d_edit = addEditor( s );
	svi->setCurrent();
}

void ScriptView::handleExportBinary(Root::Action & a)
{
	ACTION_ENABLED_IF( a, d_scriptList->currentItem() );
	_ScriptViewItem* svi = static_cast<_ScriptViewItem*>( d_scriptList->currentItem() );

	QDir dir( Root::AppAgent::getCurrentDir() );
	QString fileName = dir.absoluteFilePath( svi->d_script->getName().data() );
	fileName = QFileDialog::getSaveFileName( this, tr("Export Lua Binary"), fileName, 
        "Lua Binary (*.lua)", 0, QFileDialog::DontConfirmOverwrite );
	if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );

	if( info.extension( false ).upper() != "LUA" )
		fileName += ".lua";
	info.setFile( fileName );
	if( info.exists() )
	{
		if( QMessageBox::warning( this, "Save As",
			"This file already exists. Do you want to overwrite it?",
			"&OK", "&Cancel" ) != 0 )
			return;
	}

	Root::AppAgent::setCurrentDir( info.dirPath( true ) );
    if( !d_lua->saveBinary( svi->d_script->getCode(), fileName.toLatin1() ) )
        QMessageBox::critical( this, tr("Export Binary Script"), d_lua->getLastError() );
}

void ScriptView::handleRunFromFile(Root::Action & a)
{
    ACTION_ENABLED_IF( a, !d_lua->isExecuting() );
	QString fileName = QFileDialog::getOpenFileName(this, tr("Run From File"), Root::AppAgent::getCurrentDir(), 
		"Lua Script (*.lua)");
	if( fileName.isNull() )
		return;

	QFileInfo info( fileName );
	Root::AppAgent::setCurrentDir( info.dirPath( true ) );

    if( !d_lua->executeFile( fileName.toLatin1() ) )
    {
        if( !d_lua->isSilent() )
            QMessageBox::critical( this, tr("Run From File"), d_lua->getLastError() );
        else
            qWarning() << "ScriptView::handleRunFromFile: silently terminated:" << d_lua->getLastError();
    }
}

void ScriptView::handleRunScript(Root::Action & a)
{
    ACTION_ENABLED_IF( a, d_scriptList->currentItem() && !d_lua->isExecuting() );
    _ScriptViewItem* svi = static_cast<_ScriptViewItem*>( d_scriptList->currentItem() );

    if( d_lua->executeCmd( svi->d_script->getCode(), ":" + svi->d_script->getName() ) )
        svi->d_script->setCompiled();
    else if( !d_lua->isSilent() )
        QMessageBox::critical( this, tr("Run Script"), d_lua->getLastError() );
    else
        qWarning() << "ScriptView::handleRunScript: silently terminated:" << d_lua->getLastError();
}

