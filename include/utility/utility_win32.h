#ifndef __UTILITY__201204121035__H__
#define __UTILITY__201204121035__H__

namespace utility_hlp
{
	///////////////////////////////////////////////////////////////////////////
	//! 判断文件是否存在?
	// 
	inline BOOL IsExistFile(LPCTSTR lpszFilePath) {

		return (GetFileAttributes(lpszFilePath) 
			!= INVALID_FILE_ATTRIBUTES);
	}

	///////////////////////////////////////////////////////////////////////////
	//! 判断目录是否存在?
	// 
	inline BOOL IsExistDirectory(LPCTSTR lpszDir) {

		CFileFind Finder;
		CString sFindDir = CString(lpszDir) + _T("\\*.*");

		return Finder.FindFile(sFindDir);
	}

	///////////////////////////////////////////////////////////////////////////
	//! 如果该目录不存在，则创建
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
	//! 判断目录是否为空
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
	//! 删除目录下为空的子目录 
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


	// !获取模块文件目录
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