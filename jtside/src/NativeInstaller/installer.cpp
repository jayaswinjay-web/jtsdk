// JTS GO native installer (Windows 7+ compatible, no .NET / Node dependency).
// Self-contained: embeds the payload.zip as a Win32 RCDATA resource and extracts it.
//
// Usage:
//   JTS-IDE-Setup                install
//   JTS-IDE-Setup --install      install (same as no argument)
//   JTS-IDE-Setup --uninstall    remove the language and the IDE
//   JTS-IDE-Setup --uninstall --silent
//   JTS-IDE-Setup --version      print version
//   JTS-IDE-Setup --help         print help
#define _WIN32_WINNT 0x0601
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <objbase.h>
#include <shobjidl.h>
#include <shellapi.h>

#include <cstdio>
#include <cstring>
#include <cwctype>
#include <string>
#include <vector>

#include "miniz.h"

#define IDR_PAYLOAD 101

namespace {

const wchar_t* kAppName = L"JTS GO";
const wchar_t* kVersion = L"2.1.0";
const wchar_t* kCompany = L"JayTech Solutions";
const wchar_t* kRepoUrl = L"https://github.com/jayaswinjay-web/jtsdk";
const wchar_t* kUninstallKey =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\JTS GO";

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

std::wstring GetInstallDir() {
    return GetLocalAppData() + L"\\Programs\\JTS GO";
}
std::wstring GetBinDir() { return GetInstallDir() + L"\\bin"; }
std::wstring GetIdeDir() { return GetInstallDir() + L"\\ide"; }
std::wstring GetStartMenuFolder() { return GetProgramsMenu() + L"\\JTS GO"; }

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
    HRSRC h = FindResourceW(NULL, MAKEINTRESOURCE(IDR_PAYLOAD),
                            RT_RCDATA);
    if (h == NULL) return std::vector<BYTE>();
    HGLOBAL g = LoadResource(NULL, h);
    if (g == NULL) return std::vector<BYTE>();
    void* p = LockResource(g);
    DWORD sz = SizeofResource(NULL, h);
    if (p == NULL || sz == 0) return std::vector<BYTE>();
    return std::vector<BYTE>((BYTE*)p, (BYTE*)p + sz);
}

// Print a wide string to stdout. Uses WriteConsoleW when attached to a real
// console; otherwise converts to the ANSI codepage (keeps ASCII messages
// intact when output is redirected/piped).
void Output(const std::wstring& s) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &mode)) {
        WriteConsoleW(hOut, s.c_str(), (DWORD)s.size(), NULL, NULL);
    } else {
        int n = WideCharToMultiByte(CP_ACP, 0, s.c_str(), (int)s.size(), NULL,
                                    0, NULL, NULL);
        if (n > 0) {
            std::string a(n, '\0');
            WideCharToMultiByte(CP_ACP, 0, s.c_str(), (int)s.size(), &a[0], n,
                                NULL, NULL);
            fwrite(a.data(), 1, a.size(), stdout);
            fflush(stdout);
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

// Returns number of files extracted, or -1 on failure.
int ExtractPayload(const std::vector<BYTE>& data, const std::wstring& destRoot) {
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
    // Tell the shell (and future processes) the environment changed.
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

// Returns true if the PATH was changed.
bool AddToPath(const std::wstring& dir) {
    std::wstring current = GetUserPath();
    if (ContainsPath(current, dir)) return false;
    std::wstring sep =
        (current.empty() || current.back() == L';') ? L"" : L";";
    SetUserPath(current + sep + dir);
    return true;
}

// Returns true if the PATH was changed.
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

// Copy the running setup.exe into the install dir so the uninstall entry
// keeps working after the original setup.exe is gone.
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

// Delete everything under dir except the currently running executable.
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

// Schedule removal of the (now mostly empty) install dir after this process
// exits. The running uninstaller itself lives in the install dir, so a
// detached cmd waits a few seconds for us to exit, then removes the rest.
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

int Install(bool silent) {
    CoInitialize(NULL);
    std::vector<BYTE> payload = LoadPayload();
    if (payload.empty()) {
        if (!silent)
            OutputLine(L"Error: this installer was built without a payload.");
        return 1;
    }
    std::wstring installDir = GetInstallDir();
    CreateDirectoryW(installDir.c_str(), NULL);

    int files = ExtractPayload(payload, installDir);
    if (files < 0) {
        if (!silent) OutputLine(L"Error: failed to extract payload.");
        return 1;
    }

    bool pathAdded = AddToPath(GetBinDir());
    std::wstring uninstaller = CopySelfIntoInstallDir();
    WriteUninstallEntry(uninstaller);
    CreateShortcuts();

    if (!silent) {
        OutputLine(L"");
        OutputLine(std::wstring(kAppName) + L" " + kVersion + L" installed successfully.");
        OutputLine(L"  Location : " + installDir);
        OutputLine(L"  Files    : " + std::to_wstring(files));
        OutputLine(L"  Commands : jts (run)   jtsc (compile)   jtsvm (bytecode)");
        OutputLine(L"  IDE      : JTS IDE (Start Menu > JTS GO)");
        if (pathAdded) {
            OutputLine(L"  PATH     : " + GetBinDir() + L" added");
            OutputLine(L"");
            OutputLine(L"NOTE: Reopen any open terminals so \x27jts\x27 becomes available.");
        }
    }
    CoUninitialize();
    return 0;
}

int Uninstall(bool silent) {
    CoInitialize(NULL);
    bool removed = RemoveFromPath(GetBinDir());
    DeleteUninstallEntry();
    DeleteShortcuts();

    std::wstring installDir = GetInstallDir();
    bool fromInstallDir = IsRunningFromInstallDir();
    if (PathIsDirectoryW(installDir.c_str())) {
        if (fromInstallDir) {
            // Can't delete the running exe; clean everything else and let a
            // detached cmd remove the rest after we exit.
            RecursiveDeleteExceptSelf(installDir);
            ScheduleInstallDirCleanup();
        } else {
            RecursiveDelete(installDir);
        }
    }
    if (!silent) {
        OutputLine(std::wstring(kAppName) + L" has been uninstalled.");
        OutputLine(removed ? L"  Removed PATH entry : true" : L"  Removed PATH entry : false");
        OutputLine(L"  Removed location   : " + installDir);
    }
    CoUninitialize();
    return 0;
}

void PrintHelp() {
    OutputLine(std::wstring(kAppName) + L" " + kVersion + L" Installer");
    OutputLine(L"");
    OutputLine(L"Usage:");
    OutputLine(L"  JTS-IDE-Setup                Install " + std::wstring(kAppName) + L" + JTS IDE");
    OutputLine(L"  JTS-IDE-Setup --install       Install (same as no argument)");
    OutputLine(L"  JTS-IDE-Setup --uninstall     Remove the language and the IDE");
    OutputLine(L"  JTS-IDE-Setup --uninstall --silent");
    OutputLine(L"                                Remove without any prompts");
    OutputLine(L"  JTS-IDE-Setup --version       Show the version");
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

    if (version) {
        OutputLine(std::wstring(kAppName) + L" " + kVersion);
        return 0;
    }
    if (help) {
        PrintHelp();
        return 0;
    }
    if (uninstall) return Uninstall(silent);
    return Install(silent);
}



