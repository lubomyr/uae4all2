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
  gcn::Window *group_chipmem;
  gcn::Window *group_slowmem;
  gcn::Window *group_fastmem;
  gcn::UaeRadioButton* radioButton_chipmem_512K;
  gcn::UaeRadioButton* radioButton_chipmem_1M;
  gcn::UaeRadioButton* radioButton_chipmem_2M;
  gcn::UaeRadioButton* radioButton_chipmem_4M;
  gcn::UaeRadioButton* radioButton_chipmem_8M;
  gcn::UaeRadioButton* radioButton_slowmem_off;
  gcn::UaeRadioButton* radioButton_slowmem_512K;
  gcn::UaeRadioButton* radioButton_slowmem_1M;
  gcn::UaeRadioButton* radioButton_slowmem_15M;
  gcn::UaeRadioButton* radioButton_slowmem_18M;
  gcn::UaeRadioButton* radioButton_fastmem_off;
  gcn::UaeRadioButton* radioButton_fastmem_1M;
  gcn::UaeRadioButton* radioButton_fastmem_2M;
  gcn::UaeRadioButton* radioButton_fastmem_4M;
  gcn::UaeRadioButton* radioButton_fastmem_8M;
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


  class MemoryButtonActionListener : public gcn::ActionListener
  {
    public:
      void action(const gcn::ActionEvent& actionEvent)
      {
  	    if (actionEvent.getSource() == radioButton_chipmem_512K)
      		mainMenu_chipMemory=0;
  	    else if (actionEvent.getSource() == radioButton_chipmem_1M)
      		mainMenu_chipMemory=1;
  	    else if (actionEvent.getSource() == radioButton_chipmem_2M)
      		mainMenu_chipMemory=2;
  	    else if (actionEvent.getSource() == radioButton_chipmem_4M)
	      {
		      mainMenu_chipMemory=3;
		      mainMenu_fastMemory=0;
  	    }
	      else if (actionEvent.getSource() == radioButton_chipmem_8M)
  	    {
	  	    mainMenu_chipMemory=4;
		      mainMenu_fastMemory=0;
  	    }

  	    if (actionEvent.getSource() == radioButton_slowmem_off)
      		mainMenu_slowMemory=0;
  	    else if (actionEvent.getSource() == radioButton_slowmem_512K)
      		mainMenu_slowMemory=1;
  	    else if (actionEvent.getSource() == radioButton_slowmem_1M)
      		mainMenu_slowMemory=2;
  	    else if (actionEvent.getSource() == radioButton_slowmem_15M)
      		mainMenu_slowMemory=3;
  	    else if (actionEvent.getSource() == radioButton_slowmem_18M)
      		mainMenu_slowMemory=4;

  	    if (actionEvent.getSource() == radioButton_fastmem_off)
      		mainMenu_fastMemory=0;
  	    else if (actionEvent.getSource() == radioButton_fastmem_1M)
  	    {
	      	mainMenu_fastMemory=1;
		      if (mainMenu_chipMemory>2)
		        mainMenu_chipMemory=2;
  	    }
	      else if (actionEvent.getSource() == radioButton_fastmem_2M)
	      {
		      mainMenu_fastMemory=2;
		      if (mainMenu_chipMemory>2)
		        mainMenu_chipMemory=2;
  	    }
	      else if (actionEvent.getSource() == radioButton_fastmem_4M)
	      {
		      mainMenu_fastMemory=3;
      		if (mainMenu_chipMemory>2)
		        mainMenu_chipMemory=2;
  	    }
	      else if (actionEvent.getSource() == radioButton_fastmem_8M)
	      {
		      mainMenu_fastMemory=4;
		      if (mainMenu_chipMemory>2)
		        mainMenu_chipMemory=2;
  	    }
    		UpdateMemorySettings();
    		show_settings_TabMain();
      }
  };
  MemoryButtonActionListener* memoryButtonActionListener;


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

    // Select Chip memory
  	radioButton_chipmem_512K = new gcn::UaeRadioButton("512Kb", "radiochipmemgroup");
  	radioButton_chipmem_512K->setPosition(5,10);
  	radioButton_chipmem_512K->setId("512Kb");
  	radioButton_chipmem_1M = new gcn::UaeRadioButton("1Mb", "radiochipmemgroup");
  	radioButton_chipmem_1M->setPosition(5,40);
  	radioButton_chipmem_1M->setId("Chip1Mb");
  	radioButton_chipmem_2M = new gcn::UaeRadioButton("2Mb", "radiochipmemgroup");
  	radioButton_chipmem_2M->setPosition(5,70);
  	radioButton_chipmem_2M->setId("Chip2Mb");
  	radioButton_chipmem_4M = new gcn::UaeRadioButton("4Mb", "radiochipmemgroup");
  	radioButton_chipmem_4M->setPosition(5,100);
  	radioButton_chipmem_4M->setId("Chip4Mb");
  	radioButton_chipmem_8M = new gcn::UaeRadioButton("8Mb", "radiochipmemgroup");
  	radioButton_chipmem_8M->setPosition(5,130);
  	radioButton_chipmem_8M->setId("Chip8Mb");
    memoryButtonActionListener = new MemoryButtonActionListener();
  	radioButton_chipmem_512K->addActionListener(memoryButtonActionListener);
  	radioButton_chipmem_1M->addActionListener(memoryButtonActionListener);
  	radioButton_chipmem_2M->addActionListener(memoryButtonActionListener);
  	radioButton_chipmem_4M->addActionListener(memoryButtonActionListener);
  	radioButton_chipmem_8M->addActionListener(memoryButtonActionListener);
  	group_chipmem = new gcn::Window("Chip");
  	group_chipmem->setPosition(15,15);
  	group_chipmem->add(radioButton_chipmem_512K);
  	group_chipmem->add(radioButton_chipmem_1M);
  	group_chipmem->add(radioButton_chipmem_2M);
  	group_chipmem->add(radioButton_chipmem_4M);
  	group_chipmem->add(radioButton_chipmem_8M);
  	group_chipmem->setSize(75,175);
    group_chipmem->setBaseColor(baseCol);
  	
  	// Select Slow mem
  	radioButton_slowmem_off = new gcn::UaeRadioButton("off", "radioslowmemgroup");
  	radioButton_slowmem_off->setPosition(5,10);
  	radioButton_slowmem_off->setId("SlowMemOff");
  	radioButton_slowmem_512K = new gcn::UaeRadioButton("512Kb", "radioslowmemgroup");
  	radioButton_slowmem_512K->setPosition(5,40);
  	radioButton_slowmem_512K->setId("Slow512Kb");
  	radioButton_slowmem_1M = new gcn::UaeRadioButton("1Mb", "radioslowmemgroup");
  	radioButton_slowmem_1M->setPosition(5,70);
  	radioButton_slowmem_1M->setId("Slow1Mb");
  	radioButton_slowmem_15M = new gcn::UaeRadioButton("1.5Mb", "radioslowmemgroup");
  	radioButton_slowmem_15M->setPosition(5,100);
  	radioButton_slowmem_15M->setId("Slow1.5Mb");
  	radioButton_slowmem_18M = new gcn::UaeRadioButton("1.8Mb", "radioslowmemgroup");
  	radioButton_slowmem_18M->setPosition(5,130);
  	radioButton_slowmem_18M->setId("Slow1.8Mb");
  	radioButton_slowmem_off->addActionListener(memoryButtonActionListener);
  	radioButton_slowmem_512K->addActionListener(memoryButtonActionListener);
  	radioButton_slowmem_1M->addActionListener(memoryButtonActionListener);
  	radioButton_slowmem_15M->addActionListener(memoryButtonActionListener);
  	radioButton_slowmem_18M->addActionListener(memoryButtonActionListener);
  	group_slowmem = new gcn::Window("Slow");
  	group_slowmem->setPosition(105,15);
  	group_slowmem->add(radioButton_slowmem_off);
  	group_slowmem->add(radioButton_slowmem_512K);
  	group_slowmem->add(radioButton_slowmem_1M);
  	group_slowmem->add(radioButton_slowmem_15M);
  	group_slowmem->add(radioButton_slowmem_18M);
  	group_slowmem->setSize(75,175);
    group_slowmem->setBaseColor(baseCol);
  	
  	// Select Fast mem
  	radioButton_fastmem_off = new gcn::UaeRadioButton("off", "radiofastmemgroup");
  	radioButton_fastmem_off->setPosition(5,10);
  	radioButton_fastmem_off->setId("FastMemOff");
  	radioButton_fastmem_1M = new gcn::UaeRadioButton("1Mb", "radiofastmemgroup");
  	radioButton_fastmem_1M->setPosition(5,40);
  	radioButton_fastmem_1M->setId("Fast1Mb");
  	radioButton_fastmem_2M = new gcn::UaeRadioButton("2Mb", "radiofastmemgroup");
  	radioButton_fastmem_2M->setPosition(5,70);
  	radioButton_fastmem_2M->setId("Fast2Mb");
  	radioButton_fastmem_4M = new gcn::UaeRadioButton("4Mb", "radiofastmemgroup");
  	radioButton_fastmem_4M->setPosition(5,100);
  	radioButton_fastmem_4M->setId("Fast4Mb");
  	radioButton_fastmem_8M = new gcn::UaeRadioButton("8Mb", "radiofastmemgroup");
  	radioButton_fastmem_8M->setPosition(5,130);
  	radioButton_fastmem_8M->setId("Fast8Mb");
  	radioButton_fastmem_off->addActionListener(memoryButtonActionListener);
  	radioButton_fastmem_1M->addActionListener(memoryButtonActionListener);
  	radioButton_fastmem_2M->addActionListener(memoryButtonActionListener);
  	radioButton_fastmem_4M->addActionListener(memoryButtonActionListener);
  	radioButton_fastmem_8M->addActionListener(memoryButtonActionListener);
  	group_fastmem = new gcn::Window("Fast");
  	group_fastmem->setPosition(195,15);
  	group_fastmem->add(radioButton_fastmem_off);
  	group_fastmem->add(radioButton_fastmem_1M);
  	group_fastmem->add(radioButton_fastmem_2M);
  	group_fastmem->add(radioButton_fastmem_4M);
  	group_fastmem->add(radioButton_fastmem_8M);
  	group_fastmem->setSize(75,175);
    group_fastmem->setBaseColor(baseCol);

  	window_memory = new gcn::Window("Memory");
  	window_memory->add(icon_winlogo);
  	window_memory->setPosition(300,20);
  	window_memory->add(group_chipmem);
  	window_memory->add(group_slowmem);	
  	window_memory->add(group_fastmem);
  	window_memory->setMovable(false);
  	window_memory->setSize(287,220);
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
  	delete group_chipmem;
  	delete radioButton_chipmem_512K;
  	delete radioButton_chipmem_1M;
  	delete radioButton_chipmem_2M;
  	delete radioButton_chipmem_4M;
  	delete radioButton_chipmem_8M;
  	delete group_slowmem;
  	delete radioButton_slowmem_off;
  	delete radioButton_slowmem_512K;
  	delete radioButton_slowmem_1M;
  	delete radioButton_slowmem_15M;
  	delete radioButton_slowmem_18M;
  	delete group_fastmem;
  	delete radioButton_fastmem_off;
  	delete radioButton_fastmem_1M;
  	delete radioButton_fastmem_2M;
  	delete radioButton_fastmem_4M;
  	delete radioButton_fastmem_8M;
