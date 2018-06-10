#include "rvehd.h"
#include "rvassert.h"
#include "rvares.h"


/* Support for 'hosts' file
 * On startup, whole content of this file is read and parsed into internal cache and further
 * requests are served from this cache. Modification date of original file is checked and 
 * cache is rebuilt if needed.
 */

#if RV_DNS_USES_HOSTS && !RV_DNS_USES_CACHING

#if __GNUC__

#warning "RV_DNS_USES_HOSTS requires RV_DNS_USES_CACHING, compiling with RV_DNS_USES_HOSTS == 0 "

#else

#pragma message(RvWarning "RV_DNS_USES_HOSTS requires RV_DNS_USES_CACHING, compiling with RV_DNS_USES_HOSTS == 0")

#undef RV_DNS_USES_HOSTS
#define RV_DNS_USES_HOSTS 0

#endif



#endif

#if RV_NET_TYPE != RV_NET_NONE

#if !(RV_DNS_USES_HOSTS)

/* Stub functions in the case that we're compiled without 'hosts' file reolution
 * support
 */

RvStatus RvEHDNew(RvEHD **self, RvLogMgr *logMgr) {
    RV_UNUSED_ARG(logMgr);
    *self = 0;
    return RV_OK;
}

RvStatus RvEHDDelete(RvEHD *self) {
    RV_UNUSED_ARG(self);
    return RV_OK;
}

/* RvEHDGetSearchOrder() 
 *
 * Returns hosts file search order:
 * RV_EHD_SEARCH_HOSTS_FIRST - search hosts file before DNS servers
 * RV_EHD_SEARCH_HOSTS_LAST  - search hosts file after  DNS servers
 */
RvInt RvEHDGetSearchOrder() {
    return RV_EHD_SEARCH_HOSTS_FIRST;
}

RvStatus RvEHDFind(RvEHD *self, RvChar *name, RvUint16 qtype, RvUint8 *pBuf, RvSize_t *pSize, RvInt *pNfound) {
    (void)((void)self, (void)name, (void)qtype, (void)pBuf, (void)pSize, (void)pNfound);
    return RV_DNS_ERROR_CACHE_NOTFOUND;
}


#else

#include "rvtypes.h"
#include "rvaddress.h"
#include "rvmemory.h"
#include "rvstdio.h"
#include "rvsocket.h"
#include <stdlib.h>
#include <ctype.h>

#ifndef RvStrcasecmp
#  if RV_OS_TYPE == RV_OS_TYPE_WIN32
#    define RvStrcasecmp stricmp
#  else
#    define RvStrcasecmp strcasecmp
#  endif
#endif

#define IPV6_NEEDED ((RV_NET_TYPE) & RV_NET_IPV6)
static RvInt gIPv6Needed = IPV6_NEEDED;

/* RV_EHD_HOSTFILE may be defined to use host file different from platform default */
#ifndef RV_EHD_HOSTFILE
#define RV_EHD_HOSTFILE 0
#endif

/* RV_EHD_SEARCH_ORDER >= 0 forces specific hosts order, host.conf file isn't consulted */
#ifndef RV_EHD_SEARCH_ORDER
#define RV_EHD_SEARCH_ORDER 0
#endif

/* Used to avoid annoying compiler warnings in conditionals */
static int sRvEhdSearchOrder = RV_EHD_SEARCH_ORDER;

/* Search order in the case that no host.conf file was found, or no 'order' line found in the 
 * host.conf file
 */
#define RV_EHD_DEFAULT_SEARCH_ORDER RV_EHD_SEARCH_HOSTS_LAST 

#ifndef RV_CCORE_MODULE_EHD
#define RV_CCORE_MODULE_EHD    24        /* /etc/hosts file based resolver */
#endif

#ifndef RV_CCORE_MODULE_LOCAL
#define RV_CCORE_MODULE_LOCAL  1023      /* Error codes local for specific module - should be used between module internal functions */
#endif


#ifndef RvLocalErrorCode
#define RvLocalErrorCode(e) RvErrorCode(RV_ERROR_LIBCODE_CCORE, RV_CCORE_MODULE_LOCAL, (e))
#endif
#define RvEhdErrorCode(e) RvErrorCode(RV_ERROR_LIBCODE_CCORE, RV_CCORE_MODULE_EHD, (e))

#define RV_EH_ERROR_FORMAT RvEhdErrorCode(-513)
#define RV_LOCAL_ERROR_EOF   RvLocalErrorCode(-513)
#define RV_LOCAL_ERROR_MALFORMED RvLocalErrorCode(-514)  /* malformed message accepted */
#define RV_LOCAL_ERROR_RECTYPE RvLocalErrorCode(-515)    /* unknown record type -shouldn't happen */
#define RV_LOCAL_ERROR_FILE    RvLocalErrorCode(-516)
#define RV_LOCAL_ERROR_NAMESIZE RvLocalErrorCode(-517)   /* name is too long */

#define DEFAULT_PAGE_SIZE 4000
#define DEFAULT_HASH_SIZE 256
#define MAX_FILENAME_SIZE 256
#define MAX_DNSNAME_SIZE  256

