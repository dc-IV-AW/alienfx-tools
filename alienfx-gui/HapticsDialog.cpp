#include "alienfx-gui.h"
#include "EventHandler.h"
#include "Common.h"

extern void RedrawButton(HWND hDlg, AlienFX_SDK::Afx_colorcode*);
extern bool SetColor(HWND hDlg, AlienFX_SDK::Afx_colorcode*);
//extern groupset* FindMapping(int mid, vector<groupset>* set = conf->active_set);

//extern void RedrawGridButtonZone(RECT* what = NULL, bool recalc = false);
extern void RedrawZoneGrid(DWORD id);

extern EventHandler* eve;

int freqItem;
freq_map* freqBlock = NULL;
HWND hToolTip = NULL;
int cutMove = 0;

static const double log_divider = log(21);

void DrawFreq(HWND hysto) {

	if (hysto) {
		RECT levels_rect;
		GetClientRect(hysto, &levels_rect);
		RECT graphZone = levels_rect;

		HDC hdc_r = GetDC(hysto);

		// Double buff...
		HDC hdc = CreateCompatibleDC(hdc_r);
		HBITMAP hbmMem = CreateCompatibleBitmap(hdc_r, graphZone.right /*- graphZone.left*/, graphZone.bottom /*- graphZone.top*/);

		//SetBkMode(hdc, TRANSPARENT);

		HGDIOBJ hOld = SelectObject(hdc, hbmMem);

		//setting colors:
		SelectObject(hdc, GetStockObject(WHITE_PEN));
		SetBkMode(hdc, TRANSPARENT);

		graphZone.top = 2;
		graphZone.left = 1;
		graphZone.bottom--;

		if (eve->audio) {
			SelectObject(hdc, GetStockObject(WHITE_PEN));
			SelectObject(hdc, GetStockObject(WHITE_BRUSH));
			for (int i = 0; i < NUMBARS; i++) {
				int leftpos = (graphZone.right * i) / NUMBARS + graphZone.left,
					rightpos = (graphZone.right * (i + 1)) / NUMBARS - 2 + graphZone.left;
				Rectangle(hdc, leftpos, (255 - eve->audio->freqs[i]) * (graphZone.bottom - graphZone.top) / 255 + graphZone.top,
					rightpos, graphZone.bottom);
			}
		}
		if (freqBlock) {
			for (auto t = freqBlock->freqID.begin(); t < freqBlock->freqID.end(); t++) {
				int leftpos = (graphZone.right * (*t)) / NUMBARS + graphZone.left,
					rightpos = (graphZone.right * (*t + 1)) / NUMBARS - 2 + graphZone.left;
				int rectop = graphZone.bottom;
				// gray block
				if (eve->audio)
					rectop = (255 - eve->audio->freqs[*t]) * (graphZone.bottom - graphZone.top) / 255 + graphZone.top;
				SelectObject(hdc, GetStockObject(GRAY_BRUSH));
				SelectObject(hdc, GetStockObject(NULL_PEN));
				Rectangle(hdc, leftpos, graphZone.top, rightpos + 2, rectop);

				SetDCPenColor(hdc, RGB(255, 0, 0));
				SelectObject(hdc, GetStockObject(DC_PEN));
				int cutLevel = graphZone.bottom - ((graphZone.bottom - graphZone.top) * freqBlock->hicut / 255);
				Rectangle(hdc, leftpos, cutLevel, rightpos, cutLevel + 2);
				SetDCPenColor(hdc, RGB(0, 255, 0));
				SelectObject(hdc, GetStockObject(DC_PEN));
				cutLevel = graphZone.bottom - ((graphZone.bottom - graphZone.top) * freqBlock->lowcut / 255);
				Rectangle(hdc, leftpos, cutLevel, rightpos, cutLevel - 2);
			}
		}

		BitBlt(hdc_r, 0, 0, levels_rect.right, levels_rect.bottom, hdc, 0, 0, SRCCOPY);

		// Free-up the off-screen DC
		SelectObject(hdc, hOld);

		DeleteObject(hbmMem);
		DeleteDC(hdc);
		ReleaseDC(hysto, hdc_r);
		DeleteDC(hdc_r);
	}
}

