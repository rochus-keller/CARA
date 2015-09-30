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

#if !defined(AFX_SINGLEALIGNMENTVIEW_H__EE0C37E0_9553_11D5_8DB0_00D00918E99C__INCLUDED_)
#define AFX_SINGLEALIGNMENTVIEW_H__EE0C37E0_9553_11D5_8DB0_00D00918E99C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <qwidget.h>
#include <QMenu>
#include <QLabel>
#include <Root/Agent.h>
#include <Spec/FragmentAssignment.h>
#include <Gui/ListView.h>

class QSpinBox;
class QLabel;
class QMenu;
class QAction;
class _FragViewImp;

namespace Spec
{
	class SingleAlignmentView : public QWidget
	{
		Q_OBJECT
	public:
		SingleAlignmentView(Root::Agent *, FragmentAssignment* );
		virtual ~SingleAlignmentView();
		void handleMove( int x );
	protected slots:
		void handleStrict( bool );
		void handleExp( bool );
		void handleUpdate();
		void handleAssign();
		void handleValueChanged( int value );
		void handleGeom(bool on);
	private:
		int d_threshold;
		void load();
		Gui::ListView* d_list;
		QSpinBox* d_edit;
		Root::ExRef<FragmentAssignment> d_ass;
		Root::Ptr<Root::Agent> d_parent;
		QLabel* d_label;
		_FragViewImp* d_header;
		QAction* d_geom;
		QAction* d_exp;
		QAction* d_strict;
	};
}

#endif // !defined(AFX_SINGLEALIGNMENTVIEW_H__EE0C37E0_9553_11D5_8DB0_00D00918E99C__INCLUDED_)
