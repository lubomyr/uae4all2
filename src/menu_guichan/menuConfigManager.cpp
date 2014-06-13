#include <algorithm>
#ifdef ANDROIDSDL
#include <android/log.h>
#include <SDL_screenkeyboard.h>
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

#include "menu.h"
#include "menu_config.h"
#include "gui.h"
#include "cfgfile.h"

#define extterms files[q].size()>=4 && files[q].substr(files[q].size()-4)
#define configterms extterms!="conf" && extterms!="CONF" && extterms!="Conf"
#define ext2terms configstring.size()>=5 && configstring.substr(configstring.size()-5)
#define config2terms ext2terms!=".conf" && ext2terms!=".CONF" && ext2terms!=".Conf"

static char config_filename_default[255]={
	'/', 't', 'm', 'p', '/', 'n', 'u', 'l', 'l', '.', 'c', 'o', 'n', 'f', '\0'
};
char *config_filename=(char *)&config_filename_default[0];

extern char launchDir[300];


static void BuildBaseDir(char *filename)
{
  strcpy(filename, "");
  strcat(filename, launchDir);
  strcat(filename, "/customconf");
}


namespace widgets
{
  void show_settings(void);
  static void unraise_config_guichan();
  void showWarning(const char *msg, const char *msg2 = "");
  void showInfo(const char *msg, const char *msg2 = "");

  extern gcn::Color baseCol;
  extern gcn::Widget* activateAfterClose;
  gcn::Window *window_config;
  gcn::Button* button_cfg_load;
  gcn::Button* button_cfg_save;
  gcn::Button* button_cfg_delete;
  gcn::Button* button_cfg_cancel;
  gcn::TextField* textField_config;
#ifdef ANDROIDSDL
  gcn::Button* button_vkeybd;
#endif

  gcn::ListBox* configlistBox;
  gcn::ScrollArea* configScrollArea;


  class ConfigListModel : public gcn::ListModel
  {
    std::vector<std::string> files;

    public:
      ConfigListModel(const char * path)
      {
        changeDir(path);
      }
      
      int getNumberOfElements()
      {
        return files.size();
      }
      
      std::string getElementAt(int i)
      {
        if(i >= files.size() || i < 0)
          return "---";
        return files[i];
      }
      
      void changeDir(const char * path)
      {
        files.clear();
        DIR *dir;
        struct dirent *dent;
        dir = opendir(path);
        if(dir != NULL)
        {
          while((dent=readdir(dir))!=NULL)
          {
            if(!(dent->d_type == DT_DIR))
              files.push_back(dent->d_name);
          }
          closedir(dir);
        }
        std::sort(files.begin(), files.end());
#ifdef ANDROIDSDL
	    if (menuLoad_extfilter==1)
#endif	    
  	    for (int q=0; q<files.size(); q++)
	      {
	        if (configterms)
	        {
		        files.erase(files.begin()+q);
		        q--;
	        }
	      }
      }
  };
  ConfigListModel configList(".");


