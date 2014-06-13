#include <algorithm>
#ifdef ANDROIDSDL
#include <android/log.h>
#endif
#include <guichan.hpp>
#include <iostream>
#include <sstream>
#include <SDL/SDL_ttf.h>
#include <guichan/sdl.hpp>
#include "sdltruetypefont.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"

#include "uaeradiobutton.hpp"
#include "uaedropdown.hpp"
#include "menu.h"
#include "menu_config.h"
#include "options.h"
#include "gui.h"


extern int moveY;
extern unsigned int sound_rate;


namespace widgets
{
  void check_presetModeId(void);
  
  extern gcn::Color baseCol;
  extern gcn::Color baseColLabel;
  extern gcn::Container* top;
  extern gcn::TabbedArea* tabbedArea;
  extern gcn::Icon* icon_winlogo;
#ifdef ANDROIDSDL
  extern gcn::contrib::SDLTrueTypeFont* font14;
#endif

  // Tab Display
  gcn::Container *tab_displaysound;
  gcn::Window *group_width;
  gcn::UaeRadioButton* radioButton_visibleAreaWidth_320;
  gcn::UaeRadioButton* radioButton_visibleAreaWidth_640;
  gcn::UaeRadioButton* radioButton_visibleAreaWidth_352;
  gcn::UaeRadioButton* radioButton_visibleAreaWidth_704;
  gcn::UaeRadioButton* radioButton_visibleAreaWidth_384;
  gcn::UaeRadioButton* radioButton_visibleAreaWidth_768;
  gcn::Window *group_height;
  gcn::UaeRadioButton* radioButton_displayedLines_200;
  gcn::UaeRadioButton* radioButton_displayedLines_216;
  gcn::UaeRadioButton* radioButton_displayedLines_240;
  gcn::UaeRadioButton* radioButton_displayedLines_256;
  gcn::UaeRadioButton* radioButton_displayedLines_262;
  gcn::UaeRadioButton* radioButton_displayedLines_270;
  gcn::Window *group_frameskip;
  gcn::UaeRadioButton* radioButton_frameskip_0;
  gcn::UaeRadioButton* radioButton_frameskip_1;
  gcn::Container* backgrd_vertical_pos;
  gcn::Container* backgrd_cut_left;
  gcn::Container* backgrd_cut_right;
  gcn::Label* label_vertical_pos;
  gcn::Label* label_cut_left;
  gcn::Label* label_cut_right;
  gcn::UaeDropDown* dropDown_vertical_pos;
  gcn::UaeDropDown* dropDown_cut_left;
  gcn::UaeDropDown* dropDown_cut_right;
  
  // Sound
  gcn::Window *group_refreshrate;
  gcn::UaeRadioButton* radioButton_refreshrate_50Hz;
  gcn::UaeRadioButton* radioButton_refreshrate_60Hz;
  gcn::Window *group_sound_enable;
  gcn::UaeRadioButton* radioButton_sound_off;
  gcn::UaeRadioButton* radioButton_sound_fast;
  gcn::UaeRadioButton* radioButton_sound_accurate;
  gcn::Window *group_sound_rate;
  gcn::UaeRadioButton* radioButton_soundrate_8k;
  gcn::UaeRadioButton* radioButton_soundrate_11k;
  gcn::UaeRadioButton* radioButton_soundrate_22k;
  gcn::UaeRadioButton* radioButton_soundrate_32k;
  gcn::UaeRadioButton* radioButton_soundrate_44k;
  gcn::Window *group_sound_mode;
  gcn::UaeRadioButton* radioButton_soundmode_mono;
  gcn::UaeRadioButton* radioButton_soundmode_stereo;
#ifdef ANDROIDSDL
  gcn::Label* label1_sound;
  gcn::Label* label2_sound;
#endif


  class verposListModel : public gcn::ListModel
  {
    private:
      std::ostringstream ostr[94];

    public:
      verposListModel()
      {
	      for (int j=0; j<94; j++)
	        ostr[j] << j-42;
      }
      
      int getNumberOfElements()
      {
        return 93;
      }

      std::string getElementAt(int i)
      {
        return ostr[i].str().c_str();
      }
  };
  verposListModel verposList;  


  class cutleftListModel : public gcn::ListModel
  {
    private:
      std::ostringstream ostr[102];
        
    public:
      cutleftListModel()
      {
	      for (int j=0; j<102; j++)
	        ostr[j] << j;
      }

      int getNumberOfElements()
      {
        return 101;
      }

      std::string getElementAt(int i)
      {
        return ostr[i].str().c_str();
      }
  };
  cutleftListModel cutleftList;      


