// === createFieldsBiofilm.H  (Hito4-R2 - corregido) ===

// --- C (del solver original). Se puede desactivar con useC en controlDict.
Info << "Reading field C\n" << endl;

volScalarField C
(
    IOobject
    (
        "C",
        runTime.timeName(),
        mesh,
        IOobject::MUST_READ,
        IOobject::AUTO_WRITE
    ),
    mesh
);

// Monitores (opcionales)
dimensionedScalar ddtC("ddtC", dimensionSet(0,0,0,0,0,0,0), 0.);

// === BIOMASAS ===
// M del solver base se interpreta como biomasa desnitrificante X_desn
Info << "Reading field M (denitrifying biomass)\n" << endl;

volScalarField M
(
    IOobject
    (
        "M",
        runTime.timeName(),
        mesh,
        IOobject::MUST_READ,
        IOobject::AUTO_WRITE
    ),
    mesh
);

// Alias por claridad
volScalarField& Mdesn = M;

// Biomasa hidrolítica X_hid para S0 -> Sb (nueva en Hito4)
Info << "Reading field Mhid (hydrolytic biomass)\n" << endl;

volScalarField Mhid
(
    IOobject
    (
        "Mhid",
        runTime.timeName(),
        mesh,
        IOobject::MUST_READ,     // requiere 0/Mhid
        IOobject::AUTO_WRITE
    ),
    mesh
);

// Monitores de biomasa (opcionales)
dimensionedScalar ddtMdesn("ddtMdesn", dimensionSet(0,0,0,0,0,0,0), 0.);
dimensionedScalar ddtMhid ("ddtMhid",  dimensionSet(0,0,0,0,0,0,0), 0.);

// Azufre elemental S0 (estado — sin advección)
Info << "Reading field S0 (elemental sulfur)\n" << endl;

volScalarField S0
(
    IOobject
    (
        "S0",
        runTime.timeName(),
        mesh,
        IOobject::MUST_READ,     // requiere 0/S0
        IOobject::AUTO_WRITE
    ),
    mesh
);

// --- Parámetros del micro-continuum del solver base
Info << "Reading transportProperties\n" << endl;

IOdictionary transportProperties
(
    IOobject
    (
        "transportProperties",
        runTime.constant(),
        mesh,
        IOobject::MUST_READ_IF_MODIFIED,
        IOobject::NO_WRITE
    )
);

// dimensiones: kg | m | s | K | mol | A | cd
Info << "Reading parameter d1 \n" << endl;
dimensionedScalar d1 ("d1", dimensionSet(0, 2, -1, 0, 0, 0, 0), transportProperties);

Info << "Reading parameter mmax \n" << endl;
dimensionedScalar mmax ("mmax", dimensionSet(1, -3, 0, 0, 0, 0, 0), transportProperties);

// --------------------------------------------------------------------
// Campo adimensional de biomasa total normalizada:
// Mnorm = (M + Mhid)/mmax   (0 ~ sin biomasa, 1 ~ biomasa al máximo)
// --------------------------------------------------------------------
volScalarField Mnorm
(
    IOobject
    (
        "Mnorm",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,     // no se lee desde disco
        IOobject::AUTO_WRITE   // se escribe en cada time-step
    ),
    (M + Mhid)/mmax            // adimensional
);


Info << "Reading sulfur density rhoS0 \n" << endl;
dimensionedScalar rhoS0
(
    "rhoS0",
    dimensionSet(1,-3,0,0,0,0,0),
    transportProperties
);

Info << "Reading parameter c0 \n" << endl;
dimensionedScalar c0 ("c0", dimensionSet(1, -3, 0, 0, 0, 0, 0), transportProperties);

Info << "Reading parameter mum \n" << endl;
dimensionedScalar mum ("mum", dimensionSet(0, 0, -1, 0, 0, 0, 0), transportProperties);

Info << "Reading parameter Yxs \n" << endl;
dimensionedScalar Yxs ("Yxs", dimensionSet(0, 0, 0, 0, 0, 0, 0), transportProperties);

Info << "Reading parameter ms \n" << endl;
dimensionedScalar ms ("ms", dimensionSet(0, 0, -1, 0, 0, 0, 0), transportProperties);

Info << "Reading parameter Ks \n" << endl;
dimensionedScalar Ks ("Ks", dimensionSet(1, -3, 0, 0, 0, 0, 0), transportProperties);

Info << "Reading parameter eps \n" << endl;
dimensionedScalar eps ("eps", dimensionSet(0, 0, 0, 0, 0, 0, 0), transportProperties);

