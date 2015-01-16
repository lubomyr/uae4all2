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

namespace widgets
{
void setup_onscreen_pos();
void show_settings_TabOnScreen();

extern gcn::Color baseCol;
extern gcn::Container* top;
extern gcn::TabbedArea* tabbedArea;
extern gcn::Icon* icon_winlogo;
extern gcn::contrib::SDLTrueTypeFont* font14;

gcn::Container *tab_onscreen;
gcn::CheckBox* checkBox_onscreen_control;
gcn::CheckBox* checkBox_onscreen_textinput;
gcn::CheckBox* checkBox_onscreen_dpad;
gcn::CheckBox* checkBox_onscreen_button1;
gcn::CheckBox* checkBox_onscreen_button2;
gcn::CheckBox* checkBox_onscreen_button3;
gcn::CheckBox* checkBox_onscreen_button4;
gcn::CheckBox* checkBox_onscreen_button5;
gcn::CheckBox* checkBox_onscreen_button6;
gcn::CheckBox* checkBox_onscreen_custompos;
gcn::CheckBox* checkBox_FloatingJoystick;
gcn::Button* button_onscreen_pos;
gcn::Button* button_onscreen_ok;
gcn::Button* button_onscreen_reset;
gcn::Window *window_setup_position;
gcn::Window *window_pos_textinput;
gcn::Window *window_pos_dpad;
gcn::Window *window_pos_button1;
gcn::Window *window_pos_button2;
gcn::Window *window_pos_button3;
gcn::Window *window_pos_button4;
gcn::Window *window_pos_button5;
gcn::Window *window_pos_button6;
gcn::Label* label_setup_onscreen;
gcn::Window *group_quickSwitch;
gcn::UaeRadioButton* radioButton_quickSwitch_off;
gcn::UaeRadioButton* radioButton_quickSwitch_1;
gcn::UaeRadioButton* radioButton_quickSwitch_2;
gcn::Label* label_quickSwitch_1;
gcn::Label* label_quickSwitch_2;
gcn::Label* label_quickSwitch_3;
gcn::Label* label_quickSwitch_4;

class OnScreenCheckBoxActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) {
        if (actionEvent.getSource() == checkBox_onscreen_control) {
            if (checkBox_onscreen_control->isSelected())
                mainMenu_onScreen=1;
            else
                mainMenu_onScreen=0;
        }
        if (actionEvent.getSource() == checkBox_onscreen_textinput) {
            if (checkBox_onscreen_textinput->isSelected())
                mainMenu_onScreen_textinput=1;
            else
                mainMenu_onScreen_textinput=0;
        }
        if (actionEvent.getSource() == checkBox_onscreen_dpad) {
            if (checkBox_onscreen_dpad->isSelected())
                mainMenu_onScreen_dpad=1;
            else
                mainMenu_onScreen_dpad=0;
        }
        if (actionEvent.getSource() == checkBox_onscreen_button1) {
            if (checkBox_onscreen_button1->isSelected())
                mainMenu_onScreen_button1=1;
            else
                mainMenu_onScreen_button1=0;
        }
        if (actionEvent.getSource() == checkBox_onscreen_button2) {
            if (checkBox_onscreen_button2->isSelected())
                mainMenu_onScreen_button2=1;
            else
                mainMenu_onScreen_button2=0;
        }
        if (actionEvent.getSource() == checkBox_onscreen_button3) {
            if (checkBox_onscreen_button3->isSelected())
                mainMenu_onScreen_button3=1;
            else
                mainMenu_onScreen_button3=0;
        }
        if (actionEvent.getSource() == checkBox_onscreen_button4) {
            if (checkBox_onscreen_button4->isSelected())
                mainMenu_onScreen_button4=1;
            else
                mainMenu_onScreen_button4=0;
        }
        if (actionEvent.getSource() == checkBox_onscreen_button5) {
            if (checkBox_onscreen_button5->isSelected())
                mainMenu_onScreen_button5=1;
            else
                mainMenu_onScreen_button5=0;
        }
        if (actionEvent.getSource() == checkBox_onscreen_button6) {
            if (checkBox_onscreen_button6->isSelected())
                mainMenu_onScreen_button6=1;
            else
                mainMenu_onScreen_button6=0;
        }
        if (actionEvent.getSource() == checkBox_onscreen_custompos) {
            if (checkBox_onscreen_custompos->isSelected())
                mainMenu_custom_position=1;
            else
                mainMenu_custom_position=0;
        }
        if (actionEvent.getSource() == checkBox_FloatingJoystick)
            if (checkBox_FloatingJoystick->isSelected())
                mainMenu_FloatingJoystick=1;
            else
                mainMenu_FloatingJoystick=0;

