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

#include "ReportViewer.h"
using namespace Spec;

// Qt
#include <qtextstream.h> 
#include <qinputdialog.h> 
#include <qmessagebox.h>
#include <QFileDialog>
#include <qdir.h> 
#include <qmenubar.h>
#include <qsplitter.h> 
#include <q3vbox.h>
#include <qcombobox.h> 
#include <Gui/ListView.h> 
#include <q3scrollview.h> 
#include <qcolordialog.h>
#include <qfontdialog.h>
#include <qdatastream.h>
#include <qfile.h>
#include <QFileDialog>
#include <QHideEvent>
#include <QPrintDialog>
#include <QSettings>

#include <Root/ActionHandler.h>
#include <Root/Command.h>
#include <Root/Application.h>
#include <Root/UpstreamFilter.h>
using namespace Root;

#include <Lexi/LayoutKit.h>
#include <QColor>
#include <Lexi/CommandLine.h>
#include <Lexi/Splitter.h>
#include <Lexi/Redirector.h>
#include <Lexi/Background.h>
#include <Lexi/Bevel.h>
#include <Lexi/Interactor.h>
#include <Lexi/ContextMenu.h>
#include <Lexi/Printer.h>
#include <Lexi/Border.h>

#include <Spec/SpecProjector.h>
#include <Spec/SpectrumType.h>
#include <Spec/SpectrumPeer.h>
#include <Spec/Repository.h>	// Wegen Color

#include <SpecView/CursorMdl.h>
#include <SpecView/ViewAreaMdl.h>
#include <SpecView/SliceView.h>
#include <SpecView/PeakPlaneView.h>
#include <SpecView/PeakModelView.h>
#include <SpecView/ScrollCtrl.h>
#include <SpecView/ZoomCtrl.h>
#include <SpecView/SelectZoomCtrl.h>
#include <SpecView/SpecRuler2D.h>
#include <SpecView/SpinLinkView.h>

#include <AidaApplication.h>

using namespace Lexi;
using namespace Spec;

static QColor g_bgClr = Qt::white;
static ReportViewer* s_viewer;
static ReportViewer::Setup* s_setup = 0;

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr ReportViewer::ContourParams = "ContourParams";
Root::Action::CmdStr ReportViewer::PageSize = "PageSize";
Root::Action::CmdStr ReportViewer::FrameSize = "FrameSize";
Root::Action::CmdStr ReportViewer::PosColor = "PosColor";
Root::Action::CmdStr ReportViewer::NegColor = "NegColor";
Root::Action::CmdStr ReportViewer::FrameColor = "FrameColor";
Root::Action::CmdStr ReportViewer::FrameWidth = "FrameWidth";
Root::Action::CmdStr ReportViewer::SetLandscape = "SetLandscape";
Root::Action::CmdStr ReportViewer::PpmRange = "PpmRange";
Root::Action::CmdStr ReportViewer::SetWidth = "SetWidth";
Root::Action::CmdStr ReportViewer::RulerFont = "RulerFont";
Root::Action::CmdStr ReportViewer::RulerColor = "RulerColor";
Root::Action::CmdStr ReportViewer::RulerMajor = "RulerMajor";
Root::Action::CmdStr ReportViewer::PeakFont = "PeakFont";
Root::Action::CmdStr ReportViewer::PeakColor = "PeakColor";
Root::Action::CmdStr ReportViewer::PeakWidth = "PeakWidth";
Root::Action::CmdStr ReportViewer::PeakOff = "PeakOff";
Root::Action::CmdStr ReportViewer::PeakAngle = "PeakAngle";
Root::Action::CmdStr ReportViewer::PeakShow = "PeakShow";
Root::Action::CmdStr ReportViewer::SpinFont = "SpinFont";
Root::Action::CmdStr ReportViewer::SpinColor = "SpinColor";
Root::Action::CmdStr ReportViewer::SpinWidth = "SpinWidth";
Root::Action::CmdStr ReportViewer::SpinOff = "SpinOff";
Root::Action::CmdStr ReportViewer::SpinAngle = "SpinAngle";
Root::Action::CmdStr ReportViewer::SpinShow = "SpinShow";
Root::Action::CmdStr ReportViewer::ViewSpinLabels = "ViewSpinLabels";
Root::Action::CmdStr ReportViewer::RulerThick = "RulerThick";
Root::Action::CmdStr ReportViewer::ShowCenter = "ShowCenter";
Root::Action::CmdStr ReportViewer::CenterColor = "CenterColor";
Root::Action::CmdStr ReportViewer::LinkAdd = "LinkAdd";
Root::Action::CmdStr ReportViewer::LinkClear = "LinkClear";
Root::Action::CmdStr ReportViewer::LinkColor = "LinkColor";
Root::Action::CmdStr ReportViewer::LinkThick = "LinkThick";
Root::Action::CmdStr ReportViewer::LinkVerti = "LinkVerti";
Root::Action::CmdStr ReportViewer::FitToPage = "FitToPage";
Root::Action::CmdStr ReportViewer::RulerShowH = "RulerShowH";
Root::Action::CmdStr ReportViewer::RulerShowV = "RulerShowV";
Root::Action::CmdStr ReportViewer::SaveSettings = "SaveSettings";
Root::Action::CmdStr ReportViewer::LoadSettings = "LoadSettings";
Root::Action::CmdStr ReportViewer::TitleFont = "TitleFont";
Root::Action::CmdStr ReportViewer::TitleColor = "TitleColor";
Root::Action::CmdStr ReportViewer::TitleText = "TitleText";
Root::Action::CmdStr ReportViewer::ShowTitle = "ShowTitle";
Root::Action::CmdStr ReportViewer::AutoContour = "AutoContour";
Root::Action::CmdStr ReportViewer::AutoGain = "AutoGain";
Root::Action::CmdStr ReportViewer::AnchLabel = "AnchLabel";
Root::Action::CmdStr ReportViewer::SpinCross = "SpinCross";
Root::Action::CmdStr ReportViewer::ShowFolded = "ShowFolded";
Root::Action::CmdStr ReportViewer::ExportPdf = "ExportPdf";
Root::Action::CmdStr ReportViewer::ExportSvg = "ExportSvg";

ACTION_SLOTS_BEGIN( ReportViewer )
    { ReportViewer::ExportSvg, &ReportViewer::handleExportSvg },
    { ReportViewer::ExportPdf, &ReportViewer::handleExportPdf },
    { ReportViewer::ShowFolded, &ReportViewer::handleShowFolded },
    { ReportViewer::SpinCross, &ReportViewer::handleSpinCross },
    { ReportViewer::AnchLabel, &ReportViewer::handleAnchLabel },
    { ReportViewer::AutoContour, &ReportViewer::handleAutoContour },
    { ReportViewer::AutoGain, &ReportViewer::handleAutoGain },
    { ReportViewer::ShowTitle, &ReportViewer::handleShowTitle },
    { ReportViewer::TitleFont, &ReportViewer::handleTitleFont },
    { ReportViewer::TitleColor, &ReportViewer::handleTitleColor },
    { ReportViewer::TitleText, &ReportViewer::handleTitleText },
    { ReportViewer::SaveSettings, &ReportViewer::handleSaveSettings },
    { ReportViewer::LoadSettings, &ReportViewer::handleLoadSettings },
    { ReportViewer::RulerShowH, &ReportViewer::handleRulerShowH },
    { ReportViewer::RulerShowV, &ReportViewer::handleRulerShowV },
    { ReportViewer::FitToPage, &ReportViewer::handleFitToPage },
    { ReportViewer::LinkAdd, &ReportViewer::handleLinkAdd },
    { ReportViewer::LinkClear, &ReportViewer::handleLinkClear },
    { ReportViewer::LinkColor, &ReportViewer::handleLinkColor },
    { ReportViewer::LinkThick, &ReportViewer::handleLinkThick },
    { ReportViewer::LinkVerti, &ReportViewer::handleLinkVerti },
    { ReportViewer::ShowCenter, &ReportViewer::handleShowCenter },
    { ReportViewer::CenterColor, &ReportViewer::handleCenterColor },
    { ReportViewer::RulerThick, &ReportViewer::handleRulerThick },
    { ReportViewer::ViewSpinLabels, &ReportViewer::handleViewSpinLabels },
    { ReportViewer::SpinShow, &ReportViewer::handleSpinShow },
    { ReportViewer::SpinAngle, &ReportViewer::handleSpinAngle },
    { ReportViewer::SpinOff, &ReportViewer::handleSpinOff },
    { ReportViewer::SpinWidth, &ReportViewer::handleSpinWidth },
    { ReportViewer::SpinColor, &ReportViewer::handleSpinColor },
    { ReportViewer::SpinFont, &ReportViewer::handleSpinFont },
    { ReportViewer::PeakShow, &ReportViewer::handlePeakShow },
    { ReportViewer::PeakAngle, &ReportViewer::handlePeakAngle },
    { ReportViewer::PeakOff, &ReportViewer::handlePeakOff },
    { ReportViewer::PeakWidth, &ReportViewer::handlePeakWidth },
    { ReportViewer::PeakColor, &ReportViewer::handlePeakColor },
    { ReportViewer::PeakFont, &ReportViewer::handlePeakFont },
    { ReportViewer::RulerColor, &ReportViewer::handleRulerColor },
    { ReportViewer::RulerFont, &ReportViewer::handleRulerFont },
    { ReportViewer::RulerColor, &ReportViewer::handleRulerColor },
    { ReportViewer::RulerMajor, &ReportViewer::handleRulerMajor },
    { ReportViewer::SetWidth, &ReportViewer::handleSetWidth },
    { ReportViewer::PpmRange, &ReportViewer::handlePpmRange },
    { ReportViewer::FrameSize, &ReportViewer::handleFrameSize },
    { ReportViewer::PosColor, &ReportViewer::handlePosColor },
    { ReportViewer::NegColor, &ReportViewer::handleNegColor },
    { ReportViewer::FrameColor, &ReportViewer::handleFrameColor },
    { ReportViewer::FrameWidth, &ReportViewer::handleFrameWidth },
    { ReportViewer::SetLandscape, &ReportViewer::handleSetLandscape },
    { ReportViewer::PageSize, &ReportViewer::handlePageSize },
    { Root::Action::EditCopy, &ReportViewer::handleEditCopy },
    { Root::Action::FilePrint, &ReportViewer::handleFilePrint },
    { ReportViewer::ContourParams, &ReportViewer::handleContourParams },
ACTION_SLOTS_END( ReportViewer )

//////////////////////////////////////////////////////////////////////



