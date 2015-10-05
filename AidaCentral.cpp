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

#include "AidaCentral.h"

#include <QMessageBox>
#include <QMenuBar>
#include <QFileDialog> 
#include <QApplication>
#include <QFileInfo> 
#include <QInputDialog> 
#include <QFontDialog>
#include <QStyleFactory>
#include <QDockWidget>

#include <Root/Mem.h>
#include <Gui/Menu.h> 
#include <Lexi/MainWindow.h>
#include <Root/Action.h>
#include <Spec/Factory.h>
#include <Spec/FileSpectrum.h>
#include <Spec/CaraSpectrum.h>
#include <Root/Application.h>
#include <Root/MessageLog.h>
#include <Spec/Factory.h>
#include <SpecView/SpinPointView.h>
#include <SpecView/PeakPlaneView.h>
#include <SpecView/DynValueEditor.h>
#include <SpecView/PeakColorDlg.h>
#include <SpecView3/SpinPointView3.h>
#include <SpecView3/PeakPlaneView3.h>
#include <Script/Engine.h>
//#include <Text/RichTextEditor.h>
#ifdef _HasNeasy_
#include <Neasy/Neasy.h>
#endif
#include <Script/Terminal2.h>
#include <Script/StackView.h>
#include <Script/LocalsView.h>
#include "ScriptEditor3.h"
using namespace Spec;
using namespace Root;
using namespace Lua;

#include <AidaCentralAgent.h>
#include <SpecView/RotateDlg.h>
#include <SpectrumListView.h>
#include <SliceScope.h>
#include <ReportViewer.h>

static const Root::UInt32 s_max = 2000000000;
static const char* CAPTION2 = "CARA - Computer Aided Resonance Assignment";
//static const char* s_unit = "NMR.078";
static bool s_gcEnabled = true;
static QString s_style = "Plastique";

bool g_useXeasyN = true;

namespace DoThis
{
	static Action::CmdStr Test = "Test";
	static Action::CmdStr OpenSpectrum = "OpenSpectrum";
	static Action::CmdStr OpenSpecRot = "OpenSpecRot";
	static Action::CmdStr NewFromTemplate = "NewFromTemplate";
	static Action::CmdStr ShowNeasy = "ShowNeasy";
	static Action::CmdStr ShowNeasyPure = "ShowNeasyPure";
	static Action::CmdStr OpenPhaser = "OpenPhaser";
	static Action::CmdStr Convert = "Convert";
	static Action::CmdStr Convert2 = "Convert2";
	static Action::CmdStr MapThreshold = "MapThreshold";
	static Action::CmdStr LabelFont = "LabelFont";
	static Action::CmdStr Flatten = "Flatten";
	static Action::CmdStr LabelOffset = "LabelOffset";
	static Action::CmdStr LabelAngle = "LabelAngle";
	static Action::CmdStr LabelCross = "LabelCross";
	static Action::CmdStr GarbageCollect = "GarbageCollect";
	static Action::CmdStr EnableGc = "EnableGc";
	static Action::CmdStr EditProperties = "EditProperties";
	static Action::CmdStr XeasyN = "XeasyN";
	static Action::CmdStr OpenSitar = "OpenSitar";
	static Action::CmdStr ShowStyle = "ShowStyle";
	static Action::CmdStr LuaBox = "LuaBox";
    static Action::CmdStr SetupPeakColors = "SetupPeakColors";
}		

ACTION_SLOTS_BEGIN( AidaCentral )
    { DoThis::LuaBox, &AidaCentral::handleLuaBox },
    { DoThis::ShowStyle, &AidaCentral::handleShowStyle },
    { DoThis::OpenSitar, &AidaCentral::handleOpenSitar },
    { DoThis::XeasyN, &AidaCentral::handleXeasyN },
    { DoThis::EditProperties, &AidaCentral::handleEditProperties },
    { DoThis::EnableGc, &AidaCentral::handleEnableGc },
    { DoThis::GarbageCollect, &AidaCentral::handleGarbageCollect },
    { DoThis::LabelOffset, &AidaCentral::handleLabelOffset },
    { DoThis::LabelAngle, &AidaCentral::handleLabelAngle },
    { DoThis::SetupPeakColors, &AidaCentral::handleSetupPeakColors },
    { DoThis::LabelCross, &AidaCentral::handleLabelCross },
    { DoThis::Flatten, &AidaCentral::handleFlatten },
    { Action::ExecuteLine, &AidaCentral::handleExecute },
    { DoThis::LabelFont, &AidaCentral::handleLabelFont },
    { DoThis::MapThreshold, &AidaCentral::handleMapThreshold },
    { DoThis::Convert2, &AidaCentral::handleConvert2 },
    { DoThis::Convert, &AidaCentral::handleConvert },
    { DoThis::OpenSpecRot, &AidaCentral::handleOpenSpecRot },
    { DoThis::OpenPhaser, &AidaCentral::handleOpenPhaser },
    { DoThis::ShowNeasyPure, &AidaCentral::handleShowNeasyPure },
    { DoThis::ShowNeasy, &AidaCentral::handleShowNeasy },
    { DoThis::NewFromTemplate, &AidaCentral::handleNewFromTemplate },
    { DoThis::OpenSpectrum, &AidaCentral::handleOpenSpectrum },
    { DoThis::Test, &AidaCentral::handleTest },
    { Action::FileQuit, &AidaCentral::handleFileQuit },
    { Action::FileNew, &AidaCentral::handleFileNew },
    { Action::FileOpen, &AidaCentral::handleFileOpen },
    { Action::FileSave, &AidaCentral::handleFileSave },
    { Action::FileSaveAs, &AidaCentral::handleFileSaveAs },
    { Action::HelpAbout, &AidaCentral::handleHelpAbout },
