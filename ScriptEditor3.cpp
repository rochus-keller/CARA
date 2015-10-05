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

#include "ScriptEditor3.h"
#include <bitset>
#include <QInputDialog> 
#include <QMessageBox>
#include <QFileDialog>
#include <QPrintDialog>
#include <QFileInfo> 
#include <QPrinter>
#include <QDir> 
#include <QFontDialog>
#include <QClipboard> 
#include <QSettings>
#include <QApplication>
#include <Gui2/AutoMenu.h>
#include <Root/Application.h>
#include <Script/SyntaxHighlighter.h>
using namespace Spec;
using namespace Lua;

static const int s_lineMark = 1;
static const int s_breakMark = 2;

//////////////////////////////////////////////////////////////////////

ScriptEditor3::ScriptEditor3(QWidget * parent, Lua::Engine* e):
    Lua::CodeEditor( parent ), d_lua( e )
{
    assert( e );

	QSettings set;
    const bool showLineNumbers = set.value( "ScriptEditor3/ShowLineNumbers" ).toBool();
    setShowNumbers( showLineNumbers );
    updateBreakpoints();
    setFont( set.value( "ScriptEditor3/Font", QVariant::fromValue( font() ) ).value<QFont>() );
    connect( this, SIGNAL( cursorPositionChanged() ), this, SLOT(  updateCursor() ) );
}

ScriptEditor3::~ScriptEditor3()
{
}

void ScriptEditor3::numberAreaDoubleClicked(int line)
{
    const QByteArray n = getName().toLatin1();
    if( d_lua->getBreaks( n ).contains( line + 1 ) )
    {
        // Remove
        d_lua->removeBreak( n, line + 1 );
        removeBreakPoint( line );
    }else
    {
        // Add
        d_lua->addBreak( n, line + 1 );
        addBreakPoint( line );
    }
}

void ScriptEditor3::installDefaultPopup()
{
    Gui2::AutoMenu* pop = new Gui2::AutoMenu( this, true );
    pop->addCommand( "Check Syntax", this, SLOT(handleCheck()) );
    pop->addCommand( "&Execute",this, SLOT(handleExecute()), tr("CTRL+E"), true );
    pop->addCommand( "Continue", this, SLOT( handleContinue() ), tr("F5"), true );
    pop->addCommand( "Step", this, SLOT( handleSingleStep() ), tr("F11"), true );
    pop->addCommand( "Terminate", this, SLOT( handleAbort() ) );
    pop->addCommand( "Debugging", this, SLOT(handleSetDebug() ) );
    pop->addCommand( "Break at first line", this, SLOT( handleBreakAtFirst() ) );
    pop->addCommand( "Toggle Breakpoint", this, SLOT(handleBreakpoint() ), tr("F9"), true );
    pop->addSeparator();
    pop->addCommand( "Undo", this, SLOT(handleEditUndo()), tr("CTRL+Z"), true );
    pop->addCommand( "Redo", this, SLOT(handleEditRedo()), tr("CTRL+Y"), true );
 	pop->addSeparator();
    pop->addCommand( "Cut", this, SLOT(handleEditCut()), tr("CTRL+X"), true );
    pop->addCommand( "Copy", this, SLOT(handleEditCopy()), tr("CTRL+C"), true );
    pop->addCommand( "Paste", this, SLOT(handleEditPaste()), tr("CTRL+V"), true );
    pop->addCommand( "Select all", this, SLOT(handleEditSelectAll()), tr("CTRL+A"), true  );
	//pop->addCommand( "Select Matching Brace", this, SLOT(handleSelectBrace()) );
	pop->insertSeparator();
    pop->addCommand( "Find...", this, SLOT(handleFind()), tr("CTRL+F"), true );
    pop->addCommand( "Find again", this, SLOT(handleFindAgain()), tr("F3"), true );
    pop->addCommand( "Replace...", this, SLOT(handleReplace()), tr("CTRL+R"), true );
    pop->addCommand( "&Goto...", this, SLOT(handleGoto()), tr("CTRL+G"), true );
    pop->addCommand( "Show &Linenumbers", this, SLOT(handleShowLinenumbers()) );
	pop->insertSeparator();
    pop->addCommand( "Indent", this, SLOT(handleIndent()) );
    pop->addCommand( "Unindent", this, SLOT(handleUnindent()) );
    pop->addCommand( "Set Indentation Level...", this, SLOT(handleSetIndent()) );
	pop->addSeparator();
    pop->addCommand( "Print...", this, SLOT(handlePrint()) );
    pop->addCommand( "Export PDF...", this, SLOT(handleExportPdf()) );
	pop->addSeparator();
    pop->addCommand( "Set &Font...", this, SLOT(handleSetFont()) );
}

