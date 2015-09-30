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

#include "TestScope.h"
#include <SpecView3/PlaneGrid.h>
#include <SpecView3/SpinPointList.h>
#include <Spec/Repository.h>
#include <Spec/SpecProjector.h>
#include <Gui2/AutoMenu.h>
#include <QDockWidget>
using namespace Spec;
using namespace Root;

TestScope::TestScope(Spectrum* spec, Project * pro):d_flag(false)
{
	d_pointMdl = new PointMdl();
	d_src4D = new SpinPointSpace( pro->getSpins(), pro->getRepository()->getTypes(), false,
								 true, true, false );
	d_rot4D = new SpecRotatedSpace( d_src4D );
	d_range4D = new RangeFilterSpaceND( d_rot4D, d_pointMdl, DimVector( DimX, DimY ), true );

	d_src4D->setSpec( spec );
	d_rot4D->setSpec( spec );

	QDockWidget* dock = new QDockWidget( tr("All Points"), this );
	dock->setAllowedAreas( Qt::AllDockWidgetAreas );
	dock->setFeatures( QDockWidget::AllDockWidgetFeatures );
	dock->setFloating( false );
	addDockWidget( Qt::RightDockWidgetArea, dock );
	SpinPointList* list = new SpinPointList( dock, false );
	list->setSpace( new LinkFilterSpace( d_rot4D ) );
	dock->setWidget( list );
	connect( list, SIGNAL(singleClicked(Spec::SpinSpace::Element)), this, SLOT(onPoint(Spec::SpinSpace::Element)) );

	if( spec->getDimCount() > 2 )
	{
		dock = new QDockWidget( tr("Projection"), this );
		dock->setAllowedAreas( Qt::AllDockWidgetAreas );
		dock->setFeatures( QDockWidget::AllDockWidgetFeatures );
		dock->setFloating( false );
		addDockWidget( Qt::RightDockWidgetArea, dock );
		list = new SpinPointList( dock );
		connect(list,SIGNAL(refilled(int)),this,SLOT(onRefilled(int)) );
		list->setSpace( d_range4D );
		dock->setWidget( list );
	}

	/*
	Ref<SpinPointSpace> space = new SpinPointSpace( pro->getSpins(),
								 pro->getRepository()->getTypes(), false,
								 true, true, false );
	space->setSpec( spec );
	Ref<SpecRotatedSpace> filter1 = new SpecRotatedSpace( space, spec );

	QDockWidget* dock = new QDockWidget( tr("All Points"), this );
	dock->setAllowedAreas( Qt::AllDockWidgetAreas );
	dock->setFeatures( QDockWidget::AllDockWidgetFeatures );
	dock->setFloating( false );
	addDockWidget( Qt::RightDockWidgetArea, dock );
	SpinPointList* list = new SpinPointList( dock );
	list->setSpace( filter1 );
	dock->setWidget( list );
	connect( list, SIGNAL(doubleClicked(Spec::SpinSpace::Element)), this, SLOT(onShow(Spec::SpinSpace::Element)) );

    QSplitter* split = new QSplitter( this );
    setCentralWidget( split );
    d_pointMdl = new PointMdl();
    d_cubeMdl = new CubeMdl();

    PlaneGrid* left = new PlaneGrid( this, DimX, DimY, d_pointMdl, d_cubeMdl );
    split->addWidget( left );
    if( spec->getDimCount() == 2 )
    {
        left->setSpectrum( spec );
        left->setPoints( filter1 );
    }else
    {
        left->setSpectrum( new SpecProjector( spec, d_pointMdl, DimX, DimY ) );
        left->setPoints( new RangeFilterSpaceND( filter1, d_pointMdl, // Funktioniert mit beliebigem DimCount
                                                 DimVector( spec->getDimCount() ).tail(DimZ), true ) );
        left->getPointView()->setColors( pro->getRepository()->getColors() );

        dock = new QDockWidget( tr("2D Projection"), this );
        dock->setAllowedAreas( Qt::AllDockWidgetAreas );
        dock->setFeatures( QDockWidget::AllDockWidgetFeatures );
        dock->setFloating( false );
        addDockWidget( Qt::RightDockWidgetArea, dock );
        list = new SpinPointList( dock );
        list->setSpace( left->getPoints() );
        dock->setWidget( list );
        connect( list, SIGNAL(doubleClicked(Spec::SpinSpace::Element)), this, SLOT(onShow(Spec::SpinSpace::Element)) );
        connect( list, SIGNAL(singleClicked(Spec::SpinSpace::Element)), this, SLOT(onPoint(Spec::SpinSpace::Element)) );
    }
    left->fitWindow();

    if( spec->getDimCount() == 4 )
    {
        PlaneGrid* right = new PlaneGrid( this, DimW, DimZ, d_pointMdl, d_cubeMdl );
        split->addWidget( right );
        right->setSpectrum( new SpecProjector( spec, d_pointMdl, DimW, DimZ ) );
        Ref<SpinSpace> filter2 = new RangeFilterSpaceND( filter1, d_pointMdl, DimVector( DimX, DimY ), true );
        right->setPoints( new RotatedSpace( filter2, Rotation( DimW, DimZ, DimX, DimY ) ) );
        right->fitWindow();
        right->getPointView()->setColors( pro->getRepository()->getColors() );

        dock = new QDockWidget( tr("4D Projection"), this );
        dock->setAllowedAreas( Qt::AllDockWidgetAreas );
        dock->setFeatures( QDockWidget::AllDockWidgetFeatures );
        dock->setFloating( false );
        addDockWidget( Qt::RightDockWidgetArea, dock );
        list = new SpinPointList( dock );
        list->setSpace( filter2 );
        dock->setWidget( list );
        connect( list, SIGNAL(doubleClicked(Spec::SpinSpace::Element)), this, SLOT(onShow(Spec::SpinSpace::Element)) );
        connect( list, SIGNAL(singleClicked(Spec::SpinSpace::Element)), this, SLOT(onPoint4D(Spec::SpinSpace::Element)) );
    }
	*/

//    Gui2::AutoMenu* pop = new Gui2::AutoMenu( p, true );
//    pop->addCommand( "Test1", this, SLOT(handleTest1()), tr("CTRL+1"), true );
//    pop->addCommand( "Test2", this, SLOT(handleTest2()), tr("CTRL+2"), true );
//    pop->addAutoCommand( "Test3", SLOT(handleRepaint()), tr("CTRL+3"), true );


    showMaximized();
}

