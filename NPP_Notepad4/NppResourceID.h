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


// NPP_ResourceID.h : NPP에서 정의된 상수 모음
////////////////////////////////////////////////////////////

#pragma once

// ============================================================================
// [SECTION 1] Notepad++ 본체 고유 리소스 ID (NPP 소스코드 호환)
// ============================================================================

// 1) 메인 메뉴 명령 ID (menuCmdID.h 스타일)
#define    IDM                             40000

#define    IDM_SEARCH                      (IDM + 3000)
#define    IDM_SEARCH_FIND                 (IDM_SEARCH + 1) // 43001

// 2) 찾기 창 내부 컨트롤 ID (FindReplaceDlg_rc.h 스타일)
#define    IDFINDWHAT                      1601             // '찾을 내용' 콤보박스
#define    IDREGEXP                        1625             // '정규식' 라디오 버튼


// ============================================================================
// [SECTION 2] NPP_Notepad4 플러그인 독자 제어용 고유 ID (오프셋 구조화)
// ============================================================================

// Win32 커스텀 ID 표준 관례인 1000번을 시작값으로 정의
#define    ID_PLUGIN_SUBCLASS_BASE         1000

// 향후 추가될 서브클래스 감시 대상들은 이 아래로 규칙적으로 증가
#define    SUBCLASS_ID_FIND_DLG            (ID_PLUGIN_SUBCLASS_BASE + 1) // 1001

