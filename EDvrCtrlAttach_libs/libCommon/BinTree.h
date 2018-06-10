#pragma once

typedef enum _ERedbl
{
    _Red,
    _Black
}ERedbl;

typedef struct _STreeNode
{
    struct _STreeNode *pLeft;
    struct _STreeNode *pParent;
    struct _STreeNode *pRight;
    ERedbl eColor;
    void *pValue;
}STreeNode;

#ifdef __cplusplus
extern "C"{
#endif


#ifdef __cplusplus
};

class CBinTree
{
public:
    CBinTree(void);
    ~CBinTree(void);
};

#endif //__cplusplus