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

void DoHanjaToHangul()
{

}

void DoHangulDecomp()
{

}

void DoToggleComposition()
{

}

namespace {

    // 조합형 초성 값(2~20) -> [유니코드 음절 인덱스, 호환 자모 코드]
    static const int chosung_table[32][2] = {
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
    static const int jungsung_table[32][2] = {
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
    static const int jongsung_table[32][2] = {
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
    HWND hSci = (whichView == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;

    // [1] Read-only 강제 해제, 이 기능만 이렇게 동작함
    const bool wasReadOnly = (bool)::SendMessage(hSci, SCI_GETREADONLY, 0, 0);
    ::SendMessage(hSci, SCI_SETREADONLY, FALSE, 0);

    Sci_Position totalLen = ::SendMessage(hSci, SCI_GETLENGTH, 0, 0);
    if (totalLen <= 0) {
        if (wasReadOnly) ::SendMessage(hSci, SCI_SETREADONLY, TRUE, 0);
        return;
    }

    // [2] 상태 저장
    Sci_Position anchorPos = ::SendMessage(hSci, SCI_GETANCHOR, 0, 0);
    Sci_Position caretPos = ::SendMessage(hSci, SCI_GETCURRENTPOS, 0, 0);
    Sci_Position firstLine = ::SendMessage(hSci, SCI_GETFIRSTVISIBLELINE, 0, 0);
    Sci_Position anchorShift = 0, caretShift = 0;

    std::vector<char> src(totalLen + 1);
    ::SendMessage(hSci, SCI_GETTEXT, totalLen + 1, (LPARAM)src.data());

    std::vector<wchar_t> wResult;
    wResult.reserve(static_cast<size_t>(totalLen * 1.5));

    // [3] 변환 루프
    const unsigned char* pSrc = reinterpret_cast<const unsigned char*>(src.data());
    for (Sci_Position i = 0; i < totalLen; ) {
        unsigned char b1 = pSrc[i];
        size_t prevWSize = wResult.size();

        if (!(b1 & 0x80)) { // ASCII
            wResult.push_back(static_cast<wchar_t>(b1));
            i++;
        }
        else if (i + 1 < totalLen) { // KSSM (Johab)
            unsigned char b2 = pSrc[i + 1];

            // 추억의 비트 구조: 1 CCCCC JJ | JJJ TTTTT
            int cho = (b1 >> 2) & 0x1F;
            int jung = ((b1 & 0x03) << 3) | (b2 >> 5);
            int jong = b2 & 0x1F;

            bool converted = false;
            if (cho >= 2 && cho <= 20 && jung >= 3 && jung <= 29) {
                int cIdx = chosung_table[cho][0];
                int mIdx = jungsung_table[jung][0];
                int tIdx = (jong >= 1 && jong <= 29) ? jongsung_table[jong][0] : 0;

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
        else { i++; }
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
        Sci_Position newMaxLen = ::SendMessage(hSci, SCI_GETLENGTH, 0, 0);
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
