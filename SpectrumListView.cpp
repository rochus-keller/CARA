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
 
#include "SpectrumListView.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog> 
#include <QApplication>
#include <QComboBox>
#include <QInputDialog>
#include <Root/Application.h>
#include <Spec/Factory.h>
#include <Spec/SpecRotator.h>
#include <Spec/CaraSpectrum.h>
#include <Spec/EasySpectrum.h>
#include <Spec/EasyProtonList.h>
#include <Spec/Fragmenter.h>
#include <Spec/EasyNumber.h>
#include <Spec/SitarSpectrum.h>
#include <Spec/MemSpectrum.h>
#include <SpecView/DynValueEditor.h>
#include <SpecView/ObjectListView.h>
#include <Root/Any.h>
#include <Root/MessageLog.h>
#include <AidaCentral.h>
#include <SliceScope.h>
#include <SpecView/RotateDlg.h>
#include <Dlg.h>
#include <Gui/InputDlg.h>
#include "LuaCaraExplorer.h"
#include "TestScope.h"
using namespace Spec;
using namespace Gui;

static char s_buf[64];
static std::vector<Root::SymbolString> s_cols;
static const Root::UInt32 s_max = 2000000000;

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr SpectrumListView::AddSpectrum = "AddSpectrum";
Root::Action::CmdStr SpectrumListView::OpenSpectrum = "OpenSpectrum";
Root::Action::CmdStr SpectrumListView::RemoveSpectrum = "RemoveSpectrum";
Root::Action::CmdStr SpectrumListView::CalcLevels = "CalcLevels";
Root::Action::CmdStr SpectrumListView::RenameSpectrum = "RenameSpectrum";
Root::Action::CmdStr SpectrumListView::OpenSynchroScope = "OpenSynchroScope";
Root::Action::CmdStr SpectrumListView::OpenSynchroScope2 = "OpenSynchroScope2";
Root::Action::CmdStr SpectrumListView::OpenHomoScope = "OpenHomoScope";
Root::Action::CmdStr SpectrumListView::ExportAtomList = "ExportAtomList";
Root::Action::CmdStr SpectrumListView::ExportMapperFile = "ExportMapperFile";
Root::Action::CmdStr SpectrumListView::OpenSliceScope = "OpenSliceScope";
Root::Action::CmdStr SpectrumListView::EditFields = "EditFields";
Root::Action::CmdStr SpectrumListView::ImportAlias = "ImportAlias";
Root::Action::CmdStr SpectrumListView::WriteParamFile = "WriteParamFile";
Root::Action::CmdStr SpectrumListView::AddColumn = "AddColumn";
Root::Action::CmdStr SpectrumListView::RemoveCols = "RemoveCols";
Root::Action::CmdStr SpectrumListView::ReplaceSpec = "ReplaceSpec";
Root::Action::CmdStr SpectrumListView::ShowTable = "ShowTable";
Root::Action::CmdStr SpectrumListView::MapToType = "MapToType";
Root::Action::CmdStr SpectrumListView::OpenMonoScopeRot = "OpenMonoScopeRot";
Root::Action::CmdStr SpectrumListView::OpenHomoScopeRot = "OpenHomoScopeRot";
Root::Action::CmdStr SpectrumListView::Calibrate = "Calibrate";
Root::Action::CmdStr SpectrumListView::OpenPolyScope2 = "OpenPolyScope2";
Root::Action::CmdStr SpectrumListView::OpenFourDScope = "OpenFourDScope";
Root::Action::CmdStr SpectrumListView::OpenFourDScopeRot = "OpenFourDScopeRot";
Root::Action::CmdStr SpectrumListView::OpenPolyScope2Rot = "OpenPolyScope2Rot";
Root::Action::CmdStr SpectrumListView::OpenSystemScope2 = "OpenSystemScope2";
Root::Action::CmdStr SpectrumListView::OpenSystemScope2Rot = "OpenSystemScope2Rot";
Root::Action::CmdStr SpectrumListView::OpenStripScope2 = "OpenStripScope2";
Root::Action::CmdStr SpectrumListView::OpenStripScope2Rot = "OpenStripScope2Rot";
Root::Action::CmdStr SpectrumListView::SetPeakWidth = "SetPeakWidth";
Root::Action::CmdStr SpectrumListView::DuplicateSpec = "DuplicateSpec";
Root::Action::CmdStr SpectrumListView::OpenStripScope2D = "OpenStripScope2D";
Root::Action::CmdStr SpectrumListView::OpenStripScope2DRot = "OpenStripScope2DRot";
Root::Action::CmdStr SpectrumListView::SetFolding = "SetFolding";
Root::Action::CmdStr SpectrumListView::OpenSitar = "OpenSitar";
Root::Action::CmdStr SpectrumListView::SetSample = "SetSample";

ACTION_SLOTS_BEGIN( SpectrumListView )
    { SpectrumListView::SetSample, &SpectrumListView::handleSetSample },
    { SpectrumListView::OpenSitar, &SpectrumListView::handleOpenSitar },
    { SpectrumListView::SetFolding, &SpectrumListView::handleSetFolding },
    { SpectrumListView::OpenStripScope2DRot, &SpectrumListView::handleOpenStripScope2DRot },
    { SpectrumListView::OpenStripScope2D, &SpectrumListView::handleOpenStripScope2D },
    { SpectrumListView::DuplicateSpec, &SpectrumListView::handleDuplicateSpec },
    { SpectrumListView::SetPeakWidth, &SpectrumListView::handleSetPeakWidth },
    { SpectrumListView::OpenStripScope2Rot, &SpectrumListView::handleOpenStripScope2Rot },
    { SpectrumListView::OpenStripScope2, &SpectrumListView::handleOpenStripScope2 },
    { SpectrumListView::OpenPolyScope2Rot, &SpectrumListView::handleOpenPolyScope2Rot },
    { SpectrumListView::OpenPolyScope2, &SpectrumListView::handleOpenPolyScope2 },
    { SpectrumListView::OpenFourDScopeRot, &SpectrumListView::handleOpenFourDScopeRot },
    { SpectrumListView::OpenFourDScope, &SpectrumListView::handleOpenFourDScope },
    { SpectrumListView::Calibrate, &SpectrumListView::handleCalibrate },
    { SpectrumListView::OpenHomoScopeRot, &SpectrumListView::handleOpenHomoScopeRot },
    { SpectrumListView::OpenMonoScopeRot, &SpectrumListView::handleOpenMonoScopeRot },
    { SpectrumListView::MapToType, &SpectrumListView::handleMapToType },
    { SpectrumListView::ShowTable, &SpectrumListView::handleShowTable },
    { SpectrumListView::ReplaceSpec, &SpectrumListView::handleReplaceSpec },
    { SpectrumListView::RemoveCols, &SpectrumListView::handleRemoveCols },
    { SpectrumListView::AddColumn, &SpectrumListView::handleAddColumn },
    { SpectrumListView::WriteParamFile, &SpectrumListView::handleWriteParamFile },
    { SpectrumListView::ImportAlias, &SpectrumListView::handleImportAlias },
    { SpectrumListView::EditFields, &SpectrumListView::handleEditFields },
    { SpectrumListView::OpenSliceScope, &SpectrumListView::handleOpenSliceScope },
    { SpectrumListView::ExportMapperFile, &SpectrumListView::handleExportMapperFile },
    { SpectrumListView::ExportAtomList, &SpectrumListView::handleExportAtomList },
    { SpectrumListView::OpenHomoScope, &SpectrumListView::handleOpenHomoScope },
    { SpectrumListView::OpenSystemScope2Rot, &SpectrumListView::handleOpenSystemScope2Rot },
    { SpectrumListView::OpenSystemScope2, &SpectrumListView::handleOpenSystemScope2 },
    { SpectrumListView::OpenSynchroScope2, &SpectrumListView::handleOpenSynchroScope2 },
    { SpectrumListView::RenameSpectrum, &SpectrumListView::handleRenameSpectrum },
    { SpectrumListView::RemoveSpectrum, &SpectrumListView::handleRemoveSpectrum },
    { SpectrumListView::CalcLevels, &SpectrumListView::handleCalcLevels },
    { SpectrumListView::OpenSpectrum, &SpectrumListView::handleOpenSpectrum },
    { "OpenTestScope", &SpectrumListView::handleOpenTestScope },
    { SpectrumListView::AddSpectrum, &SpectrumListView::handleAddSpectrum },
ACTION_SLOTS_END( SpectrumListView )

//////////////////////////////////////////////////////////////////////

class _SpectrumListItem : public ListViewItem
{
public:
	Root::ExRef<SpectrumPeer> d_spec;
	_SpectrumListItem( ListView* p, SpectrumPeer* spec ):ListViewItem(p),d_spec( spec ) {}

	SpectrumPeer* getSpec() const { return d_spec; }

	enum { NumOfCols = 6 };

