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

#include "SpinSystemView.h"
#include <qinputdialog.h>
#include <qmessagebox.h>
#include <qlabel.h>
#include <qlineedit.h> 
#include <qlayout.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qcheckbox.h> 
#include <Root/Any.h>
#include <Spec/Repository.h>
#include <AidaCentral.h>
#include <Dlg.h>
#include <SpecView/CandidateDlg.h>
#include <SpecView/DynValueEditor.h>
#include <SpecView/ObjectListView.h>
#include <Root/Vector.h>
#include <stdio.h>
#include "LuaCaraExplorer.h"
using namespace Spec;
 
static char s_buf[64];
static const int s_off = 10000; // Wegen Sortierung

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr SpinSystemView::ForceLabel = "ForceLabel";
Root::Action::CmdStr SpinSystemView::SelectSys = "SelectSys";
Root::Action::CmdStr SpinSystemView::SelectAll = "SelectAll";
Root::Action::CmdStr SpinSystemView::SelectCur = "SelectCur";
Root::Action::CmdStr SpinSystemView::CreateSpin = "CreateSpin";
Root::Action::CmdStr SpinSystemView::DeleteSpin = "DeleteSpin";
Root::Action::CmdStr SpinSystemView::LabelSpin = "LabelSpin";
Root::Action::CmdStr SpinSystemView::MoveSpin = "MoveSpin";
Root::Action::CmdStr SpinSystemView::MoveSpinAlias = "MoveSpinAlias";
Root::Action::CmdStr SpinSystemView::AssignSpin = "AssignSpin";
Root::Action::CmdStr SpinSystemView::UnassignSpin = "UnassignSpin";
Root::Action::CmdStr SpinSystemView::AssignSystem = "AssignSystem";
Root::Action::CmdStr SpinSystemView::UnassignSystem = "UnassignSystem";
Root::Action::CmdStr SpinSystemView::SetSysType = "SetSysType";
Root::Action::CmdStr SpinSystemView::SetCandidates = "SetCandidates";
Root::Action::CmdStr SpinSystemView::RemoveAlias = "RemoveAlias";
Root::Action::CmdStr SpinSystemView::EditSpinAtts = "EditSpinAtts";
Root::Action::CmdStr SpinSystemView::EditSysAtts = "EditSysAtts";
Root::Action::CmdStr SpinSystemView::ShowTable = "ShowTable";
Root::Action::CmdStr SpinSystemView::SetLoc = "SetLoc";
Root::Action::CmdStr SpinSystemView::ClearLoc = "ClearLoc";

ACTION_SLOTS_BEGIN( SpinSystemView )
    { SpinSystemView::SetLoc, &SpinSystemView::handleSetLoc },
    { SpinSystemView::ClearLoc, &SpinSystemView::handleClearLoc },
    { SpinSystemView::ShowTable, &SpinSystemView::handleShowTable },
    { SpinSystemView::EditSpinAtts, &SpinSystemView::handleEditSpinAtts },
    { SpinSystemView::EditSysAtts, &SpinSystemView::handleEditSysAtts },
    { SpinSystemView::MoveSpin, &SpinSystemView::handleMoveSpin },
    { SpinSystemView::MoveSpinAlias, &SpinSystemView::handleMoveSpinAlias },
    { SpinSystemView::RemoveAlias, &SpinSystemView::handleRemoveAlias },
    { SpinSystemView::SetCandidates, &SpinSystemView::handleSetCandidates },
    { SpinSystemView::SetSysType, &SpinSystemView::handleSetSysType },
    { SpinSystemView::AssignSpin, &SpinSystemView::handleAssignSpin },
    { SpinSystemView::UnassignSpin, &SpinSystemView::handleUnassignSpin },
    { SpinSystemView::AssignSystem, &SpinSystemView::handleAssignSystem },
    { SpinSystemView::UnassignSystem, &SpinSystemView::handleUnassignSystem },
    { SpinSystemView::CreateSpin, &SpinSystemView::handleCreateSpin },
    { SpinSystemView::DeleteSpin, &SpinSystemView::handleDeleteSpin },
    { SpinSystemView::LabelSpin, &SpinSystemView::handleLabelSpin },
    { SpinSystemView::SelectSys, &SpinSystemView::handleSelectSys },
    { SpinSystemView::SelectAll, &SpinSystemView::handleSelectAll },
    { SpinSystemView::SelectCur, &SpinSystemView::handleSelectCur },
    { SpinSystemView::ForceLabel, &SpinSystemView::handleForceLabel },