        show_settings_TabOnScreen();
    }
};
OnScreenCheckBoxActionListener* onScreenCheckBoxActionListener;

class QuickSwitchActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) {
        if (actionEvent.getSource() == radioButton_quickSwitch_off)
            mainMenu_quickSwitch=0;
        else if (actionEvent.getSource() == radioButton_quickSwitch_1)
            mainMenu_quickSwitch=1;
        else if (actionEvent.getSource() == radioButton_quickSwitch_2)
            mainMenu_quickSwitch=2;
        show_settings_TabOnScreen();
    }
};
QuickSwitchActionListener* quickSwitchActionListener;

class SetupPosButtonActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) {
        if (actionEvent.getSource() == button_onscreen_pos)
            setup_onscreen_pos();
        show_settings_TabOnScreen();
    }
};
SetupPosButtonActionListener* setupPosButtonActionListener;

class WindowPosButtonActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) {
        if (actionEvent.getSource() == button_onscreen_ok) {
            mainMenu_pos_x_textinput = window_pos_textinput->getX();
            mainMenu_pos_y_textinput = window_pos_textinput->getY();
            mainMenu_pos_x_dpad = window_pos_dpad->getX();
            mainMenu_pos_y_dpad = window_pos_dpad->getY();
            mainMenu_pos_x_button1 = window_pos_button1->getX();
            mainMenu_pos_y_button1 = window_pos_button1->getY();
            mainMenu_pos_x_button2 = window_pos_button2->getX();
            mainMenu_pos_y_button2 = window_pos_button2->getY();
            mainMenu_pos_x_button3 = window_pos_button3->getX();
            mainMenu_pos_y_button3 = window_pos_button3->getY();
            mainMenu_pos_x_button4 = window_pos_button4->getX();
            mainMenu_pos_y_button4 = window_pos_button4->getY();
            mainMenu_pos_x_button5 = window_pos_button5->getX();
            mainMenu_pos_y_button5 = window_pos_button5->getY();
            mainMenu_pos_x_button6 = window_pos_button6->getX();
            mainMenu_pos_y_button6 = window_pos_button6->getY();
            top->remove(window_setup_position);
        }
        if (actionEvent.getSource() == button_onscreen_reset) {
            mainMenu_pos_x_textinput = 0;
            mainMenu_pos_y_textinput = 0;
            mainMenu_pos_x_dpad = 4;
            mainMenu_pos_y_dpad = 215;
            mainMenu_pos_x_button1 = 430;
            mainMenu_pos_y_button1 = 286;
            mainMenu_pos_x_button2 = 378;
            mainMenu_pos_y_button2 = 286;
            mainMenu_pos_x_button3 = 430;
            mainMenu_pos_y_button3 = 214;
            mainMenu_pos_x_button4 = 378;
            mainMenu_pos_y_button4 = 214;
            mainMenu_pos_x_button5 = 430;
            mainMenu_pos_y_button5 = 142;
            mainMenu_pos_x_button6 = 378;
            mainMenu_pos_y_button6 = 142;
            top->remove(window_setup_position);
        }
    }
};
WindowPosButtonActionListener* windowPosButtonActionListener;

void menuTabOnScreen_Init()
{
    checkBox_onscreen_control = new gcn::CheckBox("On-screen control");
    checkBox_onscreen_control->setPosition(10,20);
    checkBox_onscreen_control->setId("OnScrCtrl");
    onScreenCheckBoxActionListener = new OnScreenCheckBoxActionListener();
    checkBox_onscreen_control->addActionListener(onScreenCheckBoxActionListener);
    checkBox_onscreen_textinput = new gcn::CheckBox("TextInput button");
    checkBox_onscreen_textinput->setPosition(10,50);
    checkBox_onscreen_textinput->setId("OnScrTextInput");
    checkBox_onscreen_textinput->addActionListener(onScreenCheckBoxActionListener);
    checkBox_onscreen_dpad = new gcn::CheckBox("D-pad");
    checkBox_onscreen_dpad->setPosition(10,80);
    checkBox_onscreen_dpad->setId("OnScrDpad");
    checkBox_onscreen_dpad->addActionListener(onScreenCheckBoxActionListener);
    checkBox_onscreen_button1 = new gcn::CheckBox("Button 1 <A>");
    checkBox_onscreen_button1->setPosition(10,110);
    checkBox_onscreen_button1->setId("OnScrButton1");
    checkBox_onscreen_button1->addActionListener(onScreenCheckBoxActionListener);
    checkBox_onscreen_button2 = new gcn::CheckBox("Button 2 <B>");
    checkBox_onscreen_button2->setPosition(10,140);
    checkBox_onscreen_button2->setId("OnScrButton2");
    checkBox_onscreen_button2->addActionListener(onScreenCheckBoxActionListener);
    checkBox_onscreen_button3 = new gcn::CheckBox("Button 3 <X>");
    checkBox_onscreen_button3->setPosition(170,20);
    checkBox_onscreen_button3->setId("OnScrButton3");
    checkBox_onscreen_button3->addActionListener(onScreenCheckBoxActionListener);
    checkBox_onscreen_button4 = new gcn::CheckBox("Button 4 <Y>");
    checkBox_onscreen_button4->setPosition(170,50);
    checkBox_onscreen_button4->setId("OnScrButton4");
    checkBox_onscreen_button4->addActionListener(onScreenCheckBoxActionListener);
    checkBox_onscreen_button5 = new gcn::CheckBox("Button 5 <R>");
    checkBox_onscreen_button5->setPosition(170,80);
    checkBox_onscreen_button5->setId("OnScrButton5");
    checkBox_onscreen_button5->addActionListener(onScreenCheckBoxActionListener);
    checkBox_onscreen_button6 = new gcn::CheckBox("Button 6 <L>");
    checkBox_onscreen_button6->setPosition(170,110);
    checkBox_onscreen_button6->setId("OnScrButton6");
    checkBox_onscreen_button6->addActionListener(onScreenCheckBoxActionListener);
    checkBox_onscreen_custompos = new gcn::CheckBox("Custom position");
    checkBox_onscreen_custompos->setPosition(170,140);
    checkBox_onscreen_custompos->setId("CustomPos");
    checkBox_onscreen_custompos->addActionListener(onScreenCheckBoxActionListener);
    checkBox_FloatingJoystick = new gcn::CheckBox("Floating Joystick");
    checkBox_FloatingJoystick->setPosition(10,180);
    checkBox_FloatingJoystick->setId("FloatJoy");
    checkBox_FloatingJoystick->addActionListener(onScreenCheckBoxActionListener);

    button_onscreen_pos = new gcn::Button("Position Setup");
    button_onscreen_pos->setPosition(170,180);
    button_onscreen_pos->setBaseColor(baseCol);
    setupPosButtonActionListener = new SetupPosButtonActionListener();
    button_onscreen_pos->addActionListener(setupPosButtonActionListener);

    radioButton_quickSwitch_off = new gcn::UaeRadioButton("Off", "radioquickswitchgroup");
    radioButton_quickSwitch_off->setPosition(5,10);
    radioButton_quickSwitch_off->setId("QckSwtchOff");
    quickSwitchActionListener = new QuickSwitchActionListener();
    radioButton_quickSwitch_off->addActionListener(quickSwitchActionListener);
    radioButton_quickSwitch_1 = new gcn::UaeRadioButton("1", "radioquickswitchgroup");
    radioButton_quickSwitch_1->setPosition(70,10);
    radioButton_quickSwitch_1->setId("QckSwtch1");
    radioButton_quickSwitch_1->addActionListener(quickSwitchActionListener);
    radioButton_quickSwitch_2 = new gcn::UaeRadioButton("2", "radioquickswitchgroup");
    radioButton_quickSwitch_2->setPosition(135,10);
    radioButton_quickSwitch_2->setId("QckSwtch2");
    radioButton_quickSwitch_2->addActionListener(quickSwitchActionListener);
    label_quickSwitch_1 = new gcn::Label("Label 1");
    label_quickSwitch_1->setPosition(10,40);
    label_quickSwitch_1->setFont(font14);
    label_quickSwitch_1->setWidth(200);
    label_quickSwitch_2 = new gcn::Label("Label 2");
    label_quickSwitch_2->setPosition(10,65);
    label_quickSwitch_2->setFont(font14);
    label_quickSwitch_2->setWidth(200);
    label_quickSwitch_3 = new gcn::Label("Label 3");
    label_quickSwitch_3->setPosition(10,90);
    label_quickSwitch_3->setFont(font14);
    label_quickSwitch_3->setWidth(200);
    label_quickSwitch_4 = new gcn::Label("Label 4");
    label_quickSwitch_4->setPosition(10,115);
    label_quickSwitch_4->setFont(font14);
    label_quickSwitch_4->setWidth(200);

    group_quickSwitch = new gcn::Window("Quick Switch via buttons");
    group_quickSwitch->setPosition(360,20);
    group_quickSwitch->add(radioButton_quickSwitch_off);
    group_quickSwitch->add(radioButton_quickSwitch_1);
    group_quickSwitch->add(radioButton_quickSwitch_2);
    group_quickSwitch->add(label_quickSwitch_1);
    group_quickSwitch->add(label_quickSwitch_2);
    group_quickSwitch->add(label_quickSwitch_3);
    group_quickSwitch->add(label_quickSwitch_4);
    group_quickSwitch->setMovable(false);
    group_quickSwitch->setSize(200,160);
    group_quickSwitch->setBaseColor(baseCol);

    button_onscreen_ok = new gcn::Button(" Ok ");
    button_onscreen_ok->setPosition(220,175);
    button_onscreen_ok->setBaseColor(baseCol);
    button_onscreen_reset = new gcn::Button(" Reset Position to default ");
    button_onscreen_reset->setPosition(150,105);
    button_onscreen_reset->setBaseColor(baseCol);
    windowPosButtonActionListener = new WindowPosButtonActionListener();
    button_onscreen_ok->addActionListener(windowPosButtonActionListener);
    button_onscreen_reset->addActionListener(windowPosButtonActionListener);
    label_setup_onscreen = new gcn::Label("Try drag and drop window then press ok");
    label_setup_onscreen->setPosition(100,140);
    label_setup_onscreen->setFont(font14);

    window_pos_textinput = new gcn::Window("Ab");
    window_pos_textinput->setMovable(true);
    window_pos_textinput->setSize(25,30);
    window_pos_textinput->setBaseColor(baseCol);
    window_pos_dpad = new gcn::Window("Dpad");
    window_pos_dpad->setMovable(true);
    window_pos_dpad->setSize(100,130);
    window_pos_dpad->setBaseColor(baseCol);
    window_pos_button1 = new gcn::Window("1<A>");
    window_pos_button1->setMovable(true);
    window_pos_button1->setSize(50,65);
    window_pos_button1->setBaseColor(baseCol);
    window_pos_button2 = new gcn::Window("2<B>");
    window_pos_button2->setMovable(true);
    window_pos_button2->setSize(50,65);
    window_pos_button2->setBaseColor(baseCol);
    window_pos_button3 = new gcn::Window("3<X>");
    window_pos_button3->setMovable(true);
    window_pos_button3->setSize(50,65);
    window_pos_button3->setBaseColor(baseCol);
    window_pos_button4 = new gcn::Window("4<Y>");
    window_pos_button4->setMovable(true);
    window_pos_button4->setSize(50,65);
    window_pos_button4->setBaseColor(baseCol);
    window_pos_button5 = new gcn::Window("5<R>");
    window_pos_button5->setMovable(true);
    window_pos_button5->setSize(50,65);
    window_pos_button5->setBaseColor(baseCol);
    window_pos_button6 = new gcn::Window("6<L>");
    window_pos_button6->setMovable(true);
    window_pos_button6->setSize(50,65);
    window_pos_button6->setBaseColor(baseCol);

    show_settings_TabOnScreen();

    window_setup_position = new gcn::Window("Setup position");
    window_setup_position->setPosition(80,70);
    window_setup_position->add(label_setup_onscreen);
    window_setup_position->add(button_onscreen_ok);
    window_setup_position->add(button_onscreen_reset);
    window_setup_position->add(window_pos_textinput);
    window_setup_position->add(window_pos_dpad);
    window_setup_position->add(window_pos_button1);
    window_setup_position->add(window_pos_button2);
    window_setup_position->add(window_pos_button3);
    window_setup_position->add(window_pos_button4);
    window_setup_position->add(window_pos_button5);
    window_setup_position->add(window_pos_button6);
    window_setup_position->setMovable(false);
    window_setup_position->setSize(485,370);

    tab_onscreen = new gcn::Container();
    tab_onscreen->add(icon_winlogo);
    tab_onscreen->add(checkBox_onscreen_control);
    tab_onscreen->add(checkBox_onscreen_textinput);
    tab_onscreen->add(checkBox_onscreen_dpad);
    tab_onscreen->add(checkBox_onscreen_button1);
    tab_onscreen->add(checkBox_onscreen_button2);
    tab_onscreen->add(checkBox_onscreen_button3);
    tab_onscreen->add(checkBox_onscreen_button4);
    tab_onscreen->add(checkBox_onscreen_button5);
    tab_onscreen->add(checkBox_onscreen_button6);
    tab_onscreen->add(checkBox_onscreen_custompos);
    tab_onscreen->add(checkBox_FloatingJoystick);
    tab_onscreen->add(button_onscreen_pos);
    tab_onscreen->add(group_quickSwitch);
    tab_onscreen->setSize(600,280);
    tab_onscreen->setBaseColor(baseCol);
}

