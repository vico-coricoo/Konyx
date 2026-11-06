#include "GeoRecord.h"
#include <sstream>
#include <iomanip>

namespace Konyx {

static std::string Fmt(double v, int decimals) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(decimals) << v;
    return os.str();
}

std::string FormatElement(const GeoRecord& rec) {
    switch (rec.type) {
        case GeoType::Station:       return "Station";
        case GeoType::Reference:     return "Reference";
        case GeoType::Mesure:        return "Mesure";
        case GeoType::Point:         return "Point";
        case GeoType::NivLectAR:     return "NivLectAR";
        case GeoType::NivLectAV:     return "NivLectAV";
        case GeoType::Commentaire:   return "Commentaire";
        case GeoType::CheminAntenne: return "CheminAntenne";
        case GeoType::CheminEncadre: return "CheminEncadre";
        case GeoType::Appareil:      return "Appareil";
        case GeoType::Photo:         return "Photo";
        case GeoType::Polyligne:     return "Polyligne";
        case GeoType::ExcentVer:     return "ExcentVer";
        case GeoType::ExcentRad:     return "ExcentRad";
        case GeoType::ExcentTan:     return "ExcentTan";
        default:                     return rec.typeBrut.empty() ? "?" : rec.typeBrut;
    }
}


std::string FmtAligne(double valeur, int precision, int largeur = 12)
{
    std::ostringstream os;
    os << std::right << std::setw(largeur)
       << Fmt(valeur, precision);
    return os.str();
}

std::string FormatParametres(const GeoRecord& rec) {
    std::ostringstream os;

    switch (rec.type) {
    case GeoType::Station: {
        std::vector<std::string> parties;
        if (rec.HI) parties.push_back("HI = " + FmtAligne(*rec.HI, 3, 7));
        if (rec.V0 && *rec.V0 != -1.0) parties.push_back("V0 = " + FmtAligne(*rec.V0, 4, 8));

        std::string resultat;
        for (size_t i = 0; i < parties.size(); ++i) {
            if (i > 0) resultat += " , ";
            resultat += parties[i];
        }
        return resultat;
    }

    case GeoType::Reference:
    case GeoType::Mesure: {
        std::vector<std::string> parties;
        if (rec.HP) parties.push_back("HP = " + FmtAligne(*rec.HP, 3, 7));
        if (rec.AH) parties.push_back("AH = " + FmtAligne(*rec.AH, 5, 9));
        if (rec.AV) parties.push_back("AV = " + FmtAligne(*rec.AV, 5, 9));
        if (rec.DI) parties.push_back("DI = " + FmtAligne(*rec.DI, 3, 7));

        std::string resultat;
        for (size_t i = 0; i < parties.size(); ++i) {
            if (i > 0) resultat += " , ";
            resultat += parties[i];
        }
        return resultat;
    }

    case GeoType::Point: {
        std::vector<std::string> parties;
        if (rec.X) parties.push_back("X = " + FmtAligne(*rec.X, 3, 11));
        if (rec.Y) parties.push_back("Y = " + FmtAligne(*rec.Y, 3, 11));
        if (rec.Z) parties.push_back("Z = " + FmtAligne(*rec.Z, 3, 8));
        if (rec.CP) parties.push_back("CP = " + std::to_string(*rec.CP));

        std::string resultat;
        for (size_t i = 0; i < parties.size(); ++i) {
            if (i > 0) resultat += " , ";
            resultat += parties[i];
        }
        return resultat;
    }

    case GeoType::NivLectAR:
    case GeoType::NivLectAV: {
        std::vector<std::string> parties;
        if (rec.LM) parties.push_back("LM = " + FmtAligne(*rec.LM, 3, 5));
        if (rec.DH) parties.push_back("DH = " + FmtAligne(*rec.DH, 3, 6));

        std::string resultat;
        for (size_t i = 0; i < parties.size(); ++i) {
            if (i > 0) resultat += " , ";
            resultat += parties[i];
        }
        return resultat;
    }

    case GeoType::CheminAntenne:
    case GeoType::CheminEncadre: {
        for (size_t i = 0; i < rec.stationsChemin.size(); ++i) {
            if (i) os << " ";
            os << rec.stationsChemin[i];
        }
        break;
    }

    case GeoType::ExcentVer:
    case GeoType::ExcentRad:
    case GeoType::ExcentTan: {
        if (rec.excentrement) os << "Valeur = " << FmtAligne(*rec.excentrement, 3, 7);
        break;
    }

    case GeoType::Appareil: {
        std::vector<std::string> parties;
        if (rec.appCV) parties.push_back("CV = " + FmtAligne(*rec.appCV, 3, 5));
        if (rec.appCP) parties.push_back("CP = " + FmtAligne(*rec.appCP, 3, 5));
        if (rec.appPAh) parties.push_back("PAh = " + FmtAligne(*rec.appPAh, 4, 6));
        if (rec.appPAv) parties.push_back("PAv = " + FmtAligne(*rec.appPAv, 4, 6));
        if (rec.appPDi) parties.push_back("PDi = " + FmtAligne(*rec.appPDi, 4, 6));
        if (rec.appPPM) parties.push_back("PPM = " + FmtAligne(*rec.appPPM, 3, 3));
        if (!rec.appNom.empty()) parties.push_back(rec.appNom);

        std::string resultat;
        for (size_t i = 0; i < parties.size(); ++i) {
            if (i > 0) resultat += " , ";
            resultat += parties[i];
        }
        return resultat;
    }

    case GeoType::Photo:
        os << rec.cheminPhoto;
        break;

    case GeoType::Commentaire:
        os << rec.texteCommentaire;
        break;

    default:
        os << rec.texteBrutOriginal;
        break;
    }

    return os.str();
}

} // namespace Konyx
