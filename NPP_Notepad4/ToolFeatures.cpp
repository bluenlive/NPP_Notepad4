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
#include "ToolFeatures.h"
#include "Common.h"
#include <vector>
#include <string>

extern NppData nppData;
extern int g_cachedLangType; // NPPN_LANGCHANGED 등에서 캐싱된 언어 타입

void DoCalculate()
{

}

void DoEvalJS()
{

}

void DoRemoveTags()
{
    // 1. 현재 활성화된 Scintilla 핸들 가져오기
    int whichView = 0;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTVIEW, 0, (LPARAM)&whichView);
    const HWND hSci = (whichView == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
    if (!hSci) return;

    // 2. 선택 영역 범위 확인
    const Sci_Position selStart = ::SendMessage(hSci, SCI_GETSELECTIONSTART, 0, 0);
    const Sci_Position selEnd = ::SendMessage(hSci, SCI_GETSELECTIONEND, 0, 0);
    if (selStart == selEnd) return;

    // 3. 선택된 UTF-8 텍스트 가져오기
    const Sci_Position lenUtf8 = selEnd - selStart;
    std::vector<char> bufUtf8(lenUtf8 + 1, 0);

    Sci_TextRangeFull tr;
    tr.chrg.cpMin = selStart;
    tr.chrg.cpMax = selEnd;
    tr.lpstrText = bufUtf8.data();
    ::SendMessage(hSci, SCI_GETTEXTRANGEFULL, 0, (LPARAM)&tr);
    bufUtf8[lenUtf8] = 0;

    // 4. UTF-8 -> UTF-16 (WCHAR) 변환 (알고리즘 호환용)
    int cchTextW = MultiByteToWideChar(CP_UTF8, 0, bufUtf8.data(), -1, NULL, 0);
    std::vector<WCHAR> pszTextW(cchTextW);
    MultiByteToWideChar(CP_UTF8, 0, bufUtf8.data(), -1, pszTextW.data(), cchTextW);
    --cchTextW; // NULL 문자 제외

    // 5. 결과 버퍼 확보 및 알고리즘 변수 초기화
    struct ChangeHunk {
        Sci_Position start; // 시작 위치
        Sci_Position end;   // 끝 위치
        std::string replacement; // 교체될 텍스트 (UTF-8)
    };
    std::vector<ChangeHunk> changes;

    bool bInScript = false;
    bool bInStyle = false;

    // --- Notepad4 알고리즘 시작 ---
    auto IsBlockTag = [](const WCHAR* name) {
        static constexpr const WCHAR* blocks[] = {
            L"div", L"p", L"br", L"hr", L"tr", L"li", L"h1", L"h2", L"h3",
            L"h4", L"h5", L"h6", L"blockquote", L"pre", L"section",
            L"article", L"header", L"footer", L"table", L"head", L"title",
            L"meta", L"body", L"html", L"doctype", L"ul", L"ol", L"nav", L"aside",
            L"script", L"style"
        };
        for (const auto& b : blocks) {
            if (_wcsicmp(name, b) == 0)
                return true;
        }
        return false;
    };
    
    auto GetBytePos = [&](const UINT idx) -> Sci_Position {
        return selStart + (Sci_Position)WideCharToMultiByte(CP_UTF8, 0, pszTextW.data(), idx, NULL, 0, NULL, NULL);
    };

    for (int s = 0; s < cchTextW; ) {
        if (pszTextW[s] == L'<' || bInScript || bInStyle) {
            const int hunkStartIdx = s;
            bool hunkIsBlock = false;

            while (s < cchTextW) {
                if (pszTextW[s] == L'<') {
                    // 태그 분석 (tagName 추출)
                    WCHAR tagName[32] = { 0 };
                    UINT t = 0;
                    UINT peek = s + 1;
                    const bool isClosing = (peek < (UINT)cchTextW && pszTextW[peek] == L'/');
                    if (isClosing)
                        ++peek;
                    else if (peek < (UINT)cchTextW && (pszTextW[peek] == L'!' || pszTextW[peek] == L'?'))
                        ++peek;

                    while (peek < (UINT)cchTextW && t < 31 && iswalnum(pszTextW[peek])) {
                        tagName[t++] = pszTextW[peek++];
                    }
                    tagName[t] = L'\0';

                    if (IsBlockTag(tagName))
                        hunkIsBlock = true;

                    // 스크립트/스타일 상태 제어
                    if (!isClosing) {
                        if (_wcsicmp(tagName, L"script") == 0)
                            bInScript = true;
                        else if (_wcsicmp(tagName, L"style") == 0)
                            bInStyle = true;
                    }
                    else {
                        if (bInScript && _wcsicmp(tagName, L"script") == 0)
                            bInScript = false;
                        else if (bInStyle && _wcsicmp(tagName, L"style") == 0)
                            bInStyle = false;
                    }

                    // 태그 끝(>)까지 건너뛰기
                    WCHAR quote = 0;
                    while (s < cchTextW) {
                        if (quote == 0) {
                            if (pszTextW[s] == L'\'' || pszTextW[s] == L'\"')
                                quote = pszTextW[s];
                            else if (pszTextW[s] == L'>') {
                                ++s;
                                break;
                            }
                        }
                        else {
                            if (pszTextW[s] == quote)
                                quote = 0;
                            else if (pszTextW[s] == L'\\' && s + 1 < cchTextW)
                                ++s;
                        }
                        ++s;
                    }
                    if (!bInScript && !bInStyle && (s >= cchTextW || pszTextW[s] != L'<'))
                        break;
                }
                else if (bInScript || bInStyle) {
                    ++s; // 스크립트 내부 내용 건너뛰기
                }
                else {
                    break;
                }
            }

            // WCHAR 인덱스를 Scintilla 바이트 위치로 변환
            const Sci_Position bStart = GetBytePos(hunkStartIdx);
            const Sci_Position bEnd = GetBytePos(s);

            // 앞뒤 문맥을 보고 삽입할 구분자(공백/줄바꿈) 결정
            std::string repText = "";
            const WCHAR prevChar = (hunkStartIdx > 0) ? pszTextW[hunkStartIdx - 1] : 0;
            const WCHAR nextChar = (s < cchTextW) ? pszTextW[s] : 0;

            if (hunkIsBlock) {
                if (prevChar != L'\n' && prevChar != 0)
                    repText = "\r\n";
            }
            else {
                if (prevChar != L' ' && prevChar != L'\t' && prevChar != L'\n' && prevChar != 0 &&
                    nextChar != L' ' && nextChar != L'\t' && nextChar != L'\n' && nextChar != 0) {
                    repText = " ";
                }
            }

            // 원본과 교체될 텍스트가 다를 때만 기록 (이미 태그가 지워진 상태인 경우 방지)
            changes.push_back({ bStart, bEnd, repText });
        }
        else {
            ++s;
        }
    }
    // --- 기존 알고리즘 끝 ---

    // 6. 변경 사항 반영 (역순으로!)
    if (!changes.empty()) {
        ::SendMessage(hSci, SCI_BEGINUNDOACTION, 0, 0);
        for (auto it = changes.rbegin(); it != changes.rend(); ++it) {
            ::SendMessage(hSci, SCI_SETTARGETSTART, it->start, 0);
            ::SendMessage(hSci, SCI_SETTARGETEND, it->end, 0);
            ::SendMessage(hSci, SCI_REPLACETARGET, it->replacement.length(), (LPARAM)it->replacement.c_str());
        }
        ::SendMessage(hSci, SCI_ENDUNDOACTION, 0, 0);
    }
}

