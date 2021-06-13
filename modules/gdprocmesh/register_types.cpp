/*************************************************************************/
/*  register_types.cpp                                                   */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "register_types.h"

#include "src/gdprocmesh.h"
#include "src/gdprocnode.h"

#include "src/input/gdprocincurve.h"
#include "src/input/gdprocinint.h"
#include "src/input/gdprocinmesh.h"
#include "src/input/gdprocinpoolvectors.h"
#include "src/input/gdprocinreal.h"
#include "src/input/gdprocinvector.h"

#include "src/primitives/gdproccount.h"
#include "src/primitives/gdproceuler.h"
#include "src/primitives/gdprocexec.h"
#include "src/primitives/gdprocrandom.h"
#include "src/primitives/gdprocsplitvector.h"
#include "src/primitives/gdprocvector.h"

#include "src/transforms/gdprocadd.h"
#include "src/transforms/gdprocbevel.h"
#include "src/transforms/gdprocdiv.h"
#include "src/transforms/gdprocgennormals.h"
#include "src/transforms/gdprocmult.h"
#include "src/transforms/gdprocredist.h"
#include "src/transforms/gdprocrotate.h"
#include "src/transforms/gdprocrotmult.h"
#include "src/transforms/gdprocscale.h"
#include "src/transforms/gdprocsub.h"
#include "src/transforms/gdproctranslate.h"

#include "src/shapes/gdprocbox.h"
#include "src/shapes/gdproccircle.h"
#include "src/shapes/gdprocline.h"
#include "src/shapes/gdprocrect.h"

#include "src/surfaces/gdprocextrudeshape.h"
#include "src/surfaces/gdprocsimplify.h"
#include "src/surfaces/gdprocsurface.h"

#include "src/modifiers/gdprocmerge.h"
#include "src/modifiers/gdprocmirror.h"
#include "src/modifiers/gdprocplaceonpath.h"
#include "src/modifiers/gdproctransform.h"

#include "src/output/gdprocoutput.h"

void register_gdprocmesh_types() { // register our procedural mesh class
	ClassDB::register_class<godot::GDProcMesh>();

	// register all our nodes
	ClassDB::register_class<godot::GDProcNode>();

	// inputs
	ClassDB::register_class<godot::GDProcInCurve>();
	ClassDB::register_class<godot::GDProcInInt>();
	ClassDB::register_class<godot::GDProcInMesh>();
	ClassDB::register_class<godot::GDProcInPoolVectors>();
	ClassDB::register_class<godot::GDProcInReal>();
	ClassDB::register_class<godot::GDProcInVector>();

	// primitives
	ClassDB::register_class<godot::GDProcCount>();
	ClassDB::register_class<godot::GDProcEuler>();
	ClassDB::register_class<godot::GDProcRandom>();
	ClassDB::register_class<godot::GDProcVector>();
	ClassDB::register_class<godot::GDProcExec>();
	ClassDB::register_class<godot::GDProcSplitVector>();

	// transforms (work on primitives)
	ClassDB::register_class<godot::GDProcAdd>();
	ClassDB::register_class<godot::GDProcBevel>();
	ClassDB::register_class<godot::GDProcDiv>();
	ClassDB::register_class<godot::GDProcGenNormals>();
	ClassDB::register_class<godot::GDProcMult>();
	ClassDB::register_class<godot::GDProcRedist>();
	ClassDB::register_class<godot::GDProcRotate>();
	ClassDB::register_class<godot::GDProcRotMult>();
	ClassDB::register_class<godot::GDProcScale>();
	ClassDB::register_class<godot::GDProcSub>();
	ClassDB::register_class<godot::GDProcTranslate>();

	// shapes
	ClassDB::register_class<godot::GDProcBox>();
	ClassDB::register_class<godot::GDProcCircle>();
	ClassDB::register_class<godot::GDProcLine>();
	ClassDB::register_class<godot::GDProcRect>();

	// surfaces
	ClassDB::register_class<godot::GDProcExtrudeShape>();
	ClassDB::register_class<godot::GDProcSimplify>();
	ClassDB::register_class<godot::GDProcSurface>();

	// modifiers (work on surfaces)
	ClassDB::register_class<godot::GDProcMerge>();
	ClassDB::register_class<godot::GDProcMirror>();
	ClassDB::register_class<godot::GDProcPlaceOnPath>();
	ClassDB::register_class<godot::GDProcTransform>();

	// output
	ClassDB::register_class<godot::GDProcOutput>();
}

void unregister_gdprocmesh_types() {
}
