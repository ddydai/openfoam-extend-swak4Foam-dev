/*---------------------------------------------------------------------------*\
 ##   ####  ######     |
 ##  ##     ##         | Copyright: ICE Stroemungsfoschungs GmbH
 ##  ##     ####       |
 ##  ##     ##         | http://www.ice-sf.at
 ##   ####  ######     |
-------------------------------------------------------------------------------
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright  held by original author
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is based on OpenFOAM.

    OpenFOAM is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

Contributors/Copyright:
    2011-2017 Bernhard F.W. Gschaider <bgschaid@hfd-research.com>

 SWAK Revision: $Id$
\*---------------------------------------------------------------------------*/

#include "generalInterpreterWrapper.H"
#include "addToRunTimeSelectionTable.H"

#include "IFstream.H"

#include "GlobalVariablesRepository.H"

#include "vector.H"
#include "tensor.H"
#include "symmTensor.H"
#include "sphericalTensor.H"

#include <cassert>

#ifdef FOAM_HAS_STRINGOPS
#include "stringOps.H"
#endif

// #include <fcntl.h>

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(generalInterpreterWrapper, 0);

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

generalInterpreterWrapper::generalInterpreterWrapper
(
    const objectRegistry& obr,
    const dictionary& dict,
    bool forceToNamespace,
    const word interpreterName
):
    interpreterName_(
        interpreterName
    ),
    obr_(obr),
    dict_(dict),
    tolerateExceptions_(dict.lookupOrDefault<bool>("tolerateExceptions",false)),
    warnOnNonUniform_(dict.lookupOrDefault<bool>("warnOnNonUniform",true)),
    isParallelized_(dict.lookupOrDefault<bool>("isParallelized",false)),
    parallelMasterOnly_(false),
    swakToInterpreterNamespaces_(
        forceToNamespace
        ?
        wordList(dict.lookup("swakTo"+InterpreterName()+"Namespaces"))
        :
        dict.lookupOrDefault<wordList>(
            "swakTo"+InterpreterName()+"Namespaces",
            wordList(0)
        )
    ),
    interpreterToSwakNamespace_(
        dict.lookupOrDefault<word>(
            this->interpreterName()+"ToSwakNamespace",
            word("")
        )
    ),
    interpreterToSwakVariables_(
        dict.lookupOrDefault<wordList>(
            this->interpreterName()+"ToSwakVariables",
            wordList(0)
        )
    ),
    interactiveAfterExecute_(
        dict.lookupOrDefault<bool>("interactiveAfterExecute",false)
    ),
    interactiveAfterException_(
        dict.lookupOrDefault<bool>("interactiveAfterException",false)
    )
{
    Pbug << "Starting constructor" << endl;

    syncParallel();

#ifdef FOAM_HAS_LOCAL_DEBUGSWITCHES
    debug=dict.lookupOrDefault<label>("debugInterpreterWrapper",debug());
#else
    debug=dict.lookupOrDefault<label>("debugInterpreterWrapper",debug);
#endif

    if(Pstream::parRun()) {
        Pbug << "This is a parallel run" << endl;

        parallelMasterOnly_=readBool(dict.lookup("parallelMasterOnly"));
    }

    if(parallelNoRun(true)) {
        Pbug << "Getting out because of 'parallelNoRun'" << endl;
        return;
    }

    if(
        interpreterToSwakVariables_.size()>0
        &&
        interpreterToSwakNamespace_==""
    ) {
        FatalErrorIn("generalInterpreterWrapper::generalInterpreterWrapper")
            << "There are outgoing variables " << interpreterToSwakVariables_
                << " defined, but no namespace '"+this->interpreterName()+"ToSwakNamespace'"
                << " to write them to"
                << endl
                << exit(FatalError);
    }

    Pbug << "End constructor" << endl;
 }

bool generalInterpreterWrapper::parallelMustBroadcast()
{
    return Pstream::parRun() && parallelMasterOnly_;
}