	QString text( int f ) const 
	{ 
		switch( f )
		{
		case 0:
			return d_spec->getName();
		case 1:
			if( d_spec->getType() )
				return d_spec->getType()->getName().data();
			else
				return "<none>";
		case 2:
			::sprintf( s_buf, "%d", d_spec->getId() );
			return s_buf;
		case 3:
			::sprintf( s_buf, "%d", d_spec->getDimCount() );
			return s_buf;
		case 4:
			if( d_spec->getSample() )
				return QString::number( d_spec->getSample()->getId() );
			else
				return "";
		case 5:
			return d_spec->getFilePath();
		default:
			{
				Root::Object* o = d_spec;
				Root::Any a;
				o->getFieldValue( s_cols[ f - NumOfCols ], a );
				if( a.isNull() )
					return "";
				else
					return a.getCStr();
			}
			return "";
		}
	}
    QVariant font( int col ) const
	{
		switch( col )
		{
		case 5:
            if( d_spec->isDummy() )
            {
                QFont font = listView()->font();
                font.setStrikeOut(true);
                return font;
            }else
                return QVariant();
		default:
            return QVariant();
		}
	}
    QVariant foreground( int col ) const
	{
        if( d_spec->isDummy() )
        {
            return Qt::lightGray;
        }else
            return QVariant();
	}
	QString key( int f, bool ascending  ) const
	{
		switch( f )
		{
		case 0:
			return d_spec->getName();
		case 1:
			if( d_spec->getType() )
				return d_spec->getType()->getName().data();
			else
				return "<none>";
		case 2:
			::sprintf( s_buf, "%08d", d_spec->getId() );
			return s_buf;
		case 3:
			::sprintf( s_buf, "%08d", d_spec->getDimCount() );
			return s_buf;
		case 4:
			if( d_spec->getSample() )
			{
				::sprintf( s_buf, "%08d", d_spec->getSample()->getId() );
				return s_buf;
			}else
				return "000000000";
		case 5:
			return d_spec->getFilePath();
		default:
			{
				Root::Object* o = d_spec;
				Root::Any a;
				o->getFieldValue( s_cols[ f - NumOfCols ], a );
				if( a.isNull() )
					return "";
				else if( a.isNumber() )
				{
					::sprintf( s_buf, "%08.3f", a.getDouble() );
					return s_buf;
				}else
					return a.getCStr();
			}
			return "";
		}
	}
	// const QPixmap *pixmap( int i ) const { return s_pixRepository; }
};

class _SpectrumDimItem : public ListViewItem
{
public:
	_SpectrumDimItem( ListViewItem* p, Dimension d ):
	  ListViewItem( p ), d_dim( d ) {}

	Dimension d_dim;

	QString text( int f ) const 
	{ 
		SpectrumPeer* sp = static_cast<_SpectrumListItem*>( parent() )->getSpec();
		const Scale& s = sp->getScale( d_dim );
		QString str;
		switch( f )
		{
		case 0:
			str.sprintf( "range=[%03.2f..%03.2f] cal=%+0.3f res=%d delta=%0.3f fold=%s",
				s.getIdx0(), s.getIdxN(),
				sp->getScaleAdjust( d_dim ), s.getSampleCount(), s.getDelta(), 
				Scale::getFoldingName( s.getFolding() ) );
			return str;
		case 1:
			str.sprintf( "Dim. %s (%s)", getDimSymbol( d_dim, true ), 
				getDimSymbol( sp->getRotation( d_dim ), true ) );
			return str;
		case 3:
			str.sprintf( "%s (%s)",
				sp->getType()->getColor( d_dim ).getIsoLabel(), 
				s.getColor().getIsoLabel() ), sp->getLabel( d_dim );
			return str;
		}
		return "";
	}
    QVariant foreground( int col ) const
	{
        SpectrumPeer* sp = static_cast<_SpectrumListItem*>( parent() )->getSpec();
        if( sp->isDummy() )
        {
            return Qt::lightGray;
        }else
            return QVariant();
	}
};

//////////////////////////////////////////////////////////////////////

SpectrumListView::SpectrumListView(QWidget* v,AidaCentral* c, Spec::Repository* r,Spec::Project* p):
	ListView( v ), d_rep( r ), d_pro( p ), d_central( c )
{
	setShowSortIndicator( true );
	setRootIsDecorated( true );
	setAllColumnsShowFocus( true );
	addColumn( "Name" );
	addColumn( "Type" );
	addColumn( "ID" );
	addColumn( "Dim" );
	addColumn( "Samp." );
	addColumn( "Path" );
    for( int i = 0; i < int(s_cols.size()); i++ )
		addColumn( s_cols[ i ].data() );
	refill();
    d_pro->addObserver( this );
}

SpectrumListView::~SpectrumListView()
{
	d_pro->removeObserver( this );
}

static void fillSubs( _SpectrumListItem* i, SpectrumPeer * sp )
{
	for( Dimension d = 0; d < sp->getDimCount(); d++ )
	{
        new _SpectrumDimItem( i, d );
	}
}

ListViewItem* SpectrumListView::addItem( SpectrumPeer * sp)
{
	_SpectrumListItem* i = new _SpectrumListItem( this, sp );
	assert( sp->getType() );
	fillSubs( i, sp );
	return i;
}

Gui::ListViewItem* SpectrumListView::findItem( Spec::SpectrumPeer* s ) const
{
	for( int i = 0; i < count(); i++ )
		if( static_cast<_SpectrumListItem*>( child( i ) )->d_spec == s )
			return child( i );
	return 0;
}

void SpectrumListView::refill()
{
	clear();
	const Project::SpectrumMap& sm = d_pro->getSpectra();
	Project::SpectrumMap::const_iterator p;
	for( p = sm.begin(); p != sm.end(); ++p )
	{
		addItem( (*p).second );
	}
    QMetaObject::invokeMethod( this, "resizeFirstColumnToContents", Qt::QueuedConnection );
}

Gui::Menu* SpectrumListView::createPopup(Spec::Repository* rep )
{
    Gui::Menu* pop = new Gui::Menu();
	Gui::Menu::item( pop, "Open MonoScope...", OpenSpectrum, false );
	Gui::Menu::item( pop, "Open MonoScope (rotated)...", OpenMonoScopeRot, false );
	Gui::Menu::item( pop, "Open SliceScope...", OpenSliceScope, false );
	Gui::Menu::item( pop, "Open SitarViewer...", OpenSitar, false );
#ifdef _DEBUG
    Gui::Menu::item( pop, "Open TestScope...", "OpenTestScope", false );
#endif
	pop->insertSeparator();
	Gui::Menu::item( pop, "Open SynchroScope...", OpenSynchroScope2, false );
	Gui::Menu::item( pop, "Open StripScope...", OpenStripScope2, false );
	Gui::Menu::item( pop, "Open StripScope (rotated)...", OpenStripScope2Rot, false );
	Gui::Menu::item( pop, "Open StripScope 2D...", OpenStripScope2D, false );
	Gui::Menu::item( pop, "Open StripScope 2D (rotated)...", OpenStripScope2DRot, false );
	Gui::Menu::item( pop, "Open SystemScope...", OpenSystemScope2, false );
	Gui::Menu::item( pop, "Open SystemScope (rotated)...", OpenSystemScope2Rot, false );
	Gui::Menu::item( pop, "Open HomoScope...", OpenHomoScope, false );
	Gui::Menu::item( pop, "Open HomoScope (rotated)...", OpenHomoScopeRot, false );
	Gui::Menu::item( pop, "Open PolyScope...", OpenPolyScope2, false );
	Gui::Menu::item( pop, "Open PolyScope (rotated)...", OpenPolyScope2Rot, false );
    Gui::Menu::item( pop, "Open FourDScope...", OpenFourDScope, false );
	Gui::Menu::item( pop, "Open FourDScope (rotated)...", OpenFourDScopeRot, false );
	pop->insertSeparator();
#ifndef QT_MAC_USE_COCOA
 	Gui::Menu* pop2 = new Gui::Menu( pop, "Add Spectrum" );
	pop->addMenu( pop2 );
	typedef std::map<QByteArray ,SpectrumType*> Sort;
	Sort sort;
    const Repository::SpectrumTypeMap& st = rep->getSpecTypes();
	Repository::SpectrumTypeMap::const_iterator p;
	for( p = st.begin(); p != st.end(); ++p )
	{
		sort[ (*p).first.data() ] = (*p).second;
	}
	Sort::const_iterator q;
	for( q = sort.begin(); q != sort.end(); ++q )
	{
		Gui::Menu::item( pop2, (*q).second->getName().data(), AddSpectrum, false )->addParam( (*q).second );
	}
#else
	// Avoid submenus on OS X Aqua because Qt does strange things with them
	Gui::Menu::item( pop, "Add Spectrum...", AddSpectrum, false );
#endif
	Gui::Menu::item( pop, "&Map to Type...", MapToType, false );
	Gui::Menu::item( pop, "&Rename Spectrum...", RenameSpectrum, false );
	Gui::Menu::item( pop, "Assign Sample...", SetSample, false );
	Gui::Menu::item( pop, "Re&place Spectrum...", ReplaceSpec, false );
	Gui::Menu::item( pop, "Duplicate Spectrum...", DuplicateSpec, false );
	Gui::Menu::item( pop, "Remove Spectrum...", RemoveSpectrum, false );
	Gui::Menu::item( pop, "Calibrate Spectrum...", Calibrate, false );
	Gui::Menu::item( pop, "Write Calibration...", WriteParamFile, false );
    Gui::Menu::item( pop, "Calculate Levels...", CalcLevels, false );
	Gui::Menu::item( pop, "Set Folding...", SetFolding, false );
	pop->insertSeparator();
	Gui::Menu::item( pop, "Edit Attributes...", EditFields, false );
	Gui::Menu::item( pop, "Open Object Table...", ShowTable, false );
	pop->insertSeparator();
	Gui::Menu::item( pop, "Import Alias Shifts...", ImportAlias, false );
	Gui::Menu::item( pop, "Export Atom List...", ExportAtomList, false );
	Gui::Menu::item( pop, "Export Mapper File...", ExportMapperFile, false );
	pop->insertSeparator();
	Gui::Menu::item( pop, "Add Column...", AddColumn, false );
	Gui::Menu::item( pop, "Remove Columns", RemoveCols, false );
    return pop;
}