Info << "Reading parameter a \n" << endl;
dimensionedScalar a ("a", dimensionSet(0, 0, 0, 0, 0, 0, 0), transportProperties);

Info << "Reading parameter b \n" << endl;
dimensionedScalar b ("b", dimensionSet(0, 0, 0, 0, 0, 0, 0), transportProperties);

// Derivados (legado) para el modelo base
dimensionedScalar k1_legacy = mum/Yxs+ms;
dimensionedScalar k2_legacy = Ks;
dimensionedScalar k3_legacy = Yxs/mmax;
dimensionedScalar k4_legacy = ms*mmax;

dimensionedScalar K1_legacy = mmax*k1_legacy/c0;
dimensionedScalar K2_legacy = k2_legacy;
dimensionedScalar K3_legacy = mmax*k3_legacy*k1_legacy;
dimensionedScalar K4_legacy = k3_legacy*k4_legacy;

// --- Biofilm phase bookkeeping (sin cambios)
volScalarField biofilmPhase
(
    IOobject
    (
        "biofilmPhase",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    mesh,
    dimensionedScalar ("biofilmPhase", dimensionSet(0,0,0,0,0,0,0), 0.),
    "zeroGradient"
);

Info << "Reading biofilm permeability\n" << endl;
dimensionedScalar biofilmPermeability
(
    transportProperties.lookupOrDefault<dimensionedScalar>
    (
        "biofilmPermeability",
        dimensionedScalar("biofilmPermeability", dimLength*dimLength, 1e-12)
    )
);

Info << "Reading biofilm porosity\n" << endl;
dimensionedScalar biofilmPorosity
(
    transportProperties.lookupOrDefault<dimensionedScalar>
    (
        "biofilmPorosity",
        dimensionedScalar("biofilmPorosity", dimless, 1)
    )
);


// --- NUEVO: parámetros hidráulicos para el lecho de S0 (pellets de azufre)

Info << "Reading sulfur permeability (S0 pellets)\n" << endl;
dimensionedScalar sulfurPermeability
(
    transportProperties.lookupOrDefault<dimensionedScalar>
    (
        "sulfurPermeability",
        dimensionedScalar("sulfurPermeability", dimLength*dimLength, 1e-14)
    )
);

Info << "Reading sulfur porosity (S0 pellets)\n" << endl;
dimensionedScalar sulfurPorosity
(
    transportProperties.lookupOrDefault<dimensionedScalar>
    (
        "sulfurPorosity",
        dimensionedScalar("sulfurPorosity", dimless, 0.1)
    )
);

Info << "Reading threshold S0phaseOn (pellet cutoff)\n" << endl;

dimensionedScalar S0phaseOn
(
    transportProperties.lookupOrDefault<dimensionedScalar>
    (
        "S0phaseOn",
        dimensionedScalar("S0phaseOn", dimMass/dimVolume, 1e-3)  // valor razonable
    )
);



// --- HIDRÓLISIS S0 -> Sb (Huiliñir 2020/2024) ---
// μ_hid = k1 * S0 / ( K1_over_aStar + S0 )
// Nota: K1_over_aStar ya incluye el a* en el denominador del paper (K1/a*).

dimensionedScalar k1    ("k1",    dimensionSet(0, 0, -1, 0, 0, 0, 0), transportProperties); // 1/s
dimensionedScalar aStar ("aStar", dimless,                                  transportProperties); // -
/* Rendimiento de crecimiento hidrolítico (K0 del paper): adimensional (kgX/kgS) */
dimensionedScalar K0    ("K0",    dimless,                                   transportProperties); // -
/* Decaimiento X_hid */
dimensionedScalar kd_hid   ("kd_hid",   dimensionSet(0,0,-1,0,0,0,0), transportProperties);
/* Este factor suele ser 1.0 (todo S0 hidrolizado pasa a Sb) */
dimensionedScalar ySb_S0   ("ySb_S0",   dimless, transportProperties);

/* Constante compuesta K1/a* expresada como concentración (kg/m3) */
dimensionedScalar K1_over_aStar
(
    transportProperties.lookupOrDefault<dimensionedScalar>
    (
        "K1_over_aStar",
        dimensionedScalar("K1_over_aStar", dimMass/dimVolume, 1e-6) // valor por defecto prudente
    )
);

// --- Biomasa total (útil para porosidad/difusión)
volScalarField Mtot
(
    IOobject
    (
        "Mtot",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    Mdesn + Mhid
);

// === end createFieldsBiofilm.H (Hito4-R2) ===
