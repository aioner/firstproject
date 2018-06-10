#ifndef _NOVTABLE_H_INCLUDED
#define _NOVTABLE_H_INCLUDED

#ifdef _WIN32
    #define MEDIA_CLIENT_NO_VTABLE  __declspec(novtable)
#else
    #define MEDIA_CLIENT_NO_VTABLE
#endif

#endif //_NOVTABLE_H_INCLUDED
