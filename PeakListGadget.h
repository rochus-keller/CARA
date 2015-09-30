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

#if !defined(AFX_PEAKLISTGADGET_H__38D8F53F_1EE8_46C2_AF07_67CFEE37D5A3__INCLUDED_)
#define AFX_PEAKLISTGADGET_H__38D8F53F_1EE8_46C2_AF07_67CFEE37D5A3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Lexi/Interactor.h>
#include <Spec/PeakList.h>	// wegen PeakColorList
#include <Root/ActionHandler.h>
#include <Lexi/LexiListView.h>

class _PeakController;

namespace Spec
{
	class Project;

	class PeakListGadget : public Lexi::Interactor  
	{
	public:
		static Root::Action::CmdStr AddColumn;
		static Root::Action::CmdStr RemoveCols;

		Root::Index getSelected() const;
		//. Gebe die ID des selektierten Peaks oder -1
		void setPeakList( PeakList*, Spectrum* );
		PeakListGadget( Lexi::Viewport*, PeakList* = 0, Spectrum* = 0,
			QObject* main = 0, Lexi::Glyph* popup = 0,
			bool showVol = false, Project* = 0 );
		// Wenn PointSet=0 wird eine leere Liste angezeigt.
		void reload();
		void lock();
		void setSpec( Spectrum* );
		void showAssig( bool );
		bool showAssig() const;
		bool gotoPeak( Root::Index );
		Peak* getPeak( bool recursive = true) const;
		Peak::Guess* getGuess() const;
		void saveTable( const char* path );
		void setPopup( Lexi::Glyph* );
		Lexi::ListView* getListView() const;

		//* Callback Messages
		class ActivatePeakMsg : public Root::UserMessage
		{
			Root::Index d_peak;
		public:
			ActivatePeakMsg( Root::Index l ):d_peak( l ) {}
			Root::Index getPeak() const { return d_peak; }
		};
	protected:
		virtual ~PeakListGadget();
		void handle( Root::Message& );

	private:
		void handleRemoveCols( Root::Action& );
		void handleAddColumn( Root::Action& );
		FRIEND_ACTION_HANDLER( PeakListGadget );

		_PeakController* d_this;
	};
}

#endif // !defined(AFX_PEAKLISTGADGET_H__38D8F53F_1EE8_46C2_AF07_67CFEE37D5A3__INCLUDED_)
