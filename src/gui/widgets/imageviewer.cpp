/**
 * \file imageviewer.cpp
 * Window to view image.
 *
 * \b Project: Kid3
 * \author Urs Fleisch
 * \date 10 Jun 2009
 *
 * Copyright (C) 2003-2013  Urs Fleisch
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

#include "imageviewer.h"
#include <QImage>
#include <QLabel>
#include <QPushButton>
#include <QApplication>
#include <QDesktopWidget>
#include <QVBoxLayout>

/**
 * Constructor.
 *
 * @param parent parent widget
 * @param img    image to display in window
 */
ImageViewer::ImageViewer(QWidget* parent, const QImage& img) :
  QDialog(parent)
{
  setObjectName(QLatin1String("ImageViewer"));
  setModal(true);
  setWindowTitle(tr("View Picture"));
  QVBoxLayout* vlayout = new QVBoxLayout(this);
  QHBoxLayout* hlayout = new QHBoxLayout;
  QSpacerItem* hspacer = new QSpacerItem(16, 0, QSizePolicy::Expanding,
                                         QSizePolicy::Minimum);
  m_image = new QLabel(this);
  QPushButton* closeButton = new QPushButton(tr("&Close"), this);
  m_image->setScaledContents(true);
  QSize imageSize(img.size());
  QSize desktopSize(QApplication::desktop()->availableGeometry().size());
  desktopSize -= QSize(12, 12 + vlayout->spacing() + closeButton->height() +
                       vlayout->margin());
  QPixmap pm = imageSize.width() > desktopSize.width() ||
               imageSize.height() > desktopSize.height()
      ? QPixmap::fromImage(img.scaled(desktopSize, Qt::KeepAspectRatio))
      : QPixmap::fromImage(img);
#if QT_VERSION >= 0x050500
  // Try workaround for QTBUG-46846,
  // images are cropped on high pixel density displays.
  pm.setDevicePixelRatio(m_image->devicePixelRatio());
#endif
  m_image->setPixmap(pm);
  vlayout->addWidget(m_image);
  hlayout->addItem(hspacer);
  hlayout->addWidget(closeButton);
  connect(closeButton, SIGNAL(clicked()), this, SLOT(accept()));
  vlayout->addLayout(hlayout);
}


