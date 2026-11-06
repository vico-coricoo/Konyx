#include "KonyxUI.h"

namespace Konyx {
namespace UI {

HWND CreerLogoKonyx(HWND hwndParent, const std::wstring& cheminLogo,
                     int x, int y, int taille) {
    if (cheminLogo.empty()) return nullptr;

    // .ico gere nativement la transparence (recommande pour un logo) ;
    // .bmp fonctionne aussi mais sans transparence.
    bool estIcone = cheminLogo.size() > 4
        && _wcsicmp(cheminLogo.c_str() + cheminLogo.size() - 4, L".ico") == 0;

    HANDLE hImage = LoadImage(
        nullptr, cheminLogo.c_str(),
        estIcone ? IMAGE_ICON : IMAGE_BITMAP,
        taille, taille,
        LR_LOADFROMFILE | (estIcone ? 0 : LR_CREATEDIBSECTION));

    if (!hImage) return nullptr;

    HWND hwndLogo = CreateWindowEx(
        0, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | (estIcone ? SS_ICON : SS_BITMAP),
        x, y, taille, taille,
        hwndParent, nullptr, GetModuleHandle(nullptr), nullptr);

    if (hwndLogo) {
        SendMessage(hwndLogo, estIcone ? STM_SETICON : STM_SETIMAGE,
                    estIcone ? (WPARAM)hImage : IMAGE_BITMAP,
                    estIcone ? 0 : (LPARAM)hImage);
    }

    return hwndLogo;
}

} // namespace UI
} // namespace Konyx
