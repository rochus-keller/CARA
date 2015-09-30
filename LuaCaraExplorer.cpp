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

#include "LuaCaraExplorer.h"
#include "AidaCentralAgent.h"
#include "AidaCentral.h"
#include <LuaQt3/LuaGui2.h>
#include <LuaQt3/LuaBox2.h>
#include <Script2/RefBinding.h>
#include <Script2/QtObject.h>
#include <Script/Engine.h>
#include <Spec/Spectrum.h>
#include <QMainWindow>
#include <QMenuBar>
using namespace Lua;


static int getMenuBar(lua_State * L)
{
    AidaCentralAgent* obj = QtObject<AidaCentralAgent>::check( L, 1);
	QtObject<LuaMenuBar2>::create( L, static_cast<LuaMenuBar2*>( obj->getParent()->getQt()->menuBar() ), false );
	return 1;
}

static int getNaviPopup(lua_State * L)
{
    AidaCentralAgent* obj = QtObject<AidaCentralAgent>::check( L, 1);
	QtObject<LuaPopupMenu2>::create( L, static_cast<LuaPopupMenu2*>( obj->getPopup() ), false );
	return 1;
}

static int getPopup(lua_State * L)
{
    AidaCentralAgent* obj = QtObject<AidaCentralAgent>::check( L, 1);
	QMenu* menu = 0;
    if( lua_isnil( L, 2 ) )
		menu = obj->getPopup();
	else
		menu = obj->getPopup( luaL_checkstring( L, 2 ) );
	QtObject<LuaPopupMenu2>::create( L, static_cast<LuaPopupMenu2*>( menu ), false );
	return 1;
}

static const luaL_reg _lib[] = 
{
	{ "getPopup", getPopup },
	{ "getNaviPopup", getNaviPopup },
	{ "getMenuBar", getMenuBar },
	{ 0, 0 },
};

void LuaCaraExplorer::install()
{
    QtObject<AidaCentralAgent,QSplitter,NotCreatable>::install( Engine::inst()->getCtx(), "Explorer", _lib );
}

void LuaCaraExplorer::installExplorer(AidaCentralAgent * a)
{
	assert(a);
	lua_State * L = Engine::inst()->getCtx();
	lua_getglobal( L, Lua::LuaGui2::s_gui );
	const int t = lua_gettop( L );
    QtObject<AidaCentralAgent>::create( L, a );
	lua_setfield( L, t, "explorer" );
	lua_pop( L, 1 );
}

template<class T>
static void _setCurrent( T* s )
{
	assert( s );
	lua_State * L = Engine::inst()->getCtx();
	lua_getglobal( L, Lua::LuaGui2::s_gui );
	RefBinding<T>::create( L, s );
	lua_setfield( L, -2, "selected" );
	lua_pop( L, 1 );
}

void LuaCaraExplorer::setCurrentSpec( Spec::Spectrum* s )
{
	_setCurrent( s );
}

void LuaCaraExplorer::setCurrentSpin( Spec::Spin* s )
{
	_setCurrent( s );
}
void LuaCaraExplorer::setCurrentSystem( Spec::SpinSystem* s )
{
	_setCurrent( s );
}
void LuaCaraExplorer::setCurrentSpectrumType( Spec::SpectrumType* s )
{
	_setCurrent( s );
}
void LuaCaraExplorer::setCurrentResidueType( Spec::ResidueType* s )
{
	_setCurrent( s );
}
void LuaCaraExplorer::setCurrentSystemType( Spec::SystemType* s )
{
	_setCurrent( s );
}
void LuaCaraExplorer::setCurrentProject( Spec::Project* s )
{
	_setCurrent( s );
}
void LuaCaraExplorer::setCurrentResidue( Spec::Residue* s )
{
	_setCurrent( s );
}
void LuaCaraExplorer::setCurrentSample( Spec::BioSample* s )
{
	_setCurrent( s );
}
void LuaCaraExplorer::setCurrentPeakList( Spec::PeakList* s )
{
	_setCurrent( s );
}
void LuaCaraExplorer::setCurrentSpinLink( Spec::SpinLink* s )
{
	_setCurrent( s );
}
