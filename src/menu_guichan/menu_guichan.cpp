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

#include "menu.h"
#include "menu_config.h"
#include "options.h"
#include "sound.h"
#include "zfile.h"
#include "gui.h"
#include "disk.h"

#include "NavigationMap.h"


#if defined(ANDROID)
#include <SDL_screenkeyboard.h>
#endif

extern int init_sound(void);
extern void leave_program(void);
// From menu_helper.cpp:
extern int saveAdfDir(void);
extern void update_display(void);
extern void setCpuSpeed(void);
#ifdef PANDORA
extern void resetCpuSpeed(void);
#endif
extern void gp2x_stop_sound(void);


extern int kickstart;
extern int oldkickstart;
extern int bReloadKickstart;
extern unsigned int sound_rate;
extern int emulating;
extern int gp2xMouseEmuOn;
extern int gp2xButtonRemappingOn;
extern char launchDir[300];
extern char currentDir[300];
extern int current_drive;


int mainMenu_case = -1;
bool running = false;

int nr_drives = DEFAULT_DRIVES;
int lastCpuSpeed=600;
int ntsc=0;


#ifdef PANDORA
#define GUI_WIDTH  640
#define GUI_HEIGHT 480
#else
#define GUI_WIDTH  640
#define GUI_HEIGHT 480
#endif

enum { DIRECTION_NONE, DIRECTION_UP, DIRECTION_DOWN, DIRECTION_LEFT, DIRECTION_RIGHT };


bool CheckKickstart()
{
    if (kickstart!=5) {
#ifdef ANDROIDSDL
        snprintf(romfile, 256, "%s/../../com.cloanto.amigaforever.essentials/files/rom/%s",launchDir,af_kickstarts_rom_names[kickstart]);
        FILE *f_afs=fopen (romfile, "r" );
        if(f_afs) {
            fclose(f_afs);
            return true;
        }
#endif
        snprintf(romfile, 256, "%s/kickstarts/%s",launchDir,kickstarts_rom_names[kickstart]);
        FILE *f=fopen (romfile, "r" );
        if(f) {
            fclose(f);
            return true;
        }
        return false;
    } else {
        snprintf(romfile, 256, custom_kickrom);
        FILE *f=fopen (romfile, "r" );
        if(f) {
            fclose(f);
            return true;
        }
        return false;
    }
}


namespace globals
{
gcn::Gui* gui;
}

namespace widgets
{
void showWarning(const char *msg, const char *msg2 = "");
bool HandleNavigation(int direction);
extern void run_menuLoad_guichan(char *curr_path, int aLoadType);
extern gcn::Window *window_load;
extern gcn::Window *window_config;
extern gcn::Window *window_warning;
extern gcn::Button* button_df0;
extern gcn::Button* button_df1;
extern gcn::Button* button_df2;
extern gcn::Button* button_df3;
extern gcn::Widget* activateAfterClose;

gcn::TabbedArea* tabbedArea;
}

