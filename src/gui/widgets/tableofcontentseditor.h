/**
 * \file tableofcontentseditor.h
 * Editor for table of contents frames.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 18 Sep 2015
 *
 * Copyright (C) 2015  Urs Fleisch
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

#ifndef TABLEOFCONTENTSEDITOR_H
#define TABLEOFCONTENTSEDITOR_H

#include <QWidget>

class QCheckBox;
class QStringListModel;

/**
 * Editor for table of contents frames.
 */
class TableOfContentsEditor : public QWidget {
  Q_OBJECT
public:
  /**
   * Constructor.
   *
   * @param parent parent widget
   */
  explicit TableOfContentsEditor(QWidget* parent = 0);

  /**
   * Destructor.
   */
  virtual ~TableOfContentsEditor();

  /**
   * Set chapters in table of contents.
   * @param isTopLevel true if top level
   * @param isOrdered true if contents are ordered
   * @param elements list of child element IDs
   */
  void setValues(bool isTopLevel, bool isOrdered, const QStringList& elements);

  /**
   * @brief TableOfContentsEditor::getValues
   * @param isTopLevel true is returned here if top level
   * @param isOrdered true is returned here contents are ordered
   * @return list of child element IDs.
   */
  QStringList getValues(bool& isTopLevel, bool& isOrdered) const;

private:
  QStringListModel* m_elementsModel;
  QCheckBox* m_isTopLevelCheckBox;
  QCheckBox* m_isOrderedCheckBox;
};

#endif // TABLEOFCONTENTSEDITOR_H
