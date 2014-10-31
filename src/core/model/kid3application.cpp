/**
 * \file kid3application.cpp
 * Kid3 application logic, independent of GUI.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 10 Jul 2011
 *
 * Copyright (C) 2011-2013  Urs Fleisch
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

#include "kid3application.h"
#include <QFileSystemModel>
#include <QItemSelectionModel>
#include <QTextCodec>
#include <QUrl>
#include <QTextStream>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QApplication>
#include <QClipboard>
#include <QPluginLoader>
#ifdef HAVE_QTDBUS
#include <QDBusConnection>
#include <unistd.h>
#include "scriptinterface.h"
#endif
#include "icoreplatformtools.h"
#include "fileproxymodel.h"
#include "fileproxymodeliterator.h"
#include "dirproxymodel.h"
#include "modeliterator.h"
#include "trackdatamodel.h"
#include "frametablemodel.h"
#include "timeeventmodel.h"
#include "framelist.h"
#include "pictureframe.h"
#include "textimporter.h"
#include "textexporter.h"
#include "dirrenamer.h"
#include "configstore.h"
#include "formatconfig.h"
#include "tagconfig.h"
#include "fileconfig.h"
#include "importconfig.h"
#include "guiconfig.h"
#include "playlistconfig.h"
#include "playlistcreator.h"
#include "downloadclient.h"
#include "iframeeditor.h"
#include "batchimportprofile.h"
#include "batchimporter.h"
#include "iserverimporterfactory.h"
#include "iservertrackimporterfactory.h"
#include "itaggedfilefactory.h"
#if defined HAVE_PHONON || QT_VERSION >= 0x050000
#include "audioplayer.h"
#endif

#include "importplugins.h"

namespace {

/**
 * Get the file name of the plugin from the plugin name.
 * @param pluginName name of the plugin
 * @return file name.
 */
QString pluginFileName(const QString& pluginName)
{
  QString fileName = pluginName.toLower();
#ifdef Q_OS_WIN32
#ifdef Q_CC_MSVC
  fileName += QLatin1String(".dll");
#else
  fileName = QLatin1String("lib") + fileName + QLatin1String(".dll");
#endif
#elif defined Q_OS_MAC
  fileName = QLatin1String("lib") + fileName + QLatin1String(".dylib");
#else
  fileName = QLatin1String("lib") + fileName + QLatin1String(".so");
#endif
  return fileName;
}

}


/** Current directory */
QString Kid3Application::s_dirName;

/**
 * Constructor.
 * @param platformTools platform tools
 * @param parent parent object
 */
Kid3Application::Kid3Application(ICorePlatformTools* platformTools,
                                 QObject* parent) : QObject(parent),
  m_platformTools(platformTools),
  m_fileSystemModel(new QFileSystemModel(this)),
  m_fileProxyModel(new FileProxyModel(this)),
  m_fileProxyModelIterator(new FileProxyModelIterator(m_fileProxyModel)),
  m_dirProxyModel(new DirProxyModel(this)),
  m_fileSelectionModel(new QItemSelectionModel(m_fileProxyModel, this)),
  m_trackDataModel(new TrackDataModel(this)),
  m_framesV1Model(new FrameTableModel(true, this)),
  m_framesV2Model(new FrameTableModel(false, this)),
  m_framesV1SelectionModel(new QItemSelectionModel(m_framesV1Model, this)),
  m_framesV2SelectionModel(new QItemSelectionModel(m_framesV2Model, this)),
  m_framelist(new FrameList(m_framesV2Model, m_framesV2SelectionModel)),
  m_configStore(new ConfigStore(m_platformTools->applicationSettings())),
  m_netMgr(new QNetworkAccessManager(this)),
  m_downloadClient(new DownloadClient(m_netMgr)),
  m_textExporter(new TextExporter(this)),
  m_tagSearcher(new TagSearcher(this)),
  m_dirRenamer(new DirRenamer(this)),
  m_batchImporter(new BatchImporter(m_netMgr)),
#if defined HAVE_PHONON || QT_VERSION >= 0x050000
  m_player(0),
#endif
  m_expressionFileFilter(0),
  m_downloadImageDest(ImageForSelectedFiles),
  m_selectionSingleFile(0),
  m_selectionTagV1SupportedCount(0), m_selectionFileCount(0),
  m_fileFilter(0),
  m_batchImportProfile(0), m_batchImportTagVersion(TrackData::TagNone),
  m_editFrameTaggedFile(0), m_addFrameTaggedFile(0),
  m_modified(false), m_filtered(false),
  m_selectionHasTagV1(false), m_selectionHasTagV2(false)
{
  setObjectName(QLatin1String("Kid3Application"));
  m_fileProxyModel->setSourceModel(m_fileSystemModel);
  m_dirProxyModel->setSourceModel(m_fileSystemModel);

  connect(m_fileSelectionModel,
          SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(fileSelected()));

  initPlugins();
  m_batchImporter->setImporters(m_importers, m_trackDataModel);

#ifdef HAVE_QTDBUS
  if (QDBusConnection::sessionBus().isConnected()) {
    QString serviceName(QLatin1String("net.sourceforge.kid3"));
    QDBusConnection::sessionBus().registerService(serviceName);
    // For the case of multiple Kid3 instances running, register also a service
    // with the PID appended. On KDE such a service is already registered but
    // the call to registerService() seems to succeed nevertheless.
    serviceName += QLatin1Char('-');
    serviceName += QString::number(::getpid());
    QDBusConnection::sessionBus().registerService(serviceName);
    new ScriptInterface(this);
    if (!QDBusConnection::sessionBus().registerObject(QLatin1String("/Kid3"), this)) {
      qWarning("Registering D-Bus object failed");
    }
  } else {
    qWarning("Cannot connect to the D-BUS session bus.");
  }
#endif
}

/**
 * Destructor.
 */
Kid3Application::~Kid3Application()
{
  delete m_configStore;
#if defined Q_OS_MAC && QT_VERSION >= 0x050000
  // If a song is played, then stopped and Kid3 is terminated, it will crash in
  // the QMediaPlayer destructor (Dispatch queue: com.apple.main-thread,
  // objc_msgSend() selector name: setRate). Avoid calling the destructor by
  // setting the QMediaPlayer's parent to null. See also:
  // https://qt-project.org/forums/viewthread/29651
  if (m_player) {
    m_player->setParent(0);
  }
#endif
}

/**
 * Load and initialize plugins depending on configuration.
 */
void Kid3Application::initPlugins()
{
  // Load plugins, set information about plugins in configuration.
  ImportConfig& importCfg = ImportConfig::instance();
  TagConfig& tagCfg = TagConfig::instance();
  importCfg.availablePlugins().clear();
  tagCfg.availablePlugins().clear();
  foreach (QObject* plugin, loadPlugins()) {
    checkPlugin(plugin);
  }
  // Order the meta data plugins as configured.
  QStringList pluginOrder = tagCfg.pluginOrder();
  if (!pluginOrder.isEmpty()) {
    QList<ITaggedFileFactory*> orderedFactories;
    for (int i = 0; i < pluginOrder.size(); ++i) {
      orderedFactories.append(0);
    }
    foreach (ITaggedFileFactory* factory, FileProxyModel::taggedFileFactories()) {
      int idx = pluginOrder.indexOf(factory->name());
      if (idx >= 0) {
        orderedFactories[idx] = factory;
      } else {
        orderedFactories.append(factory);
      }
    }
    orderedFactories.removeAll(0);
#if QT_VERSION >= 0x040800
    FileProxyModel::taggedFileFactories().swap(orderedFactories);
#else
    FileProxyModel::taggedFileFactories() = orderedFactories;
#endif
  }
}

/**
 * Load plugins.
 * @return list of plugin instances.
 */
QObjectList Kid3Application::loadPlugins()
{
  QObjectList plugins = QPluginLoader::staticInstances();

  // First check if we are running from the build directory to load the
  // plugins from there.
  QDir pluginsDir(qApp->applicationDirPath());
  QString dirName = pluginsDir.dirName();
#ifdef Q_OS_WIN
  QString buildType;
  if (dirName.compare(QLatin1String("debug"), Qt::CaseInsensitive) == 0 ||
      dirName.compare(QLatin1String("release"), Qt::CaseInsensitive) == 0) {
    buildType = dirName;
    pluginsDir.cdUp();
    dirName = pluginsDir.dirName();
  }
#endif
  bool pluginsDirFound = pluginsDir.cd(QLatin1String(
      (dirName == QLatin1String("qt") || dirName == QLatin1String("kde") ||
       dirName == QLatin1String("cli"))
      ? "../../plugins"
      : dirName == QLatin1String("test")
        ? "../plugins"
        : CFG_PLUGINSDIR));
#ifdef Q_OS_MAC
  if (!pluginsDirFound) {
    pluginsDirFound = pluginsDir.cd(QLatin1String("../../../../../plugins"));
  }
#endif
  if (pluginsDirFound) {
#ifdef Q_OS_WIN
    if (!buildType.isEmpty()) {
      pluginsDir.cd(buildType);
    }
#endif
    ImportConfig& importCfg = ImportConfig::instance();
    TagConfig& tagCfg = TagConfig::instance();

    // Construct a set of disabled plugin file names
    QMap<QString, QString> disabledImportPluginFileNames;
    foreach (const QString& pluginName, importCfg.m_disabledPlugins) {
      disabledImportPluginFileNames.insert(pluginFileName(pluginName),
                                           pluginName);
    }
    QMap<QString, QString> disabledTagPluginFileNames;
    QStringList disabledTagPlugins = tagCfg.disabledPlugins();
    foreach (const QString& pluginName, disabledTagPlugins) {
      disabledTagPluginFileNames.insert(pluginFileName(pluginName),
                                        pluginName);
    }

    foreach (const QString& fileName, pluginsDir.entryList(QDir::Files)) {
      if (disabledImportPluginFileNames.contains(fileName)) {
        importCfg.availablePlugins().append(
              disabledImportPluginFileNames.value(fileName));
        continue;
      }
      if (disabledTagPluginFileNames.contains(fileName)) {
        tagCfg.availablePlugins().append(
              disabledTagPluginFileNames.value(fileName));
        continue;
      }
      QPluginLoader loader(pluginsDir.absoluteFilePath(fileName));
      QObject* plugin = loader.instance();
      if (plugin) {
        QString name(plugin->objectName());
        if (importCfg.m_disabledPlugins.contains(name)) {
          importCfg.availablePlugins().append(name);
          loader.unload();
        } else if (disabledTagPlugins.contains(name)) {
          tagCfg.availablePlugins().append(name);
          loader.unload();
        } else {
          plugins.append(plugin);
        }
      }
    }
  }
  return plugins;
}

/**
 * Check type of a loaded plugin and register it.
 * @param plugin instance returned by plugin loader
 */
