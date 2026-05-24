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
#include <vector>
#include <string>
#include <iterator>
#include <map>

#include "AlignFeatures.h"
#include "FindFeatures.h"
#include "KoreanFeatures.h"
#include "ToolFeatures.h"

// 복사해 넣은 NPP 헤더들
#include "PluginInterface.h"
#include "Notepad_plus_msgs.h" // nppData 구조체가 들어있음
#include "Scintilla.h"

// 새로운 정규식 주입 기능 함수 선언 외부 참조

// --- 전역 변수 ---
HINSTANCE g_hInst = nullptr;
NppData nppData;
FuncItem funcItem[100];
int g_funcCount = 0;
bool g_isKorean = false;
int g_cachedLangType = 0;

std::map<std::wstring, int> g_subMenuPositions;

// --- [1] DLL 진입점 (DllMain) ---

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hInst = hModule;
        ::DisableThreadLibraryCalls(hModule);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

// --- [2] 다국어 및 메뉴 보조 로직 ---

void DetectLanguage()
{
    const size_t len = static_cast<size_t>(::SendMessage(nppData._nppHandle, NPPM_GETNATIVELANGFILENAME, 0, 0));
    if (len > 0)
    {
        std::vector<char> langFile(len + 1);
        ::SendMessage(nppData._nppHandle, NPPM_GETNATIVELANGFILENAME, static_cast<WPARAM>(len + 1), reinterpret_cast<LPARAM>(langFile.data()));

        ::_strlwr_s(langFile.data(), len + 1);
        g_isKorean = (::strstr(langFile.data(), "korean") != nullptr);
    }
    else g_isKorean = false;
}

enum class MenuType { PluginName, Item, Separator, SubHeader, SubEnd };
struct MenuEntry {
    const TCHAR* eng;
    const TCHAR* kor;
    PFUNCPLUGINCMD pFunc;
    MenuType type;
};

// 서브 메뉴 헤더가 나오면 이후 메뉴는 서브 메뉴의 하위 메뉴
// 이것은 Separator(구분선) 또는 SubEnd가 나올 때까지 계속됨
static const MenuEntry g_menuTable[] = {
    // [0] 간판
    { _T("BLUE's Notepad4"), _T("BLUE의 Notepad4"), nullptr, MenuType::PluginName },

    // 서브 메뉴 헤더 1: 편집
    { _T("&Edit"), _T("편집(&E)"),     nullptr,       MenuType::SubHeader },

    // 서브 메뉴 아이템들 (편집)
    { _T("Alig&n Lines..."),      _T("좌우 정렬(&N)..."),     DoAlignDlg,    MenuType::Item },
    
    // 서브 메뉴 헤더 2: 찾기
    { _T("&Find"), _T("찾기(&F)"),     nullptr,       MenuType::SubHeader },

    // 서브 메뉴 아이템들 (찾기)
    { _T("Find with &Regex Presets..."), _T("정규식 프리셋으로 찾기(&R)..."), DoInjectRegexPresets, MenuType::Item },

    // 서브 메뉴 헤더 3: 선택 영역에 대하여...
    { _T("Action &on Selection"), _T("선택영역에 대하여...(&O)"), nullptr, MenuType::SubHeader },

    // 서브 메뉴 아이템들 (선택 영역에 대하여...)
    { _T("&Calculate Expression"), _T("수식 계산(&C)"),        DoCalculate,  MenuType::Item },

    // 서브 메뉴 헤더 4: 텍스트 변환
    { _T("Text &Transliteration"), _T("텍스트 변환(&T)"),     nullptr,       MenuType::SubHeader },

    // 서브 메뉴 아이템들 (텍스트 변환)
    { _T("Korean Han&ja to Hangul"), _T("한국어 한자를 한글로(&J)"), DoHanjaToHangul, MenuType::Item },
    { _T("Korean Han&gul Decomposition"), _T("한글을 풀어쓰기로(&G)"), DoHangulDecomp, MenuType::Item },
    { _T("Toggle Unicode Korean &Composition"), _T("유니코드 한글 풀어쓰기↔모아쓰기(&C)"), DoToggleComposition, MenuType::Item },
    { _T("&KSSM to Korean Wansung (Entire File)"), _T("조합형 한글을 완성형으로 (문서 전체)(&K)"), DoKssmToWansung, MenuType::Item },

    // 서브 메뉴 헤더 5: 웹 개발 도구
    { _T("&Web Tools"),           _T("웹 개발도구(&W)"),      nullptr,       MenuType::SubHeader },

    // 서브 메뉴 아이템들 (웹 도구)
    { _T("&Evaluate JS Expression"), _T("JS 표현식 평가(&E)"),  DoEvalJS,    MenuType::Item },
    { _T("&Remove HTML/XML Tags"), _T("HTML/XML 태그 삭제(&R)"), DoRemoveTags,  MenuType::Item },
    { _T("Remove C&omments (HTML/C++/Py)"), _T("주석 삭제 (HTML/C++/파이썬)(&O)"), DoRemoveComments, MenuType::Item },

    //{ {},       {},     nullptr,    MenuType::SubEnd },     // 서브 메뉴 종결자, 다음이 Seperator라면 생략 가능

    // 하단 아이템
    { {},       {},     nullptr,    MenuType::Separator},
    { _T("About"),                _T("정보"),                  DoAboutDlg,    MenuType::Item },
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
    { _T("Regex Preset Find"),               _T("정규식 프리셋 검색"), }, // 찾기 타이틀 번역
};