/* From RFC-1035 */
typedef enum {
    RV_A     = 1,
    RV_CNAME = 5
} RvEHRecordType;

typedef struct _RvAddressList {
    struct _RvAddressList *next;
    RvAddress addr;
} RvAddressList;

typedef struct _RvEHBaseRecord RvEHBaseRecord;

/* Abstract base class for records that are kept in EHDMap DB 
 * Instances of this class are never created, but only instances
 * of subclasses 
 */
struct _RvEHBaseRecord {
    RvEHBaseRecord *next;   /* points to the next record in the hash chain */
    RvChar         *name;   /* serves as key in the map */
    RvEHRecordType  type;   /* type, currently RV_A (for A and AAAA records) or RV_CNAME (alias) */
};

typedef struct _REHCnameRecord RvEHCnameRecord;

/* A record */
typedef struct {
    RvEHBaseRecord   base;         
    RvAddressList    firstAddr;
    RvEHCnameRecord *firstAlias;
} RvEHARecord;

/* CName record */
struct _REHCnameRecord {
    RvEHBaseRecord   base;
    RvEHCnameRecord *nextAlias;
    RvEHARecord     *arec;
};


/*===================  Memory management routines =================== */

/* Memory allocation pattern for EHD module (allocations are made during parsing 
* of 'hosts' file, all memory is reclaimed on EHDMap object destruction) enables us to use
* simple and efficient allocation scheme: memory is allocated in big fixed size chunks 
* that are kept on linked list, smaller allocations are done from these chunks. 
* There is no bookkeping overhead for this smaller allocations. Deallocations are 
* made by simply traversing this list and freeing big chunks.
*/


#define ALIGN_STRUCT(name) \
    typedef struct {  \
    char a;          \
    name b;          \
} ALIGN_##name;      \
const RvUint32 name##_ALIGNMENT = RV_OFFSETOF(ALIGN_##name, b)

/*lint -save -e413 */
/* Deliberate use of null pointer */
ALIGN_STRUCT(RvEHARecord);
ALIGN_STRUCT(RvEHCnameRecord);
ALIGN_STRUCT(RvAddressList);
/*line -restore */


typedef struct _RvEHMemBuf {
    struct _RvEHMemBuf *next;
    union {
        RvChar      buf[1];
        RvUint64    a1;
        void        *a2;
        void        (*a3)();
    } u;
} RvEHMemBuf;

#define RV_EHMEMBUF_OVERHEAD RV_OFFSETOF(RvEHMemBuf, u)


typedef struct {
    RvSize_t    size;
    RvMemory   *mem;
    RvEHMemBuf *curBuf;
    RvChar     *curLoc;
    RvLogMgr   *logMgr;
} RvEHAllocator;


#define RV_EHALLOC_BUF(pBuf) RvMemoryAlloc(self->mem, self->size + RV_EHMEMBUF_OVERHEAD, self->logMgr, (void **)&pBuf)

/* Actually all structures will be aligned the same way, so there is no reason for AllocType macro */
#define RvEHAllocType(allocator, type, ptr) RvEHAllocatorAllocAligned(allocator, type##_ALIGNMENT, sizeof(type), (void **)&ptr)
#define RvEHAllocString(allocator, size, ptr) RvEHAllocatorAllocAligned(allocator, 1, size, (void **)&ptr)

static
RvStatus RvEHAllocatorConstruct(RvEHAllocator *self, RvMemory *mem, RvSize_t size, RvLogMgr *logMgr) {
    RvStatus s;

    self->size = size;
    self->mem  = mem;
    self->logMgr = logMgr;
    s = RV_EHALLOC_BUF(self->curBuf);
    if(s != RV_OK) {
        return s;
    }
    self->curLoc = self->curBuf->u.buf;
    self->curBuf->next = 0;
    return RV_OK;
}

static
RvStatus RvEHAllocatorDestruct(RvEHAllocator *self) {
    RvEHMemBuf *cur, *next;

    for(cur = self->curBuf; cur; cur = next) {
        next = cur->next;
        RvMemoryFree(cur, self->logMgr);
    }

    return RV_OK;
}

static
RvStatus RvEHAllocatorAllocAligned(RvEHAllocator *self, RvUint32 alignment, RvSize_t size, void **p) {
    RvChar *curLoc = self->curLoc;
    RvChar *lastLoc;
    RvStatus s = RV_OK;

    if(size > self->size) {
        return RV_ERROR_INSUFFICIENT_BUFFER;
    }

    curLoc = RvAlignTo(curLoc, alignment);

    lastLoc = curLoc + size;
    /* There is not enough space in current buffer for this allocation, allocate new buffer */
    if(lastLoc > self->curBuf->u.buf + self->size) {
        RvEHMemBuf *mbuf;
        s = RV_EHALLOC_BUF(mbuf);
        if(s != RV_OK) {
            return s;
        }
        curLoc = mbuf->u.buf;
        mbuf->next = self->curBuf;
        self->curBuf = mbuf;
    }

    *p = (void *)curLoc;
    self->curLoc = curLoc + size;
    return s;
}

