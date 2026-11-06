#include "GeoBaseDialog.h"
#include <gdiplus.h>
#include <uxtheme.h>
#include <vector>
#include <string>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "UxTheme.lib")

namespace Konyx {

namespace {

// Charge un PNG via GDI+ et le convertit en HBITMAP 32bpp a alpha
// premultiplie, format attendu par un ImageList ILC_COLOR32 pour un
// rendu correct de la transparence.
HBITMAP ChargerPngVers32bpp(const std::wstring& chemin, int tailleIcone, int tailleCellule) {
    Gdiplus::Bitmap source(chemin.c_str());
    if (source.GetLastStatus() != Gdiplus::Ok) {
        return nullptr;
    }

    // Case transparente, plus grande que l'icone : l'icone est centree dedans.
    Gdiplus::Bitmap toile(tailleCellule, tailleCellule, PixelFormat32bppARGB);
    {
        Gdiplus::Graphics g(&toile);
        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        g.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
        const int marge = (tailleCellule - tailleIcone) / 2;
        g.DrawImage(&source, marge, marge, tailleIcone, tailleIcone);
    }

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = tailleCellule;
    bmi.bmiHeader.biHeight = -tailleCellule; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    HBITMAP hbm = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
    if (!hbm || !pBits) return nullptr;

    Gdiplus::Rect rect(0, 0, tailleCellule, tailleCellule);
    Gdiplus::BitmapData data{};
    if (toile.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data) != Gdiplus::Ok) {
        DeleteObject(hbm);
        return nullptr;
    }

    auto* srcBase = static_cast<BYTE*>(data.Scan0);
    auto* dstBase = static_cast<BYTE*>(pBits);
    for (int y = 0; y < tailleCellule; ++y) {
        auto* src = reinterpret_cast<DWORD*>(srcBase + y * data.Stride);
        auto* dst = reinterpret_cast<DWORD*>(dstBase + y * tailleCellule * 4);
        for (int x = 0; x < tailleCellule; ++x) {
            DWORD pixel = src[x];
            BYTE a = static_cast<BYTE>((pixel >> 24) & 0xFF);
            BYTE r = static_cast<BYTE>((pixel >> 16) & 0xFF);
            BYTE g = static_cast<BYTE>((pixel >> 8) & 0xFF);
            BYTE b = static_cast<BYTE>(pixel & 0xFF);
            r = static_cast<BYTE>(r * a / 255);
            g = static_cast<BYTE>(g * a / 255);
            b = static_cast<BYTE>(b * a / 255);
            dst[x] = (static_cast<DWORD>(a) << 24) | (static_cast<DWORD>(r) << 16) |
                     (static_cast<DWORD>(g) << 8) | static_cast<DWORD>(b);
        }
    }

    toile.UnlockBits(&data);
    return hbm;
}

void InitialiserGdiplusUneFois() {
    static bool initialise = false;
    if (initialise) return;
    static Gdiplus::GdiplusStartupInput entree;
    static ULONG_PTR jeton = 0;
    Gdiplus::GdiplusStartup(&jeton, &entree, nullptr);
    initialise = true;
}

} // namespace

