#include "GeoCalculV0Graphique.h"
#include <windowsx.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace Konyx {

static std::wstring VersWide(const std::string& s) {
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), len);
    if (!w.empty() && w.back() == L'\0') w.pop_back();
    return w;
}

bool GeoCalculV0Graphique::Afficher(HWND hwndParent, const std::string& matriculeStation,
                                     double xStation, double yStation,
                                     std::vector<RefGraphique>& references) {
    GeoCalculV0Graphique dlg(matriculeStation, xStation, yStation, references);

    WNDCLASSEX wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_DBLCLKS; // necessaire pour WM_LBUTTONDBLCLK / WM_RBUTTONDBLCLK
    wc.lpfnWndProc = FenetreProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = ClasseFenetre;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    RegisterClassEx(&wc); // echec silencieux si deja enregistree

    RECT rcParent{};
    GetWindowRect(hwndParent, &rcParent);
    int largeur = 600, hauteur = 520;
    int x = rcParent.left + (rcParent.right - rcParent.left - largeur) / 2;
    int y = rcParent.top + (rcParent.bottom - rcParent.top - hauteur) / 2;

    HWND hwnd = CreateWindowEx(
        WS_EX_DLGMODALFRAME, ClasseFenetre, L"KONYX - Visualisation partielle de la polygonation",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
        x, y, largeur, hauteur,
        hwndParent, nullptr, GetModuleHandle(nullptr), &dlg);

    if (!hwnd) return false;

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    //EnableWindow(hwndParent, FALSE);

    MSG msg;
    while (IsWindow(hwnd) && GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    //EnableWindow(hwndParent, TRUE);
    SetForegroundWindow(hwndParent);

    if (dlg.m_valide) {
        references = dlg.m_references;
        return true;
    }
    return false;
}

GeoCalculV0Graphique::GeoCalculV0Graphique(const std::string& matriculeStation, double xStation, double yStation,
                                            const std::vector<RefGraphique>& references)
    : m_matriculeStation(matriculeStation), m_xStation(xStation), m_yStation(yStation), m_references(references) {
    m_hFont = CreateFont(-22, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    // Police des libelles sur le dessin
    m_hFontDessin = CreateFont(-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier New");
}

GeoCalculV0Graphique::~GeoCalculV0Graphique() {
    if (m_hFont) DeleteObject(m_hFont);
    if (m_hFontDessin) DeleteObject(m_hFontDessin);
}

LRESULT CALLBACK GeoCalculV0Graphique::FenetreProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    GeoCalculV0Graphique* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        self = static_cast<GeoCalculV0Graphique*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hwnd = hwnd;
    } else {
        self = reinterpret_cast<GeoCalculV0Graphique*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    if (self) return self->GererMessage(hwnd, msg, wParam, lParam);
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT GeoCalculV0Graphique::GererMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreerControles(hwnd);
        return 0;

    case WM_SIZE: {
        RECT rc;
        GetClientRect(hwnd, &rc);
        int largeur = rc.right - rc.left;
        int hauteur = rc.bottom - rc.top;

        SetWindowPos(m_hLabelCoord, nullptr, 10, hauteur - HAUTEUR_BANDE_BASSE - 5, 300, 40,
                     SWP_NOZORDER);
        SetWindowPos(m_hBtnOK, nullptr, largeur - 240, hauteur - HAUTEUR_BANDE_BASSE - 5, 110, 40,
                     SWP_NOZORDER);
        SetWindowPos(m_hBtnAnnuler, nullptr, largeur - 120, hauteur - HAUTEUR_BANDE_BASSE - 5, 110, 40,
                     SWP_NOZORDER);

        CalculerTransformation(hwnd);
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);
        int largeur = rc.right - rc.left;
        int hauteur = rc.bottom - rc.top;

        // Double buffer pour eviter le scintillement pendant le dessin.
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, largeur, hauteur);
        HBITMAP hbmAncien = static_cast<HBITMAP>(SelectObject(hdcMem, hbmMem));

        FillRect(hdcMem, &rc, reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
        Dessiner(hdcMem, largeur, hauteur);

        BitBlt(hdc, 0, 0, largeur, hauteur, hdcMem, 0, 0, SRCCOPY);

        SelectObject(hdcMem, hbmAncien);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int sx = GET_X_LPARAM(lParam);
        int sy = GET_Y_LPARAM(lParam);
        int idx = TrouverPointSousCurseur(sx, sy);
        if (idx >= 0) {
            m_references[idx].selectionne = !m_references[idx].selectionne;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONDBLCLK: {
        for (auto& r : m_references) r.selectionne = true;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_RBUTTONDBLCLK: {
        for (auto& r : m_references) r.selectionne = false;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_MOUSEMOVE: {
        int sx = GET_X_LPARAM(lParam);
        int sy = GET_Y_LPARAM(lParam);
        double x, y;
        VersMonde(sx, sy, x, y);
        std::wostringstream oss;
        oss << L"  " << std::fixed << std::setprecision(3) << x << L" ; " << y;
        SetWindowText(m_hLabelCoord, oss.str().c_str());
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == ID_BTN_OK) {
            m_valide = true;
            DestroyWindow(hwnd);
        } else if (id == ID_BTN_ANNULER) {
            m_valide = false;
            DestroyWindow(hwnd);
        }
        return 0;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        // Pas de PostQuitMessage : boucle GetMessage locale (voir Afficher()).
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void GeoCalculV0Graphique::CreerControles(HWND hwnd) {
    m_hLabelCoord = CreateWindow(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_NOTIFY,
                             10, 10, 300, 40, hwnd,
                              reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_LABEL_COORD)), nullptr, nullptr);
    SendMessage(m_hLabelCoord, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFont), TRUE);

    m_hBtnOK = CreateWindow(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE,
                             0, 0, 120, 40, hwnd,
                             reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_BTN_OK)), nullptr, nullptr);
    SendMessage(m_hBtnOK, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFont), TRUE);

    m_hBtnAnnuler = CreateWindow(L"BUTTON", L"Annuler", WS_CHILD | WS_VISIBLE,
                                  0, 0, 120, 40, hwnd,
                                  reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_BTN_ANNULER)), nullptr, nullptr);
    SendMessage(m_hBtnAnnuler, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFont), TRUE);
}