ReportViewer::Setup::Setup()
{
	d_pageSize = 0;
	d_landscape = false;
	d_frameW = 0;
	d_frameH = 0;
	d_frameLw = 20;
	d_frameClr = ( Qt::black );

	QColor black = ( Qt::black );
	QColor gray = ( Qt::gray );
	d_posClr.assign( ClrTableSize, Qt::black );
	d_negClr.assign( ClrTableSize, Qt::black );
	for( int i = 0; i < ClrTableSize; i++ )
	{
		d_posClr[ i ] = black;
		d_negClr[ i ] = gray;
	}
	d_contLw = 20;

	d_rulFont.setPointSize( 200 / TwipsPerPoint );
	d_rulClr = ( Qt::black );
	d_space[ DimY ] = 1000;
	d_space[ DimX ] = 1300;
	d_count[ DimY ] = d_count[ DimX ] = 5;
	d_rulLw = 20;
	d_showH = true;
	d_showV = true;
	d_anchLabel = false;

	d_peakFont.setPointSize( 160 / TwipsPerPoint );
	d_peakClr = ( Qt::red );
	d_peakLw = 20;
	d_peakOff = 0;
	d_peakAng = 45;
	d_showPeak = true;

	d_spinFont.setPointSize( 160 / TwipsPerPoint );
	d_spinClr = ( Qt::red );
	d_spinLw = 20;
	d_spinOff = 0;
	d_spinAng = 45;
	d_showSpin = true;
	d_spinLbl = SpinPointView::PairIdLabelSysOrResi;
	d_showCross = true;

	d_linkClr = ( Qt::darkRed );
	d_linkLw = 20;
	d_showVerti = true;
	d_showCenter = true;

	d_centClr = ( Qt::gray );

	d_titleClr = ( Qt::black );
	d_titleFont.setPointSize( 320 / TwipsPerPoint );
	d_showTitle = true;

	d_showFolded = false;
}

static const qint32 s_magic = 302166938;
static const qint32 s_version = 6;

void ReportViewer::Setup::save(const char * path)
{
	QFile f( path );
	if( !f.open( QIODevice::WriteOnly ) )
		throw Root::Exception( "Could not open/write file ", path );
	QDataStream os( &f );                        
	os << s_magic; // 1
	os << s_version; // 2

	os << qint32( d_pageSize ); // 3
	os << qint32( d_landscape ); // 4
	os << d_frameW; // 5
	os << d_frameH; // 6
	os << d_frameLw; // 7
	os << d_frameClr; // 8
	os << qint32( d_posClr.size() ); // 9
	for( long j = 0; j < d_posClr.size(); j++ )
	{
		os << QColor( d_posClr[j] ); // 10
		os << QColor( d_negClr[j] ); // 11
	}
	os << d_contLw; // 12
	os << d_rulFont; // 13
	os << ( d_rulClr ); // 14
	os << d_space[ DimX ]; // 15
	os << d_space[ DimY ]; // 16
	os << d_count[ DimX ]; // 17
	os << d_count[ DimY ]; // 18
	os << d_rulLw; // 19
	os << qint32( d_showH ); // 20
	os << qint32( d_showV ); // 21
	os << qint32( d_anchLabel ); // 22
	os << d_peakFont; // 23
	os << ( d_peakClr ); // 24
	os << d_peakLw; // 25
	os << d_peakOff; // 26
	os << d_peakAng; // 27
	os << qint32( d_showPeak ); // 28
	os << d_spinFont; // 29
	os << ( d_spinClr ); // 30
	os << d_spinLw; // 31
	os << d_spinOff; // 32
	os << d_spinAng; // 33
	os << qint32( d_showSpin ); // 34
	os << qint32( d_spinLbl ); // 35
	os << qint32( d_links.size() ); // 36
	Root::SymbolSet::const_iterator i;
	for( i = d_links.begin(); i != d_links.end(); ++i )
		os << (*i).data(); // 37
	os << ( d_linkClr ); // 38
	os << d_linkLw; // 39
	os << qint32( d_showVerti ); // 40
	os << qint32( d_showCenter ); // 41
	os << ( d_centClr ); // 42
	os << d_range[ DimX ].first; // 43
	os << d_range[ DimX ].second; // 44
	os << d_range[ DimY ].first; // 45
	os << d_range[ DimY ].second; // 46
	os << d_titleFont; // 47
	os << ( d_titleClr ); // 48
	os << qint32( d_showTitle ); // 49
	os << qint32( d_showCross ); // 50
	os << qint32( d_showFolded ); // 51
}

void ReportViewer::Setup::save2(const char * path)
{
    QSettings out( path, QSettings::IniFormat );

    if( !out.isWritable() )
        throw Root::Exception( "Could not open/write file ", path );

    out.setValue( "magic", s_magic );
    out.setValue( "version", s_version );
    out.setValue( "pageSize", d_pageSize );
    out.setValue( "landscape", d_landscape );
    out.setValue( "frameW", d_frameW );
    out.setValue( "frameH", d_frameH );
    out.setValue( "frameLw", d_frameLw );
    out.setValue( "frameClr", d_frameClr );

    out.beginWriteArray( "posNegClr" );
    int n;
	for( n = 0; n < d_posClr.size(); n++ )
	{
        out.setArrayIndex( n );
        out.setValue( "posClr", d_posClr[n] );
        out.setValue( "negClr", d_negClr[n] );
	}
    out.endArray();

    out.setValue( "contLw", d_contLw );
    out.setValue( "rulFont", d_rulFont );
    out.setValue( "rulClr", d_rulClr );
    out.setValue( "spaceH", d_space[ DimX ] );
    out.setValue( "spaceV", d_space[ DimY ] );
    out.setValue( "countH", d_count[ DimX ] );
    out.setValue( "countV", d_count[ DimY ] );
    out.setValue( "rulLw", d_rulLw );
    out.setValue( "showH", d_showH );
    out.setValue( "showV", d_showV );
    out.setValue( "anchLabel", d_anchLabel );
    out.setValue( "peakFont", d_peakFont );
    out.setValue( "peakClr", d_peakClr );
    out.setValue( "peakLw", d_peakLw );
    out.setValue( "peakOff", d_peakOff );
    out.setValue( "peakAng", d_peakAng );
    out.setValue( "showPeak", d_showPeak );
    out.setValue( "spinFont", d_spinFont );
    out.setValue( "spinClr", d_spinClr );
    out.setValue( "spinLw", d_spinLw );
    out.setValue( "spinOff", d_spinOff );
    out.setValue( "spinAng", d_spinAng );
    out.setValue( "showSpin", d_showSpin );
    out.setValue( "spinLbl", d_spinLbl );


    out.beginWriteArray( "links" );
	Root::SymbolSet::const_iterator i;
    n = 0;
	for( i = d_links.begin(); i != d_links.end(); ++i, n++ )
    {
        out.setArrayIndex( n );
        out.setValue( "link", (*i).data() );
    }
    out.endArray();
    out.setValue( "linkClr", d_linkClr );
    out.setValue( "linkLw", d_linkLw );
    out.setValue( "showVerti", d_showVerti );
    out.setValue( "showCenter", d_showCenter );
    out.setValue( "centClr", d_centClr );
    out.setValue( "rangeHfrom", d_range[ DimX ].first );
    out.setValue( "rangeHto", d_range[ DimX ].second );
    out.setValue( "rangeVfrom", d_range[ DimY ].first );
    out.setValue( "rangeVto", d_range[ DimY ].second );
    out.setValue( "titleFont", d_titleFont );
    out.setValue( "titleClr", d_titleClr );
    out.setValue( "showTitle", d_showTitle );
    out.setValue( "showCross", d_showCross );
    out.setValue( "showFolded", d_showFolded );
}

void ReportViewer::Setup::load(const char * path)
{
	QFile f( path );
	if( !f.open( QIODevice::ReadOnly ) ) 
		throw Root::Exception( "Could not open/read file ", path );
	QDataStream is( &f );                        // serialize using f
    is.setVersion( QDataStream::Qt_4_2 );

	qint32 magic;
	qint32 version;
	is >> magic; // 1
	if( magic != s_magic )
		throw Root::Exception( "Invalid print preview setup file format" );
	is >> version; // 2
	if( version < 6 || version > s_version )
		throw Root::Exception( "Invalid print preview setup file version" );

	QColor clr;
	QFont ft;
	qint32 tmp;
	is >> tmp; // 3
	d_pageSize = tmp;
	is >> tmp; // 4
	d_landscape = tmp != 0; 
	is >> d_frameW; // 5
	is >> d_frameH; // 6
	is >> d_frameLw; // 7
	is >> d_frameClr; // 8
	if( version < 4 )
	{
		is >> d_posClr[0];
		is >> d_negClr[0];
	}else
	{
		qint32 count;
		is >> count; // 9
		for( long j = 0; j < count; j++ )
		{
			is >> clr; // 10
			if( j < ClrTableSize )
				d_posClr[j] = clr;
			is >> clr; // 11
			if( j < ClrTableSize )
				d_negClr[j] = clr;
		}
	}
	is >> d_contLw; // 12
	is >> d_rulFont; // 13
	is >> d_rulClr; // 14
	is >> d_space[ DimX ]; // 15
	is >> d_space[ DimY ]; // 16
	is >> d_count[ DimX ]; // 17
	is >> d_count[ DimY ]; // 18
	is >> d_rulLw; // 19
	is >> tmp; // 20
	d_showH = tmp != 0;
	is >> tmp; // 21
	d_showV = tmp != 0;
	if( version >= 3 )
	{
		is >> tmp; // 22
		d_anchLabel = tmp != 0;
	}
	is >> d_peakFont; // 23
	is >> d_peakClr; // 24
	is >> d_peakLw; // 25
	is >> d_peakOff; // 26
	is >> d_peakAng; // 27
	is >> tmp; // 28
	d_showPeak = tmp != 0;
	is >> d_spinFont; // 29
	is >> d_spinClr; // 30
	is >> d_spinLw; // 31
	is >> d_spinOff; // 32
	is >> d_spinAng; // 33
	is >> tmp; // 34
	d_showSpin = tmp != 0;
	is >> tmp; // 35
	d_spinLbl = (SpinPointView::Label) tmp;
	qint32 count;
	is >> count; // 36
	d_links.clear();
	char* str;
	for( long i = 0; i < count; ++i )
	{
		is >> str; // 37
		d_links.insert( str );
		delete[] str;
	}
	is >> d_linkClr; // 38
	is >> d_linkLw; // 39
	is >> tmp; // 40
	d_showVerti = tmp != 0;
	is >> tmp; // 41
	d_showCenter = tmp != 0;
	is >> d_centClr; // 42
	is >> d_range[ DimX ].first; // 43
	is >> d_range[ DimX ].second; // 44
	is >> d_range[ DimY ].first; // 45
	is >> d_range[ DimY ].second; // 46

	if( version >= 2 )
	{
		is >> d_titleFont; // 47
		is >> d_titleClr; // 48
		is >> tmp; // 49
		d_showTitle = tmp != 0;
	}
	if( version >= 5 )
	{
		is >> tmp; // 50
		d_showCross = tmp != 0;
		is >> tmp; // 51
		d_showFolded = tmp != 0;
	}
}

