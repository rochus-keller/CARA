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
#include <SpecView/OverviewCtrl.h>
#include <PeakListGadget.h>
#include <MonoScopeAgent.h>

namespace Spec
{
	class SpecViewer;
	using Root::Action;
	class PeakListGadget;
	class OverviewCtrl;

	class MonoScope : public Lexi::MainWindow
	{
	public:
		static Root::Action::CmdStr ShowSlices;
		static Root::Action::CmdStr ShowList;

		static Root::Action::CmdStr NewPeaklist;
		static Root::Action::CmdStr ImportPeaklist;
		static Root::Action::CmdStr ExportPeaklist;
		static Root::Action::CmdStr OpenPeaklist;
		static Root::Action::CmdStr SavePeaklist;
		static Root::Action::CmdStr GotoPeak;
		static Root::Action::CmdStr ExportIntegTable;
		static Root::Action::CmdStr NextSpec;
		static Root::Action::CmdStr PrevSpec;
		static Root::Action::CmdStr GotoSpec;
		static Root::Action::CmdStr SetupBatch;
		static Root::Action::CmdStr AdaptHome;
		static Root::Action::CmdStr ShowAssig;
		static Root::Action::CmdStr ImportAlias;
		static Root::Action::CmdStr ImportLinks;
		static Root::Action::CmdStr EditPeak;
		static Root::Action::CmdStr ShowPeak;
		static Root::Action::CmdStr CheckPeakDoubles;
		static Root::Action::CmdStr EditAtts;
		static Root::Action::CmdStr SetColor;
		static Root::Action::CmdStr NextList;
		static Root::Action::CmdStr PrevList;
		static Root::Action::CmdStr ShowTable;
		static Root::Action::CmdStr SetRgbColor;
		static Root::Action::CmdStr ExportCaraSpec;
		static Root::Action::CmdStr ExportEasySpec;
		static Root::Action::CmdStr SyncPeak;
		static Root::Action::CmdStr SaveColors;
		static Root::Action::CmdStr SetLabel;
		static Root::Action::CmdStr EditAssig;
		static Root::Action::CmdStr AddAssig;
		static Root::Action::CmdStr RemoveAssig;
		static Root::Action::CmdStr ExportTable;
		
		MonoScope(Root::Agent *, Spectrum*, Project* = 0, 
			PeakList* = 0, const Rotation& = Rotation() );
	protected:
		void init();
		bool handleSave(bool _export) const;
		bool askToCloseWindow() const; // Override
		virtual ~MonoScope();
		void handleAction(Root::Action&);
		void handle(Root::Message&);
	private:
		void handleExportTable( Action& );
		void handleRemoveAssig( Action& );
		void handleAddAssig( Action& );
		void handleEditAssig( Action& );
		void handleSetLabel( Action& );
		void handleSaveColors( Action& );
		void handleSyncPeak( Action& );
		void handleExportCaraSpec( Action& );
		void handleExportEasySpec( Action& );
		void handleSetRgbColor( Action& );
		void handleShowTable( Root::Action& );
		void handleNextList( Action& );
		void handlePrevList( Action& );
		void handleSelectPeaks( Action& );
		void handleSetColor( Action& );
		void handleEditAtts( Action& );
		void handleCheckPeakDoubles( Action& );
		void handleShowPeak( Action& );
		void handleEditPeak( Action& );
		void handleImportLinks( Action& );
		void handleImportAlias( Action& );
		void handleShowAssig( Action& );
		void handleAdaptHome( Action& );
		void handleSetupBatch( Action& );
		void handlePrevSpec( Action& );
		void handleNextSpec( Action& );
		void handleGotoSpec( Action& );
		void handleExportIntegTable( Action& );
		void handleSelectSpec( Action& );
		void handleGotoPeak( Action& );
		void handleSavePeaklist( Action& );
		void handleOpenPeaklist( Action& );
		void handleShowList( Action& );
		void handleExportPeaklist( Action& );
		void handleImportPeaklist( Action& );
		void handleNewPeaklist( Action& );
		void handleShowSlices( Action& );
		FRIEND_ACTION_HANDLER( MonoScope );
		void updateCaption();
		void buildMenus();
		void buildViews(bool rebuild = true);

		Root::Ref<Lexi::FocusManager> d_focus;
		Lexi::GlyphWidget* d_widget;
		Root::Ptr<MonoScopeAgent> d_agent;
		Root::Ref<PeakListGadget> d_list;
		Root::Ref<SpecViewer> d_ov;
		Root::Ref<OverviewCtrl> d_ovCtrl;
		bool d_showList;
	};
}

#endif // !defined(AFX_MONOSCOPE2_H__C8350055_D646_4DA6_88AE_9D03BC517E1F__INCLUDED_)
