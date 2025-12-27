# Implementación del solver `desnitrificationFoam`

Este documento describe:
- la estructura del solver,
- los archivos modulares `.H`,
- la formulación matemática del modelo,
- y los aspectos numéricos e hidrodinámicos.

---

## Contenidos
1. [Instalación y preparación del entorno](#instalación-y-preparación-del-entorno)
2. [Estructura general del solver](#estructura-general-del-solver)

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
### Copia del solver base

Con el fin de preservar los archivos originales de OpenFOAM, se realiza una copia del
solver `pimpleFoam`, que servirá como base para la implementación del solver
`desnitrificationFoam`.

Se debe modificar el archivo `Make/files` del nuevo solver para cambiar el nombre del
ejecutable:

```makefile
desnitrificationFoam.C
EXE = $(FOAM_USER_APPBIN)/desnitrificationFoam
```

### Inclusión de la librería `NumericalMethods`

El solver requiere la librería `NumericalMethods`, la cual contiene el componente
`timestepManager`. Esta librería se extrae desde el solver `biofilmFoam`, ubicada en ~/biofilmFoam/libraries/numericalMethods/

### Configuración de `Make/options`

Para enlazar la librería `NumericalMethods` (y poder usar `timestepManager`), se debe
incluir su ruta de headers y el linkeo en el archivo `Make/options` del solver.

```bash
cat > ~/desnitrificationFoam/Make/options << 'EOF'
EXE_INC = \
    -I$(LIB_SRC)/finiteVolume \
    -I$(LIB_SRC)/finiteVolume/lnInclude \
    -I$(LIB_SRC)/meshTools/lnInclude \
    -I$(LIB_SRC)/sampling/lnInclude \
    -I$(LIB_SRC)/TurbulenceModels/turbulenceModels/lnInclude \
    -I$(LIB_SRC)/TurbulenceModels/incompressible/lnInclude \
    -I$(LIB_SRC)/transportModels \
    -I$(LIB_SRC)/transportModels/incompressible/singlePhaseTransportModel \
    -I$(LIB_SRC)/dynamicMesh/lnInclude \
    -I$(LIB_SRC)/dynamicFvMesh/lnInclude \
    -I../../libraries/numericalMethods/lnInclude

EXE_LIBS = \
    -lfiniteVolume \
    -lfvOptions \
    -lmeshTools \
    -lsampling \
    -lturbulenceModels \
    -lincompressibleTurbulenceModels \
    -lincompressibleTransportModels \
    -ldynamicMesh \
    -ldynamicFvMesh \
    -ltopoChangerFvMesh \
    -latmosphericModels \
    -L$(FOAM_USER_LIBBIN) \
    -lNumericalMethods
EOF
```
---

## Estructura general del solver

El solver `desnitrificationFoam` está construido de forma modular. El archivo principal
`desnitrificationFoam.C` contiene el bucle de tiempo, coordina el algoritmo PIMPLE y
**incluye** archivos `.H` que encapsulan tareas específicas (creación de campos, ecuaciones,
actualización de fases, etc.).

### Archivo maestro `desnitrificationFoam.C`

Responsabilidades principales:
- inicialización del caso y lectura de controles,
- bucle temporal y control adaptativo de `Δt`,
- acoplamiento hidrodinámico mediante PIMPLE,
- ejecución secuencial de ecuaciones de transporte y crecimiento,
- escritura de resultados en cada `writeInterval`.

### Orden de ejecución dentro del bucle de tiempo

En cada paso de tiempo, la secuencia de cálculo sigue el orden:

1. Criterio de Courant y ajuste de `Δt`
2. Resolución hidrodinámica (PIMPLE: `UEqn` + `pEqn`)
3. (Opcional) transporte del campo legado `C` (`useC`)
4. Actualización de difusión morfológica (`updateDiffusion.H`)
5. Ecuación de biomasa hidrolítica (`MHidEqn.H`)
6. Transporte y reacción de especies solubles (`NOxEqn.H`)
7. Ecuación de biomasa desnitrificante (`MEqn.H`)
8. Diagnóstico de interfaz biomasa–S₀ (`updateInterfaceMS.H`)
9. Actualización de fases y propiedades hidráulicas (`updateBiofilmPhase.H`)

### Inclusión modular en el `main`

A continuación se muestra el fragmento del bucle principal donde se incluyen los módulos
en el orden de ejecución:

```cpp
while (runTime.run())
{
    #include "CourantNo.H"
    #include "setDeltaT.H"

    ++runTime;

    #include "pimple.H"

    if (useC)
    {
        #include "CEqn.H"
    }

    #include "updateDiffusion.H"
    #include "MHidEqn.H"
    #include "NOxEqn.H"
    #include "MEqn.H"
    #include "updateInterfaceMS.H"
    #include "updateBiofilmPhase.H"

    runTime.write();
}

```
