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


#include <iostream>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <sstream>
#include <gtkmm.h>
#include <gtkglmm.h>

#ifdef G_OS_WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif

#include <png.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <gtkmm/main.h>
#include <gtkmm/table.h>
#include <gtkmm/window.h>
#include <gtkmm/ruler.h>
#include <gtkmm/drawingarea.h>
#include <netinet/in.h>
#include "SpatiocyteVisualizer.hpp"

#define PI 3.1415926535897932384626433832795028841971693993751
#define MAX_COLORS 20
#define PNG_NUM_MAX 9999999
const unsigned int GLScene::TIMEOUT_INTERVAL = 10;

double hue2rgb( double a, double b, double h )
{
  if( h < 0 )
    {
      h += 1;
    }
  if( h > 1 )
    {
      h -= 1;
    }
  if( 6*h < 1 )
    {
      return (b-a)*6*h+a;
    }
  if( 2*h < 1 )
    {
      return b;
    }
  if( 3*h < 2 )
    {
      return a+(b-a)*((2./3 )-h)*6;
    }
  return a;
}

// hsl[0:1]
// rgb[0:255]
void hsl2rgb(double h, double l, float* r, float* g, float* b)
{
  double s(1);
  double a;
  double d;
  if( s == 0 )
    {
      *r = l;
      *g = l;
      *b = l;
    }
  else
    {
      if( l<0.5 ) 
        {
          d = l*(1+s);
        }
      else           
        {
          d = l+s-s*l;
        }
      a = 2*l-d; 
      *r = hue2rgb( a, d, h + ( 1./3 ) );
      *g = hue2rgb( a, d, h );
      *b = hue2rgb( a, d, h - ( 1./3 ) );
    } 
}

GLScene::GLScene(const Glib::RefPtr<const Gdk::GL::Config>& config,
                 const char* aBaseName)
: Gtk::GL::DrawingArea(config),
  m_Run(false),
  m_RunReverse(false),
  m_stepCnt(0),
  thePngNumber(1),
  theRotateAngle(5.0),
  show3DMolecule(false),
  startRecord(false),
  theResetTime(0),
  showTime(true),
  theMeanCoordSize(0),
  xAngle(0),
  yAngle(0),
  zAngle(0)
{
  add_events(Gdk::VISIBILITY_NOTIFY_MASK); 
  
  std::ostringstream aParentFileName;
  aParentFileName << aBaseName << std::ends;
  std::ifstream aParentFile(aParentFileName.str().c_str(),
                            std::ios::binary );
  aParentFile.read((char *)(&theThreadSize), sizeof(unsigned int));
  aParentFile.close();
  theFile = new std::ifstream*[theThreadSize];
  for(unsigned int i(0); i!=theThreadSize; ++i)
    {
      std::ostringstream aFileName;
      aFileName << aBaseName << std::ends;
      theFile[i] = 
        new std::ifstream( aFileName.str().c_str(), std::ios::binary );
    }
  theFile[0]->read((char*) (&theThreadSize), sizeof(theThreadSize));
  theFile[0]->read((char*) (&theLatticeType), sizeof(theLatticeType));
  theFile[0]->read((char*) (&theMeanCount), sizeof(theMeanCount));
  theFile[0]->read((char*) (&theStartCoord), sizeof(theStartCoord));
  theFile[0]->read((char*) (&theRowSize), sizeof(theRowSize));
  theFile[0]->read((char*) (&theLayerSize), sizeof(theLayerSize));
  theFile[0]->read((char*) (&theColSize), sizeof(theColSize));
  theFile[0]->read((char*) (&theRealRowSize), sizeof(theRealRowSize));
  theFile[0]->read((char*) (&theRealLayerSize), sizeof(theRealLayerSize));
  theFile[0]->read((char*) (&theRealColSize), sizeof(theRealColSize));
  theFile[0]->read((char*) (&theSpeciesSize), sizeof(theSpeciesSize));
  theFile[0]->read((char*) (&thePolymerSize), sizeof(thePolymerSize));
  theFile[0]->read((char*) (&theReservedSize), sizeof(theReservedSize));
  theFile[0]->read((char*) (&theLogMarker), sizeof(theLogMarker));
  theFile[0]->read((char*) (&theResolution), sizeof(theResolution));
  unsigned int aSourceSize(thePolymerSize);
  unsigned int aTargetSize(thePolymerSize);
  unsigned int aSharedSize(thePolymerSize);
  theTotalCoordSpeciesSize = theSpeciesSize+aSourceSize+aTargetSize+aSharedSize+
    theReservedSize;
  theTotalSpeciesSize = theTotalCoordSpeciesSize+thePolymerSize;
  theSpeciesNameList = new char*[theTotalSpeciesSize];
  for(unsigned int i(0); i!=theSpeciesSize; ++i)
    {
      unsigned int aStringSize;
      theFile[0]->read((char*) (&aStringSize), sizeof(aStringSize));
      theSpeciesNameList[i] = new char[aStringSize];
      char* buffer;
      buffer = new char[aStringSize+1];
      theFile[0]->read(buffer, aStringSize);
      buffer[aStringSize] = '\0';
      sscanf(buffer, "Variable:%s", theSpeciesNameList[i]);
      std::cout << theSpeciesNameList[i] << std::endl;
    }
  for(unsigned int i(0); i!=thePolymerSize; ++i)
    {
      unsigned int aPolySpeciesIndex;
      theFile[0]->read((char*) (&aPolySpeciesIndex), sizeof(aPolySpeciesIndex));
      thePolySpeciesList.push_back(aPolySpeciesIndex);
      theSpeciesNameList[theSpeciesSize+i] = new char[20];
      sprintf(theSpeciesNameList[theSpeciesSize+i], "source");
      theSpeciesNameList[theSpeciesSize+thePolymerSize+i] = new char[20];
      sprintf(theSpeciesNameList[theSpeciesSize+thePolymerSize+i], "target");
      theSpeciesNameList[theSpeciesSize+thePolymerSize*2+i] = new char[20];
      sprintf(theSpeciesNameList[theSpeciesSize+thePolymerSize*2+i], "shared");
      theSpeciesNameList[theSpeciesSize+thePolymerSize*3+
        theReservedSize+i] = new char[20];
      sprintf(theSpeciesNameList[theSpeciesSize+thePolymerSize*3+
              theReservedSize+i], "poly");
    }
  for(unsigned int i(0); i!=theReservedSize; ++i)
    {
      theSpeciesNameList[theSpeciesSize+thePolymerSize*3+i] = new char[20];
      sprintf(theSpeciesNameList[theSpeciesSize+thePolymerSize*3+i],
              "tmp %d", i);
    }
  theOriCol = theStartCoord/(theRowSize*theLayerSize);
  theRegionSep = new unsigned int[theThreadSize*3*2];
  for(unsigned int i(0); i!=theThreadSize*3; ++i)
    {
      theFile[0]->read((char*) (&theRegionSep[i]), sizeof(unsigned int));
      theRegionSep[i] = theRegionSep[i]/(theRowSize*theLayerSize)-
        theOriCol;
    }
  theSpeciesColor = new Color[theTotalSpeciesSize];
  theSpeciesVisibility = new bool[theTotalSpeciesSize];
  theSpeciesVisibility = new bool[theTotalSpeciesSize];
  theXUpBound = new unsigned int[theTotalSpeciesSize];
  theXLowBound = new unsigned int[theTotalSpeciesSize];;
  theYUpBound = new unsigned int[theTotalSpeciesSize];
  theYLowBound = new unsigned int[theTotalSpeciesSize];
  theZUpBound = new unsigned int[theTotalSpeciesSize];
  theZLowBound = new unsigned int[theTotalSpeciesSize];
  theMoleculeSize = new unsigned int*[theThreadSize];
  thePolymerMoleculeSize = new unsigned int*[theThreadSize];
  theCoords = new unsigned int**[theThreadSize];
  theMeanCoords = new unsigned int*[theThreadSize];
  theFrequency = new unsigned int**[theThreadSize];
  thePoints = new Point**[theThreadSize];
  for(unsigned int i(0); i!=theThreadSize; ++i)
    {
      theCoords[i] = new unsigned int*[theTotalCoordSpeciesSize];
      theMeanCoords[i] = new unsigned int[1];
      theFrequency[i] = new unsigned int*[theSpeciesSize];
      theMoleculeSize[i] = new unsigned int[theTotalCoordSpeciesSize];
      for(unsigned int j(0); j!=theTotalCoordSpeciesSize; ++j)
        {
          theSpeciesVisibility[j] = true; 
          theXUpBound[j] = 0;
          theXLowBound[j] = 0;
          theYUpBound[j] = theLayerSize;
          theYLowBound[j] = 0;
          theZUpBound[j] = theRowSize;
          theZLowBound[j] = 0;
          theMoleculeSize[i][j] = 0;
          theCoords[i][j] = new unsigned int[1];
        }
      for(unsigned int j(0); j!=theSpeciesSize; ++j )
        {
          theFrequency[i][j] = new unsigned int[1];
        }
      thePoints[i] = new Point*[thePolymerSize];
      thePolymerMoleculeSize[i] = new unsigned int[thePolymerSize];
      for(unsigned int j(0); j!=thePolymerSize; ++j )
        {
          theSpeciesVisibility[j+theTotalCoordSpeciesSize] = false;
          thePolymerMoleculeSize[i][j] = 0;
          thePoints[i][j] = new Point[1];
        }
    }
  /*
  for( unsigned int i(0); i!=theSpeciesSize; ++i )
    {
      if(std::find(thePolySpeciesList.begin(),
                   thePolySpeciesList.end(), i) == 
         thePolySpeciesList.end())
        {
          theSpeciesVisibility[i] = true;
        }
    }
    */
  double hueInterval(1.0/double(theSpeciesSize));
  double speciesLuminosity(0.4);
  double sourceLuminosity(0.6);
  double targetLuminosity(0.75);
  double sharedLuminosity(0.8);
  double polyLuminosity(0.3);
  theSpeciesColor[0].r = 0.9;
  theSpeciesColor[0].g = 0.9;
  theSpeciesColor[0].b = 0.9;
  for(unsigned int i(1); i!=theSpeciesSize; ++i)
    {
      hsl2rgb(hueInterval*i, speciesLuminosity,
              &theSpeciesColor[i].r,
              &theSpeciesColor[i].g,
              &theSpeciesColor[i].b);
    }
  for(unsigned int i(0); i!=thePolymerSize; ++i)
    {
      hsl2rgb(hueInterval*thePolySpeciesList[i],
              sourceLuminosity,
              &theSpeciesColor[theSpeciesSize+i].r,
              &theSpeciesColor[theSpeciesSize+i].g,
              &theSpeciesColor[theSpeciesSize+i].b);
      hsl2rgb(hueInterval*thePolySpeciesList[i],
              targetLuminosity,
              &theSpeciesColor[theSpeciesSize+thePolymerSize+i].r,
              &theSpeciesColor[theSpeciesSize+thePolymerSize+i].g,
              &theSpeciesColor[theSpeciesSize+thePolymerSize+i].b);
      hsl2rgb(hueInterval*thePolySpeciesList[i],
              sharedLuminosity,
              &theSpeciesColor[theSpeciesSize+thePolymerSize*2+i].r,
              &theSpeciesColor[theSpeciesSize+thePolymerSize*2+i].g,
              &theSpeciesColor[theSpeciesSize+thePolymerSize*2+i].b);
      hsl2rgb(hueInterval*thePolySpeciesList[i],
              polyLuminosity,
              &theSpeciesColor[theTotalCoordSpeciesSize+i].r,
              &theSpeciesColor[theTotalCoordSpeciesSize+i].g,
              &theSpeciesColor[theTotalCoordSpeciesSize+i].b);
    }
  hueInterval = 1.0/double(theReservedSize);
  speciesLuminosity = 0.6;
  for( unsigned int i(theTotalCoordSpeciesSize-theReservedSize);
       i!=theTotalCoordSpeciesSize; ++i )
    {
      hsl2rgb(hueInterval*(i-(theTotalCoordSpeciesSize-theReservedSize)),
              speciesLuminosity,
              &theSpeciesColor[i].r,
              &theSpeciesColor[i].g,
              &theSpeciesColor[i].b); 
      theSpeciesVisibility[i] = false;
    }

  std::cout << "thread:" << theThreadSize <<  " row:" << theRowSize <<
    " col:" << theColSize  << " layer:" << theLayerSize << " marker:" <<
    theLogMarker << std::endl << std::flush;
  theFile[0]->read((char*) (&thePrevSize), sizeof(thePrevSize));
  theFile[0]->read((char*) (&theNextSize), sizeof(theNextSize));
  //load the surface coordinates:
  loadCoords();
  theOriRow = 0;
  theOriLayer = 0;
  /*
  theColSize = 5;
  theRowSize = 4;
  theLayerSize = 3;
  */
  theRadius = 0.5;
  switch(theLatticeType)
    {
    case HCP_LATTICE: 
      theHCPk = theRadius/sqrt(3); 
      theHCPl = theRadius*sqrt(3);
      theHCPh = theRadius*sqrt(8.0/3.0); // for division require .0
      if(theMeanCount)
        {
          thePlot3DFunction = &GLScene::plotMean3DHCPMolecules;
          theLoadCoordsFunction = &GLScene::loadMeanCoords;
        }
      else
        {
          thePlotFunction = &GLScene::plotHCPPoints;
          thePlot3DFunction = &GLScene::plot3DHCPMolecules;
          theLoadCoordsFunction = &GLScene::loadCoords;
        }
      break;
    case CUBIC_LATTICE:
      if(theMeanCount)
        {
          thePlot3DFunction = &GLScene::plotMean3DCubicMolecules;
          theLoadCoordsFunction = &GLScene::loadMeanCoords;
        }
      else
        {
          thePlotFunction = &GLScene::plotCubicPoints;
          thePlot3DFunction = &GLScene::plot3DCubicMolecules;
          theLoadCoordsFunction = &GLScene::loadCoords;
        }
      break;
    }
  ViewSize = 1.05*sqrt((theRealColSize)*(theRealColSize)+
                       (theRealLayerSize)*(theRealLayerSize)+
                       (theRealRowSize)*(theRealRowSize));
  if(ViewSize==0)
    { 
      ViewSize=1.0;
    }
  ViewMidx=(theRealColSize)/2.0;
  ViewMidy=(theRealLayerSize)/2.0; 
  ViewMidz=(theRealRowSize)/2.0;
  FieldOfView=45;
  Xtrans=Ytrans=0;
  Near=-ViewSize/2.0;
  Aspect=1.0;
  set_size_request((unsigned int) 400, 400);
  std::cout << "done" << std::endl;
}