void ScriptEditor3::updateBreakpoints()
{
    clearBreakPoints();
    const Engine::Breaks& b = d_lua->getBreaks( getName().toLatin1() );
    foreach( quint32 l, b )
        addBreakPoint( l - 1 );
}

void ScriptEditor3::updateCursor()
{
	int line, col;
	getCursorPosition( &line, &col );
    Root::ReportStatus msg( QString("Line: %1  Column: %2  %3").
                            arg(line + 1).arg(col + 1).
                            arg( SyntaxHighlighter::format( getTokenTypeAtCursor() ) ).toLatin1() );
    msg.sendToQt();
}

void ScriptEditor3::handleCheck()
{
    ENABLED_IF( true );
    if( d_lua->pushFunction( text().toLatin1(), getName().toLatin1() ) )
    {
        d_lua->pop();
        QMessageBox::information( this, tr("Checking Syntax"), tr("No errors found!") );
    }else
        QMessageBox::critical( this, tr("Checking Syntax"), d_lua->getLastError() );
}

void ScriptEditor3::handleExecute()
{
    ENABLED_IF( !d_lua->isExecuting() );
	if( d_lua->executeCmd( text().toLatin1(), ( getName().isEmpty() ) ? QByteArray("#Editor") : getName().toLatin1() ) )
        emit sigCompiled( getName().toLatin1() );
}

void ScriptEditor3::handleSetFont()
{
    ENABLED_IF( true );

	bool ok;
    QFont res = QFontDialog::getFont( &ok, font(), this );
	if( !ok )
		return;
	QSettings set;
    set.setValue( "ScriptEditor3/Font", QVariant::fromValue(res) );
    set.sync();
    setFont( res );
}

void ScriptEditor3::handleContinue()
{
    ENABLED_IF( d_lua->isDebug() && d_lua->isExecuting() );

    d_lua->runToBreakPoint();
    if( false ) // !d_lua->isExecuting() )
	{
        if( d_lua->executeCmd( text().toLatin1(), getName().toLatin1() ) )
        {
            emit sigCompiled( getName().toLatin1() );
        }else
            return;
	}
}

void ScriptEditor3::handleSingleStep()
{
    ENABLED_IF( d_lua->isDebug() && d_lua->isExecuting() );

    d_lua->runToNextLine();
    if( false ) // !d_lua->isExecuting() )
    {
        if( d_lua->executeCmd( text().toLatin1(), getName().toLatin1() ) )
        {
            emit sigCompiled( getName().toLatin1() );
        }else
            return;
    }
}

void ScriptEditor3::handleAbort()
{
    ENABLED_IF( d_lua->isExecuting() && d_lua->isDebug() );

    d_lua->terminate();
}

void ScriptEditor3::handleBreakpoint()
{
    ENABLED_IF( !getName().isEmpty() );

    int line;
    getCursorPosition( &line );
    if( line == -1 )
        return;
    numberAreaDoubleClicked(line);
}

void ScriptEditor3::handleRemoveAllBreaks()
{
    const QSet<int>& bps = getBreakPoints();
    ENABLED_IF( !bps.isEmpty() );
    foreach( int l, bps )
    {
        d_lua->removeBreak( getName().toLatin1(), l );
        removeBreakPoint( l );
    }
}

void ScriptEditor3::handleSetDebug()
{
    CHECKED_IF( !d_lua->isExecuting(), d_lua->isDebug() );
    d_lua->setDebug( !d_lua->isDebug() );
}

void ScriptEditor3::handleBreakAtFirst()
{
    CHECKED_IF( true, d_lua->getDefaultCmd() == Engine::RunToNextLine );
    if( d_lua->getDefaultCmd() == Engine::RunToNextLine )
        d_lua->setDefaultCmd( Engine::RunToBreakPoint );
    else
        d_lua->setDefaultCmd( Engine::RunToNextLine );
}

void ScriptEditor3::handleShowLinenumbers()
{
    CHECKED_IF( true, showNumbers() );

    const bool showLineNumbers = !showNumbers();

    QSettings set;
    set.setValue( "ScriptEditor3/ShowLineNumbers", showLineNumbers );
    setShowNumbers( showLineNumbers );
}

