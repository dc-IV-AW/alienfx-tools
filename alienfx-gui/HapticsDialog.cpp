#include "alienfx-gui.h"
#include "EventHandler.h"

extern void SwitchTab(int);
extern void RedrawButton(HWND hDlg, unsigned id, AlienFX_SDK::Colorcode);
extern HWND CreateToolTip(HWND hwndParent, HWND oldTip);
extern void SetSlider(HWND tt, int value);
extern int UpdateLightList(HWND light_list, FXHelper *fxhl, int flag = 0);
extern bool SetColor(HWND hDlg, int id, AlienFX_SDK::Colorcode*);
extern haptics_map *FindHapMapping(int lid);

extern EventHandler* eve;
extern int eItem;

void DrawFreq(HWND hDlg, int *freq) {
	unsigned i, rectop;
	char szSize[100]; //for labels

	HWND hysto = GetDlgItem(hDlg, IDC_LEVELS);

	if (hysto) {
		RECT levels_rect, graphZone;
		GetClientRect(hysto, &levels_rect);
		graphZone = levels_rect;

		HDC hdc_r = GetDC(hysto);

		// Double buff...
		HDC hdc = CreateCompatibleDC(hdc_r);
		HBITMAP hbmMem = CreateCompatibleBitmap(hdc_r, graphZone.right - graphZone.left, graphZone.bottom - graphZone.top);

		SetBkMode(hdc, TRANSPARENT);

		HGDIOBJ hOld = SelectObject(hdc, hbmMem);

		//setting colors:
		//SetDCBrushColor(hdc, RGB(255, 255, 255));
		//SetDCPenColor(hdc, RGB(255, 255, 39));
		SetTextColor(hdc, RGB(255, 255, 255));
		//SelectObject(hdc, GetStockObject(DC_BRUSH));
		SelectObject(hdc, GetStockObject(WHITE_PEN));
		SetBkMode(hdc, TRANSPARENT);

		if (conf->hap_conf->showAxis) {
			//draw x axis:
			MoveToEx(hdc, 10, levels_rect.bottom - 21, (LPPOINT) NULL);
			LineTo(hdc, levels_rect.right - 10, levels_rect.bottom - 21);
			LineTo(hdc, levels_rect.right - 15, levels_rect.bottom - 26);
			MoveToEx(hdc, levels_rect.right - 10, levels_rect.bottom - 21, (LPPOINT) NULL);
			LineTo(hdc, levels_rect.right - 15, levels_rect.bottom - 16);
			//TextOut(hdc, rcClientP->right - 45, rcClientP->bottom - 27, "f(kHz)", 6);

			//draw y axis:
			MoveToEx(hdc, 10, levels_rect.bottom - 21, (LPPOINT) NULL);
			LineTo(hdc, 10, 10);
			LineTo(hdc, 15, 15);
			MoveToEx(hdc, 10, 10, (LPPOINT) NULL);
			LineTo(hdc, 5, 15);
			//TextOut(hdc, 15, 10, "[Power]", 7);
			//wsprintf(szSize, "%6d", (int)y_scale);
			//TextOut(hdc, 150, 10, szSize, 6);
			//TextOut(hdc, 10, 40, "255", 3);
			//TextOut(hdc, 10, (rcClientP->bottom) / 2, "128", 3);
			//TextOut(hdc, 10, rcClientP->bottom - 35, "  0", 3);
			//axis_draw = false;
			int oldvalue = (-1);
			double coeff = 22 / (log(22.0));
			for (i = 0; i <= 22; i++) {
				int frq = int(22 - round((log(22.0 - i) * coeff)));
				if (frq > oldvalue) {
					wsprintf(szSize, "%2d", frq);
					TextOut(hdc, ((levels_rect.right - 20) * i) / 22 + 10, levels_rect.bottom - 20, szSize, 2);
					oldvalue = frq;
				}
			}
			graphZone.top = 10;
			graphZone.bottom -= 21;
			graphZone.left = 10;
			graphZone.right -= 20;
		} else {
			graphZone.top = 2;
			graphZone.left = 1;
			graphZone.bottom--;
		}
		if (freq)
			for (i = 0; i < NUMBARS; i++) {
				rectop = (255 - freq[i]) * (graphZone.bottom - graphZone.top) / 255 + graphZone.top;
				Rectangle(hdc, (graphZone.right * i) / NUMBARS + graphZone.left, rectop,
						  (graphZone.right * (i + 1)) / NUMBARS - 2 + graphZone.left, graphZone.bottom);
				//wsprintf(szSize, "%3d", freq[i]);
				//TextOut(hdc, ((rcClientP->right - 20) * i) / config->numbars + 10, rectop - 15, szSize, 3);
			}

		BitBlt(hdc_r, 0, 0, levels_rect.right - levels_rect.left, levels_rect.bottom - levels_rect.top, hdc, 0, 0, SRCCOPY);

		// Free-up the off-screen DC
		SelectObject(hdc, hOld);

		DeleteObject(hbmMem);
		DeleteDC(hdc);
		ReleaseDC(hysto, hdc_r);
		DeleteDC(hdc_r);
	}
}

