#ifndef __UTILITY__201204121035__H__
#define __UTILITY__201204121035__H__

namespace utility_hlp
{
	///////////////////////////////////////////////////////////////////////////
	//! �ж��ļ��Ƿ����?
	// 
	inline BOOL IsExistFile(LPCTSTR lpszFilePath) {

		return (GetFileAttributes(lpszFilePath) 
			!= INVALID_FILE_ATTRIBUTES);
	}

	///////////////////////////////////////////////////////////////////////////
	//! �ж�Ŀ¼�Ƿ����?
	// 
	inline BOOL IsExistDirectory(LPCTSTR lpszDir) {

		CFileFind Finder;
		CString sFindDir = CString(lpszDir) + _T("\\*.*");

		return Finder.FindFile(sFindDir);
	}

	///////////////////////////////////////////////////////////////////////////
	//! �����Ŀ¼�����ڣ��򴴽�
	// 
	inline void CreateDirForFullPath(const CString& sFullPath) {

		CString sDir;
		int curPos = 0;
		CString resToken= sFullPath.Tokenize(_T("\\"),curPos);
		while ( resToken != _T("") ) {

			sDir += resToken;		
			if ( !IsExistDirectory(sDir) ) 
				::CreateDirectory(sDir, NULL);
			sDir += _T("\\");

			resToken = sFullPath.Tokenize(_T("\\"), curPos);
		};   	
	}

	///////////////////////////////////////////////////////////////////////////
	//! �ж�Ŀ¼�Ƿ�Ϊ��
	// 
	inline BOOL IsDirectoryEmpty(const CString& Path) {

		CFileFind f;
		CString strWildcard(Path);
		strWildcard += _T("\\*.*");

		BOOL bEmpty = TRUE;
		BOOL bWorking = f.FindFile(strWildcard);
		while (bWorking) {

			bWorking = f.FindNextFile();
			if ( f.IsDots() )
				continue;

			bEmpty = FALSE;
			break;
		}
		f.Close();

		return bEmpty;
	}

	///////////////////////////////////////////////////////////////////////////
	//! ɾ��Ŀ¼��Ϊ�յ���Ŀ¼ 
	// 
	inline void RemoveDir(const CString& FullPath) {

		CString Path = FullPath;
		int iPos = Path.GetLength();

		do {

			Path = Path.Mid(0, iPos);
			if ( IsDirectoryEmpty(Path) )
				RemoveDirectory(Path);
			else
				break;

			iPos = Path.ReverseFind('\\');
			if ( iPos == -1 )
				break;		

		} while (TRUE);
	}


	// !��ȡģ���ļ�Ŀ¼
	inline CString GetModulePath() {

		TCHAR szPath[MAX_PATH];
		if( GetModuleFileName(NULL, szPath, MAX_PATH ) != 0 ) {

			CString sPath = szPath;
			int iPos = sPath.ReverseFind('\\');
			sPath.Delete(iPos, sPath.GetLength() - iPos);

			return sPath;
		}

		return _T("");
	}

	template<class T> inline void destruct_ptr(T* pT) {

		if ( pT ) {	

			delete pT;	
			pT = NULL;	
		} 
	}

	struct destruct_obj {

		template<class T> void operator() (T* pT) {

			if ( pT )  { 

				delete pT;	
				pT = NULL; 
			}
		}
	};

	template<typename T>
	inline T& singleton_object()
	{
		static T obj;
		return obj;
	}
}

#endif