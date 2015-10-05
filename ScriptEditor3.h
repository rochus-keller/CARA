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

#if !defined(ScriptEditor3__INCLUDED_)
#define ScriptEditor3__INCLUDED_

#include <Script/CodeEditor.h>
#include <Spec/Repository.h>
#include <Script/Engine.h>

class ScriptEditor3 : public Lua::CodeEditor, Root::Messenger
{
	Q_OBJECT
public:
	ScriptEditor3(QWidget * parent,Lua::Engine*);
	void installDefaultPopup();
    void updateBreakpoints();
signals:
	void sigCompiled( QByteArray getName );
protected:
    void find( bool fromTop );
	virtual ~ScriptEditor3();
    void numberAreaDoubleClicked( int line );
protected slots:
	void updateCursor();
public slots:
    void handleShowLinenumbers();
    void handleRemoveAllBreaks();
    void handleBreakpoint();
    void handleAbort();
    void handleSingleStep();
    void handleContinue();
	//void handleSelectBrace();
    void handleSetFont();
    void handleExecute();
    void handleCheck();
    void handleSetDebug();
    void handleBreakAtFirst();
private:
	Root::Ref<Lua::Engine> d_lua;
};

#endif // !defined(ScriptEditor3__INCLUDED_)
