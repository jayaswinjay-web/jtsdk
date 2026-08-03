// JTS GO native installer (Windows 7+ compatible, no .NET / Node dependency).
// Self-contained: embeds payload.zip as a Win32 RCDATA resource and extracts it.
//
// GUI mode (default): a PropertySheet wizard with Welcome / License / Location /
// PATH / Ready / Progress / Done pages.
//
// Usage:
//   JTS-IDE-Setup                GUI wizard install
//   JTS-IDE-Setup --install      GUI wizard install (same as no argument)
//   JTS-IDE-Setup --silent       silent install (accept license, default dir, add PATH)
//   JTS-IDE-Setup --uninstall    remove the language and the IDE
//   JTS-IDE-Setup --uninstall --silent
//   JTS-IDE-Setup --version      print version
//   JTS-IDE-Setup --help         print help
#define _WIN32_WINNT 0x0601
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <prsht.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <objbase.h>
#include <shellapi.h>
#include <urlmon.h>

#include <cstdio>
#include <cstring>
#include <cwctype>
#include <string>
#include <vector>

#include "miniz.h"

#define IDR_PAYLOAD 101
#define IDR_LICENSE 102
#define IDI_APP 1

#define IDD_WELCOME 200
#define IDD_LICENSE 201
#define IDD_LOCATION 202
#define IDD_PATH 203
#define IDD_READY 204
#define IDD_PROGRESS 205
#define IDD_DONE 206

#define IDC_ACCEPT 1001
#define IDC_DIR 1002
#define IDC_BROWSE 1003
#define IDC_PATH_YES 1004
#define IDC_PATH_NO 1005
#define IDC_SUMMARY 1006
#define IDC_PROGRESS_BAR 1007
#define IDC_STATUS 1008
#define IDC_LAUNCH 1009
#define IDC_LICENSE_EDIT 1010

namespace {

const wchar_t* kAppName = L"JTS GO";
const wchar_t* kVersion = L"2.1.0";
const wchar_t* kCompany = L"JayTech Solutions";
const wchar_t* kRepoUrl = L"https://github.com/jayaswinjay-web/jtsdk";
const wchar_t* kUninstallKey =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\JTS GO";

const UINT kInstallDoneMsg = WM_APP + 1;
const int kPageWelcome = 0;
const int kPageLicense = 1;
const int kPageLocation = 2;
const int kPagePath = 3;
const int kPageReady = 4;
const int kPageProgress = 5;
const int kPageDone = 6;

std::wstring g_installDir;  // default filled in before use
bool g_pathChoice = true;   // add to PATH?

// ---- forward declarations ----
void RecursiveDelete(const std::wstring& dir);
void RecursiveDeleteExceptSelf(const std::wstring& dir);

std::wstring GetSpecialFolder(int csidl) {
    wchar_t buf[MAX_PATH] = {0};
    if (FAILED(SHGetFolderPathW(NULL, csidl, NULL, SHGFP_TYPE_CURRENT, buf)))
        return L"";
    return std::wstring(buf);
}

std::wstring GetLocalAppData() { return GetSpecialFolder(CSIDL_LOCAL_APPDATA); }
std::wstring GetProgramsMenu() { return GetSpecialFolder(CSIDL_PROGRAMS); }

std::wstring GetDefaultInstallDir() {
    return GetLocalAppData() + L"\\Programs\\JTS GO";
}

std::wstring GetInstallDir() {
    return g_installDir.empty() ? GetDefaultInstallDir() : g_installDir;
}

std::wstring GetBinDir() {
    std::wstring base = g_installDir.empty() ? GetDefaultInstallDir() : g_installDir;
    return base + L"\\bin";
}
std::wstring GetIdeDir() {
    std::wstring base = g_installDir.empty() ? GetDefaultInstallDir() : g_installDir;
    return base + L"\\ide";
}
std::wstring GetStartMenuFolder() { return GetProgramsMenu() + L"\\JTS GO"; }

std::wstring ReadInstallLocationFromRegistry() {
    HKEY key = NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kUninstallKey, 0, KEY_READ, &key) !=
        ERROR_SUCCESS)
        return L"";
    DWORD sz = 0, type = 0;
    RegQueryValueExW(key, L"InstallLocation", NULL, &type, NULL, &sz);
    std::wstring val;
    if (sz > 0) {
        val.resize(sz / sizeof(wchar_t));
        RegQueryValueExW(key, L"InstallLocation", NULL, &type, (LPBYTE)&val[0],
                         &sz);
        val.resize(sz / sizeof(wchar_t));
        if (!val.empty() && val.back() == L'\0') val.pop_back();
    }
    RegCloseKey(key);
    return val;
}

std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) return std::string();
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), NULL, 0,
                                NULL, NULL);
    std::string out(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &out[0], n, NULL,
                        NULL);
    return out;
}

