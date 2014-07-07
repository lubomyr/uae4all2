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
#include "sound.h"
#include "zfile.h"
#include "gui.h"
#include "disk.h"


extern int kickstart;


namespace widgets
{
  void show_settings_TabMain(void);

  extern gcn::Color baseCol;
  extern gcn::Color baseColLabel;
  extern gcn::Container* top;
  extern gcn::TabbedArea* tabbedArea;
  extern gcn::Icon* icon_winlogo;
  
#ifdef ANDROIDSDL
  extern gcn::contrib::SDLTrueTypeFont* font2;
#endif

  // Tab Main
  gcn::Container *tab_main;
  gcn::Window *group_cpu;
  gcn::UaeRadioButton* radioButton_cpu68000;
  gcn::UaeRadioButton* radioButton_cpu68020;
  gcn::Window *group_chipset;
  gcn::UaeRadioButton* radioButton_chipsetocs;
  gcn::UaeRadioButton* radioButton_chipsetecs;
  gcn::UaeRadioButton* radioButton_chipsetaga;
  gcn::Window *group_kickstart;
  gcn::UaeRadioButton* radioButton_kick12;
  gcn::UaeRadioButton* radioButton_kick13;
  gcn::UaeRadioButton* radioButton_kick20;
  gcn::UaeRadioButton* radioButton_kick31;
  gcn::UaeRadioButton* radioButton_aros;
  gcn::Window *group_cpuspeed;
  gcn::UaeRadioButton* radioButton_cpuspeed_7Mhz;
  gcn::UaeRadioButton* radioButton_cpuspeed_14Mhz;
  gcn::UaeRadioButton* radioButton_cpuspeed_28Mhz;
#if defined(WIN32) || defined(ANDROIDSDL)
  gcn::UaeRadioButton* radioButton_cpuspeed_56Mhz;
  gcn::UaeRadioButton* radioButton_cpuspeed_112Mhz; 
#endif
  gcn::Window *window_memory;
  gcn::Label* label_chipmem;
  gcn::Label* label_slowmem;
  gcn::Label* label_fastmem;
  gcn::Container* backgrd_chipmem;
  gcn::Container* backgrd_slowmem;
  gcn::Container* backgrd_fastmem;
  gcn::Label* label_chipsize;
  gcn::Label* label_slowsize;
  gcn::Label* label_fastsize;
  gcn::Container* backgrd_chipsize;
  gcn::Container* backgrd_slowsize;
  gcn::Container* backgrd_fastsize;  
  gcn::Slider* slider_chipmem;
  gcn::Slider* slider_slowmem;
  gcn::Slider* slider_fastmem;  

#ifdef PANDORA
  gcn::Container* backgrd_pandspeed;
  gcn::Label* label_pandspeed;
  gcn::UaeDropDown* dropDown_pandspeed;
#endif


#ifdef PANDORA
  class PandSpeedListModel : public gcn::ListModel
  {
    private:
      std::ostringstream ostr[38];

    public:
      PandSpeedListModel()
      {
	      for (int j=500,i=0; j<1260; j+=20, i++)
	        ostr[i] << j;
      }
      
      int getNumberOfElements()
      {
        return 38;
      }

      std::string getElementAt(int i)
      {
        return ostr[i].str().c_str();
      }
  };
  PandSpeedListModel pandSpeedList;  
#endif


  class CPUButtonActionListener : public gcn::ActionListener
  {
    public:
      void action(const gcn::ActionEvent& actionEvent)
      {
  	    if (actionEvent.getSource() == radioButton_cpu68000)
    		  mainMenu_CPU_model = 0;
	      else
		      mainMenu_CPU_model = 1;
		    UpdateCPUModelSettings();
      }
  };
  CPUButtonActionListener* cpuButtonActionListener;


  class ChipsetButtonActionListener : public gcn::ActionListener
  {
    public:
      void action(const gcn::ActionEvent& actionEvent)
      {
  	    if (actionEvent.getSource() == radioButton_chipsetocs)
      		mainMenu_chipset=0;
  	    else if (actionEvent.getSource() == radioButton_chipsetecs)
      		mainMenu_chipset=1;
  	    else
      		mainMenu_chipset=2;
    		UpdateChipsetSettings();
      }
  };
  ChipsetButtonActionListener* chipsetButtonActionListener;


