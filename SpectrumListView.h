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

#if !defined(AFX_SPECTRUMLISTVIEW_H__6F22531A_A900_4946_AFC5_E6C12DA04CF6__INCLUDED_)
#define AFX_SPECTRUMLISTVIEW_H__6F22531A_A900_4946_AFC5_E6C12DA04CF6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Gui/ListView.h>
#include <Gui/Menu.h>
#include <Spec/Repository.h>
#include <Root/ActionHandler.h>

class AidaCentral;

namespace Spec
{
	class SpectrumPeer;
}
class SpectrumListView : public Gui::ListView, public Root::Messenger
{
public:
	static Root::Action::CmdStr AddSpectrum;
	static Root::Action::CmdStr OpenSpectrum;
	static Root::Action::CmdStr RemoveSpectrum;
    static Root::Action::CmdStr CalcLevels;
	static Root::Action::CmdStr RenameSpectrum;
	static Root::Action::CmdStr OpenSynchroScope;
	static Root::Action::CmdStr OpenHomoScope;
	static Root::Action::CmdStr OpenHomoScopeRot;
	static Root::Action::CmdStr ExportAtomList;
	static Root::Action::CmdStr ExportMapperFile;
	static Root::Action::CmdStr OpenSliceScope;
	static Root::Action::CmdStr EditFields;
	static Root::Action::CmdStr ImportAlias;
	static Root::Action::CmdStr WriteParamFile;
	static Root::Action::CmdStr AddColumn;
	static Root::Action::CmdStr RemoveCols;
	static Root::Action::CmdStr ReplaceSpec;
	static Root::Action::CmdStr ShowTable;
	static Root::Action::CmdStr MapToType;
	static Root::Action::CmdStr OpenMonoScopeRot;
	static Root::Action::CmdStr Calibrate;
	static Root::Action::CmdStr OpenPolyScope2;
	static Root::Action::CmdStr OpenPolyScope2Rot;
	static Root::Action::CmdStr OpenSystemScope2Rot;
	static Root::Action::CmdStr OpenSystemScope2;
	static Root::Action::CmdStr OpenStripScope2Rot;
	static Root::Action::CmdStr OpenStripScope2;
    static Root::Action::CmdStr OpenFourDScope;
    static Root::Action::CmdStr OpenFourDScopeRot;
	static Root::Action::CmdStr SetPeakWidth;
	static Root::Action::CmdStr DuplicateSpec;
	static Root::Action::CmdStr OpenSynchroScope2;
	static Root::Action::CmdStr OpenStripScope2DRot;
	static Root::Action::CmdStr OpenStripScope2D;
	static Root::Action::CmdStr SetFolding;
	static Root::Action::CmdStr OpenSitar;
	static Root::Action::CmdStr SetSample;

	static bool mapToType( QWidget*, Spec::SpectrumType*, Spec::SpectrumPeer* );
	static void saveFlatCaraSpectrum( QWidget*, Spec::Spectrum* );
	static void saveCaraSpectrum( QWidget*, Spec::Spectrum* );
	static void saveEasySpectrum( QWidget*, Spec::Spectrum* );
	static void exportAtomList( QWidget*, Spec::Project*, Spec::Spectrum* = 0 );
	static bool importAlias( QWidget*, Spec::PointSet*, Spec::Project*, Spec::Spectrum* );
	static bool importLinks( QWidget*, Spec::PointSet*, Spec::Project*, Spec::Spectrum* );
	void refill();
	SpectrumListView(QWidget*, AidaCentral*, Spec::Repository*,Spec::Project*);
	virtual ~SpectrumListView();
    static Gui::Menu* createPopup( Spec::Repository* );
protected:
	GENERIC_MESSENGER(Gui::ListView)
	void handle( Root::Message& );
	void onCurrentChanged();
private:
	void handleSetSample( Root::Action& );
	void handleOpenSitar( Root::Action& );
	void handleSetFolding( Root::Action& );
	void handleDuplicateSpec( Root::Action& );
	void handleSetPeakWidth( Root::Action& );
	void handleOpenStripScope2DRot( Root::Action& );
	void handleOpenStripScope2D( Root::Action& );
	void handleOpenStripScope2Rot( Root::Action& );
	void handleOpenStripScope2( Root::Action& );
	void handleOpenSystemScope2Rot( Root::Action& );
	void handleOpenSystemScope2( Root::Action& );
	void handleOpenPolyScope2Rot( Root::Action& );
	void handleOpenPolyScope2( Root::Action& );
    void handleOpenFourDScopeRot( Root::Action& );
	void handleOpenFourDScope( Root::Action& );
	void handleCalibrate( Root::Action& );
	void handleOpenHomoScopeRot( Root::Action& );
	void handleOpenMonoScopeRot( Root::Action& );
	void handleMapToType( Root::Action& );
	void handleShowTable( Root::Action& );
	void handleReplaceSpec( Root::Action& );
	void handleRemoveCols( Root::Action& );
	void handleAddColumn( Root::Action& );
	void handleWriteParamFile( Root::Action& );
	void handleImportAlias( Root::Action& );
	void handleEditFields( Root::Action& );
	void handleOpenSliceScope( Root::Action& );
	void handleExportMapperFile( Root::Action& );
	void handleExportAtomList( Root::Action& );
	void handleOpenHomoScope( Root::Action& );
	void handleOpenSystemScopeFlipped( Root::Action& );
	void handleOpenSystemScope( Root::Action& );
	void handleOpenStripScope( Root::Action& );
	void handleOpenSynchroScope2( Root::Action& );
	void handleRemoveSpectrum( Root::Action& );
	void handleRenameSpectrum( Root::Action& );
    void handleCalcLevels( Root::Action& );
	void handleOpenSpectrum( Root::Action& );
	void handleAddSpectrum( Root::Action& );
	void handleOpenTestScope( Root::Action& );
	FRIEND_ACTION_HANDLER( SpectrumListView );
	Gui::ListViewItem* addItem( Spec::SpectrumPeer* );
	Gui::ListViewItem* findItem( Spec::SpectrumPeer* ) const;
	Root::ExRef<Spec::Project> d_pro;
	Root::ExRef<Spec::Repository> d_rep;
	Root::Ptr<AidaCentral> d_central;
};

#endif // !defined(AFX_SPECTRUMLISTVIEW_H__6F22531A_A900_4946_AFC5_E6C12DA04CF6__INCLUDED_)
