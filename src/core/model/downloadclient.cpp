/**
 * \file downloadclient.cpp
 * Client to download via http.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 12 Jun 2011
 *
 * Copyright (C) 2011-2014  Urs Fleisch
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

#include "downloadclient.h"
#if QT_VERSION >= 0x050100
#include <QRegularExpression>
#else
#include <QRegExp>
#endif
#include "importconfig.h"

/**
 * Constructor.
 *
 * @param netMgr network access manager
 */
DownloadClient::DownloadClient(QNetworkAccessManager* netMgr) :
  HttpClient(netMgr), m_canceled(false)
{
  connect(this, SIGNAL(bytesReceived(QByteArray)),
          this, SLOT(requestFinished(QByteArray)));
}

/**
 * Destructor.
 */
DownloadClient::~DownloadClient()
{
}

/**
 * Send a download request.
 *
 * @param url URL of resource to download
 */
void DownloadClient::startDownload(const QUrl& url)
{
  m_canceled = false;
  m_url = url;
  emit downloadStarted(m_url.toString());
  emit progress(tr("Ready."), 0, 0);
  sendRequest(m_url);
}

/**
 * Cancel a download.
 */
void DownloadClient::cancelDownload()
{
  m_canceled = true;
  abort();
  emit aborted();
}

/**
 * Handle response when request is finished.
 * downloadFinished() is emitted.
 *
 * @param data received data
 */
void DownloadClient::requestFinished(const QByteArray& data)
{
  if (!m_canceled) {
    emit downloadFinished(data, getContentType(), m_url.toString());
  }
}

/**
 * Get the URL of an image file.
 * The input URL is transformed using the match picture URL table to
 * get the URL of an image file.
 *
 * @param url URL from image drag
 *
 * @return URL of image file, empty if no image URL found.
 */
QUrl DownloadClient::getImageUrl(const QUrl& url)
{
  QString urlStr = url.toString();
  if (urlStr.endsWith(QLatin1String(".jpg"), Qt::CaseInsensitive) ||
      urlStr.endsWith(QLatin1String(".jpeg"), Qt::CaseInsensitive) ||
      urlStr.endsWith(QLatin1String(".png"), Qt::CaseInsensitive))
    return url;

  QUrl imgurl;
  QMap<QString, QString> urlMap =
      ImportConfig::instance().matchPictureUrlMap();
  for (QMap<QString, QString>::const_iterator it = urlMap.constBegin();
       it != urlMap.constEnd();
       ++it) {
#if QT_VERSION >= 0x050100
    QRegularExpression re(it.key());
    QRegularExpressionMatch match = re.match(
          urlStr, 0, QRegularExpression::NormalMatch,
          QRegularExpression::AnchoredMatchOption);
    if (match.hasMatch())
#else
    QRegExp re(it.key());
    if (re.exactMatch(urlStr))
#endif
    {
      QString newUrl = urlStr;
      newUrl.replace(re, *it);
      if (newUrl.indexOf(QLatin1String("%25")) != -1) {
        // double URL encoded: first decode
        newUrl = QUrl::fromPercentEncoding(newUrl.toUtf8());
      }
      if (newUrl.indexOf(QLatin1String("%2F")) != -1) {
        // URL encoded: decode
        newUrl = QUrl::fromPercentEncoding(newUrl.toUtf8());
      }
#if QT_VERSION >= 0x050000
      imgurl.setUrl(newUrl);
#else
      if (!newUrl.contains(QLatin1Char('%'))) {
        imgurl.setUrl(newUrl);
      } else {
        imgurl.setEncodedUrl(newUrl.toAscii());
      }
#endif
      break;
    }
  }
  return imgurl;
}
