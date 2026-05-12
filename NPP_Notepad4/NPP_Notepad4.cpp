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


// NPP_Notepad4.cpp : DLL의 초기화 루틴을 정의함
/////////////////////////////////////////////////////////

#include "pch.h"
#include "framework.h"
#include "NPP_Notepad4.h"

#include "AlignFeatures.h"
#include "KoreanFeatures.h"
#include "ToolFeatures.h"

// 복사해 넣은 NPP 헤더들
#include "PluginInterface.h"
#include "Notepad_plus_msgs.h" // nppData 구조체가 들어있음
#include "Scintilla.h"

// --- 전역 변수 ---
HINSTANCE g_hInst = NULL;
NppData nppData;
FuncItem funcItem[100];
int g_funcCount = 0;
bool g_isKorean = false;
int g_cachedLangType = 0;

// 서브 메뉴 위치 기록용
int g_posTextTrans = -1;
int g_posWebTools = -1;

// --- [1] DLL 진입점 (DllMain) ---

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hInst = hModule;
        DisableThreadLibraryCalls(hModule);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

// --- [2] 다국어 및 메뉴 보조 로직 ---

void DetectLanguage()
{
    size_t len = (size_t)::SendMessage(nppData._nppHandle, NPPM_GETNATIVELANGFILENAME, 0, 0);
    if (len > 0)
    {
        std::vector<char> langFile(len + 1);
        ::SendMessage(nppData._nppHandle, NPPM_GETNATIVELANGFILENAME, (WPARAM)(len + 1), (LPARAM)langFile.data());

        _strlwr_s(langFile.data(), len + 1);
        g_isKorean = (strstr(langFile.data(), "korean") != nullptr);
    }
    else g_isKorean = false;
}

enum class MenuType { PluginName, Item, Separator, SubHeader };
struct MenuEntry {
    const TCHAR* eng;
    const TCHAR* kor;
    PFUNCPLUGINCMD pFunc;
    MenuType type;
    int* pPosOut;
};

static const MenuEntry g_menuTable[] = {
    // [0] 간판
    { _T("BLUEnLIVE's Notepad4"), _T("BLUEnLIVE의 Notepad4"), nullptr, MenuType::PluginName, nullptr },

    // [1~3] 일반 아이템
    { _T("Alig&n Lines..."),      _T("좌우 정렬(&N)..."),     DoAlignDlg,    MenuType::Item, nullptr },
    { _T("---"),                  nullptr,                    nullptr,       MenuType::Separator, nullptr },
    { _T("&Calculate Expression"), _T("수식 계산(&C)"),       DoCalculate,   MenuType::Item, nullptr },

    // [4] 서브 메뉴 헤더 1
    { _T("Text &Transliteration"), _T("텍스트 변환(&T)"),     nullptr,       MenuType::SubHeader, &g_posTextTrans },

    // [5~8] 서브 메뉴 아이템들
    { _T("Korean Han&ja to Hangul"), _T("한국어 한자를 한글로(&J)"), DoHanjaToHangul, MenuType::Item, nullptr },
    { _T("Korean Han&gul Decomposition"), _T("한글을 풀어쓰기로(&G)"), DoHangulDecomp, MenuType::Item, nullptr },
    { _T("Toggle Unicode Korean &Composition"), _T("유니코드 한글 풀어쓰기↔모아쓰기(&C)"), DoToggleComposition, MenuType::Item, nullptr },
    { _T("&KSSM to Korean Wansung (Entire File)"), _T("조합형 한글을 완성형으로 (문서 전체)(&K)"), DoKssmToWansung, MenuType::Item, nullptr },

    // [9] 서브 메뉴 헤더 2
    { _T("&Web Tools"),           _T("웹 개발도구(&W)"),      nullptr,       MenuType::SubHeader, &g_posWebTools },

    // [10~12] 서브 메뉴 아이템들
    { _T("&Evaluate JS Expression"), _T("JS 표현식 평가(&E)"),  DoEvalJS,      MenuType::Item, nullptr },
    { _T("&Remove HTML/XML Tags"), _T("HTML/XML 태그 삭제(&R)"), DoRemoveTags,  MenuType::Item, nullptr },
    { _T("Remove C&omments (HTML/C++/Py)"), _T("주석 삭제 (HTML/C++/파이썬)(&O)"), DoRemoveComments, MenuType::Item, nullptr },

    // [13~14] 하단 아이템
    { _T("---"),                  nullptr,                    nullptr,       MenuType::Separator },
    { _T("About"),                _T("정보"),                 DoAboutDlg,    MenuType::Item },
};