  class CfgLoadButtonActionListener : public gcn::ActionListener
  {
    public:
      void action(const gcn::ActionEvent& actionEvent)
      {
        int selected_item;
        selected_item = configlistBox->getSelected();
        BuildBaseDir(config_filename);
        strcat(config_filename, "/");
        strcat(config_filename, configList.getElementAt(selected_item).c_str());
        loadconfig(3);
        unraise_config_guichan();
        showInfo("Config file loaded.");
      }
  };
  CfgLoadButtonActionListener* cfgloadButtonActionListener;
  
  
  class CfgSaveButtonActionListener : public gcn::ActionListener
  {
    public:
      void action(const gcn::ActionEvent& actionEvent)
      {
    	  char filename[256]="";
    	  std::string configstring;

        BuildBaseDir(config_filename);
    	  strcat(config_filename, "/");
    	  configstring = textField_config->getText().c_str();
    	  strcat(config_filename, textField_config->getText().c_str());
    	  // check extension of editable name
    	  if (config2terms || configstring.size()<5)
          strcat(config_filename, ".conf");
//__android_log_print(ANDROID_LOG_INFO, "UAE4ALL2","cfg filename = %s", filename);
    	  saveconfig(3);
    	  unraise_config_guichan();
    	  showInfo("Config file saved.");
      }
  };
  CfgSaveButtonActionListener* cfgsaveButtonActionListener;
  
  
  class CfgDeleteButtonActionListener : public gcn::ActionListener
  {
    public:
      void action(const gcn::ActionEvent& actionEvent)
      {
        int selected_item;
        selected_item = configlistBox->getSelected();
        BuildBaseDir(config_filename);
        strcat(config_filename, "/");
        strcat(config_filename, configList.getElementAt(selected_item).c_str());
    	  if(unlink(config_filename))
    	  {
  	      unraise_config_guichan();
  	      showWarning("Failed to delete config.");
    	  }
    	  else
  	    {
          BuildBaseDir(config_filename);
    	    configList = config_filename;
        }
      }
  };
  CfgDeleteButtonActionListener* cfgdeleteButtonActionListener;

  
  class CfgCancelButtonActionListener : public gcn::ActionListener
  {
    public:
      void action(const gcn::ActionEvent& actionEvent)
      {
	      unraise_config_guichan();
      }
  };
  CfgCancelButtonActionListener* cfgcancelButtonActionListener;


#ifdef ANDROIDSDL
  class VkeybdButtonActionListener : public gcn::ActionListener
  {
    public:
      void action(const gcn::ActionEvent& actionEvent)
      {
      	SDL_ANDROID_ToggleScreenKeyboardTextInput("old text");
      }
  };
  VkeybdButtonActionListener* vkeybdButtonActionListener;
#endif

  
  class TextFieldconfigActionListener : public gcn::ActionListener
  {
    public:
      void action(const gcn::ActionEvent& actionEvent)
      {
#ifdef ANDROIDSDL
	SDL_ANDROID_ToggleScreenKeyboardTextInput("old text");
#endif
      }
  };
  TextFieldconfigActionListener* textFieldconfigActionListener;


  class ConfiglistBoxActionListener : public gcn::ActionListener
  {
    public:
      void action(const gcn::ActionEvent& actionEvent)
      {
    	  int selected_item;
    	  selected_item = configlistBox->getSelected();
    	  textField_config->setText(configList.getElementAt(selected_item).c_str());
      }
  };
  ConfiglistBoxActionListener* configlistBoxActionListener;


