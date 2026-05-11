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


// --- Scintilla 기본 타입 정의 (Scintilla.h에 없다면 필요) ---
#ifndef Sci_Position
typedef intptr_t Sci_Position;
#endif
#ifndef Sci_Line
typedef intptr_t Sci_Line;
#endif

// --- Scintilla Lexer Styles (Notepad_plus_msgs.h에는 없는 값들) ---

// ----------------------------------------------------------------------------
// Scintilla Lexer Style Definitions (Extracted from Lexilla Standards)
// These values are specific to the Notepad++ environment.
// ----------------------------------------------------------------------------

// --- C/C++ Styles (SCE_C_...) ---
#define SCE_C_COMMENT 1
#define SCE_C_COMMENTLINE 2
#define SCE_C_COMMENTDOC 3
#define SCE_C_COMMENTLINEDOC 15
#define SCE_C_COMMENTDOCKEYWORD 17
#define SCE_C_PREPROCESSORCOMMENT 23
#define SCE_C_PREPROCESSORCOMMENTDOC 24

// --- Python Styles (SCE_P_...) ---
#define SCE_P_COMMENTLINE 1
#define SCE_P_TRIPLE 6
#define SCE_P_TRIPLEDOUBLE 7
#define SCE_P_COMMENTBLOCK 12
#define SCE_P_FTRIPLE 18
#define SCE_P_FTRIPLEDOUBLE 19

// --- HTML/XML & Sub-Lexer Styles (SCE_H_..., SCE_HJ_..., etc.) ---
// HTML/XML Core
#define SCE_H_COMMENT 9
#define SCE_H_XCCOMMENT 20
#define SCE_H_SGML_COMMENT 29
#define SCE_H_SGML_1ST_PARAM_COMMENT 30

// JavaScript in HTML (HJ/HJA)
#define SCE_HJ_COMMENT 42
#define SCE_HJ_COMMENTLINE 43
#define SCE_HJ_COMMENTDOC 44
#define SCE_HJA_COMMENT 57
#define SCE_HJA_COMMENTLINE 58
#define SCE_HJA_COMMENTDOC 59

// VBScript in HTML (HB/HBA)
#define SCE_HB_COMMENTLINE 72
#define SCE_HBA_COMMENTLINE 82

// Python in HTML (HP/HPA)
#define SCE_HP_COMMENTLINE 92
#define SCE_HPA_COMMENTLINE 107

// PHP in HTML (HPHP)
#define SCE_HPHP_COMMENT 124
#define SCE_HPHP_COMMENTLINE 125

void DoCalculate();
void DoEvalJS();
void DoRemoveTags();
void DoRemoveComments();
