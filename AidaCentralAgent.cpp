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

#include "AidaCentralAgent.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog> 
#include <QFileInfo> 
#include <QApplication>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QtDebug>

#include <Root/Application.h>
#include <Spec/SequenceFile.h>
#include <Spec/EasyProtonList.h>
#include <Lexi/GlyphWidget.h> 
#include <Spec/Fragmenter.h>
#include <Spec/StarFile.h>
#include <Root/MessageLog.h>
#include <Script/Engine.h>
#include <Script/Terminal2.h>

#include <AidaCentral.h>
#include <ResidueTypeView.h>
#include <SpectrumTypeView.h>
#include <SequenceView.h>
#include <SampleView.h>
#include <SpectrumListView.h>
#include <PeakListListView.h>
#include <SpinSystemView.h>
#include <SystemTypeView.h>
#include <LabelingSchemeView.h>
#include <ObjectDefView.h>
#include <MessageView.h>
#include <SpecView/DynValueEditor.h>
#include <StripListGadget.h>
#include <SpinLinkListView.h>
#include <ScriptView.h>
#include <Icons.h>
#include <Dlg.h>
#include <Root/Vector.h>

#include "LuaCaraExplorer.h"
using namespace Spec;
using namespace Lua;

//////////////////////////////////////////////////////////////////////

Root::Action::CmdStr AidaCentralAgent::NewProject = "NewProject";
Root::Action::CmdStr AidaCentralAgent::ImportSeq = "ImportSeq";
Root::Action::CmdStr AidaCentralAgent::ImportAtomList = "ImportAtomList";
Root::Action::CmdStr AidaCentralAgent::ImportSeqVals = "ImportSeqVals";
Root::Action::CmdStr AidaCentralAgent::ExportMapper = "ExportMapper";
Root::Action::CmdStr AidaCentralAgent::ExportSequence = "ExportSequence";
Root::Action::CmdStr AidaCentralAgent::ExportAtomList = "ExportAtomList";
Root::Action::CmdStr AidaCentralAgent::RemoveProject = "RemoveProject";
Root::Action::CmdStr AidaCentralAgent::RenameProject = "RenameProject";
Root::Action::CmdStr AidaCentralAgent::DuplicatePro = "DuplicatePro";
Root::Action::CmdStr AidaCentralAgent::ImportProject = "ImportProject";
Root::Action::CmdStr AidaCentralAgent::SetOrigin = "SetOrigin";

ACTION_SLOTS_BEGIN( AidaCentralAgent )
    { AidaCentralAgent::SetOrigin, &AidaCentralAgent::handleSetOrigin },
    { AidaCentralAgent::ImportProject, &AidaCentralAgent::handleImportProject },
    { AidaCentralAgent::DuplicatePro, &AidaCentralAgent::handleDuplicatePro },
    { AidaCentralAgent::RenameProject, &AidaCentralAgent::handleRenameProject },
    { AidaCentralAgent::RemoveProject, &AidaCentralAgent::handleRemoveProject },
    { AidaCentralAgent::ImportSeq, &AidaCentralAgent::handleImportSeq },
    { AidaCentralAgent::ImportAtomList, &AidaCentralAgent::handleImportAtomList },
    { AidaCentralAgent::ImportSeqVals, &AidaCentralAgent::handleImportSeqVals },
    { AidaCentralAgent::ExportAtomList, &AidaCentralAgent::handleExportAtomList },
    { AidaCentralAgent::ExportSequence, &AidaCentralAgent::handleExportSequence },
    { AidaCentralAgent::ExportMapper, &AidaCentralAgent::handleExportMapper },
    { AidaCentralAgent::NewProject, &AidaCentralAgent::handleNewProject },
ACTION_SLOTS_END( AidaCentralAgent )

//////////////////////////////////////////////////////////////////////

enum _ItemType { 
	_SpectrumTypes, 
	_Scripts, 
	_ObjectDefs, // Attribute Definitions
	_ObjectDef, // class
	_ResidueTypes, 
	_SystemTypes, // Spin System Types
	_LabelingSchemes, 
	_Projects,
	_Project, // name
	_Sequence,
	_Samples,
	_Specta, 
	_PeakLists, 
	_Spins,
	_Systems,
	_SpinLinks,
	_MessageLog,
	_Terminal
};

static const char* s_names[] =
{
	"Spectrum Types", //_SpectrumTypes, 
	"Scripts", //_Scripts, 
	"Attribute Definitions", // _ObjectDefs, // Attribute Definitions
	"Attribute Definitions", // _ObjectDef, // class
	"Residue Types", // _ResidueTypes, 
	"Spin System Types", //_SystemTypes, // Spin System Types
	"Labeling Schemes", //_LabelingSchemes, 
	"Projects", // _Projects,
	"Project", // _Project, // name
	"Sequence", //_Sequence,
	"Samples", // _Samples,
	"Spectra", // _Specta, 
	"Peak Lists", // _PeakLists, 
	"Spins", // _Spins,
	"Systems", // _Systems,
	"Spin Links", //_SpinLinks,
	"Message Log", //_MessageLog,
	"Terminal", // _Terminal,
	0,
};

class AidaCentralAgentTreeItem : public Gui::ListViewItem
{
	QString d_text;
	int d_type;
public:
	AidaCentralAgentTreeItem( Gui::ListView* v, const QString& t, int type ):
        ListViewItem( v ),d_text(t),d_type( type),d_view(0) {}
	AidaCentralAgentTreeItem( Gui::ListViewItem* v, const QString& t,int type ):
        ListViewItem( v ),d_text(t),d_type(type),d_view(0) {}
	virtual QString text ( int column ) const { return d_text; }
	int type() const { return d_type; }
	QWidget* d_view;
};

//////////////////////////////////////////////////////////////////////