void TestScope::handleTest1()
{
    ENABLED_IF( d_flag );

    qDebug() << "handleTest1";
}

void TestScope::handleTest2()
{
    CHECKED_IF( true, d_flag );

    d_flag = !d_flag;
    qDebug() << "handleTest2";
}

void TestScope::onShow(Spec::SpinSpace::Element e)
{
    PeakPos p;
    for( Dimension d = 0; d < SpinPoint::MAX_DIM; d++ )
    {
        if( e.d_point[d] != 0 )
            p[d] = e.d_point[d]->getShift();
        else
            break;
    }
    d_pointMdl->setPos( p );
}

void TestScope::onPoint(SpinSpace::Element e)
{
    if( e.d_point[DimX] && e.d_point[DimY] )
        d_pointMdl->setPos( DimX, e.d_point[DimX]->getShift(), DimY, e.d_point[DimY]->getShift() );
	d_range4D->setSys( e.d_point[DimX]->getSystem() );
}

void TestScope::onPoint4D(SpinSpace::Element e)
{
    if( e.d_point[DimW] && e.d_point[DimZ] )
		d_pointMdl->setPos( DimW, e.d_point[DimW]->getShift(), DimZ, e.d_point[DimZ]->getShift() );
}

void TestScope::onRefilled(int count)
{
	setStatusMessage( tr("%1 elements found").arg( count ) );
}