ACTION_SLOTS_END( AidaCentral )
//////////////////////////////////////////////////////////////////////

AidaCentral::AidaCentral(Root::Agent* parent, AidaApplication* app):
	MainWindow( parent, true, true, true ), d_app( app ),d_context(0)
{
	// CARA 1.9.2: use default style and bigger Font on Mac
#ifdef QT_MAC_USE_COCOA
	// Qt sets the default Style to Aqua
	s_style = "Macintosh (aqua)"; // no way to get the name from QStyle
	QFont f = QApplication::font();
	f.setPointSize(11);
	SpinPointView::s_defaultFont = f;
	SpinPointView3::s_defaultFont = f;
	PeakPlaneView::s_default = f;
	PeakPlaneView3::s_default = f;
#else
	qApp->setStyle( s_style );
#endif

    d_agent = new AidaCentralAgent( this );
	getQt()->setCentralWidget( d_agent );

	d_agent->initialize();
	getRep()->addObserver( this );
	buildMenus();
	updateTitle();

	getQt()->resize( 600, 400 );
	getQt()->showMaximized();

	Root::MessageLog::inst()->addObserver( this );

	d_term = new QDockWidget( "Lua Terminal", getQt() );
	d_term->setWidget( new Lua::Terminal2( d_term, Engine::inst() ) );
	d_term->setAllowedAreas( Qt::AllDockWidgetAreas );
	d_term->setFeatures( QDockWidget::AllDockWidgetFeatures );
	d_term->setFloating( false );
	d_term->setVisible( false );
    getQt()->addDockWidget( Qt::BottomDockWidgetArea, d_term );

    d_stack = new QDockWidget( "Lua Stack", getQt() );
	d_stack->setWidget( new StackView( Engine::inst(), d_stack ) );
    d_stack->setAllowedAreas( Qt::AllDockWidgetAreas );
    d_stack->setFeatures( QDockWidget::AllDockWidgetFeatures );
    d_stack->setFloating( false );
    d_stack->setVisible( false );
    getQt()->addDockWidget( Qt::BottomDockWidgetArea, d_stack );

    d_locals = new QDockWidget( "Lua Locals", getQt() );
	d_locals->setWidget( new LocalsView( Engine::inst(), d_locals ) );
    d_locals->setAllowedAreas( Qt::AllDockWidgetAreas );
    d_locals->setFeatures( QDockWidget::AllDockWidgetFeatures );
    d_locals->setFloating( false );
    d_locals->setVisible( false );
    getQt()->addDockWidget( Qt::BottomDockWidgetArea, d_locals );

	Engine::inst()->addObserver(this);
    // d_app->runScripts(); // wird in run oder runTerminal ausgefhrt

	/* TEST
	int i = 0;
	while( Root::ActionHandler<AidaCentral>::d_slots[i].d_command != 0 )
	{
		qDebug( "Command %d = %s", i, 
			(const char*)Root::ActionHandler<AidaCentral>::d_slots[i].d_command );
		i++;
	}
	*/
}

AidaCentral::~AidaCentral()
{
	Engine::inst()->removeObserver(this);
	Root::MessageLog::inst()->removeObserver( this );
	try
	{
		Resource::flushDeferreds();
	}catch( ... )
	{
		qDebug( "WARNING: AidaCentral::~AidaCentral: unknown exception" );
	}
}

void AidaCentral::buildMenus()
{
	Gui::Menu* menuFile = new Gui::Menu( menuBar() );
    Gui::Menu::item( menuFile, this, Root::Action::FileNew, Qt::CTRL+Qt::Key_N );
	Gui::Menu::item( menuFile, this, "New from template...", DoThis::NewFromTemplate, false );
    Gui::Menu::item( menuFile, this, Root::Action::FileOpen, Qt::CTRL+Qt::Key_O );
	menuFile->insertSeparator();
    Gui::Menu::item( menuFile, this, Root::Action::FileSave, Qt::CTRL+Qt::Key_S );
    Gui::Menu::item( menuFile, this, Root::Action::FileSaveAs );
	menuFile->insertSeparator();
    Gui::Menu::item( menuFile, Root::Action::FilePrint, Qt::CTRL+Qt::Key_P );
    Gui::Menu::item( menuFile, Root::Action::FileExportPdf );
 	menuFile->insertSeparator();
    Gui::Menu::item( menuFile, this, Root::Action::FileQuit, Qt::CTRL+Qt::Key_Q );
	menuBar()->insertItem( "&File", menuFile );

	Gui::Menu* menuEdit = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuEdit, Root::Action::EditUndo, Qt::CTRL+Qt::Key_Z );
	Gui::Menu::item( menuEdit, Root::Action::EditRedo, Qt::CTRL+Qt::Key_Y );
 	menuEdit->insertSeparator();
    Gui::Menu::item( menuEdit, Root::Action::EditCut, Qt::CTRL+Qt::Key_X );
    Gui::Menu::item( menuEdit, Root::Action::EditCopy, Qt::CTRL+Qt::Key_C );
    Gui::Menu::item( menuEdit, Root::Action::EditPaste, Qt::CTRL+Qt::Key_V );
    Gui::Menu::item( menuEdit, Root::Action::EditSelectAll, Qt::CTRL+Qt::Key_A );
