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


namespace widgets
{
void draw_customcontrol(void);

extern gcn::Color baseCol;
extern gcn::Color baseColLabel;
extern gcn::Container* top;
extern gcn::TabbedArea* tabbedArea;
extern gcn::Icon* icon_winlogo;

// Tab Custom controls
gcn::Container *tab_custom_control;
gcn::Window *group_custom_control_enable;
gcn::UaeRadioButton* radioButton_custom_control_on;
gcn::UaeRadioButton* radioButton_custom_control_off;
gcn::Window *group_dpad;
gcn::UaeRadioButton* radioButton_dpad_custom;
gcn::UaeRadioButton* radioButton_dpad_joystick;
gcn::UaeRadioButton* radioButton_dpad_mouse;
gcn::Container* backgrd_up;
gcn::Container* backgrd_down;
gcn::Container* backgrd_left;
gcn::Container* backgrd_right;
gcn::Container* backgrd_a;
gcn::Container* backgrd_b;
gcn::Container* backgrd_x;
gcn::Container* backgrd_y;
gcn::Container* backgrd_l;
gcn::Container* backgrd_r;
gcn::Label* label_up;
gcn::Label* label_down;
gcn::Label* label_left;
gcn::Label* label_right;
gcn::Label* label_a;
gcn::Label* label_b;
gcn::Label* label_x;
gcn::Label* label_y;
gcn::Label* label_l;
gcn::Label* label_r;
gcn::UaeDropDown* dropDown_up;
gcn::UaeDropDown* dropDown_down;
gcn::UaeDropDown* dropDown_left;
gcn::UaeDropDown* dropDown_right;
gcn::UaeDropDown* dropDown_a;
gcn::UaeDropDown* dropDown_b;
gcn::UaeDropDown* dropDown_x;
gcn::UaeDropDown* dropDown_y;
gcn::UaeDropDown* dropDown_l;
gcn::UaeDropDown* dropDown_r;


class MappingListModel : public gcn::ListModel
{
public:
    int getNumberOfElements() {
        return 105;
    }