void RemoveHapMapping(haptics_map *map) {
	for (auto Iter = conf->hap_conf->haptics.begin(); Iter != conf->hap_conf->haptics.end(); Iter++)
		if (Iter->devid == map->devid && Iter->lightid == map->lightid) {
			conf->hap_conf->haptics.erase(Iter);
			break;
		}
}

void SetMappingData(HWND hDlg, haptics_map* map) {
	HWND hLowSlider = GetDlgItem(hDlg, IDC_SLIDER_LOWCUT);
	HWND hHiSlider = GetDlgItem(hDlg, IDC_SLIDER_HICUT);
	bool needClean = false;
	if (!map) {
		map = new haptics_map({0}); 
		map->hicut = 255;
		needClean = true;
	}
	RedrawButton(hDlg, IDC_BUTTON_LPC, map->colorfrom);
	RedrawButton(hDlg, IDC_BUTTON_HPC, map->colorto);
	// load cuts...
	SendMessage(hLowSlider, TBM_SETPOS, true, map->lowcut);
	SendMessage(hHiSlider, TBM_SETPOS, true, map->hicut);
	SetSlider(sTip1, map->lowcut);
	SetSlider(sTip2, map->hicut);
	CheckDlgButton(hDlg, IDC_GAUGE, map->flags ? BST_CHECKED : BST_UNCHECKED);
	if (needClean) delete map;
}