/*======================== End of memory management =============================*/

typedef struct {
    RvEHBaseRecord **hash;
    RvSize_t         sizeMask; /* sizeof(hash) - 1 */
    RvEHAllocator    alloc;
} RvEHMap;

struct _RvEHD {
    RvMemory *mem;
    RvLogSource *pLogSource;
	RvLogSource logSource;
    RvLogMgr   *logMgr;
    RvEHMap  db;
    RvChar   etcFileName[MAX_FILENAME_SIZE];
    RvInt32  lastModificationTime;
};

#define EHD_LOG_INFO_0(f)                RvLogInfo(self->pLogSource, (self->pLogSource, f))
#define EHD_LOG_INFO_1(f, p1)            RvLogInfo(self->pLogSource, (self->pLogSource, f, p1))
#define EHD_LOG_INFO_2(f, p1, p2)        RvLogInfo(self->pLogSource, (self->pLogSource, f, p1, p2))
#define EHD_LOG_INFO_3(f, p1, p2, p3)    RvLogInfo(self->pLogSource, (self->pLogSource, f, p1, p2, p3))
#define EHD_LOG_DEBUG_0(f)                RvLogDebug(self->pLogSource, (self->pLogSource, FUNCTION_NAME ": " f))
#define EHD_LOG_DEBUG_1(f, p1)            RvLogDebug(self->pLogSource, (self->pLogSource, FUNCTION_NAME ": " f, p1))
#define EHD_LOG_DEBUG_2(f, p1, p2)        RvLogDebug(self->pLogSource, (self->pLogSource, FUNCTION_NAME ": " f, p1, p2))
#define EHD_LOG_DEBUG_3(f, p1, p2, p3)    RvLogDebug(self->pLogSource, (self->pLogSource, FUNCTION_NAME ": " f, p1, p2, p3))

#define EHD_LOG_ENTER_0(f)                RvLogEnter(self->pLogSource, (self->pLogSource, FUNCTION_NAME ": " f))
#define EHD_LOG_ENTER_1(f, p1)            RvLogEnter(self->pLogSource, (self->pLogSource, FUNCTION_NAME ": " f, p1))
#define EHD_LOG_ENTER_2(f, p1, p2)        RvLogEnter(self->pLogSource, (self->pLogSource, FUNCTION_NAME ": " f, p1, p2))
#define EHD_LOG_ENTER_3(f, p1, p2, p3)    RvLogEnter(self->pLogSource, (self->pLogSource, FUNCTION_NAME ": " f, p1, p2, p3))
#define EHD_LOG_ENTER                     EHD_LOG_ENTER_0("")                   


#define EHD_LOG_LEAVE_0(f)                RvLogLeave(self->pLogSource, (self->pLogSource, FUNCTION_NAME ": " f))
#define EHD_LOG_LEAVE_1(f, p1)            RvLogLeave(self->pLogSource, (self->pLogSource, FUNCTION_NAME ": " f, p1))
#define EHD_LOG_LEAVE_2(f, p1, p2)        RvLogLeave(self->pLogSource, (self->pLogSource, FUNCTION_NAME ": " f, p1, p2))
#define EHD_LOG_LEAVE_3(f, p1, p2, p3)    RvLogLeave(self->pLogSource, (self->pLogSource, FUNCTION_NAME ": " f, p1, p2, p3))
#define EHD_LOG_LEAVE                     EHD_LOG_LEAVE_0("")


#define EHD_LOG_ERROR_0(f)                RvLogError(self->pLogSource, (self->pLogSource, FUNCTION_NAME ": " f))
#define EHD_LOG_ERROR_1(f, p1)            RvLogError(self->pLogSource, (self->pLogSource, FUNCTION_NAME ": " f, p1))
#define EHD_LOG_ERROR_2(f, p1, p2)        RvLogError(self->pLogSource, (self->pLogSource, FUNCTION_NAME ": " f, p1, p2))
#define EHD_LOG_ERROR_3(f, p1, p2, p3)    RvLogError(self->pLogSource, (self->pLogSource, FUNCTION_NAME ": " f, p1, p2, p3))



static RvUint32 hash(RvChar *key) {
  RvUint32 result = 5381;

  while(*key) {
    result = (result << 5) + result;
    result ^= (RvUint32) tolower(*key);
    ++key;
  }

  return result;
}

#define RvEHMapGetMemory(self) ((self)->alloc.mem)
#define RvEHMapGetLog(self)    ((self)->alloc.logMgr)

/*
 * RvBool RvEHMapFind(RvEHMap *self, RvChar *key, RvEHBaseRecord ***prec)
 * 
 * Searches for 'key' entry in the map given by 'self'
 * Arguments:
 *  self - (in) pointer to map
 *  key  - (in) key to find
 *  prec - (out) if entry given by 'key' exists in the map 'self' **prec points to this entry
 *               otherwise, (*prec) points points to the pointer that should be changed in order 
 *               to add new entry to the map
 *
 *
 * Returns:
 *  RV_TRUE - if found, 
 *  RV_FALSE - otherwise
 *
 */

