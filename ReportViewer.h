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

#if !defined(AFX_REPORTVIEWER_H__500C5ACE_60D0_4A60_8C65_3036A23D7907__INCLUDED_)
#define AFX_REPORTVIEWER_H__500C5ACE_60D0_4A60_8C65_3036A23D7907__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Lexi/MainWindow.h>
#include <Root/ActionHandler.h>
#include <Lexi/GlyphWidget.h>
#include <Lexi/FocusManager.h>
#include <Lexi/Shapes.h>
#include <Lexi/Label.h>
#include <SpecView/ContourView.h>
#include <SpecView/SpecViewer.h>
#include <SpecView/PeakPlaneView.h>
#include <SpecView/SpinPointView.h>
#include <Dlg.h>
#include <Lexi/FixedFrame.h>
#include <Q3MainWindow>
#include <QHideEvent>
#include <Gui/Menu.h>

class QComboBox;
class Q3ScrollView;

namespace Spec
{
	class SpecViewer;
	class SpecProjector;
	class ViewAreaMdl;
	using Root::Action;
	class SpecRuler2D;
	class StripRuler2D;
	class SpinLinkView;
	class Repository;

    class ReportViewer : public Q3MainWindow, public Root::Messenger
	{
	public:
		static Root::Action::CmdStr ContourParams;
		static Root::Action::CmdStr PageSize;
		static Root::Action::CmdStr FrameSize;
		static Root::Action::CmdStr PosColor;
		static Root::Action::CmdStr NegColor;
		static Root::Action::CmdStr FrameColor;
		static Root::Action::CmdStr FrameWidth;
		static Root::Action::CmdStr SetLandscape;
		static Root::Action::CmdStr PpmRange;
		static Root::Action::CmdStr SetWidth;
		static Root::Action::CmdStr RulerFont;
		static Root::Action::CmdStr RulerColor;
		static Root::Action::CmdStr RulerMajor;
		static Root::Action::CmdStr RulerThick;
		static Root::Action::CmdStr RulerShowH;
		static Root::Action::CmdStr RulerShowV;
		static Root::Action::CmdStr PeakFont;
		static Root::Action::CmdStr PeakColor;
		static Root::Action::CmdStr PeakWidth;
		static Root::Action::CmdStr PeakOff;
		static Root::Action::CmdStr PeakAngle;
		static Root::Action::CmdStr PeakShow;
		static Root::Action::CmdStr SpinFont;
		static Root::Action::CmdStr SpinColor;
		static Root::Action::CmdStr SpinWidth;
		static Root::Action::CmdStr SpinOff;
		static Root::Action::CmdStr SpinAngle;
		static Root::Action::CmdStr SpinShow;
		static Root::Action::CmdStr SpinCross;
		static Root::Action::CmdStr ViewSpinLabels;
		static Root::Action::CmdStr ShowCenter;
		static Root::Action::CmdStr CenterColor;
		static Root::Action::CmdStr LinkAdd;
		static Root::Action::CmdStr LinkClear;
		static Root::Action::CmdStr LinkColor;
		static Root::Action::CmdStr LinkThick;
		static Root::Action::CmdStr LinkVerti;
		static Root::Action::CmdStr FitToPage;
		static Root::Action::CmdStr SaveSettings;
		static Root::Action::CmdStr LoadSettings;
		static Root::Action::CmdStr TitleFont;
		static Root::Action::CmdStr TitleColor;
		static Root::Action::CmdStr TitleText;
		static Root::Action::CmdStr ShowTitle;
		static Root::Action::CmdStr AutoContour;
		static Root::Action::CmdStr AutoGain;
		static Root::Action::CmdStr AnchLabel;
		static Root::Action::CmdStr ShowFolded;
		static Root::Action::CmdStr ExportPdf;
		static Root::Action::CmdStr ExportSvg;

		struct Setup 
		{
			enum { ClrTableSize = 10 };

			int d_pageSize;
			bool d_landscape;

			qint32 d_frameW, d_frameH, d_frameLw; // width, heigth, line width
			QColor d_frameClr; 

			Root::Vector< QColor > d_posClr;
			Root::Vector< QColor > d_negClr; 
			qint32 d_contLw; // Contour line width

			QFont d_rulFont; // Ruler font
			QColor d_rulClr;
			qint32 d_space[ 2 ];
			Root::UInt8 d_count[ 2 ];
			qint32 d_rulLw;
			bool d_showH, d_showV, d_anchLabel;

			QFont d_peakFont; 
			QColor d_peakClr;
			qint32 d_peakLw; // Peak line width
			qint32 d_peakOff; // Peak label offset
			short d_peakAng; // Peak angle
			bool d_showPeak;

			QFont d_spinFont; 
			QColor d_spinClr;
			qint32 d_spinLw; // Spin line width
			qint32 d_spinOff; // Spin label offset
			short d_spinAng; // Spin angle
			bool d_showSpin;
			bool d_showCross;
			SpinPointView::Label d_spinLbl;

			Root::SymbolSet d_links;
			QColor d_linkClr;
			qint32 d_linkLw; 
			bool d_showVerti;
			bool d_showCenter;
			QColor d_centClr;

