import QtQuick 2.2
import ".."

DropDownList {
  id: dropDownList

  property ActionList actions

  dropDownRoot: root
  width: constants.gu(25)
  model: actions.items
  onStateChanged: {
    if (state === "dropDown") {
      // Called before parent change, move popup menu to left of parent.
      x = -width
    }
  }
}
