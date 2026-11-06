#include "GeoBaseParser.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace Konyx {

namespace {

GeoType TypeFromKeyword(const std::string& kw) {
    if (kw == "Station")       return GeoType::Station;
    if (kw == "Reference")     return GeoType::Reference;
    if (kw == "Mesure")        return GeoType::Mesure;
    if (kw == "Point")         return GeoType::Point;
    if (kw == "NivLectAR")     return GeoType::NivLectAR;
    if (kw == "NivLectAV")     return GeoType::NivLectAV;
    if (kw == "CheminAntenne") return GeoType::CheminAntenne;
    if (kw == "CheminEncadre") return GeoType::CheminEncadre;
    if (kw == "Appareil")      return GeoType::Appareil;
    if (kw == "Photo")         return GeoType::Photo;
    if (kw == "Polyligne")     return GeoType::Polyligne;
    if (kw == "ExcentVer")     return GeoType::ExcentVer;
    if (kw == "ExcentRad")     return GeoType::ExcentRad;
    if (kw == "ExcentTan")     return GeoType::ExcentTan;
    if (kw == "Commentaire")   return GeoType::Commentaire;
    return GeoType::Inconnu;
}

bool EstLigneEntete(const std::string& premierMot) {
    return premierMot.rfind("Option_", 0) == 0; // commence par "Option_"
}

std::string Trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// Decoupe une ligne en tokens separes par des espaces/tabulations.
std::vector<std::string> Tokeniser(const std::string& ligne) {
    std::vector<std::string> tokens;
    std::istringstream iss(ligne);
    std::string tok;
    while (iss >> tok) tokens.push_back(tok);
    return tokens;
}

std::optional<double> ToDouble(const std::string& s) {
    try { return std::stod(s); } catch (...) { return std::nullopt; }
}
std::optional<int> ToInt(const std::string& s) {
    try { return std::stoi(s); } catch (...) { return std::nullopt; }
}

// Remplit les champs specifiques d'un record selon son type, a partir
// des tokens restants (apres le matricule, pour les types qui en ont un).
// NOTE: la syntaxe .geo a un nombre de champs variable selon ce qui a ete
// mesure ou non (voir doc Covadis - Types d'elements). Certains cas sont
// ambigus par simple comptage de tokens (ex: Station 2 tokens peut etre
// HI+V0 ou HI+SY selon la doc) ; on privilegie ici le motif observe dans
// les fichiers .geo reels (HI+V0), a affiner si des cas SY apparaissent.
void RemplirChampsSpecifiques(GeoRecord& rec, const std::vector<std::string>& rt) {
    switch (rec.type) {
    case GeoType::Station:
        if (rt.size() >= 1) rec.HI = ToDouble(rt[0]);
        if (rt.size() >= 2) rec.V0 = ToDouble(rt[1]);
        if (rt.size() >= 3) rec.SY = ToInt(rt[2]);
        break;

    case GeoType::Reference:
    case GeoType::Mesure:
        if (rt.size() == 1) {
            rec.AH = ToDouble(rt[0]);
        } else if (rt.size() == 2) {
            rec.AH = ToDouble(rt[0]);
            rec.SY = ToInt(rt[1]);
        } else if (rt.size() >= 4) {
            rec.HP = ToDouble(rt[0]);
            rec.AH = ToDouble(rt[1]);
            rec.AV = ToDouble(rt[2]);
            rec.DI = ToDouble(rt[3]);
            if (rt.size() >= 5) rec.SY = ToInt(rt[4]);
        }
        break;

    case GeoType::Point:
        // Motif dominant observe dans les fichiers reels : CP X Y Z [SY]
        if (rt.size() >= 1) rec.CP = ToInt(rt[0]);
        if (rt.size() == 2) {
            // ambigu (SY ou Z) : on tente Z (decimal) en priorite
            rec.Z = ToDouble(rt[1]);
        } else if (rt.size() == 3) {
            rec.X = ToDouble(rt[1]);
            rec.Y = ToDouble(rt[2]);
        } else if (rt.size() >= 4) {
            rec.X = ToDouble(rt[1]);
            rec.Y = ToDouble(rt[2]);
            rec.Z = ToDouble(rt[3]);
            if (rt.size() >= 5) rec.SY = ToInt(rt[4]);
        }
        break;

    case GeoType::NivLectAR:
    case GeoType::NivLectAV:
        if (rt.size() >= 1) rec.LM = ToDouble(rt[0]);
        if (rt.size() >= 2) rec.DH = ToDouble(rt[1]);
        if (rt.size() >= 3) rec.SY = ToInt(rt[2]);
        break;

    case GeoType::CheminAntenne:
    case GeoType::CheminEncadre:
        rec.stationsChemin.assign(rt.begin(), rt.end());
        break;

    case GeoType::ExcentVer:
    case GeoType::ExcentRad:
    case GeoType::ExcentTan:
        if (rt.size() >= 1) rec.excentrement = ToDouble(rt[0]);
        break;

    default:
        break;
    }
}

} // namespace