namespace sdl
{
// Main objects to draw graphics and get user input
SDL_Surface* screen;
gcn::SDLGraphics* graphics;
gcn::SDLInput* input;
gcn::SDLImageLoader* imageLoader;

void init()
{
#ifdef PANDORA
    char layersize[20];
    snprintf(layersize, 20, "%dx%d", GUI_WIDTH, GUI_HEIGHT);

#ifndef WIN32
    setenv("SDL_OMAP_LAYER_SIZE",layersize,1);
#endif
    char bordercut[20];
    snprintf(bordercut, 20, "0,0,0,0");

#ifndef WIN32
    setenv("SDL_OMAP_BORDER_CUT",bordercut,1);
#endif
#endif
    screen = SDL_SetVideoMode(GUI_WIDTH, GUI_HEIGHT, 16, SDL_SWSURFACE);
    SDL_EnableUNICODE(1);
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

#ifdef ANDROIDSDL
    // Enable Android multitouch
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    SDL_JoystickOpen(0);
#endif
#ifdef PANDORA
    SDL_ShowCursor(SDL_ENABLE);
#endif

    imageLoader = new gcn::SDLImageLoader();
    gcn::Image::setImageLoader(imageLoader);

    graphics = new gcn::SDLGraphics();
    graphics->setTarget(screen);

    input = new gcn::SDLInput();

    globals::gui = new gcn::Gui();
    globals::gui->setGraphics(graphics);
    globals::gui->setInput(input);
}


void halt()
{
    delete globals::gui;
    delete imageLoader;
    delete input;
    delete graphics;
}


void run()
{
    // The main loop
    while(running) {
        // Check user input
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                mainMenu_case = MAIN_MENU_CASE_QUIT;
                running = false;
                break;
            } else if (event.type == SDL_KEYDOWN) {
                switch(event.key.keysym.sym) {
#ifdef PANDORA
                case SDLK_q:
                    mainMenu_case = MAIN_MENU_CASE_QUIT;
                    running = false;
                    break;
#endif

                case SDLK_ESCAPE:
                case SDLK_RCTRL:
                    if(widgets::window_load->isVisible() || widgets::window_warning->isVisible() || widgets::window_config->isVisible())
                        break;
                    if(CheckKickstart()) {
                        mainMenu_case = MAIN_MENU_CASE_RESET;
                        running = false;
                    } else
                        widgets::showWarning("Kickstart ROM not found.", romfile);
                    break;

// Solved problem with returning to menu
/*
                case SDLK_LALT:
                case SDLK_F12:
                    if(widgets::window_load->isVisible() || widgets::window_warning->isVisible() || widgets::window_config->isVisible())
                        break;
                    if (emulating) {
                        mainMenu_case = MAIN_MENU_CASE_RUN;
                        running = false;
                    }
                    break;*/

                case SDLK_UP:
                    if(widgets::HandleNavigation(DIRECTION_UP))
                        continue; // Don't change value when enter ComboBox -> dont't send event to control
                    break;

                case SDLK_DOWN:
                    if(widgets::HandleNavigation(DIRECTION_DOWN))
                        continue; // Don't change value when enter ComboBox -> dont't send event to control
                    break;

                case SDLK_LEFT:
                    if(widgets::HandleNavigation(DIRECTION_LEFT))
                        continue; // Don't change value when enter Slider -> dont't send event to control
                    break;

                case SDLK_RIGHT:
                    if(widgets::HandleNavigation(DIRECTION_RIGHT))
                        continue; // Don't change value when enter Slider -> dont't send event to control
                    break;

                case SDLK_PAGEDOWN:
                case SDLK_HOME:
                    event.key.keysym.sym = SDLK_RETURN;
                    input->pushInput(event); // Fire key down
                    event.type = SDL_KEYUP;  // and the key up
                    break;

                case SDLK_0:
                    if(widgets::window_load->isVisible() || widgets::window_warning->isVisible() || widgets::window_config->isVisible())
                        break;
                    if(widgets::tabbedArea->getSelectedTabIndex() == 0) {
                        current_drive=0;
                        widgets::activateAfterClose = widgets::button_df0;
                        widgets::run_menuLoad_guichan(currentDir, MENU_LOAD_FLOPPY);
                    }
                    break;

                case SDLK_1:
                    if(widgets::window_load->isVisible() || widgets::window_warning->isVisible() || widgets::window_config->isVisible())
                        break;
                    if(widgets::tabbedArea->getSelectedTabIndex() == 0 && nr_drives >= 2) {
                        current_drive=1;
                        widgets::activateAfterClose = widgets::button_df1;
                        widgets::run_menuLoad_guichan(currentDir, MENU_LOAD_FLOPPY);
                    }
                    break;

                case SDLK_2:
                    if(widgets::window_load->isVisible() || widgets::window_warning->isVisible() || widgets::window_config->isVisible())
                        break;
                    if(widgets::tabbedArea->getSelectedTabIndex() == 0 && nr_drives >= 3) {
                        current_drive=2;
                        widgets::activateAfterClose = widgets::button_df2;
                        widgets::run_menuLoad_guichan(currentDir, MENU_LOAD_FLOPPY);
                    }
                    break;

                case SDLK_3:
                    if(widgets::window_load->isVisible() || widgets::window_warning->isVisible() || widgets::window_config->isVisible())
                        break;
                    if(widgets::tabbedArea->getSelectedTabIndex() == 0 && nr_drives >= 4) {
                        current_drive=3;
                        widgets::activateAfterClose = widgets::button_df3;
                        widgets::run_menuLoad_guichan(currentDir, MENU_LOAD_FLOPPY);
                    }
                    break;

#ifndef PANDORA
                case SDLK_f:
                    if (event.key.keysym.mod & KMOD_CTRL) {
                        // Works with X11 only
                        SDL_WM_ToggleFullScreen(screen);
                    }
                    break;
#endif
                }
            }
#ifdef ANDROIDSDL
            /*
             * Now that we are done polling and using SDL events we pass
             * the leftovers to the SDLInput object to later be handled by
             * the Gui. (This example doesn't require us to do this 'cause a
             * label doesn't use input. But will do it anyway to show how to
             * set up an SDL application with Guichan.)
             */
            if (event.type == SDL_MOUSEMOTION ||
                event.type == SDL_MOUSEBUTTONDOWN ||
                event.type == SDL_MOUSEBUTTONUP) {
                // Filter emulated mouse events for Guichan, we wand absolute input
            } else {
                // Convert multitouch event to SDL mouse event
                static int x = 0, y = 0, buttons = 0, wx=0, wy=0, pr=0;
                SDL_Event event2;
                memcpy(&event2, &event, sizeof(event));
                if (event.type == SDL_JOYBALLMOTION &&
                    event.jball.which == 0 &&
                    event.jball.ball == 0) {
                    event2.type = SDL_MOUSEMOTION;
                    event2.motion.which = 0;
                    event2.motion.state = buttons;
                    event2.motion.xrel = event.jball.xrel - x;
                    event2.motion.yrel = event.jball.yrel - y;
                    if (event.jball.xrel!=0) {
                        x = event.jball.xrel;
                        y = event.jball.yrel;
                    }
                    event2.motion.x = x;
                    event2.motion.y = y;
                    //__android_log_print(ANDROID_LOG_INFO, "GUICHAN","Mouse motion %d %d btns %d", x, y, buttons);
                    if (buttons == 0) {
                        // Push mouse motion event first, then button down event
                        input->pushInput(event2);
                        buttons = SDL_BUTTON_LMASK;
                        event2.type = SDL_MOUSEBUTTONDOWN;
                        event2.button.which = 0;
                        event2.button.button = SDL_BUTTON_LEFT;
                        event2.button.state =  SDL_PRESSED;
                        event2.button.x = x;
                        event2.button.y = y;
                        //__android_log_print(ANDROID_LOG_INFO, "GUICHAN","Mouse button %d coords %d %d", buttons, x, y);
                    }
                }
                if (event.type == SDL_JOYBUTTONUP &&
                    event.jbutton.which == 0 &&
                    event.jbutton.button == 0) {
                    // Do not push button down event here, because we need mouse motion event first
                    buttons = 0;
                    event2.type = SDL_MOUSEBUTTONUP;
                    event2.button.which = 0;
                    event2.button.button = SDL_BUTTON_LEFT;
                    event2.button.state = SDL_RELEASED;
                    event2.button.x = x;
                    event2.button.y = y;
                    //__android_log_print(ANDROID_LOG_INFO, "GUICHAN","Mouse button %d coords %d %d", buttons, x, y);
                }
                input->pushInput(event2);
            }
#else
            input->pushInput(event);
#endif
        }
        // Now we let the Gui object perform its logic.
        globals::gui->logic();
        // Now we let the Gui object draw itself.
        globals::gui->draw();
        // Finally we update the screen.
        SDL_Flip(screen);
    }
}

}