std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return std::wstring();
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    std::wstring out(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &out[0], n);
    return out;
}

bool EndsWithSlash(const std::wstring& s) {
    return !s.empty() && (s.back() == L'\\' || s.back() == L'/');
}

void CreateParentDirs(const std::wstring& fullPath) {
    std::wstring cur;
    for (size_t i = 0; i < fullPath.size(); ++i) {
        if (fullPath[i] == L'\\' || fullPath[i] == L'/') {
            if (!cur.empty()) CreateDirectoryW(cur.c_str(), NULL);
        }
        cur += fullPath[i];
    }
}

std::vector<BYTE> LoadPayload() {
    HRSRC h = FindResourceW(NULL, MAKEINTRESOURCE(IDR_PAYLOAD), RT_RCDATA);
    if (h == NULL) return std::vector<BYTE>();
    HGLOBAL g = LoadResource(NULL, h);
    if (g == NULL) return std::vector<BYTE>();
    void* p = LockResource(g);
    DWORD sz = SizeofResource(NULL, h);
    if (p == NULL || sz == 0) return std::vector<BYTE>();
    return std::vector<BYTE>((BYTE*)p, (BYTE*)p + sz);
}

std::string LoadLicenseText() {
    HRSRC h = FindResourceW(NULL, MAKEINTRESOURCE(IDR_LICENSE), RT_RCDATA);
    if (h == NULL) return std::string();
    HGLOBAL g = LoadResource(NULL, h);
    if (g == NULL) return std::string();
    void* p = LockResource(g);
    DWORD sz = SizeofResource(NULL, h);
    if (p == NULL || sz == 0) return std::string();
    return std::string((const char*)p, (size_t)sz);
}

// Print a wide string to stdout. Works both when attached to a real console
// and when stdout is a pipe (writes UTF-8 bytes to the inherited handle).
void Output(const std::wstring& s) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (hOut != INVALID_HANDLE_VALUE && hOut != NULL &&
        GetConsoleMode(hOut, &mode)) {
        WriteConsoleW(hOut, s.c_str(), (DWORD)s.size(), NULL, NULL);
        return;
    }
    if (hOut != INVALID_HANDLE_VALUE && hOut != NULL) {
        std::string utf8 = WideToUtf8(s);
        WriteFile(hOut, utf8.data(), (DWORD)utf8.size(), &mode, NULL);
        return;
    }
    // No usable std handle: attach to a console and open CONOUT$ directly.
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        HANDLE c = CreateFileW(L"CONOUT$", GENERIC_WRITE, FILE_SHARE_WRITE,
                               NULL, OPEN_EXISTING, 0, NULL);
        if (c != INVALID_HANDLE_VALUE) {
            WriteConsoleW(c, s.c_str(), (DWORD)s.size(), NULL, NULL);
            CloseHandle(c);
        }
    }
}

void OutputLine(const std::wstring& s) { Output(s + L"\n"); }

struct WriteCtx {
    HANDLE file;
};

size_t ExtractWrite(void* opaque, mz_uint64 /*file_ofs*/, const void* buf,
                    size_t n) {
    WriteCtx* ctx = static_cast<WriteCtx*>(opaque);
    DWORD written = 0;
    if (n == 0) return 0;
    if (!WriteFile(ctx->file, buf, (DWORD)n, &written, NULL)) return 0;
    return written;
}

typedef void (*ProgressFn)(void* ctx, int done, int total);

// Returns number of files extracted, or -1 on failure.
int ExtractPayload(const std::vector<BYTE>& data, const std::wstring& destRoot,
                   ProgressFn prog = NULL, void* progCtx = NULL) {
    mz_zip_archive zip;
    ZeroMemory(&zip, sizeof(zip));
    if (!mz_zip_reader_init_mem(&zip, data.data(), data.size(), 0)) return -1;

    mz_uint n = mz_zip_reader_get_num_files(&zip);
    int count = 0;
    for (mz_uint i = 0; i < n; ++i) {
        mz_zip_archive_file_stat st;
        if (!mz_zip_reader_file_stat(&zip, i, &st)) continue;
        if (st.m_is_directory) continue;

        std::wstring wname = Utf8ToWide(st.m_filename);
        if (wname.empty() || EndsWithSlash(wname)) continue;
        for (auto& c : wname)
            if (c == L'/') c = L'\\';

        std::wstring target = destRoot + L"\\" + wname;
        CreateParentDirs(target);
        HANDLE h = CreateFileW(target.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                               NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                               NULL);
        if (h == INVALID_HANDLE_VALUE) continue;
        WriteCtx ctx = {h};
        bool ok = mz_zip_reader_extract_to_callback(
            &zip, i, ExtractWrite, &ctx, 0);
        CloseHandle(h);
        if (!ok) continue;
        ++count;
        if (prog && ((i % 4) == 0 || i == n - 1)) prog(progCtx, (int)i + 1, (int)n);
    }
    mz_zip_reader_end(&zip);
    return count;
}

