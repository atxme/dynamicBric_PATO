# dynamicBric_PATO

## Description
dynamicBric_PATO est une bibliothèque modulaire conçue pour les systèmes embarqués qui implémente plusieurs fonctionnalités fondamentales :

- **Gestion de tableaux dynamiques** : Allocation et manipulation de tableaux redimensionnables
- **Gestion de mutex** : Synchronisation thread-safe des accès aux ressources partagées
- **Gestion des threads** : Création et contrôle de threads pour les opérations parallèles
- **Gestion des assertions** : Vérification des conditions et validation pendant l'exécution
- **Gestion des logs** : Système de journalisation configurable et extensible
- **Gestion du temps** : Fonctions précises de mesure et de gestion du temps
- **Gestion réseau** : Communication réseau robuste avec IPv4, TCP/UDP et keep-alive
- **Gestion des fichiers** : Opérations sur les fichiers optimisées pour les systèmes embarqués

## Nouveauté : Mécanisme de Keep-Alive Réseau

Le projet intègre désormais un mécanisme de keep-alive pour maintenir les connexions réseau actives et détecter les déconnexions de manière précoce. Ce mécanisme :

- Envoie périodiquement (par défaut toutes les 30 secondes) des paquets de test
- Détecte automatiquement les connexions interrompues
- Fournit un système de callbacks pour notifier l'application des événements
- Est entièrement thread-safe et compatible avec l'architecture existante
- Optimisé pour les environnements à ressources limitées

### Utilisation du Keep-Alive :

```c
// Initialiser et démarrer le keep-alive
NetworkKeepAlive* keepAlive = networkKeepAliveInit(socket, 30, 5, 3);
networkKeepAliveSetCallback(keepAlive, myCallback, userData);
networkKeepAliveStart(keepAlive, NULL, 0);

// Plus tard, nettoyer les ressources
networkKeepAliveStop(keepAlive);
networkKeepAliveCleanup(keepAlive);
```

Pour plus d'informations sur le keep-alive, consultez `network/KEEPALIVE_README.md`.

## Installation

```bash
sudo apt install build-essential cmake libssl-dev ninja-build libgtest-dev
```

## Compilation
```bash
./compile_test.sh
```

## Structure du projet

```
.
├── core/           # Fonctionnalités fondamentales
├── network/        # Modules de communication réseau
│   ├── xNetwork.h        # API réseau principale
│   ├── xNetwork.c        # Implémentation réseau
│   ├── xNetworkKeepAlive.h  # API keep-alive
│   ├── xNetworkKeepAlive.c  # Implémentation keep-alive
│   └── xNetworkKeepAliveExample.c  # Exemple d'utilisation
├── file/           # Gestion des fichiers
├── thread/         # Gestion des threads
├── time/           # Utilitaires de temps
└── tests/          # Tests unitaires
```

## Caractéristiques principales

- **Optimisé pour l'embarqué** : Empreinte mémoire minimale, allocation contrôlée
- **Thread-safe** : Conception pensée pour les environnements multi-threadés
- **Portabilité** : Fonctionne sur diverses architectures et systèmes d'exploitation
- **Modularité** : Les composants peuvent être utilisés indépendamment
- **Extensibilité** : Architecture permettant d'ajouter facilement de nouvelles fonctionnalités

## Exemples d'utilisation

Consultez le dossier `examples/` pour des démonstrations d'utilisation des différentes fonctionnalités.

## Documentation

Une documentation détaillée de chaque module est disponible dans les fichiers README correspondants :

- `network/KEEPALIVE_README.md` : Documentation du mécanisme de keep-alive
- [Autres liens de documentation]
