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

#if !defined(AFX_AIDAAPPLICATION_H__AB5EEF13_7BF5_44AC_9463_55B9AFA02B5E__INCLUDED_)
#define AFX_AIDAAPPLICATION_H__AB5EEF13_7BF5_44AC_9463_55B9AFA02B5E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Root/Application.h>
#include <Spec/Repository.h>

class AidaApplication : public Root::AppAgent
{
public:
	void flushRepository();
	static const char* s_relDate;
    static const char* s_copyRightYear;
	static const char* s_release;
	static const char* s_appName;
	static const char* s_help;

	Spec::Repository* getRep() const { return d_rep; }

	void saveRepository(const char* path);
	static void save(Spec::Repository*, const char* path);

	void newFromTemplate( const char* path );
	void newRepository();
	void openRepository( const char* path );

	bool isBatch() const { return d_batch; }
	bool isConsole() const { return d_console; }
	bool hasGui() const { return !d_batch && !d_console; }
	bool isSilent() const { return d_silent; }

	int run();
	bool runScripts();
	int runTerminal();
	void reloadSpecs();

	AidaApplication();
	virtual ~AidaApplication();
protected:
	void handle( Root::Message& );
private:
	void sendClosing();
	void sendReady();
	Root::Ref<Spec::Repository> d_rep;
	bool d_batch;
	bool d_console;
	bool d_silent;

	typedef Root::Deque<QByteArray > StringDeq;
	StringDeq d_scripts;
	StringDeq d_commands;
	StringDeq d_requires;
};

#endif // !defined(AFX_AIDAAPPLICATION_H__AB5EEF13_7BF5_44AC_9463_55B9AFA02B5E__INCLUDED_)