void SpectrumListView::saveCaraSpectrum(QWidget * parent, Spectrum * spec)
{
	QFileInfo info2( spec->getName() );

	RotateDlg dlg( parent, "Original", "Saved", true ); 
	dlg.setCaption( "Save Original Spectrum Rotated" );
	Rotation rot( spec->getDimCount() );
	QString s1;
	Dimension d;
	for( d = 0; d < rot.size(); d++ )
	{
		s1.sprintf( "%s (%s)", 
			spec->getScale( d ).getColor().getIsoLabel(), 
			spec->getLabel( d ) );
		dlg.addDimension( s1, getDimSymbol( d ), spec->getScale( d ).getColor() );
		rot[ d ] = d;
	}
	if( !dlg.rotate( rot ) )
		return;
	ColorMap clr( rot.size() );
	for( d = 0; d < rot.size(); d++ )
		clr[ rot[ d ] ] = dlg.getColors()[ d ];

	QString fileName = Root::AppAgent::getCurrentDir();
	fileName += "/"; // RISK
	fileName += info2.baseName();
	fileName = QFileDialog::getSaveFileName( parent, 
			"Save CARA Spectrum", fileName, 
            "CARA Spectrum (*.nmr)", 0, QFileDialog::DontConfirmOverwrite );
	if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );

	if( info.extension( false ).upper() != "NMR" )
		fileName += ".nmr";
	info.setFile( fileName );
	if( info.exists() )
	{
		if( QMessageBox::warning( parent, "Save As",
			"This file already exists. Do you want to overwrite it?",
			"&OK", "&Cancel" ) != 0 )
			return;
	}

	QApplication::setOverrideCursor( Qt::waitCursor );
	SpecRef<Spectrum> tmp = new SpecRotator( spec, rot );
	Spectrum::Levels l = spec->getLevels( false );	// RISK: false ist schneller
	Amplitude porig = l.d_pMax, norig = l.d_nMax;
	QApplication::restoreOverrideCursor();
	int res;
	if( !Dlg::getSpectrumFormat( parent, res, l.d_pMax, l.d_nMax ) )
		return;
	if( res >= 100 )
	{
		res -= 100;
		// TODO: Noise anpassen
	}
	try
	{
		QApplication::setOverrideCursor( Qt::waitCursor );
		CaraSpectrum::writeToFile( fileName, tmp, l, porig, norig, 
			CaraSpectrum::Kind(res), clr );
		QApplication::restoreOverrideCursor();
	}catch( Root::Exception& e )
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::critical( parent, "Error saving spectrum", e.what(), "&Cancel" );
	}
}

void SpectrumListView::saveEasySpectrum(QWidget * parent, Spectrum * spec)
{
	QFileInfo info2( spec->getName() );

	RotateDlg dlg( parent, "Original", "Saved", true ); 
	dlg.setCaption( "Save Original Spectrum Rotated" );
	Rotation rot( spec->getDimCount() );
	QString s1;
	Dimension d;
	for( d = 0; d < rot.size(); d++ )
	{
		s1.sprintf( "%s (%s)", 
			spec->getScale( d ).getColor().getIsoLabel(), 
			spec->getLabel( d ) );
		dlg.addDimension( s1, getDimSymbol( d ), spec->getScale( d ).getColor() );
		rot[ d ] = d;
	}
	if( !dlg.rotate( rot ) )
		return;
	ColorMap clr( rot.size() );
	for( d = 0; d < rot.size(); d++ )
		clr[ rot[ d ] ] = dlg.getColors()[ d ];

	QString fileName = Root::AppAgent::getCurrentDir();
	fileName += "/"; // RISK
	fileName += info2.baseName();
	fileName = QFileDialog::getSaveFileName( parent, 
			"Save Easy Spectrum", fileName, 
            "EASY Spectrum (*.param)", 0, QFileDialog::DontConfirmOverwrite );
	if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );

	if( info.extension( false ).upper() != "3D.PARAM" )
		fileName += ".3D.param";
	info.setFile( fileName );
	if( info.exists() )
	{
		if( QMessageBox::warning( parent, "Save As",
			"This file already exists. Do you want to overwrite it?",
			"&OK", "&Cancel" ) != 0 )
			return;
	}

	QApplication::setOverrideCursor( Qt::waitCursor );
	SpecRef<Spectrum> tmp = new SpecRotator( spec, rot );
	QApplication::restoreOverrideCursor();
	try
	{
		QApplication::setOverrideCursor( Qt::waitCursor );

		Spectrum::Levels l = spec->getLevels();
		double amax = l.d_pMax;
		if( -l.d_nMax > amax )
			amax = -l.d_nMax;
		double factor = 1.0;
		if( amax > EasyNumber::s_max )
			factor = double( EasyNumber::s_max ) / amax;
		QApplication::restoreOverrideCursor();

		Root::Extension ext;
		ext.assign( spec->getDimCount(), 1 );
		for( int i = 0; i < ext.size(); i++ )
		{
			ext[ i ] = log( (double)spec->getScale(i).getSampleCount() ) / log( 2.0 );
			ext[ i ] = pow( 2.0, floor( log( (double)ext[ i ] ) / log( 2.0 ) ) );
		}
		if( !Dlg::getSubMatCount( parent, spec, ext, factor ) )
			return;
		QApplication::setOverrideCursor( Qt::waitCursor );
		EasySpectrum::writeParamFile( fileName, tmp, ext );
		fileName = fileName.left( fileName.findRev( "PARAM", -1, false ) );
		fileName += "16";
		EasySpectrum::writeToFile( fileName, tmp, factor, ext );
		QApplication::restoreOverrideCursor();
	}catch( Root::Exception& e )
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::critical( parent, "Error saving spectrum", e.what(), "&Cancel" );
	}
}

void SpectrumListView::saveFlatCaraSpectrum(QWidget * parent, Spectrum * spec)
{
	if( spec->getDimCount() <= 2 )
	{
		QMessageBox::critical( parent, "Error saving projected spectrum", 
			"Only three or higher dimensional spectra supported", "&Cancel" );
		return;
	}
	QFileInfo info2( spec->getName() );

	RotateDlg dlg( parent, "Original", "Projected", true ); 
	dlg.setCaption( "Save Projected Spectrum" );
	Rotation rot( spec->getDimCount() );
	QString s1;
	Dimension d;
	for( d = 0; d < rot.size(); d++ )
	{
		s1.sprintf( "%s (%s)", 
			spec->getScale( d ).getColor().getIsoLabel(), 
			spec->getLabel( d ) );
		dlg.addDimension( s1, (d<2)?getDimSymbol( d ):"-", spec->getScale( d ).getColor() );
		rot[ d ] = d;
	}
	if( !dlg.rotate( rot ) )
		return;
	ColorMap clr( 2 );
	for( d = 0; d < clr.size(); d++ )
		clr[ d ] = dlg.getColors()[ rot[ d ] ];

	QString fileName = Root::AppAgent::getCurrentDir();
	fileName += "/"; // RISK
	fileName += info2.baseName();
	fileName = QFileDialog::getSaveFileName( parent, 
			"Save CARA Spectrum", fileName, 
            "CARA Spectrum (*.nmr)", 0, QFileDialog::DontConfirmOverwrite );
	if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );

	if( info.extension( false ).upper() != "NMR" )
		fileName += ".nmr";
	info.setFile( fileName );
	if( info.exists() )
	{
		if( QMessageBox::warning( parent, "Save As",
			"This file already exists. Do you want to overwrite it?",
			"&OK", "&Cancel" ) != 0 )
			return;
	}

	QApplication::setOverrideCursor( Qt::waitCursor );

	// Hier die Projektion herstellen.

	ScaleVector sv( 2 );
	sv[ DimX ] = spec->getScale( rot[ DimX ] );
	sv[ DimX ].setColor( clr[ DimX ] );
	sv[ DimY ] = spec->getScale( rot[ DimY ] );
	sv[ DimY ].setColor( clr[ DimY ] );
	SpecRef<Spectrum> tmp = spec;
	Root::Ref<MemSpectrum> mem = new MemSpectrum( sv );
	mem->flatten( spec, rot );
	Spectrum::Levels l = mem->getLevels();
	Amplitude porig = l.d_pMax, norig = l.d_nMax;
	QApplication::restoreOverrideCursor();

	// Die Amplituden sind dann bereits bekannt
	int res;
	if( !Dlg::getSpectrumFormat( parent, res, l.d_pMax, l.d_nMax ) )
		return;
	if( res >= 100 )
	{
		res -= 100;
		// TODO: Noise anpassen
	}
	try
	{
		QApplication::setOverrideCursor( Qt::waitCursor );
		CaraSpectrum::writeToFile( fileName, mem, l, porig, norig, 
			CaraSpectrum::Kind(res), clr );
		QApplication::restoreOverrideCursor();
	}catch( Root::Exception& e )
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::critical( parent, "Error saving spectrum", e.what(), "&Cancel" );
	}
}

