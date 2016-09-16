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

#include "AidaApplication.h"
#include <qfileinfo.h> 
#include <qinputdialog.h>
#include <qmessagebox.h>
#include <LuaSpec2/LuaSpec2.h>
#include <Qtl2/Objects.h>
#include <Qtl2/Variant.h>
#include <LuaQt3/LuaDlg2.h>
#include <LuaQt3/LuaGui2.h>
#include <LuaQt3/LuaXml2.h>
#include <Root/MessageLog.h>
#include <Script2/QtObject.h>
#include <Star/LuaStar.h>
#include <QDir>

#ifdef _WIN32
#include <windows.h>
#include <wincon.h>
#endif

using namespace Spec;
using namespace Lua;

#define PROMPT		"CARA> "
#define PROMPT2		"CARA>> "

const char* AidaApplication::s_release = "1.9.1.6";
const char* AidaApplication::s_relDate = "2016-09-15";
const char* AidaApplication::s_copyRightYear = "2016";
const char* AidaApplication::s_appName = "CARA";
const char* AidaApplication::s_help = 
"cara [options] [files]\n"
"Options:\n"
"-req, -r <script>.....execute the script which is part of the repository\n"
"-lua, -a <statement>..execute given Lua statement\n"
"-log, -g <file path>..write output to logfile (useful on Windows)\n"
"-batch, -b....start CARA without GUI, execute scripts and quit\n"
"-cons, -c.....like -batch, but remain in console mode (not supported on Windows)\n"
"-silent, -s.....GUI mode without CARA explorer, quit with os.exit()\n"
"-help, -h.....display this message\n"
"Files:\n"
"*.cara....the repository file to be opened\n"
"*.lua.....one or more script files to be executed\n"
"Order of execution: -req, *.lua, -lua\n"
"Files only in GUI mode:\n"
        #ifdef _HasNeasy_
"*.xed, *.xep....open this XEASY project file in NEASY window\n"
        #endif
"*.param, *.nmr..open one or more spectra in MonoScopes\n";

////////////////////////////////////////////////

static FILE* s_log = 0;

static void print_info( const char* msg )
{
    ::fprintf( stdout, "%s\n", msg );
	if( s_log )
		::fprintf( s_log, "%s\n", msg );
}

static void print_error( const char *msg) 
{
    ::fprintf( stderr, "%s\n", msg );
	if( s_log )
		::fprintf( s_log, "%s\n", msg );
}

#ifndef _DEBUG
static void myMessageOutput( QtMsgType type, const char *msg )
{
	return;
	// Wir wollen im Release-Terminal-Mode keine Debug-Messages mehr.
}
#endif

char* read_line( char *buffer, int n )
{
	return fgets( buffer, n, stdin );
}

extern bool g_useXeasyN;
const char* s_ext_delta = "#ext-delta#";

#ifdef _WIN32

static BOOL WINAPI MyConsoleHandler( DWORD dwCtrlType )
{
	// ::fprintf( stdout, "Event %d\n", dwCtrlType );
	return TRUE;
}

#endif

static int type(lua_State * L)
{
	luaL_checkany(L, 1);
	if( lua_type(L,1) == LUA_TUSERDATA )
	{
		ValueBindingBase::pushTypeName( L, 1 );
		if( !lua_isnil( L, -1 ) )
			return 1;
		lua_pop( L, 1 );
	}
	lua_pushstring(L, luaL_typename(L, 1) );
	return 1;
}

////////////////////////////////////////////////////