void SetMappingData(HWND hDlg) {
	DrawFreq(GetDlgItem(hDlg, IDC_LEVELS));
	RedrawButton(GetDlgItem(hDlg, IDC_BUTTON_LPC), freqBlock ? &freqBlock->colorfrom : NULL);
	RedrawButton(GetDlgItem(hDlg, IDC_BUTTON_HPC), freqBlock ? &freqBlock->colorto : NULL);
}

void SetFreqGroups(HWND hDlg) {
	HWND grp_list = GetDlgItem(hDlg, IDC_FREQ_GROUP);
	groupset* map = conf->FindMapping(eItem);
	ListBox_ResetContent(grp_list);
	freqItem = 0;
	if (map && map->haptics.size()) {
		// Set groups
		for (int j = 0; j < map->haptics.size(); j++) {
			ListBox_AddString(grp_list, ("Group " + to_string(j + 1)).c_str());
		}
		ListBox_SetCurSel(grp_list, 0);
		freqBlock = &map->haptics.front();
	}
	else
		freqBlock = NULL;
	SetMappingData(hDlg);
}

INT_PTR CALLBACK FreqLevels(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT pos;
	groupset* map = conf->FindMapping(eItem);
	GetClientRect(hDlg, &pos);
	pos.right--;
	pos.bottom;
	byte cIndex = (byte)(GET_X_LPARAM(lParam) * NUMBARS / pos.right);
	int clickLevel = min((pos.bottom - GET_Y_LPARAM(lParam)) * 255 / (pos.bottom - 3), 255);

	switch (message) {
	case WM_LBUTTONDOWN: {
		if (freqBlock)
			if (find(freqBlock->freqID.begin(), freqBlock->freqID.end(), cIndex) != freqBlock->freqID.end()) {
					if (abs(clickLevel - freqBlock->hicut) < 10)
						cutMove = 1;
					if (abs(clickLevel - freqBlock->lowcut) < 10)
						cutMove = 2;
					break;
			}
	} break;
	case WM_LBUTTONUP: {
		if (freqBlock) {
			auto idx = find(freqBlock->freqID.begin(), freqBlock->freqID.end(), cIndex);
			if (idx != freqBlock->freqID.end()) {
				if (!cutMove)
					freqBlock->freqID.erase(idx);
			}
			else
				freqBlock->freqID.push_back(cIndex);
		}
		cutMove = 0;
	} break;
	case WM_MOUSEMOVE: {
		if (wParam & MK_LBUTTON) {
			switch (cutMove) {
			case 1: freqBlock->hicut = clickLevel; break;
			case 2: freqBlock->lowcut = clickLevel; break;
			}
		}
		int freq = eve->audio ? eve->audio->pwfx->nSamplesPerSec >> 1 : 22050;
		SetToolTip(hToolTip, "Freq: " + to_string(cIndex ? freq - (int)round((log(21 - cIndex) * freq / log_divider)) : 20) +
			"-" + to_string(freq - (int)round((log(20 - cIndex) * freq / log_divider))) + " Hz, Level: " +
			to_string(clickLevel * 100 / 255));
	} break;
	case WM_PAINT:
		DrawFreq(hDlg);
		break;
	case WM_TIMER:
		if (eve->audio && IsWindowVisible(hDlg)) {
			//DebugPrint("Haptics UI update...\n");
			DrawFreq(hDlg);
		}
		break;
	case WM_ERASEBKGND:
		return true;
	}
	return DefWindowProc(hDlg, message, wParam, lParam);;
}