GeoRecord GeoBaseParser::ParserLigne(int numLigne, const std::string& ligneOriginale) {
    GeoRecord rec;
    rec.numLigne = numLigne;
    rec.texteBrutOriginal = ligneOriginale;

    std::vector<std::string> tokens = Tokeniser(ligneOriginale);
    if (tokens.empty()) {
        rec.type = GeoType::Inconnu;
        return rec;
    }

    size_t idx = 0;
    std::string premier = tokens[idx];

    // Gere le prefixe "Commentaire" devant un autre type connu
    // (ex: "Commentaire Point C2 0 ... ...")
    if (premier == "Commentaire") {
        rec.estCommentaire = true;
        idx++;
        if (idx >= tokens.size()) {
            rec.type = GeoType::Commentaire;
            return rec;
        }
        GeoType sousType = TypeFromKeyword(tokens[idx]);
        if (sousType == GeoType::Inconnu) {
            // Commentaire "pur" : le reste de la ligne est du texte libre
            rec.type = GeoType::Commentaire;
            size_t posTexte = ligneOriginale.find(tokens[idx]);
            rec.texteCommentaire = (posTexte != std::string::npos)
                ? Trim(ligneOriginale.substr(posTexte)) : "";
            return rec;
        }
        rec.type = sousType;
        rec.typeBrut = tokens[idx];
        idx++;
    } else {
        rec.type = TypeFromKeyword(premier);
        rec.typeBrut = premier;
        idx++;
    }

    // Types sans matricule : Appareil, Photo, Excent*
    if (rec.type == GeoType::Appareil) {
        std::vector<double> vals;
        while (idx < tokens.size() && vals.size() < 6) {
            auto v = ToDouble(tokens[idx]);
            if (!v) break;
            vals.push_back(*v);
            idx++;
        }
        if (vals.size() >= 1) rec.appCV = vals[0];
        if (vals.size() >= 2) rec.appCP = vals[1];
        if (vals.size() >= 3) rec.appPAh = vals[2];
        if (vals.size() >= 4) rec.appPAv = vals[3];
        if (vals.size() >= 5) rec.appPDi = vals[4];
        if (vals.size() >= 6) rec.appPPM = vals[5];
        // le reste de la ligne (nom de l'appareil) est pris tel quel
        size_t posNom = ligneOriginale.find(tokens[idx > 0 ? idx - 1 : 0]);
        // on reconstruit simplement en reprenant les tokens restants
        std::ostringstream nom;
        for (size_t i = idx; i < tokens.size(); ++i) {
            if (i > idx) nom << " ";
            nom << tokens[i];
        }
        rec.appNom = nom.str();
        return rec;
    }

    if (rec.type == GeoType::Photo) {
        std::ostringstream chemin;
        for (size_t i = idx; i < tokens.size(); ++i) {
            if (i > idx) chemin << " ";
            chemin << tokens[i];
        }
        rec.cheminPhoto = chemin.str();
        return rec;
    }

    if (rec.type == GeoType::Commentaire) {
        // "Commentaire <texte libre>" sans sous-type reconnu, cas direct
        size_t posTexte = ligneOriginale.find("Commentaire");
        if (posTexte != std::string::npos) {
            std::string reste = ligneOriginale.substr(posTexte + std::string("Commentaire").length());
            rec.texteCommentaire = Trim(reste);
        }
        return rec;
    }

    // Tous les autres types ont un matricule en premier token
    if (idx < tokens.size()) {
        rec.matricule = tokens[idx];
        idx++;
    }

    std::vector<std::string> reste(tokens.begin() + idx, tokens.end());
    RemplirChampsSpecifiques(rec, reste);

    return rec;
}

