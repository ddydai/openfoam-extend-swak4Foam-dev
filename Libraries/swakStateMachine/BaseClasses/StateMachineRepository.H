/*---------------------------------------------------------------------------*\
|                       _    _  _     ___                       | The         |
|     _____      ____ _| | _| || |   / __\__   __ _ _ __ ___    | Swiss       |
|    / __\ \ /\ / / _` | |/ / || |_ / _\/ _ \ / _` | '_ ` _ \   | Army        |
|    \__ \\ V  V / (_| |   <|__   _/ / | (_) | (_| | | | | | |  | Knife       |
|    |___/ \_/\_/ \__,_|_|\_\  |_| \/   \___/ \__,_|_| |_| |_|  | For         |
|                                                               | OpenFOAM    |
-------------------------------------------------------------------------------
License
    This file is part of swak4Foam.

    swak4Foam is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    swak4Foam is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with swak4Foam; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

Class
    Foam::StateMachineRepository

Description

SourceFiles
    StateMachineRepository.C

Contributors/Copyright:
    2016, 2018 Bernhard F.W. Gschaider <bgschaid@hfd-research.com>
    2018 Mark Olesen <Mark.Olesen@esi-group.com>

 SWAK Revision: $Id$
\*---------------------------------------------------------------------------*/

#ifndef StateMachineRepository_H
#define StateMachineRepository_H

#include "StateMachine.H"

#include "HashPtrTable.H"

#include "regIOobject.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           Class StateMachineRepository Declaration
\*---------------------------------------------------------------------------*/

class StateMachineRepository
:
    public regIOobject
{
private:
   // Private data

    typedef HashPtrTable<StateMachine> MachineTable;
    MachineTable allMachines_;

    dictionary readFromRestart_;

    // Constructors

    //- Construct null
    StateMachineRepository(const objectRegistry &obr);

    //- The only instance of this repository
    static StateMachineRepository *repositoryInstance;

public:
     //- Runtime type information
    TypeName("StateMachineRepository");

    //- Destructor

    virtual ~StateMachineRepository();

    //- Get the Singleton of this class
    static StateMachineRepository &getStateMachines(
        const objectRegistry &obr
    );

    static StateMachineRepository &getStateMachines();

    //- make sure that the state is consistently written
    void ensureWrite();

    virtual bool writeData(Ostream &) const ;

    virtual bool readData(Istream &);

    bool found(const word &name);

    void insert(
        autoPtr<StateMachine> machine
    );

    StateMachine &operator[](const word &name);

    wordList toc();
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