void GeoCalculV0Graphique::CalculerTransformation(HWND hwnd) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    m_largeurDessin = rc.right - rc.left;
    m_hauteurDessin = (rc.bottom - rc.top) - HAUTEUR_BANDE_BASSE;
    if (m_hauteurDessin < 10) m_hauteurDessin = 10;

    double xMin = m_xStation, xMax = m_xStation;
    double yMin = m_yStation, yMax = m_yStation;
    for (const auto& r : m_references) {
        xMin = (std::min)(xMin, r.x); xMax = (std::max)(xMax, r.x);
        yMin = (std::min)(yMin, r.y); yMax = (std::max)(yMax, r.y);
    }

    double largeurMonde = (std::max)(xMax - xMin, 0.001);
    double hauteurMonde = (std::max)(yMax - yMin, 0.001);

    double echelleX = (m_largeurDessin - 2.0 * MARGE) / largeurMonde;
    double echelleY = (m_hauteurDessin - 2.0 * MARGE) / hauteurMonde;
    m_echelle = (std::min)(echelleX, echelleY);
    if (m_echelle <= 0.0) m_echelle = 1.0;

    // Centre la scene dans la zone de dessin.
    double largeurAffichee = largeurMonde * m_echelle;
    double hauteurAffichee = hauteurMonde * m_echelle;
    double margeExtraX = (m_largeurDessin - largeurAffichee) / 2.0;
    double margeExtraY = (m_hauteurDessin - hauteurAffichee) / 2.0;

    m_xMin = xMin - margeExtraX / m_echelle;
    m_yMin = yMin - margeExtraY / m_echelle;
}

POINT GeoCalculV0Graphique::VersEcran(double x, double y) const {
    POINT p;
    p.x = static_cast<LONG>((x - m_xMin) * m_echelle);
    p.y = static_cast<LONG>(m_hauteurDessin - (y - m_yMin) * m_echelle); // Y ecran inverse
    return p;
}

void GeoCalculV0Graphique::VersMonde(int sx, int sy, double& x, double& y) const {
    x = m_xMin + sx / m_echelle;
    y = m_yMin + (m_hauteurDessin - sy) / m_echelle;
}

POINT GeoCalculV0Graphique::PositionFleche(size_t idx, POINT pStation) const {
    POINT pRef = VersEcran(m_references[idx].x, m_references[idx].y);
    double dx = pRef.x - pStation.x, dy = pRef.y - pStation.y;
    POINT p;
    p.x = static_cast<LONG>(pStation.x + dx * 0.55);
    p.y = static_cast<LONG>(pStation.y + dy * 0.55);
    return p;
}

int GeoCalculV0Graphique::TrouverPointSousCurseur(int sx, int sy) const {
    constexpr int RAYON_CLIC = 10;
    constexpr int RAYON = 8; // doit rester coherent avec Dessiner()
    POINT pStation = VersEcran(m_xStation, m_yStation);

    HDC hdc = GetDC(m_hwnd);
    HFONT hFontAncien = static_cast<HFONT>(SelectObject(hdc, m_hFontDessin));

    for (size_t i = 0; i < m_references.size(); ++i) {
        POINT p = VersEcran(m_references[i].x, m_references[i].y);
        int dx = sx - p.x, dy = sy - p.y;
        if (dx * dx + dy * dy <= RAYON_CLIC * RAYON_CLIC) {
            SelectObject(hdc, hFontAncien);
            ReleaseDC(m_hwnd, hdc);
            return static_cast<int>(i);
        }

        POINT pFleche = PositionFleche(i, pStation);
        int dxF = sx - pFleche.x, dyF = sy - pFleche.y;
        if (dxF * dxF + dyF * dyF <= RAYON_CLIC * RAYON_CLIC) {
            SelectObject(hdc, hFontAncien);
            ReleaseDC(m_hwnd, hdc);
            return static_cast<int>(i);
        }

        // Zone du libelle (matricule), meme ancrage que dans Dessiner().
        std::wstring texte = VersWide(m_references[i].matricule);
        SIZE taille{};
        GetTextExtentPoint32(hdc, texte.c_str(), static_cast<int>(texte.size()), &taille);
        int xTexte = p.x + RAYON + 4;
        int yTexte = p.y - RAYON;
        if (sx >= xTexte && sx <= xTexte + taille.cx && sy >= yTexte && sy <= yTexte + taille.cy) {
            SelectObject(hdc, hFontAncien);
            ReleaseDC(m_hwnd, hdc);
            return static_cast<int>(i);
        }
    }

    SelectObject(hdc, hFontAncien);
    ReleaseDC(m_hwnd, hdc);
    return -1;
}