void SpectrumListView::exportAtomList(QWidget * parent, Project * pro, Spectrum * spec)
{
	QString fileName = QFileDialog::getSaveFileName( parent, 
			"Export Atom List", Root::AppAgent::getCurrentDir(), 
            "Atom List (*.prot)", 0, QFileDialog::DontConfirmOverwrite );
	if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );

	if( info.extension( false ).upper() != "PROT" )
		fileName += ".prot";
	info.setFile( fileName );
	if( info.exists() )
	{
		if( QMessageBox::warning( parent, "Save As",
			"This file already exists. Do you want to overwrite it?",
			"&OK", "&Cancel" ) != 0 )
			return;
	}
	Root::AppAgent::setCurrentDir( info.dirPath( true ) );

	bool resi = true;
	switch( QMessageBox::information( parent, "Export Atom List",
			  "Shall the assignments reference spin systems or residues? ",
			  "&System", "&Residue", "&Cancel", 0, 2 ) )		
	{
	case 0: // System
		resi = false;
		break;
	case 1:	// Residue
		resi = true;
		break;	
	default:
		return;	// Cancel
	}

	QApplication::setOverrideCursor( Qt::waitCursor );

	const SpinBase::SpinMap& sm = pro->getSpins()->getSpins();
	Root::Ref<EasyProtonList> pl = new EasyProtonList( sm.size() );
	int i = 0;
	SpinBase::SpinMap::const_iterator p1;
	EasyProtonList::Atom at;
	Spin* s;
	ResidueType* rt;
	for( p1 = sm.begin(); p1 != sm.end(); ++p1, i++ )
	{
		s = (*p1).second;
		if( !s->getLabel().isNull() && s->getLabel().getOffset() == 0 &&
			s->getLabel().isFinal() )
		{
			if( resi && ( s->getSystem() == 0 || s->getSystem()->getAssig() == 0 ) )
				continue;
			if( s->getSystem() && s->getSystem()->getAssig() )
				rt = s->getSystem()->getAssig()->getType();
			else
				rt = 0;

			// If assigned, only export valid tags
			if( rt && rt->getAtom( s->getLabel().getTag() ) == 0 &&
				rt->getGroup( s->getLabel().getTag() ) == 0 )
				continue;

			at.d_spin = s->getId();
			at.d_shift = s->getShift( spec ); 
			at.d_label = s->getLabel().getTag();
			if( resi && s->getSystem() && s->getSystem()->getAssig() )
				at.d_sys = s->getSystem()->getAssig()->getId();
			else if( !resi && s->getSystem() )
				at.d_sys = s->getSystem()->getId();
			else
				at.d_sys = -1;
			pl->setAtom( i, at );
		}
		// TODO: Nachbarwerte bercksichten, wenn diese lokal fehlen
	}
	try
	{
		pl->writeToFile( fileName );
	}catch( Root::Exception& e )
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::critical( parent, "Export Atom List", e.what(), "&Cancel" );
		return;
	}
	QApplication::restoreOverrideCursor();

}

bool SpectrumListView::mapToType( QWidget* p, Spec::SpectrumType* st, Spec::SpectrumPeer* sp )
{
	assert( sp );
	assert( st );
	RotateDlg dlg( p, "Original Dimension Order", "Mapped", false ); 
	dlg.setCaption( "Map Original Spectrum to Spectrum Type" );
	Rotation rot = sp->getRotation();
	QString s1, s2;
	for( Dimension d = 0; d < sp->getDimCount(); d++ )
	{
		s1.sprintf( "%s (%s)", 
			sp->getSpectrum()->getScale( d ).getColor().getIsoLabel(), 
			sp->getSpectrum()->getLabel( d ) );
		s2.sprintf( "%s (%s)", st->getColor( d ).getIsoLabel(), st->getName( d ).data() );
		dlg.addDimension( s1, s2, sp->getSpectrum()->getScale( d ).getColor() );
	}
	if( dlg.rotate( rot ) )
	{
		sp->setRotation( rot );
		return true;
	}else
		return false;
}

bool SpectrumListView::importAlias(QWidget* w, PointSet* ps, Spec::Project* pro, Spec::Spectrum* spec )
{
	typedef std::map<Root::Index, PPM> AliasMap;
	AliasMap ali;
	AliasMap::const_iterator p2;

	typedef std::set<Root::Index> Ambig;
	Ambig::const_iterator p3;
	Ambig ambig, wrongColor;
	int unknown = 0;
	int undef = 0;
	int inval = 0;

	PointSet::Selection sel = ps->findAll();
	PointSet::Selection::const_iterator p1;
	Dimension d;
	SpinBase* base = pro->getSpins();
	Spin* spin;
	// Flle ali mit allen Assignments und Shifts der Peakliste
	const Dimension dim = ps->getDimCount();
	for( p1 = sel.begin(); p1 != sel.end(); ++p1 )
	{
		const PointSet::Assig& assig = ps->getAssig( (*p1) );
		bool doit = false;
		for( d = 0; d < dim; d++ )
			if( assig[ d ] != 0 )
			{
				doit = true;
				break;
			}
		if( doit )
		{
			const PeakPos& point = ps->getPoint( (*p1) );
			for( d = 0; d < dim; d++ )
			{
				if( assig[ d ] == 0 ) // Ignorieren
					undef++; 
				else if( assig[ d ] <= 0 )
					inval++;
				else if( ( spin = base->getSpin( assig[ d ] ) ) )
				{
					if( spin->getAtom() != ps->getColor( d ) )
						wrongColor.insert( assig[ d ] );
					p2 = ali.find( assig[ d ] );
					if( p2 != ali.end() )
					{
						// Duplikat gefunden
						if( ali[ assig[ d ] ] != point[ d ] )
						{
							// Anderer PPM-Wert als bestehender
							ambig.insert( assig[ d ] );
							//qDebug( "ERROR: Ambiguous shift %f (seen %f) for assignment %d",
							//	point[ d ], ali[ assig[ d ] ], assig[ d ] );
						}
					}else
					{
						// Neuen gefunden
						ali[ assig[ d ] ] = point[ d ]; 
					}
				}else
					unknown++;
			}
		}
	}
	QString str;
	if( !ambig.empty() )
	{
		str.sprintf( "Cannot import. There are %d ambiguous shifts for "
			"the following atom numbers (see message log):\n", 
			ambig.size() );
		for( p3 = ambig.begin(); p3 != ambig.end(); ++p3 )
		{
			str += QString().setNum( (*p3) );
			str += " ";
		}
		Root::MessageLog::inst()->warning( "Import Alias Shifts", str );
		QMessageBox::critical( w, "Import Alias Shifts", str, "&Cancel" );
		return false;
	}
	if( !wrongColor.empty() )
	{
		str.sprintf( "Cannot import. The %d following atom numbers "
			"have another atom type than the referenced spin (see message log):\n", 
			wrongColor.size() );
		for( p3 = wrongColor.begin(); p3 != wrongColor.end(); ++p3 )
		{
			str += QString().setNum( (*p3) );
			str += " ";
		}
		Root::MessageLog::inst()->warning( "Import Alias Shifts", str );
		QMessageBox::critical( w, "Import Alias Shifts", str, "&Cancel" );
		return false;
	}
	if( inval > 0 || unknown > 0 )
	{
		str.sprintf( "The peak list contains %d invalid and %d unknown atom numbers. "
			"Continue anyway?", inval, unknown );
		if( QMessageBox::warning( w, "Import Alias Shifts", str, "&Import", "&Cancel", QString::null, 1, 1 ) != 0 )		
			return false;	// Cancel
	}else if( !ali.empty() )
	{
		str.sprintf( "Do you really want to import %d alias shifts? Cannot be undone!", ali.size() );
		if( QMessageBox::warning( w, "Import Alias Shifts", str, "&Import", "&Cancel", QString::null, 1, 1 ) != 0 )		
			return false;	// Cancel
	}else
	{
		QMessageBox::information( w, "Import Alias Shifts", "Nothing to import!", "&Cancel" );
		return false;
	}
	for( p2 = ali.begin(); p2 != ali.end(); ++p2 )
	{
		spin = base->getSpin( (*p2).first );
		assert( spin );
		if( spin->getShift( spec ) != (*p2).second )
		{	// Kein Import, wenn bereits richtiges Alias.
			base->setShift( spin, (*p2).second, spec );
		}
	}
	return true;
}

