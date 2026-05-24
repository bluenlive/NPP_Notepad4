// This file is part of NPP_Notepad4 project
// Copyright (C)2026 BLUEnLIVE (https://teus.me)

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

/*****************************************************************************
* ​ ​ ​______ ​_____ ​ ​ ​_______ ​_______ ​ ​ ​ ​ ​ ​ ​ ​ ​_____ ​ ​ ​_______ ​___ ​___ ​_______ ​ ​ *
* ​ ​| ​ ​ ​__ ​\ ​ ​ ​ ​ ​|_| ​ ​ ​| ​ ​ ​| ​ ​ ​ ​___|.-----.| ​ ​ ​ ​ ​|_|_ ​ ​ ​ ​ ​_| ​ ​ ​| ​ ​ ​| ​ ​ ​ ​___| ​ *
* ​ ​| ​ ​ ​__ ​< ​ ​ ​ ​ ​ ​ ​| ​ ​ ​| ​ ​ ​| ​ ​ ​ ​___|| ​ ​ ​ ​ ​|| ​ ​ ​ ​ ​ ​ ​|_| ​ ​ ​|_| ​ ​ ​| ​ ​ ​| ​ ​ ​ ​___| ​ *
* ​ ​|______/_______|_______|_______||__|__||_______|_______|\_____/|_______| ​ *
* ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ ​ *
*****************************************************************************/


#include "pch.h"
#include "NPP_Notepad4.h"
#include <string>
#include "FindFeatures.h"
#include "Common.h"

#include "NppResourceID.h"
#include <CommCtrl.h>
#pragma comment(lib, "comctl32.lib")

namespace {

    std::vector<std::wstring> g_savedSearchHistory;
    std::wstring g_savedWindowTitle;
    bool g_isHistorySaved = false;

    // 💡 찾기 창의 메시지를 감시하다가 닫히는 순간 히스토리를 복원하는 콜백 함수
    LRESULT CALLBACK FindDlgSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/)
    {
        // WM_SHOWWINDOW에서 wParam이 FALSE이면 창이 숨겨지는(사용자가 닫는) 순간임
        if (uMsg == WM_SHOWWINDOW && wParam == FALSE) {
            if (g_isHistorySaved) {
                HWND hComboFind = ::GetDlgItem(hWnd, IDFINDWHAT);
                if (hComboFind) {
                    // 1. 임시 프리셋 리스트를 초기화
                    ::SendMessage(hComboFind, CB_RESETCONTENT, 0, 0);

                    // 2. 백업해 두었던 원래의 히스토리를 그대로 복원
                    for (const auto& text : g_savedSearchHistory) {
                        ::SendMessage(hComboFind, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
                    }

                    // 3. 복원 후 첫 번째 항목을 다시 선택 상태로 설정
                    if (!g_savedSearchHistory.empty()) {
                        ::SendMessage(hComboFind, CB_SETCURSEL, 0, 0);
                        ::SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDFINDWHAT, CBN_SELCHANGE), reinterpret_cast<LPARAM>(hComboFind));
                    }
                }

                // 창이 닫힐 때 백업해뒀던 원래 제목(Find 또는 Replace 등)으로 복원
                if (!g_savedWindowTitle.empty()) {
                    ::SetWindowTextW(hWnd, g_savedWindowTitle.c_str());
                    g_savedWindowTitle.clear();
                }

                // 메모리 정리 및 플래그 초기화
                g_savedSearchHistory.clear();
                g_isHistorySaved = false;
            }
            // 복원 임무를 완수했으므로 감시카메라(서브클래스)를 제거
            ::RemoveWindowSubclass(hWnd, FindDlgSubclassProc, uIdSubclass);
        }
        return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

}