struct TrEntry {
    const TCHAR* eng;
    const TCHAR* kor;
};

static const TrEntry g_uiTrTable[] = {
    { _T("Align Lines"),                     _T("좌우 정렬"), },
    { _T("&Left."),                          _T("왼쪽 정렬(&L)"), },
    { _T("&Right."),                         _T("오른쪽 정렬(&R)"), },
    { _T("&Center."),                        _T("가운데 정렬(&C)"), },
    { _T("&Justify."),                       _T("양쪽 정렬(&J)"), },
    { _T("Justify (&Paragraph mode)."),      _T("마지막 행 제외하고 양쪽 정렬(&P)"), },
    { _T("OK"),                              _T("확인"), },
    { _T("Cancel"),                          _T("취소"), },
};

const TCHAR* GetTr(const TCHAR* engKey) {
    if (!g_isKorean) return engKey;

    // 1. 먼저 UI 전용 테이블에서 검색
    for (const auto& item : g_uiTrTable) {
        if (_tcscmp(item.eng, engKey) == 0) return item.kor;
    }

    // 2. 없으면 메뉴 테이블에서 검색
    for (const auto& item : g_menuTable) {
        if (_tcscmp(item.eng, engKey) == 0) return item.kor ? item.kor : item.eng;
    }

    return engKey; // 둘 다 없으면 기본값(영어) 반환
}

// --- [3] Notepad++ 인터페이스 구현 ---

extern "C" __declspec(dllexport) void setInfo(NppData notepadPlusData) {
    nppData = notepadPlusData;

    // 초기 메뉴 등록
    g_funcCount = 0;
    for (const auto& entry : g_menuTable) {
        // 실제 '메뉴 명령'이 되는 Item과 Separator만 등록
        if (entry.type == MenuType::Item || entry.type == MenuType::Separator) {

            // 안전 장치: 메뉴 아이템인데 실행 함수가 없는 경우는 제외 (Separator 제외)
            if (entry.type == MenuType::Item && entry.pFunc == nullptr) continue;

            if (entry.type == MenuType::Separator) {
                funcItem[g_funcCount]._itemName[0] = _T('\0');
                funcItem[g_funcCount]._pFunc = NULL;
            }
            else {
                _tcscpy_s(funcItem[g_funcCount]._itemName, _countof(funcItem[g_funcCount]._itemName), entry.eng);
                funcItem[g_funcCount]._pFunc = entry.pFunc;
            }

            funcItem[g_funcCount]._init2Check = false;
            funcItem[g_funcCount]._pShKey = NULL;
            g_funcCount++;
        }
    }
}

extern "C" __declspec(dllexport) const TCHAR* getName() {
    return GetTr(g_menuTable[0].eng);
}

extern "C" __declspec(dllexport) FuncItem* getFuncsArray(int* nbF) {
    *nbF = g_funcCount;
    return funcItem;
}

extern "C" __declspec(dllexport) BOOL isUnicode() { return TRUE; }

extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam) { return TRUE; }

// --- [4] 메뉴 재구성 및 상태 업데이트 (Win32 API 기반) ---

