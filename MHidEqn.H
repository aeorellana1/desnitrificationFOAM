// === MHidEqn.H: hidrólisis S0→Sb + difusión de biomasa hidrolítica =======
// Evitar divisiones por cero (mismas dimensiones que S0)
const dimensionedScalar tinyS0("tinyS0", S0.dimensions(), SMALL);

// μ_hidr = k1 * S0 / (K1_over_aStar + S0)
volScalarField mu_hid
(
    IOobject("mu_hid", runTime.timeName(), mesh, IOobject::NO_READ, IOobject::NO_WRITE),
    k1 * S0 / (K1_over_aStar + S0 + tinyS0)   // [1/s]
);


// Tasa de hidrólisis volumétrica (kg S0/m3/s), se mantiene igual
volScalarField R_hyd
(
    IOobject("R_hyd", runTime.timeName(), mesh, IOobject::NO_READ, IOobject::AUTO_WRITE),
    mu_hid * Mhid
);

// PDE con difusión:
// dMhid/dt - ∇·(d2 ∇Mhid) = (K0*mu_hid - kd_hid) * Mhid
fvScalarMatrix MHidEqn
(
    fvm::ddt(Mhid)
  - fvm::laplacian(d2, Mhid)
 ==
    (K0*mu_hid - kd_hid) * Mhid
);

MHidEqn.solve();
// ========================================================================
