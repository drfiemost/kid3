/**
 * \file MessageDialog.qml
 * Message dialog.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 16 Feb 2015
 *
 * Copyright (C) 2015-2018  Urs Fleisch
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.9
import QtQuick.Controls 2.2

Dialog {
  id: page

  property alias text: msg.text
  signal yes
  signal no
  signal cancel

  width: 400
  height: 300
  modal: true
  standardButtons: Dialog.Yes | Dialog.No | Dialog.Cancel

  Connections {
    target: footer
    onClicked: {
      switch (button.DialogButtonBox.buttonRole) {
      case DialogButtonBox.YesRole:
        yes()
        break;
      case DialogButtonBox.NoRole:
        no()
        break;
      case DialogButtonBox.RejectRole:
        cancel()
        break;
      }
    }
  }

  contentItem: Label {
    id: msg
  }
}