std::wstring GetUserPath() {
    HKEY key = NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Environment", 0, KEY_READ, &key) !=
        ERROR_SUCCESS)
        return L"";
    DWORD sz = 0;
    DWORD type = 0;
    RegQueryValueExW(key, L"Path", NULL, &type, NULL, &sz);
    std::wstring val;
    if (sz > 0) {
        val.resize(sz / sizeof(wchar_t));
        RegQueryValueExW(key, L"Path", NULL, &type, (LPBYTE)&val[0], &sz);
        val.resize(sz / sizeof(wchar_t));
        if (!val.empty() && val.back() == L'\0') val.pop_back();
    }
    RegCloseKey(key);
    return val;
}

void SetUserPath(const std::wstring& value) {
    HKEY key = NULL;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Environment", 0, NULL, 0,
                        KEY_WRITE, NULL, &key, NULL) != ERROR_SUCCESS)
        return;
    DWORD len = (DWORD)((value.size() + 1) * sizeof(wchar_t));
    RegSetValueExW(key, L"Path", 0, REG_EXPAND_SZ, (const BYTE*)value.c_str(),
                   len);
    RegCloseKey(key);
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                        (LPARAM)L"Environment", SMTO_ABORTIFHUNG, 5000, NULL);
}

std::wstring Upper(const std::wstring& s) {
    std::wstring out = s;
    for (auto& c : out) c = towupper(c);
    return out;
}

std::wstring Trim(const std::wstring& s) {
    size_t a = s.find_first_not_of(L" \t");
    size_t b = s.find_last_not_of(L" \t");
    if (a == std::wstring::npos) return L"";
    return s.substr(a, b - a + 1);
}

std::wstring TrimTrailingSlash(const std::wstring& s) {
    std::wstring out = Trim(s);
    if (!out.empty() && out.back() == L'\\') out.pop_back();
    return out;
}

bool ContainsPath(const std::wstring& current, const std::wstring& dir) {
    std::wstring norm = Upper(TrimTrailingSlash(dir));
    size_t start = 0;
    while (start <= current.size()) {
        size_t end = current.find(L';', start);
        std::wstring part =
            current.substr(start, end == std::wstring::npos
                                      ? std::wstring::npos
                                      : end - start);
        if (Upper(TrimTrailingSlash(part)) == norm) return true;
        if (end == std::wstring::npos) break;
        start = end + 1;
    }
    return false;
}

bool AddToPath(const std::wstring& dir) {
    std::wstring current = GetUserPath();
    if (ContainsPath(current, dir)) return false;
    std::wstring sep =
        (current.empty() || current.back() == L';') ? L"" : L";";
    SetUserPath(current + sep + dir);
    return true;
}

bool RemoveFromPath(const std::wstring& dir) {
    std::wstring current = GetUserPath();
    std::wstring norm = Upper(TrimTrailingSlash(dir));

    std::wstring out;
    size_t start = 0;
    bool removed = false;
    while (start <= current.size()) {
        size_t end = current.find(L';', start);
        std::wstring part =
            current.substr(start, end == std::wstring::npos
                                      ? std::wstring::npos
                                      : end - start);
        if (Upper(TrimTrailingSlash(part)) == norm) {
            removed = true;
        } else {
            if (!out.empty()) out += L";";
            out += part;
        }
        if (end == std::wstring::npos) break;
        start = end + 1;
    }
    if (removed) SetUserPath(out);
    return removed;
}

void WriteUninstallEntry(const std::wstring& uninstallerPath) {
    HKEY key = NULL;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kUninstallKey, 0, NULL, 0,
                        KEY_WRITE, NULL, &key, NULL) != ERROR_SUCCESS)
        return;
    std::wstring displayName = std::wstring(kAppName) + L" " + kVersion;
    std::wstring displayIcon = GetIdeDir() + L"\\Jts.Ide.exe";
    std::wstring uninst = L"\"" + uninstallerPath + L"\" --uninstall";
    std::wstring quiet = L"\"" + uninstallerPath + L"\" --uninstall --silent";

    auto put = [&](const wchar_t* name, const std::wstring& value) {
        RegSetValueExW(key, name, 0, REG_SZ, (const BYTE*)value.c_str(),
                       (DWORD)((value.size() + 1) * sizeof(wchar_t)));
    };
    put(L"DisplayName", displayName);
    put(L"DisplayVersion", kVersion);
    put(L"Publisher", kCompany);
    put(L"URLInfoAbout", kRepoUrl);
    put(L"InstallLocation", GetInstallDir());
    put(L"DisplayIcon", displayIcon);
    put(L"UninstallString", uninst);
    put(L"QuietUninstallString", quiet);
    DWORD dwOne = 1;
    RegSetValueExW(key, L"NoModify", 0, REG_DWORD, (const BYTE*)&dwOne,
                   sizeof(dwOne));
    RegSetValueExW(key, L"NoRepair", 0, REG_DWORD, (const BYTE*)&dwOne,
                   sizeof(dwOne));
    RegCloseKey(key);
}

