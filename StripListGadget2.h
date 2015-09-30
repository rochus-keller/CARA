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

#if !defined(StripListGadget2__INCLUDED_)
#define StripListGadget2__INCLUDED_

#include <Gui/ListView.h>
#include <Gui2/AutoMenu.h>
#include <Spec/Project.h>

class _SystemItem2;
class _PeakLabelItem2;
class _SpinLinkItem3;

namespace Spec
{
	class Project;
	class SpinSystem;
	class Spin;
	class Spectrum;

    class StripListGadget2 : public Gui::ListView, public Root::Messenger
	{
        Q_OBJECT
	public:
        StripListGadget2( QWidget* parent, Root::Agent*, Project*, bool links = false );
        void addCommands( Gui2::AutoMenu* );

        void showStrip( SpinSystem* );
		SpinSystem* getCandPred() const; // selektierter Link als potentieller Predecessor des Strips
		SpinSystem* getCandSucc() const; // selektierter Link als potentieller Successor des Strips
		SpinSystem* getSelectedStrip();
		Spin* getSelectedSpin() const;
		Spin* getSelectedLink() const;
		bool gotoSpin( Spin* );
        Spectrum* getSpec() const { return d_spec; }
		void setSpec( Spectrum* );
    public slots:
        void handleLinkParams();
        void handleCloseAll();
        void handleOpenAll();
        void handleShowTable();
        void handleAcceptLabel();
        void handleGotoOther();
        void handleFindSpin();
        void handleShowLinks();
        void handleCreateLink();
        void handleEatSystem();
        void handleCreateSystem();
        void handleForceLabel();
        void handleCreateSpin();
        void handleLabelSpin();
        void handleMoveSpin();
        void handleMoveSpinAlias();
        void handleSetSysType();
        void handleDelete();
        void handleLinkThis();
        void handleShowAlignment();
        void handleUnlinkSucc();
        void handleUnlinkPred();
        void handleAssign();
        void handleUnassign();
        void handleRunStripper();
        void handleUnlabeledStripMatch();
        void handleStrictStripMatch();
        void handleSetSpinTol();
        void handleSetCandidates();
        void handleEditAtts();
	protected:
		virtual ~StripListGadget2();
		void handle( Root::Message& );
        void loadAllStrips();
        void updateItem( _SystemItem2 * s, bool repaint = true );
        _PeakLabelItem2* createSpinView(_SystemItem2 * s, Spin* spin);
        _SystemItem2* createSysView( Gui::ListView* v, SpinSystem* sys );
        _SpinLinkItem3* createLinkView( _PeakLabelItem2 * p, SpinLink* l);
        SpinSystem* getSys() const;
        Spin* getLink() const;
        void clearMatches( _SystemItem2* s );
        void fillMatches( _SystemItem2* s );
        // Overrides
        void onCurrentChanged();
    private:
        Root::Ref<Project> d_pro;

        Root::Ptr<Root::Agent> d_agent;
        Root::Ref<Spectrum> d_spec;
        bool d_showLinks;

        QHash<Root::Index, _SystemItem2*> d_map; // SpinSystem ID
	};
}

#endif // !defined(StripListGadget2__INCLUDED_)
