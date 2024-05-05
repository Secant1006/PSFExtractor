#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <windows.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <fdi.h>
#include "../resource.h"
#include "../libs/pugixml/src/pugixml.hpp"

#define MAX_PATH_W 32767
#define ProgressBarWidth 58

using namespace std;

const char PathSeparator = '\\';
const char WrongPathSeparator = '/';

// Strings
const char* ProgramTitle = "PSFExtractor v3.07 (Sep 7 2022) by th1r5bvn23\nhttps://www.betaworld.cn/\n\n";
const char* HelpInformation = "Usage:\n    PSFExtractor.exe <CAB file>\n    PSFExtractor.exe -v[N] <PSF file> <description file> <destination>\n\n    <CAB file>          Auto detect CAB file and corresponding PSF file which\n                        are in the same location with the same name.\n    -v[N]               Specify PSFX version. N = 1 | 2. PSFX v1 is for Windows\n                        2000 to Server 2003, while PSFX v2 is for Windows Vista\n                        and above.\n    <PSF file>          Path to PSF payload file.\n    <description file>  Path to description file. For PSFX v1, the description\n                        file has an extension \".psm\". For PSFX v2, a standard\n                        XML document is used.\n    <destination>       Path to output folder. If the folder doesn\'t exist, it\n                        will be created automatically.\n";

// Global settings
int PSFXVersion;
char* DescriptionFileName;
char* PayloadFileName;
char* TargetDirectoryName;
bool MSPatchALoadFlag = false;
bool MSDeltaLoadFlag = false;

// Utility functions
bool FileExists(const WCHAR* FileName) {
	return (GetFileAttributesW(FileName) != INVALID_FILE_ATTRIBUTES);
}

int CurrentPosition, CurrentProgress;
HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo = { 0 };

void CreateProgressBar() {
	CurrentPosition = 0;
	cout << '[';
	for (int i = 0; i < ProgressBarWidth; i++) {
		cout << ' ';
	}
	cout << "] 0%";
	cout.flush();
	GetConsoleScreenBufferInfo(hConsoleOutput, &ConsoleScreenBufferInfo);
}

void UpdateProgressBar(float progress) {
	COORD dwCursorPosition = ConsoleScreenBufferInfo.dwCursorPosition;
	bool FlushFlag = false;
	int pos = (int)(progress * (float)ProgressBarWidth);
	if (CurrentPosition != pos) {
		dwCursorPosition.X = CurrentPosition + 1;
		SetConsoleCursorPosition(hConsoleOutput, dwCursorPosition);
		for (int i = CurrentPosition; i < ProgressBarWidth; i++) {
			if (i < pos) {
				cout << '=';
			}
			else if (i == pos) {
				cout << '>';
				break;
			}
		}
		CurrentPosition = pos;
		FlushFlag = true;
	}
	if (CurrentProgress != (int)(progress * 100)) {
		dwCursorPosition.X = ProgressBarWidth + 3;
		SetConsoleCursorPosition(hConsoleOutput, dwCursorPosition);
		CurrentProgress = (int)(progress * 100);
		cout << CurrentProgress << '%';
		FlushFlag = true;
	}
	if (FlushFlag) {
		cout.flush();
		FlushFlag = false;
	}
}

void FinishProgressBar() {
	CurrentPosition = 0;
	cout << endl;
}

// Cabinet API functions
FNALLOC(FDIAlloc) {
	return HeapAlloc(GetProcessHeap(), 0, cb);
}

FNFREE(FDIFree) {
	HeapFree(GetProcessHeap(), 0, pv);
}

void CreateDirectoryRecursive(const WCHAR* name) {
	WCHAR* path, * p;
	path = (WCHAR*)FDIAlloc((lstrlenW(name) + 1) * sizeof(WCHAR));
	lstrcpyW(path, name);
	p = wcschr(path, '\\');
	while (p != NULL) {
		*p = 0;
		CreateDirectoryW(path, NULL);
		*p = '\\';
		p = wcschr(p + 1, '\\');
	}
	FDIFree(path);
}

