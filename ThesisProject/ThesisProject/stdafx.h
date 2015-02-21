// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include<stdio.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <gl\GL.h>
#include <gl\GLU.h>

#include "opencv2\highgui\highgui.hpp"
#include "opencv2\core\opengl_interop.hpp"
#include "opencv2\core\opengl_interop_deprecated.hpp"
#include "opencv2\core\core.hpp"
#include "opencv2\imgproc\imgproc.hpp"
#include "opencv2\features2d\features2d.hpp"
#include "opencv2\legacy\compat.hpp"

#include "cudnn.h"


#include<iostream>
#include<string>


using namespace cv;
using namespace std;


// TODO: reference additional headers your program requires here
