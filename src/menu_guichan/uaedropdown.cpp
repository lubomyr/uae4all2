#include "uaedropdown.hpp"
#include "guichan/widgets/dropdown.hpp"

#include "guichan/font.hpp"
#include "guichan/graphics.hpp"
#include "guichan/key.hpp"
#include "guichan/mouseinput.hpp"


namespace gcn
{
    UaeDropDown::UaeDropDown(ListModel *listModel,
                  ScrollArea *scrollArea,
                  ListBox *listBox)
      : DropDown(listModel, scrollArea, listBox)
    {
      mScrollArea->setScrollbarWidth(20);
    }


    UaeDropDown::~UaeDropDown()
    {
    }
}
