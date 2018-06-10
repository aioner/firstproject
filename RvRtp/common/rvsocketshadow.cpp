#include "rvsocketshadow.h"

#if RV_SOCKET_USE_SHADOWS

#include <map>
#include "rvsocket.h"
#include "rvmemory.h"
#include "rvlock.h"

struct RvSocketShadowNode {
    RvSocketShadowNode *next;
    RvSocketShadowID    id;
    RvSocketShadow      shadow;
    RvSocket           *socket;
    RvSocketShadowDestruct destructor;
};

using namespace std;

typedef map<RvSocket*, RvSocketShadowNode* > RvSocketShadowMap;

class RvSocketShadows {
    typedef RvSocketShadowMap::iterator MapIterator;
    RvSocketShadowMap m_map;
    RvLock m_lock;

    enum Operation {
        NoOp,
        Add,
        Remove
    };

private:

    class Lock {
        RvLock *lock;
    public:
        Lock(RvLock *lock): lock(lock) {
            RvLockGet(lock, 0);
        }

        ~Lock() {
            RvLockRelease(lock, 0);
        }
    };

    RvSocketShadowNode* find(RvSocket *sock, RvSocketShadowID id, Operation op);

public:
    RvStatus init();
    RvStatus add(RvSocket *sock, RvSocketShadowID id, RvSocketShadow shadow, RvSocketShadowDestruct destructor);
    RvSocketShadow find(RvSocket *sock, RvSocketShadowID id);
    RvSocketShadow remove(RvSocket *sock, RvSocketShadowID id, bool bDestruct);
    void remove(RvSocket *sock);
    ~RvSocketShadows();
};

RvStatus RvSocketShadows::init() {
    RvStatus s = RvLockConstruct(0, &m_lock);
    return s;
}

RvSocketShadowNode* RvSocketShadows::find(RvSocket *sock, RvSocketShadowID id, Operation op) {
    RvSocketShadowNode *node = 0;
    RvSocketShadowNode *last = 0;

    MapIterator iter = m_map.find(sock);

    if(iter != m_map.end()) {
        for(RvSocketShadowNode *cur = iter->second; cur != 0; last = cur, cur = cur->next) {
            if(cur->id == id) {
                node = cur;
                break;
            }
        }
    }

    if(op == NoOp) {
        return node;
    }

    if(op == Add) {
        // Node already exists
        if(node) {
            return 0;
        }

        RvStatus s = RvMemoryAlloc(0, sizeof(*node), 0, (void **)&node);
        if(s != RV_OK) {
            return 0;
        }

        node->next = 0;
        if(last != 0) {
            last->next = node;
        } else {
            m_map.insert(make_pair(sock, node));
        }

        return node;
    }

    if(op == Remove) {
        // Node wasn't found
        if(node == 0) {
            return 0;
        }

        if(last != 0) {
            last->next = node->next;
            return node;
        }

        RvSocketShadowNode *next = node->next;
        if(next) {
            iter->second = next;
        } else {
            m_map.erase(iter);
        }
        return node;
    }

    // Shouldn't happen, illegal operation
    return 0;
}

RvSocketShadow RvSocketShadows::find(RvSocket *sock, RvSocketShadowID id) {
    Lock l(&m_lock);

    RvSocketShadow shadow = 0;
    RvSocketShadowNode *node = find(sock, id, NoOp);

    if(node) {
        shadow = node->shadow;
    }

    return shadow;
}

RvStatus RvSocketShadows::add(RvSocket *sock, RvSocketShadowID id, RvSocketShadow shadow, RvSocketShadowDestruct destructor) {
    RvSocketShadowNode *node = 0;
    Lock l(&m_lock);


    node = find(sock, id, Add);
    if(node == 0) {
        return RV_ERROR_UNKNOWN;
    }

    node->id = id;
    node->next = 0;
    node->shadow = shadow;
    node->socket = sock;
    node->destructor = destructor;

    return RV_OK;
}


RvSocketShadow RvSocketShadows::remove(RvSocket *sock, RvSocketShadowID id, bool bDestruct) {
    RvSocketShadowNode *node = 0;

    {
        Lock l(&m_lock);
        node = find(sock, id, Remove);
        if(node == 0) {
            return 0;
        }
    }

    RvSocketShadowDestruct destructor = node->destructor;
    RvSocketShadow shadow = node->shadow;
    RvMemoryFree(node, 0);
    if(bDestruct && destructor) {
        destructor(shadow);
    }

    return shadow;
}

void RvSocketShadows::remove(RvSocket *sock) {
    Lock l(&m_lock);

    MapIterator iter = m_map.find(sock);
    if(iter == m_map.end()) {
        return;
    }

    RvSocketShadowNode *cur, *next;

    for(cur = iter->second; cur != 0; cur = next) {
        next = cur->next;

        if(cur->destructor) {
            cur->destructor(cur->shadow);
        }

        RvMemoryFree(cur, 0);
    }

    m_map.erase(iter);

}


RvSocketShadows::~RvSocketShadows() {
    for(MapIterator iter = m_map.begin(); iter != m_map.end(); iter++) {
        RvSocketShadowNode *cur, *next;

        for(cur = iter->second; cur != 0; cur = next) {
            next = cur->next;

            if(cur->destructor) {
                cur->destructor(cur->shadow);
            }

            RvMemoryFree(cur, 0);
        }
    }

    RvLockDestruct(&m_lock, 0);
}

static union {
    RvAlign_s a;
    char gsArea[sizeof(RvSocketShadows)];
};


static RvSocketShadows *gsShadows;

RvStatus RvSocketShadowInit() {
    RvStatus s = RV_OK;

    // keep compliler happy 
    (void)a;

    gsShadows = new (gsArea) RvSocketShadows;

    s = gsShadows->init();

    return s;
}


void RvSocketShadowEnd() {
    gsShadows->~RvSocketShadows();
}

RvSocketShadow RvSocketShadowFind(RvSocket *sock, RvSocketShadowID id) {
    return gsShadows->find(sock, id);
}

RvStatus RvSocketShadowAdd(RvSocket *sock, RvSocketShadowID id, RvSocketShadow shadow, RvSocketShadowDestruct destruct) {
    return gsShadows->add(sock, id, shadow, destruct);
}

RvSocketShadow RvSocketShadowRemove(RvSocket *sock, RvSocketShadowID id, RvBool bDestruct) {
    return gsShadows->remove(sock, id, bDestruct ? true : false);
}

void RvSocketShadowRemoveAll(RvSocket *sock) {
    gsShadows->remove(sock);
}

#else

int rvsocketshadow_unused;

#endif