namespace widgets
{
void showInfo(const char *msg, const char *msg2 = "");
void run_menuLoad_guichan(char *curr_path, int aLoadType);
void run_config_guichan();
void show_settings(void);
void show_settings_TabMain(void);
void show_settings_TabFloppy(void);
void show_settings_TabHD(void);
void show_settings_TabDisplaySound(void);
void show_settings_TabSavestates(void);
void show_settings_TabControl(void);
void show_settings_TabCustomCtrl(void);
#ifdef ANDROIDSDL
void show_settings_TabOnScreen(void);
#endif
void loadMenu_Init(void);
void loadMenu_Exit(void);
void confMan_Init(void);
void confMan_Exit(void);
void menuMessage_Init(void);
void menuMessage_Exit(void);
void menuTabMain_Init(void);
void menuTabMain_Exit(void);
void menuTabFloppy_Init(void);
void menuTabFloppy_Exit(void);
void menuTabHD_Init(void);
void menuTabHD_Exit(void);
void menuTabDisplaySound_Init(void);
void menuTabDisplaySound_Exit(void);
void menuTabSavestates_Init(void);
void menuTabSavestates_Exit(void);
void menuTabControl_Init(void);
void menuTabControl_Exit(void);
void menuTabCustomCtrl_Init(void);
void menuTabCustomCtrl_Exit(void);
#ifdef ANDROIDSDL
void menuTabOnScreen_Init(void);
void menuTabOnScreen_Exit(void);
#endif

static int lastActiveTab = 0;

gcn::Color baseCol;
gcn::Color baseColLabel;
gcn::Container* top;
#ifdef RASPBERRY
gcn::ImageFont* font;
#else
gcn::contrib::SDLTrueTypeFont* font;
#endif
#ifdef ANDROIDSDL
gcn::contrib::SDLTrueTypeFont* font2;
gcn::contrib::SDLTrueTypeFont* font14;
#endif
gcn::Image *background_image;
gcn::Icon* background;
gcn::Image *image_logo;
gcn::Icon* icon_logo;
gcn::Image *image_winlogo;
gcn::Icon* icon_winlogo;

// Main buttons
gcn::Button* button_quit;
gcn::Button* button_reset;
gcn::Button* button_resume;
gcn::Button* button_save_config;
gcn::Button* button_customconfig;

// Presets
gcn::Window* window_preset;
gcn::Button* button_presetA500;
gcn::Button* button_presetA1200;

// Tab-Dialog
extern gcn::Container *tab_main;
extern gcn::Container *tab_floppy;
extern gcn::Container *tab_hard;
extern gcn::Container *tab_displaysound;
extern gcn::Container *tab_savestates;
extern gcn::Container *tab_control;
extern gcn::Container *tab_custom_control;
#ifdef ANDROIDSDL
extern gcn::Container *tab_onscreen;
#endif
#ifdef ANDROIDSDL
const int numTabs = 8;
#else
const int numTabs = 7;
#endif
gcn::Tab *allTabs[numTabs];
gcn::Widget *firstFocus[numTabs];


class QuitButtonActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) {
        mainMenu_case = MAIN_MENU_CASE_QUIT;
        running = false;
    }
};
QuitButtonActionListener* quitButtonActionListener;


class ResetButtonActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) {
        if(CheckKickstart()) {
            mainMenu_case = MAIN_MENU_CASE_RESET;
            running = false;
        } else
            widgets::showWarning("Kickstart ROM not found.", romfile);
    }
};
ResetButtonActionListener* resetButtonActionListener;


class ResumeButtonActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) {
        if (emulating) {
            mainMenu_case = MAIN_MENU_CASE_RUN;
            running = false;
        }
    }
};
ResumeButtonActionListener* resumeButtonActionListener;


class SaveCfgActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) {
        if(saveconfig(1))
            showInfo("General config file saved.");
    }
};
SaveCfgActionListener* saveCfgActionListener;


class CustomCfgActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) {
        run_config_guichan();
    }
};
CustomCfgActionListener* customCfgActionListener;


class A500ButtonActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) {
        mainMenu_chipMemory = 1;
        mainMenu_slowMemory = 0;
        mainMenu_fastMemory = 0;
        kickstart = 1;
        mainMenu_CPU_model = 0;
        mainMenu_chipset = (mainMenu_chipset & 0xff00) | 0; // Leave immediate_blit flag untouched
        mainMenu_CPU_speed = 0;
        UpdateCPUModelSettings();
        UpdateChipsetSettings();
        UpdateMemorySettings();
        show_settings();
    }
};
A500ButtonActionListener* a500ButtonActionListener;