BOOL CALLBACK TabHapticsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND freq_list = GetDlgItem(hDlg, IDC_FREQ);
	HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS);
	HWND hLowSlider = GetDlgItem(hDlg, IDC_SLIDER_LOWCUT);
	HWND hHiSlider = GetDlgItem(hDlg, IDC_SLIDER_HICUT);
	
	
	haptics_map *map = FindHapMapping(eItem);

	switch (message) {
	case WM_INITDIALOG:
	{
		if (UpdateLightList(light_list, fxhl, 3) < 0) {
			// no lights, switch to setup
			SwitchTab(TAB_DEVICES);
			return false;
		}

		double coeff = 22030 / log(21);
		int prevfreq = 20;
		for (int i = 1; i < 21; i++) {
			int frq = 22050 - (int) round((log(21 - i) * coeff));
			string frqname = to_string(prevfreq) + "-" + to_string(frq) + "Hz";
			prevfreq = frq;
			ListBox_AddString(freq_list, frqname.c_str());
		}

		SendMessage(hLowSlider, TBM_SETRANGE, true, MAKELPARAM(0, 255));
		SendMessage(hHiSlider, TBM_SETRANGE, true, MAKELPARAM(0, 255));

		SendMessage(hLowSlider, TBM_SETTICFREQ, 16, 0);
		SendMessage(hHiSlider, TBM_SETTICFREQ, 16, 0);

		sTip1 = CreateToolTip(hLowSlider, sTip1);
		sTip2 = CreateToolTip(hHiSlider, sTip2);

		CheckDlgButton(hDlg, IDC_RADIO_INPUT, conf->hap_conf->inpType ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_RADIO_OUTPUT, conf->hap_conf->inpType ? BST_UNCHECKED : BST_CHECKED);

		CheckDlgButton(hDlg, IDC_SHOWAXIS, conf->hap_conf->showAxis ? BST_CHECKED : BST_UNCHECKED);

		conf->hap_conf->dlg = hDlg;

		if (eItem >= 0) {
			SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_LIGHTS, LBN_SELCHANGE), (LPARAM)light_list);
		}
	}
	break;

	case WM_COMMAND:
	{
		switch (LOWORD(wParam)) {
		case IDC_RADIO_OUTPUT:
			conf->hap_conf->inpType = 0;
			CheckDlgButton(hDlg, IDC_RADIO_INPUT, conf->hap_conf->inpType ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hDlg, IDC_RADIO_OUTPUT, conf->hap_conf->inpType ? BST_UNCHECKED : BST_CHECKED);
			if (eve->audio)
				eve->audio->RestartDevice(conf->hap_conf->inpType);
		break;
		case IDC_RADIO_INPUT:
			conf->hap_conf->inpType = 1;
			CheckDlgButton(hDlg, IDC_RADIO_INPUT, conf->hap_conf->inpType ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hDlg, IDC_RADIO_OUTPUT, conf->hap_conf->inpType ? BST_UNCHECKED : BST_CHECKED);
		if (eve->audio)
			eve->audio->RestartDevice(conf->hap_conf->inpType);
		break;
		case IDC_LIGHTS: // should reload mappings
		switch (HIWORD(wParam)) {
		case LBN_SELCHANGE:
		{
			// check in config - do we have mappings?
			eItem = (int) ListBox_GetItemData(light_list, ListBox_GetCurSel(light_list));
			map = FindHapMapping(eItem);
			EnableWindow(freq_list, TRUE);
			ListBox_SetSel(freq_list, FALSE, -1);
			if (map) {
				for (int j = 0; j < map->map.size(); j++) {
					ListBox_SetSel(freq_list, TRUE, map->map[j]);
				}
			}
			SetMappingData(hDlg, map);
		}
		break;
		}
		break;
		case IDC_FREQ:
		{ // should update mappings list
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE:
			{
				if (!map) {
					map = new haptics_map();
					if (eItem > 0xffff) {
						// group
						map->lightid = eItem;
					} else {
						// light
						AlienFX_SDK::mapping* lgh = fxhl->afx_dev.GetMappings()->at(eItem);
						map->devid = lgh->devid;
						map->lightid = lgh->lightid;
					}
					conf->hap_conf->haptics.push_back(*map);
					map = &conf->hap_conf->haptics.back();
				}

				int fid = ListBox_GetCurSel(freq_list);
				auto Iter = map->map.begin();
				for (; Iter != map->map.end(); Iter++)
					if (*Iter == fid)
						break;
				if (Iter == map->map.end())
					// new mapping, add and select
					map->map.push_back(fid);
				else
					map->map.erase(Iter);
				// Erase mapping if no freqs!
				if (!map->map.size()) {
					RemoveHapMapping(map);
					SetMappingData(hDlg, NULL);
				}
			} break;
			}
		} break;
		case IDC_BUTTON_LPC:
		switch (HIWORD(wParam)) {
		case BN_CLICKED:
		{
			if (map) {
				SetColor(hDlg, IDC_BUTTON_LPC, &map->colorfrom);
			}
		} break;
		} break;
		case IDC_BUTTON_HPC:
		switch (HIWORD(wParam)) {
		case BN_CLICKED:
		{
			if (map) {
				SetColor(hDlg, IDC_BUTTON_HPC, &map->colorto);
			}
		} break;
		} break;
		case IDC_GAUGE:
		{
			if (map)
				map->flags = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED;
		} break;
		case IDC_BUTTON_REMOVE:
		{
			switch (HIWORD(wParam)) {
			case BN_CLICKED:
			{
				if (map) {
					RemoveHapMapping(map);
					SetMappingData(hDlg, NULL);
					ListBox_SetSel(freq_list, FALSE, -1);
				}
			} break;
			}
		} break;
		case IDC_SHOWAXIS:
		{
			conf->hap_conf->showAxis = IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED;
			SendMessage(hDlg, WM_PAINT, 0, 0);
		} break;
		}
	} break;
	case WM_HSCROLL:
	{
		switch (LOWORD(wParam)) {
		case TB_THUMBTRACK: case TB_ENDTRACK:
		{
			if (map) {
				if ((HWND) lParam == hLowSlider) {
					map->lowcut = (UCHAR) SendMessage(hLowSlider, TBM_GETPOS, 0, 0);
					SetSlider(sTip1, map->lowcut);
				}
				if ((HWND) lParam == hHiSlider) {
					map->hicut = (UCHAR) SendMessage(hHiSlider, TBM_GETPOS, 0, 0);
					SetSlider(sTip2, map->hicut);
				}
			}
		} break;
		}
	} break;
	case WM_DRAWITEM:
		switch (((DRAWITEMSTRUCT *) lParam)->CtlID) {
		case IDC_BUTTON_LPC: case IDC_BUTTON_HPC:
		{
			AlienFX_SDK::Colorcode c{0};
			if (map)
				if (((DRAWITEMSTRUCT *) lParam)->CtlID == IDC_BUTTON_LPC)
					c = map->colorfrom;
				else
					c = map->colorto;
			RedrawButton(hDlg, ((DRAWITEMSTRUCT *) lParam)->CtlID, c);
			return true;
		}
		case IDC_LEVELS:
			DrawFreq(hDlg, eve->audio ? eve->audio->freqs : NULL);
			return true;
		}
		return false;
	break;
	case WM_CLOSE: 
	case WM_DESTROY:
		conf->hap_conf->dlg = NULL;
	break;
	default: return false;
	}

	return TRUE;
}