static 
RvBool RvEHMapFind(RvEHMap *self, RvChar *key, RvEHBaseRecord ***prec) {
    RvUint32 h;
    RvEHBaseRecord **pcur;

    h = hash(key) & self->sizeMask;

    for(pcur = &self->hash[h]; *pcur; pcur = &(*pcur)->next) {
        if(!RvStrcasecmp((*pcur)->name, key)) {
            *prec = pcur;
            return RV_TRUE;
        }

    }

    *prec = pcur;
    return RV_FALSE;
}

static 
RvStatus RvEHMapConstruct(RvEHMap *self, RvMemory *pmem, RvUint32 hSize, RvUint32 pageSize, RvLogMgr *logMgr) {
    RvUint32 hashSize;
    RvUint32 byteHashSize;
    RvStatus s = RV_OK;

    hashSize = 1;

    /* Find 2^n, such that hsize <= 2^(n) && hsize > 2^(n-1) */
    while(hashSize < hSize) {
        hashSize <<= 1;
    }

    byteHashSize = hashSize * sizeof(self->hash[0]);

    s = RvMemoryAlloc(pmem, byteHashSize, logMgr, (void **)&self->hash);
    if(s != RV_OK) {
        return s;
    }

    memset(self->hash, 0, byteHashSize);
    self->sizeMask = hashSize - 1;
    if(pageSize == 0) {
        pageSize = DEFAULT_PAGE_SIZE;
    }

    s = RvEHAllocatorConstruct(&self->alloc, pmem, pageSize, logMgr);
    if(s != RV_OK) {
        RvMemoryFree(self->hash, logMgr);
    }
    return s;
}

static 
RvStatus RvEHMapDestruct(RvEHMap *self) {
    RvMemoryFree(self->hash, self->alloc.logMgr);
    RvEHAllocatorDestruct(&self->alloc);
    return RV_OK;
}


static 
void RvAddressConstructT(RvAddress *paddr, RvInt32 addrType, void *addrData) {
    if(addrType == RV_ADDRESS_TYPE_IPV4) {
        RvAddressConstructIpv4(paddr, *(RvUint32 *)addrData, 0);
    } else {
        (void)RvAddressConstructIpv6(paddr, (RvUint8 *)addrData, 0, 0);
    }
}

#define IS_BLANK(ch) ((ch) == ' ' || (ch) == '\t')
#define SKIP_BLANKS(p) while(IS_BLANK(*p)) {p++;}
#define IS_EOL(ch) ((ch) == '\n' || (ch) == '\r' || (ch) == 0 || (ch) == '#')
#define IS_TOKEN(ch) (!(IS_BLANK(ch) || IS_EOL(ch)))
#define SKIP_TOKEN(p) while(IS_TOKEN(*p)) {p++;}