void ReportViewer::Setup::load2( const char* path )
{
    QSettings in( path, QSettings::IniFormat );

    if( in.status() != QSettings::NoError )
		throw Root::Exception( "Could not open/read file ", path );

    if( in.value( "magic" ).toUInt() != s_magic )
		throw Root::Exception( "Invalid print preview setup file format" );
    if( in.value( "version" ).toUInt() != s_version )
		throw Root::Exception( "Invalid print preview setup file version" );

#define _READ( name, to, type ) if( in.contains( name ) ) to = in.value( name ).value<type>();

    _READ( "pageSize", d_pageSize, int );
    _READ( "landscape", d_landscape, bool );
    _READ( "frameW", d_frameW, qint32 );
    _READ( "frameH", d_frameH, qint32 );
    _READ( "frameLw", d_frameLw, qint32 );
    _READ( "frameClr", d_frameClr, QColor );

    quint32 count = in.beginReadArray( "posNegClr" );
    quint32 n;
    for( n = 0; n < count && n < ClrTableSize; n++ )
    {
        in.setArrayIndex(n);
        _READ( "posClr", d_posClr[n], QColor );
        _READ( "negClr", d_negClr[n], QColor );
    }
    in.endArray();

    _READ( "contLw", d_contLw, qint32 );
    _READ( "rulFont", d_rulFont, QFont );
    _READ( "rulClr", d_rulClr, QColor );
    _READ( "spaceH",  d_space[ DimX ], qint32 );
    _READ( "spaceV",  d_space[ DimY ], qint32 );
    _READ( "countH",  d_count[ DimY ], quint8 );
    _READ( "countV",  d_count[ DimY ], quint8 );
    _READ( "rulLw",  d_rulLw, qint32 );
    _READ( "showH",  d_showH, bool );
    _READ( "showV",  d_showV, bool );
    _READ( "anchLabel",  d_anchLabel, bool );
    _READ( "peakFont",  d_peakFont, QFont );
    _READ( "peakClr",  d_peakClr, QColor );
    _READ( "peakLw",  d_peakLw, qint32 );
    _READ( "peakOff",  d_peakOff, qint32 );
    _READ( "peakAng",  d_peakAng, short );
    _READ( "showPeak",  d_showPeak, bool );
    _READ( "spinFont",  d_spinFont, QFont );
    _READ( "spinClr",  d_spinClr, QColor );
    _READ( "spinLw",  d_spinLw, qint32 );
    _READ( "spinOff",  d_spinOff, qint32 );
    _READ( "spinAng",  d_spinAng, short );
    _READ( "showSpin",  d_showSpin, bool );
    _READ( "spinLbl",  count, quint32 );
    d_spinLbl = (SpinPointView::Label) count;

    count = in.beginReadArray( "links" );
    for( n = 0; n < count; n++ )
    {
        in.setArrayIndex(n);
        QString str;
        _READ( "link", str, QString );
        d_links.insert( str );
    }
    in.endArray();

    _READ( "linkClr",  d_linkClr, QColor );
    _READ( "linkLw",  d_linkLw, qint32 );
    _READ( "showVerti",  d_showVerti, bool );
    _READ( "showCenter",  d_showCenter, bool );
    _READ( "centClr",  d_centClr, QColor );

    _READ( "rangeHfrom",  d_range[ DimX ].first, PPM );
    _READ( "rangeHto",  d_range[ DimX ].second, PPM );
    _READ( "rangeVfrom",  d_range[ DimY ].first, PPM );
    _READ( "rangeVto",  d_range[ DimY ].second, PPM );

    _READ( "titleFont",  d_titleFont, QFont );
    _READ( "titleClr",  d_titleClr, QColor );
    _READ( "showTitle",  d_showTitle, bool );
    _READ( "showCross",  d_showCross, bool );
    _READ( "showFolded",  d_showFolded, bool );
}

// TODO: mit Lexi::hCenter und vCenter lsen.
class _CenterBox : public MonoGlyph
{
public:
	_CenterBox( Glyph* body ):MonoGlyph( body ) {}

	void request( Requisition& r )
	{
		MonoGlyph::request( r );
		d_req = r;
	}
	void allocate( const Allocation& a)
	{
		Allocation aa;
		center( a, aa );
		MonoGlyph::allocate( aa );
	}
	void draw(Canvas& v, const Allocation& a)
	{
		Allocation aa;
		center( a, aa );
		MonoGlyph::draw( v, aa );
	}
	void pick( Twips x, Twips y, const Allocation& a, Trace& t)
	{
		Allocation aa;
		center( a, aa );
		MonoGlyph::pick( x,y,aa, t );
	}
	void traverse( const Allocation& a, GlyphTraversal& t)
	{
		Allocation aa;
		center( a, aa );
		MonoGlyph::traverse( aa, t );
	}
private:
	void center( const Allocation& in, Allocation& out )
	{
		for( Dimension d = 0; d <= DimY; d++ )
		{
			const Requirement& r = d_req.getRequirement( d );
			const Allotment& i = in.getAllotment( d );
			Allotment& o = out.getAllotment( d );
			o.setSpan( r.getNatural() );
			o.setOrigin( i.getOrigin() + ( i.getSpan() - o.getSpan() ) / 2 );
		}
	}
	Requisition d_req;
};

ReportViewer::ReportViewer(Root::Agent* a,const Dlg::ContourParams& p,
						   float ag, bool ac, bool fold, Repository* rep ):
	Q3MainWindow( 0 ), d_legende(0), d_params( p ), d_title( 0 )
{
	setAttribute( Qt::WA_DeleteOnClose );
	d_gain = ag;
	d_autoContour = ac;

	if( s_setup == 0 )
		s_setup = new Setup();	// Dieses Objekt wird erst bei Quit dealloziiert.
	d_sup = s_setup;

	if( rep )
	{
		const Repository::SlotColors& sc = rep->getScreenClr();
		for( int i = 0; i < d_sup->d_posClr.size() && i < sc.size(); i++ )
		{
			d_sup->d_posClr[ i ] = sc[ i ].d_pos;
			d_sup->d_negClr[ i ] = sc[ i ].d_neg;
		}
	}
	d_sup->d_showFolded = fold;

	if( !g_bgClr.isValid() )
		g_bgClr = ( Qt::white );

	setCaption( "CARA Print Preview" );

	d_focus = new FocusManager( nil, true );

	d_scroller = new Q3ScrollView( this );
	setCentralWidget( d_scroller );
	d_widget = new GlyphWidget( this, 
		new Background( d_focus, g_bgClr, true ), 0, true );
	d_scroller->addChild( d_widget );

	setPageSize( d_sup->d_pageSize );
	d_widget->setFocusGlyph( d_focus );

	buildMenues();
	resize( 600, 400 ); // RISK
	//showMaximized();
	show();
}
 
ReportViewer::~ReportViewer()
{
	// Wieso? if( s_viewer == this )
		s_viewer = 0;
}

void ReportViewer::kill()
{
	if( s_viewer )
		delete s_viewer;
}

ReportViewer* ReportViewer::getViewer( Root::Agent*, const Dlg::ContourParams& p,
									  float ag, bool ac, bool fold, Repository* r)
{
	if( s_viewer == 0 )
	{
		s_viewer = new ReportViewer( 0, p, ag, ac, fold, r );
		s_viewer->showMaximized();
	}else
		s_viewer->show();
	s_viewer->raise();
	return s_viewer;
}

void ReportViewer::hideEvent(QHideEvent * e)
{
	/* 
	RISK
	Wenn ich mit Overlay arbeite und ein Setup lade, strzt er mit er Zeit,
	falls dieser Code ausgefhrt wird. Es gibt einen Error fr zweimal lschen.
	So wird halt etwas Speicher unrig behalten, aber sonst passiert nichts.
	d_strips.assign( 1, Strip() );
	d_rules.clear();
	d_focus->setBody( 0 );
	*/
	Q3MainWindow::hideEvent( e );
}