void menuTabOnScreen_Exit()
{
    delete tab_onscreen;
    delete checkBox_onscreen_control;
    delete checkBox_onscreen_textinput;
    delete checkBox_onscreen_dpad;
    delete checkBox_onscreen_button1;
    delete checkBox_onscreen_button2;
    delete checkBox_onscreen_button3;
    delete checkBox_onscreen_button4;
    delete checkBox_onscreen_button5;
    delete checkBox_onscreen_button6;
    delete checkBox_onscreen_custompos;
    delete checkBox_FloatingJoystick;
    delete button_onscreen_pos;
    delete button_onscreen_ok;
    delete button_onscreen_reset;
    delete window_setup_position;
    delete window_pos_textinput;
    delete window_pos_dpad;
    delete window_pos_button1;
    delete window_pos_button2;
    delete window_pos_button3;
    delete window_pos_button4;
    delete window_pos_button5;
    delete window_pos_button6;
    delete label_setup_onscreen;
    delete group_quickSwitch;
    delete radioButton_quickSwitch_off;
    delete radioButton_quickSwitch_1;
    delete radioButton_quickSwitch_2;
    delete label_quickSwitch_1;
    delete label_quickSwitch_2;
    delete label_quickSwitch_3;
    delete label_quickSwitch_4;

    delete onScreenCheckBoxActionListener;
    delete setupPosButtonActionListener;
    delete windowPosButtonActionListener;
    delete quickSwitchActionListener;
}