GLScene::~GLScene()
{
}

void GLScene::setXUpBound(unsigned int aBound )
{
  for(unsigned int i(0); i!=theTotalCoordSpeciesSize; ++i )
    {
      if(theSpeciesVisibility[i])
        {
          theXUpBound[i] = aBound;
        }
    }
  queue_draw();
}

void GLScene::setXLowBound(unsigned int aBound )
{
  for(unsigned int i(0); i!=theTotalCoordSpeciesSize; ++i )
    {
      if(theSpeciesVisibility[i])
        {
          theXLowBound[i] = aBound;
        }
    }
  queue_draw();
}

void GLScene::setYUpBound(unsigned int aBound )
{
  for(unsigned int i(0); i!=theTotalCoordSpeciesSize; ++i )
    {
      if(theSpeciesVisibility[i])
        {
          theYUpBound[i] = aBound;
        }
    }
  queue_draw();
}

void GLScene::setYLowBound(unsigned int aBound )
{
  for(unsigned int i(0); i!=theTotalCoordSpeciesSize; ++i )
    {
      if(theSpeciesVisibility[i])
        {
          theYLowBound[i] = aBound;
        }
    }
  queue_draw();
}

void GLScene::setZUpBound(unsigned int aBound )
{
  for(unsigned int i(0); i!=theTotalCoordSpeciesSize; ++i )
    {
      if(theSpeciesVisibility[i])
        {
          theZUpBound[i] = aBound;
        }
    }
  queue_draw();
}

void GLScene::setZLowBound(unsigned int aBound )
{
  for(unsigned int i(0); i!=theTotalCoordSpeciesSize; ++i )
    {
      if(theSpeciesVisibility[i])
        {
          theZLowBound[i] = aBound;
        }
    }
  queue_draw();
}

void GLScene::set3DMolecule(bool is3D)
{
  show3DMolecule = is3D;
  queue_draw();
}

void GLScene::setShowTime(bool isShowTime)
{
  showTime = isShowTime;
  queue_draw();
}

void GLScene::setRecord(bool isRecord)
{
  std::cout << "start rec" << std::endl;
  startRecord = isRecord;
}

void GLScene::resetTime()
{
  theResetTime = theCurrentTime;
  queue_draw();
}

void GLScene::setSpeciesVisibility(unsigned int id, bool isVisible)
{
  theSpeciesVisibility[id] = isVisible;
  queue_draw();
}

bool GLScene::getSpeciesVisibility(unsigned int id)
{
  return theSpeciesVisibility[id];
}

void GLScene::setControlBox(ControlBox* aControl)
{
  m_control = aControl;
}

void GLScene::setReverse(bool isReverse)
{
  m_RunReverse = isReverse;
}

Color GLScene::getSpeciesColor(unsigned int id)
{
  return theSpeciesColor[id];
}

void GLScene::setSpeciesColor(unsigned int id, Color aColor)
{
  theSpeciesColor[id] = aColor;
  queue_draw();
}

char* GLScene::getSpeciesName(unsigned int id)
{
  return theSpeciesNameList[id];
}

void GLScene::on_realize()
{
  Gtk::GL::DrawingArea::on_realize();
  Glib::RefPtr<Gdk::GL::Window> glwindow = get_gl_window();
  if (!glwindow->gl_begin(get_gl_context()))
    {
      return;
    }
  //background color3D:
  glClearColor (0, 0, 0, 0);
  glClearDepth (1);
  if(!theMeanCount)
    {
      glEnable(GL_DEPTH_TEST); //To darken molecules farther away
      glDepthFunc(GL_LESS); //To show the molecule only if it is nearer (less)
    }
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  if(!theMeanCount)
    {
      // This hint is for antialiasing
      glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST); 
    }
  glEnable(GL_TEXTURE_2D);
  glColorMaterial( GL_FRONT_AND_BACK, GL_DIFFUSE );
  glEnable(GL_COLOR_MATERIAL);
  glMatrixMode(GL_MODELVIEW);
  glTranslatef(-ViewMidx,-ViewMidy,-ViewMidz); 

  /*
  m_FontListBase = glGenLists(128); 
  m_FontString = "Courier 8";
  Pango::FontDescription font_desc(m_FontString); 
  Glib::RefPtr<Pango::Font> font(Gdk::GL::Font::use_pango_font(
                                         font_desc, 0, 128, m_FontListBase));
  Pango::FontMetrics font_metrics(font->get_metrics()); 
  m_FontHeight = font_metrics.get_ascent() + font_metrics.get_descent();
  m_FontHeight = PANGO_PIXELS(m_FontHeight);
  m_FontWidth = PANGO_PIXELS(font_metrics.get_approximate_digit_width());
  */
  if(theMeanCount)
    {
      //for GFP visualization:
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
  else
    {
      //for 3D molecules:
      glEnable(GL_LIGHTING);
      GLfloat LightAmbient[]= { 0.8, 0.8, 0.8, 1 }; 
      GLfloat LightDiffuse[]= { 1, 1, 1, 1 };
      GLfloat LightPosition[]= { theLayerSize/2, theRowSize/2, theColSize, 1 };
      glLightfv(GL_LIGHT0, GL_AMBIENT, LightAmbient);
      glLightfv(GL_LIGHT0, GL_DIFFUSE, LightDiffuse);
      glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);
      glEnable(GL_LIGHT0);
    }
  GLUquadricObj* qobj = gluNewQuadric();
  gluQuadricDrawStyle(qobj, GLU_FILL);
  glNewList(SPHERE, GL_COMPILE);
  if(!theMeanCount)
    {
      gluSphere(qobj, 0.5, 30, 30);
    }
  else
    {
      gluSphere(qobj, 0.5, 10, 10);
    }
  glEndList();
  glNewList(BOX, GL_COMPILE);
  //drawBox(0,theRealColSize,0,theRealLayerSize,0,theRealRowSize);
  glEndList();

  /*
  glNewList(GRID, GL_COMPILE);
  plotGrid();
  glEndList();
  */
  glwindow->gl_end();
}

bool GLScene::on_expose_event(GdkEventExpose* event)
{
  Glib::RefPtr<Gdk::GL::Window> glwindow = get_gl_window();
  if (!glwindow->gl_begin(get_gl_context()))
    {
      return false;
    }
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  if(show3DMolecule)
    {
      (this->*thePlot3DFunction)();
    }
  else
    { 
      (this->*thePlotFunction)();
    }
  if(showTime)
    {
      /*
      glListBase(m_FontListBase);
      char buffer[50];
      sprintf(buffer, "t = %.1fs", theCurrentTime-theResetTime);
      m_timeString = buffer;
      //glColor3f(0.2, 0.5, 0.8);
      glColor3f(1.0, 1.0, 1.0);
      glRasterPos3f(ViewMidx-(m_timeString.length()*m_FontWidth)/2.0, 0, 0);
      glCallLists(m_timeString.length(), GL_UNSIGNED_BYTE,
                  m_timeString.c_str());
                  */
    }
  glCallList(BOX);
  //glCallList(GRID);
  glwindow->swap_buffers();
  glwindow->gl_end();
  return true;
}

