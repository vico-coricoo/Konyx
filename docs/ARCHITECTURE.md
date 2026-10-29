# Architecture — notes

## Pourquoi ObjectARX plutôt que AutoLISP ou .NET

Covadis historique est en grande partie un module ARX/C++. Pour une
réplique fidèle (performances, accès bas niveau à la base de données du
dessin via `AcDb*`), ObjectARX C++ est le choix cohérent. C'est aussi le
plus exigeant techniquement — d'où l'accompagnement pas à pas.

## Cycle de vie d'un module ObjectARX

AutoCAD charge le `.arx` comme une DLL et appelle `acrxEntryPoint` avec
différents messages. Les deux qui nous intéressent pour l'instant :

| Message          | Quand                        | Ce qu'on fait                     |
|-------------------|-------------------------------|------------------------------------|
| `kInitAppMsg`     | juste après le chargement     | enregistrer toutes les commandes   |
| `kUnloadAppMsg`   | juste avant le déchargement   | désenregistrer proprement          |

## Pourquoi un `CommandRegistry` plutôt que des appels directs

Sans ce mécanisme, chaque nouvelle commande obligerait à :
1. écrire la fonction C++
2. ET ajouter une ligne dans `KonyxApp.cpp` pour l'enregistrer

Avec des centaines de commandes prévues (tout Covadis), cela deviendrait
vite une source d'oublis et de fichiers centraux énormes. Le pattern
"auto-registration" (classique en C++, basé sur l'exécution de
constructeurs d'objets statiques avant `main`/`acrxEntryPoint`) permet à
chaque fichier de commande d'être totalement autonome.

## Prochaines briques (pas encore dans ce squelette)

- **Lanceur Konyx** : exécutable qui démarre AutoCAD avec chargement
  automatique du `.arx` (ex: `acad.exe /b konyx_autoload.scr`), et qui
  seul doit permettre à Konyx d'apparaître (AutoCAD lancé seul = pas de
  surcouche).
- **Ruban/menus personnalisés** (fichiers `.cuix`, ou création par code
  via `AcRibbon`) pour les 4 menus Kon. Calculs / Konyx 2D / Konyx 3D /
  Kon. Edition.
- **Première vraie commande de Kon. Calculs** une fois le squelette validé
  par compilation + chargement réel sous Windows.
