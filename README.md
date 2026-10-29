# Konyx

Clone libre et open source de **Covadis** (Sogelink) — logiciel de
topographie et de développement VRD en surcouche d'AutoCAD.

> ⚠️ Projet en tout début de développement. Ce dépôt contient un
> **squelette minimal** destiné à poser l'architecture avant d'implémenter
> les vraies fonctions.

## Feuille de route des modules

Ordre de développement retenu :

1. **Kon. Calculs** (`src/commands/KonCalculs/`) — équivalent de Cov. Calcul
2. **Konyx 2D** (`src/commands/Konyx2D/`) — équivalent de Covadis 2D
3. **Konyx 3D** (`src/commands/Konyx3D/`) — équivalent de Covadis 3D
4. **Kon. Edition** (`src/commands/KonEdition/`) — équivalent de Cov. Edition

Les menus VRD et Échanges sont volontairement mis de côté pour l'instant.

## Architecture du squelette

```
Konyx/
├── CMakeLists.txt              # Build (Windows/MSVC uniquement)
├── src/
│   ├── KonyxApp.cpp / .h       # Point d'entrée ObjectARX (acrxEntryPoint)
│   ├── core/
│   │   └── CommandRegistry.*   # Auto-enregistrement des commandes
│   └── commands/
│       ├── KonCalculs/
│       ├── Konyx2D/
│       ├── Konyx3D/
│       └── KonEdition/
├── resources/                  # Futurs rubans/menus/icônes
└── docs/
    └── ARCHITECTURE.md
```

### Le point clé à comprendre : `KONYX_COMMAND`

Covadis compte des centaines de commandes. Pour ne jamais avoir à modifier
un fichier central (et donc éviter de régénérer de grosses portions de code
à chaque ajout), chaque commande s'auto-déclare avec une macro, où qu'elle
soit dans le projet :

```cpp
KONYX_COMMAND("KONYX", "KonCalculsVersion", "KCVERSION", 0, KonCalculsVersion)
{
    acutPrintf(_T("\nKonyx pret.\n"));
}
```

Elle est automatiquement collectée par `CommandRegistry` et enregistrée
auprès d'AutoCAD au chargement du module. Voir
`src/commands/KonCalculs/KonCalculsCommands.cpp` pour un exemple commenté.

## Compilation

**La compilation ne peut se faire que sous Windows avec Visual Studio**
(ObjectARX est une techno Windows). Sur votre MBP2024/VSCode, vous pouvez
lire, écrire et faire évoluer le code ; la compilation et les tests réels
se feront sur votre MBP2019 en dual-boot Windows 11.

Prérequis côté Windows :
- Visual Studio 2022 (Desktop development with C++)
- CMake ≥ 3.20
- ObjectARX SDK pour AutoCAD 2027
- AutoCAD 2027 installé

Étapes :

```powershell
$env:KONYX_ARX_SDK = "C:\ObjectARX 2027 SDK"
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Le fichier produit sera `build/Release/Konyx.arx`.

## Charger Konyx dans AutoCAD (test manuel)

1. Lancer AutoCAD 2027
2. Taper `NETLOAD` ou `APPLOAD`, sélectionner `Konyx.arx`
3. Taper `KCVERSION`, `K2DVERSION`, `K3DVERSION`, `KEVERSION` pour vérifier
   que les 4 modules répondent correctement

## Prochaine étape : le lanceur "surcouche"

Vous avez précisé que Konyx doit se comporter comme Covadis : lancer
AutoCAD avec le module chargé automatiquement, et ne rien faire si
AutoCAD est ouvert seul (sans passer par Konyx). Ce comportement est géré
par un **lanceur séparé** (un petit exécutable ou script qui démarre
AutoCAD avec un paramètre `/b` pointant vers un script de chargement), pas
par le module `.arx` lui-même. C'est volontairement laissé de côté dans ce
squelette pour se concentrer d'abord sur la structure des commandes — on
l'ajoutera dans une étape dédiée une fois Kon. Calculs bien avancé.