void GeoCalculV0Graphique::Dessiner(HDC hdc, int /*largeur*/, int /*hauteur*/) {
    SelectObject(hdc, m_hFontDessin);

    POINT pStation = VersEcran(m_xStation, m_yStation);

    // Lignes + fleches station -> reference (dessinees en premier, les
    // libelles passeront par-dessus avec un fond opaque pour les masquer).
    for (size_t i = 0; i < m_references.size(); ++i) {
        const auto& r = m_references[i];
        COLORREF couleur = r.selectionne ? RGB(0, 0, 0) : RGB(120, 120, 120);

        HPEN hPenLigne = CreatePen(PS_SOLID, 2, couleur);
        HPEN hPenAncien = static_cast<HPEN>(SelectObject(hdc, hPenLigne));

        POINT pRef = VersEcran(r.x, r.y);
        MoveToEx(hdc, pStation.x, pStation.y, nullptr);
        LineTo(hdc, pRef.x, pRef.y);

        double dx = pRef.x - pStation.x, dy = pRef.y - pStation.y;
        double longueur = std::sqrt(dx * dx + dy * dy);
        if (longueur > 1.0) {
            double ux = dx / longueur, uy = dy / longueur;
            POINT pFleche = PositionFleche(i, pStation);
            double mx = pFleche.x, my = pFleche.y;
            double taille = 9.0; // +30% par rapport a 7.0
            double pointeAvant = taille * 0.6, baseArriere = taille * 0.4, largeurBase = taille * 0.6;
            double bx = mx - ux * baseArriere, by = my - uy * baseArriere; // centre de la base
            POINT fleche[3];
            fleche[0] = { static_cast<LONG>(mx + ux * pointeAvant), static_cast<LONG>(my + uy * pointeAvant) };
            fleche[1] = { static_cast<LONG>(bx - uy * largeurBase), static_cast<LONG>(by + ux * largeurBase) };
            fleche[2] = { static_cast<LONG>(bx + uy * largeurBase), static_cast<LONG>(by - ux * largeurBase) };
            HBRUSH hBrFleche = CreateSolidBrush(couleur);
            HBRUSH hBrAncien = static_cast<HBRUSH>(SelectObject(hdc, hBrFleche));
            Polygon(hdc, fleche, 3);
            SelectObject(hdc, hBrAncien);
            DeleteObject(hBrFleche);
        }

        SelectObject(hdc, hPenAncien);
        DeleteObject(hPenLigne);
    }

    // Texte en fond opaque blanc : masque toute fleche/ligne qui passerait dessous.
    SetBkMode(hdc, OPAQUE);
    SetBkColor(hdc, RGB(255, 255, 255));

    // Points references
    constexpr int RAYON = 8; // +30% par rapport a 6
    for (const auto& r : m_references) {
        POINT p = VersEcran(r.x, r.y);
        HBRUSH hBr = CreateSolidBrush(r.selectionne ? RGB(255, 165, 0) : RGB(255, 230, 0));
        HBRUSH hBrAncien = static_cast<HBRUSH>(SelectObject(hdc, hBr));
        Ellipse(hdc, p.x - RAYON, p.y - RAYON, p.x + RAYON, p.y + RAYON);
        SelectObject(hdc, hBrAncien);
        DeleteObject(hBr);

        std::wstring texte = VersWide(r.matricule);
        TextOut(hdc, p.x + RAYON + 4, p.y - RAYON, texte.c_str(), static_cast<int>(texte.size()));
    }

    // Point station
    {
        HBRUSH hBr = CreateSolidBrush(RGB(220, 20, 20));
        HBRUSH hBrAncien = static_cast<HBRUSH>(SelectObject(hdc, hBr));
        Ellipse(hdc, pStation.x - RAYON, pStation.y - RAYON, pStation.x + RAYON, pStation.y + RAYON);
        SelectObject(hdc, hBrAncien);
        DeleteObject(hBr);

        std::wstring texteStation = VersWide(m_matriculeStation);
        TextOut(hdc, pStation.x + RAYON + 4, pStation.y - RAYON, texteStation.c_str(),
                static_cast<int>(texteStation.size()));
    }

    SetBkMode(hdc, TRANSPARENT);
}

} // namespace Konyx