AidaApplication::AidaApplication():
	d_batch( false ), d_console( false ), d_silent( false )
{
#ifndef _DEBUG
	qInstallMsgHandler( myMessageOutput );
#endif

	try
	{
		Engine* l = new Engine();
		const QString appPath = QApplication::applicationDirPath();
#ifdef _WIN32
        const QString dllExt = "dll";
#else
        const QString dllExt = "so";
#endif
		l->setPluginPath( QString( "%1/plugins/?.lua" ).arg( appPath ).toLatin1() );
        l->setPluginPath( QString( "%1/plugins/?.%2" ).
                          arg( appPath ).arg( dllExt ).toLatin1(), true );
		l->addStdLibs();
		l->addLibrary( Lua::Engine::PACKAGE );
		l->addLibrary( Lua::Engine::OS );
		l->addLibrary( Lua::Engine::IO );
        l->addLibrary( Lua::Engine::LOAD );
        const char* loader = "local function loadScript( name )\n"
                             "   script = cara:getScript(name)\n"
                             "   if script == nil then \n"
                             "      return \"\\n\\tno script cara:getScript('\"..name..\"')\"\n"
                             "   else\n"
							 "      return loadstring( script, \":\"..name )\n"
                             "   end\n"
                             "end\n"
                             "table.insert( package.loaders, 2, loadScript )";
        if( !l->executeCmd( loader, "#loadScript" ) )
            throw Root::Exception( "syntax error in loader" );
	}catch( Root::Exception& e )
	{
		Root::MessageLog::inst()->error( "Scripting", e.what() );
	}

	QString arg;
	QString rep;
	QByteArray  log_name;
	bool show_help = false;
	QStringList args = arguments();
	for( int i = 1; i < args.size(); i++ ) // arg 0 enthlt Anwendungspfad
	{
		arg = args[ i ];
		if( arg[ 0 ] != '-' )
		{
			QFileInfo info( arg );
			if( info.extension( false ).upper() == "CARA" )
			{
				arg = info.absFilePath();
				QString str;
                str.sprintf( "Opening %s", arg.toLatin1().data() );
				print_error( str.toLatin1().data() );
				setCurrentDir( info.dirPath( true ) );
				rep = arg;
			}else if( info.extension( false ).upper() == "LUA" )
			{
				setCurrentDir( info.dirPath( true ) );
				d_scripts.push_back( arg.toLatin1() );
			}
		}else
		{
			arg = arg.mid( 1 ).upper();
			if( arg == "BATCH" || arg == "B" )
				d_batch = true;
			else if( arg == "CONS" || arg == "C" || arg == "CONSOLE" )
				d_console = true;
			else if( arg == "SILENT" || arg == "S" )
				d_silent = true;
			else if( arg == "HELP" || arg == "H" )
			{
				show_help = true;
				break;
			}
			else if( arg == "-L" || arg == "-LOAD" || arg == "-INI" || arg == "-INITIAL" ||
				arg == "-D" || arg == "-DEFAULT" || arg == "-T" || arg == "-TEXT" )
				; // Das sind die von XEASY absorbierten Optionen
			else if( arg == "LUA" || arg == "A" )
			{
				const char *chunk = args[++i];
				if( chunk == 0 )
					Root::MessageLog::inst()->error( "Command Line", "Invalid Lua Statement" );
				else
					d_commands.push_back( chunk );
			}else if( arg == "LOG" || arg == "G" )
			{
				const char *chunk = args[++i];
				if( chunk == 0 )
					Root::MessageLog::inst()->error( "Command Line", "Invalid logfile name" );
				else
					log_name = chunk;
			}else if( arg == "REQ" || arg == "R" )
			{
				const char *chunk = args[++i];
				if( chunk == 0 )
					Root::MessageLog::inst()->error( "Command Line", "Invalid script name" );
				else
					d_requires.push_back( chunk );
			}else if( arg == "PLUGINDIR" || arg == "PD" )
			{
				QDir path( args[++i] );
				if( path.isRelative() )
					path = QApplication::applicationDirPath() + "/" + path.path();
				Engine::inst()->setPluginPath( path.absolutePath().toLatin1() );
			}else
			{
				QString msg = "Invalid option -";
				msg += arg.lower();
				Root::MessageLog::inst()->error( "Command Line", msg.toLatin1() );
				show_help = true;
				break;
			}
		}
	}

	if( show_help )
	{
#ifndef _WIN32
		print_error( s_help );
#else
		QMessageBox::information( 0, QString( "%1 %2" ).arg( s_appName ).arg( s_release ), s_help );
#endif
		d_batch = true;
		d_silent = false;
		d_requires.clear();
		d_commands.clear();
		d_scripts.clear();
	}

	if( !hasGui() && d_silent )
	{
		print_error( "Silent mode not supported in batch or terminal mode!" );
		d_silent = false; 
	}

#ifdef _WIN32_old
	{
		QString str;
		str.sprintf( "%s Console %s", s_appName, s_release );
		if( hasGui() && !d_silent )
		{
			SetConsoleCtrlHandler( MyConsoleHandler, TRUE );
			// SetConsoleCtrlHandler( NULL, TRUE ); // friss CTRL-C
			print_error( "GUI mode, console locked, use CARA Explorer to quit" );
		}else if( hasGui() )
		{
			print_error( "Silent GUI mode, console locked, use CTRL-C to quit" );
			str += " (use CTRL-C to quit)";
		}else
			str += " (use CTRL-C to quit)";
		SetConsoleTitle( str.toLatin1().data() );
	}
#endif

	if( !rep.isEmpty() )
	{
		try
		{
            QString msg = "Loading ";
            msg += rep;

            Root::MessageLog::inst()->info( "Repository", msg.toLatin1() );
			d_rep = new Repository();
			d_rep->load( rep );
			reloadSpecs();
		}catch( Root::Exception& e )
		{
			Root::MessageLog::inst()->error( "Repository", e.what() );
            d_rep = 0; // 1.9.0 Beta 9: vorher wurde via Command Line unvollstndiges Rep geladen
		}
	}
	if( d_rep.isNull() )
	{
		d_rep = new Repository();
		d_rep->reloadEmpty();
		reloadSpecs();
	}
	d_rep->addObserver( this );

	if( isSilent() || !hasGui() ) 
		Root::MessageLog::inst()->addObserver( this );

	if( hasGui() )
	{
        Lua::QtObjectBase::install( Engine::inst()->getCtx() );
        Qtl::Variant::install( Engine::inst()->getCtx() );
		Qtl::Objects::install( Engine::inst()->getCtx() );
		Lua::LuaDlg2::install( Engine::inst()->getCtx() );
        Lua::LuaGui2::install( Engine::inst() );
    }
	Lua::LuaDlg2::install2( Engine::inst()->getCtx() ); 
	LuaSpec2::install( Engine::inst()->getCtx() );
	LuaSpec2::installRepository( Engine::inst()->getCtx(), d_rep );
	LuaDomDocument2::install( Engine::inst()->getCtx() );
	Star::LuaStar::install( Engine::inst()->getCtx() );
	lua_pushcfunction( Engine::inst()->getCtx(), type );
	lua_setfield( Engine::inst()->getCtx(), LUA_GLOBALSINDEX, "type" );

	if( true ) // isSilent() || !hasGui() ) 
		Engine::inst()->addObserver( this );
	if( !log_name.isEmpty() )
	{
		if( s_log )
		{
			::fclose( s_log );
			s_log = 0;
		}
		s_log = ::fopen( log_name.data(), "w" );
		if( s_log == 0 )
			Root::MessageLog::inst()->error( "Command Line", "Cannot open logfile for writing" );
	}
}