class A1200ButtonActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) {
        mainMenu_chipMemory = 2;
        mainMenu_slowMemory = 0;
        mainMenu_fastMemory = 4;
        kickstart = 3;
        mainMenu_CPU_model = 1;
        mainMenu_chipset = (mainMenu_chipset & 0xff00) | 2; // Leave immediate_blit flag untouched
        mainMenu_CPU_speed = 1;
        UpdateCPUModelSettings();
        UpdateChipsetSettings();
        UpdateMemorySettings();
        show_settings();
    }
};
A1200ButtonActionListener* a1200ButtonActionListener;


void init()
{
    baseCol.r = 192;
    baseCol.g = 192;
    baseCol.b = 208;
    baseColLabel.r = baseCol.r;
    baseColLabel.g = baseCol.g;
    baseColLabel.b = baseCol.b;
    baseColLabel.a = 192;

    top = new gcn::Container();
    top->setDimension(gcn::Rectangle(0, 0, GUI_WIDTH, GUI_HEIGHT));
    top->setBaseColor(baseCol);
    globals::gui->setTop(top);

    TTF_Init();
#ifdef ANDROIDSDL
    font = new gcn::contrib::SDLTrueTypeFont("data/FreeSans.ttf", 16);
    font2 = new gcn::contrib::SDLTrueTypeFont("data/FreeNina.ttf", 16);
    font14 = new gcn::contrib::SDLTrueTypeFont("data/FreeSans.ttf", 14);
#elif RASPBERRY
    font = new gcn::ImageFont("data/fixedfont.bmp"," abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-()[]+'");
#else
    font = new gcn::contrib::SDLTrueTypeFont("data/FreeSans.ttf", 14);
#endif
    gcn::Widget::setGlobalFont(font);

    background_image = gcn::Image::load("data/supersonic640.jpg");
    background = new gcn::Icon(background_image);
    image_logo = gcn::Image::load("data/logo_uae4all.jpg");
    icon_logo = new gcn::Icon(image_logo);
    image_winlogo = gcn::Image::load("data/amiga-wallpaper-3.jpg");
    icon_winlogo = new gcn::Icon(image_winlogo);

    //--------------------------------------------------
    // Create main buttons
    //--------------------------------------------------
    button_quit = new gcn::Button("Quit");
    button_quit->setSize(90,50);
    button_quit->setBaseColor(baseCol);
    button_quit->setId("Quit");
    quitButtonActionListener = new QuitButtonActionListener();
    button_quit->addActionListener(quitButtonActionListener);

#if defined(AROS) || defined(WIN32) || defined(ANDROIDSDL)
    button_reset = new gcn::Button("Start");
#else
    button_reset = new gcn::Button("Reset");
#endif
    button_reset->setSize(90,50);
    button_reset->setBaseColor(baseCol);
    button_reset->setId("Reset");
    resetButtonActionListener = new ResetButtonActionListener();
    button_reset->addActionListener(resetButtonActionListener);

    button_resume = new gcn::Button("Resume");
    button_resume->setSize(90,50);
    button_resume->setBaseColor(baseCol);
    button_resume->setId("Resume");
    resumeButtonActionListener = new ResumeButtonActionListener();
    button_resume->addActionListener(resumeButtonActionListener);

    button_save_config = new gcn::Button("Save Config");
    button_save_config->setSize(90,40);
#ifdef ANDROIDSDL
    button_save_config->setFont(font14);
#endif
    button_save_config->setBaseColor(baseCol);
    button_save_config->setId("Save Config");
    saveCfgActionListener = new SaveCfgActionListener();
    button_save_config->addActionListener(saveCfgActionListener);

    button_customconfig = new gcn::Button("Conf. Manag.");
    button_customconfig->setSize(90,40);
#ifdef ANDROIDSDL
    button_customconfig->setFont(font14);
#endif
    button_customconfig->setBaseColor(baseCol);
    button_customconfig->setId("ConfManager");
    customCfgActionListener = new CustomCfgActionListener();
    button_customconfig->addActionListener(customCfgActionListener);


    //--------------------------------------------------
    // Controls for Presets
    //--------------------------------------------------
    button_presetA500 = new gcn::Button("A500");
    button_presetA500->setSize(56,30);
    button_presetA500->setPosition(10,10);
    button_presetA500->setBaseColor(baseCol);
    button_presetA500->setId("A500");
    a500ButtonActionListener = new A500ButtonActionListener();
    button_presetA500->addActionListener(a500ButtonActionListener);
    button_presetA1200 = new gcn::Button("A1200");
    button_presetA1200->setSize(56,30);
    button_presetA1200->setPosition(10,50);
    button_presetA1200->setBaseColor(baseCol);
    button_presetA1200->setId("A1200");
    a1200ButtonActionListener = new A1200ButtonActionListener();
    button_presetA1200->addActionListener(a1200ButtonActionListener);
    window_preset = new gcn::Window("Presets");
    window_preset->add(button_presetA500);
    window_preset->add(button_presetA1200);
    window_preset->setMovable(false);
    window_preset->setSize(80,110);
    window_preset->setBaseColor(baseCol);

    //--------------------------------------------------
    // Tabs
    //--------------------------------------------------
    menuTabMain_Init();
    menuTabFloppy_Init();
    menuTabHD_Init();
    menuTabDisplaySound_Init();
    menuTabSavestates_Init();
    menuTabControl_Init();
    menuTabCustomCtrl_Init();
#ifdef ANDROIDSDL
    menuTabOnScreen_Init();
#endif

    //--------------------------------------------------
    // Tab-Dialog
    //--------------------------------------------------
    tabbedArea = new gcn::TabbedArea();
    tabbedArea->setSize(600, 305);
    tabbedArea->setFocusable(false);
    tabbedArea->setBaseColor(baseCol);
    tabbedArea->setId("tabbedArea");
    for(int i=0; i<numTabs; ++i) {
        allTabs[i] = new gcn::Tab();
        allTabs[i]->setBaseColor(baseCol);
    }
#ifdef ANDROIDSDL
    allTabs[0]->setCaption(" Floppy Dr ");
#else
    allTabs[0]->setCaption(" Floppy Drv ");
#endif
    tabbedArea->addTab(allTabs[0], tab_floppy);
    allTabs[1]->setCaption(" Hard Drv ");
    tabbedArea->addTab(allTabs[1], tab_hard);
#ifdef ANDROIDSDL
    allTabs[2]->setCaption(" Misc ");
#else
    allTabs[2]->setCaption(" CPU/RAM ");
#endif
    tabbedArea->addTab(allTabs[2], tab_main);
#ifdef ANDROIDSDL
    allTabs[3]->setCaption(" Displ./Snd.");
#else
    allTabs[3]->setCaption(" Display/Sound ");
#endif
    tabbedArea->addTab(allTabs[3], tab_displaysound);
    allTabs[4]->setCaption(" Savestates ");
    tabbedArea->addTab(allTabs[4], tab_savestates);
    allTabs[5]->setCaption(" Control ");
    tabbedArea->addTab(allTabs[5], tab_control);
#ifdef ANDROIDSDL
    allTabs[6]->setCaption(" Custom ");
#else
    allTabs[6]->setCaption(" Custom Ctrl ");
#endif
    tabbedArea->addTab(allTabs[6], tab_custom_control);
#ifdef ANDROIDSDL
    allTabs[7]->setCaption(" OnScr. ");
    tabbedArea->addTab(allTabs[7], tab_onscreen);
#endif
    tabbedArea->setSelectedTab(lastActiveTab);

    loadMenu_Init();
    confMan_Init();
    menuMessage_Init();

    //--------------------------------------------------
    // Place everything on main form
    //--------------------------------------------------
    top->add(background, 0, 0);
    top->add(icon_logo, 20, 340);
    top->add(button_reset, 210, 410);
    top->add(button_resume, 320, 410);
    top->add(button_quit, 430, 410);
    top->add(button_save_config, 430, 350);
    top->add(button_customconfig, 320, 350);
    top->add(window_preset, 540, 350);
    top->add(tabbedArea, 20, 20);
    top->add(window_load, 120, 90);
    top->add(window_config, 120, 90);
    top->add(window_warning, 170, 220);

    //--------------------------------------------------
    // Initialize focus handling
    //--------------------------------------------------
    tabbedArea->setFocusable(true);
    button_reset->setFocusable(true);
    button_resume->setFocusable(true);
    button_quit->setFocusable(true);
    button_save_config->setFocusable(true);
    button_customconfig->setFocusable(true);
    button_presetA500->setFocusable(true);
    button_presetA1200->setFocusable(true);
    tabbedArea->requestFocus();

    //--------------------------------------------------
    // Display values on controls
    //--------------------------------------------------
    show_settings();
}


