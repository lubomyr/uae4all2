#include <algorithm>
#ifdef ANDROIDSDL
#include <android/log.h>
#endif
#include <guichan.hpp>
#include <iostream>
#include <sstream>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_image.h>
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
#include "savestate.h"


extern void setCpuSpeed(void);


extern int emulating;
extern char *screenshot_filename;
extern int mainMenu_case;
extern int savestate_state;


namespace widgets
{
void check_savestate_screen(void);
void show_error(const char *msg, const char *msg2 = "");
void showWarning(const char *msg, const char *msg2 = "");

extern gcn::Color baseCol;
extern gcn::Container* top;
extern gcn::TabbedArea* tabbedArea;
extern gcn::Icon* icon_winlogo;


// Tab Savestates
gcn::Container *tab_savestates;
gcn::Window *group_savestates_number;
gcn::UaeRadioButton* radioButton_savestate_1;
gcn::UaeRadioButton* radioButton_savestate_2;
gcn::UaeRadioButton* radioButton_savestate_3;
gcn::UaeRadioButton* radioButton_savestate_4;
gcn::Window *window_savestate_screen;
gcn::Icon* icon_savestate = 0;
gcn::Image *image_savestate = 0;
gcn::Button* button_load_savestate;
gcn::Button* button_save_savestate;


class SavestateSelActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) {
        if (actionEvent.getSource() == radioButton_savestate_1)
            saveMenu_n_savestate=0;
        else if (actionEvent.getSource() == radioButton_savestate_2)
            saveMenu_n_savestate=1;
        else if (actionEvent.getSource() == radioButton_savestate_3)
            saveMenu_n_savestate=2;
        else if (actionEvent.getSource() == radioButton_savestate_4)
            saveMenu_n_savestate=3;
        check_savestate_screen();
    }
};
SavestateSelActionListener* savestateselActionListener;


class LoadStateActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) {
        if(emulating) {
            mainMenu_case = MAIN_MENU_CASE_SAVESTATES;
            strcpy(savestate_filename,uae4all_image_file0);
            switch(saveMenu_n_savestate) {
            case 1:
                strcat(savestate_filename,"-1.asf");
                break;
            case 2:
                strcat(savestate_filename,"-2.asf");
                break;
            case 3:
                strcat(savestate_filename,"-3.asf");
                break;
            default:
                strcat(savestate_filename,".asf");
            }
            FILE *f = fopen(savestate_filename,"rb");
            if (f) {
                fclose(f);
                savestate_state = STATE_DORESTORE;
                mainMenu_case = MAIN_MENU_CASE_SAVESTATES;
                running = false;
            } else {
                show_error("File doesn't exist.");
            }
        } else
            showWarning("Emulation hasn't started yet.");
    }
};
LoadStateActionListener* loadStateActionListener;


class SaveStateActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) {
        if(emulating) {
            savestate_state = STATE_DOSAVE;
            mainMenu_case = MAIN_MENU_CASE_SAVESTATES;
            running = false;
        } else
            showWarning("Emulation hasn't started yet.");
    }
};
SaveStateActionListener* saveStateActionListener;