ACTION_SLOTS_END( SpinSystemView )

//////////////////////////////////////////////////////////////////////

class _SpinListItem : public Gui::ListViewItem
{
public:
	_SpinListItem( Gui::ListView* p, Spin* s ):
		Gui::ListViewItem(p),d_spin( s ) {}

	Root::ExRef<Spin> d_spin;

	QString text( int f ) const 
	{ 
		switch( f )
		{
		case 0:
			::sprintf( s_buf, "%d", d_spin->getId() );
			return s_buf;
		case 1:
			return d_spin->getAtom().getIsoLabel();
		case 2:
			d_spin->getLabel().format( s_buf, sizeof( s_buf ) );
			return s_buf;
		case 3:
			::sprintf( s_buf, "%0.3f", d_spin->getShift() );
			return s_buf;
		case 4:
			if( d_spin->getSystem() )
			{
				::sprintf( s_buf, "%d", d_spin->getSystem()->getId() );
				return s_buf;
			}else
				return "";
		case 5:
			if( d_spin->getSystem() && d_spin->getSystem()->getSysType() )
			{
				::sprintf( s_buf, "%s  ", d_spin->getSystem()->getSysType()->getName().data() );
				return s_buf;
			}else
				return "";
		case 6:
			if( d_spin->getSystem() )
			{
				d_spin->getSystem()->formatCands( s_buf, sizeof( s_buf ) );
				return s_buf;
			}else
				return "";
		case 7:
			if( d_spin->getSystem() && d_spin->getSystem()->getAssig() )
			{
				d_spin->getSystem()->formatLabel( s_buf, sizeof(s_buf) );
				return s_buf;
			}else
				return "";
		case 8:
			if( d_spin->getLoc() )
			{
				const Location* loc = d_spin->getLoc();
				::sprintf( s_buf, "[%0.2f,%0.2f,%0.2f]:%0.2f", 
					loc->d_pos[DimX], loc->d_pos[DimY], loc->d_pos[DimZ], loc->d_dev );
				return s_buf;
			}else
				return "";
		default:
			return "";
		}
	}
	QString key( int f, bool ascending  ) const
	{
		switch( f )
		{
		case 0:
			::sprintf( s_buf, "%06d", d_spin->getId() );
			return s_buf;
		case 3:
			if( d_spin->getShift() < 0.0 )
				::sprintf( s_buf, "A%+08.3f", double( d_spin->getShift() ) );
			else
				::sprintf( s_buf, "B%+08.3f", double( d_spin->getShift() ) );
			return s_buf;
		case 4:
			if( d_spin->getSystem() )
			{
				::sprintf( s_buf, "%06d", d_spin->getSystem()->getId() );
				return s_buf;
			}else
				return "";
		case 7:
			if( d_spin->getSystem() && d_spin->getSystem()->getAssig() )
			{
				::sprintf( s_buf, "%s%09d", 
					d_spin->getSystem()->getAssig()->getChain().data(),
					d_spin->getSystem()->getAssig()->getNr() + s_off );
				return s_buf;
			}else
				return "";
		case 2:
		case 1:
		case 5:
		case 6:
		case 8:
			return text( f );
		default:
			return "";
		}
	}
};

class _SpinAliasItem : public Gui::ListViewItem
{
public:
	_SpinAliasItem( Gui::ListViewItem* p, Spectrum* spec, PPM shift ):
		Gui::ListViewItem(p), d_spec( spec ), d_shift( shift ) {}
	Root::ExRef<Spectrum> d_spec;
	PPM d_shift;