  void confMan_Init()
  {
    activateAfterClose = NULL;
    button_cfg_load = new gcn::Button("Load");
    button_cfg_load->setId("cfgLoad");
    button_cfg_load->setPosition(316,10);
    button_cfg_load->setSize(70, 26);
    button_cfg_load->setBaseColor(baseCol);
    cfgloadButtonActionListener = new CfgLoadButtonActionListener();
    button_cfg_load->addActionListener(cfgloadButtonActionListener);
    button_cfg_save = new gcn::Button("Save");
    button_cfg_save->setId("cfgSave");
#ifdef ANDROIDSDL
    button_cfg_save->setPosition(316,65);
#else
    button_cfg_save->setPosition(316,55);
#endif
    button_cfg_save->setSize(70, 26);
    button_cfg_save->setBaseColor(baseCol);
    cfgsaveButtonActionListener = new CfgSaveButtonActionListener();
    button_cfg_save->addActionListener(cfgsaveButtonActionListener);
    button_cfg_delete = new gcn::Button("Delete");
    button_cfg_delete->setId("cfgDelete");
#ifdef ANDROIDSDL
    button_cfg_delete->setPosition(316,120);
#else
    button_cfg_delete->setPosition(316,100);
#endif
    button_cfg_delete->setSize(70, 26);
    button_cfg_delete->setBaseColor(baseCol);
    cfgdeleteButtonActionListener = new CfgDeleteButtonActionListener();
    button_cfg_delete->addActionListener(cfgdeleteButtonActionListener);
    button_cfg_cancel = new gcn::Button("Cancel");
    button_cfg_cancel->setId("cfgCancel");
#ifdef ANDROIDSDL
    button_cfg_cancel->setPosition(316,175);
#else
    button_cfg_cancel->setPosition(316,145);
#endif
    button_cfg_cancel->setSize(70, 26);
    button_cfg_cancel->setBaseColor(baseCol);
    cfgcancelButtonActionListener = new CfgCancelButtonActionListener();
    button_cfg_cancel->addActionListener(cfgcancelButtonActionListener);
#ifdef ANDROIDSDL
    button_vkeybd = new gcn::Button("VKeybd");
    button_vkeybd->setId("cfgVKeybd");
    button_vkeybd->setPosition(316,230);
    button_vkeybd->setSize(70, 26);
    button_vkeybd->setBaseColor(baseCol);
    vkeybdButtonActionListener = new VkeybdButtonActionListener();
    button_vkeybd->addActionListener(vkeybdButtonActionListener);
#endif
    textField_config = new gcn::TextField("");
    textField_config->setId("cfgText");
  	textField_config->setPosition(10,10);
  	textField_config->setSize(295,22);
    textField_config->setBaseColor(baseCol);
    textFieldconfigActionListener = new TextFieldconfigActionListener();
    textField_config->addActionListener(textFieldconfigActionListener);
    
    configlistBox = new gcn::ListBox(&configList);
    configlistBox->setId("configList");
    configlistBox->setSize(650,150);
    configlistBox->setBaseColor(baseCol);
    configlistBox->setWrappingEnabled(true);
    configlistBoxActionListener = new ConfiglistBoxActionListener();
    configlistBox->addActionListener(configlistBoxActionListener);
    configScrollArea = new gcn::ScrollArea(configlistBox);
    configScrollArea->setFrameSize(1);
    configScrollArea->setPosition(10,40);
    configScrollArea->setSize(295,228);
    configScrollArea->setScrollbarWidth(20);
    configScrollArea->setBaseColor(baseCol);
    
    window_config = new gcn::Window("Config Manager");
    window_config->setSize(400,300);
    window_config->setBaseColor(baseCol);
    window_config->add(button_cfg_load);
    window_config->add(button_cfg_save);
    window_config->add(button_cfg_delete);
    window_config->add(button_cfg_cancel);
#ifdef ANDROIDSDL
    window_config->add(button_vkeybd);
#endif
    window_config->add(textField_config);
    window_config->add(configScrollArea);
    window_config->setVisible(false);
  }


  void confMan_Exit()
  {
    delete configlistBox;
    delete configScrollArea;

    delete button_cfg_load;
    delete button_cfg_save;
    delete button_cfg_delete;
    delete button_cfg_cancel;
    delete textField_config;
#ifdef ANDROIDSDL
    delete button_vkeybd;
#endif
  	
    delete cfgloadButtonActionListener;
    delete cfgsaveButtonActionListener;
    delete cfgdeleteButtonActionListener;
    delete cfgcancelButtonActionListener;
#ifdef ANDROIDSDL
    delete vkeybdButtonActionListener;
#endif
    delete textFieldconfigActionListener;
    delete configlistBoxActionListener;
    	
    delete window_config;
  }
  
  
  static void unraise_config_guichan()
  {
    window_config->releaseModalFocus();
    window_config->setVisible(false);
    if(activateAfterClose != NULL)
      activateAfterClose->requestFocus();
    activateAfterClose = NULL;  
    show_settings();
  }


  static void raise_config_guichan()
  {
    window_config->setVisible(true);
    window_config->requestModalFocus();
    configlistBox->requestFocus();
  }


  void run_config_guichan()
  {
    BuildBaseDir(config_filename);
    configList = config_filename;
    raise_config_guichan();
  } 
  
}