static
RvStatus RvEHMapParseLine(RvEHMap *self, RvChar *line) {
    RvChar *cur;
    RvChar *saddr;
    RvChar *cname;
    RvStatus s = RV_OK;
    RvEHARecord *arec;
    RvEHCnameRecord *crec, **lastAlias;
    union {
        RvUint8 ipv6[16];
        RvUint32 ipv4;
    } ua;

    RvInt addrType = 0;
    RvEHBaseRecord **pbrec = 0, *brec;
    RvBool r;
    RvEHAllocator *allocator = &self->alloc;
    RvSize_t clen;
    RvBool eol = RV_FALSE;
    
    cur = line;
    SKIP_BLANKS(cur);
    if(IS_EOL(*cur)) {
        return s;
    }

    /* saddr now points to the address part of the line */
    saddr = cur;
    SKIP_TOKEN(cur);
    /* No names are associated with this address - return */
    if(IS_EOL(*cur)) {
        return RV_EH_ERROR_FORMAT;
    }
    *cur++ = '\0';
    SKIP_BLANKS(cur);
    if(IS_EOL(*cur)) {
        return RV_EH_ERROR_FORMAT;
    }

    /* cname now points to the start of canonical name */
    cname = cur;
    SKIP_TOKEN(cur);
    clen = cur - cname + 1;
    if(!(eol = IS_EOL(*cur))) {
        *cur++ = '\0';
    } else {
        *cur = '\0';
    }

    /* If canonical name is longer than 256 - skip to the next line
     * Actually, we should check also that the size of individual label is
     * less than 64.
     */
    if(clen > MAX_DNSNAME_SIZE) {
        return RV_LOCAL_ERROR_NAMESIZE;
    }

    /* Now we construct A record */

    /* try to parse address as ipv4 address */
    if(RvAddressStringToIpv4(&ua.ipv4, saddr)) {
        addrType = RV_ADDRESS_TYPE_IPV4;
#if IPV6_NEEDED
    } else if(RvAddressStringToIpv6(ua.ipv6, saddr)) {
        addrType = RV_ADDRESS_TYPE_IPV6;
#endif
    } else {
        return RV_EH_ERROR_FORMAT;
    }

    /* Try to find previous instance of this name in the hash */
    r = RvEHMapFind(self, cname, &pbrec);
    brec = *pbrec;

    if(r) {
        RvAddressList *addrNode;
        RvAddressList *curAddr;
        RvAddressList *prevAddr;
        RvAddress newAddr;
        RvBool addrSeen;

        /* Record with this name already exists */
        if(brec->type == RV_A) {
            arec = (RvEHARecord *)brec;
        } else {
            crec = (RvEHCnameRecord *)brec;
            arec = crec->arec;
        }


        RvAddressConstructT(&newAddr, addrType, &ua);
        addrSeen = RV_FALSE;
        prevAddr = 0;
        for(curAddr = &arec->firstAddr; curAddr != 0 && !addrSeen; curAddr = curAddr->next) {
            addrSeen = RvAddressCompare(&newAddr, &curAddr->addr, RV_ADDRESS_BASEADDRESS);
            prevAddr = curAddr;
        }

        /* This address is new */
        if(!addrSeen) {
            /* Add new IP address to the linked list of addresses of this 'A' record */
            s = RvEHAllocType(allocator, RvAddressList, addrNode);

            if(s != RV_OK) {
                return s;
            }
            addrNode->next = 0;
            RvAddressCopy(&newAddr, &addrNode->addr);
            prevAddr->next = addrNode;
        }
    } else {
        RvChar *oname;
        RvAddress *paddr;

        /* Record with this name doesn't exist, create new 'A' record */
        s = RvEHAllocType(allocator, RvEHARecord, arec);
        if(s != RV_OK) {
            return s;
        }

        arec->base.type = RV_A;
        arec->firstAlias = 0;
        s = RvEHAllocString(allocator, clen, oname);
        if(s != RV_OK) {
            return s;
        }
        strcpy(oname, cname);
        arec->base.name = oname;
        arec->base.next = 0;
        paddr = &arec->firstAddr.addr;
        RvAddressConstructT(paddr, addrType, (void *)&ua);
        arec->firstAddr.next = 0;
        *pbrec = (RvEHBaseRecord *)arec;
    }

    /* Now 'arec' points to the 'A' record (newly created or existent) 
     * Add aliases
     */

    lastAlias = &arec->firstAlias;
    while(*lastAlias) {
        lastAlias = &(*lastAlias)->nextAlias;
    }

    SKIP_BLANKS(cur);
    while(!eol && !IS_EOL(*cur)) {
        RvChar *alias = cur;
        RvSize_t alen;

        SKIP_TOKEN(cur);
        alen = cur - alias + 1;

        /* We're at eol */
        if(IS_EOL(*cur)) {
            eol = RV_TRUE;
            *cur = 0;
        } else {
            *cur++ = '\0';
        }

        SKIP_BLANKS(cur);

        /* If the size of alias is too long, skip to the next alias
         * In contrast with cname, we use a more relaxed policy here:
         * instead of skipping to the next line we skip to the next field
         */
        if(alen > MAX_DNSNAME_SIZE) {
            continue;
        }

        r = RvEHMapFind(self, alias, &pbrec);
        /* If this alias was already seen - just ignore it (warning on log should be printed */
        if(r) {
            continue;
        }
        /* Create new alias record */
        s = RvEHAllocType(allocator, RvEHCnameRecord, crec);
        if(s != RV_OK) {
            return s;
        }

        crec->arec = arec;
        crec->base.next = 0;
        crec->base.type = RV_CNAME;
        s = RvEHAllocString(allocator, alen, crec->base.name);
        if(s != RV_OK) {
            return s;
        }
        strcpy(crec->base.name, alias);
        *lastAlias = crec;
        crec->nextAlias = 0;
        lastAlias = &crec->nextAlias;
        *pbrec = (RvEHBaseRecord *)crec;

    }

    return RV_OK;
}

#define DEFAULT_LINE_SIZE 256

/* Reads the next line from file given
 *
 *
 */

static 
RvStatus RvEHMapReadLine(RvEHMap *self, RvFILE *fp, RvChar **pline, RvSize_t *psize) {
    RvChar *line = *pline;
    RvSize_t size = *psize;
    RvSize_t lastpos = size - 2;
    RvChar *p;
    RvChar lastChar;
    RvStatus s;
    RvMemory *mem = self->alloc.mem;
    RvLogMgr *logMgr = self->alloc.logMgr;

    line[lastpos] = 0;
    p = RvFgets(line, size, fp);
    if(p == 0) {
        return RV_LOCAL_ERROR_EOF;
    }

    lastChar = line[lastpos];
    if(lastChar == 0 || lastChar == '\n') {
        return RV_OK;
    }

    /* line is too long, try to enlarge buffer and read the rest of the line */

    for(;;) {
        RvChar *tmp;
        RvSize_t tmpSize = size << 1;

        s = RvMemoryAlloc(mem, tmpSize, logMgr, (void **)&tmp);
        if(s != RV_OK) {
            return s;
        }

        memcpy(tmp, line, size);

        /* If memory for 'line' was allocated by this routine, e.g. that's at least a second time
         * in this loop - free it now
         */
        if(line != *pline) {
            RvMemoryFree(line, logMgr);
        }

        line = tmp;
        lastpos = tmpSize - 2;
        line[lastpos] = 0;
        p = RvFgets(line + size - 1, size + 1, fp);
        size = tmpSize;
        if(p == 0) {
            break;
        }

        lastChar = line[lastpos];
        if(lastChar == 0 || lastChar == '\n') {
            break;
        }
    }

    *pline = line;
    *psize = size;
    return RV_OK;
}