void DeleteUninstallEntry() {
    RegDeleteKeyW(HKEY_CURRENT_USER, kUninstallKey);
}

bool CreateShortcut(const std::wstring& lnkPath, const std::wstring& target,
                    const std::wstring& args, const std::wstring& workDir,
                    const std::wstring& iconPath) {
    IShellLinkW* psl = NULL;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                IID_IShellLinkW, (void**)&psl)))
        return false;
    psl->SetPath(target.c_str());
    psl->SetArguments(args.c_str());
    psl->SetWorkingDirectory(workDir.c_str());
    psl->SetIconLocation(iconPath.c_str(), 0);
    IPersistFile* ppf = NULL;
    bool ok = false;
    if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf))) {
        ok = SUCCEEDED(ppf->Save(lnkPath.c_str(), TRUE));
        ppf->Release();
    }
    psl->Release();
    return ok;
}

void CreateShortcuts() {
    std::wstring folder = GetStartMenuFolder();
    CreateDirectoryW(folder.c_str(), NULL);
    std::wstring ideExe = GetIdeDir() + L"\\Jts.Ide.exe";
    CreateShortcut(folder + L"\\JTS IDE.lnk", ideExe, L"", GetIdeDir(),
                   ideExe);
    std::wstring jtsBat = GetInstallDir() + L"\\jts.bat";
    CreateShortcut(folder + L"\\JTS GO Prompt.lnk", L"cmd.exe",
                   L"/K \"" + jtsBat + L"\"", GetInstallDir(), L"cmd.exe");
}

void DeleteShortcuts() {
    std::wstring folder = GetStartMenuFolder();
    if (!PathIsDirectoryW(folder.c_str())) return;
    std::wstring pattern = folder + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            std::wstring name = fd.cFileName;
            if (name == L"." || name == L"..") continue;
            std::wstring full = folder + L"\\" + name;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                RemoveDirectoryW(full.c_str());
            else
                DeleteFileW(full.c_str());
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    RemoveDirectoryW(folder.c_str());
}

std::wstring CopySelfIntoInstallDir() {
    wchar_t self[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, self, MAX_PATH);
    std::wstring installDir = GetInstallDir();
    CreateDirectoryW(installDir.c_str(), NULL);
    std::wstring target = installDir + L"\\" + PathFindFileNameW(self);
    CopyFileW(self, target.c_str(), FALSE);
    return target;
}

bool IsRunningFromInstallDir() {
    wchar_t self[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, self, MAX_PATH);
    std::wstring dir = self;
    size_t slash = dir.find_last_of(L'\\');
    if (slash == std::wstring::npos) return false;
    dir.resize(slash);
    std::wstring installDir = GetInstallDir();
    return _wcsicmp(dir.c_str(), installDir.c_str()) == 0;
}

void RecursiveDelete(const std::wstring& dir) {
    std::wstring pattern = dir + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        std::wstring name = fd.cFileName;
        if (name == L"." || name == L"..") continue;
        std::wstring full = dir + L"\\" + name;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            RecursiveDelete(full);
        else
            DeleteFileW(full.c_str());
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    RemoveDirectoryW(dir.c_str());
}

