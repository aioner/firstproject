#pragma once

// ���º궨��Ҫ������ƽ̨��Ҫ������ƽ̨
// �Ǿ�������Ӧ�ó������蹦�ܵ� Windows��Internet Explorer �Ȳ�Ʒ��
// ����汾��ͨ����ָ���汾�����Ͱ汾��ƽ̨���������п��õĹ��ܣ������
// ����������

// �������Ҫ��Ե�������ָ���汾��ƽ̨�����޸����ж��塣
// �йز�ͬƽ̨��Ӧֵ��������Ϣ����ο� MSDN��
#ifndef WINVER                  // ָ��Ҫ������ƽ̨�� Windows XP SP2��
#define WINVER 0x0502           // ����ֵ����Ϊ��Ӧ��ֵ���������� Windows �������汾��
#endif

#ifndef _WIN32_WINNT            // ָ��Ҫ������ƽ̨�� Windows XP SP2��
#define _WIN32_WINNT 0x0502     // ����ֵ����Ϊ��Ӧ��ֵ���������� Windows �������汾��
#endif

#ifndef _WIN32_WINDOWS          // ָ��Ҫ������ƽ̨�� Windows 98��
#define _WIN32_WINDOWS 0x0410   // ����ֵ����Ϊ�ʵ���ֵ���������� Windows Me ����߰汾��
#endif

#ifndef _WIN32_IE               // ָ��Ҫ������ƽ̨�� Internet Explorer 6.0��
#define _WIN32_IE 0x0600        // ����ֵ����Ϊ��Ӧ��ֵ���������� IE �������汾��
#endif

#if (_MSC_VER >= 1400)
#define _CRT_SECURE_NO_WARNINGS
#endif
