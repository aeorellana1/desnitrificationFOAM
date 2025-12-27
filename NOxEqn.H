// ==== NOxEqn.H — Hito4-R7c: explícito + limitación por donador + integrales ====

// Pequeños epsilons y constantes con dimensiones correctas
const dimensionedScalar epsNO3("epsNO3", NO3.dimensions(), SMALL);
const dimensionedScalar epsNO2("epsNO2", NO2.dimensions(), SMALL);
const dimensionedScalar epsSb ("epsSb",  Csb.dimensions(), SMALL);
const dimensionedScalar epsS  ("epsS",   Csb.dimensions(), SMALL);
const dimensionedScalar one   ("one",    dimless,          1.0);

// Δt con dimensiones de tiempo (s)
const dimensionedScalar dtDim("dt", dimTime, runTime.deltaTValue());

// Ceros con dimensión adecuada para recorte de positividad
const dimensionedScalar ZERO_NO3("ZERO_NO3", NO3.dimensions(), 0);
const dimensionedScalar ZERO_NO2("ZERO_NO2", NO2.dimensions(), 0);
const dimensionedScalar ZERO_S  ("ZERO_S",   Csb.dimensions(), 0);
const dimensionedScalar ZERO_SO4("ZERO_SO4", SO4.dimensions(), 0);
const dimensionedScalar ZERO_S0 ("ZERO_S0",  S0.dimensions(),  0);
const dimensionedScalar ZERO_N2 ("ZERO_N2",  N2.dimensions(),  0);
// Positividad
volScalarField NO3pos = max(NO3, ZERO_NO3);
volScalarField NO2pos = max(NO2, ZERO_NO2);
volScalarField SbPos  = max(Csb, ZERO_S);

// Factores de Monod (usar Csb, no C)
volScalarField fSb  = SbPos /(KSb  + SbPos  + epsSb);
volScalarField fNO3 = NO3pos/(KNO3 + NO3pos + epsNO3);
volScalarField fNO2 = NO2pos/(KNO2 + NO2pos + epsNO2);

// Tasas "crudas" explícitas (kg N / m3 / s), crecen con Mdesn (=M)
volScalarField rNO3_raw
(
    IOobject("rNO3_raw", runTime.timeName(), mesh, IOobject::NO_READ, IOobject::NO_WRITE),
    (kNO3 * fSb * fNO3 * Mdesn)/YX_NO3
);
volScalarField rNO2_raw
(
    IOobject("rNO2_raw", runTime.timeName(), mesh, IOobject::NO_READ, IOobject::NO_WRITE),
    (kNO2 * fSb * fNO2 * Mdesn)/YX_NO2
);

// ---- Limitación por donador (base S) ----
// Demanda de Sb del paso (kg S/m3) y Oferta del paso (kg S/m3)
volScalarField demandSb =
    (ySb_NO3 * rNO3_raw + ySb_NO2 * rNO2_raw) * dtDim;

volScalarField supplySb =
    SbPos + ySb_S0 * R_hyd * dtDim;

// Factor limitante gamma (adimensional)
volScalarField gamma
(
    IOobject("gamma", runTime.timeName(), mesh, IOobject::NO_READ, IOobject::AUTO_WRITE),
    min(one, supplySb / (demandSb + epsS))
);

// Tasas efectivas limitadas por donador
volScalarField rNO3
(
    IOobject("rNO3", runTime.timeName(), mesh, IOobject::NO_READ, IOobject::NO_WRITE),
    gamma * rNO3_raw
);
volScalarField rNO2
(
    IOobject("rNO2", runTime.timeName(), mesh, IOobject::NO_READ, IOobject::NO_WRITE),
    gamma * rNO2_raw
);

