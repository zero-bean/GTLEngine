#ifndef PCH_H
#define PCH_H

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <set>
#include <unordered_map>



#include <d3d11.h>
#include <d3dcompiler.h>

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

struct FVector3
{
    FVector3(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}
    float x, y, z;
};

struct FConstants
{
    FVector3 WorldPosition;
    float Scale;
};  

struct FVertexSimple
{
    float x, y, z;
    float r, g, b, a;
};


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

