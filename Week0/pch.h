#ifndef PCH_H
#define PCH_H

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <set>
#include <unordered_map>


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

