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

#include "ProposeSpinDlg.h"
#include <Q3VBoxLayout>
#include <Q3HBoxLayout>
#include <Gui/ListView.h>
#include <QPushButton>
#include <QComboBox>
#include <QMessageBox>
#include <Spec/Repository.h>
#include <Spec/Project.h>
#include <Spec/Spectrum.h>
#include <Spec/SpinSystem.h>
#include "Dlg.h"
#include <Script/Engine.h>
#include <Script/Util.h>
#include <Script2/QtObject.h>
#include <Script2/RefBinding.h>
#include <LuaSpec2/LuaSpectrum.h>
using namespace Spec;
using namespace Lua;

static QByteArray  s_script = "";
static bool s_inited = false;


struct LuaProposeSpinDlg 
{
	static void install( lua_State *L );

	static int getShift(lua_State *L)
	{
		ProposeSpinDlg* obj = QtObject<ProposeSpinDlg>::check( L, 1);
		Util::push( L, obj->getShift() );
		return 1;
	}
	static int getAtomType(lua_State *L)
	{
		ProposeSpinDlg* obj = QtObject<ProposeSpinDlg>::check( L, 1);
		Util::push( L, obj->getAtom().getAtomLabel() );
		return 1;
	}
	static int getIsotope(lua_State *L)
	{
		ProposeSpinDlg* obj = QtObject<ProposeSpinDlg>::check( L, 1);
		Util::push( L, obj->getAtom().getIsoLabel() );
		return 1;
	}
	static int getSpectrum( lua_State *L )
	{
		ProposeSpinDlg* obj = QtObject<ProposeSpinDlg>::check( L, 1);
		if( obj->getSpec() )
			LuaSpectrum::push( L, obj->getSpec() );
		else
			Util::push( L );
		return 1;
	}
	static int getProject( lua_State *L )
	{
		ProposeSpinDlg* obj = QtObject<ProposeSpinDlg>::check( L, 1);
		RefBinding<Project>::create( L, obj->getPro() );
		return 1;
	}
	static int addSpin( lua_State *L )
	{
		ProposeSpinDlg* obj = QtObject<ProposeSpinDlg>::check( L, 1);
		Spin* s = RefBinding<Spin>::check( L, 2 );
		new Dlg::SpinItem( obj->getList(), s, luaL_checknumber( L, 3 ) );
		return 0;
	}
	static int findMatchingSpins( lua_State *L )
	{
		ProposeSpinDlg* obj = QtObject<ProposeSpinDlg>::check( L, 1);
		SpinMatcher::Result spins = obj->getPro()->getMatcher()->findMatchingSpins( 
			obj->getPro()->getSpins(), obj->getAtom(), obj->getShift(), obj->getSpec() );
		if( spins.empty() )
		{
			Util::push( L );
			return 1;
		}
		lua_createtable( L, spins.size(), 0 );
		const int res = lua_gettop( L );
		SpinMatcher::Result::const_iterator p;
		int i = 1;
		for( p = spins.begin(); p != spins.end(); ++p, i++ )
		{
			lua_newtable( L );
			const int t = lua_gettop( L );
			Util::push( L, (*p).second );
			lua_setfield( L, t, "fit" );
			RefBinding<Spin>::create( L, (*p).first );
			lua_setfield( L, t, "spin" );
			lua_rawseti( L, res, i );
		}
		return 1;
	}
	static int getAnchor( lua_State *L )
	{
		ProposeSpinDlg* obj = QtObject<ProposeSpinDlg>::check( L, 1);
		Dimension d = luaL_checknumber( L, 2 ) - 1;
		if( d < 0 || d >= SpinPoint::maxSize() )
			Util::error( L, "dimension out of range" );
		if( obj->getAnchor(d) )
			RefBinding<Spin>::create( L, obj->getAnchor(d) );
		else
			Util::push(L);
		return 1;
	}
	static int enableOkHook( lua_State *L )
	{
		ProposeSpinDlg* obj = QtObject<ProposeSpinDlg>::check( L, 1);
		obj->enableOkHook();
		return 0;
	}
	static int getSpin( lua_State *L )
	{
		ProposeSpinDlg* obj = QtObject<ProposeSpinDlg>::check( L, 1);
		if( obj->getSpin() )
			RefBinding<Spin>::create( L, obj->getSpin() );
		else
			Util::push(L);
		return 1;
	}
};

static const luaL_reg _methods[] =
{
	{"getSpin",  LuaProposeSpinDlg::getSpin },
	{"enableOkHook",  LuaProposeSpinDlg::enableOkHook },
	{"getAnchor",  LuaProposeSpinDlg::getAnchor },
	{"getShift",  LuaProposeSpinDlg::getShift },
	{"getAtomType",  LuaProposeSpinDlg::getAtomType },
	{"getIsotope",  LuaProposeSpinDlg::getIsotope },
	{"getSpectrum",  LuaProposeSpinDlg::getSpectrum },
	{"getProject",  LuaProposeSpinDlg::getProject },
	{"addSpin",  LuaProposeSpinDlg::addSpin },
	{"findMatchingSpins",  LuaProposeSpinDlg::findMatchingSpins },
	{0,0}
};

