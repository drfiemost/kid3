/**
 * \file textimporter.cpp
 * Import tags from text.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 19 Jun 2011
 *
 * Copyright (C) 2011  Urs Fleisch
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

#include "textimporter.h"
#include "importparser.h"
#include "trackdatamodel.h"

/**
 * Constructor.
 *
 * @param trackDataModel track data to be filled with imported values
 */
TextImporter::TextImporter(TrackDataModel* trackDataModel) :
  m_headerParser(new ImportParser),
  m_trackParser(new ImportParser),
  m_trackDataModel(trackDataModel)
{
}

/**
 * Destructor.
 */
TextImporter::~TextImporter()
{
  delete m_headerParser;
  delete m_trackParser;
}

/**
 * Look for album specific information (artist, album, year, genre) in
 * a header.
 *
 * @param frames frames to put resulting values in,
 *           fields which are not found are not touched.
 *
 * @return true if one or more field were found.
 */
bool TextImporter::parseHeader(FrameCollection& frames)
{
  int pos = 0;
  m_headerParser->setFormat(m_headerFormat);
  return m_headerParser->getNextTags(m_text, frames, pos);
}

/**
 * Update track data list with imported tags.
 *
 * @param text text to import
 * @param headerFormat header format
 * @param trackFormat track format
 *
 * @return true if tags were found.
 */
bool TextImporter::updateTrackData(
  const QString& text,
  const QString& headerFormat, const QString& trackFormat) {
  m_text = text;
  m_headerFormat = headerFormat;
  m_trackFormat = trackFormat;

  FrameCollection framesHdr;
  (void)parseHeader(framesHdr);

  FrameCollection frames(framesHdr);
  bool start = true;
  ImportTrackDataVector trackDataVector(m_trackDataModel->getTrackData());
  ImportTrackDataVector::iterator it = trackDataVector.begin();
  bool atTrackDataListEnd = (it == trackDataVector.end());
  while (getNextTags(frames, start)) {
    start = false;
    if (atTrackDataListEnd) {
      ImportTrackData trackData;
      trackData.setFrameCollection(frames);
      trackDataVector.push_back(trackData);
    } else {
      while (!atTrackDataListEnd && !it->isEnabled()) {
        ++it;
        atTrackDataListEnd = (it == trackDataVector.end());
      }
      if (!atTrackDataListEnd) {
        (*it).setFrameCollection(frames);
        ++it;
        atTrackDataListEnd = (it == trackDataVector.end());
      }
    }
    frames = framesHdr;
  }
  frames.clear();
  while (!atTrackDataListEnd) {
    if (it->isEnabled()) {
      if ((*it).getFileDuration() == 0) {
        it = trackDataVector.erase(it);
      } else {
        (*it).setFrameCollection(frames);
        (*it).setImportDuration(0);
        ++it;
      }
    } else {
      ++it;
    }
    atTrackDataListEnd = (it == trackDataVector.end());
  }

  if (!start) {
    /* start is false => tags were found */
    QList<int> trackDuration = getTrackDurations();
    if (!trackDuration.isEmpty()) {
      it = trackDataVector.begin();
      for (QList<int>::const_iterator tdit = trackDuration.begin();
           tdit != trackDuration.end();
           ++tdit) {
        if (it != trackDataVector.end()) {
          if (it->isEnabled()) {
            (*it).setImportDuration(*tdit);
          }
          ++it;
        } else {
          break;
        }
      }
    }
    m_trackDataModel->setTrackData(trackDataVector);
    return true;
  }
  return false;
}

/**
 * Get next line as frames from imported file or clipboard.
 *
 * @param frames frames
 * @param start true to start with the first line, false for all
 *              other lines
 *
 * @return true if ok (result in st),
 *         false if end of file reached.
 */
bool TextImporter::getNextTags(FrameCollection& frames, bool start)
{
  static int pos = 0;
  if (start || pos == 0) {
    pos = 0;
    m_trackParser->setFormat(m_trackFormat, true);
  }
  return m_trackParser->getNextTags(m_text, frames, pos);
}

/**
 * Get list with track durations.
 *
 * @return list with track durations,
 *         empty if no track durations found.
 */
QList<int> TextImporter::getTrackDurations()
{
  QList<int> lst;
  if (m_headerParser) {
    lst = m_headerParser->getTrackDurations();
  }
  if (lst.isEmpty() && m_trackParser) {
    lst = m_trackParser->getTrackDurations();
  }
  return lst;
}

/**
 * Import text from tags to other tags.
 *
 * @param sourceFormat format to create source text
 * @param extractionFormat regular expression to extract other tags
 * @param trackDataVector track data to process
 */
void TextImporter::importFromTags(
  const QString& sourceFormat,
  const QString& extractionFormat,
  ImportTrackDataVector& trackDataVector)
{
  ImportParser parser;
  parser.setFormat(extractionFormat);
  for (ImportTrackDataVector::iterator it = trackDataVector.begin();
       it != trackDataVector.end();
       ++it) {
    if (it->isEnabled()) {
      QString text(it->formatString(sourceFormat));
      int pos = 0;
      parser.getNextTags(text, *it, pos);
    }
  }
}