void ReportViewer::showPlane( ViewAreaMdl* a, const Spector& specs, 
							 PeakSpace* pa, SpinSpace* ss )
{
	d_strips.assign( 1, Strip() );
	d_rules.clear();
	Strip& s = d_strips[0];

	assert( !specs.empty() && specs[0]->getDimCount() == 2 );
	Root::Ref<ViewAreaMdl> mdl = new ViewAreaMdl( true, true, true, true );
	mdl->addObserver( this );
	mdl->setRange( a->getRange( DimX ), a->getRange( DimY ) );
	s.d_viewer = new SpecViewer( mdl );

	s.d_viewer->getHandlers()->append( new ZoomCtrl( s.d_viewer, true, true ) );
	s.d_viewer->getHandlers()->append( new SelectZoomCtrl( s.d_viewer, true, true ) );
	s.d_viewer->getHandlers()->append( new ScrollCtrl( s.d_viewer ) );
	if( pa )
	{
		d_peaks = new PeakPlaneView( s.d_viewer, pa, d_sup->d_peakClr,
			&d_sup->d_peakFont );
		d_peaks->setWidth( d_sup->d_peakLw );
		d_peaks->setOff( d_sup->d_peakOff );
		d_peaks->setAngle( d_sup->d_peakAng );
		d_peaks->show( d_sup->d_showPeak );
	}else
		d_peaks = 0;
	if( ss )
	{
		s.d_spins = new SpinPointView( s.d_viewer, ss, d_sup->d_spinClr,
			&d_sup->d_spinFont );
		s.d_spins->setWidth( d_sup->d_spinLw );
		s.d_spins->setOff( d_sup->d_spinOff );
		s.d_spins->setAngle( d_sup->d_spinAng );
		s.d_spins->show( d_sup->d_showSpin );
		s.d_spins->setLabel( d_sup->d_spinLbl, DimY );
		s.d_spins->showGhost( false );
		// TODO showGhost
		// TODO ghostLabel
	}else
		s.d_spins = 0;

	s.d_viewer->getHandlers()->append( new Lexi::ContextMenu( d_pop, false ) );

	if( d_sup->d_frameW == 0 || d_sup->d_frameH == 0 )
	{
		d_sup->d_frameW = a->getAllocation().getWidth();
		d_sup->d_frameH = a->getAllocation().getHeight();
	}
	d_frame = new FixedFrame( s.d_viewer, 
		d_sup->d_frameW, d_sup->d_frameH, d_sup->d_frameClr, d_sup->d_frameLw );

	d_ruler = new SpecRuler2D( d_frame, 0, &d_sup->d_rulFont, d_sup->d_rulClr, true );
	d_ruler->setSpace( DimX, d_sup->d_space[ DimX ] );
	d_ruler->setCount( DimX, d_sup->d_count[ DimX ] );
	d_ruler->setSpace( DimY, d_sup->d_space[ DimY ] );
	d_ruler->setCount( DimY, d_sup->d_count[ DimY ] );
	d_ruler->setThick( d_sup->d_rulLw );
	d_ruler->show( DimY, d_sup->d_showV );
	d_ruler->show( DimX, d_sup->d_showH );
	d_title = new Lexi::Label( "", &d_sup->d_titleFont, d_sup->d_titleClr, 
		0.5, Lexi::AlignBottom );
	d_space = new Lexi::Label( "", &d_sup->d_titleFont, d_sup->d_titleClr, 
		0.5, Lexi::AlignBottom );
	Glyph* box = LayoutKit::vbox();
	box->append( d_title );
	box->append( d_space );
	box->append( d_ruler );
	d_titleText = specs[0]->getName();
	setTitle();
	d_focus->setBody( new _CenterBox( box ) );

	d_widget->reallocate();

	SpecProjector* spec;
	Root::Ref<SpecProjector> sp;
	Root::Ref<SpecBufferMdl> buf;
	s.d_cv.assign( specs.size(), Root::Ref<ContourView>() );
	for( int i = 0; i < specs.size(); i++ )
	{
		spec = specs[ i ];
		assert( spec );
		sp = new SpecProjector( spec->getSpectrum(), 
			spec->getMapping()[ DimX ], spec->getMapping()[ DimY ] );
		sp->setOrigin( spec->getOrigin() );
		buf = new SpecBufferMdl( s.d_viewer->getViewArea(), sp );
		buf->setFolding( d_sup->d_showFolded );

		s.d_cv[i] = new ContourView( buf, false, d_sup->getPosClr( i ), d_sup->getNegClr( i ) );
		s.d_cv[i]->setWidth( d_sup->d_contLw );
		s.d_viewer->getViews()->append( s.d_cv[i] );
		if( d_autoContour )
			s.d_cv[i]->createLevelsAuto( d_params.d_factor, d_params.d_option, d_gain );
		else
			s.d_cv[i]->createLevelsMin( d_params.d_factor, d_params.d_threshold, d_params.d_option );
	}
	d_ruler->setModel( buf );
	if( d_peaks )
		s.d_viewer->getViews()->append( d_peaks );
	if( s.d_spins )
		s.d_viewer->getViews()->append( s.d_spins );
	d_widget->getViewport()->damageAll();
}

void ReportViewer::showStrips( const Root::Vector<Param>& ss )
{
	d_strips.assign( ss.size(), Strip() );
	d_rules.clear();

	Root::Ref<ViewAreaMdl> mdl;
	Lexi::Glyph* box = LayoutKit::hbox();
	Root::Ref<SpecBufferMdl> buf;
	QColor black = ( Qt::black );

	d_frame = new FixedFrame( box, d_sup->d_frameW, d_sup->d_frameH, 
		d_sup->d_frameClr, d_sup->d_frameLw );
	d_legende = new StripRuler2D( d_frame, &d_sup->d_rulFont, 
		d_sup->d_rulClr, d_sup->d_centClr );
	d_legende->anchLabel( d_sup->d_anchLabel );

	Lexi::Twips w = 0, h = 0;
	int i;
	for( i = 0; i < ss.size(); i++ )
	{
		if( i != 0 )
		{
			d_rules.push_back( new Rule( DimY, black, 20 ) );
			box->append( d_rules.back() );
		}
		Strip& s = d_strips[i];
		const Param& t = ss[i];
		// assert( t.d_spec && t.d_spec->getDimCount() == 2 );

		mdl = new ViewAreaMdl( true, true, false, true );
		assert( t.d_area );
		mdl->setRange( t.d_area->getRange( DimX ), t.d_area->getRange( DimY ) );
		s.d_viewer = new SpecViewer( mdl );

		if( t.d_spec )
		{

			Root::Ref<SpecProjector> sp = new SpecProjector( t.d_spec->getSpectrum(), 
				t.d_spec->getMapping()[ DimX ], t.d_spec->getMapping()[ DimY ] );
			sp->setOrigin( t.d_spec->getOrigin() );
			buf = new SpecBufferMdl( s.d_viewer->getViewArea(), sp );
			buf->setFolding( d_sup->d_showFolded );

			s.d_cv[0] = new ContourView( buf, false, d_sup->getPosClr(0), d_sup->getNegClr(0) );
			s.d_cv[0]->setWidth( d_sup->d_contLw );
			s.d_viewer->getViews()->append( s.d_cv[0] );
			if( d_autoContour )
				s.d_cv[0]->createLevelsAuto( d_params.d_factor, 
					d_params.d_option, d_gain );
			else
				s.d_cv[0]->createLevelsMin( d_params.d_factor, 
					d_params.d_threshold, d_params.d_option );
			
			s.d_spins = new SpinPointView( s.d_viewer, t.d_tuple, d_sup->d_spinClr,
				&d_sup->d_spinFont );
			s.d_spins->setWidth( d_sup->d_spinLw );
			s.d_spins->setOff( d_sup->d_spinOff );
			s.d_spins->setAngle( d_sup->d_spinAng );
			s.d_spins->show( d_sup->d_showSpin );
			s.d_spins->setLabel( d_sup->d_spinLbl, DimY );
			s.d_viewer->getViews()->append( s.d_spins );

			s.d_spins->showGhost( false );
			// TODO showGhost
			// TODO ghostLabel

			s.d_links = new SpinLinkView( s.d_viewer, t.d_tuple, d_sup->d_linkClr );
			s.d_links->setLabels( d_sup->d_links );
			s.d_links->setThick( d_sup->d_linkLw );
			s.d_links->showVerti( d_sup->d_showVerti );
			s.d_viewer->getViews()->append( s.d_links );

			s.d_viewer->getHandlers()->append( new Lexi::ContextMenu( d_pop, false ) );
		}else
		{
			s.d_cv[0] = 0;
			s.d_spins = 0;
			s.d_links = 0;
		}

		w += t.d_area->getAllocation().getWidth();
		h = t.d_area->getAllocation().getHeight();
		box->append( s.d_viewer );

		if( t.d_spec )
			d_legende->addStrip( buf, t.d_x, t.d_y );
	}
	if( d_sup->d_frameW == 0 || d_sup->d_frameH == 0 )
	{
		d_frame->setSize( w, h );
		d_sup->d_frameW = w;
		d_sup->d_frameH = h;
	}

	d_ruler = new SpecRuler2D( d_legende, buf, &d_sup->d_rulFont, d_sup->d_rulClr, false );
	d_ruler->setSpace( DimX, d_sup->d_space[ DimX ] );
	d_ruler->setCount( DimX, d_sup->d_count[ DimX ] );
	d_ruler->setSpace( DimY, d_sup->d_space[ DimY ] );
	d_ruler->setCount( DimY, d_sup->d_count[ DimY ] );
	d_ruler->setThick( d_sup->d_rulLw );
	d_ruler->show( DimY, d_sup->d_showV );

	d_title = new Lexi::Label( "", &d_sup->d_titleFont, d_sup->d_titleClr, 
		0.5, Lexi::AlignBottom );
	d_space = new Lexi::Label( "", &d_sup->d_titleFont, d_sup->d_titleClr, 
		0.5, Lexi::AlignBottom );
	Glyph* vbox = LayoutKit::vbox();
	vbox->append( d_title );
	vbox->append( d_space );
	vbox->append( d_ruler );
	d_titleText = ss[0].d_spec->getName();
	setTitle();
	d_focus->setBody( new _CenterBox( vbox ) );

    for( i = 0; i < int(d_rules.size()); i++ )
	{
		d_rules[ i ]->setColor( d_sup->d_frameClr );
		d_rules[ i ]->setThickness( d_sup->d_frameLw );
	}

	d_legende->showCenter( d_sup->d_showCenter );
	d_legende->setThick( d_sup->d_rulLw );
	d_legende->show( d_sup->d_showH );

	d_widget->getViewport()->damageAll();
}

void ReportViewer::setTitle()
{
	bool show = d_sup->d_showTitle && !d_titleText.isEmpty();
	if( show )
		d_title->setText( d_titleText.data() );
	else
		d_title->setText( "" );
	if( !show )
		d_space->setText( "" );
	else
		d_space->setText( " " );
}

