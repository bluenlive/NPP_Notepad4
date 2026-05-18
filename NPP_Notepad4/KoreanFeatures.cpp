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
#include "KoreanFeatures.h"
#include "Common.h"
#include <string>
#include <string_view>
#include <vector>
//#include <windows.h>

void DoHanjaToHangul()
{
    int whichView = 0;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTVIEW, 0, (LPARAM)&whichView);
    HWND hSci = (whichView == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;

    ::SendMessage(hSci, WM_IME_KEYDOWN, VK_HANJA, 0);
}

namespace {

    //  콜백 함수 타입 정의: 날것의 문자열 뷰, 그리고 코드 페이지를 받음
    typedef std::string(*TransformCallback)(std::string_view, UINT);

    void DoCommonTransform(const TransformCallback transformFunc) {
        int whichView = 0;
        ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTVIEW, 0, (LPARAM)&whichView);
        const HWND hSci = (whichView == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;

        UINT cpDoc = (UINT)::SendMessage(hSci, SCI_GETCODEPAGE, 0, 0);
        if (cpDoc == 0) cpDoc = ::GetACP();

        const int selCount = (int)::SendMessage(hSci, SCI_GETSELECTIONS, 0, 0);
        bool isUndoOpened = false;
        const HCURSOR hOldCursor = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));

        for (int i = selCount - 1; i >= 0; --i) {
            const Sci_Position start = ::SendMessage(hSci, SCI_GETSELECTIONNSTART, i, 0);
            const Sci_Position end = ::SendMessage(hSci, SCI_GETSELECTIONNEND, i, 0);
            if (start == end) continue;

            std::string sRaw(end - start, '\0');
            Sci_TextRangeFull tr = { {start, end}, sRaw.data() };
            ::SendMessage(hSci, SCI_GETTEXTRANGEFULL, 0, (LPARAM)&tr);

            // [B] 본체 호출: 이제 어떤 메뉴인지 묻지 않고 바로 실행합니다.
            const std::string sMapped = transformFunc(sRaw, cpDoc);

            if (sMapped != sRaw) {
                if (!isUndoOpened) {
                    ::SendMessage(hSci, SCI_BEGINUNDOACTION, 0, 0);
                    isUndoOpened = true;
                }

                // [최적화: 선택 영역 보정]
                ::SendMessage(hSci, SCI_SETTARGETRANGE, start, end);
                ::SendMessage(hSci, SCI_REPLACETARGET, sMapped.length(), (LPARAM)sMapped.c_str());

                // 변환 후 영역 유지
                ::SendMessage(hSci, SCI_SETSELECTIONNSTART, i, start);
                ::SendMessage(hSci, SCI_SETSELECTIONNEND, i, start + (Sci_Position)sMapped.length());
            }
        }

        if (isUndoOpened) ::SendMessage(hSci, SCI_ENDUNDOACTION, 0, 0);
        ::SetCursor(hOldCursor);
    }

    // --- 인코딩/디코딩 헬퍼 함수 ---

    std::wstring ConvertToWString(std::string_view s, UINT cp) {
        if (s.empty()) return L"";
        int len = ::MultiByteToWideChar(cp, 0, s.data(), (int)s.length(), NULL, 0);
        std::wstring ws(len, L'\0');
        ::MultiByteToWideChar(cp, 0, s.data(), (int)s.length(), ws.data(), len);
        return ws;
    }

    std::string ConvertToString(std::wstring_view ws, UINT cp) {
        if (ws.empty()) return "";
        int len = ::WideCharToMultiByte(cp, 0, ws.data(), (int)ws.length(), NULL, 0, NULL, NULL);
        std::string s(len, '\0');
        ::WideCharToMultiByte(cp, 0, ws.data(), (int)ws.length(), s.data(), len, NULL, NULL);
        return s;
    }

    std::string HangulDecomposeCore(std::string_view sRaw, UINT cpDoc) {

        // 1. 초성 (19개)
        static constexpr const wchar_t* kChoSplit[] = {
            L"ㄱ", L"ㄲ", L"ㄴ", L"ㄷ", L"ㄸ", L"ㄹ", L"ㅁ", L"ㅂ", L"ㅃ", L"ㅅ", L"ㅆ", L"ㅇ", L"ㅈ", L"ㅉ", L"ㅊ", L"ㅋ", L"ㅌ", L"ㅍ", L"ㅎ"
        };

        // 2. 중성 (21개) - ㅘ, ㅞ 등 복합 모음 분리
        static constexpr const wchar_t* kJungSplit[] = {
            L"ㅏ", L"ㅐ", L"ㅑ", L"ㅒ", L"ㅓ", L"ㅔ", L"ㅕ", L"ㅖ", L"ㅗ", L"ㅗㅏ", L"ㅗㅐ", L"ㅗㅣ", L"ㅛ", L"ㅜ", L"ㅜㅓ", L"ㅜㅔ", L"ㅜㅣ", L"ㅠ", L"ㅡ", L"ㅡㅣ", L"ㅣ"
        };

        // 3. 종성 (28개) - ㄳ, ㄺ 등 복합 받침 분리
        static constexpr const wchar_t* kJongSplit[] = {
            L"", L"ㄱ", L"ㄲ", L"ㄱㅅ", L"ㄴ", L"ㄴㅈ", L"ㄴㅎ", L"ㄷ", L"ㄹ", L"ㄹㄱ", L"ㄹㅁ", L"ㄹㅂ", L"ㄹㅅ", L"ㄹㅌ", L"ㄹㅍ", L"ㄹㅎ", L"ㅁ", L"ㅂ", L"ㅂㅅ", L"ㅅ", L"ㅆ", L"ㅇ", L"ㅈ", L"ㅊ", L"ㅋ", L"ㅌ", L"ㅍ", L"ㅎ"
        };

        struct JamoMapping {
            const wchar_t key;
            const wchar_t* value;
        };

        // 4. 한글 호환 자모 테이블 (U+3131 ~ U+318E)
        static constexpr JamoMapping kDecompTable[] = {
            // 복합 자음 (11개)
            { 0x3133, L"ㄱㅅ" }, { 0x3135, L"ㄴㅈ" }, { 0x3136, L"ㄴㅎ" }, { 0x313A, L"ㄹㄱ" },
            { 0x313B, L"ㄹㅁ" }, { 0x313C, L"ㄹㅂ" }, { 0x313D, L"ㄹㅅ" }, { 0x313E, L"ㄹㅌ" },
            { 0x313F, L"ㄹㅍ" }, { 0x3140, L"ㄹㅎ" }, { 0x3144, L"ㅂㅅ" },

            // 복합 모음 (7개)
            { 0x3158, L"ㅗㅏ" }, { 0x3159, L"ㅗㅐ" }, { 0x315A, L"ㅗㅣ" }, // ㅘ, ㅙ, ㅚ
            { 0x315D, L"ㅜㅓ" }, { 0x315E, L"ㅜㅔ" }, { 0x315F, L"ㅜㅣ" }, // ㅝ, ㅞ, ㅟ
            { 0x3162, L"ㅡㅣ" }                                            // ㅢ
        };

        if (sRaw.empty()) return std::string(sRaw);

        const std::wstring wsText = ConvertToWString(sRaw, cpDoc);
        std::wstring wsMapped;
        wsMapped.reserve(wsText.length() * 5); // 뷁 -> ㅂㅜㅔㄹㄱ (5배까지 가능)

        for (wchar_t w : wsText) {
            // 1. 완성형 한글 (AC00 ~ D7A3)
            if (w >= 0xAC00 && w <= 0xD7A3) {
                const int c = w - 0xAC00;
                wsMapped += kChoSplit[c / 588];
                wsMapped += kJungSplit[(c % 588) / 28];
                const int jong = c % 28;
                if (jong > 0) wsMapped += kJongSplit[jong];
            }
            // 2. 호환 자모 중 분리 가능 범위 (ㄳ ~ ㅢ)
            else if (w >= 0x3133 && w <= 0x3162) {
                auto it = std::lower_bound(std::begin(kDecompTable), std::end(kDecompTable), w,
                    [](const JamoMapping& m, wchar_t key) { return m.key < key; });

                if (it != std::end(kDecompTable) && it->key == w) {
                    wsMapped += it->value;
                }
                else {
                    wsMapped += w;
                }
            }
            // 3. 그 외 영역은 그대로 유지
            else {
                wsMapped += w;
            }
        }

        return (wsMapped != wsText) ? ConvertToString(wsMapped, cpDoc) : std::string(sRaw);
    }

    std::string HangulToggleCore(std::string_view sRaw, const UINT cpDoc) {
        if (sRaw.empty()) return std::string(sRaw);

        const std::wstring wsText = ConvertToWString(sRaw, cpDoc);
        std::wstring wsMapped;
        wsMapped.reserve(wsText.length() * 3);

        size_t s = 0;
        while (s < wsText.length()) {
            const wchar_t w0 = wsText[s];
            const wchar_t w1 = (s + 1 < wsText.length()) ? wsText[s + 1] : 0;
            const wchar_t w2 = (s + 2 < wsText.length()) ? wsText[s + 2] : 0;

            // Notepad4에 구현해둔 토글 알고리즘
            if (w0 >= 0xAC00 && w0 <= 0xD7A3) {
                const wchar_t c = w0 - 0xAC00;
                wsMapped += static_cast<wchar_t>(0x1100 + c / 588);
                wsMapped += static_cast<wchar_t>(0x1161 + (c % 588) / 28);
                wchar_t c3 = static_cast<wchar_t>(0x11A8 + c % 28 - 1);
                if (c3 != 0x11A7) wsMapped += c3;
                ++s;
            }
            else if (w0 >= 0x1100 && w0 <= 0x1112) {
                if (w1 >= 0x1161 && w1 <= 0x1175) {
                    if (w2 >= 0x11A8 && w2 <= 0x11C2) {
                        wsMapped += static_cast<wchar_t>(0xAC00 + (w0 - 0x1100) * 588 + (w1 - 0x1161) * 28 + (w2 - 0x11A8) + 1);
                        s += 3;
                    }
                    else {
                        wsMapped += static_cast<wchar_t>(0xAC00 + (w0 - 0x1100) * 588 + (w1 - 0x1161) * 28);
                        s += 2;
                    }
                }
                else {
                    wsMapped += w0;
                    ++s;
                }
            }
            else {
                wsMapped += w0;
                ++s;
            }
        }

        return (wsMapped != wsText) ? ConvertToString(wsMapped, cpDoc) : std::string(sRaw);
    }

}