void Kid3Application::checkPlugin(QObject* plugin)
{
  if (IServerImporterFactory* importerFactory =
      qobject_cast<IServerImporterFactory*>(plugin)) {
    ImportConfig& importCfg = ImportConfig::instance();
    importCfg.availablePlugins().append(plugin->objectName());
    if (!importCfg.m_disabledPlugins.contains(plugin->objectName())) {
      foreach (const QString& key, importerFactory->serverImporterKeys()) {
        m_importers.append(importerFactory->createServerImporter(
                             key, m_netMgr, m_trackDataModel));
      }
    }
  }
  if (IServerTrackImporterFactory* importerFactory =
      qobject_cast<IServerTrackImporterFactory*>(plugin)) {
    ImportConfig& importCfg = ImportConfig::instance();
    importCfg.availablePlugins().append(plugin->objectName());
    if (!importCfg.m_disabledPlugins.contains(plugin->objectName())) {
      foreach (const QString& key, importerFactory->serverTrackImporterKeys()) {
        m_trackImporters.append(importerFactory->createServerTrackImporter(
                             key, m_netMgr, m_trackDataModel));
      }
    }
  }
  if (ITaggedFileFactory* taggedFileFactory =
      qobject_cast<ITaggedFileFactory*>(plugin)) {
    TagConfig& tagCfg = TagConfig::instance();
    tagCfg.availablePlugins().append(plugin->objectName());
    if (!tagCfg.disabledPlugins().contains(plugin->objectName())) {
      int features = tagCfg.taggedFileFeatures();
      foreach (const QString& key, taggedFileFactory->taggedFileKeys()) {
        taggedFileFactory->initialize(key);
        features |= taggedFileFactory->taggedFileFeatures(key);
      }
      tagCfg.setTaggedFileFeatures(features);
      FileProxyModel::taggedFileFactories().append(taggedFileFactory);
    }
  }
}

#if defined HAVE_PHONON || QT_VERSION >= 0x050000
/**
 * Get audio player.
 * @return audio player.
 */
AudioPlayer* Kid3Application::getAudioPlayer()
{
  if (!m_player) {
    m_player = new AudioPlayer(this);
  }
  return m_player;
}
#endif

/**
 * Get settings.
 * @return settings.
 */
ISettings* Kid3Application::getSettings() const
{
  return m_platformTools->applicationSettings();
}

/**
 * Save settings to the configuration.
 */
void Kid3Application::saveConfig()
{
  if (FileConfig::instance().m_loadLastOpenedFile) {
    FileConfig::instance().m_lastOpenedFile =
        m_fileProxyModel->filePath(currentOrRootIndex());
  }
  m_configStore->writeToConfig();
  getSettings()->sync();
}

/**
 * Read settings from the configuration.
 */
void Kid3Application::readConfig()
{
  if (FileConfig::instance().m_nameFilter.isEmpty()) {
    FileConfig::instance().m_nameFilter = createFilterString();
  }
  notifyConfigurationChange();
  FrameCollection::setQuickAccessFrames(
        TagConfig::instance().quickAccessFrames());
}

/**
 * Open directory.
 * When finished directoryOpened() is emitted, also if false is returned.
 *
 * @param paths file or directory paths, if multiple paths are given, the
 * common directory is opened and the files are selected
 * @param fileCheck if true, only open directory if paths exist
 *
 * @return true if ok.
 */
bool Kid3Application::openDirectory(const QStringList& paths, bool fileCheck)
{
  bool ok = true;
  QStringList filePaths;
  QStringList dirComponents;
  foreach (const QString& path, paths) {
    if (!path.isEmpty()) {
      QFileInfo fileInfo(path);
      if (fileCheck && !fileInfo.exists()) {
        ok = false;
        break;
      }
      QString dirPath;
      if (!fileInfo.isDir()) {
        dirPath = fileInfo.absolutePath();
        if (fileInfo.isFile()) {
          filePaths.append(fileInfo.absoluteFilePath());
        }
      } else {
        dirPath = QDir(path).absolutePath();
      }
      QStringList dirPathComponents = dirPath.split(QDir::separator());
      if (dirComponents.isEmpty()) {
        dirComponents = dirPathComponents;
      } else {
        // Reduce dirPath to common prefix.
        QStringList::iterator dirIt = dirComponents.begin();
        QStringList::const_iterator dirPathIt = dirPathComponents.constBegin();
        while (dirIt != dirComponents.end() &&
               dirPathIt != dirPathComponents.constEnd() &&
               *dirIt == *dirPathIt) {
          ++dirIt;
          ++dirPathIt;
        }
        dirComponents.erase(dirIt, dirComponents.end());
      }
    }
  }
  QString dir;
  if (ok) {
    dir = dirComponents.join(QDir::separator());
    if (dir.isEmpty() && !filePaths.isEmpty()) {
      dir = QDir::rootPath();
    }
    ok = !dir.isEmpty();
  }
  QModelIndex rootIndex;
  QModelIndexList fileIndexes;
  if (ok) {
    QStringList nameFilters(m_platformTools->getNameFilterPatterns(
                              FileConfig::instance().m_nameFilter).
                            split(QLatin1Char(' ')));
    m_fileProxyModel->setNameFilters(nameFilters);
    m_fileSystemModel->setFilter(QDir::AllEntries | QDir::AllDirs);
    rootIndex = m_fileSystemModel->setRootPath(dir);
    foreach (const QString& filePath, filePaths) {
      fileIndexes.append(m_fileSystemModel->index(filePath));
    }
    ok = rootIndex.isValid();
  }
  if (ok) {
    setModified(false);
    setFiltered(false);
    setDirName(dir);
    QModelIndex oldRootIndex = m_fileProxyModelRootIndex;
    m_fileProxyModelRootIndex = m_fileProxyModel->mapFromSource(rootIndex);
    m_fileProxyModelFileIndexes.clear();
    foreach (const QModelIndex& fileIndex, fileIndexes) {
      m_fileProxyModelFileIndexes.append(
            m_fileProxyModel->mapFromSource(fileIndex));
    }
    if (m_fileProxyModelRootIndex != oldRootIndex) {
      connect(m_fileProxyModel, SIGNAL(sortingFinished()),
              this, SLOT(onDirectoryLoaded()));
    } else {
      QTimer::singleShot(0, this, SLOT(emitDirectoryOpened()));
    }
  }
  if (!ok) {
    QTimer::singleShot(0, this, SLOT(emitDirectoryOpened()));
  }
  return ok;
}

/**
 * Emit directoryOpened().
 */
void Kid3Application::emitDirectoryOpened()
{
  emit directoryOpened(m_fileProxyModelRootIndex, m_fileProxyModelFileIndexes);
}

/**
 * Called when the gatherer thread has finished to load the directory.
 */
void Kid3Application::onDirectoryLoaded()
{
  disconnect(m_fileProxyModel, SIGNAL(sortingFinished()),
             this, SLOT(onDirectoryLoaded()));
  emitDirectoryOpened();
}

/**
 * Get directory path of opened directory.
 * @return directory path.
 */
QString Kid3Application::getDirPath() const
{
  return FileProxyModel::getPathIfIndexOfDir(m_fileProxyModelRootIndex);
}

/**
 * Get current index in file proxy model or root index if current index is
 * invalid.
 * @return current index, root index if not valid.
 */
QModelIndex Kid3Application::currentOrRootIndex() const
{
  QModelIndex index(m_fileSelectionModel->currentIndex());
  if (index.isValid())
    return index;
  else
    return m_fileProxyModelRootIndex;
}

/**
 * Save all changed files.
 * saveStarted() and saveProgress() are emitted while saving files.
 *
 * @return list of files with error, empty if ok.
 */
QStringList Kid3Application::saveDirectory()
{
  QStringList errorFiles;
  int numFiles = 0, totalFiles = 0;
  // Get number of files to be saved to display correct progressbar
  TaggedFileIterator countIt(m_fileProxyModelRootIndex);
  while (countIt.hasNext()) {
    if (countIt.next()->isChanged()) {
      ++totalFiles;
    }
  }
  emit saveStarted(totalFiles);

  TaggedFileIterator it(m_fileProxyModelRootIndex);
  while (it.hasNext()) {
    TaggedFile* taggedFile = it.next();
    bool renamed = false;
    if (!taggedFile->writeTags(false, &renamed,
                               FileConfig::instance().m_preserveTime)) {
      QString errorMsg = taggedFile->getAbsFilename();
      errorFiles.push_back(errorMsg);
    }
    ++numFiles;
    emit saveProgress(numFiles);
  }

  return errorFiles;
}

/**
 * Update tags of selected files to contain contents of frame models.
 *
 * @param selItems list of selected file indexes
 */
void Kid3Application::frameModelsToTags(
    const QList<QPersistentModelIndex>& selItems)
{
  if (!selItems.isEmpty()) {
    FrameCollection framesV1(m_framesV1Model->getEnabledFrames());
    FrameCollection framesV2(m_framesV2Model->getEnabledFrames());
    for (QList<QPersistentModelIndex>::const_iterator it = selItems.begin();
         it != selItems.end();
         ++it) {
      if (TaggedFile* taggedFile = FileProxyModel::getTaggedFileOfIndex(*it)) {
        taggedFile->setFramesV1(framesV1);
        taggedFile->setFramesV2(framesV2);
      }
    }
  }
}

/**
 * Update frame models to contain contents of selected files.
 * The properties starting with "selection" will be set by this method.
 *
 * @param selItems list of selected file indexes
 */
void Kid3Application::tagsToFrameModels(
    const QList<QPersistentModelIndex>& selItems)
{
  m_selectionSingleFile = 0;
  m_selectionTagV1SupportedCount = 0;
  m_selectionFileCount = 0;
  m_selectionHasTagV1 = false;
  m_selectionHasTagV2 = false;

  for (QList<QPersistentModelIndex>::const_iterator it = selItems.begin();
       it != selItems.end();
       ++it) {
    TaggedFile* taggedFile = FileProxyModel::getTaggedFileOfIndex(*it);
    if (taggedFile) {
      taggedFile->readTags(false);

      taggedFile = FileProxyModel::readWithId3V24IfId3V24(taggedFile);

      if (taggedFile->isTagV1Supported()) {
        if (m_selectionTagV1SupportedCount == 0) {
          FrameCollection frames;
          taggedFile->getAllFramesV1(frames);
          m_framesV1Model->transferFrames(frames);
        } else {
          FrameCollection fileFrames;
          taggedFile->getAllFramesV1(fileFrames);
          m_framesV1Model->filterDifferent(fileFrames);
        }
        ++m_selectionTagV1SupportedCount;
      }
      if (m_selectionFileCount == 0) {
        FrameCollection frames;
        taggedFile->getAllFramesV2(frames);
        m_framesV2Model->transferFrames(frames);
        m_selectionSingleFile = taggedFile;
      } else {
        FrameCollection fileFrames;
        taggedFile->getAllFramesV2(fileFrames);
        m_framesV2Model->filterDifferent(fileFrames);
        m_selectionSingleFile = 0;
      }
      ++m_selectionFileCount;

      m_selectionHasTagV1 = m_selectionHasTagV1 || taggedFile->hasTagV1();
      m_selectionHasTagV2 = m_selectionHasTagV2 || taggedFile->hasTagV2();
    }
  }

  m_framesV1Model->setAllCheckStates(m_selectionTagV1SupportedCount == 1);
  m_framesV2Model->setAllCheckStates(m_selectionFileCount == 1);
  if (GuiConfig::instance().m_autoHideTags) {
    // If a tag is supposed to be absent, make sure that there is really no
    // unsaved data in the tag.
    if (!m_selectionHasTagV1 &&
        (m_selectionTagV1SupportedCount > 0 || m_selectionFileCount == 0)) {
      const FrameCollection& frames = m_framesV1Model->frames();
      for (FrameCollection::const_iterator it = frames.begin();
           it != frames.end();
           ++it) {
        if (!(*it).getValue().isEmpty()) {
          m_selectionHasTagV1 = true;
          break;
        }
      }
    }
    if (!m_selectionHasTagV2) {
      const FrameCollection& frames = m_framesV2Model->frames();
      for (FrameCollection::const_iterator it = frames.begin();
           it != frames.end();
           ++it) {
        if (!(*it).getValue().isEmpty()) {
          m_selectionHasTagV2 = true;
          break;
        }
      }
    }
  }

  if (m_selectionSingleFile) {
    m_framelist->setTaggedFile(m_selectionSingleFile);

    if (TagConfig::instance().markTruncations()) {
      m_framesV1Model->markRows(m_selectionSingleFile->getTruncationFlags());
    }
    if (FileConfig::instance().m_markChanges) {
      m_framesV1Model->markChangedFrames(
        m_selectionSingleFile->getChangedFramesV1());
      m_framesV2Model->markChangedFrames(
        m_selectionSingleFile->getChangedFramesV2());
    }
  } else {
    if (TagConfig::instance().markTruncations()) {
      m_framesV1Model->markRows(0);
    }
    if (FileConfig::instance().m_markChanges) {
      m_framesV1Model->markChangedFrames(0);
      m_framesV2Model->markChangedFrames(0);
    }
  }
}