void RecursiveDeleteExceptSelf(const std::wstring& dir) {
    wchar_t self[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, self, MAX_PATH);

    std::wstring pattern = dir + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        std::wstring name = fd.cFileName;
        if (name == L"." || name == L"..") continue;
        std::wstring full = dir + L"\\" + name;
        if (_wcsicmp(full.c_str(), self) == 0) continue;  // running exe
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            RecursiveDelete(full);
        else
            DeleteFileW(full.c_str());
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

void ScheduleInstallDirCleanup() {
    std::wstring installDir = GetInstallDir();
    wchar_t tmp[MAX_PATH];
    GetTempPathW(MAX_PATH, tmp);
    std::wstring bat = std::wstring(tmp) + L"jts_uninstall.bat";
    std::wstring content =
        L"@echo off\r\n"
        L"timeout /t 3 /nobreak >nul\r\n"
        L"rmdir /s /q \"" + installDir + L"\"\r\n"
        L"del /q \"" + bat + L"\"\r\n";
    HANDLE f = CreateFileW(bat.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, NULL);
    if (f != INVALID_HANDLE_VALUE) {
        std::string utf8 = WideToUtf8(content);
        DWORD written = 0;
        WriteFile(f, utf8.data(), (DWORD)utf8.size(), &written, NULL);
        CloseHandle(f);
    }
    std::wstring cmdLine = L"cmd.exe /c \"" + bat + L"\"";
    STARTUPINFOW si = {sizeof(si)};
    PROCESS_INFORMATION pi = {0};
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    CreateProcessW(NULL, &cmdLine[0], NULL, NULL, FALSE,
                   CREATE_NO_WINDOW | DETACHED_PROCESS, NULL, NULL, &si, &pi);
    if (pi.hProcess) CloseHandle(pi.hProcess);
    if (pi.hThread) CloseHandle(pi.hThread);
}

// .NET Framework 4.8 is required by the JTS IDE. It ships with recent Windows
// 10/11 builds but is NOT present on Windows 7 or older Windows 10 builds, so
// we detect it and offer to install it (offline installer, ~120 MB download).
const wchar_t* kNet48Url = L"https://go.microsoft.com/fwlink/?linkid=2088631";
const wchar_t* kNet48ManualUrl =
    L"https://dotnet.microsoft.com/download/dotnet-framework/net48";

bool HasNetFramework48() {
    const wchar_t* sub =
        L"SOFTWARE\\Microsoft\\NET Framework Setup\\NDP\\v4\\Full";
    const DWORD kRelease48 = 528040;
    HKEY key = NULL;
    DWORD type = 0, size = sizeof(DWORD), release = 0;
    for (int pass = 0; pass < 2; ++pass) {
        REGSAM access = KEY_READ;
        if (pass == 1) access |= KEY_WOW64_64KEY;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, sub, 0, access, &key) ==
            ERROR_SUCCESS) {
            release = 0;
            size = sizeof(DWORD);
            RegQueryValueExW(key, L"Release", NULL, &type, (LPBYTE)&release,
                             &size);
            RegCloseKey(key);
            if (release >= kRelease48) return true;
        }
    }
    return false;
}

bool DownloadNetFramework(const std::wstring& dest) {
    return SUCCEEDED(URLDownloadToFileW(NULL, kNet48Url, dest.c_str(), 0, NULL));
}

// Runs the offline .NET Framework 4.8 installer. quiet=true for silent
// installs; otherwise uses /passive so the user can watch the progress.
bool RunNetFrameworkInstaller(const std::wstring& path, bool quiet) {
    std::wstring args = quiet ? L" /quiet /norestart" : L" /passive /norestart";
    std::wstring cmd = L"\"" + path + L"\"" + args;
    STARTUPINFOW si = {sizeof(si)};
    PROCESS_INFORMATION pi = {0};
    if (!CreateProcessW(NULL, &cmd[0], NULL, NULL, FALSE, 0, NULL, NULL, &si,
                        &pi))
        return false;
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return code == 0;
}

// Returns true if .NET Framework 4.8 is present or was installed. Called by
// both the silent and GUI paths before the wizard/install finishes.
bool EnsureNetFramework(bool silent) {
    if (HasNetFramework48()) return true;

    if (silent) {
        wchar_t tmp[MAX_PATH];
        GetTempPathW(MAX_PATH, tmp);
        std::wstring dest = std::wstring(tmp) + L"ndp48-x86-x64-allos-enu.exe";
        DeleteFileW(dest.c_str());
        if (!DownloadNetFramework(dest)) return false;
        return RunNetFrameworkInstaller(dest, true);
    }

    int answer = MessageBoxW(
        NULL,
        L"The JTS IDE requires Microsoft .NET Framework 4.8, which was not "
        L"found on this computer.\r\n\r\n"
        L"JTS GO will now download and install .NET Framework 4.8 "
        L"(about 120 MB). You can also install it yourself later from\r\n"
        L"https://dotnet.microsoft.com/download/dotnet-framework/net48\r\n\r\n"
        L"Install .NET Framework 4.8 now?",
        kAppName, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);
    if (answer != IDYES) return false;

    HCURSOR prev = SetCursor(LoadCursorW(NULL, IDC_WAIT));
    wchar_t tmp[MAX_PATH];
    GetTempPathW(MAX_PATH, tmp);
    std::wstring dest = std::wstring(tmp) + L"ndp48-x86-x64-allos-enu.exe";
    DeleteFileW(dest.c_str());
    bool ok = DownloadNetFramework(dest);
    if (ok) ok = RunNetFrameworkInstaller(dest, false);
    SetCursor(prev);
    if (!ok) {
        std::wstring msg =
            std::wstring(L".NET Framework 4.8 could not be installed "
                         L"automatically.\r\n\r\n"
                         L"Please install it manually from:\r\n") +
            kNet48ManualUrl +
            std::wstring(L"\r\n\r\nThe command-line tools (jts, jtsc, jtsvm) "
                         L"will still work, but the JTS IDE will not start "
                         L"until .NET Framework 4.8 is installed.");
        MessageBoxW(NULL, msg.c_str(), kAppName, MB_OK | MB_ICONWARNING);
    }
    return ok;
}