//	menuEdit->insertSeparator();
//    Gui::Menu::item( menuEdit, Root::Action::EditFind, tr("CTRL+F") );
//    Gui::Menu::item( menuEdit, Root::Action::EditFindAgain, tr("F3") );
//    Gui::Menu::item( menuEdit, Root::Action::EditReplace, tr("CTRL+R") );
	menuEdit->insertSeparator();
	Gui::Menu::item( menuEdit, this, "Properties...", DoThis::EditProperties, false );
	menuBar()->insertItem( "&Edit", menuEdit );

	Gui::Menu* menuProject = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuProject, d_agent, "&New Project...", 
		AidaCentralAgent::NewProject, false );
	Gui::Menu::item( menuProject, d_agent, "&Remove Project...", 
		AidaCentralAgent::RemoveProject, false );
	Gui::Menu::item( menuProject, d_agent, "Rena&me Project...", 
		AidaCentralAgent::RenameProject, false );
	Gui::Menu::item( menuProject, d_agent, "&Duplicate Project...", 
		AidaCentralAgent::DuplicatePro, false );
	menuProject->insertSeparator();

	Gui::Menu* subImport = new Gui::Menu( menuProject );
	menuProject->insertItem( "Import", subImport );
	Gui::Menu::item( subImport, d_agent, "&Project from Sequence...", 
		AidaCentralAgent::ImportSeq, false );
	Gui::Menu::item( subImport, d_agent, "&Project from BMRB File...", 
		AidaCentralAgent::ImportProject, false );
	Gui::Menu::item( subImport, d_agent, "&Atom List...", 
		AidaCentralAgent::ImportAtomList, false );
	Gui::Menu::item( subImport, d_agent, "&Sequence Values...", 
		AidaCentralAgent::ImportSeqVals, false );

	Gui::Menu* subExport = new Gui::Menu( menuProject );
	menuProject->insertItem( "Export", subExport );
	Gui::Menu::item( subExport, d_agent, "&Mapper File...", 
		AidaCentralAgent::ExportMapper, false );
	Gui::Menu::item( subExport, d_agent, "&Sequence...", 
		AidaCentralAgent::ExportSequence, false );
	Gui::Menu::item( subExport, d_agent, "&Atom List...", 
		AidaCentralAgent::ExportAtomList, false );

	menuBar()->insertItem( "&Projects", menuProject );

	Gui::Menu* menuTools = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuTools, this, "Open Spectrum...", DoThis::OpenSpectrum, false, Qt::CTRL+Qt::Key_M );
    Gui::Menu::item( menuTools, this, "Open Spectrum (rotated)...", DoThis::OpenSpecRot, false );
	Gui::Menu::item( menuTools, this, "Open Sitar...", DoThis::OpenSitar, false );
#ifdef _HasNeasy_
	Gui::Menu::item( menuTools, this, "Open NEASY...", DoThis::ShowNeasyPure, false );
	Gui::Menu::item( menuTools, this, "Open NEASY with Spectrum...", DoThis::ShowNeasy, false );
#endif
	Gui::Menu::item( menuTools, this, "Phase Spectrum...", DoThis::OpenPhaser, false );
	Gui::Menu::item( menuTools, this, "Project Spectrum...", DoThis::Flatten, false );
	Gui::Menu::item( menuTools, this, "Convert to CARA Spectrum...", DoThis::Convert, false );
	Gui::Menu::item( menuTools, this, "Convert to EASY Spectrum...", DoThis::Convert2, false );
	Gui::Menu::item( menuTools, this, "Lua Terminal2...", DoThis::LuaBox, false, Qt::CTRL+Qt::Key_L );
	menuBar()->insertItem( "&Tools", menuTools );

	Gui::Menu* menuSetup = new Gui::Menu( menuBar() );
	Gui::Menu::item( menuSetup, this, "Set Mapping Threshold...", DoThis::MapThreshold, false );
	Gui::Menu::item( menuSetup, this, "Preset Label Font...", DoThis::LabelFont, false );
	Gui::Menu::item( menuSetup, this, "Preset Label Offset...", DoThis::LabelOffset, false );
	Gui::Menu::item( menuSetup, this, "Preset Label Angle...", DoThis::LabelAngle, false );
    Gui::Menu::item( menuSetup, this, "Setup Peak Colors...", DoThis::SetupPeakColors, false );
	Gui::Menu::item( menuSetup, this, "Preset Show Cross", DoThis::LabelCross, true );
	// 	1.9.1: macht keinen Sinn mehr
    // Gui::Menu::item( menuSetup, this, "Spectrum width+delta format", DoThis::XeasyN, true );
	menuSetup->insertSeparator();
	/*
	Gui::Menu::item( menuSetup, this, "Enable Collector", DoThis::EnableGc, true );
	Gui::Menu::item( menuSetup, this, "Collect Garbage", DoThis::GarbageCollect, false, Qt::CTRL+Qt::Key_G );
	*/
	Gui::Menu* pop2 = new Gui::Menu( menuSetup );
	menuSetup->insertItem( "Select Look && Feel", pop2 );
	QStringList keys = QStyleFactory::keys();
	for( int i = 0; i < keys.size(); i++ )
	{
		Gui::Menu::item( pop2, this, keys[i].toLatin1(), 
			DoThis::ShowStyle, true )->addParam( keys[i].toLatin1().data() );
	}
	menuBar()->insertItem( "&Setup", menuSetup );

	Gui::Menu* menuHelp = new Gui::Menu( menuBar() );
    Gui::Menu::item( menuHelp, this, Root::Action::HelpAbout );
	menuBar()->insertItem( "&?", menuHelp );
	d_help = menuHelp;
}

