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

#ifndef _Cara_ScriptView_
#define _Cara_ScriptView_

#include <QSplitter>
#include <Script/Engine.h>
#include <Spec/Repository.h>
#include <Root/ActionHandler.h>
#include <Gui/Menu.h>
#include <Gui/ListView.h>

class QTabWidget;
class QToolButton;
class ScriptEditor3;

class ScriptView : public QSplitter, public Root::Messenger
{
	Q_OBJECT
public:
	ScriptView( QWidget* parent, Root::Agent*, Lua::Engine*, Spec::Repository* );
	~ScriptView();
    Gui::ListView* getList() const { return d_scriptList; }
    static Gui::Menu* createPopup();
protected slots:
	void onItemDoubleClicked( Gui::ListViewItem * item ); 
	void onClose();
	void onCurrentTabChanged(int);
	void onTextChanged();
private:
	void handleRunFromFile( Root::Action& );
	void handleExportBinary( Root::Action& );
	void handleDuplicate( Root::Action& );
	void handleExportScript( Root::Action& );
	void handleImportScript( Root::Action& );
	void handleOpenScript( Root::Action& );
	void handleEditScript( Root::Action& );
	void handlEditAtts( Root::Action& );
	void handleRunScript( Root::Action& );
	void handleCheckScript( Root::Action& );
	void handleRenameScript( Root::Action& );
	void handleDeleteScript( Root::Action& );
	void handleCreateScript( Root::Action& );
    void handleShowFile( Root::Action& );
    FRIEND_ACTION_HANDLER( ScriptView );
protected:
    GENERIC_MESSENGER( QSplitter )
    void handle( Root::Message& );
    ScriptEditor3* addEditor( Spec::Script* );
    ScriptEditor3* createViewFromFile( const QByteArray& name, bool meetLineNumbers );
    int tabIndexOf( QObject* );
    ScriptEditor3* findEditorInTab( const QString& name );
    ScriptEditor3* findEditorByName( const QByteArray& name, bool open );
    void doDebugHit(const QByteArray& script, int line );
    void doActiveLevel( const QByteArray& script, int line );
private:
	Root::ExRef<Lua::Engine> d_lua;
	Root::ExRef<Spec::Repository> d_rep;
	Root::Ptr<Root::Agent> d_parent;
	Gui::ListView* d_scriptList;
	QTabWidget* d_tab;
	QToolButton* d_closer;
};

#endif