RvStatus
RvEHMapParse(RvEHMap *self, RvFILE *fp) {
    RvChar line[DEFAULT_LINE_SIZE];
    RvSize_t lineSize = sizeof(line);
    RvChar *pLine = line;
    RvStatus s;
    RvLogMgr *logMgr = RvEHMapGetLog(self);

    while(RvEHMapReadLine(self, fp, &pLine, &lineSize) == RV_OK) {
        s = RvEHMapParseLine(self, pLine);
        if(pLine != line) {
            RvMemoryFree(pLine, logMgr);
            pLine = line;
            lineSize = sizeof(line);
        }
    }

    return RV_OK;
}

#if RV_EHD_DEBUG

void RvEHMapDump(RvEHMap *self, RvFILE *fp) {
    RvEHBaseRecord *cur;
    RvEHARecord *a;
    RvSize_t i, hsize;
    RvEHBaseRecord **harr = self->hash;

    hsize = self->sizeMask;

    for(i = 0; i <= hsize; i++) {
        cur = harr[i];

        for(; cur; cur = cur->next) {
            RvAddressList *addrNode;
            RvEHCnameRecord *cnameNode;

            if(cur->type == RV_CNAME) continue;
            a = (RvEHARecord *)cur;
            RvFprintf(fp, "A record for %s\n\tAddresses\n", a->base.name);
            for(addrNode = &a->firstAddr; addrNode; addrNode = addrNode->next) {
                RvChar buf[50];

                RvAddressGetString(&addrNode->addr, sizeof(buf), buf);
                RvFprintf(fp, "\t\t%s\n", buf);
            }

            RvFprintf(fp, "\tAliases\n");

            for(cnameNode = a->firstAlias; cnameNode; cnameNode = cnameNode->nextAlias) {
                RvFprintf(fp, "\t\t%s\n", cnameNode->base.name);
            }
        }
    }
}

#endif

#include "ares_dns.h"

#include <sys/types.h>
#include <sys/stat.h>

RvStatus RvGetLastModificationTime(const RvChar *fileName, RvInt32 *lmt) {
    struct stat st;
    int r;

    r = stat(fileName, &st);
    if(r != 0) {
        return RV_ERROR_UNKNOWN;
    }
    
    *lmt = st.st_mtime;
    return RV_OK;
}

#if (RV_OS_TYPE == RV_OS_TYPE_LINUX) || (RV_OS_TYPE == RV_OS_TYPE_SOLARIS) || \
    (RV_OS_TYPE == RV_OS_TYPE_FREEBSD) || (RV_OS_TYPE == RV_OS_TYPE_NETBSD)

#ifndef RV_EHD_DEFAULT_HOSTFILE 
#define RV_EHD_DEFAULT_HOSTFILE "/etc/hosts"
#endif

const RvChar *sDefaultHostsFileName = RV_EHD_DEFAULT_HOSTFILE;
static 
const RvChar* RvEHDGetDefaultHostsFileName() {
    return sDefaultHostsFileName;
}

/* 0 - don't search hosts file
 * 1 - search hosts file before DNS servers
 * >= 2 - search hosts file after DNS servers
 */
const RvChar sHostConfFileName[] = "/etc/host.conf";


#define IS_CONF_ARG(ch) (IS_TOKEN(ch) && (ch != ','))
#define IS_CONF_BLANK(ch) (!IS_CONF_ARG(ch))

RvInt RvEHDGetSearchOrder() {
    RvFILE *fp;
    RvChar line[256];
    RvChar *cur;

    if(sRvEhdSearchOrder > -1) {
        return sRvEhdSearchOrder;
    }

    fp = fopen(sHostConfFileName, "r");
    if(fp == 0) {
        return RV_EHD_DEFAULT_SEARCH_ORDER;
    }

    while(RvFgets(line, sizeof(line), fp)) {
        RvChar *keyWord;
        RvBool eol;
        RvChar *arg;

        cur = line;
        SKIP_BLANKS(cur);
        if(IS_EOL(*cur)) {
            continue;
        }
        
        keyWord = cur;
        SKIP_TOKEN(cur);
        eol = IS_EOL(*cur);
        *cur++ = '\0';

        /* Current line isn't 'order' line, skip it */
        if(strcmp(keyWord, "order")) {
            continue;
        }

        SKIP_BLANKS(cur);

        if(IS_EOL(*cur)) {
            return RV_EHD_DEFAULT_SEARCH_ORDER;
        }

        arg = cur;
        while(IS_CONF_ARG(*cur)) {
            cur++;
        }

        *cur = '\0';
        if(!strcmp(arg, "hosts")) {
            return RV_EHD_SEARCH_HOSTS_FIRST;
        }

        return RV_EHD_SEARCH_HOSTS_LAST;
    }

    return RV_EHD_DEFAULT_SEARCH_ORDER;
}