	QString text( int f ) const 
	{ 
		switch( f )
		{
		case 0:
			::sprintf( s_buf, "%d %s", d_spec->getId(), d_spec->getName() );
			return s_buf;
		case 3:
			::sprintf( s_buf, "%0.3f", d_shift );
			return s_buf;
		default:
			return "";
		}
	}
};


//////////////////////////////////////////////////////////////////////

SpinSystemView::SpinSystemView(QWidget* p, AidaCentral* par, Spec::Project* r):
	Gui::ListView( p ), d_pro( r ), d_sys( 0 ), d_last( 0 ), d_parent( par )
{
	setShowSortIndicator( true );
	setRootIsDecorated( true );
	setAllColumnsShowFocus( true );
	addColumn( "Spin" );
	addColumn( "Atom" );
	addColumn( "Label" );
	addColumn( "Shift" );
	addColumn( "System" );
	addColumn( "Type" );
	addColumn( "Cand." );
	addColumn( "Assig." );
	addColumn( "Location" );
	refill();

	d_popLabel = new Gui::Menu( this, "Label Spin" );

	d_pro->addObserver( this );
}

SpinSystemView::~SpinSystemView()
{
	d_pro->removeObserver( this );
}

Gui::Menu* SpinSystemView::createPopup()
{
	Gui::Menu* pop = new Gui::Menu();
	Gui::Menu::item( pop, "Filter Current", SelectCur, false );
	Gui::Menu::item( pop, "Filter...", SelectSys, false );
	Gui::Menu::item( pop, "Show All", SelectAll, false );
	pop->insertSeparator();
	Gui::Menu::item( pop, "&Create Spin...", CreateSpin, false );
	Gui::Menu::item( pop, "&Move Spin...", MoveSpin, false );
	Gui::Menu::item( pop, "Move Spin &Alias...", MoveSpinAlias, false );
	Gui::Menu::item( pop, "Remove Alias", RemoveAlias, false );
	// TODO: pop->addMenu( d_popLabel );
	Gui::Menu::item( pop, "&Force Spin Label...", ForceLabel, false );
	Gui::Menu::item( pop, "&Delete Spin", DeleteSpin, false );
	pop->insertSeparator();
	Gui::Menu::item( pop, "Assign Spin...", AssignSpin, false );
	Gui::Menu::item( pop, "Unassign Spin", UnassignSpin, false );
	Gui::Menu::item( pop, "Set System Type...", SetSysType, false );
	Gui::Menu::item( pop, "Set Assig. Candidates...", SetCandidates, false );
	Gui::Menu::item( pop, "Assign System...", AssignSystem, false );
	Gui::Menu::item( pop, "Unassign System", UnassignSystem, false );
	pop->insertSeparator();
	Gui::Menu::item( pop, "Edit Location...", SetLoc, false );
	Gui::Menu::item( pop, "Clear Location", ClearLoc, false );
	pop->insertSeparator();
	Gui::Menu::item( pop, "Edit Spin Attributes...", EditSpinAtts, false );
	Gui::Menu::item( pop, "Edit System Attributes...", EditSysAtts, false );
	Gui::Menu::item( pop, "Open Object Table...", ShowTable, false );
	return pop; 
}

static void fillAlias( Project* pro, Gui::ListViewItem* i, Spin* s )
{
	i->clearChildren();
	const Spin::Shifts& ali = s->getShifts();
	Spin::Shifts::const_iterator p;
	Spectrum* spec;
	for( p = ali.begin(); p != ali.end(); ++p )
		if( (*p).first != s->getHome() )
		{
			spec = pro->getSpec( (*p).first );
			if( spec )
				new _SpinAliasItem( i, spec, (*p).second );
		}
}

void SpinSystemView::refill()
{
	clear();
	const SpinBase::SpinMap& sm = d_pro->getSpins()->getSpins();
	SpinBase::SpinMap::const_iterator p;
	if( d_sys == 0 )
	{
		for( p = sm.begin(); p != sm.end(); ++p )
			fillAlias( d_pro, new _SpinListItem( this , (*p).second ), (*p).second );
	}else
	{
		for( p = sm.begin(); p != sm.end(); ++p )
		{
			if( (*p).second->getSystem() && 
				(*p).second->getSystem()->getId() == d_sys )
				fillAlias( d_pro, new _SpinListItem( this , (*p).second ), (*p).second );
		}
	}
}

