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

#if !defined(_Cara_SampleView)
#define _Cara_SampleView


#include <Gui/ListView.h>
#include <Gui/Menu.h>
#include <Spec/Project.h>
#include <Root/ActionHandler.h>


class SampleView : public Gui::ListView, public Root::Messenger
{
public:
	static Root::Action::CmdStr EditAtts;
	static Root::Action::CmdStr ShowTable;
	static Root::Action::CmdStr RemoveRange;
	static Root::Action::CmdStr AddRange;
	static Root::Action::CmdStr RemoveSample;
	static Root::Action::CmdStr AddSample;
	static Root::Action::CmdStr RenameSample;

	void refill();
	SampleView(QWidget*,Spec::Project*);
	virtual ~SampleView();
	static Gui::Menu* createPopup();
protected:
	GENERIC_MESSENGER(Gui::ListView)
	void handle( Root::Message& );
	void onCurrentChanged();
private:
	void handleShowTable( Root::Action& );
	void handleEditAtts( Root::Action& );
	void handleRemoveRange( Root::Action& );
	void handleAddRange( Root::Action& );
	void handleRemoveSample( Root::Action& );
	void handleAddSample( Root::Action& );
	void handleRenameSample( Root::Action& );
	FRIEND_ACTION_HANDLER( SampleView );
	Root::Ref<Spec::Project> d_pro;
};

#endif // !defined(_Cara_SampleView)