void ReportViewer::buildMenues()
{
	Gui::Menu* menuFile = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuFile, this, "Save Settings...", SaveSettings, false );
	Gui::Menu::item( menuFile, this, "Load Settings...", LoadSettings, false );
	menuFile->insertSeparator();
    Gui::Menu::item( menuFile, this, Root::Action::FilePrint, Qt::CTRL+Qt::Key_P );
	Gui::Menu::item( menuFile, this, "Export PDF...", ExportPdf, false );
	Gui::Menu::item( menuFile, this, "Export SVG...", ExportSvg, false );
	menuFile->insertSeparator();
	menuFile->insertItem( "&Close", this, SLOT(close()), Qt::CTRL+Qt::Key_W );
	menuBar()->insertItem( "&File", menuFile );

	Gui::Menu* menuEdit = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuEdit, this, Root::Action::EditCopy, Qt::CTRL+Qt::Key_C );
	menuBar()->insertItem( "&Edit", menuEdit );

	Gui::Menu* menuView = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuView, this, "Set Display Page Size...", PageSize, false );
	Gui::Menu::item( menuView, this, "Landscape Orientation", SetLandscape, true );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, this, "Set Frame Size...", FrameSize, false );
	Gui::Menu::item( menuView, this, "Fit Frame to Page", FitToPage, false, Qt::CTRL+Qt::Key_Home );
	Gui::Menu::item( menuView, this, "Set PPM Range...", PpmRange, false );
	Gui::Menu::item( menuView, this, "Show Folded", ShowFolded, true );
	Gui::Menu::item( menuView, this, "Set Frame Line Color...", FrameColor, false );
	Gui::Menu::item( menuView, this, "Set Frame Line Width...", FrameWidth, false );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, this, "Set Contour Parameters...", ContourParams, false );
	Gui::Menu::item( menuView, this, "Auto Contour Level", AutoContour, true );
	Gui::Menu::item( menuView, this, "Set Auto Contour Gain...", AutoGain, false );
	Gui::Menu::item( menuView, this, "Set Positive Color...", PosColor, false );
	Gui::Menu::item( menuView, this, "Set Negative Color...", NegColor, false );
	Gui::Menu::item( menuView, this, "Set Contour Line Width...", SetWidth, false );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, this, "Set Ruler Font...", RulerFont, false );
	Gui::Menu::item( menuView, this, "Set Ruler Color...", RulerColor, false );
	Gui::Menu::item( menuView, this, "Set Ruler Resolution...", RulerMajor, false );
	Gui::Menu::item( menuView, this, "Show Horizontal", RulerShowH, true );
	Gui::Menu::item( menuView, this, "Show Vertical", RulerShowV, true );
	Gui::Menu::item( menuView, this, "Show Anchor Labels", AnchLabel, true );
	Gui::Menu::item( menuView, this, "Set Ruler Line Width...", RulerThick, false );
	Gui::Menu::item( menuView, this, "Show Center Line", ShowCenter, true );
	Gui::Menu::item( menuView, this, "Set Center Line Color...", CenterColor, false );
	menuView->insertSeparator();
	Gui::Menu::item( menuView, this, "Show Title", ShowTitle, true );
	Gui::Menu::item( menuView, this, "Set Title Text...", TitleText, false );
	Gui::Menu::item( menuView, this, "Set Title Font...", TitleFont, false );
	Gui::Menu::item( menuView, this, "Set Title Color...", TitleColor, false );
	menuBar()->insertItem( "&View", menuView );

	Gui::Menu* menuPeaks = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuPeaks, this, "Show Peaks", PeakShow, true );
	Gui::Menu::item( menuPeaks, this, "Set Peak Font...", PeakFont, false );
	Gui::Menu::item( menuPeaks, this, "Set Peak Color...", PeakColor, false );
	Gui::Menu::item( menuPeaks, this, "Set Peak Line Width...", PeakWidth, false );
	Gui::Menu::item( menuPeaks, this, "Set Label Offset...", PeakOff, false );
	Gui::Menu::item( menuPeaks, this, "Set Label Angle...", PeakAngle, false );
	menuBar()->insertItem( "&Peaks", menuPeaks );

	Gui::Menu* menuSpins = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuSpins, this, "Show Spins", SpinShow, true );
	Gui::Menu::item( menuSpins, this, "Set Spin Font...", SpinFont, false );
	Gui::Menu::item( menuSpins, this, "Set Spin Color...", SpinColor, false );
	Gui::Menu::item( menuSpins, this, "Set Spin Line Width...", SpinWidth, false );
	Gui::Menu::item( menuSpins, this, "Show Spin Cross", SpinCross, true );
	Gui::Menu::item( menuSpins, this, "Set Label Offset...", SpinOff, false );
	Gui::Menu::item( menuSpins, this, "Set Label Angle...", SpinAngle, false );
	Gui::Menu* sub = new Gui::Menu( menuBar() );
	menuSpins->insertItem( "Show Labels", sub );
	int i;
	for( i = SpinPointView::None; i < SpinPointView::End; i++ )
	{
		Gui::Menu::item( sub, this, SpinPointView::menuText[ i ], 
			ViewSpinLabels, true )->addParam( short( i ) );
	}
	menuBar()->insertItem( "&Spins", menuSpins );

	Gui::Menu* menuLinks = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuLinks, this, "Clear Links", LinkClear, false );
	Gui::Menu::item( menuLinks, this, "Add Link...", LinkAdd, false );
	menuLinks->insertSeparator();
	Gui::Menu::item( menuLinks, this, "Set Link Color...", LinkColor, false );
	Gui::Menu::item( menuLinks, this, "Set Link Width...", LinkThick, false );
	Gui::Menu::item( menuLinks, this, "Show Vertical", LinkVerti, true );
	menuBar()->insertItem( "&Links", menuLinks );

	Gui::Menu* menuHelp = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuHelp, this, Root::Action::HelpAbout );
	menuBar()->insertItem( "&?", menuHelp );

	d_pop = new Gui::Menu( this );
	sub = new Gui::Menu( d_pop );
	Gui::Menu::item( sub, this, "Show Peaks", PeakShow, true );
	Gui::Menu::item( sub, this, "Set Peak Font...", PeakFont, false );
	Gui::Menu::item( sub, this, "Set Peak Color...", PeakColor, false );
	Gui::Menu::item( sub, this, "Set Peak Line Width...", PeakWidth, false );
	Gui::Menu::item( sub, this, "Set Label Offset...", PeakOff, false );
	Gui::Menu::item( sub, this, "Set Label Angle...", PeakAngle, false );
	d_pop->insertItem( "Peaks", sub );
	sub = new Gui::Menu( d_pop );
	Gui::Menu::item( sub, this, "Show Spins", SpinShow, true );
	Gui::Menu::item( sub, this, "Set Spin Font...", SpinFont, false );
	Gui::Menu::item( sub, this, "Set Spin Color...", SpinColor, false );
	Gui::Menu::item( sub, this, "Set Spin Line Width...", SpinWidth, false );
	Gui::Menu::item( sub, this, "Show Spin Cross", SpinCross, true );
	Gui::Menu::item( sub, this, "Set Label Offset...", SpinOff, false );
	Gui::Menu::item( sub, this, "Set Label Angle...", SpinAngle, false );
	Gui::Menu* sub2 = new Gui::Menu( menuBar() );
	sub->insertItem( "Show Labels", sub2 );
	for( i = SpinPointView::None; i < SpinPointView::End; i++ )
	{
		Gui::Menu::item( sub2, this, SpinPointView::menuText[ i ], 
			ViewSpinLabels, true )->addParam( short( i ) );
	}
	d_pop->insertItem( "Spins", sub );
	sub = new Gui::Menu( d_pop );
	Gui::Menu::item( sub, this, "Clear Links", LinkClear, false );
	Gui::Menu::item( sub, this, "Add Link...", LinkAdd, false );
	Gui::Menu::item( sub, this, "Set Link Color...", LinkColor, false );
	Gui::Menu::item( sub, this, "Set Link Width...", LinkThick, false );
	Gui::Menu::item( sub, this, "Show Vertical", LinkVerti, true );
	d_pop->insertItem( "Links", sub );
	d_pop->insertSeparator();
	Gui::Menu::item( d_pop, this, "Set Frame Size...", FrameSize, false );
	Gui::Menu::item( d_pop, this, "Fit Frame to Page", FitToPage, false );
	Gui::Menu::item( d_pop, this, "Set PPM Range...", PpmRange, false );
	Gui::Menu::item( d_pop, this, "Set Frame Line Color...", FrameColor, false );
	Gui::Menu::item( d_pop, this, "Set Frame Line Width...", FrameWidth, false );
	d_pop->insertSeparator();
	Gui::Menu::item( d_pop, this, "Set Contour Parameters...", ContourParams, false );
	Gui::Menu::item( d_pop, this, "Set Positive Color...", PosColor, false );
	Gui::Menu::item( d_pop, this, "Set Negative Color...", NegColor, false );
	Gui::Menu::item( d_pop, this, "Set Contour Line Width...", SetWidth, false );
	d_pop->insertSeparator();
	Gui::Menu::item( d_pop, this, "Set Ruler Font...", RulerFont, false );
	Gui::Menu::item( d_pop, this, "Set Ruler Color...", RulerColor, false );
	Gui::Menu::item( d_pop, this, "Set Ruler Resolution...", RulerMajor, false );
	Gui::Menu::item( d_pop, this, "Set Ruler Line Width...", RulerThick, false );
	Gui::Menu::item( d_pop, this, "Show Horizontal", RulerShowH, true );
	Gui::Menu::item( d_pop, this, "Show Vertical", RulerShowV, true );
	Gui::Menu::item( d_pop, this, "Show Anchor Labels", AnchLabel, true );
	Gui::Menu::item( d_pop, this, "Show Center Line", ShowCenter, true );
	Gui::Menu::item( d_pop, this, "Set Center Line Color...", CenterColor, false );
}

void ReportViewer::setPageSize(int i)
{
	d_sup->d_pageSize = i;
	if( d_sup->d_landscape )
		d_widget->setFixedSize( Printer::mmToPoint( Printer::s_sizes[ d_sup->d_pageSize ].h ), 
			Printer::mmToPoint( Printer::s_sizes[ d_sup->d_pageSize ].w ) );
	else
		d_widget->setFixedSize( Printer::mmToPoint( Printer::s_sizes[ d_sup->d_pageSize ].w ), 
			Printer::mmToPoint( Printer::s_sizes[ d_sup->d_pageSize ].h ) );
}

int ReportViewer::selectLayer()
{
	if( d_strips[0].d_cv.size() == 1 )
		return 0;
	Dlg::StringList l( d_strips[0].d_cv.size() );
	QString str;
    for( int i = 0; i < int(d_strips[0].d_cv.size()); i++ )
	{
		str.sprintf( "&%d %s", i, d_strips[0].d_cv[ i ]->getModel()->getSpectrum()->getName() );
		l[ i ] = str.toLatin1();
	}
	return Dlg::getOption( this, l, "Select Overlay Layer", 0 );
}

void ReportViewer::handle(Root::Message& m)
{
	try
	{
		BEGIN_HANDLER();
		MESSAGE( Root::Action, a, m )
		{
			EXECUTE_ACTION( ReportViewer, *a );
		}
		MESSAGE( ViewAreaMdl::Update, a, m )
		{
            Q_UNUSED(a)
			d_widget->getViewport()->damageAll();
			m.consume();
		}
		END_HANDLER();
	}catch( ... )
	{
		qDebug( "Exception in ReportViewer::handle" );	// RISK: workaround
	}
}

void ReportViewer::handleContourParams(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	if( Dlg::setParams( this, d_params ) )
	{
        for( int i = 0; i < int(d_strips.size()); i++ )
            for( int j = 0; j < int(d_strips[i].d_cv.size()); j++ )
				if( d_strips[i].d_cv[j] )
					d_strips[i].d_cv[j]->createLevelsMin( d_params.d_factor, 
						d_params.d_threshold, d_params.d_option );
		d_widget->getViewport()->damageAll();
	}
}

void ReportViewer::handleFilePrint(Action& a)
{
	ACTION_ENABLED_IF( a, true );

	Root::Ref<Glyph> tmp = d_focus->getBody();
	d_focus->setBody( nil );
	Printer print( tmp );
	print.setPageSize( d_sup->d_pageSize );
	if( d_sup->d_landscape )
		print.setOrientation( QPrinter::Landscape );
	else
		print.setOrientation( QPrinter::Portrait );
	QString str;
	str.sprintf( "%s %s", AidaApplication::s_appName, AidaApplication::s_release );
	print.setPrintProgram( str );
	print.setCreator( str ); // RISK
	print.setDocName( d_titleText.data() ); // RISK
	// print.setFontEmbeddingEnabled( true );
#ifdef _WIN32
	// print.setOutputFormat(QPrinter::NativeFormat);
	// Wenn man das hier explizit setzt, strzt es immer.
#else
    print.setOutputFormat(QPrinter::PostScriptFormat);
#endif
    // Strzt print.setOutputFileName("*.ps");

	QPrintDialog printDialog( &print, this );
    if( printDialog.exec() == QDialog::Accepted )
	//if( print.setup( this ) )
	{
		print.print();
	}
	d_focus->setBody( tmp );
	d_widget->getViewport()->damageAll();
}
 
