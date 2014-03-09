#include <windows.h>
#include <string>
#include <iostream>
#include <SetupApi.h>
#pragma comment(lib, "setupapi.lib")


const GUID GUID_CLASS_MONITOR = { 0x4d36e96e, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 };

// based off of http://ofekshilon.com/2011/11/13/reading-monitor-physical-dimensions-or-getting-the-edid-the-right-way/


bool GetMonitorSizeFromEDID(const HKEY hDevRegKey, short& WidthMm, short& HeightMm) {
	unsigned long dwType = 128;
	unsigned long ActualValueNameLength = 128;
	wchar_t valueName[128];

	unsigned char EDIDdata[1024];
	unsigned long edidsize = sizeof(EDIDdata);

	for (long i = 0, retValue = ERROR_SUCCESS; retValue != 259L; ++i) {
		retValue = RegEnumValueW(hDevRegKey, i, &valueName[0], &ActualValueNameLength, nullptr, &dwType, EDIDdata, &edidsize);

		if (retValue != 0L || 0 != std::wstring(valueName).compare(L"EDID"))
			continue;
		short Hres = ((EDIDdata[58] & 0xF0) << 4) + EDIDdata[56];
		short Vres = ((EDIDdata[61] & 0xF0) << 4) + EDIDdata[59];

		WidthMm = ((EDIDdata[68] & 0xF0) << 4) + EDIDdata[66];
		HeightMm = ((EDIDdata[68] & 0x0F) << 8) + EDIDdata[67];
		std::wcout << std::to_wstring(Hres) << L"x" << std::to_wstring(Vres) << L"\n";

		return true;
	}

	return false;
}

bool GetSizeForDevID(const std::wstring& TargetDevID, short& WidthMm, short& HeightMm) {
	HDEVINFO devInfo = SetupDiGetClassDevsExW(&GUID_CLASS_MONITOR, nullptr, nullptr, 2, nullptr, nullptr, nullptr);

	if (nullptr == devInfo) {
		return false;
	}

	bool bRes = false;

	for (unsigned long i = 0; 259L != GetLastError(); ++i) {
		SP_DEVINFO_DATA devInfoData;
		memset(&devInfoData, 0, sizeof(devInfoData));
		devInfoData.cbSize = sizeof(devInfoData);

		if (SetupDiEnumDeviceInfo(devInfo, i, &devInfoData)) {
			HKEY hDevRegKey = SetupDiOpenDevRegKey(devInfo, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
			if (!hDevRegKey || (hDevRegKey == INVALID_HANDLE_VALUE)) {
				continue;
			}
			bRes = GetMonitorSizeFromEDID(hDevRegKey, WidthMm, HeightMm);
			RegCloseKey(hDevRegKey);
		}
	}
	SetupDiDestroyDeviceInfoList(devInfo);
	return bRes;
}

int wmain(int argc, wchar_t* argv[]) {
	short WidthMm, HeightMm;

	DISPLAY_DEVICE dd;
	dd.cb = sizeof(dd);
	unsigned long dev = 0; // device index
	int id = 1; // monitor number, as used by Display Properties > Settings

	std::wstring DeviceID;
	bool bFoundDevice = false;
	while (EnumDisplayDevicesW(0, dev, &dd, 0) && !bFoundDevice) {
		DISPLAY_DEVICEW ddMon;
		ZeroMemory(&ddMon, sizeof(ddMon));
		ddMon.cb = sizeof(ddMon);
		unsigned long devMon = 0;

		while (EnumDisplayDevices(dd.DeviceName, devMon, &ddMon, 0) && !bFoundDevice) {
			if (ddMon.StateFlags & DISPLAY_DEVICE_ACTIVE && !(ddMon.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)) {
				DeviceID = (std::wstring(ddMon.DeviceID)).substr(8, DeviceID.find(L"\\", 9) - 8);
				bFoundDevice = GetSizeForDevID(DeviceID, WidthMm, HeightMm);
			}
			devMon++;
			ZeroMemory(&ddMon, sizeof(ddMon));
			ddMon.cb = sizeof(ddMon);
		}
		ZeroMemory(&dd, sizeof(dd));
		dd.cb = sizeof(dd);
		dev++;
	}
	// double diag = sqrt(WidthMm*WidthMm + HeightMm*HeightMm)
	std::wcout << std::to_wstring(WidthMm / 25.4) << L"x" << std::to_wstring(HeightMm / 25.4);
	std::wcin.get();
	return 0;
}