bool SpectrumListView::importLinks(QWidget* w, PointSet* ps, Spec::Project* pro, Spec::Spectrum* spec )
{
	int unknown = 0;
	int undef = 0;
	int inval = 0;
	int good = 0;

	if( pro == 0 )
		return false;

	typedef std::set<Root::Index> Ambig;
	Ambig::const_iterator p3;
	Ambig wrongColor;

	PointSet::Selection sel = ps->findAll();
	PointSet::Selection::const_iterator p1;
	int d, k;
	SpinBase* base = pro->getSpins();
	// Flle ali mit allen Assignments und Shifts der Peakliste
	const int dim = ps->getDimCount();
	Spin* spin;
	for( p1 = sel.begin(); p1 != sel.end(); ++p1 )
	{
		const PointSet::Assig& assig = ps->getAssig( (*p1) );
		bool doit = false;
		for( d = 0; d < dim; d++ )
			if( assig[ d ] != 0 )
			{
				doit = true;
				break;
			}
		if( doit )
		{
			for( d = 0; d < dim; d++ )
			{
				if( assig[ d ] == 0 ) // Ignorieren
					undef++; 
				else if( assig[ d ] <= 0 )
					inval++;
				else 
				{
					spin = base->getSpin( assig[ d ] );
					if( spin == 0 )
						unknown++;
					else if( spin->getAtom() != ps->getColor( d ) )
						wrongColor.insert( assig[ d ] );
					else
						good++;
				}
			}
		}
	}
	QString str;
	if( !wrongColor.empty() )
	{
		str.sprintf( "Cannot import. The %d following atom numbers "
			"have another atom type than the referenced spin (see message log):\n", 
			wrongColor.size() );
		for( p3 = wrongColor.begin(); p3 != wrongColor.end(); ++p3 )
		{
			str += QString().setNum( (*p3) );
			str += " ";
		}
		Root::MessageLog::inst()->warning( "Import Alias Shifts", str );
		QMessageBox::critical( w, "Import Spin Links", str, "&Cancel" );
		return false;
	}
	if( good == 0 )
	{
		QMessageBox::information( w, "Import Spin Links", "Nothing to import!", "&Cancel" );
		return false;
	}
	if( inval > 0 || unknown > 0 )
	{
		str.sprintf( "The peak list contains %d invalid and %d unknown atom numbers. "
			"Continue anyway?", inval, unknown );
		if( QMessageBox::warning( w, "Import Spin Links", str, "&Import", "&Cancel", QString::null, 1, 1 ) != 0 )		
			return false;	// Cancel
	}
	Dlg::LinkParams params;
	params.d_atom = AtomType::H1;
	params.d_onlyInter = false;
	params.d_useShifts = false;
	params.d_hideOthers = true;
	if( !Dlg::getLinkParams( w, good, params ) )
		return false;

	if( spec && params.d_hideOthers )
	{
		SpinBase::SpinLinkSet::const_iterator p2;
		for( p2 = base->getLinks().begin(); p2 != base->getLinks().end(); ++p2 )
		{
			// Auf Wunsch alle brigen Links fr dieses Spektrum verstecken.
			base->setVisible( (*p2), false, spec );
		}
	}

	Root::Vector<Spin*> point( dim );
	SpinLink* link;
	Root::UInt8 code;
	bool created;
	for( p1 = sel.begin(); p1 != sel.end(); ++p1 )
	{
		const PointSet::Assig& assig = ps->getAssig( (*p1) );
		const PeakPos& shift = ps->getPoint( (*p1) );
		code = ps->getCode( (*p1) );
		bool doit = false;
		for( d = 0; d < dim; d++ )
			if( assig[ d ] != 0 )
			{
				doit = true;
				break;
			}
		if( doit )
		{
			for( d = 0; d < dim; d++ )
			{
				point[ d ] = base->getSpin( assig[ d ] );
				if( spec && params.d_useShifts && point[ d ] )
				{
					base->setShift( point[ d ], shift[ d ], spec );
				}
			}
			for( d = 0; d < dim; d++ )
			{
				if( point[ d ] )
				{
					for( k = d + 1; k < dim; k++ )
					{
						if( point[ k ] )
						{
							if( !params.d_atom.isNone() &&
								( point[ d ]->getAtom() != params.d_atom ||
								  point[ k ]->getAtom() != params.d_atom ) )
								// Wenn Atomtyp eingegrenzt und die Spins nicht passen
								// lasse das Paar aus.
								continue;
							if( params.d_onlyInter && point[ d ]->getSystem() &&
								point[ d ]->getSystem() == point[ k ]->getSystem() )
								// Wenn nur interresiduelle Links gesucht werden,
								// lasse links innerhalb desselben Systems aus.
								continue;
							link = point[ d ]->findLink( point[ k ] );
							created = false;
							if( link == 0 )
							{
								link = base->link( point[ d ], point[ k ] );
								created = true;
							}
							if( created && params.d_hideOthers )
								base->setVisible( link, false, 0 );
								// Die Default-Visibility wird auf false gesetzt fr 
								// importierte Links
							if( spec )
							{
								// TODO: rating
								base->setAlias( link, spec, 0, code, true );
							}
						}
					}
				}
			}
		}
	}
	return true;
}

void SpectrumListView::handle(Root::Message & msg)
{
	BEGIN_HANDLER();
	MESSAGE( SpectrumPeer::Added, a, msg )
	{
		addItem( a->sender() );
	}
	MESSAGE( SpectrumPeer::Removed, a, msg )
	{
		Gui::ListViewItem* i = findItem( a->sender() );
		if( i )
			i->removeMe();
	}
	MESSAGE( Root::Action, a, msg )
	{
		EXECUTE_ACTION( SpectrumListView, *a );
	}
	END_HANDLER();
}

void SpectrumListView::onCurrentChanged ()
{
	if( currentItem() )
	{
		Spectrum* spec = 0;
		if( currentItem()->parent() == 0 )
			spec = static_cast<_SpectrumListItem*>( currentItem() )->d_spec;
		else 
			spec = static_cast<_SpectrumListItem*>( currentItem()->parent() )->d_spec;
		LuaCaraExplorer::setCurrentSpec( spec );
	}
}

void SpectrumListView::handleAddSpectrum(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );
	
	SpectrumType* st = dynamic_cast<SpectrumType*>( a.getParam( 0 ).getObject() );
	if( st == 0 )
	{
		typedef std::map<QByteArray ,SpectrumType*> Sort;
		Sort sort;
		const Repository::SpectrumTypeMap& stm = d_rep->getSpecTypes();
		Repository::SpectrumTypeMap::const_iterator p;
		for( p = stm.begin(); p != stm.end(); ++p )
			sort[ (*p).first.data() ] = (*p).second;
		Sort::const_iterator q;
		QStringList names;
		QList<SpectrumType*> types;
		for( q = sort.begin(); q != sort.end(); ++q )
		{
			names.push_back((*q).second->getName().data());
			types.append((*q).second );
		}
		bool ok;
		const int selected = names.indexOf( QInputDialog::getItem( this, "Add Spectrum - CARA",
			"Select a spectrum type from the list:", names, 0, false, &ok ) );
		if( !ok || selected == -1 )
			return;
		else
			st = types[selected];
	}

#ifndef _WIN32
	// funktioniert nicht unter Windows 10; es werden keine Ordner angezeigt; s. http://forum.cara.nmr.ch/index.php?action=vthread&forum=2&topic=10
	QFileDialog dlg( this, "Select Spectrum Files", Root::AppAgent::getCurrentDir(),
		Spectrum::s_fileFilter );
	dlg.setFileMode( QFileDialog::ExistingFiles );

	if( dlg.exec() != QDialog::Accepted )
		return;

	const QStringList files = dlg.selectedFiles();
#else
	const QStringList files = QFileDialog::getOpenFileNames( this, "Select Spectrum Files",
															 Root::AppAgent::getCurrentDir(), Spectrum::s_fileFilter );
#endif
	QString str;
	QString log;
	QStringList::const_iterator it;
	QApplication::setOverrideCursor( Qt::waitCursor );
    //Root::UInt32 old = FileSpectrum::getMapThreshold();	// RISK
	//FileSpectrum::setMapThreshold( s_max );	// RISK Sicher mit Mapping ffnen
	for( it = files.begin(); it != files.end(); ++it )
	{
		QFileInfo info( (*it) );
        str.sprintf( "Opening %s", info.fileName().toLatin1().data() );
		Lexi::ShowStatusMessage msg( str );
		d_central->traverse( msg );
		try
		{
			Root::AppAgent::setCurrentDir( info.dirPath( true ) );
			Root::Ref<Spectrum> spec = Factory::createSpectrum( (*it) );
			if( spec == 0 )
			{
				str.sprintf( "Unknown spectrum format of %s\n", 
					info.fileName().toLatin1().data() );
				log += str;
				continue;
			}
			assert( st );
			SpecRef<SpectrumPeer> sp = new SpectrumPeer( spec );
			sp->setType( st );
			if( files.count() == 1 )
			{
				if( !sp->autoRotate( st ) || sp->ambiguousDims() )
				{
					QApplication::restoreOverrideCursor();
					if( !mapToType( this, st, sp ) )
					{
						//FileSpectrum::setMapThreshold( old );
						return;
					}
					QApplication::setOverrideCursor( Qt::waitCursor );
				}
			}else
			{
				if( !sp->autoRotate( st ) )
				{
					str.sprintf( "Could not automatically determine dimension mapping for %s\n",
						info.fileName().toLatin1().data() );
					log += str;
				}
			}
			// 10.8.14: neu als explizite Menfunktion, nicht mehr automatisch, ausser bei ...
            if( dynamic_cast<CaraSpectrum*>( spec.deref() ) )
                sp->calcLevels();
			d_pro->addSpectrum( sp );
            if( d_pro->findSpectrum( info.completeBaseName() ) )
                sp->setName( QString( "%1:%2" ).arg( info.completeBaseName() ).
                             arg( sp->getId() ).toLatin1() );
            else
                sp->setName( info.completeBaseName() );
			setSelected( findItem( sp ), true ); // TODO: funktioniert nicht. findItem gibt null zurck
		}catch( Root::Exception& e )
		{
			str.sprintf( "Error adding %s: %s\n",
				info.fileName().toLatin1().data(), e.what() );
			log += str;
		}
	}
	//FileSpectrum::setMapThreshold( old );
	QApplication::restoreOverrideCursor();
	if( !log.isEmpty() )
		QMessageBox::critical( this, "Adding Spectrum Files",
				log, "&OK" );
	Lexi::ShowStatusMessage msg( "Done" );
	d_central->traverse( msg );
}

extern void createMonoScope( Root::Agent* a, Spec::Spectrum* s, 
					 Spec::Project* p, PeakList* l, const Rotation& r );

void SpectrumListView::handleOpenSpectrum(Root::Action & a)
{
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 && 
		i->getSpec()->getDimCount() >= 2 );

	if( i->getSpec()->getDimCount() < 2 )
	{
		QMessageBox::critical( this, "Error opening spectrum", 
			"At least two dimensions required", "&Cancel" );
		return;
	}
	QApplication::setOverrideCursor( Qt::waitCursor );
	try
	{
		SpecRef<Spectrum> tmp = i->getSpec();
		createMonoScope( d_central, i->getSpec(), d_pro, 0, Rotation() );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Error opening spectrum", 
			e.what(), "&Cancel" );
	}
	QApplication::restoreOverrideCursor();
}

void SpectrumListView::handleRenameSpectrum(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );

	bool ok = false;
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
	QString res = QInputDialog::getText( "Change Name of Spectrum",
		"Enter Name:", QLineEdit::Normal, i->getSpec()->getName(), &ok, this );
	if( ok )
	{
		if( res.isEmpty() )
		{
			QMessageBox::critical( this, "Change Name of Spectrum", 
				"Invalid spectrum name", "&Cancel" );
			return;
		}
		Project::SpectrumMap::const_iterator p;
		for( p = d_pro->getSpectra().begin(); p != d_pro->getSpectra().end(); ++p )
		{
			if( res == (*p).second->getName() )
			{
				QMessageBox::critical( this, "Change Name of Spectrum", 
					"The selected name is not unique", "&Cancel" );
				return;
			}
		}
		i->getSpec()->setName( res );
		d_rep->touch();
    }
}

