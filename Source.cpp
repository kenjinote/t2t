#ifndef UNICODE
#define UNICODE
#endif
#define CURL_STATICLIB
#include <windows.h>
#include <windowsx.h>
#include <d2d1.h>
#include <dwrite_3.h> 
#include <curl/curl.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <atomic>
#include "resource.h"
#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Wldap32.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Normaliz.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Secur32.lib")
#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#define WM_CONTENT_LOADED (WM_APP + 1)
#define WM_SUBMIT_FORM    (WM_APP + 2) 
#define WM_DO_TAB         (WM_APP + 3) 
#define WM_SYNC_FOCUS     (WM_APP + 4) 
#define WM_EXECUTE_SEARCH (WM_APP + 5) 
#define WM_CANCEL_SEARCH  (WM_APP + 6) 
#define WM_EXECUTE_URL    (WM_APP + 7) 
#define TIMER_AUTOSCROLL  1001
const wchar_t* CUSTOM_FONT_FAMILY = L"ノスタルドット（M+）";
const int CUSTOM_FONT_SIZE = 24;
ID2D1Factory* pD2DFactory = nullptr;
ID2D1HwndRenderTarget* pRT = nullptr;
ID2D1SolidColorBrush* pTextBrush = nullptr;
ID2D1SolidColorBrush* pSelectionBrush = nullptr;
ID2D1SolidColorBrush* pLinkBrush = nullptr;
ID2D1SolidColorBrush* pFocusBrush = nullptr;
ID2D1SolidColorBrush* pHrBrush = nullptr;
ID2D1SolidColorBrush* pStatusBarBrush = nullptr;
ID2D1SolidColorBrush* pStatusTextBrush = nullptr;
IDWriteFactory* pDWriteFactory = nullptr;
IDWriteTextFormat* pTextFormat = nullptr;
IDWriteTextLayout* pTextLayout = nullptr;
HFONT g_hCustomFont = nullptr;
HANDLE g_hGdiFont = nullptr;
struct Hyperlink { UINT32 start; UINT32 length; std::string url; };
struct HrElement { UINT32 textPosition; };
struct FormInput {
    UINT32 start;
    UINT32 length;
    std::wstring type;
    std::wstring name;
    std::wstring value;
    HWND hwnd;
    std::string action;
    std::string method;
    bool checked;
};
enum class FocusType { LINK, INPUT };
struct FocusItem {
    FocusType type;
    size_t index;
    UINT32 start;
};
std::wstring g_WebPageContent = L"Loading...";
std::wstring g_PageTitle = L"t2t";
std::vector<Hyperlink> g_Links;
std::vector<FormInput> g_Inputs;
std::vector<FocusItem> g_FocusItems;
std::vector<HrElement> g_Hrs;
int g_FocusIndex = -1;
std::mutex g_ContentMutex;
float g_ScrollY = 0.0f;
float g_MaxScrollY = 0.0f;
std::atomic<bool> g_AppRunning{ true };
std::atomic<int> g_LoadEpoch{ 0 };
UINT32 g_SelectionAnchor = UINT32_MAX;
UINT32 g_SelectionStart = 0;
UINT32 g_SelectionLength = 0;
bool g_IsSelecting = false;
bool g_IsHoveringLink = false;
std::string g_HoveredUrl = "";
std::vector<std::string> g_History;
int g_HistoryIndex = -1;
HWND g_hSearchEdit = nullptr;
bool g_IsSearching = false;
bool g_IsUrlMode = false;
std::wstring g_LastSearchQuery = L"";
WNDPROC g_OldSearchEditProc = nullptr;
void NavigateTo(HWND hwnd, const std::string& url, const std::string& postData = "");
void GoBack(HWND hwnd);
void GoForward(HWND hwnd);
void ReloadCurrentPage(HWND hwnd);
void OpenAddressBar(HWND hwnd);
std::string WideToNarrow(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    if (size <= 0) return "";
    std::string narrowStr(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &narrowStr[0], size, NULL, NULL);
    return narrowStr;
}
std::wstring NarrowToWide(const std::string& str, UINT codePage = CP_UTF8) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(codePage, 0, str.c_str(), -1, NULL, 0);
    if (size <= 0) return L"";
    std::wstring wstr(size - 1, 0);
    MultiByteToWideChar(codePage, 0, str.c_str(), -1, &wstr[0], size);
    return wstr;
}
bool HandleGlobalShortcuts(HWND hWnd, UINT uMsg, WPARAM wParam) {
    HWND hMain = GetParent(hWnd);
    if (hMain == NULL) {
        hMain = hWnd;
    }
    if (uMsg == WM_SYSKEYDOWN) {
        if (wParam == VK_LEFT) { GoBack(hMain); return true; }
        if (wParam == VK_RIGHT) { GoForward(hMain); return true; }
        if (wParam == VK_HOME) { NavigateTo(hMain, "about:home"); return true; }
        if (wParam == 'D') { OpenAddressBar(hMain); return true; }
    }
    if (uMsg == WM_KEYDOWN) {
        if (wParam == VK_F1) {
            NavigateTo(hMain, "about:help");
            return true;
        }
        if (wParam == 'L' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            OpenAddressBar(hMain);
            return true;
        }
        if (wParam == VK_F5 || (wParam == 'R' && (GetKeyState(VK_CONTROL) & 0x8000))) {
            ReloadCurrentPage(hMain);
            return true;
        }
    }
    return false;
}
LRESULT CALLBACK SearchEditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (HandleGlobalShortcuts(hWnd, uMsg, wParam)) return 0;
    if (uMsg == WM_KEYDOWN) {
        if (wParam == VK_RETURN) {
            bool isCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            if (wParam == VK_PRIOR || wParam == VK_NEXT ||
                (isCtrl && (wParam == VK_HOME || wParam == VK_END))) {
                PostMessage(GetParent(hWnd), uMsg, wParam, lParam);
                return 0;
            }
            if (g_IsUrlMode) {
                PostMessage(GetParent(hWnd), WM_EXECUTE_URL, 0, 0);
            }
            else {
                wchar_t buf[4096] = { 0 };
                GetWindowText(hWnd, buf, 4096);
                g_LastSearchQuery = buf;
                PostMessage(GetParent(hWnd), WM_EXECUTE_SEARCH, 1, 0);
            }
            return 0;
        }
        if (wParam == VK_ESCAPE) {
            PostMessage(GetParent(hWnd), WM_CANCEL_SEARCH, 0, 0);
            return 0;
        }
    }
    if (uMsg == WM_CHAR && (wParam == VK_RETURN || wParam == VK_ESCAPE)) return 0;
    if (uMsg == WM_SETFOCUS) {
        PostMessage(hWnd, EM_SETSEL, 0, -1);
    }
    return CallWindowProc(g_OldSearchEditProc, hWnd, uMsg, wParam, lParam);
}
LRESULT CALLBACK InputSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WNDPROC oldProc = (WNDPROC)GetProp(hWnd, L"OldProc");
    if (HandleGlobalShortcuts(hWnd, uMsg, wParam)) return 0;
    if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) {
        bool isCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        if (wParam == VK_PRIOR || wParam == VK_NEXT ||
            (isCtrl && (wParam == VK_HOME || wParam == VK_END))) {
            PostMessage(GetParent(hWnd), uMsg, wParam, lParam);
            return 0;
        }
        if (wParam == VK_TAB) {
            PostMessage(GetParent(hWnd), WM_DO_TAB, (GetKeyState(VK_SHIFT) & 0x8000) ? 1 : 0, 0);
            return 0;
        }
        if (wParam == VK_RETURN) {
            wchar_t className[256];
            GetClassName(hWnd, className, 256);
            if (lstrcmpi(className, L"EDIT") == 0) {
                PostMessage(GetParent(hWnd), WM_SUBMIT_FORM, 0, 0);
                return 0;
            }
            else if (lstrcmpi(className, L"BUTTON") == 0) {
                PostMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hWnd), BN_CLICKED), (LPARAM)hWnd);
                return 0;
            }
        }
    }
    if (uMsg == WM_CHAR && (wParam == VK_RETURN || wParam == VK_TAB)) {
        wchar_t className[256];
        GetClassName(hWnd, className, 256);
        if (lstrcmpi(className, L"EDIT") == 0 || lstrcmpi(className, L"BUTTON") == 0) return 0;
    }
    if (uMsg == WM_SETFOCUS) {
        PostMessage(GetParent(hWnd), WM_SYNC_FOCUS, (WPARAM)hWnd, 0);
        wchar_t className[256];
        GetClassName(hWnd, className, 256);
        if (lstrcmpi(className, L"EDIT") == 0) {
            PostMessage(hWnd, EM_SETSEL, 0, -1);
        }
    }
    if (oldProc) return CallWindowProc(oldProc, hWnd, uMsg, wParam, lParam);
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
std::wstring ExtractAttribute(const std::wstring& tag, const std::wstring& attr) {
    std::wstring search = attr + L"=";
    size_t pos = 0;
    while ((pos = tag.find(search, pos)) != std::wstring::npos) {
        if (pos > 0 && (tag[pos - 1] == L' ' || tag[pos - 1] == L'\t' || tag[pos - 1] == L'\n')) {
            pos += search.length();
            if (pos >= tag.length()) return L"";
            wchar_t quote = tag[pos];
            if (quote == L'"' || quote == L'\'') {
                pos++; size_t endPos = tag.find(quote, pos);
                if (endPos != std::wstring::npos) return tag.substr(pos, endPos - pos);
                else return tag.substr(pos);
            }
            else {
                size_t endPos = tag.find_first_of(L" >/", pos);
                if (endPos == std::wstring::npos) endPos = tag.length();
                return tag.substr(pos, endPos - pos);
            }
        }
        else { pos += search.length(); }
    }
    return L"";
}
std::string ResolveUrl(const std::string& base, const std::string& rel) {
    if (rel.empty() || rel.find("http://") == 0 || rel.find("https://") == 0 || rel.find("about:") == 0) return rel;
    if (rel[0] == '/') {
        size_t pos = base.find("://");
        if (pos != std::string::npos) {
            pos = base.find("/", pos + 3);
            if (pos != std::string::npos) return base.substr(0, pos) + rel;
            return base + rel;
        }
    }
    std::string basePath = base;
    size_t hashPos = basePath.find('#');
    if (hashPos != std::string::npos) basePath = basePath.substr(0, hashPos);
    size_t queryPos = basePath.find('?');
    if (queryPos != std::string::npos) basePath = basePath.substr(0, queryPos);
    size_t lastSlash = basePath.rfind('/');
    size_t protoSlash = basePath.find("://");
    if (lastSlash != std::string::npos && (protoSlash == std::string::npos || lastSlash > protoSlash + 2)) {
        basePath = basePath.substr(0, lastSlash + 1);
    }
    else {
        if (!basePath.empty() && basePath.back() != '/') basePath += '/';
    }
    return basePath + rel;
}
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb); return size * nmemb;
}
std::wstring GetTagName(const std::wstring& tag) {
    std::wstring name = L"";
    for (wchar_t c : tag) {
        if (c == L' ' || c == L'\t' || c == L'\n' || c == L'\r' || c == L'>') break;
        name += towlower(c);
    }
    return name;
}
struct ParsedPage {
    std::wstring text; std::wstring title;
    std::vector<Hyperlink> links; std::vector<FormInput> inputs;
    std::vector<HrElement> hrs;
};
UINT DetectCodePage(const std::string& html) {
    std::string lowerHtml = html;
    std::transform(lowerHtml.begin(), lowerHtml.end(), lowerHtml.begin(), ::tolower);
    if (lowerHtml.find("charset=shift_jis") != std::string::npos ||
        lowerHtml.find("charset=\"shift_jis\"") != std::string::npos ||
        lowerHtml.find("charset='shift_jis'") != std::string::npos ||
        lowerHtml.find("charset=sjis") != std::string::npos) {
        return 932;
    }
    if (lowerHtml.find("charset=euc-jp") != std::string::npos ||
        lowerHtml.find("charset=\"euc-jp\"") != std::string::npos) {
        return 20932;
    }
    return CP_UTF8;
}
ParsedPage FetchAndStripHTML(const std::string& url, const std::string& postData = "") {
    std::wstring wstr;
    if (url == "about:home") {
        wstr = L"<html><head><title>Start Page</title></head><body>"
            L"▄▄▄ ▄▄ ▄▄▄<br>"
            L"　█　  ▄█ 　█<br>"
            L"　█　 █▄ 　█<br><hr>"
            L"<form action=\"https://search.yahoo.co.jp/search\" method=\"GET\">"
            L"Web検索 (Yahoo!検索): <input type=\"text\" name=\"p\" value=\"\"> "
            L"<button type=\"submit\">検索</button>"
            L"</form><br><br>"
            L"<h3>ブックマーク</h3>"
            L"<a href=\"https://ja.wikipedia.org\">Wikipedia</a><br>"
            L"<a href=\"https://lite.cnn.com\">CNN Lite</a><br><br>"
            L"<a href=\"https://hack.jp\">hack.jp</a><br>"
            L"<hr><br>"
            L"※ショートカットキーの一覧を見るには <a href=\"about:help\">F1キー</a> を押してください。"
            L"</body></html>";
    }
    else if (url == "about:help") {
        wstr = L"<html><head><title>Help</title></head><body>"
            L"<h1>ヘルプ</h1>"
            L"<h3>◆ ナビゲーション</h3>"
            L"<b>F1</b> : このヘルプ画面を開く<br>"
            L"<b>Alt + Home</b> / <b>Ctrl + L</b> : ホーム画面 (about:home) へ移動<br>"
            L"<b>Alt + D</b> : アドレスバー (URL入力) を開く<br>"
            L"<b>F5</b> / <b>Ctrl + R</b> : ページを再読み込み (リロード)<br>"
            L"<b>Alt + ←</b> / <b>Alt + →</b> : 前のページに戻る / 次のページに進む<br><br>"
            L"<h3>◆ スクロール ＆ フォーカス</h3>"
            L"<b>j</b> / <b>↓キー</b> : 下へスクロール<br>"
            L"<b>k</b> / <b>↑キー</b> : 上へスクロール<br>"
            L"<b>Space</b> / <b>PageDown</b> : 1画面下へスクロール<br>"
            L"<b>b</b> / <b>PageUp</b> : 1画面上へスクロール<br>"
            L"<b>Home</b> / <b>End</b> : ページの先頭 / 末尾へジャンプ<br>"
            L"<b>Tab</b> / <b>Shift + Tab</b> : 次のリンク・入力欄へフォーカス / 前へ戻る<br>"
            L"<b>Enter</b> : リンクを開く / フォームを送信する<br><br>"
            L"<h3>◆ 検索 ＆ コピー操作</h3>"
            L"<b>/</b> (スラッシュ) : ページ内検索<br>"
            L"<b>n</b> / <b>N (Shift+n)</b> : 次の検索結果 / 前の検索結果へジャンプ<br>"
            L"<b>Esc</b> : テキスト選択を解除 / 検索ボックスを閉じる<br>"
            L"<b>ダブルクリック</b> : クリックした位置の単語を選択<br>"
            L"<b>Ctrl + A</b> : ページのテキストをすべて選択<br>"
            L"<b>Ctrl + C</b> : 選択中のテキストをクリップボードにコピー<br>"
            L"<b>q</b> : ブラウザを終了する<br><br>"
            L"<hr><br>"
            L"t2t(v1.0.1)"
            L"</body></html>";
    }
    else {
        CURL* curl = curl_easy_init();
        std::string readBuffer;
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
            if (!postData.empty()) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
            }
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
            curl_easy_perform(curl);
            curl_easy_cleanup(curl);
        }
        UINT cp = DetectCodePage(readBuffer);
        wstr = NarrowToWide(readBuffer, cp);
    }
    std::wstring stripped; std::wstring pageTitle;
    std::vector<Hyperlink> links; std::vector<FormInput> inputs;
    std::vector<HrElement> hrs;
    bool inTag = false, inScriptOrStyle = false, inTitle = false, inPreTag = false;
    std::wstring currentTag = L"";
    int currentLinkStart = -1; std::string currentLinkUrl = "";
    std::wstring currentFormAction = L"";
    std::string currentFormMethod = "GET";
    bool inButton = false;
    std::wstring currentBtnType = L"";
    std::wstring currentBtnName = L"";
    std::wstring currentBtnVal = L"";
    std::string currentBtnAction = "";
    auto AppendTextChar = [&](wchar_t c) {
        if (inTitle) {
            pageTitle += c;
        }
        else if (inButton) {
            if (c != L'\r' && c != L'\n' && c != L'\t') currentBtnVal += c;
            else currentBtnVal += L' ';
        }
        else if (!inScriptOrStyle) {
            if (inPreTag) {
                if (c == L'\r') return;
                stripped += c;
            }
            else {
                if (c == L'\r' || c == L'\n' || c == L'\t') c = L' ';
                if (c == L' ' && !stripped.empty() && (stripped.back() == L' ' || stripped.back() == L'\n')) return;
                stripped += c;
            }
        }
        };
    for (size_t i = 0; i < wstr.length(); ++i) {
        wchar_t c = wstr[i];
        if (c == L'<') { inTag = true; currentTag = L""; }
        else if (c == L'>') {
            inTag = false; std::wstring tagName = GetTagName(currentTag);
            if (tagName == L"script" || tagName == L"style") inScriptOrStyle = true;
            else if (tagName == L"/script" || tagName == L"/style") inScriptOrStyle = false;
            else if (tagName == L"title") inTitle = true;
            else if (tagName == L"/title") inTitle = false;
            else if (tagName == L"form") {
                currentFormAction = ExtractAttribute(currentTag, L"action");
                std::wstring wMethod = ExtractAttribute(currentTag, L"method");
                std::transform(wMethod.begin(), wMethod.end(), wMethod.begin(), ::toupper);
                if (wMethod == L"POST") currentFormMethod = "POST";
                else currentFormMethod = "GET";
            }
            else if (tagName == L"/form") {
                currentFormAction = L"";
                currentFormMethod = "GET";
            }
            else if (tagName == L"pre") {
                inPreTag = true;
                if (!stripped.empty() && stripped.back() != L'\n') stripped += L"\n";
            }
            else if (tagName == L"/pre") {
                inPreTag = false;
                if (!stripped.empty() && stripped.back() != L'\n') stripped += L"\n";
            }
            else if (tagName == L"a") {
                currentLinkStart = (int)stripped.length();
                std::wstring wHref = ExtractAttribute(currentTag, L"href");
                currentLinkUrl = ResolveUrl(url, WideToNarrow(wHref));
            }
            else if (tagName == L"/a") {
                if (currentLinkStart != -1 && !currentLinkUrl.empty()) {
                    int len = (int)stripped.length() - currentLinkStart;
                    if (len > 0) links.push_back({ (UINT32)currentLinkStart, (UINT32)len, currentLinkUrl });
                }
                currentLinkStart = -1; currentLinkUrl = "";
            }
            else if (tagName == L"input") {
                std::wstring type = ExtractAttribute(currentTag, L"type");
                std::transform(type.begin(), type.end(), type.begin(), ::towlower);
                std::wstring name = ExtractAttribute(currentTag, L"name");
                std::wstring val = ExtractAttribute(currentTag, L"value");
                if (type.empty()) type = L"text";
                std::wstring lowerTag = currentTag;
                std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), ::towlower);
                bool isChecked = (lowerTag.find(L" checked") != std::wstring::npos ||
                    lowerTag.find(L"checked=") != std::wstring::npos ||
                    lowerTag.find(L"checked>") != std::wstring::npos);
                std::string narrowAction = WideToNarrow(currentFormAction);
                std::string resolvedAction = ResolveUrl(url, narrowAction);
                if (type == L"hidden") {
                    inputs.push_back({ (UINT32)stripped.length(), 0, type, name, val, NULL, resolvedAction, currentFormMethod, isChecked });
                }
                else {
                    std::wstring placeholder;
                    if (type == L"button" || type == L"submit" || type == L"reset") {
                        if (val.empty()) val = L"送信";
                        std::wstring safeVal = val;
                        std::replace(safeVal.begin(), safeVal.end(), L' ', L'\x00A0');
                        placeholder = std::wstring(3, L'\x00A0') + safeVal + std::wstring(3, L'\x00A0');
                    }
                    else if (type == L"radio" || type == L"checkbox") {
                        placeholder = std::wstring(3, L'\x00A0');
                    }
                    else {
                        placeholder = std::wstring(20, L'\x00A0');
                    }
                    int start = (int)stripped.length();
                    stripped += placeholder;
                    inputs.push_back({ (UINT32)start, (UINT32)placeholder.length(), type, name, val, NULL, resolvedAction, currentFormMethod, isChecked });
                    stripped += L" ";
                }
            }
            else if (tagName == L"button") {
                inButton = true;
                currentBtnType = ExtractAttribute(currentTag, L"type");
                std::transform(currentBtnType.begin(), currentBtnType.end(), currentBtnType.begin(), ::towlower);
                if (currentBtnType.empty()) currentBtnType = L"submit";
                currentBtnName = ExtractAttribute(currentTag, L"name");
                currentBtnVal = ExtractAttribute(currentTag, L"value");
                std::string narrowAction = WideToNarrow(currentFormAction);
                currentBtnAction = ResolveUrl(url, narrowAction);
            }
            else if (tagName == L"/button") {
                inButton = false;
                currentBtnVal.erase(0, currentBtnVal.find_first_not_of(L" \t\r\n"));
                currentBtnVal.erase(currentBtnVal.find_last_not_of(L" \t\r\n") + 1);
                if (currentBtnVal.empty()) currentBtnVal = L"送信";
                std::wstring safeVal = currentBtnVal;
                std::replace(safeVal.begin(), safeVal.end(), L' ', L'\x00A0');
                std::wstring placeholder = std::wstring(3, L'\x00A0') + safeVal + std::wstring(3, L'\x00A0');
                int start = (int)stripped.length();
                stripped += placeholder;
                inputs.push_back({ (UINT32)start, (UINT32)placeholder.length(), currentBtnType, currentBtnName, currentBtnVal, NULL, currentBtnAction, currentFormMethod, false });
                stripped += L" ";
            }
            else if (tagName == L"frame" || tagName == L"iframe") {
                std::wstring wSrc = ExtractAttribute(currentTag, L"src");
                std::wstring wName = ExtractAttribute(currentTag, L"name");
                if (!wSrc.empty()) {
                    std::string narrowSrc = WideToNarrow(wSrc);
                    std::string resolvedSrc = ResolveUrl(url, narrowSrc);
                    std::wstring frameLabel = (tagName == L"frame") ? L"[フレーム: " : L"[iframe: ";
                    frameLabel += wName.empty() ? wSrc : wName;
                    frameLabel += L"]";
                    if (!stripped.empty() && stripped.back() != L'\n') stripped += L"\n";
                    int start = (int)stripped.length();
                    stripped += frameLabel;
                    links.push_back({ (UINT32)start, (UINT32)frameLabel.length(), resolvedSrc });
                    stripped += L"\n";
                }
            }
            else if (tagName == L"hr" || tagName == L"hr/") {
                if (!inScriptOrStyle && !inTitle && !inButton) {
                    if (!stripped.empty() && stripped.back() != L'\n') stripped += L"\n";
                    hrs.push_back({ (UINT32)stripped.length() });
                    stripped += L" \n";
                }
            }
            else if (tagName == L"br" || tagName == L"br/" || tagName == L"p" || tagName == L"/p" || tagName == L"div" || tagName == L"/div" || tagName == L"tr" || tagName == L"/tr" || tagName == L"li" || (tagName.length() == 2 && tagName[0] == L'h' && tagName[1] >= L'1' && tagName[1] <= L'6') || (tagName.length() == 3 && tagName[0] == L'/' && tagName[1] == L'h' && tagName[2] >= L'1' && tagName[2] <= L'6')) {
                if (!inScriptOrStyle && !inTitle && !inButton) {
                    if (!stripped.empty() && stripped.back() != L'\n') stripped += L"\n";
                }
            }
        }
        else if (inTag) { currentTag += c; }
        else if (c == L'&') {
            size_t semi = wstr.find(L';', i);
            if (semi != std::wstring::npos && (semi - i) <= 10) {
                std::wstring ent = wstr.substr(i, semi - i + 1);
                wchar_t decoded = 0;
                if (ent == L"&nbsp;") decoded = 0x00A0;
                else if (ent == L"&lt;") decoded = L'<';
                else if (ent == L"&gt;") decoded = L'>';
                else if (ent == L"&amp;") decoded = L'&';
                else if (ent == L"&quot;") decoded = L'"';
                else if (ent == L"&apos;") decoded = L'\'';
                else if (ent == L"&copy;") decoded = 0x00A9;
                else if (ent.length() > 3 && ent[1] == L'#') {
                    bool isHex = (ent[2] == L'x' || ent[2] == L'X');
                    size_t startIdx = isHex ? 3 : 2;
                    std::wstring numStr = ent.substr(startIdx, ent.length() - startIdx - 1);
                    wchar_t* endPtr = nullptr;
                    unsigned long val = wcstoul(numStr.c_str(), &endPtr, isHex ? 16 : 10);
                    if (endPtr != numStr.c_str() && val > 0 && val <= 0xFFFF) {
                        decoded = (wchar_t)val;
                    }
                }
                if (decoded != 0) { AppendTextChar(decoded); i = semi; continue; }
            }
            AppendTextChar(c);
        }
        else { AppendTextChar(c); }
    }
    pageTitle.erase(0, pageTitle.find_first_not_of(L" \t\r\n"));
    pageTitle.erase(pageTitle.find_last_not_of(L" \t\r\n") + 1);
    return { stripped, pageTitle, links, inputs, hrs };
}
void LoadUrlAsync(HWND hwnd, const std::string& url, const std::string& postData, int epoch) {
    std::thread([hwnd, url, postData, epoch]() {
        ParsedPage* page = new ParsedPage(FetchAndStripHTML(url, postData));
        if (!g_AppRunning || epoch != g_LoadEpoch.load()) {
            delete page;
            return;
        }
        PostMessage(hwnd, WM_CONTENT_LOADED, (WPARAM)page, 0);
        }).detach();
}
void TriggerLoad(HWND hwnd, const std::string& url, const std::string& postData = "") {
    int currentEpoch = ++g_LoadEpoch;
    {
        std::lock_guard<std::mutex> lock(g_ContentMutex);
        for (auto& input : g_Inputs) {
            if (input.hwnd) DestroyWindow(input.hwnd);
        }
        g_Inputs.clear();
        g_FocusItems.clear();
        g_Hrs.clear();
        g_WebPageContent = L"Loading...\n" + NarrowToWide(url);
        g_PageTitle = L"Loading...";
    }
    SetWindowText(hwnd, L"Loading... - t2t");
    InvalidateRect(hwnd, NULL, FALSE);
    LoadUrlAsync(hwnd, url, postData, currentEpoch);
}
void NavigateTo(HWND hwnd, const std::string& url, const std::string& postData) {
    if (g_HistoryIndex >= 0 && g_HistoryIndex < (int)g_History.size() - 1) g_History.resize(g_HistoryIndex + 1);
    if (g_History.empty() || g_History.back() != url) {
        g_History.push_back(url); g_HistoryIndex = (int)g_History.size() - 1;
    }
    TriggerLoad(hwnd, url, postData);
}
void GoBack(HWND hwnd) { if (g_HistoryIndex > 0) TriggerLoad(hwnd, g_History[--g_HistoryIndex]); }
void GoForward(HWND hwnd) { if (g_HistoryIndex < (int)g_History.size() - 1) TriggerLoad(hwnd, g_History[++g_HistoryIndex]); }
void ReloadCurrentPage(HWND hwnd) {
    if (g_HistoryIndex >= 0 && g_HistoryIndex < (int)g_History.size()) {
        TriggerLoad(hwnd, g_History[g_HistoryIndex]);
    }
}
void OpenAddressBar(HWND hwnd) {
    if (!g_hSearchEdit) {
        g_hSearchEdit = CreateWindowEx(0, L"EDIT", L"", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            0, 0, 0, 0, hwnd, (HMENU)999, GetModuleHandle(NULL), NULL);
        if (g_hCustomFont) {
            SendMessage(g_hSearchEdit, WM_SETFONT, (WPARAM)g_hCustomFont, TRUE);
        }
        else {
            SendMessage(g_hSearchEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        }
        g_OldSearchEditProc = (WNDPROC)SetWindowLongPtr(g_hSearchEdit, GWLP_WNDPROC, (LONG_PTR)SearchEditSubclassProc);
    }
    g_IsUrlMode = true;
    g_IsSearching = true;
    RECT rc; GetClientRect(hwnd, &rc);
    int editHeight = CUSTOM_FONT_SIZE + 6;
    MoveWindow(g_hSearchEdit, 0, rc.bottom - editHeight, rc.right, editHeight, TRUE);
    std::string currentUrl = "about:home";
    if (g_HistoryIndex >= 0 && g_HistoryIndex < (int)g_History.size()) {
        currentUrl = g_History[g_HistoryIndex];
    }
    std::wstring wCurrentUrl = NarrowToWide(currentUrl);
    SetWindowText(g_hSearchEdit, wCurrentUrl.c_str());
    SendMessage(g_hSearchEdit, EM_SETSEL, 0, -1);
    ShowWindow(g_hSearchEdit, SW_SHOW);
    SetFocus(g_hSearchEdit);
}
void CopyToClipboard(HWND hwnd) {
    if (g_SelectionLength == 0) return;
    std::wstring textToCopy;
    {
        std::lock_guard<std::mutex> lock(g_ContentMutex);
        if (g_SelectionStart + g_SelectionLength <= g_WebPageContent.length()) {
            textToCopy = g_WebPageContent.substr(g_SelectionStart, g_SelectionLength);
        }
    }
    if (!textToCopy.empty() && OpenClipboard(hwnd)) {
        EmptyClipboard();
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (textToCopy.length() + 1) * sizeof(wchar_t));
        if (hMem) {
            memcpy(GlobalLock(hMem), textToCopy.c_str(), (textToCopy.length() + 1) * sizeof(wchar_t));
            GlobalUnlock(hMem);
            SetClipboardData(CF_UNICODETEXT, hMem);
        }
        CloseClipboard();
    }
}
void EnsureResources(HWND hwnd) {
    if (!pRT) {
        RECT rc; GetClientRect(hwnd, &rc);
        pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(rc.right, rc.bottom)), &pRT);
        pRT->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_ALIASED);
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.8f, 0.8f, 0.8f), &pTextBrush);
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.2f, 0.4f, 0.8f, 0.8f), &pSelectionBrush);
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.4f, 0.6f, 1.0f), &pLinkBrush);
        pRT->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.6f, 0.0f), &pFocusBrush);
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.8f, 0.8f, 0.8f), &pHrBrush);
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.1f, 0.1f, 0.1f, 0.9f), &pStatusBarBrush);
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.9f, 0.9f, 0.9f), &pStatusTextBrush);
    }
}
void PositionInputControls(HWND hwnd) {
    if (!pTextLayout) return;
    std::lock_guard<std::mutex> lock(g_ContentMutex);
    for (auto& input : g_Inputs) {
        if (input.hwnd) {
            UINT32 actualCount = 0;
            DWRITE_HIT_TEST_METRICS hitMetrics;
            pTextLayout->HitTestTextRange(input.start, input.length, 0, 0, &hitMetrics, 1, &actualCount);
            if (actualCount > 0) {
                float x = 10.0f + hitMetrics.left;
                float y = 13.0f + hitMetrics.top;
                int cx = (int)hitMetrics.width;
                int cy = (int)hitMetrics.height;
                if (input.type == L"radio" || input.type == L"checkbox") {
                    cx = cy;
                }
                MoveWindow(input.hwnd, (int)x, (int)(y - g_ScrollY), cx, cy, TRUE);
            }
        }
    }
    if (g_hSearchEdit && g_IsSearching) {
        RECT rc; GetClientRect(hwnd, &rc);
        int editHeight = CUSTOM_FONT_SIZE + 6;
        MoveWindow(g_hSearchEdit, 0, rc.bottom - editHeight, rc.right, editHeight, TRUE);
    }
}
void ApplyFocus(HWND hwnd) {
    if (g_FocusItems.empty()) return;
    if (g_FocusIndex < 0) g_FocusIndex = 0;
    if (g_FocusIndex >= (int)g_FocusItems.size()) g_FocusIndex = (int)g_FocusItems.size() - 1;
    auto& item = g_FocusItems[g_FocusIndex];
    if (pTextLayout) {
        UINT32 actualCount = 0;
        DWRITE_HIT_TEST_METRICS hm;
        pTextLayout->HitTestTextRange(item.start, 1, 0, 0, &hm, 1, &actualCount);
        if (actualCount > 0) {
            float targetY = hm.top;
            RECT rc; GetClientRect(hwnd, &rc);
            if (targetY < g_ScrollY) {
                g_ScrollY = targetY - 20.0f;
            }
            else if (targetY + hm.height > g_ScrollY + rc.bottom - 40.0f) {
                g_ScrollY = targetY + hm.height - rc.bottom + 40.0f;
            }
            g_ScrollY = max(0.0f, min(g_ScrollY, g_MaxScrollY));
            SetScrollPos(hwnd, SB_VERT, (int)g_ScrollY, TRUE);
            PositionInputControls(hwnd);
        }
    }
    if (item.type == FocusType::INPUT) {
        HWND hInput = g_Inputs[item.index].hwnd;
        if (hInput) SetFocus(hInput);
    }
    else {
        SetFocus(hwnd);
    }
    InvalidateRect(hwnd, NULL, FALSE);
}
void UpdateLayoutAndScrollbar(HWND hwnd) {
    if (!pDWriteFactory) return;
    EnsureResources(hwnd);
    RECT rc; GetClientRect(hwnd, &rc);
    float width = max(10.0f, (float)(rc.right - 20));
    std::wstring contentCopy;
    std::vector<Hyperlink> linksCopy;
    {
        std::lock_guard<std::mutex> lock(g_ContentMutex);
        contentCopy = g_WebPageContent; linksCopy = g_Links;
    }
    if (pTextLayout) { pTextLayout->Release(); pTextLayout = nullptr; }
    pDWriteFactory->CreateTextLayout(contentCopy.c_str(), (UINT32)contentCopy.length(),
        pTextFormat, width, 1000000.0f, &pTextLayout);
    if (pTextLayout) {
        for (const auto& link : linksCopy) {
            DWRITE_TEXT_RANGE range = { link.start, link.length };
            pTextLayout->SetUnderline(TRUE, range);
            if (pLinkBrush) pTextLayout->SetDrawingEffect(pLinkBrush, range);
        }
        DWRITE_TEXT_METRICS metrics; pTextLayout->GetMetrics(&metrics);
        SCROLLINFO si = { sizeof(si) }; si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin = 0; si.nMax = (int)metrics.height + 40; si.nPage = rc.bottom;
        g_MaxScrollY = max(0.0f, (float)(si.nMax - si.nPage));
        if (g_ScrollY > g_MaxScrollY) g_ScrollY = g_MaxScrollY;
        si.nPos = (int)g_ScrollY; SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
        {
            std::lock_guard<std::mutex> lock(g_ContentMutex);
            for (size_t i = 0; i < g_Inputs.size(); ++i) {
                auto& input = g_Inputs[i];
                if (input.type == L"hidden" || input.hwnd) continue;
                DWORD style = WS_CHILD | WS_VISIBLE;
                LPCWSTR className = L"EDIT";
                if (input.type == L"button" || input.type == L"submit" || input.type == L"reset") {
                    className = L"BUTTON"; style |= BS_PUSHBUTTON | BS_FLAT;
                }
                else if (input.type == L"radio") {
                    className = L"BUTTON"; style |= BS_AUTORADIOBUTTON | BS_FLAT;
                }
                else if (input.type == L"checkbox") {
                    className = L"BUTTON"; style |= BS_AUTOCHECKBOX | BS_FLAT;
                }
                else {
                    style |= WS_BORDER | ES_AUTOHSCROLL;
                }
                std::wstring windowText = input.value;
                if (input.type == L"radio" || input.type == L"checkbox") {
                    windowText = L"";
                }
                input.hwnd = CreateWindowEx(0, className, windowText.c_str(), style,
                    0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)(1000 + i), GetModuleHandle(NULL), NULL);
                if (input.checked && (input.type == L"radio" || input.type == L"checkbox")) {
                    SendMessage(input.hwnd, BM_SETCHECK, BST_CHECKED, 0);
                }
                if (g_hCustomFont) {
                    SendMessage(input.hwnd, WM_SETFONT, (WPARAM)g_hCustomFont, TRUE);
                }
                else {
                    SendMessage(input.hwnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
                }
                WNDPROC oldProc = (WNDPROC)SetWindowLongPtr(input.hwnd, GWLP_WNDPROC, (LONG_PTR)InputSubclassProc);
                SetProp(input.hwnd, L"OldProc", (HANDLE)oldProc);
            }
            g_FocusItems.clear();
            for (size_t i = 0; i < g_Links.size(); ++i) {
                g_FocusItems.push_back({ FocusType::LINK, i, g_Links[i].start });
            }
            for (size_t i = 0; i < g_Inputs.size(); ++i) {
                if (g_Inputs[i].type != L"hidden" && g_Inputs[i].hwnd) {
                    g_FocusItems.push_back({ FocusType::INPUT, i, g_Inputs[i].start });
                }
            }
            std::sort(g_FocusItems.begin(), g_FocusItems.end(), [](const FocusItem& a, const FocusItem& b) {
                return a.start < b.start;
                });
        }
    }
    PositionInputControls(hwnd);
}
void Render(HWND hwnd) {
    EnsureResources(hwnd);
    pRT->BeginDraw();
    pRT->Clear(D2D1::ColorF(0.05f, 0.05f, 0.05f));
    if (pTextLayout) {
        if (g_SelectionLength > 0) {
            UINT32 actualCount = 0;
            pTextLayout->HitTestTextRange(g_SelectionStart, g_SelectionLength, 0, 0, nullptr, 0, &actualCount);
            if (actualCount > 0) {
                std::vector<DWRITE_HIT_TEST_METRICS> hitMetrics(actualCount);
                pTextLayout->HitTestTextRange(g_SelectionStart, g_SelectionLength, 0, 0, hitMetrics.data(), actualCount, &actualCount);
                for (UINT32 i = 0; i < actualCount; ++i) {
                    D2D1_RECT_F rect = D2D1::RectF(
                        10.0f + hitMetrics[i].left, 10.0f - g_ScrollY + hitMetrics[i].top,
                        10.0f + hitMetrics[i].left + hitMetrics[i].width, 10.0f - g_ScrollY + hitMetrics[i].top + hitMetrics[i].height
                    );
                    pRT->FillRectangle(rect, pSelectionBrush);
                }
            }
        }
        pRT->DrawTextLayout(D2D1::Point2F(10.0f, 10.0f - g_ScrollY), pTextLayout, pTextBrush, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
        if (pHrBrush) {
            std::lock_guard<std::mutex> lock(g_ContentMutex);
            for (const auto& hr : g_Hrs) {
                if (hr.textPosition < g_WebPageContent.length()) {
                    UINT32 actualCount = 0;
                    DWRITE_HIT_TEST_METRICS hm;
                    pTextLayout->HitTestTextRange(hr.textPosition, 1, 0, 0, &hm, 1, &actualCount);
                    if (actualCount > 0) {
                        float hrY = 10.0f - g_ScrollY + hm.top + (hm.height / 2.0f);
                        D2D1_RECT_F rect = D2D1::RectF(10.0f, hrY - 2.0f, pRT->GetSize().width - 10.0f, hrY + 2.0f);
                        pRT->FillRectangle(rect, pHrBrush);
                    }
                }
            }
        }
        if (g_FocusIndex >= 0 && g_FocusIndex < g_FocusItems.size() && g_FocusItems[g_FocusIndex].type == FocusType::LINK) {
            auto& link = g_Links[g_FocusItems[g_FocusIndex].index];
            UINT32 actualCount = 0;
            pTextLayout->HitTestTextRange(link.start, link.length, 0, 0, nullptr, 0, &actualCount);
            if (actualCount > 0) {
                std::vector<DWRITE_HIT_TEST_METRICS> hitMetrics(actualCount);
                pTextLayout->HitTestTextRange(link.start, link.length, 0, 0, hitMetrics.data(), actualCount, &actualCount);
                for (UINT32 i = 0; i < actualCount; ++i) {
                    D2D1_RECT_F rect = D2D1::RectF(
                        10.0f + hitMetrics[i].left - 2.0f, 10.0f - g_ScrollY + hitMetrics[i].top - 2.0f,
                        10.0f + hitMetrics[i].left + hitMetrics[i].width + 2.0f, 10.0f - g_ScrollY + hitMetrics[i].top + hitMetrics[i].height + 2.0f
                    );
                    pRT->DrawRectangle(rect, pFocusBrush, 2.0f);
                }
            }
        }
    }
    std::string displayUrl = "";
    if (g_IsHoveringLink && !g_HoveredUrl.empty()) {
        displayUrl = g_HoveredUrl;
    }
    else if (g_FocusIndex >= 0 && g_FocusIndex < (int)g_FocusItems.size() && g_FocusItems[g_FocusIndex].type == FocusType::LINK) {
        displayUrl = g_Links[g_FocusItems[g_FocusIndex].index].url;
    }
    if (!displayUrl.empty()) {
        std::wstring wUrl = NarrowToWide(displayUrl);
        IDWriteTextLayout* pStatusLayout = nullptr;
        pDWriteFactory->CreateTextLayout(wUrl.c_str(), (UINT32)wUrl.length(),
            pTextFormat, pRT->GetSize().width - 10.0f, 100.0f, &pStatusLayout);
        if (pStatusLayout) {
            pStatusLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            DWRITE_TEXT_METRICS sm;
            pStatusLayout->GetMetrics(&sm);
            float barHeight = sm.height + 4.0f;
            float yPos = pRT->GetSize().height - barHeight;
            D2D1_RECT_F bgRect = D2D1::RectF(0.0f, yPos, pRT->GetSize().width, pRT->GetSize().height);
            if (pStatusBarBrush) pRT->FillRectangle(bgRect, pStatusBarBrush);
            if (pStatusTextBrush) pRT->DrawTextLayout(D2D1::Point2F(5.0f, yPos + 2.0f), pStatusLayout, pStatusTextBrush);
            pStatusLayout->Release();
        }
    }
    pRT->EndDraw();
}
UINT32 GetTextPositionFromMouse(int mouseX, int mouseY, BOOL* outIsInside = nullptr) {
    if (!pTextLayout) {
        if (outIsInside) *outIsInside = FALSE;
        return UINT32_MAX;
    }
    float x = (float)mouseX - 10.0f;
    float y = (float)mouseY - 10.0f + g_ScrollY;
    BOOL isTrailingHit, isInside;
    DWRITE_HIT_TEST_METRICS metrics;
    pTextLayout->HitTestPoint(x, y, &isTrailingHit, &isInside, &metrics);
    if (outIsInside) *outIsInside = isInside;
    return metrics.textPosition + (isTrailingHit ? 1 : 0);
}
void UpdateScrollPos(HWND hwnd, float newY) {
    g_ScrollY = max(0.0f, min(newY, g_MaxScrollY));
    SetScrollPos(hwnd, SB_VERT, (int)g_ScrollY, TRUE);
    PositionInputControls(hwnd);
    InvalidateRect(hwnd, NULL, FALSE);
}
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CONTENT_LOADED:
    {
        ParsedPage* page = (ParsedPage*)wp;
        std::wstring windowTitle;
        {
            std::lock_guard<std::mutex> lock(g_ContentMutex);
            if (page) {
                for (auto& input : g_Inputs) {
                    if (input.hwnd) DestroyWindow(input.hwnd);
                }
                g_WebPageContent = page->text;
                g_PageTitle = page->title;
                g_Links = page->links;
                g_Inputs = page->inputs;
                g_Hrs = page->hrs;
                delete page;
            }
            windowTitle = g_PageTitle.empty() ? L"t2t" : (g_PageTitle + L" - t2t");
            g_IsHoveringLink = false;
        }
        SetWindowText(hwnd, windowTitle.c_str());
        g_ScrollY = 0.0f;
        g_SelectionAnchor = UINT32_MAX;
        g_SelectionStart = 0;
        g_SelectionLength = 0;
        g_FocusIndex = -1;
        if (g_IsSearching) {
            ShowWindow(g_hSearchEdit, SW_HIDE);
            g_IsSearching = false;
        }
        UpdateLayoutAndScrollbar(hwnd);
        if (!g_FocusItems.empty()) {
            g_FocusIndex = 0;
            ApplyFocus(hwnd);
        }
        InvalidateRect(hwnd, NULL, FALSE);
    }
    return 0;
    case WM_PAINT:
        Render(hwnd); ValidateRect(hwnd, NULL); return 0;
    case WM_SIZE:
        if (pRT) {
            pRT->Resize(D2D1::SizeU(LOWORD(lp), HIWORD(lp)));
            UpdateLayoutAndScrollbar(hwnd);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    case WM_TIMER:
        if (wp == TIMER_AUTOSCROLL && g_IsSelecting) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hwnd, &pt);
            RECT rc;
            GetClientRect(hwnd, &rc);
            bool needsScroll = false;
            float scrollAmount = 0.0f;
            if (pt.y < 0) {
                scrollAmount = (float)pt.y;
                needsScroll = true;
            }
            else if (pt.y > rc.bottom) {
                scrollAmount = (float)(pt.y - rc.bottom);
                needsScroll = true;
            }
            if (needsScroll) {
                scrollAmount = max(-50.0f, min(scrollAmount, 50.0f));
                UpdateScrollPos(hwnd, g_ScrollY + scrollAmount);
                UINT32 currentPos = GetTextPositionFromMouse(pt.x, pt.y);
                if (currentPos != UINT32_MAX && g_SelectionAnchor != UINT32_MAX) {
                    if (currentPos < g_SelectionAnchor) {
                        g_SelectionStart = currentPos;
                        g_SelectionLength = g_SelectionAnchor - currentPos;
                    }
                    else {
                        g_SelectionStart = g_SelectionAnchor;
                        g_SelectionLength = currentPos - g_SelectionAnchor;
                    }
                    InvalidateRect(hwnd, NULL, FALSE);
                }
            }
        }
        return 0;
    case WM_EXECUTE_SEARCH:
    {
        if (g_LastSearchQuery.empty()) {
            PostMessage(hwnd, WM_CANCEL_SEARCH, 0, 0);
            return 0;
        }
        bool forward = (wp != 0);
        std::wstring lowerContent, lowerQuery;
        {
            std::lock_guard<std::mutex> lock(g_ContentMutex);
            lowerContent = g_WebPageContent;
        }
        std::transform(lowerContent.begin(), lowerContent.end(), lowerContent.begin(), ::towlower);
        lowerQuery = g_LastSearchQuery;
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::towlower);
        size_t startPos = g_SelectionStart;
        if (g_SelectionLength > 0) {
            startPos += (forward ? 1 : -1);
        }
        size_t foundPos = std::wstring::npos;
        if (forward) {
            foundPos = lowerContent.find(lowerQuery, startPos);
            if (foundPos == std::wstring::npos) foundPos = lowerContent.find(lowerQuery, 0);
        }
        else {
            if (startPos > 0) foundPos = lowerContent.rfind(lowerQuery, startPos);
            if (foundPos == std::wstring::npos) foundPos = lowerContent.rfind(lowerQuery, lowerContent.length());
        }
        if (foundPos != std::wstring::npos) {
            g_SelectionStart = (UINT32)foundPos;
            g_SelectionLength = (UINT32)lowerQuery.length();
            if (pTextLayout) {
                UINT32 actualCount = 0;
                DWRITE_HIT_TEST_METRICS hm;
                pTextLayout->HitTestTextRange(g_SelectionStart, g_SelectionLength, 0, 0, &hm, 1, &actualCount);
                if (actualCount > 0) {
                    RECT rc; GetClientRect(hwnd, &rc);
                    float targetY = hm.top;
                    if (targetY < g_ScrollY || targetY + hm.height > g_ScrollY + rc.bottom - 40.0f) {
                        g_ScrollY = targetY - (rc.bottom / 2.0f);
                        g_ScrollY = max(0.0f, min(g_ScrollY, g_MaxScrollY));
                        SetScrollPos(hwnd, SB_VERT, (int)g_ScrollY, TRUE);
                        PositionInputControls(hwnd);
                    }
                }
            }
        }
        else {
            MessageBeep(MB_OK);
        }
        PostMessage(hwnd, WM_CANCEL_SEARCH, 0, 0);
        InvalidateRect(hwnd, NULL, FALSE);
    }
    return 0;
    case WM_EXECUTE_URL:
    {
        wchar_t buf[4096] = { 0 };
        GetWindowText(g_hSearchEdit, buf, 4096);
        std::wstring wUrl = buf;
        std::string narrowUrl = WideToNarrow(wUrl);
        PostMessage(hwnd, WM_CANCEL_SEARCH, 0, 0);
        if (!narrowUrl.empty()) {
            if (narrowUrl.find("://") == std::string::npos && narrowUrl != "about:home" && narrowUrl != "about:help") {
                narrowUrl = "https://" + narrowUrl;
            }
            NavigateTo(hwnd, narrowUrl);
        }
    }
    return 0;
    case WM_CANCEL_SEARCH:
        if (g_IsSearching) {
            ShowWindow(g_hSearchEdit, SW_HIDE);
            SetFocus(hwnd);
            g_IsSearching = false;
        }
        return 0;
    case WM_DO_TAB:
    {
        bool shift = (wp != 0);
        if (g_FocusItems.empty()) return 0;
        if (g_FocusIndex == -1) {
            if (shift) {
                g_FocusIndex = (int)g_FocusItems.size() - 1;
                if (g_SelectionAnchor != UINT32_MAX) {
                    for (int i = (int)g_FocusItems.size() - 1; i >= 0; --i) {
                        if (g_FocusItems[i].start <= g_SelectionAnchor) {
                            g_FocusIndex = i;
                            break;
                        }
                    }
                }
            }
            else {
                g_FocusIndex = 0;
                if (g_SelectionAnchor != UINT32_MAX) {
                    for (int i = 0; i < (int)g_FocusItems.size(); ++i) {
                        if (g_FocusItems[i].start > g_SelectionAnchor) {
                            g_FocusIndex = i;
                            break;
                        }
                    }
                }
            }
        }
        else {
            if (shift) {
                g_FocusIndex--;
                if (g_FocusIndex < 0) g_FocusIndex = (int)g_FocusItems.size() - 1;
            }
            else {
                g_FocusIndex++;
                if (g_FocusIndex >= (int)g_FocusItems.size()) g_FocusIndex = 0;
            }
        }
        ApplyFocus(hwnd);
    }
    return 0;
    case WM_SYNC_FOCUS:
    {
        HWND hFocused = (HWND)wp;
        for (size_t i = 0; i < g_FocusItems.size(); ++i) {
            if (g_FocusItems[i].type == FocusType::INPUT && g_Inputs[g_FocusItems[i].index].hwnd == hFocused) {
                g_FocusIndex = (int)i;
                InvalidateRect(hwnd, NULL, FALSE);
                break;
            }
        }
    }
    return 0;
    case WM_SUBMIT_FORM:
    {
        if (g_Inputs.empty()) return 0;
        std::string targetAction = "";
        std::string targetMethod = "GET";
        std::string queryString = "";
        CURL* curl = curl_easy_init();
        int submitFormIndex = -1;
        if (g_FocusIndex >= 0 && g_FocusIndex < (int)g_FocusItems.size() && g_FocusItems[g_FocusIndex].type == FocusType::INPUT) {
            submitFormIndex = (int)g_FocusItems[g_FocusIndex].index;
        }
        if (submitFormIndex != -1) {
            targetAction = g_Inputs[submitFormIndex].action;
            targetMethod = g_Inputs[submitFormIndex].method;
        }
        for (const auto& input : g_Inputs) {
            if (targetAction.empty() && !input.action.empty()) {
                targetAction = input.action;
                targetMethod = input.method;
            }
            if (!input.action.empty() && input.action != targetAction && targetAction != "") continue;
            if (input.name.empty()) continue;
            if (input.type == L"button" || input.type == L"submit" || input.type == L"reset") continue;
            std::wstring val = input.value;
            if (input.hwnd && input.type != L"hidden") {
                if (input.type == L"radio" || input.type == L"checkbox") {
                    if (SendMessage(input.hwnd, BM_GETCHECK, 0, 0) != BST_CHECKED) {
                        continue;
                    }
                    if (val.empty()) val = L"on";
                }
                else {
                    wchar_t buf[4096] = { 0 };
                    GetWindowText(input.hwnd, buf, 4096);
                    val = buf;
                }
            }
            std::string n8 = WideToNarrow(input.name);
            std::string v8 = WideToNarrow(val);
            char* escName = curl_easy_escape(curl, n8.c_str(), (int)n8.length());
            char* escVal = curl_easy_escape(curl, v8.c_str(), (int)v8.length());
            if (!queryString.empty()) queryString += "&";
            queryString += (escName ? escName : "");
            queryString += "=";
            queryString += (escVal ? escVal : "");
            if (escName) curl_free(escName);
            if (escVal) curl_free(escVal);
        }
        if (curl) curl_easy_cleanup(curl);
        if (targetAction.empty() && !g_History.empty()) targetAction = g_History.back();
        std::string finalUrl = targetAction;
        if (targetMethod == "POST") {
            NavigateTo(hwnd, finalUrl, queryString);
        }
        else {
            if (!queryString.empty()) {
                if (finalUrl.find('?') != std::string::npos) finalUrl += "&" + queryString;
                else finalUrl += "?" + queryString;
            }
            NavigateTo(hwnd, finalUrl);
        }
    }
    return 0;
    case WM_COMMAND:
        if (HIWORD(wp) == BN_CLICKED && LOWORD(wp) >= 1000) {
            int index = LOWORD(wp) - 1000;
            bool doSubmit = false;
            {
                std::lock_guard<std::mutex> lock(g_ContentMutex);
                if (index >= 0 && index < (int)g_Inputs.size()) {
                    std::wstring t = g_Inputs[index].type;
                    if (t == L"submit" || t == L"button") {
                        doSubmit = true;
                    }
                    else if (t == L"radio") {
                        std::wstring rName = g_Inputs[index].name;
                        for (size_t i = 0; i < g_Inputs.size(); ++i) {
                            if (i != index && g_Inputs[i].type == L"radio" && g_Inputs[i].name == rName) {
                                SendMessage(g_Inputs[i].hwnd, BM_SETCHECK, BST_UNCHECKED, 0);
                            }
                        }
                    }
                }
            }
            if (doSubmit) {
                PostMessage(hwnd, WM_SUBMIT_FORM, 0, 0);
            }
            return 0;
        }
        return DefWindowProc(hwnd, msg, wp, lp);
    case WM_SETCURSOR:
        if (LOWORD(lp) == HTCLIENT) {
            SetCursor(LoadCursor(NULL, g_IsHoveringLink ? IDC_HAND : IDC_IBEAM));
            return TRUE;
        }
        break;
    case WM_LBUTTONDBLCLK:
    {
        UINT32 currentPos = GetTextPositionFromMouse(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
        if (currentPos != UINT32_MAX) {
            std::lock_guard<std::mutex> lock(g_ContentMutex);
            if (currentPos < g_WebPageContent.length()) {
                auto get_char_type = [](wchar_t c) {
                    if (iswspace(c) || c == L'　') return 0;
                    if (wcschr(L".,:;!?()[]{}<>\"'/\\|`~@#$%^&*-+=、。，．（）「」『』【】", c) != nullptr) return 1;
                    return 2;
                    };
                int type = get_char_type(g_WebPageContent[currentPos]);
                if (type != 0) {
                    UINT32 start = currentPos;
                    while (start > 0 && get_char_type(g_WebPageContent[start - 1]) == type) {
                        start--;
                    }
                    UINT32 end = currentPos;
                    while (end < g_WebPageContent.length() && get_char_type(g_WebPageContent[end]) == type) {
                        end++;
                    }
                    g_SelectionStart = start;
                    g_SelectionLength = end - start;
                    g_SelectionAnchor = start;
                    g_IsSelecting = true;
                    SetCapture(hwnd);
                    SetTimer(hwnd, TIMER_AUTOSCROLL, 30, NULL);
                    InvalidateRect(hwnd, NULL, FALSE);
                }
            }
        }
        return 0;
    }
    case WM_LBUTTONDOWN:
        SetFocus(hwnd);
        SetCapture(hwnd);
        g_IsSelecting = true;
        SetTimer(hwnd, TIMER_AUTOSCROLL, 30, NULL);
        g_SelectionAnchor = GetTextPositionFromMouse(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
        g_SelectionLength = 0;
        g_FocusIndex = -1;
        if (g_SelectionAnchor != UINT32_MAX) {
            g_SelectionStart = g_SelectionAnchor;
        }
        else {
            g_SelectionStart = 0;
            g_SelectionAnchor = 0;
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    case WM_MOUSEMOVE:
    {
        BOOL isInside = FALSE;
        UINT32 currentPos = GetTextPositionFromMouse(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), &isInside);
        bool foundLink = false;
        std::string targetUrl = "";
        if (isInside && currentPos != UINT32_MAX && !g_IsSelecting) {
            std::lock_guard<std::mutex> lock(g_ContentMutex);
            for (const auto& link : g_Links) {
                if (currentPos >= link.start && currentPos < link.start + link.length) {
                    foundLink = true; targetUrl = link.url; break;
                }
            }
        }
        if (g_IsHoveringLink != foundLink || (foundLink && g_HoveredUrl != targetUrl)) {
            g_IsHoveringLink = foundLink;
            g_HoveredUrl = targetUrl;
            SetCursor(LoadCursor(NULL, g_IsHoveringLink ? IDC_HAND : IDC_IBEAM));
            InvalidateRect(hwnd, NULL, FALSE);
        }
        if (g_IsSelecting && currentPos != UINT32_MAX && g_SelectionAnchor != UINT32_MAX) {
            if (currentPos < g_SelectionAnchor) {
                g_SelectionStart = currentPos; g_SelectionLength = g_SelectionAnchor - currentPos;
            }
            else {
                g_SelectionStart = g_SelectionAnchor; g_SelectionLength = currentPos - g_SelectionAnchor;
            }
            InvalidateRect(hwnd, NULL, FALSE);
        }
    }
    return 0;
    case WM_LBUTTONUP:
        if (g_IsSelecting) {
            KillTimer(hwnd, TIMER_AUTOSCROLL);
            ReleaseCapture();
            g_IsSelecting = false;
            if (g_SelectionLength == 0 && g_IsHoveringLink && !g_HoveredUrl.empty()) {
                NavigateTo(hwnd, g_HoveredUrl);
            }
        }
        return 0;
    case WM_CHAR:
        if (wp == '/') {
            if (!g_hSearchEdit) {
                g_hSearchEdit = CreateWindowEx(0, L"EDIT", L"", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                    0, 0, 0, 0, hwnd, (HMENU)999, GetModuleHandle(NULL), NULL);
                if (g_hCustomFont) SendMessage(g_hSearchEdit, WM_SETFONT, (WPARAM)g_hCustomFont, TRUE);
                else SendMessage(g_hSearchEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
                g_OldSearchEditProc = (WNDPROC)SetWindowLongPtr(g_hSearchEdit, GWLP_WNDPROC, (LONG_PTR)SearchEditSubclassProc);
            }
            g_IsUrlMode = false;
            RECT rc; GetClientRect(hwnd, &rc);
            int editHeight = CUSTOM_FONT_SIZE + 6;
            MoveWindow(g_hSearchEdit, 0, rc.bottom - editHeight, rc.right, editHeight, TRUE);
            SetWindowText(g_hSearchEdit, g_LastSearchQuery.c_str());
            SendMessage(g_hSearchEdit, EM_SETSEL, 0, -1);
            ShowWindow(g_hSearchEdit, SW_SHOW);
            SetFocus(g_hSearchEdit);
            g_IsSearching = true;
            return 0;
        }
        if (wp == 'n') {
            PostMessage(hwnd, WM_EXECUTE_SEARCH, 1, 0);
            return 0;
        }
        if (wp == 'N') {
            PostMessage(hwnd, WM_EXECUTE_SEARCH, 0, 0);
            return 0;
        }
        break;
    case WM_SYSKEYDOWN:
        if (HandleGlobalShortcuts(hwnd, msg, wp)) return 0;
        return DefWindowProc(hwnd, msg, wp, lp);
    case WM_KEYDOWN:
        if (HandleGlobalShortcuts(hwnd, msg, wp)) return 0;
        if (wp == VK_ESCAPE) {
            bool needsRepaint = false;
            if (g_SelectionLength > 0) {
                g_SelectionLength = 0;
                g_SelectionStart = 0;
                g_SelectionAnchor = UINT32_MAX;
                needsRepaint = true;
            }
            if (g_IsSearching) {
                PostMessage(hwnd, WM_CANCEL_SEARCH, 0, 0);
            }
            if (needsRepaint) {
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }
        if (wp == 'A' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            std::lock_guard<std::mutex> lock(g_ContentMutex);
            g_SelectionStart = 0; g_SelectionLength = (UINT32)g_WebPageContent.length();
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        if (wp == 'C' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            CopyToClipboard(hwnd);
            return 0;
        }
        if (wp == VK_TAB) {
            PostMessage(hwnd, WM_DO_TAB, (GetKeyState(VK_SHIFT) & 0x8000) ? 1 : 0, 0);
            return 0;
        }
        if (wp == VK_RETURN) {
            if (g_FocusIndex >= 0 && g_FocusIndex < g_FocusItems.size()) {
                if (g_FocusItems[g_FocusIndex].type == FocusType::LINK) {
                    NavigateTo(hwnd, g_Links[g_FocusItems[g_FocusIndex].index].url);
                }
            }
            return 0;
        }
        {
            RECT rc; GetClientRect(hwnd, &rc);
            float pageHeight = max(40.0f, (float)rc.bottom - 40.0f);
            switch (wp) {
            case VK_NEXT:  UpdateScrollPos(hwnd, g_ScrollY + pageHeight); return 0;
            case VK_PRIOR: UpdateScrollPos(hwnd, g_ScrollY - pageHeight); return 0;
            case VK_HOME:  UpdateScrollPos(hwnd, 0.0f); return 0;
            case VK_END:   UpdateScrollPos(hwnd, g_MaxScrollY); return 0;
            }
            if (!g_IsSearching) {
                switch (wp) {
                case VK_DOWN: case 'J': UpdateScrollPos(hwnd, g_ScrollY + 40.0f); break;
                case VK_UP:   case 'K': UpdateScrollPos(hwnd, g_ScrollY - 40.0f); break;
                case VK_SPACE:          UpdateScrollPos(hwnd, g_ScrollY + pageHeight); break;
                case 'B':               UpdateScrollPos(hwnd, g_ScrollY - pageHeight); break;
                case 'Q':               PostQuitMessage(0); break;
                }
            }
        }
        return 0;
    case WM_VSCROLL:
    {
        SCROLLINFO si = { sizeof(si), SIF_ALL }; GetScrollInfo(hwnd, SB_VERT, &si);
        int pos = si.nPos;
        switch (LOWORD(wp)) {
        case SB_TOP: pos = si.nMin; break; case SB_BOTTOM: pos = si.nMax; break;
        case SB_LINEUP: pos -= 40; break; case SB_LINEDOWN: pos += 40; break;
        case SB_PAGEUP: pos -= si.nPage; break; case SB_PAGEDOWN: pos += si.nPage; break;
        case SB_THUMBTRACK: pos = si.nTrackPos; break;
        }
        UpdateScrollPos(hwnd, (float)pos);
    }
    return 0;
    case WM_MOUSEWHEEL:
        UpdateScrollPos(hwnd, g_ScrollY - (GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA) * 50.0f);
        return 0;
    case WM_DESTROY:
        g_AppRunning = false;
        PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}
int WINAPI wWinMain(HINSTANCE hi, HINSTANCE, PWSTR pCmdLine, int n) {
    curl_global_init(CURL_GLOBAL_ALL);
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&pDWriteFactory);
    IDWriteFactory5* pDWriteFactory5 = nullptr;
    IDWriteInMemoryFontFileLoader* pMemLoader = nullptr;
    IDWriteFontCollection1* pCustomFontCollection = nullptr;
    if (SUCCEEDED(pDWriteFactory->QueryInterface(__uuidof(IDWriteFactory5), (void**)&pDWriteFactory5))) {
        pDWriteFactory5->CreateInMemoryFontFileLoader(&pMemLoader);
        pDWriteFactory5->RegisterFontFileLoader(pMemLoader);
        HRSRC hRes = FindResource(hi, MAKEINTRESOURCE(101), RT_RCDATA);
        if (hRes) {
            HGLOBAL hMem = LoadResource(hi, hRes);
            void* pFontData = LockResource(hMem);
            DWORD fontSize = SizeofResource(hi, hRes);
            if (pFontData && fontSize > 0) {
                DWORD numFonts = 0;
                g_hGdiFont = AddFontMemResourceEx(pFontData, fontSize, nullptr, &numFonts);
            }
            IDWriteFontFile* pFontFile = nullptr;
            pMemLoader->CreateInMemoryFontFileReference(pDWriteFactory5, pFontData, fontSize, nullptr, &pFontFile);
            IDWriteFontSetBuilder1* pSetBuilder = nullptr;
            pDWriteFactory5->CreateFontSetBuilder(&pSetBuilder);
            pSetBuilder->AddFontFile(pFontFile);
            IDWriteFontSet* pFontSet = nullptr;
            pSetBuilder->CreateFontSet(&pFontSet);
            pDWriteFactory5->CreateFontCollectionFromFontSet(pFontSet, &pCustomFontCollection);
            pFontSet->Release();
            pSetBuilder->Release();
            pFontFile->Release();
        }
    }
    if (pCustomFontCollection) {
        pDWriteFactory->CreateTextFormat(CUSTOM_FONT_FAMILY, pCustomFontCollection, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, (float)CUSTOM_FONT_SIZE, L"ja-jp", &pTextFormat);
        if (pTextFormat) pTextFormat->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, (float)CUSTOM_FONT_SIZE, (float)CUSTOM_FONT_SIZE);
        pCustomFontCollection->Release();
        g_hCustomFont = CreateFontW(-CUSTOM_FONT_SIZE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, CUSTOM_FONT_FAMILY);
    }
    else {
        pDWriteFactory->CreateTextFormat(L"MS Gothic", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 16.0f, L"ja-jp", &pTextFormat);
        if (pTextFormat) pTextFormat->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, 16.0f, 16.0f);
    }
    WNDCLASS wc = { 0 };
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = WndProc; wc.hInstance = hi; wc.lpszClassName = L"t2t";
    wc.hIcon = LoadIcon(hi, MAKEINTRESOURCE(IDI_ICON1));
    wc.hCursor = LoadCursor(NULL, IDC_IBEAM);
    RegisterClass(&wc);
    HWND hwnd = CreateWindow(L"t2t", L"t2t", WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_CLIPCHILDREN, 100, 100, 800, 600, 0, 0, hi, 0);
    ShowWindow(hwnd, n);
    std::wstring cmd = pCmdLine;
    if (cmd.length() >= 2 && cmd.front() == L'"' && cmd.back() == L'"') cmd = cmd.substr(1, cmd.length() - 2);
    if (cmd.empty()) NavigateTo(hwnd, "about:home");
    else NavigateTo(hwnd, WideToNarrow(cmd));
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    if (pTextLayout) pTextLayout->Release();
    if (pTextFormat) pTextFormat->Release();
    if (g_hCustomFont) DeleteObject(g_hCustomFont);
    if (g_hGdiFont) RemoveFontMemResourceEx(g_hGdiFont);
    if (pMemLoader && pDWriteFactory5) {
        pDWriteFactory5->UnregisterFontFileLoader(pMemLoader);
        pMemLoader->Release();
    }
    if (pDWriteFactory5) pDWriteFactory5->Release();
    if (pDWriteFactory) pDWriteFactory->Release();
    if (pTextBrush) pTextBrush->Release();
    if (pSelectionBrush) pSelectionBrush->Release();
    if (pLinkBrush) pLinkBrush->Release();
    if (pFocusBrush) pFocusBrush->Release();
    if (pHrBrush) pHrBrush->Release();
    if (pStatusBarBrush) pStatusBarBrush->Release();
    if (pStatusTextBrush) pStatusTextBrush->Release();
    if (pRT) pRT->Release();
    if (pD2DFactory) pD2DFactory->Release();
    curl_global_cleanup();
    return 0;
}