static void insertProject( AidaCentralAgentTreeItem* pi, Project* pro )
{
	AidaCentralAgentTreeItem* p = new AidaCentralAgentTreeItem( pi, pro->getName().data(), _Project );
	// NOTE: Fr Name-Update wird in AidaCentral reloadProjects aufgerufen
	p->setOpen( true );
	new AidaCentralAgentTreeItem( p, s_names[_PeakLists], _PeakLists );
	new AidaCentralAgentTreeItem( p, s_names[_Samples], _Samples );
	new AidaCentralAgentTreeItem( p, s_names[_Sequence], _Sequence );
	new AidaCentralAgentTreeItem( p, s_names[_Specta], _Specta );
	new AidaCentralAgentTreeItem( p, s_names[_SpinLinks], _SpinLinks );
	new AidaCentralAgentTreeItem( p, s_names[_Spins], _Spins );
	new AidaCentralAgentTreeItem( p, s_names[_Systems], _Systems );
}

//////////////////////////////////////////////////////////////////////

AidaCentralAgent::AidaCentralAgent(AidaCentral* p):
	QSplitter( p->getQt() ), d_parent( p ), d_pane( 0 ), d_cur( 0 ), d_popup(0)
{
	Icons::init();

    d_menuTree = new Gui::ListView( this );
	d_menuTree->addColumn( "" );
	d_menuTree->setSorting( 0 );
	d_menuTree->setAlternatingRowColors( false );
	d_menuTree->setRootIsDecorated( true );
	d_menuTree->header()->hide();
	addWidget( d_menuTree );
	connect( d_menuTree, SIGNAL( currentChanged () ), this, SLOT( onCurrentItemChanged () ) );

    QWidget* pane = new QWidget( this );
    d_hbox = new QHBoxLayout( pane );
	d_hbox->setSpacing( 0 );
	d_hbox->setMargin( 0 );
    addWidget( pane );

    // NOTE: Splitterposition wird in initialize() gesetzt!

	buildPopups();

	LuaCaraExplorer::install();
	LuaCaraExplorer::installExplorer( this );
}

void AidaCentralAgent::setPane( QWidget* w )
{
	d_pane = w;
	d_hbox->addWidget( w );
	//w->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Minimum );
	w->show();
	w->setFocus();
}

void AidaCentralAgent::clearPane()
{
	if( d_pane )
	{
		d_pane->hide();
		d_hbox->removeWidget( d_pane );
		assert( d_cur );
		if( !isPersistent( d_cur->type() ) )
			delete d_pane;
		d_pane = 0;
		d_cur = 0;
	}
}

void AidaCentralAgent::buildPopups()
{
	Gui::Menu* popProject = new Gui::Menu( d_menuTree, true );
	Gui::Menu::item( popProject, this, "&New Project...", NewProject, false );
	Gui::Menu::item( popProject, this, "&Import Project from Sequence...", ImportSeq, false );
	Gui::Menu::item( popProject, this, "&Import Project from BMRB...", ImportProject, false );

	popProject->addSeparator();

	Gui::Menu::item( popProject, this, "&Remove Project...", 
		AidaCentralAgent::RemoveProject, false );
	Gui::Menu::item( popProject, this, "Rena&me Project...", 
		AidaCentralAgent::RenameProject, false );
	Gui::Menu::item( popProject, this, "Duplicate Project...", 
		AidaCentralAgent::DuplicatePro, false );
	Gui::Menu::item( popProject, this, "Set Location Origin...", 
		AidaCentralAgent::SetOrigin, false );

	popProject->addSeparator();

	Gui::Menu* subImport = new Gui::Menu( popProject );
	popProject->insertItem( "Import", subImport );
	Gui::Menu::item( subImport, this, "&Atom List...", 
		AidaCentralAgent::ImportAtomList, false );
	Gui::Menu::item( subImport, this, "&Sequence Values...", 
		AidaCentralAgent::ImportSeqVals, false );
	Gui::Menu* subExport = new Gui::Menu( popProject );
	popProject->insertItem( "Export", subExport );
	Gui::Menu::item( subExport, this, "&Mapper File...", 
		AidaCentralAgent::ExportMapper, false );
	Gui::Menu::item( subExport, this, "&Sequence...", 
		AidaCentralAgent::ExportSequence, false );
	Gui::Menu::item( subExport, this, "&Atom List...", 
		AidaCentralAgent::ExportAtomList, false );

	d_popup = popProject;

}

AidaCentralAgent::~AidaCentralAgent()
{
	QMap<int,QPointer<Gui::Menu> >::const_iterator i;
	for( i = d_pops.begin(); i != d_pops.end(); ++i )
		if( i.value() )
			delete i.value();
}

static void _deletePane( AidaCentralAgentTreeItem* item )
{
	if( item->d_view )
		delete item->d_view;
	for( int i = 0; i < item->count(); i++ )
		_deletePane( static_cast<AidaCentralAgentTreeItem*>( item->child( i ) ) );
}

