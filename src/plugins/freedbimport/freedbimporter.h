/**
 * \file freedbimporter.h
 * freedb.org importer.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 18 Jan 2004
 *
 * Copyright (C) 2004-2011  Urs Fleisch
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

#ifndef FREEDBIMPORTER_H
#define FREEDBIMPORTER_H

#include "serverimporter.h"

/**
 * freedb.org importer.
 */
class FreedbImporter : public ServerImporter
{
public:
  /**
   * Constructor.
   *
   * @param netMgr network access manager
   * @param trackDataModel track data to be filled with imported values
   */
  FreedbImporter(QNetworkAccessManager* netMgr,
                 TrackDataModel *trackDataModel);

  /**
   * Destructor.
   */
  virtual ~FreedbImporter() override = default;

  /**
   * Name of import source.
   * @return name.
   */
  virtual const char* name() const override;

  /** NULL-terminated array of server strings, 0 if not used */
  virtual const char** serverList() const override;

  /** default server, 0 to disable */
  virtual const char* defaultServer() const override;

  /** default CGI path, 0 to disable */
  virtual const char* defaultCgiPath() const override;

  /** anchor to online help, 0 to disable */
  virtual const char* helpAnchor() const override;

  /** configuration, 0 if not used */
  virtual ServerImporterConfig* config() const override;

  /**
   * Process finished findCddbAlbum request.
   *
   * @param searchStr search data received
   */
  virtual void parseFindResults(const QByteArray& searchStr) override;

  /**
   * Parse result of album request and populate m_trackDataModel with results.
   *
   * @param albumStr album data received
   */
  virtual void parseAlbumResults(const QByteArray& albumStr) override;

  /**
   * Send a query command to search on the server.
   *
   * @param cfg      import source configuration
   * @param artist   artist to search
   * @param album    album to search
   */
  virtual void sendFindQuery(
    const ServerImporterConfig* cfg,
    const QString& artist, const QString& album) override;

  /**
   * Send a query command to fetch the track list
   * from the server.
   *
   * @param cfg      import source configuration
   * @param cat      category
   * @param id       ID
   */
  virtual void sendTrackListQuery(
    const ServerImporterConfig* cfg, const QString& cat, const QString& id) override;
};

#endif