bool GLScene::writePng()
{
  char filename[256];
  char str[256];
  sprintf(str,"image%%0%ii.png",(int)log10(PNG_NUM_MAX)+1);
  sprintf(filename,str,thePngNumber);
  ++thePngNumber; 
  GLfloat w(get_width());
  GLfloat h(get_height());
  unsigned int screenWidth(static_cast<GLsizei>(w));
  unsigned int screenHeight(static_cast<GLsizei>(h));

  FILE *outFile;
  outFile = fopen(filename, "wb");
  if(outFile == NULL)
    {
      return false;
    } 
  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                NULL, NULL, NULL);
  if(!png_ptr)
    {
      fclose(outFile);
      return false;
    } 
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if(!info_ptr)
    {
      png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
      fclose(outFile);
      return false;
    } 
  if(setjmp(png_jmpbuf(png_ptr)))
    {
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      fclose(outFile);
      return false;
   }
  png_init_io(png_ptr, outFile);
  /* set the zlib compression level */
  /*png_set_compression_level(png_ptr, Z_NO_COMPRESSION);*/
  png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
  /* set other zlib parameters */
  png_set_compression_mem_level(png_ptr, 8);
  png_set_compression_strategy(png_ptr, Z_DEFAULT_STRATEGY);
  png_set_compression_window_bits(png_ptr, 15);
  png_set_compression_method(png_ptr, 8);
  png_set_compression_buffer_size(png_ptr, 8192);
  png_set_IHDR(png_ptr, info_ptr, screenWidth, screenHeight, 8,
               PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png_ptr, info_ptr); 
  GLubyte *image =
    (GLubyte*)malloc(screenWidth*screenHeight*sizeof(GLubyte)*3);
  if(!image)
    {
      return false;
    }
  glPixelStorei(GL_PACK_ALIGNMENT,1);
  glReadPixels(0, 0, screenWidth, screenHeight, GL_RGB,
               GL_UNSIGNED_BYTE, image); 
  GLubyte** rowPtrs = new GLubyte*[screenHeight];
  for(GLuint i = 0; i < screenHeight; i++)
    {
      rowPtrs[screenHeight - i - 1] = &(image[i * screenWidth * 3]);
    } 
  png_write_image(png_ptr, rowPtrs); 
  png_write_end(png_ptr, info_ptr); 
  png_destroy_write_struct(&png_ptr, &info_ptr); 
  free(rowPtrs);
  fclose(outFile);
  return true;
}


void GLScene::drawBox(GLfloat xlo, GLfloat xhi, GLfloat ylo, GLfloat yhi,
                      GLfloat zlo, GLfloat zhi)
{
  glBegin(GL_LINE_STRIP);
  glVertex3f(xlo,ylo,zlo);
  glVertex3f(xlo,ylo,zhi);
  glVertex3f(xlo,yhi,zhi);
  glVertex3f(xlo,yhi,zlo);
  glVertex3f(xlo,ylo,zlo);
  glVertex3f(xhi,ylo,zlo);
  glVertex3f(xhi,yhi,zlo);
  glVertex3f(xhi,yhi,zhi);
  glVertex3f(xhi,ylo,zhi);
  glVertex3f(xhi,ylo,zlo);
  glEnd();

  glBegin(GL_LINES);
  glVertex3f(xlo,ylo,zhi);glVertex3f(xhi,ylo,zhi);
  glVertex3f(xlo,yhi,zhi);glVertex3f(xhi,yhi,zhi);
  glVertex3f(xlo,yhi,zlo);glVertex3f(xhi,yhi,zlo);
  glEnd();
}

bool GLScene::on_configure_event(GdkEventConfigure* event)
{
  Glib::RefPtr<Gdk::GL::Window> glwindow = get_gl_window();
  if (!glwindow->gl_begin(get_gl_context()))
    {
      return false;
    }
  GLfloat w = get_width();
  GLfloat h = get_height();
  GLfloat nearold = Near;
  GLfloat m[16];
  Aspect = w/h;
  glViewport(0, 0, static_cast<GLsizei>(w), static_cast<GLsizei>(h));
  if(w>=h) Near=ViewSize/2.0/tan(FieldOfView*PI/180.0/2.0);
  else Near=ViewSize/2.0/tan(FieldOfView*Aspect*PI/180.0/2.0);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(FieldOfView,Aspect,Near,ViewSize+Near); 
  glMatrixMode(GL_MODELVIEW);
  glGetFloatv(GL_MODELVIEW_MATRIX,m);
  glLoadIdentity();
  glTranslatef(0,0,nearold-Near);
  glMultMatrixf(m);
  glwindow->gl_end();
  return true;
}

void GLScene::plotGrid()
{
  glColor3f(0,1,0);
  GLfloat x,y,z;
  const GLfloat c(theRadius*sqrt(2.0));
  int m(0);
  for( unsigned int i(0); i!=theColSize; ++i )
    {
      for( unsigned int j(0); j!=theLayerSize; ++j )
        { 
          for( unsigned int k(0); k!=theRowSize; ++k )
            { 
              setTranslucentColor(m,0.5);
              //if( j<2 && i<2 ) {
              glPushMatrix();
              x = i*2*theRadius + (j%2)*theRadius;
              y = j*c;
              z = (j%2)*theRadius + k*2*theRadius;
              glTranslatef(x,y,z);
              glCallList(SPHERE);
              glPopMatrix();
              //}
              m++;
            }
        }
    }
}

void GLScene::loadCoords()
{
  for(unsigned int i(0); i!=theThreadSize; ++i)
    {
      if(theFile[i]->eof() != true)
        {
          theFile[i]->read((char*) (&theCurrentTime), sizeof(theCurrentTime));
          unsigned int anIndex;
          // Get the species index
          theFile[i]->read((char*) (&anIndex), sizeof(anIndex));
          while( anIndex != theLogMarker && theFile[i]->eof() != true )
            { 
              // Get the number of molecules for the species
              if( theMoleculeSize[i][anIndex] != 0 )
                {
                  delete []theCoords[i][anIndex];
                }
              theFile[i]->read((char*) (&theMoleculeSize[i][anIndex]),
                               sizeof(unsigned int));
              if( theMoleculeSize[i][anIndex] != 0 )
                {
                  theCoords[i][anIndex] = 
                    new unsigned int[theMoleculeSize[i][anIndex]];
                  theFile[i]->read((char*) (theCoords[i][anIndex]), 
                         sizeof(unsigned int)*theMoleculeSize[i][anIndex]);
                }
              theFile[i]->read((char*) (&anIndex), sizeof(anIndex));
            }
          theFile[i]->read((char*) (&anIndex), sizeof(anIndex));
          while( anIndex != theLogMarker && theFile[i]->eof() != true )
            { 
              // Get the number of molecules for the species
              if( thePolymerMoleculeSize[i][anIndex] != 0 )
                {
                  delete []thePoints[i][anIndex];
                }
              theFile[i]->read((char*) (&thePolymerMoleculeSize[i][anIndex]),
                               sizeof(unsigned int));
              if( thePolymerMoleculeSize[i][anIndex] != 0 )
                {
                  thePoints[i][anIndex] = 
                    new Point[thePolymerMoleculeSize[i][anIndex]];
                  theFile[i]->read((char*) (thePoints[i][anIndex]), 
                       sizeof(Point)*thePolymerMoleculeSize[i][anIndex]);
                }
              theFile[i]->read((char*) (&anIndex), sizeof(anIndex));
            }
          theFile[i]->read((char*) (&thePrevSize), sizeof(int));
          theFile[i]->read((char*) (&theNextSize), sizeof(int));
        }
    }
}

void GLScene::loadMeanCoords()
{
  for(unsigned int i(0); i!=theThreadSize; ++i)
    {
      if(theFile[i]->eof() != true)
        {
          theFile[i]->read((char*) (&theCurrentTime), sizeof(theCurrentTime));
          theFile[i]->read((char*) (&theMeanCoordSize),
                           sizeof(theMeanCoordSize));
          delete []theMeanCoords[i];
          theMeanCoords[i] = new unsigned int[theMeanCoordSize];
          theFile[i]->read((char*) (theMeanCoords[i]), 
                           sizeof(unsigned int)*theMeanCoordSize);
          for(unsigned int j(0); j != theSpeciesSize; ++j)
            {
              delete []theFrequency[i][j];
              theFrequency[i][j] = new unsigned int[theMeanCoordSize];
              theFile[i]->read((char*) (theFrequency[i][j]), 
                               sizeof(unsigned int)*theMeanCoordSize);
            }
          theFile[i]->read((char*) (&thePrevSize), sizeof(int));
          theFile[i]->read((char*) (&theNextSize), sizeof(int));
        }
    }
}

void GLScene::plotMean3DHCPMolecules()
{
  unsigned int col, layer, row;
  double x,y,z;
  for(unsigned int i(0); i != theThreadSize; ++i)
    { 
      for(unsigned int k(0); k != theMeanCoordSize; ++k)
        {
          col = theMeanCoords[i][k]/(theRowSize*theLayerSize)-theOriCol; 
          layer = (theMeanCoords[i][k]%(theRowSize*theLayerSize))/theRowSize;
          row = (theMeanCoords[i][k]%(theRowSize*theLayerSize))%theRowSize;
          y = (col%2)*theHCPk + theHCPl*layer + theRadius;
          z = row*2*theRadius + ((layer+col)%2)*theRadius + theRadius;
          x = col*theHCPh + theRadius; 
          for(unsigned int j(0); j!=theSpeciesSize; ++j)
            {
              if(theSpeciesVisibility[j])
                {
                  Color clr(theSpeciesColor[j]);
                  double intensity((double)(theFrequency[i][j][k])/
                                   (double)(theMeanCount/4));
                  //glColor3f(clr.r*intensity, clr.g*intensity, clr.b*intensity); 
                  glColor4f(clr.r, clr.g, clr.b, intensity);
                  //glColor4f(clr.r*intensity, clr.g*intensity, clr.b*intensity,
                   //         0.5f); 
                  if(!( x <= theXUpBound[j] && x >= theXLowBound[j] &&
                      y <= theYUpBound[j] && y >= theYLowBound[j] &&
                      z <= theZUpBound[j] && z >= theZLowBound[j]))
                    {
                      glPushMatrix();
                      glTranslatef(x,y,z);
                      glCallList(SPHERE);
                      glPopMatrix();
                    }
                }
            }
        }
      for( unsigned int j(theSpeciesSize);
           j!=theTotalCoordSpeciesSize; ++j )
        {
          if(theSpeciesVisibility[j])
            {
              Color clr(theSpeciesColor[j]);
              glColor3f(clr.r, clr.g, clr.b); 
              for( unsigned int k(0); k!=theMoleculeSize[i][j]; ++k )
                {
                  col = theCoords[i][j][k]/(theRowSize*theLayerSize)-theOriCol; 
                  layer =
                    (theCoords[i][j][k]%(theRowSize*theLayerSize))/theRowSize;
                  row =
                    (theCoords[i][j][k]%(theRowSize*theLayerSize))%theRowSize;
                  y = (col%2)*theHCPk + theHCPl*layer + theRadius;
                  z = row*2*theRadius + ((layer+col)%2)*theRadius + theRadius;
                  x = col*theHCPh + theRadius;
                  if(!( x <= theXUpBound[j] && x >= theXLowBound[j] &&
                      y <= theYUpBound[j] && y >= theYLowBound[j] &&
                      z <= theZUpBound[j] && z >= theZLowBound[j]))
                    {
                      glPushMatrix();
                      glTranslatef(x,y,z);
                      glCallList(SPHERE);
                      glPopMatrix();
                    }
                }
            }
        }
    }
}

void GLScene::plotMean3DCubicMolecules()
{
  unsigned int col, layer, row;
  double x,y,z;
  for(unsigned int i(0); i != theThreadSize; ++i)
    { 
      for(unsigned int k(0); k != theMeanCoordSize; ++k)
        {
          col = theMeanCoords[i][k]/(theRowSize*theLayerSize)-theOriCol; 
          layer = (theMeanCoords[i][k]%(theRowSize*theLayerSize))/theRowSize;
          row = (theMeanCoords[i][k]%(theRowSize*theLayerSize))%theRowSize;
          y = layer*2*theRadius + theRadius;
          z = row*2*theRadius + theRadius;
          x = col*2*theRadius + theRadius; 
          for(unsigned int j(0); j!=theSpeciesSize; ++j)
            {
              if(theSpeciesVisibility[j])
                {
                  Color clr(theSpeciesColor[j]);
                  double intensity((double)(theFrequency[i][j][k])/
                                   (double)(theMeanCount/4));
                  //glColor3f(clr.r*intensity, clr.g*intensity, clr.b*intensity); 
                  glColor4f(clr.r, clr.g, clr.b, intensity);
                  //glColor4f(clr.r*intensity, clr.g*intensity, clr.b*intensity,
                   //         0.5f); 
                  if(!( x <= theXUpBound[j] && x >= theXLowBound[j] &&
                      y <= theYUpBound[j] && y >= theYLowBound[j] &&
                      z <= theZUpBound[j] && z >= theZLowBound[j]))
                    {
                      glPushMatrix();
                      glTranslatef(x,y,z);
                      glCallList(SPHERE);
                      glPopMatrix();
                    }
                }
            }
        }
      for( unsigned int j(theSpeciesSize);
           j!=theTotalCoordSpeciesSize; ++j )
        {
          if(theSpeciesVisibility[j])
            {
              Color clr(theSpeciesColor[j]);
              glColor3f(clr.r, clr.g, clr.b); 
              for( unsigned int k(0); k!=theMoleculeSize[i][j]; ++k )
                {
                  col = theCoords[i][j][k]/(theRowSize*theLayerSize)-theOriCol; 
                  layer =
                    (theCoords[i][j][k]%(theRowSize*theLayerSize))/theRowSize;
                  row =
                    (theCoords[i][j][k]%(theRowSize*theLayerSize))%theRowSize;
                  y = layer*2*theRadius + theRadius;
                  z = row*2*theRadius + theRadius;
                  x = col*2*theRadius + theRadius; 
                  if(!( x <= theXUpBound[j] && x >= theXLowBound[j] &&
                      y <= theYUpBound[j] && y >= theYLowBound[j] &&
                      z <= theZUpBound[j] && z >= theZLowBound[j]))
                    {
                      glPushMatrix();
                      glTranslatef(x,y,z);
                      glCallList(SPHERE);
                      glPopMatrix();
                    }
                }
            }
        }
    }
}

void GLScene::plot3DHCPMolecules()
{
  unsigned int col, layer, row;
  double x,y,z;
  for( unsigned int i(0); i!=theThreadSize; ++i )
    {
      for( unsigned int j(0); j!=theTotalCoordSpeciesSize; ++j )
        {
          if( theSpeciesVisibility[j] )
            {
              Color clr(theSpeciesColor[j]);
              glColor3f(clr.r, clr.g, clr.b); 
              for( unsigned int k(0); k!=theMoleculeSize[i][j]; ++k )
                {
                  col = theCoords[i][j][k]/(theRowSize*theLayerSize)-theOriCol; 
                  layer =
                    (theCoords[i][j][k]%(theRowSize*theLayerSize))/theRowSize;
                  row =
                    (theCoords[i][j][k]%(theRowSize*theLayerSize))%theRowSize;
                  y = (col%2)*theHCPk + theHCPl*layer + theRadius;
                  z = row*2*theRadius + ((layer+col)%2)*theRadius + theRadius;
                  x = col*theHCPh + theRadius;
                  if(!( x <= theXUpBound[j] && x >= theXLowBound[j] &&
                      y <= theYUpBound[j] && y >= theYLowBound[j] &&
                      z <= theZUpBound[j] && z >= theZLowBound[j]))
                    {
                      glPushMatrix();
                      glTranslatef(x,y,z);
                      glCallList(SPHERE);
                      glPopMatrix();
                    }
                }
            }
        }
      for(int j(thePolymerSize-1); j!=-1; --j )
        {
          if( theSpeciesVisibility[j+theTotalCoordSpeciesSize] )
            {
              Color clr(theSpeciesColor[j+theTotalCoordSpeciesSize]);
              glColor3f(clr.r, clr.g, clr.b); 
              for( unsigned int k(0); k!=thePolymerMoleculeSize[i][j]; ++k )
                {
                  x = (thePoints[i][j][k].x/theResolution)*theRadius+theRadius;
                  y = (thePoints[i][j][k].y/theResolution)*theRadius+theRadius;
                  z = (thePoints[i][j][k].z/theResolution)*theRadius+theRadius;
                  if(!( x <= theXUpBound[j] && x >= theXLowBound[j] &&
                      y <= theYUpBound[j] && y >= theYLowBound[j] &&
                      z <= theZUpBound[j] && z >= theZLowBound[j]))
                    {
                      glPushMatrix();
                      glTranslatef(x,y,z);
                      glCallList(SPHERE);
                      glPopMatrix();
                    }
                }
            }
        }
    }
}

void GLScene::plot3DCubicMolecules()
{
  unsigned int col, layer, row;
  double x,y,z;
  for( unsigned int i(0); i!=theThreadSize; ++i )
    {
      for( unsigned int j(0); j!=theTotalCoordSpeciesSize; ++j )
        {
          if( theSpeciesVisibility[j] )
            {
              Color clr(theSpeciesColor[j]);
              glColor3f(clr.r, clr.g, clr.b); 
              for( unsigned int k(0); k!=theMoleculeSize[i][j]; ++k )
                {
                  col = theCoords[i][j][k]/(theRowSize*theLayerSize)-theOriCol; 
                  layer =
                    (theCoords[i][j][k]%(theRowSize*theLayerSize))/theRowSize;
                  row =
                    (theCoords[i][j][k]%(theRowSize*theLayerSize))%theRowSize;
                  y = layer*2*theRadius + theRadius;
                  z = row*2*theRadius + theRadius;
                  x = col*2*theRadius + theRadius; 
                  if(!( x <= theXUpBound[j] && x >= theXLowBound[j] &&
                      y <= theYUpBound[j] && y >= theYLowBound[j] &&
                      z <= theZUpBound[j] && z >= theZLowBound[j]))
                    {
                      glPushMatrix();
                      glTranslatef(x,y,z);
                      glCallList(SPHERE);
                      glPopMatrix();
                    }
                }
            }
        }
      for(int j(thePolymerSize-1); j!=-1; --j )
        {
          if( theSpeciesVisibility[j+theTotalCoordSpeciesSize] )
            {
              Color clr(theSpeciesColor[j+theTotalCoordSpeciesSize]);
              glColor3f(clr.r, clr.g, clr.b); 
              for( unsigned int k(0); k!=thePolymerMoleculeSize[i][j]; ++k )
                {
                  x = (thePoints[i][j][k].x/theResolution)*theRadius+theRadius;
                  y = (thePoints[i][j][k].y/theResolution)*theRadius+theRadius;
                  z = (thePoints[i][j][k].z/theResolution)*theRadius+theRadius;
                  if(!( x <= theXUpBound[j] && x >= theXLowBound[j] &&
                      y <= theYUpBound[j] && y >= theYLowBound[j] &&
                      z <= theZUpBound[j] && z >= theZLowBound[j]))
                    {
                      glPushMatrix();
                      glTranslatef(x,y,z);
                      glCallList(SPHERE);
                      glPopMatrix();
                    }
                }
            }
        }
    }
}

void GLScene::plotHCPPoints()
{
  glBegin(GL_POINTS);
  unsigned int col, layer, row;
  double x,y,z;
  for( unsigned int i(0); i!=theThreadSize; ++i )
    {
      for( unsigned int j(0); j!=theTotalCoordSpeciesSize; ++j )
        {
          if( theSpeciesVisibility[j] )
            {
              Color clr(theSpeciesColor[j]);
              glColor3f(clr.r, clr.g, clr.b); 
              for( unsigned int k(0); k!=theMoleculeSize[i][j]; ++k )
                {
                  col = theCoords[i][j][k]/(theRowSize*theLayerSize)-theOriCol; 
                  layer =
                    (theCoords[i][j][k]%(theRowSize*theLayerSize))/theRowSize;
                  row =
                    (theCoords[i][j][k]%(theRowSize*theLayerSize))%theRowSize;
                  y = (col%2)*theHCPk + theHCPl*layer + theRadius;
                  z = row*2*theRadius + ((layer+col)%2)*theRadius + theRadius;
                  x = col*theHCPh + theRadius;
                  if(!( x <= theXUpBound[j] && x >= theXLowBound[j] &&
                      y <= theYUpBound[j] && y >= theYLowBound[j] &&
                      z <= theZUpBound[j] && z >= theZLowBound[j]))
                    {
                      glVertex3f(x, y, z);
                    }
                }
            }
        }
      for(int j(thePolymerSize-1); j!=-1; --j )
        {
          if( theSpeciesVisibility[j+theTotalCoordSpeciesSize] )
            {
              Color clr(theSpeciesColor[j+theTotalCoordSpeciesSize]);
              glColor3f(clr.r, clr.g, clr.b); 
              for( unsigned int k(0); k!=thePolymerMoleculeSize[i][j]; ++k )
                {
                  x = (thePoints[i][j][k].x/theResolution)*theRadius+theRadius;
                  y = (thePoints[i][j][k].y/theResolution)*theRadius+theRadius;
                  z = (thePoints[i][j][k].z/theResolution)*theRadius+theRadius;
                  if(!( x <= theXUpBound[j] && x >= theXLowBound[j] &&
                      y <= theYUpBound[j] && y >= theYLowBound[j] &&
                      z <= theZUpBound[j] && z >= theZLowBound[j]))
                    {
                      glVertex3f(x, y, z);
                    }
                }
            }
        }
    }
  glEnd();
}

void GLScene::plotCubicPoints()
{
  glBegin(GL_POINTS);
  unsigned int col, layer, row;
  double x,y,z;
  for( unsigned int i(0); i!=theThreadSize; ++i )
    {
      for( unsigned int j(0); j!=theTotalCoordSpeciesSize; ++j )
        {
          if( theSpeciesVisibility[j] )
            {
              Color clr(theSpeciesColor[j]);
              glColor3f(clr.r, clr.g, clr.b); 
              for( unsigned int k(0); k!=theMoleculeSize[i][j]; ++k )
                {
                  col = theCoords[i][j][k]/(theRowSize*theLayerSize)-theOriCol; 
                  layer =
                    (theCoords[i][j][k]%(theRowSize*theLayerSize))/theRowSize;
                  row =
                    (theCoords[i][j][k]%(theRowSize*theLayerSize))%theRowSize;
                  y = layer*2*theRadius + theRadius;
                  z = row*2*theRadius + theRadius;
                  x = col*2*theRadius + theRadius; 
                  if(!( x <= theXUpBound[j] && x >= theXLowBound[j] &&
                      y <= theYUpBound[j] && y >= theYLowBound[j] &&
                      z <= theZUpBound[j] && z >= theZLowBound[j]))
                    {
                      glVertex3f(x, y, z);
                    }
                }
            }
        }
      for(int j(thePolymerSize-1); j!=-1; --j )
        {
          if( theSpeciesVisibility[j+theTotalCoordSpeciesSize] )
            {
              Color clr(theSpeciesColor[j+theTotalCoordSpeciesSize]);
              glColor3f(clr.r, clr.g, clr.b); 
              for( unsigned int k(0); k!=thePolymerMoleculeSize[i][j]; ++k )
                {
                  x = (thePoints[i][j][k].x/theResolution)*theRadius+theRadius;
                  y = (thePoints[i][j][k].y/theResolution)*theRadius+theRadius;
                  z = (thePoints[i][j][k].z/theResolution)*theRadius+theRadius;
                  if(!( x <= theXUpBound[j] && x >= theXLowBound[j] &&
                      y <= theYUpBound[j] && y >= theYLowBound[j] &&
                      z <= theZUpBound[j] && z >= theZLowBound[j]))
                    {
                      glVertex3f(x, y, z);
                    }
                }
            }
        }
    }
  glEnd();
}

void GLScene::setLayerColor( unsigned int aLayer )
{

  GLfloat a( theLayerSize/3.0 );
  GLfloat aRes( 1.0/a );
  GLfloat r( aLayer*aRes );
  GLfloat g( 0.0 );
  GLfloat b( 0.0 );
  if( r > 1 )
    {
      g = r - 1;
      r = 1.0;
      if( g > 1 )
        {
          b = g - 1;
          g = 1.0;
         }
    }
  glColor3f(r,g,b);
}

void GLScene::setTranslucentColor( unsigned int i, GLfloat j )
{  
  switch(i)
    {
    case 0:
      glColor4f(0.2,0.2,0.2,j);
      break;
    case 1:
      glColor4f(0.9,0.2,0.0,j);
      break;
    case 2:
      glColor4f(0,0.9,0.2,j);
      break;
    case 3:
      glColor4f(0.5,0.5,0.5,j);
      break;
    case 4:
      glColor4f(0.2,0,0.9,j);
      break;
    case 5:
      glColor4f(0.9,0,0.8,j);
      break;
    case 6:
      glColor4f(0,0.7,0.8,j);
      break;
    case 7:
      glColor4f(0.5,0,0,j);
      break;
    case 8:
      glColor4f(0,0.5,0,j);
      break;
    case 9:
      glColor4f(0,0,0.5,j);
      break;
    case 10:
      glColor4f(0.5,0.5,0,j);
      break;
    default :
      glColor4f((float)rand()/RAND_MAX,
                (float)rand()/RAND_MAX,
                (float)rand()/RAND_MAX,j);
    }
}

void GLScene::rotate(int aMult, int x, int y, int z)
{
  glMatrixMode(GL_MODELVIEW);
  glRotatef(theRotateAngle*aMult,x,y,z);
  invalidate();
}

void GLScene::translate(int x, int y, int z)
{
  glMatrixMode(GL_MODELVIEW);
  glTranslatef(x,y,z);
  invalidate();
}


void GLScene::zoomIn()
{ 
  FieldOfView/=1.05;
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(FieldOfView,Aspect,Near,ViewSize+Near);
  glMatrixMode(GL_MODELVIEW);
  invalidate();
}

void GLScene::zoomOut()
{
  FieldOfView*=1.05;
  if(FieldOfView>180)
    {
      FieldOfView=180;
    }
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(FieldOfView,Aspect,Near,ViewSize+Near);
  glMatrixMode(GL_MODELVIEW);
  invalidate();
}


bool GLScene::on_timeout()
{
  if( m_RunReverse )
    {
      for( unsigned int i(0); i!=theThreadSize; ++i )
        { 
          theFile[i]->seekg(-thePrevSize, std::ios::cur); 
          theFile[i]->read((char*) (&thePrevSize), sizeof(thePrevSize));
          theFile[i]->read((char*) (&theNextSize), sizeof(theNextSize));
          theFile[i]->seekg(-thePrevSize, std::ios::cur); 
          theFile[i]->read((char*) (&thePrevSize), sizeof(thePrevSize));
          theFile[i]->read((char*) (&theNextSize), sizeof(theNextSize));
        }
    }
  (this->*theLoadCoordsFunction)();
  if( theFile[0]->eof() != true )
    {
      char buffer[50];
      if( !m_RunReverse )
        {
          ++m_stepCnt;
        }
      else
        {
          if( m_stepCnt != 0 )
            {
              --m_stepCnt;
            }
        }
      sprintf(buffer, "%d", m_stepCnt);
      m_control->setStep(buffer);
      sprintf(buffer, "%f", theCurrentTime);
      m_control->setTime(buffer);
    }
  invalidate();
  if( startRecord )
    {
      timeout_remove();
      writePng();
      std::cout << "wrote png:" << thePngNumber << std::endl; 
      timeout_add();
    }
  return true;
}

void GLScene::step()
{
  if(m_Run)
    {
      m_Run = false;
      timeout_remove();
    }
  if( m_RunReverse )
    {
      for( unsigned int i(0); i!=theThreadSize; ++i )
        { 
          theFile[i]->seekg(-thePrevSize, std::ios::cur); 
          theFile[i]->read((char*) (&thePrevSize), sizeof(thePrevSize));
          theFile[i]->read((char*) (&theNextSize), sizeof(theNextSize));
          theFile[i]->seekg(-thePrevSize, std::ios::cur); 
          theFile[i]->read((char*) (&thePrevSize), sizeof(thePrevSize));
          theFile[i]->read((char*) (&theNextSize), sizeof(theNextSize));
        }
    }
  (this->*theLoadCoordsFunction)();
  if( theFile[0]->eof() != true )
    {
      char buffer[50];
      if( !m_RunReverse )
        {
          ++m_stepCnt;
        }
      else
        {
          if( m_stepCnt != 0 )
            {
              --m_stepCnt;
            }
        }
      sprintf(buffer, "%d", m_stepCnt);
      m_control->setStep(buffer);
      sprintf(buffer, "%f", theCurrentTime);
      m_control->setTime(buffer);
    }
  isChanged = true;
  invalidate();
  if( startRecord )
    {
      timeout_remove();
      writePng();
      std::cout << "wrote png:" << thePngNumber << std::endl; 
      timeout_add();
    }
}

/*
  Gtk::HBox aStepBox;
  m_table.attach(aStepBox, 0, 1, 0, 1, Gtk::FILL,
                 Gtk::SHRINK | Gtk::FILL, 0, 0 );
  Gtk::Label aStepLabel("Step:");
  aStepBox.pack_start(aStepLabel);
  aStepBox.pack_start(m_steps);
  Gtk::HBox aTimeBox;
  m_table.attach(aTimeBox, 0, 1, 1, 2, Gtk::FILL,
                 Gtk::SHRINK | Gtk::FILL, 0, 0 );
  Gtk::Label aTimeLabel("Time:");
  aTimeBox.pack_start(aTimeLabel);
  aTimeBox.pack_start(m_time);
  */

void GLScene::timeout_add()
{
  if (!m_ConnectionTimeout.connected())
    m_ConnectionTimeout = Glib::signal_timeout().connect(
      sigc::mem_fun(*this, &GLScene::on_timeout), TIMEOUT_INTERVAL);
}

void GLScene::timeout_remove()
{
  if (m_ConnectionTimeout.connected())
    m_ConnectionTimeout.disconnect();
}

bool GLScene::on_map_event(GdkEventAny* event)
{
  if (m_Run)
    timeout_add();

  return true;
}

bool GLScene::on_unmap_event(GdkEventAny* event)
{
  timeout_remove();

  return true;
}

bool GLScene::on_visibility_notify_event(GdkEventVisibility* event)
{
  if (m_Run)
    {
      if (event->state == GDK_VISIBILITY_FULLY_OBSCURED)
        timeout_remove();
      else
        timeout_add();
    }

  return true;
}

void GLScene::resetView()
{
  GLfloat w = get_width();
  GLfloat h = get_height();
  Aspect = w/h;
  glViewport(0, 0, static_cast<GLsizei>(w), static_cast<GLsizei>(h));
  FieldOfView=45;
  Xtrans=Ytrans=0;
  if(w>=h) Near=ViewSize/2.0/tan(FieldOfView*PI/180.0/2.0);
  else Near=ViewSize/2.0/tan(FieldOfView*Aspect*PI/180.0/2.0);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(FieldOfView,Aspect,Near,ViewSize+Near);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(-ViewMidx,-ViewMidy,-ViewMidz);
  glTranslatef(0,0,-ViewSize/2.0-Near);
  invalidate();
  xAngle = 0;
  yAngle = 0;
  zAngle = 0;
  m_control->setXangle(xAngle);
  m_control->setYangle(yAngle);
  m_control->setZangle(zAngle);
}

void GLScene::resetBound()
{

}

void GLScene::rotateMidAxis(int aMult, int x, int y, int z)
{
  GLfloat m[16];
  glMatrixMode(GL_MODELVIEW);
  glGetFloatv(GL_MODELVIEW_MATRIX,m);
  glLoadIdentity();
  glTranslatef(Xtrans,Ytrans,-(Near+ViewSize/2.0));
  glRotatef(theRotateAngle*aMult,x,y,z);
  glTranslatef(-Xtrans,-Ytrans,+(Near+ViewSize/2.0));
  glMultMatrixf(m);
  invalidate();
  if(x)
    {
      xAngle += aMult*theRotateAngle;
      normalizeAngle(xAngle);
      m_control->setXangle(xAngle);
    }
  if(y)
    {
      yAngle += aMult*theRotateAngle;
      normalizeAngle(yAngle);
      m_control->setYangle(yAngle);
    }
  if(z)
    {
      zAngle += aMult*theRotateAngle;
      normalizeAngle(zAngle);
      m_control->setZangle(zAngle);
    }
}

void GLScene::rotateMidAxisAbs(double angle, int x, int y, int z)
{
  GLfloat m[16];
  glMatrixMode(GL_MODELVIEW);
  glGetFloatv(GL_MODELVIEW_MATRIX,m);
  glLoadIdentity();
  glTranslatef(Xtrans,Ytrans,-(Near+ViewSize/2.0));
  if(x)
    {
      glRotatef(angle-xAngle,x,y,z);
      xAngle = angle;
      m_control->setXangle(xAngle);
    }
  if(y)
    {
      glRotatef(angle-yAngle,x,y,z);
      yAngle = angle;
      m_control->setYangle(yAngle);
    }
  if(z)
    {
      glRotatef(angle-zAngle,x,y,z);
      zAngle = angle;
      m_control->setZangle(zAngle);
    }
  glTranslatef(-Xtrans,-Ytrans,+(Near+ViewSize/2.0));
  glMultMatrixf(m);
  invalidate();
}

void GLScene::normalizeAngle(double &angle)
{
  while(angle > 180)
    {
      angle -= 360;
    }
  while(angle < -180)
    {
      angle += 360;
    }
}

void GLScene::pause()
{
  m_Run = !m_Run;

  if (m_Run)
    {
      timeout_add();
    }
  else
    {
      timeout_remove();
      invalidate();
    }
}

void GLScene::play()
{
  m_Run = true;
  if (m_Run)
    {
      timeout_add();
    }
}

ControlBox::ControlBox(GLScene *anArea) :
  m_table(10, 10),
  theFrameRotAdj( "Rotation" ),
  theResetRotButton( "Reset" ),
  theCheckFix( "Fix rotation" ),
  theXLabel( "x" ),
  theXAdj( 0, -180, 180, 5, 20, 0 ),
  theXScale( theXAdj ),
  theXSpin( theXAdj, 0, 0  ),
  theYLabel( "y" ),
  theYAdj( 0, -180, 180, 5, 20, 0 ),
  theYScale( theYAdj ),
  theYSpin( theYAdj, 0, 0  ),
  theZLabel( "z" ),
  theZAdj( 0, -180, 180, 5, 20, 0 ),
  theZScale( theZAdj ),
  theZSpin( theZAdj, 0, 0  ),
  theFrameBoundAdj("Bounding"),
  theCheckFixBound( "Fix bounding" ),
  theResetBoundButton( "Reset" ),
  theXUpBoundLabel( "+x" ),
  theXUpBoundAdj( 100, 0, 0, 1, 0, 0 ),
  theXUpBoundScale( theXUpBoundAdj ),
  theXUpBoundSpin( theXUpBoundAdj, 0, 0  ),
  theXLowBoundLabel( "-x" ),
  theXLowBoundAdj( 0, 0, 100, 1, 0, 0 ),
  theXLowBoundScale( theXLowBoundAdj ),
  theXLowBoundSpin( theXLowBoundAdj, 0, 0  ),
  theYUpBoundLabel( "+y" ),
  theYUpBoundAdj( 100, 0, 100, 1, 0, 0 ),
  theYUpBoundScale( theYUpBoundAdj ),
  theYUpBoundSpin( theYUpBoundAdj, 0, 0  ),
  theYLowBoundLabel( "-y" ),
  theYLowBoundAdj( 0, 0, 100, 1, 0, 0 ),
  theYLowBoundScale( theYLowBoundAdj ),
  theYLowBoundSpin( theYLowBoundAdj, 0, 0  ),
  theZUpBoundLabel( "+z" ),
  theZUpBoundAdj( 100, 0, 100, 1, 0, 0 ),
  theZUpBoundScale( theZUpBoundAdj ),
  theZUpBoundSpin( theZUpBoundAdj, 0, 0  ),
  theZLowBoundLabel( "-z" ),
  theZLowBoundAdj( 0, 0, 100, 1, 0, 0 ),
  theZLowBoundScale( theZLowBoundAdj ),
  theZLowBoundSpin( theZLowBoundAdj, 0, 0  ),
  theFrameLatticeAdj("Zoom"),
  theResetDepthButton( "Reset" ),
  theCheck3DMolecule( "Show 3D Molecules" ),
  theCheckShowTime( "Show Time" ),
  theButtonResetTime( "Reset Time" ),
  theDepthLabel( "Depth" ),
  theDepthAdj( 0, -200, 130, 5, 0, 0 ),
  theDepthScale( theDepthAdj ),
  theDepthSpin( theDepthAdj, 0, 0  ),
  theButtonRecord( "Record Frames" ),
  m_area(anArea)
{
  set_border_width(2);
  set_size_request(500, 300);
  set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  add(m_rightBox);
  m_table.set_row_spacings(1);
  m_table.set_col_spacings(2);
  m_table.attach(m_stepBox, 0, 1, 0, 1, Gtk::FILL,
                 Gtk::SHRINK | Gtk::FILL, 0, 0 );
  m_rightBox.pack_start(m_table, Gtk::PACK_SHRINK);
  //Create a table on the right that holds the control.
  m_rightBox.pack_start(theBoxCtrl, Gtk::PACK_SHRINK); 
  // Create a frame the will have the rotation adjusters
  // and the Fix and Reset buttons.
  theBoxCtrl.pack_start( theFrameRotAdj, false, false, 1 );
  theBoxInFrame.set_border_width( 3 );
  theFrameRotAdj.add( theBoxInFrame );
  theXBox.set_homogeneous( false );
  theBoxInFrame.pack_start( theXBox, false, false, 1 );
  theBoxInFrame.pack_start( theYBox, false, false, 1 );
  theBoxInFrame.pack_start( theZBox, false, false, 1 );
  theBoxInFrame.pack_start( theBoxRotFixReset, false, false, 1 );
  //theCheckFix.connect( 'toggled', fixRotToggled );
  theBoxRotFixReset.pack_start( theCheckFix );
  theResetRotButton.signal_clicked().connect( sigc::mem_fun(*this,
                            &ControlBox::onResetRotation) );
  theBoxRotFixReset.pack_start( theResetRotButton );

  // X
  theXLabel.set_width_chars( 1 );
  theXBox.pack_start( theXLabel, false, false, 2 ); 
  theXScale.set_draw_value( false );
  theXBox.pack_start( theXScale );
  theXSpin.set_width_chars( 3 );
  theXSpin.set_wrap( true );
  theXSpin.set_has_frame( false );
  theXBox.pack_start( theXSpin, false, false, 2 );
  theXAdj.signal_value_changed().connect( sigc::mem_fun(*this, 
                           &ControlBox::xRotateChanged ) );

  // Y
  theYLabel.set_width_chars( 1 );
  theYBox.pack_start( theYLabel, false, false, 2 ); 
  theYScale.set_draw_value( false );
  theYBox.pack_start( theYScale );
  theYSpin.set_width_chars( 3 );
  theYSpin.set_wrap( true );
  theYSpin.set_has_frame( false );
  theYBox.pack_start( theYSpin, false, false, 2 );
  theYAdj.signal_value_changed().connect( sigc::mem_fun(*this, 
                           &ControlBox::yRotateChanged ) );

  // Z
  theZLabel.set_width_chars( 1 );
  theZBox.pack_start( theZLabel, false, false, 2 ); 
  theZScale.set_draw_value( false );
  theZBox.pack_start( theZScale );
  theZSpin.set_width_chars( 3 );
  theZSpin.set_wrap( true );
  theZSpin.set_has_frame( false );
  theZBox.pack_start( theZSpin, false, false, 2 );
  theZAdj.signal_value_changed().connect( sigc::mem_fun(*this, 
                           &ControlBox::zRotateChanged ) );

  // Create a frame the will have the lattice boundary adjusters
  // and the Fix and Reset buttons.
  theBoxCtrl.pack_start( theFrameBoundAdj, false, false, 1 );
  theBoxInBound.set_border_width( 3 );
  theFrameBoundAdj.add( theBoxInBound );
  theBoxInBound.pack_start( theXUpBoundBox, false, false, 1 ); 
  theBoxInBound.pack_start( theXLowBoundBox, false, false, 1 ); 
  theBoxInBound.pack_start( theYUpBoundBox, false, false, 1 ); 
  theBoxInBound.pack_start( theYLowBoundBox, false, false, 1 ); 
  theBoxInBound.pack_start( theZUpBoundBox, false, false, 1 ); 
  theBoxInBound.pack_start( theZLowBoundBox, false, false, 1 ); 
  theBoxInBound.pack_start( theBoxBoundFixReset, false, false, 1 ); 
  //theCheckFixBound.connect( 'toggled', fixBoundToggled );
  theBoxBoundFixReset.pack_start( theCheckFixBound );
  theResetBoundButton.signal_clicked().connect( sigc::mem_fun(*this,
                            &ControlBox::onResetBound) );
  theBoxBoundFixReset.pack_start( theResetBoundButton );

  unsigned int aLayerSize( m_area->getLayerSize() );
  unsigned int aColSize( m_area->getColSize() );
  unsigned int aRowSize( m_area->getRowSize() );

  // x up bound
  theXUpBoundLabel.set_width_chars( 2 );
  theXUpBoundBox.pack_start( theXUpBoundLabel, false, false, 2 );
  theXUpBoundAdj.set_value( aColSize );
  theXUpBoundAdj.set_upper( aColSize );
  theXUpBoundAdj.signal_value_changed().connect( sigc::mem_fun(*this, 
                           &ControlBox::xUpBoundChanged ) );
  theXUpBoundScale.set_draw_value( false );
  theXUpBoundBox.pack_start( theXUpBoundScale );
  theXUpBoundSpin.set_width_chars( 3 );
  theXUpBoundSpin.set_has_frame( false );
  theXUpBoundBox.pack_start( theXUpBoundSpin, false, false, 2 );

  // x low bound
  theXLowBoundLabel.set_width_chars( 2 );
  theXLowBoundBox.pack_start( theXLowBoundLabel, false, false, 2 );
  theXLowBoundAdj.set_upper( aColSize );
  theXLowBoundAdj.signal_value_changed().connect( sigc::mem_fun(*this, 
                           &ControlBox::xLowBoundChanged ) );
  theXLowBoundScale.set_draw_value( false );
  theXLowBoundBox.pack_start( theXLowBoundScale );
  theXLowBoundSpin.set_width_chars( 3 );
  theXLowBoundSpin.set_has_frame( false );
  theXLowBoundBox.pack_start( theXLowBoundSpin, false, false, 2 );

  // y up bound
  theYUpBoundLabel.set_width_chars( 2 );
  theYUpBoundBox.pack_start( theYUpBoundLabel, false, false, 2 );
  theYUpBoundAdj.set_value( aLayerSize );
  theYUpBoundAdj.set_upper( aLayerSize );
  theYUpBoundAdj.signal_value_changed().connect( sigc::mem_fun(*this, 
                           &ControlBox::yUpBoundChanged ) );
  theYUpBoundScale.set_draw_value( false );
  theYUpBoundBox.pack_start( theYUpBoundScale );
  theYUpBoundSpin.set_width_chars( 3 );
  theYUpBoundSpin.set_has_frame( false );
  theYUpBoundBox.pack_start( theYUpBoundSpin, false, false, 2 );

  // y low bound
  theYLowBoundLabel.set_width_chars( 2 );
  theYLowBoundBox.pack_start( theYLowBoundLabel, false, false, 2 );
  theYLowBoundAdj.set_upper( aLayerSize );
  theYLowBoundAdj.signal_value_changed().connect( sigc::mem_fun(*this, 
                           &ControlBox::yLowBoundChanged ) );
  theYLowBoundScale.set_draw_value( false );
  theYLowBoundBox.pack_start( theYLowBoundScale );
  theYLowBoundSpin.set_width_chars( 3 );
  theYLowBoundSpin.set_has_frame( false );
  theYLowBoundBox.pack_start( theYLowBoundSpin, false, false, 2 );

  // z up bound
  theZUpBoundLabel.set_width_chars( 2 );
  theZUpBoundBox.pack_start( theZUpBoundLabel, false, false, 2 );
  theZUpBoundAdj.set_value( aRowSize );
  theZUpBoundAdj.set_upper( aRowSize );
  theZUpBoundAdj.signal_value_changed().connect( sigc::mem_fun(*this, 
                           &ControlBox::zUpBoundChanged ) );
  theZUpBoundScale.set_draw_value( false );
  theZUpBoundBox.pack_start( theZUpBoundScale );
  theZUpBoundSpin.set_width_chars( 3 );
  theZUpBoundSpin.set_has_frame( false );
  theZUpBoundBox.pack_start( theZUpBoundSpin, false, false, 2 );

  // z low bound
  theZLowBoundLabel.set_width_chars( 2 );
  theZLowBoundBox.pack_start( theZLowBoundLabel, false, false, 2 );
  theZLowBoundAdj.set_upper( aRowSize );
  theZLowBoundAdj.signal_value_changed().connect( sigc::mem_fun(*this, 
                           &ControlBox::zLowBoundChanged ) );
  theZLowBoundScale.set_draw_value( false );
  theZLowBoundBox.pack_start( theZLowBoundScale );
  theZLowBoundSpin.set_width_chars( 3 );
  theZLowBoundSpin.set_has_frame( false );
  theZLowBoundBox.pack_start( theZLowBoundSpin, false, false, 2 );

  // Create a frame the will have the lattice depth adjuster
  // and background color selector
  theBoxCtrl.pack_start( theFrameLatticeAdj, false, false, 1 );
  theBoxInLattice.set_border_width( 3 );
  theFrameLatticeAdj.add( theBoxInLattice );
  theDepthBox.set_homogeneous( false );
  theBoxInLattice.pack_start( theDepthBox, false, false, 1 );
  theBoxInLattice.pack_start( the3DMoleculeBox, false, false, 1 );
  //theResetDepthButton.connect( 'clicked', resetDepth );
  the3DMoleculeBox.pack_start( theResetDepthButton ); 
  theCheckShowTime.signal_toggled().connect( sigc::mem_fun(*this,
                            &ControlBox::on_showTime_toggled) );
  theCheckShowTime.set_active();
  theBoxCtrl.pack_start( theCheckShowTime, false, false, 2 );
  theCheck3DMolecule.signal_toggled().connect( sigc::mem_fun(*this,
                            &ControlBox::on_3DMolecule_toggled) );
  //theCheck3DMolecule.set_active();
  theCheck3DMolecule.set_active(false);
  theBoxCtrl.pack_start( theCheck3DMolecule, false, false, 2 );
  theButtonResetTime.signal_clicked().connect( sigc::mem_fun(*this,
                            &ControlBox::on_resetTime_clicked) );
  theBoxCtrl.pack_start( theButtonResetTime, false, false, 2 );
  theButtonRecord.signal_toggled().connect( sigc::mem_fun(*this,
                            &ControlBox::on_record_toggled) );
  theBoxCtrl.pack_start( theButtonRecord, false, false, 2 );
  theDepthLabel.set_width_chars( 1 );
  theDepthBox.pack_start( theDepthLabel, false, false, 2 );
  //theDepthAdj.connect( 'value_changed', depthChanged );
  theDepthScale.set_draw_value( false );
  theDepthBox.pack_start( theDepthScale );
  theDepthSpin.set_width_chars( 3 );
  theDepthSpin.set_has_frame( false );
  theDepthBox.pack_start( theDepthSpin, false, false, 2 );

  m_stepLabel.set_text("Step:");
  m_sizeGroup = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
  m_sizeGroup->add_widget(m_stepLabel);
  m_stepBox.pack_start(m_stepLabel, Gtk::PACK_SHRINK);
  m_stepBox.pack_start(m_steps, Gtk::PACK_SHRINK);
  m_table.attach(m_timeBox, 0, 1, 1, 2, Gtk::FILL,
                 Gtk::SHRINK | Gtk::FILL, 0, 0 );
  m_timeLabel.set_text("Time:");
  m_sizeGroup->add_widget(m_timeLabel);
  m_timeBox.pack_start(m_timeLabel, Gtk::PACK_SHRINK);
  m_timeBox.pack_start(m_time, Gtk::PACK_SHRINK);
  theSpeciesSize = m_area->getSpeciesSize();
  theButtonList = new Gtk::CheckButton*[theSpeciesSize]; 
  theLabelList = new Gtk::Label*[theSpeciesSize]; 
  for(unsigned int i(0); i!=theSpeciesSize; ++i)
    {
      theButtonList[i] = Gtk::manage(new Gtk::CheckButton());
      theLabelList[i] = Gtk::manage(new Gtk::Label);
      Gtk::HBox* aHBox = Gtk::manage(new Gtk::HBox);
      Gtk::EventBox* m_EventBox=Gtk::manage(new Gtk::EventBox);
      m_EventBox->set_events(Gdk::BUTTON_RELEASE_MASK);
      theLabelList[i]->set_text(m_area->getSpeciesName(i));
      Color clr(m_area->getSpeciesColor(i));
      Gdk::Color aColor;
      aColor.set_rgb(int(clr.r*65535), int(clr.g*65535), int(clr.b*65535));
      theLabelList[i]->modify_fg(Gtk::STATE_NORMAL, aColor);
      theLabelList[i]->modify_fg(Gtk::STATE_ACTIVE, aColor);
      theLabelList[i]->modify_fg(Gtk::STATE_PRELIGHT, aColor);
      theLabelList[i]->modify_fg(Gtk::STATE_SELECTED, aColor);
      theButtonList[i]->signal_toggled().connect(sigc::bind( 
              sigc::mem_fun(*this, &ControlBox::on_checkbutton_toggled), i ) );
      m_EventBox->signal_button_release_event().connect(sigc::bind( 
              sigc::mem_fun(*this, &ControlBox::on_checkbutton_clicked), i ) );
      m_EventBox->add(*theLabelList[i]);
      theButtonList[i]->set_active(m_area->getSpeciesVisibility(i));
      aHBox->pack_start(*theButtonList[i], false, false, 2);
      aHBox->pack_start(*m_EventBox, false, false, 2);
      m_table.attach(*aHBox, 0, 1, i+2, i + 3, Gtk::FILL,
                     Gtk::SHRINK | Gtk::FILL, 0, 0 );
    }
  std::cout << "theSpeciesSize:" << theSpeciesSize << std::endl;
}

