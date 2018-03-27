/******************************************************************
*
* Project: Explorer++
* File: DisplayWindow.cpp
* License: GPL - See LICENSE in the top level directory
*
* Written by David Erceg
* www.explorerplusplus.com
*
*****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "../DisplayWindow/DisplayWindow.h"
#include "../Helper/FolderSize.h"
#include "../Helper/ShellHelper.h"
#include "MainResource.h"

void Explorerplusplus::UpdateDisplayWindow(void)
{
	int nSelected;

	DisplayWindow_ClearTextBuffer(m_hDisplayWindow);

	nSelected = m_pActiveShellBrowser->QueryNumSelected();

	if (nSelected == 0)
		UpdateDisplayWindowForZeroFiles();
	else if (nSelected == 1)
		UpdateDisplayWindowForOneFile();
	else if (nSelected > 1)
		UpdateDisplayWindowForMultipleFiles();
}

void Explorerplusplus::UpdateDisplayWindowForZeroFiles(void)
{
	/* Clear out any previous data shown in the display window. */
	DisplayWindow_ClearTextBuffer(m_hDisplayWindow);
	DisplayWindow_SetThumbnailFile(m_hDisplayWindow, EMPTY_STRING, FALSE);

	TCHAR szCurrentDirectory[MAX_PATH];
	m_pActiveShellBrowser->QueryCurrentDirectory(SIZEOF_ARRAY(szCurrentDirectory), szCurrentDirectory);
	LPITEMIDLIST pidlDirectory = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();

	LPITEMIDLIST pidlComputer = NULL;
	SHGetFolderLocation(NULL, CSIDL_DRIVES, NULL, 0, &pidlComputer);

	if (CompareIdls(pidlDirectory, pidlComputer))
	{
		TCHAR szDisplay[512];
		DWORD dwSize = SIZEOF_ARRAY(szDisplay);
		GetComputerName(szDisplay, &dwSize);
		DisplayWindow_BufferText(m_hDisplayWindow, szDisplay);

		char szCPUBrand[64];
		GetCPUBrandString(szCPUBrand, SIZEOF_ARRAY(szCPUBrand));

		TCHAR szTemp[512];
		WCHAR wszCPUBrand[64];
		MultiByteToWideChar(CP_ACP, 0, szCPUBrand, -1, wszCPUBrand, SIZEOF_ARRAY(wszCPUBrand));
		LoadString(m_hLanguageModule, IDS_GENERAL_DISPLAY_WINDOW_PROCESSOR, szTemp, SIZEOF_ARRAY(szTemp));
		StringCchPrintf(szDisplay, SIZEOF_ARRAY(szDisplay), szTemp, wszCPUBrand);
		DisplayWindow_BufferText(m_hDisplayWindow, szDisplay);

		MEMORYSTATUSEX msex;
		msex.dwLength = sizeof(msex);
		GlobalMemoryStatusEx(&msex);

		ULARGE_INTEGER lTotalPhysicalMem;
		lTotalPhysicalMem.QuadPart = msex.ullTotalPhys;

		TCHAR szMemorySize[32];
		FormatSizeString(lTotalPhysicalMem, szMemorySize, SIZEOF_ARRAY(szMemorySize));
		LoadString(m_hLanguageModule, IDS_GENERAL_DISPLAY_WINDOW_MEMORY, szTemp, SIZEOF_ARRAY(szTemp));
		StringCchPrintf(szDisplay, SIZEOF_ARRAY(szDisplay), szTemp, szMemorySize);
		DisplayWindow_BufferText(m_hDisplayWindow, szDisplay);
	}
	else
	{
		/* Folder name. */
		TCHAR szFolderName[MAX_PATH];
		GetDisplayName(szCurrentDirectory, szFolderName, SIZEOF_ARRAY(szFolderName), SHGDN_INFOLDER);
		DisplayWindow_BufferText(m_hDisplayWindow, szFolderName);

		/* Folder type. */
		SHFILEINFO shfi;
		SHGetFileInfo(reinterpret_cast<LPCTSTR>(pidlDirectory), NULL, &shfi, sizeof(shfi), SHGFI_PIDL | SHGFI_TYPENAME);
		DisplayWindow_BufferText(m_hDisplayWindow, shfi.szTypeName);
	}

	CoTaskMemFree(pidlComputer);
	CoTaskMemFree(pidlDirectory);
}

