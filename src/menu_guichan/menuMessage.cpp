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
#include "gui.h"


namespace widgets
{
static void unraise_Message();

extern gcn::Color baseCol;
extern gcn::Button* button_reset;

gcn::Window *window_warning;
gcn::Button* button_warning;
gcn::Label* label_text;
gcn::Label* label2_text;

gcn::Widget* activateAfterMsg = NULL;


class WarningButtonActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) {
        unraise_Message();
    }
};
WarningButtonActionListener* warningButtonActionListener;


void menuMessage_Init()
{
    button_warning = new gcn::Button(" Ok ");
    button_warning->setPosition(115,45);
    button_warning->setSize(70, 26);
    button_warning->setBaseColor(baseCol);
    warningButtonActionListener = new WarningButtonActionListener();
    button_warning->addActionListener(warningButtonActionListener);

    label_text = new gcn::Label("");
    label_text->setPosition(4, 4);
    label_text->setSize(290, 15);

    label2_text = new gcn::Label("");
    label2_text->setPosition(4, 23);
    label2_text->setSize(430, 15);

    window_warning = new gcn::Window("Warning");
    window_warning->add(button_warning);
    window_warning->add(label_text);
    window_warning->add(label2_text);
    window_warning->setMovable(false);
    window_warning->setSize(300,95);
    window_warning->setBaseColor(baseCol);
    window_warning->setVisible(false);
}


void menuMessage_Exit()
{
    delete button_warning;
    delete label_text;
    delete label2_text;

    delete warningButtonActionListener;

    delete window_warning;
}


static void unraise_Message()
{
    window_warning->releaseModalFocus();
    window_warning->setVisible(false);
    if(activateAfterMsg != NULL)
        activateAfterMsg->requestFocus();
    else
        button_reset->requestFocus();
    activateAfterMsg = NULL;
}


void adjustSize(const char *msg2)
{
    if(msg2 == "") {
        window_warning->setPosition(210, 220);
        window_warning->setSize(240,95);
        button_warning->setPosition(85,45);
    } else {
        window_warning->setPosition(100, 220);
        window_warning->setSize(440,95);
        button_warning->setPosition(185,45);
    }
}


void showInfo(const char *msg, const char *msg2 = "")
{
    label_text->setCaption(msg);
    label2_text->setCaption(msg2);
    adjustSize(msg2);
    window_warning->setCaption("Information");
    window_warning->setVisible(true);
    window_warning->requestModalFocus();
    button_warning->requestFocus();
}


void showWarning(const char *msg, const char *msg2 = "")
{
    label_text->setCaption(msg);
    label2_text->setCaption(msg2);
    adjustSize(msg2);
    window_warning->setCaption("Warning");
    window_warning->setVisible(true);
    window_warning->requestModalFocus();
    button_warning->requestFocus();
}


void show_error(const char *msg, const char *msg2 = "")
{
    label_text->setCaption(msg);
    label2_text->setCaption(msg2);
    adjustSize(msg2);
    window_warning->setCaption("Error");
    window_warning->setVisible(true);
    window_warning->requestModalFocus();
    button_warning->requestFocus();
}

}
