#ifndef PCH_H
#define PCH_H

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <queue>
#include <set>
#include <unordered_map>
#include <algorithm>


#include <d3d11.h>
#include <d3dcompiler.h>

#include <DirectXMath.h>
#include <cmath>

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

#include "WICTextureLoader.h"
//#include "TimeManager.h"
//#include "SceneManager.h"

#include "Define.h"

#define		SAFE_DELETE(p)		 if(p) { delete p; p = nullptr;}
#define		SAFE_DELETE_ARRAY(p) if(p) { delete [] p; p = nullptr;}

#ifdef _DEBUG

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#ifndef DBG_NEW 
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ ) 
#define new DBG_NEW 
#endif

#endif

#endif

