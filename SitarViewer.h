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

#if !defined(AFX_SITARVIEWER_H__4376600C_401A_48F2_B9DF_B8FE741F2EFF__INCLUDED_)
#define AFX_SITARVIEWER_H__4376600C_401A_48F2_B9DF_B8FE741F2EFF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Lexi/MainWindow.h>
#include <Root/ActionHandler.h>
#include <Lexi/GlyphWidget.h>
#include <Lexi/FocusManager.h>
#include <Spec/Units.h>
#include <SpecView/SpecView.h>
#include <Spec/SitarSpectrum.h>
#include <SpecView/CursorMdl.h>
#include <SpecView/ViewAreaMdl.h>

namespace Spec
{
	using Root::Action;
	class Spectrum;
	class SitarSpectrum;
	class ContourView;
	class SpecBufferMdl;
	class SpecProjector;
	class SpecViewer;
	class CursorMdl;
	class SliceView;
	class OverviewCtrl;
	class CursorView;

	class SitarCurveView : public SpecView
	{
	public:
		bool isVisible() const { return d_visi; }
		void setVisible( bool on );
		SitarCurveView(SpecViewer*, SitarSpectrum*, CursorView*);
		void setColor( QColor c );
		//* Overrides von Glyph
		void draw( Canvas&, const Allocation& );
	private:
		SitarSpectrum* d_spec;
		CursorView* d_cur;
		QColor d_clr;
		bool d_visi;
	};

	class SitarViewer : public Lexi::MainWindow
	{
	public:
		struct PlaneSocket
		{
			ContourView* d_viewL;
			SpecBufferMdl* d_bufL;
			SpecProjector* d_specL;
			ContourView* d_viewR;
			SpecBufferMdl* d_bufR;
			SpecProjector* d_specR;
			SpecViewer* d_viewer;
			CursorMdl* d_cur;
			CursorView* d_lhs;
			CursorView* d_rhs;
			SitarCurveView* d_curve;
			PlaneSocket():d_cur(0),d_specL(0),d_specR(0) {}
		};
		struct SliceSocket
		{
			SpecViewer* d_viewer;
			CursorMdl* d_cur;
			SpecProjector* d_specL;
			SliceView* d_sliceL;
			SpecBufferMdl* d_bufL;	
			SpecProjector* d_specR;
			SliceView* d_sliceR;
			SpecBufferMdl* d_bufR;	
			CursorView* d_lhs;
			CursorView* d_rhs;
			SliceSocket():d_cur(0),d_specL(0),d_bufL(0),d_sliceL(0),
				d_specR(0),d_bufR(0),d_sliceR(0){}
		};
		struct StripSocket
		{
			ContourView* d_view;
			SpecViewer* d_viewer;
			CursorMdl* d_cur; 
			SpecProjector* d_spec;
			SpecBufferMdl* d_buf;
			StripSocket():d_spec(0) {}
		};

		SitarViewer(Root::Agent *, Spectrum*);
	protected:
		void centerY();
		void handleSyncDepth( Action& );
		void setOffCursor( PPM );
		void updateContour2(Dimension d, bool ac);
		void syncStripsToCur();
		void adjustN();
		void updatePlane(CursorMdl::Update * msg);
		void updateSlice(Dimension dim, CursorMdl::Update *msg);
		void updateSlice(Dimension dim, ViewAreaMdl::Update *msg);
		void updatePlane(ViewAreaMdl::Update * msg);
		void updateStrip(Dimension dim, ViewAreaMdl::Update *msg);
		void updateStrip(Dimension dim, CursorMdl::Update *msg);
		void notifyCursor(bool plane = true );

		void createSlice(Dimension view, Dimension spec );
		void createStrip( Dimension );
		void createPlane();

		void updateCaption();
		void buildViews();
		void buildPopups();
		void buildMenus();
		void handle(Root::Message&);
		virtual ~SitarViewer();
	private:
		void handleShowSumSlice( Action& );
		void handleOneOff( Action& );
		void handleRectify( Action& );
		void handleShowLhs( Action& );
		void handleShowRhs( Action& );
		void handleContourParams( Action& );
		void handleAutoContour( Action& );
		void handleLineSlices( Action& );
		void handleClip( Action& );
		void handleInterpolate( Action& );
		void handleMatchGrad( Action&);
		void handleShowCurve( Action& );
		void handleFitWindow( Action& );
		void handleRangeSync(Action & a);
		void handleCursorSync( Action& );
		void handleSetPeakWidht( Action& );
		void handleFitWindow2( Action& );
		void handleContourParams2( Action& );
		void handleAutoContour2( Action& );
		FRIEND_ACTION_HANDLER( SitarViewer );

		Root::Ref<SliceView> d_sliceSum;
		Root::Ref<SpecBufferMdl> d_bufSum;
		Root::Ref<SpecProjector> d_specSum;

		Spec::SpecRef<SitarSpectrum> d_spec;
		Lexi::GlyphWidget* d_widget;
		Root::Ref<Lexi::FocusManager> d_focus;
		PpmPoint d_cursor;
		PlaneSocket d_plane;
		Root::Vector<SliceSocket> d_slices;
		Root::Vector<StripSocket> d_strips;			
		Root::Ref<SpecViewer> d_ov;
		Root::Ref<OverviewCtrl> d_ovCtrl;
		PPM d_off; // H-Offset
		PPM d_pw[2];
		bool d_lock;
		bool d_cursorSync;
		bool d_syncDepth;
		bool d_rangeSync;
	};
}

#endif // !defined(AFX_SITARVIEWER_H__4376600C_401A_48F2_B9DF_B8FE741F2EFF__INCLUDED_)
