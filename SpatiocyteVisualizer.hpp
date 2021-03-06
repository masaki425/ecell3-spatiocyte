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


#ifndef __SpatiocyteVisualizer_hpp
#define __SpatiocyteVisualizer_hpp

#define SPHERE          1
#define BOX             2
#define GRID            3

//Lattice type:
#define HCP_LATTICE   0
#define CUBIC_LATTICE 1

using namespace std;

struct Color
{
  float r;
  float g;
  float b;
};

struct Point 
{
  double  x;
  double  y;
  double  z;
};

class GLScene;

class ControlBox : public Gtk::ScrolledWindow
{
public:
  ControlBox(GLScene *anArea);
  virtual ~ControlBox();
  void setStep(char* buffer);
  void setTime(char* buffer);
  void setXangle(double);
  void setYangle(double);
  void setZangle(double);
protected:
  void on_checkbutton_toggled(unsigned int id);
  bool on_checkbutton_clicked(GdkEventButton*, unsigned int);
  void update_species_color(unsigned int, Gtk::ColorSelection*);
  void on_3DMolecule_toggled();
  void on_showTime_toggled();
  void on_record_toggled();
  void on_resetTime_clicked();
  void onResetRotation();
  void onResetBound();
  void xRotateChanged();
  void yRotateChanged();
  void zRotateChanged();
  void xUpBoundChanged();
  void xLowBoundChanged();
  void yUpBoundChanged();
  void yLowBoundChanged();
  void zUpBoundChanged();
  void zLowBoundChanged();
  Gtk::Table m_table;
  Gtk::CheckButton** theButtonList;
  Gtk::Label** theLabelList;
  unsigned int theSpeciesSize;
private:
  Gtk::Entry m_steps;
  Gtk::Entry m_time;
  Gtk::HBox m_stepBox;
  Gtk::HBox m_timeBox;
  Gtk::HBox m_rightBox;
  Gtk::VBox theBoxCtrl;
  Gtk::Frame theFrameRotAdj;
  Gtk::VBox theBoxInFrame;
  Gtk::ToggleButton m_3d;
  Gtk::ToggleButton m_showTime;
  Gtk::Label m_stepLabel;
  Gtk::Label m_timeLabel;
  Gtk::HBox theXBox;
  Gtk::HBox theYBox;
  Gtk::HBox theZBox;
  Gtk::HBox theBoxRotFixReset;
  Gtk::Button theResetRotButton;
  Gtk::CheckButton theCheckFix;
  Gtk::Label theXLabel;
  Gtk::Adjustment theXAdj;
  Gtk::HScale theXScale;
  Gtk::SpinButton theXSpin;
  Gtk::Label theYLabel;
  Gtk::Adjustment theYAdj;
  Gtk::HScale theYScale;
  Gtk::SpinButton theYSpin;
  Gtk::Label theZLabel;
  Gtk::Adjustment theZAdj;
  Gtk::HScale theZScale;
  Gtk::SpinButton theZSpin;
  Gtk::Frame theFrameBoundAdj;
  Gtk::VBox theBoxInBound;
  Gtk::HBox theXUpBoundBox;
  Gtk::HBox theXLowBoundBox;
  Gtk::HBox theYUpBoundBox;
  Gtk::HBox theYLowBoundBox;
  Gtk::HBox theZUpBoundBox;
  Gtk::HBox theZLowBoundBox;
  Gtk::HBox theBoxBoundFixReset;
  Gtk::CheckButton theCheckFixBound;
  Gtk::Button theResetBoundButton;
  Gtk::Label theXUpBoundLabel;
  Gtk::Adjustment theXUpBoundAdj;
  Gtk::HScale theXUpBoundScale;
  Gtk::SpinButton theXUpBoundSpin;
  Gtk::Label theXLowBoundLabel;
  Gtk::Adjustment theXLowBoundAdj;
  Gtk::HScale theXLowBoundScale;
  Gtk::SpinButton theXLowBoundSpin;
  Gtk::Label theYUpBoundLabel;
  Gtk::Adjustment theYUpBoundAdj;
  Gtk::HScale theYUpBoundScale;
  Gtk::SpinButton theYUpBoundSpin;
  Gtk::Label theYLowBoundLabel;
  Gtk::Adjustment theYLowBoundAdj;
  Gtk::HScale theYLowBoundScale;
  Gtk::SpinButton theYLowBoundSpin;
  Gtk::Label theZUpBoundLabel;
  Gtk::Adjustment theZUpBoundAdj;
  Gtk::HScale theZUpBoundScale;
  Gtk::SpinButton theZUpBoundSpin;
  Gtk::Label theZLowBoundLabel;
  Gtk::Adjustment theZLowBoundAdj;
  Gtk::HScale theZLowBoundScale;
  Gtk::SpinButton theZLowBoundSpin;
  Gtk::Frame theFrameLatticeAdj;
  Gtk::VBox theBoxInLattice;
  Gtk::HBox theDepthBox;
  Gtk::HBox the3DMoleculeBox;
  Gtk::Button theResetDepthButton;
  Gtk::CheckButton theCheck3DMolecule;
  Gtk::CheckButton theCheckShowTime;
  Gtk::Button theButtonResetTime;
  Gtk::Label theDepthLabel;
  Gtk::Adjustment theDepthAdj;
  Gtk::HScale theDepthScale;
  Gtk::SpinButton theDepthSpin;
  Gtk::ToggleButton theButtonRecord;
  Glib::RefPtr<Gtk::SizeGroup> m_sizeGroup;
  Gtk::ColorButton m_Button;
protected:
  GLScene *m_area;
};

class GLScene : public Gtk::GL::DrawingArea
{
public:
  static const unsigned int TIMEOUT_INTERVAL;

public:
  GLScene(const Glib::RefPtr<const Gdk::GL::Config>& config,
          const char* aFileName);
  virtual ~GLScene();

protected:
  virtual void on_realize();
  virtual bool on_configure_event(GdkEventConfigure* event);
  virtual bool on_expose_event(GdkEventExpose* event);
  virtual bool on_map_event(GdkEventAny* event);
  virtual bool on_unmap_event(GdkEventAny* event);
  virtual bool on_visibility_notify_event(GdkEventVisibility* event);
  virtual bool on_timeout();

public:
  // Invalidate whole window.
  void rotate(int aMult, int x, int y, int z);
  void translate(int x, int y, int z);
  void rotateMidAxis(int aMult, int x, int y, int z);
  void rotateMidAxisAbs(double, int , int , int );
  void resetBound();
  void pause();
  void play();
  void resetView();
  void zoomIn();
  void zoomOut();
  bool writePng();
  void step();
  void setReverse(bool isReverse);
  void setSpeciesVisibility(unsigned int id, bool isVisible);
  bool getSpeciesVisibility(unsigned int id);
  void set3DMolecule(bool is3D);
  void setShowTime(bool isShowTime);
  void setRecord(bool isRecord);
  void resetTime();
  void setControlBox(ControlBox* aControl);
  Color getSpeciesColor(unsigned int id);
  void setSpeciesColor(unsigned int id, Color);
  char* getSpeciesName(unsigned int id);
  void invalidate() {
    get_window()->invalidate_rect(get_allocation(), false);
  }

  // Update window synchronously (fast).
  void update()
  { get_window()->process_updates(false); }