void AidaCentralAgent::initialize()
{
	clearPane(); 
	for( int i = 0; i < d_menuTree->count(); i++ )
		_deletePane( static_cast<AidaCentralAgentTreeItem*>( d_menuTree->child( i ) ) );
	d_menuTree->clear(); 

	Repository* rep = d_parent->getRep();
	assert( rep );
	Engine* lua = Engine::inst();
	assert( lua );

	AidaCentralAgentTreeItem* oi = new AidaCentralAgentTreeItem(
                                       d_menuTree, s_names[_ObjectDefs], _ObjectDefs );
	Repository::ObjectDefs::const_iterator p1;
	for( p1 = rep->getClasses().begin(); p1 != rep->getClasses().end(); ++p1 )
        new AidaCentralAgentTreeItem( oi, (*p1).first.data(), _ObjectDef );
	oi->sort(false);

	new AidaCentralAgentTreeItem( d_menuTree, s_names[_LabelingSchemes], _LabelingSchemes );
	new AidaCentralAgentTreeItem( d_menuTree, s_names[_MessageLog], _MessageLog );

	d_pi = new AidaCentralAgentTreeItem( d_menuTree, s_names[_Projects], _Projects );
	d_pi->setOpen( true );
	Repository::ProjectMap::const_iterator p;
	for( p = rep->getProjects().begin(); p != rep->getProjects().end(); ++p )
		insertProject( d_pi, (*p).second );
	d_pi->sort( false );

	new AidaCentralAgentTreeItem( d_menuTree, s_names[_ResidueTypes], _ResidueTypes );
	new AidaCentralAgentTreeItem( d_menuTree, s_names[_SpectrumTypes], _SpectrumTypes );
	new AidaCentralAgentTreeItem( d_menuTree, s_names[_SystemTypes], _SystemTypes );
	new AidaCentralAgentTreeItem( d_menuTree, s_names[_Scripts], _Scripts );
	AidaCentralAgentTreeItem* term = new AidaCentralAgentTreeItem( d_menuTree, s_names[_Terminal], _Terminal );

	term->setCurrent(); // Terminal soll gleich angelegt werden, damit die Messages nicht verlorengehen
	d_pi->setCurrent();
	d_menuTree->resizeColumnToContents( 0 );
    const int w = d_menuTree->columnWidth( 0 ) + 2 * d_menuTree->frameWidth();
    d_menuTree->setMinimumWidth( w );
    // Dummerweise ist Fenster hier noch nicht offen, weshalb width nur Unsinn zurckgibt.
    // 10 * w scheint eine vernnftige Heuristik zu sein.
    setSizes( QList<int>() << w << 10 * w );
	// frher: d_menuTree->setFixedWidth( d_menuTree->columnWidth( 0 ) + 2 * d_menuTree->frameWidth() );
}

void AidaCentralAgent::reloadProjects()
{
	d_pi->setCurrent();
	while( d_pi->count() )
	{
		AidaCentralAgentTreeItem* item = static_cast<AidaCentralAgentTreeItem*>( d_pi->child( 0 ) );
		if( item->d_view )
			delete item->d_view;
		d_menuTree->removeItem( item );
	}
	Repository* rep = d_parent->getRep();
	assert( rep );
	Repository::ProjectMap::const_iterator p;
	for( p = rep->getProjects().begin(); p != rep->getProjects().end(); ++p )
		insertProject( d_pi, (*p).second );
	d_pi->sort( false );
}

QWidget* AidaCentralAgent::createView( int type, const char* name )
{
	QWidget* w;
	switch( type )
	{
	case _ResidueTypes:
		w = new ResidueTypeView( this, d_parent->getRep() );
		break;
	case _SpectrumTypes:
		w = new SpectrumTypeView( this, d_parent->getRep() );
		break;
	case _MessageLog:
		return new MessageView( this );
	case _Scripts:
		w = new ScriptView( this, d_parent, Engine::inst(), d_parent->getRep() );
		break;
	case _ObjectDef:
		{
			ObjectDef* def = d_parent->getRep()->findObjectDef( name );
			assert( def );
			w = new ObjectDefView( this, def );
		}
		break;
	case _Project:
		{
			Project* pro = d_parent->getRep()->findProject( name );
			assert( pro );
			return new DynValueEditor( this, 
				d_parent->getRep()->findObjectDef( Repository::keyProject ), pro ); 
		}
	case _Samples:
		{
			Project* pro = d_parent->getRep()->findProject( name );
			assert( pro );
			w = new SampleView( this, pro );
		}
		break;
	case _Sequence:
		{
			Project* pro = d_parent->getRep()->findProject( name );
			assert( pro );
			w = new SequenceView( this, pro );
		}
		break;
	case _Specta:
		{
			Project* pro = d_parent->getRep()->findProject( name );
			assert( pro );
			w = new SpectrumListView( this, d_parent, d_parent->getRep(), pro );
		}
		break;
	case _PeakLists:
		{
			Project* pro = d_parent->getRep()->findProject( name );
			assert( pro );
			w = new PeakListListView( this, d_parent, d_parent->getRep(), pro );
		}
		break;
	case _Spins:
		{
			Project* pro = d_parent->getRep()->findProject( name );
			assert( pro );
			w = new SpinSystemView( this, d_parent, pro );
		}
		break;
	case _Systems:
		{
			Project* pro = d_parent->getRep()->findProject( name );
			assert( pro );
			Lexi::GlyphWidget* w = new Lexi::GlyphWidget( this, 0, 0 );
			StripListGadget* g = new StripListGadget( w->getViewport(),
				pro, d_parent, 0, true, true );
			Gui::Menu* m = createMenu( type );
			if( m )
				m->connectPopup( g->getListView()->getImp() );
			w->setBody( g );
			return w;
		}
	case _SpinLinks:
		{
			Project* pro = d_parent->getRep()->findProject( name );
			assert( pro );
			w = new SpinLinkListView( this, d_parent, pro );
		}
		break;
	case _SystemTypes:
		w = new SystemTypeView( this, d_parent->getRep() );
		break;
	case _LabelingSchemes:
		w = new LabelingSchemeView( this, d_parent->getRep() );
		break;
	case _Terminal:
		return new Lua::Terminal2( this, Engine::inst() );
	default:
		return new DynValueEditor( this, 
			d_parent->getRep()->findObjectDef( Repository::keyRepository ), d_parent->getRep() );
	}
	Gui::Menu* m = createMenu( type );
	if( m )
	{
        if( type == _Scripts )
            m->connectPopup( static_cast<ScriptView*>( w )->getList() ); // Die Ausnahme
        else
            m->connectPopup( w );
	}
	return w;
}

