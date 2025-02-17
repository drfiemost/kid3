/**
 * \file filelist.h
 * List of files to operate on.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 9 Jan 2003
 *
 * Copyright (C) 2003-2014  Urs Fleisch
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

#ifndef FILELIST_H
#define FILELIST_H

#include <QList>
#include <QMap>
#include "configurabletreeview.h"
#include "config.h"
#include "kid3api.h"

class BaseMainWindowImpl;
class ExternalProcess;

/**
 * List of files to operate on.
 */
class KID3_GUI_EXPORT FileList : public ConfigurableTreeView {
Q_OBJECT

public:
  /**
   * Constructor.
   * @param parent parent widget
   * @param mainWin main window
   */
  FileList(QWidget* parent, BaseMainWindowImpl* mainWin);

  /**
   * Destructor.
   */
  virtual ~FileList();

  /**
   * Returns the recommended size for the widget.
   * @return recommended size.
   */
  virtual QSize sizeHint() const;

  /**
   * Set rename action.
   * @param action rename action
   */
  void setRenameAction(QAction* action);

  /**
   * Set delete action.
   * @param action delete action
   */
  void setDeleteAction(QAction* action);

public slots:
  /**
   * Init the user actions for the context menu.
   */
  void initUserActions();

signals:
  /**
   * Emitted when a user action is added.
   * @param name name of action
   * @param action action added
   */
  void userActionAdded(const QString& name, QAction* action);

  /**
   * Emitted when a user action is removed.
   * @param name name of action
   * @param action action removed
   */
  void userActionRemoved(const QString& name, QAction* action);

private slots:
  /**
   * Display a context menu with operations for selected files.
   *
   * @param index index of item
   * @param pos   position where context menu is drawn on screen
   */
  void contextMenu(const QModelIndex& index, const QPoint& pos);

  /**
   * Execute a context menu command.
   *
   * @param id command ID
   */
  void executeContextCommand(int id);

  /**
   * Execute a context menu action.
   *
   * @param action action of selected menu, 0 to use sender() action
   */
  void executeAction(QAction* action = 0);

  /**
   * Display a custom context menu with operations for selected files.
   *
   * @param pos  position where context menu is drawn on screen
   */
  void customContextMenu(const QPoint& pos);

#if defined HAVE_PHONON || QT_VERSION >= 0x050000
  /**
   * Play item if it is a tagged file.
   *
   * @param index model index of item
   */
  void playIfTaggedFile(const QModelIndex& index);
#endif

  /**
   * Open with standard application.
   */
  void openFile();

  /**
   * Open containing folder.
   */
  void openContainingFolder();

private:
  Q_DISABLE_COPY(FileList)

  /**
   * Format a string list from the selected files.
   * Supported format fields:
   * Those supported by FrameFormatReplacer::getReplacement(),
   * when prefixed with u, encoded as URL
   * %f filename
   * %F list of files
   * %uf URL of single file
   * %uF list of URLs
   * %d directory name
   * %b the web browser set in the configuration
   *
   * @todo %f and %F are full paths, which is inconsistent with the
   * export format strings but compatible with .desktop files.
   * %d is duration in export format.
   * The export codes should be changed.
   *
   * @param format format specification
   *
   * @return formatted string list.
   */
  QStringList formatStringList(const QStringList& format);

  /** Process for context menu commands */
  ExternalProcess* m_process;
  BaseMainWindowImpl* m_mainWin;
  QAction* m_renameAction;
  QAction* m_deleteAction;
  QMap<QString, QAction*> m_userActions;
};

#endif // FILELIST_H
