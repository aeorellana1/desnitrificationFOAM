// === createFieldsNOx.H  (Hito4-R2) ===

Info << "Reading field NO3\n" << endl;
volScalarField NO3
(
    IOobject("NO3", runTime.timeName(), mesh, IOobject::MUST_READ, IOobject::AUTO_WRITE),
    mesh
);

Info << "Reading field NO2\n" << endl;
volScalarField NO2
(
    IOobject("NO2", runTime.timeName(), mesh, IOobject::MUST_READ, IOobject::AUTO_WRITE),
    mesh
);

Info << "Reading field Csb (soluble sulfur donor)\n" << endl;
volScalarField Csb
(
    IOobject("Csb", runTime.timeName(), mesh, IOobject::MUST_READ, IOobject::AUTO_WRITE),
    mesh
);

Info << "Reading field SO4 (sulfate)\n" << endl;
volScalarField SO4
(
    IOobject("SO4", runTime.timeName(), mesh, IOobject::MUST_READ, IOobject::AUTO_WRITE),
    mesh
);

Info << "Reading field N2 (gaseous nitrogen tracker)\n" << endl;
volScalarField N2
(
    IOobject("N2", runTime.timeName(), mesh, IOobject::MUST_READ, IOobject::AUTO_WRITE),
    mesh
);

// -------- Monitors (opcionales) --------
dimensionedScalar ddtNO3("ddtNO3", dimensionSet(0,0,0,0,0,0,0), 0.);
dimensionedScalar ddtNO2("ddtNO2", dimensionSet(0,0,0,0,0,0,0), 0.);
dimensionedScalar ddtSb ("ddtSb",  dimensionSet(0,0,0,0,0,0,0), 0.);
dimensionedScalar ddtSO4("ddtSO4", dimensionSet(0,0,0,0,0,0,0), 0.);
dimensionedScalar ddtS0 ("ddtS0",  dimensionSet(0,0,0,0,0,0,0), 0.);
dimensionedScalar ddtN2("ddtN2", dimensionSet(0,0,0,0,0,0,0), 0.);
// -------- Parámetros de transporte/kinética existentes (obligatorios) --------
Info << "Reading NOx transport/kinetics\n" << endl;
dimensionedScalar DNO3 ("DNO3",  dimensionSet(0, 2, -1, 0, 0, 0, 0), transportProperties);
dimensionedScalar DNO2 ("DNO2",  dimensionSet(0, 2, -1, 0, 0, 0, 0), transportProperties);
dimensionedScalar Dsb  ("Dsb",   dimensionSet(0, 2, -1, 0, 0, 0, 0), transportProperties);
dimensionedScalar DSO4 ("DSO4",  dimensionSet(0, 2, -1, 0, 0, 0, 0), transportProperties);
dimensionedScalar DN2  ("DN2", dimensionSet(0, 2, -1, 0, 0, 0, 0), transportProperties);
dimensionedScalar kNO3 ("kNO3",  dimensionSet(0, 0, -1, 0, 0, 0, 0), transportProperties); // 1/s
dimensionedScalar KNO3 ("KNO3",  dimensionSet(1, -3,  0, 0, 0, 0, 0), transportProperties); // kg/m^3

dimensionedScalar kNO2 ("kNO2",  dimensionSet(0, 0, -1, 0, 0, 0, 0), transportProperties); // 1/s
dimensionedScalar KNO2 ("KNO2",  dimensionSet(1, -3,  0, 0, 0, 0, 0), transportProperties); // kg/m^3

dimensionedScalar KSb  ("KSb",   dimensionSet(1, -3,  0, 0, 0, 0, 0), transportProperties); // kg/m^3

// Yields/estequiometría (existentes)
dimensionedScalar YX_NO3("YX_NO3", dimless, transportProperties); // crecimiento X_desn por NO3
dimensionedScalar YX_NO2("YX_NO2", dimless, transportProperties); // crecimiento X_desn por NO2
dimensionedScalar ySO4_NO3("ySO4_NO3", dimless, transportProperties);
dimensionedScalar ySO4_NO2("ySO4_NO2", dimless, transportProperties);
dimensionedScalar ySb_NO3 ("ySb_NO3",  dimless, transportProperties);
dimensionedScalar ySb_NO2 ("ySb_NO2",  dimless, transportProperties);

// Decaimiento (existente: kd general). Creamos alias específico para X_desn.
dimensionedScalar kd    ("kd",     dimensionSet(0,0,-1,0,0,0,0), transportProperties);
dimensionedScalar kd_desn
(
    transportProperties.lookupOrDefault<dimensionedScalar>
    (
        "kd_desn", dimensionedScalar("kd_desn", kd.dimensions(), kd.value())
    )
);



// === end createFieldsNOx.H (Hito4-R2) ===
