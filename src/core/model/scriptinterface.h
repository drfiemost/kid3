/**
 * \file scriptinterface.h
 * D-Bus script adaptor.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 20 Dec 2007
 *
 * Copyright (C) 2007-2012  Urs Fleisch
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

#ifndef SCRIPTINTERFACE_H
#define SCRIPTINTERFACE_H

#include "config.h"
#ifdef HAVE_QTDBUS
#include <QDBusAbstractAdaptor>
#include <QStringList>

class Kid3Application;

/**
 * Adaptor class for interface net.sourceforge.Kid3
 * Create net.sourceforge.Kid3.xml with:
 * echo "#define HAVE_QTDBUS" >config.h
 * echo "#define HAVE_PHONON" >>config.h
 * qdbuscpp2xml scriptinterface.h >net.sourceforge.Kid3.xml
 * rm config.h
 */
class ScriptInterface : public QDBusAbstractAdaptor {
Q_OBJECT
Q_CLASSINFO("D-Bus Interface", "net.sourceforge.Kid3")
public:
  /**
   * Constructor.
   *
   * @param app parent application
   */
  explicit ScriptInterface(Kid3Application* app);

  /**
   * Destructor.
   */
  virtual ~ScriptInterface();

public slots:
  /**
   * Open file or directory.
   *
   * @param path path to file or directory
   *
   * @return true if ok.
   */
  bool openDirectory(const QString& path);

  /**
   * Save all modified files.
   *
   * @return true if ok,
   *         else the error message is available using getErrorMessage().
   */
  bool save();

  /**
   * Get a detailed error message provided by some methods.
   * @return detailed error message.
   */
  QString getErrorMessage() const;

  /**
   * Revert changes in the selected files.
   */
  void revert();

  /**
   * Import tags from a file.
   *
   * @param tagMask tag bit (1 for tag 1, 2 for tag 2)
   * @param path    path of file, "clipboard" for import from clipboard
   * @param fmtIdx  index of format
   *
   * @return true if ok.
   */
  bool importFromFile(int tagMask, const QString& path, int fmtIdx);

  /**
   * Import from tags.
   *
   * @param tagMask tag mask
   * @param source format to get source text from tags
   * @param extraction regular expression with frame names and captures to
   * extract from source text
   */
  void importFromTags(int tagMask,
                      const QString& source, const QString& extraction);

  /**
   * Start an automatic batch import.
   *
   * @param tagMask tag mask (bit 0 for tag 1, bit 1 for tag 2)
   * @param profileName name of batch import profile to use
   *
   * @return true if profile found.
   */
  bool batchImport(int tagMask, const QString& profileName);

  /**
   * Download album cover art into the picture frame of the selected files.
   *
   * @param url           URL of picture file or album art resource
   * @param allFilesInDir true to add the image to all files in the directory
   */
  void downloadAlbumArt(const QString& url, bool allFilesInDir);

  /**
   * Export tags to a file.
   *
   * @param tagMask tag bit (1 for tag 1, 2 for tag 2)
   * @param path    path of file, "clipboard" for export to clipboard
   * @param fmtIdx  index of format
   *
   * @return true if ok.
   */
  bool exportToFile(int tagMask, const QString& path, int fmtIdx);

  /**
   * Create a playlist.
   *
   * @return true if ok.
   */
  bool createPlaylist();

  /**
   * Quit the application.
   * Omitted Q_NOREPLY because the Qt 3 moc chokes on it.
   */
  void quit();

  /**
   * Select all files.
   */
  void selectAll();

  /**
   * Deselect all files.
   */
  void deselectAll();

  /**
   * Set the first file as the current file.
   *
   * @return true if there is a first file.
   */
  bool firstFile();

  /**
   * Set the previous file as the current file.
   *
   * @return true if there is a previous file.
   */
  bool previousFile();

  /**
   * Set the next file as the current file.
   *
   * @return true if there is a next file.
   */
  bool nextFile();

  /**
   * Select the first file.
   *
   * @return true if there is a first file.
   */
  bool selectFirstFile();

  /**
   * Select the previous file.
   *
   * @return true if there is a previous file.
   */
  bool selectPreviousFile();

  /**
   * Select the next file.
   *
   * @return true if there is a next file.
   */
  bool selectNextFile();

  /**
   * Select the current file.
   *
   * @return true if there is a current file.
   */
  bool selectCurrentFile();

  /**
   * Expand or collapse the current file item if it is a directory.
   * A file list item is a directory if getFileName() returns a name with
   * '/' as the last character.
   *
   * @return true if current file item is a directory.
   */
  bool expandDirectory();

  /**
   * Expand the file list.
   */
  void expandFileList();

  /**
   * Apply the file name format.
   */
  void applyFilenameFormat();

  /**
   * Apply the tag format.
   */
  void applyTagFormat();

