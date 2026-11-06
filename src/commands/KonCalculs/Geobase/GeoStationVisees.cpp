#include "GeoStationVisees.h"
#include <map>

namespace Konyx {

std::vector<StationEtVisees> ExtraireStationsEtVisees(const std::vector<GeoRecord>& enregistrements) {
    std::vector<StationEtVisees> resultat;
    std::map<std::string, int> compteurReprises; // matricule -> nb d'occurrences vues

    for (size_t i = 0; i < enregistrements.size(); ++i) {
        const GeoRecord& rec = enregistrements[i];
        if (rec.estCommentaire) {
            continue; // une ligne desactivee n'existe pas pour cette analyse
        }
        if (rec.type != GeoType::Station) {
            continue;
        }

        StationEtVisees groupe;
        groupe.station = &rec;
        groupe.indiceReprise = ++compteurReprises[rec.matricule];

        // Les visees sont les References/Mesures qui suivent immediatement,
        // en ignorant les lignes desactivees, jusqu'a un autre type.
        for (size_t j = i + 1; j < enregistrements.size(); ++j) {
            const GeoRecord& suivant = enregistrements[j];
            if (suivant.estCommentaire) {
                continue;
            }
            if (suivant.type == GeoType::Reference || suivant.type == GeoType::Mesure) {
                groupe.visees.push_back(&suivant);
            } else {
                break; // un autre type de ligne : fin du groupe de visees
            }
        }

        resultat.push_back(std::move(groupe));
    }

    return resultat;
}

const GeoRecord* TrouverPointParMatricule(const std::vector<GeoRecord>& enregistrements,
                                           const std::string& matricule) {
    for (const GeoRecord& rec : enregistrements) {
        if (rec.estCommentaire) {
            continue;
        }
        if (rec.type == GeoType::Point && rec.matricule == matricule) {
            return &rec;
        }
    }
    return nullptr;
}

std::vector<std::string> ListerMatriculesStations(const std::vector<GeoRecord>& enregistrements) {
    std::vector<std::string> resultat;
    for (const GeoRecord& rec : enregistrements) {
        if (rec.estCommentaire || rec.type != GeoType::Station) {
            continue;
        }
        resultat.push_back(rec.matricule);
    }
    return resultat;
}

} // namespace Konyx
