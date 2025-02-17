/**
 * \file playlistcreator.cpp
 * Playlist creator.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 21 Sep 2009
 *
 * Copyright (C) 2009-2013  Urs Fleisch
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

#include "playlistcreator.h"
#include <QDir>
#include <QUrl>
#include <QFile>
#include <QTextStream>
#include "playlistconfig.h"
#include "fileconfig.h"
#include "formatconfig.h"
#include "taggedfile.h"
#include "trackdata.h"
#include "fileproxymodel.h"
#include "config.h"

/**
 * Constructor.
 *
 * @param topLevelDir top-level directory of playlist
 * @param cfg         playlist configuration
 */
PlaylistCreator::PlaylistCreator(const QString& topLevelDir,
                                 const PlaylistConfig& cfg) :
  m_cfg(cfg)
{
  if (m_cfg.location() == PlaylistConfig::PL_TopLevelDirectory) {
    m_playlistDirName = topLevelDir;
    if (!m_playlistDirName.endsWith(QLatin1Char('/'))) {
      m_playlistDirName += QLatin1Char('/');
    }
  }
}

/**
 * Write playlist containing added Entry elements.
 *
 * @return true if ok.
 */
bool PlaylistCreator::write()
{
  bool ok = true;
  if (!m_playlistFileName.isEmpty()) {
    QFile file(m_playlistDirName + m_playlistFileName);
    ok = file.open(QIODevice::WriteOnly);
    if (ok) {
      QTextStream stream(&file);
      QString codecName = FileConfig::instance().textEncoding();
      if (codecName != QLatin1String("System")) {
        stream.setCodec(codecName.toLatin1());
      }

      switch (m_cfg.format()) {
        case PlaylistConfig::PF_M3U:
          if (m_cfg.writeInfo()) {
            stream << "#EXTM3U\n";
          }
          for (QMap<QString, Entry>::const_iterator it = m_entries.begin();
               it != m_entries.end();
               ++it) {
            if (m_cfg.writeInfo()) {
              stream << QString(QLatin1String("#EXTINF:%1,%2\n")).
                arg((*it).duration).arg((*it).info);
            }
            stream << (*it).filePath << "\n";
          }
          break;
        case PlaylistConfig::PF_PLS:
        {
          unsigned nr = 1;
          stream << "[playlist]\n";
          stream << QString(QLatin1String("NumberOfEntries=%1\n")).arg(m_entries.size());
          for (QMap<QString, Entry>::const_iterator it = m_entries.begin();
               it != m_entries.end();
               ++it) {
            stream << QString(QLatin1String("File%1=%2\n")).arg(nr).arg((*it).filePath);
            if (m_cfg.writeInfo()) {
              stream << QString(QLatin1String("Title%1=%2\n")).arg(nr).arg((*it).info);
              stream << QString(QLatin1String("Length%1=%2\n")).arg(nr).arg((*it).duration);
            }
            ++nr;
          }
          stream << "Version=2\n";
        }
        break;
        case PlaylistConfig::PF_XSPF:
        {
          stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
          QString line = QLatin1String("<playlist version=\"1\" xmlns=\"http://xspf.org/ns/0/\"");
          if (!m_cfg.useFullPath()) {
            QUrl url(m_playlistDirName);
            url.setScheme(QLatin1String("file"));
            line += QString(QLatin1String(" xml:base=\"%1\"")).arg(QString::fromLatin1(url.toEncoded().data()));
          }
          line += QLatin1String(">\n");
          stream << line;
          stream << "  <trackList>\n";

          for (QMap<QString, Entry>::const_iterator it = m_entries.begin();
               it != m_entries.end();
               ++it) {
            stream << "    <track>\n";
            QUrl url((*it).filePath);
            if (m_cfg.useFullPath()) {
              url.setScheme(QLatin1String("file"));
            }
            stream << QString(QLatin1String("      <location>%1</location>\n")).arg(
              QString::fromLatin1(url.toEncoded().data()));
            if (m_cfg.writeInfo()) {
              // the info is already formatted in the case of XSPF
              stream << (*it).info;
            }
            stream << "    </track>\n";
          }

          stream << "  </trackList>\n";
          stream << "</playlist>\n";
        }
        break;
      }

      m_entries.clear();
      m_playlistFileName = QLatin1String("");
      file.close();
    }
  }
  return ok;
}


/**
 * Constructor.
 *
 * @param index model index
 * @param ctr  associated playlist creator
 */
