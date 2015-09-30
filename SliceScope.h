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

#if !defined(AFX_SLICERSCOPE_H__E811CCF0_0712_46DC_B3F8_6052B701BE4F__INCLUDED_)
#define AFX_SLICERSCOPE_H__E811CCF0_0712_46DC_B3F8_6052B701BE4F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Lexi/MainWindow.h>
#include <Root/ActionHandler.h>
#include <Lexi/GlyphWidget.h>
#include <Lexi/FocusManager.h>
#include <SliceScopeAgent.h>

namespace Spec
{
	using Root::Action;

	class SliceScope : public Lexi::MainWindow
	{
	public:
		SliceScope(Root::Agent *, Spectrum*, Project*, PeakList*);
	protected:
		void addSpecLane();
		virtual ~SliceScope();
		void handle(Root::Message&);
	private:
		void handleRemoveSpec( Action& );
		void handleAddSpec( Action& );
		FRIEND_ACTION_HANDLER( SliceScope );
		void buildViews();
		void buildMenus();
		void updateCaption();
		Root::Ref<Lexi::FocusManager> d_focus;
		Lexi::GlyphWidget* d_widget;
		Root::Ptr<SliceScopeAgent> d_agent;
		Root::Ref<Lexi::Glyph> d_stripBox;
		Root::Ref<Lexi::Glyph> d_box;
	};
}

#endif // !defined(AFX_SLICERSCOPE_H__E811CCF0_0712_46DC_B3F8_6052B701BE4F__INCLUDED_)