void halt()
{
    menuMessage_Exit();
    loadMenu_Exit();
    confMan_Exit();
    menuTabMain_Exit();
    menuTabFloppy_Exit();
    menuTabHD_Exit();
    menuTabDisplaySound_Exit();
    menuTabSavestates_Exit();
    menuTabControl_Exit();
    menuTabCustomCtrl_Exit();
#ifdef ANDROIDSDL
    menuTabOnScreen_Exit();
#endif

    lastActiveTab = tabbedArea->getSelectedTabIndex();
    delete tabbedArea;

    delete window_preset;
    delete button_presetA500;
    delete button_presetA1200;
    delete button_resume;
    delete button_reset;
    delete button_quit;
    delete button_save_config;
    delete button_customconfig;

    delete a500ButtonActionListener;
    delete a1200ButtonActionListener;
    delete resumeButtonActionListener;
    delete resetButtonActionListener;
    delete quitButtonActionListener;
    delete saveCfgActionListener;
    delete customCfgActionListener;

    delete icon_winlogo;
    delete image_winlogo;
    delete icon_logo;
    delete image_logo;
    delete background;
    delete background_image;
    delete font;
#ifdef ANDROIDSDL
    delete font14;
    delete font2;
#endif
    delete top;

    for(int i=0; i<numTabs; ++i)
        delete allTabs[i];
}