void DoHangulDecomp()
{
    DoCommonTransform(HangulDecomposeCore);
}

void DoToggleComposition()
{
    DoCommonTransform(HangulToggleCore);
}

namespace {

    // 조합형 초성 값(2~20) -> [유니코드 음절 인덱스, 호환 자모 코드]
    static constexpr int chosung_table[32][2] = {
        { -1, 0x0000 }, { -1, 0x0000 },                 // 0, 1 (1은 채움)
        { 0,  0x3131 }, { 1,  0x3132 }, { 2,  0x3134 }, // 2:ㄱ, 3:ㄲ, 4:ㄴ
        { 3,  0x3137 }, { 4,  0x3138 }, { 5,  0x3139 }, // 5:ㄷ, 6:ㄸ, 7:ㄹ
        { 6,  0x3141 }, { 7,  0x3142 }, { 8,  0x3143 }, // 8:ㅁ, 9:ㅂ, 10:ㅃ
        { 9,  0x3145 }, { 10, 0x3146 }, { 11, 0x3147 }, // 11:ㅅ, 12:ㅆ, 13:ㅇ
        { 12, 0x3148 }, { 13, 0x3149 }, { 14, 0x314A }, // 14:ㅈ, 15:ㅉ, 16:ㅊ
        { 15, 0x314B }, { 16, 0x314C }, { 17, 0x314D }, // 17:ㅋ, 18:ㅌ, 19:ㅍ
        { 18, 0x314E },                                 // 20:ㅎ
        { -1, 0x0000 }, { -1, 0x0000 }, { -1, 0x0000 }, { -1, 0x0000 }, // 21~24
        { -1, 0x0000 }, { -1, 0x0000 }, { -1, 0x0000 }, { -1, 0x0000 }, // 25~28
        { -1, 0x0000 }, { -1, 0x0000 }, { -1, 0x0000 }                  // 29~31
    };