// ===== Integrales acumuladas (kg) para diagnóstico =====
// Nota: acumulamos con integrales de tasa volumétrica * dt.
// Se guardan como estáticos para persistir durante la corrida.
static scalar A_NO3kg = 0.0; // consumo integrado por vía NO3 (kg N)
static scalar A_NO2kg = 0.0; // consumo integrado por vía NO2 (kg N)
static scalar A_Xkg   = 0.0; // producción integrada de X_desn (kg X)
static scalar A_N2kg  = 0.0; // producción integrada de N2 (kg N)
// incrementos del paso (kg)
const scalar dNO3_step =
    fvc::domainIntegrate(rNO3).value() * runTime.deltaTValue();

const scalar dNO2_step =
    fvc::domainIntegrate(rNO2).value() * runTime.deltaTValue();

const scalar dX_step =
    fvc::domainIntegrate(YX_NO3*rNO3 + YX_NO2*rNO2).value()
  * runTime.deltaTValue();

const scalar dN2_step =
    fvc::domainIntegrate(rNO2).value() * runTime.deltaTValue();
// acumular
A_NO3kg += dNO3_step;
A_NO2kg += dNO2_step;
A_Xkg   += dX_step;
A_N2kg  += dN2_step;
// --- Diagnóstico en cada writeInterval: gamma, balances y acumulados ---
if (runTime.outputTime())
{
    // Paso: oferta/demanda de Sb en kg (ya están en kg/m3 -> integrar a kg)
    const dimensionedScalar demKg = fvc::domainIntegrate(demandSb);
    const dimensionedScalar supKg = fvc::domainIntegrate(supplySb);

    Info<< "gamma[min,max]=[" << gMin(gamma) << "," << gMax(gamma) << "] "
        << "Sb_demand_step=" << demKg.value() << " kgS "
        << "Sb_supply_step=" << supKg.value() << " kgS" << nl;

    // Totales de dominio para balances (en kg)
    const dimensionedScalar S0_sum  = fvc::domainIntegrate(S0);
    const dimensionedScalar Sb_sum  = fvc::domainIntegrate(Csb);
    const dimensionedScalar SO4_sum = fvc::domainIntegrate(SO4);
    const dimensionedScalar N3_sum  = fvc::domainIntegrate(NO3);
    const dimensionedScalar N2_sum  = fvc::domainIntegrate(NO2);
    const dimensionedScalar M_sum   = fvc::domainIntegrate(M);
    const dimensionedScalar Mh_sum  = fvc::domainIntegrate(Mhid);
    const dimensionedScalar N2gas_sum = fvc::domainIntegrate(N2);
    Info<< "TOT (kg): S0="  << S0_sum.value()
        << ", Sb="  << Sb_sum.value()
        << ", SO4=" << SO4_sum.value()
        << ", NO3=" << N3_sum.value()
        << ", NO2=" << N2_sum.value()
        << ", N2="  << N2gas_sum.value()
        << ", M="   << M_sum.value()
        << ", Mhid="<< Mh_sum.value()
        << nl;

    // Línea de integrales acumuladas para comparar con AWK
    Info<< "INT (kg): NO3=" << A_NO3kg
        << ", NO2=" << A_NO2kg
        << ", N2="  << A_N2kg
        << ", Xdesn=" << A_Xkg
        << nl;
}

// =================== NO3 (consumo) ===================
// ======= NO3 =======
fvScalarMatrix NO3Eqn
(
    fvm::ddt(porosity, NO3)
  - fvm::laplacian(DNO3*porosity, NO3)
 == - rNO3
);

// HITO4: advección sólo en fase fluido (se apaga en biofilm con 'porosity')
if (advectSolutes)
{
    NO3Eqn += porosity * fvm::div(phiByPorosity, NO3);
}

NO3Eqn.solve();
NO3.correctBoundaryConditions();
NO3 = Foam::max(NO3, ZERO_NO3);

// =================== NO2 (prod - cons) ===============
fvScalarMatrix NO2Eqn
(
    fvm::ddt(porosity, NO2)
  - fvm::laplacian(DNO2*porosity, NO2)
 ==  rNO3 - rNO2
);
// HITO4: advección solo en fase fluido (se apaga en biofilm con 'porosity')
if (advectSolutes)
{
    NO2Eqn += porosity * fvm::div(phiByPorosity, NO2);
}
NO2Eqn.solve();
NO2.correctBoundaryConditions();
NO2 = max(NO2, ZERO_NO2);

