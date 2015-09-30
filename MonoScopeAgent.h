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
#include <Spec/SpecProjector.h>
#include <Spec/PeakList.h>
#include <Spec/BackCalculation.h>
#include <Root/Vector.h>
#include <SpecView/SpecViewer.h>
#include <SpecView/ContourView.h>
#include <SpecView/SpecBufferMdl.h>
#include <SpecView/IntensityView.h>
#include <Spec/Project.h>
#include <Lexi/Layer.h>
#include <Gui/Menu.h>
#include <SpecView/CursorMdl.h>
#include <SpecView/ViewAreaMdl.h>

namespace Spec
{
	class CursorMdl;
	class ViewAreaMdl;
	class SpecViewer;
	class PeakPlaneView;
	class PeakProjector;
	class SliceView;
	class PeakModelView;
	class SpecBufferMdl;
	using Root::Action;

	class MonoScopeAgent : public Root::Agent
	{
	public:
		static Action::CmdStr SetResolution;
		static Action::CmdStr ShowLowRes;
		static Action::CmdStr Forward;
		static Action::CmdStr Backward;
		static Action::CmdStr FitWindow;
		static Action::CmdStr FitWindowX;
		static Action::CmdStr FitWindowY;
		static Action::CmdStr PickPeak;
		static Action::CmdStr MovePeak;
		static Action::CmdStr MovePeakAlias;
		static Action::CmdStr LabelPeak;
		static Action::CmdStr ForceLabel;
		static Action::CmdStr DeletePeaks;
		static Action::CmdStr PeakCalibrate;	
		static Action::CmdStr ShowFolded;
		static Action::CmdStr ViewLabels;
		static Action::CmdStr SpecCalibrate;
		static Action::CmdStr AutoCenter;
		static Action::CmdStr DeleteAliasPeaks;

		static Action::CmdStr Goto;
		static Action::CmdStr ShowContour;	
		static Action::CmdStr ShowIntensity;
		static Action::CmdStr AutoContour;
		static Action::CmdStr ContourParams;
		static Action::CmdStr PeakRotate;	
		static Action::CmdStr SpecRotate;	
		static Action::CmdStr MapPeakList;	

		static Action::CmdStr ShowModel;	
		static Action::CmdStr ShowBaseWidth;	
		static Action::CmdStr TunePeakModel;	
		static Action::CmdStr UpdateAllAmps;	
		static Action::CmdStr IntegrateAll;	
		static Action::CmdStr IntegrateSel;	
		static Action::CmdStr ShowBackCalc;	
		static Action::CmdStr ShowDiff;	
		static Action::CmdStr ExactBackCalc;	
		static Action::CmdStr SelectSpec;	
		static Action::CmdStr BatchIntegrate;	
		static Action::CmdStr PeakCurve;	

		static Action::CmdStr PickBounds;	
		static Action::CmdStr SetSliceMinMax;	
		static Action::CmdStr SliceAutoScale;	
		static Action::CmdStr PickBoundsSym;	

		static Action::CmdStr ForwardPlane;	
		static Action::CmdStr BackwardPlane;	
		static Action::CmdStr CreateReport;	
		static Action::CmdStr EditAtts;
		static Action::CmdStr SetColor;
		static Action::CmdStr SelectPeaks;
		static Action::CmdStr CursorSync;
		static Action::CmdStr CalcMaximum;
		static Action::CmdStr AutoGain;
		static Action::CmdStr RangeSync;
		static Action::CmdStr SetDepth;
		static Action::CmdStr SyncDepth;

		static Action::CmdStr OverlayCount;
		static Action::CmdStr ActiveOverlay;
		static Action::CmdStr SetPosColor;
		static Action::CmdStr SetNegColor;
		static Action::CmdStr OverlaySpec;
		static Action::CmdStr CntFactor;
		static Action::CmdStr CntThreshold;
		static Action::CmdStr CntOption;
		static Action::CmdStr AddLayer;
		static Action::CmdStr ComposeLayers;
		static Action::CmdStr AdjustIntensity;
		static Action::CmdStr SetIntMeth;

		struct PlaneSocket
		{
			struct Overlay
			{
				Root::Ref<ContourView> d_view;
				Root::Ref<SpecBufferMdl> d_buf;
				Root::Ref<SpecProjector> d_spec;
				/* folgende bereits in ContourView
				float d_gain; 
				float d_contourFactor;
				ContourView::Option d_contourOption;
				bool d_autoContour;
				*/
			};
			Root::Ref<SpecViewer> d_viewer;
			Root::Ref<IntensityView> d_intens;
			CursorMdl* d_cur;
			PeakPlaneView* d_peaks;
			Root::Vector<Overlay> d_ol;
			PlaneSocket():d_cur(0) {}
		};
		struct SliceSocket
		{
			Root::Ref<SpecViewer> d_viewer;
			CursorMdl* d_cur;
			SpecProjector* d_spec;
			PeakModelView* d_peak;
			SliceView* d_slice;
			Root::Ref<SpecBufferMdl> d_buf;	// RefCount kann sonst auf Null gehen.
			SliceSocket():d_cur(0),d_spec(0),d_buf(0),d_slice(0){}
		};

		bool isDirty() const;
		void setCursor( PpmPoint = PpmPoint() );
		void setCursor( Root::Index );
		const PpmPoint& getCursor() const { return d_cursor; }

		void fitToView();
		void initOrigin();
		void setPeaklist( PeakList*, bool autorot = false );
		PeakList* getPeaklist() const { return d_peaks; }
		void setAutoCenter( bool on = true ) { d_autoCenter = on; }