void AidaCentral::setContextMenu( QMenu* m )
{
	if( d_context )
		menuBar()->removeAction( d_context->menuAction() );
	if( m )
	{
		d_context = m;
		menuBar()->insertMenu( d_help->menuAction(), m );
	}
}

extern void createMonoScope( Root::Agent* a, Spec::Spectrum* s, 
					 Spec::Project* p, PeakList* l, const Rotation& r );

void AidaCentral::openSpectrum(const char *path)
{
	QFileInfo info( path );
	AppAgent::setCurrentDir( info.dirPath( true ) );

	try
	{
		QApplication::setOverrideCursor( Qt::waitCursor );
		SpecRef<Spectrum> spec = Factory::createSpectrum( path );
		if( spec == 0 )
			Root::ReportToUser::alert( this, "Error opening spectrum",
				"Unknown spectrum format" );
		else if( spec->getDimCount() == 1 )
			new SliceScope( this, spec, 0, 0 );
		else
			createMonoScope( this, spec, 0, 0, Rotation() );
		QApplication::restoreOverrideCursor();
	}catch( Root::Exception& e )
	{
		QApplication::restoreOverrideCursor();
		Root::ReportToUser::alert( this, "Error opening spectrum", e.what() );
	}catch( ... )
	{
		QApplication::restoreOverrideCursor();
		Root::ReportToUser::alert( this, "Error opening spectrum: Unknown exception" );
	}
}

void AidaCentral::closeRepository()
{
	// Diese Methode soll dafr sorgen, dass das aktuelle Rep in keinem Viewer
	// und der UndoQueue nicht mehr sichtbar ist. Da ein Rep auch wiederverwendet
	// werden kann, wird hier nicht dispose aufgerufen.
	Root::StateAction a( Root::StateAction::SystemKillUndo );
	// Undo lschen.
	traverseUp( a );
	Root::StateAction m( Action::WindowClose );
	traverseChildren( m, true );
	ReportViewer::kill();
}

void AidaCentral::handle(Root::Message& m )
{
	if( getQt() == 0 )
		return;

	BEGIN_HANDLER();
	MESSAGE( Repository::Changed, e, m )
	{
		switch( e->d_hint )
		{		
		case Repository::Closing:
			closeRepository();
			break;
		case Repository::Ready:
			d_agent->initialize();
			e->sender()->addObserver( this );
			updateTitle();
			break;
		case Repository::Saved:
			setStatusMessage( "Repository saved." );
			updateTitle();
			break;
		default:
			updateTitle();
			break;
		}
		m.consume();
	}
	MESSAGE( Project::Added, a, m )
    {
        Q_UNUSED(a)
        d_agent->reloadProjects();
    }
	MESSAGE( Project::Removed, a, m )
	{
        Q_UNUSED(a)
		d_agent->reloadProjects();
	}
    MESSAGE( Project::Changed, a, m )
    {
        Q_UNUSED(a)
        if( a->d_hint == Project::Name )
            d_agent->reloadProjects();
    }
	MESSAGE( Root::MessageLog::Update, e, m )
	{
		if( e->getType() == Root::MessageLog::Update::Add )
		{
			QString str;
			const Root::MessageLog::Entry& t = 
				Root::MessageLog::inst()->getEntry( e->getNr() );
			str.sprintf( "%s from %s", Root::MessageLog::s_pretty[ t.d_kind ],
				t.d_src.data() );
			setStatusMessage( str );
		}
		m.consume();
    }
    MESSAGE( Engine::Update, a, m )
    {
        switch( a->getType() )
        {
		case Engine::LineHit:
		case Engine::BreakHit:
            d_term->setVisible(true);
            d_stack->setVisible(true);
            d_locals->setVisible(true);
            break;
        default:
            break;
        }
    }
    HANDLE_ELSE()
		MainWindow::handle( m );
	END_HANDLER();
}

void AidaCentral::handleAction(Root::Action & a )
{
	if( !EXECUTE_ACTION( AidaCentral, a ) )
		MainWindow::handleAction( a );
}

void AidaCentral::handleOpenSpectrum( Action& a )
{
	ACTION_ENABLED_IF( a, true );

	QString fileName = QFileDialog::getOpenFileName( getQt(), "Select Spectrum File", 
        AppAgent::getCurrentDir(), Spectrum::s_fileFilter );
    if( fileName.isNull() ) 
		return;

	openSpectrum( fileName );
}

void AidaCentral::handleTest( Action& a )
{
	ACTION_ENABLED_IF( a, true );

	/*
	Spectrum* spec = Factory::createSpectrum( "D:\\TopTen\\Data\\PBP_HSQC.2.3D.PARAM" );
	if( spec == 0 )
		Root::ReportToUser::alert( this, "Error opening spectrum",
			"Unknown spectrum format" );
	else
		new MonoScope( this, spec );
	*/
	QFileInfo info( "test.cara" );
	AppAgent::setCurrentDir( info.dirPath( true ) );
	openRepository( info.absFilePath() );
}