// Installs into g_installDir. Returns 0 on success, 1 on failure.
int RunInstall(bool addPath, ProgressFn prog = NULL, void* progCtx = NULL,
               int* filesOut = NULL) {
    CoInitialize(NULL);
    int result = 1;
    if (filesOut) *filesOut = 0;

    std::vector<BYTE> payload = LoadPayload();
    if (!payload.empty()) {
        CreateDirectoryW(g_installDir.c_str(), NULL);
        int files = ExtractPayload(payload, g_installDir, prog, progCtx);
        if (files >= 0) {
            if (addPath) AddToPath(GetBinDir());
            std::wstring uninstaller = CopySelfIntoInstallDir();
            WriteUninstallEntry(uninstaller);
            CreateShortcuts();
            if (filesOut) *filesOut = files;
            result = 0;
        }
    }
    CoUninitialize();
    return result;
}

int Uninstall() {
    CoInitialize(NULL);
    bool removed = RemoveFromPath(GetBinDir());
    DeleteUninstallEntry();
    DeleteShortcuts();

    std::wstring installDir = GetInstallDir();
    bool fromInstallDir = IsRunningFromInstallDir();
    if (PathIsDirectoryW(installDir.c_str())) {
        if (fromInstallDir) {
            RecursiveDeleteExceptSelf(installDir);
            ScheduleInstallDirCleanup();
        } else {
            RecursiveDelete(installDir);
        }
    }
    CoUninitialize();
    return 0;
}

void PrintHelp() {
    OutputLine(std::wstring(kAppName) + L" " + kVersion + L" Installer");
    OutputLine(L"");
    OutputLine(L"Usage:");
    OutputLine(L"  JTS-IDE-Setup                Install " + std::wstring(kAppName) + L" + JTS IDE (GUI)");
    OutputLine(L"  JTS-IDE-Setup --silent       Install without any prompts");
    OutputLine(L"  JTS-IDE-Setup --uninstall    Remove the language and the IDE");
    OutputLine(L"  JTS-IDE-Setup --uninstall --silent");
    OutputLine(L"                                Remove without any prompts");
    OutputLine(L"  JTS-IDE-Setup --version       Show the version");
}

// ---- GUI wizard ----

struct InstallArgs {
    HWND page;
    HWND bar;
    std::wstring dir;
    bool addPath;
};

void ProgressCb(void* ctx, int done, int total) {
    InstallArgs* a = static_cast<InstallArgs*>(ctx);
    if (a->bar) {
        int pct = total > 0 ? (int)((long long)done * 100 / total) : 0;
        if (pct > 100) pct = 100;
        SendMessageW(a->bar, PBM_SETPOS, (WPARAM)pct, 0);
    }
}

DWORD WINAPI InstallThreadProc(LPVOID param) {
    InstallArgs* a = static_cast<InstallArgs*>(param);
    g_installDir = a->dir;
    int files = 0;
    int code = RunInstall(a->addPath, ProgressCb, a, &files);
    PostMessageW(a->page, kInstallDoneMsg, (WPARAM)code, (LPARAM)files);
    delete a;
    return 0;
}

std::wstring PickDirectory(HWND hwnd, const std::wstring& initial) {
    wchar_t buf[MAX_PATH] = {0};
    if (!initial.empty() && initial.size() < MAX_PATH)
        wcscpy(buf, initial.c_str());
    BROWSEINFOW bi = {0};
    bi.hwndOwner = hwnd;
    bi.pszDisplayName = buf;
    bi.lpszTitle =
        L"Choose the folder where JTS GO and the JTS IDE will be installed:";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        SHGetPathFromIDListW(pidl, buf);
        CoTaskMemFree(pidl);
        return std::wstring(buf);
    }
    return L"";
}

void SetWizButtons(HWND hwnd, DWORD flags) {
    PropSheet_SetWizButtons(GetParent(hwnd), flags);
}

std::wstring BuildReadySummary() {
    std::wstring s = L"JTS GO " + std::wstring(kVersion) + L" will be installed to:\r\n\r\n  " +
        g_installDir + L"\r\n\r\n";
    s += g_pathChoice
        ? L"jts, jtsc and jtsvm will be added to your PATH (user level)."
        : L"JTS GO commands will NOT be added to your PATH.";
    s += L"\r\n\r\nA JTS IDE shortcut will be added to the Start Menu.";
    return s;
}

// ---- per-page handlers ----