Gui::Menu* AidaCentralAgent::createMenu( int type )
{
    // Wir machen das, damit man per Lua auf die Popups zugreifen kann!
    // Das kann passieren, bevor die View überhaupt erzeugt wurde.
	Gui::Menu* pop = d_pops.value( type );
	if( pop == 0 )
	{
		switch( type )
		{
        case _LabelingSchemes:
            pop = LabelingSchemeView::createPopup();
            break;
        case _SpectrumTypes:
            pop = SpectrumTypeView::createPopup();
            break;
        case _Scripts:
            pop = ScriptView::createPopup();
            break;
        case _ObjectDefs:
            return 0; // kein Menu vorhanden
        case _ObjectDef:
            pop = ObjectDefView::createPopup();
            break;
        case _ResidueTypes:
			pop = ResidueTypeView::createPopup();
			break;
		case _SystemTypes:
			pop = SystemTypeView::createPopup();
			break;
        case _Projects:
        case _Project:
            return 0; // Kein Men
        case _Sequence:
			pop = SequenceView::createPopup();
			break;
		case _Samples:
			pop = SampleView::createPopup();
			break;
        case _Specta:
            pop = SpectrumListView::createPopup( d_parent->getRep() );
            break;
        case _PeakLists:
			pop = PeakListListView::createPopup();
			break;
		case _Spins:
			pop = SpinSystemView::createPopup();
			break; 
		case _Systems:
			pop = StripListGadget::createPopup();
			break;
		case _SpinLinks:
			pop = SpinLinkListView::createPopup();
			break;
		case _MessageLog:
            return 0; // nicht automatisierbar
		case _Terminal:
            return 0; // nicht automatisierbar
        default:
			return 0;
		}
        assert( pop != 0 );
        // NOTE: wenn man Parent setzt, geht Men nicht mehr zu, da sich Popup-Ziel von this unterscheidet
		//pop->setParent( this );
		d_pops[type] = pop;
		pop->setTitle( s_names[type] );
	}
	return pop;
}

static int findName( const QByteArray& n )
{
	int i = 0;
	while( s_names[i] )
	{
		QByteArray s = s_names[i];
		if( n == s.toLower() )
			return i;
		i++;
	}
	return -1;
}

QMenu* AidaCentralAgent::getPopup(const QByteArray& name)
{ 
	QByteArray n = name.toLower();
	if( n.isEmpty() || n == "navi" || n == "navigator" )
		return d_popup;
	else 
		return createMenu( findName( n ) );
}

bool AidaCentralAgent::isPersistent( int type ) const
{
	switch( type )
	{
	case _Samples:
	case _Sequence:
	case _Specta:
	case _PeakLists:
	case _Spins:
	case _Systems:
	case _SpinLinks:
	case _SpectrumTypes:
	case _Scripts:
	case _ResidueTypes:
	case _SystemTypes:
	case _LabelingSchemes:
	case _MessageLog:
	case _Terminal:
		return true;
	default:
		return false;
	}
}

void AidaCentralAgent::onCurrentItemChanged()
{
	AidaCentralAgentTreeItem* item = static_cast<AidaCentralAgentTreeItem*>( d_menuTree->currentItem() );
	if( item == 0 )
		return;
	clearPane();
	QWidget* view = 0;
	if( isPersistent( item->type() ) )
	{
		view = item->d_view;
	}
	if( view == 0 )
	{
		switch( item->type() )
		{
		case _ObjectDef:
		case _Project:
			view = createView( item->type(), item->text(0).toLatin1() );
			break;
		case _Samples:
		case _Sequence:
		case _Specta:
		case _PeakLists:
		case _Spins:
		case _Systems:
		case _SpinLinks:
			view = createView( item->type(), item->parent()->text(0).toLatin1() );
			break;
		default:
			view = createView( item->type() );
			break;
		}
	}
	assert( view );
	if( isPersistent( item->type() ) )
		item->d_view = view;
	d_cur = item;
	if( item->type() == _Project )
		LuaCaraExplorer::setCurrentProject( d_parent->getRep()->findProject( item->text(0).toLatin1() ) );
	setPane( view );
	d_parent->setContextMenu( createMenu( item->type() ) );
}

void AidaCentralAgent::handle(Root::Message & msg)
{
	BEGIN_HANDLER();
	MESSAGE( Root::Action, a, msg )
	{
		EXECUTE_ACTION( AidaCentralAgent, *a );
	}
	END_HANDLER();
}

void AidaCentralAgent::handleNewProject(Root::Action& a)
{
	ACTION_ENABLED_IF( a, true );

	bool ok;
	QString res = QInputDialog::getText( "New Project", 
		"Please enter a unique short name:", QLineEdit::Normal, 
		"", &ok, this );
	if( !ok || res.isEmpty() )
		return;
	
	bool loadSeq = false;
	switch( QMessageBox::warning( this, "New Project",
			  "Do you want to import a sequence?",
			  "&Yes", "&No", "&Cancel", 0, 2 ) )		
	{
	case 0:
		loadSeq = true; // Yes
		break;
	case 1:
		break;	// No
	default:
		return;	// Cancel
	}

	createProject( res, loadSeq );
}

