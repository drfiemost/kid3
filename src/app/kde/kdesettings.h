/**
 * \file kdesettings.h
 * Wrapper for KDE application settings.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 07 Apr 2013
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

#ifndef KDESETTINGS_H
#define KDESETTINGS_H

#include "isettings.h"

class KConfig;
class KConfigGroup;

/**
 * Wrapper for KDE application settings.
 */
class KdeSettings : public ISettings {
public:
  /**
   * Constructor.
   * @param config KDE settings
   */
  explicit KdeSettings(KConfig* config);

  /**
   * Destructor.
   */
  virtual ~KdeSettings();

  /**
   * Use settings subgroup.
   * @param prefix group name
   */
  virtual void beginGroup(const QString& prefix);

  /**
   * Finnish using settings subgroup.
   */
  virtual void endGroup();

  /**
   * Set value for setting.
   * @param key name of setting
   * @param value value for setting
   */
  virtual void setValue(const QString& key, const QVariant& value);

  /**
   * Get value for setting.
   * @param key name of setting
   * @param defaultValue default value
   * @return value of setting as variant.
   */
  virtual QVariant value(const QString& key,
                         const QVariant& defaultValue) const;

  /**
   * Remove setting.
   * @param key name of setting
   */
  virtual void remove(const QString& key);

  /**
   * Check if setting exists.
   * @param key name of setting
   * @return true if setting exists.
   */
  virtual bool contains(const QString& key) const;

  /**
   * Write unsaved changes to permanent storage.
   */
  virtual void sync();

private:
  KConfig* m_config;
  KConfigGroup* m_group;
};

#endif // KDESETTINGS_H