BOOL CALLBACK TabHapticsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND grp_list = GetDlgItem(hDlg, IDC_FREQ_GROUP),
	     gridTab = GetDlgItem(hDlg, IDC_TAB_COLOR_GRID);

	//groupset *map = conf->FindMapping(eItem);

	switch (message) {
	case WM_INITDIALOG:
	{
		CheckDlgButton(hDlg, IDC_RADIO_INPUT, conf->hap_inpType ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_RADIO_OUTPUT, conf->hap_inpType ? BST_UNCHECKED : BST_CHECKED);

		SetWindowLongPtr(GetDlgItem(hDlg, IDC_LEVELS), GWLP_WNDPROC, (LONG_PTR)FreqLevels);
		hToolTip = CreateToolTip(GetDlgItem(hDlg, IDC_LEVELS), hToolTip);

		SetFreqGroups(hDlg);

		// Start UI update thread...
		SetTimer(GetDlgItem(hDlg, IDC_LEVELS), 0, 50, NULL);
	}
	break;
	case WM_COMMAND:
	{
		groupset* map = conf->FindMapping(eItem);
		switch (LOWORD(wParam)) {
		case IDC_RADIO_OUTPUT: case IDC_RADIO_INPUT:
			conf->hap_inpType = LOWORD(wParam) == IDC_RADIO_INPUT;
			CheckDlgButton(hDlg, IDC_RADIO_INPUT, conf->hap_inpType ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hDlg, IDC_RADIO_OUTPUT, conf->hap_inpType ? BST_UNCHECKED : BST_CHECKED);
			if (eve->audio)
				eve->audio->RestartDevice(conf->hap_inpType);
		break;
		case IDC_FREQ_GROUP:
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE:
				if (map) {
					freqItem = ListBox_GetCurSel(grp_list);
					freqBlock = &map->haptics[freqItem];
					SetMappingData(hDlg);
				}
			break;
			}
			break;
		case IDC_BUT_ADD_GROUP:
		{
			if (map) {
				map->haptics.push_back({0,0,255});
				int pos = ListBox_AddString(grp_list, ("Group " + to_string(map->haptics.size())).c_str());
				ListBox_SetCurSel(grp_list, pos);
				freqBlock = &map->haptics.back();
				SetMappingData(hDlg);
				RedrawZoneGrid(map->group);
			}
		} break;
		case IDC_BUT_REM_GROUP:
			if (freqBlock) {
				map->haptics.erase(map->haptics.begin() + freqItem);
				ListBox_DeleteString(grp_list, freqItem);
				if (map->haptics.size()) {
					if (freqItem) {
						freqItem--;
						freqBlock = &(*(map->haptics.begin() + freqItem));
					}
					else
						freqBlock = &map->haptics.front();
					ListBox_SetCurSel(grp_list, freqItem);
				}
				else
					freqBlock = NULL;
				SetMappingData(hDlg);
				RedrawZoneGrid(map->group);
			}
			break;
		case IDC_BUTTON_LPC:
			if (freqBlock) {
				SetColor(GetDlgItem(hDlg, IDC_BUTTON_LPC), &freqBlock->colorfrom);
				RedrawZoneGrid(map->group);
			}
			break;
		case IDC_BUTTON_HPC:
			if (freqBlock) {
				SetColor(GetDlgItem(hDlg, IDC_BUTTON_HPC), &freqBlock->colorto);
				RedrawZoneGrid(map->group);
			}
			break;
		case IDC_BUTTON_REMOVE:
			if (freqBlock) {
				freqBlock->freqID.clear();
				SetMappingData(hDlg);
				RedrawZoneGrid(map->group);
			}
		}
	} break;
	case WM_APP + 2: {
		SetFreqGroups(hDlg);
	} break;
	case WM_DRAWITEM:
		switch (((DRAWITEMSTRUCT *) lParam)->CtlID) {
		case IDC_BUTTON_LPC: case IDC_BUTTON_HPC:
		{
			AlienFX_SDK::Afx_colorcode* c = NULL;
			if (freqBlock)
				if (((DRAWITEMSTRUCT *) lParam)->CtlID == IDC_BUTTON_LPC)
					c = &freqBlock->colorfrom;
				else
					c = &freqBlock->colorto;
			RedrawButton(((DRAWITEMSTRUCT *) lParam)->hwndItem, c);
			return true;
		}
		break;
		}
		return false;
	break;
	default: return false;
	}

	return true;
}
