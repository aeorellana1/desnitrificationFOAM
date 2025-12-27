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
```
### Carga del entorno

Una vez descargado OpenFOAM, se debe cargar el entorno de compilación:

```bash
source ~/OpenFOAM-2406/etc/bashrc
```
### Compilación base

Para verificar que el entorno está correctamente configurado, se recomienda
compilar al menos un solver o librería usando:

```bash
wmake
```