//-----------------------------------------------
// Start of helper functions
//-----------------------------------------------
void show_settings()
{
    show_settings_TabMain();
    show_settings_TabFloppy();
    show_settings_TabHD();
    show_settings_TabDisplaySound();
    show_settings_TabSavestates();
    show_settings_TabControl();
    show_settings_TabCustomCtrl();
#ifdef ANDROIDSDL
    show_settings_TabOnScreen();
#endif
}


bool HandleNavigation(int direction)
{
    int tabNum = tabbedArea->getSelectedTabIndex();
    gcn::FocusHandler* focusHdl = top->_getFocusHandler();
    gcn::Widget* focusTarget = NULL;

    if(focusHdl != NULL) {
        gcn::Widget* activeWidget = focusHdl->getFocused();
        if(activeWidget != NULL) {
            std::string activeName = activeWidget->getId();

            for(int i=0; navMap[i].activeWidget != "END"; ++i) {
                if(navMap[i].activeWidget == activeName) {
                    switch(direction) {
                    case DIRECTION_LEFT:
                        focusTarget = top->findWidgetById(navMap[i].leftWidget);
                        break;
                    case DIRECTION_RIGHT:
                        focusTarget = top->findWidgetById(navMap[i].rightWidget);
                        break;
                    case DIRECTION_UP:
                        focusTarget = top->findWidgetById(navMap[i].upWidget);
                        break;
                    case DIRECTION_DOWN:
                        focusTarget = top->findWidgetById(navMap[i].downWidget);
                        break;
                    }
                }
            }

            if(focusTarget == NULL) {
                switch(direction) {
                case DIRECTION_UP:
                    if(activeName == "tabbedArea")
                        focusTarget = button_reset;
                    break;

                case DIRECTION_DOWN:
                    if(activeName == "tabbedArea") {
                        switch(tabNum) {
                        case 0:
                            focusTarget = tab_floppy->findWidgetById("DF0");
                            break;
                        case 1:
                            focusTarget = tab_hard->findWidgetById("HDOff");
                            break;
                        case 2:
                            focusTarget = tab_main->findWidgetById("68000");
                            break;
                        case 3:
                            focusTarget = tab_displaysound->findWidgetById("320");
                            break;
                        case 4:
                            focusTarget = tab_savestates->findWidgetById("Savestate0");
                            break;
                        case 5:
                            focusTarget = tab_control->findWidgetById("ControlCfg1");
                            break;
                        case 6:
                            focusTarget = tab_custom_control->findWidgetById("CustomCtrlOff");
                            break;
#ifdef ANDROIDSDL
                        case 7:
                            focusTarget = tab_onscreen->findWidgetById("OnScrCtrl");
                            break;
#endif
                        }
                    }
                    break;
                }
            }

        }
    }

    if(focusTarget != NULL)
        focusTarget->requestFocus();

    return (focusTarget != NULL);
}

}
#ifdef ANDROIDSDL
void ClearTempFiles()
{
    struct dirent *de;
    DIR *dd;
    dd = opendir("."); /* assume it worked */
    while ((de = readdir(dd)) != NULL) {
        if (strstr(de->d_name, "uaetmp")) {
            if (unlink(de->d_name)) __android_log_print(ANDROID_LOG_INFO, "UAE4ALL2","delete failed");
        }
    }
}
#endif