#if defined(PANDORA) && !defined(WIN32)
    delete backgrd_pandspeed;
    delete label_pandspeed;
    delete dropDown_pandspeed;
#endif

    delete cpuButtonActionListener;
    delete chipsetButtonActionListener;
    delete kickstartButtonActionListener;
    delete cpuSpeedButtonActionListener;
    delete memoryButtonActionListener;
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

	  if (mainMenu_chipMemory==0)
      radioButton_chipmem_512K->setSelected(true);
	  else if (mainMenu_chipMemory==1)
      radioButton_chipmem_1M->setSelected(true);
	  else if (mainMenu_chipMemory==2)
      radioButton_chipmem_2M->setSelected(true);
	  else if (mainMenu_chipMemory==3)
      radioButton_chipmem_4M->setSelected(true);
	  else if (mainMenu_chipMemory==4)
      radioButton_chipmem_8M->setSelected(true);

  	if (mainMenu_slowMemory==0)
      radioButton_slowmem_off->setSelected(true);
	  else if (mainMenu_slowMemory==1)
      radioButton_slowmem_512K->setSelected(true);
	  else if (mainMenu_slowMemory==2)
      radioButton_slowmem_1M->setSelected(true);
	  else if (mainMenu_slowMemory==3)
      radioButton_slowmem_15M->setSelected(true);
	  else if (mainMenu_slowMemory==4)
      radioButton_slowmem_18M->setSelected(true);

  	if (mainMenu_fastMemory==0)
      radioButton_fastmem_off->setSelected(true);
	  else if (mainMenu_fastMemory==1)
      radioButton_fastmem_1M->setSelected(true);
	  else if (mainMenu_fastMemory==2)
      radioButton_fastmem_2M->setSelected(true);
	  else if (mainMenu_fastMemory==3)
      radioButton_fastmem_4M->setSelected(true);
	  else if (mainMenu_fastMemory==4)
      radioButton_fastmem_8M->setSelected(true);

#if defined(PANDORA) && !defined(WIN32)
    if(dropDown_pandspeed->getSelected() != (mainMenu_cpuSpeed - 500) / 20)
      dropDown_pandspeed->setSelected((mainMenu_cpuSpeed - 500) / 20);
#endif
  }
  
}