		SpecViewer* createSliceViewer( Dimension view, Dimension spec );
		SpecViewer* getSliceViewer( Dimension d ) const { return d_slices[ d ].d_viewer; }
		SpecViewer* createPlaneViewer();
		SpecViewer* getPlaneViewer() const { return d_plane.d_viewer; }
		const PlaneSocket& getPlane() const { return d_plane; }
		Gui::Menu* getPopSpec() const { return d_popSpec; }
		Gui::Menu* getPopHisto() const { return d_popHisto; }
		Project* getPro() const { return d_pro; }

		bool showSlices() const { return d_showSlices; }
		void showSlices( bool on ) { d_showSlices = on; }
		void allocate();
		Spectrum* getMainSpec() const { return d_main; }
		Spectrum* getSpec() const { return d_spec; }
		bool setSpec( Spectrum* );
		// SpecBufferMdl* getPlaneBuf() const { return d_plane.d_buf; }
		MonoScopeAgent(Root::Agent* parent, Spectrum*, Project*, const Rotation&);
		void reloadSpecPop();

		typedef std::set<Root::Ref<PeakList> > Histo;
		const Histo& getHisto() const { return d_histo; }
	protected:
		void showIntens( bool on );
		void updateContour( int i, bool redraw );
		int selectLayer();
		void setActiveOverlay( int );
		void initOverlay( int );
		void initParams();
		void buildPopup();
		void registerPlane();
		virtual ~MonoScopeAgent();
		void handle(Root::Message&);
		void updatePlane( CursorMdl::Update* );
		void updateSlice( Dimension, CursorMdl::Update * msg );
		void updatePlane( ViewAreaMdl::Update* );
		void updateSlice( Dimension, ViewAreaMdl::Update * msg );
	private:
		void handleSetIntMeth(Action & a);
		void handleAdjustIntensity(Action & a);
		void handleDeleteAliasPeaks( Action& );
		void handleFitWindowY( Action& );
		void handleFitWindowX( Action& );
		void handleSyncDepth( Action& );
		void handleComposeLayers( Action& );
		void handleAddLayer( Action& );
		void handleCntOption( Action& );
		void handleCntThreshold( Action& );
		void handleCntFactor( Action& );
		void handleOverlaySpec( Action& );
		void handleSetNegColor( Action&);
		void handleSetPosColor( Action& );
		void handleActiveOverlay( Action& );
		void handleOverlayCount( Action& );
		void handleSetDepth( Action& );
		void handleRangeSync( Action& );
		void handleAutoGain( Action& );
		void handleCalcMaximum( Action& );
		void handleCursorSync( Action& );
		void handleSetColor( Action& );
		void handleEditAtts( Action& );
		void handleMovePeakAlias( Action& );
		void handleCreateReport(Action & a);
		void handleForwardPlane( Action& );
		void handleBackwardPlane( Action& );
		void handlePeakCurve( Action& );
		void handlePickBoundsSym( Action& );
		void handleSliceAutoScale( Action& );
		void handleSetSliceMinMax( Action& );
		void handlePickBounds( Action& );
		void handleBatchIntegrate( Action& );
		void handleExactBackCalc( Action& );
		void handleShowDiff( Action& );
		void handleShowBackCalc( Action& );
		void handleIntegrateSel( Action& );
		void handleIntegrateAll( Action& );
		void handleUpdateAllAmps( Action& );
		void handleTunePeakModel( Action& );
		void handleShowBaseWidth( Action& );
		void handleShowModel( Action& );
		void handleForceLabel( Action& );
		void handleMapPeakList( Action& );
		void handlePeakRotate( Action& );
		void handleGoto( Action& );
		void handleContourParams( Action& );
		void handleAutoContour( Action& );
		void handleShowIntensity( Action& );
		void handleShowContour( Action& );
		void handleSpecRotate( Root::Action& );
		void handleAutoCenter( Action& );
		void handleSpecCalibrate( Action& );
		void handlePeakCalibrate( Action& );
		void handleViewLabels( Action& );
		void handleShowFolded( Action& );
		void handleDeletePeaks( Action& );
		void handleMovePeak( Action& );
		void handlePickPeak( Action& );
		void handleBackward( Action& );
		void handleForward( Action& );
		void handleFitWindow( Action& );
		void handleShowLowRes( Action& );
		void handleSetResolution( Action& );
		FRIEND_ACTION_HANDLER( MonoScopeAgent );
		void notifyCursor();

		Rotation d_rot;
		SpecRef<Spectrum> d_spec;
		SpecRef<Spectrum> d_main;	// Das Spektrum, mit dem MonoSope geöffnet wurde
		Root::Ref<PeakList> d_peaks;
		PeakProjector* d_pp;
		Root::Ref<Project> d_pro; 
		Root::Ref<BackCalculation> d_back;

		PlaneSocket d_plane;
		Root::Vector<SliceSocket> d_slices;

		PpmPoint d_cursor;
		PeakList::IntegrationMethod d_intMeth;
		Root::Byte d_resol;
		int d_aol;	// Aktiver Overlay, 0..n-1
		bool d_lock;

		bool d_autoCenter;
		bool d_showSlices;

		bool d_lowResol;
		bool d_folding;
		bool d_showModel;
		bool d_showBase;
		bool d_showBack;
		bool d_cursorSync;
		bool d_syncDepth;
		bool d_rangeSync;
		Root::Deque< std::pair<PpmCube,PpmPoint> > d_backward;
		Root::Deque< std::pair<PpmCube,PpmPoint> > d_forward;
		Gui::Menu* d_popSpec;
		Gui::Menu* d_popPlane;
		Gui::Menu* d_popHisto;

		Histo d_histo;
	};
}

#endif // !defined(AFX_MONOSCOPEAGENT_H__52C41A62_C31B_468C_BF3A_4E8E24923833__INCLUDED_)