static _SpinListItem* _findItem( Gui::ListView* lv, Spin* spin )
{

    Gui::ListViewItemIterator it( lv );
    // iterate through all items of the listview
	_SpinListItem* i = 0;
    for ( ; it.current(); ++it ) 
	{
		i = dynamic_cast<_SpinListItem*>( it.current() );
		if( i && i->d_spin == spin )
			return i;
    }
	return 0;
}

void SpinSystemView::handle(Root::Message & msg)
{
	if( parent() == 0 )
		return;
	BEGIN_HANDLER();
	MESSAGE( Spin::Update, a, msg )
	{
		switch( a->getType() )
		{
		case Spin::Update::Create:
			fillAlias( d_pro, new _SpinListItem( this, a->getSpin() ), a->getSpin() );
			break;
		case Spin::Update::Delete:
			{
				_SpinListItem* i = _findItem( this, a->getSpin() );
				if( i )
					i->removeMe();
			}
			break;
		case Spin::Update::Shift:
			{
				_SpinListItem* i = _findItem( this, a->getSpin() );
				if( i )
				{
					i->repaint();
					fillAlias( d_pro, i, a->getSpin() );
				}
			}
			break;
		case Spin::Update::Label:
		case Spin::Update::System:
			{
				_SpinListItem* i = _findItem( this, a->getSpin() );
				if( i )
					i->repaint();
			}
			break;
		case Spin::Update::All:
			refill();
			break;
        default:
            break;
		}
		msg.consume();
	}
	MESSAGE( SpinSystem::Update, a, msg )
	{
		switch( a->getType() )
		{
		case SpinSystem::Update::Delete:
		case SpinSystem::Update::Assig:
		case SpinSystem::Update::SysType:
		case SpinSystem::Update::Candidates:
			viewport()->repaint( true );
			break;
		case SpinSystem::Update::All:
			refill();
			break;
        default:
            break;
		}
		msg.consume();
	}
	MESSAGE( Root::Action, a, msg )
	{
		EXECUTE_ACTION( SpinSystemView, *a );
	}
	END_HANDLER();
}

void SpinSystemView::onCurrentChanged()
{
	_SpinListItem* i = dynamic_cast<_SpinListItem*>( currentItem() );
	if( i == 0 )
	{
		_SpinAliasItem* a = dynamic_cast<_SpinAliasItem*>( currentItem() );
		if( a )
			i = (_SpinListItem*)a->parent();
	}

	if( i )
		LuaCaraExplorer::setCurrentSpin( i->d_spin );

	ResidueType* cur = 0;
	AtomType t;
	if( i && i->d_spin->getSystem() && i->d_spin->getSystem()->getAssig() )
	{
		cur = i->d_spin->getSystem()->getAssig()->getType();
		t = i->d_spin->getAtom();
	}
	if( cur != d_last || t != d_t )
	{
		d_popLabel->purge();
		Gui::Menu::item( d_popLabel, this, "?", LabelSpin, true )->addParam( Root::Any() );
		Gui::Menu::item( d_popLabel, this, "?-1", LabelSpin, true )->
			addParam( Root::Any( new SpinLabelHolder( "?-1" ) ) );
		if( cur )
		{
			ResidueType::Selection sel = cur->findAll( t );
			ResidueType::Selection::const_iterator p1;
			typedef std::set<QByteArray > Sort;
			Sort sort;
			Sort::const_iterator q1;
			for( p1 = sel.begin(); p1 != sel.end(); ++p1 )
			{
				sort.insert( (*p1).data() );
			}
			for( q1 = sort.begin(); q1 != sort.end(); ++q1 )
			{
				Gui::Menu::item( d_popLabel, this, (*q1).data(), 
					LabelSpin, true )->addParam( Root::Any( new SpinLabelHolder( (*q1).data() ) ) );
			}
		}
		d_last = cur;
		d_t = t;
	}
}