bool AidaCentral::handleSave(bool as) const
{
	if( !as && ( d_app->getRep()->getFilePath() == 0 || *d_app->getRep()->getFilePath() == 0 ) )
		as = true;

	QString fileName;
	
	if( !as  )
	{
		fileName = d_app->getRep()->getFilePath();
		QFileInfo info( fileName );
		if( !info.isWritable() )
		{
			as = true;
			Root::ReportToUser::alert( const_cast<AidaCentral*>(this), "Error Saving File", 
				"Cannot write to current repository file. "
				"Continue with 'Save as...'" );
		}else
		{
			QDir dir( AppAgent::getCurrentDir() );
			dir.remove( fileName + ".bak" );
			if( !dir.rename( fileName, fileName + ".bak" ) )
			{
				as = true;
				Root::ReportToUser::alert( const_cast<AidaCentral*>(this), "Error Saving Backup File", 
					"Cannot create backup of currently open repository file. "
					"Continue with 'Save as...'" );
			}
		}
	}

	if( as )
	{
		fileName = QFileDialog::getSaveFileName( getQt(), "Save Repository", AppAgent::getCurrentDir(), 
            "Repository (*.cara)", 0, QFileDialog::DontConfirmOverwrite );
		if( fileName.isNull() ) 
			return false;

		QFileInfo info( fileName );
		if( info.extension( false ).upper() != "CARA" )
			fileName += ".cara";
		info.setFile( fileName );
		fileName = info.absFilePath();
		if( info.exists() )
		{
			if( QMessageBox::warning( getQt(), "Save As",
				"This file already exists. Do you want to overwrite it?",
				"&OK", "&Cancel" ) != 0 )
				return false;
		}
		AppAgent::setCurrentDir( info.dirPath( true ) );
	}

	try
	{
		d_app->saveRepository( fileName.toLatin1().data() );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( const_cast<AidaCentral*>(this), "Error Saving Repository", e.what() );
		return false;
	}
	return true;
}

bool AidaCentral::askToCloseWindow() const
{
	if( Engine::inst()->isExecuting() )
    {
        QMessageBox::information( getQt(), "About to quit", "Cannot quit CARA while Lua debugger is running!" );
        return false;
    }
	Root::AboutToQuit m;
	AidaCentral* a = const_cast<AidaCentral*>( this );
	a->traverseChildren( m );
	if( m.getCount() != 0 )
		return false;
	if( !askToCloseProject() )
		return false;
    qApp->quit();
	return true;
}

bool AidaCentral::askToCloseProject() const
{
	if( !d_app->getRep()->isDirty() )
		return true;
	switch( QMessageBox::warning( getQt(), "About to close repository",
										  "Do you want to save the repository before closing?",
										  "&Save", "&Don't Save", "&Cancel",
										  0,		// Enter == button 0
										  2 ) )		// Escape == button 2
	{
	case 0:
		return handleSave( false ); // Do it with action.
	case 1:
		d_app->flushRepository(); // Damit nicht zweimal fragt
		return true;	// Do it without save
	default:
		return false;	// Don't do it.
	}
}

void AidaCentral::updateTitle()
{
	QString str1, str2( d_app->getRep()->getFilePath() );
	if( str2.isEmpty() )
	{
		str2 = "<Untitled>";
		str1.sprintf( "%s - %s", CAPTION2, str2.toAscii().data() );
	}else
        str1.sprintf( "%s - %s", AidaApplication::s_appName, str2.toAscii().data() );
	if( d_app->getRep()->isDirty() )
		str1 += "*";
	getQt()->setCaption( str1 );
}

void AidaCentral::openRepository(const char * path)
{
	try
	{
		QApplication::setOverrideCursor( Qt::waitCursor );
		d_app->openRepository( path );
		d_app->reloadSpecs();
		updateTitle();
		QApplication::restoreOverrideCursor();
	}catch( Root::Exception& e )
	{
		QApplication::restoreOverrideCursor();
		Root::ReportToUser::alert( this, "Error Opening Repository", e.what() );
		return;
	}
}

void AidaCentral::handleFileQuit( Action& a )
{
	ACTION_ENABLED_IF( a, true );
	// Das muss hier behandelt werden und nicht in Application, da
	// sonst unterschiedliches Verhalten ob Click in x oder FileQuit.
	closeIfPossible();
}

void AidaCentral::handleFileOpen( Action& a )
{
	ACTION_ENABLED_IF( a, true );

	if( !askToCloseProject() )
		return;

	QString fileName = QFileDialog::getOpenFileName( getQt(), 
		"Select Repository", AppAgent::getCurrentDir(), 
		"Repository (*.cara)" );
    if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );
	fileName = info.absFilePath();
	AppAgent::setCurrentDir( info.dirPath( true ) );

	openRepository( fileName );
}

void AidaCentral::handleFileNew( Action& a )
{
	ACTION_ENABLED_IF( a, true );

	if( !askToCloseProject() )
		return;

	d_app->newRepository();
}

void AidaCentral::handleFileSave( Action& a )
{
	ACTION_ENABLED_IF( a, true );
	handleSave( false );
}

void AidaCentral::handleFileSaveAs( Action& a )
{
	ACTION_ENABLED_IF( a, true );
	handleSave( true );
}