void GeoBaseDialog::CreerRubanBoutons(HWND hwnd) {
    InitialiserGdiplusUneFois();

    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_BAR_CLASSES };
    InitCommonControlsEx(&icc);

    m_hRuban = CreateWindowEx(0, TOOLBARCLASSNAME, nullptr,
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_NODIVIDER,
        0, 0, 0, 0, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);

    // Desactive le theme visuel (sinon il redessine par-dessus notre
    // couleur de fond avec son propre degrade) puis sous-classe pour
    // intercepter WM_ERASEBKGND.
    SetWindowTheme(m_hRuban, L" ", L" ");
    //g_procRubanOriginal = reinterpret_cast<WNDPROC>(SetWindowLongPtr(m_hRuban, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ProcRubanPersonnalise)));

    SendMessage(m_hRuban, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    // Pas de TB_SETBUTTONSIZE explicite : on laisse le controle calculer
    // automatiquement la taille des boutons = taille icone + 2*padding,
    // ce qui garantit un espacement homogene entre TOUS les boutons
    // (un TB_SETBUTTONSIZE fixe desactive l'effet du padding).
    constexpr int TAILLE_ICONE   = 24;                        // taille de l'icone
    constexpr int ESPACE         = 6;                        // espace visible entre icones (px)
    constexpr int TAILLE_CELLULE = TAILLE_ICONE + ESPACE;     // case de l'ImageList
    //SendMessage(m_hRuban, TB_SETBUTTONSIZE, 0, MAKELPARAM(LARGEUR_BOUTON, HAUTEUR_BOUTON));

    struct IconeBouton { const wchar_t* fichier; int id; };
    static const IconeBouton icones[] = {
        { L"GEO_BAR_nouveau_fichier.png",     ID_FICHIER_NOUVEAU },
        { L"ouvrir_chemin.png",               ID_FICHIER_OUVRIR },
        { L"GEO_BAR_sauvegarder.png",         ID_FICHIER_ENREGISTRER },
        { L"GEO_BAR_recherche.PNG",           ID_CTX_RECHERCHER_MAT },
        { L"GEO_BAR_impression.png",          ID_FICHIER_IMPRIMER_GEOBASE },
        { L"GEO_BAR_annuler.png",             ID_EDITION_ANNULER },
        { L"GEO_BAR_retablir.PNG",            ID_EDITION_RETABLIR },
        { L"GEO_BAR_modifier.png",            ID_EDITION_MODIFIER },
        { L"GEO_BAR_supprimer.png",           ID_EDITION_SUPPRIMER },
        { L"GEO_BAR_nouvelle_station.png",    ID_TB_AJOUTER_STATION },
        { L"GEO_BAR_nouvelle_reference.png",  ID_TB_AJOUTER_REFERENCE },
        { L"GEO_BAR_nouvelle_mesure.png",     ID_TB_AJOUTER_MESURE },
        { L"GEO_BAR_nouveau_point.png",       ID_TB_AJOUTER_POINT },
        { L"GEO_BAR_nouvelle_lect_ar.png",    ID_TB_AJOUTER_LECT_AR },
        { L"GEO_BAR_nouvelle_lect_av.png",    ID_TB_AJOUTER_LECT_AV },
        { L"GEO_BAR_nouvelle_commande.png",   ID_TB_AJOUTER_COMMANDE },
        { L"GEO_BAR_nouveau_commentaire.png", ID_TB_AJOUTER_COMMENTAIRE },
        { L"GEO_BAR_nouveau_cheminement.png", ID_TB_AJOUTER_CHEMINEMENT },
        { L"GEO_BAR_nouvel_appareil.png",     ID_TB_AJOUTER_APPAREIL },
    };
    constexpr int NB_ICONES = sizeof(icones) / sizeof(icones[0]);

    static const wchar_t DOSSIER_ICONES[] = L"C:\\Dev\\Konyx\\resources\\cuix\\";

    HIMAGELIST hImageList = ImageList_Create(TAILLE_CELLULE, TAILLE_CELLULE, ILC_COLOR32, NB_ICONES, 0);
    for (const auto& ic : icones) {
        std::wstring chemin = std::wstring(DOSSIER_ICONES) + ic.fichier;
        HBITMAP hbm = ChargerPngVers32bpp(chemin, TAILLE_ICONE, TAILLE_CELLULE);
        if (hbm) {
            ImageList_Add(hImageList, hbm, nullptr);
            DeleteObject(hbm);
        } else {
            HICON hIcone = LoadIcon(nullptr, IDI_WARNING);
            ImageList_AddIcon(hImageList, hIcone);
        }
    }
    SendMessage(m_hRuban, TB_SETBITMAPSIZE, 0, MAKELPARAM(TAILLE_CELLULE, TAILLE_CELLULE));
    SendMessage(m_hRuban, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(hImageList));
    SendMessage(m_hRuban, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(hImageList));

    std::vector<TBBUTTON> boutons;
    int image = 0;
    auto ajouterBouton = [&](int id) {
        TBBUTTON b{};
        b.iBitmap = image++;
        b.idCommand = id;
        b.fsState = TBSTATE_ENABLED;
        b.fsStyle = TBSTYLE_BUTTON;
        boutons.push_back(b);
    };
    auto ajouterSeparateur = [&]() {
        TBBUTTON b{};
        b.fsStyle = TBSTYLE_SEP;
        boutons.push_back(b);
    };

    ajouterBouton(ID_FICHIER_NOUVEAU);
    ajouterBouton(ID_FICHIER_OUVRIR);
    ajouterBouton(ID_FICHIER_ENREGISTRER);
    ajouterSeparateur();
    ajouterBouton(ID_CTX_RECHERCHER_MAT);
    ajouterBouton(ID_FICHIER_IMPRIMER_GEOBASE);
    ajouterSeparateur();
    ajouterBouton(ID_EDITION_ANNULER);
    ajouterBouton(ID_EDITION_RETABLIR);
    ajouterSeparateur();
    ajouterBouton(ID_EDITION_MODIFIER);
    ajouterBouton(ID_EDITION_SUPPRIMER);
    ajouterSeparateur();
    ajouterBouton(ID_TB_AJOUTER_STATION);
    ajouterBouton(ID_TB_AJOUTER_REFERENCE);
    ajouterBouton(ID_TB_AJOUTER_MESURE);
    ajouterBouton(ID_TB_AJOUTER_POINT);
    ajouterBouton(ID_TB_AJOUTER_LECT_AR);
    ajouterBouton(ID_TB_AJOUTER_LECT_AV);
    ajouterBouton(ID_TB_AJOUTER_COMMANDE);
    ajouterBouton(ID_TB_AJOUTER_COMMENTAIRE);
    ajouterBouton(ID_TB_AJOUTER_CHEMINEMENT);
    ajouterBouton(ID_TB_AJOUTER_APPAREIL);

    SendMessage(m_hRuban, TB_ADDBUTTONS, boutons.size(), reinterpret_cast<LPARAM>(boutons.data()));
    SendMessage(m_hRuban, TB_AUTOSIZE, 0, 0);
}

} // namespace Konyx