  class cutrightListModel : public gcn::ListModel
  {
    private:
  	  std::ostringstream ostr[102];

    public:
      cutrightListModel()
      {
	      for (int j=0; j<102; j++)
	        ostr[j] << j;
      }
      
      int getNumberOfElements()
      {
        return 101;
      }

      std::string getElementAt(int i)
      {
      	return ostr[i].str().c_str();
      }
  };
  cutrightListModel cutrightList;        


  class WidthActionListener : public gcn::ActionListener
  {
    public:
      void action(const gcn::ActionEvent& actionEvent)
      {
	      if (actionEvent.getSource() == radioButton_visibleAreaWidth_320)
  	    {
		      visibleAreaWidth=320;
      		mainMenu_displayHires=0;
	      }
	      else if (actionEvent.getSource() == radioButton_visibleAreaWidth_640)
	      {
		      visibleAreaWidth=640;
		      mainMenu_displayHires=1;
	      }
	      else if (actionEvent.getSource() == radioButton_visibleAreaWidth_352)
	      {
		      visibleAreaWidth=352;
		      mainMenu_displayHires=0;
	      }
	      else if (actionEvent.getSource() == radioButton_visibleAreaWidth_704)
	      {
		      visibleAreaWidth=704;
		      mainMenu_displayHires=1;
	      }
	      else if (actionEvent.getSource() == radioButton_visibleAreaWidth_384)
	      {
		      visibleAreaWidth=384;
		      mainMenu_displayHires=0;
	      }
	      else if (actionEvent.getSource() == radioButton_visibleAreaWidth_768)
	      {
		      visibleAreaWidth=768;
		      mainMenu_displayHires=1;
	      }
	      check_presetModeId();
      }
  };
  WidthActionListener* widthActionListener;


  class HeightActionListener : public gcn::ActionListener
  {
    public:
      void action(const gcn::ActionEvent& actionEvent)
      {
  	    if (actionEvent.getSource() == radioButton_displayedLines_200)
	  	    mainMenu_displayedLines=200;
	      else if (actionEvent.getSource() == radioButton_displayedLines_216)
		      mainMenu_displayedLines=216;
	      else if (actionEvent.getSource() == radioButton_displayedLines_240)
		      mainMenu_displayedLines=240;
	      else if (actionEvent.getSource() == radioButton_displayedLines_256)
		      mainMenu_displayedLines=256;
	      else if (actionEvent.getSource() == radioButton_displayedLines_262)
		      mainMenu_displayedLines=262;
	      else if (actionEvent.getSource() == radioButton_displayedLines_270)
		      mainMenu_displayedLines=270;
        check_presetModeId();
      }
  };
  HeightActionListener* heightActionListener;


  class FrameskipActionListener : public gcn::ActionListener
  {
    public:
      void action(const gcn::ActionEvent& actionEvent)
      {
  	    if (actionEvent.getSource() == radioButton_frameskip_0)
	      	mainMenu_frameskip=0;
	      else if (actionEvent.getSource() == radioButton_frameskip_1)
		      mainMenu_frameskip=1;
      }
  };
  FrameskipActionListener* frameskipActionListener;


  class RefreshRateActionListener : public gcn::ActionListener
  {
    public:
      void action(const gcn::ActionEvent& actionEvent)
      {
	      if (actionEvent.getSource() == radioButton_refreshrate_50Hz)
		      mainMenu_ntsc=false;
	      else if (actionEvent.getSource() == radioButton_refreshrate_60Hz)
		      mainMenu_ntsc=true;
      }
  };
  RefreshRateActionListener* refreshRateActionListener;


  class VerticalPosActionListener : public gcn::ActionListener
  {
    public:
      void action(const gcn::ActionEvent& actionEvent)
      {
	      moveY = dropDown_vertical_pos->getSelected()-42;
      }
  };
  VerticalPosActionListener* verticalPosActionListener;


  class CutLeftActionListener : public gcn::ActionListener
  {
    public:
      void action(const gcn::ActionEvent& actionEvent)
      {
	      mainMenu_cutLeft = dropDown_cut_left->getSelected();
      }
  };
  CutLeftActionListener* cutLeftActionListener;


  class CutRightActionListener : public gcn::ActionListener
  {
    public:
      void action(const gcn::ActionEvent& actionEvent)
      {
	      mainMenu_cutRight = dropDown_cut_right->getSelected();
      }
  };
  CutRightActionListener* cutRightActionListener;


