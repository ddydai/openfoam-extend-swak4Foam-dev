#! /bin/bash

topDir=$(pwd)
marker="testStarted"

function runIt {
    local here=$1
    shift 1
    cd $here
    if [ -e $marker ]; then
        echo "$here already tested"
        cd $topDir
        return
    fi
    touch $marker
    echo
    echo "$here starting"
    echo
    pyFoamPrepareCase.py .
    pyFoamRunner.py $* --progress --auto auto
    echo
    echo "$here finished"
    echo
    cd $topDir
}

runIt DynamicMesh/hecticInletACMI2D
runIt DynamicMesh/rotatingTank2D  --run-until=5
runIt DynamicMesh/switchedInletACMI2D
runIt DynamicMesh/switchedTJunction --run-until=0.5
runIt FiniteArea/planeTransport
runIt functionPlugins/chemPluginCounterFlowFlame2D
runIt functionPlugins/shiftCavity
runIt FvOptions/angleDuctWithSources
runIt FvOptions/heatExchangerSources
runIt groovyBC/average-t-junction
runIt groovyBC/circulatingSplash --run-until=0.004
runIt groovyBC/delayed-t-junction
runIt groovyBC/fillingTheDam
runIt groovyBC/jumpChannel
runIt groovyBC/movingConeDistorted
runIt groovyBC/pulsedPitzDaily --run-until=1e-4
runIt groovyBC/wobbler
runIt ImmersedBC/pitzDailyImmersed
runIt Lagrangian/functionObjects/angledDuctWithBalls
runIt Lagrangian/functionObjects/hotStream
runIt Lagrangian/functionObjects/icoFoamCavityWithParcel
runIt Lagrangian/LanguageIntegration/angledDuctWithLuaInject
runIt Lagrangian/LanguageIntegration/angledDuctWithPython2Inject
runIt Lagrangian/LanguageIntegration/angledDuctWithPython3Inject
runIt Lagrangian/parser/parcelInBoxWithExpressions
runIt Lagrangian/parser/simplifiedSiwek
runIt LuaIntegration/flowStatePitzDaily
runIt LuaIntegration/luaScriptedBCAngledDuct
runIt LuaIntegration/raiseRelaxationPitzDaily
runIt MakeAxialMesh/axialCavity
runIt MakeAxialMesh/axialPitzDaily
runIt manipulateFvSolutionFvSchemes/pitzDailyStateMachineSwitched
runIt manipulateFvSolutionFvSchemes/pitzDailyTimeSwitched
runIt other/angledDuctImplicit
runIt other/capillaryRise --run-until=0.05
runIt other/counterFlowFlame2DInitialized
runIt other/simpleBendedPipe
runIt other/topoSetDamBreak
runIt Python3Integration/flowStatePitzDaily
runIt Python3Integration/python3ScriptedBCAngledDuct
runIt SimpleSurface/littleVolcano --run-until=0.1
runIt solvePDE/flangeWithPDE
runIt solvePDE/pitzDailyWithPDE
runIt StateMachine/stateCavity
runIt tests/languageIntegration/luaIntegration
runIt tests/languageIntegration/pythonIntegration
runIt tests/languageIntegration/python3Integration
runIt tests/SearchableSurfaces/objectTests
runIt tests/SearchableSurfaces/operationsTest
runIt tests/SearchableSurfaces/operationsTestRotated
runIt tests/SearchableSurfaces/scaledIglooWithFridges
runIt tests/testFOExecution