void LuaProposeSpinDlg::install( lua_State *L )
{
	if( !s_inited )
        QtObject<ProposeSpinDlg,QDialog,NotCreatable>::install( L, "ProposeSpinDlg", _methods );
	s_inited = true;
}

ProposeSpinDlg::ProposeSpinDlg( QWidget* p, Spec::Project* pro, Spec::AtomType t,
							   Spec::PPM shift, Spec::Spectrum* spec, const QString& title ):
	d_pro( pro ), d_spec( spec ), d_atom( t ), d_shift( shift ),d_okHook(false)
{
	assert( pro );

	LuaProposeSpinDlg::install( Lua::Engine::inst()->getCtx() );

	if( title.isEmpty() )
		setCaption( "Select Spin" );
	else
		setCaption( title );
	setModal( true );

	Q3VBoxLayout* top = new Q3VBoxLayout( this, 10);
	Q3HBoxLayout* buttons = new Q3HBoxLayout();

	d_lv = new Gui::ListView( this );
	d_lv->setAllColumnsShowFocus( true );
	d_lv->setShowSortIndicator( true );
	d_lv->addColumn( "Fit" );
	d_lv->addColumn( "Shift" );
	d_lv->addColumn( "Spin" );
	d_lv->addColumn( "Sys." );

	d_cbo = new QComboBox( this );
	d_cbo->addItem( "" );
	std::set<QByteArray > sort;
	Repository::ScriptMap::const_iterator p1;
	for( p1 = d_pro->getRepository()->getScripts().begin(); p1 != 
		d_pro->getRepository()->getScripts().end(); ++p1 )
		sort.insert( (*p1).first.data() );
	std::set<QByteArray >::const_iterator p2;
	for( p2 = sort.begin(); p2 != sort.end(); ++p2 )
		d_cbo->addItem( (*p2).data() );
	top->addWidget( d_cbo );

	connect( d_cbo, SIGNAL( currentIndexChanged(int) ), this, SLOT( currentIndexChanged(int) ) );

	top->addWidget( d_lv );
	QObject::connect( d_lv, SIGNAL( doubleClicked ( Gui::ListViewItem * ) ), this, SLOT( accept() ) );

	QPushButton* ok = new QPushButton( "&OK", this );
	QObject::connect( ok, SIGNAL( clicked() ), this, SLOT( accept() ) );
	buttons->addWidget( ok );
	ok->setDefault( true );

	QPushButton* cancel = new QPushButton( "&Cancel", this );
	QObject::connect( cancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
	buttons->addWidget( cancel );

	top->addLayout( buttons );
}

bool ProposeSpinDlg::exec()
{
	d_okHook = false;
	const int cur = d_cbo->findText( s_script.data() );
	if( cur <= 0 )
	{
		d_cbo->setCurrentIndex( 0 );
		currentIndexChanged( 0 );
	}else
		d_cbo->setCurrentIndex( cur );
	if( QDialog::exec() != QDialog::Accepted )
		return false;

	if( d_okHook && d_lv->selectedItem() != 0 )
	{
		runSpinFilter( true );
	}

	return d_lv->selectedItem() != 0; 
}

void ProposeSpinDlg::runSpinFilter( bool okHook )
{
	Repository::ScriptMap::const_iterator p1 = 
		d_pro->getRepository()->getScripts().find( s_script.data() );
	if( p1 != d_pro->getRepository()->getScripts().end() )
	{
		Lua::Engine* l = Lua::Engine::inst();
		if( !l->pushFunction( (*p1).second->getCode() ) )
		{
			QMessageBox::critical( this, "Executing Spin Filter", l->getLastError() );
			return;
		}
		QtObject<ProposeSpinDlg>::create( l->getCtx(), this );
		Util::push( l->getCtx(), okHook );
        if( !l->runFunction( 2, 0 ) && !l->isSilent() )
		{
			QMessageBox::critical( this, "Executing Spin Filter", l->getLastError() );
			return;
		}
	}
}

Spin* ProposeSpinDlg::getSpin() const
{
	Dlg::SpinItem* item = (Dlg::SpinItem*) d_lv->selectedItem();
	if( item == 0 )
		return 0;
	else
		return item->getSpin();
}

void ProposeSpinDlg::currentIndexChanged( int i )
{
	d_lv->clear();
	if( i < 0 )
		return;
	else if( i == 0 )
	{
		SpinMatcher::Result spins = d_pro->getMatcher()->findMatchingSpins( d_pro->getSpins(),
			d_atom, d_shift, d_spec );
		SpinMatcher::Result::const_iterator p;
		for( p = spins.begin(); p != spins.end(); ++p )
			new Dlg::SpinItem( d_lv, (*p).first, (*p).second );
		s_script = "";
    }else
	{
		s_script = d_cbo->itemText( i ).toLatin1().data();
		runSpinFilter( false );
    }
	d_lv->setSorting( 0, false );
    d_lv->resizeAllColumnsToContents();
}

void ProposeSpinDlg::setAnchor( Dimension d, Spin* s )
{
	d_spins[d] = s;
}
