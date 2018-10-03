/**
 * \file musicbrainzimportplugin.cpp
 * MusicBrainz import plugin.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 15 Jul 2013
 *
 * Copyright (C) 2013  Urs Fleisch
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

#include "musicbrainzimportplugin.h"
#include "musicbrainzimporter.h"

static const QLatin1String IMPORTER_NAME("MusicBrainzImport");

/*!
 * Constructor.
 * @param parent parent object
 */
MusicBrainzImportPlugin::MusicBrainzImportPlugin(QObject* parent) : QObject(parent)
{
  setObjectName(QLatin1String("MusicBrainzImport"));
}

/**
 * Get keys of available server importers.
 * @return list of keys.
 */
QStringList MusicBrainzImportPlugin::serverImporterKeys() const
{
  return {IMPORTER_NAME};
}

/**
 * Create server importer.
 * @param key server importer key
 * @param netMgr network access manager
 * @param trackDataModel track data to be filled with imported values
 * @return server importer instance, 0 if key unknown.
 * @remarks The caller takes ownership of the returned instance.
 */
ServerImporter* MusicBrainzImportPlugin::createServerImporter(
    const QString& key,
    QNetworkAccessManager* netMgr, TrackDataModel* trackDataModel)
{
  if (key == IMPORTER_NAME) {
    return new MusicBrainzImporter(netMgr, trackDataModel);
  }
  return nullptr;
}
