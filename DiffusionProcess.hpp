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
  typedef const void (DiffusionProcess::*WalkMethod)(void) const;
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
    theWalkMethod(&DiffusionProcess::volumeWalk) {}
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
      for(vector<Species*>::const_iterator i(theProcessSpecies.begin());
          i != theProcessSpecies.end(); ++i)
        {
          (*i)->setDiffusionCoefficient(D);
        }
    }
  virtual void initializeThird()
    {
      Species* aSpecies(theProcessSpecies[0]);
      isVolume = aSpecies->getIsVolume();
      double rho(aSpecies->getMaxReactionProbability());
      if(D > 0)
        {
          for(vector<Species*>::const_iterator i(theProcessSpecies.begin());
              i != theProcessSpecies.end(); ++i)
            {
              if((*i)->getIsVolume() != isVolume)
                {
                  THROW_EXCEPTION(ValueError, String(
                                   getPropertyInterface().getClassName()) +
                                  "[" + getFullID().asString() + 
                                  "]: A DiffusionProcess can only execute" +
                                  " multiple species when they are all either" +
                                  " in a volume compartment or a surface" +
                                  " compartment, not both concurrently. " +
                                  getIDString(theProcessSpecies[0]) + " and " +
                                  getIDString(*i) + " belong to different" +
                                  " types of compartment.");
                }
              if(rho < (*i)->getMaxReactionProbability())
                {
                  if(rho > P)
                    {
                      THROW_EXCEPTION(ValueError, String(
                                       getPropertyInterface().getClassName()) + 
                                      "[" + getFullID().asString() + 
                                      "]: Create separate" +
                                      " DiffusionProcesses for " +
                                      getIDString(aSpecies) + " and " +
                                      getIDString(*i) + " since their" +
                                      " reaction probabilities are not the" +
                                      " same and the latter's reaction" +
                                      " probability is higher than P.");
                    }
                  aSpecies = *i;
                  rho = (*i)->getMaxReactionProbability();
                }
            }
        }
      if(rho > P)
        {
          WalkProbability = P/rho;
        }
      for(vector<Species*>::const_iterator i(theProcessSpecies.begin());
          i != theProcessSpecies.end(); ++i)
        {
          (*i)->rescaleReactionProbabilities(WalkProbability);
        }
      if(D > 0)
        {
          double r_v(theSpatiocyteStepper->getVoxelRadius());
          double lambda(2.0/3);
          if(!isVolume)
            {
              lambda = pow((2*sqrt(2)+4*sqrt(3)+3*sqrt(6)+sqrt(22))/
                           (6*sqrt(2)+4*sqrt(3)+3*sqrt(6)), 2);
            }
          theStepInterval = lambda*r_v*r_v*WalkProbability/D;
        }
      for(vector<Species*>::const_iterator i(theProcessSpecies.begin());
          i != theProcessSpecies.end(); ++i)
        {
          (*i)->setDiffusionInterval(theStepInterval);
        }
      if(isVolume)
        {
          if(WalkProbability == 1)
            {
              theWalkMethod = &DiffusionProcess::volumeWalk;
            }
          else
            {
              theWalkMethod = &DiffusionProcess::volumeWalkCollide;
            }
        }
      else
        {
          if(WalkProbability == 1)
            {
              theWalkMethod = &DiffusionProcess::surfaceWalk;
            }
          else
            {
              theWalkMethod = &DiffusionProcess::surfaceWalkCollide;
            }
        }
      //At the start of the simulation, we must make sure the CollisionProcess
      //is fired first before the DiffusionProcess. This is to make sure
      //the reaction probability is valid for reactants that are initially
      //at contact:
    }
  virtual void printParameters()
    {
      for(vector<Species*>::const_iterator i(theProcessSpecies.begin());
          i != theProcessSpecies.end(); ++i)
        {
          cout << getIDString(*i) << " ";
        }
      cout << ":" << endl << "  Diffusion interval=" << theStepInterval <<
        ", D=" << D << ", Walk probability (P/rho)=" <<
        WalkProbability << endl;
    }
  virtual void fire()
    {
      (this->*theWalkMethod)();
      theTime += theStepInterval;
      thePriorityQueue->moveTop();
    }
  const void volumeWalk() const
    {
      for(vector<Species*>::const_iterator i(theProcessSpecies.begin());
          i != theProcessSpecies.end(); ++i)
        {
          (*i)->volumeWalk();
        }
    }
  const void volumeWalkCollide() const
    {
      for(vector<Species*>::const_iterator i(theProcessSpecies.begin());
          i != theProcessSpecies.end(); ++i)
        {
          (*i)->volumeWalkCollide();
        }
    }
  const void surfaceWalk() const
    {
      for(vector<Species*>::const_iterator i(theProcessSpecies.begin());
          i != theProcessSpecies.end(); ++i)
        {
          (*i)->surfaceWalk();
        }
    }
  const void surfaceWalkCollide() const
    {
      for(vector<Species*>::const_iterator i(theProcessSpecies.begin());
          i != theProcessSpecies.end(); ++i)
        {
          (*i)->surfaceWalkCollide();
        }
    }
  virtual void initializeLastOnce()
    {
      for(vector<Species*>::const_iterator i(theProcessSpecies.begin());
          i != theProcessSpecies.end(); ++i)
        {
          (*i)->addInterruptedProcess(
                            reinterpret_cast<SpatiocyteProcess*>(this));
        }
    }
  virtual void addSubstrateInterrupt(Species* aSpecies, Voxel* aMolecule)
    {
      if(theStepInterval == libecs::INF)
        {
          theStepInterval = theProcessSpecies[0]->getDiffusionInterval();
          theTime = theSpatiocyteStepper->getCurrentTime() + theStepInterval; 
          thePriorityQueue->move(theQueueID);
        }
    }
  virtual void removeSubstrateInterrupt(Species* aSpecies, Voxel* aMolecule)
    {
      if(theStepInterval != libecs::INF)
        {
          for(vector<Species*>::const_iterator i(theProcessSpecies.begin());
              i != theProcessSpecies.end(); ++i)
            {
              if((*i)->size())
                {
                  return;
                }
            }
          theStepInterval = libecs::INF;
          theTime = theStepInterval; 
          thePriorityQueue->move(theQueueID);
        }
    }
protected:
  bool isVolume;
  double D;
  double P;
  double WalkProbability;
  WalkMethod theWalkMethod;
};

#endif /* __DiffusionProcess_hpp */
