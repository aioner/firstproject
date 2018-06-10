#ifndef NDEBUG			
	#include <stdlib.h>	
	#include <crtdbg.h>	
	#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif