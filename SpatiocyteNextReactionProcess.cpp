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

#include "SpatiocyteNextReactionProcess.hpp"
#include "SpatiocyteSpecies.hpp"

LIBECS_DM_INIT(SpatiocyteNextReactionProcess, Process);

void SpatiocyteNextReactionProcess::fire()
{
  if(theOrder == 0)
    {
      if(C)
        { 
          Voxel* moleculeC(C->getRandomCompVoxel());
          if(moleculeC == NULL)
            {
              requeue();
              return;
            }
          C->addMolecule(moleculeC);
        }
      else if(variableC)
        {
          variableC->addValue(1);
        }
    }
  else if(theOrder == 1)
    { 
      //nonHD_A -> nonHD_C + nonHD_D:
      if(A && C && D)
        {
          Voxel* moleculeA(A->getRandomMolecule());
          //If the product C is not in the same Comp as A,
          //we need to find a vacant adjoining voxel of A that belongs
          //to the Comp of C:
          Voxel* moleculeC;
          if(A->getComp() != C->getComp())
            {
              moleculeC = C->getRandomAdjoiningVoxel(moleculeA);
              //Only proceed if we can find an adjoining vacant voxel
              //of A which can be occupied by C:
              if(moleculeC == NULL)
                {
                  requeue();
                  return;
                }
            }
          else
            {
              moleculeC = moleculeA;
            }
          Voxel* moleculeD(D->getRandomAdjoiningVoxel(moleculeC, moleculeC));
          //Only proceed if we can find an adjoining vacant voxel
          //of A which can be occupied by D:
          if(moleculeD == NULL)
            {
              requeue();
              return;
            }
          D->addMolecule(moleculeD);
          A->removeMolecule(moleculeA);
          C->addMolecule(moleculeC);
        }
      //nonHD_A -> nonHD_C:
      else if(A && C && !D && !variableD)
        {
          Voxel* moleculeA(A->getRandomMolecule());
          //If the product C is not in the same Comp as A,
          //we need to find a vacant adjoining voxel of A that belongs
          //to the Comp of C:
          Voxel* moleculeC;
          if(A->getComp() != C->getComp())
            {
              moleculeC = C->getRandomAdjoiningVoxel(moleculeA);
              //Only proceed if we can find an adjoining vacant voxel
              //of A which can be occupied by C:
              if(moleculeC == NULL)
                {
                  requeue();
                  return;
                }
            }
          else
            {
              moleculeC = moleculeA;
            }
          A->removeMolecule(moleculeA);
          C->addMolecule(moleculeC);
        }
      //nonHD_A -> HD_C + HD_D:
      else if(A && variableC && variableD)
        {
          Voxel* moleculeA(A->getRandomMolecule());
          A->removeMolecule(moleculeA);
          variableC->addValue(1);
          variableD->addValue(1);
        }
      //nonHD_A -> nonHD_C + HD_D:
      else if(A && C && variableD)
        {
          Voxel* moleculeA(A->getRandomMolecule());
          Voxel* moleculeC;
          if(A->getComp() != C->getComp())
            {
              moleculeC = C->getRandomAdjoiningVoxel(moleculeA);
              //Only proceed if we can find an adjoining vacant voxel
              //of A which can be occupied by C:
              if(moleculeC == NULL)
                {
                  requeue();
                  return;
                }
            }
          else
            {
              moleculeC = moleculeA;
            }
          A->removeMolecule(moleculeA);
          C->addMolecule(moleculeC);
          variableD->addValue(1);
        }
      //nonHD_A -> HD_C:
      else if(A && variableC && !D && !variableD)
        {
          Voxel* moleculeA(A->getRandomMolecule());
          A->removeMolecule(moleculeA);
          variableC->addValue(1);
        }
      //nonHD_A -> nonHD_C + HD_D:
      //nonHD_A -> HD_C + nonHD_D:
      else if(A && ((variableC && D) || (C && variableD)))
        {
          Variable* HD_p(variableC);
          Species* nonHD_p(D);
          if(variableD)
            {
              HD_p = variableD;
              nonHD_p = C;
            }
          Voxel* moleculeA(A->getRandomMolecule());
          Voxel* molecule;
          if(A->getComp() != nonHD_p->getComp())
            {
              molecule = nonHD_p->getRandomAdjoiningVoxel(moleculeA);
              //Only proceed if we can find an adjoining vacant voxel
              //of A which can be occupied by nonHD:
              if(molecule == NULL)
                {
                  requeue();
                  return;
                }
            }
          else
            {
              molecule = moleculeA;
            }
          A->removeMolecule(moleculeA);
          nonHD_p->addMolecule(molecule);
          HD_p->addValue(1);
        }
      //HD_A -> nonHD_C + nonHD_D:
      else if(variableA && C && D)
        {
        }
      //HD_A -> nonHD_C:
      else if(variableA && C && !D && !variableD)
        {
          Voxel* moleculeC;
          Comp* compA(theSpatiocyteStepper->system2Comp(
                         variableA->getSuperSystem()));
          if(compA == C->getComp() || compA->dimension == 3)
            {
              moleculeC = C->getRandomCompVoxel();
            }
          //Occupy C in a voxel of compartment C that adjoins compartment A
          //if A is a surface compartment:
          else
            {
              moleculeC = C->getRandomAdjoiningCompVoxel(compA);
            }
          if(moleculeC == NULL)
            {
              requeue();
              return;
            }
          variableA->addValue(-1);
          C->addMolecule(moleculeC);
        }
      //HD_A -> HD_C + HD_D:
      else if(variableA && variableC && variableD)
        {
          variableA->addValue(-1);
          variableC->addValue(1);
          variableD->addValue(1);
        }
      //HD_A -> HD_C:
      else if(variableA && variableC && !D && !variableD)
        {
          variableA->addValue(-1);
          variableC->addValue(1);
        }
      //HD_A -> nonHD_C + HD_D:
      //HD_A -> HD_C + nonHD_D:
      else if(variableA && ((variableC && D) || (C && variableD)))
        {
          Variable* HD_p(variableC);
          Species* nonHD_p(D);
          if(variableD)
            {
              HD_p = variableD;
              nonHD_p = C;
            }
          Voxel* molecule(nonHD_p->getRandomCompVoxel());
          if(molecule == NULL)
            {
              requeue();
              return;
            }
          variableA->addValue(-1);
          nonHD_p->addMolecule(molecule);
          HD_p->addValue(1);
        }
    }
  //theOrder = 2
  else
    {
      //HD + HD -> product(s)
      if(variableA && variableB)
        {
          //HD + HD -> HD: 
          if(variableC && !variableD && !D)
            {
              variableA->addValue(-1);
              variableB->addValue(-1);
              variableC->addValue(1);
            }
          //HD + HD -> nonHD: 
          else if(C && !variableD && !D)
            { 
              Voxel* molecule(C->getRandomCompVoxel());
              if(molecule == NULL)
                {
                  requeue();
                  return;
                }
              variableA->addValue(-1);
              variableB->addValue(-1);
              C->addMolecule(molecule);
            }
          //HD + HD -> HD + HD: 
          else if(variableC && variableD)
            {
              variableA->addValue(-1);
              variableB->addValue(-1);
              variableC->addValue(1);
              variableD->addValue(1);
            }
          //HD + HD -> HD + nonHD: 
          //HD + HD -> nonHD + HD: 
          else if((variableC && D) || (C && variableD))
            {
              Variable* HD_p(variableC);
              Species* nonHD_p(D);
              if(variableD)
                {
                  HD_p = variableD;
                  nonHD_p = C;
                }
              Voxel* molecule(nonHD_p->getRandomCompVoxel());
              if(molecule == NULL)
                {
                  requeue();
                  return;
                }
              variableA->addValue(-1);
              variableB->addValue(-1);
              nonHD_p->addMolecule(molecule);
              HD_p->addValue(1);
            }
          //HD + HD -> nonHD + nonHD: 
          else if(C && D)
            {
              Voxel* moleculeC(C->getRandomCompVoxel());
              if(moleculeC == NULL)
                {
                  requeue();
                  return;
                }
              Voxel* moleculeD(D->getRandomAdjoiningVoxel(moleculeC));
              //Only proceed if we can find an adjoining vacant voxel
              //of C which can be occupied by D:
              if(moleculeD == NULL)
                {
                  requeue();
                  return;
                }
              variableA->addValue(-1);
              variableB->addValue(-1);
              C->addMolecule(moleculeC);
              D->addMolecule(moleculeD);
            }
        }
      //HD + nonHD -> product(s)
      //nonHD + HD -> product(s)
      else
        {
          Species* nonHD(A);
          Variable* HD(variableB);
          if(B)
            {
              nonHD = B;
              HD = variableA;
            }
          //nonHD + HD (+E) -> nonHD + nonHD: 
          //HD + nonHD (+E) -> nonHD + nonHD: 
          if(C && D)
            {
              Voxel* moleculeNonHD(nonHD->getRandomMolecule());
              //If the product C is not in the same Comp as nonHD,
              //we need to find a vacant adjoining voxel of nonHD that belongs
              //to the Comp of C:
              Voxel* moleculeC(NULL);
              Voxel* moleculeD(NULL);
              Voxel* moleculeE(NULL);
              if(nonHD->getVacantID() == C->getVacantID())
                {
                  moleculeC = moleculeNonHD;
                  moleculeD = D->getRandomAdjoiningVoxel(moleculeC, moleculeC);
                  if(moleculeD == NULL)
                    {
                      requeue();
                      return;
                    }
                }
              else if(nonHD->getVacantID() == D->getVacantID())
                {
                  moleculeD = moleculeNonHD;
                  moleculeC = C->getRandomAdjoiningVoxel(moleculeD, moleculeD);
                  if(moleculeC == NULL)
                    {
                      requeue();
                      return;
                    }
                }
              else
                {
                  moleculeC = C->getRandomAdjoiningVoxel(moleculeNonHD);
                  if(moleculeC == NULL)
                    {
                      //Only proceed if we can find an adjoining vacant voxel
                      //of nonND which can be occupied by C:
                      requeue();
                      return;
                    }
                  moleculeD = D->getRandomAdjoiningVoxel(moleculeC, moleculeC);
                  if(moleculeD == NULL)
                    {
                      requeue();
                      return;
                    }
                }
              HD->addValue(-1);
              nonHD->removeMolecule(moleculeNonHD);
              C->addMolecule(moleculeC);
              D->addMolecule(moleculeD);
            }
          //nonHD + HD -> nonHD:
          //HD + nonHD -> nonHD:
          else if(C && !D && !variableD)
            {
              Voxel* moleculeNonHD(nonHD->getRandomMolecule());
              //If the product C is not in the same Comp as nonHD,
              //we need to find a vacant adjoining voxel of nonHD that belongs
              //to the Comp of C:
              Voxel* moleculeC;
              if(nonHD->getComp() != C->getComp())
                {
                  moleculeC = C->getRandomAdjoiningVoxel(moleculeNonHD);
                  //Only proceed if we can find an adjoining vacant voxel
                  //of nonHD which can be occupied by C:
                  if(moleculeC == NULL)
                    {
                      requeue();
                      return;
                    }
                }
              else
                {
                  moleculeC = moleculeNonHD;
                }
              HD->addValue(-1);
              nonHD->removeMolecule(moleculeNonHD);
              C->addMolecule(moleculeC);
            }
          //HD + nonHD -> HD + nonHD:
          //HD + nonHD -> nonHD + HD:
          //nonHD + HD -> HD + nonHD:
          //nonHD + HD -> nonHD + HD:
          else if((variableC && D) || (C && variableD))
            {
              Variable* HD_p(variableC);
              Species* nonHD_p(D);
              if(variableD)
                {
                  HD_p = variableD;
                  nonHD_p = C;
                }
              Voxel* moleculeNonHD(nonHD->getRandomMolecule());
              //If the nonHD product is not in the same Comp as nonHD,
              //we need to find a vacant adjoining voxel of nonHD that belongs
              //to the Comp of nonHD product:
              Voxel* moleculeNonHD_p;
              if(nonHD->getComp() != nonHD_p->getComp())
                {
                  moleculeNonHD_p = 
                    nonHD_p->getRandomAdjoiningVoxel(moleculeNonHD);
                  //Only proceed if we can find an adjoining vacant voxel
                  //of nonHD which can be occupied by C:
                  if(moleculeNonHD_p == NULL)
                    {
                      requeue();
                      return;
                    }
                }
              else
                {
                  moleculeNonHD_p = moleculeNonHD;
                }
              HD->addValue(-1);
              nonHD->removeMolecule(moleculeNonHD);
              HD_p->addValue(1);
              nonHD_p->addMolecule(moleculeNonHD_p);
            }
        }
    }
  ReactionProcess::fire();
}

