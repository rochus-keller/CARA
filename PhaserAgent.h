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

#if !defined(AFX_MONOSCOPEAGENT_H__52C41A62_C31B_468C_BF3A_4E8E24923833__INCLUDED_)
#define AFX_MONOSCOPEAGENT_H__52C41A62_C31B_468C_BF3A_4E8E24923833__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Root/Agent.h>
#include <Root/ActionHandler.h>
#include <Spec/Spectrum.h>
#include <Root/Vector.h>
#include <SpecView/SpecViewer.h>
#include <SpecView/ContourView.h>
#include <SpecView/SpecBufferMdl.h>
#include <Spec/PhasedSpec.h>
#include <Lexi/Layer.h>
#include <Lexi/Label.h>
#include <Gui/Menu.h>
#include <SpecView/CursorMdl.h>
#include <SpecView/ViewAreaMdl.h>

namespace Spec
{
	class CursorMdl;
	class ViewAreaMdl;
	class SpecViewer;
	class PeakPlaneView;
	class SliceView;
	class SpecBufferMdl;
	class SpecProjector;
	using Root::Action;

	class PhaserAgent : public Root::Agent
	{
	public:
		static Action::CmdStr SetResolution;
		static Action::CmdStr ShowLowRes;
		static Action::CmdStr Forward;
		static Action::CmdStr Backward;
		static Action::CmdStr FitWindow;
		static Action::CmdStr ShowFolded;
		static Action::CmdStr Goto;
		static Action::CmdStr ShowContour;	
		static Action::CmdStr ShowIntensity;
		static Action::CmdStr AutoContour;
		static Action::CmdStr ContourParams;
		static Action::CmdStr SpecRotate;	
		static Action::CmdStr PickBounds;	
		static Action::CmdStr SetSliceMinMax;	
		static Action::CmdStr SliceAutoScale;	
		static Action::CmdStr PickBoundsSym;	
		static Action::CmdStr ForwardPlane;	
		static Action::CmdStr BackwardPlane;	
		static Action::CmdStr SetPhase;	
		static Action::CmdStr UseDimX;	
		static Action::CmdStr UseDimY;	
		static Action::CmdStr UseDimZ;	
		static Action::CmdStr SetPivot;	
		static Action::CmdStr PointPivot;	
		static Action::CmdStr AutoGain;

		struct PlaneSocket
		{
			Root::Ref<SpecViewer> d_viewer;
			CursorMdl* d_cur;
			CursorMdl* d_pivot;
			SpecProjector* d_spec;
			Root::Ref<SpecBufferMdl> d_buf;	// RefCount kann sonst auf Null gehen.
			PlaneSocket():d_cur(0),d_spec(0),d_buf(0),d_pivot(0) {}
		};
		struct SliceSocket
		{
			Root::Ref<SpecViewer> d_viewer;
			CursorMdl* d_cur;
			CursorMdl* d_pivot;
			SpecProjector* d_spec;
			SliceView* d_slice;
			Root::Ref<SpecBufferMdl> d_buf;	// RefCount kann sonst auf Null gehen.
			SliceSocket():d_cur(0),d_spec(0),d_buf(0),d_slice(0),d_pivot(0){}
		};
		struct PhaserSocket
		{
			Root::Ref<SpecViewer> d_viewer;
			Root::Ref<Lexi::Label> d_label;
		};

		void setCursor( const PpmPoint& );
		const PpmPoint& getCursor() const { return d_cursor; }
		bool isAutoContour() const { return d_autoContour; }
		void setAutoContour( bool on );
		ContourView::Option getContourOption() const { return d_contourOption; }
		Amplitude getContourLevel() const { return d_contourLevel; }
		float getContourFactor() const { return d_contourFactor; }
		void setContourParams(Amplitude, float, ContourView::Option);
		void showIntens( bool on, bool redraw = true );
		bool showIntens() const { return d_showIntens; }
		void showContour( bool on, bool redraw = true );
		bool showContour() const { return d_showContour; }

		void fitToView();
		void initOrigin();

		SpecViewer* createSliceViewer( Dimension view, Dimension spec );
		SpecViewer* getSliceViewer( Dimension d ) const { return d_slices[ d ].d_viewer; }
		SpecViewer* createPlaneViewer();
		SpecViewer* getPlaneViewer() const { return d_plane.d_viewer; }
		SpecViewer* createPhaser();
		const PlaneSocket& getPlane() const { return d_plane; }

		void setCurDim( Dimension );
		void allocate();
		PhasedSpec* getSpec() const { return d_spec; }
		SpecBufferMdl* getPlaneBuf() const { return d_plane.d_buf; }
		PhaserAgent(Root::Agent* parent, Spectrum* real );
		void setPhiPsi( float, float, bool );
		float getPhi() const { return d_spec->getPhi( d_curDim ); }
		float getPsi() const { return d_spec->getPsi( d_curDim ); }
		Dimension getCurDim() const { return d_curDim; }
	protected:
		void updatePane();
		void initParams();
		void buildPopup();
		void registerPlane();
		virtual ~PhaserAgent();
		void handle(Root::Message&);
		void updatePlane( CursorMdl::Update* );
		void updateSlice( Dimension, CursorMdl::Update * msg );
		void updatePlane( ViewAreaMdl::Update* );
		void updateSlice( Dimension, ViewAreaMdl::Update * msg );
	private:
		void handleAutoGain( Action& );
		void handlePointPivot( Action& );
		void handleSetPivot( Action& );
		void handleUseDimZ( Action& );
		void handleUseDimY( Action& );
		void handleUseDimX( Action& );
		void handleSetPhase( Action& );
		void handleForwardPlane( Action& );
		void handleBackwardPlane( Action& );
		void handlePickBoundsSym( Action& );
		void handleSliceAutoScale( Action& );
		void handleSetSliceMinMax( Action& );
		void handlePickBounds( Action& );
		void handleGoto( Action& );
		void handleContourParams( Action& );
		void handleAutoContour( Action& );
		void handleShowIntensity( Action& );
		void handleShowContour( Action& );
		void handleSpecRotate( Root::Action& );
		void handleShowFolded( Action& );
		void handleDeletePeaks( Action& );
		void handleMovePeak( Action& );
		void handlePickPeak( Action& );
		void handleBackward( Action& );
		void handleForward( Action& );
		void handleFitWindow( Action& );
		void handleShowLowRes( Action& );
		void handleSetResolution( Action& );
		FRIEND_ACTION_HANDLER( PhaserAgent );
		void notifyCursor();
		SpecRef<PhasedSpec> d_spec;

		PlaneSocket d_plane;
		PhaserSocket d_phaser;
		Root::Vector<SliceSocket> d_slices;

		PpmPoint d_cursor;
		Dimension d_curDim;

		float d_gain;
		float d_contourFactor;
		ContourView::Option d_contourOption;
		Amplitude d_contourLevel;
		Root::Byte d_resol;

		bool d_lock;
		bool d_showContour;
		bool d_showIntens;

		bool d_autoContour;
		bool d_lowResol;
		bool d_folding;
		Root::Deque< std::pair<PpmCube,PpmPoint> > d_backward;
		Root::Deque< std::pair<PpmCube,PpmPoint> > d_forward;
		Gui::Menu* d_popPlane;
		Gui::Menu* d_popSlice;
		Gui::Menu* d_popPane;
	};
}

#endif // !defined(AFX_MONOSCOPEAGENT_H__52C41A62_C31B_468C_BF3A_4E8E24923833__INCLUDED_)
