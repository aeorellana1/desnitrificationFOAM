fvScalarMatrix CEqn
(
    fvm::ddt(porosity,C)
    + porosity * fvm::div(phiByPorosity, C)
    - fvm::laplacian(d1*porosity, C)
    ==
    - fvm::Sp(dimensionedScalar("zeroC", dimensionSet(0,0,-1,0,0,0,0), 0), C)
);

solve(CEqn);

ddtC = gMax(mag(fvc::ddt(C)())())/(gMax(C)+SMALL);