const TCHAR* GetTr(const TCHAR* const engKey) {
    if (!engKey) return _T("");
    if (!g_isKorean) return engKey;

    for (const auto& item : g_uiTrTable) {
        if (item.eng && ::_tcscmp(item.eng, engKey) == 0) return item.kor;
    }
    for (const auto& item : g_menuTable) {
        if (item.eng && ::_tcscmp(item.eng, engKey) == 0) {
            return item.kor ? item.kor : item.eng;
        }
    }
    return engKey;
}

// --- [3] Notepad++ 인터페이스 구현 ---

extern "C" __declspec(dllexport) void setInfo(NppData notepadPlusData) {
    nppData = notepadPlusData;
    g_funcCount = 0;
    for (const auto& entry : g_menuTable) {
        if (entry.type == MenuType::Item || entry.type == MenuType::Separator) {
            if (entry.type == MenuType::Item && (entry.pFunc == nullptr || entry.eng == nullptr)) continue;

            if (entry.type == MenuType::Separator) {
                funcItem[g_funcCount]._itemName[0] = _T('\0');
                funcItem[g_funcCount]._pFunc = nullptr;
            }
            else {
                ::_tcscpy_s(funcItem[g_funcCount]._itemName, _countof(funcItem[g_funcCount]._itemName), entry.eng);
                funcItem[g_funcCount]._pFunc = entry.pFunc;
            }
            funcItem[g_funcCount]._init2Check = false;
            funcItem[g_funcCount]._pShKey = nullptr;
            g_funcCount++;
        }
    }
}

extern "C" __declspec(dllexport) const TCHAR* getName() { return GetTr(g_menuTable[0].eng); }
extern "C" __declspec(dllexport) FuncItem* getFuncsArray(int* const nbF) { *nbF = g_funcCount; return funcItem; }
extern "C" __declspec(dllexport) BOOL isUnicode() { return TRUE; }
extern "C" __declspec(dllexport) LRESULT messageProc(UINT /*Message*/, WPARAM /*wParam*/, LPARAM /*lParam*/) { return TRUE; }

// --- [4] 메뉴 재구성 및 상태 업데이트 (Win32 API 기반) ---

void RestructureMenu() {
    DetectLanguage();
    const HMENU hPluginsMenu = reinterpret_cast<HMENU>(::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0));
    if (!hPluginsMenu) return;

    int pos = -1;
    const int count = ::GetMenuItemCount(hPluginsMenu);
    TCHAR buf[256];
    for (int i = 0; i < count; ++i) {
        ::GetMenuStringW(hPluginsMenu, i, buf, 255, MF_BYPOSITION);
        if (::_tcscmp(buf, g_menuTable[0].eng) == 0 || (g_menuTable[0].kor && ::_tcscmp(buf, g_menuTable[0].kor) == 0)) {
            pos = i; break;
        }
    }
    if (pos == -1) return;

    ::ModifyMenuW(hPluginsMenu, pos, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(::GetSubMenu(hPluginsMenu, pos)), getName());
    const HMENU hMyMenu = ::GetSubMenu(hPluginsMenu, pos);
    if (!hMyMenu) return;

    while (::GetMenuItemCount(hMyMenu) > 0) {
        ::DeleteMenu(hMyMenu, 0, MF_BYPOSITION);
    }

    // 메뉴 트리 구조 복원을 위한 동적 제어 스택
    std::vector<HMENU> menuStack;
    menuStack.push_back(hMyMenu);

    g_subMenuPositions.clear();
    int funcIdx = 0;

    for (int i = 1; i < (int)_countof(g_menuTable); ++i) {
        const auto& entry = g_menuTable[i];

        // Separator, SubEnd, 혹은 새로운 SubHeader를 만나면 현재의 하위 서브메뉴 그룹을 강제로 닫고 루트로 복귀
        if (entry.type == MenuType::Separator || entry.type == MenuType::SubEnd || entry.type == MenuType::SubHeader) {
            if (menuStack.size() > 1) {
                menuStack.pop_back();
            }
        }

        HMENU hCurrentMenu = menuStack.back();

        if (entry.type == MenuType::SubHeader) {
            HMENU hSubMenu = ::CreatePopupMenu();
            ::AppendMenuW(hCurrentMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hSubMenu), GetTr(entry.eng));
            menuStack.push_back(hSubMenu);

            // 최상위 플러그인 직속 서브메뉴 그룹의 현재 마크 위치 자동 추적 (UpdateMenuState 상시 연동)
            if (entry.eng && menuStack.size() == 2) {
                g_subMenuPositions[entry.eng] = ::GetMenuItemCount(hMyMenu) - 1;
            }
        }
        else if (entry.type == MenuType::Separator) {
            if (funcIdx < g_funcCount) {
                ::AppendMenuW(hCurrentMenu, MF_SEPARATOR, 0, nullptr);
                funcIdx++;
            }
        }
        else if (entry.type == MenuType::Item) {
            if (funcIdx < g_funcCount) {
                if (entry.pFunc == nullptr || entry.eng == nullptr) continue;

                const int id = funcItem[funcIdx]._cmdID;
                ::AppendMenuW(hCurrentMenu, MF_STRING, id, GetTr(entry.eng));
                funcIdx++;
            }
        }
    }
}

