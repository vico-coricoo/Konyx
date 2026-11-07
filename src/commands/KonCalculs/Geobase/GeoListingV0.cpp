#include "GeoListingV0.h"
#include "GeoStationVisees.h"
#include "GeoCalculV0.h"

#include <windows.h>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstdint>
#include <cstdio>

namespace Konyx {

// =======================================================================
// Utilitaires generaux
// =======================================================================

// Convertit une chaine UTF-8 en texte RTF : echappe \ { }, et encode tout
// caractere non-ASCII en \uN? (N = point de code UTF-16 signe, "?" = repli
// pour les lecteurs qui ne savent pas lire \u). Les codes > 0xFFFF
// (hors BMP) ne sont pas attendus ici (texte metier francais uniquement).
static std::string EchapperRtf(const std::string& utf8) {
    std::string out;
    out.reserve(utf8.size() * 2);

    size_t i = 0;
    while (i < utf8.size()) {
        unsigned char c = static_cast<unsigned char>(utf8[i]);
        unsigned int codepoint = 0;
        size_t octets = 0;

        if (c < 0x80) { codepoint = c; octets = 1; }
        else if ((c & 0xE0) == 0xC0) { codepoint = c & 0x1F; octets = 2; }
        else if ((c & 0xF0) == 0xE0) { codepoint = c & 0x0F; octets = 3; }
        else if ((c & 0xF8) == 0xF0) { codepoint = c & 0x07; octets = 4; }
        else { ++i; continue; } // octet invalide : ignore

        if (i + octets > utf8.size()) break;
        for (size_t k = 1; k < octets; ++k) {
            unsigned char suite = static_cast<unsigned char>(utf8[i + k]);
            if ((suite & 0xC0) != 0x80) { octets = 1; codepoint = c; break; } // suite invalide
            codepoint = (codepoint << 6) | (suite & 0x3F);
        }

        if (codepoint == '\\' || codepoint == '{' || codepoint == '}') {
            out += '\\';
            out += static_cast<char>(codepoint);
        } else if (codepoint == '\n') {
            out += "\\line ";
        } else if (codepoint < 0x80) {
            out += static_cast<char>(codepoint);
        } else {
            int16_t valeurSignee = (codepoint > 32767) ? static_cast<int16_t>(codepoint - 65536)
                                                         : static_cast<int16_t>(codepoint);
            out += "\\u";
            out += std::to_string(static_cast<int>(valeurSignee));
            out += "?";
        }

        i += octets;
    }

    return out;
}

static std::string WstringVersUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string s(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, s.data(), len, nullptr, nullptr);
    if (!s.empty() && s.back() == '\0') s.pop_back();
    return s;
}

static std::string FormatGon(double v, int decimales = 4) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimales) << v << "g";
    return oss.str();
}

static std::string FormatNombre(double v, int decimales) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimales) << v;
    return oss.str();
}

static std::string DateHeureActuelle() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%02u/%02u/%04u - %02u:%02u:%02u",
                  st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
    return buf;
}

// =======================================================================
// Construction de tableaux RTF
// =======================================================================

// Largeur utile de la page : A4 (11906 twips) - marges (850 x 2).
static constexpr int LARGEUR_PAGE = 10206;

static std::string BordureCellule() {
    return "\\clbrdrt\\brdrs\\brdrw10\\clbrdrl\\brdrs\\brdrw10"
           "\\clbrdrb\\brdrs\\brdrw10\\clbrdrr\\brdrs\\brdrw10";
}

// Banniere : une seule cellule bordee et grisee, texte gras centre.
// Utilisee pour les titres de section ("CALCULS TOPOMETRIQUES",
// "Calcul de la station...", "Remarques").
static std::string Banniere(const std::string& texteUtf8, int taillePolice = 24) {
    std::ostringstream oss;
    oss << "\\trowd\\trgaph80\\trleft0"
        << BordureCellule() << "\\clshdng10000\\clcbpat2\\cellx" << LARGEUR_PAGE
        << "\\pard\\intbl\\qc\\b\\fs" << taillePolice << " " << EchapperRtf(texteUtf8)
        << "\\b0\\fs20\\cell\\row\n";
    return oss.str();
}

// Une colonne (largeur cumulee, texte deja echappe RTF).
struct Colonne {
    int largeurCumulee;
    std::string texte;
};

static std::string LigneTableau(const std::vector<Colonne>& colonnes,
                                 bool gras, bool italique, bool ombree) {
    std::ostringstream oss;
    oss << "\\trowd\\trgaph80\\trleft0";
    for (const auto& c : colonnes) {
        oss << BordureCellule();
        if (ombree) oss << "\\clshdng10000\\clcbpat2";
        oss << "\\cellx" << c.largeurCumulee;
    }
    for (const auto& c : colonnes) {
        oss << "\\pard\\intbl\\qc";
        if (gras) oss << "\\b";
        if (italique) oss << "\\i";
        oss << " " << c.texte;
        if (italique) oss << "\\i0";
        if (gras) oss << "\\b0";
        oss << "\\cell";
    }
    oss << "\\row\n";
    return oss.str();
}

