/**
 * \file musicbrainzconfig.h
 * MusicBrainz configuration.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 13 Sep 2005
 *
 * Copyright (C) 2005-2007  Urs Fleisch
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

#ifndef MUSICBRAINZCONFIG_H
#define MUSICBRAINZCONFIG_H

#include "serverimporterconfig.h"

/**
 * MusicBrainz configuration.
 */
class MusicBrainzConfig : public StoredConfig<MusicBrainzConfig, ServerImporterConfig> {
public:
  /**
   * Constructor.
   */
  MusicBrainzConfig();

  /**
   * Destructor.
   */
  virtual ~MusicBrainzConfig();

private:
  friend MusicBrainzConfig&
  StoredConfig<MusicBrainzConfig, ServerImporterConfig>::instance();

  /** Index in configuration storage */
  static int s_index;
};

#endif // MUSICBRAINZCONFIG_H