void Explorerplusplus::UpdateDisplayWindowForOneFile(void)
{
	WIN32_FIND_DATA	*pwfd = NULL;
	SHFILEINFO		shfi;
	TCHAR			szFullItemName[MAX_PATH];
	TCHAR			szFileDate[256];
	TCHAR			szDisplayDate[512];
	TCHAR			szDisplayName[MAX_PATH];
	TCHAR			szDateModified[256];
	int				iSelected;

	iSelected = ListView_GetNextItem(m_hActiveListView, -1, LVNI_SELECTED);

	if (iSelected != -1)
	{
		m_pActiveShellBrowser->QueryDisplayName(iSelected,
			SIZEOF_ARRAY(szDisplayName), szDisplayName);

		/* File name. */
		DisplayWindow_BufferText(m_hDisplayWindow, szDisplayName);

		m_pActiveShellBrowser->QueryFullItemName(iSelected, szFullItemName, SIZEOF_ARRAY(szFullItemName));

		if (!m_pActiveShellBrowser->InVirtualFolder())
		{
			DWORD dwAttributes;

			m_pActiveShellBrowser->QueryFullItemName(iSelected, szFullItemName, SIZEOF_ARRAY(szFullItemName));

			pwfd = m_pActiveShellBrowser->QueryFileFindData(iSelected);

			dwAttributes = GetFileAttributes(szFullItemName);

			if (((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) ==
				FILE_ATTRIBUTE_DIRECTORY) && m_bShowFolderSizes)
			{
				FolderSize_t	*pfs = NULL;
				FolderSizeExtraInfo_t	*pfsei = NULL;
				DWFolderSize_t	DWFolderSize;
				TCHAR			szDisplayText[256];
				TCHAR			szTotalSize[64];
				TCHAR			szCalculating[64];
				DWORD			ThreadId;

				pfs = (FolderSize_t *)malloc(sizeof(FolderSize_t));

				if (pfs != NULL)
				{
					pfsei = (FolderSizeExtraInfo_t *)malloc(sizeof(FolderSizeExtraInfo_t));

					if (pfsei != NULL)
					{
						pfsei->pContainer = (void *)this;
						pfsei->uId = m_iDWFolderSizeUniqueId;
						pfs->pData = (LPVOID)pfsei;

						pfs->pfnCallback = FolderSizeCallbackStub;

						StringCchCopy(pfs->szPath, SIZEOF_ARRAY(pfs->szPath), szFullItemName);

						LoadString(m_hLanguageModule, IDS_GENERAL_TOTALSIZE,
							szTotalSize, SIZEOF_ARRAY(szTotalSize));
						LoadString(m_hLanguageModule, IDS_GENERAL_CALCULATING,
							szCalculating, SIZEOF_ARRAY(szCalculating));
						StringCchPrintf(szDisplayText, SIZEOF_ARRAY(szDisplayText),
							_T("%s: %s"), szTotalSize, szCalculating);
						DisplayWindow_BufferText(m_hDisplayWindow, szDisplayText);

						/* Maintain a global list of folder size operations. */
						DWFolderSize.uId = m_iDWFolderSizeUniqueId;
						DWFolderSize.iTabId = m_selectedTabId;
						DWFolderSize.bValid = TRUE;
						m_DWFolderSizes.push_back(DWFolderSize);

						HANDLE hThread = CreateThread(NULL, 0, Thread_CalculateFolderSize, (LPVOID)pfs, 0, &ThreadId);
						CloseHandle(hThread);

						m_iDWFolderSizeUniqueId++;
					}
					else
					{
						free(pfs);
					}
				}
			}
			else
			{
				SHGetFileInfo(szFullItemName, pwfd->dwFileAttributes,
					&shfi, sizeof(shfi), SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES);

				DisplayWindow_BufferText(m_hDisplayWindow, shfi.szTypeName);
			}

			CreateFileTimeString(&pwfd->ftLastWriteTime,
				szFileDate, SIZEOF_ARRAY(szFileDate),
				m_bShowFriendlyDatesGlobal);

			LoadString(m_hLanguageModule, IDS_GENERAL_DATEMODIFIED, szDateModified,
				SIZEOF_ARRAY(szDateModified));

			StringCchPrintf(szDisplayDate,
				SIZEOF_ARRAY(szDisplayDate),
				_T("%s: %s"), szDateModified, szFileDate);

			/* File (modified) date. */
			DisplayWindow_BufferText(m_hDisplayWindow, szDisplayDate);

			if (IsImage(szFullItemName))
			{
				TCHAR szOutput[256];
				TCHAR szTemp[64];
				UINT uWidth;
				UINT uHeight;
				Gdiplus::Image *pimg = NULL;

				pimg = new Gdiplus::Image(szFullItemName, FALSE);

				if (pimg->GetLastStatus() == Gdiplus::Ok)
				{
					uWidth = pimg->GetWidth();
					LoadString(m_hLanguageModule, IDS_GENERAL_DISPLAYWINDOW_IMAGEWIDTH, szTemp, SIZEOF_ARRAY(szTemp));
					StringCchPrintf(szOutput, SIZEOF_ARRAY(szOutput), szTemp, uWidth);
					DisplayWindow_BufferText(m_hDisplayWindow, szOutput);

					uHeight = pimg->GetHeight();
					LoadString(m_hLanguageModule, IDS_GENERAL_DISPLAYWINDOW_IMAGEHEIGHT, szTemp, SIZEOF_ARRAY(szTemp));
					StringCchPrintf(szOutput, SIZEOF_ARRAY(szOutput), szTemp, uHeight);
					DisplayWindow_BufferText(m_hDisplayWindow, szOutput);

					Gdiplus::PixelFormat format;
					UINT uBitDepth;

					format = pimg->GetPixelFormat();

					switch (format)
					{
					case PixelFormat1bppIndexed:
						uBitDepth = 1;
						break;

					case PixelFormat4bppIndexed:
						uBitDepth = 4;
						break;

					case PixelFormat8bppIndexed:
						uBitDepth = 8;
						break;

					case PixelFormat16bppARGB1555:
					case PixelFormat16bppGrayScale:
					case PixelFormat16bppRGB555:
					case PixelFormat16bppRGB565:
						uBitDepth = 16;
						break;

					case PixelFormat24bppRGB:
						uBitDepth = 24;
						break;

					case PixelFormat32bppARGB:
					case PixelFormat32bppPARGB:
					case PixelFormat32bppRGB:
						uBitDepth = 32;
						break;

					case PixelFormat48bppRGB:
						uBitDepth = 48;
						break;

					case PixelFormat64bppARGB:
					case PixelFormat64bppPARGB:
						uBitDepth = 64;
						break;

					default:
						uBitDepth = 0;
						break;
					}

					if (uBitDepth == -1)
					{
						LoadString(m_hLanguageModule, IDS_GENERAL_DISPLAYWINDOW_BITDEPTHUNKNOWN, szTemp, SIZEOF_ARRAY(szTemp));
						StringCchCopy(szOutput, SIZEOF_ARRAY(szOutput), szTemp);
					}
					else
					{
						LoadString(m_hLanguageModule, IDS_GENERAL_DISPLAYWINDOW_BITDEPTH, szTemp, SIZEOF_ARRAY(szTemp));
						StringCchPrintf(szOutput, SIZEOF_ARRAY(szOutput), szTemp, uBitDepth);
					}

					DisplayWindow_BufferText(m_hDisplayWindow, szOutput);

					Gdiplus::REAL res;

					res = pimg->GetHorizontalResolution();
					LoadString(m_hLanguageModule, IDS_GENERAL_DISPLAYWINDOW_HORIZONTALRESOLUTION, szTemp, SIZEOF_ARRAY(szTemp));
					StringCchPrintf(szOutput, SIZEOF_ARRAY(szOutput), szTemp, res);
					DisplayWindow_BufferText(m_hDisplayWindow, szOutput);

					res = pimg->GetVerticalResolution();
					LoadString(m_hLanguageModule, IDS_GENERAL_DISPLAYWINDOW_VERTICALRESOLUTION, szTemp, SIZEOF_ARRAY(szTemp));
					StringCchPrintf(szOutput, SIZEOF_ARRAY(szOutput), szTemp, res);
					DisplayWindow_BufferText(m_hDisplayWindow, szOutput);
				}

				delete pimg;
			}

			/* Only attempt to show file previews for files (not folders). Also, only
			attempt to show a preview if the display window is actually active. */
			if (((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) !=
				FILE_ATTRIBUTE_DIRECTORY) && m_bShowFilePreviews
				&& m_bShowDisplayWindow)
			{
				DisplayWindow_SetThumbnailFile(m_hDisplayWindow, szFullItemName, TRUE);
			}
			else
			{
				DisplayWindow_SetThumbnailFile(m_hDisplayWindow, EMPTY_STRING, FALSE);
			}
		}
		else
		{
			m_pActiveShellBrowser->QueryFullItemName(iSelected, szFullItemName, SIZEOF_ARRAY(szFullItemName));

			if (PathIsRoot(szFullItemName))
			{
				TCHAR szMsg[64];
				TCHAR szTemp[64];
				ULARGE_INTEGER ulTotalNumberOfBytes;
				ULARGE_INTEGER ulTotalNumberOfFreeBytes;
				BOOL bRet = GetDiskFreeSpaceEx(szFullItemName, NULL, &ulTotalNumberOfBytes, &ulTotalNumberOfFreeBytes);

				if (bRet)
				{
					TCHAR szSize[32];
					FormatSizeString(ulTotalNumberOfFreeBytes, szSize, SIZEOF_ARRAY(szSize));
					LoadString(m_hLanguageModule, IDS_GENERAL_DISPLAY_WINDOW_FREE_SPACE, szTemp, SIZEOF_ARRAY(szTemp));
					StringCchPrintf(szMsg, SIZEOF_ARRAY(szMsg), szTemp, szSize);
					DisplayWindow_BufferText(m_hDisplayWindow, szMsg);

					FormatSizeString(ulTotalNumberOfBytes, szSize, SIZEOF_ARRAY(szSize));
					LoadString(m_hLanguageModule, IDS_GENERAL_DISPLAY_WINDOW_TOTAL_SIZE, szTemp, SIZEOF_ARRAY(szTemp));
					StringCchPrintf(szMsg, SIZEOF_ARRAY(szMsg), szTemp, szSize);
					DisplayWindow_BufferText(m_hDisplayWindow, szMsg);
				}

				TCHAR szFileSystem[MAX_PATH + 1];
				bRet = GetVolumeInformation(szFullItemName, NULL, 0, NULL, NULL, NULL, szFileSystem, SIZEOF_ARRAY(szFileSystem));

				if (bRet)
				{
					LoadString(m_hLanguageModule, IDS_GENERAL_DISPLAY_WINDOW_FILE_SYSTEM, szTemp, SIZEOF_ARRAY(szTemp));
					StringCchPrintf(szMsg, SIZEOF_ARRAY(szMsg), szTemp, szFileSystem);
					DisplayWindow_BufferText(m_hDisplayWindow, szMsg);
				}
			}
		}
	}
}