    // 조합형 중성 값(3~26) -> [유니코드 음절 인덱스, 호환 자모 코드]
    static constexpr int jungsung_table[32][2] = {
        { -1, 0x0000 }, { -1, 0x0000 }, { -1, 0x0000 }, // 0, 1, 2 (2는 채움)
        { 0,  0x314F }, { 1,  0x3150 }, { 2,  0x3151 }, // 3:ㅏ, 4:ㅐ, 5:ㅑ
        { 3,  0x3152 }, { 4,  0x3153 },                 // 6:ㅒ, 7:ㅓ
        { -1, 0x0000 }, { -1, 0x0000 },                 // 8, 9 (빈 값)
        { 5,  0x3154 }, { 6,  0x3155 }, { 7,  0x3156 }, // 10:ㅔ, 11:ㅕ, 12:ㅖ
        { 8,  0x3157 }, { 9,  0x3158 }, { 10, 0x3159 }, // 13:ㅗ, 14:ㅘ, 15:ㅙ
        { -1, 0x0000 }, { -1, 0x0000 },                 // 16, 17 (빈 값)
        { 11, 0x315A }, { 12, 0x315B }, { 13, 0x315C }, // 18:ㅚ, 19:ㅛ, 20:ㅜ
        { 14, 0x315D }, { 15, 0x315E }, { 16, 0x315F }, // 21:ㅝ, 22:ㅞ, 23:ㅟ
        { -1, 0x0000 }, { -1, 0x0000 },                 // 24, 25 (빈 값)
        { 17, 0x3160 }, { 18, 0x3161 }, { 19, 0x3162 }, // 26:ㅠ, 27:ㅡ, 28:ㅢ
        { 20, 0x3163 },                                 // 29:ㅣ
        { -1, 0x0000 }, { -1, 0x0000 }                  // 30~31
    };