void RestructureMenu() {
    DetectLanguage();
    HMENU hPluginsMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0);
    if (!hPluginsMenu) return;

    int pos = -1;
    int count = GetMenuItemCount(hPluginsMenu);
    TCHAR buf[256];
    for (int i = 0; i < count; i++) {
        GetMenuString(hPluginsMenu, i, buf, 255, MF_BYPOSITION);
        if (_tcscmp(buf, g_menuTable[0].eng) == 0 || (g_menuTable[0].kor && _tcscmp(buf, g_menuTable[0].kor) == 0)) {
            pos = i; break;
        }
    }
    if (pos == -1) return;

    // 간판 업데이트
    ::ModifyMenu(hPluginsMenu, pos, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hPluginsMenu, pos), getName());
    HMENU hMyMenu = GetSubMenu(hPluginsMenu, pos);
    if (!hMyMenu) return;

    // 아이템 이름 업데이트
    for (int i = 0; i < g_funcCount; i++) {
        if (funcItem[i]._pFunc != nullptr) {
            ::ModifyMenu(hMyMenu, funcItem[i]._cmdID, MF_BYCOMMAND | MF_STRING, funcItem[i]._cmdID, GetTr(funcItem[i]._itemName));
        }
    }

    // 서브 메뉴 재구성
    int funcIdxCounter = 0;
    int topLevelItemPos = 0;
    bool isInsideSubGroup = false;

    for (int i = 1; i < (int)_countof(g_menuTable); i++) {
        const auto& entry = g_menuTable[i];
        if (entry.type == MenuType::SubHeader) {
            int startIdx = funcIdxCounter;
            int memberCount = 0;
            for (int j = i + 1; j < (int)_countof(g_menuTable); j++) {
                if (g_menuTable[j].type == MenuType::Item) memberCount++;
                else break;
            }
            int endIdx = startIdx + memberCount - 1;

            bool isFound = false;
            int currentSubCount = GetMenuItemCount(hMyMenu);
            for (int k = 0; k < currentSubCount; k++) {
                GetMenuString(hMyMenu, k, buf, 255, MF_BYPOSITION);
                if (_tcscmp(buf, entry.eng) == 0 || (entry.kor && _tcscmp(buf, entry.kor) == 0)) {
                    ::ModifyMenu(hMyMenu, k, MF_BYPOSITION | MF_POPUP, (UINT_PTR)GetSubMenu(hMyMenu, k), GetTr(entry.eng));
                    isFound = true; break;
                }
            }

            if (!isFound) {
                HMENU hNewSubMenu = CreatePopupMenu();
                for (int m = startIdx; m <= endIdx; m++) {
                    if (m >= 0 && m < g_funcCount) {
                        int id = funcItem[m]._cmdID;
                        GetMenuString(hMyMenu, id, buf, 255, MF_BYCOMMAND);
                        AppendMenu(hNewSubMenu, MF_STRING, id, buf);
                        DeleteMenu(hMyMenu, id, MF_BYCOMMAND);
                    }
                }
                InsertMenu(hMyMenu, topLevelItemPos, MF_BYPOSITION | MF_POPUP, (UINT_PTR)hNewSubMenu, GetTr(entry.eng));
            }
            if (entry.pPosOut) *(entry.pPosOut) = topLevelItemPos;
            topLevelItemPos++;
            isInsideSubGroup = true;
        }
        else if (entry.type == MenuType::Separator) {
            if (!isInsideSubGroup) topLevelItemPos++;
            funcIdxCounter++;
            isInsideSubGroup = false;
        }
        else if (entry.type == MenuType::Item) {
            if (!isInsideSubGroup) topLevelItemPos++;
            funcIdxCounter++;
        }
    }
}

