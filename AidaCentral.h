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

#if !defined(AFX_AIDACENTRAL_H__8AC4550C_B0F1_4452_8174_B9FC30D337BB__INCLUDED_)
#define AFX_AIDACENTRAL_H__8AC4550C_B0F1_4452_8174_B9FC30D337BB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Lexi/MainWindow.h>
#include <Root/ActionHandler.h>
#include <AidaApplication.h>

using Lexi::MainWindow;
using Root::Action;
class AidaCentralAgent;
class QMenu;
class QDockWidget;

class AidaCentral : public MainWindow
{
public:
	AidaCentral( Root::Agent*, AidaApplication *);
	Spec::Repository* getRep() const { return d_app->getRep(); }
	void openSpectrum( const char* path );
	virtual ~AidaCentral();
	void setContextMenu( QMenu* );
protected:
	void closeRepository();
	void openRepository( const char* path );
	void handle( Root::Message& );
	void handleAction(Root::Action & msg );
private:
	void handleTextEditor( Action& );
	void handleLuaBox( Action& );
	void handleShowStyle( Action& );
	void handleOpenSitar( Action& );
	void handleXeasyN( Action& );
	void handleEditProperties( Action& );
	void handleEnableGc( Action& );
	void handleGarbageCollect( Action& );
	void handleLabelCross( Action& );
	void handleLabelAngle( Action& );
	void handleLabelOffset( Action& );
	void handleFlatten( Action& );
	void handleExecute( Action& );
	void handleLabelFont( Action& );
	void handleMapThreshold( Action& );
	void handleConvert( Action& );
	void handleConvert2( Action& );
	void handleOpenSpecRot( Action& );
	void handleOpenPhaser( Action& );
	void handleShowNeasyPure( Action& );
	void handleShowNeasy( Action& );
	void handleNewFromTemplate( Action& );
	void handleFileSaveAs( Action& a );
	void handleFileSave( Action& a );
	void handleFileNew( Action& a );
	void handleFileOpen( Action& a );
	void handleFileQuit( Action& a );
	void handleOpenSpectrum( Action& a );
	void handleTest( Action& a );
	void handleHelpAbout( Action& a );
    void handleSetupPeakColors( Action& a );
	FRIEND_ACTION_HANDLER( AidaCentral );
	void buildMenus();
	bool askToCloseWindow() const;	// Override
	bool askToCloseProject() const;	
	bool handleSave( bool as = false ) const; // true: success

	void updateTitle();

	AidaCentralAgent* d_agent;
	AidaApplication* d_app; // Dies ist Parent/Owner, darum kein Ref
	QMenu* d_help;
	QMenu* d_context;
	QDockWidget* d_term;
    QDockWidget* d_stack;
    QDockWidget* d_locals;
};

#endif // !defined(AFX_AIDACENTRAL_H__8AC4550C_B0F1_4452_8174_B9FC30D337BB__INCLUDED_)
