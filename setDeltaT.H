/*---------------------------------------------------------------------------*\


    ]3 | () /= | |_ |\/| /= () /\ |\/|

    Copyright 2023 Toulouse INP
    Romain Guibert

License
    This file is part of biofilmFoam, an extension of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Global
    setDeltaT

\*---------------------------------------------------------------------------*/

if (adjustTimeStep)
{
    // Límite por Courant (requiere haber incluido CourantNo.H antes)
    scalar maxDeltaTFact = maxCo/(CoNum + SMALL);
    scalar dtGrowthSlope = runTime.controlDict().lookupOrDefault<scalar>("dtGrowthSlope", 0.5);
    scalar dtGrowthCap   = runTime.controlDict().lookupOrDefault<scalar>("dtGrowthCap",   1.8);
    scalar deltaTFact = min( min(maxDeltaTFact, 1.0 + dtGrowthSlope*maxDeltaTFact), dtGrowthCap );

    // Sugerencias de los gestores por variable
    scalar dtForM   = dtManagerM.computeTimestep();
    scalar dtForNO3 = dtManagerNO3.computeTimestep();
    scalar dtForNO2 = dtManagerNO2.computeTimestep();

    // Si C está desactivado, no lo consideres (usa GREAT para que no limite)
    scalar dtForC   = useC ? dtManagerC.computeTimestep() : GREAT;

    // Más restrictivo entre todos los activos
    scalar dtFromManagers = min(min(dtForM, dtForNO3), min(dtForNO2, dtForC));

    // Aplica también el límite por Co y maxDeltaT
    runTime.setDeltaT
    (
        min
        (
            dtFromManagers,
            min(deltaTFact*runTime.deltaTValue(), maxDeltaT)
        )
    );

    Info<< "Δt managers: "
        << "M="   << dtForM
        << ", NO3=" << dtForNO3
        << ", NO2=" << dtForNO2;

    if (useC) Info << ", C=" << dtForC; else Info << ", C=off";

    Info<< " | elegido=" << runTime.deltaTValue()
        << " (factor Co=" << deltaTFact << ")"
        << nl << endl;
}
// ************************************************************************* //