void
ControlBox::on_checkbutton_toggled(unsigned int id)
{
  m_area->setSpeciesVisibility(id, theButtonList[id]->get_active());
}

bool ControlBox::on_checkbutton_clicked(GdkEventButton* event, unsigned int id)
{
  if(event->type == GDK_BUTTON_RELEASE && event->button == 3)
    {
      Color clr(m_area->getSpeciesColor(id));
      Gdk::Color aColor;
      aColor.set_rgb(int(clr.r*65535), int(clr.g*65535), int(clr.b*65535));
      Gtk::ColorSelectionDialog dlg("Select a color"); 
      Gtk::ColorSelection* colorSel(dlg.get_colorsel());
      colorSel->set_current_color(aColor);
      colorSel->signal_color_changed().connect(sigc::bind( 
        sigc::mem_fun(*this, &ControlBox::update_species_color), id, colorSel));
      if(dlg.run() == Gtk::RESPONSE_CANCEL)
        {
          m_area->setSpeciesColor(id, clr);
          theLabelList[id]->modify_fg(Gtk::STATE_NORMAL, aColor);
          theLabelList[id]->modify_fg(Gtk::STATE_ACTIVE, aColor);
          theLabelList[id]->modify_fg(Gtk::STATE_PRELIGHT, aColor);
          theLabelList[id]->modify_fg(Gtk::STATE_SELECTED, aColor);
        }
      else
        {
          update_species_color(id, colorSel);
        }
      return true;
    }
  else
    {
      if(theButtonList[id]->get_active())
        {
          theButtonList[id]->set_active(false);
        }
      else
        {
          theButtonList[id]->set_active(true);
        }
    }
  return false;
}

void 
ControlBox::update_species_color(unsigned int id, Gtk::ColorSelection* colorSel)
{
  Gdk::Color aColor(colorSel->get_current_color());
  Color clr;
  clr.r = aColor.get_red()/65535.0;
  clr.g = aColor.get_green()/65535.0;
  clr.b = aColor.get_blue()/65535.0;
  m_area->setSpeciesColor(id, clr);
  theLabelList[id]->modify_fg(Gtk::STATE_NORMAL, aColor);
  theLabelList[id]->modify_fg(Gtk::STATE_ACTIVE, aColor);
  theLabelList[id]->modify_fg(Gtk::STATE_PRELIGHT, aColor);
  theLabelList[id]->modify_fg(Gtk::STATE_SELECTED, aColor);
}

void
ControlBox::on_3DMolecule_toggled()
{
  m_area->set3DMolecule(theCheck3DMolecule.get_active());
}

void
ControlBox::on_showTime_toggled()
{
  m_area->setShowTime(theCheckShowTime.get_active());
}

void
ControlBox::on_resetTime_clicked()
{
  m_area->resetTime();
}

void
ControlBox::onResetRotation()
{
  m_area->resetView();
}

void
ControlBox::onResetBound()
{
  m_area->resetBound();
}

void
ControlBox::on_record_toggled()
{
  m_area->setRecord(theButtonRecord.get_active());
}

void
ControlBox::setXangle(double angle)
{
  theXAdj.set_value(angle);
}

void
ControlBox::setYangle(double angle)
{
  theYAdj.set_value(angle);
}

void
ControlBox::setZangle(double angle)
{
  theZAdj.set_value(angle);
}

void
ControlBox::xRotateChanged()
{
  m_area->rotateMidAxisAbs(theXAdj.get_value(), 1, 0, 0);
}

void
ControlBox::yRotateChanged()
{
  m_area->rotateMidAxisAbs(theYAdj.get_value(), 0, 1, 0);
}

void
ControlBox::zRotateChanged()
{
  m_area->rotateMidAxisAbs(theZAdj.get_value(), 0, 0, 1);
}

void
ControlBox::xUpBoundChanged()
{
  m_area->setXUpBound((unsigned int)theXUpBoundAdj.get_value());
}

void
ControlBox::xLowBoundChanged()
{
  m_area->setXLowBound((unsigned int)theXLowBoundAdj.get_value());
}

void
ControlBox::yUpBoundChanged()
{
  m_area->setYUpBound((unsigned int)theYUpBoundAdj.get_value());
}

void
ControlBox::yLowBoundChanged()
{
  m_area->setYLowBound((unsigned int)theYLowBoundAdj.get_value());
}

void
ControlBox::zUpBoundChanged()
{
  m_area->setZUpBound((unsigned int)theZUpBoundAdj.get_value());
}

void
ControlBox::zLowBoundChanged()
{
  m_area->setZLowBound((unsigned int)theZLowBoundAdj.get_value());
}

void
ControlBox::setStep(char* buffer)
{
  m_steps.set_text(buffer);
}

void
ControlBox::setTime(char* buffer)
{
  m_time.set_text(buffer);
}

ControlBox::~ControlBox()
{
}