void AidaCentral::handleHelpAbout( Action& a )
{
	ACTION_ENABLED_IF( a, true );

	QString str;
	str.sprintf( "<html><body><p>Release: <strong>%s</strong> Date: <strong>%s</strong> </p>"

		"<p>CARA is an application for the analysis of NMR spectra and computer "
		"aided resonance assignment developed in the group of Prof. Dr. Kurt "
		"Wüthrich at the Institute of Molecular Biology and Biophysics, ETH Zürich.</p>"

		"<p>CARA is written, maintained and supported by the <em>CARA Definition Team</em> "
		"(CDT): <a href='mailto:fred@nmr.ch'>Fred Damberger</a>, "
        "<a href='rochus@nmr.ch'>Rochus Keller</a>. "
		"For documentation, updates and maintenance requests "
		"please refer to our website "
        "<a href='http://cara.nmr.ch'>cara.nmr.ch</a>.</p>"

		"<p>Copyright (c) 2000-%s by Rochus Keller and others</p>"

		"<h4>Additional Credits:</h4>"
		"<p>Peter Güntert, Pascal Bettendorff, James Masse, Konstantin Pervushin, "
		"Roland Riek, Beat Meier, Ansgar Siemer, Kurt Wüthrich, Thorsten Lührs, Jocélyne Fiaux, "
		"Sebastian Hiller, Alvar Gossert, Daniel Perez, Francesco Fiorito, Rudolf Baumann<br>"
		"NEWMAT (c) 1991-2002 by R. B. Davies<br>"
		"Qt GUI Toolkit 4.3 (c) 1995-2009 Trolltech AS<br>"
		"Expat XML Parser (c) 1998-2006 by Clark Cooper and others<br>"
		"Lua 5.1 by R. Ierusalimschy, L. H. de Figueiredo & W. Celes "
		"(c) 1994-2006 Tecgraf, PUC-Rio<p>"

		"<h4>Terms of use:</h4>"
		"<p>CARA is free software licensed under the <em>GNU General Public License</em> "
		"(<a href='http://www.gnu.org/licenses/'>GPL</a>). Please cite Rochus Keller's PhD "
		"<em>Optimizing the Process of Nuclear Magnetic Resonance Spectrum Analysis and Computer Aided Resonance Assignment</em> "
		"(<a href='http://www.cara.nmr-software.org/downloads/NMR.017-1.1_Online.pdf'>Diss. ETH Nr. 15947</a>) "
		"and the website <a href='http://cara.nmr.ch'>cara.nmr.ch</a> in your resulting publications. "
		"The program and documentation are provided <em>as is</em>, without warranty of any kind, "
		"expressed or implied, including but not limited to the warranties of merchantability, "
		"fitness for a particular purpose and noninfringement. In no event shall the author or copyright holders be "
		"liable for any claim, damages or other liability, whether in an action of contract, tort or otherwise, "
		"arising from, out of or in connection with the software or the use or other dealings in the software."
		"</p></body></html>",
		AidaApplication::s_release, AidaApplication::s_relDate, AidaApplication::s_copyRightYear );

    QMessageBox::about( getQt(), CAPTION2, str );
}

void AidaCentral::handleSetupPeakColors(Action &a)
{
    ACTION_ENABLED_IF( a, true );

    PeakColorDlg dlg(getQt());
    dlg.select( d_app->getRep()->getColors() );
}

void AidaCentral::handleNewFromTemplate(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	if( !askToCloseProject() )
		return;

	QString fileName = QFileDialog::getOpenFileName( getQt(), 
		"Select Template", AppAgent::getCurrentDir(), 
		"Repository (*.cara)" );
    if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );
	AppAgent::setCurrentDir( info.dirPath( true ) );

	try
	{
		QApplication::setOverrideCursor( Qt::waitCursor );
		d_app->newFromTemplate( fileName );
		d_app->reloadSpecs();
		QApplication::restoreOverrideCursor();
	}catch( Root::Exception& e )
	{
		QApplication::restoreOverrideCursor();
		Root::ReportToUser::alert( this, "Error Creating Repository", e.what() );
		return;
	}
}

void AidaCentral::handleShowNeasy(Action & a)
{
#ifdef _HasNeasy_
	ACTION_ENABLED_IF( a, true );

	QMainWindow* w = Neasy::showMainWindow();
	if( w )
		w->raise();
#endif
}

void AidaCentral::handleShowNeasyPure(Action & a)
{
#ifdef _HasNeasy_
    ACTION_ENABLED_IF( a, true );

	QMainWindow* w = Neasy::showMainWindow( false );
	if( w )
		w->raise();
#endif
}

void createPhaser( Root::Agent* a, Spec::Spectrum* s );

void AidaCentral::handleOpenPhaser(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	QString fileName = QFileDialog::getOpenFileName( getQt(), "Select Real Part Spectrum",
        AppAgent::getCurrentDir(), Spectrum::s_fileFilter );
    if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );
	AppAgent::setCurrentDir( info.dirPath( true ) );

	try
	{
		QApplication::setOverrideCursor( Qt::waitCursor );
		Spectrum* spec = Factory::createSpectrum( fileName );
		if( spec == 0 )
			Root::ReportToUser::alert( this, "Error opening spectrum",
				"Unknown spectrum format" );
		else
			createPhaser( this, spec );
		QApplication::restoreOverrideCursor();
	}catch( Root::Exception& e )
	{
		QApplication::restoreOverrideCursor();
		Root::ReportToUser::alert( this, "Error Phasing Spectrum", e.what() );
	}
}

