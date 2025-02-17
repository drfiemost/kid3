/**
 * \file qmlcommandplugin.h
 * Starter for QML scripts.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 15 Feb 2015
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

#ifndef QMLCOMMANDPLUGIN_H
#define QMLCOMMANDPLUGIN_H

#include <QObject>
#if QT_VERSION < 0x050000
#include <QDeclarativeView>
#endif
#include "iusercommandprocessor.h"

class Kid3Application;
#if QT_VERSION >= 0x050000
class QQuickView;
class QQmlEngine;
class QQmlError;
#else
class QDeclarativeEngine;
class QQuickCloseEvent;
#endif

/**
 * Starter for QML scripts.
 */
class KID3_PLUGIN_EXPORT QmlCommandPlugin :
    public QObject, public IUserCommandProcessor {
  Q_OBJECT
#if QT_VERSION >= 0x050000
  Q_PLUGIN_METADATA(IID "net.sourceforge.kid3.IUserCommandProcessor")
#endif
  Q_INTERFACES(IUserCommandProcessor)
public:
  /**
   * Constructor.
   *
   * @param parent parent object
   */
  explicit QmlCommandPlugin(QObject* parent = 0);

  /**
   * Destructor.
   */
  virtual ~QmlCommandPlugin();

  /**
   * Get keys of available user commands.
   * @return list of keys, ["qml", "qmlview"].
   */
  virtual QStringList userCommandKeys() const;

  /**
   * Initialize processor.
   * This method must be invoked before the first call to startUserCommand()
   * to set the application context.
   * @param app application context
   */
  virtual void initialize(Kid3Application* app);

  /**
   * Cleanup processor.
   * This method must be invoked to close and delete the QML resources.
   */
  virtual void cleanup();

  /**
   * Start a QML script.
   * @param key user command name, "qml" or "qmlview"
   * @param arguments arguments to pass to script
   * @param showOutput true to enable output in output viewer, using signal
   *                   commandOutput().
   * @return true if command is started.
   */
  virtual bool startUserCommand(
      const QString& key, const QStringList& arguments, bool showOutput);

  /**
   * Return object which emits commandOutput() signal.
   * @return this.
   */
  virtual QObject* qobject();

signals:
  /**
   * Emitted when output is enabled and a QML message is generated.
   * @param msg message from QML, error or console output
   */
  void commandOutput(const QString& msg);

private slots:
#if QT_VERSION >= 0x050000
  void onEngineError(const QList<QQmlError>& errors);
#else
  void onEngineError(const QList<QDeclarativeError>& errors);
#endif
  void onQmlViewClosing();
  void onQmlViewFinished();
  void onQmlEngineQuit();
  void onEngineFinished();

private:
#if QT_VERSION >= 0x050000
  void setupQmlEngine(QQmlEngine* engine);
#else
  void setupQmlEngine(QDeclarativeEngine* engine);
#endif
  void onEngineReady();

#if QT_VERSION >= 0x050000
  static void messageHandler(QtMsgType type, const QMessageLogContext& context,
                             const QString& msg);
#else
  static void messageHandler(QtMsgType type, const char* msg);
#endif

  Kid3Application* m_app;
#if QT_VERSION >= 0x050000
  QQuickView* m_qmlView;
  QQmlEngine* m_qmlEngine;
#else
  QDeclarativeView* m_qmlView;
  QDeclarativeEngine* m_qmlEngine;
#endif
  bool m_showOutput;

  static QmlCommandPlugin* s_messageHandlerInstance;
};

#if QT_VERSION < 0x050000
/**
 * QDeclarativeView with a closing signal.
 */
class QmlView : public QDeclarativeView {
  Q_OBJECT
public:
  /**
   * Constructor.
   * @param parent parent widget
   */
  explicit QmlView(QWidget* parent = 0);

  /**
   * Destructor.
   */
  virtual ~QmlView();

signals:
  /**
   * Emitted when window is closed.
   * @param ev close event, always 0, just for compatibility with Qt 5
   */
  void closing(QQuickCloseEvent* ev);

protected:
  /**
   * Handle close event.
   * @param ev close event
   */
  virtual void closeEvent(QCloseEvent* ev);
};
#endif

#endif // QMLCOMMANDPLUGIN_H
