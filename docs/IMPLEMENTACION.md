# Implementación del solver `desnitrificationFoam`

Este documento describe:
- la estructura del solver,
- los archivos modulares `.H`,
- la formulación matemática del modelo,
- y los aspectos numéricos e hidrodinámicos.

---

## Contenidos
1. [Instalación y preparación del entorno](#instalación-y-preparación-del-entorno)

---

## Instalación y preparación del entorno
### Descarga de OpenFOAM

La implementación se desarrolló sobre **OpenFOAM v2406**.  
El código fuente se descarga directamente desde el repositorio oficial de OpenFOAM:

```bash
git clone --branch OpenFOAM-v2406 \
    https://develop.openfoam.com/Development/openfoam.git \
    ~/OpenFOAM-2406
