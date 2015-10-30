/**
 * \file RaisableRectangle.qml
 * Rectangle which can be raised by click.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 16 Feb 2015
 *
 * Copyright (C) 2015  Urs Fleisch
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

import QtQuick 2.2

Rectangle {
  signal clicked

  property bool raised: true

  z: raised ? 1 : 0

  Rectangle {
    id: darkOverlay
    anchors.fill: parent
    color: "#80000000"
    z: parent.raised ? 0 : 1
    visible: !parent.raised
  }

  MouseArea {
    z: parent.raised ? 0 : 1

    anchors.fill: parent
    onClicked: {
      parent.clicked()
    }
  }
}