void show_settings_TabOnScreen()
{
    if (mainMenu_onScreen==0)
        checkBox_onscreen_control->setSelected(false);
    else if (mainMenu_onScreen==1)
        checkBox_onscreen_control->setSelected(true);
    if (mainMenu_onScreen_textinput==0)
        checkBox_onscreen_textinput->setSelected(false);
    else if (mainMenu_onScreen_textinput==1)
        checkBox_onscreen_textinput->setSelected(true);
    if (mainMenu_onScreen_dpad==0)
        checkBox_onscreen_dpad->setSelected(false);
    else if (mainMenu_onScreen_dpad==1)
        checkBox_onscreen_dpad->setSelected(true);
    if (mainMenu_onScreen_button1==0)
        checkBox_onscreen_button1->setSelected(false);
    else if (mainMenu_onScreen_button1==1)
        checkBox_onscreen_button1->setSelected(true);
    if (mainMenu_onScreen_button2==0)
        checkBox_onscreen_button2->setSelected(false);
    else if (mainMenu_onScreen_button2==1)
        checkBox_onscreen_button2->setSelected(true);
    if (mainMenu_onScreen_button3==0)
        checkBox_onscreen_button3->setSelected(false);
    else if (mainMenu_onScreen_button3==1)
        checkBox_onscreen_button3->setSelected(true);
    if (mainMenu_onScreen_button4==0)
        checkBox_onscreen_button4->setSelected(false);
    else if (mainMenu_onScreen_button4==1)
        checkBox_onscreen_button4->setSelected(true);
    if (mainMenu_onScreen_button5==0)
        checkBox_onscreen_button5->setSelected(false);
    else if (mainMenu_onScreen_button5==1)
        checkBox_onscreen_button5->setSelected(true);
    if (mainMenu_onScreen_button6==0)
        checkBox_onscreen_button6->setSelected(false);
    else if (mainMenu_onScreen_button6==1)
        checkBox_onscreen_button6->setSelected(true);
    if (mainMenu_custom_position==0)
        checkBox_onscreen_custompos->setSelected(false);
    else if (mainMenu_custom_position==1)
        checkBox_onscreen_custompos->setSelected(true);
    if (mainMenu_FloatingJoystick)
        checkBox_FloatingJoystick->setSelected(true);
    else
        checkBox_FloatingJoystick->setSelected(false);
    if (mainMenu_quickSwitch==0) {
        radioButton_quickSwitch_off->setSelected(true);
        label_quickSwitch_1->setCaption("");
        label_quickSwitch_2->setCaption("");
        label_quickSwitch_3->setCaption("");
        label_quickSwitch_4->setCaption("");
        group_quickSwitch->setHeight(55);
    } else if (mainMenu_quickSwitch==1) {
        radioButton_quickSwitch_1->setSelected(true);
        label_quickSwitch_1->setCaption("'2'<B> + UP - lores/hires");
        label_quickSwitch_2->setCaption("'2'<B> + DOWN - scr.height");
        label_quickSwitch_3->setCaption("'2'<B> + LEFT - save state");
        label_quickSwitch_4->setCaption("'2'<B> + RIGHT - load state");
        group_quickSwitch->setHeight(160);
    } else if (mainMenu_quickSwitch==2) {
        radioButton_quickSwitch_2->setSelected(true);
        label_quickSwitch_1->setCaption("'4'<Y> + UP - lores/hires");
        label_quickSwitch_2->setCaption("'4'<Y> + DOWN - scr.height");
        label_quickSwitch_3->setCaption("'4'<Y> + LEFT - save state");
        label_quickSwitch_4->setCaption("'4'<Y> + RIGHT - load state");
        group_quickSwitch->setHeight(160);
    }


    window_pos_textinput->setX(mainMenu_pos_x_textinput);
    window_pos_textinput->setY(mainMenu_pos_y_textinput);
    window_pos_textinput->setVisible(mainMenu_onScreen_textinput);
    window_pos_dpad->setX(mainMenu_pos_x_dpad);
    window_pos_dpad->setY(mainMenu_pos_y_dpad);
    window_pos_dpad->setVisible(mainMenu_onScreen_dpad);
    window_pos_button1->setX(mainMenu_pos_x_button1);
    window_pos_button1->setY(mainMenu_pos_y_button1);
    window_pos_button1->setVisible(mainMenu_onScreen_button1);
    window_pos_button2->setX(mainMenu_pos_x_button2);
    window_pos_button2->setY(mainMenu_pos_y_button2);
    window_pos_button2->setVisible(mainMenu_onScreen_button2);
    window_pos_button3->setX(mainMenu_pos_x_button3);
    window_pos_button3->setY(mainMenu_pos_y_button3);
    window_pos_button3->setVisible(mainMenu_onScreen_button3);
    window_pos_button4->setX(mainMenu_pos_x_button4);
    window_pos_button4->setY(mainMenu_pos_y_button4);
    window_pos_button4->setVisible(mainMenu_onScreen_button4);
    window_pos_button5->setX(mainMenu_pos_x_button5);
    window_pos_button5->setY(mainMenu_pos_y_button5);
    window_pos_button5->setVisible(mainMenu_onScreen_button5);
    window_pos_button6->setX(mainMenu_pos_x_button6);
    window_pos_button6->setY(mainMenu_pos_y_button6);
    window_pos_button6->setVisible(mainMenu_onScreen_button6);
    button_onscreen_pos->setVisible(mainMenu_custom_position);
}

void setup_onscreen_pos()
{
    top->add(window_setup_position);
}

}