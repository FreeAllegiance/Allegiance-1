#ifndef _zlib_H_
#define _zlib_H_

//////////////////////////////////////////////////////////////////////////////
//
// zlib.H
//
//////////////////////////////////////////////////////////////////////////////

// warning C4355: 'this' : used in base member initializer list
#pragma warning(disable:4355)

#if !defined (_WIN64) 
	#define FLOATASM //Imago guarded from x64 compilers on 6/20/09
	#define USEASM
#endif

//
// Required CRT headers
//

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "math.h"
#include "time.h"
#include "malloc.h"

//
// Required Win32 headers
//

#include "windows.h"
#include "wininet.h"

//
// DirectX Stuff
//

// KGJV - Removed dependancy from DX
#include "OldDXDefns.h"	// Older DX definitions included to get the project to build. At some point,
						// they all become redundant.

//
// Library Headers
//

#include "zassert.h"
#include "alloc.h"
#include "mask.h"
#include "tref.h"
#include "cast.h"
#include "zmath.h"
#include "zadapt.h"

#include "tcompare.h"
#include "tarray.h"
#include "tvector.h"
#include "tstack.h"
#include "tlist.h"
#include "tmap.h"

//
// Types
//

#include "ztime.h"
#include "zstring.h"
#include "point.h"
#include "rect.h"
#include "vector.h"
#include "vertex.h"
#include "orientation.h"
#include "matrix.h"
#include "quaternion.h"
#include "color.h"


//
// Interfaces
//

#include "event.h"
#include "input.h"
#include "genericlist.h"
#include "FTPSession.h"

//
// Win32 Wrapper Classes
//

#include "base.h"
#include "window.h"
#include "win32app.h"
#include "VersionInfo.h"

//
// Parser
//

#include "token.h"

#endif