static WCHAR* strdupAtoW(UINT cp, const char* str) {
	WCHAR* res = NULL;
	if (str) {
		DWORD len = MultiByteToWideChar(cp, 0, str, -1, NULL, 0) + 2;
		if ((res = (WCHAR*)FDIAlloc(sizeof(WCHAR) * len))) {
			MultiByteToWideChar(cp, 0, str, -1, res, len);
		}
	}
	return res;
}

static char* strdupWtoA(UINT cp, const WCHAR* str) {
	char* res = NULL;
	if (str) {
		DWORD len = WideCharToMultiByte(cp, 0, str, -1, NULL, 0, NULL, NULL) + 2;
		if ((res = (char*)FDIAlloc(sizeof(char) * len))) {
			WideCharToMultiByte(cp, 0, str, -1, res, len, NULL, NULL);
		}
	}
	return res;
}

FNOPEN(FDIOpen) {
	HANDLE hf = NULL;
	DWORD dwDesiredAccess = 0;
	DWORD dwCreationDisposition = 0;
	UNREFERENCED_PARAMETER(pmode);
	if (oflag & _O_RDWR) {
		dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
	}
	else if (oflag & _O_WRONLY) {
		dwDesiredAccess = GENERIC_WRITE;
	}
	else {
		dwDesiredAccess = GENERIC_READ;
	}
	if (oflag & _O_CREAT) {
		dwCreationDisposition = CREATE_ALWAYS;
	}
	else {
		dwCreationDisposition = OPEN_EXISTING;
	}
	WCHAR* pszFileW = strdupAtoW(CP_ACP, pszFile);
	hf = CreateFileW(pszFileW, dwDesiredAccess, FILE_SHARE_READ, NULL, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL);
	FDIFree(pszFileW);
	return (INT_PTR)hf;
}

FNREAD(FDIRead) {
	DWORD num_read;
	if (!ReadFile((HANDLE)hf, pv, cb, &num_read, NULL)) {
		return -1;
	}
	return num_read;
}

FNWRITE(FDIWrite) {
	DWORD written;
	if (!WriteFile((HANDLE)hf, pv, cb, &written, NULL)) {
		return -1;
	}
	return written;
}

FNCLOSE(FDIClose) {
	if (!CloseHandle((HANDLE)hf)) {
		return -1;
	}
	return 0;
}

FNSEEK(FDISeek) {
	DWORD res;
	res = SetFilePointer((HANDLE)hf, dist, NULL, seektype);
	if (res == INVALID_SET_FILE_POINTER && GetLastError()) {
		return -1;
	}
	return res;
}

int CurrentFiles, TotalFiles;

