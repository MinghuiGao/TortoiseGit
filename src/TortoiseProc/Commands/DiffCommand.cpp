// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN
// Copyright (C) 2008-2017 - TortoiseGit

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
#include "DiffCommand.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "ChangedDlg.h"
#include "GitDiff.h"
#include "../TGitCache/CacheInterface.h"
#include "UnicodeUtils.h"

bool DiffCommand::Execute()
{
	bool bRet = false;
	CString path2 = CPathUtils::GetLongPathname(parser.GetVal(L"path2"));
	bool bAlternativeTool = !!parser.HasKey(L"alternative");
//	bool bBlame = !!parser.HasKey(L"blame");
	if (path2.IsEmpty())
	{
		if (this->orgCmdLinePath.IsDirectory())
		{
			CChangedDlg dlg;
			dlg.m_pathList = CTGitPathList(cmdLinePath);
			dlg.DoModal();
			bRet = true;
		}
		else
		{
			if (cmdLinePath.IsEmpty())
				return false;
			//diff.SetAlternativeTool(bAlternativeTool);
			if (parser.HasKey(L"startrev") && parser.HasKey(L"endrev"))
			{
				if (parser.HasKey(L"unified"))
					bRet = !!CAppUtils::StartShowUnifiedDiff(nullptr, cmdLinePath, parser.GetVal(L"endrev"), cmdLinePath, parser.GetVal(L"startrev"), bAlternativeTool);
				else
					bRet = !!CGitDiff::Diff(&cmdLinePath, &cmdLinePath, parser.GetVal(L"startrev"), parser.GetVal(L"endrev"), false, parser.HasKey(L"unified") == TRUE, parser.GetLongVal(L"line"), bAlternativeTool);
			}
			else
			{
				// check if it is a newly added (but uncommitted) file
				unsigned int status_flags = 0;
				CAutoRepository repo(g_Git.GetGitRepository());
				if (repo)
					git_status_file(&status_flags, repo, CUnicodeUtils::GetUTF8(cmdLinePath.GetWinPathString()));
				if (status_flags == GIT_STATUS_INDEX_NEW)
				{
					if (!g_Git.IsInitRepos())
					{
						// this might be a rename, try to find original name
						BYTE_VECTOR cmdout;
						g_Git.Run(L"git.exe diff-index --raw HEAD -M -C -z --", &cmdout);
						CTGitPathList changedFiles;
						changedFiles.ParserFromLog(cmdout);
						for (int i = 0; i < changedFiles.GetCount(); ++i)
						{
							if (changedFiles[i].GetGitPathString() == cmdLinePath.GetGitPathString())
							{
								if (!changedFiles[i].GetGitOldPathString().IsEmpty())
								{
									CTGitPath oldPath(changedFiles[i].GetGitOldPathString());
									if (parser.HasKey(L"unified"))
										return !!CAppUtils::StartShowUnifiedDiff(nullptr, cmdLinePath, L"HEAD", cmdLinePath, GIT_REV_ZERO, bAlternativeTool);
									return !!CGitDiff::Diff(&cmdLinePath, &oldPath, GIT_REV_ZERO, L"HEAD", false, parser.HasKey(L"unified") == TRUE, parser.GetLongVal(L"line"), bAlternativeTool);
								}
								break;
							}
						}
					}
					if (parser.HasKey(L"unified"))
					{
						cmdLinePath.m_Action = cmdLinePath.LOGACTIONS_ADDED;
						return !!CAppUtils::StartShowUnifiedDiff(nullptr, cmdLinePath, L"HEAD", cmdLinePath, GIT_REV_ZERO, bAlternativeTool);
					}
					else
						return !!CGitDiff::DiffNull(&cmdLinePath, GIT_REV_ZERO, true, parser.GetLongVal(L"line"), bAlternativeTool);
				}

				if (parser.HasKey(L"unified"))
					bRet = !!CAppUtils::StartShowUnifiedDiff(nullptr, cmdLinePath, L"HEAD", cmdLinePath, GIT_REV_ZERO, bAlternativeTool);
				else
					bRet = !!CGitDiff::Diff(&cmdLinePath, &cmdLinePath, GIT_REV_ZERO, L"HEAD", false, parser.HasKey(L"unified") == TRUE, parser.GetLongVal(L"line"), bAlternativeTool);
			}
		}
	}
	else
	{
		if (parser.HasKey(L"startrev") && parser.HasKey(L"endrev") && CStringUtils::StartsWith(path2, g_Git.m_CurrentDir + L"\\"))
		{
			CTGitPath tgitPath2 = path2.Mid(g_Git.m_CurrentDir.GetLength() + 1);
			bRet = !!CGitDiff::Diff(&tgitPath2, &cmdLinePath, parser.GetVal(L"startrev"), parser.GetVal(L"endrev"), false, parser.HasKey(L"unified") == TRUE, parser.GetLongVal(L"line"), bAlternativeTool);
		}
		else
		{
			bRet = CAppUtils::StartExtDiff(
				path2, orgCmdLinePath.GetWinPathString(), CString(), CString(),
				CString(), CString(), GIT_REV_ZERO, GIT_REV_ZERO,
				CAppUtils::DiffFlags().AlternativeTool(bAlternativeTool), parser.GetLongVal(L"line"));
		}
	}

	return bRet;
}
