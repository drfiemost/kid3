/**
 * \file qmlimageprovider.h
 * Image provider to get images from QML code.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 21 Jun 2014
 *
 * Copyright (C) 2014  Urs Fleisch
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

#ifndef QMLIMAGEPROVIDER_H
#define QMLIMAGEPROVIDER_H

#include <QtGlobal>
#include "pixmapprovider.h"

#if QT_VERSION >= 0x050000
#include <QQuickImageProvider>
#else
#include <QDeclarativeImageProvider>
#endif

/**
 * Image provider to get images from QML code.
 *
 * The following source IDs are supported (starting with "image://kid3/"):
 * - "fileicon/" followed by "null", "notag", "v1", "v2", "v1v2", or "modified",
 * - "data" followed by a changing string to force loading of the image set with
 *   TaggedFileIconProvider::setImageData().
 */
class QmlImageProvider : public 
#if QT_VERSION >= 0x050000
QQuickImageProvider
#else
QDeclarativeImageProvider
#endif
, public PixmapProvider {
public:
  /**
   * Constructor.
   * @param iconProvider icon provider to use
   */
  explicit QmlImageProvider(TaggedFileIconProvider* iconProvider);

  /**
   * Destructor.
   */
  virtual ~QmlImageProvider();

  /**
   * Request a pixmap.
   * @param id ID of pixmap to get, "image://kid3/fileicon/..." or
   *  "image://kid3/data..."
   * @param size the original size of the image is returned here
   * @param requestedSize the size requested via the Image.sourceSize property
   * @return pixmap for ID.
   */
  virtual QPixmap requestPixmap(const QString& id, QSize* size,
                                const QSize& requestedSize);
};

#endif // QMLIMAGEPROVIDER_H
