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

#if !defined(AFX_SYSTEMSCOPE_H__F08DD7D7_D68A_468D_AA5E_40695C594891__INCLUDED_)
#define AFX_SYSTEMSCOPE_H__F08DD7D7_D68A_468D_AA5E_40695C594891__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Lexi/MainWindow.h>
#include <Root/ActionHandler.h>
#include <Lexi/GlyphWidget.h>
#include <Lexi/FocusManager.h>
#include <Spec/Units.h>
#include <Root/Vector.h>
//Added by qt3to4:
#include <Gui/Menu.h>
#include <Gui/ListView.h>

class Q3ComboBox;

namespace Spec
{
	class SpecViewer;
	class SystemScopeAgent2;
	class Project;
	class Spectrum;
	class SpinSystem;
	class ResidueType;
	class ResidueParamView;
	using Root::Action;

	class SystemScope2 : public Lexi::MainWindow
	{ 
	public:
		static Action::CmdStr StripOrder;
		static Action::CmdStr ShowSpinPath;
		static Action::CmdStr ShowStrip;
		static Action::CmdStr CreateSpin;
		static Action::CmdStr DeleteSpin;
		static Action::CmdStr ForceLabelSpin;
		static Action::CmdStr LabelSpin;
		static Action::CmdStr MoveSpin;
		static Action::CmdStr MoveSpinAlias;
		static Action::CmdStr EditSpinAtts;
		static Action::CmdStr EditSysAtts;
		static Action::CmdStr AcceptLabel;
		static Action::CmdStr GotoSystem;
		static Action::CmdStr GotoResidue;
		static Action::CmdStr SetWidthFactor;
		static Action::CmdStr ShowRulers;
		static Action::CmdStr SetTol;
		static Action::CmdStr SetTolOrtho;

		SystemScope2(Root::Agent*,Project*,Spectrum* );
	protected:
		void adjustTol(Dimension);
		void curSystemSet();
		void selectAnchor();
		void buildPopups();
		virtual ~SystemScope2();
		void handle(Root::Message&);
	private:
		void handleSetTol( Action& );
		void handleSetTolOrtho( Action& );
		void handleShowRulers( Action& );
		void handleSetWidthFactor( Action& );
		void handleGotoResidue( Action& );
		void handleGotoSystem( Action& );
		void handleAcceptLabel( Action& );
		void handleEditSpinAtts( Action& );
		void handleEditSysAtts( Action& );
		void handleShowDepth( Action& );
		void handleMoveSpinAlias( Action& );
		void handleMoveSpin( Action& );
		void handleLabelSpin( Action& );
		void handleForceLabelSpin( Action& );
		void handleDeleteSpin( Action& );
		void handleCreateSpin( Action& );
		void handleShowSpinPath( Action& );
		void handleShowStrip( Action& );
		void handleShowOrthogonal( Action& );
		void handleStripOrder( Action& );
		FRIEND_ACTION_HANDLER( SystemScope2 );
		void updatePopup();
		void reloadAnchors();
		void reloadSpins();
		void loadSetup();
		void reloadSystemSelector();
		void buildMenus();
		void buildViews();
		void updateCaption();

		Root::Ref<Lexi::FocusManager> d_focus;
		Lexi::GlyphWidget* d_widget;
		Root::Ptr<SystemScopeAgent2> d_agent;
		Gui::ListView* d_spins;
		Gui::ListView* d_bonds;
		Q3ComboBox* d_sysSelector;
		Q3ComboBox* d_typeSelector;
		Root::Ref<SpinSystem> d_curSystem;
		Root::Ref<ResidueType> d_curType;
		Root::Ref<Lexi::Glyph> d_orthoBox;
		Root::Ref<ResidueParamView> d_resiView;
		bool d_stripOrder;
	};

	// Diese Klasse ist privat, nur hier wegen moc
	class _Redirector2 : public QObject
	{
		Q_OBJECT
		Root::Ptr<SystemScope2> d_parent;
	public:
		_Redirector2( SystemScope2* parent );

		struct SelectSystem : public Root::UserMessage
		{
			int d_sel;
			SelectSystem( int s ):d_sel( s ) {}
		};
		struct SelectType : public Root::UserMessage
		{
			int d_sel;
			SelectType( int s ):d_sel( s ) {}
		};
		struct SelectAnchor : public Root::UserMessage
		{
		};
	public slots:
		void handleSelectSys(int i);
		void handleSelectType(int i);
		void doubleClicked( Gui::ListViewItem * i );
	};

}

#endif // !defined(AFX_SYSTEMSCOPE_H__F08DD7D7_D68A_468D_AA5E_40695C594891__INCLUDED_)