void SpinSystemView::handleForceLabel(Root::Action & a)
{
	_SpinListItem* item = dynamic_cast<_SpinListItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	assert( item );
	bool ok	= FALSE;
	QString res;
	res.sprintf( "Please enter a valid label (%s):", SpinLabel::s_syntax );
	res = QInputDialog::getText( "Force Spin Label", res, QLineEdit::Normal, 
			item->d_spin->getLabel().data(), &ok, this );
		if( !ok )
			return;

	SpinLabel l;
	if( !SpinLabel::parse( res, l ) )
	{
		Root::ReportToUser::alert( this, "Force Spin Label", "Invalid spin label syntax!" );
		return;
	}

	Root::Ref<LabelSpinCmd> cmd =
		new LabelSpinCmd( d_pro->getSpins(), item->d_spin, l );
	cmd->handle( d_parent );
}

void SpinSystemView::handleSelectSys(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );
	bool ok = false;
	int res = QInputDialog::getInteger( "Select Spin System",
		"Enter system id:", d_sys, 0, 99999999, 1, &ok, this );
	if( ok )
	{
		d_sys = res;
		refill();
	}
}

void SpinSystemView::handleSelectAll(Root::Action & a)
{
	ACTION_ENABLED_IF( a, d_sys > 0 );
	d_sys = 0;
	refill();
}

void SpinSystemView::handleSelectCur(Root::Action & a)
{
	_SpinListItem* i = dynamic_cast<_SpinListItem*>( currentItem() );
	ACTION_ENABLED_IF( a, i && i->d_spin->getSystem() );
	d_sys = i->d_spin->getSystem()->getId();
	refill();
}

void SpinSystemView::handleDeleteSpin(Action & a)
{
	_SpinListItem* item = dynamic_cast<_SpinListItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	Root::Ref<DeleteSpinCmd> cmd = new DeleteSpinCmd( d_pro->getSpins(), item->d_spin );
	cmd->handle( d_parent );
	// delete item;
}

void SpinSystemView::handleLabelSpin(Action & a)
{
	_SpinListItem* item = dynamic_cast<_SpinListItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	SpinLabelHolder* lh = dynamic_cast<SpinLabelHolder*>( a.getParam( 0 ).getObject() );
	SpinLabel l;
	if( lh )
		l = lh->d_label;
	Root::Ref<LabelSpinCmd> cmd =
		new LabelSpinCmd( d_pro->getSpins(), item->d_spin, l );
	cmd->handle( d_parent );
}

void SpinSystemView::handleCreateSpin(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	Dlg::SpinParams sp;
	if( !Dlg::getSpinParams( this, sp ) )
		return;
	Root::Ref<PickSystemSpinLabelCmd> cmd = new PickSystemSpinLabelCmd( 
		d_pro->getSpins(), 0, // Darf null sein.
		sp.d_type, sp.d_shift, sp.d_label, 0 ); 
		// Pick generisches Spektrum
	if( cmd->handle( d_parent ) )
	{
		_SpinListItem* i = _findItem( this, cmd->getSpin() );
		setSelected( i, true );
		ensureItemVisible( i );
	}
}

void SpinSystemView::handleAssignSpin(Action & a)
{
	_SpinListItem* item = dynamic_cast<_SpinListItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item && item->d_spin->getSystem() == 0 );

	bool ok = false;
	int res = QInputDialog::getInteger( "Select Spin System",
		"Enter system id:", d_sys, 1, 99999999, 1, &ok, this );
	if( !ok )
		return;
	SpinSystem* sys = d_pro->getSpins()->getSystem( res );
	if( sys == 0 )
	{
		if( QMessageBox::warning( this, "Assign Spin",
				  "The selected spin system doesn't exist. Do you want to create it?",
				  "&OK", "&Cancel" ) != 0 )	
				  return;
		sys = d_pro->getSpins()->addSystem( res );
	}
	Root::Ref<AssignSpinCmd> cmd = new AssignSpinCmd( 
		d_pro->getSpins(), item->d_spin, sys ); 
	cmd->handle( d_parent );
}