void SpectrumListView::handleCalcLevels(Root::Action & a)
{
    ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );

    if( QMessageBox::warning( this, "Calculate Levels",
		"Do you really want to recalculate min/max/threshold levels? This can take some time.",
		"&OK", "&Cancel" ) != 0 )
		return;
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
    QApplication::setOverrideCursor( Qt::waitCursor );
    i->getSpec()->calcLevels();
    QApplication::restoreOverrideCursor();
    d_rep->touch();
}

void SpectrumListView::handleRemoveSpectrum(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );

	if( QMessageBox::warning( this, "Remove Spectrum",
		"Do you really want to remove the spectrum (cannot be undone)?",
		"&OK", "&Cancel" ) != 0 )
		return;
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
	d_pro->removeSpectrum( i->getSpec() );
}

void createSynchroScope2(Root::Agent*, Spec::Spectrum*, Spec::Project*);

void SpectrumListView::handleOpenSynchroScope2(Action & a)
{
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0
		&& i->getSpec()->getDimCount() >= 2 && i->getSpec()->getDimCount() <= 3 );

	if( i->getSpec()->getDimCount() != 3 && i->getSpec()->getDimCount() != 2 )
	{
		QMessageBox::critical( this, "Error opening spectrum", 
			"Only two or three dimensional spectra supported!", "&Cancel" );
		return;
	}
 
	QApplication::setOverrideCursor( Qt::waitCursor );
	try
	{
		SpecRef<Spectrum> tmp = i->getSpec();
		createSynchroScope2( d_central, i->getSpec(), d_pro );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Error opening spectrum", 
			e.what(), "&Cancel" );
	}
	QApplication::restoreOverrideCursor();
}

void createHomoScope2(Root::Agent*, Spec::Spectrum*, Spec::Project*);

void SpectrumListView::handleOpenHomoScope(Root::Action & a)
{
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 &&
		i->getSpec()->getDimCount() == 2 );

	if( i->getSpec()->getDimCount() != 2 )
	{
		QMessageBox::critical( this, "Error opening spectrum", 
			"Only two dimensional spectra supported!", "&Cancel" );
		return;
	}
 
	QApplication::setOverrideCursor( Qt::waitCursor );
	try
	{
		SpecRef<Spectrum> tmp = i->getSpec();
		createHomoScope2( d_central, i->getSpec(), d_pro );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Error opening spectrum", 
			e.what(), "&Cancel" );
	}
	QApplication::restoreOverrideCursor();
}

void SpectrumListView::handleExportAtomList(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );

	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();

	exportAtomList( this, d_pro, i->getSpec() );
}

void SpectrumListView::handleExportMapperFile(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );

	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();

	QString fileName = QFileDialog::getSaveFileName( this,
		"Export Mapper File", Root::AppAgent::getCurrentDir(), 
        "Mapper Input File (*.fra)", 0, QFileDialog::DontConfirmOverwrite );
	if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );

	if( info.extension( false ).upper() != "FRA" )
		fileName += ".fra";
	info.setFile( fileName );
	if( info.exists() )
	{
		if( QMessageBox::warning( this, "Save As",
			"This file already exists. Do you want to overwrite it?",
			"&OK", "&Cancel" ) != 0 )
			return;
	}
	Root::AppAgent::setCurrentDir( info.dirPath( true ) );

	Root::SymbolString CA = "CA";
	Root::SymbolString CB = "CB";
	if( !Dlg::getLabelPair( this, CA, CB ) )
		return;
	Root::Ref<Fragmenter> f = new Fragmenter( d_pro->getSpins() );
	f->rebuildAll();
	if( !f->writeMapperFile( fileName, i->getSpec(), CA, CB ) )
		QMessageBox::critical( this, "Export Mapper File", 
			"Cannot write to selected file!", "&Cancel" );
}

void SpectrumListView::handleOpenSliceScope(Root::Action & a)
{
	return; // TODO

	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 &&
		i->getSpec()->getDimCount() == 1 );

	QApplication::setOverrideCursor( Qt::waitCursor );
	try
	{
		if( i->getSpec()->getDimCount() != 1 )
			throw Root::Exception( "Only one dimensional spectra supported!" );
 
		SpecRef<Spectrum> tmp = i->getSpec();
		new SliceScope( d_central, i->getSpec(), d_pro, 0 );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Error opening spectrum", e.what(), "&Cancel" );
	}
	QApplication::restoreOverrideCursor();
}

void SpectrumListView::handleEditFields(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();

	DynValueEditor::edit( this, d_rep->findObjectDef( Repository::keySpectrum ),
		i->getSpec() );
}

void SpectrumListView::handleImportAlias(Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();

	QString fileName = QFileDialog::getOpenFileName( this, "Open Peaklist", Root::AppAgent::getCurrentDir(), 
			"*.peaks" );
	if( fileName.isNull() ) 
		return;
	Root::Ref<PointSet> ps;
	try
	{
		ps = Factory::createEasyPeakList( fileName );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Error Opening Peaklist", e.what(), "&Cancel" );
		return;
	}

	importAlias( this, ps, d_pro, i->getSpec() );
}

void createPolyScope2(Root::Agent*, Spec::Spectrum*, Spec::Project*);
void createPolyScope3(Root::Agent*, Spec::Spectrum*, Spec::Project*);

void SpectrumListView::handleOpenPolyScope2(Root::Action & a)
{
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 &&
		i->getSpec()->getDimCount() >= 2 && i->getSpec()->getDimCount() <= 3 );

	if( i->getSpec()->getDimCount() != 3 && i->getSpec()->getDimCount() != 2 )
	{
		QMessageBox::critical( this, "Error opening spectrum", 
			"Only two or three dimensional spectra supported!", "&Cancel" );
		return;
	}
 
	QApplication::setOverrideCursor( Qt::waitCursor );
	try
	{
		SpecRef<Spectrum> tmp = i->getSpec();
        if( QApplication::keyboardModifiers() == Qt::ControlModifier )
            createPolyScope3( d_central, i->getSpec(), d_pro );
        else
            createPolyScope2( d_central, i->getSpec(), d_pro );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Error opening spectrum", 
			e.what(), "&Cancel" );
	}
	QApplication::restoreOverrideCursor();
}

void SpectrumListView::handleOpenPolyScope2Rot(Root::Action & a)
{
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 &&
		i->getSpec()->getDimCount() >= 2 && i->getSpec()->getDimCount() <= 3 );

	if( i->getSpec()->getDimCount() != 3 && i->getSpec()->getDimCount() != 2 )
	{
		QMessageBox::critical( this, "Error opening spectrum", 
			"Only two or three dimensional spectra supported!", "&Cancel" );
		return;
	}
	try
	{
		Spectrum* spec = i->getSpec();
		Rotation rot( spec->getDimCount() );	

		RotateDlg dlg( this, "Original", "View" );
		dlg.setCaption( "Open PolyScope" );
		QString s1, s2;
		for( Dimension d = 0; d < spec->getDimCount(); d++ )
		{
			rot[ d ] = d;
			s1.sprintf( "%s (%s)", 
				spec->getScale( d ).getColor().getIsoLabel(), 
				spec->getDimName( d ) );
			s2.sprintf( "%s", RotateDlg::stdLabel( d ) );
			dlg.addDimension( s1, s2 );
		}
		if( !dlg.rotate( rot ) )
			return;
		QApplication::setOverrideCursor( Qt::waitCursor );
		SpecRef<Spectrum> tmp = i->getSpec();
        if( QApplication::keyboardModifiers() == Qt::ControlModifier )
            createPolyScope3( d_central, new SpecRotator( spec, rot ), d_pro );
        else
            createPolyScope2( d_central, new SpecRotator( spec, rot ), d_pro );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Error opening spectrum", e.what(), "&Cancel" );
	}
	QApplication::restoreOverrideCursor();
}

void createFourDScope(Root::Agent*, Spec::Spectrum*, Spec::Project*);
void createFourDScope2(Root::Agent*, Spec::Spectrum*, Spec::Project*);
void createFourDScope3(Root::Agent*, Spec::Spectrum*, Spec::Project*);

void SpectrumListView::handleOpenFourDScope(Root::Action & a)
{
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 &&
		( i->getSpec()->getDimCount() == 2 || i->getSpec()->getDimCount() == 4 ) );

	if( i->getSpec()->getDimCount() != 4 && i->getSpec()->getDimCount() != 2 )
	{
		QMessageBox::critical( this, "Error opening spectrum",
			"Only two or four dimensional spectra supported!", "&Cancel" );
		return;
	}

	QApplication::setOverrideCursor( Qt::waitCursor );
#ifndef _DEBUG
	try
#endif
	{
		SpecRef<Spectrum> tmp = i->getSpec();
        if( QApplication::keyboardModifiers() == Qt::ShiftModifier )
            createFourDScope( d_central, i->getSpec(), d_pro );
        else if( QApplication::keyboardModifiers() == Qt::ControlModifier )
            createFourDScope2( d_central, i->getSpec(), d_pro );
        else
            createFourDScope3( d_central, i->getSpec(), d_pro );
    }
#ifndef _DEBUG
    catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Error opening spectrum",
			e.what(), "&Cancel" );
	}
#endif
    QApplication::restoreOverrideCursor();
}