    std::string getElementAt(int i) {
        switch(i) {
        case 0:
            return std::string("Joystick RIGHT");
        case 1:
            return std::string("Joystick LEFT");
        case 2:
            return std::string("Joystick DOWN");
        case 3:
            return std::string("Joystick UP");
        case 4:
            return std::string("Joystick fire but.2");
        case 5:
            return std::string("Joystick fire but.1");
        case 6:
            return std::string("Mouse right button");
        case 7:
            return std::string("Mouse left button");
        case 8:
            return std::string("------------------");
        case 9:
            return std::string("arrow UP");
        case 10:
            return std::string("arrow DOWN");
        case 11:
            return std::string("arrow LEFT");
        case 12:
            return std::string("arrow RIGHT");
        case 13:
            return std::string("numpad 0");
        case 14:
            return std::string("numpad 1");
        case 15:
            return std::string("numpad 2");
        case 16:
            return std::string("numpad 3");
        case 17:
            return std::string("numpad 4");
        case 18:
            return std::string("numpad 5");
        case 19:
            return std::string("numpad 6");
        case 20:
            return std::string("numpad 7");
        case 21:
            return std::string("numpad 8");
        case 22:
            return std::string("numpad 9");
        case 23:
            return std::string("numpad ENTER");
        case 24:
            return std::string("numpad DIVIDE");
        case 25:
            return std::string("numpad MULTIPLY");
        case 26:
            return std::string("numpad MINUS");
        case 27:
            return std::string("numpad PLUS");
        case 28:
            return std::string("numpad DELETE");
        case 29:
            return std::string("numpad LEFT PARENTHESIS");
        case 30:
            return std::string("numpad RIGHT PARENTHESIS");
        case 31:
            return std::string("SPACE");
        case 32:
            return std::string("BACKSPACE");
        case 33:
            return std::string("TAB");
        case 34:
            return std::string("RETURN");
        case 35:
            return std::string("ESCAPE");
        case 36:
            return std::string("DELETE");
        case 37:
            return std::string("left SHIFT");
        case 38:
            return std::string("right SHIFT");
        case 39:
            return std::string("CAPS LOCK");
        case 40:
            return std::string("CTRL");
        case 41:
            return std::string("left ALT");
        case 42:
            return std::string("right ALT");
        case 43:
            return std::string("left AMIGA key");
        case 44:
            return std::string("right AMIGA key");
        case 45:
            return std::string("HELP");
        case 46:
            return std::string("left bracket");
        case 47:
            return std::string("right bracket");
        case 48:
            return std::string("semicolon");
        case 49:
            return std::string("comma");
        case 50:
            return std::string("period");
        case 51:
            return std::string("slash");
        case 52:
            return std::string("backslash");
        case 53:
            return std::string("quote");
        case 54:
            return std::string("numbersign");
        case 55:
            return std::string("less than - greater than");
        case 56:
            return std::string("backquote");
        case 57:
            return std::string("minus");
        case 58:
            return std::string("equal");
        case 59:
            return std::string("A");
        case 60:
            return std::string("B");
        case 61:
            return std::string("C");
        case 62:
            return std::string("D");
        case 63:
            return std::string("E");
        case 64:
            return std::string("F");
        case 65:
            return std::string("G");
        case 66:
            return std::string("H");
        case 67:
            return std::string("I");
        case 68:
            return std::string("J");
        case 69:
            return std::string("K");
        case 70:
            return std::string("L");
        case 71:
            return std::string("M");
        case 72:
            return std::string("N");
        case 73:
            return std::string("O");
        case 74:
            return std::string("P");
        case 75:
            return std::string("Q");
        case 76:
            return std::string("R");
        case 77:
            return std::string("S");
        case 78:
            return std::string("T");
        case 79:
            return std::string("U");
        case 80:
            return std::string("V");
        case 81:
            return std::string("W");
        case 82:
            return std::string("X");
        case 83:
            return std::string("Y");
        case 84:
            return std::string("Z");
        case 85:
            return std::string("1");
        case 86:
            return std::string("2");
        case 87:
            return std::string("3");
        case 88:
            return std::string("4");
        case 89:
            return std::string("5");
        case 90:
            return std::string("6");
        case 91:
            return std::string("7");
        case 92:
            return std::string("8");
        case 93:
            return std::string("9");
        case 94:
            return std::string("0");
        case 95:
            return std::string("F1");
        case 96:
            return std::string("F2");
        case 97:
            return std::string("F3");
        case 98:
            return std::string("F4");
        case 99:
            return std::string("F5");
        case 100:
            return std::string("F6");
        case 101:
            return std::string("F7");
        case 102:
            return std::string("F8");
        case 103:
            return std::string("F9");
        case 104:
            return std::string("F10");
        case 105:
            return std::string("NULL");
        }
        return std::string(""); // Keep the compiler happy
    }
};
MappingListModel mappingList;


class CustomCtrlActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) {
        if (actionEvent.getSource() == radioButton_custom_control_on)
            mainMenu_customControls=1;
        else if (actionEvent.getSource() == radioButton_custom_control_off)
            mainMenu_customControls=0;
        draw_customcontrol();
    }
};
CustomCtrlActionListener* customCtrlActionListener;


class DPadActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) {
        if (actionEvent.getSource() == radioButton_dpad_custom)
            mainMenu_custom_dpad=0;
        else if (actionEvent.getSource() == radioButton_dpad_joystick)
            mainMenu_custom_dpad=1;
        else if (actionEvent.getSource() == radioButton_dpad_mouse)
            mainMenu_custom_dpad=2;
        draw_customcontrol();
    }
};
DPadActionListener* dPadActionListener;


class ComboActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) {
        if (actionEvent.getSource() == dropDown_up)
            mainMenu_custom_up=dropDown_up->getSelected()-8;
        if (actionEvent.getSource() == dropDown_down)
            mainMenu_custom_down=dropDown_down->getSelected()-8;
        if (actionEvent.getSource() == dropDown_left)
            mainMenu_custom_left=dropDown_left->getSelected()-8;
        if (actionEvent.getSource() == dropDown_right)
            mainMenu_custom_right=dropDown_right->getSelected()-8;
        if (actionEvent.getSource() == dropDown_a)
            mainMenu_custom_A=dropDown_a->getSelected()-8;
        if (actionEvent.getSource() == dropDown_b)
            mainMenu_custom_B=dropDown_b->getSelected()-8;
        if (actionEvent.getSource() == dropDown_x)
            mainMenu_custom_X=dropDown_x->getSelected()-8;
        if (actionEvent.getSource() == dropDown_y)
            mainMenu_custom_Y=dropDown_y->getSelected()-8;
        if (actionEvent.getSource() == dropDown_l)
            mainMenu_custom_L=dropDown_l->getSelected()-8;
        if (actionEvent.getSource() == dropDown_r)
            mainMenu_custom_R=dropDown_r->getSelected()-8;
    }
};
ComboActionListener* comboActionListener;


