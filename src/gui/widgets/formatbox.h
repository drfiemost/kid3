/**
 * \file formatbox.h
 * Group box containing format options.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 17 Sep 2003
 *
 * Copyright (C) 2003-2012  Urs Fleisch
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

#ifndef FORMATBOX_H
#define FORMATBOX_H

#include <QGroupBox>

class QComboBox;
class QCheckBox;
class QString;
class FormatConfig;
class ConfigTable;
class ConfigTableModel;

/**
 * Group box containing format options.
 */
class FormatBox : public QGroupBox {
  Q_OBJECT
public:
  /**
   * Constructor.
   *
   * @param title  title
   * @param parent parent widget
   */
  explicit FormatBox(const QString& title, QWidget* parent = 0);

  /**
   * Destructor.
   */
  virtual ~FormatBox();

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
  QComboBox* m_caseConvComboBox;
#if QT_VERSION >= 0x040800
  QComboBox* m_localeComboBox;
#endif
  QCheckBox* m_strRepCheckBox;
  ConfigTable* m_strReplTable;
  ConfigTableModel* m_strReplTableModel;
  QCheckBox* m_formatEditingCheckBox;
};

#endif