  class SoundActionListener : public gcn::ActionListener
  {
    public:
      void action(const gcn::ActionEvent& actionEvent)
      {
  	    if (actionEvent.getSource() == radioButton_sound_off)
		      mainMenu_sound=0;
	      else if (actionEvent.getSource() == radioButton_sound_fast)
		      mainMenu_sound=1;
	      else if (actionEvent.getSource() == radioButton_sound_accurate)
		      mainMenu_sound=2;
      }
  };
  SoundActionListener* soundActionListener;


  class SoundrateActionListener : public gcn::ActionListener
  {
    public:
      void action(const gcn::ActionEvent& actionEvent)
      {
	      if (actionEvent.getSource() == radioButton_soundrate_8k)
		      sound_rate=8000;
	      else if (actionEvent.getSource() == radioButton_soundrate_11k)
		      sound_rate=11025;
	      else if (actionEvent.getSource() == radioButton_soundrate_22k)
		      sound_rate=22050;
	      else if (actionEvent.getSource() == radioButton_soundrate_32k)
		      sound_rate=32000;
	      else if (actionEvent.getSource() == radioButton_soundrate_44k)
		      sound_rate=44100;
      }
  };
  SoundrateActionListener* soundrateActionListener;


  class SoundmodeActionListener : public gcn::ActionListener
  {
    public:
      void action(const gcn::ActionEvent& actionEvent)
      {
  	    if (actionEvent.getSource() == radioButton_soundmode_mono)
	      	mainMenu_soundStereo=0;
	      else if (actionEvent.getSource() == radioButton_soundmode_stereo)
		      mainMenu_soundStereo=1;
      }
  };
  SoundmodeActionListener* soundmodeActionListener;


