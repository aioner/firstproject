#pragma once

#define MAX_MATRIX_RECT 6

//����������ת��
//arrMatrix         ��������
//nPitch            �����п�
//sizeResolution    ���ֱ���
//sizeUnit          ��С��Ԫ��
//arrRect           ��������
//nRectNum          ��������
#ifdef __cplusplus
extern "C" {
#endif

int MatrixToRect(BYTE IN arrMatrix[], int nPitch, SIZE sizeResolution, SIZE sizeUnit, RECT OUT arrRect[], int OUT *nRectNum);
int RectToMatrix(RECT IN arrRect[], int nRectNum, int nPitch, SIZE sizeResolution, SIZE sizeUnit, BYTE OUT arrMatrix[]);
int RectExchange(SIZE sizeSourceResolution, SIZE sizeObjectResolution, RECT rcSourceRect, RECT *pObjectRect);

#ifdef __cplusplus
}
#endif
