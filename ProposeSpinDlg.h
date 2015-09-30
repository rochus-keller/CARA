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

#ifndef _Cara_ProposeSpinDlg
#define _Cara_ProposeSpinDlg

#include <QDialog>
#include <Spec/Project.h>
#include <Gui/ListView.h>

class QComboBox;

class ProposeSpinDlg : public QDialog
{
	Q_OBJECT
public:
	ProposeSpinDlg( QWidget* parent, Spec::Project*, Spec::AtomType, Spec::PPM,
		Spec::Spectrum* = 0,const QString& title = "" );
	bool exec();
	Spec::Spin* getSpin() const;
	Spec::PPM getShift() const { return d_shift; }
	Spec::AtomType getAtom() const { return d_atom; }
	Spec::Spectrum* getSpec() const { return d_spec; }
	Spec::Project* getPro() const { return d_pro; }
	Gui::ListView* getList() const { return d_lv; }
	void setAnchor( Spec::Dimension, Spec::Spin* );
	Spec::Spin* getAnchor(Spec::Dimension d) const { return d_spins[d]; }
	void enableOkHook() { d_okHook = true; }
private slots:
	void currentIndexChanged( int );
private:
	void runSpinFilter( bool okHook );

	Root::Ref<Spec::Project> d_pro;
	Root::Ref<Spec::Spectrum> d_spec;
	Spec::AtomType d_atom;
	Spec::PPM d_shift;
	Spec::SpinPoint d_spins;
	Gui::ListView* d_lv;
	QComboBox* d_cbo;
	bool d_okHook;
};

#endif