  void menuTabDisplaySound_Init()
  {
  	// Select screen width
  	radioButton_visibleAreaWidth_320 = new gcn::UaeRadioButton("320", "radiowidthgroup");
  	radioButton_visibleAreaWidth_320->setPosition(5,10);
  	radioButton_visibleAreaWidth_320->setId("320");
  	widthActionListener = new WidthActionListener();
  	radioButton_visibleAreaWidth_320->addActionListener(widthActionListener);
  	radioButton_visibleAreaWidth_640 = new gcn::UaeRadioButton("640", "radiowidthgroup");
  	radioButton_visibleAreaWidth_640->setPosition(5,40);
  	radioButton_visibleAreaWidth_640->setId("640");
  	radioButton_visibleAreaWidth_640->addActionListener(widthActionListener);
  	radioButton_visibleAreaWidth_352 = new gcn::UaeRadioButton("352", "radiowidthgroup");
  	radioButton_visibleAreaWidth_352->setPosition(5,70);
  	radioButton_visibleAreaWidth_352->setId("352");
  	radioButton_visibleAreaWidth_352->addActionListener(widthActionListener);
  	radioButton_visibleAreaWidth_704 = new gcn::UaeRadioButton("704", "radiowidthgroup");
  	radioButton_visibleAreaWidth_704->setPosition(5,100);
  	radioButton_visibleAreaWidth_704->setId("704");
  	radioButton_visibleAreaWidth_704->addActionListener(widthActionListener);
  	radioButton_visibleAreaWidth_384 = new gcn::UaeRadioButton("384", "radiowidthgroup");
  	radioButton_visibleAreaWidth_384->setPosition(5,130);
  	radioButton_visibleAreaWidth_384->setId("384");
  	radioButton_visibleAreaWidth_384->addActionListener(widthActionListener);
  	radioButton_visibleAreaWidth_768 = new gcn::UaeRadioButton("768", "radiowidthgroup");
  	radioButton_visibleAreaWidth_768->setPosition(5,160);
  	radioButton_visibleAreaWidth_768->setId("768");
  	radioButton_visibleAreaWidth_768->addActionListener(widthActionListener);
  	group_width = new gcn::Window("Width");
  	group_width->setPosition(10,20);
  	group_width->add(radioButton_visibleAreaWidth_320);
  	group_width->add(radioButton_visibleAreaWidth_640);
  	group_width->add(radioButton_visibleAreaWidth_352);
  	group_width->add(radioButton_visibleAreaWidth_704);
  	group_width->add(radioButton_visibleAreaWidth_384);
  	group_width->add(radioButton_visibleAreaWidth_768);
  	group_width->setMovable(false);
  	group_width->setSize(70,205);
    group_width->setBaseColor(baseCol);

    // Select Screen height
  	radioButton_displayedLines_200 = new gcn::UaeRadioButton("200", "radioheightgroup");
  	radioButton_displayedLines_200->setPosition(5,10);
  	radioButton_displayedLines_200->setId("200");
    heightActionListener = new HeightActionListener();
  	radioButton_displayedLines_200->addActionListener(heightActionListener);
  	radioButton_displayedLines_216 = new gcn::UaeRadioButton("216", "radioheightgroup");
  	radioButton_displayedLines_216->setPosition(5,40);
  	radioButton_displayedLines_216->setId("216");
  	radioButton_displayedLines_216->addActionListener(heightActionListener);
  	radioButton_displayedLines_240 = new gcn::UaeRadioButton("240", "radioheightgroup");
  	radioButton_displayedLines_240->setPosition(5,70);
  	radioButton_displayedLines_240->setId("240");
  	radioButton_displayedLines_240->addActionListener(heightActionListener);
  	radioButton_displayedLines_256 = new gcn::UaeRadioButton("256", "radioheightgroup");
  	radioButton_displayedLines_256->setPosition(5,100);
  	radioButton_displayedLines_256->setId("256");
  	radioButton_displayedLines_256->addActionListener(heightActionListener);
  	radioButton_displayedLines_262 = new gcn::UaeRadioButton("262", "radioheightgroup");
  	radioButton_displayedLines_262->setPosition(5,130);
  	radioButton_displayedLines_262->setId("262");
  	radioButton_displayedLines_262->addActionListener(heightActionListener);
  	radioButton_displayedLines_270 = new gcn::UaeRadioButton("270", "radioheightgroup");
  	radioButton_displayedLines_270->setPosition(5,160);
  	radioButton_displayedLines_270->setId("270");
  	radioButton_displayedLines_270->addActionListener(heightActionListener);
  	group_height = new gcn::Window("Height");
  	group_height->setPosition(95,20);
  	group_height->add(radioButton_displayedLines_200);
  	group_height->add(radioButton_displayedLines_216);
  	group_height->add(radioButton_displayedLines_240);
  	group_height->add(radioButton_displayedLines_256);
  	group_height->add(radioButton_displayedLines_262);
  	group_height->add(radioButton_displayedLines_270);
  	group_height->setMovable(false);
  	group_height->setSize(70,205);
    group_height->setBaseColor(baseCol);
    
    // Select Frameskip
  	radioButton_frameskip_0 = new gcn::UaeRadioButton("0", "radioframeskipgroup");
  	radioButton_frameskip_0->setPosition(5,10);
  	radioButton_frameskip_0->setId("Frameskip0");
    frameskipActionListener = new FrameskipActionListener();
  	radioButton_frameskip_0->addActionListener(frameskipActionListener);
  	radioButton_frameskip_1 = new gcn::UaeRadioButton("1", "radioframeskipgroup");
  	radioButton_frameskip_1->setPosition(5,40);
  	radioButton_frameskip_1->setId("Frameskip1");
  	radioButton_frameskip_1->addActionListener(frameskipActionListener);
  	group_frameskip = new gcn::Window("Frameskip");
  	group_frameskip->setPosition(180,20);
  	group_frameskip->setMovable(false);
  	group_frameskip->add(radioButton_frameskip_0);
  	group_frameskip->add(radioButton_frameskip_1);
  	group_frameskip->setSize(80,85);
    group_frameskip->setBaseColor(baseCol);
    
  	label_vertical_pos = new gcn::Label("Vert.Pos.");
  	label_vertical_pos->setPosition(4, 2);
  	backgrd_vertical_pos = new gcn::Container();
  	backgrd_vertical_pos->setOpaque(true);
  	backgrd_vertical_pos->setBaseColor(baseColLabel);
  	backgrd_vertical_pos->setPosition(180, 120);
  	backgrd_vertical_pos->setSize(70, 21);
    backgrd_vertical_pos->add(label_vertical_pos);
  	dropDown_vertical_pos = new gcn::UaeDropDown(&verposList);
  	dropDown_vertical_pos->setPosition(260,120);
    dropDown_vertical_pos->setBaseColor(baseCol);
    dropDown_vertical_pos->setId("VertPos");
    verticalPosActionListener = new VerticalPosActionListener();
  	dropDown_vertical_pos->addActionListener(verticalPosActionListener);
    
  	label_cut_left = new gcn::Label("Cut Left");
  	label_cut_left->setPosition(4, 2);
  	backgrd_cut_left = new gcn::Container();
  	backgrd_cut_left->setOpaque(true);
  	backgrd_cut_left->setBaseColor(baseColLabel);
  	backgrd_cut_left->setPosition(180, 150);
  	backgrd_cut_left->setSize(70, 21);
    backgrd_cut_left->add(label_cut_left);
  	dropDown_cut_left = new gcn::UaeDropDown(&cutleftList);
  	dropDown_cut_left->setPosition(260,150);
    dropDown_cut_left->setBaseColor(baseCol);
    dropDown_cut_left->setId("CutLeft");
    cutLeftActionListener = new CutLeftActionListener();
  	dropDown_cut_left->addActionListener(cutLeftActionListener);
    
  	label_cut_right = new gcn::Label("Cut Right");
  	label_cut_right->setPosition(4, 2);
  	backgrd_cut_right = new gcn::Container();
  	backgrd_cut_right->setOpaque(true);
  	backgrd_cut_right->setBaseColor(baseColLabel);
  	backgrd_cut_right->setPosition(180, 180);
  	backgrd_cut_right->setSize(70, 21);
    backgrd_cut_right->add(label_cut_right);
  	dropDown_cut_right = new gcn::UaeDropDown(&cutrightList);
  	dropDown_cut_right->setPosition(260,180);
    dropDown_cut_right->setBaseColor(baseCol);
    dropDown_cut_right->setId("CutRight");
    cutRightActionListener = new CutRightActionListener();
  	dropDown_cut_right->addActionListener(cutRightActionListener);

  	radioButton_refreshrate_50Hz = new gcn::UaeRadioButton("50Hz", "radiorefreshrategroup");
  	radioButton_refreshrate_50Hz->setPosition(5,10);
  	radioButton_refreshrate_50Hz->setId("50Hz");
    refreshRateActionListener = new RefreshRateActionListener();
  	radioButton_refreshrate_50Hz->addActionListener(refreshRateActionListener);
  	radioButton_refreshrate_60Hz = new gcn::UaeRadioButton("60Hz", "radiorefreshrategroup");
  	radioButton_refreshrate_60Hz->setPosition(5,40);
  	radioButton_refreshrate_60Hz->setId("60Hz");
  	radioButton_refreshrate_60Hz->addActionListener(refreshRateActionListener);
  	group_refreshrate = new gcn::Window("Refresh Rate");
  	group_refreshrate->setPosition(275,20);
  	group_refreshrate->add(radioButton_refreshrate_50Hz);
  	group_refreshrate->add(radioButton_refreshrate_60Hz);
  	group_refreshrate->setMovable(false);
  	group_refreshrate->setSize(100,85);
    group_refreshrate->setBaseColor(baseCol);
    
  	// Select Sound enable/accuracy
  	radioButton_sound_off = new gcn::UaeRadioButton("off", "radiosoundpresentgroup");
  	radioButton_sound_off->setPosition(5,10);
  	radioButton_sound_off->setId("SoundOff");
   	soundActionListener = new SoundActionListener();
  	radioButton_sound_off->addActionListener(soundActionListener);
  	radioButton_sound_fast = new gcn::UaeRadioButton("fast", "radiosoundpresentgroup");
  	radioButton_sound_fast->setPosition(5,40);
  	radioButton_sound_fast->setId("SoundFast");
  	radioButton_sound_fast->addActionListener(soundActionListener);
  	radioButton_sound_accurate = new gcn::UaeRadioButton("accurate", "radiosoundpresentgroup");
  	radioButton_sound_accurate->setPosition(5,70);
  	radioButton_sound_accurate->setId("SoundAcc");
  	radioButton_sound_accurate->addActionListener(soundActionListener);
  	group_sound_enable = new gcn::Window("Sound");
  	group_sound_enable->setPosition(390,20);
  	group_sound_enable->add(radioButton_sound_off);
  	group_sound_enable->add(radioButton_sound_fast);
  	group_sound_enable->add(radioButton_sound_accurate);	
  	group_sound_enable->setMovable(false);
  	group_sound_enable->setSize(100,115);
    group_sound_enable->setBaseColor(baseCol);

    // Select Soundrate
  	radioButton_soundrate_8k = new gcn::UaeRadioButton("8K", "radiosoundrategroup");
  	radioButton_soundrate_8k->setPosition(5,10);
  	radioButton_soundrate_8k->setId("8K");
    soundrateActionListener = new SoundrateActionListener();
  	radioButton_soundrate_8k->addActionListener(soundrateActionListener);
  	radioButton_soundrate_11k = new gcn::UaeRadioButton("11K", "radiosoundrategroup");
  	radioButton_soundrate_11k->setPosition(5,40);
  	radioButton_soundrate_11k->setId("11K");
  	radioButton_soundrate_11k->addActionListener(soundrateActionListener);
  	radioButton_soundrate_22k = new gcn::UaeRadioButton("22K", "radiosoundrategroup");
  	radioButton_soundrate_22k->setPosition(5,70);
  	radioButton_soundrate_22k->setId("22K");
  	radioButton_soundrate_22k->addActionListener(soundrateActionListener);
  	radioButton_soundrate_32k = new gcn::UaeRadioButton("32K", "radiosoundrategroup");
  	radioButton_soundrate_32k->setPosition(5,100);
  	radioButton_soundrate_32k->setId("32K");
  	radioButton_soundrate_32k->addActionListener(soundrateActionListener);
  	radioButton_soundrate_44k = new gcn::UaeRadioButton("44K", "radiosoundrategroup");
  	radioButton_soundrate_44k->setPosition(5,130);
  	radioButton_soundrate_44k->setId("44K");
  	radioButton_soundrate_44k->addActionListener(soundrateActionListener);
  	group_sound_rate = new gcn::Window("Soundrate");
  	group_sound_rate->setPosition(505,20);
  	group_sound_rate->add(radioButton_soundrate_8k);
  	group_sound_rate->add(radioButton_soundrate_11k);
  	group_sound_rate->add(radioButton_soundrate_22k);
  	group_sound_rate->add(radioButton_soundrate_32k);
  	group_sound_rate->add(radioButton_soundrate_44k);
  	group_sound_rate->setMovable(false);
  	group_sound_rate->setSize(80,175);
    group_sound_rate->setBaseColor(baseCol);

    // Select Sound mode
  	radioButton_soundmode_mono = new gcn::UaeRadioButton("mono", "radiosoundmodegroup");
  	radioButton_soundmode_mono->setPosition(5,10);
  	radioButton_soundmode_mono->setId("mono");
    soundmodeActionListener = new SoundmodeActionListener();
  	radioButton_soundmode_mono->addActionListener(soundmodeActionListener);
  	radioButton_soundmode_stereo = new gcn::UaeRadioButton("stereo", "radiosoundmodegroup");
  	radioButton_soundmode_stereo->setPosition(5,40);
  	radioButton_soundmode_stereo->setId("stereo");
  	radioButton_soundmode_stereo->addActionListener(soundmodeActionListener);
  	group_sound_mode = new gcn::Window("Mode");
  	group_sound_mode->setPosition(390,150);
  	group_sound_mode->add(radioButton_soundmode_mono);
  	group_sound_mode->add(radioButton_soundmode_stereo);
  	group_sound_mode->setMovable(false);
  	group_sound_mode->setSize(100,85);
    group_sound_mode->setBaseColor(baseCol);

#ifdef ANDROIDSDL
  	label1_sound = new gcn::Label("Please use Save Setting and restart app");
  	label1_sound->setPosition(330,240);
	label1_sound->setFont(font14);
  	label2_sound = new gcn::Label("after any changes in sound settings.");
  	label2_sound->setPosition(330,255);
	label2_sound->setFont(font14);
#endif

  	tab_displaysound = new gcn::Container();
  	tab_displaysound->add(icon_winlogo);
  	tab_displaysound->add(group_width);
  	tab_displaysound->add(group_height);
  	tab_displaysound->add(group_frameskip);
    tab_displaysound->add(backgrd_vertical_pos);
  	tab_displaysound->add(dropDown_vertical_pos);
    tab_displaysound->add(backgrd_cut_left);
  	tab_displaysound->add(dropDown_cut_left);
    tab_displaysound->add(backgrd_cut_right);
  	tab_displaysound->add(dropDown_cut_right);
  	tab_displaysound->add(group_refreshrate);
  	tab_displaysound->setSize(600,280);
    tab_displaysound->setBaseColor(baseCol);
  	tab_displaysound->add(group_sound_enable);
  	tab_displaysound->add(group_sound_rate);
  	tab_displaysound->add(group_sound_mode);
#ifdef ANDROIDSDL
  	tab_displaysound->add(label1_sound);
  	tab_displaysound->add(label2_sound);
#endif
  	tab_displaysound->setSize(600,280);
    tab_displaysound->setBaseColor(baseCol);
  }


