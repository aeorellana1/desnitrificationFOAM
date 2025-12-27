// crecimiento consistente con NOxEqn (tasas EFECTIVAS)
volScalarField Rg_M
(
    IOobject("Rg_M", runTime.timeName(), mesh, IOobject::NO_READ, IOobject::AUTO_WRITE),
    YX_NO3 * rNO3 + YX_NO2 * rNO2
);

fvScalarMatrix MEqn
(
    fvm::ddt(M)
  - fvm::laplacian(d2, M, "laplacian(d2,M)")
  + fvm::Sp(kd_desn, M)
  == Rg_M
);
MEqn.relax();
MEqn.solve();

// --- DEBUG R9 ---
Info<< "MEqn R9 activo. "
    << "Rg_M[min,max]=[" << gMin(Rg_M) << "," << gMax(Rg_M) << "] "
    << "M[min,max]=["    << gMin(M)    << "," << gMax(M)    << "]\n";