/**
 * Revert file modifications.
 * Acts on selected files or all files if no file is selected.
 */
void Kid3Application::revertFileModifications()
{
  SelectedTaggedFileIterator it(getRootIndex(),
                                getFileSelectionModel(),
                                true);
  while (it.hasNext()) {
    TaggedFile* taggedFile = it.next();
    taggedFile->readTags(true);
    // update icon
    getFileProxyModel()->emitDataChanged(taggedFile->getIndex(),
                                         taggedFile->getIndex());
  }
  if (!it.hasNoSelection()) {
    emit selectedFilesUpdated();
  }
  else {
    emit fileModified();
  }
}

/**
 * Update modification state from files.
 */
void Kid3Application::updateModified()
{
  TaggedFileIterator it(m_fileProxyModelRootIndex);
  while (it.hasNext()) {
    TaggedFile* taggedFile = it.next();
    if (taggedFile->isChanged()) {
      m_modified = true;
      return;
    }
  }
  m_modified = false;
}

/**
 * Import.
 *
 * @param tagMask tag mask
 * @param path    path of file, "clipboard" for import from clipboard
 * @param fmtIdx  index of format
 *
 * @return true if ok.
 */
bool Kid3Application::importTags(TrackData::TagVersion tagMask,
                                 const QString& path, int fmtIdx)
{
  filesToTrackDataModel(ImportConfig::instance().m_importDest);
  QString text;
  if (path == QLatin1String("clipboard")) {
    QClipboard* cb = QApplication::clipboard();
    text = cb->text(QClipboard::Clipboard);
    if (text.isNull())
      text = cb->text(QClipboard::Selection);
  } else {
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
      text = QTextStream(&file).readAll();
      file.close();
    }
  }
  if (!text.isNull() &&
      fmtIdx < ImportConfig::instance().m_importFormatHeaders.size()) {
    TextImporter(getTrackDataModel()).updateTrackData(
      text,
      ImportConfig::instance().m_importFormatHeaders.at(fmtIdx),
      ImportConfig::instance().m_importFormatTracks.at(fmtIdx));
    trackDataModelToFiles(tagMask);
    return true;
  }
  return false;
}

/**
 * Export.
 *
 * @param tagVersion tag version
 * @param path   path of file, "clipboard" for export to clipboard
 * @param fmtIdx index of format
 *
 * @return true if ok.
 */
bool Kid3Application::exportTags(TrackData::TagVersion tagVersion,
                                 const QString& path, int fmtIdx)
{
  ImportTrackDataVector trackDataVector;
  filesToTrackData(tagVersion, trackDataVector);
  m_textExporter->setTrackData(trackDataVector);
  m_textExporter->updateTextUsingConfig(fmtIdx);
  if (path == QLatin1String("clipboard")) {
    m_textExporter->exportToClipboard();
    return true;
  } else {
    return m_textExporter->exportToFile(path);
  }
}

/**
 * Write playlist according to playlist configuration.
 *
 * @param cfg playlist configuration to use
 *
 * @return true if ok.
 */
bool Kid3Application::writePlaylist(const PlaylistConfig& cfg)
{
  PlaylistCreator plCtr(getDirPath(), cfg);
  QItemSelectionModel* selectModel = getFileSelectionModel();
  bool noSelection = !cfg.m_onlySelectedFiles || !selectModel ||
                     !selectModel->hasSelection();
  bool ok = true;
  QModelIndex rootIndex;

  if (cfg.m_location == PlaylistConfig::PL_CurrentDirectory) {
    // Get first child of parent of current index.
    rootIndex = currentOrRootIndex();
    if (rootIndex.model() && rootIndex.model()->rowCount(rootIndex) <= 0)
      rootIndex = rootIndex.parent();
    if (const QAbstractItemModel* model = rootIndex.model()) {
      for (int row = 0; row < model->rowCount(rootIndex); ++row) {
        QModelIndex index = model->index(row, 0, rootIndex);
        PlaylistCreator::Item plItem(index, plCtr);
        if (plItem.isFile() &&
            (noSelection || selectModel->isSelected(index))) {
          ok = plItem.add() && ok;
        }
      }
    }
  } else {
    QString selectedDirPrefix;
    rootIndex = getRootIndex();
    ModelIterator it(rootIndex);
    while (it.hasNext()) {
      QModelIndex index = it.next();
      PlaylistCreator::Item plItem(index, plCtr);
      bool inSelectedDir = false;
      if (plItem.isDir()) {
        if (!selectedDirPrefix.isEmpty()) {
          if (plItem.getDirName().startsWith(selectedDirPrefix)) {
            inSelectedDir = true;
          } else {
            selectedDirPrefix = QLatin1String("");
          }
        }
        if (inSelectedDir || noSelection || selectModel->isSelected(index)) {
          // if directory is selected, all its files are selected
          if (!inSelectedDir) {
            selectedDirPrefix = plItem.getDirName();
          }
        }
      } else if (plItem.isFile()) {
        QString dirName = plItem.getDirName();
        if (!selectedDirPrefix.isEmpty()) {
          if (dirName.startsWith(selectedDirPrefix)) {
            inSelectedDir = true;
          } else {
            selectedDirPrefix = QLatin1String("");
          }
        }
        if (inSelectedDir || noSelection || selectModel->isSelected(index)) {
          ok = plItem.add() && ok;
        }
      }
    }
  }

  ok = plCtr.write() && ok;
  return ok;
}

/**
 * Write playlist using current playlist configuration.
 *
 * @return true if ok.
 */
bool Kid3Application::writePlaylist()
{
  return writePlaylist(PlaylistConfig::instance());
}

/**
 * Set track data with tagged files of directory.
 *
 * @param tagVersion tag version
 * @param trackDataList is filled with track data
 */
void Kid3Application::filesToTrackData(TrackData::TagVersion tagVersion,
                                       ImportTrackDataVector& trackDataList)
{
  TaggedFileOfDirectoryIterator it(currentOrRootIndex());
  while (it.hasNext()) {
    TaggedFile* taggedFile = it.next();
    taggedFile->readTags(false);
    taggedFile = FileProxyModel::readWithId3V24IfId3V24(taggedFile);
    trackDataList.push_back(ImportTrackData(*taggedFile, tagVersion));
  }
}

/**
 * Set track data model with tagged files of directory.
 *
 * @param tagVersion tag version
 */
void Kid3Application::filesToTrackDataModel(TrackData::TagVersion tagVersion)
{
  ImportTrackDataVector trackDataList;
  filesToTrackData(tagVersion, trackDataList);
  getTrackDataModel()->setTrackData(trackDataList);
}

/**
 * Set tagged files of directory from track data model.
 *
 * @param tagVersion tags to set
 */
void Kid3Application::trackDataModelToFiles(TrackData::TagVersion tagVersion)
{
  ImportTrackDataVector trackDataList(getTrackDataModel()->getTrackData());
  ImportTrackDataVector::iterator it = trackDataList.begin();
  FrameFilter flt((tagVersion & TrackData::TagV1) ?
                  frameModelV1()->getEnabledFrameFilter(true) :
                  frameModelV2()->getEnabledFrameFilter(true));
  TaggedFileOfDirectoryIterator tfit(currentOrRootIndex());
  while (tfit.hasNext()) {
    TaggedFile* taggedFile = tfit.next();
    taggedFile->readTags(false);
    if (it != trackDataList.end()) {
      it->removeDisabledFrames(flt);
      formatFramesIfEnabled(*it);
      if (tagVersion & TrackData::TagV1) taggedFile->setFramesV1(*it, false);
      if (tagVersion & TrackData::TagV2) {
        FrameCollection oldFrames;
        taggedFile->getAllFramesV2(oldFrames);
        it->markChangedFrames(oldFrames);
        taggedFile->setFramesV2(*it, true);
      }
      ++it;
    } else {
      break;
    }
  }

  if ((tagVersion & TrackData::TagV2) && flt.isEnabled(Frame::FT_Picture) &&
      !trackDataList.getCoverArtUrl().isEmpty()) {
    downloadImage(trackDataList.getCoverArtUrl(), ImageForImportTrackData);
  }

  if (getFileSelectionModel()->hasSelection()) {
    emit selectedFilesUpdated();
  }
  else {
    emit fileModified();
  }
}

/**
 * Download an image file.
 *
 * @param url  URL of image
 * @param dest specifies affected files
 */
void Kid3Application::downloadImage(const QString& url, DownloadImageDestination dest)
{
  QUrl imgurl(DownloadClient::getImageUrl(url));
  if (!imgurl.isEmpty()) {
    m_downloadImageDest = dest;
    m_downloadClient->startDownload(imgurl);
  }
}

/**
 * Perform a batch import for the selected directories.
 * @param profile batch import profile
 * @param tagVersion import destination tag versions
 */
void Kid3Application::batchImport(const BatchImportProfile& profile,
                                  TrackData::TagVersion tagVersion)
{
  m_batchImportProfile = &profile;
  m_batchImportTagVersion = tagVersion;
  m_batchImportAlbums.clear();
  m_batchImportTrackDataList.clear();
  m_lastProcessedDirName.clear();
  m_batchImporter->clearAborted();
  m_batchImporter->emitReportImportEvent(BatchImportProfile::ReadingDirectory,
                                         QString());
  // If no directories are selected, process files of the current directory.
  QList<QPersistentModelIndex> indexes;
  foreach (const QModelIndex& index, m_fileSelectionModel->selectedRows()) {
    if (m_fileProxyModel->isDir(index)) {
      indexes.append(index);
    }
  }
  if (indexes.isEmpty()) {
    indexes.append(m_fileProxyModelRootIndex);
  }

  connect(m_fileProxyModelIterator, SIGNAL(nextReady(QPersistentModelIndex)),
          this, SLOT(batchImportNextFile(QPersistentModelIndex)));
  m_fileProxyModelIterator->start(indexes);
}

/**
 * Apply single file to batch import.
 *
 * @param index index of file in file proxy model
 */
