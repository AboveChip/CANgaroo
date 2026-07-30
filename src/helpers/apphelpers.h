#pragma once
#include <QTreeWidgetItem>
#include <QColor>
#include <QIcon>
#include <QString>
namespace AppHelpers
{
    void setRowColor(QTreeWidgetItem *item, bool enabled);
    QColor toPastel(const QColor &c);

    // Renders a bundled SVG and recolors it to `color` through an alpha mask,
    // so the icon stays readable in both light and dark themes regardless of
    // the SVG's own fill/stroke. Needed because QSvgRenderer does not resolve
    // `stroke="currentColor"` and paints such icons black.
    [[nodiscard]] QIcon recoloredSvgIcon(const QString &resourcePath, const QColor &color);

    // Prefers the desktop icon theme (freedesktop name) and falls back to a
    // bundled SVG recolored to the current palette's WindowText — the icon
    // source of truth when no icon theme is available (e.g. in the AppImage).
    [[nodiscard]] QIcon themedIcon(const QString &themeName, const QString &fallbackResourcePath);
}