void menuTabCustomCtrl_Init()
{
    // Select Custom control on/off
    radioButton_custom_control_off = new gcn::UaeRadioButton("off", "radiocustomcontrolgroup");
    radioButton_custom_control_off->setPosition(5,10);
    radioButton_custom_control_off->setId("CustomCtrlOff");
    customCtrlActionListener = new CustomCtrlActionListener();
    radioButton_custom_control_off->addActionListener(customCtrlActionListener);
    radioButton_custom_control_on = new gcn::UaeRadioButton("on", "radiocustomcontrolgroup");
    radioButton_custom_control_on->setPosition(60,10);
    radioButton_custom_control_on->setId("CustomCtrlOn");
    radioButton_custom_control_on->addActionListener(customCtrlActionListener);
    group_custom_control_enable = new gcn::Window("Custom Control");
    group_custom_control_enable->setPosition(10,20);
    group_custom_control_enable->add(radioButton_custom_control_off);
    group_custom_control_enable->add(radioButton_custom_control_on);
    group_custom_control_enable->setMovable(false);
    group_custom_control_enable->setSize(120,55);
    group_custom_control_enable->setBaseColor(baseCol);

    // Select dPad
    radioButton_dpad_custom = new gcn::UaeRadioButton("Custom", "radiodpadgroup");
    radioButton_dpad_custom->setPosition(5,10);
    radioButton_dpad_custom->setId("DPadCustom");
    dPadActionListener = new DPadActionListener();
    radioButton_dpad_custom->addActionListener(dPadActionListener);
    radioButton_dpad_joystick = new gcn::UaeRadioButton("Joystick", "radiodpadgroup");
    radioButton_dpad_joystick->setPosition(5,40);
    radioButton_dpad_joystick->setId("DPadJoystick");
    radioButton_dpad_joystick->addActionListener(dPadActionListener);
    radioButton_dpad_mouse = new gcn::UaeRadioButton("Mouse", "radiodpadgroup");
    radioButton_dpad_mouse->setPosition(5,70);
    radioButton_dpad_mouse->setId("DPadMouse");
    radioButton_dpad_mouse->addActionListener(dPadActionListener);
    group_dpad = new gcn::Window("DPAD");
    group_dpad->setPosition(10,100);
    group_dpad->add(radioButton_dpad_custom);
    group_dpad->add(radioButton_dpad_joystick);
    group_dpad->add(radioButton_dpad_mouse);
    group_dpad->setMovable(false);
    group_dpad->setSize(120,115);
    group_dpad->setBaseColor(baseCol);

    label_up = new gcn::Label("up");
    label_up->setPosition(4, 2);
    backgrd_up = new gcn::Container();
    backgrd_up->setOpaque(true);
    backgrd_up->setBaseColor(baseColLabel);
    backgrd_up->setPosition(380, 20);
    backgrd_up->setSize(50, 21);
    backgrd_up->add(label_up);
    backgrd_up->setVisible(false);
    label_down = new gcn::Label("down");
    label_down->setPosition(4, 2);
    backgrd_down = new gcn::Container();
    backgrd_down->setOpaque(true);
    backgrd_down->setBaseColor(baseColLabel);
    backgrd_down->setPosition(380, 45);
    backgrd_down->setSize(50, 21);
    backgrd_down->add(label_down);
    backgrd_down->setVisible(false);
    label_left = new gcn::Label("left");
    label_left->setPosition(4, 2);
    backgrd_left = new gcn::Container();
    backgrd_left->setOpaque(true);
    backgrd_left->setBaseColor(baseColLabel);
    backgrd_left->setPosition(380, 70);
    backgrd_left->setSize(50, 21);
    backgrd_left->add(label_left);
    backgrd_left->setVisible(false);
    label_right = new gcn::Label("right");
    label_right->setPosition(4, 2);
    backgrd_right = new gcn::Container();
    backgrd_right->setOpaque(true);
    backgrd_right->setBaseColor(baseColLabel);
    backgrd_right->setPosition(380, 95);
    backgrd_right->setSize(50, 21);
    backgrd_right->add(label_right);
    backgrd_right->setVisible(false);
    label_a = new gcn::Label("<A>");
    label_a->setPosition(4, 2);
    backgrd_a = new gcn::Container();
    backgrd_a->setOpaque(true);
    backgrd_a->setBaseColor(baseColLabel);
    backgrd_a->setPosition(160, 20);
    backgrd_a->setSize(50, 21);
    backgrd_a->add(label_a);
    backgrd_a->setVisible(false);
    label_b = new gcn::Label("<B>");
    label_b->setPosition(4, 2);
    backgrd_b = new gcn::Container();
    backgrd_b->setOpaque(true);
    backgrd_b->setBaseColor(baseColLabel);
    backgrd_b->setPosition(160, 45);
    backgrd_b->setSize(50, 21);
    backgrd_b->add(label_b);
    backgrd_b->setVisible(false);
    label_x = new gcn::Label("<X>");
    label_x->setPosition(4, 2);
    backgrd_x = new gcn::Container();
    backgrd_x->setOpaque(true);
    backgrd_x->setBaseColor(baseColLabel);
    backgrd_x->setPosition(160, 70);
    backgrd_x->setSize(50, 21);
    backgrd_x->add(label_x);
    backgrd_x->setVisible(false);
    label_y = new gcn::Label("<Y>");
    label_y->setPosition(4, 2);
    backgrd_y = new gcn::Container();
    backgrd_y->setOpaque(true);
    backgrd_y->setBaseColor(baseColLabel);
    backgrd_y->setPosition(160, 95);
    backgrd_y->setSize(50, 21);
    backgrd_y->add(label_y);
    backgrd_y->setVisible(false);
    label_l = new gcn::Label("<L>");
    label_l->setPosition(4, 2);
    backgrd_l = new gcn::Container();
    backgrd_l->setOpaque(true);
    backgrd_l->setBaseColor(baseColLabel);
    backgrd_l->setPosition(160, 130);
    backgrd_l->setSize(50, 21);
    backgrd_l->add(label_l);
    backgrd_l->setVisible(false);
    label_r = new gcn::Label("<R>");
    label_r->setPosition(4, 2);
    backgrd_r = new gcn::Container();
    backgrd_r->setOpaque(true);
    backgrd_r->setBaseColor(baseColLabel);
    backgrd_r->setPosition(160, 155);
    backgrd_r->setSize(50, 21);
    backgrd_r->add(label_r);
    backgrd_r->setVisible(false);

    dropDown_up = new gcn::UaeDropDown(&mappingList);
    dropDown_up->setPosition(440,20);
    dropDown_up->setVisible(false);
    dropDown_up->setWidth(130);
    dropDown_up->setBaseColor(baseCol);
    dropDown_up->setId("CtrlUp");
    comboActionListener = new ComboActionListener();
    dropDown_up->addActionListener(comboActionListener);
    dropDown_down = new gcn::UaeDropDown(&mappingList);
    dropDown_down->setPosition(440,45);
    dropDown_down->setVisible(false);
    dropDown_down->setWidth(130);
    dropDown_down->setBaseColor(baseCol);
    dropDown_down->setId("CtrlDown");
    dropDown_down->addActionListener(comboActionListener);
    dropDown_left = new gcn::UaeDropDown(&mappingList);
    dropDown_left->setPosition(440,70);
    dropDown_left->setVisible(false);
    dropDown_left->setWidth(130);
    dropDown_left->setBaseColor(baseCol);
    dropDown_left->setId("CtrlLeft");
    dropDown_left->addActionListener(comboActionListener);
    dropDown_right = new gcn::UaeDropDown(&mappingList);
    dropDown_right->setPosition(440,95);
    dropDown_right->setVisible(false);
    dropDown_right->setWidth(130);
    dropDown_right->setBaseColor(baseCol);
    dropDown_right->setId("CtrlRight");
    dropDown_right->addActionListener(comboActionListener);
    dropDown_a = new gcn::UaeDropDown(&mappingList);
    dropDown_a->setPosition(220,20);
    dropDown_a->setVisible(false);
    dropDown_a->setWidth(130);
    dropDown_a->setBaseColor(baseCol);
    dropDown_a->setId("CtrlA");
    dropDown_a->addActionListener(comboActionListener);
    dropDown_b = new gcn::UaeDropDown(&mappingList);
    dropDown_b->setPosition(220,45);
    dropDown_b->setVisible(false);
    dropDown_b->setWidth(130);
    dropDown_b->setBaseColor(baseCol);
    dropDown_b->setId("CtrlB");
    dropDown_b->addActionListener(comboActionListener);
    dropDown_x = new gcn::UaeDropDown(&mappingList);
    dropDown_x->setPosition(220,70);
    dropDown_x->setVisible(false);
    dropDown_x->setWidth(130);
    dropDown_x->setBaseColor(baseCol);
    dropDown_x->setId("CtrlX");
    dropDown_x->addActionListener(comboActionListener);
    dropDown_y = new gcn::UaeDropDown(&mappingList);
    dropDown_y->setPosition(220,95);
    dropDown_y->setVisible(false);
    dropDown_y->setWidth(130);
    dropDown_y->setBaseColor(baseCol);
    dropDown_y->setId("CtrlY");
    dropDown_y->addActionListener(comboActionListener);
    dropDown_l = new gcn::UaeDropDown(&mappingList);
    dropDown_l->setPosition(220,130);
    dropDown_l->setVisible(false);
    dropDown_l->setWidth(130);
    dropDown_l->setBaseColor(baseCol);
    dropDown_l->setId("CtrlL");
    dropDown_l->addActionListener(comboActionListener);
    dropDown_r = new gcn::UaeDropDown(&mappingList);
    dropDown_r->setPosition(220,155);
    dropDown_r->setVisible(false);
    dropDown_r->setWidth(130);
    dropDown_r->setBaseColor(baseCol);
    dropDown_r->setId("CtrlR");
    dropDown_r->addActionListener(comboActionListener);

    tab_custom_control = new gcn::Container();
    tab_custom_control->add(icon_winlogo);
    tab_custom_control->add(group_custom_control_enable);
    tab_custom_control->add(group_dpad);
    tab_custom_control->add(backgrd_up);
    tab_custom_control->add(backgrd_down);
    tab_custom_control->add(backgrd_left);
    tab_custom_control->add(backgrd_right);
    tab_custom_control->add(backgrd_a);
    tab_custom_control->add(backgrd_b);
    tab_custom_control->add(backgrd_x);
    tab_custom_control->add(backgrd_y);
    tab_custom_control->add(backgrd_l);
    tab_custom_control->add(backgrd_r);
    tab_custom_control->add(dropDown_up);
    tab_custom_control->add(dropDown_down);
    tab_custom_control->add(dropDown_left);
    tab_custom_control->add(dropDown_right);
    tab_custom_control->add(dropDown_a);
    tab_custom_control->add(dropDown_b);
    tab_custom_control->add(dropDown_x);
    tab_custom_control->add(dropDown_y);
    tab_custom_control->add(dropDown_l);
    tab_custom_control->add(dropDown_r);
    tab_custom_control->setSize(600,280);
    tab_custom_control->setBaseColor(baseCol);
}