void Kid3Application::batchImportNextFile(const QPersistentModelIndex& index)
{
  bool terminated = !index.isValid();
  if (!terminated) {
    if (TaggedFile* taggedFile = FileProxyModel::getTaggedFileOfIndex(index)) {
      taggedFile->readTags(false);
      taggedFile = FileProxyModel::readWithId3V24IfId3V24(taggedFile);
      if (taggedFile->getDirname() != m_lastProcessedDirName) {
        m_lastProcessedDirName = taggedFile->getDirname();
        if (!m_batchImportTrackDataList.isEmpty()) {
          m_batchImportAlbums.append(m_batchImportTrackDataList);
        }
        m_batchImportTrackDataList.clear();
        if (m_batchImporter->isAborted()) {
          terminated = true;
        }
      }
      m_batchImportTrackDataList.append(ImportTrackData(*taggedFile,
                                                      m_batchImportTagVersion));
    }
  }
  if (terminated) {
    m_fileProxyModelIterator->abort();
    disconnect(m_fileProxyModelIterator,
               SIGNAL(nextReady(QPersistentModelIndex)),
               this, SLOT(batchImportNextFile(QPersistentModelIndex)));
    if (!m_batchImporter->isAborted()) {
      if (!m_batchImportTrackDataList.isEmpty()) {
        m_batchImportAlbums.append(m_batchImportTrackDataList);
      }
      m_batchImporter->setFrameFilter(
            (m_batchImportTagVersion & TrackData::TagV1) != 0
          ? frameModelV1()->getEnabledFrameFilter(true)
          : frameModelV2()->getEnabledFrameFilter(true));
      m_batchImporter->start(m_batchImportAlbums, *m_batchImportProfile,
                             m_batchImportTagVersion);
    }
  }
}

/**
 * Format a filename if format while editing is switched on.
 *
 * @param taggedFile file to modify
 */
void Kid3Application::formatFileNameIfEnabled(TaggedFile* taggedFile) const
{
  if (FilenameFormatConfig::instance().m_formatWhileEditing) {
    QString fn(taggedFile->getFilename());
    FilenameFormatConfig::instance().formatString(fn);
    taggedFile->setFilename(fn);
  }
}

/**
 * Format frames if format while editing is switched on.
 *
 * @param frames frames
 */
void Kid3Application::formatFramesIfEnabled(FrameCollection& frames) const
{
  TagFormatConfig::instance().formatFramesIfEnabled(frames);
}

/**
 * Get name of selected file.
 *
 * @return absolute file name, ends with "/" if it is a directory.
 */
QString Kid3Application::getFileNameOfSelectedFile()
{
  QModelIndex index = getFileSelectionModel()->currentIndex();
  QString dirname = FileProxyModel::getPathIfIndexOfDir(index);
  if (!dirname.isNull()) {
    if (!dirname.endsWith(QLatin1Char('/'))) dirname += QLatin1Char('/');
    return dirname;
  } else if (TaggedFile* taggedFile =
             FileProxyModel::getTaggedFileOfIndex(index)) {
    return taggedFile->getAbsFilename();
  }
  return QLatin1String("");
}

/**
 * Set name of selected file.
 * Exactly one file has to be selected.
 *
 * @param name file name.
 */
void Kid3Application::setFileNameOfSelectedFile(const QString& name)
{
  if (TaggedFile* taggedFile = getSelectedFile()) {
    QFileInfo fi(name);
    taggedFile->setFilename(fi.fileName());
    emit selectedFilesUpdated();
  }
}

/**
 * Apply filename format.
 */
void Kid3Application::applyFilenameFormat()
{
  emit fileSelectionUpdateRequested();
  SelectedTaggedFileIterator it(getRootIndex(),
                                getFileSelectionModel(),
                                true);
  while (it.hasNext()) {
    TaggedFile* taggedFile = it.next();
    taggedFile->readTags(false);
    QString fn = taggedFile->getFilename();
    FilenameFormatConfig::instance().formatString(fn);
    taggedFile->setFilename(fn);
  }
  emit selectedFilesUpdated();
}

/**
 * Apply ID3 format.
 */
void Kid3Application::applyId3Format()
{
  emit fileSelectionUpdateRequested();
  FrameCollection frames;
  FrameFilter fltV1(frameModelV1()->getEnabledFrameFilter(true));
  FrameFilter fltV2(frameModelV2()->getEnabledFrameFilter(true));
  SelectedTaggedFileIterator it(getRootIndex(),
                                getFileSelectionModel(),
                                true);
  while (it.hasNext()) {
    TaggedFile* taggedFile = it.next();
    taggedFile->readTags(false);
    taggedFile->getAllFramesV1(frames);
    frames.removeDisabledFrames(fltV1);
    TagFormatConfig::instance().formatFrames(frames);
    taggedFile->setFramesV1(frames);
    taggedFile->getAllFramesV2(frames);
    frames.removeDisabledFrames(fltV2);
    TagFormatConfig::instance().formatFrames(frames);
    taggedFile->setFramesV2(frames);
  }
  emit selectedFilesUpdated();
}

/**
 * Apply text encoding.
 * Set the text encoding selected in the settings Tags/ID3v2/Text encoding
 * for all selected files which have an ID3v2 tag.
 */
void Kid3Application::applyTextEncoding()
{
  emit fileSelectionUpdateRequested();
  Frame::Field::TextEncoding encoding;
  switch (TagConfig::instance().textEncoding()) {
  case TagConfig::TE_UTF16:
    encoding = Frame::Field::TE_UTF16;
    break;
  case TagConfig::TE_UTF8:
    encoding = Frame::Field::TE_UTF8;
    break;
  case TagConfig::TE_ISO8859_1:
  default:
    encoding = Frame::Field::TE_ISO8859_1;
  }
  FrameCollection frames;
  SelectedTaggedFileIterator it(getRootIndex(),
                                getFileSelectionModel(),
                                true);
  while (it.hasNext()) {
    TaggedFile* taggedFile = it.next();
    taggedFile->readTags(false);
    taggedFile->getAllFramesV2(frames);
    for (FrameCollection::iterator frameIt = frames.begin();
         frameIt != frames.end();
         ++frameIt) {
      Frame& frame = const_cast<Frame&>(*frameIt);
      Frame::Field::TextEncoding enc = encoding;
      if (taggedFile->getTagFormatV2() == QLatin1String("ID3v2.3.0")) {
        // TagLib sets the ID3v2.3.0 frame containing the date internally with
        // ISO-8859-1, so the encoding cannot be set for such frames.
        if (taggedFile->taggedFileKey() == QLatin1String("TaglibMetadata") &&
            frame.getType() == Frame::FT_Date &&
            enc != Frame::Field::TE_ISO8859_1)
          continue;
        // Only ISO-8859-1 and UTF16 are allowed for ID3v2.3.0.
        if (enc != Frame::Field::TE_ISO8859_1)
          enc = Frame::Field::TE_UTF16;
      }
      Frame::FieldList& fields = frame.fieldList();
      for (Frame::FieldList::iterator fieldIt = fields.begin();
           fieldIt != fields.end();
           ++fieldIt) {
        if (fieldIt->m_id == Frame::Field::ID_TextEnc &&
            fieldIt->m_value.toInt() != enc) {
          fieldIt->m_value = enc;
          frame.setValueChanged();
        }
      }
    }
    taggedFile->setFramesV2(frames);
  }
  emit selectedFilesUpdated();
}

/**
 * Copy tags 1 into copy buffer.
 */
void Kid3Application::copyTagsV1()
{
  emit fileSelectionUpdateRequested();
  m_copyTags = frameModelV1()->frames().copyEnabledFrames(
    frameModelV1()->getEnabledFrameFilter(true));
}

/**
 * Copy tags 2 into copy buffer.
 */
void Kid3Application::copyTagsV2()
{
  emit fileSelectionUpdateRequested();
  m_copyTags = frameModelV2()->frames().copyEnabledFrames(
    frameModelV2()->getEnabledFrameFilter(true));
}

/**
 * Copy tags into copy buffer.
 *
 * @param tagMask tag bit (1 for tag 1, 2 for tag 2)
 */
void Kid3Application::copyTags(TrackData::TagVersion tagMask)
{
  if (tagMask & TrackData::TagV1) {
    copyTagsV1();
  } else if (tagMask & TrackData::TagV2) {
    copyTagsV2();
  }
}

/**
 * Paste from copy buffer to ID3v1 tags.
 */
void Kid3Application::pasteTagsV1()
{
  emit fileSelectionUpdateRequested();
  FrameCollection frames(m_copyTags.copyEnabledFrames(
                         frameModelV1()->getEnabledFrameFilter(true)));
  formatFramesIfEnabled(frames);
  SelectedTaggedFileIterator it(getRootIndex(),
                                getFileSelectionModel(),
                                false);
  while (it.hasNext()) {
    it.next()->setFramesV1(frames, false);
  }
  emit selectedFilesUpdated();
}

/**
 * Paste from copy buffer to ID3v2 tags.
 */
void Kid3Application::pasteTagsV2()
{
  emit fileSelectionUpdateRequested();
  FrameCollection frames(m_copyTags.copyEnabledFrames(
                         frameModelV2()->getEnabledFrameFilter(true)));
  formatFramesIfEnabled(frames);
  SelectedTaggedFileIterator it(getRootIndex(),
                                getFileSelectionModel(),
                                false);
  while (it.hasNext()) {
    it.next()->setFramesV2(frames, false);
  }
  emit selectedFilesUpdated();
}

/**
 * Paste from copy buffer to tags.
 *
 * @param tagMask tag bit (1 for tag 1, 2 for tag 2)
 */
void Kid3Application::pasteTags(TrackData::TagVersion tagMask)
{
  if (tagMask & TrackData::TagV1) {
    pasteTagsV1();
  } else if (tagMask & TrackData::TagV2) {
    pasteTagsV2();
  }
}

/**
 * Copy ID3v1 tags to ID3v2 tags of selected files.
 */
void Kid3Application::copyV1ToV2()
{
  emit fileSelectionUpdateRequested();
  FrameCollection frames;
  FrameFilter flt(frameModelV2()->getEnabledFrameFilter(true));
  SelectedTaggedFileIterator it(getRootIndex(),
                                getFileSelectionModel(),
                                false);
  while (it.hasNext()) {
    TaggedFile* taggedFile = it.next();
    taggedFile->getAllFramesV1(frames);
    frames.removeDisabledFrames(flt);
    formatFramesIfEnabled(frames);
    taggedFile->setFramesV2(frames, false);
  }
  emit selectedFilesUpdated();
}

/**
 * Copy ID3v2 tags to ID3v1 tags of selected files.
 */
void Kid3Application::copyV2ToV1()
{
  emit fileSelectionUpdateRequested();
  FrameCollection frames;
  FrameFilter flt(frameModelV1()->getEnabledFrameFilter(true));
  SelectedTaggedFileIterator it(getRootIndex(),
                                getFileSelectionModel(),
                                false);
  while (it.hasNext()) {
    TaggedFile* taggedFile = it.next();
    taggedFile->getAllFramesV2(frames);
    frames.removeDisabledFrames(flt);
    formatFramesIfEnabled(frames);
    taggedFile->setFramesV1(frames, false);
  }
  emit selectedFilesUpdated();
}

/**
 * Set tag from other tag.
 *
 * @param tagMask tag bit (1 for tag 1, 2 for tag 2)
 */
void Kid3Application::copyToOtherTag(TrackData::TagVersion tagMask)
{
  if (tagMask & TrackData::TagV1) {
    copyV2ToV1();
  } else if (tagMask & TrackData::TagV2) {
    copyV1ToV2();
  }
}

/**
 * Remove ID3v1 tags in selected files.
 */
void Kid3Application::removeTagsV1()
{
  emit fileSelectionUpdateRequested();
  FrameFilter flt(frameModelV1()->getEnabledFrameFilter(true));
  SelectedTaggedFileIterator it(getRootIndex(),
                                getFileSelectionModel(),
                                false);
  while (it.hasNext()) {
    it.next()->deleteFramesV1(flt);
  }
  emit selectedFilesUpdated();
}