  void menuTabDisplaySound_Exit()
  {
  	delete tab_displaysound;
  	delete group_width;
  	delete radioButton_visibleAreaWidth_320;
  	delete radioButton_visibleAreaWidth_640;
  	delete radioButton_visibleAreaWidth_352;
  	delete radioButton_visibleAreaWidth_704;
  	delete radioButton_visibleAreaWidth_384;
  	delete radioButton_visibleAreaWidth_768;
  	delete group_height;
  	delete radioButton_displayedLines_200;
  	delete radioButton_displayedLines_216;
  	delete radioButton_displayedLines_240;
  	delete radioButton_displayedLines_256;
  	delete radioButton_displayedLines_262;
  	delete radioButton_displayedLines_270;
  	delete group_frameskip;
  	delete radioButton_frameskip_0;
  	delete radioButton_frameskip_1;
  	delete backgrd_vertical_pos;
  	delete backgrd_cut_left;
  	delete backgrd_cut_right;
  	delete label_vertical_pos;
  	delete label_cut_left;
  	delete label_cut_right;
  	delete dropDown_vertical_pos;
  	delete dropDown_cut_left;
  	delete dropDown_cut_right;
  	delete group_refreshrate;
  	delete radioButton_refreshrate_50Hz;
  	delete radioButton_refreshrate_60Hz;
  	delete group_sound_enable;
  	delete radioButton_sound_off;
  	delete radioButton_sound_fast;
  	delete radioButton_sound_accurate;
    delete group_sound_rate;
  	delete radioButton_soundrate_8k;
  	delete radioButton_soundrate_11k;
  	delete radioButton_soundrate_22k;
  	delete radioButton_soundrate_32k;
  	delete radioButton_soundrate_44k;
    delete group_sound_mode;
  	delete radioButton_soundmode_mono;
  	delete radioButton_soundmode_stereo;
#ifdef ANDROIDSDL
  	delete label1_sound;
  	delete label2_sound;
#endif
    
  	delete widthActionListener;
  	delete heightActionListener;
  	delete frameskipActionListener;
  	delete refreshRateActionListener;
  	delete verticalPosActionListener;
  	delete cutLeftActionListener;
  	delete cutRightActionListener;
    delete soundActionListener;
    delete soundrateActionListener;
    delete soundmodeActionListener;
  }