PlaylistCreator::Item::Item(const QModelIndex& index, PlaylistCreator& ctr) :
  m_ctr(ctr), m_taggedFile(FileProxyModel::getTaggedFileOfIndex(index)),
  m_trackData(0), m_isDir(false)
{
  if (m_taggedFile) {
    m_dirName = m_taggedFile->getDirname();
  } else {
    m_dirName = FileProxyModel::getPathIfIndexOfDir(index);
    m_isDir = !m_dirName.isNull();
  }
  if (!m_dirName.endsWith(QLatin1Char('/'))) {
    m_dirName += QLatin1Char('/');
  }
  // fix double separators
  m_dirName.replace(QLatin1String("//"), QLatin1String("/"));
}

/**
 * Destructor.
 */
PlaylistCreator::Item::~Item()
{
  delete m_trackData;
}

/**
 * Format string using tags and properties of item.
 *
 * @param format format string
 *
 * @return string with percent codes replaced.
 */
QString PlaylistCreator::Item::formatString(const QString& format)
{
  if (!m_trackData) {
    m_taggedFile = FileProxyModel::readTagsFromTaggedFile(m_taggedFile);
    m_trackData = new ImportTrackData(*m_taggedFile, Frame::TagVAll);
  }
  return m_trackData->formatString(format);
}

/**
 * Add item to playlist.
 * This operation will write a playlist if the configuration is set to write
 * a playlist in every directory and a new directory is entered.
 *
 * @return true if ok.
 */
bool PlaylistCreator::Item::add()
{
  bool ok = true;
  if (m_ctr.m_cfg.location() != PlaylistConfig::PL_TopLevelDirectory) {
    if (m_ctr.m_playlistDirName != m_dirName) {
      ok = m_ctr.write();
      m_ctr.m_playlistDirName = m_dirName;
    }
  }
  if (m_ctr.m_playlistFileName.isEmpty()) {
    if (!m_ctr.m_cfg.useFileNameFormat()) {
      m_ctr.m_playlistFileName = QDir(m_ctr.m_playlistDirName).dirName();
    } else {
      m_ctr.m_playlistFileName = formatString(m_ctr.m_cfg.fileNameFormat());

      // Replace illegal characters in the playlist file name.
      // Use replacements from the file name format config if enabled,
      // otherwise remove the characters.
      const FormatConfig& fnCfg = FilenameFormatConfig::instance();
      QMap<QString, QString> replaceMap;
      if (fnCfg.strRepEnabled()) {
        replaceMap = fnCfg.strRepMap();
      }
#ifdef Q_OS_WIN32
      static const char illegalChars[] = "<>:\"|?*\\/";
#else
      static const char illegalChars[] = "/";
#endif
      for (int i = 0;
           i < static_cast<int>(sizeof(illegalChars) / sizeof(illegalChars[0]));
           ++i) {
        QChar illegalChar = QLatin1Char(illegalChars[i]);
        QString replacement = replaceMap.value(illegalChar);
        m_ctr.m_playlistFileName.replace(illegalChar, replacement);
      }
    }
    QString ext;
    switch (m_ctr.m_cfg.format()) {
      case PlaylistConfig::PF_M3U:
        ext = QLatin1String(".m3u");
        break;
      case PlaylistConfig::PF_PLS:
        ext = QLatin1String(".pls");
        break;
      case PlaylistConfig::PF_XSPF:
        ext = QLatin1String(".xspf");
        break;
    }
    m_ctr.m_playlistFileName = FilenameFormatConfig::instance().joinFileName(
          m_ctr.m_playlistFileName, ext);
  }
  QString filePath = m_dirName + m_taggedFile->getFilename();
  if (!m_ctr.m_cfg.useFullPath() &&
      filePath.startsWith(m_ctr.m_playlistDirName)) {
    filePath = filePath.mid(m_ctr.m_playlistDirName.length());
  }
  QString sortKey;
  if (m_ctr.m_cfg.useSortTagField()) {
    sortKey = formatString(m_ctr.m_cfg.sortTagField());
  }
  sortKey += filePath;
  PlaylistCreator::Entry entry;
  entry.filePath = filePath;
  if (m_ctr.m_cfg.writeInfo()) {
    if (m_ctr.m_cfg.format() != PlaylistConfig::PF_XSPF) {
      entry.info = formatString(m_ctr.m_cfg.infoFormat());
    } else {
      entry.info = formatString(QLatin1String(
        "      <title>%{title}</title>\n"
        "      <creator>%{artist}</creator>\n"
        "      <album>%{album}</album>\n"
        "      <trackNum>%{track.1}</trackNum>\n"
        "      <duration>%{seconds}000</duration>\n"));
    }
    TaggedFile::DetailInfo detailInfo;
    m_taggedFile->getDetailInfo(detailInfo);
    entry.duration = detailInfo.duration;
  } else {
    entry.info = QString();
    entry.duration = 0;
  }
  m_ctr.m_entries.insert(sortKey, entry);
  return ok;
}