/**
 * Remove ID3v2 tags in selected files.
 */
void Kid3Application::removeTagsV2()
{
  emit fileSelectionUpdateRequested();
  FrameFilter flt(frameModelV2()->getEnabledFrameFilter(true));
  SelectedTaggedFileIterator it(getRootIndex(),
                                getFileSelectionModel(),
                                false);
  while (it.hasNext()) {
    it.next()->deleteFramesV2(flt);
  }
  emit selectedFilesUpdated();
}

/**
 * Remove tags in selected files.
 *
 * @param tagMask tag bit (1 for tag 1, 2 for tag 2)
 */
void Kid3Application::removeTags(TrackData::TagVersion tagMask)
{
  if (tagMask & TrackData::TagV1) {
    removeTagsV1();
  } else if (tagMask & TrackData::TagV2) {
    removeTagsV2();
  }
}

/**
 * Set ID3v1 tags according to filename.
 * If a single file is selected the tags in the GUI controls
 * are set, else the tags in the multiple selected files.
 */
void Kid3Application::getTagsFromFilenameV1()
{
  emit fileSelectionUpdateRequested();
  FrameCollection frames;
  QItemSelectionModel* selectModel = getFileSelectionModel();
  SelectedTaggedFileIterator it(getRootIndex(),
                                selectModel,
                                false);
  FrameFilter flt(frameModelV1()->getEnabledFrameFilter(true));
  while (it.hasNext()) {
    TaggedFile* taggedFile = it.next();
    taggedFile->getAllFramesV1(frames);
    taggedFile->getTagsFromFilename(frames, m_filenameToTagsFormat);
    frames.removeDisabledFrames(flt);
    formatFramesIfEnabled(frames);
    taggedFile->setFramesV1(frames);
  }
  emit selectedFilesUpdated();
}

/**
 * Set ID3v2 tags according to filename.
 * If a single file is selected the tags in the GUI controls
 * are set, else the tags in the multiple selected files.
 */
void Kid3Application::getTagsFromFilenameV2()
{
  emit fileSelectionUpdateRequested();
  FrameCollection frames;
  QItemSelectionModel* selectModel = getFileSelectionModel();
  SelectedTaggedFileIterator it(getRootIndex(),
                                selectModel,
                                false);
  FrameFilter flt(frameModelV2()->getEnabledFrameFilter(true));
  while (it.hasNext()) {
    TaggedFile* taggedFile = it.next();
    taggedFile->getAllFramesV2(frames);
    taggedFile->getTagsFromFilename(frames, m_filenameToTagsFormat);
    frames.removeDisabledFrames(flt);
    formatFramesIfEnabled(frames);
    taggedFile->setFramesV2(frames);
  }
  emit selectedFilesUpdated();
}

/**
 * Set tags according to filename.
 *
 * @param tagMask tag bit (1 for tag 1, 2 for tag 2)
 */
void Kid3Application::getTagsFromFilename(TrackData::TagVersion tagMask)
{
  if (tagMask & TrackData::TagV1) {
    getTagsFromFilenameV1();
  } else if (tagMask & TrackData::TagV2) {
    getTagsFromFilenameV2();
  }
}

/**
 * Set filename according to tags.
 * If a single file is selected the tags in the GUI controls
 * are used, else the tags in the multiple selected files.
 *
 * @param tagVersion tag version
 */
void Kid3Application::getFilenameFromTags(TrackData::TagVersion tagVersion)
{
  emit fileSelectionUpdateRequested();
  QItemSelectionModel* selectModel = getFileSelectionModel();
  SelectedTaggedFileIterator it(getRootIndex(),
                                selectModel,
                                false);
  while (it.hasNext()) {
    TaggedFile* taggedFile = it.next();
    TrackData trackData(*taggedFile, tagVersion);
    if (!trackData.isEmptyOrInactive()) {
      taggedFile->setFilename(
            trackData.formatFilenameFromTags(m_tagsToFilenameFormat));
      formatFileNameIfEnabled(taggedFile);
    }
  }
  emit selectedFilesUpdated();
}

/**
 * Set format used to generate filename from tags.
 * When changed, filenameToTagsFormatChanged() is emitted.
 * @param format format
 */
void Kid3Application::setFilenameToTagsFormat(const QString& format) {
  if (m_filenameToTagsFormat != format) {
    m_filenameToTagsFormat = format;
    emit filenameToTagsFormatChanged(format);
  }
}

/**
 * Set format used to generate filename from tags without emitting
 * filenameToTagsFormatChanged() signal.
 * This has to be used when connected from the GUI to avoid that the GUI
 * is updated because of its own changes.
 * @param format format
 */
void Kid3Application::setFilenameToTagsFormatWithoutSignaling(
  const QString& format) {
  m_filenameToTagsFormat = format;
}

/**
 * Set format used to generate tags from filename.
 * When changed, tagsToFilenameFormatChanged() is emitted.
 * @param format format
 */
void Kid3Application::setTagsToFilenameFormat(const QString& format) {
  if (m_tagsToFilenameFormat != format) {
    m_tagsToFilenameFormat = format;
    emit tagsToFilenameFormatChanged(format);
  }
}

/**
   * Set format used to generate tags from filename without emitting
   * tagsToFilenameFormatChanged() signal.
   * This has to be used when connected from the GUI to avoid that the GUI
   * is updated because of its own changes.
   * @param format format
 */
void Kid3Application::setTagsToFilenameFormatWithoutSignaling(
  const QString& format) {
  m_tagsToFilenameFormat = format;
}

/**
 * Get the selected file.
 *
 * @return the selected file,
 *         0 if not exactly one file is selected
 */
TaggedFile* Kid3Application::getSelectedFile()
{
  QModelIndexList selItems(
      m_fileSelectionModel->selectedRows());
  if (selItems.size() != 1)
    return 0;

  return FileProxyModel::getTaggedFileOfIndex(selItems.first());
}


/**
 * Edit selected frame.
 *
 * @param frameEditor editor for frame fields
 */
void Kid3Application::editFrame(IFrameEditor* frameEditor)
{
  emit fileSelectionUpdateRequested();
  m_editFrameTaggedFile = getSelectedFile();
  if (const Frame* selectedFrame = frameModelV2()->getFrameOfIndex(
        getFramesV2SelectionModel()->currentIndex())) {
    if (m_editFrameTaggedFile) {
      connect(frameEditor->frameEditedEmitter(),
              SIGNAL(frameEdited(const Frame*)),
              this, SLOT(onFrameEdited(const Frame*)), Qt::UniqueConnection);
      frameEditor->editFrameOfTaggedFile(selectedFrame, m_editFrameTaggedFile);
    } else {
      // multiple files selected
      // Get the first selected file by using a temporary iterator.
      TaggedFile* firstFile = SelectedTaggedFileIterator(
            getRootIndex(), getFileSelectionModel(), false).peekNext();
      if (firstFile) {
        m_framelist->setTaggedFile(firstFile);
        m_editFrameName = m_framelist->getSelectedName();
        if (!m_editFrameName.isEmpty()) {
          connect(frameEditor->frameEditedEmitter(),
                  SIGNAL(frameEdited(const Frame*)),
                  this, SLOT(onFrameEdited(const Frame*)),
                  Qt::UniqueConnection);
          frameEditor->editFrameOfTaggedFile(selectedFrame, firstFile);
        }
      }
    }
  }
}

/**
 * Called when a frame is edited.
 * @param frame edited frame, 0 if canceled
 */
void Kid3Application::onFrameEdited(const Frame* frame)
{
  if (QObject* emitter = sender()) {
    if (emitter->metaObject()->indexOfSignal("frameEdited(const Frame*)") != -1)
    {
      disconnect(emitter, SIGNAL(frameEdited(const Frame*)),
                 this, SLOT(onFrameEdited(const Frame*)));
    }
  }
  if (!frame)
    return;

  if (m_editFrameTaggedFile) {
    emit frameModified(m_editFrameTaggedFile);
  } else {
    m_framelist->setFrame(*frame);

    // Start a new iteration because the file selection model can be
    // changed by editFrameOfTaggedFile(), e.g. when a file is exported
    // from a picture frame.
    SelectedTaggedFileIterator tfit(getRootIndex(),
                                    getFileSelectionModel(),
                                    false);
    while (tfit.hasNext()) {
      TaggedFile* currentFile = tfit.next();
      FrameCollection frames;
      currentFile->getAllFramesV2(frames);
      for (FrameCollection::const_iterator it = frames.begin();
           it != frames.end();
           ++it) {
        if (it->getName() == m_editFrameName) {
          currentFile->deleteFrameV2(*it);
          break;
        }
      }
      m_framelist->setTaggedFile(currentFile);
      m_framelist->pasteFrame();
    }
    emit selectedFilesUpdated();
  }
}

/**
 * Delete selected frame.
 *
 * @param frameName name of frame to delete, empty to delete selected frame
 */
void Kid3Application::deleteFrame(const QString& frameName)
{
  emit fileSelectionUpdateRequested();
  TaggedFile* taggedFile = getSelectedFile();
  if (taggedFile && frameName.isEmpty()) {
    // delete selected frame from single file
    if (!m_framelist->deleteFrame()) {
      // frame not found
      return;
    }
    emit frameModified(taggedFile);
  } else {
    // multiple files selected or frame name specified
    bool firstFile = true;
    QString name;
    SelectedTaggedFileIterator tfit(getRootIndex(),
                                    getFileSelectionModel(),
                                    false);
    while (tfit.hasNext()) {
      TaggedFile* currentFile = tfit.next();
      if (firstFile) {
        firstFile = false;
        taggedFile = currentFile;
        m_framelist->setTaggedFile(taggedFile);
        name = frameName.isEmpty() ? m_framelist->getSelectedName() :
          frameName;
      }
      FrameCollection frames;
      currentFile->getAllFramesV2(frames);
      for (FrameCollection::const_iterator it = frames.begin();
           it != frames.end();
           ++it) {
        if (it->getName() == name) {
          currentFile->deleteFrameV2(*it);
          break;
        }
      }
    }
    emit selectedFilesUpdated();
  }
}

/**
 * Select a frame type and add such a frame to frame list.
 *
 * @param frame frame to add, if 0 the user has to select and edit the frame
 * @param frameEditor editor for frame fields, if not null and a frame
 * is set, the user can edit the frame before it is added
 */
void Kid3Application::addFrame(const Frame* frame, IFrameEditor* frameEditor)
{
  emit fileSelectionUpdateRequested();
  TaggedFile* currentFile = 0;
  m_addFrameTaggedFile = getSelectedFile();
  if (m_addFrameTaggedFile) {
    currentFile = m_addFrameTaggedFile;
  } else {
    // multiple files selected
    SelectedTaggedFileIterator tfit(getRootIndex(),
                                    getFileSelectionModel(),
                                    false);
    if (tfit.hasNext()) {
      currentFile = tfit.next();
      m_framelist->setTaggedFile(currentFile);
    }
  }

  if (currentFile) {
    if (frameEditor) {
      connect(m_framelist, SIGNAL(frameEdited(const Frame*)),
              this, SLOT(onFrameAdded(const Frame*)), Qt::UniqueConnection);
      if (frame) {
        m_framelist->setFrame(*frame);
        m_framelist->addAndEditFrame(frameEditor);
      } else {
        m_framelist->selectAddAndEditFrame(frameEditor);
      }
    } else {
      m_framelist->setFrame(*frame);
      onFrameAdded(m_framelist->pasteFrame() ? &m_framelist->getFrame() : 0);
    }
  }
}