void SpectrumListView::handleOpenFourDScopeRot(Root::Action & a)
{
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 &&
		( i->getSpec()->getDimCount() == 2 || i->getSpec()->getDimCount() == 4 ) );

	if( i->getSpec()->getDimCount() != 4 && i->getSpec()->getDimCount() != 2 )
	{
		QMessageBox::critical( this, "Error opening spectrum",
			"Only two or four dimensional spectra supported!", "&Cancel" );
		return;
	}
	try
	{
		Spectrum* spec = i->getSpec();
		Rotation rot( spec->getDimCount() );

		RotateDlg dlg( this, "Original", "View" );
		dlg.setCaption( "Open PolyScope" );
		QString s1, s2;
		for( Dimension d = 0; d < spec->getDimCount(); d++ )
		{
			rot[ d ] = d;
			s1.sprintf( "%s (%s)",
				spec->getScale( d ).getColor().getIsoLabel(),
				spec->getDimName( d ) );
			s2.sprintf( "%s", RotateDlg::stdLabel( d ) );
			dlg.addDimension( s1, s2 );
		}
		if( !dlg.rotate( rot ) )
			return;
		QApplication::setOverrideCursor( Qt::waitCursor );
		SpecRef<Spectrum> tmp = i->getSpec();
        if( QApplication::keyboardModifiers() == Qt::ShiftModifier )
            createFourDScope( d_central, new SpecRotator( spec, rot ), d_pro );
        else if( QApplication::keyboardModifiers() == Qt::ControlModifier )
            createFourDScope2( d_central, new SpecRotator( spec, rot ), d_pro );
        else
            createFourDScope3( d_central, new SpecRotator( spec, rot ), d_pro );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Error opening spectrum", e.what(), "&Cancel" );
	}
	QApplication::restoreOverrideCursor();
}

void SpectrumListView::handleWriteParamFile(Action & a)
{
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 &&
		i->getSpec()->canSave() );

	if( QMessageBox::warning( this, "Write Calibration",
		"Do you really want to write calibration to parameter file (cannot be undone)?"
		" The CARA file has to be explicitly saved afterwards!",
		"&OK", "&Cancel" ) != 0 )
		return;
	try
	{
		i->getSpec()->saveParamFile();
		i->clearChildren();
		fillSubs( i, i->getSpec() );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Error Writing Calibration", e.what(), "&Cancel" );
	}
}

void SpectrumListView::handleAddColumn(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );

	ObjectDef* od = d_rep->findObjectDef( Repository::keySpectrum );
	QStringList l;
	ObjectDef::Attributes::const_iterator i;
	for( i = od->getAttributes().begin(); i != od->getAttributes().end(); ++i )
		l.append( (*i).first.data() ); 
	l.sort();

	bool ok;
	QString res = QInputDialog::getItem( "Add Column", "Select an attribute:", 
		l, 0, false, &ok, this );
	if( !ok || res.isEmpty() )
		return;
	s_cols.push_back( res.toLatin1() );
	addColumn( res );
}

void SpectrumListView::handleRemoveCols(Root::Action & a)
{
	ACTION_ENABLED_IF( a, !s_cols.empty() );

    for( int i = 0; i < int(s_cols.size()); i++ )
		removeColumn( columns() - 1 );
	s_cols.clear();
}

void SpectrumListView::handleReplaceSpec(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );

	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();

	QString fileName = QFileDialog::getOpenFileName( this, "Replace Spectrum",
		Root::AppAgent::getCurrentDir(), 
        Spectrum::s_fileFilter );
    if( fileName.isNull() ) 
		return;
	
	QString str;
	QApplication::setOverrideCursor( Qt::waitCursor );
	QFileInfo info( fileName );
	try
	{
		Root::AppAgent::setCurrentDir( info.dirPath( true ) );
		Root::Ref<Spectrum> spec = Factory::createSpectrum( fileName );
		if( spec == 0 )
		{
			QApplication::restoreOverrideCursor();
			QMessageBox::critical( this, "Replace Spectrum", "Unknown spectrum format", 
				"&Cancel" );
			return;
		}
		i->d_spec->replaceSpectrum( spec );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Replace Spectrum", e.what(), 
			"&Cancel" );
	}
	QApplication::restoreOverrideCursor();
}

void SpectrumListView::handleShowTable(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );

	const Project::SpectrumMap& sm = d_pro->getSpectra();
	ObjectListView::ObjectList o( sm.size() );
	Project::SpectrumMap::const_iterator p;
	int i = 0;
	for( p = sm.begin(); p != sm.end(); ++p, ++i)
	{
		o[ i ] = (*p).second;
	}
	ObjectListView::edit( this, d_rep->findObjectDef( Repository::keySpectrum ), o );
}

void SpectrumListView::handleMapToType(Root::Action & a)
{
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );

	if( mapToType( this, i->getSpec()->getType(), i->getSpec() ) )
	{
		ListViewItem* j = addItem( i->getSpec() );
		i->removeMe();
		setSelected( j, true );
	}
}

void SpectrumListView::handleOpenMonoScopeRot(Root::Action & a)
{
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 && 
		i->getSpec()->getDimCount() >= 2 );

	Spectrum* spec = i->getSpec();
	Rotation rot( spec->getDimCount() );	

	RotateDlg dlg( this, "Original", "View" );
	dlg.setCaption( "Open MonoScope" );
	QString s1, s2;
	for( Dimension d = 0; d < spec->getDimCount(); d++ )
	{
		rot[ d ] = d;
		s1.sprintf( "%s (%s)", 
			spec->getScale( d ).getColor().getIsoLabel(), 
			spec->getDimName( d ) );
		s2.sprintf( "%s", RotateDlg::stdLabel( d ) );
		dlg.addDimension( s1, s2 );
	}
	if( !dlg.rotate( rot ) )
		return;
	QApplication::setOverrideCursor( Qt::waitCursor );
	try
	{
		SpecRef<Spectrum> tmp = i->getSpec();
		createMonoScope( d_central, i->getSpec(), d_pro, 0, rot );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Error opening spectrum", 
			e.what(), "&Cancel" );
	}
	QApplication::restoreOverrideCursor();
}

void SpectrumListView::handleOpenHomoScopeRot(Root::Action & a)
{
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 &&
		i->getSpec()->getDimCount() == 2 );

	if( i->getSpec()->getDimCount() != 2 )
	{
		QMessageBox::critical( this, "Error opening spectrum", 
			"Only two dimensional spectra supported!", "&Cancel" );
		return;
	}
	try
	{
		Spectrum* spec = i->getSpec();
		Rotation rot( spec->getDimCount() );	

		RotateDlg dlg( this, "Original", "View" );
		dlg.setCaption( "Open HomoScope" );
		QString s1, s2;
		for( Dimension d = 0; d < spec->getDimCount(); d++ )
		{
			rot[ d ] = d;
			s1.sprintf( "%s (%s)", 
				spec->getScale( d ).getColor().getIsoLabel(), 
				spec->getDimName( d ) );
			s2.sprintf( "%s", RotateDlg::stdLabel( d ) );
			dlg.addDimension( s1, s2 );
		}
		if( !dlg.rotate( rot ) )
			return;
		QApplication::setOverrideCursor( Qt::waitCursor );
		SpecRef<Spectrum> tmp = i->getSpec();
		createHomoScope2( d_central, new SpecRotator( spec, rot ), d_pro );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Error opening spectrum", e.what(), "&Cancel" );
	}
	QApplication::restoreOverrideCursor();
}

void SpectrumListView::handleCalibrate(Root::Action & a)
{
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );
		SpectrumPeer* spec = i->getSpec();

	PpmPoint p;
	const Rotation& rot = spec->getRotation();
	p.assign( spec->getDimCount(), 0 );
	Dimension d;
	for( d = 0; d < p.size(); d++ )
		p[ rot[ d ] ] = spec->getOffset( d );
	if( !Dlg::getPpmPoint( this, p, "Calibrate Spectrum" ) )
		return;
	for( d = 0; d < p.size(); d++ )
		spec->setOffset( d, p[ rot[ d ] ] );
	/*
	for( d = 0; d < p.size(); d++ )
		p[ rot[ d ] ] = p[ rot[ d ] ] - spec->getOffset( d );
	Root::Ref<SpecCalibrateCmd> cmd = new SpecCalibrateCmd( spec, p );
	cmd->handle( d_central );
	*/
	i->clearChildren();
	fillSubs( i, i->getSpec() );
}

void createSystemScope2(Root::Agent*, Spec::Project*, Spec::Spectrum* );

void SpectrumListView::handleOpenSystemScope2(Action & a)
{
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 &&
		i->getSpec()->getDimCount() > 2 );

	QApplication::setOverrideCursor( Qt::waitCursor );
	try
	{
		if( i->getSpec()->getDimCount() != 3 )
			throw Root::Exception( "Only three dimensional spectra supported!" );	// RISK
		SpecRef<Spectrum> tmp = i->getSpec();
		createSystemScope2( d_central, d_pro, tmp );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Error opening spectrum", e.what(), "&Cancel" );
	}
	QApplication::restoreOverrideCursor();
}

void SpectrumListView::handleOpenSystemScope2Rot(Root::Action & a)
{
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 &&
		i->getSpec()->getDimCount() == 3 );

	try
	{
		Spectrum* spec = i->getSpec();
		if( spec->getDimCount() != 3 )
			throw Root::Exception( "Only three dimensional spectra supported!" );
		Rotation rot( 3 );	// Vertausche die Anchor-Dimensionen X und Z

		RotateDlg dlg( this, "Original", "View" );
		dlg.setCaption( "Open SystemScope" );
		QString s1, s2;
		for( Dimension d = 0; d < spec->getDimCount(); d++ )
		{
			rot[ d ] = d;
			s1.sprintf( "%s (%s)", 
				spec->getScale( d ).getColor().getIsoLabel(), 
				spec->getDimName( d ) );
			s2.sprintf( "%s", RotateDlg::stdLabel( d ) );
			dlg.addDimension( s1, s2 );
		}
		if( !dlg.rotate( rot ) )
			return;
		QApplication::setOverrideCursor( Qt::waitCursor );
		SpecRef<Spectrum> tmp = i->getSpec();
		createSystemScope2( d_central, d_pro, new SpecRotator( i->getSpec(), rot ) );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Error opening spectrum", e.what(), "&Cancel" );
	}
	QApplication::restoreOverrideCursor();
}