// =======================================================================
// En-tete / pied de page du document
// =======================================================================

static std::string EnteteDocument() {
    std::string footer =
        "{\\footer\\pard\\tqr\\tx" + std::to_string(LARGEUR_PAGE) + "\\fs16 "
        + EchapperRtf(DateHeureActuelle())
        + "\\tab Page \\chpgn  / {\\field{\\*\\fldinst  NUMPAGES }{\\fldrslt 1}}\\fs20\\par}\n";

    return
        "{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1036\n"
        "{\\fonttbl{\\f0\\fswiss\\fcharset0 Arial;}}\n"
        "{\\colortbl;\\red0\\green0\\blue0;\\red224\\green224\\blue224;}\n"
        "\\paperw11906\\paperh16838\\margl850\\margr850\\margt850\\margb850\n"
        + footer +
        "\\f0\\fs20\n";
}

// =======================================================================
// Contenu d'une page (une station)
// =======================================================================

static std::string GenererPageStation(const StationEtVisees& s,
                                       const std::vector<GeoRecord>& enregistrements,
                                       const std::string& nomGeobase) {
    std::ostringstream page;

    const GeoRecord* pointStation = TrouverPointParMatricule(enregistrements, s.station->matricule);

    std::vector<const GeoRecord*> references;
    for (const GeoRecord* v : s.visees) {
        if (v->type == GeoType::Reference) references.push_back(v);
    }

    ResultatV0 resultat = CalculerV0(pointStation, references, enregistrements);

    // --- Entete general (repete sur chaque page) ---------------------
    page << Banniere("CALCULS TOPOMÉTRIQUES", 28);
    page << "\\pard\\qc\\b\\fs22 CALCUL DES V0 DE STATIONS\\b0\\fs20\\par\\par\n";

    std::string texteGeobase = nomGeobase.empty() ? "(non enregistrée)" : nomGeobase;
    page << "\\pard\\ql " << EchapperRtf("Géobase : ") << "\\b " << EchapperRtf(texteGeobase) << "\\b0\\par\n";
    page << "\\par\n";
    page << EchapperRtf(std::string("Projection utilisée : ") + PROJECTION_LIBELLE) << "\\par\n";
    page << EchapperRtf("Altération linéaire = 0 mm/km - Correction de niveau zéro = 0 mm/km") << "\\par\n";
    page << "\\par\n";
    page << EchapperRtf(std::string("Tolérances utilisées : ") + TOLERANCES_LIBELLE) << "\\par\n";
    page << "\\par\n";

    // --- Banniere station ---------------------------------------------
    std::ostringstream titreStation;
    titreStation << "Calcul de la station " << s.station->matricule << ", reprise " << s.indiceReprise;
    page << Banniere(titreStation.str(), 22);
    page << "\\pard\\par\n";

    // --- Coordonnees des points d'appui --------------------------------
    page << "\\pard\\ql\\b " << EchapperRtf("Coordonnées des points d'appui") << "\\b0\\par\\par\n";

    page << LigneTableau({ {1800, ""}, {3600, "\\b Matricule\\b0"}, {5800, "\\b X\\b0"},
                            {8000, "\\b Y\\b0"}, {10206, "\\b Z\\b0"} }, false, false, true);

    auto formateCoord = [](const std::optional<double>& v) {
        return v.has_value() ? FormatNombre(*v, 3) : std::string();
    };

    page << LigneTableau({
        {1800, "Station"},
        {3600, EchapperRtf(s.station->matricule)},
        {5800, pointStation ? formateCoord(pointStation->X) : ""},
        {8000, pointStation ? formateCoord(pointStation->Y) : ""},
        {10206, pointStation ? formateCoord(pointStation->Z) : ""}
    }, false, false, false);

    for (const GeoRecord* ref : references) {
        const GeoRecord* pt = TrouverPointParMatricule(enregistrements, ref->matricule);
        page << LigneTableau({
            {1800, EchapperRtf("Référence")},
            {3600, EchapperRtf(ref->matricule)},
            {5800, pt ? formateCoord(pt->X) : ""},
            {8000, pt ? formateCoord(pt->Y) : ""},
            {10206, pt ? formateCoord(pt->Z) : ""}
        }, false, false, false);
    }
    page << "\\pard\\par\n";

    // --- Calcul du V0 ----------------------------------------------------
    std::ostringstream titreCalcul;
    titreCalcul << "Calcul du V0 de la station " << s.station->matricule
                << " (" << s.indiceReprise << ") (tolérance = " << FormatGon(TOLERANCE_V0_GON) << ")";
    page << "\\pard\\ql\\b " << EchapperRtf(titreCalcul.str()) << "\\b0\\par\\par\n";

    page << LigneTableau({ {1700, "\\b " + EchapperRtf("Référence") + "\\b0"}, {3400, "\\b Gi\\b0"}, {5100, "\\b Dh\\b0"},
                            {6800, "\\b Ah\\b0"}, {8500, "\\b V0\\b0"}, {10206, "\\b eV0\\b0"} },
                          false, false, true);

    std::vector<std::string> remarques;

    for (const auto& c : resultat.contributions) {
        if (!c.valide) {
            remarques.push_back("Référence " + c.visee->matricule + " non exploitable (" + c.raisonInvalide + ")");
            continue;
        }

        std::string eV0Texte = "-";
        if (resultat.v0Moyen.has_value()) {
            double ecart = *resultat.v0Moyen - c.v0;
            eV0Texte = FormatGon(ecart);

            if (std::fabs(ecart) > TOLERANCE_V0_GON) {
                std::ostringstream r;
                r << "V0 hors tolérance pour la référence <" << c.visee->matricule << "> !"
                  << "\n    (écart = " << FormatGon(std::fabs(ecart)) << " Gr > "
                  << FormatGon(TOLERANCE_V0_GON) << " Gr)";
                remarques.push_back(r.str());
            }
        }

        page << LigneTableau({
            {1700, EchapperRtf(c.visee->matricule)},
            {3400, FormatGon(c.gisement)},
            {5100, FormatNombre(c.distance, 3)},
            {6800, c.visee->AH.has_value() ? FormatGon(*c.visee->AH) : "-"},
            {8500, FormatGon(c.v0)},
            {10206, eV0Texte}
        }, false, false, false);
    }

    page << LigneTableau({
        {1700, ""}, {3400, ""}, {5100, ""}, {6800, ""},
        {8500, resultat.v0Moyen.has_value() ? FormatGon(*resultat.v0Moyen) : "-"}, {10206, ""}
    }, false, true, false);
    page << "\\pard\\qr\\i " << EchapperRtf("Valeur du V0 avant calcul") << "\\i0\\par\n"; // libelle sous forme de legende

    page << LigneTableau({
        {1700, ""}, {3400, ""}, {5100, ""}, {6800, ""},
        {8500, resultat.v0Pondere.has_value() ? FormatGon(*resultat.v0Pondere) : "-"}, {10206, ""}
    }, true, false, false);
    page << "\\pard\\qr\\b " << EchapperRtf("Valeur du V0 après calcul pondéré") << "\\b0\\par\n";

    page << "\\pard\\par\n";

    // --- Remarques ---------------------------------------------------
    page << Banniere("Remarques", 20);
    page << "\\pard\\ql\\fs18\n";
    if (remarques.empty()) {
        page << EchapperRtf("Aucune remarque.") << "\\par\n";
    } else {
        for (const auto& r : remarques) {
            page << " -> " << EchapperRtf(r) << "\\par\n";
        }
    }
    page << "\\fs20\\par\n";

    return page.str();
}