void ReportViewer::handleEditCopy(Action &)
{
	// TODO
}

void ReportViewer::handlePageSize(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	QStringList l;
	int i = 0;
	while( Printer::s_sizeNames[ i ] )
		l.append( Printer::s_sizeNames[ i++ ] ); 

	bool ok;
	QString res = QInputDialog::getItem( "Set Display Page Size", "Select a page size:", 
		l, d_sup->d_pageSize, false, &ok, this );
	if( !ok )
		return;

	i = 0;
	while( Printer::s_sizeNames[ i ] )
	{
		if( strcmp( Printer::s_sizeNames[ i ], res.toLatin1() ) == 0 )
		{
			setPageSize( i );
			break;
		}
		i++;
	}
}

void ReportViewer::handleSetLandscape(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_sup->d_landscape );

	d_sup->d_landscape = !d_sup->d_landscape;
	setPageSize( d_sup->d_pageSize );
}

void ReportViewer::handlePpmRange(Action & a)
{
	ACTION_ENABLED_IF( a, !d_strips.empty() );
	
	Spec::PpmRange w = d_strips[0].d_viewer->getViewArea()->getRange( DimX );
	Spec::PpmRange h = d_strips[0].d_viewer->getViewArea()->getRange( DimY );
	if( d_strips.size() > 1 )
		w = Spec::PpmRange(); // RISK: dirty trick
	if( Dlg::getPpmRange( this, w, h ) )
	{
		if( d_strips.size() == 1 )
			d_strips[ 0 ].d_viewer->getViewArea()->setRange( w, h );
		else
		{
            for( int i = 0; i < int(d_strips.size()); i++ )
				d_strips[ i ].d_viewer->getViewArea()->setRange( DimY, h );
		}
		d_widget->getViewport()->damageAll();
	}
}

void ReportViewer::handleFrameSize(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	
	float w = Printer::twipToMm( d_frame->getWidth() );
	float h = Printer::twipToMm( d_frame->getHeight() );
	if( Dlg::getFrameSize( this, w, h ) )
	{
		d_sup->d_frameW = Printer::mmToTwip( w );
		d_sup->d_frameH = Printer::mmToTwip( h );
		d_frame->setSize( d_sup->d_frameW, d_sup->d_frameH );
		d_widget->getViewport()->damageAll();
	}
}

void ReportViewer::handlePosColor(Action & a)
{
	ACTION_ENABLED_IF( a, d_strips[0].d_cv[0] );
	int j = selectLayer();
	if( j == -1 )
		return;
	QColor clr = QColorDialog::getColor( d_strips[0].d_cv[j]->getPosColor(), this );
	if( clr.isValid() )
	{
		QColor c = ( clr );
        for( int i = 0; i < int(d_strips.size()); i++ )
		{
            if( j < int(d_strips[i].d_cv.size()) && d_strips[i].d_cv[j] )
				d_strips[i].d_cv[j]->setPosColor( c );
			d_strips[i].d_viewer->redraw();
		}
        if( j < int(d_sup->d_posClr.size()) )
			d_sup->d_posClr[ j ] = c;
	}
}

void ReportViewer::handleNegColor(Action & a)
{
	ACTION_ENABLED_IF( a, d_strips[0].d_cv[0] );
	int j = selectLayer();
	if( j == -1 )
		return;
	QColor clr = QColorDialog::getColor( d_strips[0].d_cv[j]->getNegColor(), this );
	if( clr.isValid() )
	{
		QColor c = ( clr );
        for( int i = 0; i < int(d_strips.size()); i++ )
		{
            if( j < int(d_strips[i].d_cv.size()) && d_strips[i].d_cv[j] )
				d_strips[i].d_cv[j]->setNegColor( c );
			d_strips[i].d_viewer->redraw();
		}
        if( j < int(d_sup->d_negClr.size()) )
			d_sup->d_negClr[ j ] = c;
	}
}

void ReportViewer::handleFrameColor(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	QColor clr = QColorDialog::getColor( d_frame->getColor(), this );
	if( clr.isValid() )
	{
		d_sup->d_frameClr = ( clr );
		d_frame->setColor( d_sup->d_frameClr );
        for( int i = 0; i < int(d_rules.size()); i++ )
			d_rules[ i ]->setColor( d_sup->d_frameClr );
		d_widget->getViewport()->damageAll();
	}
}

void ReportViewer::handleFrameWidth(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	bool ok;
	int v = QInputDialog::getInteger( "Set Frame Line Width", 
		"Enter a value in TWIP (1/1440 Inch):", d_frame->getThickness(), 
		1, 999999, 1, &ok, this );
	if( !ok )
		return;
	d_sup->d_frameLw = v;
	d_frame->setThickness( v );
    for( int i = 0; i < int(d_rules.size()); i++ )
		d_rules[ i ]->setThickness( v );
	d_widget->getViewport()->damageAll();
}

void ReportViewer::handleSetWidth(Action & a)
{
	ACTION_ENABLED_IF( a, d_strips[0].d_cv[0] );

	bool ok;
	int v = QInputDialog::getInteger( "Set Contour Line Width", 
		"Enter a value in TWIP (1/1440 Inch):", d_strips[0].d_cv[0]->getWidth(), 
		0, 1440, 1, &ok, this );
	if( !ok )
		return;
	d_sup->d_contLw = v;
    for( int i = 0; i < int(d_strips.size()); i++ )
        for( int j = 0; j < int(d_strips[i].d_cv.size()); j++ )
			if( d_strips[i].d_cv[j] )
				d_strips[i].d_cv[j]->setWidth( d_sup->d_contLw );
	d_widget->getViewport()->damageAll();
}

void ReportViewer::handleRulerFont(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	bool ok;
	QFont res = QFontDialog::getFont( &ok, d_ruler->getFont(), this );
	if( !ok )
		return;
	d_sup->d_rulFont = res;
	d_ruler->setFont( d_sup->d_rulFont );
	if( d_legende )
		d_legende->setFont( d_sup->d_rulFont );
	d_widget->getViewport()->damageAll();
}

void ReportViewer::handleRulerColor(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	QColor clr = QColorDialog::getColor( d_ruler->getColor(), this );
	if( clr.isValid() )
	{
		d_sup->d_rulClr = ( clr );
		d_ruler->setColor( d_sup->d_rulClr );
		if( d_legende )
			d_legende->setColor( d_sup->d_rulClr );
		d_widget->getViewport()->damageAll();
	}
}

void ReportViewer::handleRulerMajor(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	d_sup->d_space[ DimX ] = d_ruler->getSpace( DimX );
	d_sup->d_count[ DimX ] = d_ruler->getCount( DimX );
	d_sup->d_space[ DimY ] = d_ruler->getSpace( DimY );
	d_sup->d_count[ DimY ] = d_ruler->getCount( DimY );
	if( Dlg::getResolution( this, d_sup->d_space[ DimX ], d_sup->d_count[ DimX ], 
		d_sup->d_space[ DimY ], d_sup->d_count[ DimY ] ) )
	{
		d_ruler->setSpace( DimX, d_sup->d_space[ DimX ] );
		d_ruler->setCount( DimX, d_sup->d_count[ DimX ] );
		d_ruler->setSpace( DimY, d_sup->d_space[ DimY ] );
		d_ruler->setCount( DimY, d_sup->d_count[ DimY ] );
		d_widget->getViewport()->damageAll();
	}
}

void ReportViewer::handlePeakFont(Action & a)
{
	ACTION_ENABLED_IF( a, !d_peaks.isNull() );

	bool ok;
	QFont res = QFontDialog::getFont( &ok, d_peaks->getFont(), this );
	if( !ok )
		return;
	d_sup->d_peakFont = res;
	d_peaks->setFont( d_sup->d_peakFont );
}

void ReportViewer::handlePeakColor(Action & a)
{
	ACTION_ENABLED_IF( a, !d_peaks.isNull() );
	QColor clr = QColorDialog::getColor( d_peaks->getColor(), this );
	if( clr.isValid() )
	{
		d_sup->d_peakClr = ( clr );
		d_peaks->setColor( d_sup->d_peakClr );
	}
}

void ReportViewer::handlePeakWidth(Action & a)
{
	ACTION_ENABLED_IF( a, !d_peaks.isNull() );
	bool ok;
	int v = QInputDialog::getInteger( "Set Peak Line Width", 
		"Enter a value in TWIP (1/1440 Inch) or 0:", d_peaks->getWidth(), 
		0, 999999, 1, &ok, this );
	if( !ok )
		return;
	d_sup->d_peakLw = v;
	d_peaks->setWidth( d_sup->d_peakLw );
}

void ReportViewer::handlePeakOff(Action & a)
{
	ACTION_ENABLED_IF( a, !d_peaks.isNull() );
	bool ok;
	int v = QInputDialog::getInteger( "Set Label Offset", 
		"Enter a value in TWIP (1/1440 Inch) or 0:", d_peaks->getOff(), 
		0, 999999, 1, &ok, this );
	if( !ok )
		return;
	d_sup->d_peakOff = v;
	d_peaks->setOff( d_sup->d_peakOff );
}

void ReportViewer::handlePeakAngle(Action & a)
{
	ACTION_ENABLED_IF( a, !d_peaks.isNull() );
	bool ok;
	int v = QInputDialog::getInteger( "Set Peak Angle", 
		"Enter a value between 0 and 359 degree:", d_peaks->getAngle(), 
		0, 359, 1, &ok, this );
	if( !ok )
		return;
	d_sup->d_peakAng = v;
	d_peaks->setAngle( d_sup->d_peakAng );
}

void ReportViewer::handlePeakShow(Action & a)
{
	ACTION_CHECKED_IF( a, !d_peaks.isNull(), !d_peaks.isNull() && d_peaks->show() );

	d_peaks->show( !d_peaks->show() );
	d_sup->d_showPeak = d_peaks->show();
}

void ReportViewer::handleSpinFont(Action & a)
{
	ACTION_ENABLED_IF( a, !d_strips[0].d_spins.isNull() );

	bool ok;
	QFont res = QFontDialog::getFont( &ok, d_strips[0].d_spins->getFont(), this );
	if( !ok )
		return;
	d_sup->d_spinFont = res;
    for( int i = 0; i < int(d_strips.size()); i++ )
		if( d_strips[i].d_spins )
			d_strips[i].d_spins->setFont( d_sup->d_spinFont );
}