#elif (RV_OS_TYPE == RV_OS_TYPE_WIN32)

static
const RvChar* RvEHDGetDefaultHostsFileName() {
    static RvChar sDefaultHostsFileName[262] = "";

    if(sDefaultHostsFileName[0]) {
        return sDefaultHostsFileName;
    }

    ExpandEnvironmentStrings("%SYSTEMROOT%\\System32\\Drivers\\Etc\\Hosts", sDefaultHostsFileName, sizeof(sDefaultHostsFileName));

    return sDefaultHostsFileName;
}

RvInt RvEHDGetSearchOrder() {
	if(sRvEhdSearchOrder > -1) {
		return sRvEhdSearchOrder;
	}

	return RV_EHD_SEARCH_HOSTS_LAST;
}

#endif


RvStatus RvEHDConstruct(RvEHD *self, const RvChar *etcFileName, RvMemory *mem, RvLogMgr *logMgr) {
#define EHMAP_CREATED  2
#define FILE_OPENED    4
#define FUNCTION_NAME "RvEHDConstruct"

    RvStatus s = RV_OK;
    RvUint32  constructStep = 0;
    RvFILE    *fp = 0;

   	if(logMgr) {
        self->pLogSource = &self->logSource;
        RvLogSourceConstruct(logMgr, self->pLogSource, "EHD", "Hosts daemon");
    } else {
        self->pLogSource = 0;
    }

    EHD_LOG_ENTER;
    EHD_LOG_DEBUG_1("Constructing EHD server %p", self);

    self->mem    = mem;
    self->logMgr = logMgr;
    self->etcFileName[MAX_FILENAME_SIZE - 1] = 0;
    if(etcFileName == 0 || etcFileName[0] == 0) {
        etcFileName = RvEHDGetDefaultHostsFileName();
    }
    strncpy(self->etcFileName, etcFileName, MAX_FILENAME_SIZE);
    if(self->etcFileName[MAX_FILENAME_SIZE - 1]) {
        EHD_LOG_ERROR_1("Hosts filename too long: %s", etcFileName);
        return RV_ERROR_BADPARAM;
    }

    EHD_LOG_DEBUG_1("Using file %s as hosts file", self->etcFileName);
    
    s = RvEHMapConstruct(&self->db, mem, DEFAULT_HASH_SIZE, DEFAULT_PAGE_SIZE, logMgr);
    if(s != RV_OK) {
        EHD_LOG_ERROR_1("Failed to constuct hosts data base (error: %d)", s);
        goto failure;
    }
    constructStep |= EHMAP_CREATED;

    fp = RvFopen(etcFileName, "r");
    if(fp == 0) {
        EHD_LOG_ERROR_1("Failed to open hosts file: %s", etcFileName);
        s = RV_ERROR_UNKNOWN;
        goto failure;
    }

    constructStep |= FILE_OPENED;
    s = RvEHMapParse(&self->db, fp);
    RvFclose(fp);

    RvGetLastModificationTime(etcFileName, &self->lastModificationTime);

    if(s != RV_OK) {
        goto failure;
    }


    EHD_LOG_LEAVE_0("EHD server constructed successfully");
    return s;

failure:
    if(constructStep & EHMAP_CREATED) {
        RvEHMapDestruct(&self->db);
    }

	/*lint -e644 */
	/* Prevent variable not initialized warning from lint
	 * if constructStep & FILE_OPENED is true it means that
	 * fp was initialized (file opened)
	 */
    if(constructStep & FILE_OPENED) {
        RvFclose(fp);
    }
	/*lint +e644 */


    return s;
#undef EHMAP_CREATED  
#undef FILE_OPENED    
#undef FUNCTION_NAME
}

static 
RvStatus RvEHDReload(RvEHD *self) {
    RvStatus s = RV_OK;
    RvFILE *fp;

    fp = RvFopen(self->etcFileName, "r");
    if(fp == 0) {
        return RV_LOCAL_ERROR_FILE;
    }
    RvEHMapDestruct(&self->db);
    s = RvEHMapConstruct(&self->db, self->mem, DEFAULT_HASH_SIZE, DEFAULT_PAGE_SIZE, self->logMgr);
    if(s != RV_OK) {
        goto failure;
    }
    s = RvEHMapParse(&self->db, fp);
failure:
    RvFclose(fp);
    return s;
}


static 
RvStatus RvEHDCheckUpdate(RvEHD *self) {
#define FUNCTION_NAME "RvEHDCheckUpdate"
    RvInt32 lmt;
    RvStatus s;

    s = RvGetLastModificationTime(self->etcFileName, &lmt);
    /* If for some reason RvGetLastModificationTime fails, just do nothing */
    if(s != RV_OK) {
        EHD_LOG_DEBUG_1("Failed to get last modification time of %s, do nothing", self->etcFileName);
        return RV_OK;
    }

    /* If last modification time wasn't changed - return, in all other cases - try to reload hosts file */
    if(lmt == self->lastModificationTime) {
        return RV_OK;
    }

    EHD_LOG_DEBUG_0("Modification time of 'hosts' file changed, reload database");
    RvEHDReload(self);
    self->lastModificationTime = lmt;
    return RV_OK;
#undef FUNCTION_NAME
}