// =======================================================================

std::string GenererListingV0(const std::vector<GeoRecord>& enregistrements,
                              const std::wstring& cheminGeobase) {
    std::vector<StationEtVisees> stations = ExtraireStationsEtVisees(enregistrements);

    std::vector<const StationEtVisees*> retenues;
    for (const auto& s : stations) {
        const GeoRecord* pt = TrouverPointParMatricule(enregistrements, s.station->matricule);
        if (!pt || !pt->X.has_value() || !pt->Y.has_value()) continue;

        bool auMoinsUneRef = false;
        for (const GeoRecord* v : s.visees) {
            if (v->type == GeoType::Reference) { auMoinsUneRef = true; break; }
        }
        if (!auMoinsUneRef) continue;

        retenues.push_back(&s);
    }

    std::string nomGeobase = cheminGeobase.empty() ? std::string() : WstringVersUtf8(cheminGeobase);

    std::string rtf = EnteteDocument();

    if (retenues.empty()) {
        rtf += "\\pard\\qc " + EchapperRtf("Aucune station calculable dans cette géobase.") + "\\par\n";
    }

    for (size_t i = 0; i < retenues.size(); ++i) {
        rtf += GenererPageStation(*retenues[i], enregistrements, nomGeobase);
        if (i + 1 < retenues.size()) rtf += "\\page\n";
    }

    rtf += "}\n";
    return rtf;
}

} // namespace Konyx