void AidaCentralAgent::createProject(const char *_name, bool loadSeq )
{
	Root::SymbolString name = _name;

	Root::Ref<SequenceFile> sf;
	Repository* rep = d_parent->getRep();
	if( loadSeq )
	{
		try
		{
			QString fileName = QFileDialog::getOpenFileName( this, tr("Select Sequence File"),
				Root::AppAgent::getCurrentDir(), "Sequence (*.seq)" );
			if( !fileName.isNull() ) 
			{
				sf = new SequenceFile();
				sf->loadFromFile( rep, fileName );
				// Wenn man Cancel drckt, wird ein Projekt ohne Sequenz angelegt.

				QFileInfo info( fileName );
				Root::AppAgent::setCurrentDir( info.dirPath( true ) );
				if( name.isNull() )
				{
					name = info.baseName().toLatin1();
				}
			}else
				return;
		}catch( Root::Exception& e )
		{
			QMessageBox::critical( this, "New Project", e.what(), "&Cancel" );
			return;
		}
	}
	while( true )
	{
		const Repository::ProjectMap& pm = rep->getProjects();
		if( pm.find( name ) != pm.end() )
		{
			bool ok;
			QString res = QInputDialog::getText( "New Project", 
				"Name is not unique. Please enter a unique name:", QLineEdit::Normal, 
				name.data(), &ok, this );
			if( !ok || res.isEmpty() )
				return;
			name = res.toLatin1();
		}else
			break;
	}

	bool createSys = false;
	if( sf && sf->hasSys() )
	{
		switch( QMessageBox::information( this, "New Project",
				  "The sequence contains assignment information. "
				  "Do you want to initially create the corresponding spin systems?",
				  "&Yes", "&No", "&Cancel", 0, 2 ) )		
		{
		case 0: // Yes
			if( sf->checkSystems() )
				createSys = true; 
			else 
			{
				QMessageBox::critical( this, "New Project", 
					"The sequence contains non unique assignments!", "&Cancel" );
				return;
			}
			break;
		case 1:
			break;	// No
		default:
			return;	// Cancel
		}
	}else if( sf && sf->hasId() )
	{
		switch( QMessageBox::information( this, "New Project",
				  "Do the numbers given in the sequence file identify residues "
				  " or spin systems? ",
				  "&Residues", "&Spin Systems", "&Cancel", 0, 2 ) )		
		{
		case 0: // Resi
			break;
		case 1:
			sf->renumber();
			createSys = true; 
			break;	// Sys
		default:
			return;	// Cancel
		}
	}

	QApplication::setOverrideCursor( Qt::waitCursor );
	Root::Ref<Project> pro;
	try
	{
		pro = new Project( rep, name, sf, createSys );
		rep->addProject( pro );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "New Project", 
			e.what(), "&Cancel" );
	}
	QApplication::restoreOverrideCursor();
}

void AidaCentralAgent::handleImportSeq(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	createProject( "", true );
}