  class KickstartButtonActionListener : public gcn::ActionListener
  {
    public:
      void action(const gcn::ActionEvent& actionEvent)
      {
  	    if (actionEvent.getSource() == radioButton_kick12)
  		    kickstart=0;
  	    else if (actionEvent.getSource() == radioButton_kick13)
  		    kickstart=1;
  	    else if (actionEvent.getSource() == radioButton_kick20)
  		    kickstart=2;
  	    else if (actionEvent.getSource() == radioButton_kick31)
  		    kickstart=3;
  	    else if (actionEvent.getSource() == radioButton_aros)
  		    kickstart=4;
      }
  };
  KickstartButtonActionListener* kickstartButtonActionListener;


  class CPUSpeedButtonActionListener : public gcn::ActionListener
  {
    public:
      void action(const gcn::ActionEvent& actionEvent)
      {
  	    if (actionEvent.getSource() == radioButton_cpuspeed_7Mhz)
	      	mainMenu_CPU_speed=0;
	      else if (actionEvent.getSource() == radioButton_cpuspeed_14Mhz)
		      mainMenu_CPU_speed=1;
	      else if (actionEvent.getSource() == radioButton_cpuspeed_28Mhz)
		      mainMenu_CPU_speed=2;
#if defined(WIN32) || defined(ANDROIDSDL)
	      else if (actionEvent.getSource() == radioButton_cpuspeed_56Mhz)
		      mainMenu_CPU_speed=3;
	      else if (actionEvent.getSource() == radioButton_cpuspeed_112Mhz)
		      mainMenu_CPU_speed=4;
#endif
      }
  };
  CPUSpeedButtonActionListener* cpuSpeedButtonActionListener;

  class MemorySliderActionListener : public gcn::ActionListener
  {
    public:
      void action(const gcn::ActionEvent& actionEvent)
      {
 	    if (actionEvent.getSource() == slider_chipmem) {
    		mainMenu_chipMemory=(int)(slider_chipmem->getValue());
			}
 	    if (actionEvent.getSource() == slider_slowmem)
      		mainMenu_slowMemory=(int)(slider_slowmem->getValue());
  	    if (actionEvent.getSource() == slider_fastmem) {
		      if (mainMenu_chipMemory>2)
		        mainMenu_chipMemory=2;
     		mainMenu_fastMemory=(int)(slider_fastmem->getValue());
		}	
    		UpdateMemorySettings();
    		show_settings_TabMain();
      }
  };
  MemorySliderActionListener* memorySliderActionListener;

#if defined(PANDORA) && !defined(WIN32)
  class PandSpeedActionListener : public gcn::ActionListener
  {
    public:
      void action(const gcn::ActionEvent& actionEvent)
      {
	      mainMenu_cpuSpeed = dropDown_pandspeed->getSelected() * 20 + 500;
      }
  };
  PandSpeedActionListener* pandSpeedActionListener;
#endif