    // 조합형 종성 값(2~29) -> [유니코드 음절 인덱스, 호환 자모 코드]
    static constexpr int jongsung_table[32][2] = {
        { -1, 0x0000 },                                 // 0
        { 0,  0x0000 },                                 // 1 (채움)
        { 1,  0x3131 }, { 2,  0x3132 }, { 3,  0x3133 }, // 2:ㄱ, 3:ㄲ, 4:ㄳ
        { 4,  0x3134 }, { 5,  0x3135 }, { 6,  0x3136 }, // 5:ㄴ, 6:ㄵ, 7:ㄶ
        { 7,  0x3137 }, { 8,  0x3139 }, { 9,  0x313A }, // 8:ㄷ, 9:ㄹ, 10:ㄺ
        { 10, 0x313B }, { 11, 0x313C }, { 12, 0x313D }, // 11:ㄻ, 12:ㄼ, 13:ㄽ
        { 13, 0x313E }, { 14, 0x313F }, { 15, 0x3140 }, // 14:ㄾ, 15:ㄿ, 16:ㅀ
        { 16, 0x3141 },                                 // 17:ㅁ
        { -1, 0x0000 },                                 // 18 (빈 값)
        { 17, 0x3142 }, { 18, 0x3144 }, { 19, 0x3145 }, // 19:ㅂ, 20:ㅄ, 21:ㅅ
        { 20, 0x3146 }, { 21, 0x3147 }, { 22, 0x3148 }, // 22:ㅆ, 23:ㅇ, 24:ㅈ
        { 23, 0x314A }, { 24, 0x314B }, { 25, 0x314C }, // 25:ㅊ, 26:ㅋ, 27:ㅌ
        { 26, 0x314D }, { 27, 0x314E },                 // 28:ㅍ, 29:ㅎ
        { -1, 0x0000 }, { -1, 0x0000 }                  // 30, 31
    };

}

