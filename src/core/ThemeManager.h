/*

  Copyright (c) 2026 Jayachandran Dharuman

  This file is part of CANgaroo.

  cangaroo is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  cangaroo is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with cangaroo.  If not, see <http://www.gnu.org/licenses/>.

*/

#pragma once

#include <QObject>
#include <QColor>
#include <QPalette>
#include <QString>

struct ThemeColors {
    QColor window;
    QColor windowText;
    QColor base;
    QColor alternateBase;
    QColor toolTipBase;
    QColor toolTipText;
    QColor text;
    QColor button;
    QColor buttonText;
    QColor brightText;
    QColor link;
    QColor highlight;
    QColor highlightedText;
    
    // Graph specific
    QColor graphBackground;
    QColor graphGrid;
    QColor graphAxisText;
    QColor graphCursor;
};

class ThemeManager : public QObject
{
    Q_OBJECT
public:
    enum Theme {
        Light,
        Dark
    };

    static ThemeManager& instance();

    // When nativeStyling is true, CANgaroo defers to the active QStyle and the
    // desktop platform theme: no custom palette or global stylesheet is applied.
    void applyTheme(Theme theme, bool nativeStyling = false);
    Theme currentTheme() const { return _currentTheme; }
    bool nativeStyling() const { return _nativeStyling; }
    bool isDarkMode() const { return _currentTheme == Dark; }
    
    const ThemeColors& colors() const { return _colors; }

signals:
    void themeChanged(Theme theme);

private:
    explicit ThemeManager(QObject *parent = nullptr);
    Theme _currentTheme;
    bool _nativeStyling = false;
    ThemeColors _colors;

    void updateColors(Theme theme);
    void applyStyleSheet(Theme theme);
    void applyPalette(Theme theme);
};
