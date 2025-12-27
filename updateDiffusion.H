// Actualizar campos Mtot y Mnorm
Mtot = M + Mhid;
Mnorm = Mtot/mmax;

// Proteger singularidades
Mnorm = min(max(Mnorm, SMALL), 1.0 - SMALL);

// d2 como en solver original, usando Mnorm adimensional
volScalarField d2 =
(
    Foam::pow(eps/(1 - Mnorm), a) *
    Foam::pow(Mnorm, b)
)();

// Dimensiones físicas m^2/s
d2.dimensions().reset(dimensionSet(0,2,-1,0,0,0,0));

// Piso numérico
d2 = max(d2, dimensionedScalar("d2Floor", dimensionSet(0,2,-1,0,0,0,0), 1e-20));

// En caras
surfaceScalarField d2f( fvc::interpolate(d2, "d2") );