FNFDINOTIFY(FDINotify) {
	WCHAR* nameW = NULL, * file = NULL, * TargetDirectoryNameW = NULL, * TargetFilePath = NULL;
	FILETIME FileTime = { 0 };
	HANDLE hf = NULL;
	switch (fdint) {
	case fdintCABINET_INFO:
		return 0;
	case fdintCOPY_FILE:
		nameW = strdupAtoW((pfdin->attribs & _A_NAME_IS_UTF) ? CP_UTF8 : CP_ACP, pfdin->psz1);
		file = nameW;
		while (*file == '\\') {
			file++;
		}
		TargetDirectoryNameW = strdupAtoW(CP_ACP, TargetDirectoryName);
		TargetFilePath = (WCHAR*)FDIAlloc((lstrlenW(TargetDirectoryNameW) + lstrlenW(file) + 1) * sizeof(WCHAR));
		wcscpy_s(TargetFilePath, (lstrlenW(TargetDirectoryNameW) + lstrlenW(file) + 1), TargetDirectoryNameW);
		wcscat_s(TargetFilePath, (lstrlenW(TargetDirectoryNameW) + lstrlenW(file) + 1), file);
		CreateDirectoryRecursive(TargetFilePath);
		hf = CreateFileW(TargetFilePath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		FDIFree(nameW);
		FDIFree(TargetDirectoryNameW);
		FDIFree(TargetFilePath);
		return (INT_PTR)hf;
	case fdintCLOSE_FILE_INFO:
		DosDateTimeToFileTime(pfdin->date, pfdin->time, &FileTime);
		SetFileTime((HANDLE)(pfdin->hf), NULL, NULL, &FileTime);
		FDIClose(pfdin->hf);
		CurrentFiles++;
		UpdateProgressBar((float)CurrentFiles / (float)TotalFiles);
		return TRUE;
	case fdintPARTIAL_FILE:
		return -1;
	case fdintENUMERATE:
		return 0;
	case fdintNEXT_CABINET:
		return -1;
	default:
		return 0;
	}
}

bool ExtractCABFile(char* CABFilePart, char* CABPathPart) {
	// Create FDI context
	ERF* FDIErf = (ERF*)FDIAlloc(sizeof(ERF));
	HFDI FDIContext = FDICreate(FDIAlloc, FDIFree, FDIOpen, FDIRead, FDIWrite, FDIClose, FDISeek, cpuUNKNOWN, FDIErf);
	if (FDIContext == NULL) {
		return false;
	}

	// Get file number of CAB file
	char* CABFileFullName = (char*)FDIAlloc(sizeof(char) * MAX_PATH_W);
	strcpy_s(CABFileFullName, MAX_PATH_W, CABPathPart);
	strcat_s(CABFileFullName, MAX_PATH_W, CABFilePart);
	WCHAR* CABFileFullNameW = strdupAtoW(CP_ACP, CABFileFullName);
	FDIFree(CABFileFullName);
	HANDLE hf = CreateFileW(CABFileFullNameW, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	FDIFree(CABFileFullNameW);
	FDICABINETINFO* CabinetInfo = (FDICABINETINFO*)FDIAlloc(sizeof(FDICABINETINFO));
	if (!FDIIsCabinet(FDIContext, (INT_PTR)hf, CabinetInfo)) {
		return false;
	}
	if (CabinetInfo->hasprev || CabinetInfo->hasnext) {
		return false;
	}
	TotalFiles = CabinetInfo->cFiles;
	cout << TotalFiles << " files..." << endl;
	FDIFree(CabinetInfo);
	CloseHandle(hf);

	// Extract file
	CurrentFiles = 0;
	CreateProgressBar();
	if (!FDICopy(FDIContext, CABFilePart, CABPathPart, 0, FDINotify, NULL, NULL)) {
		return false;
	}
	FinishProgressBar();

	// Destroy FDI context
	FDIDestroy(FDIContext);

	// Cleanup and return
	FDIFree(FDIErf);
	return true;
}

// Parsing data structures
enum sourceType { RAW, PA19, PA30 };

typedef struct {
	WCHAR* name;
	FILETIME time;
	sourceType type;
	ULARGE_INTEGER offset;
	DWORD length;
} DeltaFile;

vector<DeltaFile> DeltaFileList;

// Description file parsing functions
bool ParseDescriptionFileV1() {
	WCHAR* DescriptionFileNameW = strdupAtoW(CP_ACP, DescriptionFileName);
	HANDLE hf = CreateFileW(DescriptionFileNameW, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	FDIFree(DescriptionFileNameW);
	if (hf == INVALID_HANDLE_VALUE) {
		return false;
	}
	LARGE_INTEGER FileSize;
	GetFileSizeEx(hf, &FileSize);
	char* FileBuffer = (char*)FDIAlloc(FileSize.QuadPart);
	DWORD temp = 0;
	if (!ReadFile(hf, FileBuffer, FileSize.QuadPart, &temp, NULL)) {
		return false;
	}
	stringstream s(FileBuffer);
	char* LineBuffer = (char*)FDIAlloc(MAX_PATH + 3);
	char* FileNameBuffer = (char*)FDIAlloc(MAX_PATH + 3);
	bool isReadingAFile = false;
	WCHAR* FileName = NULL;
	sourceType type;
	while (true) {
		s.getline(LineBuffer, MAX_PATH + 3);
		if (s.fail()) {
			if (s.eof()) {
				break;
			}
			else {
				return false;
			}
		}
		if (LineBuffer[0] == '[') {
			FileName = strdupAtoW(CP_ACP, &LineBuffer[1]);
			FileName[lstrlenW(FileName) - 2] = '\0';
			isReadingAFile = true;
			continue;
		}
		if (isReadingAFile) {
			char* p, * q;
			if (p = strstr(LineBuffer, "p0=")) {
				type = PA19;
				MSPatchALoadFlag = true;
				p += 3;
			}
			else if (p = strstr(LineBuffer, "full=")) {
				type = RAW;
				p += 5;
			}
			else {
				return false;
			}
			unsigned long long offset = _strtoui64(p, &q, 16);
			q++;
			unsigned int length = strtoul(q, &p, 16);
			ULARGE_INTEGER time_ularge = { 0 };
			FILETIME time = { time_ularge.LowPart, time_ularge.HighPart };
			ULARGE_INTEGER offset_ularge = { 0 };
			offset_ularge.QuadPart = offset;
			DeltaFile file;
			if (FileName) {
				file.name = FileName;
				file.time = time;
				file.type = type;
				file.offset = offset_ularge;
				file.length = (DWORD)length;
			}
			else {
				return false;
			}
			DeltaFileList.push_back(file);
			isReadingAFile = false;
		}
	}
	FDIFree(FileNameBuffer);
	FDIFree(LineBuffer);
	FDIFree(FileBuffer);
	CloseHandle(hf);
	return true;
}

bool ParseDescriptionFileV2() {
	using namespace pugi;
	xml_document XMLDocument;
	if (!XMLDocument.load_file(DescriptionFileName)) {
		return false;
	}
	WCHAR* FileName = NULL;
	xml_node FilesNode = XMLDocument.child("Container").child("Files");
	xml_node_iterator file;
	for (file = FilesNode.begin(); file != FilesNode.end(); file++) {
		FileName = strdupAtoW(CP_ACP, file->attribute("name").value());
		ULARGE_INTEGER time_ularge = { 0 };
		time_ularge.QuadPart = file->attribute("time").as_ullong();
		FILETIME FileTime = { time_ularge.LowPart, time_ularge.HighPart };
		xml_node SourceNode = file->child("Delta").child("Source");
		const char* type = SourceNode.attribute("type").value();
		sourceType FileType = RAW;
		if (!_strcmpi(type, "RAW")) {
			FileType = RAW;
		}
		else if (!_strcmpi(type, "PA19")) {
			FileType = PA19;
			MSPatchALoadFlag = true;
		}
		else if (!_strcmpi(type, "PA30")) {
			FileType = PA30;
			MSDeltaLoadFlag = true;
		}
		ULARGE_INTEGER FileOffset = { 0 };
		FileOffset.QuadPart = SourceNode.attribute("offset").as_ullong();
		unsigned long FileLength = SourceNode.attribute("length").as_uint();
		DeltaFile file = { FileName, FileTime, FileType, FileOffset, FileLength };
		DeltaFileList.push_back(file);
	}
	return true;
}

typedef BOOL(CALLBACK PATCH_PROGRESS_CALLBACK)(PVOID CallbackContext, ULONG CurrentPosition, ULONG MaximumPosition);
typedef PATCH_PROGRESS_CALLBACK* PPATCH_PROGRESS_CALLBACK;
typedef BOOL(WINAPI *ApplyPatchToFileByBuffersFunc)(PBYTE PatchFileMapped, ULONG PatchFileSize, PBYTE OldFileMapped, ULONG OldFileSize, PBYTE* NewFileBuffer, ULONG NewFileBufferSize, ULONG* NewFileActualSize, FILETIME* NewFileTime, ULONG ApplyOptionFlags, PPATCH_PROGRESS_CALLBACK ProgressCallback, PVOID CallbackContext);

typedef struct _DELTA_INPUT {
	union {
		LPCVOID lpcStart;
		LPVOID lpStart;
	};
	SIZE_T uSize;
	BOOL Editable;
} DELTA_INPUT;
typedef struct _DELTA_OUTPUT {
	LPVOID lpStart;
	SIZE_T uSize;
} DELTA_OUTPUT;
typedef DELTA_OUTPUT* LPDELTA_OUTPUT;
typedef __int64 DELTA_FLAG_TYPE;
typedef BOOL(WINAPI *ApplyDeltaBFunc)(DELTA_FLAG_TYPE ApplyFlags, DELTA_INPUT Source, DELTA_INPUT Delta, LPDELTA_OUTPUT lpTarget);
typedef BOOL(WINAPI *DeltaFreeFunc)(LPVOID lpMemory);

bool LoadMSPatchA(ApplyPatchToFileByBuffersFunc* ApplyPatchToFileByBuffersAddr) {
	HMODULE MSPatchA = NULL;
	MSPatchA = LoadLibraryW(L"mspatcha.dll");
	if (MSPatchA) {
		*ApplyPatchToFileByBuffersAddr = (ApplyPatchToFileByBuffersFunc)GetProcAddress(MSPatchA, "ApplyPatchToFileByBuffers");
		if (*ApplyPatchToFileByBuffersAddr) {
			return true;
		}
	}
	HRSRC ResourceInformation = NULL;
	ResourceInformation = FindResourceW(NULL, MAKEINTRESOURCE(IDR_DLL1), RT_RCDATA);
	if (!ResourceInformation) {
		return false;
	}
	HGLOBAL hResourcesData = LoadResource(NULL, ResourceInformation);
	if (!hResourcesData) {
		return false;
	}
	LPVOID ResourcePointer = NULL;
	ResourcePointer = LockResource(hResourcesData);
	if (!ResourcePointer) {
		return false;
	}
	WCHAR MSPatchA_Temp[MAX_PATH + 2];
	if (!GetTempPathW(MAX_PATH + 2, MSPatchA_Temp)) {
		return false;
	}
	lstrcatW(MSPatchA_Temp, L"mspatcha.dll");
	HANDLE hf = CreateFileW(MSPatchA_Temp, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hf == INVALID_HANDLE_VALUE) {
		return false;
	}
	DWORD temp;
	if (!WriteFile(hf, ResourcePointer, SizeofResource(NULL, ResourceInformation), &temp, NULL)) {
		return false;
	}
	else {
		if (!CloseHandle(hf)) {
			return false;
		}
	}
	MSPatchA = LoadLibraryW(MSPatchA_Temp);
	if (MSPatchA) {
		*ApplyPatchToFileByBuffersAddr = (ApplyPatchToFileByBuffersFunc)GetProcAddress(MSPatchA, "ApplyPatchToFileByBuffers");
		if (*ApplyPatchToFileByBuffersAddr) {
			return true;
		}
	}
	return false;
}

bool LoadMSDelta(ApplyDeltaBFunc* ApplyDeltaBAddr, DeltaFreeFunc* DeltaFreeAddr) {
	HMODULE MSDelta = NULL;
	MSDelta = LoadLibraryW(L"msdelta.dll");
	if (MSDelta) {
		*ApplyDeltaBAddr = (ApplyDeltaBFunc)GetProcAddress(MSDelta, "ApplyDeltaB");
		*DeltaFreeAddr = (DeltaFreeFunc)GetProcAddress(MSDelta, "DeltaFree");
		if (*ApplyDeltaBAddr && *DeltaFreeAddr) {
			return true;
		}
	}
	HRSRC ResourceInformation = NULL;
	ResourceInformation = FindResourceW(NULL, MAKEINTRESOURCE(IDR_DLL2), RT_RCDATA);
	if (!ResourceInformation) {
		return false;
	}
	HGLOBAL hResourcesData = LoadResource(NULL, ResourceInformation);
	if (!hResourcesData) {
		return false;
	}
	LPVOID ResourcePointer = NULL;
	ResourcePointer = LockResource(hResourcesData);
	if (!ResourcePointer) {
		return false;
	}
	WCHAR MSDelta_Temp[MAX_PATH + 2];
	if (!GetTempPathW(MAX_PATH + 2, MSDelta_Temp)) {
		return false;
	}
	lstrcatW(MSDelta_Temp, L"msdelta.dll");
	HANDLE hf = CreateFileW(MSDelta_Temp, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hf == INVALID_HANDLE_VALUE) {
		return false;
	}
	DWORD temp;
	if (!WriteFile(hf, ResourcePointer, SizeofResource(NULL, ResourceInformation), &temp, NULL)) {
		return false;
	}
	else {
		if (!CloseHandle(hf)) {
			return false;
		}
	}
	MSDelta = LoadLibraryW(MSDelta_Temp);
	if (MSDelta) {
		*ApplyDeltaBAddr = (ApplyDeltaBFunc)GetProcAddress(MSDelta, "ApplyDeltaB");
		*DeltaFreeAddr = (DeltaFreeFunc)GetProcAddress(MSDelta, "DeltaFree");
		if (*ApplyDeltaBAddr && *DeltaFreeAddr) {
			return true;
		}
	}
	return false;
}

// Write output
bool WriteOutput() {
	WCHAR* PayloadFileNameW = strdupAtoW(CP_ACP, PayloadFileName);
	HANDLE input = CreateFileW(PayloadFileNameW, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	FDIFree(PayloadFileNameW);
	if (input == INVALID_HANDLE_VALUE) {
		return false;
	}
	WCHAR* file = NULL, * TargetDirectoryNameW = NULL, * TargetFilePath = NULL, * TargetFilePathWithPrefix = NULL;
	HANDLE output = NULL;
	void* buffer = NULL;
	DWORD temp;
	ApplyPatchToFileByBuffersFunc ApplyPatchToFileByBuffers = NULL;
	if (MSPatchALoadFlag) {
		if (!LoadMSPatchA(&ApplyPatchToFileByBuffers)) {
			return false;
		}
	}
	ApplyDeltaBFunc ApplyDeltaB = NULL;
	DeltaFreeFunc DeltaFree = NULL;
	if (MSDeltaLoadFlag) {
		if (!LoadMSDelta(&ApplyDeltaB, &DeltaFree)) {
			return false;
		}
	}
	CurrentFiles = 0;
	CreateProgressBar();
	for (vector<DeltaFile>::iterator item = DeltaFileList.begin(); item != DeltaFileList.end(); item++) {
		buffer = FDIAlloc(item->length);
		LARGE_INTEGER _offset = { 0 };
		_offset.QuadPart = item->offset.QuadPart;
		if (!SetFilePointerEx(input, _offset, NULL, FILE_BEGIN)) {
			return false;
		}
		if (!ReadFile(input, buffer, item->length, &temp, NULL)) {
			return false;
		}
		file = item->name;
		while (*file == '\\') {
			file++;
		}
		TargetDirectoryNameW = strdupAtoW(CP_ACP, TargetDirectoryName);
		TargetFilePath = (WCHAR*)FDIAlloc((lstrlenW(TargetDirectoryNameW) + lstrlenW(file) + 1) * sizeof(WCHAR));
		wcscpy_s(TargetFilePath, (lstrlenW(TargetDirectoryNameW) + lstrlenW(file) + 1), TargetDirectoryNameW);
		wcscat_s(TargetFilePath, (lstrlenW(TargetDirectoryNameW) + lstrlenW(file) + 1), file);
		CreateDirectoryRecursive(TargetFilePath);
		output = CreateFileW(TargetFilePath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (output == INVALID_HANDLE_VALUE || output == NULL) {
			return false;
		}
		FDIFree(TargetDirectoryNameW);
		FDIFree(TargetFilePath);
		if (item->type == PA19) {
			PBYTE PA19DeltaOutput = NULL;
			ULONG PA19TargetSize;
			if (!ApplyPatchToFileByBuffers((PBYTE)buffer, item->length, NULL, 0, &PA19DeltaOutput, 0, &PA19TargetSize, &item->time, NULL, NULL, NULL)) {
				return false;
			}
			if (!WriteFile(output, PA19DeltaOutput, PA19TargetSize, &temp, NULL)) {
				return false;
			}
			VirtualFree(PA19DeltaOutput, 0, MEM_RELEASE);
		}
		else if (item->type == PA30) {
			DELTA_INPUT PA30DeltaInput = { buffer, item->length, false };
			DELTA_INPUT PA30NullDeltaInput = { NULL, 0, false };
			DELTA_OUTPUT PA30DeltaOutput;
			if (!ApplyDeltaB(0, PA30NullDeltaInput, PA30DeltaInput, &PA30DeltaOutput)) {
				return false;
			}
			if (!WriteFile(output, PA30DeltaOutput.lpStart, PA30DeltaOutput.uSize, &temp, NULL)) {
				return false;
			}
			DeltaFree(PA30DeltaOutput.lpStart);
		}
		else {
			if (!WriteFile(output, buffer, item->length, &temp, NULL)) {
				return false;
			}
		}
		FDIFree(buffer);
		SetFileTime(output, NULL, NULL, &item->time);
		CloseHandle(output);
		CurrentFiles++;
		UpdateProgressBar((float)CurrentFiles / (float)TotalFiles);
	}
	CloseHandle(input);
	FinishProgressBar();
	return true;
}

// Main entry
int wmain(int argc, WCHAR* argv[]) {
	cout << ProgramTitle;
	const WCHAR PSFXv1[] = L"-v1";
	const WCHAR PSFXv2[] = L"-v2";
	const WCHAR CABExtension[] = L".cab";
	const WCHAR DOSDevicePrefix[] = L"\\\\?\\";

	// Filter wrong path separator
	for (int i = 1; i < argc; i++) {
		for (WCHAR* p = argv[i]; *p; p++) {
			if (*p == '/') {
				*p = '\\';
			}
		}
	}

	// Parse arguments
	// Automatic mode
	if (argc == 2) {
		PSFXVersion = 2;
		WCHAR* CABFileName = argv[1];

		// Check CAB extension
		if (lstrlenW(CABFileName) >= 4) {
			if (lstrcmpiW(&CABFileName[lstrlenW(CABFileName) - 4], CABExtension)) {
				cout << "Error: Invalid CAB file name." << endl;
			}
		}
		else {
			cout << "Error: Invalid CAB file name." << endl;
		}

		// Check if CAB exists
		if (!FileExists(CABFileName)) {
			cout << "Error: CAB file does not exist." << endl;
		}

		// Get full path and split it into directory and file name
		WCHAR* CABFileFullName = (WCHAR*)FDIAlloc(sizeof(WCHAR) * MAX_PATH_W);
		WCHAR* CABFilePartPointer;
		char* CABFilePart;
		char* CABPathPart;
		if (!GetFullPathNameW(CABFileName, MAX_PATH_W, CABFileFullName, &CABFilePartPointer) || !CABFilePartPointer) {
			cout << "Error: Can't get full path of the CAB file." << endl;
			return 1;
		}

		// Generate payload file name
		PayloadFileName = strdupWtoA(CP_ACP, CABFileName);
		PayloadFileName[strlen(PayloadFileName) - 3] = '\0';
		strcat_s(PayloadFileName, strlen(PayloadFileName) + 4, "psf");

		// Generate target directory name
		WCHAR* TargetDirectoryNameW = (WCHAR*)FDIAlloc(sizeof(WCHAR) * MAX_PATH_W);
		wcscpy_s(TargetDirectoryNameW, MAX_PATH_W, DOSDevicePrefix);
		wcscat_s(TargetDirectoryNameW, MAX_PATH_W, CABFileFullName);
		size_t len = lstrlenW(TargetDirectoryNameW);
		TargetDirectoryNameW[len - 4] = '\\';
		TargetDirectoryNameW[len - 3] = '\0';
		TargetDirectoryName = strdupWtoA(CP_ACP, TargetDirectoryNameW);
		WCHAR* TargetDirectoryNameBuffer = (WCHAR*)FDIAlloc(sizeof(WCHAR) * MAX_PATH_W);
		GetFullPathNameW(TargetDirectoryNameW, MAX_PATH_W, TargetDirectoryNameBuffer, (LPWSTR*)NULL);
		TargetDirectoryName = strdupWtoA(CP_ACP, TargetDirectoryNameBuffer);
		if (TargetDirectoryName[strlen(TargetDirectoryName) - 1] != PathSeparator) {
			size_t len = strlen(TargetDirectoryName);
			TargetDirectoryName[len] = PathSeparator;
			TargetDirectoryName[len + 1] = '\0';
		}
		CreateDirectoryW(TargetDirectoryNameW, NULL);
		FDIFree(TargetDirectoryNameW);

		// Generate description file name
		WIN32_FIND_DATAA findData;
		HANDLE hFind = FindFirstFileA((std::string(TargetDirectoryName) + "*.psf.cix.xml").c_str(), &findData);
		std::string defaultResult = "express.psf.cix.xml";
		std::string result = "";
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				std::string fileName = findData.cFileName;
				if (result.empty()) {
					result = fileName;
				}
				if (fileName == defaultResult) {
					result = defaultResult;
				}
			} while (FindNextFileA(hFind, &findData) != 0);
			FindClose(hFind);
		}
		DescriptionFileName = (char*)FDIAlloc(sizeof(char) * strlen(TargetDirectoryName) + 2 + result.size());
		strcpy_s(DescriptionFileName, strlen(TargetDirectoryName) + 2 + result.size(), TargetDirectoryName);
		strcat_s(DescriptionFileName, strlen(TargetDirectoryName) + 2 + result.size() , result.c_str());

		// Execute CAB extracting function
		CABFilePart = strdupWtoA(CP_ACP, CABFilePartPointer);
		CABFilePartPointer[0] = '\0';
		CABPathPart = strdupWtoA(CP_ACP, CABFileFullName);
		FDIFree(CABFileFullName);
		cout << "Extracting: ";
		if (!ExtractCABFile(CABFilePart, CABPathPart)) {
			cout << "\nError: Error extracting CAB." << endl;
			return 1;
		}

		// Cleanup and return
		FDIFree(CABFilePart);
		FDIFree(CABPathPart);
	}

	// Manual mode
	else if (argc == 5) {
		if (!lstrcmpiW(argv[1], PSFXv1)) {
			PSFXVersion = 1;
		}
		else if (!lstrcmpiW(argv[1], PSFXv2)) {
			PSFXVersion = 2;
		}
		else {
			cout << "Error: Invalid argument \'" << argv[1] << "\'." << endl;
			cout << HelpInformation;
			return 1;
		}
		PayloadFileName = strdupWtoA(CP_ACP, argv[2]);
		DescriptionFileName = strdupWtoA(CP_ACP, argv[3]);
		WCHAR* TargetDirectoryNameW = (WCHAR*)FDIAlloc(sizeof(WCHAR) * MAX_PATH_W);
		wcscpy_s(TargetDirectoryNameW, MAX_PATH_W, DOSDevicePrefix);
		CreateDirectoryW(argv[4], NULL);
		int len = lstrlenW(TargetDirectoryNameW);
		GetFullPathNameW(argv[4], MAX_PATH_W - len, &TargetDirectoryNameW[len], NULL);
		TargetDirectoryName = strdupWtoA(CP_ACP, TargetDirectoryNameW);
		FDIFree(TargetDirectoryNameW);
		if (TargetDirectoryName[strlen(TargetDirectoryName) - 1] != PathSeparator) {
			size_t len = strlen(TargetDirectoryName);
			TargetDirectoryName[len] = PathSeparator;
			TargetDirectoryName[len + 1] = '\0';
		}
	}

	// No argument
	else if (argc == 1) {
		cout << HelpInformation;
		return 0;
	}

	// Wrong argument number
	else {
		cout << HelpInformation;
		return 1;
	}

	// Read description file
	cout << "Reading file info...";
	switch (PSFXVersion) {
	case 1:
		if (!ParseDescriptionFileV1()) {
			cout << "\nError: Error reading description file." << endl;
			return 1;
		}
		break;
	case 2:
		if (!ParseDescriptionFileV2()) {
			cout << "\nError: Error reading description file." << endl;
			return 1;
		}
		break;
	}
	cout << " OK." << endl;

	// Write output
	TotalFiles = DeltaFileList.size();
	if (TotalFiles) {
		cout << "Writing: " << TotalFiles << " files..." << endl;
		if (!WriteOutput()) {
			cout << "\nError: Error writing output files." << endl;
			return 1;
		}
	}

	// Cleanup and exit
	if (PayloadFileName) {
		FDIFree(PayloadFileName);
	}
	if (DescriptionFileName) {
		FDIFree(DescriptionFileName);
	}
	if (TargetDirectoryName) {
		FDIFree(TargetDirectoryName);
	}
	cout << "Finished." << endl;
	return 0;
}
