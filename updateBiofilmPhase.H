// === updateBiofilmPhase.H — modo ORIGINAL usando Mnorm + manejo de S0 ====

// Atajos a los campos internos
scalarField&       MField     = M.primitiveFieldRef();
scalarField&       MhidField  = Mhid.primitiveFieldRef();
const scalarField& S0Field    = S0.internalField();       // solo lectura
const scalarField& MnormField = Mnorm.internalField();    // solo lectura

scalarField&       KField     = K.primitiveFieldRef();
scalarField&       epsField   = porosity.primitiveFieldRef();
scalarField&       bfField    = biofilmPhase.primitiveFieldRef();
scalarField&       darcyField = darcyTerm.primitiveFieldRef();

// Umbral mínimo para considerar que hay pellet de S0
const scalar S0thVal = S0phaseOn.value();  // umbral físico
forAll (mesh.C(), celli)
{
   
    const scalar S0Cell    = S0Field[celli];
    const scalar MnormCell = MnormField[celli];
    const bool hasSulfur = (S0Cell > S0thVal);
    const bool hasBiofilm = (!hasSulfur && MnormCell > SMALL);  // LÓGICA ORIGINAL

    if (hasSulfur)
    {
        // ------------------------------------------------------
        // 1) PELLET DE S0 — domina el sólido, NO hay biofilm
        // ------------------------------------------------------
        bfField[celli]    = 0.0;

        KField[celli]     = sulfurPermeability.value();
        epsField[celli]   = sulfurPorosity.value();
        darcyField[celli] = nu.value()/KField[celli];

        // OPCIONAL: vaciar biomasa dentro del pellet
        // (No estaba en el original porque no existía S0)
        // MField[celli]    = 0.0;
        // MhidField[celli] = 0.0;
    }
    else if (hasBiofilm)
    {
        // ------------------------------------------------------
        // 2) BIOFILM — EXACTO COMO EL SOLVER ORIGINAL
        // ------------------------------------------------------
        bfField[celli]    = 1.0;

        KField[celli]     = biofilmPermeability.value();
        epsField[celli]   = biofilmPorosity.value();
        darcyField[celli] = nu.value()/KField[celli];
    }
    else
    {
        // ------------------------------------------------------
        // 3) FLUIDO LIBRE — IMPLEMENTACIÓN 100% ORIGINAL
        // ------------------------------------------------------
        bfField[celli]    = 0.0;

        // Parámetros del fluido libre (igual que original)
        KField[celli]     = 1.0;
        epsField[celli]   = 1.0;
        darcyField[celli] = 0.0;

        // *** AQUÍ ESTÁ EL COMPORTAMIENTO ORIGINAL ***
        // En el solver original, M se pone a cero si M < SMALL.
        // Adaptación correcta: usar Mnorm en vez de M adimensional.
        if (MnormCell < SMALL)
        {
            MField[celli]    = 0.0;
            MhidField[celli] = 0.0;
        }
    }
}

// === Fin updateBiofilmPhase.H ==========================================