void AidaApplication::reloadSpecs()
{
	Root::Any v( true );
	if( d_rep->getFieldInfo( s_ext_delta ) != Root::Object::UnknownField )
		d_rep->getDynValue( s_ext_delta, v );
	const bool old = g_useXeasyN;
	g_useXeasyN = v.getBoolean();
	if( g_useXeasyN != old )
		d_rep->reloadSpecs();
}

AidaApplication::~AidaApplication()
{
	d_rep->removeObserver( this );
	d_rep = 0;
	LuaSpec2::killRepository(Engine::inst()->getCtx());
	Root::MessageLog::inst()->removeObserver( this );
	Engine::inst()->removeObserver( this );
	if( s_log )
	{
		::fclose( s_log );
		s_log = 0;
	}
}

void AidaApplication::handle(Root::Message& m )
{
	BEGIN_HANDLER();
	MESSAGE( Repository::Changed, e, m )
	{
        Q_UNUSED(e)
		traverseChildren( m, true );
		m.consume();
	}MESSAGE( Root::MessageLog::Update, e, m )
	{
		if( e->getType() == Root::MessageLog::Update::Add )
		{
            const Root::MessageLog::Entry& t = Root::MessageLog::inst()->getEntry( e->getNr() );
            QString str = QString( "%1 from %2: %3" ).arg( Root::MessageLog::s_pretty[ t.d_kind ] ).
                    arg( t.d_src.data() ).arg( t.d_msg.data() );
			print_error( str.toLatin1().data() );
        }
		m.consume();
	}MESSAGE( Engine::Update, a, m )
	{
		if( isSilent() || !hasGui() )
		{
			switch( a->getType() )
			{
			case Engine::Print:
                print_info( a->d_val1 );
				break;
			case Engine::Error:
                print_error( a->d_val1 );
				break;
            default:
                break;
			}
			m.consume();
		}
	}HANDLE_ELSE()
		AppAgent::handle( m );
	END_HANDLER();
}