BOOL OnSetActive(HWND hwnd, int page) {
    switch (page) {
        case kPageWelcome: {
            static bool netPrompted = false;
            if (!netPrompted) {
                netPrompted = true;
                EnsureNetFramework(false);
            }
            SetWizButtons(hwnd, PSWIZB_NEXT);
            break;
        }        case kPageLicense: {
            if (!GetWindowTextLengthW(GetDlgItem(hwnd, IDC_LICENSE_EDIT))) {
                std::wstring text = Utf8ToWide(LoadLicenseText());
                SetWindowTextW(GetDlgItem(hwnd, IDC_LICENSE_EDIT),
                               text.c_str());
            }
            bool accept = IsDlgButtonChecked(hwnd, IDC_ACCEPT) == BST_CHECKED;
            SetWizButtons(hwnd, accept ? (PSWIZB_BACK | PSWIZB_NEXT)
                                       : PSWIZB_BACK);
            break;
        }
        case kPageLocation: {
            wchar_t cur[MAX_PATH] = {0};
            GetWindowTextW(GetDlgItem(hwnd, IDC_DIR), cur, MAX_PATH);
            if (cur[0] == L'\0')
                SetWindowTextW(GetDlgItem(hwnd, IDC_DIR),
                               GetDefaultInstallDir().c_str());
            SetWizButtons(hwnd, PSWIZB_BACK | PSWIZB_NEXT);
            break;
        }
        case kPagePath:
            SetWizButtons(hwnd, PSWIZB_BACK | PSWIZB_NEXT);
            break;
        case kPageReady:
            SetWindowTextW(GetDlgItem(hwnd, IDC_SUMMARY),
                           BuildReadySummary().c_str());
            SetWizButtons(hwnd, PSWIZB_BACK | PSWIZB_NEXT);
            break;
        case kPageProgress: {
            SetWizButtons(hwnd, 0);
            HWND bar = GetDlgItem(hwnd, IDC_PROGRESS_BAR);
            SendMessageW(bar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
            SendMessageW(bar, PBM_SETPOS, 0, 0);
            SetWindowTextW(GetDlgItem(hwnd, IDC_STATUS),
                           L"Installing JTS GO...");
            InstallArgs* a = new InstallArgs();
            a->page = hwnd;
            a->bar = bar;
            a->dir = g_installDir;
            a->addPath = g_pathChoice;
            CreateThread(NULL, 0, InstallThreadProc, a, 0, NULL);
            break;
        }
        case kPageDone:
            SetWizButtons(hwnd, PSWIZB_FINISH);
            break;
    }
    return TRUE;
}

BOOL OnKillActive(HWND hwnd, int page) {
    switch (page) {
        case kPageLicense: {
            if (!IsDlgButtonChecked(hwnd, IDC_ACCEPT)) {
                MessageBoxW(hwnd,
                            L"You must accept the license agreement to "
                            L"continue installing JTS GO.",
                            kAppName, MB_OK | MB_ICONWARNING);
                SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, TRUE);
                return TRUE;
            }
            break;
        }
        case kPageLocation: {
            wchar_t buf[MAX_PATH] = {0};
            GetWindowTextW(GetDlgItem(hwnd, IDC_DIR), buf, MAX_PATH);
            std::wstring dir = TrimTrailingSlash(buf);
            if (dir.empty()) {
                MessageBoxW(hwnd, L"Please choose an install location.",
                            kAppName, MB_OK | MB_ICONWARNING);
                SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, TRUE);
                return TRUE;
            }
            g_installDir = dir;
            break;
        }
        case kPagePath:
            g_pathChoice =
                IsDlgButtonChecked(hwnd, IDC_PATH_YES) == BST_CHECKED;
            break;
    }
    return FALSE;
}

BOOL OnWizFinish(HWND hwnd) {
    if (IsDlgButtonChecked(hwnd, IDC_LAUNCH) == BST_CHECKED) {
        std::wstring ideExe = GetIdeDir() + L"\\Jts.Ide.exe";
        ShellExecuteW(NULL, L"open", ideExe.c_str(), NULL,
                      GetIdeDir().c_str(), SW_SHOWNORMAL);
    }
    return TRUE;
}

void ShowInstallError(HWND hwnd, int code) {
    MessageBoxW(hwnd,
                L"JTS GO could not be installed. The installer may have been "
                L"built without its payload, or the payload failed to extract.",
                kAppName, MB_OK | MB_ICONERROR);
}