/**
 * Called when a frame is added.
 * @param frame edited frame, 0 if canceled
 */
void Kid3Application::onFrameAdded(const Frame* frame)
{
  if (QObject* emitter = sender()) {
    if (emitter->metaObject()->indexOfSignal("frameEdited(const Frame*)") != -1)
    {
      disconnect(emitter, SIGNAL(frameEdited(const Frame*)),
                 this, SLOT(onFrameAdded(const Frame*)));
    }
  }
  if (!frame)
    return;

  if (m_addFrameTaggedFile) {
    emit frameModified(m_addFrameTaggedFile);
    if (m_framelist->isPictureFrame()) {
      // update preview picture
      emit selectedFilesUpdated();
    }
  } else {
    // multiple files selected
    bool firstFile = true;
    int frameId = -1;
    m_framelist->setFrame(*frame);

    SelectedTaggedFileIterator tfit(getRootIndex(),
                                    getFileSelectionModel(),
                                    false);
    while (tfit.hasNext()) {
      TaggedFile* currentFile = tfit.next();
      if (firstFile) {
        firstFile = false;
        m_addFrameTaggedFile = currentFile;
        m_framelist->setTaggedFile(currentFile);
        frameId = m_framelist->getSelectedId();
      } else {
        m_framelist->setTaggedFile(currentFile);
        m_framelist->pasteFrame();
      }
    }
    m_framelist->setTaggedFile(m_addFrameTaggedFile);
    if (frameId != -1) {
      m_framelist->setSelectedId(frameId);
    }
    emit selectedFilesUpdated();
  }
}

/**
 * Edit a picture frame if one exists or add a new one.
 *
 * @param frameEditor editor for frame fields
 */
void Kid3Application::editOrAddPicture(IFrameEditor* frameEditor)
{
  if (m_framelist->selectByName(QLatin1String("Picture"))) {
    editFrame(frameEditor);
  } else {
    PictureFrame frame;
    addFrame(&frame, frameEditor);
  }
}

/**
 * Open directory or add pictures on drop.
 *
 * @param paths paths of directories or files in directory
 */
void Kid3Application::openDrop(const QStringList& paths)
{
  QStringList filePaths;
  QStringList picturePaths;
  foreach (QString txt, paths) {
    int lfPos = txt.indexOf(QLatin1Char('\n'));
    if (lfPos > 0 && lfPos < static_cast<int>(txt.length()) - 1) {
      txt.truncate(lfPos + 1);
    }
    QUrl url(txt);
    if (!url.path().isEmpty()) {
#ifdef Q_OS_WIN32
      QString dir = url.toString();
#else
      QString dir = url.path().trimmed();
#endif
      if (dir.endsWith(QLatin1String(".jpg"), Qt::CaseInsensitive) ||
          dir.endsWith(QLatin1String(".jpeg"), Qt::CaseInsensitive) ||
          dir.endsWith(QLatin1String(".png"), Qt::CaseInsensitive)) {
        picturePaths.append(dir);
      } else {
        filePaths.append(dir);
      }
    }
  }
  if (!filePaths.isEmpty()) {
    // Check if the file filter has to be removed to open the dropped files.
    QStringList nameFilters(m_platformTools->getNameFilterPatterns(
                FileConfig::instance().m_nameFilter).split(QLatin1Char(' ')));
    if (!nameFilters.isEmpty() && nameFilters.first() != QLatin1String("*")) {
      foreach (const QString& filePath, filePaths) {
        if (!QDir::match(nameFilters, filePath) &&
            !QFileInfo(filePath).isDir()) {
          FileConfig::instance().m_nameFilter = QLatin1String("");
          break;
        }
      }
    }
    emit fileSelectionUpdateRequested();
    emit confirmedOpenDirectoryRequested(filePaths);
  } else if (!picturePaths.isEmpty()) {
    foreach (const QString& picturePath, picturePaths) {
      PictureFrame frame;
      if (PictureFrame::setDataFromFile(frame, picturePath)) {
        QString fileName(picturePath);
        int slashPos = fileName.lastIndexOf(QLatin1Char('/'));
        if (slashPos != -1) {
          fileName = fileName.mid(slashPos + 1);
        }
        PictureFrame::setMimeTypeFromFileName(frame, fileName);
        PictureFrame::setDescription(frame, fileName);
        addFrame(&frame);
        emit selectedFilesUpdated();
      }
    }
  }
}

/**
 * Add picture on drop.
 *
 * @param image dropped image.
 */
void Kid3Application::dropImage(const QImage& image)
{
  if (!image.isNull()) {
    PictureFrame frame;
    if (PictureFrame::setDataFromImage(frame, image)) {
      addFrame(&frame);
      emit selectedFilesUpdated();
    }
  }
}

/**
 * Handle URL on drop.
 *
 * @param txt dropped URL.
 */
void Kid3Application::dropUrl(const QString& txt)
{
  downloadImage(txt, Kid3Application::ImageForSelectedFiles);
}

/**
 * Add a downloaded image.
 *
 * @param data     HTTP response of download
 * @param mimeType MIME type of data
 * @param url      URL of downloaded data
 */
void Kid3Application::imageDownloaded(const QByteArray& data,
                              const QString& mimeType, const QString& url)
{
  // An empty mime type is accepted to allow downloads via FTP.
  if (mimeType.startsWith(QLatin1String("image")) ||
      mimeType.isEmpty()) {
    PictureFrame frame(data, url, PictureFrame::PT_CoverFront, mimeType);
    if (getDownloadImageDestination() == ImageForAllFilesInDirectory) {
      TaggedFileOfDirectoryIterator it(currentOrRootIndex());
      while (it.hasNext()) {
        TaggedFile* taggedFile = it.next();
        taggedFile->readTags(false);
        taggedFile->addFrameV2(frame);
      }
    } else if (getDownloadImageDestination() == ImageForImportTrackData) {
      const ImportTrackDataVector& trackDataVector(
            getTrackDataModel()->trackData());
      for (ImportTrackDataVector::const_iterator it =
           trackDataVector.constBegin();
           it != trackDataVector.constEnd();
           ++it) {
        TaggedFile* taggedFile;
        if (it->isEnabled() && (taggedFile = it->getTaggedFile()) != 0) {
          taggedFile->readTags(false);
          taggedFile->addFrameV2(frame);
        }
      }
    } else {
      addFrame(&frame);
    }
    emit selectedFilesUpdated();
  }
}

/**
 * Set the first file as the current file.
 *
 * @param select true to select the file
 *
 * @return true if a file exists.
 */
bool Kid3Application::firstFile(bool select)
{
  m_fileSelectionModel->setCurrentIndex(getRootIndex(),
                                        QItemSelectionModel::NoUpdate);
  return nextFile(select);
}

/**
 * Set the next file as the current file.
 *
 * @param select true to select the file
 *
 * @return true if a next file exists.
 */
bool Kid3Application::nextFile(bool select)
{
  QModelIndex current(m_fileSelectionModel->currentIndex()), next;
  if (m_fileProxyModel->rowCount(current) > 0) {
    // to first child
    next = m_fileProxyModel->index(0, 0, current);
  } else {
    QModelIndex parent = current;
    while (!next.isValid() && parent.isValid()) {
      // to next sibling or next sibling of parent
      int row = parent.row();
      if (parent == getRootIndex()) {
        // do not move beyond root index
        return false;
      }
      parent = parent.parent();
      if (row + 1 < m_fileProxyModel->rowCount(parent)) {
        // to next sibling
        next = m_fileProxyModel->index(row + 1, 0, parent);
      }
    }
  }
  if (!next.isValid())
    return false;
  m_fileSelectionModel->setCurrentIndex(next,
    select ? QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows
           : QItemSelectionModel::Current);
  return true;
}

/**
 * Set the previous file as the current file.
 *
 * @param select true to select the file
 *
 * @return true if a previous file exists.
 */
bool Kid3Application::previousFile(bool select)
{
  QModelIndex current(m_fileSelectionModel->currentIndex()), previous;
  int row = current.row() - 1;
  if (row >= 0) {
    // to last leafnode of previous sibling
    previous = current.sibling(row, 0);
    row = m_fileProxyModel->rowCount(previous) - 1;
    while (row >= 0) {
      previous = m_fileProxyModel->index(row, 0, previous);
      row = m_fileProxyModel->rowCount(previous) - 1;
    }
  } else {
    // to parent
    previous = current.parent();
  }
  if (!previous.isValid() || previous == getRootIndex())
    return false;
  m_fileSelectionModel->setCurrentIndex(previous,
    select ? QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows
           : QItemSelectionModel::Current);
  return true;
}

/**
 * Select or deselect the current file.
 *
 * @param select true to select the file, false to deselect it
 *
 * @return true if a current file exists.
 */
bool Kid3Application::selectCurrentFile(bool select)
{
  QModelIndex currentIdx(m_fileSelectionModel->currentIndex());
  if (!currentIdx.isValid() || currentIdx == getRootIndex())
    return false;

  m_fileSelectionModel->setCurrentIndex(currentIdx,
    (select ? QItemSelectionModel::Select : QItemSelectionModel::Deselect) |
    QItemSelectionModel::Rows);
  return true;
}

/**
 * Select all files.
 */
