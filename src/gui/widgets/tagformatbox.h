/**
 * \file tagformatbox.h
 * Group box containing tag format options.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 12 Nov 2017
 *
 * Copyright (C) 2017  Urs Fleisch
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

#ifndef TAGFORMATBOX_H
#define TAGFORMATBOX_H

#include "formatbox.h"

/**
 * Group box containing tag format options.
 */
class TagFormatBox : public FormatBox {
  Q_OBJECT
public:
  /**
   * Constructor.
   *
   * @param title  title
   * @param parent parent widget
   */
  explicit TagFormatBox(const QString& title, QWidget* parent = 0);

  /**
   * Destructor.
   */
  virtual ~TagFormatBox();

  /**
   * Set the values from a format configuration.
   *
   * @param cfg format configuration
   */
  virtual void fromFormatConfig(const FormatConfig& cfg);

  /**
   * Store the values in a format configuration.
   *
   * @param cfg format configuration
   */
  virtual void toFormatConfig(FormatConfig& cfg) const;

private:
  QCheckBox* m_validationCheckBox;
};

#endif // TAGFORMATBOX_H