// =================== Csb (Sb) ========================
// + hidrólisis – consumos por ambas vías (USANDO rNOx EFECTIVOS)
fvScalarMatrix CsbEqn
(
    fvm::ddt(porosity, Csb)
  - fvm::laplacian(Dsb*porosity, Csb)
 ==  ySb_S0 * R_hyd
   - (ySb_NO3 * rNO3 + ySb_NO2 * rNO2)
);
// HITO4: advección solo en fase fluido (se apaga dentro de biofilm con 'porosity')
if (advectSolutes)
{
    CsbEqn += porosity * fvm::div(phiByPorosity, Csb);
}
CsbEqn.solve();
Csb.correctBoundaryConditions();
Csb = max(Csb, ZERO_S);

// =================== SO4 (base S) ====================
fvScalarMatrix SO4Eqn
(
    fvm::ddt(porosity, SO4)
  - fvm::laplacian(DSO4*porosity, SO4)
 ==  ySO4_NO3 * rNO3 + ySO4_NO2 * rNO2
);
// HITO4: advección solo en fase fluido (se apaga dentro de biofilm con 'porosity')
if (advectSolutes)
{
    SO4Eqn += porosity * fvm::div(phiByPorosity, SO4);
}

SO4Eqn.solve();
SO4.correctBoundaryConditions();
SO4 = max(SO4, ZERO_SO4);

// =================== N2 (nitrógeno gaseoso disuelto) ======================
fvScalarMatrix N2Eqn
(
    fvm::ddt(porosity, N2)
  - fvm::laplacian(DN2*porosity, N2)
 ==  rNO2       // producción de N en forma N2 por consumo de NO2
);

// HITO4: advección solo en fase fluido (se apaga dentro de biofilm con 'porosity')
if (advectSolutes)
{
    N2Eqn += porosity * fvm::div(phiByPorosity, N2);
}

N2Eqn.solve();
N2.correctBoundaryConditions();
N2 = max(N2, ZERO_N2);



// =================== S0 (sólido inmóvil) =============
fvScalarMatrix S0Eqn
(
    fvm::ddt(S0) == - R_hyd
);
S0Eqn.solve();
S0.correctBoundaryConditions();
S0 = max(S0, ZERO_S0);

// (Opcional) monitores de derivadas normalizadas
ddtNO3 = gMax(mag(fvc::ddt(NO3)())())/(gMax(NO3)+SMALL);
ddtNO2 = gMax(mag(fvc::ddt(NO2)())())/(gMax(NO2)+SMALL);
ddtSb  = gMax(mag(fvc::ddt(Csb)())())/(gMax(Csb)+SMALL);
ddtSO4 = gMax(mag(fvc::ddt(SO4)())())/(gMax(SO4)+SMALL);
ddtS0  = gMax(mag(fvc::ddt(S0)()) ())/(gMax(S0) +SMALL);
ddtN2  = gMax(mag(fvc::ddt(N2)()) ())/(gMax(N2) +SMALL);
// ---- Monitor de la biomasa total, análogo al del solver original ----
// Actualizamos Mtot para este paso
Mtot = M + Mhid;

// Monitor al estilo del solver original pero con Mtot
dimensionedScalar ddtMtot
(
    "ddtMtot", dimless,
    gMax(mag(fvc::ddt(Mtot)())())/(gMax(Mtot) + SMALL)
);

if (runTime.outputTime())
{
    Info<< "Monitores biofilm: ddtMtot=" << ddtMtot.value()
        << ", ddtMdesn=" << ddtMdesn.value()
        << ", ddtMhid="  << ddtMhid.value() << nl;
}
// ==== fin NOxEqn.H =========================================================