void DoKssmToWansung() {
    int whichView = 0;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTVIEW, 0, (LPARAM)&whichView);
    const HWND hSci = (whichView == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;

    int nppEncoding = 0;
    ::SendMessage(nppData._nppHandle, NPPM_GETBUFFERENCODING, 0, (LPARAM)&nppEncoding);
    const int cp = (int)::SendMessage(hSci, SCI_GETCODEPAGE, 0, 0);
    const Sci_Position totalLen = ::SendMessage(hSci, SCI_GETLENGTH, 0, 0);
    if (nppEncoding != 0 || (cp != 0 && cp != 949) || totalLen <= 0) {
        return;
    }

    // [1] Read-only 강제 해제, 이 기능만 이렇게 동작함
    const bool wasReadOnly = (bool)::SendMessage(hSci, SCI_GETREADONLY, 0, 0);
    ::SendMessage(hSci, SCI_SETREADONLY, FALSE, 0);

    // [2] 상태 저장
    const Sci_Position anchorPos = ::SendMessage(hSci, SCI_GETANCHOR, 0, 0);
    const Sci_Position caretPos = ::SendMessage(hSci, SCI_GETCURRENTPOS, 0, 0);
    const Sci_Position firstLine = ::SendMessage(hSci, SCI_GETFIRSTVISIBLELINE, 0, 0);
    Sci_Position anchorShift = 0, caretShift = 0;

    std::vector<char> src(totalLen + 1);
    ::SendMessage(hSci, SCI_GETTEXT, totalLen + 1, (LPARAM)src.data());

    std::vector<wchar_t> wResult;
    wResult.reserve(static_cast<size_t>(totalLen * 1.5));

    // [3] 변환 루프
    const unsigned char* pSrc = reinterpret_cast<const unsigned char*>(src.data());
    for (Sci_Position i = 0; i < totalLen; ) {
        const unsigned char b1 = pSrc[i];
        const size_t prevWSize = wResult.size();

        if (!(b1 & 0x80)) { // ASCII
            wResult.push_back(static_cast<wchar_t>(b1));
            ++i;
        }
        else if (i + 1 < totalLen) { // KSSM (Johab)
            unsigned char b2 = pSrc[i + 1];

            // 추억의 비트 구조: 1 CCCCC JJ | JJJ TTTTT
            const int cho = (b1 >> 2) & 0x1F;
            const int jung = ((b1 & 0x03) << 3) | (b2 >> 5);
            const int jong = b2 & 0x1F;

            bool converted = false;
            if (cho >= 2 && cho <= 20 && jung >= 3 && jung <= 29) {
                const int cIdx = chosung_table[cho][0];
                const int mIdx = jungsung_table[jung][0];
                const int tIdx = (jong >= 1 && jong <= 29) ? jongsung_table[jong][0] : 0;

                if (cIdx != -1 && mIdx != -1) {
                    wResult.push_back(static_cast<wchar_t>(0xAC00 + (cIdx * 588) + (mIdx * 28) + (tIdx > 0 ? tIdx : 0)));
                    converted = true;
                }
            }

            if (!converted) {
                // 조합 실패 시 호환 자모 보존
                if (cho < 32 && chosung_table[cho][1] != 0) wResult.push_back(static_cast<wchar_t>(chosung_table[cho][1]));
                if (jung < 32 && jungsung_table[jung][1] != 0) wResult.push_back(static_cast<wchar_t>(jungsung_table[jung][1]));
                if (jong < 32 && jongsung_table[jong][1] != 0) wResult.push_back(static_cast<wchar_t>(jongsung_table[jong][1]));
            }

            // 커서 위치 복원을 위한 바이트 변화량 계산
            int bytesAfter = 0;
            for (size_t k = prevWSize; k < wResult.size(); ++k) {
                bytesAfter += (wResult[k] < 0x80) ? 1 : 2;
            }
            int currentByteShift = bytesAfter - 2;

            if (i < anchorPos) anchorShift += currentByteShift;
            if (i < caretPos) caretShift += currentByteShift;

            i += 2;
        }
        else { ++i; }
    }

    // [4] 결과 적용
    if (!wResult.empty()) {
        int ansiLen = WideCharToMultiByte(949, 0, wResult.data(), (int)wResult.size(), nullptr, 0, nullptr, nullptr);
        std::vector<char> ansiResult(ansiLen + 1, 0);
        WideCharToMultiByte(949, 0, wResult.data(), (int)wResult.size(), ansiResult.data(), ansiLen, nullptr, nullptr);

        ::SendMessage(hSci, SCI_BEGINUNDOACTION, 0, 0);
        ::SendMessage(hSci, SCI_SETTARGETSTART, 0, 0);
        ::SendMessage(hSci, SCI_SETTARGETEND, totalLen, 0);
        ::SendMessage(hSci, SCI_REPLACETARGET, -1, (LPARAM)ansiResult.data());

        // 위치 복원
        const Sci_Position newMaxLen = ::SendMessage(hSci, SCI_GETLENGTH, 0, 0);
        Sci_Position newAnchor = anchorPos + anchorShift;
        Sci_Position newCaret = caretPos + caretShift;
        if (newAnchor > newMaxLen) newAnchor = newMaxLen;
        if (newCaret > newMaxLen) newCaret = newMaxLen;

        ::SendMessage(hSci, SCI_SETSEL, newAnchor, newCaret);
        ::SendMessage(hSci, SCI_SETFIRSTVISIBLELINE, firstLine, 0);
        ::SendMessage(hSci, SCI_ENDUNDOACTION, 0, 0);
    }

    // [5] 읽기 전용 속성 원상 복구 및 수정 마크 제거
    if (wasReadOnly) {
        ::SendMessage(hSci, SCI_SETSAVEPOINT, 0, 0);
        ::SendMessage(hSci, SCI_SETREADONLY, TRUE, 0);
    }
}