void SpinSystemView::handleUnassignSpin(Action & a)
{
	_SpinListItem* item = dynamic_cast<_SpinListItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item && item->d_spin->getSystem() );

	Root::Ref<UnassignSpinCmd> cmd = new UnassignSpinCmd( 
		d_pro->getSpins(), item->d_spin ); 
	cmd->handle( d_parent );
}

void SpinSystemView::handleAssignSystem(Action & a)
{
	_SpinListItem* item = dynamic_cast<_SpinListItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item && item->d_spin->getSystem() );

	QString str;
	SpinSystem* sys = item->d_spin->getSystem();
	Root::Index r = 0;

	bool ok;
	str.sprintf( "Assignment for spin system %d:", sys->getId() );
	r = QInputDialog::getInteger( "Assign Spin System", 
		str, (sys->getAssig())?sys->getAssig()->getId():0, 
		-999999, 999999, 1, &ok, this );
	if( !ok )
		return;

	Residue* res = d_pro->getSequence()->getResidue( r );
	if( res == 0 )
	{
		QMessageBox::critical( this, "Assign Spin System", 
			"Unknown residue number!", "&Cancel" );
		return;
	}

	Root::Ref<AssignSystemCmd> cmd =
		new AssignSystemCmd( d_pro->getSpins(), sys, res );
	cmd->handle( d_parent );
}

void SpinSystemView::handleUnassignSystem(Action & a)
{
	_SpinListItem* item = dynamic_cast<_SpinListItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item && item->d_spin->getSystem() && 
		item->d_spin->getSystem()->getAssig() != 0 );

	Root::Ref<UnassignSystemCmd> cmd =
		new UnassignSystemCmd( d_pro->getSpins(), item->d_spin->getSystem() );
	cmd->handle( d_parent );
}

void SpinSystemView::handleSetSysType(Action & a)
{
	_SpinListItem* item = dynamic_cast<_SpinListItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item && item->d_spin->getSystem() );

	Repository::SystemTypeMap::const_iterator p;
	const Repository::SystemTypeMap& sm = d_pro->getRepository()->getSystemTypes();

	int cur = 0, i = 0;
	QStringList l;
	l.append( "" );
	for( p = sm.begin(); p != sm.end(); ++p, i++ )
	{
		l.append( (*p).second->getName().data() ); // NOTE: Name sollte Unique sein.
	}
	l.sort();
	if( item->d_spin->getSystem()->getSysType() )
		cur = l.findIndex( item->d_spin->getSystem()->getSysType()->getName().data() );

	bool ok;
	QString res = QInputDialog::getItem( "Set System Type", "Select a spin system type:", 
		l, cur, false, &ok, this );
	if( !ok )
		return;

	SystemType* sst = 0;
	for( p = sm.begin(); p != sm.end(); ++p )
	{
		if( res == (*p).second->getName().data() )
		{
			sst = (*p).second;
			break;
		}
	}

	Root::Ref<ClassifySystemCmd> cmd =
		new ClassifySystemCmd( d_pro->getSpins(), item->d_spin->getSystem(), sst );
	cmd->handle( d_parent );
}

void SpinSystemView::handleSetCandidates(Action & a)
{
	_SpinListItem* item = dynamic_cast<_SpinListItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item && item->d_spin->getSystem() );

	CandidateDlg dlg( this, d_pro->getRepository() );
	dlg.setTitle( item->d_spin->getSystem() );
	if( dlg.exec() )
		d_pro->getSpins()->setCands( item->d_spin->getSystem(), dlg.d_cands );
}

void SpinSystemView::handleMoveSpin(Action & a)
{
	_SpinListItem* item = dynamic_cast<_SpinListItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	bool ok;
	QString res;
	res.sprintf( "%f", item->d_spin->getShift() );
	res = QInputDialog::getText( "Move Spin", 
		"Please	enter a new ppm value for spin:", QLineEdit::Normal, 
		res, &ok, this );
	if( !ok )
		return;
	PPM p = res.toFloat( &ok );
	if( !ok )
	{
		Root::ReportToUser::alert( d_parent, "Move Spin", "Invalid ppm value!" );
		return;
	}

	Root::Ref<MoveSpinCmd> cmd =
		new MoveSpinCmd( d_pro->getSpins(), item->d_spin, p, 0 );
		// Move generisches Spektrum
	cmd->handle( d_parent );
}