void AidaCentralAgent::importAtomList(Project* pro, EasyProtonList* pl, bool fragId )
{
	SpinBase* b = pro->getSpins();
	Sequence* seq = pro->getSequence();
	Residue* r;
	SpinSystem* ss;
	QString str;
	std::map<Root::Index,std::set<Root::SymbolString> > sys;
	std::set<Root::Index> spin;
	AtomType t;
	int i;
	std::set<Root::Index> doubles;
	SpinLabel l;
	int incompatibleSyntax = 0;
	int projectedSpin = 0;
	for( i = 0; i < pl->getCount(); i++ )
	{
		const EasyProtonList::Atom& a = pl->getAtom( i );
		if( a.isValid() )
		{
			// Prfe Uniqueness von Spin
			if( spin.count( a.d_spin ) != 0 )
			{
				QApplication::restoreOverrideCursor();
				str.sprintf( "Atom %d not unique within atom list!", a.d_spin );
				QMessageBox::critical( this, "Import Atom List", 
					str, "&Cancel" );
				return;
			}
			if( b->getSpins().count( a.d_spin ) != 0 )
			{
				doubles.insert( a.d_spin );
			}else
				spin.insert( a.d_spin );

			// Errate AtomType
			t = AtomType::parse( a.d_label.data(), a.d_shift );
			if( t.isNone() )
			{
				QApplication::restoreOverrideCursor();
				str.sprintf( "Cannot guess atom type %s of atom %d!", 
					a.d_label.data(), a.d_spin );
				QMessageBox::critical( this, "Import Atom List", 
					str, "&Cancel" );
				return;
			}

			if( a.d_sys > 0 )
			{
				// Residue muss bereits existieren
				if( !fragId )
				{
					r = seq->getResidue( a.d_sys );
					if( r == 0 )
					{
						QApplication::restoreOverrideCursor();
						str.sprintf( "Residue %d of atom %d does not exist!", 
							a.d_sys, a.d_spin );
						QMessageBox::critical( this, "Import Atom List", 
							str, "&Cancel" );
						return;
					}else
					{
						ss = r->getSystem();
						if( ss == 0 )
						{
							if( r->getPred() && r->getPred()->getSystem() )
							{
								if( r->getPred()->getSystem()->getSucc() != 0 )
								{
									QApplication::restoreOverrideCursor();
									str.sprintf( "System %d of pred. residue %d of atom %d is already occupied.", 
										r->getPred()->getSystem()->getId(), r->getPred()->getId(), a.d_spin );
									QMessageBox::critical( this, "Import Atom List", 
										str, "&Cancel" );
									return;
								}
							}
							if( r->getSucc() && r->getSucc()->getSystem() )
							{
								if( r->getSucc()->getSystem()->getPred() != 0 )
								{
									QApplication::restoreOverrideCursor();
									str.sprintf( "System %d of succ. residue %d of atom %d is already occupied.", 
										r->getSucc()->getSystem()->getId(), r->getSucc()->getId(), a.d_spin );
									QMessageBox::critical( this, "Import Atom List", 
										str, "&Cancel" );
									return;
								}
							}
						}
					}
				}

				// Prfe Label-Eindeutigkeit
				if( !a.d_label.isNull() && sys[ a.d_sys ].count( a.d_label ) != 0 )
				{
					QApplication::restoreOverrideCursor();
					str.sprintf( "Label %s not unique within system of atom %d!", 
						a.d_label.data(), a.d_spin );
					QMessageBox::critical( this, "Import Atom List", 
						str, "&Cancel" );
					return;
				}else
					sys[ a.d_sys ].insert( a.d_label );

				// Prfe, welche Art von Labels enthalten sind:
				if( !SpinLabel::parse( a.d_label.data(), l ) )
					incompatibleSyntax++;
				else if( l.getOffset() != 0 )
					projectedSpin++;
			}
		}
	}
	std::set<Root::Index>::const_iterator p3;
	if( !doubles.empty() )
	{
		str = "The following spins are already defined in project:\n";
		for( p3 = doubles.begin(); p3 != doubles.end(); ++p3 )
		{
			str += QString().setNum( (*p3) );
			str += " ";
		}
		Root::MessageLog::inst()->warning( "Import Atom List", str );
		str.sprintf( "The atom list contains %d spins which are already defined in project. "
			"Continue anyway?", doubles.size() );
		if( QMessageBox::warning( this, "Import Atom List", str, 
			"&Import", "&Cancel", QString::null, 1, 1 ) != 0 )		
			return;	// Cancel
	}
	doubles.clear();

	bool parseLabels = false;
	bool ignoreProjected = false;

	QApplication::restoreOverrideCursor();
	if( projectedSpin > 0 )
	{
		Dlg::StringList sl;
		sl.push_back( "Ignore label syntax" );
		sl.push_back( "Import all spins (local && projected)" );
		sl.push_back( "Don't import projected spins" );
		switch( Dlg::getOption( this, sl, "Import Atom List", 0 ) )
		{
		case -1:
			return; // Cancel
		case 0:
			break;
		case 1:
			parseLabels = true;
			break;
		case 2:
			parseLabels = true;
			ignoreProjected = true;
			break;
		}
		if( parseLabels && incompatibleSyntax > 0 )
		{
			str.sprintf( "The atom list contains %d spins with incompatible label syntax. "
				"Continue anyway (ignoring the incompatible ones)?", incompatibleSyntax );
			if( QMessageBox::warning( this, "Import Atom List", str, 
				"&Import", "&Cancel", QString::null, 1, 1 ) != 0 )		
				return;	// Cancel
		}
	}
	Viewport::pushHourglass();

	// Alles ok. Fge nun alles in scharfe DB.
	Spin* s;
	l = SpinLabel();
	l.setAssigned();
//	try
	{
		for( i = 0; i < pl->getCount(); i++ )
		{
			const EasyProtonList::Atom& a = pl->getAtom( i );
			if( a.isValid() && spin.count( a.d_spin ) > 0 )
			{
				ss = 0;
				if( a.d_sys > 0 )
				{
					if( fragId )
					{
						ss = b->getSystem( a.d_sys );
						if( ss == 0 )
							ss = b->addSystem( a.d_sys );
					}else
					{
						r = seq->getResidue( a.d_sys );
						assert( r );
						ss = r->getSystem();
						if( ss == 0 )
						{
							ss = b->addSystem( a.d_sys );
							b->assignSystem( ss, r );
							if( r->getPred() && r->getPred()->getSystem() )
								b->link( r->getPred()->getSystem(), ss );
							if( r->getSucc() && r->getSucc()->getSystem() )
								b->link( ss, r->getSucc()->getSystem() );
						}
					}
				}
				t = AtomType::parse( a.d_label.data(), a.d_shift );
				if( parseLabels )
				{
					bool res = SpinLabel::parse( a.d_label.data(), l );
					l.setAssigned();
					if( !res )
						continue; // Ignore incompatible syntax
					if( ignoreProjected && l.getOffset() != 0 )
						continue;

					s = b->addSpin( t, a.d_shift, 0, a.d_spin );
					if( !b->setLabel( s, l ) )
						doubles.insert( a.d_spin );
					else	if( ss )
					{
						if( !b->assignSpin( ss, s ) )
							doubles.insert( a.d_spin );
					}
				}else
				{
					l.setTag( a.d_label );
					s = b->addSpin( t, a.d_shift, 0, a.d_spin );
					b->setLabel( s, l );
					if( ss )
					{
						if( !b->assignSpin( ss, s ) )
							doubles.insert( a.d_spin );
					}
				}
			}
		}
	}/*catch( Root::Exception& e )
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::critical( this, "Import Atom List", 
			e.what(), "&Cancel" );
		return;
	}*/
	QApplication::restoreOverrideCursor();
	if( !doubles.empty() )
	{
		str = "The following spins could not be assigned to their spin systems:\n";
		for( p3 = doubles.begin(); p3 != doubles.end(); ++p3 )
		{
			str += QString().setNum( (*p3) );
			str += " ";
		}
		Root::MessageLog::inst()->warning( "Import Atom List", str );
		str.sprintf( "%d spins could not be assigned to their spin system "
			"due to label uniqueness violation. Check message log for details.", 
			doubles.size() );
		QMessageBox::warning( this, "Import Atom List", 
			str, "&OK" );
	}
}

