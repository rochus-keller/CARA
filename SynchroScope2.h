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

#if !defined(AFX_SYNCHROSCOPE2_H__DA76A762_3A1D_4A9B_AB7E_E6738842EB1E__INCLUDED_)
#define AFX_SYNCHROSCOPE2_H__DA76A762_3A1D_4A9B_AB7E_E6738842EB1E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Lexi/MainWindow.h>
#include <Root/ActionHandler.h>
#include <Lexi/GlyphWidget.h>
#include <Lexi/FocusManager.h>
#include <PolyScope2.h>

namespace Spec
{
	using Root::Action;

	class SynchroScope2 : public PolyScope2
	{
	public:
		SynchroScope2(Root::Agent *, Spectrum*, Project*);
	protected:
		virtual ~SynchroScope2();
		void handle(Root::Message&);
		bool askToCloseWindow() const; // Override
	private:
		FRIEND_ACTION_HANDLER( SynchroScope2 );
		void updateCaption();
		void buildMenus();
		void buildViews();
	};
}

#endif // !defined(AFX_SYNCHROSCOPE2_H__DA76A762_3A1D_4A9B_AB7E_E6738842EB1E__INCLUDED_)