  void menuTabMain_Init()
  {
  	// Select CPU
  	radioButton_cpu68000 = new gcn::UaeRadioButton("68000", "radiocpugroup");
	radioButton_cpu68000->setPosition(5,10);
  	radioButton_cpu68000->setId("68000");
	radioButton_cpu68020 = new gcn::UaeRadioButton("68020", "radiocpugroup");
	radioButton_cpu68020->setPosition(5,40);
  	radioButton_cpu68020->setId("68020");
#ifdef ANDROIDSDL
	radioButton_cpu68000->setFont(font2);
	radioButton_cpu68020->setFont(font2);
#endif
	cpuButtonActionListener = new CPUButtonActionListener();
  	radioButton_cpu68000->addActionListener(cpuButtonActionListener);
  	radioButton_cpu68020->addActionListener(cpuButtonActionListener);
  	group_cpu = new gcn::Window("CPU");
  	group_cpu->setPosition(10,20);
  	group_cpu->add(radioButton_cpu68000);
  	group_cpu->add(radioButton_cpu68020);
  	group_cpu->setMovable(false);
  	group_cpu->setSize(80,85);
    group_cpu->setBaseColor(baseCol);

    // Select Chipset
   	radioButton_chipsetocs = new gcn::UaeRadioButton("OCS", "radiochipsetgroup");
  	radioButton_chipsetocs->setPosition(5,10);
  	radioButton_chipsetocs->setId("OCS");
  	radioButton_chipsetecs = new gcn::UaeRadioButton("ECS", "radiochipsetgroup");	      
  	radioButton_chipsetecs->setPosition(5,40);
  	radioButton_chipsetecs->setId("ECS");
  	radioButton_chipsetaga = new gcn::UaeRadioButton("AGA", "radiochipsetgroup");
  	radioButton_chipsetaga->setPosition(5,70);
  	radioButton_chipsetaga->setId("AGA");
 	chipsetButtonActionListener = new ChipsetButtonActionListener();
  	radioButton_chipsetocs->addActionListener(chipsetButtonActionListener);
  	radioButton_chipsetecs->addActionListener(chipsetButtonActionListener);
  	radioButton_chipsetaga->addActionListener(chipsetButtonActionListener);
 	group_chipset = new gcn::Window("Chipset");
   	group_chipset->setPosition(10,120);
  	group_chipset->add(radioButton_chipsetocs);
  	group_chipset->add(radioButton_chipsetecs);
  	group_chipset->add(radioButton_chipsetaga);	
  	group_chipset->setMovable(false);
  	group_chipset->setSize(80,115);
    group_chipset->setBaseColor(baseCol);

    // Select Kickstart
  	radioButton_kick12 = new gcn::UaeRadioButton("1.2", "radiokickgroup");
  	radioButton_kick12->setPosition(5,10);
  	radioButton_kick12->setId("1.2");
  	radioButton_kick13 = new gcn::UaeRadioButton("1.3", "radiokickgroup");
  	radioButton_kick13->setPosition(5,40);
  	radioButton_kick13->setId("1.3");
  	radioButton_kick20 = new gcn::UaeRadioButton("2.0", "radiokickgroup");
  	radioButton_kick20->setPosition(5,70);
  	radioButton_kick20->setId("2.0");
  	radioButton_kick31 = new gcn::UaeRadioButton("3.1", "radiokickgroup");
  	radioButton_kick31->setPosition(5,100);
  	radioButton_kick31->setId("3.1");
  	radioButton_aros = new gcn::UaeRadioButton("Aros", "radiokickgroup");
  	radioButton_aros->setPosition(5,130);
  	radioButton_aros->setId("AROS");
  	kickstartButtonActionListener = new KickstartButtonActionListener();
  	radioButton_kick12->addActionListener(kickstartButtonActionListener);
  	radioButton_kick13->addActionListener(kickstartButtonActionListener);
  	radioButton_kick20->addActionListener(kickstartButtonActionListener);
  	radioButton_kick31->addActionListener(kickstartButtonActionListener);
  	radioButton_aros->addActionListener(kickstartButtonActionListener);
  	group_kickstart = new gcn::Window("Kickstart");
  	group_kickstart->setPosition(105,20);
  	group_kickstart->add(radioButton_kick12);
  	group_kickstart->add(radioButton_kick13);
  	group_kickstart->add(radioButton_kick20);
  	group_kickstart->add(radioButton_kick31);
  	group_kickstart->add(radioButton_aros);
  	group_kickstart->setMovable(false);
  	group_kickstart->setSize(80,175);
    group_kickstart->setBaseColor(baseCol);
  	
    // Select CPU speed
  	radioButton_cpuspeed_7Mhz = new gcn::UaeRadioButton("7MHz", "radiocpuspeedgroup");
  	radioButton_cpuspeed_7Mhz->setPosition(5,10);
  	radioButton_cpuspeed_7Mhz->setId("7MHz");
  	radioButton_cpuspeed_14Mhz = new gcn::UaeRadioButton("14MHz", "radiocpuspeedgroup");
  	radioButton_cpuspeed_14Mhz->setPosition(5,40);
  	radioButton_cpuspeed_14Mhz->setId("14MHz");
  	radioButton_cpuspeed_28Mhz = new gcn::UaeRadioButton("28MHz", "radiocpuspeedgroup");
  	radioButton_cpuspeed_28Mhz->setPosition(5,70);
  	radioButton_cpuspeed_28Mhz->setId("28MHz");
#if defined(WIN32) || defined(ANDROIDSDL)
  	radioButton_cpuspeed_56Mhz = new gcn::UaeRadioButton("56MHz", "radiocpuspeedgroup");
  	radioButton_cpuspeed_56Mhz->setPosition(5,100);
  	radioButton_cpuspeed_56Mhz->setId("56MHz");
  	radioButton_cpuspeed_112Mhz = new gcn::UaeRadioButton("112MHz", "radiocpuspeedgroup");
  	radioButton_cpuspeed_112Mhz->setPosition(5,130);
  	radioButton_cpuspeed_112Mhz->setId("112MHz");
#endif
#ifdef ANDROIDSDL
	radioButton_cpuspeed_7Mhz->setFont(font2);
	radioButton_cpuspeed_14Mhz->setFont(font2);
	radioButton_cpuspeed_28Mhz->setFont(font2);
	radioButton_cpuspeed_56Mhz->setFont(font2);
	radioButton_cpuspeed_112Mhz->setFont(font2);
#endif
	cpuSpeedButtonActionListener = new CPUSpeedButtonActionListener();
	radioButton_cpuspeed_7Mhz->addActionListener(cpuSpeedButtonActionListener);
	radioButton_cpuspeed_14Mhz->addActionListener(cpuSpeedButtonActionListener);
	radioButton_cpuspeed_28Mhz->addActionListener(cpuSpeedButtonActionListener);
#if defined(WIN32) || defined(ANDROIDSDL)
	radioButton_cpuspeed_56Mhz->addActionListener(cpuSpeedButtonActionListener);
	radioButton_cpuspeed_112Mhz->addActionListener(cpuSpeedButtonActionListener);
#endif
	group_cpuspeed = new gcn::Window("CPU Speed");
	group_cpuspeed->setPosition(200,20);
	group_cpuspeed->add(radioButton_cpuspeed_7Mhz);
	group_cpuspeed->add(radioButton_cpuspeed_14Mhz);
	group_cpuspeed->add(radioButton_cpuspeed_28Mhz);
	group_cpuspeed->setMovable(false);
#if defined(WIN32) || defined(ANDROIDSDL)
  	group_cpuspeed->add(radioButton_cpuspeed_56Mhz);
  	group_cpuspeed->add(radioButton_cpuspeed_112Mhz);
  	group_cpuspeed->setSize(87,175);
#else
  	group_cpuspeed->setSize(85,115);
#endif
	group_cpuspeed->setBaseColor(baseCol);

    // Select Memory
  	label_chipmem = new gcn::Label("Chip");
  	label_chipmem->setPosition(4, 2);
  	label_slowmem = new gcn::Label("Slow");
  	label_slowmem->setPosition(4, 2);
  	label_fastmem = new gcn::Label("Fast");
  	label_fastmem->setPosition(4, 2);
  	backgrd_chipmem = new gcn::Container();
  	backgrd_chipmem->setOpaque(true);
  	backgrd_chipmem->setBaseColor(baseColLabel);
  	backgrd_chipmem->setPosition(15, 20);
  	backgrd_chipmem->setSize(50, 21);
    backgrd_chipmem->add(label_chipmem);
  	backgrd_slowmem = new gcn::Container();
  	backgrd_slowmem->setOpaque(true);
  	backgrd_slowmem->setBaseColor(baseColLabel);
  	backgrd_slowmem->setPosition(15, 60);
  	backgrd_slowmem->setSize(50, 21);
    backgrd_slowmem->add(label_slowmem);
  	backgrd_fastmem = new gcn::Container();
  	backgrd_fastmem->setOpaque(true);
  	backgrd_fastmem->setBaseColor(baseColLabel);
  	backgrd_fastmem->setPosition(15, 100);
  	backgrd_fastmem->setSize(50, 21);
    backgrd_fastmem->add(label_fastmem);
    slider_chipmem = new gcn::Slider(0, 4);
	slider_chipmem->setPosition(85,20);
    slider_chipmem->setSize(110, 21);
	slider_chipmem->setMarkerLength(20);
	slider_chipmem->setStepLength(1);
	slider_chipmem->setId("ChipMem");
	slider_slowmem = new gcn::Slider(0, 4);
	slider_slowmem->setPosition(85,60);
	slider_slowmem->setSize(110, 21);
	slider_slowmem->setMarkerLength(20);
	slider_slowmem->setStepLength(1);
	slider_slowmem->setId("SlowMem");
	slider_fastmem = new gcn::Slider(0, 4);
	slider_fastmem->setPosition(85,100);
	slider_fastmem->setSize(110, 21);
	slider_fastmem->setMarkerLength(20);
	slider_fastmem->setStepLength(1);
	slider_fastmem->setId("FastMem");
    memorySliderActionListener = new MemorySliderActionListener();
	slider_chipmem->addActionListener(memorySliderActionListener);
	slider_slowmem->addActionListener(memorySliderActionListener);
	slider_fastmem->addActionListener(memorySliderActionListener);
  	label_chipsize = new gcn::Label("None  ");
  	label_chipsize->setPosition(4, 2);
  	label_slowsize = new gcn::Label("None  ");
  	label_slowsize->setPosition(4, 2);
  	label_fastsize = new gcn::Label("None  ");
  	label_fastsize->setPosition(4, 2);
   	backgrd_chipsize = new gcn::Container();
  	backgrd_chipsize->setOpaque(true);
  	backgrd_chipsize->setBaseColor(baseColLabel);
  	backgrd_chipsize->setPosition(215, 20);
  	backgrd_chipsize->setSize(50, 21);
    backgrd_chipsize->add(label_chipsize);
  	backgrd_slowsize = new gcn::Container();
  	backgrd_slowsize->setOpaque(true);
  	backgrd_slowsize->setBaseColor(baseColLabel);
  	backgrd_slowsize->setPosition(215, 60);
  	backgrd_slowsize->setSize(50, 21);
    backgrd_slowsize->add(label_slowsize);
  	backgrd_fastsize = new gcn::Container();
  	backgrd_fastsize->setOpaque(true);
  	backgrd_fastsize->setBaseColor(baseColLabel);
  	backgrd_fastsize->setPosition(215, 100);
  	backgrd_fastsize->setSize(50, 21);
    backgrd_fastsize->add(label_fastsize);
		
  	window_memory = new gcn::Window("Memory");
  	window_memory->add(icon_winlogo);
  	window_memory->setPosition(300,20);
  	window_memory->add(backgrd_chipmem);
  	window_memory->add(backgrd_slowmem);	
  	window_memory->add(backgrd_fastmem);
  	window_memory->add(slider_chipmem);
  	window_memory->add(slider_slowmem);	
  	window_memory->add(slider_fastmem);
  	window_memory->add(backgrd_chipsize);
  	window_memory->add(backgrd_slowsize);	
  	window_memory->add(backgrd_fastsize);
  	window_memory->setMovable(false);
  	window_memory->setSize(287,160);
    window_memory->setBaseColor(baseCol);

#if defined(PANDORA) && !defined(WIN32)
    // Pandora CPU speed
  	label_pandspeed = new gcn::Label("Pandora MHz");
  	label_pandspeed->setPosition(4, 2);
  	backgrd_pandspeed = new gcn::Container();
  	backgrd_pandspeed->setOpaque(true);
  	backgrd_pandspeed->setBaseColor(baseColLabel);
  	backgrd_pandspeed->setPosition(105, 210);
  	backgrd_pandspeed->setSize(100, 21);
    backgrd_pandspeed->add(label_pandspeed);
  	dropDown_pandspeed = new gcn::UaeDropDown(&pandSpeedList);
  	dropDown_pandspeed->setPosition(215,210);
  	dropDown_pandspeed->setWidth(70);
    dropDown_pandspeed->setBaseColor(baseCol);
    dropDown_pandspeed->setId("PandSpeed");
    pandSpeedActionListener = new PandSpeedActionListener();
  	dropDown_pandspeed->addActionListener(pandSpeedActionListener);
#endif

  	tab_main = new gcn::Container();
	  tab_main->add(icon_winlogo);
	  tab_main->add(group_cpu);
	  tab_main->add(group_chipset);
	  tab_main->add(group_kickstart);
	  tab_main->add(group_cpuspeed);
	  tab_main->add(window_memory);
#if defined(PANDORA) && !defined(WIN32)
    tab_main->add(backgrd_pandspeed);
  	tab_main->add(dropDown_pandspeed);
#endif
	  tab_main->setSize(600,280);
    tab_main->setBaseColor(baseCol);
  }
  
  
  void menuTabMain_Exit()
  {
    delete tab_main;
    delete group_cpu;
    delete radioButton_cpu68000;
    delete radioButton_cpu68020;
    delete group_chipset;
    delete radioButton_chipsetocs;
    delete radioButton_chipsetecs;        
    delete radioButton_chipsetaga;
    delete group_kickstart;
    delete radioButton_kick12;
    delete radioButton_kick13;
    delete radioButton_kick20;
    delete radioButton_kick31;
    delete radioButton_aros;
    delete group_cpuspeed;
    delete radioButton_cpuspeed_7Mhz;
    delete radioButton_cpuspeed_14Mhz;
    delete radioButton_cpuspeed_28Mhz;
#if defined(WIN32) || defined(ANDROIDSDL)
    delete radioButton_cpuspeed_56Mhz;
    delete radioButton_cpuspeed_112Mhz; 
#endif
  	delete window_memory;
  	delete label_chipmem;
  	delete label_slowmem;
  	delete label_fastmem;
  	delete backgrd_chipmem;
  	delete backgrd_slowmem;
  	delete backgrd_fastmem;
  	delete label_chipsize;
  	delete label_slowsize;
  	delete label_fastsize;
   	delete backgrd_chipsize;
  	delete backgrd_slowsize;
  	delete backgrd_fastsize;  
  	delete slider_chipmem;
  	delete slider_slowmem;
  	delete slider_fastmem;  

#if defined(PANDORA) && !defined(WIN32)
    delete backgrd_pandspeed;
    delete label_pandspeed;
    delete dropDown_pandspeed;
#endif

    delete cpuButtonActionListener;
    delete chipsetButtonActionListener;
    delete kickstartButtonActionListener;
    delete cpuSpeedButtonActionListener;
	delete memorySliderActionListener;
#if defined(PANDORA) && !defined(WIN32)
    delete pandSpeedActionListener;
#endif
  }
  
  
  void show_settings_TabMain()
  {
  	if (mainMenu_CPU_model==0)
	    radioButton_cpu68000->setSelected(true);
	  else if (mainMenu_CPU_model==1)
	    radioButton_cpu68020->setSelected(true);

  	if (mainMenu_chipset==0)
	    radioButton_chipsetocs->setSelected(true);
	  else if (mainMenu_chipset==1)
	    radioButton_chipsetecs->setSelected(true);
	  else if (mainMenu_chipset==2)
	    radioButton_chipsetaga->setSelected(true);

  	if (kickstart==0)
	    radioButton_kick12->setSelected(true);
	  else if (kickstart==1)
	    radioButton_kick13->setSelected(true);
	  else if (kickstart==2)
	    radioButton_kick20->setSelected(true);
	  else if (kickstart==3)
	    radioButton_kick31->setSelected(true);
	  else if (kickstart==4)
	    radioButton_aros->setSelected(true);

  	if (mainMenu_CPU_speed==0)
	    radioButton_cpuspeed_7Mhz->setSelected(true);
	  else if (mainMenu_CPU_speed==1)
	    radioButton_cpuspeed_14Mhz->setSelected(true);
	  else if (mainMenu_CPU_speed==2)
	    radioButton_cpuspeed_28Mhz->setSelected(true);
#if defined(WIN32) || defined(ANDROIDSDL)
	  else if (mainMenu_CPU_speed==3)
	    radioButton_cpuspeed_56Mhz->setSelected(true);
	  else if (mainMenu_CPU_speed==4)
	    radioButton_cpuspeed_112Mhz->setSelected(true);
#endif

	const char *ChipMem_list[] = { "512Kb", "1Mb", "2Mb", "4Mb", "8Mb" };
	const char *SlowMem_list[] = { "None", "512Kb", "1Mb", "1.5Mb", "1.8Mb" };
	const char *FastMem_list[] = { "None", "1Mb", "2Mb", "4mb", "8Mb" };
	slider_chipmem->setValue(mainMenu_chipMemory);
	slider_slowmem->setValue(mainMenu_slowMemory);
	slider_fastmem->setValue(mainMenu_fastMemory);
	label_chipsize->setCaption(ChipMem_list[mainMenu_chipMemory]);
	label_slowsize->setCaption(SlowMem_list[mainMenu_slowMemory]);
	label_fastsize->setCaption(FastMem_list[mainMenu_fastMemory]);
	
#if defined(PANDORA) && !defined(WIN32)
    if(dropDown_pandspeed->getSelected() != (mainMenu_cpuSpeed - 500) / 20)
      dropDown_pandspeed->setSelected((mainMenu_cpuSpeed - 500) / 20);
#endif
  }
  
}