RvStatus RvEHDDestruct(RvEHD *self) {
    RvEHMapDestruct(&self->db);
	if(self->logMgr) {
		RvLogSourceDestruct(self->pLogSource);
	}
    return RV_OK;
}


/* -->> */
RvStatus RvEHDFind(RvEHD *self, RvChar *name, RvUint16 qtype, RvUint8 *pBuf, RvSize_t *pSize, RvInt *pNfound) {
#define FUNCTION_NAME "RvEHDFind"
    RvBool found;
    RvEHBaseRecord **pprec;
    RvEHBaseRecord *prec;
    RvEHARecord *arec;
    RvInt nAns = 0;
    RvInt targetAddressType;
    RvAddressList *curNode;
    RvSize_t arecSize;
    RvUint8 *curBuf = pBuf;
    RvUint8 *pend = pBuf + *pSize;


    RvEHDCheckUpdate(self);
    
    /* Our server serves only A or AAAA queries (when compile with IPv6 support) */
    if(qtype != T_A && (qtype != T_AAAA || !gIPv6Needed)) {
        EHD_LOG_DEBUG_1("Got query type not supported by 'hosts' file: %d", qtype);
        return RV_DNS_ERROR_RTNOTSUPP;
    }


    found = RvEHMapFind(&self->db, name, &pprec);
    if(!found) {
        EHD_LOG_DEBUG_1("Name %s not found in 'hosts' database, returning 'not found' response", name);
        return RV_DNS_ERROR_CACHE_NOTFOUND;
    }

    prec = *pprec;

    if(prec->type == RV_CNAME) {
        RvEHCnameRecord *crec = (RvEHCnameRecord *)prec;
        arec = crec->arec;
    } else if(prec->type == RV_A) {
        arec = (RvEHARecord *)prec;
    } else {
        RvAssert(0);
        return RV_ERROR_UNKNOWN; /* something strange happens */
    }

    if(qtype == T_A) {
        targetAddressType = RV_ADDRESS_TYPE_IPV4;
        arecSize = 4;
    } else {
        targetAddressType = RV_ADDRESS_TYPE_IPV6;
        arecSize = 16;
    }

    for(curNode = &arec->firstAddr; curNode; curNode = curNode->next) {
        RvAddress *curAddr = &curNode->addr;
        RvInt      addrType;

        addrType = RvAddressGetType(curAddr);
		/* filter out addresses that differs in type */
        if(addrType != targetAddressType) {
            continue;
		} 
		
		if(addrType == RV_ADDRESS_TYPE_IPV4) {
            curBuf = RvAlign32(curBuf);
            if(curBuf + 4 >= pend) {
                return RV_ERROR_INSUFFICIENT_BUFFER;
            }
			*(RvUint32 *)curBuf = curAddr->data.ipv4.ip;
            curBuf += 4;
		}
#if IPV6_NEEDED
		else {
            if(curBuf + arecSize >= pend) {
                return RV_ERROR_INSUFFICIENT_BUFFER;
            }

			memcpy(curBuf, curAddr->data.ipv6.ip, 16);
            curBuf += 16;
		}
#endif

        nAns++;
    }

    if(nAns == 0) {
        return RV_DNS_ERROR_CACHE_NOTFOUND;
    }

    if(pNfound) {
        *pNfound = nAns;
    }
    *pSize = curBuf - pBuf;
    return RV_OK;
#undef FUNCTION_NAME
}


/* RvStatus RvEHDNew(RvEHD **self, RvSelectEngine *seli, RvLogMgr *logMgr) 
 * 
 * Creates new instance of RvEHD
 *
 * Arguments:
 * self   - (out) points to the newly allocated instance of RvEHD
 * seli   - select engine to use
 * logMgr - log to use
 *
 */
RvStatus RvEHDNew(RvEHD **self, RvLogMgr *logMgr) {
    RvStatus s = RV_OK;

    s = RvMemoryAlloc(0, sizeof(RvEHD), logMgr, (void **)self);
    if(s != RV_OK) {
        *self = 0;
        return s;
    }

    s = RvEHDConstruct(*self, RV_EHD_HOSTFILE, 0, logMgr);
    if(s != RV_OK) {
        RvMemoryFree(*self, logMgr);
        *self = 0;
    }

    return s;
}

/* RvStatus RvEHDDelete(RvEHD *self) 
 * 
 * Deletes RvEHD instance pointed by 'self'
 *
 */
RvStatus RvEHDDelete(RvEHD *self) {
    RvLogMgr *logMgr = self->logMgr;
    RvEHDDestruct(self);
    RvMemoryFree(self, logMgr);
    return RV_OK;
}


#endif /* RV_DNS_USES_HOSTS */

#endif /* RV_NET_TYPE != RV_NET_NONE */