void DoInjectRegexPresets()
{
    // 1. NPP의 찾기 창 호출 (IDM_SEARCH_FIND = 43001)
    ::SendMessage(nppData._nppHandle, WM_COMMAND, IDM_SEARCH_FIND, 0);

    // 2. Notepad++ 메인 윈도우가 소유한 찾기 다이얼로그 핸들 탐색
    HWND hFindDlg = nullptr;
    HWND hCurrent = ::GetWindow(::GetDesktopWindow(), GW_CHILD);
    while (hCurrent) {
        TCHAR className[256];
        ::GetClassNameW(hCurrent, className, _countof(className));

        if (::_tcscmp(className, L"#32770") == 0 && ::GetWindow(hCurrent, GW_OWNER) == nppData._nppHandle) {
            // 순정 이름인 IDFINDWHAT 컨트롤이 존재하는지 검증
            if (::GetDlgItem(hCurrent, IDFINDWHAT) != nullptr) {
                hFindDlg = hCurrent;
                break;
            }
        }
        hCurrent = ::GetWindow(hCurrent, GW_HWNDNEXT);
    }

    if (!hFindDlg) return;

    // 3. '찾을 내용' 콤보박스 핸들 획득 (IDC_FIND_COMBO_TEXT = 1601)
    const HWND hComboFind = ::GetDlgItem(hFindDlg, IDFINDWHAT);
    if (!hComboFind) return;

    // 아직 백업된 히스토리가 없다면, 원래 들어있던 검색 목록을 모두 저장
    if (!g_isHistorySaved) {
        g_savedSearchHistory.clear();
        int count = static_cast<int>(::SendMessage(hComboFind, CB_GETCOUNT, 0, 0));
        for (int i = 0; i < count; ++i) {
            int len = static_cast<int>(::SendMessage(hComboFind, CB_GETLBTEXTLEN, i, 0));
            if (len != CB_ERR) {
                std::vector<TCHAR> buf(len + 1);
                ::SendMessage(hComboFind, CB_GETLBTEXT, i, reinterpret_cast<LPARAM>(buf.data()));
                g_savedSearchHistory.push_back(buf.data());
            }
        }

        // 원래 창 제목(텍스트) 백업
        int titleLen = ::GetWindowTextLengthW(hFindDlg);
        if (titleLen > 0) {
            std::vector<TCHAR> titleBuf(titleLen + 1);
            ::GetWindowTextW(hFindDlg, titleBuf.data(), titleLen + 1);
            g_savedWindowTitle = titleBuf.data();
        }

        g_isHistorySaved = true;

        // 💡 찾기 창에 감시카메라를 달아 창이 닫힐 때 위의 복원 함수가 실행되도록 묶어줌
        ::SetWindowSubclass(hFindDlg, FindDlgSubclassProc, SUBCLASS_ID_FIND_DLG, 0);
    }

    ::SetWindowTextW(hFindDlg, GetTr(_T("Regex Preset Find")));

    // Notepad4에 추가한 내용과 동일한 프리셋 목록
    const std::vector<std::wstring> regexPresets = {
        L"[^\\x01-\\x7e]",
        L"[ \\t]+$",
        L"^[ \\t]+$",
        L"(\\r?\\n){2,}",
        L"\\bhttps?://[^\\s/]+(?:/[^\\s]*)?\\b",
        L"\\b[^@\\s]+@[^@\\s.]+(?:\\.[^@\\s.]+)+\\b",
        L"[가-힣ㄱ-ㅎㅏ-ㅣ]",
        L"([\\x{2600}-\\x{27BF}]|.[\\x{DC00}-\\x{DFFF}])",
        L"//.*?([\\x{2600}-\\x{27BF}]|.[\\x{DC00}-\\x{DFFF}])",
    };

    // 4. 기존 히스토리 싹 비우고 새 프리셋 깔끔하게 적재
    ::SendMessage(hComboFind, CB_RESETCONTENT, 0, 0);
    for (const auto& regex : regexPresets) {
        ::SendMessage(hComboFind, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(regex.c_str()));
    }

    // 5. 첫 번째 프리셋 항목을 기본 선택 후, Notepad++ 본체에 변경 통지(CBN_SELCHANGE) 전송
    //    이 통지를 보내야 Notepad++ 내부 검색 엔진 버퍼가 프리셋 문자열을 인식함
    ::SendMessage(hComboFind, CB_SETCURSEL, 0, 0);
    ::SendMessage(hFindDlg, WM_COMMAND, MAKEWPARAM(IDFINDWHAT, CBN_SELCHANGE), reinterpret_cast<LPARAM>(hComboFind));

    // 6. 검색 모드를 '정규식' 라디오 버튼(IDC_FIND_RADIO_REGEX = 1625)으로 강제 전환 및 클릭 이벤트 트리거
    const HWND hRadioRegex = ::GetDlgItem(hFindDlg, IDREGEXP);
    if (hRadioRegex) {
        ::SendMessage(hRadioRegex, BM_SETCHECK, BST_CHECKED, 0);
        ::SendMessage(hFindDlg, WM_COMMAND, MAKEWPARAM(IDREGEXP, BN_CLICKED), reinterpret_cast<LPARAM>(hRadioRegex));
    }
}