void SpatiocyteNextReactionProcess::initializeThird()
{
  ReactionProcess::initializeThird();
  if(p != -1)
    {
      return;
    }
  Comp* compA(NULL);
  Comp* compB(NULL);
  Comp* compC(NULL);
  Comp* compD(NULL);
  if(A)
    {
      compA = A->getComp();
    }
  else
    {
      compA = theSpatiocyteStepper->system2Comp(
                         variableA->getSuperSystem());
    }
  if(B)
    {
      compB = B->getComp();
    }
  else if(variableB)
    {
      compB = theSpatiocyteStepper->system2Comp(
                         variableB->getSuperSystem());
    }
  if(C)
    {
      compC = C->getComp();
    }
  else
    {
      compC = theSpatiocyteStepper->system2Comp(
                         variableC->getSuperSystem());
    }
  if(D)
    {
      compD = D->getComp();
    }
  else if(variableD)
    {
      compD = theSpatiocyteStepper->system2Comp(
                         variableD->getSuperSystem());
    }
  double aVolume(0);
  double anArea(0);
  if(theOrder == 0)
    {
      double aSpace(0);
      if(SpaceC > 0)
        {
          aSpace = SpaceC;
          pFormula << "[aSpace = SpaceC:" << aSpace << "]";
        }
      else if(compC->dimension == 2)
        {
          aSpace = compC->actualArea;
          pFormula << "[aSpace = compC.Area:" << aSpace << "]";
        }
      else
        {
          aSpace = compC->actualVolume;
          pFormula << "[aSpace = compC.Volume:" << aSpace << "]";
        }
      p = k*aSpace;
      pFormula << "[k*aSpace = " << k << "*" << aSpace << "]";
    }
  else if(theOrder == 1) 
    {
      //Convert the unit m/s of k to 1/s for p if the reaction is a surface
      //adsorption reaction:
      if(compA->dimension == 3 && compC->dimension == 2)
        { 
          if(SpaceA > 0)
            {
              aVolume = SpaceA;
              pFormula << "[aVolume = SpaceA:" << aVolume << "]";
            }
          else
            {
              aVolume = compA->actualVolume;
              pFormula << "[aVolume = compA.Volume:" << aVolume << "]";
            }
          if(SpaceC > 0)
            {
              anArea = SpaceC;
              pFormula << "[anArea = SpaceC:" << anArea << "]";
            }
          else
            {
              anArea = compC->actualArea;
              pFormula << "[anArea = compC.Area:" << anArea << "]";
            }
          k = k*anArea/aVolume;
          pFormula << "[k*anArea/aVolume = " << k << "*" << anArea << "/"
            << aVolume << "]";
        }
      p = k;
      pFormula << "[k = " << k << "]";
    }
  else if(theOrder == 2)
    {
      //If there are two products that don't belong to the same compartment,
      //the reactants must also belong to different compartments:
      if((compD && compD != compC) && (compA == compB))
        {
          NEVER_GET_HERE;
        }
      //If volume + surface = k(volume)(surface) or
      //   volume + surface = k(surface)(volume) or
      //   surface + volume = k(volume)(surface) or
      //   surface + volume = k(surface)(volume)
      if((compD && (
        (compC->dimension == 3 && compD->dimension == 2 &&
         compA->dimension == 3 && compB->dimension == 2) ||
        (compC->dimension == 3 && compD->dimension == 2 &&
         compA->dimension == 2 && compB->dimension == 3) ||
        (compC->dimension == 2 && compD->dimension == 3 &&
         compA->dimension == 3 && compB->dimension == 2) ||
        (compC->dimension == 2 && compD->dimension == 3 &&
         compA->dimension == 2 && compB->dimension == 3))) ||
      //If volume (+volume) = k(volume)(volume) or
      //   surface (+surface) = k(volume)(surface) or
      //   surface (+surface) = k(surface)(volume)
         ((compC->dimension == 3 && compA->dimension == 3
          && compB->dimension == 3) ||
         (compC->dimension == 2 && compA->dimension == 3 
          && compB->dimension == 2) ||
         (compC->dimension == 2 && compA->dimension == 2 
          && compB->dimension == 3)))
        {
          if(compA->dimension == 3)
            {
              if(SpaceA > 0)
                {
                  aVolume = SpaceA;
                  pFormula << "[aVolume = SpaceA:" << aVolume << "]";
                }
              else
                {
                  aVolume = compA->actualVolume;
                  pFormula << "[aVolume = compA.Volume:" << aVolume << "]";
                }
            }
          else
            {
              if(SpaceB > 0)
                {
                  aVolume = SpaceB;
                  pFormula << "[aVolume = SpaceB:" << aVolume << "]";
                }
              else
                {
                  aVolume = compB->actualVolume;
                  pFormula << "[aVolume = compB.Volume:" << aVolume << "]";
                }
            }
          //unit of k is in m^3/s
          p = k/aVolume;
          pFormula << "[k/aVolume = " << k << "/" << aVolume << "]";
        }
      //If surface (+surface) = k(surface)(surface) or
      //   volume (+volume) = k(volume)(surface) or
      //   volume (+volume) = k(surface)(volume)
      else if((compC->dimension == 2 && compA->dimension == 2 
               && compB->dimension == 2) ||
              (compC->dimension == 3 && compA->dimension == 3 
               && compB->dimension == 2) ||
              (compC->dimension == 3 && compA->dimension == 2 
               && compB->dimension == 3))
        {
          if(compA->dimension == 2)
            {
              if(SpaceA > 0)
                {
                  anArea = SpaceA;
                  pFormula << "[anArea = SpaceA:" << anArea << "]";
                }
              else
                {
                  anArea = compA->actualArea;
                  pFormula << "[anArea = compA.Area:" << anArea << "]";
                }
            }
          else
            {
              if(SpaceB > 0)
                {
                  anArea = SpaceB;
                  pFormula << "[anArea = SpaceB:" << anArea << "]";
                }
              else
                {
                  anArea = compB->actualArea;
                  pFormula << "[anArea = compB.Area:" << anArea << "]";
                }
            }
          //unit of k is in m^2/s
          p = k/anArea;
          pFormula << "[k/anArea = " << k << "/" << anArea << "]";
        }
      else
        {
          NEVER_GET_HERE;
        }
      //A + A -> products
      if(getZeroVariableReferenceOffset() == 1)
        {
          p = k;
          pFormula << "[k = " << k << "]";
        }
    }
  else
    {
      NEVER_GET_HERE;
    } 
}