void menuTabSavestates_Init()
{
    // Select savestate number
    radioButton_savestate_1 = new gcn::UaeRadioButton("0", "radiosavestategroup");
    radioButton_savestate_1->setPosition(5,10);
    radioButton_savestate_1->setId("Savestate0");
    savestateselActionListener = new SavestateSelActionListener();
    radioButton_savestate_1->addActionListener(savestateselActionListener);
    radioButton_savestate_2 = new gcn::UaeRadioButton("1", "radiosavestategroup");
    radioButton_savestate_2->setPosition(5,40);
    radioButton_savestate_2->setId("Savestate1");
    radioButton_savestate_2->addActionListener(savestateselActionListener);
    radioButton_savestate_3 = new gcn::UaeRadioButton("2", "radiosavestategroup");
    radioButton_savestate_3->setPosition(5,70);
    radioButton_savestate_3->setId("Savestate2");
    radioButton_savestate_3->addActionListener(savestateselActionListener);
    radioButton_savestate_4 = new gcn::UaeRadioButton("3", "radiosavestategroup");
    radioButton_savestate_4->setPosition(5,100);
    radioButton_savestate_4->setId("Savestate3");
    radioButton_savestate_4->addActionListener(savestateselActionListener);
    group_savestates_number = new gcn::Window("Number");
    group_savestates_number->setPosition(10,20);
    group_savestates_number->add(radioButton_savestate_1);
    group_savestates_number->add(radioButton_savestate_2);
    group_savestates_number->add(radioButton_savestate_3);
    group_savestates_number->add(radioButton_savestate_4);
    group_savestates_number->setMovable(false);
    group_savestates_number->setSize(60,145);
    group_savestates_number->setBaseColor(baseCol);

    window_savestate_screen = new gcn::Window("State screen");
    window_savestate_screen->setPosition(120,20);
    window_savestate_screen->setMovable(false);
    window_savestate_screen->setSize(320,240);
    window_savestate_screen->setBaseColor(baseCol);

    button_load_savestate = new gcn::Button("Load State");
    button_load_savestate->setPosition(10,180);
    button_load_savestate->setSize(85, 26);
    button_load_savestate->setBaseColor(baseCol);
    button_load_savestate->setId("LoadState");
    loadStateActionListener = new LoadStateActionListener();
    button_load_savestate->addActionListener(loadStateActionListener);

    button_save_savestate = new gcn::Button("Save State");
    button_save_savestate->setPosition(10,220);
    button_save_savestate->setSize(85, 26);
    button_save_savestate->setBaseColor(baseCol);
    button_save_savestate->setId("SaveState");
    saveStateActionListener = new SaveStateActionListener();
    button_save_savestate->addActionListener(saveStateActionListener);

    tab_savestates = new gcn::Container();
    tab_savestates->add(icon_winlogo);
    tab_savestates->add(group_savestates_number);
    tab_savestates->add(window_savestate_screen);
    tab_savestates->add(button_load_savestate);
    tab_savestates->add(button_save_savestate);
    tab_savestates->setSize(600,280);
    tab_savestates->setBaseColor(baseCol);
}


void menuTabSavestates_Exit()
{
    delete tab_savestates;
    delete group_savestates_number;
    delete radioButton_savestate_1;
    delete radioButton_savestate_2;
    delete radioButton_savestate_3;
    delete radioButton_savestate_4;
    delete window_savestate_screen;
    if(image_savestate != 0)
        delete image_savestate;
    image_savestate = 0;
    if(icon_savestate != 0)
        delete icon_savestate;
    icon_savestate = 0;
    delete button_load_savestate;
    delete button_save_savestate;

    delete savestateselActionListener;
    delete loadStateActionListener;
    delete saveStateActionListener;
}


void check_savestate_screen()
{
    if(icon_savestate != 0) {
        window_savestate_screen->remove(icon_savestate);
        delete icon_savestate;
        icon_savestate = 0;
    }
    if(image_savestate != 0) {
        delete image_savestate;
        image_savestate = 0;
    }

    strcpy(screenshot_filename, uae4all_image_file0);
    switch(saveMenu_n_savestate) {
    case 1:
        strcat(screenshot_filename,"-1.png");
        break;
    case 2:
        strcat(screenshot_filename,"-2.png");
        break;
    case 3:
        strcat(screenshot_filename,"-3.png");
        break;
    default:
        strcat(screenshot_filename,".png");
    }
    FILE *f=fopen(screenshot_filename,"rb");
    if (f) {
        fclose(f);
        gcn::Rectangle rect = window_savestate_screen->getChildrenArea();
        SDL_Surface *loadedImage = IMG_Load(screenshot_filename);
        if(loadedImage != NULL) {
            SDL_Rect source = {0, 0, 0, 0 };
            SDL_Rect target = {0, 0, 0, 0 };
            SDL_Surface *scaled = SDL_CreateRGBSurface(loadedImage->flags, rect.width, rect.height, loadedImage->format->BitsPerPixel, loadedImage->format->Rmask, loadedImage->format->Gmask, loadedImage->format->Bmask, loadedImage->format->Amask);
            source.w = loadedImage->w;
            source.h = loadedImage->h;
            target.w = rect.width;
            target.h = rect.height;
            SDL_SoftStretch(loadedImage, &source, scaled, &target);
            SDL_FreeSurface(loadedImage);
            image_savestate = new gcn::SDLImage(scaled, true);
            icon_savestate = new gcn::Icon(image_savestate);
            window_savestate_screen->add(icon_savestate);
        }
    }
}

void show_settings_TabSavestates()
{
    if (saveMenu_n_savestate==0)
        radioButton_savestate_1->setSelected(true);
    else if (saveMenu_n_savestate==1)
        radioButton_savestate_2->setSelected(true);
    else if (saveMenu_n_savestate==2)
        radioButton_savestate_3->setSelected(true);
    else if (saveMenu_n_savestate==3)
        radioButton_savestate_4->setSelected(true);
    check_savestate_screen();
}

}
