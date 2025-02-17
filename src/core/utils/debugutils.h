/**
 * \file debugutils.h
 * Utility functions for debugging.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 13 Jan 2013
 *
 * Copyright (C) 2013  Urs Fleisch
 *
 * This file is part of Kid3.
 *
 * Kid3 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Kid3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DEBUGUTILS_H
#define DEBUGUTILS_H

#include <QObject>
#include <QModelIndex>
#include "kid3api.h"

class QAbstractItemModel;

namespace DebugUtils {

/**
 * Displays information about signals emitted by an object.
 */
class KID3_CORE_EXPORT SignalEmissionDumper : public QObject {
  Q_OBJECT
public:
  /**
   * Constructor.
   * @param parent parent object
   */
  explicit SignalEmissionDumper(QObject* parent = 0);

  /**
   * Destructor.
   */
  virtual ~SignalEmissionDumper();

  /**
   * Monitor signal emissions of object
   * @param obj object to monitor
   */
  void connectObject(QObject* obj);

private slots:
  void printSignal();
};

#ifndef QT_NO_DEBUG

/**
 * Dump an item model.
 * @param model item model to dump
 * @param parent parent model index
 * @param indent number of spaces to indent
 */
void KID3_CORE_EXPORT dumpModel(const QAbstractItemModel& model,
               const QModelIndex& parent = QModelIndex(), int indent = 0);

#endif

}

#endif // DEBUGUTILS_H
