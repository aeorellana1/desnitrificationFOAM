// === updateFields.H (Hito4 - criterio con Mtot y SMALL) ===================

forAll (mesh.C(), celli)
{
    const scalar MtotCell = M[celli] + Mhid[celli];

    if (MtotCell > SMALL)
    {
        // Dentro del biofilm: mismas propiedades que en updateBiofilmPhase.H
        K[celli]            = biofilmPermeability.value();
        porosity[celli]     = biofilmPorosity.value();
        biofilmPhase[celli] = 1.0;
        darcyTerm[celli]    = nu.value()/K[celli];
    }
    else
    {
        // Fuera del biofilm
        biofilmPhase[celli] = 0.0;
        // De nuevo: no tocar M ni Mhid aquí
        K[celli]         = 1.0;
        porosity[celli]  = 1.0;
        darcyTerm[celli] = 0.0;
    }
}

// === fin updateFields.H ===================================================