bool generalInterpreterWrapper::parallelNoRun(bool doWarning)
{
    if(Pstream::parRun()) {
        if(isParallelized_ && doWarning) {
            WarningIn("bool generalInterpreterWrapper::parallelNoRun()")
                << "The parameter 'isParallelized' was set in " << dict_.name()
                    << ". This means that the "
                    << InterpreterName()+" code has no adverse side effects in parallel"
                    << endl;
        }
        if(parallelMasterOnly_) {
            return !Pstream::master();
        } else {
            if(!isParallelized_) {
                FatalErrorIn("generalInterpreterWrapper::parallelNoRun(bool doWarning)")
                    << "This is a parallel run and the "+InterpreterName()+"-snipplets may have"
                        << " adverse side effects. If you do not think so set"
                        << " the 'isParallelized'-flag to true in " << dict_.name()
                        << endl
                        << exit(FatalError);
            }
            return false;
        }
    } else {
        return false;
    }
}

generalInterpreterWrapper::~generalInterpreterWrapper()
{
    if(parallelNoRun()) {
        return;
    }
}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void generalInterpreterWrapper::assertParallel(const word &name)
{
    if(Pstream::parRun() && parallelNoRun()) {
        FatalErrorIn("generalInterpreterWrapper::"+name)
            << "This function is not meant to be called on this processor" << endl
                << "Programmers error"
                << endl
                << exit(FatalError);
    }
}


void generalInterpreterWrapper::setInteractive(
    bool interactiveAfterExecute,
    bool interactiveAfterException
)
{
    interactiveAfterExecute_=interactiveAfterExecute;
    interactiveAfterException_=interactiveAfterException;
    if(
        interactiveAfterExecute_
        ||
        interactiveAfterException_
    ) {
        initInteractiveSpecial();
    }
}


string generalInterpreterWrapper::readCode(
    const word &prefix,
    bool mustRead
) {
    string result;
    readCode(
        dict_,
        prefix,
        result,
        mustRead
    );
    return result;
}

void generalInterpreterWrapper::readCode(
    const dictionary &dict,
    const word &prefix,
    string &code,
    bool mustRead
) {
    if(
        dict.found(prefix+"Code")
        &&
        dict.found(prefix+"File")
    ) {
        FatalErrorIn("generalInterpreterWrapper::readCode")
            << "Either specify " << prefix+"Code" << " or "
                << prefix+"File" << " but not both" << endl
                << exit(FatalError);
    }
    if(
        mustRead
        &&
        (
            !dict.found(prefix+"Code")
            &&
            !dict.found(prefix+"File")
        )
    ) {
        FatalErrorIn("generalInterpreterWrapper::readCode")
            << "Neither " << prefix+"Code" << " nor "
                << prefix+"File" << " specified in " << dict.name() << endl
                << exit(FatalError);
    }
    if(dict.found(prefix+"Code")) {
        code=string(dict.lookup(prefix+"Code"));
    } else {
        if(dict.found(prefix+"File")) {
            fileName fName(dict.lookup(prefix+"File"));
            fName.expand();
            if(!exists(fName)) {
                fileName oldFName=fName;
                fName=dict.name().path() / fName;
                if(!exists(fName)) {
                    FatalErrorIn("generalInterpreterWrapper::readCode")
                        << "Can't find source file " << oldFName
                            << " or "<< fName
                            << endl << exit(FatalError);
                }
            }

            IFstream in(fName);
            code="";
            while(in.good()) {
                char c;
                in.get(c);
                code+=c;
            }
        } else {
            assert(mustRead==false);
            code="";
        }
    }
#ifdef FOAM_HAS_STRINGOPS
    code=stringOps::trim(code);
#endif
}

