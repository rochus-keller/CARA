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

#if !defined(AFX_AIDACENTRALAGENT_H__7F8C2721_4034_4606_B6F4_B81B172C6412__INCLUDED_)
#define AFX_AIDACENTRALAGENT_H__7F8C2721_4034_4606_B6F4_B81B172C6412__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <QSplitter>
#include <Gui/ListView.h>
#include <QPointer>
#include <Gui/Menu.h>
#include <Root/ActionHandler.h>

class AidaCentral;
class QHBoxLayout;
class AidaCentralAgentTreeItem;

using Root::Action;

namespace Spec
{
	class EasyProtonList;
	class Project;
}

class AidaCentralAgent : public QSplitter, public Root::Messenger
{
	Q_OBJECT
public:
	static Root::Action::CmdStr NewProject;
	static Root::Action::CmdStr ImportSeq;
	static Root::Action::CmdStr ImportAtomList;
	static Root::Action::CmdStr ImportSeqVals;
	static Root::Action::CmdStr ExportMapper;
	static Root::Action::CmdStr ExportSequence;
	static Root::Action::CmdStr ExportAtomList;
	static Root::Action::CmdStr RemoveProject;
	static Root::Action::CmdStr RenameProject;
	static Root::Action::CmdStr DuplicatePro;
	static Root::Action::CmdStr ImportProject;
	static Root::Action::CmdStr SetOrigin;

	AidaCentral* getParent() const { return d_parent; }
	QMenu* getPopup(const QByteArray& = QByteArray() );
	void reloadProjects();
	void initialize();
	AidaCentralAgent(AidaCentral*);
	virtual ~AidaCentralAgent();
protected:
	GENERIC_MESSENGER(QWidget)
	void handle( Root::Message& );
	void createProject( const char* name, bool loadSeq );
	void importAtomList(Spec::Project*, Spec::EasyProtonList* pl, bool);
	void setPane( QWidget* );
	void clearPane();
	QWidget* createView( int type, const char* = "" );
	Gui::Menu* createMenu( int type );
	bool isPersistent( int type ) const;
protected slots:
	void buildPopups();
	void onCurrentItemChanged();
private:
	void handleSetOrigin( Action& );
	void handleImportProject( Action& );
	void handleDuplicatePro( Action& );
	void handleRenameProject( Action& );
	void handleImportAtomList( Action& );
	void handleRemoveProject( Action& );
	void handleExportMapper( Action& );
	void handleExportSequence( Action& );
	void handleExportAtomList( Action& );
	void handleImportSeqVals( Action& );
	void handleImportSeq( Action& );
	void handleNewProject(Root::Action&);
	FRIEND_ACTION_HANDLER( AidaCentralAgent );
private:
	Gui::ListView* d_menuTree;
	QHBoxLayout* d_hbox;
	QWidget* d_pane;
	AidaCentralAgentTreeItem* d_pi;
	AidaCentralAgentTreeItem* d_cur;
	Root::Ptr<AidaCentral> d_parent;
	QMenu* d_popup;
	QMap<int,QPointer<Gui::Menu> > d_pops; // Pointer, da via Lua löschbar
};

#endif // !defined(AFX_AIDACENTRALAGENT_H__7F8C2721_4034_4606_B6F4_B81B172C6412__INCLUDED_)