void AidaApplication::openRepository(const char *path)
{
	Root::Ref<Repository> r = new Repository();
	r->load( path );
	d_rep->removeObserver( this );
	sendClosing();
	d_rep = r;
	d_rep->addObserver( this );
	LuaSpec2::installRepository( Engine::inst()->getCtx(),d_rep );

	sendReady();
}

void AidaApplication::newRepository()
{
	d_rep->removeObserver( this );
	sendClosing();
	d_rep = new Repository();
	d_rep->reloadEmpty();
	d_rep->addObserver( this );
	LuaSpec2::installRepository( Engine::inst()->getCtx(),d_rep );
	sendReady();
}

void AidaApplication::newFromTemplate(const char *path)
{
	Root::Ref<Repository> temp = new Repository();
	temp->load( path, true );
	Root::Ref<Repository> r = new Repository();
	r->copyFrom( *temp );
	d_rep->removeObserver( this );
	sendClosing();
	d_rep = r;
	d_rep->addObserver( this );
	LuaSpec2::installRepository( Engine::inst()->getCtx(),d_rep );
	sendReady();
}

void AidaApplication::sendReady()
{
	Repository::Changed m( d_rep, Repository::Ready );
	traverseChildren( m, true );
}

void AidaApplication::sendClosing()
{
	Repository::Changed m( d_rep, Repository::Closing );
	traverseChildren( m, true );
	LuaSpec2::killRepository(Engine::inst()->getCtx());
	Engine* lua = Engine::inst();
	if( lua )
		lua->collect();
	d_rep = 0;
}

bool AidaApplication::runScripts()
{
	for( int k = 0; k < d_requires.size(); k++ )
	{
		Repository::ScriptMap::const_iterator p = 
			d_rep->getScripts().find( d_requires[k].data() );
		if( p == d_rep->getScripts().end() )
		{
			QString msg = "Cannot find script ";
			msg += d_requires[k].data();
			Root::MessageLog::inst()->error( "Command Line", msg.toLatin1() );
			return false;
		}
		Script* s = (*p).second;

        if( !Engine::inst()->executeCmd( s->getCode(), ":" + s->getName() ) )
			return false;
	}
	QString str;
	for( int i = 0; i < d_scripts.size(); i++ )
	{
		str.sprintf( "Running script %s", d_scripts[i].data() );
        print_error( str.toLatin1() );
        if( !Engine::inst()->executeFile( d_scripts[i] ) )
        {
            print_error( Engine::inst()->getLastError() );
            return false;
        }
	}
	for( int j = 0; j < d_commands.size(); j++ )
	{
		if( !Engine::inst()->executeCmd( d_commands[j].data(), "#Command Line" ) )
			return false;
	}

	d_scripts.clear();
	d_commands.clear();
	d_requires.clear();
	return true;
}

void AidaApplication::saveRepository(const char* path)
{
	d_rep->setFieldValue( s_ext_delta, g_useXeasyN );
	save( d_rep, path );
}

void AidaApplication::save(Repository* rep, const char* path)
{
	assert( rep );
	QString str;
	str.sprintf( "%s %s", s_appName, s_release );
	rep->saveToFile( path, str );
}

//////////////////////////////

#include <signal.h>

GCC_IGNORE(-Wformat-security);

static int readline( lua_State *l, const char *prompt ) 
{
#ifdef _DEBUG
	bool ok;
	QString str = QInputDialog::getText( "CARA Console", prompt, QLineEdit::Normal, "", &ok );
	if( !ok )
		return 0;
	lua_pushstring( l, str.latin1() );
	return 1;
#else
	// RISK: nur hier lesen und schreiben
	static char buffer[512];
	if( prompt ) 
		::fprintf( stdout, prompt );
	if( read_line( buffer, sizeof(buffer) ) == NULL )
		return 0;  /* read fails */
	else 
	{
		lua_pushstring( l, buffer );
		return 1;
	}
#endif
}