void ReportViewer::handleSpinColor(Action & a)
{
	ACTION_ENABLED_IF( a, !d_strips[0].d_spins.isNull() );
	QColor clr = QColorDialog::getColor( 
		d_strips[0].d_spins->getColor(), this );
	if( clr.isValid() )
	{
		d_sup->d_spinClr = ( clr );
        for( int i = 0; i < int(d_strips.size()); i++ )
			if( d_strips[i].d_spins )
				d_strips[i].d_spins->setColor( d_sup->d_spinClr );
	}
}

void ReportViewer::handleSpinWidth(Action & a)
{
	ACTION_ENABLED_IF( a, !d_strips[0].d_spins.isNull() );
	bool ok;
	int v = QInputDialog::getInteger( "Set Spin Line Width", 
		"Enter a value in TWIP (1/1440 Inch) or 0:", d_strips[0].d_spins->getWidth(), 
		0, 999999, 1, &ok, this );
	if( !ok )
		return;
	d_sup->d_spinLw = v;
    for( int i = 0; i < int(d_strips.size()); i++ )
		if( d_strips[i].d_spins )
			d_strips[i].d_spins->setWidth( d_sup->d_spinLw );
}

void ReportViewer::handleSpinOff(Action & a)
{
	ACTION_ENABLED_IF( a, !d_strips[0].d_spins.isNull() );
	bool ok;
	int v = QInputDialog::getInteger( "Set Label Offset", 
		"Enter a value in TWIP (1/1440 Inch) or 0:", d_strips[0].d_spins->getOff(), 
		0, 999999, 1, &ok, this );
	if( !ok )
		return;
	d_sup->d_spinOff = v;
    for( int i = 0; i < int(d_strips.size()); i++ )
		if( d_strips[i].d_spins )
			d_strips[i].d_spins->setOff( v );
}

void ReportViewer::handleSpinAngle(Action & a)
{
	ACTION_ENABLED_IF( a, !d_strips[0].d_spins.isNull() );
	bool ok;
	int v = QInputDialog::getInteger( "Set Spin Angle", 
		"Enter a value between 0 and 359 degree:", d_strips[0].d_spins->getAngle(), 
		0, 359, 1, &ok, this );
	if( !ok )
		return;
	d_sup->d_spinAng = v;
    for( int i = 0; i < int(d_strips.size()); i++ )
		if( d_strips[i].d_spins )
			d_strips[i].d_spins->setAngle( v );
}

void ReportViewer::handleSpinShow(Action & a)
{
	ACTION_CHECKED_IF( a, !d_strips[0].d_spins.isNull(), 
		!d_strips[0].d_spins.isNull() && d_strips[0].d_spins->show() );

    for( int i = 0; i < int(d_strips.size()); i++ )
	{
		if( d_strips[i].d_spins == 0 )
			continue;
		d_strips[i].d_spins->show( !d_strips[i].d_spins->show() );
		d_sup->d_showSpin = d_strips[i].d_spins->show();
	}
}

void ReportViewer::handleViewSpinLabels(Action & a)
{
	if( a.getParamCount() == 0 )
		return;

	SpinPointView::Label q = (SpinPointView::Label) a.getParam( 0 ).getShort();

	ACTION_CHECKED_IF( a, !d_strips[0].d_spins.isNull(),
		!d_strips[0].d_spins.isNull() && d_strips[0].d_spins->getLabel() == q );
	
	d_sup->d_spinLbl = q;
    for( int i = 0; i < int(d_strips.size()); i++ )
	{
		if( d_strips[i].d_spins )
			d_strips[i].d_spins->setLabel( d_sup->d_spinLbl, DimY );
		d_strips[i].d_viewer->redraw();
	}
}

void ReportViewer::handleRulerThick(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	bool ok;
	int v = QInputDialog::getInteger( "Set Ruler Line Width", 
		"Enter a value in TWIP (1/1440 Inch):", d_ruler->getThick(), 
		1, 999999, 1, &ok, this );
	if( !ok )
		return;
	d_sup->d_rulLw = v;
	d_ruler->setThick( d_sup->d_rulLw );
	if( d_legende )
		d_legende->setThick( d_sup->d_rulLw );
	d_widget->getViewport()->damageAll();
}

void ReportViewer::handleShowCenter(Action & a)
{
	ACTION_CHECKED_IF( a, d_legende, d_legende && d_legende->showCenter() );
	d_legende->showCenter( !d_legende->showCenter() );
	d_sup->d_showCenter = d_legende->showCenter();
	d_widget->getViewport()->damageAll();
}

void ReportViewer::handleCenterColor(Action & a)
{
	ACTION_ENABLED_IF( a, d_legende );

	QColor clr = QColorDialog::getColor( d_legende->getCenterColor(), this );
	if( clr.isValid() )
	{
		d_sup->d_centClr = ( clr );
		d_legende->setCenterColor( d_sup->d_centClr );
	}
}

void ReportViewer::handleLinkAdd(Action & a)
{
	ACTION_ENABLED_IF( a, d_legende );
	bool ok	= FALSE;
	QString res	= QInputDialog::getText( "Add Link for Label", 
		"Please	enter a label:", QLineEdit::Normal, 
		"", &ok, this );
	if( !ok )
		return;
	Root::SymbolString s = res.toLatin1();
	d_sup->d_links.insert( s );
    for( int i = 0; i < int(d_strips.size()); i++ )
		if( d_strips[ i ].d_links )
			d_strips[ i ].d_links->addLabel( s );

}

void ReportViewer::handleLinkClear(Action & a)
{
	ACTION_ENABLED_IF( a, d_legende );
	d_sup->d_links.clear();
    for( int i = 0; i < int(d_strips.size()); i++ )
		if( d_strips[ i ].d_links )
			d_strips[ i ].d_links->clear();
}

void ReportViewer::handleLinkColor(Action & a)
{
	ACTION_ENABLED_IF( a, d_legende && d_strips[ 0 ].d_links );

	QColor clr = QColorDialog::getColor( d_strips[ 0 ].d_links->getColor(), this );
	if( clr.isValid() )
	{
		d_sup->d_linkClr = ( clr );
        for( int i = 0; i < int(d_strips.size()); i++ )
			if( d_strips[ i ].d_links )
				d_strips[ i ].d_links->setColor( d_sup->d_linkClr );
	}
}

void ReportViewer::handleLinkThick(Action & a)
{
	ACTION_ENABLED_IF( a, d_legende && d_strips[ 0 ].d_links );
	bool ok;
	int v = QInputDialog::getInteger( "Set Links Line Width", 
		"Enter a value in TWIP (1/1440 Inch):", d_strips[ 0 ].d_links->getThick(), 
		1, 999999, 1, &ok, this );
	if( !ok )
		return;
	d_sup->d_linkLw = v;
    for( int i = 0; i < int(d_strips.size()); i++ )
		if( d_strips[ i ].d_links )
			d_strips[ i ].d_links->setThick( v );
}

void ReportViewer::handleLinkVerti(Action & a)
{
	ACTION_CHECKED_IF( a, d_legende, d_legende && d_strips[ 0 ].d_links &&
		d_strips[ 0 ].d_links->showVerti() );
    for( int i = 0; i < int(d_strips.size()); i++ )
	{
		if( d_strips[ i ].d_links == 0 )
			continue;
		d_strips[ i ].d_links->showVerti( !d_strips[ i ].d_links->showVerti() );
		d_sup->d_showVerti = d_strips[ i ].d_links->showVerti();
	}
}

void ReportViewer::handleFitToPage(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	Lexi::Twips w = d_ruler->getWidth() * 2.5;
	Lexi::Twips h = d_ruler->getHeight() * 2.5;
	if( d_legende )
		h = 2 * ( d_legende->getHLower() + d_legende->getHUpper() );
	if( d_title )
	{
		Font::BoundingBox b;
		b.stringBox( d_title->getFont() );
		h += b.getHeight() * 2;
	}

	w = d_widget->width() * 20 - w;
	h = d_widget->height() * 20 - h;
	d_sup->d_frameW = w;
	d_sup->d_frameH = h;
	d_frame->setSize( d_sup->d_frameW, d_sup->d_frameH );
	d_widget->getViewport()->damageAll();
}

void ReportViewer::handleRulerShowH(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_sup->d_showH );
	d_sup->d_showH = !d_sup->d_showH;
	if( d_legende )
		d_legende->show( d_sup->d_showH );
	else
		d_ruler->show( DimX, d_sup->d_showH );
	d_widget->getViewport()->damageAll();
}

void ReportViewer::handleRulerShowV(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_sup->d_showV );
	d_sup->d_showV = !d_sup->d_showV;
	d_ruler->show( DimY, d_sup->d_showV );
	d_widget->getViewport()->damageAll();
}

void ReportViewer::handleSaveSettings(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	Strip& s = d_strips[0];
	d_sup->d_range[ DimX ] = s.d_viewer->getViewArea()->getRange( DimX );
	d_sup->d_range[ DimY ] = s.d_viewer->getViewArea()->getRange( DimY );

	QString fileName = QFileDialog::getSaveFileName( this, "Save Settings",
                                                     AppAgent::getCurrentDir(), "*.ini" );
	if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );

	if( info.extension( false ).upper() != "INI" )
		fileName += ".ini";
	info.setFile( fileName );
	if( info.exists() )
	{
		if( QMessageBox::warning( this, "Save As",
			"This file already exists. Do you want to overwrite it?",
			"&OK", "&Cancel" ) != 0 )
			return;
	}
	Root::AppAgent::setCurrentDir( info.dirPath( true ) );

	try
	{
		d_sup->save2( fileName );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Save Settings", e.what(), "OK" ); 
	}
}

