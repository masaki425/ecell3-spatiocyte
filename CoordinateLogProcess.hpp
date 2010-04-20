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


#ifndef __CoordinateLogProcess_hpp
#define __CoordinateLogProcess_hpp

#include <fstream> //provides ofstream
#include <MethodProxy.hpp>
#include "IteratingLogProcess.hpp"
#include "SpatiocyteSpecies.hpp"

LIBECS_DM_CLASS(CoordinateLogProcess, IteratingLogProcess)
{ 
public:
  LIBECS_DM_OBJECT(CoordinateLogProcess, Process)
    {
      INHERIT_PROPERTIES(IteratingLogProcess);
    }
  CoordinateLogProcess():
    theMoleculeSize(0) {}
  virtual ~CoordinateLogProcess() {}
  virtual void initializeLastOnce()
    {
      for(unsigned int i(0); i != theProcessSpecies.size(); ++i)
        {
          theMoleculeSize += theProcessSpecies[i]->size();
        }
      theLogFile.open(FileName.c_str(), ios::trunc);
      initializeLog();
      logSpecies();
    }
  virtual void fire()
    {
      if(theTime <= LogDuration)
        {
          logSpecies();
          theTime += theStepInterval;
        }
      else
        {
          theTime = libecs::INF;
          theLogFile.flush();
          theLogFile.close();
        }
      thePriorityQueue->moveTop();
    }
  void logSpecies()
    {
      for(unsigned int i(0); i != theProcessSpecies.size(); ++i)
        {
          logMolecules(i);
        }
      theLogFile << endl;
    }
protected:
  void initializeLog()
    {
      Point aCenterPoint(theSpatiocyteStepper->getCenterPoint());
      theLogFile
        << "startCoord:" << theSpatiocyteStepper->getStartCoord()
        << " rowSize:" << theSpatiocyteStepper->getRowSize() 
        << " layerSize:" << theSpatiocyteStepper->getLayerSize()
        << " colSize:" << theSpatiocyteStepper->getColSize()
        << " width:" << aCenterPoint.z*2
        << " height:" << aCenterPoint.y*2
        << " length:" <<  aCenterPoint.x*2
        << " voxelRadius:" << theSpatiocyteStepper->getVoxelRadius()
        << " moleculeSize:" << theMoleculeSize << endl;
    }
  void logMolecules(int anIndex)
    {
      Species* aSpecies(theProcessSpecies[anIndex]);
      for(unsigned int i(0); i != aSpecies->size(); ++i)
        {
          theLogFile << ", " << aSpecies->getCoord(i);
        }
    }
private:
  double theMoleculeSize;
};

#endif /* __CoordinateLogProcess_hpp */
