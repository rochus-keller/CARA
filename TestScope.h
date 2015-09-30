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

#ifndef _TestScope_
#define _TestScope_

#include <SpecView3/GenericScope.h>
#include <Spec/Spectrum.h>
#include <Spec/Project.h>
#include <Spec/SpinSpace.h>
#include <Spec/PointMdl.h>
#include <Spec/CubeMdl.h>

namespace Spec
{
	class SpinPointSpace;
	class RangeFilterSpaceND;
}
class TestScope : public Spec::GenericScope
{
    Q_OBJECT
public:
    TestScope(Spec::Spectrum*, Spec::Project* pro);
protected slots:
    void handleTest1();
    void handleTest2();
    void onShow( Spec::SpinSpace::Element );
    void onPoint( Spec::SpinSpace::Element );
    void onPoint4D( Spec::SpinSpace::Element );
	void onRefilled(int count );
private:
    Root::Ref<Spec::PointMdl> d_pointMdl;
    Root::Ref<Spec::CubeMdl> d_cubeMdl;
	Spec::SpinPointSpace* d_src4D;
	Spec::SpecRotatedSpace* d_rot4D;
	Spec::RangeFilterSpaceND* d_range4D;
    bool d_flag;
};

#endif
