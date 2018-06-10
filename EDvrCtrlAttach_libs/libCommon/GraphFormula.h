#pragma once

#define MAX_MATRIX_RECT 6

//矩阵与区域转换
//arrMatrix         矩阵数组
//nPitch            矩阵行宽
//sizeResolution    主分辨率
//sizeUnit          最小单元格
//arrRect           区域数组
//nRectNum          区域数量
#ifdef __cplusplus
extern "C" {
#endif

int MatrixToRect(BYTE IN arrMatrix[], int nPitch, SIZE sizeResolution, SIZE sizeUnit, RECT OUT arrRect[], int OUT *nRectNum);
int RectToMatrix(RECT IN arrRect[], int nRectNum, int nPitch, SIZE sizeResolution, SIZE sizeUnit, BYTE OUT arrMatrix[]);
int RectExchange(SIZE sizeSourceResolution, SIZE sizeObjectResolution, RECT rcSourceRect, RECT *pObjectRect);

#ifdef __cplusplus
}
#endif