static const char *get_prompt(bool firstline) 
{
	lua_State* L = Engine::inst()->getCtx();
	const char *p = NULL;
	lua_pushstring( L, firstline ? "_PROMPT" : "_PROMPT2" );
	lua_rawget( L, LUA_GLOBALSINDEX );
	p = lua_tostring( L, -1 );
	if( p == NULL ) 
		p = ( firstline ? PROMPT : PROMPT2 );
	lua_pop(L, 1);  /* remove global */
	return p;
}

static int incomplete (int status) 
{
	lua_State* L = Engine::inst()->getCtx();
	if( status == LUA_ERRSYNTAX && strstr(lua_tostring(L, -1), "near '<eof>'") != NULL) 
	{
		lua_pop(L, 1);
		return 1;
	}
	else
		return 0;
}

static int load_string (void) 
{
	int status;
	lua_State* L = Engine::inst()->getCtx();
	lua_settop(L, 0);
	if( readline(L, get_prompt( true ) ) == 0 )  /* no input? */
		return -1;
	if( lua_tostring(L, -1)[0] == '=' ) 
	{  /* line starts with `=' ? */
		lua_pushfstring( L, "return %s", lua_tostring(L, -1) + 1 );/* `=' -> `return' */
		lua_remove( L, -2 );  /* remove original line */
	}
	for (;;) 
	{  /* repeat until gets a complete line */
		status = luaL_loadbuffer( L, lua_tostring(L, 1), lua_strlen(L, 1), "=stdin" );
		if( !incomplete(status) ) 
			break;  /* cannot try to add lines? */
		if( readline( L, get_prompt( false ) ) == 0)  /* no more input? */
			return -1;
		lua_concat( L, lua_gettop(L) );  /* join lines */
	}
	// lua_saveline( L, lua_tostring(L, 1) );
	lua_remove(L, 1);  /* remove line */
	return status;
}

static void manual_input (void) 
{
	int status;
	lua_State* L = Engine::inst()->getCtx();
	while( ( status = load_string() ) != -1 ) 
	{
		if( status == 0 ) 
            status = lua_pcall(L, 0, LUA_MULTRET, 0); // hier ok, da Terminal ohne Gui
		if( status != 0 ) 
		{  
			print_error( lua_tostring( L, -1) );
			lua_pop( L, 1 );  /* remove error message */
		}
	}
	lua_settop(L, 0);  /* clear stack */
	// print_info( "" );
}

int AidaApplication::runTerminal()
{
	QString str;

	str.sprintf( "%s Console %s", s_appName, s_release );
	print_error( str.toLatin1().data() );
	str.sprintf( "Copyright (c) 2000-%s by Rochus Keller and others", s_copyRightYear );
	print_error( str.toLatin1().data() );
	str.sprintf( "%s %s", LUA_VERSION, LUA_COPYRIGHT );
	print_error( str.toLatin1().data() );

	// Zuerst einmal Message-Log ausgeben.
	for( int i = 0; i < Root::MessageLog::inst()->getCount(); i++ )
	{
		const Root::MessageLog::Entry& t = 
		Root::MessageLog::inst()->getEntry( i );
		str.sprintf( "%s from %s: %s", Root::MessageLog::s_pretty[ t.d_kind ], 
			t.d_src.data(), t.d_msg.data() );
		print_error( str.toLatin1().data() );
	}
	Root::MessageLog::inst()->clearLog();

	if( !runScripts() )
		return -1;

	// Single-Shot oder Interaktiv
	if( d_console )
	{
#ifndef _WIN32
		manual_input();
#else
		str.sprintf( "%s Console %s", s_appName, s_release );
		QMessageBox::information( 0, str, 
			tr("CARA doesn't support console mode on Windows anymore; use silent or batch mode instead.") );
#endif
	}
	return 0;
}

int AidaApplication::run()
{
	if( !runScripts() && isSilent() )
		return -1;
	return AppAgent::run();
}

void AidaApplication::flushRepository()
{
	d_rep->removeObserver( this );
	sendClosing();
	d_rep = new Repository();
}