void UpdateMenuState() {
    const int whichView = static_cast<int>(::SendMessage(nppData._nppHandle, NPPM_GETCURRENTVIEW, 0, 0));
    const HWND hSci = (whichView == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
    if (!hSci) return;

    int langType = 0;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTLANGTYPE, 0, reinterpret_cast<LPARAM>(&langType));
    int nppEncoding = 0;
    ::SendMessage(nppData._nppHandle, NPPM_GETBUFFERENCODING, 0, reinterpret_cast<LPARAM>(&nppEncoding));
    const int cp = static_cast<int>(::SendMessage(hSci, SCI_GETCODEPAGE, 0, 0));

    const Sci_Position len = ::SendMessage(hSci, SCI_GETLENGTH, 0, 0);
    const Sci_Position selStart = static_cast<Sci_Position>(::SendMessage(hSci, SCI_GETSELECTIONSTART, 0, 0));
    const Sci_Position selEnd = static_cast<Sci_Position>(::SendMessage(hSci, SCI_GETSELECTIONEND, 0, 0));
    const bool hasSelection = (selStart != selEnd);
    const Sci_Position startLine = static_cast<Sci_Position>(::SendMessage(hSci, SCI_LINEFROMPOSITION, selStart, 0));
    const Sci_Position endLine = static_cast<Sci_Position>(::SendMessage(hSci, SCI_LINEFROMPOSITION, selEnd, 0));
    const bool isMultiLine = hasSelection && (endLine > startLine);

    const int selMode = static_cast<int>(::SendMessage(hSci, SCI_GETSELECTIONMODE, 0, 0));
    const bool isColumnSelection = (selMode == SC_SEL_RECTANGLE || selMode == SC_SEL_THIN);

    const HMENU hPluginsMenu = reinterpret_cast<HMENU>(::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0));
    if (!hPluginsMenu) return;

    int myPos = -1;
    const int count = ::GetMenuItemCount(hPluginsMenu);
    TCHAR buf[256];
    for (int i = 0; i < count; ++i) {
        ::GetMenuStringW(hPluginsMenu, i, buf, 255, MF_BYPOSITION);
        if (::_tcscmp(buf, getName()) == 0) { myPos = i; break; }
    }
    if (myPos == -1) return;
    const HMENU hMyMenu = ::GetSubMenu(hPluginsMenu, myPos);

    // 일반 기능 매칭용 람다
    const auto setItem = [&](const PFUNCPLUGINCMD pTargetFunc, const bool en) noexcept {
        for (int i = 0; i < g_funcCount; ++i) {
            if (funcItem[i]._pFunc == pTargetFunc) {
                ::EnableMenuItem(hMyMenu, funcItem[i]._cmdID, MF_BYCOMMAND | (en ? MF_ENABLED : MF_GRAYED));
                return;
            }
        }
        };

    // 서브메뉴 활성/비활성 제어용 위치값 조회도 동적 람다로 한 방에 해결
    const auto getSubMenuPos = [](const TCHAR* const engName) noexcept -> int {
        if (!engName) return -1;
        const auto it = g_subMenuPositions.find(engName);
        return (it != g_subMenuPositions.end()) ? it->second : -1;
        };

    setItem(DoAlignDlg, isMultiLine);
    setItem(DoCalculate, hasSelection && !isColumnSelection);
    setItem(DoInjectRegexPresets, true);
    setItem(DoHanjaToHangul, hasSelection);
    setItem(DoHangulDecomp, hasSelection);
    setItem(DoToggleComposition, hasSelection);
    setItem(DoKssmToWansung, (nppEncoding == 0 && (cp == 0 || cp == 949) && len > 0));
    setItem(DoEvalJS, hasSelection && !isColumnSelection);
    setItem(DoRemoveTags, hasSelection && (langType == L_TEXT || langType == L_HTML || langType == L_XML));
    setItem(DoRemoveComments, hasSelection && (langType != L_TEXT));
    setItem(DoAboutDlg, true);

    const int posTextTrans = getSubMenuPos(_T("Text &Transliteration"));
    if (posTextTrans != -1) {
        const bool canKssm = (nppEncoding == 0 && (cp == 0 || cp == 949) && len > 0);
        const bool isParentEnabled = hasSelection || canKssm;
        ::EnableMenuItem(hMyMenu, posTextTrans, MF_BYPOSITION | (isParentEnabled ? MF_ENABLED : MF_GRAYED));
    }

    const int posWebTools = getSubMenuPos(_T("&Web Tools"));
    if (posWebTools != -1) {
        ::EnableMenuItem(hMyMenu, posWebTools, MF_BYPOSITION | (hasSelection ? MF_ENABLED : MF_GRAYED));
    }
}