void createStripScope2(Root::Agent*, Spec::Project*, Spec::Spectrum* );

void SpectrumListView::handleOpenStripScope2(Action & a)
{
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 &&
		i->getSpec()->getDimCount() > 2 );

	QApplication::setOverrideCursor( Qt::waitCursor );
	try
	{
		if( i->getSpec()->getDimCount() != 3 )
			throw Root::Exception( "Only three dimensional spectra supported!" );	// RISK
		SpecRef<Spectrum> tmp = i->getSpec();
		createStripScope2( d_central, d_pro, tmp );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Error opening spectrum", e.what(), "&Cancel" );
	}
	QApplication::restoreOverrideCursor();
}

void SpectrumListView::handleOpenStripScope2Rot(Root::Action & a)
{
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 &&
		i->getSpec()->getDimCount() == 3 );

	try
	{
		Spectrum* spec = i->getSpec();
		if( spec->getDimCount() != 3 )
			throw Root::Exception( "Only three dimensional spectra supported!" );
		Rotation rot( 3 );	// Vertausche die Anchor-Dimensionen X und Z

		RotateDlg dlg( this, "Original", "View" );
		dlg.setCaption( "Open StripScope" );
		QString s1, s2;
		for( Dimension d = 0; d < spec->getDimCount(); d++ )
		{
			rot[ d ] = d;
			s1.sprintf( "%s (%s)", 
				spec->getScale( d ).getColor().getIsoLabel(), 
				spec->getDimName( d ) );
			s2.sprintf( "%s", RotateDlg::stdLabel( d ) );
			dlg.addDimension( s1, s2 );
		}
		if( !dlg.rotate( rot ) )
			return;
		QApplication::setOverrideCursor( Qt::waitCursor );
		SpecRef<Spectrum> tmp = i->getSpec();
		createStripScope2( d_central, d_pro, new SpecRotator( i->getSpec(), rot ) );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Error opening spectrum", e.what(), "&Cancel" );
	}
	QApplication::restoreOverrideCursor();
}

void createStripScope2D(Root::Agent*, Spec::Project*, Spec::Spectrum* );

void SpectrumListView::handleOpenStripScope2D(Action & a)
{
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 &&
		i->getSpec()->getDimCount() == 2 );

	QApplication::setOverrideCursor( Qt::waitCursor );
	try
	{
		if( i->getSpec()->getDimCount() != 2 )
			throw Root::Exception( "Only two dimensional spectra supported!" );	// RISK
		SpecRef<Spectrum> tmp = i->getSpec();
		createStripScope2D( d_central, d_pro, tmp );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Error opening spectrum", e.what(), "&Cancel" );
	}
	QApplication::restoreOverrideCursor();
}

void SpectrumListView::handleOpenStripScope2DRot(Root::Action & a)
{
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 &&
		i->getSpec()->getDimCount() == 2 );

	try
	{
		Spectrum* spec = i->getSpec();
		if( spec->getDimCount() != 2 )
			throw Root::Exception( "Only two dimensional spectra supported!" );
		Rotation rot( spec->getDimCount() );	// Vertausche die Anchor-Dimensionen X und Z

		RotateDlg dlg( this, "Original", "View" );
		dlg.setCaption( "Open StripScope" );
		QString s1, s2;
		for( Dimension d = 0; d < spec->getDimCount(); d++ )
		{
			rot[ d ] = d;
			s1.sprintf( "%s (%s)", 
				spec->getScale( d ).getColor().getIsoLabel(), 
				spec->getDimName( d ) );
			s2.sprintf( "%s", RotateDlg::stdLabel( d ) );
			dlg.addDimension( s1, s2 );
		}
		if( !dlg.rotate( rot ) )
			return;
		QApplication::setOverrideCursor( Qt::waitCursor );
		SpecRef<Spectrum> tmp = i->getSpec();
		createStripScope2D( d_central, d_pro, new SpecRotator( i->getSpec(), rot ) );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Error opening spectrum", e.what(), "&Cancel" );
	}
	QApplication::restoreOverrideCursor();
}

void SpectrumListView::handleSetPeakWidth(Root::Action & a)
{

}

void SpectrumListView::handleDuplicateSpec(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );

	bool ok = false;
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
	QString res = QInputDialog::getText( "Duplicate Spectrum",
		"Enter Name:", QLineEdit::Normal, i->getSpec()->getName(), &ok, this );
	if( ok )
	{
		if( res.isEmpty() )
		{
			QMessageBox::critical( this, "Duplicate Spectrum", 
				"Invalid spectrum name", "&Cancel" );
			return;
		}
		Project::SpectrumMap::const_iterator p;
		for( p = d_pro->getSpectra().begin(); p != d_pro->getSpectra().end(); ++p )
		{
			if( res == (*p).second->getName() )
			{
				QMessageBox::critical( this, "Duplicate Spectrum", 
					"The selected name is not unique", "&Cancel" );
				return;
			}
		}
		Root::Ref<SpectrumPeer> spec = i->getSpec()->clone();
		spec->setName( res );
		d_pro->addSpectrum( spec );
		setSelected( findItem( spec ), true );
	}
}

void SpectrumListView::handleSetFolding(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() != 0 );

	_SpectrumDimItem* di = (_SpectrumDimItem*) currentItem();
	_SpectrumListItem* si = (_SpectrumListItem*) currentItem()->parent();

	QStringList l;
	l.append( "None" ); 
	l.append( "RSH" ); 
	l.append( "TPPI" ); 

	bool ok;
	QString res = QInputDialog::getItem( "Set Folding", "", 
		l, si->d_spec->getScale( di->d_dim ).getFolding(), false, &ok, this );
	if( !ok || res.isEmpty() )
		return;

	if( res == "RSH" )
		si->d_spec->setFolding( di->d_dim, Scale::RSH );
	else if( res == "TPPI" )
		si->d_spec->setFolding( di->d_dim, Scale::TPPI );
	else
		si->d_spec->setFolding( di->d_dim, Scale::Unfolded );
}

extern void createSitarViewer( Root::Agent* a, Spec::Spectrum* s );

void SpectrumListView::handleOpenSitar(Root::Action & a)
{
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 &&
		dynamic_cast<SitarSpectrum*>( i->getSpec()->getSpectrum() ) );

	QApplication::setOverrideCursor( Qt::waitCursor );
	try
	{
		SpecRef<SitarSpectrum> tmp = dynamic_cast<SitarSpectrum*>( i->getSpec()->getSpectrum() );
		createSitarViewer( d_central, tmp );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Error opening spectrum", 
			e.what(), "&Cancel" );
	}
	QApplication::restoreOverrideCursor();
}

void SpectrumListView::handleSetSample(Root::Action & a)
{
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 );

	_SpectrumListItem* item = (_SpectrumListItem*) currentItem();

	Gui::InputDlg dlg( this, "Assign Sample" );

	QComboBox _samps;
	_samps.addItem( "0 Default", 0 );
	Project::SampleMap::const_iterator i;
	for( i = d_pro->getSamples().begin(); i != d_pro->getSamples().end(); ++i )
	{
		_samps.addItem( QString( "%1 %2" ).arg( (*i).first ).
			arg( (*i).second->getName() ), (*i).first );
	}
	dlg.addLabel( "Select Sample:", 0, 0 );
	dlg.add( &_samps, 0, 1 );
	if( dlg.exec() )
	{
		int i = _samps.currentIndex();
		assert( i != -1 );
		Root::Index samp = _samps.itemData( i ).toInt();
		item->d_spec->setSample( d_pro->getSample( samp ) );
	}
}

void SpectrumListView::handleOpenTestScope( Root::Action& a )
{
	_SpectrumListItem* i = (_SpectrumListItem*) currentItem();
	ACTION_ENABLED_IF( a, currentItem() && currentItem()->parent() == 0 && 
        i->getSpec()->getDimCount() >= 2 );

    if( i->getSpec()->getDimCount() < 2 )
    {
        QMessageBox::critical( this, "Error opening spectrum",
            "At least two dimensions required", "&Cancel" );
        return;
    }
    QApplication::setOverrideCursor( Qt::waitCursor );
#ifndef _DEBUG
    try
#endif
	{
		SpecRef<Spectrum> tmp = i->getSpec();

		if( QApplication::keyboardModifiers() == Qt::ControlModifier )
		{
			Rotation rot( tmp->getDimCount() );

			RotateDlg dlg( this, "Original", "View" );
			dlg.setCaption( "Open TestScope" );
			QString s1, s2;
			for( Dimension d = 0; d < tmp->getDimCount(); d++ )
			{
				rot[ d ] = d;
				s1.sprintf( "%s (%s)",
					tmp->getScale( d ).getColor().getIsoLabel(),
					tmp->getDimName( d ) );
				s2.sprintf( "%s", RotateDlg::stdLabel( d ) );
				dlg.addDimension( s1, s2 );
			}
			if( !dlg.rotate( rot ) )
				return;
			tmp = new SpecRotator( tmp, rot );
		}
        new TestScope( tmp, d_pro );
    }
#ifndef _DEBUG
    catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Error opening spectrum", 
			e.what(), "&Cancel" );
	}
#endif
	QApplication::restoreOverrideCursor();
}