  void setXUpBound( unsigned int aBound );
  void setXLowBound( unsigned int aBound );
  void setYUpBound( unsigned int aBound );
  void setYLowBound( unsigned int aBound );
  void setZUpBound( unsigned int aBound );
  void setZLowBound( unsigned int aBound );
  unsigned int getSpeciesSize()
    {
      return theTotalSpeciesSize;
    };
  unsigned int getLayerSize()
    {
      return theLayerSize;
    };
  unsigned int getColSize()
    {
      return theColSize;
    };
  unsigned int getRowSize()
    {
      return theRowSize;
    };
  Color getColor(unsigned int i)
    {
      return theSpeciesColor[i];
    };

protected:
  void drawBox(GLfloat xlo, GLfloat xhi, GLfloat ylo, GLfloat yhi,
                      GLfloat zlo, GLfloat zhi);
  void drawScene(double);
  void timeout_add();
  void plotGrid();
  void plot3DHCPMolecules();
  void plotMean3DHCPMolecules();
  void plotHCPPoints();
  void plot3DCubicMolecules();
  void plotMean3DCubicMolecules();
  void plotCubicPoints();
  void timeout_remove();
  void loadCoords();
  void loadMeanCoords();
  void setColor(unsigned int i, Color *c);
  void setRandColor(Color *c);
  void setTranslucentColor(unsigned int i, GLfloat j);
  void setLayerColor(unsigned int i);
  void (GLScene::*thePlotFunction)();
  void (GLScene::*thePlot3DFunction)();
  void (GLScene::*theLoadCoordsFunction)();
  void normalizeAngle(double&);
protected:
  double xAngle;
  double yAngle;
  double zAngle;
  bool m_Run;
  bool m_RunReverse;
  sigc::connection m_ConnectionTimeout;
  Glib::ustring m_FontString;
  Glib::ustring m_timeString;
  GLuint m_FontListBase;
  int m_FontHeight;
  int m_FontWidth;
protected:
  unsigned int theThreadSize;
  unsigned int theLatticeType;
  unsigned int theDimension;
  unsigned int theColSize;
  unsigned int theRowSize;
  unsigned int theLayerSize;
  unsigned int theSpeciesSize;
  unsigned int thePolymerSize;
  unsigned int theReservedSize;
  unsigned int theTotalSpeciesSize;
  unsigned int theTotalCoordSpeciesSize;
  std::vector<unsigned int> thePolySpeciesList;
  char** theSpeciesNameList;
  double theRealColSize;
  double theRealLayerSize;
  double theRealRowSize;
  unsigned int theOriCol;
  unsigned int theOriRow;
  unsigned int theOriLayer;
  unsigned int theLogMarker;
  unsigned int* theRegionSep;
  unsigned int ***theCoords;
  unsigned int ***theFrequency;
  Point        ***thePoints;
  unsigned int **theMoleculeSize;
  unsigned int **thePolymerMoleculeSize;
  unsigned int **theMeanCoords;
  unsigned int theStartCoord;
  unsigned int theCutCol;
  unsigned int theCutLayer;
  unsigned int theCutRow;
  unsigned int theMeanCount;
  unsigned int theMeanCoordSize;
  Color *theSpeciesColor;
  bool *theSpeciesVisibility;
  std::ifstream** theFile;
  double theRadius;
  double theResolution;
  double theCurrentTime;
  int thePrevSize;
  int theNextSize;
  bool isChanged;
  GLfloat theHCPk;
  GLfloat theHCPh;
  GLfloat theHCPl;
  GLfloat theBCCc;
  GLfloat prevX;
  GLfloat prevY;
  GLfloat prevZ;
  GLfloat X;
  GLfloat Y;
  GLfloat Z;
  GLfloat ViewSize;
  GLfloat ViewMidx;
  GLfloat ViewMidy;
  GLfloat ViewMidz;
  GLfloat FieldOfView;
  GLfloat Xtrans;
  GLfloat Ytrans;
  GLfloat Near;
  GLfloat Aspect;
  unsigned int m_stepCnt;
  unsigned int thePngNumber;
  GLfloat theRotateAngle;
  ControlBox* m_control;
  bool show3DMolecule;
  bool startRecord;
  unsigned int* theXUpBound;
  unsigned int* theXLowBound;
  unsigned int* theYUpBound;
  unsigned int* theYLowBound;
  unsigned int* theZUpBound;
  unsigned int* theZLowBound;
  double theResetTime;
  bool showTime;
};

class Rulers : public Gtk::Window
{
public:
  Rulers(const Glib::RefPtr<const Gdk::GL::Config>& config,
         const char* aFileName);

protected:

  //signal handlers:
  virtual bool on_area_motion_notify_event(GdkEventMotion* event); //override
  virtual bool on_key_press_event(GdkEventKey* event);

  GLScene m_area;
  Gtk::Table m_table;
  Gtk::HPaned m_hbox;
  ControlBox m_control;
  //Gtk::DrawingArea m_area;
  Gtk::HRuler m_hrule;
  Gtk::VRuler m_vrule;
  static const int XSIZE = 200, YSIZE = 200;
  bool isRecord;
};

#endif /* __SpatiocyteVisualizer_hpp */

