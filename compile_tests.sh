#!/bin/bash

# Définition des couleurs
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}=== Nettoyage du dossier build ===${NC}"
rm -rf build/
if [ $? -eq 0 ]; then
    echo -e "${GREEN}Nettoyage réussi${NC}"
else
    echo -e "${RED}Erreur lors du nettoyage${NC}"
    exit 1
fi

echo -e "\n${YELLOW}=== Création du dossier build ===${NC}"
mkdir -p build
cd build

echo -e "\n${YELLOW}=== Configuration CMake ===${NC}"
cmake ../test -GNinja
if [ $? -eq 0 ]; then
    echo -e "${GREEN}Configuration CMake réussie${NC}"
else
    echo -e "${RED}Erreur lors de la configuration CMake${NC}"
    exit 1
fi

echo -e "\n${YELLOW}=== Compilation ===${NC}"
ninja
if [ $? -eq 0 ]; then
    echo -e "${GREEN}Compilation réussie${NC}"
else
    echo -e "${RED}Erreur lors de la compilation${NC}"
    exit 1
fi

echo -e "\n${YELLOW}=== Exécution des tests ===${NC}"
./dynamicBric_PATO_test
if [ $? -eq 0 ]; then
    echo -e "${GREEN}Tests réussis${NC}"
else
    echo -e "${RED}Échec des tests${NC}"
    exit 1
fi