  void check_presetModeId()
  {
    int newPresetModeId = -1;
    
    if (visibleAreaWidth==320 && mainMenu_displayedLines == 200)
      newPresetModeId=0;
    else if (visibleAreaWidth==320 && mainMenu_displayedLines == 216)
      newPresetModeId=1;
    else if (visibleAreaWidth==320 && mainMenu_displayedLines == 240)
      newPresetModeId=2;
    else if (visibleAreaWidth==320 && mainMenu_displayedLines == 256)
      newPresetModeId=3;
    else if (visibleAreaWidth==320 && mainMenu_displayedLines == 262)
      newPresetModeId=4;
    else if (visibleAreaWidth==320 && mainMenu_displayedLines == 270)
      newPresetModeId=5;
    else if (visibleAreaWidth==640 && mainMenu_displayedLines == 200)
      newPresetModeId=10;
    else if (visibleAreaWidth==640 && mainMenu_displayedLines == 216)
      newPresetModeId=11;
    else if (visibleAreaWidth==640 && mainMenu_displayedLines == 240)
      newPresetModeId=12;
    else if (visibleAreaWidth==640 && mainMenu_displayedLines == 256)
      newPresetModeId=13;
    else if (visibleAreaWidth==640 && mainMenu_displayedLines == 262)
      newPresetModeId=14;
    else if (visibleAreaWidth==640 && mainMenu_displayedLines == 270)
      newPresetModeId=15;
    else if (visibleAreaWidth==352 && mainMenu_displayedLines == 200)
      newPresetModeId=20;
    else if (visibleAreaWidth==352 && mainMenu_displayedLines == 216)
      newPresetModeId=21;
    else if (visibleAreaWidth==352 && mainMenu_displayedLines == 240)
      newPresetModeId=22;
    else if (visibleAreaWidth==352 && mainMenu_displayedLines == 256)
      newPresetModeId=23;
    else if (visibleAreaWidth==352 && mainMenu_displayedLines == 262)
      newPresetModeId=24;
    else if (visibleAreaWidth==352 && mainMenu_displayedLines == 270)
      newPresetModeId=25;
    else if (visibleAreaWidth==704 && mainMenu_displayedLines == 200)
      newPresetModeId=30;
    else if (visibleAreaWidth==704 && mainMenu_displayedLines == 216)
      newPresetModeId=31;
    else if (visibleAreaWidth==704 && mainMenu_displayedLines == 240)
      newPresetModeId=32;
    else if (visibleAreaWidth==704 && mainMenu_displayedLines == 256)
      newPresetModeId=33;
    else if (visibleAreaWidth==704 && mainMenu_displayedLines == 262)
      newPresetModeId=34;
    else if (visibleAreaWidth==704 && mainMenu_displayedLines == 270)
      newPresetModeId=35;
    else if (visibleAreaWidth==384 && mainMenu_displayedLines == 200)
      newPresetModeId=40;
    else if (visibleAreaWidth==384 && mainMenu_displayedLines == 216)
      newPresetModeId=41;
    else if (visibleAreaWidth==384 && mainMenu_displayedLines == 240)
      newPresetModeId=42;
    else if (visibleAreaWidth==384 && mainMenu_displayedLines == 256)
      newPresetModeId=43;
    else if (visibleAreaWidth==384 && mainMenu_displayedLines == 262)
      newPresetModeId=44;
    else if (visibleAreaWidth==384 && mainMenu_displayedLines == 270)
      newPresetModeId=45;
    else if (visibleAreaWidth==768 && mainMenu_displayedLines == 200)
      newPresetModeId=50;
    else if (visibleAreaWidth==768 && mainMenu_displayedLines == 216)
      newPresetModeId=51;
    else if (visibleAreaWidth==768 && mainMenu_displayedLines == 240)
      newPresetModeId=52;
    else if (visibleAreaWidth==768 && mainMenu_displayedLines == 256)
      newPresetModeId=53;
    else if (visibleAreaWidth==768 && mainMenu_displayedLines == 262)
      newPresetModeId=54;
    else if (visibleAreaWidth==768 && mainMenu_displayedLines == 270)
      newPresetModeId=55;
  
    if(newPresetModeId >= 0)
      SetPresetMode(newPresetModeId);
  }

