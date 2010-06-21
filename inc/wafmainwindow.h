/***************************************************************************
 *  wafmeter - Woman Acceptance Factor measurement / Qt GUI class
 *
 *  2009-08-10 21:22:13
 *  Copyright  2007  Christophe Seyve
 *  Email cseyve@free.fr
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef WAFMAINWINDOW_H
#define WAFMAINWINDOW_H

#include <QtCore/QTimer>
#include <QtGui/QMainWindow>
#include "wafmeter.h"

namespace Ui
{
    class WAFMainWindow;
}

class WAFMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    WAFMainWindow(QWidget *parent = 0);
    ~WAFMainWindow();

private:
    Ui::WAFMainWindow *ui;
	QString m_path;

	QTimer m_timer;
	WAFMeter m_wafMeter;
	/** @brief Compute WAF and display result */
	void computeWAF(IplImage *);

private slots:
	void on_fileButton_pressed();
	void on_camButton_toggled(bool checked);
	void on_m_timer_timeout();
};

#endif // WAFMAINWINDOW_H