void Kid3Application::selectAllFiles()
{
  QItemSelection selection;
  ModelIterator it(m_fileProxyModelRootIndex);
  while (it.hasNext()) {
    selection.append(QItemSelectionRange(it.next()));
  }
  m_fileSelectionModel->select(selection,
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
}

/**
 * Deselect all files.
 */
void Kid3Application::deselectAllFiles()
{
  m_fileSelectionModel->clearSelection();
}

/**
 * Fetch entries of directory if not already fetched.
 * This works like FileList::expand(), but without expanding tree view
 * items and independent of the GUI. The processing is done in the background
 * by QFileSystemModel, so the fetched items are not immediately available
 * after calling this method.
 *
 * @param index index of directory item
 */
void Kid3Application::fetchDirectory(const QModelIndex& index)
{
  if (index.isValid() && m_fileProxyModel->canFetchMore(index)) {
    m_fileProxyModel->fetchMore(index);
  }
}

/**
 * Fetch entries of directory and toggle expanded state if GUI available.
 * @param index index of directory item
 */
void Kid3Application::expandDirectory(const QModelIndex& index)
{
  fetchDirectory(index);
  emit toggleExpandedRequested(index);
}

/**
 * Expand the whole file list if GUI available.
 */
void Kid3Application::requestExpandFileList()
{
  emit expandFileListRequested();
}

/**
 * Process change of selection.
 * The GUI is signaled to update the current selection and the controls.
 */
void Kid3Application::fileSelected()
{
  emit fileSelectionUpdateRequested();
  emit selectedFilesUpdated();
}

/**
 * Search in tags for a given text.
 * @param params search parameters
 */
void Kid3Application::findText(const TagSearcher::Parameters& params)
{
  m_tagSearcher->setModel(m_fileProxyModel);
  m_tagSearcher->setRootIndex(m_fileProxyModelRootIndex);
  m_tagSearcher->find(params);
}

/**
 * Replace found text.
 * @param params search parameters
 */
void Kid3Application::replaceText(const TagSearcher::Parameters& params)
{
  m_tagSearcher->setModel(m_fileProxyModel);
  m_tagSearcher->setRootIndex(m_fileProxyModelRootIndex);
  m_tagSearcher->replace(params);
}

/**
 * Replace all occurrences.
 * @param params search parameters
 */
void Kid3Application::replaceAll(const TagSearcher::Parameters& params)
{
  m_tagSearcher->setModel(m_fileProxyModel);
  m_tagSearcher->setRootIndex(m_fileProxyModelRootIndex);
  m_tagSearcher->replaceAll(params);
}

/**
 * Schedule actions to rename a directory.
 * When finished renameActionsScheduled() is emitted.
 */
void Kid3Application::scheduleRenameActions()
{
  m_dirRenamer->clearActions();
  m_dirRenamer->clearAborted();
  // If directories are selected, rename them, else process files of the
  // current directory.
  QList<QPersistentModelIndex> indexes;
  foreach (const QModelIndex& index, m_fileSelectionModel->selectedRows()) {
    if (m_fileProxyModel->isDir(index)) {
      indexes.append(index);
    }
  }
  if (indexes.isEmpty()) {
    indexes.append(m_fileProxyModelRootIndex);
  }

  connect(m_fileProxyModelIterator, SIGNAL(nextReady(QPersistentModelIndex)),
          this, SLOT(scheduleNextRenameAction(QPersistentModelIndex)));
  m_fileProxyModelIterator->start(indexes);
}

/**
 * Schedule rename action for a file.
 *
 * @param index index of file in file proxy model
 */
void Kid3Application::scheduleNextRenameAction(const QPersistentModelIndex& index)
{
  bool terminated = !index.isValid();
  if (!terminated) {
    if (TaggedFile* taggedFile = FileProxyModel::getTaggedFileOfIndex(index)) {
      taggedFile->readTags(false);
      taggedFile = FileProxyModel::readWithId3V24IfId3V24(taggedFile);
      m_dirRenamer->scheduleAction(taggedFile);
      if (m_dirRenamer->isAborted()) {
        terminated = true;
      }
    }
  }
  if (terminated) {
    m_fileProxyModelIterator->abort();
    disconnect(m_fileProxyModelIterator, SIGNAL(nextReady(QPersistentModelIndex)),
               this, SLOT(scheduleNextRenameAction(QPersistentModelIndex)));
    emit renameActionsScheduled();
  }
}

/**
 * Apply a file filter.
 *
 * @param fileFilter filter to apply.
 */
void Kid3Application::applyFilter(FileFilter& fileFilter)
{
  m_fileProxyModel->disableFilteringOutIndexes();
  setFiltered(false);
  fileFilter.clearAborted();
  emit fileFiltered(FileFilter::Started, QString());

  m_fileFilter = &fileFilter;
  m_lastProcessedDirName.clear();
  connect(m_fileProxyModelIterator, SIGNAL(nextReady(QPersistentModelIndex)),
          this, SLOT(filterNextFile(QPersistentModelIndex)));
  m_fileProxyModelIterator->start(m_fileProxyModelRootIndex);
}

/**
 * Apply single file to file filter.
 *
 * @param index index of file in file proxy model
 */
void Kid3Application::filterNextFile(const QPersistentModelIndex& index)
{
  if (!m_fileFilter)
    return;

  bool terminated = !index.isValid();
  if (!terminated) {
    if (TaggedFile* taggedFile = FileProxyModel::getTaggedFileOfIndex(index)) {
      taggedFile->readTags(false);
      taggedFile = FileProxyModel::readWithId3V24IfId3V24(taggedFile);
      if (taggedFile->getDirname() != m_lastProcessedDirName) {
        m_lastProcessedDirName = taggedFile->getDirname();
        emit fileFiltered(FileFilter::Directory, m_lastProcessedDirName);
      }
      bool ok;
      bool pass = m_fileFilter->filter(*taggedFile, &ok);
      if (ok) {
        emit fileFiltered(
              pass ? FileFilter::FilePassed : FileFilter::FileFilteredOut,
              taggedFile->getFilename());
        if (!pass)
          m_fileProxyModel->filterOutIndex(taggedFile->getIndex());
      } else {
        emit fileFiltered(FileFilter::ParseError, QString());
        terminated = true;
      }

      if (m_fileFilter->isAborted()) {
        terminated = true;
        emit fileFiltered(FileFilter::Aborted, QString());
      }
    }
  }
  if (terminated) {
    if (!m_fileFilter->isAborted()) {
      emit fileFiltered(FileFilter::Finished, QString());
    }

    m_fileProxyModelIterator->abort();
    m_fileProxyModel->applyFilteringOutIndexes();
    setFiltered(!m_fileFilter->isEmptyFilterExpression());
    emit fileModified();

    disconnect(m_fileProxyModelIterator, SIGNAL(nextReady(QPersistentModelIndex)),
               this, SLOT(filterNextFile(QPersistentModelIndex)));
  }
}

/**
 * Apply a file filter.
 *
 * @param expression filter expression
 */
void Kid3Application::applyFilter(const QString& expression)
{
  if (!m_expressionFileFilter) {
    m_expressionFileFilter = new FileFilter(this);
  }
  m_expressionFileFilter->clearAborted();
  m_expressionFileFilter->setFilterExpression(expression);
  m_expressionFileFilter->initParser();
  applyFilter(*m_expressionFileFilter);
}

/**
 * Abort expression file filter.
 */
void Kid3Application::abortFilter()
{
  if (m_expressionFileFilter) {
    m_expressionFileFilter->abort();
  }
}

/**
 * Perform rename actions and change application directory afterwards if it
 * was renamed.
 *
 * @return error messages, null string if no error occurred.
 */
QString Kid3Application::performRenameActions()
{
  QString errorMsg;
  m_dirRenamer->setDirName(getDirName());
  m_dirRenamer->performActions(&errorMsg);
  if (m_dirRenamer->getDirName() != getDirName()) {
    openDirectory(QStringList() << m_dirRenamer->getDirName());
  }
  return errorMsg;
}

/**
 * Set the directory name from the tags.
 * The directory must not have modified files.
 * renameActionsScheduled() is emitted when the rename actions have been
 * scheduled. Then performRenameActions() has to be called to effectively
 * rename the directory.
 *
 * @param tagMask tag mask
 * @param format  directory name format
 * @param create  true to create, false to rename
 *
 * @return true if ok.
 */
bool Kid3Application::renameDirectory(TrackData::TagVersion tagMask,
                                     const QString& format, bool create)
{
  TaggedFile* taggedFile =
    TaggedFileOfDirectoryIterator::first(currentOrRootIndex());
  if (!isModified() && taggedFile) {
    m_dirRenamer->setTagVersion(tagMask);
    m_dirRenamer->setFormat(format);
    m_dirRenamer->setAction(create);
    scheduleRenameActions();
    return true;
  }
  return false;
}

/**
 * Number tracks in selected files of directory.
 *
 * @param nr start number
 * @param total total number of tracks, used if >0
 * @param tagVersion determines on which tags the numbers are set
 */
void Kid3Application::numberTracks(int nr, int total,
                                   TrackData::TagVersion tagVersion)
{
  emit fileSelectionUpdateRequested();
  int numDigits = TagConfig::instance().trackNumberDigits();
  if (numDigits < 1 || numDigits > 5)
    numDigits = 1;

  // If directories are selected, number their files, else process the selected
  // files of the current directory.
  AbstractTaggedFileIterator* it =
      new TaggedFileOfSelectedDirectoriesIterator(getFileSelectionModel());
  if (!it->hasNext()) {
    delete it;
    it = new SelectedTaggedFileOfDirectoryIterator(
               currentOrRootIndex(),
               getFileSelectionModel(),
               true);
  }
  while (it->hasNext()) {
    TaggedFile* taggedFile = it->next();
    taggedFile->readTags(false);
    if (tagVersion & TrackData::TagV1) {
      int oldnr = taggedFile->getTrackNumV1();
      if (nr != oldnr) {
        taggedFile->setTrackNumV1(nr);
      }
    }
    if (tagVersion & TrackData::TagV2) {
      // For tag 2 the frame is written, so that we have control over the
      // format and the total number of tracks, and it is possible to change
      // the format even if the numbers stay the same.
      QString value;
      if (total > 0) {
        value.sprintf("%0*d/%0*d", numDigits, nr, numDigits, total);
      } else {
        value.sprintf("%0*d", numDigits, nr);
      }
      FrameCollection frames;
      taggedFile->getAllFramesV2(frames);
      Frame frame(Frame::FT_Track, QLatin1String(""), QLatin1String(""), -1);
      FrameCollection::const_iterator frameIt = frames.find(frame);
      if (frameIt != frames.end()) {
        frame = *frameIt;
        frame.setValueIfChanged(value);
        if (frame.isValueChanged()) {
          taggedFile->setFrameV2(frame);
        }
      } else {
        frame.setValue(value);
        frame.setExtendedType(Frame::ExtendedType(Frame::FT_Track));
        taggedFile->setFrameV2(frame);
      }
    }
    ++nr;
  }
  emit selectedFilesUpdated();
  delete it;
}

#if defined HAVE_PHONON || QT_VERSION >= 0x050000
/**
 * Play audio file.
 */
void Kid3Application::playAudio()
{
  QStringList files;
  int fileNr = 0;
  if (m_fileSelectionModel->selectedRows().size() > 1) {
    // play only the selected files if more than one is selected
    SelectedTaggedFileIterator it(m_fileProxyModelRootIndex,
                                  m_fileSelectionModel,
                                  false);
    while (it.hasNext()) {
      files.append(it.next()->getAbsFilename());
    }
  } else {
    // play all files if none or only one is selected
    int idx = 0;
    ModelIterator it(m_fileProxyModelRootIndex);
    while (it.hasNext()) {
      QModelIndex index = it.next();
      if (TaggedFile* taggedFile = FileProxyModel::getTaggedFileOfIndex(index)) {
        files.append(taggedFile->getAbsFilename());
        if (m_fileSelectionModel->isSelected(index)) {
          fileNr = idx;
        }
        ++idx;
      }
    }
  }
  emit aboutToPlayAudio();
  getAudioPlayer()->setFiles(files, fileNr);
}

/**
 * Show play tool bar.
 */
void Kid3Application::showAudioPlayer()
{
  emit aboutToPlayAudio();
}
#endif

/**
 * Get number of tracks in current directory.
 *
 * @return number of tracks, 0 if not found.
 */
int Kid3Application::getTotalNumberOfTracksInDir()
{
  if (TaggedFile* taggedFile = TaggedFileOfDirectoryIterator::first(
      currentOrRootIndex())) {
    return taggedFile->getTotalNumberOfTracksInDir();
  }
  return 0;
}

/**
 * Create a filter string for the file dialog.
 * The filter string contains entries for all supported types.
 *
 * @return filter string.
 */
QString Kid3Application::createFilterString() const
{
  QStringList extensions;
  foreach (ITaggedFileFactory* factory, FileProxyModel::taggedFileFactories()) {
    foreach (const QString& key, factory->taggedFileKeys()) {
      extensions.append(factory->supportedFileExtensions(key));
    }
  }
  // remove duplicates
  extensions.sort();
  QString lastExt(QLatin1String(""));
  for (QStringList::iterator it = extensions.begin();
       it != extensions.end();) {
    if (*it == lastExt) {
      it = extensions.erase(it);
    } else {
      lastExt = *it;
      ++it;
    }
  }

  QString allPatterns;
  QList<QPair<QString, QString> > nameFilters;
  for (QStringList::const_iterator it = extensions.begin();
       it != extensions.end();
       ++it) {
    QString text = (*it).mid(1).toUpper();
    QString pattern = QLatin1Char('*') + *it;
    if (!allPatterns.isEmpty()) {
      allPatterns += QLatin1Char(' ');
    }
    allPatterns += pattern;
    nameFilters.append(qMakePair(text, pattern));
  }
  if (!allPatterns.isEmpty()) {
    nameFilters.prepend(qMakePair(tr("All Supported Files"), allPatterns));
  }
  nameFilters.append(qMakePair(tr("All Files"), QString(QLatin1Char('*'))));
  return m_platformTools->fileDialogNameFilter(nameFilters);
}

/**
 * Notify the tagged file factories about the changed configuration.
 */
void Kid3Application::notifyConfigurationChange()
{
  foreach (ITaggedFileFactory* factory, FileProxyModel::taggedFileFactories()) {
    foreach (const QString& key, factory->taggedFileKeys()) {
      factory->notifyConfigurationChange(key);
    }
  }
}

/**
 * Convert ID3v2.3 to ID3v2.4 tags.
 */
void Kid3Application::convertToId3v24()
{
  emit fileSelectionUpdateRequested();
  SelectedTaggedFileIterator it(getRootIndex(),
                                getFileSelectionModel(),
                                false);
  while (it.hasNext()) {
    TaggedFile* taggedFile = it.next();
    taggedFile->readTags(false);
    if (taggedFile->hasTagV2() && !taggedFile->isChanged()) {
      QString tagFmt = taggedFile->getTagFormatV2();
      if (tagFmt.length() >= 7 && tagFmt.startsWith(QLatin1String("ID3v2.")) &&
          tagFmt[6] < QLatin1Char('4')) {
        if ((taggedFile->taggedFileFeatures() &
             (TaggedFile::TF_ID3v23 | TaggedFile::TF_ID3v24)) ==
              TaggedFile::TF_ID3v23) {
          FrameCollection frames;
          taggedFile->getAllFramesV2(frames);
          FrameFilter flt;
          flt.enableAll();
          taggedFile->deleteFramesV2(flt);

          // The file has to be reread to write ID3v2.4 tags
          taggedFile = FileProxyModel::readWithId3V24(taggedFile);

          // Restore the frames
          FrameFilter frameFlt;
          frameFlt.enableAll();
          taggedFile->setFramesV2(frames.copyEnabledFrames(frameFlt), false);
        }

        // Write the file with ID3v2.4 tags
        bool renamed;
        int storedFeatures = taggedFile->activeTaggedFileFeatures();
        taggedFile->setActiveTaggedFileFeatures(TaggedFile::TF_ID3v24);
        taggedFile->writeTags(true, &renamed,
                              FileConfig::instance().m_preserveTime);
        taggedFile->setActiveTaggedFileFeatures(storedFeatures);
        taggedFile->readTags(true);
      }
    }
  }
  emit selectedFilesUpdated();
}

/**
 * Convert ID3v2.4 to ID3v2.3 tags.
 */
void Kid3Application::convertToId3v23()
{
  emit fileSelectionUpdateRequested();
  SelectedTaggedFileIterator it(getRootIndex(),
                                getFileSelectionModel(),
                                false);
  while (it.hasNext()) {
    TaggedFile* taggedFile = it.next();
    taggedFile->readTags(false);
    if (taggedFile->hasTagV2() && !taggedFile->isChanged()) {
      QString tagFmt = taggedFile->getTagFormatV2();
      QString ext = taggedFile->getFileExtension();
      if (tagFmt.length() >= 7 && tagFmt.startsWith(QLatin1String("ID3v2.")) &&
          tagFmt[6] > QLatin1Char('3') &&
          (ext == QLatin1String(".mp3") || ext == QLatin1String(".mp2") ||
           ext == QLatin1String(".aac"))) {
        if (!(taggedFile->taggedFileFeatures() & TaggedFile::TF_ID3v23)) {
          FrameCollection frames;
          taggedFile->getAllFramesV2(frames);
          FrameFilter flt;
          flt.enableAll();
          taggedFile->deleteFramesV2(flt);

          // The file has to be reread to write ID3v2.3 tags
          taggedFile = FileProxyModel::readWithId3V23(taggedFile);

          // Restore the frames
          FrameFilter frameFlt;
          frameFlt.enableAll();
          taggedFile->setFramesV2(frames.copyEnabledFrames(frameFlt), false);
        }

        // Write the file with ID3v2.3 tags
        bool renamed;
        int storedFeatures = taggedFile->activeTaggedFileFeatures();
        taggedFile->setActiveTaggedFileFeatures(TaggedFile::TF_ID3v23);
        taggedFile->writeTags(true, &renamed,
                              FileConfig::instance().m_preserveTime);
        taggedFile->setActiveTaggedFileFeatures(storedFeatures);
        taggedFile->readTags(true);
      }
    }
  }
  emit selectedFilesUpdated();
}

/**
 * Get value of frame.
 * To get binary data like a picture, the name of a file to write can be
 * added after the @a name, e.g. "Picture:/path/to/file".
 *
 * @param tagMask tag bit (1 for tag 1, 2 for tag 2)
 * @param name    name of frame (e.g. "Artist")
 */
QString Kid3Application::getFrame(TrackData::TagVersion tagMask,
                                  const QString& name)
{
  QString frameName(name);
  QString dataFileName;
  int colonIndex = frameName.indexOf(QLatin1Char(':'));
  if (colonIndex != -1) {
    dataFileName = frameName.mid(colonIndex + 1);
    frameName.truncate(colonIndex);
  }
  FrameTableModel* ft = (tagMask & TrackData::TagV2) ? m_framesV2Model :
    m_framesV1Model;
  FrameCollection::const_iterator it = ft->frames().findByName(frameName);
  if (it != ft->frames().end()) {
    if (!dataFileName.isEmpty()) {
      bool isSylt = it->getInternalName().startsWith(QLatin1String("SYLT"));
      if (isSylt ||
          it->getInternalName().startsWith(QLatin1String("ETCO"))) {
        QFile file(dataFileName);
        if (file.open(QIODevice::WriteOnly)) {
          TimeEventModel timeEventModel;
          if (isSylt) {
            timeEventModel.setType(TimeEventModel::SynchronizedLyrics);
            timeEventModel.fromSyltFrame(it->getFieldList());
          } else {
            timeEventModel.setType(TimeEventModel::EventTimingCodes);
            timeEventModel.fromEtcoFrame(it->getFieldList());
          }
          QTextStream stream(&file);
          const FrameCollection& frames = ft->frames();
          timeEventModel.toLrcFile(stream, frames.getTitle(),
                                   frames.getArtist(), frames.getAlbum());
          file.close();
        }
      } else {
        PictureFrame::writeDataToFile(*it, dataFileName);
      }
    }
    return it->getValue();
  } else {
    return QLatin1String("");
  }
}

/**
 * Set value of frame.
 * For tag 2 (@a tagMask 2), if no frame with @a name exists, a new frame
 * is added, if @a value is empty, the frame is deleted.
 * To add binary data like a picture, a file can be added after the
 * @a name, e.g. "Picture:/path/to/file".
 *
 * @param tagMask tag bit (1 for tag 1, 2 for tag 2)
 * @param name    name of frame (e.g. "Artist")
 * @param value   value of frame
 */
bool Kid3Application::setFrame(TrackData::TagVersion tagMask,
                               const QString& name, const QString& value)
{
  QString frameName(name);
  QString dataFileName;
  int colonIndex = frameName.indexOf(QLatin1Char(':'));
  if (colonIndex != -1) {
    dataFileName = frameName.mid(colonIndex + 1);
    frameName.truncate(colonIndex);
  }
  FrameTableModel* ft = (tagMask & TrackData::TagV2) ? m_framesV2Model :
    m_framesV1Model;
  FrameCollection frames(ft->frames());
  FrameCollection::const_iterator it = frames.findByName(frameName);
  if (it != frames.end()) {
    bool isPicture, isSylt;
    if (!dataFileName.isEmpty() && (tagMask & 2) != 0 &&
        ((isPicture = (it->getType() == Frame::FT_Picture)) ||
         (isSylt = it->getName().startsWith(QLatin1String("SYLT"))) ||
         it->getName().startsWith(QLatin1String("ETCO")))) {
      if (isPicture) {
        deleteFrame(it->getName());
        PictureFrame frame;
        PictureFrame::setDescription(frame, value);
        PictureFrame::setDataFromFile(frame, dataFileName);
        PictureFrame::setMimeTypeFromFileName(frame, dataFileName);
        addFrame(&frame);
      } else {
        QFile file(dataFileName);
        if (file.open(QIODevice::ReadOnly)) {
          QTextStream stream(&file);
          Frame frame(*it);
          Frame::setField(frame, Frame::Field::ID_Description, value);
          deleteFrame(it->getName());
          TimeEventModel timeEventModel;
          if (isSylt) {
            timeEventModel.setType(TimeEventModel::SynchronizedLyrics);
            timeEventModel.fromLrcFile(stream);
            timeEventModel.toSyltFrame(frame.fieldList());
          } else {
            timeEventModel.setType(TimeEventModel::EventTimingCodes);
            timeEventModel.fromLrcFile(stream);
            timeEventModel.toEtcoFrame(frame.fieldList());
          }
          file.close();
          addFrame(&frame);
        }
      }
    } else if (value.isEmpty() && (tagMask & 2) != 0) {
      deleteFrame(it->getName());
    } else {
      Frame& frame = const_cast<Frame&>(*it);
      frame.setValueIfChanged(value);
      ft->transferFrames(frames);
      ft->selectChangedFrames();
      emit fileSelectionUpdateRequested();
      emit selectedFilesUpdated();
    }
    return true;
  } else if (tagMask & 2) {
    Frame frame(Frame::ExtendedType(frameName), value, -1);
    bool isPicture, isSylt;
    if (!dataFileName.isEmpty() &&
        ((isPicture = (frame.getType() == Frame::FT_Picture)) ||
         (isSylt = frame.getInternalName().startsWith(QLatin1String("SYLT"))) ||
         frame.getInternalName().startsWith(QLatin1String("ETCO")))) {
      if (isPicture) {
        PictureFrame::setFields(frame);
        PictureFrame::setDescription(frame, value);
        PictureFrame::setDataFromFile(frame, dataFileName);
        PictureFrame::setMimeTypeFromFileName(frame, dataFileName);
      } else {
        QFile file(dataFileName);
        if (file.open(QIODevice::ReadOnly)) {
          Frame::Field field;
          Frame::FieldList& fields = frame.fieldList();
          fields.clear();
          field.m_id = Frame::Field::ID_Description;
          field.m_value = value;
          fields.append(field);
          field.m_id = Frame::Field::ID_Data;
          field.m_value = QVariant(QVariant::List);
          fields.append(field);
          QTextStream stream(&file);
          TimeEventModel timeEventModel;
          if (isSylt) {
            timeEventModel.setType(TimeEventModel::SynchronizedLyrics);
            timeEventModel.fromLrcFile(stream);
            timeEventModel.toSyltFrame(frame.fieldList());
          } else {
            timeEventModel.setType(TimeEventModel::EventTimingCodes);
            timeEventModel.fromLrcFile(stream);
            timeEventModel.toEtcoFrame(frame.fieldList());
          }
          file.close();
        }
      }
    }
    addFrame(&frame);
    return true;
  }
  return false;
}