void ReportViewer::handleLoadSettings(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	QString fileName = QFileDialog::getOpenFileName( this, "Load Settings",
                                                     AppAgent::getCurrentDir(),
                                                     "Setup File (*.ini *.set)" );
	if( fileName.isNull() ) 
		return;

	bool setRange = false;
	setRange = QMessageBox::warning( this, "Load Settings",
		"Do you want to use the saved PPM ranges?",
		"&Yes", "&No" ) == 0;

	try
	{
        if( fileName.toLower().endsWith( ".set" ) )
            d_sup->load( fileName );
        else
            d_sup->load2( fileName );
		int i;
        for( i = 0; i < int(d_strips.size()); i++ )
		{
			Strip& s = d_strips[ i ];
            for( int j = 0; j < int(s.d_cv.size()); j++ )
			{
				s.d_cv[j]->setPosColor( d_sup->getPosClr( j ) );
				s.d_cv[j]->setNegColor( d_sup->getNegClr( j ) );
				s.d_cv[j]->setWidth( d_sup->d_contLw );
				s.d_cv[j]->getModel()->setFolding( d_sup->d_showFolded );
			}
			if( s.d_spins )
			{
				s.d_spins->setColor( d_sup->d_spinClr );
				s.d_spins->setFont( d_sup->d_spinFont );
				s.d_spins->setWidth( d_sup->d_spinLw );
				s.d_spins->setOff( d_sup->d_spinOff );
				s.d_spins->setAngle( d_sup->d_spinAng );
				s.d_spins->show( d_sup->d_showSpin );
				s.d_spins->setLabel( d_sup->d_spinLbl, DimY );
				s.d_spins->showCross( d_sup->d_showCross );
			}
			if( s.d_links )
			{
				s.d_links->setColor( d_sup->d_linkClr );
				s.d_links->setLabels( d_sup->d_links );
				s.d_links->setThick( d_sup->d_linkLw );
				s.d_links->showVerti( d_sup->d_showVerti );
			}
			if( setRange )
			{
				if( d_legende )
					s.d_viewer->getViewArea()->setRange( 
						DimY, d_sup->d_range[ DimY ] );
				else
					s.d_viewer->getViewArea()->setRange( 
						d_sup->d_range[ DimX ], d_sup->d_range[ DimY ] );
			}
		}
		if( d_peaks )
		{
			d_peaks->setColor( d_sup->d_peakClr );
			d_peaks->setFont( d_sup->d_peakFont );
			d_peaks->setWidth( d_sup->d_peakLw );
			d_peaks->setOff( d_sup->d_peakOff );
			d_peaks->setAngle( d_sup->d_peakAng );
			d_peaks->show( d_sup->d_showPeak );
		}
		d_ruler->setColor( d_sup->d_rulClr );
		d_ruler->setFont( d_sup->d_rulFont );
		d_ruler->setSpace( DimX, d_sup->d_space[ DimX ] );
		d_ruler->setCount( DimX, d_sup->d_count[ DimX ] );
		d_ruler->setSpace( DimY, d_sup->d_space[ DimY ] );
		d_ruler->setCount( DimY, d_sup->d_count[ DimY ] );
		d_ruler->setThick( d_sup->d_rulLw );
		d_ruler->show( DimY, d_sup->d_showV );
		if( d_legende == 0 )
			d_ruler->show( DimX, d_sup->d_showH );

		if( d_legende )
		{
			d_legende->setColor( d_sup->d_rulClr );
			d_legende->setCenterColor( d_sup->d_centClr );
			d_legende->setFont( d_sup->d_rulFont );
			d_legende->showCenter( d_sup->d_showCenter );
			d_legende->setThick( d_sup->d_rulLw );
			d_legende->show( d_sup->d_showH );
			d_legende->anchLabel( d_sup->d_anchLabel );
		}

        for( i = 0; i < int(d_rules.size()); i++ )
		{
			d_rules[ i ]->setColor( d_sup->d_frameClr );
			d_rules[ i ]->setThickness( d_sup->d_frameLw );
		}
		d_frame->setSize( d_sup->d_frameW, d_sup->d_frameH );
		d_frame->setColor( d_sup->d_frameClr );
		d_frame->setThickness( d_sup->d_frameLw );
		d_title->setColor( d_sup->d_titleClr );
		d_title->setFont( d_sup->d_titleFont );
		d_space->setFont( d_sup->d_titleFont );
		setTitle();
		setPageSize( d_sup->d_pageSize );
		d_widget->getViewport()->damageAll();
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Load Settings", e.what(), "OK" ); 
	}catch( ... )
	{
		QMessageBox::critical( this, "Load Settings", "unknown exception", "OK" ); 
	}
}

void ReportViewer::handleTitleFont(Action & a)
{
	ACTION_ENABLED_IF( a, d_title );

	bool ok;
	QFont res = QFontDialog::getFont( &ok, d_title->getFont(), this );
	if( !ok )
		return;
	d_sup->d_titleFont = res;
	d_title->setFont( d_sup->d_titleFont );
	d_space->setFont( d_sup->d_titleFont );
	d_frame->setSize( d_sup->d_frameW, d_sup->d_frameH );
	d_widget->getViewport()->damageAll();
}

void ReportViewer::handleTitleColor(Action & a)
{
	ACTION_ENABLED_IF( a, d_title );
	QColor clr = QColorDialog::getColor( d_title->getColor(), this );
	if( clr.isValid() )
	{
		d_sup->d_titleClr = ( clr );
		d_title->setColor( d_sup->d_titleClr );
		d_widget->getViewport()->damageAll();
	}
}

void ReportViewer::handleTitleText(Action & a)
{
	ACTION_ENABLED_IF( a, d_title );
	bool ok	= FALSE;
	QString res	= QInputDialog::getText( "Set Title", 
		"Please	enter a text:", QLineEdit::Normal, d_titleText.data(), &ok, this );
	if( !ok )
		return;
	res = res.stripWhiteSpace();
	d_sup->d_showTitle = !res.isEmpty();
	d_titleText = res.toLatin1();
	setTitle();
	d_widget->getViewport()->damageAll();
}

void ReportViewer::handleShowTitle(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_sup->d_showTitle );
	d_sup->d_showTitle = !d_sup->d_showTitle;
	setTitle();
	d_widget->getViewport()->damageAll();
}

void ReportViewer::handleAutoContour(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_autoContour );
	d_autoContour = !d_autoContour;

    for( int i = 0; i < int(d_strips.size()); i++ )
        for( int j = 0; j < int(d_strips[i].d_cv.size()); j++ )
			if( d_strips[i].d_cv[j] )
			{
				if( d_autoContour )
					d_strips[i].d_cv[j]->createLevelsAuto( d_params.d_factor, 
						d_params.d_option, d_gain );
				else
					d_strips[i].d_cv[j]->createLevelsMin( d_params.d_factor, 
						d_params.d_threshold, d_params.d_option );
			}
	d_widget->getViewport()->damageAll();
}

void ReportViewer::handleAutoGain(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	bool ok	= FALSE;
	QString res;
	res.sprintf( "%0.2f", d_gain );
	res	= QInputDialog::getText( "Set Auto Gain", 
		"Please	enter a positive factor:", QLineEdit::Normal, res, &ok, this );
	if( !ok )
		return;
	float f = res.toFloat( &ok );
	if( !ok || f < 0.0 )
	{
		QMessageBox::critical( this, "Set Auto Gain",
				"Invalid factor!", "&Cancel" );
		return;
	}
	d_autoContour = true;
	d_gain = f;
    for( int i = 0; i < int(d_strips.size()); i++ )
        for( int j = 0; j < int(d_strips[i].d_cv.size()); j++ )
			if( d_strips[i].d_cv[j] )
				d_strips[i].d_cv[j]->createLevelsAuto( d_params.d_factor, 
					d_params.d_option, d_gain );
	d_widget->getViewport()->damageAll();
}

void ReportViewer::handleAnchLabel(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_sup->d_anchLabel );
	d_sup->d_anchLabel = !d_sup->d_anchLabel;
	if( d_legende )
		d_legende->anchLabel( d_sup->d_anchLabel );
	d_widget->getViewport()->damageAll();
}

void ReportViewer::handleSpinCross(Action & a)
{
	ACTION_CHECKED_IF( a, !d_strips[0].d_spins.isNull(), 
		!d_strips[0].d_spins.isNull() && d_strips[0].d_spins->showCross() );

    for( int i = 0; i < int(d_strips.size()); i++ )
	{
		if( d_strips[i].d_spins == 0 )
			continue;
		d_strips[i].d_spins->showCross( !d_strips[i].d_spins->showCross() );
		d_sup->d_showCross = d_strips[i].d_spins->showCross();
	}
	d_widget->getViewport()->damageAll();
}

void ReportViewer::handleShowFolded(Action & a)
{
	ACTION_CHECKED_IF( a, true, d_sup->d_showFolded );

	d_sup->d_showFolded = !d_sup->d_showFolded;
    for( int i = 0; i < int(d_strips.size()); i++ )
        for( int j = 0; j < int(d_strips[i].d_cv.size()); j++ )
			if( d_strips[i].d_cv[j] )
				d_strips[i].d_cv[j]->getModel()->setFolding( d_sup->d_showFolded );
	d_widget->getViewport()->damageAll();
}

void ReportViewer::handleExportPdf(Action& a)
{
	ACTION_ENABLED_IF( a, true );

    QString fileName = QFileDialog::getSaveFileName(this, "Export PDF", 
		AppAgent::getCurrentDir(), "*.pdf");
    if (fileName.isEmpty())
        return;

	QFileInfo info( fileName );

	if( info.suffix().toUpper() != "PDF" )
		fileName += ".pdf";
	info.setFile( fileName );
	Root::AppAgent::setCurrentDir( info.absolutePath().toLatin1() );

	Root::Ref<Glyph> tmp = d_focus->getBody();
	d_focus->setBody( nil );
	Printer print( tmp );
    print.setOutputFormat(QPrinter::PdfFormat);
    print.setOutputFileName(fileName);

	print.setPageSize( d_sup->d_pageSize );
	if( d_sup->d_landscape )
		print.setOrientation( QPrinter::Landscape );
	else
		print.setOrientation( QPrinter::Portrait );
	QString str;
	str.sprintf( "%s %s", AidaApplication::s_appName, AidaApplication::s_release );
	print.setPrintProgram( str );
	print.setCreator( str ); // RISK
	print.setDocName( d_titleText.data() ); // RISK
	// print.setFontEmbeddingEnabled( true );

	print.print();

	d_focus->setBody( tmp );
	d_widget->getViewport()->damageAll();
}
 
void ReportViewer::handleExportSvg(Action& a)
{
	ACTION_ENABLED_IF( a, true );

    QString fileName = QFileDialog::getSaveFileName(this, "Export SVG", 
		AppAgent::getCurrentDir(), "*.svg");
    if (fileName.isEmpty())
        return;

	QFileInfo info( fileName );

	if( info.suffix().toUpper() != "SVG" )
		fileName += ".svg";
	info.setFile( fileName );
	Root::AppAgent::setCurrentDir( info.absolutePath().toLatin1() );

	Root::Ref<Glyph> tmp = d_focus->getBody();
	d_focus->setBody( nil );
	Picture print( tmp );
	print.setPageSize( d_sup->d_pageSize );
	print.setLandscape( d_sup->d_landscape );
	print.print();
	print.save( fileName, "svg" );
	d_focus->setBody( tmp );
	d_widget->getViewport()->damageAll();
}
