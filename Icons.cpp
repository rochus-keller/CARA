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

#include "Icons.h"
#include <qpixmap.h>

static const char * x_repository []=
{
    "16 16 7 1",
    "# c #000000",
    "b c #ffffff",
    "e c #000000",
    "d c #404000",
    "c c #c0c000",
    "a c #ffffc0",
    ". c None",
    "................",
    ".........#......",
    "......#.#a##....",
    ".....#b#bbba##..",
    "....#b#bbbabbb#.",
    "...#b#bba##bb#..",
    "..#b#abb#bb##...",
    ".#a#aab#bbbab##.",
    "#a#aaa#bcbbbbbb#",
    "#ccdc#bcbbcbbb#.",
    ".##c#bcbbcabb#..",
    "...#acbacbbbe...",
    "..#aaaacaba#....",
    "...##aaaaa#.....",
    ".....##aa#......",
    ".......##......."
};

QPixmap* Icons::s_repository = 0;

void Icons::init()
{
	if( s_repository )
		return;

	// Pix können nicht statisch alloziiert werden, da Qt noch nicht initialisiert ist.
	s_repository = new QPixmap( x_repository );

}
