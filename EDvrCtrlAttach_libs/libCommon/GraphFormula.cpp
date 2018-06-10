#include "stdafx.h"
#include ".\GraphFormula.h"

#ifdef __cplusplus
extern "C" {
#endif

int MatrixToRect(BYTE IN arrMatrix[], int nPitch, SIZE sizeResolution, SIZE sizeUnit, RECT OUT arrRect[], int OUT *nRectNum)
{
    int i=0, j=0, k=0, x=0, y=0, nXSize, nYSize;
    int nStart = 0;
    int id = 0;
    RECT tmpRect[2][MAX_MATRIX_RECT];
    memset(tmpRect, -1, sizeof(RECT)*MAX_MATRIX_RECT*2);

    //等分画面
    nXSize = sizeResolution.cx/sizeUnit.cx;
    nYSize = sizeResolution.cy/sizeUnit.cy;

    *nRectNum = 0;
    for(y=0; y<nYSize; y++)
    {
        memset(tmpRect, -1, sizeof(RECT)*MAX_MATRIX_RECT);
        nStart = 0;
        id = 0;
        for(x=0; x<nXSize; x++)
        {
            if(id>=MAX_MATRIX_RECT) break;
            if((arrMatrix+y*nPitch)[x] == 0)
            {
                if(nStart)//停止向右合并
                {
                    id++;
                    nStart = 0;
                }
                continue;
            }
            if(tmpRect[0][id].left==-1)//向右合并
            {
                tmpRect[0][id].left = x;
                tmpRect[0][id].top = y;
                tmpRect[0][id].bottom = y+1;
            }
            tmpRect[0][id].right = x+1;
            nStart = 1;
        }
        if(nStart)
            id++;
        if(id == 0)
            continue;
        for(i=0; i<(id<=MAX_MATRIX_RECT?id:MAX_MATRIX_RECT); i++)
        {
            for(j=0; j<MAX_MATRIX_RECT; j++)
            {
                if(tmpRect[0][i].left != -1
                    && tmpRect[0][i].right  != -1
                    && tmpRect[0][i].left   <= tmpRect[1][j].left//包含宽度且
                    && tmpRect[0][i].right  >= tmpRect[1][j].right
                    && tmpRect[0][i].top    == tmpRect[1][j].bottom)//上下相邻
                {
                    tmpRect[1][j].bottom = tmpRect[0][i].bottom;//向下合并
                    if(tmpRect[0][i].left == tmpRect[1][j].left//有相同宽度
                        && tmpRect[0][i].right == tmpRect[1][j].right)
                        break;
                    continue;
                }
                if(tmpRect[1][j].left == -1 && tmpRect[1][j].right == -1)//记录新区域
                {
                    memcpy(tmpRect[1]+j, tmpRect[0]+i, sizeof(RECT));
                    (*nRectNum)++;
                    break;
                }
            }
        }
    }
    memcpy(arrRect, tmpRect[1], sizeof(RECT) * (*nRectNum));
    for(i=0; i<(*nRectNum); i++)
    {
        arrRect[i].left     = arrRect[i].left * sizeUnit.cx;
        arrRect[i].right    = arrRect[i].right * sizeUnit.cx;
        arrRect[i].top      = arrRect[i].top * sizeUnit.cy;
        arrRect[i].bottom   = arrRect[i].bottom * sizeUnit.cy;
    }
    return 0;
}

int RectToMatrix(RECT IN arrRect[], int nRectNum, int nPitch, SIZE sizeResolution, SIZE sizeUnit, BYTE OUT arrMatrix[])
{
    int nXSize=0, nYSize=0, i=0, j=0, k=0;
    RECT rRect[MAX_MATRIX_RECT] = {0};
    BYTE rOutput[64*96*2] = {0};

    //等分画面
    nXSize = sizeResolution.cx/sizeUnit.cx;
    nYSize = sizeResolution.cy/sizeUnit.cy;

    for(i=0; i<nRectNum; i++)
    {
        rRect[i].left = (arrRect[i].left+sizeUnit.cx-1)/sizeUnit.cx;
        if(rRect[i].left>nXSize) rRect[i].left = nXSize;

        rRect[i].right = (arrRect[i].right+sizeUnit.cx-1)/sizeUnit.cx;
        if(rRect[i].right>nXSize) rRect[i].right = nXSize;

        rRect[i].top = (arrRect[i].top+sizeUnit.cy-1)/sizeUnit.cy;
        if(rRect[i].top>nYSize) rRect[i].top = nYSize;

        rRect[i].bottom = (arrRect[i].bottom+sizeUnit.cy-1)/sizeUnit.cy;
        if(rRect[i].bottom>nXSize) rRect[i].bottom = nYSize;
    }

    nRectNum = nRectNum<MAX_MATRIX_RECT?nRectNum:MAX_MATRIX_RECT;
    for(k = 0; k < nRectNum; k++)
    {
        if(rRect[k].left>nXSize || rRect[k].left<0 ||
            rRect[k].right>nXSize || rRect[k].right<0 ||
            rRect[k].top>nYSize || rRect[k].top<0 ||
            rRect[k].bottom>nYSize || rRect[k].bottom<0)

            continue;
        if (rRect[k].top <= rRect[k].bottom)//上下顺序
        {
            if (rRect[k].left <= rRect[k].right)//左右顺序
            {
                for(i = rRect[k].top; i < rRect[k].bottom; i++)
                {
                    for(j = rRect[k].left; j < rRect[k].right; j++)
                    {
                        rOutput[i*nPitch+j] = 1;
                    }
                }
            }
            else//左右倒序
            {
                for(i = rRect[k].top; i < rRect[k].bottom; i++)
                {
                    for(j = rRect[k].right; j < rRect[k].left; j++)
                    {
                        rOutput[i*nPitch+j] = 1;
                    }
                }
            }
        }
        else//上下倒序
        {
            if (rRect[k].left <= rRect[k].right)//左右顺序
            {
                for(i = rRect[k].bottom; i < rRect[k].top; i++)
                {
                    for(j = rRect[k].left; j < rRect[k].right; j++)
                    {
                        rOutput[i*nPitch+j] = 1;
                    }
                }
            }
            else//左右倒序
            {
                for(i = rRect[k].bottom; i < rRect[k].top; i++)
                {
                    for(j = rRect[k].right; j < rRect[k].left; j++)
                    {
                        rOutput[i*nPitch+j] = 1;
                    }
                }
            }
        }
    }

    memcpy(arrMatrix, rOutput, nPitch*nYSize);
    return 0;
}

int RectExchange(SIZE sizeSourceResolution, SIZE sizeObjectResolution, RECT rcSourceRect, RECT *pObjectRect)
{
    if (sizeSourceResolution.cx == 0 || sizeSourceResolution.cy == 0)
		return -1;
	
	pObjectRect->left = rcSourceRect.left * sizeObjectResolution.cx / sizeSourceResolution.cx;
    pObjectRect->right = rcSourceRect.right * sizeObjectResolution.cx / sizeSourceResolution.cx;
    pObjectRect->top = rcSourceRect.top * sizeObjectResolution.cy / sizeSourceResolution.cy;
    pObjectRect->bottom = rcSourceRect.bottom * sizeObjectResolution.cy / sizeSourceResolution.cy;
    return 0;
}

#ifdef __cplusplus
}
#endif