void AidaCentral::handleOpenSpecRot(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	QString fileName = QFileDialog::getOpenFileName( getQt(), "Select Spectrum File", 
        AppAgent::getCurrentDir(), Spectrum::s_fileFilter );
    if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );
	AppAgent::setCurrentDir( info.dirPath( true ) );

	try
	{
		SpecRef<Spectrum> spec = Factory::createSpectrum( fileName );
		if( spec == 0 )
			Root::ReportToUser::alert( this, "Error opening spectrum",
				"Unknown spectrum format" );

		if( spec->getDimCount() == 1 )
			new SliceScope( this, spec, 0, 0 );
		else
		{ 
			Rotation rot( spec->getDimCount() );	
			RotateDlg dlg( getQt(), "Original", "View" );
			dlg.setCaption( "Open MonoScope" );
			QString s1, s2;
			for( Dimension d = 0; d < spec->getDimCount(); d++ )
			{
				rot[ d ] = d;
				s1.sprintf( "%s (%s)", 
					spec->getScale( d ).getColor().getIsoLabel(), 
					spec->getDimName( d ) );
				s2.sprintf( "%s", RotateDlg::stdLabel( d ) );
				dlg.addDimension( s1, s2 );
			}
			if( !dlg.rotate( rot ) )
				return;
			QApplication::setOverrideCursor( Qt::waitCursor );
			createMonoScope( this, spec, 0, 0, rot );
			QApplication::restoreOverrideCursor();
		}
	}catch( Root::Exception& e )
	{
		QApplication::restoreOverrideCursor();
		Root::ReportToUser::alert( this, "Error opening spectrum", e.what() );
	}
}

void AidaCentral::handleConvert(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	QString fileName = QFileDialog::getOpenFileName( getQt(), "Select Source Spectrum File",
        AppAgent::getCurrentDir(), Spectrum::s_fileFilter );
    if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );
	AppAgent::setCurrentDir( info.dirPath( true ) );

	Root::Ref<Spectrum> spec;
    //Root::UInt32 old = FileSpectrum::getMapThreshold();
	try
	{
		spec = Factory::createSpectrum( fileName );
		if( spec == 0 )
		{
			Root::ReportToUser::alert( this, "Error opening spectrum",
				"Unknown source spectrum format" );
			return;
		}
		//FileSpectrum::setMapThreshold( s_max );	// RISK Sicher mit Mapping ffnen
		SpectrumListView::saveCaraSpectrum( getQt(), spec );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Error converting spectrum", e.what() );
	}
	//FileSpectrum::setMapThreshold( old );
}

void AidaCentral::handleConvert2(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	QString fileName = QFileDialog::getOpenFileName( getQt(), "Select Source Spectrum File",
        AppAgent::getCurrentDir(), Spectrum::s_fileFilter );
    if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );
	AppAgent::setCurrentDir( info.dirPath( true ) );

	Root::Ref<Spectrum> spec;
    //Root::UInt32 old = FileSpectrum::getMapThreshold();
	try
	{
		spec = Factory::createSpectrum( fileName );
		if( spec == 0 )
		{
			Root::ReportToUser::alert( this, "Error opening spectrum",
				"Unknown source spectrum format" );
			return;
		}
		//FileSpectrum::setMapThreshold( s_max );	// RISK Sicher mit Mapping ffnen
		SpectrumListView::saveEasySpectrum( getQt(), spec );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Error converting spectrum", e.what() );
	}
	//FileSpectrum::setMapThreshold( old );
}

void AidaCentral::handleMapThreshold(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	bool ok;
	int v = QInputDialog::getInteger( "Set Mapping Threshold", 
		"Enter an upper byte limit (0..2 GB) under which memory mapping is used:", FileSpectrum::getMapThreshold(), 
		0, s_max, 1, &ok, getQt() );
	if( !ok )
		return;
	FileSpectrum::setMapThreshold( v );
}

void AidaCentral::handleLabelFont(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	bool ok;
    QFont res = SpinPointView3::s_defaultFont;
	res = QFontDialog::getFont( &ok, res, getQt() );
	if( !ok )
		return;
	SpinPointView::s_defaultFont = res;
    SpinPointView3::s_defaultFont = res;
    PeakPlaneView::s_default = res;
    PeakPlaneView3::s_default = res;
}

void AidaCentral::handleExecute(Action & a)
{
    ACTION_ENABLED_IF( a, Engine::inst() && !Engine::inst()->isExecuting() );
	if( a.getParamCount() > 0 )
    {
		if( !Engine::inst()->executeCmd( a.getParam( 0 ).getCStr(), "#Command Line" ) && !Engine::inst()->isSilent() )
            setStatusMessage( Engine::inst()->getLastError() );
    }
}

void AidaCentral::handleFlatten(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	QString fileName = QFileDialog::getOpenFileName( getQt(), "Select Source Spectrum File", 
        AppAgent::getCurrentDir(), Spectrum::s_fileFilter );
    if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );
	AppAgent::setCurrentDir( info.dirPath( true ) );

	Root::Ref<Spectrum> spec;
    //Root::UInt32 old = FileSpectrum::getMapThreshold();
	try
	{
		spec = Factory::createSpectrum( fileName );
		if( spec == 0 )
		{
			Root::ReportToUser::alert( this, "Error opening spectrum",
				"Unknown source spectrum format" );
			return;
		}
		//FileSpectrum::setMapThreshold( s_max );	// RISK Sicher mit Mapping ffnen
		SpectrumListView::saveFlatCaraSpectrum( getQt(), spec );
	}catch( Root::Exception& e )
	{
		Root::ReportToUser::alert( this, "Error projecting spectrum", e.what() );
	}
	//FileSpectrum::setMapThreshold( old );
}