void menuTabCustomCtrl_Exit()
{
    delete tab_custom_control;
    delete group_custom_control_enable;
    delete radioButton_custom_control_on;
    delete radioButton_custom_control_off;
    delete group_dpad;
    delete radioButton_dpad_custom;
    delete radioButton_dpad_joystick;
    delete radioButton_dpad_mouse;
    delete backgrd_up;
    delete backgrd_down;
    delete backgrd_left;
    delete backgrd_right;
    delete backgrd_a;
    delete backgrd_b;
    delete backgrd_x;
    delete backgrd_y;
    delete backgrd_l;
    delete backgrd_r;
    delete label_up;
    delete label_down;
    delete label_left;
    delete label_right;
    delete label_a;
    delete label_b;
    delete label_x;
    delete label_y;
    delete label_l;
    delete label_r;
    delete dropDown_up;
    delete dropDown_down;
    delete dropDown_left;
    delete dropDown_right;
    delete dropDown_a;
    delete dropDown_b;
    delete dropDown_x;
    delete dropDown_y;
    delete dropDown_l;
    delete dropDown_r;

    delete customCtrlActionListener;
    delete dPadActionListener;
    delete comboActionListener;
}


void draw_customcontrol()
{
    if(dropDown_up->getSelected() != mainMenu_custom_up+8)
        dropDown_up->setSelected(mainMenu_custom_up+8);
    if(dropDown_down->getSelected() != mainMenu_custom_down+8)
        dropDown_down->setSelected(mainMenu_custom_down+8);
    if(dropDown_left->getSelected() != mainMenu_custom_left+8)
        dropDown_left->setSelected(mainMenu_custom_left+8);
    if(dropDown_right->getSelected() != mainMenu_custom_right+8)
        dropDown_right->setSelected(mainMenu_custom_right+8);
    if(dropDown_a->getSelected() != mainMenu_custom_A+8)
        dropDown_a->setSelected(mainMenu_custom_A+8);
    if(dropDown_b->getSelected() != mainMenu_custom_B+8)
        dropDown_b->setSelected(mainMenu_custom_B+8);
    if(dropDown_x->getSelected() != mainMenu_custom_X+8)
        dropDown_x->setSelected(mainMenu_custom_X+8);
    if(dropDown_y->getSelected() != mainMenu_custom_Y+8)
        dropDown_y->setSelected(mainMenu_custom_Y+8);
    if(dropDown_l->getSelected() != mainMenu_custom_L+8)
        dropDown_l->setSelected(mainMenu_custom_L+8);
    if(dropDown_r->getSelected() != mainMenu_custom_R+8)
        dropDown_r->setSelected(mainMenu_custom_R+8);
    if (mainMenu_custom_dpad==0) {
        backgrd_up->setVisible(true);
        backgrd_down->setVisible(true);
        backgrd_left->setVisible(true);
        backgrd_right->setVisible(true);
        backgrd_a->setVisible(true);
        backgrd_b->setVisible(true);
        backgrd_x->setVisible(true);
        backgrd_y->setVisible(true);
        backgrd_l->setVisible(true);
        backgrd_r->setVisible(true);
        dropDown_up->setVisible(true);
        dropDown_down->setVisible(true);
        dropDown_left->setVisible(true);
        dropDown_right->setVisible(true);
        dropDown_a->setVisible(true);
        dropDown_b->setVisible(true);
        dropDown_x->setVisible(true);
        dropDown_y->setVisible(true);
        dropDown_l->setVisible(true);
        dropDown_r->setVisible(true);
    } else if (mainMenu_custom_dpad!=0) {
        backgrd_up->setVisible(false);
        backgrd_down->setVisible(false);
        backgrd_left->setVisible(false);
        backgrd_right->setVisible(false);
        backgrd_a->setVisible(true);
        backgrd_b->setVisible(true);
        backgrd_x->setVisible(true);
        backgrd_y->setVisible(true);
        backgrd_l->setVisible(true);
        backgrd_r->setVisible(true);
        dropDown_up->setVisible(false);
        dropDown_down->setVisible(false);
        dropDown_left->setVisible(false);
        dropDown_right->setVisible(false);
        dropDown_a->setVisible(true);
        dropDown_b->setVisible(true);
        dropDown_x->setVisible(true);
        dropDown_y->setVisible(true);
        dropDown_l->setVisible(true);
        dropDown_r->setVisible(true);
    }
}


void show_settings_TabCustomCtrl()
{
    if (mainMenu_customControls==1)
        radioButton_custom_control_on->setSelected(true);
    else if (mainMenu_customControls==0)
        radioButton_custom_control_off->setSelected(true);

    if (mainMenu_custom_dpad==0)
        radioButton_dpad_custom->setSelected(true);
    else if (mainMenu_custom_dpad==1)
        radioButton_dpad_joystick->setSelected(true);
    else if (mainMenu_custom_dpad==2)
        radioButton_dpad_mouse->setSelected(true);

    draw_customcontrol();
}

}

