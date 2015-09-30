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

#if !defined(AFX_MONOSCOPE2_H__C8350055_D646_4DA6_88AE_9D03BC517E1F__INCLUDED_)
#define AFX_MONOSCOPE2_H__C8350055_D646_4DA6_88AE_9D03BC517E1F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Lexi/MainWindow.h>
#include <Root/ActionHandler.h>
#include <Lexi/GlyphWidget.h>
#include <Lexi/FocusManager.h>
#include <PhaserAgent.h>

namespace Spec
{

	class Phaser : public Lexi::MainWindow
	{
	public:
		static Root::Action::CmdStr LoadImagX;
		static Root::Action::CmdStr LoadImagY;
		static Root::Action::CmdStr LoadImagZ;

		Phaser(Root::Agent *, Spectrum* real);
	protected:
		void openImag( Dimension );
		void init();
		virtual ~Phaser();
		void handleAction(Root::Action&);
		void handle(Root::Message&);
	private:
		void handleLoadImagZ( Action& );
		void handleLoadImagY( Action& );
		void handleLoadImagX( Action& );
		FRIEND_ACTION_HANDLER( Phaser );
		void updateCaption();
		void buildMenus();
		void buildViews(bool rebuild = true);

		Root::Ref<Lexi::FocusManager> d_focus;
		Lexi::GlyphWidget* d_widget;
		Root::Ptr<PhaserAgent> d_agent;
	};
}

#endif // !defined(AFX_MONOSCOPE2_H__C8350055_D646_4DA6_88AE_9D03BC517E1F__INCLUDED_)