void generalInterpreterWrapper::scatterGlobals()
{
    Pbug << "scatterGlobals" << endl;

    forAll(interpreterToSwakVariables_,i) {
        const word &name=interpreterToSwakVariables_[i];
        Pbug << "Scattering " << name << endl;

        ExpressionResult result;
        if(!parallelNoRun()) {
            result=*GlobalVariablesRepository::getGlobalVariables(
                obr_
            ).getNamespace(
                interpreterToSwakNamespace_
            )[name];
            Pbug << "Value to scatter: " << result << endl;
        }

        // this doesn't work for some dubious reason
        // Pstream::scatter(result);

        //- do the scattering by ourself
        List<Pstream::commsStruct> comms;
        if (Pstream::nProcs() < Pstream::nProcsSimpleSum)
        {
            comms=Pstream::linearCommunication();
        }
        else
        {
            comms=Pstream::treeCommunication();
        }
        const Pstream::commsStruct& myComm = comms[Pstream::myProcNo()];
        if (myComm.above() != -1)
        {
            string incoming="";
            IPstream fromAbove(
#ifdef FOAM_PSTREAM_COMMSTYPE_IS_ENUMCLASS
                Pstream::commsTypes::scheduled,
#else
                Pstream::scheduled,
#endif
                myComm.above()
            );
            fromAbove >> incoming;
            IStringStream inStream(incoming);
            inStream >> result;
        }
        forAll(myComm.below(), belowI)
        {
            OPstream toBelow(
#ifdef FOAM_PSTREAM_COMMSTYPE_IS_ENUMCLASS
                Pstream::commsTypes::scheduled,
#else
                Pstream::scheduled,
#endif
                myComm.below()[belowI]
            );
            OStringStream outgoing;
            outgoing << result;
            toBelow << outgoing.str();
        }

        if(parallelNoRun()) {
            Info << "Setting value of " << name << endl;

            ExpressionResult &res=GlobalVariablesRepository::getGlobalVariables(
                obr_
            ).addValue(
                name,
                interpreterToSwakNamespace_,
                result
            );
            res.noReset();
        }
    }
}

void generalInterpreterWrapper::syncParallel() const
{
    static label nr=0;

    if(Pstream::parRun()) {
        nr++;
        label id=nr;
        Pbug << "syncParallel: wait nr " << nr << endl;
        reduce(id,plusOp<label>());
        Pbug << "syncParallel: all here " << id << endl;
        if(id!=nr*Pstream::nProcs()) {
            Pout << "syncParallel. Id: " << id
                << " expected: " << nr*Pstream::nProcs() << endl;
            FatalErrorIn("generalInterpreterWrapper::syncParallel()")
                << "Wrong sequence number. Out of sync"
                    << endl
                    << exit(FatalError);

         }
    } else {
        Dbug << "syncParallel: no wait" << endl;
    }
}

const word generalInterpreterWrapper::InterpreterName() {
    word result=interpreterName_;
    result[0]=toupper(result[0]);
    return result;
}

bool generalInterpreterWrapper::executeCode(
    const string &code,
    bool putVariables,
    bool failOnException
)
{
    Pbug << "ExecuteCode: " << code << endl;
    if(code.size()==0) {
        Pbug << "No code. Exiting" << endl;
        return true;
    }
    syncParallel();

    bool success=false;

    if(!parallelNoRun()) {
        setInterpreter();

        getGlobals();

        Pbug << "Execute" << endl;
        success=executeCodeInternal(code);
        Pbug << "Success: " << success << endl;

        doAfterExecution(!success,code,putVariables,failOnException);

        releaseInterpreter();
    }

    if(parallelMustBroadcast()) {
        Pbug << "Prescatter: " << success << endl;
        Pstream::scatter(success);
        Pbug << "Postscatter: " << success << endl;
        if(putVariables) {
            scatterGlobals();
        }
    }

    return success;
}

bool generalInterpreterWrapper::executeCodeCaptureOutput(
    const string &code,
    string &stdout,
    bool putVariables,
    bool failOnException
) {
    Pbug << "ExecuteCodeCaptureOutput: " << code << endl;
    syncParallel();

    bool success=false;

    if(!parallelNoRun()) {
        setInterpreter();

        getGlobals();

        success=executeCodeCaptureOutputInternal(
            code,
            stdout
        );

        doAfterExecution(!success,code,putVariables,failOnException);

        Pbug << "Success: " << success << " Captured: " << stdout << endl;

        releaseInterpreter();
    }

    if(parallelMustBroadcast()) {
        Pbug << "Prescatter: " << success << endl;
        Pstream::scatter(success);
        Pstream::scatter(stdout);
        Pbug << "Postscatter: " << success << endl;
        if(putVariables) {
            scatterGlobals();
        }
    }

    return success;
}


} // namespace Foam

// ************************************************************************* //