int run_mainMenuGuichan()
{
    int old_sound_rate = sound_rate;
    int old_stereo = mainMenu_soundStereo;

    mainMenu_case = -1;
#ifdef ANDROIDSDL
    SDL_ANDROID_SetScreenKeyboardShown(0);
#endif
    running = true;
#ifndef ANDROIDSDL
    try
#endif
    {
        sdl::init();
        widgets::init();
        sdl::run();
        widgets::halt();
        sdl::halt();
    }
#ifndef ANDROIDSDL
    // Catch all Guichan exceptions.
    catch (gcn::Exception e) {
        std::cout << e.getMessage() << std::endl;
        mainMenu_case = MAIN_MENU_CASE_QUIT;
    }
    // Catch all Std exceptions.
    catch (std::exception e) {
        std::cout << "Std exception: " << e.what() << std::endl;
        mainMenu_case = MAIN_MENU_CASE_QUIT;
    }
    // Catch all unknown exceptions.
    catch (...) {
        std::cout << "Unknown exception" << std::endl;
        mainMenu_case = MAIN_MENU_CASE_QUIT;
    }
#endif

    switch(mainMenu_case) {
    case MAIN_MENU_CASE_SAVESTATES:
        update_display();
        setCpuSpeed();
        mainMenu_case = 1;
        break;

    case MAIN_MENU_CASE_RESET:
        setCpuSpeed();
        gp2xMouseEmuOn = 0;
        gp2xButtonRemappingOn = 0;
        mainMenu_drives = nr_drives;
        if (kickstart != oldkickstart) {
            bool bKickstartOk = true;
            oldkickstart = kickstart;
            printf("kickstart=%d\n",kickstart);
            if (kickstart==5) {
                snprintf(romfile, 256, custom_kickrom);
                uae4all_init_rom(romfile);
                bReloadKickstart = 1;
            } else {
                snprintf(romfile, 256, "%s/kickstarts/%s",launchDir,kickstarts_rom_names[kickstart]);
                if(strlen(extended_rom_names[kickstart]) == 0)
                    snprintf(extfile, 256, "");
                else
                    snprintf(extfile, 256, "%s/kickstarts/%s",launchDir,extended_rom_names[kickstart]);
                bReloadKickstart = 1;
                if(uae4all_init_rom(romfile) == -1) {
#ifdef ANDROIDSDL
                    snprintf(romfile, 256, "%s/../../com.cloanto.amigaforever.essentials/files/rom/%s",launchDir,af_kickstarts_rom_names[kickstart]);
                    uae4all_init_rom(romfile);
#endif
                }
            }
        }
        reset_hdConf();
        update_display();
        if (emulating) {
            mainMenu_case = 2;
            break;
        }
        setCpuSpeed();
        mainMenu_case = 1;
        break;

    case MAIN_MENU_CASE_RUN:
        update_display();
        setCpuSpeed();
        mainMenu_case = 1;
        break;

    case MAIN_MENU_CASE_QUIT:
#ifdef PANDORA
        resetCpuSpeed();
#endif
#ifndef USE_SDLSOUND
//			gp2x_stop_sound();
#endif
        saveAdfDir();
#ifndef ANDROIDSDL
        leave_program();
#endif
#ifdef ANDROIDSDL
        ClearTempFiles();
#endif
// FIXME (WIN32) error: 'sync' was not declared in this scope
#ifndef WIN32
        sync();
#endif
        exit(0);
        break;
    }

    if (sound_rate != old_sound_rate || mainMenu_soundStereo != old_stereo)
        init_sound();

    return mainMenu_case;
}
