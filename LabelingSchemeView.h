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

#if !defined(_Cara_LabelingSchemeView)
#define _Cara_LabelingSchemeView

#include <Gui/ListView.h>
#include <Gui/Menu.h>
#include <Spec/Repository.h>
#include <Root/ActionHandler.h>

class LabelingSchemeView : public Gui::ListView, Root::Messenger
{
public:
	static Root::Action::CmdStr Add;
	static Root::Action::CmdStr Rename;
	static Root::Action::CmdStr Remove;
	static Root::Action::CmdStr EditAtts;
	static Root::Action::CmdStr ShowTable;

	LabelingSchemeView(QWidget*,Spec::Repository*);
	virtual ~LabelingSchemeView();
    static Gui::Menu *createPopup();
protected:
	GENERIC_MESSENGER(Gui::ListView)
	void handle( Root::Message& );
	void refill();
private:
	void handleShowTable( Root::Action& );
	void handleEditAtts( Root::Action& );
	void handleRename( Root::Action& );
	void handleRemove( Root::Action& );
	void handleNew( Root::Action& );
	FRIEND_ACTION_HANDLER( LabelingSchemeView );
	Root::ExRef<Spec::Repository> d_rep;
};

#endif // !defined(AFX_SYSTEMTYPEVIEW_H__E2F7F16D_F980_49A8_B18D_CBE34EECF11F__INCLUDED_)