void AidaCentral::handleLabelOffset(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	bool ok;
	int v = QInputDialog::getInteger( "Set Label Offset", 
		"Enter a value in TWIP (1/1440 Inch) or 0:", SpinPointView::s_defaultOff, 
		0, 999999, 1, &ok, getQt() );
	if( !ok )
		return;
	SpinPointView::s_defaultOff = v;
    SpinPointView3::s_defaultOff = v;
}

void AidaCentral::handleLabelAngle(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	bool ok;
	int v = QInputDialog::getInteger( "Set Spin Angle", 
		"Enter a value between 0 and 359 degree:", SpinPointView::s_defaultAngle, 
		0, 359, 1, &ok, getQt() );
	if( !ok )
		return;
	SpinPointView::s_defaultAngle = v;
    SpinPointView3::s_defaultAngle = v;
}

void AidaCentral::handleLabelCross(Action & a)
{
	ACTION_CHECKED_IF( a, true, SpinPointView::s_defaultCross );

	SpinPointView::s_defaultCross = !SpinPointView::s_defaultCross;
    SpinPointView3::s_defaultCross = SpinPointView::s_defaultCross;
}

void AidaCentral::handleGarbageCollect(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	QApplication::setOverrideCursor( Qt::waitCursor );
	Root::Mem::collect();
	Engine* lua = Engine::inst();
	if( lua )
		lua->collect();

	QApplication::restoreOverrideCursor();
}

void AidaCentral::handleEnableGc(Action & a)
{
	ACTION_CHECKED_IF( a, true, s_gcEnabled );
	
	QApplication::setOverrideCursor( Qt::waitCursor );
	if( s_gcEnabled )
		Root::Mem::disableGc();
	else
		Root::Mem::enableGc();
	QApplication::restoreOverrideCursor();
	s_gcEnabled = !s_gcEnabled;
}

void AidaCentral::handleEditProperties(Action & a)
{
	ACTION_ENABLED_IF( a, true );
	static Root::Ref<ObjectDef> s_obj;

	if( s_obj == 0 )
	{
		s_obj = new ObjectDef( Repository::keyRepository );
		s_obj->setField( Repository::keyAuthor, Root::Any::CStr, "" );
		s_obj->setField( Repository::keyCreationDate, Root::Any::Date, "" );
		s_obj->setField( Repository::keyDesc, Root::Any::Memo, "" );
		s_obj->setField( Repository::keyTplDesc, Root::Any::Memo, "" );
		s_obj->setField( Repository::keyTplAuthor, Root::Any::CStr, "" );
		s_obj->setField( Repository::keyTplPath, Root::Any::CStr, "" );
		s_obj->setField( Repository::keyTplDate, Root::Any::Date, "" );
		s_obj->setField( Repository::keyOrigTplDate, Root::Any::Date, "" );
		s_obj->setField( Repository::keyOrigTplDesc, Root::Any::Memo, "" );
		s_obj->setField( Repository::keyOrigTplAuthor, Root::Any::CStr, "" );
	}
	DynValueEditor::edit( getQt(), s_obj, getRep() );
}

void AidaCentral::handleXeasyN(Action & a)
{
	ACTION_CHECKED_IF( a, true, g_useXeasyN );

	g_useXeasyN = !g_useXeasyN;
	try
	{
		QApplication::setOverrideCursor( Qt::waitCursor );
		d_app->getRep()->reloadSpecs();
		QApplication::restoreOverrideCursor();
	}catch( Root::Exception& e )
	{
		QApplication::restoreOverrideCursor();
		Root::ReportToUser::alert( this, "Error Reloading Spectra", e.what() );
	}
}

extern void createSitarViewer( Root::Agent* a, Spec::Spectrum* s );

void AidaCentral::handleOpenSitar(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	QString fileName = QFileDialog::getOpenFileName( getQt(), "Select Spectrum File", AppAgent::getCurrentDir(), 
		"Sitar Spectrum (*.sitar)" );
    if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );
	AppAgent::setCurrentDir( info.dirPath( true ) );

	try
	{
		QApplication::setOverrideCursor( Qt::waitCursor );
		SpecRef<Spectrum> spec = Factory::createSpectrum( fileName );
		if( spec == 0 )
			Root::ReportToUser::alert( this, "Error opening spectrum",
				"Unknown spectrum format" );
		createSitarViewer( this, spec );
		QApplication::restoreOverrideCursor();
	}catch( Root::Exception& e )
	{
		QApplication::restoreOverrideCursor();
		Root::ReportToUser::alert( this, "Error opening sitar spectrum", e.what() );
	}catch( ... )
	{
		QApplication::restoreOverrideCursor();
		Root::ReportToUser::alert( this, "Error opening sitar spectrum: Unknown exception" );
	}
}

void AidaCentral::handleShowStyle(Action & a)
{
	ACTION_CHECKED_IF( a, true, s_style == a.getParam( 0 ).getCStr() );

	s_style = a.getParam( 0 ).getCStr();
	qApp->setStyle( QStyleFactory::create( s_style ) );
}

void AidaCentral::handleLuaBox(Action & a)
{
	ACTION_ENABLED_IF( a, Engine::inst() );

	d_term->setVisible( !d_term->isVisible() );
}