void AidaCentralAgent::handleImportAtomList(Action & a)
{
	ACTION_ENABLED_IF( a, d_cur && d_cur->type() == _Project );
	// Nur erlaubt, wenn Projekt noch keine Spins enthlt.
	// Systeme knnen schon vorhanden sein wegen Sequence-Import.
	// Sie knnen aber noch keine Spins enthalten.

	Project* pro = d_parent->getRep()->findProject( d_menuTree->currentItem()->text(0).toLatin1() );
	assert( pro );

	if( !pro->getSpins()->getSpins().empty() )
	{
		if( QMessageBox::warning( this, "Import Atom List", 
			"The project already contains spins. "
			"Do you really wand to continue (cannot be undone)?", 
			"&OK", "&Cancel", QString::null, 1, 1 ) != 0 )		
			return;	// Cancel
	}

	QString fileName = QFileDialog::getOpenFileName( this, tr("Select Atom List"),
		Root::AppAgent::getCurrentDir(), "Atom List (*.prot)" );
	if( fileName.isNull() )
		return;

	QFileInfo info( fileName );
	Root::AppAgent::setCurrentDir( info.dirPath( true ) );

	bool fragId = true;
	switch( QMessageBox::information( this, "Import Atom List",
			  "Do the assignments given in the atom list reference "
			  "residues or spin systems? ",
			  "&Residues", "&Spin Systems", "&Cancel", 0, 2 ) )		
	{
	case 0: // Resi
		fragId = false;
		break;
	case 1:
		fragId = true;
		break;	// Sys
	default:
		return;	// Cancel
	}

	QApplication::setOverrideCursor( Qt::waitCursor );

	Root::Ref<EasyProtonList> pl;
	try
	{
		pl = new EasyProtonList( fileName.toLatin1().data() );
	}catch( Root::Exception& e )
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::critical( this, "Import Atom List", e.what(), "&Cancel" );
		return;
	}

	importAtomList( pro, pl, fragId );
}

void AidaCentralAgent::handleImportSeqVals(Action &)
{
	// TODO
}

void AidaCentralAgent::handleExportAtomList(Action & a)
{
	ACTION_ENABLED_IF( a, d_cur && d_cur->type() == _Project );

	Project* pro = d_parent->getRep()->findProject( d_menuTree->currentItem()->text(0).toLatin1() );
	assert( pro );
	SpectrumListView::exportAtomList( this, pro );
}

void AidaCentralAgent::handleExportSequence(Action & a)
{
	ACTION_ENABLED_IF( a, d_cur && d_cur->type() == _Project  );

	QString fileName = QFileDialog::getSaveFileName( this, tr( "Export Sequence File" ), 
            Root::AppAgent::getCurrentDir(), "Sequence File (*.seq)", 0, QFileDialog::DontConfirmOverwrite );
	if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );

	if( info.extension( false ).upper() != "SEQ" )
		fileName += ".seq";
	info.setFile( fileName );
	if( info.exists() )
	{
		if( QMessageBox::warning( this, "Save As",
			"This file already exists. Do you want to overwrite it?",
			"&OK", "&Cancel" ) != 0 )
			return;
	}

	Project* pro = d_parent->getRep()->findProject( d_menuTree->currentItem()->text(0).toLatin1() );
	assert( pro );
	Root::Ref<SequenceFile> sf = new SequenceFile();
	Sequence* s = pro->getSequence();
	Sequence::ResidueMap::const_iterator i;
	for( i = s->getResi().begin(); i != s->getResi().end(); ++i )
		sf->addResidue( (*i).second, 
			( (*i).second->getSystem() )?(*i).second->getSystem()->getId():0 );
	try
	{
		sf->writeToFile( fileName );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Export Sequence File", e.what(), "&Cancel" );
		return;
	}
}

void AidaCentralAgent::handleExportMapper(Action & a)
{
	ACTION_ENABLED_IF( a, d_cur && d_cur->type() == _Project );

	QString fileName = QFileDialog::getSaveFileName( this, tr( "Export Mapper File" ),
        Root::AppAgent::getCurrentDir(), "Mapper Input File (*.fra)", 0, QFileDialog::DontConfirmOverwrite );
	if( fileName.isNull() ) 
		return;

	QFileInfo info( fileName );

	if( info.extension( false ).upper() != "FRA" )
		fileName += ".fra";
	info.setFile( fileName );
	if( info.exists() )
	{
		if( QMessageBox::warning( this, "Save As",
			"This file already exists. Do you want to overwrite it?",
			"&OK", "&Cancel" ) != 0 )
			return;
	}
	Root::AppAgent::setCurrentDir( info.dirPath( true ) );

	Root::SymbolString CA = "CA";
	Root::SymbolString CB = "CB";
	if( !Dlg::getLabelPair( this, CA, CB ) )
		return;
	Project* pro = d_parent->getRep()->findProject( d_menuTree->currentItem()->text(0).toLatin1() );
	assert( pro );
	Root::Ref<Fragmenter> f = new Fragmenter( pro->getSpins() );
	f->rebuildAll();
	if( !f->writeMapperFile( fileName, 0, CA, CB ) )
		QMessageBox::critical( this, "Export Mapper File", 
			"Cannot write to selected file!", "&Cancel" );
}

void AidaCentralAgent::handleRemoveProject(Action & a)
{
	ACTION_ENABLED_IF( a, d_cur && d_cur->type() == _Project );

	if( QMessageBox::warning( this, "Remove Project",
		"Do you really want to remove the selected project (cannot be undone)?",
		"&OK", "&Cancel" ) != 0 )
		return;
	Project* pro = d_parent->getRep()->findProject( d_menuTree->currentItem()->text(0).toLatin1() );
	assert( pro );
	d_parent->getRep()->removeProject( pro );
	// Unntig, wird bereits mit Update-Handler ausgefhrt: delete p;
}

void AidaCentralAgent::handleRenameProject(Action & a)
{
	ACTION_ENABLED_IF( a, d_cur && d_cur->type() == _Project );

	Project* pro = d_parent->getRep()->findProject( d_menuTree->currentItem()->text(0).toLatin1() );
	assert( pro );
	bool ok	= FALSE;
	QString res = QInputDialog::getText( "Rename Project", "Please enter a unique name:", QLineEdit::Normal, 
		pro->getName().data(), &ok, this );
	if( !ok )
		return;
	if( !d_parent->getRep()->rename( pro, res ) )
		QMessageBox::critical( this, "Rename Project", 
			"Cannot rename project to the given name!", "&Cancel" );
}

