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
3. [Inicialización de campos](#inicialización-de-campos)
4. [Gestión de fases y geometría](#gestión-de-fases-y-geometría)
5. [Difusión morfológica del biofilm](#difusión-morfológica-del-biofilm)
6. [Modelo de reacciones y transporte (NOx)](#modelo-de-reacciones-y-transporte-nox)
7. [Crecimiento de biomasas](#crecimiento-de-biomasas)
8. [Hidrodinámica y control numérico](#hidrodinámica-y-control-numérico)
9. [Ejecución del solver y caso mínimo](#ejecución-del-solver-y-caso-mínimo)

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
---

## Inicialización de campos

La inicialización de variables se organiza en tres archivos `.H`, incluidos al inicio
del solver principal. Esta separación permite mantener el código modular y facilita
la extensión del modelo.

### `createFields.H` (hidrodinámica y medio poroso)

Inicializa los campos hidrodinámicos estándar:
- presión `p`
- velocidad `U`
- flujo másico `phi`

Además, lee y escribe las propiedades hidráulicas del medio poroso:
- porosidad `porosity`
- permeabilidad `K`
- término de Darcy `darcyTerm`

y crea `phiByPorosity`, utilizado para transporte convectivo en fase fluida.

### `createFieldsBiofilm.H` (fase biológica y azufre sólido)

Inicializa los campos de biomasa y soporte sólido:
- `M`: biomasa desnitrificante (kg/m³)
- `Mhid`: biomasa hidrolítica (kg/m³)
- `S0`: azufre elemental sólido (kg/m³)

Define también magnitudes derivadas utilizadas por otros módulos:
- `Mtot = M + Mhid`
- `Mnorm = Mtot / mmax`

y lee parámetros del micro-continuum (por ejemplo `mmax`, `eps`, `a`, `b`) y parámetros
de hidrólisis (por ejemplo `k1`, `K0`, `kd_hid`, `ySb_S0`, `K1_over_aStar`).

### `createFieldsNOx.H` (especies solubles y parámetros cinéticos)

Inicializa las especies solubles:
- `NO3`, `NO2`
- `Csb` (donador soluble)
- `SO4`
- `N2` (tracker de nitrógeno gaseoso disuelto)

y lee parámetros de transporte y cinética:
- difusividades (`DNO3`, `DNO2`, `Dsb`, `DSO4`, `DN2`)
- constantes cinéticas (`kNO3`, `kNO2`) y de saturación (`KNO3`, `KNO2`, `KSb`)
- rendimientos/estequiometría (`YX_NO3`, `YX_NO2`, `ySb_NO3`, `ySb_NO2`, `ySO4_NO3`, `ySO4_NO2`)
- decaimiento `kd_desn`

### Inclusión en el solver

Los tres archivos se incluyen durante la etapa de preparación del caso:

```cpp
#include "createFieldsBiofilm.H"
#include "createFieldsNOx.H"
#include "createFields.H"
```
---

## Gestión de fases y geometría

El dominio puede estar localmente en tres estados principales:
1. **Pellet de azufre sólido (S₀)**: región de muy baja permeabilidad y porosidad.
2. **Biofilm**: región biológica con permeabilidad reducida y porosidad intermedia.
3. **Fluido libre**: región sin obstrucción (porosidad 1, permeabilidad 1).

La clasificación se realiza en `updateBiofilmPhase.H`, asignando `porosity`, `K` y
`darcyTerm` de acuerdo con umbrales físicos.

### Criterio de pellet (S₀) y biofilm

- Si `S0 > S0phaseOn` → la celda se considera **pellet**.
- Si no hay pellet y `Mnorm > SMALL` → la celda se considera **biofilm**.
- En caso contrario → **fluido libre**.

### Fragmento representativo (`updateBiofilmPhase.H`)

```cpp
const scalar S0thVal = S0phaseOn.value();

forAll (mesh.C(), celli)
{
    const scalar S0Cell    = S0.internalField()[celli];
    const scalar MnormCell = Mnorm.internalField()[celli];

    const bool hasSulfur  = (S0Cell > S0thVal);
    const bool hasBiofilm = (!hasSulfur && MnormCell > SMALL);

    if (hasSulfur)
    {
        // Pellet de S0
        porosity[celli] = sulfurPorosity.value();
        K[celli]        = sulfurPermeability.value();
        darcyTerm[celli]= nu.value()/K[celli];
    }
    else if (hasBiofilm)
    {
        // Biofilm
        porosity[celli] = biofilmPorosity.value();
        K[celli]        = biofilmPermeability.value();
        darcyTerm[celli]= nu.value()/K[celli];
    }
    else
    {
        // Fluido libre
        porosity[celli] = 1.0;
        K[celli]        = 1.0;
        darcyTerm[celli]= 0.0;
    }
}
```


## Difusión morfológica del biofilm

La redistribución espacial de biomasa se modela mediante una difusión morfológica
cuyo coeficiente depende de la biomasa total normalizada:
![Mtot](https://latex.codecogs.com/svg.image?M_%7B%5Ctext%7Btot%7D%7D%20%3D%20M%20%2B%20M_%7B%5Ctext%7Bhid%7D%7D%2C%5Cqquad%20M_%7B%5Ctext%7Bnorm%7D%7D%20%3D%20%5Cfrac%7BM_%7B%5Ctext%7Btot%7D%7D%7D%7Bm_%7B%5Cmax%7D%7D)


El coeficiente difusivo morfológico se define como:

![d2](https://latex.codecogs.com/svg.image?d_%7B2%7D%20%3D%20%5Cleft(%5Cfrac%7B%5Cvarepsilon%7D%7B1%20-%20M_%7B%5Ctext%7Bnorm%7D%7D%7D%5Cright)%5E%7Ba%7D%5C%3B%5Cleft(M_%7B%5Ctext%7Bnorm%7D%7D%5Cright)%5E%7Bb%7D)


donde **ε**, **a** y **b** son parámetros adimensionales del modelo, y  
**mₘₐₓ** corresponde a la densidad máxima de biomasa.

### Implementación (`updateDiffusion.H`)

```cpp
// Biomasa total y normalización
Mtot  = M + Mhid;
Mnorm = Mtot/mmax;

// Proteger singularidades
Mnorm = min(max(Mnorm, SMALL), 1.0 - SMALL);

// Coeficiente difusivo morfológico
volScalarField d2 =
(
    Foam::pow(eps/(1 - Mnorm), a) *
    Foam::pow(Mnorm, b)
)();

// Dimensiones físicas: m^2/s
d2.dimensions().reset(dimensionSet(0,2,-1,0,0,0,0));
```
---

## Modelo de reacciones y transporte (NOx)

El transporte y la reacción de las especies solubles se implementan en
`NOxEqn.H`, considerando desnitrificación autótrofa acoplada al crecimiento
biológico del biofilm y al consumo del donador de electrones soluble \(S_b\).

Las especies consideradas son:
- nitrato (**NO₃⁻**),
- nitrito (**NO₂⁻**),
- azufre soluble (**S_b**),
- sulfato (**SO₄²⁻**),
- nitrógeno gaseoso disuelto (**N₂**).

---

### Cinética de desnitrificación

Las tasas específicas se describen mediante cinéticas tipo Monod,
dependientes del donador de electrones y del aceptor de electrones.

**Factores de saturación**

![fSb](https://latex.codecogs.com/svg.image?f_{Sb}=\frac{c_{Sb}}{K_{Sb}+c_{Sb}})

![fNO3](https://latex.codecogs.com/svg.image?f_{NO_3}=\frac{c_{NO_3}}{K_{NO_3}+c_{NO_3}})

![fNO2](https://latex.codecogs.com/svg.image?f_{NO_2}=\frac{c_{NO_2}}{K_{NO_2}+c_{NO_2}})


**Tasas volumétricas de consumo**

![rNO3raw](https://latex.codecogs.com/svg.image?r_{NO_3}^{raw}=\frac{\mu_{\max,1}f_{Sb}f_{NO_3}X_{desn}}{Y_{X/NO_3}})

![rNO2raw](https://latex.codecogs.com/svg.image?r_{NO_2}^{raw}=\frac{\mu_{\max,2}f_{Sb}f_{NO_2}X_{desn}}{Y_{X/NO_2}})


### Limitación por disponibilidad de donador (γ)

Para evitar que el consumo requerido de \(S_b\) exceda la disponibilidad local
dentro de un paso de tiempo, se introduce un factor limitante \(\gamma\):

![gamma](https://latex.codecogs.com/svg.image?\gamma=\min\left(1,\frac{c_{Sb}+y_{Sb/S0}R_{hyd}\Delta%20t}{(y_{Sb,NO_3}r_{NO_3}^{raw}+y_{Sb,NO_2}r_{NO_2}^{raw})\Delta%20t}\right))



Las tasas efectivas quedan definidas como:

![rNO3](https://latex.codecogs.com/svg.image?r_{NO_3}=\gamma%20r_{NO_3}^{raw})

![rNO2](https://latex.codecogs.com/svg.image?r_{NO_2}=\gamma%20r_{NO_2}^{raw})

### Ecuaciones de transporte

Las ecuaciones de conservación para las especies solubles se escriben como:

**Nitrato (NO₃⁻)**

![NO3](https://latex.codecogs.com/svg.image?\frac{\partial(\varepsilon%20c_{NO_3})}{\partial%20t}-\nabla\cdot(\varepsilon%20D_{NO_3}\nabla%20c_{NO_3})=-r_{NO_3})



**Nitrito (NO₂⁻)**

![NO2](https://latex.codecogs.com/svg.image?\frac{\partial(\varepsilon%20c_{NO_2})}{\partial%20t}-\nabla\cdot(\varepsilon%20D_{NO_2}\nabla%20c_{NO_2})=r_{NO_3}-r_{NO_2})

**Donador soluble (S_b)**

![Sb](https://latex.codecogs.com/svg.image?\frac{\partial(\varepsilon%20c_{Sb})}{\partial%20t}-\nabla\cdot(\varepsilon%20D_{Sb}\nabla%20c_{Sb})=y_{Sb/S0}R_{hyd}-(y_{Sb,NO_3}r_{NO_3}+y_{Sb,NO_2}r_{NO_2}))

**Sulfato (SO₄²⁻)**

![SO4](https://latex.codecogs.com/svg.image?\frac{\partial(\varepsilon%20c_{SO_4})}{\partial%20t}-\nabla\cdot(\varepsilon%20D_{SO_4}\nabla%20c_{SO_4})=y_{SO4,NO_3}r_{NO_3}+y_{SO4,NO_2}r_{NO_2})

**Nitrógeno gaseoso disuelto (N₂)**

![N2](https://latex.codecogs.com/svg.image?\frac{\partial(\varepsilon%20c_{N_2})}{\partial%20t}-\nabla\cdot(\varepsilon%20D_{N_2}\nabla%20c_{N_2})=r_{NO_2})

**Azufre elemental (S⁰)**

![S0](https://latex.codecogs.com/svg.image?\frac{\partial%20S^0}{\partial%20t}=-R_{hyd})


---

## Crecimiento de biomasas

El modelo considera dos poblaciones biológicas:
- **Biomasa desnitrificante** \(M\), responsable del consumo de NO₃⁻ y NO₂⁻.
- **Biomasa hidrolítica** \(M_{hid}\), asociada a la hidrólisis de azufre elemental \(S^0\).

Ambas poblaciones se expresan en unidades de masa por volumen y se redistribuyen
espacialmente mediante el coeficiente de difusión morfológica \(d_2\).

---

### Crecimiento de biomasa desnitrificante

La tasa de crecimiento de la biomasa desnitrificante se acopla directamente a las
tasas efectivas de consumo de NO₃⁻ y NO₂⁻:

![RgM](https://latex.codecogs.com/svg.image?R_{g,M}=Y_{X/NO_3}r_{NO_3}+Y_{X/NO_2}r_{NO_2})

La ecuación de transporte para \(M\) se escribe como:

![MEqn](https://latex.codecogs.com/svg.image?\frac{\partial%20M}{\partial%20t}-\nabla\cdot(d_2\nabla%20M)=R_{g,M}-k_{d,desn}M)

#### Implementación (`MEqn.H`)

```cpp
volScalarField Rg_M
(
    IOobject("Rg_M", runTime.timeName(), mesh, IOobject::NO_READ, IOobject::AUTO_WRITE),
    YX_NO3 * rNO3 + YX_NO2 * rNO2
);

fvScalarMatrix MEqn
(
    fvm::ddt(M)
  - fvm::laplacian(d2, M)
  + fvm::Sp(kd_desn, M)
  == Rg_M
);

MEqn.solve();
```
---

### Crecimiento de biomasa hidrolítica

La biomasa hidrolítica \(M_{hid}\) crece asociada a la hidrólisis del azufre elemental
\(S^0\). La tasa específica de hidrólisis se define como:

![muH](https://latex.codecogs.com/svg.image?\mu_{hid}=\frac{k_1S^0}{K_1/a^*+S^0})

La tasa volumétrica de hidrólisis queda dada por:

![Rhyd](https://latex.codecogs.com/svg.image?R_{hyd}=\mu_{hid}M_{hid})

El término neto de crecimiento de biomasa hidrolítica se expresa como:

![RgH](https://latex.codecogs.com/svg.image?R_{g,hid}=(K_0\mu_{hid}-k_{d,hid})M_{hid})

La ecuación de transporte para \(M_{hid}\) se escribe como:

![MHidEqn](https://latex.codecogs.com/svg.image?\frac{\partial%20M_{hid}}{\partial%20t}-\nabla\cdot(d_2\nabla%20M_{hid})=R_{g,hid})

#### Implementación (`MHidEqn.H`)

```cpp
volScalarField mu_hid
(
    IOobject("mu_hid", runTime.timeName(), mesh, IOobject::NO_READ, IOobject::NO_WRITE),
    k1 * S0 / (K1_over_aStar + S0 + SMALL)
);

volScalarField R_hyd
(
    IOobject("R_hyd", runTime.timeName(), mesh, IOobject::NO_READ, IOobject::AUTO_WRITE),
    mu_hid * Mhid
);

volScalarField Rg_hid
(
    IOobject("Rg_hid", runTime.timeName(), mesh, IOobject::NO_READ, IOobject::AUTO_WRITE),
    (K0 * mu_hid - kd_hid) * Mhid
);

fvScalarMatrix MHidEqn
(
    fvm::ddt(Mhid)
  - fvm::laplacian(d2, Mhid)
  == Rg_hid
);

MHidEqn.solve();

```
---

## Hidrodinámica y control numérico

El campo de velocidades se resuelve mediante las ecuaciones de Navier–Stokes
para flujo incompresible, acopladas al algoritmo PIMPLE. El efecto del biofilm
y del pellet de azufre sólido se introduce mediante un término de resistencia
tipo Darcy, que frena el flujo en regiones de baja permeabilidad.

### Ecuación de momento (UEqn)

La ecuación de cantidad de movimiento se  implementa explícitamente en `UEqn.H` mediante `fvm::Sp(darcyTerm,U)`.

### Ecuación de presión (pEqn)

La presión se resuelve para garantizar la conservación de masa, ajustando el
flujo másico \(\phi\) en el medio poroso y asegurando la continuidad del flujo
incompresible en todo el dominio.

---

### Control adaptativo del paso de tiempo (Δt)

El solver utiliza un control adaptativo del paso de tiempo para mantener la
estabilidad numérica frente a cinéticas rígidas y gradientes fuertes de biomasa
y especies químicas.

El paso de tiempo está limitado por:
- el criterio de Courant,
- la evolución temporal de la biomasa desnitrificante \(M\),
- la evolución de \(NO_3^-\) y \(NO_2^-\),
- opcionalmente el campo legado \(C\).

Este control se implementa mediante el componente `timestepManager`, heredado
del solver `biofilmFoam`, y configurado en los archivos `readTimeControls.H`
y `setDeltaT.H`.

Para cada variable controlada se define un error de truncamiento permitido
(`truncationError_*`), a partir del cual se estima un paso de tiempo máximo
compatible con la precisión deseada.

El paso de tiempo final se selecciona como el mínimo entre:
- el límite impuesto por Courant,
- el límite impuesto por cada gestor temporal,
- un factor de crecimiento máximo entre pasos consecutivos.

Este enfoque permite capturar correctamente transitorios rápidos (por ejemplo,
picos de nitrito) sin comprometer la eficiencia computacional.

---

## Ejecución del solver y caso mínimo

El solver `desnitrificationFoam` se ejecuta dentro de un caso estándar de
OpenFOAM, compuesto por las carpetas `0/`, `constant/` y `system/`.

### Archivos requeridos

- `0/`: campos iniciales (`U`, `p`, `M`, `Mhid`, `NO3`, `NO2`, `Csb`, `SO4`, `N2`, `S0`, `porosity`, `K`)
- `constant/transportProperties`: parámetros físicos, cinéticos y estequiométricos
- `system/controlDict`: control temporal y activación de opciones (`useC`, `advectSolutes`)
- `system/fvSchemes`, `system/fvSolution`: esquemas numéricos y solvers lineales