  void show_settings_TabDisplaySound()
  {
  	if (visibleAreaWidth==320)
	    radioButton_visibleAreaWidth_320->setSelected(true);
	  else if (visibleAreaWidth==640)
	    radioButton_visibleAreaWidth_640->setSelected(true);
	  else if (visibleAreaWidth==352)
	    radioButton_visibleAreaWidth_352->setSelected(true);
	  else if (visibleAreaWidth==704)
	    radioButton_visibleAreaWidth_704->setSelected(true);
	  else if (visibleAreaWidth==384)
	    radioButton_visibleAreaWidth_384->setSelected(true);
	  else if (visibleAreaWidth==768)
	    radioButton_visibleAreaWidth_768->setSelected(true);

	  if (mainMenu_displayedLines==200)
	    radioButton_displayedLines_200->setSelected(true);
	  else if (mainMenu_displayedLines==216)
	    radioButton_displayedLines_216->setSelected(true);
	  else if (mainMenu_displayedLines==240)
	    radioButton_displayedLines_240->setSelected(true);
	  else if (mainMenu_displayedLines==256)
	    radioButton_displayedLines_256->setSelected(true);
	  else if (mainMenu_displayedLines==262)
	    radioButton_displayedLines_262->setSelected(true);
	  else if (mainMenu_displayedLines==270)
	    radioButton_displayedLines_270->setSelected(true);

  	if (mainMenu_frameskip==0)
	    radioButton_frameskip_0->setSelected(true);
	  else if (mainMenu_frameskip==1)
	    radioButton_frameskip_1->setSelected(true);

    if(dropDown_vertical_pos->getSelected() != moveY+42)
      dropDown_vertical_pos->setSelected(moveY+42);
    if(dropDown_cut_left->getSelected() != mainMenu_cutLeft)
      dropDown_cut_left->setSelected(mainMenu_cutLeft);
    if(dropDown_cut_right->getSelected() != mainMenu_cutRight)
      dropDown_cut_right->setSelected(mainMenu_cutRight);

  	if (!mainMenu_ntsc)
	    radioButton_refreshrate_50Hz->setSelected(true);
	  else if (mainMenu_ntsc)
	    radioButton_refreshrate_60Hz->setSelected(true);

  	if (mainMenu_sound==0)
	    radioButton_sound_off->setSelected(true);
	  else if (mainMenu_sound==1)
	    radioButton_sound_fast->setSelected(true);
	  else if (mainMenu_sound==2)
      radioButton_sound_accurate->setSelected(true);

  	if (sound_rate==8000)
	    radioButton_soundrate_8k->setSelected(true);
	  else if (sound_rate==11025)
	    radioButton_soundrate_11k->setSelected(true);
	  else if (sound_rate==22050)
	    radioButton_soundrate_22k->setSelected(true);
	  else if (sound_rate==32000)
	    radioButton_soundrate_32k->setSelected(true);
	  else if (sound_rate==44100)
	    radioButton_soundrate_44k->setSelected(true);

  	if (mainMenu_soundStereo==0)
	    radioButton_soundmode_mono->setSelected(true);
	  else if (mainMenu_soundStereo==1)
	    radioButton_soundmode_stereo->setSelected(true);
  }

}