  /**
   * Apply text encoding.
   */
  void applyTextEncoding();

  /**
   * Set the directory name from the tags.
   *
   * @param tagMask tag mask (bit 0 for tag 1, bit 1 for tag 2)
   * @param format  directory name format
   * @param create  true to create, false to rename
   *
   * @return true if ok,
   *         else the error message is available using getErrorMessage().
   */
  bool setDirNameFromTag(int tagMask, const QString& format, bool create);

  /**
   * Set subsequent track numbers in the selected files.
   *
   * @param tagMask      tag mask (bit 0 for tag 1, bit 1 for tag 2)
   * @param firstTrackNr number to use for first file
   */
  void numberTracks(int tagMask, int firstTrackNr);

  /**
   * Filter the files.
   *
   * @param expression filter expression
   */
  void filter(const QString& expression);

  /**
   * Convert ID3v2.3 tags to ID3v2.4.
   */
  void convertToId3v24();

  /**
   * Convert ID3v2.4 tags to ID3v2.3.
   */
  void convertToId3v23();

  /**
   * Get path of directory.
   *
   * @return absolute path of directory.
   */
  QString getDirectoryName();

  /**
   * Get name of current file.
   *
   * @return absolute file name, ends with "/" if it is a directory.
   */
  QString getFileName();

  /**
   * Set name of selected file.
   * The file will be renamed when the directory is saved.
   *
   * @param name file name.
   */
  void setFileName(const QString& name);

  /**
   * Set format to use when setting the filename from the tags.
   *
   * @param format file name format
   * @see setFileNameFromTag()
   */
  void setFileNameFormat(const QString& format);

  /**
   * Set the file names of the selected files from the tags.
   *
   * @param tagMask tag bit (1 for tag 1, 2 for tag 2)
   * @see setFileNameFormat()
   */
  void setFileNameFromTag(int tagMask);

  /**
   * Get value of frame.
   * To get binary data like a picture, the name of a file to write can be
   * added after the @a name, e.g. "Picture:/path/to/file".
   *
   * @param tagMask tag bit (1 for tag 1, 2 for tag 2)
   * @param name    name of frame (e.g. "artist")
   */
  QString getFrame(int tagMask, const QString& name);

  /**
   * Set value of frame.
   * For tag 2 (@a tagMask 2), if no frame with @a name exists, a new frame
   * is added, if @a value is empty, the frame is deleted.
   * To add binary data like a picture, a file can be added after the
   * @a name, e.g. "Picture:/path/to/file".
   *
   * @param tagMask tag bit (1 for tag 1, 2 for tag 2)
   * @param name    name of frame (e.g. "artist")
   * @param value   value of frame
   */
  bool setFrame(int tagMask, const QString& name, const QString& value);

  /**
   * Get all frames of a tag.
   *
   * @param tagMask tag bit (1 for tag 1, 2 for tag 2)
   *
   * @return list with alternating frame names and values.
   */
  QStringList getTag(int tagMask);

  /**
   * Get technical information about file.
   * Properties are Format, Bitrate, Samplerate, Channels, Duration,
   * Channel Mode, VBR, Tag 1, Tag 2.
   * Properties which are not available are omitted.
   *
   * @return list with alternating property names and values.
   */
  QStringList getInformation();

  /**
   * Set tag from file name.
   *
   * @param tagMask tag bit (1 for tag 1, 2 for tag 2)
   */
  void setTagFromFileName(int tagMask);

  /**
   * Set tag from other tag.
   *
   * @param tagMask tag bit (1 for tag 1, 2 for tag 2)
   */
  void setTagFromOtherTag(int tagMask);

  /**
   * Copy tag.
   *
   * @param tagMask tag bit (1 for tag 1, 2 for tag 2)
   */
  void copyTag(int tagMask);

  /**
   * Paste tag.
   *
   * @param tagMask tag bit (1 for tag 1, 2 for tag 2)
   */
  void pasteTag(int tagMask);

  /**
   * Remove tag.
   *
   * @param tagMask tag bit (1 for tag 1, 2 for tag 2)
   */
  void removeTag(int tagMask);

  /**
   * Reparse the configuration.
   * Automated configuration changes are possible by modifying
   * the configuration file and then reparsing the configuration.
   */
  void reparseConfiguration();

#if defined HAVE_PHONON || QT_VERSION >= 0x050000
  /**
   * Play selected audio files.
   */
  void playAudio();
#endif

private slots:
  void onRenameActionsScheduled();

private:
  Kid3Application* m_app;
  QString m_errorMsg;
};
#else // HAVE_QTDBUS

#include <QObject>
// Just to suppress moc "No relevant classes found" warning.
class ScriptInterface : public QObject {
  Q_OBJECT
};

#endif // HAVE_QTDBUS

#endif // SCRIPTINTERFACE_H
