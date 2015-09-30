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

#if !defined(AFX_DLG_H__F6AC8148_E916_4874_A79D_BFACA432AA6F__INCLUDED_)
#define AFX_DLG_H__F6AC8148_E916_4874_A79D_BFACA432AA6F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <SpecView/ContourView.h>
#include <Spec/SpinLabel.h>
#include <Spec/ResidueType.h>
#include <Spec/SpinSpace.h>
#include <SpecView/IntensityView.h>
#include <Gui/ListView.h>
#include <QDialog>
class QWidget;
class Q3ComboBox;
class QCheckBox;

namespace Spec
{
	class PeakList;
	class Project;
	class Spin;
	class Repository;
	class IntensityView;

	class Dlg
	{
	public:
		class SpinItem : public Gui::ListViewItem
		{
		public:
			SpinItem( Gui::ListView * parent, Spin* lhs, float f );
			QString text( int f ) const;
			QString key( int f, bool ) const;
			Spin* getSpin() const { return d_spin; }
		private:
			Root::ExRef<Spin> d_spin;
			float d_fit;
		};

		typedef Root::Vector<Spin*> Spins;
		typedef Root::Vector< std::pair<Spin*,float> > SpinRanking;
		typedef Root::FixVector<Spin*> SpinTuple; // RISK: Referenz indirekt geschtzt.

		static Spin* selectSpin( QWidget*, const SpinRanking&, const char* title );
		static SpinTuple selectSpinPair( QWidget*, const SpinRanking&, const SpinRanking&, 
			const char* title );
		static SpinTuple selectSpinTriple( QWidget*, const SpinRanking&, const SpinRanking&, 
			const SpinRanking&,	const char* title );
        static SpinTuple selectSpinQuadruple( QWidget*, const SpinRanking&, const SpinRanking&,
			const SpinRanking&,	const SpinRanking&, const char* title );

		static SpinPoint selectTuple( QWidget*, const SpinSpace::Result&, Dimension, 
			const char* title );

		typedef std::set<QByteArray > StringSet;
		static bool selectStrings( QWidget*, const char* title, StringSet&, bool multi = true );

		static PeakList* selectPeakList( QWidget * parent, Project* );
		static bool getBounds( QWidget * parent, Spec::Amplitude& min, 
			Spec::Amplitude& max, const char* title = "Get Slice Bounds" );
		static void showInfo( QWidget*, const char* title, const char* text );
		struct ContourParams
		{
			Root::Float d_factor;
			Spec::ContourView::Option d_option;
			Spec::Amplitude d_threshold;
		};
		static bool setParams( QWidget* parent, ContourParams& );
		static bool getPredSucc( QWidget*, int& pred, int& succ );
		static bool getFrameSize( QWidget*, float& w, float& h );	// Milimeter
		static bool getPpmRange( QWidget*, PpmRange& w, PpmRange& h, const char* = "Set PPM Range" );
		static bool getLabels( QWidget*, Root::Index x, Root::Index y, 
			SpinLabel& lx, SpinLabel& ly, 
			const SpinLabelSet& sx = SpinLabelSet(),
			const SpinLabelSet& sy = SpinLabelSet() );
        static bool getLabelPoint(QWidget * parent, const SpinLabelPoints& pairs, SpinLabelPoint& res, Dimension count, const char *title);
		static bool getLabel( QWidget*, SpinLabel& lx, const SpinLabelSet& sx = SpinLabelSet() );
		static bool getLabelPair( QWidget*, Root::SymbolString&, Root::SymbolString&,
			const char* = "Select Label Symbols", 
			const char* = "CA-Symbol:", const char* = "CB-Symbol:" );
        static bool getPpmPoint( QWidget*, PpmPoint&, const QString& = "Edit Peak", const QStringList& labels = QStringList(), bool onlyLabels = false );
		static bool getPhase(QWidget * parent, float &phi, float &psi);
		static bool getLocation( QWidget*, Location&, const char* = "Edit Location", bool r = true );

		static bool getSubMatCount( QWidget*, Spectrum*, Root::Extension&, double& );

		typedef std::vector<QByteArray > StringList;
		static int getOption(QWidget * parent, const StringList&, const char*, int init = -1 );
		static bool getStrings(QWidget * parent, StringList& inout, const char* title);

		struct SpinParams
		{
			AtomType d_type;
			PPM d_shift;
			SpinLabel d_label;
		};
		static bool getSpinParams( QWidget*, SpinParams& );

		struct Value
		{
			DisPar d_dp;
			Root::SymbolString d_name;
		};
		static bool getValue( QWidget*, Value&, bool setName = false );

		static bool getResolution( QWidget* parent, Root::Long& sh, Root::UInt8& ch,
								Root::Long& sv, Root::UInt8& cv );
		static bool getSpectrumFormat(QWidget * parent, int& res, 
			Spec::Amplitude& pmax, Spec::Amplitude& nmax );

		struct LinkParams
		{
			AtomType d_atom;
			bool d_onlyInter;
			bool d_useShifts;
			bool d_hideOthers;
			LinkParams():d_onlyInter(true),d_useShifts(true),d_hideOthers(true) {}
		};
		static bool getLinkParams( QWidget*, int count, LinkParams& );

		struct LinkParams2
		{
			Root::Float d_rating;
			Root::UInt8 d_code;
			bool d_visible;
			LinkParams2():d_rating(0),d_code(0),d_visible(true) {}
		};
		static bool getLinkParams2( QWidget*, LinkParams2& );

		typedef Root::FixPoint<Root::Index, PpmPoint::MAX_DIM > Assig;	// Verweis auf Spin-IDs
		static bool getAssig( QWidget*, Dimension, Assig&, float* p, const char* = "Edit Assignment" );

		struct LP
		{
			SpinLabel d_x;
			SpinLabel d_y;
			SystemType* d_sys;
			LP():d_sys(0) {}
		};
		static bool getLabelsSysType( QWidget* parent, LP&, 
			Repository* rep, SpectrumType* st, Dimension x, Dimension y );

		static bool adjustIntensity( QWidget* parent, IntensityView* );
	};

	// Private Klasse, nur hier wegen Q_OBJECT und moc
	class _LabelSysTypeDlg : public QDialog
	{
		Q_OBJECT
	public:
		Repository* d_rep;
		Q3ComboBox* d_lx;
		Q3ComboBox* d_ly;
		Q3ComboBox* d_z;
		QCheckBox* d_r;
		SpectrumType* d_st;
		Dimension d_x, d_y;

		_LabelSysTypeDlg( QWidget* parent, Repository* rep, SpectrumType* st, 
			Dimension x, Dimension y );
		SystemType* getSysType() const { return s_sys; }
		void setSysType( SystemType* s );

		bool select( SpinLabel& lx, SpinLabel& ly );

	private:
		static SystemType* s_sys;
		static bool s_remember;
		static SpinLabel s_lx, s_ly;

	public slots:
		void handleSysType();
	};
	class ColorEdit : public QDialog
	{
		Q_OBJECT
	public:
		IntensityView* d_iv;
		Root::UInt8 d_intens, d_thres;
		ColorEdit(QWidget* parent, IntensityView* iv);
		bool run();
	private slots:
		void handleIntensity( int v );
		void handleThreshold( int v );
	};

}

#endif // !defined(AFX_DLG_H__F6AC8148_E916_4874_A79D_BFACA432AA6F__INCLUDED_)