void AidaCentralAgent::handleDuplicatePro(Action & a)
{
	ACTION_ENABLED_IF( a, d_cur && d_cur->type() == _Project );

	Project* pro = d_parent->getRep()->findProject( d_menuTree->currentItem()->text(0).toLatin1() );
	assert( pro );
	bool ok	= FALSE;
	QString res = QInputDialog::getText( "Duplicate Project", "Please enter a unique name:", QLineEdit::Normal, 
		pro->getName().data(), &ok, this );
	if( !ok )
		return;
	Repository* rep = d_parent->getRep();
	if( rep->getProjects().find( res.toLatin1() ) != rep->getProjects().end() )
	{
		QMessageBox::critical( this, "Duplicate Project", 
			"Cannot create project with the given name!", "&Cancel" );
		return;
	}

	Root::Ref<Project> pro2;
	try
	{
		pro2 = new Project( *pro, res.toLatin1() );
		rep->addProject( pro2 );
	}catch( Root::Exception& e )
	{
		QMessageBox::critical( this, "Duplicate Project", 
			e.what(), "&Cancel" );
	}
}

 
#define _Entry_title "Entry Title"
#define _BMRB_accession_number "BMRB Accession Number"
#define _BMRB_flat_file_name "BMRB Flat File Name"
#define _Submission_date "Submission Date"
#define _Accession_date "Accession Date"
#define _Citation_full "Citation Full"

void AidaCentralAgent::handleImportProject(Action & a)
{
	ACTION_ENABLED_IF( a, true );

	QString log;
	try
	{
		QString str;
		QString fileName = QFileDialog::getOpenFileName( this, tr("Select BMRB File"),
			Root::AppAgent::getCurrentDir(), "BMRB File (*.bmrb *.str)" );
		if( fileName.isNull() ) 
			return;

		QFileInfo info( fileName );
		QString name = info.baseName();
		Repository* rep = d_parent->getRep();
		while( true )
		{
			const Repository::ProjectMap& pm = rep->getProjects();
			if( pm.find( name.toLatin1() ) != pm.end() )
			{
				bool ok;
				QString res = QInputDialog::getText( "New Project", 
					"Name is not unique. Please enter a unique name:", QLineEdit::Normal, 
					name, &ok, this );
				if( !ok || res.isEmpty() )
					return;
				name = res;
			}else
				break;
		}

		Root::AppAgent::setCurrentDir( info.dirPath( true ) );
		QApplication::setOverrideCursor( Qt::waitCursor );

        str.sprintf( "Opening %s", fileName.toLatin1().data() );
		d_parent->setStatusMessage( str );

		StarFile sf( fileName );
		sf.parse();
		if( !sf.d_err.isEmpty() )
			throw Root::Exception( sf.d_err.data() );
		StarFile::Resi::const_iterator i;
		Root::Ref<SequenceFile> seq = new SequenceFile();
		for( i = sf.getSeq().begin(); i != sf.getSeq().end(); ++i )
		{
			// qDebug( "%d %s", (*i).first, (*i).second.data() );
			// out << (*i).first << " " << (*i).second.data() << std::endl;
			seq->addResidue( rep, (*i).first, (*i).second );
		}
		Root::Ref<Project> pro = new Project( rep, name.toLatin1(), seq, true );

		EasyProtonList::Atom ea;
		Root::Ref<EasyProtonList> pl = new EasyProtonList( sf.getCount() );
		StarFile::AtomSym* a = sf.getFirst();
		Root::Index j = 0;
		while( a )
		{
			// qDebug( "%d %d %s %f", a->d_resi, a->d_spin, a->d_label.data(), a->d_shift );
			//out << a->d_resi << " " << a->d_spin << " " << a->d_label.data() << " " <<
			//	a->d_shift << std::endl;
			ea.d_spin = a->d_spin;
			ea.d_sys = a->d_resi;
			ea.d_shift = a->d_shift;
			ea.d_label = a->d_label;
			pl->setAtom( j, ea );
			a = a->getNext();
			j++;
		}
		ObjectDef* od = rep->findObjectDef( Repository::keyProject );
		od->setField( _Entry_title, Root::Any::Memo, "" );
		od->setField( _BMRB_accession_number, Root::Any::CStr, "" );
		od->setField( _BMRB_flat_file_name, Root::Any::CStr, "" );
		od->setField( _Submission_date, Root::Any::CStr, "" );
		od->setField( _Accession_date, Root::Any::CStr, "" );
		od->setField( _Citation_full, Root::Any::Memo, "" );

		pro->setFieldValue( _Entry_title, sf.d_Entry_title.data() );
		pro->setFieldValue( _BMRB_accession_number, sf.d_BMRB_accession_number.data() );
		pro->setFieldValue( _BMRB_flat_file_name, sf.d_BMRB_flat_file_name.data() );
		pro->setFieldValue( _Submission_date, sf.d_Submission_date.data() );
		pro->setFieldValue( _Accession_date, sf.d_Accession_date.data() );
		pro->setFieldValue( _Citation_full, sf.d_Citation_full.data() );

		importAtomList( pro, pl, false );
		rep->addProject( pro );
		rep->touch(); // Da sonst nicht dirty
		QApplication::restoreOverrideCursor();
	}catch( Root::Exception& e )
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::critical( this, "Import Project", e.what(), "&Cancel" );
		return;
	}
	d_parent->setStatusMessage( "Done" );
	if( !log.isEmpty() )
		QMessageBox::critical( this, "Import Project",
				log, "&OK" );
}

void AidaCentralAgent::handleSetOrigin(Action & a)
{
	ACTION_ENABLED_IF( a, d_cur && d_cur->type() == _Project );

	Project* pro = d_parent->getRep()->findProject( d_menuTree->currentItem()->text(0).toLatin1() );
	assert( pro );
	Location loc;
	loc.d_pos = pro->getSpins()->getOrig();
	if( Dlg::getLocation( this, loc, "Set Location Origin", false ) )
		pro->getSpins()->setOrig( loc.d_pos );
}

