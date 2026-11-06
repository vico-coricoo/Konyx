#include "GeoCalculV0.h"
#include "GeoStationVisees.h"
#include <cmath>
#include <algorithm>

namespace Konyx {

static constexpr double PI = 3.14159265358979323846;

double NormaliserGrades(double grades) {
    double g = std::fmod(grades, 400.0);
    if (g < 0.0) {
        g += 400.0;
    }
    return g;
}

double CalculerGisement(double xSt, double ySt, double xRef, double yRef) {
    double dx = xRef - xSt;
    double dy = yRef - ySt;
    double rad = std::atan2(dx, dy); // atan2(X,Y) : convention gisement (angle depuis le Nord = Y)
    double grades = rad * (200.0 / PI);
    return NormaliserGrades(grades);
}

static double Distance(double xSt, double ySt, double xRef, double yRef) {
    double dx = xRef - xSt;
    double dy = yRef - ySt;
    return std::sqrt(dx * dx + dy * dy);
}

ResultatV0 CalculerV0(const GeoRecord* pointStation,
                       const std::vector<const GeoRecord*>& visees,
                       const std::vector<GeoRecord>& enregistrements) {
    ResultatV0 resultat;

    for (const GeoRecord* visee : visees) {
        ContributionV0 c;
        c.visee = visee;

        if (!pointStation || !pointStation->X.has_value() || !pointStation->Y.has_value()) {
            c.valide = false;
            c.raisonInvalide = "coordonnées de la station inconnues";
            resultat.contributions.push_back(c);
            continue;
        }

        if (!visee->AH.has_value()) {
            c.valide = false;
            c.raisonInvalide = "AH non mesuré";
            resultat.contributions.push_back(c);
            continue;
        }

        const GeoRecord* pointRef = TrouverPointParMatricule(enregistrements, visee->matricule);
        if (!pointRef || !pointRef->X.has_value() || !pointRef->Y.has_value()) {
            c.valide = false;
            c.raisonInvalide = "coordonnées de la référence inconnues";
            resultat.contributions.push_back(c);
            continue;
        }

        c.pointRef = pointRef;
        c.gisement = CalculerGisement(*pointStation->X, *pointStation->Y, *pointRef->X, *pointRef->Y);
        c.v0 = NormaliserGrades(c.gisement - *visee->AH);
        c.distance = Distance(*pointStation->X, *pointStation->Y, *pointRef->X, *pointRef->Y);
        c.valide = true;
        resultat.contributions.push_back(c);
    }

    double sommeSin = 0.0, sommeCos = 0.0;
    double sommeSinPondere = 0.0, sommeCosPondere = 0.0;
    double sommeDistances = 0.0;
    int nbValides = 0;

    for (const ContributionV0& c : resultat.contributions) {
        if (!c.valide) {
            continue;
        }
        double rad = c.v0 * (PI / 200.0);
        double s = std::sin(rad);
        double co = std::cos(rad);

        sommeSin += s;
        sommeCos += co;
        sommeSinPondere += s * c.distance;
        sommeCosPondere += co * c.distance;
        sommeDistances += c.distance;
        ++nbValides;
    }

    constexpr double EPSILON = 1e-9;

    if (nbValides > 0 && (std::abs(sommeSin) > EPSILON || std::abs(sommeCos) > EPSILON)) {
        double rad = std::atan2(sommeSin, sommeCos);
        resultat.v0Moyen = NormaliserGrades(rad * (200.0 / PI));
    }

    if (sommeDistances > 0.0 && (std::abs(sommeSinPondere) > EPSILON || std::abs(sommeCosPondere) > EPSILON)) {
        double rad = std::atan2(sommeSinPondere, sommeCosPondere);
        resultat.v0Pondere = NormaliserGrades(rad * (200.0 / PI));
    }

    return resultat;
}

std::vector<bool> SelectionAutomatiqueV0(const std::vector<ContributionV0>& contributions, double seuilGon) {
    std::vector<bool> coches(contributions.size(), false);

    std::vector<size_t> indicesValides;
    for (size_t i = 0; i < contributions.size(); ++i) {
        if (contributions[i].valide) indicesValides.push_back(i);
    }

    if (indicesValides.size() < 3) {
        for (size_t i : indicesValides) coches[i] = true;
        return coches;
    }

    auto ecartCirculaire = [](double a, double b) {
        double d = std::fmod(std::abs(a - b), 400.0);
        return (std::min)(d, 400.0 - d);
    };

    std::vector<double> minEcart(indicesValides.size(), 1e18);
    for (size_t a = 0; a < indicesValides.size(); ++a) {
        for (size_t b = 0; b < indicesValides.size(); ++b) {
            if (a == b) continue;
            double e = ecartCirculaire(contributions[indicesValides[a]].v0, contributions[indicesValides[b]].v0);
            minEcart[a] = (std::min)(minEcart[a], e);
        }
    }

    // Si personne n'est proche de personne (ecart > seuil partout), pas
    // de "groupe majoritaire" identifiable : on garde tout le monde
    // plutot que de tout exclure.
    bool auMoinsUnProche = false;
    for (double e : minEcart) {
        if (e <= seuilGon) { auMoinsUnProche = true; break; }
    }

    for (size_t a = 0; a < indicesValides.size(); ++a) {
        bool garder = !auMoinsUnProche || minEcart[a] <= seuilGon;
        coches[indicesValides[a]] = garder;
    }

    return coches;
}

} // namespace Konyx