bool GeoBaseParser::Charger(const std::string& cheminFichier,
                             GeoBaseEntete& entete,
                             std::vector<GeoRecord>& enregistrements) {
    std::ifstream fichier(cheminFichier);
    if (!fichier.is_open()) return false;

    enregistrements.clear();
    entete = GeoBaseEntete{};

    std::string ligne;
    int numLigneCourant = 0;

    while (std::getline(fichier, ligne)) {
        std::string ligneTrim = Trim(ligne);
        if (ligneTrim.empty()) continue;

        std::vector<std::string> tokens = Tokeniser(ligneTrim);
        if (tokens.empty()) continue;

        // Lignes d'entete (non numerotees) : Option_Correc, Option_Projec, ...
        if (EstLigneEntete(tokens[0])) {
            if (tokens[0] == "Option_Correc") entete.optionCorrec = ligneTrim;
            else if (tokens[0] == "Option_Projec") entete.optionProjec = ligneTrim;
            else if (tokens[0] == "Option_Nivell") entete.optionNivell = ligneTrim;
            else if (tokens[0] == "Option_Planim") entete.optionPlanim = ligneTrim;
            continue;
        }

        // Ligne numerotee : "000001  Station ..."
        auto numOpt = ToInt(tokens[0]);
        if (!numOpt) {
            // Ligne imprevue : on la garde telle quelle pour ne rien perdre
            GeoRecord rec;
            rec.type = GeoType::Inconnu;
            rec.texteBrutOriginal = ligneTrim;
            enregistrements.push_back(rec);
            continue;
        }
        numLigneCourant = *numOpt;

        size_t posApresNum = ligneTrim.find_first_of(" \t");
        std::string resteLigne = (posApresNum != std::string::npos)
            ? Trim(ligneTrim.substr(posApresNum)) : "";

        GeoRecord rec = ParserLigne(numLigneCourant, resteLigne);
        rec.numLigne = numLigneCourant;

        // Fusion des cheminements ecrits sur plusieurs lignes
        // (meme type + meme matricule que la ligne precedente)
        if (!enregistrements.empty()
            && (rec.type == GeoType::CheminAntenne || rec.type == GeoType::CheminEncadre)
            && enregistrements.back().type == rec.type
            && enregistrements.back().matricule == rec.matricule) {
            auto& prec = enregistrements.back();
            prec.stationsChemin.insert(prec.stationsChemin.end(),
                                        rec.stationsChemin.begin(), rec.stationsChemin.end());
            continue;
        }

        enregistrements.push_back(rec);
    }

    return true;
}

