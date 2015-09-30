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

#ifndef _LuaCaraExplorer_
#define _LuaCaraExplorer_

class AidaCentralAgent;

namespace Spec
{
	class Spectrum;
	class SpinSystem;
	class Spin;
	class SpinSystem;
	class SpectrumType;
	class ResidueType;
	class SystemType;
	class Project;
	class Residue;
	class BioSample;
	class PeakList;
	class SpinLink;
}

struct LuaCaraExplorer
{
	static void install();
	static void installExplorer(  AidaCentralAgent* );
	static void setCurrentSpec( Spec::Spectrum* );
	static void setCurrentSpin( Spec::Spin* );
	static void setCurrentSystem( Spec::SpinSystem* );
	static void setCurrentSpectrumType( Spec::SpectrumType* );
	static void setCurrentResidueType( Spec::ResidueType* );
	static void setCurrentSystemType( Spec::SystemType* );
	static void setCurrentProject( Spec::Project* );
	static void setCurrentResidue( Spec::Residue* );
	static void setCurrentSample( Spec::BioSample* );
	static void setCurrentPeakList( Spec::PeakList* );
	static void setCurrentSpinLink( Spec::SpinLink* );
};

#endif
