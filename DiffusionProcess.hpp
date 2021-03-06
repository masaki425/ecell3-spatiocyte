//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//
//        This file is part of E-Cell Simulation Environment package
//
//                Copyright (C) 2006-2009 Keio University
//
//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//
//
// E-Cell is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// E-Cell is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public
// License along with E-Cell -- see the file COPYING.
// If not, write to the Free Software Foundation, Inc.,
// 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
// 
//END_HEADER
//
// written by Satya Arjunan <satya.arjunan@gmail.com>
// E-Cell Project, Institute for Advanced Biosciences, Keio University.
//


#ifndef __DiffusionProcess_hpp
#define __DiffusionProcess_hpp

#include <sstream>
#include <MethodProxy.hpp>
#include "SpatiocyteProcess.hpp"
#include "SpatiocyteSpecies.hpp"

LIBECS_DM_CLASS(DiffusionProcess, SpatiocyteProcess)
{ 
  typedef void (DiffusionProcess::*WalkMethod)(void) const;
public:
  LIBECS_DM_OBJECT(DiffusionProcess, Process)
    {
      INHERIT_PROPERTIES(Process);
      PROPERTYSLOT_SET_GET(Real, D);
      PROPERTYSLOT_SET_GET(Real, P);
      PROPERTYSLOT_SET_GET(Real, WalkProbability);
    }
  DiffusionProcess():
    D(0),
    P(1),
    WalkProbability(1),
    theDiffusionSpecies(NULL),
    theVacantSpecies(NULL),
    theWalkMethod(&DiffusionProcess::walk) {}
  virtual ~DiffusionProcess() {}
  SIMPLE_SET_GET_METHOD(Real, D);
  SIMPLE_SET_GET_METHOD(Real, P);
  SIMPLE_SET_GET_METHOD(Real, WalkProbability);
  virtual void initialize()
    {
      if(isInitialized)
        {
          return;
        }
      SpatiocyteProcess::initialize();
      for(VariableReferenceVector::iterator
          i(theVariableReferenceVector.begin());
          i != theVariableReferenceVector.end(); ++i)
        {
          Species* aSpecies(theSpatiocyteStepper->variable2species(
                                   (*i).getVariable())); 
          if(!(*i).getCoefficient())
            {
              if(theDiffusionSpecies)
                {
                  THROW_EXCEPTION(ValueError, String(
                                  getPropertyInterface().getClassName()) +
                                  "[" + getFullID().asString() + 
                                  "]: A DiffusionProcess requires only one " +
                                  "nonHD variable reference with zero " +
                                  "coefficient as the species to be " +
                                  "diffused, but " + 
                                  getIDString(theDiffusionSpecies) + " and " +
                                  getIDString(aSpecies) + " are given."); 
                }
              theDiffusionSpecies = aSpecies;
              theDiffusionSpecies->setDiffusionCoefficient(D);
            }
          else
            {
              if(theVacantSpecies)
                {
                  THROW_EXCEPTION(ValueError, String(
                                  getPropertyInterface().getClassName()) +
                                  "[" + getFullID().asString() + 
                                  "]: A DiffusionProcess requires only one " +
                                  "nonHD variable reference with negative " +
                                  "coefficient as the vacant species to be " +
                                  "diffused on, but " +
                                  getIDString(theVacantSpecies) + " and " +
                                  getIDString(aSpecies) + " are given."); 
                }
              theVacantSpecies = aSpecies;
            }
        }
      if(!theDiffusionSpecies)
        {
          THROW_EXCEPTION(ValueError, String(
                          getPropertyInterface().getClassName()) +
                          "[" + getFullID().asString() + 
                          "]: A DiffusionProcess requires only one " +
                          "nonHD variable reference with zero coefficient " +
                          "as the species to be diffused, but none is given."); 
        }
    }
  virtual void initializeSecond()
    {
      if(theVacantSpecies)
        {
          theVacantSpecies->setIsDiffuseVacant();
          theDiffusionSpecies->setVacantSpecies(theVacantSpecies);
        }
    }
  virtual void initializeThird()
    {
      double rho(theDiffusionSpecies->getMaxReactionProbability());
      if(rho > P)
        {
          WalkProbability = P/rho;
        }
      theDiffusionSpecies->rescaleReactionProbabilities(WalkProbability);
      if(D > 0)
        {
          double r_v(theSpatiocyteStepper->getVoxelRadius());
          double lambda(2.0/3);
          if(!theDiffusionSpecies->getIsVolume())
            {
              lambda = pow((2*sqrt(2)+4*sqrt(3)+3*sqrt(6)+sqrt(22))/
                           (6*sqrt(2)+4*sqrt(3)+3*sqrt(6)), 2);
            }
          theStepInterval = lambda*r_v*r_v*WalkProbability/D;
        }
      theDiffusionSpecies->setDiffusionInterval(theStepInterval);
      if(theDiffusionSpecies->getIsDiffuseVacant())
        {
          theWalkMethod = &DiffusionProcess::walkVacant;
        }
      else
        {
          theWalkMethod = &DiffusionProcess::walk;
        }
    }
  virtual void printParameters()
    {
      String aProcess(String(getPropertyInterface().getClassName()) + 
                      "[" + getFullID().asString() + "]");
      std::cout << aProcess << std::endl;
      std::cout << "  " << getIDString(theDiffusionSpecies) << " ";
      std::cout << ":" << std::endl << "  Diffusion interval=" <<
        theStepInterval << ", D=" << D << ", Walk probability (P/rho)=" <<
        WalkProbability << std::endl;
    }
  virtual void fire()
    {
      theDiffusionSpecies->resetFinalizeReactions();
      (this->*theWalkMethod)();
      theDiffusionSpecies->finalizeReactions();
      theTime += theStepInterval;
      thePriorityQueue->moveTop();
    }
  void walk() const
    {
      theDiffusionSpecies->walk();
    }
  void walkVacant() const
    {
      theDiffusionSpecies->walkVacant();
    }
  virtual void initializeLastOnce()
    {
      theDiffusionSpecies->addInterruptedProcess(this);
    }
  virtual void addSubstrateInterrupt(Species* aSpecies, Voxel* aMolecule)
    {
      if(theStepInterval == libecs::INF)
        {
          theStepInterval = theDiffusionSpecies->getDiffusionInterval();
          theTime = theSpatiocyteStepper->getCurrentTime() + theStepInterval; 
          thePriorityQueue->move(theQueueID);
        }
    }
  virtual void removeSubstrateInterrupt(Species* aSpecies, Voxel* aMolecule)
    {
      if(theStepInterval != libecs::INF)
        {
          if(theDiffusionSpecies->size())
            {
              return;
            }
          theStepInterval = libecs::INF;
          theTime = theStepInterval; 
          thePriorityQueue->move(theQueueID);
        }
    }
protected:
  double D;
  double P;
  double WalkProbability;
  Species* theDiffusionSpecies;
  Species* theVacantSpecies;
  WalkMethod theWalkMethod;
};

#endif /* __DiffusionProcess_hpp */