			Spec::PpmRange d_range[ 2 ];

			QColor d_titleClr;
			QFont d_titleFont;
			bool d_showTitle;
			bool d_showFolded;
		public:
			void load( const char* );
            void load2( const char* );
			void save( const char* );
            void save2( const char* );
			Setup();
			QColor  getPosClr( int i ) const
			{
				if( i < d_posClr.size() )
					return d_posClr[ i ];
				else
					return d_posClr[ 0 ];
			}
			QColor  getNegClr( int i ) const
			{
				if( i < d_negClr.size() )
					return d_negClr[ i ];
				else
					return d_negClr[ 0 ];
			}
		};

		struct Param
		{
			ViewAreaMdl* d_area;
			SpecProjector* d_spec;
			SpinSpace* d_tuple;
			Spin* d_x;
			Spin* d_y;
			Param():d_tuple(0),d_spec(0),d_area(0), d_x(0), d_y(0) {}
		};

		void setPageSize( int );
		typedef Root::Vector<SpecProjector*> Spector;
		void showPlane(ViewAreaMdl*,const Spector&, 
			PeakSpace* = 0,SpinSpace* = 0);
		void showStrips(const Root::Vector<Param>& );

		static void kill();
		static ReportViewer* getViewer(Root::Agent*, const Dlg::ContourParams&,
			float ag = 2.0, bool ac = false, bool fold = false, Repository* = 0 );
	protected:
		void hideEvent( QHideEvent * );
		int selectLayer();
		void setTitle();
		ReportViewer(Root::Agent*,const Dlg::ContourParams&, float ag, bool ac,
			bool fold, Repository* );
		virtual ~ReportViewer();

		GENERIC_MESSENGER(Q3MainWindow)
		void handle(Root::Message&);
	private:
		void handleExportSvg( Action& );
		void handleExportPdf( Action& );
		void handleShowFolded( Action& );
		void handleSpinCross( Action& );
		void handleAnchLabel( Action& );
		void handleAutoGain( Action& );
		void handleAutoContour( Action& );
		void handleShowTitle( Action& );
		void handleTitleText( Action& );
		void handleTitleColor( Action& );
		void handleTitleFont( Action& );
		void handleLoadSettings( Action& );
		void handleSaveSettings( Action& );
		void handleRulerShowV( Action& );
		void handleRulerShowH( Action& );
		void handleFitToPage( Action& );
		void handleLinkVerti( Action& );
		void handleLinkThick( Action& );
		void handleLinkColor( Action& );
		void handleLinkClear( Action& );
		void handleLinkAdd( Action& );
		void handleCenterColor( Action& );
		void handleShowCenter( Action& );
		void handleRulerThick( Action& );
		void handleViewSpinLabels( Action& );
		void handleSpinShow( Action& );
		void handleSpinAngle( Action& );
		void handleSpinOff( Action& );
		void handleSpinWidth( Action& );
		void handleSpinColor( Action& );
		void handleSpinFont( Action& );
		void handlePeakShow( Action& );
		void handlePeakAngle( Action& );
		void handlePeakOff( Action& );
		void handlePeakWidth( Action& );
		void handlePeakColor( Action& );
		void handlePeakFont( Action& );
		void handleRulerMinor( Action& );
		void handleRulerMajor( Action& );
		void handleRulerColor( Action& );
		void handleRulerFont( Action& );
		void handleSetWidth( Action& );
		void handlePpmRange( Action& );
		void handleSetLandscape( Action& );
		void handleFrameWidth( Action& );
		void handleFrameColor( Action& );
		void handleNegColor( Action& );
		void handlePosColor( Action& );
		void handleFrameSize( Action& );
		void handlePageSize( Action& );
		void handleEditCopy( Action& );
		void handleFilePrint(Action& );
		void handleContourParams( Action& );
		FRIEND_ACTION_HANDLER( ReportViewer );
		void buildMenues();
		Root::ExRef<Lexi::FocusManager> d_focus;
		Root::Ptr<Lexi::GlyphWidget> d_widget;
		Gui::Menu* d_pop;

		Dlg::ContourParams d_params;
		struct Strip
		{
			std::vector< Root::Ref<ContourView> > d_cv;
			Root::Ref<SpecViewer> d_viewer;
			Root::Ref<SpinPointView> d_spins;
			SpinLinkView* d_links;
			Strip():d_links(0), d_cv( 1 ) {}
		};
		std::vector<Strip> d_strips;
		std::vector<Lexi::Rule*> d_rules;

		Root::ExRef<PeakPlaneView> d_peaks;
		SpecRuler2D* d_ruler;
		StripRuler2D* d_legende;
		Lexi::Label* d_title;
		Lexi::Label* d_space;

		Q3ScrollView* d_scroller;
		Lexi::FixedFrame* d_frame;

		Setup* d_sup;
		QByteArray  d_titleText;
		bool d_autoContour;
		float d_gain;
	};
}

#endif // !defined(AFX_REPORTVIEWER_H__500C5ACE_60D0_4A60_8C65_3036A23D7907__INCLUDED_)