void SpinSystemView::handleMoveSpinAlias(Action & a)
{
	_SpinAliasItem* item = dynamic_cast<_SpinAliasItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );
	_SpinListItem* par = dynamic_cast<_SpinListItem*>( item->parent() );

	bool ok;
	QString res;
	res.sprintf( "%f", item->d_shift );
	res = QInputDialog::getText( "Move Spin Alias", 
		"Please	enter a new ppm value for spin alias:", QLineEdit::Normal, 
		res, &ok, this );
	if( !ok )
		return;
	PPM p = res.toFloat( &ok );
	if( !ok )
	{
		Root::ReportToUser::alert( d_parent, "Move Spin Alias", "Invalid ppm value!" );
		return;
	}

	Root::Ref<MoveSpinCmd> cmd =
		new MoveSpinCmd( d_pro->getSpins(), par->d_spin, p, item->d_spec );
		// Move generisches Spektrum
	cmd->handle( d_parent );
}

void SpinSystemView::handleRemoveAlias(Action & a)
{
	_SpinAliasItem* item = dynamic_cast<_SpinAliasItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );
	_SpinListItem* par = dynamic_cast<_SpinListItem*>( item->parent() );

	Root::Ref<MoveSpinCmd> cmd =
		new MoveSpinCmd( d_pro->getSpins(), par->d_spin, 
			par->d_spin->getShift(), // auf Home schieben lscht Alias
			item->d_spec );
	cmd->handle( d_parent );
}

void SpinSystemView::handleEditSpinAtts(Root::Action & a)
{
	_SpinListItem* item = dynamic_cast<_SpinListItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	DynValueEditor::edit( this, d_parent->getRep()->findObjectDef( Repository::keySpin ),
		item->d_spin );
}

void SpinSystemView::handleEditSysAtts(Root::Action & a)
{
	_SpinListItem* item = dynamic_cast<_SpinListItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item && item->d_spin->getSystem() );

	DynValueEditor::edit( this, 
		d_parent->getRep()->findObjectDef( Repository::keySpinSystem ),
		item->d_spin->getSystem() );
}

void SpinSystemView::handleShowTable(Root::Action & a)
{
	ACTION_ENABLED_IF( a, true );

	int i = 0;
	const SpinBase::SpinMap& sm = d_pro->getSpins()->getSpins();
	SpinBase::SpinMap::const_iterator p;
	ObjectListView::ObjectList o( sm.size() );
	if( d_sys == 0 )
	{
		for( p = sm.begin(); p != sm.end(); ++p, ++i )
			o[ i ] = (*p).second;
	}else
	{
		for( p = sm.begin(); p != sm.end(); ++p, ++i )
		{
			if( (*p).second->getSystem() && 
				(*p).second->getSystem()->getId() == d_sys )
				o[ i ] = (*p).second;
		}
	}
	ObjectListView::edit( this, d_pro->getRepository()
		->findObjectDef( Repository::keySpin ), o );
}

void SpinSystemView::handleSetLoc(Root::Action & a)
{
	_SpinListItem* item = dynamic_cast<_SpinListItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	Location loc;
	if( item->d_spin->getLoc() )
		loc = *item->d_spin->getLoc();

	if( Dlg::getLocation( this, loc ) )
		d_pro->getSpins()->setLoc( item->d_spin, loc );
	item->repaint();
}

void SpinSystemView::handleClearLoc(Root::Action & a)
{
	_SpinListItem* item = dynamic_cast<_SpinListItem*>( currentItem() );
	ACTION_ENABLED_IF( a, item );

	d_pro->getSpins()->setLoc( item->d_spin );
	item->repaint();
}



