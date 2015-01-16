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
#include "menu.h"
#include "menu_config.h"
#include "options.h"
#include "gui.h"


extern void extractFileName(char * str,char *buffer);

extern char currentDir[300];


namespace widgets
{
void show_settings_TabFloppy(void);
void run_menuLoad_guichan(char *curr_path, int aLoadType);
void showInfo(const char *msg, const char *msg2 = "");

extern gcn::Color baseCol;
extern gcn::Container* top;
extern gcn::TabbedArea* tabbedArea;
extern gcn::Icon* icon_winlogo;
extern gcn::Widget* activateAfterClose;

gcn::Container *tab_hard;
gcn::Window *group_boothd;
gcn::UaeRadioButton* radioButton_boothd_off;
gcn::UaeRadioButton* radioButton_boothd_hddir;
gcn::UaeRadioButton* radioButton_boothd_hdfile;
gcn::Button* button_hddir;
gcn::TextField* textField_hddir;
gcn::Button* button_hdfile;
gcn::TextField* textField_hdfile;
gcn::Button* button_save_hd;


class BootHDButtonActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) {
        if (actionEvent.getSource() == radioButton_boothd_off)
            mainMenu_bootHD=0;
        else if (actionEvent.getSource() == radioButton_boothd_hddir)
            mainMenu_bootHD=1;
        else if (actionEvent.getSource() == radioButton_boothd_hdfile)
            mainMenu_bootHD=2;
    }
};
BootHDButtonActionListener* bootHDButtonActionListener;


class HDDirButtonActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) {
        activateAfterClose = button_hddir;
        run_menuLoad_guichan(currentDir, MENU_LOAD_HD_DIR);
    }
};
HDDirButtonActionListener* hdDirButtonActionListener;


class HDFileButtonActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) {
        activateAfterClose = button_hdfile;
        run_menuLoad_guichan(currentDir, MENU_LOAD_HDF);
    }
};
HDFileButtonActionListener* hdFileButtonActionListener;


class SaveHDButtonActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) {
        if (saveconfig(2))
            showInfo("Config saved for current HD");
    }
};
SaveHDButtonActionListener* saveHDButtonActionListener;


void menuTabHD_Init()
{
    // Select HD boot option
    radioButton_boothd_off = new gcn::UaeRadioButton("Off", "radioboothdgroup");
    radioButton_boothd_off->setPosition(5,10);
    radioButton_boothd_off->setId("HDOff");
    bootHDButtonActionListener = new BootHDButtonActionListener();
    radioButton_boothd_off->addActionListener(bootHDButtonActionListener);
    radioButton_boothd_hddir = new gcn::UaeRadioButton("Dir", "radioboothdgroup");
    radioButton_boothd_hddir->setPosition(5,40);
    radioButton_boothd_hddir->setId("Dir");
    radioButton_boothd_hddir->addActionListener(bootHDButtonActionListener);
    radioButton_boothd_hdfile = new gcn::UaeRadioButton("File", "radioboothdgroup");
    radioButton_boothd_hdfile->setPosition(5,70);
    radioButton_boothd_hdfile->setId("File");
    radioButton_boothd_hdfile->addActionListener(bootHDButtonActionListener);
    group_boothd = new gcn::Window("Boot HD");
    group_boothd->setPosition(10,20);
    group_boothd->add(radioButton_boothd_off);
    group_boothd->add(radioButton_boothd_hddir);
    group_boothd->add(radioButton_boothd_hdfile);
    group_boothd->setMovable(false);
    group_boothd->setSize(70,115);
    group_boothd->setBaseColor(baseCol);

    button_hddir = new gcn::Button("HD Dir");
    button_hddir->setPosition(100,40);
    button_hddir->setSize(60,22);
    button_hddir->setBaseColor(baseCol);
    button_hddir->setId("HDDir");
    hdDirButtonActionListener = new HDDirButtonActionListener();
    button_hddir->addActionListener(hdDirButtonActionListener);
    textField_hddir = new gcn::TextField("not selected                    ");
    textField_hddir->setPosition(170,40);
    textField_hddir->setSize(390,22);
    textField_hddir->setEnabled(false);
    textField_hddir->setBaseColor(baseCol);

    button_hdfile = new gcn::Button("HD File");
    button_hdfile->setPosition(100,80);
    button_hdfile->setSize(60,22);
    button_hdfile->setBaseColor(baseCol);
    button_hdfile->setId("HDFile");
    hdFileButtonActionListener = new HDFileButtonActionListener();
    button_hdfile->addActionListener(hdFileButtonActionListener);
    textField_hdfile = new gcn::TextField("not selected                    ");
    textField_hdfile->setPosition(170,80);
    textField_hdfile->setSize(390,22);
    textField_hdfile->setEnabled(false);
    textField_hdfile->setBaseColor(baseCol);

    button_save_hd = new gcn::Button("Save config for current HD");
    button_save_hd->setHeight(30);
    button_save_hd->setPosition(200,200);
    button_save_hd->setBaseColor(baseCol);
    button_save_hd->setId("SaveHD");
    saveHDButtonActionListener = new SaveHDButtonActionListener();
    button_save_hd->addActionListener(saveHDButtonActionListener);

    tab_hard = new gcn::Container();
    tab_hard->add(icon_winlogo);
    tab_hard->add(group_boothd);
    tab_hard->add(button_hddir);
    tab_hard->add(textField_hddir);
    tab_hard->add(button_hdfile);
    tab_hard->add(textField_hdfile);
    tab_hard->add(button_save_hd);
    tab_hard->setSize(600,280);
    tab_hard->setBaseColor(baseCol);
}


void menuTabHD_Exit()
{
    delete tab_hard;
    delete group_boothd;
    delete radioButton_boothd_off;
    delete radioButton_boothd_hddir;
    delete radioButton_boothd_hdfile;
    delete button_hddir;
    delete textField_hddir;
    delete button_hdfile;
    delete textField_hdfile;
    delete button_save_hd;

    delete bootHDButtonActionListener;
    delete hdDirButtonActionListener;
    delete hdFileButtonActionListener;
    delete saveHDButtonActionListener;
}


void show_settings_TabHD()
{
    static char tmpHDDir[256];
    static char tmpHDFile[256];

    if (mainMenu_bootHD==0)
        radioButton_boothd_off->setSelected(true);
    else if (mainMenu_bootHD==1)
        radioButton_boothd_hddir->setSelected(true);
    else if (mainMenu_bootHD==2)
        radioButton_boothd_hdfile->setSelected(true);

    if (strcmp(uae4all_hard_dir, "")==0)
        textField_hddir->setText("not selected");
    else {
        extractFileName(uae4all_hard_dir, tmpHDDir);
        textField_hddir->setText(tmpHDDir);
    }

    if (strcmp(uae4all_hard_file, "")==0)
        textField_hdfile->setText("not selected");
    else {
        extractFileName(uae4all_hard_file, tmpHDFile);
        textField_hdfile->setText(tmpHDFile);
    }
}

}