void UpdateMenuState() {
    const int whichView = (int)::SendMessage(nppData._nppHandle, NPPM_GETCURRENTVIEW, 0, 0);
    const HWND hSci = (whichView == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
    if (!hSci) return;

    int langType = 0;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTLANGTYPE, 0, (LPARAM)&langType);

    // 1. Notepad++가 인지하는 인코딩 형식 확인용 (0: ANSI)
    int nppEncoding = 0;
    ::SendMessage(nppData._nppHandle, NPPM_GETBUFFERENCODING, 0, (LPARAM)&nppEncoding);

    // 2. Scintilla가 사용하는 실제 코드페이지 확인용 (949: Korean ANSI)
    const int cp = (int)::SendMessage(hSci, SCI_GETCODEPAGE, 0, 0);

    const Sci_Position len = ::SendMessage(hSci, SCI_GETLENGTH, 0, 0);
    const Sci_Position selStart = (Sci_Position)::SendMessage(hSci, SCI_GETSELECTIONSTART, 0, 0);
    const Sci_Position selEnd = (Sci_Position)::SendMessage(hSci, SCI_GETSELECTIONEND, 0, 0);
    const bool hasSelection = (selStart != selEnd);
    const Sci_Position startLine = (Sci_Position)::SendMessage(hSci, SCI_LINEFROMPOSITION, selStart, 0);
    const Sci_Position endLine = (Sci_Position)::SendMessage(hSci, SCI_LINEFROMPOSITION, selEnd, 0);
    const bool isMultiLine = hasSelection && (endLine > startLine);

    const HMENU hPluginsMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0);
    if (!hPluginsMenu) return;

    int myPos = -1;
    const int count = GetMenuItemCount(hPluginsMenu);
    TCHAR buf[256];
    for (int i = 0; i < count; i++) {
        GetMenuString(hPluginsMenu, i, buf, 255, MF_BYPOSITION);
        if (_tcscmp(buf, getName()) == 0) { myPos = i; break; }
    }
    if (myPos == -1) return;
    const HMENU hMyMenu = GetSubMenu(hPluginsMenu, myPos);

    auto setItem = [&](int idx, bool en) {
        ::EnableMenuItem(hMyMenu, funcItem[idx]._cmdID, MF_BYCOMMAND | (en ? MF_ENABLED : MF_GRAYED));
        };

    setItem(0, isMultiLine);    // 좌우 정렬
    setItem(2, hasSelection);   // 수식 계산
    setItem(3, hasSelection);   // 한자->한글
    setItem(4, hasSelection);   // 한글 풀어쓰기
    setItem(5, hasSelection);   // 유니코드 한글 조합<->풀어쓰기
    setItem(6, (nppEncoding == 0 && cp == 949 && len > 0)); // 조합형->완성형 (문서 전체)
    setItem(7, hasSelection);   // JS 표현식 평가
    setItem(8, hasSelection && (langType == L_TEXT || langType == L_HTML || langType == L_XML));    // HTML/XML 태그 삭제
    setItem(9, hasSelection && (langType != L_TEXT));   // 주석 삭제 (HTML/C++/Py)
    setItem(11, true);  // About

    if (g_posTextTrans != -1) {
        const bool canKssm = (nppEncoding == 0 && cp == 949 && len > 0);
        const bool isParentEnabled = hasSelection || canKssm;
        ::EnableMenuItem(hMyMenu, g_posTextTrans, MF_BYPOSITION | (isParentEnabled ? MF_ENABLED : MF_GRAYED));
    }
    if (g_posWebTools != -1)  ::EnableMenuItem(hMyMenu, g_posWebTools, MF_BYPOSITION | (hasSelection ? MF_ENABLED : MF_GRAYED));
}

extern "C" __declspec(dllexport) void beNotified(SCNotification* notifyCode) {
    switch (notifyCode->nmhdr.code) {
    case NPPN_READY:
        DetectLanguage();
        ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTLANGTYPE, 0, (LPARAM)&g_cachedLangType);
        RestructureMenu();
        UpdateMenuState();
        break;
    case NPPN_NATIVELANGCHANGED:
        DetectLanguage(); RestructureMenu(); UpdateMenuState(); break;
    case NPPN_BUFFERACTIVATED:
    case NPPN_LANGCHANGED:
        ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTLANGTYPE, 0, (LPARAM)&g_cachedLangType);
        UpdateMenuState(); break;
    case SCN_UPDATEUI:
        UpdateMenuState(); break;
    }
}

// About 다이얼로그의 메시지를 처리하는 콜백 함수
INT_PTR CALLBACK AboutDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        // 0. 번역 적용
        SetDlgItemText(hwnd, IDOK, GetTr(_T("OK")));
        SetDlgItemText(hwnd, IDCANCEL, GetTr(_T("Cancel")));

        // 1. 부모 창(Notepad++)과 내 창(About)의 좌표 정보를 가져옴
        HWND hwndParent = GetParent(hwnd);
        if (hwndParent) {
            RECT rcParent, rcWindow;
            GetWindowRect(hwndParent, &rcParent);
            GetWindowRect(hwnd, &rcWindow);

            int parentWidth = rcParent.right - rcParent.left;
            int parentHeight = rcParent.bottom - rcParent.top;
            int windowWidth = rcWindow.right - rcWindow.left;
            int windowHeight = rcWindow.bottom - rcWindow.top;

            // 2. 부모 창의 정중앙 좌표를 계산함
            int x = rcParent.left + (parentWidth - windowWidth) / 2;
            int y = rcParent.top + (parentHeight - windowHeight) / 2;

            // 3. 계산된 위치로 다이얼로그를 이동시킴
            SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hwnd, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// 메뉴에서 'About'을 클릭했을 때 호출되는 함수
void DoAboutDlg()
{
    // g_hInst: dllmain에서 저장한 인스턴스 핸들
    // IDD_DIALOG_ABOUT: resource.h에 정의된 다이얼로그 ID
    // nppData._nppHandle: Notepad++ 메인 윈도우 핸들 (Parent)
    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_ABOUT), nppData._nppHandle, AboutDlgProc);
}

