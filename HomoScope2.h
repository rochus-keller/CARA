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

#if !defined(AFX_HOMOSCOPE_H__BEF0EC37_D637_41A1_A04E_F72AC9EAEA32__INCLUDED_)
#define AFX_HOMOSCOPE_H__BEF0EC37_D637_41A1_A04E_F72AC9EAEA32__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <PolyScope2.h>

namespace Spec
{
	class HomoScope2 : public PolyScope2
	{
	public:
		static Action::CmdStr ShowList;

		HomoScope2(Root::Agent *, Spectrum*, Project*);
	protected:
		virtual ~HomoScope2();
		void handle(Root::Message&);
		bool askToCloseWindow() const; // Override
	private:
		void handleShowList( Action& );
		FRIEND_ACTION_HANDLER( HomoScope2 );
		void updateCaption();
		void buildMenus();
		void buildViews( bool reuse = false);
	};
}

#endif // !defined(AFX_HOMOSCOPE_H__BEF0EC37_D637_41A1_A04E_F72AC9EAEA32__INCLUDED_)
