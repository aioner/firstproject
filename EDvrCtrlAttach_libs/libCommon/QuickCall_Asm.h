
void __inline qmemset(void * dst, int val, size_t count)
{
    __asm
    {
        cld                 ; clear the direction flag
        mov     edi, dst    ; move pointer into edi
        mov     ecx, count  ; ecx hold loop count
        mov     eax, val    ; eax hold value
        rep     stosd       ; preform fill
    }
}


__int64 __inline GetTimeR()
{
    LARGE_INTEGER lFreq;
    LARGE_INTEGER lTimeCount;
    QueryPerformanceCounter(&lTimeCount);
    QueryPerformanceFrequency(&lFreq);
    return lTimeCount.QuadPart * 1000000 / lFreq.QuadPart;
}

//void *sse_memcpy_64(void *to, const void *from, size_t len)
//{
//    void *const save = to;
//
//    __asm __volatile
//    {
//        prefetchnta (%0)
//        prefetchnta 64(%0)
//        prefetchnta 128(%0)
//        prefetchnta 192(%0)
//        prefetchnta 256(%0)
//        :: "r" (from)
//    }
//
//    if (len >= MIN_LEN) {
//        register int i;
//        register int j;
//        register unsigned int delta;
//
//        delta = ((unsigned int)to)&(SSE_MMREG_SIZE-1);
//        if (delta) {
//            delta=SSE_MMREG_SIZE-delta;
//            len -= delta;
//            small_memcpy(to, from, delta);
//        }
//        j = len >> 6;
//        len &= 63;
//
//        for(i=0; i<j; i++) {
//            __asm __volatile__ {
//                "prefetchnta 320(%0)\n"
//                "movups (%0), %%xmm0\n"
//                "movups 16(%0), %%xmm1\n"
//                "movups 32(%0), %%xmm2\n"
//                "movups 48(%0), %%xmm3\n"
//                "movntps %%xmm0, (%1)\n"
//                "movntps %%xmm1, 16(%1)\n"
//                "movntps %%xmm2, 32(%1)\n"
//                "movntps %%xmm3, 48(%1)\n"
//                ::"r" (from), "r" (to) : "memory"
//            }
//            from+=64;
//            to+=64;
//        }
//        __asm__ __volatile sfence ::: memory
//    }
//    if (len != 0)
//        __memcpy(to, from, len);
//    return save;
//}