INT_PTR CALLBACK PageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG: {
            LPPROPSHEETPAGEW psp = (LPPROPSHEETPAGEW)lParam;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)psp->lParam);
            return TRUE;
        }
        case WM_COMMAND: {
            int page = (int)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
            if (HIWORD(wParam) == BN_CLICKED) {
                if (LOWORD(wParam) == IDC_ACCEPT) {
                    bool accept =
                        IsDlgButtonChecked(hwnd, IDC_ACCEPT) == BST_CHECKED;
                    SetWizButtons(hwnd, accept ? (PSWIZB_BACK | PSWIZB_NEXT)
                                               : PSWIZB_BACK);
                } else if (LOWORD(wParam) == IDC_BROWSE) {
                    wchar_t cur[MAX_PATH] = {0};
                    GetWindowTextW(GetDlgItem(hwnd, IDC_DIR), cur, MAX_PATH);
                    std::wstring dir = PickDirectory(hwnd, cur);
                    if (!dir.empty())
                        SetWindowTextW(GetDlgItem(hwnd, IDC_DIR), dir.c_str());
                }
            }
            return TRUE;
        }
        case WM_NOTIFY: {
            NMHDR* nm = (NMHDR*)lParam;
            int page = (int)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
            switch (nm->code) {
                case PSN_SETACTIVE:
                    return OnSetActive(hwnd, page);
                case PSN_KILLACTIVE:
                    return OnKillActive(hwnd, page);
                case PSN_WIZFINISH:
                    return OnWizFinish(hwnd);
                case PSN_QUERYCANCEL:
                    if (page == kPageProgress) {
                        SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, TRUE);
                        return TRUE;
                    }
                    break;
            }
            break;
        }
        case WM_APP + 1: {
            if (wParam != 0) {
                ShowInstallError(hwnd, (int)wParam);
            } else {
                int files = (int)lParam;
                wchar_t msg[64];
                wsprintfW(msg, L"Installed %d files.", files);
                SetWindowTextW(GetDlgItem(hwnd, IDC_STATUS), msg);
            }
            SendMessageW(GetParent(hwnd), PSM_SETCURSEL, (WPARAM)kPageDone, 0);
            return TRUE;
        }
    }
    return FALSE;
}

void RunInstallWizard() {
    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_WIN95_CLASSES |
                                              ICC_STANDARD_CLASSES |
                                              ICC_PROGRESS_CLASS};
    InitCommonControlsEx(&icc);

    HINSTANCE hInst = GetModuleHandleW(NULL);
    static const int ids[] = {IDD_WELCOME,    IDD_LICENSE, IDD_LOCATION,
                              IDD_PATH,       IDD_READY,   IDD_PROGRESS,
                              IDD_DONE};
    const int count = sizeof(ids) / sizeof(ids[0]);

    HPROPSHEETPAGE pages[7];
    PROPSHEETPAGEW psp = {0};
    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = hInst;
    for (int i = 0; i < count; ++i) {
        psp.pszTemplate = MAKEINTRESOURCEW(ids[i]);
        psp.pfnDlgProc = PageProc;
        psp.lParam = (LPARAM)i;
        pages[i] = CreatePropertySheetPageW(&psp);
    }

    PROPSHEETHEADERW psh = {0};
    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_WIZARD97 | PSH_USEHICON;
    psh.hInstance = hInst;
    psh.hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_APP));
    psh.pszCaption = L"JTS GO 2.1.0 Setup";
    psh.nPages = count;
    psh.phpage = pages;

    PropertySheetW(&psh);
}

void RunUninstallGui() {
    int answer = MessageBoxW(
        NULL,
        L"Are you sure you want to remove JTS GO and the JTS IDE from this "
        L"computer?\r\n\r\nThis will delete the installed files and remove "
        L"the PATH entry.",
        kAppName, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
    if (answer == IDYES) {
        HCURSOR prev = SetCursor(LoadCursorW(NULL, IDC_WAIT));
        Uninstall();
        SetCursor(prev);
        MessageBoxW(NULL, L"JTS GO has been uninstalled.", kAppName,
                    MB_OK | MB_ICONINFORMATION);
    }
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
    bool uninstall = false, silent = false, version = false, help = false;
    for (int i = 1; i < argc; ++i) {
        std::wstring low;
        for (auto& c : std::wstring(argv[i])) low += towlower(c);
        if (low == L"--uninstall") uninstall = true;
        else if (low == L"--silent") silent = true;
        else if (low == L"--version") version = true;
        else if (low == L"--help" || low == L"-h" || low == L"-?") help = true;
    }

    if (version || help) {
        if (version) OutputLine(std::wstring(kAppName) + L" " + kVersion);
        if (help) PrintHelp();
        return 0;
    }

    if (uninstall) {
        std::wstring loc = ReadInstallLocationFromRegistry();
        g_installDir = loc.empty() ? GetDefaultInstallDir() : loc;
        if (silent) return Uninstall();
        RunUninstallGui();
        return 0;
    }

    // Install
    if (silent) {
        g_installDir = GetDefaultInstallDir();
        EnsureNetFramework(true);
        return RunInstall(true);
    }
    g_installDir = GetDefaultInstallDir();
    RunInstallWizard();
    return 0;
}