std::string GeoBaseParser::SerialiserLigne(const GeoRecord& rec) {
    std::ostringstream os;

    // Types geres sans re-serialisation fidele pour l'instant (Polyligne, Inconnu) :
    // on reecrit le texte original tel quel pour ne rien perdre.
    if (rec.type == GeoType::Polyligne || rec.type == GeoType::Inconnu) {
        return rec.texteBrutOriginal;
    }

    if (rec.estCommentaire) os << "Commentaire ";

    switch (rec.type) {
    case GeoType::Station:
        os << "Station " << rec.matricule;
        if (rec.HI) os << " " << std::fixed << std::setprecision(6) << *rec.HI;
        if (rec.V0) os << " " << std::fixed << std::setprecision(7) << *rec.V0;
        if (rec.SY) os << " " << *rec.SY;
        break;

    case GeoType::Reference:
    case GeoType::Mesure:
        os << (rec.type == GeoType::Reference ? "Reference " : "Mesure ") << rec.matricule;
        if (rec.HP) {
            os << " " << std::fixed << std::setprecision(6) << *rec.HP;
            os << " " << std::fixed << std::setprecision(7) << rec.AH.value_or(0.0);
            os << " " << std::fixed << std::setprecision(7) << rec.AV.value_or(0.0);
            os << " " << std::fixed << std::setprecision(6) << rec.DI.value_or(0.0);
        } else if (rec.AH) {
            os << " " << std::fixed << std::setprecision(7) << *rec.AH;
        }
        if (rec.SY) os << " " << *rec.SY;
        break;

    case GeoType::Point:
        os << "Point " << rec.matricule;
        if (rec.CP) os << " " << *rec.CP;
        if (rec.X && rec.Y) {
            os << " " << std::fixed << std::setprecision(6) << *rec.X;
            os << " " << std::fixed << std::setprecision(6) << *rec.Y;
        }
        if (rec.Z) os << " " << std::fixed << std::setprecision(6) << *rec.Z;
        if (rec.SY) os << " " << *rec.SY;
        break;

    case GeoType::NivLectAR:
    case GeoType::NivLectAV:
        os << (rec.type == GeoType::NivLectAR ? "NivLectAR " : "NivLectAV ") << rec.matricule;
        if (rec.LM) os << " " << std::fixed << std::setprecision(6) << *rec.LM;
        if (rec.DH) os << " " << std::fixed << std::setprecision(3) << *rec.DH;
        if (rec.SY) os << " " << *rec.SY;
        break;

    case GeoType::CheminAntenne:
    case GeoType::CheminEncadre:
        os << (rec.type == GeoType::CheminAntenne ? "CheminAntenne " : "CheminEncadre ") << rec.matricule;
        for (const auto& s : rec.stationsChemin) os << " " << s;
        break;

    case GeoType::Appareil:
        os << "Appareil"
           << " " << std::fixed << std::setprecision(2) << rec.appCV.value_or(0.0)
           << " " << std::fixed << std::setprecision(2) << rec.appCP.value_or(0.0)
           << " " << std::fixed << std::setprecision(3) << rec.appPAh.value_or(0.0)
           << " " << std::fixed << std::setprecision(3) << rec.appPAv.value_or(0.0)
           << " " << std::fixed << std::setprecision(3) << rec.appPDi.value_or(0.0)
           << " " << std::fixed << std::setprecision(1) << rec.appPPM.value_or(0.0)
           << " " << rec.appNom;
        break;

    case GeoType::Photo:
        os << "Photo " << rec.cheminPhoto;
        break;

    case GeoType::ExcentVer: os << "ExcentVer " << std::fixed << std::setprecision(2) << rec.excentrement.value_or(0.0); break;
    case GeoType::ExcentRad: os << "ExcentRad " << std::fixed << std::setprecision(2) << rec.excentrement.value_or(0.0); break;
    case GeoType::ExcentTan: os << "ExcentTan " << std::fixed << std::setprecision(2) << rec.excentrement.value_or(0.0); break;

    case GeoType::Commentaire:
        os << "Commentaire " << rec.texteCommentaire;
        break;

    default:
        os << rec.texteBrutOriginal;
        break;
    }

    return os.str();
}

bool GeoBaseParser::Sauvegarder(const std::string& cheminFichier,
                                 const GeoBaseEntete& entete,
                                 const std::vector<GeoRecord>& enregistrements) {
    std::ofstream fichier(cheminFichier, std::ios::trunc);
    if (!fichier.is_open()) return false;

    if (!entete.optionCorrec.empty()) fichier << "        " << entete.optionCorrec << "\n";
    if (!entete.optionProjec.empty()) fichier << "        " << entete.optionProjec << "\n";
    if (!entete.optionNivell.empty()) fichier << "        " << entete.optionNivell << "\n";
    if (!entete.optionPlanim.empty()) fichier << "        " << entete.optionPlanim << "\n";

    for (const auto& rec : enregistrements) {
        std::ostringstream numero;
        numero << std::setw(6) << std::setfill('0') << rec.numLigne;
        fichier << numero.str() << "  " << SerialiserLigne(rec) << "\n";
    }

    return true;
}

} // namespace Konyx