void SpatiocyteNextReactionProcess::printParameters()
{
  String aProcess(String(getPropertyInterface().getClassName()) + 
                                      "[" + getFullID().asString() + "]");
  std::cout << aProcess << std::endl;
  if(A)
    {
      std::cout << "  " << getIDString(A);
    }
  else if(variableA)
    {
      std::cout << "  " << getIDString(variableA);
    }
  if(B)
    {
      std::cout << " + " << getIDString(B);
    }
  else if(variableB)
    {
      std::cout << " + " << getIDString(variableB);
    }
  if(!A && !variableA)
    {
      if(C)
        {
          std::cout << "0 -> " << getIDString(C);
        }
      else if(variableC)
        {
          std::cout << "0 -> " << getIDString(variableC);
        }
    }
  else
    {
      if(C)
        {
          std::cout << " -> " << getIDString(C);
        }
      else if(variableC)
        {
          std::cout << " -> " << getIDString(variableC);
        }
    }
  if(D)
    {
      std::cout << " + " << getIDString(D);
    }
  else if(variableD)
    {
      std::cout << " + " << getIDString(variableD);
    }
  std::cout << " k:" << k << " p = " << pFormula.str() << " = " << p
    << std::endl;
}


GET_METHOD_DEF(Real, StepInterval, SpatiocyteNextReactionProcess)
{
  return getPropensity_R()*
    (-log(gsl_rng_uniform_pos(getStepper()->getRng())));
}