Rulers::Rulers(const Glib::RefPtr<const Gdk::GL::Config>& config,
               const char* aFileName) :
  m_area(config, aFileName),
  m_table(3, 2, false),
  m_hbox(),
  m_control(&m_area),
  isRecord(false)
{
  m_area.setControlBox(&m_control);
  set_title("Spatiocyte Visualizer");
  set_reallocate_redraws(true);
  set_border_width(10);

  add(m_hbox);
  m_hbox.pack1(m_table, Gtk::PACK_EXPAND_WIDGET, 5);
  m_hbox.pack2(m_control, Gtk::PACK_SHRINK, 5);
  //m_area.set_size_request(XSIZE, YSIZE); 
  m_table.attach(m_area, 1,2,1,2,
		 Gtk::EXPAND | Gtk::FILL , Gtk::FILL, 0, 0);
  
  m_area.set_events(Gdk::POINTER_MOTION_MASK | Gdk::BUTTON_PRESS_MASK );

  //Connect a signal handler for the DrawingArea's
  //"motion_notify_event" signal, to detect cursor movement:
  m_area.signal_motion_notify_event().connect( 
         sigc::mem_fun(*this, &Rulers::on_area_motion_notify_event) );

  // The horizontal ruler goes on top:
  m_hrule.set_metric(Gtk::PIXELS);
  m_hrule.set_range(0, XSIZE, 10, XSIZE );
  //C example uses 7, 13, 0, 20 - don't know why.

  m_table.attach(m_hrule, 1,2,0,1,
		 Gtk::EXPAND | Gtk::SHRINK | Gtk::FILL, Gtk::FILL,
		 0, 0);

  // Vertical ruler:
  m_vrule.set_metric(Gtk::PIXELS);
  m_vrule.set_range(0, YSIZE, 10, YSIZE );

  m_table.attach(m_vrule, 0, 1, 1, 2,
		 Gtk::FILL, Gtk::EXPAND | Gtk::SHRINK | Gtk::FILL, 0, 0 );

  show_all_children();
}

bool Rulers::on_area_motion_notify_event(GdkEventMotion* event)
{
  //The cursor was moved in the m_area widget.
  //Show the position in the rulers:

  if(event)
  {
    m_hrule.property_position().set_value(event->x);
    m_vrule.property_position().set_value(event->y);
  }

  return false;  //false = signal not fully handled, pass it on..
}

bool Rulers::on_key_press_event(GdkEventKey* event)
{
  switch (event->keyval)
    {
    case GDK_x:
      m_area.rotate(1,1,0,0);
      break;
    case GDK_X:
      m_area.rotate(-1,1,0,0);
      break;
    case GDK_y:
      m_area.rotate(1,0,1,0);
      break;
    case GDK_Y:
      m_area.rotate(-1,0,1,0);
      break;
    case GDK_Home:
      m_area.resetView();
      break;
    case GDK_Pause:
      m_area.pause();
      break;
    case GDK_p:
      m_area.pause();
      break;
    case GDK_P:
      m_area.pause();
      break;
    case GDK_Return:
      if(event->state&Gdk::SHIFT_MASK)
        {
          m_area.setReverse(true);
          m_area.step();
        }
      else
        {
          m_area.setReverse(false);
          m_area.step();
        }
      break;
    case GDK_space:
      m_area.pause();
      break;
    case GDK_Page_Up:
      m_area.zoomIn();
      break;
    case GDK_Page_Down:
      m_area.zoomOut();
      break;
    case GDK_0:
      if(event->state&Gdk::CONTROL_MASK)
        {
          m_area.resetView();
        }
      break;
    case GDK_equal:
      if(event->state&Gdk::CONTROL_MASK)
        {
          m_area.zoomIn();
        }
      break;
    case GDK_plus:
      if(event->state&Gdk::CONTROL_MASK)
        {
          m_area.zoomIn();
        }
      break;
    case GDK_minus:
      if(event->state&Gdk::CONTROL_MASK)
        {
          m_area.zoomOut();
        }
      break;
    case GDK_Down:
      if(event->state&Gdk::SHIFT_MASK)
        {
          m_area.translate(0,-1,0);
        }
      else if (event->state&Gdk::CONTROL_MASK)
        {
          m_area.rotateMidAxis(1,1,0,0);
        }
      else
        {
          m_area.setReverse(true);
          m_area.step();
        }
      break;
    case GDK_Up:
      if(event->state&Gdk::SHIFT_MASK)
        {
          m_area.translate(0,1,0);
        }
      else if (event->state&Gdk::CONTROL_MASK)
        {
          m_area.rotateMidAxis(-1,1,0,0);
        }
      else
        {
          m_area.setReverse(false);
          m_area.step();
        }
      break;
    case GDK_Right:
      if(event->state&Gdk::SHIFT_MASK)
        {
          m_area.translate(1,0,0);
        }
      else if (event->state&Gdk::CONTROL_MASK)
        {
          m_area.rotateMidAxis(1,0,1,0);
        }
      else
        {
          m_area.setReverse(false);
          m_area.play();
        }
      break;
    case GDK_Left:
      if(event->state&Gdk::SHIFT_MASK)
        {
          m_area.translate(-1,0,0);
        }
      else if (event->state&Gdk::CONTROL_MASK)
        {
          m_area.rotateMidAxis(-1,0,1,0);
        }
      else
        {
          m_area.setReverse(true);
          m_area.play();
        }
      break;
    case GDK_z:
      m_area.rotateMidAxis(-1,0,0,1);
      break;
    case GDK_Z:
      m_area.rotateMidAxis(1,0,0,1);
      break;
    case GDK_l:
      m_area.rotate(1,0,0,1);
      break;
    case GDK_r:
      m_area.rotate(-1,0,0,1);
      break;
    case GDK_s:
      std::cout << "saving frame" << std::endl;
      m_area.writePng();
      break;
    case GDK_S:
      if(!isRecord)
        {
          isRecord = true;
          std::cout << "Started saving frames" << std::endl; 
        }
      else
        {
          isRecord = false;
          std::cout << "Stopped saving frames" << std::endl; 
        }
      m_area.setRecord(isRecord);
      break;
    default:
      return true;
    }
  return true;
}
void printUsage( const char* aProgramName )
{
  std::cerr << "usage:" << std::endl;
  std::cerr <<  aProgramName <<
    " <fileBaseName> (for Little Endian binary files)" << std::endl;
  std::cerr <<  aProgramName <<
    " -b <fileBaseName> (for Big Endian binary files)"
    << std::endl << std::flush;
}

void printNotEndian( const char* anEndian, const char* aBaseName, 
                     const unsigned int count, const unsigned int aThreadSize )
{
  std::cerr << "theThreadSize from file " << aBaseName << " is:" <<
    aThreadSize << "\nbut I can't open " << aBaseName << " file.\n";
  std::cerr << "Could it be that " << aBaseName << " is not";
  std::cerr << " a " << anEndian << " Endian binary file?\n";
}

unsigned int convertFiles2LittleEndian( const char* aBaseName )
                          
{
  unsigned int aThreadSize;
  std::ostringstream aFileName;
  aFileName << aBaseName << std::ends;
  std::ifstream aParentFile( aFileName.str().c_str(), std::ios::binary );
  if ( !aParentFile.is_open() )
    {
      std::cerr << "Could not open file: " << aBaseName << std::endl;
      return 1;
    }
  aParentFile.read((char *)(&aThreadSize), sizeof(unsigned int)); 
  aThreadSize = ntohl( aThreadSize );
  for( unsigned int i(1); i!=aThreadSize; ++i )
    {
      std::ostringstream aFileName;
      aFileName << aBaseName << std::ends;
      std::ifstream aFile( aFileName.str().c_str(), std::ios::binary );
      if( !aFile.is_open() )
        {
          printNotEndian( "Big", aBaseName, i, aThreadSize );
          return 1;
        }
    }

  // do the conversion
  std::cout << "Starting to convert data files to Little Endian format...\n";
  for( unsigned int i(0); i!=aThreadSize; ++i )
    {
      std::ostringstream aFileName;
      aFileName << aBaseName << std::ends;
      std::ifstream anInput( aFileName.str().c_str(), std::ios::binary );

      std::ostringstream anOutFileName;
      anOutFileName << "lcoord" << i << ".dat" << std::ends;
      std::ofstream anOutput( anOutFileName.str().c_str(), std::ios::binary | 
                              std::ios::trunc );
      while( anInput.eof() != true )
        {
          unsigned int aValue;
          anInput.read((char *)(&aValue), sizeof(unsigned int)); 
          aValue = ntohl( aValue );
          anOutput.write((char*) (&aValue), sizeof(unsigned int));
        }
      anInput.close();
      anOutput.close();
    }
  std::cout << "Finished converting data files to Little Endian format...\n";
  std::cout << "The Little Endian files have lcoord as the base name.\n";
  return 0;
}


int main(int argc, char** argv)
{
  char* aBaseName;
  unsigned int aThreadSize;
  if(argc == 1)
    {
      aBaseName = "visualLog0.dat";
    }
  else if(argc == 2)
    {
      aBaseName = argv[1];
    }
  else if(argc == 3)
    {
      if( argv[1][1] == 'b' )
        {
          if( convertFiles2LittleEndian( argv[2] ) )
            {
              printUsage( argv[0] );
              std::exit(1);
            }
          aBaseName = "lvisualLog0.dat";
        }
      else
        {
          std::cout << "Unknown option " << argv[1] << " ignored\n";
          printUsage( argv[0] );
          std::exit(1);
        }
    }
  else
    {
      printUsage( argv[0] );
      std::exit(1);
    }
  std::ostringstream aFileName;
  aFileName << aBaseName << std::ends;
  std::ifstream aParentFile( aFileName.str().c_str(), std::ios::binary );
  if ( !aParentFile.is_open() )
    {
      std::cerr << "Could not open file: " << aBaseName <<  
        std::endl;
      printUsage( argv[0] );
      std::exit(1);
    }
  else
    {
      aParentFile.read((char *)(&aThreadSize), sizeof(unsigned int)); 
      for( unsigned int i(1); i!=aThreadSize; ++i )
        {
          std::ostringstream aFileName;
          aFileName << aBaseName << std::ends;
          std::ifstream aFile( aFileName.str().c_str(), std::ios::binary );
          if( !aFile.is_open() )
            {
              printNotEndian( "Little", aBaseName, i, aThreadSize );
              printUsage( argv[0] );
              std::exit(1);
            }
        }
    }
  Gtk::Main anApp(argc, argv);
  Gtk::GL::init(argc, argv);
  Glib::RefPtr<Gdk::GL::Config> aGLConfig;
  aGLConfig = Gdk::GL::Config::create(Gdk::GL::MODE_RGB |
                                      Gdk::GL::MODE_DEPTH |
                                      Gdk::GL::MODE_DOUBLE);
  if (!aGLConfig)
    {
      std::cerr << "*** Cannot find the double-buffered visual.\n"
                << "*** Trying single-buffered visual.\n";
      aGLConfig = Gdk::GL::Config::create(Gdk::GL::MODE_RGB   |
                                         Gdk::GL::MODE_DEPTH);
      if (!aGLConfig)
        {
          std::cerr << "*** Cannot find any OpenGL-capable visual.\n";
          std::exit(1);
        }
    }

  Rulers aRuler(aGLConfig, aBaseName);
  Gtk::Main::run(aRuler);
  return 0;
}



