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
#include "framework.h"
#include "NPP_Notepad4.h"
#include "AlignFeatures.h"
#include "Common.h"
#include <algorithm>
#include <span>
#include <memory>

static int g_alignMode = IDC_ALIGN_LEFT;

INT_PTR CALLBACK AlignDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        // 1. 번역 적용
        SetWindowText(hwnd, GetTr(_T("Align Lines")));
        SetDlgItemText(hwnd, IDC_ALIGN_LEFT, GetTr(_T("&Left.")));
        SetDlgItemText(hwnd, IDC_ALIGN_RIGHT, GetTr(_T("&Right.")));
        SetDlgItemText(hwnd, IDC_ALIGN_CENTER, GetTr(_T("&Center.")));
        SetDlgItemText(hwnd, IDC_ALIGN_JUSTIFY, GetTr(_T("&Justify.")));
        SetDlgItemText(hwnd, IDC_ALIGN_JUSTIFY_PAR, GetTr(_T("Justify (&Paragraph mode).")));
        SetDlgItemText(hwnd, IDOK, GetTr(_T("OK")));
        SetDlgItemText(hwnd, IDCANCEL, GetTr(_T("Cancel")));

        // 2. 이전에 선택했던 라디오 버튼 체크
        CheckRadioButton(hwnd, IDC_ALIGN_LEFT, IDC_ALIGN_JUSTIFY_PAR, g_alignMode);

        // 3. 부모 창(Notepad++) 중앙 정렬 로직 (이전에 만든 것과 동일)
        const HWND hwndParent = GetParent(hwnd);
        if (hwndParent) {
            RECT rcP, rcD;
            GetWindowRect(hwndParent, &rcP);
            GetWindowRect(hwnd, &rcD);
            SetWindowPos(hwnd, NULL,
                rcP.left + (rcP.right - rcP.left - (rcD.right - rcD.left)) / 2,
                rcP.top + (rcP.bottom - rcP.top - (rcD.bottom - rcD.top)) / 2,
                0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
        // 확인 버튼 클릭 시 현재 선택된 라디오 버튼 ID를 저장하고 종료
        if (LOWORD(wParam) == IDOK)
        {
            for (int id = IDC_ALIGN_LEFT; id <= IDC_ALIGN_JUSTIFY_PAR; id++) {
                if (IsDlgButtonChecked(hwnd, id)) {
                    g_alignMode = id;
                    break;
                }
            }
            EndDialog(hwnd, IDOK);
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hwnd, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

namespace {

    // 폭 구하기
    struct interval {
        const int first;
        const int last;
    };

    /* auxiliary function for binary search in interval table */
    static constexpr bool bisearch(const int ucs, std::span<const interval> table) {
        if (table.empty() || ucs < table.front().first || ucs > table.back().last)
            return false;

        const auto it = std::ranges::lower_bound(table, ucs, {}, &interval::last);

        return (it != table.end() && ucs >= it->first);
    }

    static constexpr struct interval doubleWidthList[]{
        { 0x00A1, 0x00A1 }, { 0x00A4, 0x00A4 }, { 0x00A7, 0x00A8 },
        { 0x00AA, 0x00AA }, { 0x00AE, 0x00AE }, { 0x00B0, 0x00B4 },
        { 0x00B6, 0x00BA }, { 0x00BC, 0x00BF }, { 0x00C6, 0x00C6 },
        { 0x00D0, 0x00D0 }, { 0x00D7, 0x00D8 }, { 0x00DE, 0x00E1 },
        { 0x00E6, 0x00E6 }, { 0x00E8, 0x00EA }, { 0x00EC, 0x00ED },
        { 0x00F0, 0x00F0 }, { 0x00F2, 0x00F3 }, { 0x00F7, 0x00FA },
        { 0x00FC, 0x00FC }, { 0x00FE, 0x00FE }, { 0x0101, 0x0101 },
        { 0x0111, 0x0111 }, { 0x0113, 0x0113 }, { 0x011B, 0x011B },
        { 0x0126, 0x0127 }, { 0x012B, 0x012B }, { 0x0131, 0x0133 },
        { 0x0138, 0x0138 }, { 0x013F, 0x0142 }, { 0x0144, 0x0144 },
        { 0x0148, 0x014B }, { 0x014D, 0x014D }, { 0x0152, 0x0153 },
        { 0x0166, 0x0167 }, { 0x016B, 0x016B }, { 0x01CE, 0x01CE },
        { 0x01D0, 0x01D0 }, { 0x01D2, 0x01D2 }, { 0x01D4, 0x01D4 },
        { 0x01D6, 0x01D6 }, { 0x01D8, 0x01D8 }, { 0x01DA, 0x01DA },
        { 0x01DC, 0x01DC }, { 0x0251, 0x0251 }, { 0x0261, 0x0261 },
        { 0x02C4, 0x02C4 }, { 0x02C7, 0x02C7 }, { 0x02C9, 0x02CB },
        { 0x02CD, 0x02CD }, { 0x02D0, 0x02D0 }, { 0x02D8, 0x02DB },
        { 0x02DD, 0x02DD }, { 0x02DF, 0x02DF }, { 0x0391, 0x03A1 },
        { 0x03A3, 0x03A9 }, { 0x03B1, 0x03C1 }, { 0x03C3, 0x03C9 },
        { 0x0401, 0x0401 }, { 0x0410, 0x044F }, { 0x0451, 0x0451 },

        // 한글 자모(U+1100) 중 폭 2를 가져야 하는 영역을 분리함
        { 0x1100, 0x115F }, // 현대 초성 + 옛 초성 (0x1113..0x115F)
        { 0x1161, 0x1175 }, // 현대 중성 (낱자로 찢어지므로 폭 2)
        { 0x11A8, 0x11C2 }, // 현대 종성 (낱자로 찢어지므로 폭 2)

        { 0x2010, 0x2010 }, { 0x2013, 0x2016 },
        { 0x2018, 0x2019 }, { 0x201C, 0x201D }, { 0x2020, 0x2022 },
        { 0x2024, 0x2027 }, { 0x2030, 0x2030 }, { 0x2032, 0x2033 },
        { 0x2035, 0x2035 }, { 0x203B, 0x203B }, { 0x203E, 0x203E },
        { 0x2074, 0x2074 }, { 0x207F, 0x207F }, { 0x2081, 0x2084 },
        { 0x20AC, 0x20AC }, { 0x2103, 0x2103 }, { 0x2105, 0x2105 },
        { 0x2109, 0x2109 }, { 0x2113, 0x2113 }, { 0x2116, 0x2116 },
        { 0x2121, 0x2122 }, { 0x2126, 0x2126 }, { 0x212B, 0x212B },
        { 0x2153, 0x2155 }, { 0x215B, 0x215E }, { 0x2160, 0x216B },
        { 0x2170, 0x2179 }, { 0x2190, 0x2199 }, { 0x21B8, 0x21B9 },
        { 0x21D2, 0x21D2 }, { 0x21D4, 0x21D4 }, { 0x21E7, 0x21E7 },
        { 0x2200, 0x2200 }, { 0x2202, 0x2203 }, { 0x2207, 0x2208 },
        { 0x220B, 0x220B }, { 0x220F, 0x220F }, { 0x2211, 0x2211 },
        { 0x2215, 0x2215 }, { 0x221A, 0x221A }, { 0x221D, 0x2220 },
        { 0x2223, 0x2223 }, { 0x2225, 0x2225 }, { 0x2227, 0x222C },
        { 0x222E, 0x222E }, { 0x2234, 0x2237 }, { 0x223C, 0x223D },
        { 0x2248, 0x2248 }, { 0x224C, 0x224C }, { 0x2252, 0x2252 },
        { 0x2260, 0x2261 }, { 0x2264, 0x2267 }, { 0x226A, 0x226B },
        { 0x226E, 0x226F }, { 0x2282, 0x2283 }, { 0x2286, 0x2287 },
        { 0x2295, 0x2295 }, { 0x2299, 0x2299 }, { 0x22A5, 0x22A5 },
        { 0x22BF, 0x22BF }, { 0x2312, 0x2312 }, { 0x2329, 0x232A },
        { 0x2460, 0x254B }, { 0x2550, 0x2573 }, { 0x2580, 0x258F },
        { 0x2592, 0x2595 }, { 0x25A0, 0x25A1 }, { 0x25A3, 0x25A9 },
        { 0x25B2, 0x25B3 }, { 0x25B6, 0x25B7 }, { 0x25BC, 0x25BD },
        { 0x25C0, 0x25C1 }, { 0x25C6, 0x25C8 }, { 0x25CB, 0x25CB },
        { 0x25CE, 0x25D1 }, { 0x25E2, 0x25E5 }, { 0x25EF, 0x25EF },
        { 0x2600, 0x27BF }, { 0x2E80, 0xA4CF }, { 0xA700, 0xA7F5 },

        // 옛 초성 확장-A (폭 2)
        { 0xA960, 0xA97F },

        { 0xAC00, 0xD7A3 },
        { 0xF900, 0xFAFF }, { 0xFE10, 0xFE19 },
        { 0xFE30, 0xFE6F }, { 0xFF00, 0xFF60 }, { 0xFFE0, 0xFFE6 },
        { 0xFFFD, 0xFFFD },

        // U+1F100 부터 있는 보조 다국어 평면 중 전각 문자
        { 0x1F100, 0x1F10C }, { 0x1F110, 0x1F12E }, { 0x1F130, 0x1F169 },
        { 0x1F170, 0x1F19A }, { 0x1F200, 0x1F202 }, { 0x1F210, 0x1F23A },
        { 0x1F240, 0x1F248 }, { 0x1F250, 0x1F251 },

        // U+1F300 부터 있는 에모지
        { 0x1F300, 0x1F32C }, { 0x1F330, 0x1F37D }, { 0x1F380, 0x1F3CE },
        { 0x1F3D4, 0x1F3F7 }, { 0x1F400, 0x1F4FE }, { 0x1F500, 0x1F54B },
        { 0x1F550, 0x1F6CF },
        { 0x1F6D0, 0x1F6ED },
        { 0x1F6F0, 0x1F6FC },

        // U+1F700 부터 있는 보조 다국어 평면 중 전각 문자 #1
        { 0x1F700, 0x1F773 }, { 0x1F780, 0x1F7D4 },
        { 0x1F7E0, 0x1F7EB },

        // U+1F700 부터 있는 보조 다국어 평면 중 전각 문자 #2
        { 0x1F800, 0x1F80B }, { 0x1F810, 0x1F847 },
        { 0x1F850, 0x1F859 }, { 0x1F860, 0x1F887 }, { 0x1F890, 0x1F8AD },
        { 0x1F900, 0x1F9FF }, { 0x1FA00, 0x1FAFF }, { 0x20000, 0x3FFFD },
        { 0xF0000, 0x10FFFD }
    };

    static constexpr struct interval zeroWidthList[]{
        // 0x00..0x08, 0x0B, 0x0C, 0x0E..0x1F, 0x7F 는 코드에서 선처리 (\t(09), \n(0A), \r(0D) 제외)
        { 0x0080, 0x009F }, // C1 제어 문자 (0x7F 이하는 코드에서 선처리하므로 0x80부터 시작)
        { 0x0300, 0x036F },
        { 0x0483, 0x0489 }, { 0x0591, 0x05BD }, { 0x05BF, 0x05BF },
        { 0x05C1, 0x05C2 }, { 0x05C4, 0x05C5 }, { 0x05C7, 0x05C7 },
        { 0x0610, 0x061A }, { 0x064B, 0x065F }, { 0x0670, 0x0670 },
        { 0x06D6, 0x06DC }, { 0x06DF, 0x06E4 }, { 0x06E7, 0x06E8 },
        { 0x06EA, 0x06ED },

        // 옛한글 NFD 결합 문자 구간 (중성/종성)
        { 0x1176, 0x11A7 }, // 옛 중성 (화면 결합용)
        { 0x11C3, 0x11FF }, // 옛 종성 (화면 결합용)

        { 0x200B, 0x200F }, { 0x202A, 0x202E }, { 0x2060, 0x206F },

        // 기호용 결합 다이아크리틱 마크 (수학 기호나 도형 위에 붙는 제로 너비 기호 무리)
        { 0x20D0, 0x20FF },

        // 옛한글 NFD 결합 문자 확장 블록
        { 0xD7B0, 0xD7C6 }, // 확장-B 옛 중성
        { 0xD7CB, 0xD7FB }, // 확장-B 옛 종성

        { 0xFE00, 0xFE0F }, { 0xFE20, 0xFE2F }, { 0xFEFF, 0xFEFF },
        { 0xFFF9, 0xFFFB }, { 0x1D167, 0x1D169 }, { 0x1D173, 0x1D182 },
        { 0x1D185, 0x1D18B }, { 0x1D1AA, 0x1D1AD }, { 0x1F3FB, 0x1F3FF },

        // 유니코드 언어 태그 영역 (에모지 국가 코드 조합 등에 쓰이며 화면엔 안 보임)
        { 0xE0000, 0xE007F },
        { 0xE0100, 0xE01EF }
    };

    static constexpr Sci_Position GetConsoleWidth1CH(const int ucs) {
        // 1. ASCII 영역 (0x00 ~ 0x7F) 통합 Fast-path
        // ucs의 7번 비트 이상이 모두 0인지 확인 (ucs < 0x80 과 동일)
        if (!(ucs & ~0x7F)) [[likely]] {

            // 1-1. ASCII 제어 문자 영역 (0x00 ~ 0x1F)
            // ucs의 5번 비트 이상이 모두 0인지 확인 (ucs < 0x20 과 동일)
            if (!(ucs & ~0x1F)) {
                // \t(09), \n(0A), \r(0D)는 기본 폭(1) 유지
                if (ucs == 0x09 || ucs == 0x0A || ucs == 0x0D) return 1;
                return 0; // 그 외 제어 문자는 폭 0
            }

            // 1-2. DEL 문자 예외 처리
            if (ucs == 0x7F) return 0;

            // 1-3. 일반 출력 가능한 ASCII (0x20 ~ 0x7E)
            return 1;
        }

        // 2. 폭이 0인 특수 문자/결합 문자 체크 (이진 탐색)
        if (bisearch(ucs, zeroWidthList)) [[unlikely]] return 0;

        // 3. 전각 문자 체크 (기존 리스트)
        if (bisearch(ucs, doubleWidthList)) return 2;

        // 4. 나머지는 일반 반각 문자
        return 1;
    }

    static constexpr wchar_t SURROGATE_LEAD_FIRST_BLUEnLIVE{ 0xD800u };
    static constexpr wchar_t SURROGATE_LEAD_LAST_BLUEnLIVE{ 0xDBFFu };
    static constexpr wchar_t SURROGATE_TRAIL_FIRST_BLUEnLIVE{ 0xDC00u };
    static constexpr wchar_t SURROGATE_TRAIL_LAST_BLUEnLIVE{ 0xDFFFu };

    static constexpr bool IsSurrogate(const wchar_t uc) noexcept { return (uc - SURROGATE_LEAD_FIRST_BLUEnLIVE) < 2048u; }
    static constexpr bool IsHighSurrogate(const wchar_t uc) noexcept { return (uc & 0xFC00u) == SURROGATE_LEAD_FIRST_BLUEnLIVE; }
    static constexpr bool IsLowSurrogate(const wchar_t uc) noexcept { return (uc & 0xFC00u) == SURROGATE_TRAIL_FIRST_BLUEnLIVE; }

    static constexpr int SurrogateToUTF32(const wchar_t high, const wchar_t low) noexcept {
        return (static_cast<int>(high) << 10) + static_cast<int>(low) - 0x35fdc00;
    }

}

// --- [정렬 실행 함수] ---
void ExecuteAlignLines(const int nMode) {
    int whichView = 0;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTVIEW, 0, (LPARAM)&whichView);
    const HWND hSci = (whichView == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;

    if (::SendMessage(hSci, SCI_GETSELECTIONMODE, 0, 0) != 0) return;

    constexpr Sci_Position BUFSIZE_ALIGN = 1024;

    const Sci_Position iSelStart = ::SendMessage(hSci, SCI_GETSELECTIONSTART, 0, 0);
    const Sci_Position iSelEnd = ::SendMessage(hSci, SCI_GETSELECTIONEND, 0, 0);
    Sci_Position iCurPos = ::SendMessage(hSci, SCI_GETCURRENTPOS, 0, 0);
    Sci_Position iAnchorPos = ::SendMessage(hSci, SCI_GETANCHOR, 0, 0);
    const UINT cpEdit = (UINT)::SendMessage(hSci, SCI_GETCODEPAGE, 0, 0);

    const Sci_Position iLineStart = ::SendMessage(hSci, SCI_LINEFROMPOSITION, iSelStart, 0);
    Sci_Position iLineEnd = ::SendMessage(hSci, SCI_LINEFROMPOSITION, iSelEnd, 0);

    if (iSelEnd <= ::SendMessage(hSci, SCI_POSITIONFROMLINE, iLineEnd, 0)) {
        if (iLineEnd - iLineStart >= 1)
            --iLineEnd;
    }

    Sci_Position iMinIndent = BUFSIZE_ALIGN;
    Sci_Position iMaxLength = 0;

    // [Pass 1] 최대 폭 및 최소 들여쓰기 계산 (Notepad4 로직)
    for (Sci_Position iLine = iLineStart; iLine <= iLineEnd; iLine++) {
        Sci_Position iLineEndPos = ::SendMessage(hSci, SCI_GETLINEENDPOSITION, iLine, 0);
        const Sci_Position iLineIndentPos = ::SendMessage(hSci, SCI_GETLINEINDENTPOSITION, iLine, 0);

        if (iLineIndentPos < iLineEndPos) {
            while (iLineEndPos >= iLineIndentPos) {
                --iLineEndPos;
                const int ch = (int)::SendMessage(hSci, SCI_GETCHARAT, iLineEndPos, 0);
                if (ch != ' ' && ch != '\t') {
                    break;
                }
            }
            ++iLineEndPos;

            Sci_Position iEndCol = ::SendMessage(hSci, SCI_GETCOLUMN, iLineEndPos, 0);
            {
                char tchLineBuf[BUFSIZE_ALIGN * 3]{ "" };
                wchar_t wchLineBuf[BUFSIZE_ALIGN * 3]{ L"" };

                Sci_Position cchLine = ::SendMessage(hSci, SCI_GETLINE, iLine, (LPARAM)tchLineBuf);
                Sci_Position iEndColNew = 0;
                MultiByteToWideChar(CP_UTF8, 0, tchLineBuf, (int)cchLine, wchLineBuf, _countof(wchLineBuf));

                wchar_t* p = wchLineBuf;
                for (Sci_Position i = 0; i < iEndCol; ++i, ++p) {
                    const wchar_t uc = *p;
                    if (!IsSurrogate(uc)) {
                        iEndColNew += GetConsoleWidth1CH((int)uc);
                    }
                    else {
                        const wchar_t uc2 = p[1];
                        if (IsHighSurrogate(uc) && IsLowSurrogate(uc2)) {
                            iEndColNew += GetConsoleWidth1CH(SurrogateToUTF32(uc, uc2));
                            ++p;
                        }
                        else {
                            ;
                            // ERROR, do nothing
                        }
                    }
                }
                iEndCol = iEndColNew;
            }

            const Sci_Position iIndentCol = ::SendMessage(hSci, SCI_GETLINEINDENTATION, iLine, 0);
            iMinIndent = (std::min)(iMinIndent, iIndentCol);
            iMaxLength = (std::max)(iMaxLength, iEndCol);
        }
    }

    // [Pass 2] 실제 정렬 작업 (메모리 할당 및 단어 분할)
    if (iMaxLength < BUFSIZE_ALIGN) {
        struct EditAlignTextVar {
            char tchLineBuf[BUFSIZE_ALIGN * 3]; // kMaxMultiByteCount = 3
            WCHAR wchLineBuf[BUFSIZE_ALIGN];
            LPWSTR pWords[BUFSIZE_ALIGN];
            WCHAR wchNewLineBuf[BUFSIZE_ALIGN * 3];
        };
        // NPP 환경에 맞게 std::unique_ptr로 안전하게 관리 (Notepad4의 HeapAlloc 대응)
        auto var = std::make_unique<EditAlignTextVar>();

        ::SendMessage(hSci, SCI_BEGINUNDOACTION, 0, 0);
        for (Sci_Position iLine = iLineStart; iLine <= iLineEnd; iLine++) {
            const Sci_Position iIndentPos = ::SendMessage(hSci, SCI_GETLINEINDENTPOSITION, iLine, 0);
            const Sci_Position iEndPos = ::SendMessage(hSci, SCI_GETLINEENDPOSITION, iLine, 0);

            if (iIndentPos == iEndPos && iEndPos > 0) {
                const Sci_Position iStartPos = ::SendMessage(hSci, SCI_POSITIONFROMLINE, iLine, 0);
                ::SendMessage(hSci, SCI_DELETERANGE, iStartPos, iEndPos - iStartPos);
            }
            else {
                Sci_Position iWordsLength = 0;
                const Sci_Position cchLine = ::SendMessage(hSci, SCI_GETLINE, iLine, (LPARAM)var->tchLineBuf);

                int iWords = MultiByteToWideChar(cpEdit, 0, var->tchLineBuf, static_cast<int>(cchLine), var->wchLineBuf, BUFSIZE_ALIGN);
                var->wchLineBuf[iWords] = L'\0';
                iWords = 0;

                // StrTrim(var->wchLineBuf, L"\r\n\t "); 구현
                {
                    LPWSTR pBuf = var->wchLineBuf;
                    // 1. 뒤쪽 공백 제거 (먼저 해야 계산이 편합니다)
                    size_t len = wcslen(pBuf);
                    while (len > 0 && wcschr(L"\r\n\t ", pBuf[len - 1])) {
                        pBuf[--len] = L'\0';
                    }

                    // 2. 앞쪽 공백 제거 및 데이터 시프트
                    LPWSTR pStart = pBuf;
                    while (*pStart && wcschr(L"\r\n\t ", *pStart)) {
                        ++pStart;
                    }

                    if (pStart != pBuf) {
                        // 앞쪽 공백이 있다면 데이터를 앞당겨서 버퍼의 시작점을 맞춤
                        memmove(pBuf, pStart, (wcslen(pStart) + 1) * sizeof(WCHAR));
                    }
                }

                WCHAR* p = var->wchLineBuf;
                while (*p) {
                    if (*p != L' ' && *p != L'\t') {
                        const WCHAR uc = *p;
                        if (!IsSurrogate(uc)) {
                            iWordsLength += GetConsoleWidth1CH((int)uc);
                            var->pWords[iWords++] = ++p;
                        }
                        else {
                            const WCHAR uc2{ p[1] };
                            if (IsHighSurrogate(uc) && uc2 && uc2 != L' ' && uc2 != L'\t' && IsLowSurrogate(p[1])) {
                                iWordsLength += GetConsoleWidth1CH(SurrogateToUTF32(uc, p[1]));
                                var->pWords[iWords++] = p;
                                p += 2;
                            }
                            else {
                                // ERROR 발생, 응급조치
                                // iWordsLength는 정확성이 없는 값임
                                iWordsLength += GetConsoleWidth1CH((int)uc);
                                var->pWords[iWords++] = ++p;
                            }
                        }
                        while (*p && *p != L' ' && *p != L'\t') {
                            const WCHAR uc3{ *p };
                            if (!IsSurrogate(uc3)) {
                                iWordsLength += GetConsoleWidth1CH((int)uc3);
                                ++p;
                            }
                            else {
                                const WCHAR uc4{ p[1] };
                                if (IsHighSurrogate(uc3) && (uc4 && uc4 != L' ' && uc4 != L'\t') && IsLowSurrogate(uc4)) {
                                    iWordsLength += GetConsoleWidth1CH(SurrogateToUTF32(uc3, uc4));
                                    p += 2;
                                }
                                else {
                                    // ERROR 발생, 응급조치
                                    // iWordsLength는 정확성이 없는 값임
                                    iWordsLength += GetConsoleWidth1CH((int)uc3);
                                    ++p;
                                }
                            }
                        }
                    }
                    else {
                        *p++ = L'\0';
                    }
                }

                if (iWords > 0) {
                    if (nMode == IDC_ALIGN_JUSTIFY || nMode == IDC_ALIGN_JUSTIFY_PAR) {
                        bool bNextLineIsBlank = false;
                        if (nMode == IDC_ALIGN_JUSTIFY_PAR) {
                            const Sci_Position lineCount = (Sci_Position)::SendMessage(hSci, SCI_GETLINECOUNT, 0, 0);
                            if (lineCount <= iLine + 1) {
                                bNextLineIsBlank = true;
                            }
                            else {
                                const Sci_Position nextLineEnd = (Sci_Position)::SendMessage(hSci, SCI_GETLINEENDPOSITION, iLine + 1, 0);
                                const Sci_Position nextLineIndent = (Sci_Position)::SendMessage(hSci, SCI_GETLINEINDENTPOSITION, iLine + 1, 0);
                                if (nextLineIndent == nextLineEnd) {
                                    bNextLineIsBlank = true;
                                }
                            }
                        }

                        if ((nMode == IDC_ALIGN_JUSTIFY || nMode == IDC_ALIGN_JUSTIFY_PAR) &&
                            iWords > 1 && iWordsLength >= 2 &&
                            ((nMode != IDC_ALIGN_JUSTIFY_PAR || !bNextLineIsBlank || iLineStart == iLineEnd) ||
                                (bNextLineIsBlank && iWordsLength * 4 > (iMaxLength - iMinIndent) * 3))) {
                            const int iGaps = iWords - 1;
                            const Sci_Position iSpacesPerGap = (iMaxLength - iMinIndent - iWordsLength) / iGaps;
                            const Sci_Position iExtraSpaces = (iMaxLength - iMinIndent - iWordsLength) % iGaps;

                            lstrcpy(var->wchNewLineBuf, var->pWords[0]);
                            p = var->wchNewLineBuf + wcslen(var->wchNewLineBuf);

                            for (int i = 1; i < iWords; i++) {
                                for (Sci_Position j = 0; j < iSpacesPerGap; j++) {
                                    *p++ = L' ';
                                }
                                if (i > iGaps - iExtraSpaces) {
                                    *p++ = L' ';
                                }
                                *p = L'\0';
                                lstrcat(p, var->pWords[i]);
                                p += wcslen(p);
                            }

                            WideCharToMultiByte(CP_UTF8, 0, var->wchNewLineBuf, -1, var->tchLineBuf, sizeof(var->tchLineBuf), nullptr, nullptr);
                            ::SendMessage(hSci, SCI_SETTARGETRANGE, (WPARAM)::SendMessage(hSci, SCI_POSITIONFROMLINE, iLine, 0), (LPARAM)::SendMessage(hSci, SCI_GETLINEENDPOSITION, iLine, 0));
                            ::SendMessage(hSci, SCI_REPLACETARGET, -1, (LPARAM)var->tchLineBuf);
                            ::SendMessage(hSci, SCI_SETLINEINDENTATION, iLine, iMinIndent);
                        }
                        else {
                            lstrcpy(var->wchNewLineBuf, var->pWords[0]);
                            p = var->wchNewLineBuf + wcslen(var->wchNewLineBuf);

                            for (int i = 1; i < iWords; i++) {
                                *p++ = L' ';
                                *p = L'\0';
                                lstrcat(var->wchNewLineBuf, var->pWords[i]);
                                p += wcslen(p);
                            }

                            WideCharToMultiByte(CP_UTF8, 0, var->wchNewLineBuf, -1, var->tchLineBuf, sizeof(var->tchLineBuf), nullptr, nullptr);
                            ::SendMessage(hSci, SCI_SETTARGETRANGE, (WPARAM)::SendMessage(hSci, SCI_POSITIONFROMLINE, iLine, 0), (LPARAM)::SendMessage(hSci, SCI_GETLINEENDPOSITION, iLine, 0));
                            ::SendMessage(hSci, SCI_REPLACETARGET, -1, (LPARAM)var->tchLineBuf);
                            ::SendMessage(hSci, SCI_SETLINEINDENTATION, iLine, iMinIndent);
                        }
                    }
                    else {
                        const Sci_Position iExtraSpaces = iMaxLength - iMinIndent - iWordsLength - iWords + 1;
                        Sci_Position iOddSpaces = (iExtraSpaces > 0) ? (iExtraSpaces % 2) : 0;

                        p = var->wchNewLineBuf;
                        if (nMode == IDC_ALIGN_RIGHT) {
                            for (Sci_Position i = 0; i < iExtraSpaces; i++) {
                                *p++ = L' ';
                            }
                        }
                        else if (nMode == IDC_ALIGN_CENTER) {
                            for (Sci_Position i = 1; i < iExtraSpaces - iOddSpaces; i += 2) {
                                *p++ = L' ';
                            }
                        }
                        *p = L'\0';

                        for (int i = 0; i < iWords; i++) {
                            lstrcat(p, var->pWords[i]);
                            if (i < iWords - 1) {
                                lstrcat(p, L" ");
                            }
                            // 가운데 정렬 시 홀수 공백 보정
                            if (nMode == IDC_ALIGN_CENTER && iWords > 1 && iOddSpaces > 0 && i + 1 >= iWords / 2) {
                                lstrcat(p, L" ");
                                --iOddSpaces;
                            }
                            p += wcslen(p);
                        }

                        WideCharToMultiByte(cpEdit, 0, var->wchNewLineBuf, -1, var->tchLineBuf, sizeof(var->tchLineBuf), nullptr, nullptr);

                        Sci_Position iPos;
                        if (nMode == IDC_ALIGN_RIGHT || nMode == IDC_ALIGN_CENTER) {
                            ::SendMessage(hSci, SCI_SETLINEINDENTATION, iLine, iMinIndent);
                            iPos = (Sci_Position)::SendMessage(hSci, SCI_GETLINEINDENTPOSITION, iLine, 0);
                        }
                        else {
                            iPos = (Sci_Position)::SendMessage(hSci, SCI_POSITIONFROMLINE, iLine, 0);
                        }

                        ::SendMessage(hSci, SCI_SETTARGETRANGE, iPos, ::SendMessage(hSci, SCI_GETLINEENDPOSITION, iLine, 0));
                        ::SendMessage(hSci, SCI_REPLACETARGET, -1, (LPARAM)var->tchLineBuf);
                        if (nMode == IDC_ALIGN_LEFT) {
                            ::SendMessage(hSci, SCI_SETLINEINDENTATION, iLine, iMinIndent);
                        }
                    }
                }
            }
        }
        ::SendMessage(hSci, SCI_ENDUNDOACTION, 0, 0);

        if (iCurPos < iAnchorPos) {
            iCurPos = iLineStart;
            iAnchorPos = iLineEnd + 1;
        }
        else {
            iAnchorPos = iLineStart;
            iCurPos = iLineEnd + 1;
        }
        ::SendMessage(hSci, SCI_SETSEL,
            ::SendMessage(hSci, SCI_POSITIONFROMLINE, iAnchorPos, 0),
            ::SendMessage(hSci, SCI_POSITIONFROMLINE, iCurPos, 0));
    }
}

void DoAlignDlg()
{
    if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ALIGN), nppData._nppHandle, AlignDlgProc) == IDOK)
    {
        // 결과는 g_alignMode에 저장됨 (IDC_ALIGN_LEFT 등)
        ExecuteAlignLines(g_alignMode);
    }
}