void DoRemoveComments()
{
    int whichView = 0;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTVIEW, 0, (LPARAM)&whichView);
    const HWND hSci = (whichView == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
    if (!hSci) return;

    Sci_Position startPos = ::SendMessage(hSci, SCI_GETSELECTIONSTART, 0, 0);
    Sci_Position endPos = ::SendMessage(hSci, SCI_GETSELECTIONEND, 0, 0);
    if (startPos == endPos) return;

    const Sci_Position docLen = ::SendMessage(hSci, SCI_GETLENGTH, 0, 0);

    // --- [람다 1] Python 트리플 쿼트 문맥 판별 ---
    auto IsPythonStandaloneDocstring = [](const HWND hSci, const Sci_Position blockStart) -> bool {
        const Sci_Line line = ::SendMessage(hSci, SCI_LINEFROMPOSITION, blockStart, 0);
        const Sci_Position lineStart = ::SendMessage(hSci, SCI_POSITIONFROMLINE, line, 0);
        bool isFirstOnLine = true;
        for (Sci_Position p = lineStart; p < blockStart; ++p) {
            const int ch = (unsigned char)::SendMessage(hSci, SCI_GETCHARAT, p, 0);
            if (ch != ' ' && ch != '\t') { isFirstOnLine = false; break; }
        }
        Sci_Position p = (isFirstOnLine ? lineStart : blockStart) - 1;
        while (p >= 0) {
            const int ch = (unsigned char)::SendMessage(hSci, SCI_GETCHARAT, p, 0);
            const int style = (int)::SendMessage(hSci, SCI_GETSTYLEAT, p, 0);
            if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || style == 1 || style == 12) {
                --p; continue;
            }
            if (strchr("(=[,{+", ch)) return false;
            break;
        }
        return isFirstOnLine;
    };

    // --- [람다 2] 스타일 판별 (이중 switch-case) ---
    auto IsCommentStyle = [&](const HWND hSci, const Sci_Position pos, const int style) -> bool {
        switch (g_cachedLangType) {
        case L_C:
        case L_CPP:
            switch (style) {
            case SCE_C_COMMENT:
            case SCE_C_COMMENTLINE:
            case SCE_C_COMMENTDOC:
            case SCE_C_COMMENTLINEDOC:
            case SCE_C_COMMENTDOCKEYWORD:
            case SCE_C_PREPROCESSORCOMMENT:
            case SCE_C_PREPROCESSORCOMMENTDOC:
                return true;
            default: return false;
            }
        case L_PYTHON:
            switch (style) {
            case SCE_P_COMMENTLINE:
            case SCE_P_COMMENTBLOCK:
                return true;
            case SCE_P_TRIPLE:
            case SCE_P_TRIPLEDOUBLE:
            case SCE_P_FTRIPLE:
            case SCE_P_FTRIPLEDOUBLE:
            {
                Sci_Position blockStart = pos;
                while (blockStart > 0 && (int)::SendMessage(hSci, SCI_GETSTYLEAT, blockStart - 1, 0) == style) --blockStart;
                return IsPythonStandaloneDocstring(hSci, blockStart);
            }
            default: return false;
            }
        case L_HTML:
        case L_XML:
            switch (style) {
            case SCE_H_COMMENT:
            case SCE_H_XCCOMMENT:
            case SCE_H_SGML_COMMENT:
            case SCE_H_SGML_1ST_PARAM_COMMENT:
            case SCE_HJ_COMMENT:
            case SCE_HJ_COMMENTLINE:
            case SCE_HJ_COMMENTDOC:
            case SCE_HJA_COMMENT:
            case SCE_HJA_COMMENTLINE:
            case SCE_HJA_COMMENTDOC:
            case SCE_HB_COMMENTLINE:
            case SCE_HBA_COMMENTLINE:
            case SCE_HP_COMMENTLINE:
            case SCE_HPA_COMMENTLINE:
            case SCE_HPHP_COMMENT:
            case SCE_HPHP_COMMENTLINE:
                return true;
            default: return false;
            }
        default: return false;
        }
    };

    // --- [람다 3] 블록 주석 여부 판별 (이중 switch-case) ---
    auto IsBlockCommentStyle = [](const int style) -> bool {
        switch (g_cachedLangType) {
        case L_C:
        case L_CPP:
            switch (style) {
            case SCE_C_COMMENT:
            case SCE_C_COMMENTDOC:
            case SCE_C_COMMENTDOCKEYWORD:
            case SCE_C_PREPROCESSORCOMMENTDOC:
                return true;
            default: return false;
            }
        case L_PYTHON:
            switch (style) {
            case SCE_P_COMMENTBLOCK:
            case SCE_P_TRIPLE:
            case SCE_P_TRIPLEDOUBLE:
            case SCE_P_FTRIPLE:
            case SCE_P_FTRIPLEDOUBLE:
                return true;
            default: return false;
            }
        case L_HTML:
        case L_XML:
            switch (style) {
            case SCE_H_COMMENT:
            case SCE_H_XCCOMMENT:
            case SCE_H_SGML_COMMENT:
                return true;
            default: return false;
            }
        default: return false;
        }
    };

    // --- [람다 4] 제거 실행 (멀티라인 대응) ---
    auto ProcessCommentRemoval = [&](const Sci_Position start, const Sci_Position end) {
        // 시작 위치와 끝 위치의 행 번호를 각각 계산
        const Sci_Line lineS = ::SendMessage(hSci, SCI_LINEFROMPOSITION, start, 0);
        const Sci_Line lineE = ::SendMessage(hSci, SCI_LINEFROMPOSITION, end, 0);

        const Sci_Position fullStartPos = ::SendMessage(hSci, SCI_POSITIONFROMLINE, lineS, 0);
        const Sci_Position fullEndPos = ::SendMessage(hSci, SCI_GETLINEENDPOSITION, lineE, 0);

        auto IsSpaceExtra = [](const unsigned char c) { return c == ' ' || c == '\t' || c == 0xA0; };

        // 시작 부분 공백 트리밍
        Sci_Position finalStart = start;
        while (finalStart > fullStartPos && IsSpaceExtra((unsigned char)::SendMessage(hSci, SCI_GETCHARAT, finalStart - 1, 0))) {
            --finalStart;
        }

        // 끝 부분 공백 트리밍
        Sci_Position finalEnd = end;
        while (finalEnd < fullEndPos && IsSpaceExtra((unsigned char)::SendMessage(hSci, SCI_GETCHARAT, finalEnd, 0))) {
            ++finalEnd;
        }

        // 주석 범위가 시작 행의 처음부터 끝 행의 마지막까지를 모두 차지한다면 (즉, 행 전체가 주석군)
        if (finalStart == fullStartPos && finalEnd == fullEndPos) {
            // 마지막 행의 줄바꿈 문자까지 포함해서 삭제
            Sci_Position nextLineStart = ::SendMessage(hSci, SCI_POSITIONFROMLINE, lineE + 1, 0);
            if (nextLineStart > 0)
                ::SendMessage(hSci, SCI_DELETERANGE, fullStartPos, nextLineStart - fullStartPos);
            else
                ::SendMessage(hSci, SCI_DELETERANGE, fullStartPos, fullEndPos - fullStartPos);
        }
        else {
            // 행의 일부인 경우 주석 영역만 삭제
            ::SendMessage(hSci, SCI_DELETERANGE, finalStart, finalEnd - finalStart);
        }
    };

    // --- [람다 5] 스타일 패밀리 판별 ---
    auto IsSameFamily = [](const int s1, const int s2) {
        if (g_cachedLangType == L_C || g_cachedLangType == L_CPP) {
            auto isC = [](const int s) {
                return (s == SCE_C_COMMENT || s == SCE_C_COMMENTLINE ||
                        s == SCE_C_COMMENTDOC || s == SCE_C_COMMENTLINEDOC ||
                        s == SCE_C_COMMENTDOCKEYWORD || s == SCE_C_PREPROCESSORCOMMENT ||
                        s == SCE_C_PREPROCESSORCOMMENTDOC);
                };
            return isC(s1) && isC(s2);
        }
        if (g_cachedLangType == L_PYTHON) {
            return ((s1 == SCE_P_COMMENTLINE || s1 == SCE_P_COMMENTBLOCK) &&
                    (s2 == SCE_P_COMMENTLINE || s2 == SCE_P_COMMENTBLOCK));
        }
        return s1 == s2;
    };

    // 블록 주석 범위 확장 보정
    const int startStyle = (int)::SendMessage(hSci, SCI_GETSTYLEAT, startPos, 0);
    if (IsBlockCommentStyle(startStyle)) {
        while (startPos > 0 && (int)::SendMessage(hSci, SCI_GETSTYLEAT, startPos - 1, 0) == startStyle)
            --startPos;
    }
    if (endPos > 0) {
        int endStyle = (int)::SendMessage(hSci, SCI_GETSTYLEAT, endPos - 1, 0);
        if (IsBlockCommentStyle(endStyle)) {
            while (endPos < docLen && (int)::SendMessage(hSci, SCI_GETSTYLEAT, endPos, 0) == endStyle)
                ++endPos;
        }
    }

    const Sci_Line startLine = ::SendMessage(hSci, SCI_LINEFROMPOSITION, startPos, 0);
    const Sci_Line endLine = ::SendMessage(hSci, SCI_LINEFROMPOSITION, endPos, 0);
    const Sci_Position targetStart = ::SendMessage(hSci, SCI_POSITIONFROMLINE, startLine, 0);
    const Sci_Position targetEnd = ::SendMessage(hSci, SCI_GETLINEENDPOSITION, endLine, 0);

    // 실행 루프
    ::SendMessage(hSci, SCI_BEGINUNDOACTION, 0, 0);
    for (Sci_Position i = targetEnd - 1; i >= targetStart; --i) {
        const int style = (int)::SendMessage(hSci, SCI_GETSTYLEAT, i, 0);
        if (IsCommentStyle(hSci, i, style)) {
            Sci_Position commentEnd = i + 1;
            Sci_Position commentStart = i;
            const Sci_Line curLine = ::SendMessage(hSci, SCI_LINEFROMPOSITION, i, 0);
            const Sci_Position curLineStart = ::SendMessage(hSci, SCI_POSITIONFROMLINE, curLine, 0);
            const Sci_Position curLineEnd = ::SendMessage(hSci, SCI_GETLINEENDPOSITION, curLine, 0);
            if (commentEnd > curLineEnd)
                commentEnd = curLineEnd;

            const bool isBlock = IsBlockCommentStyle(style);
            const Sci_Position limit = isBlock ? targetStart : curLineStart;

            while (commentStart > limit) {
                int prevStyle = (int)::SendMessage(hSci, SCI_GETSTYLEAT, commentStart - 1, 0);
                if (!IsCommentStyle(hSci, commentStart - 1, prevStyle))
                    break;
                if (!isBlock && !IsSameFamily(style, prevStyle))
                    break;
                --commentStart;
            }
            i = commentStart;
            ProcessCommentRemoval(commentStart, commentEnd);
        }
    }
    ::SendMessage(hSci, SCI_ENDUNDOACTION, 0, 0);
}