void Explorerplusplus::UpdateDisplayWindowForMultipleFiles(void)
{
	TCHAR			szNumSelected[64] = EMPTY_STRING;
	TCHAR			szTotalSize[64] = EMPTY_STRING;
	TCHAR			szTotalSizeFragment[32] = EMPTY_STRING;
	TCHAR			szMore[64];
	TCHAR			szTotalSizeString[64];
	FolderInfo_t	FolderInfo;
	int				nSelected;

	DisplayWindow_SetThumbnailFile(m_hDisplayWindow, EMPTY_STRING, FALSE);

	nSelected = m_pActiveShellBrowser->QueryNumSelected();

	LoadString(m_hLanguageModule, IDS_GENERAL_SELECTED_MOREITEMS,
		szMore, SIZEOF_ARRAY(szMore));

	StringCchPrintf(szNumSelected, SIZEOF_ARRAY(szNumSelected),
		_T("%d %s"), nSelected, szMore);

	DisplayWindow_BufferText(m_hDisplayWindow, szNumSelected);

	if (!m_pActiveShellBrowser->InVirtualFolder())
	{
		m_pActiveShellBrowser->QueryFolderInfo(&FolderInfo);

		FormatSizeString(FolderInfo.TotalSelectionSize, szTotalSizeFragment,
			SIZEOF_ARRAY(szTotalSizeFragment), m_bForceSize, m_SizeDisplayFormat);

		LoadString(m_hLanguageModule, IDS_GENERAL_TOTALFILESIZE,
			szTotalSizeString, SIZEOF_ARRAY(szTotalSizeString));

		StringCchPrintf(szTotalSize, SIZEOF_ARRAY(szTotalSize),
			_T("%s: %s"), szTotalSizeString, szTotalSizeFragment);
	}

	DisplayWindow_BufferText(m_hDisplayWindow, szTotalSize);
}