extern "C" __declspec(dllexport) void beNotified(SCNotification* notifyCode) {
    switch (notifyCode->nmhdr.code) {
    case NPPN_READY:
        DetectLanguage();
        ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTLANGTYPE, 0, reinterpret_cast<LPARAM>(&g_cachedLangType));
        RestructureMenu();
        UpdateMenuState();
        break;
    case NPPN_NATIVELANGCHANGED:
        DetectLanguage(); RestructureMenu(); UpdateMenuState(); break;
    case NPPN_BUFFERACTIVATED:
    case NPPN_LANGCHANGED:
        ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTLANGTYPE, 0, reinterpret_cast<LPARAM>(&g_cachedLangType));
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
        // 1. Ctrl+Alt 조합 시 IDC_STATIC_DEBUG에 디버그 정보 출력
        const bool isCtrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
        const bool isAlt = (::GetKeyState(VK_MENU) & 0x8000) != 0;

        // 2. 번역 적용
        ::SetDlgItemTextW(hwnd, IDOK, GetTr(_T("OK")));
        ::SetDlgItemTextW(hwnd, IDCANCEL, GetTr(_T("Cancel")));

        // 3. 부모 창(Notepad++)과 내 창(About)의 좌표 정보를 가져옴
        const HWND hwndParent = ::GetParent(hwnd);
        if (hwndParent) {
            RECT rcParent{}, rcWindow{};
            ::GetWindowRect(hwndParent, &rcParent);
            ::GetWindowRect(hwnd, &rcWindow);

            const int parentWidth = rcParent.right - rcParent.left;
            const int parentHeight = rcParent.bottom - rcParent.top;
            const int windowWidth = rcWindow.right - rcWindow.left;
            const int windowHeight = rcWindow.bottom - rcWindow.top;

            // 2. 부모 창의 정중앙 좌표를 계산함
            const int x = rcParent.left + (parentWidth - windowWidth) / 2;
            const int y = rcParent.top + (parentHeight - windowHeight) / 2;

            // 3. 계산된 위치로 다이얼로그를 이동시킴
            ::SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }

        if (isCtrl && isAlt) {
            int whichView = 0;
            ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTVIEW, 0, reinterpret_cast<LPARAM>(&whichView));
            const HWND hSci = (whichView == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;

            const int cp = static_cast<int>(::SendMessage(hSci, SCI_GETCODEPAGE, 0, 0));
            int nppEncoding = 0;
            ::SendMessage(nppData._nppHandle, NPPM_GETBUFFERENCODING, 0, reinterpret_cast<LPARAM>(&nppEncoding));

            TCHAR szDebug[128];
            ::_stprintf_s(szDebug, _countof(szDebug), _T("Debug: CP[%d] / NPP_ENC[%d] / View[%d]"), cp, nppEncoding, whichView);
            ::SetDlgItemTextW(hwnd, IDC_STATIC_DEBUG, szDebug);
        }
        else {
            ::SetDlgItemTextW(hwnd, IDC_STATIC_DEBUG, _T(""));
        }

        return static_cast<INT_PTR>(TRUE);
    }

    case WM_COMMAND:
    {
        const WORD wpLow = LOWORD(wParam);
        if (wpLow == IDOK || wpLow == IDCANCEL)
        {
            ::EndDialog(hwnd, wpLow);
            return static_cast<INT_PTR>(TRUE);
        }
        break;
    }
    }
    return static_cast<INT_PTR>(FALSE);
}

// 메뉴에서 'About'을 클릭했을 때 호출되는 함수
void DoAboutDlg()
{
    ::DialogBoxParamW(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_ABOUT), nppData._nppHandle, AboutDlgProc, 0);
}
