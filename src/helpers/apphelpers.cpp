#include "apphelpers.h"

#include <QApplication>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QSvgRenderer>

namespace AppHelpers
{

    void setRowColor(QTreeWidgetItem *item, bool enabled)
    {
        QColor colEnabled(200, 255, 200);
        QColor colDisabled(255, 220, 220);

        QColor bg = enabled ? colEnabled : colDisabled;

        for (int c = 0; c < item->columnCount(); ++c)
            item->setBackground(c, bg);
    }

    QColor toPastel(const QColor &c)
    {

        int r = (c.red() + 255) / 2;
        int g = (c.green() + 255) / 2;
        int b = (c.blue() + 255) / 2;
        QColor pastel;
        pastel.setRgb(r, g, b);
        return pastel;
    }

    QIcon recoloredSvgIcon(const QString &resourcePath, const QColor &color)
    {
        QSvgRenderer renderer(resourcePath);
        if (!renderer.isValid())
            return QIcon();

        QIcon icon;
        for (const int sz : {16, 18, 22, 24, 32})
        {
            QPixmap glyph(sz, sz);
            glyph.fill(Qt::transparent);
            QPainter gp(&glyph);
            renderer.render(&gp);
            gp.end();

            QPixmap colored(sz, sz);
            colored.fill(Qt::transparent);
            QPainter cp(&colored);
            cp.drawPixmap(0, 0, glyph);
            cp.setCompositionMode(QPainter::CompositionMode_SourceIn);
            cp.fillRect(colored.rect(), color);
            cp.end();

            icon.addPixmap(colored);
        }
        return icon;
    }

    QIcon themedIcon(const QString &themeName, const QString &fallbackResourcePath)
    {
        const QColor color = QApplication::palette().color(QPalette::WindowText);
        return QIcon::fromTheme(themeName, recoloredSvgIcon(fallbackResourcePath, color));
    }

}
