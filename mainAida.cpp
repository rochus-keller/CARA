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

#include <qfileinfo.h>
#include <qapplication.h>
#include <qmessagebox.h>
#include <QtDebug>
#include <QPicture>
#include <AidaCentral.h>
#include <Root/UndoManager.h>
#include <Root/MessageLog.h>
#ifdef _HasNeasy_
#include <Neasy/Neasy.h>
#endif
#include <Root/UniqueList.h>
#include <Script/Engine.h>
#include "ReportViewer.h"

#ifndef _DEBUG
static void emergencyWrite( Spec::Repository* rep )
{
	if( rep == 0 )
		return;
	try
	{
		QString name = rep->getFilePath();
		if( name.isEmpty() )
			name = ".cara.dump";
		else
			name += ".dump";
		QString str;
		str.sprintf( "%s %s Dumped", AidaApplication::s_appName, AidaApplication::s_release );
		rep->saveToFile( name, str );
		::fprintf( stderr, "Repository has been dumped to %s", name.toLatin1().data() );
	}catch( ... )
	{
	}
}
#endif

int main( int argc, char **argv )
{
//    Root::Int32 a = 2020202020;
//    Root::Int32 b = 20;
    // NOTE: wenn kein Cast auf Int64 kommt falsches Resultat! Also kein automatischer Cast!
//    Root::Int64 c = Root::Int64(a) * Root::Int64(b) + 3;
//    const Root::Int64 count = 1073741824;
//    const Root::UInt32 s_mapThreshold = 200000000;
//    if( count == 0 || count >= Root::Int64(s_mapThreshold) )
//        qDebug() << "gugus";
//    else
//        qDebug() << count << s_mapThreshold;

    Root::Application qapp( argc, argv );

    QIcon icon;
	icon.addFile( ":/cara/images/logo_16.png" );
	icon.addFile( ":/cara/images/logo_24.png" );
	icon.addFile( ":/cara/images/logo_32.png" );
	icon.addFile( ":/cara/images/logo_48.png" );
	icon.addFile( ":/cara/images/logo_128.png" );
	qapp.setWindowIcon( icon );
	qapp.setOrganizationDomain( "cara.nmr.ch" );
	qapp.setOrganizationName( "cara.nmr.ch" );
	qapp.setApplicationName( AidaApplication::s_appName );

	AidaApplication* app = 0;
#ifndef _DEBUG
	try
	{
#endif
		app = new AidaApplication();

		if( app->hasGui() && app->isSilent() )
		{
			Root::MessageLog::inst()->setRemember( true );

			int res = app->run();
			delete app;
			Spec::ReportViewer::kill();
			Root::UndoManager::kill();
			Lua::Engine::kill();
			Root::MessageLog::kill();
#ifdef _DEBUG
            //Lua::ObjectPeer::printLeftOvers();
			Root::Resource::printLeftOvers();
			Root::Resource::printLeftOvers(false, false);
#endif
			return res;
		}else if( app->hasGui() && !app->isSilent() )
		{
			// Ownership: AppAgent -> UndoManager -> Central

			AidaCentral* w = new AidaCentral( new Root::UndoManager( app, 15 ), app );
				// RISK: Nr. Undo-Steps

			QString arg;
			QStringList args = app->arguments();
			for( int i = 1; i < args.size(); i++ ) // arg 0 enthält Anwendungspfad
			{
				arg = args[ i ];
				if( arg[ 0 ] != '-' )
				{
					// Reagiere nun auf übrige File-Typen
					QFileInfo info( arg );

					if( info.extension( false ).upper() == "XEP" || 
						info.extension( false ).upper() == "XED" )
					{
#ifdef _HasNeasy_

						Root::AppAgent::setCurrentDir( info.dirPath( true ) );
						QMainWindow* w = Neasy::showMainWindow( false );
						if( w )
							w->raise();
#endif
					}else if( info.extension( false ).upper() == "PARAM" ||
						info.extension( false ).upper() == "NMR" )
					{
						Root::AppAgent::setCurrentDir( info.dirPath( true ) );
						w->openSpectrum(arg);
					}			
				}
			}
			int res = app->run();
			delete app;
			Spec::ReportViewer::kill();
			Root::UndoManager::kill();
			Lua::Engine::kill();
			Root::MessageLog::kill();
#ifdef _DEBUG
            //Lua::ObjectPeer::printLeftOvers();
			Root::Resource::printLeftOvers();
			Root::Resource::printLeftOvers(false, false);
#endif
			return res;
		}else
		{
			Root::MessageLog::inst()->setRemember( false );
			int res = app->runTerminal();
			delete app;
			Spec::ReportViewer::kill();
			Lua::Engine::kill();
			Root::UndoManager::kill();
			Root::MessageLog::kill();
#ifdef _DEBUG
            //Lua::ObjectPeer::printLeftOvers();
			Root::Resource::printLeftOvers();
			Root::Resource::printLeftOvers(false, false);
#endif
			return res;
		}
#ifndef _DEBUG
	}catch( Root::Exception& e )
	{
		if( app )
			emergencyWrite( app->getRep() );
		if( app && app->hasGui() )
			QMessageBox::critical( 0, "Severe Uncaught Exception", 
				e.what(), "&Cancel" );
		else
			::fprintf( stderr, "Severe Exception: %s", e.what() );
		if( app )
			delete app;
		return -1;
	}catch( ... )
	{
		if( app )
			emergencyWrite( app->getRep() );
		if( app && app->hasGui() )
			QMessageBox::critical( 0, "Severe Unknown Exception", 
				"Sorry, cannot continue execution of CARA!", "&Cancel" );
		else
			::fprintf( stderr, "Severe Exception: unknown reason, sorry!" );
		if( app )
			delete app;
		return -1;
	}
#endif
}


// Ist nun direkt in qvalidator.cpp: double _HUGE = 1.7E308; 
