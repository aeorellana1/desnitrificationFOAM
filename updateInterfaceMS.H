// === updateInterfaceMS.H: campo diagnóstico chi_MS ===================
// chi_MS indica quién domina localmente:
//  - chi_MS ~ 0   -> domina S0
//  - chi_MS ~ 1   -> domina biomasa (Mtot)
//  - chi_MS ~ 0.5 -> zona de interfaz S0 - biofilm

// Biomasa total (ya definido en createFieldsBiofilm.H):
// volScalarField Mtot = M + Mhid;

// Epsilon con mismas dimensiones que Mtot para evitar división por cero
const dimensionedScalar epsMS("epsMS", Mtot.dimensions(), SMALL);

// Factor físico alpha_MS = mmax / rhoS0 (adimensional)
dimensionedScalar alphaMS
(
    "alphaMS",
    dimless,
    mmax.value()/rhoS0.value()
);

// Denominador: Mtot + alpha_MS * S0  ≈ masa de biofilm + masa equivalente de S0
volScalarField denomMS
(
    IOobject("denomMS", runTime.timeName(), mesh, IOobject::NO_READ, IOobject::NO_WRITE),
    Mtot + alphaMS*S0 + epsMS
);

// Campo diagnóstico chi_MS (adimensional, fracción volumétrica relativa)
volScalarField chi_MS
(
    IOobject
    (
        "chi_MS",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    Mtot/denomMS
);

// Acotar chi_MS en [0, 1]
chi_MS = min
(
    max(chi_MS, dimensionedScalar("zero", dimless, 0.0)),
    dimensionedScalar("one",  dimless, 1.0)
);

// (Opcional) log a consola
if (runTime.outputTime())
{
    Info<< "chi_MS[min,max]=[" << gMin(chi_MS) << "," << gMax(chi_MS) << "]" << nl;
}

// === fin updateInterfaceMS.H ========================================
