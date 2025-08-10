#ifndef IMAGEUTIL_H
#define IMAGEUTIL_H

#include <QPixmap>
#include <QList>
#include <QVector3D>
#include "coloroctree.h"

#define IMAGE_CALC_PIXEL_MAX_SIZE 128 // 计算的最大边长（大图缩小）

class ImageUtil
{
public:
    static QColor getImageAverageColor(QImage image, int maxPool = IMAGE_CALC_PIXEL_MAX_SIZE);

    static QList<ColorOctree::ColorCount> extractImageThemeColors(QImage image, int count);

    static QList<QColor> extractImageThemeColorsInPalette(QImage image, QList<QColor> paletteColors, int needCount);

    static QColor getInvertColor(QColor color);

    static bool getBgFgColor(QList<ColorOctree::ColorCount> colors, QColor *bg, QColor *fg);

    static bool getBgFgSgColor(QList<ColorOctree::ColorCount> colors, QColor *bg, QColor *fg, QColor *sg);

    static bool getBgFgSgColor(QList<ColorOctree::ColorCount> colors, QColor *bg, QColor *fg, QColor *sbg, QColor *sfg);

    static QColor getFastestColor(QColor bg, QList<QColor> palette);

    static QColor getFastestColor(QColor bg, QList<ColorOctree::ColorCount> palette, int enableCount = 2);

    static QColor randomColor();

    static double calculateLuminance(QColor c);

    static QColor getCloestColorByRGB(QColor color, const QList<QColor>& colors);

    static QColor getVisuallyClosestColorByCIELAB(const QColor& target, const QList<QColor>& colors, const QList<QVector3D>& colorLabs);

    static QVector3D rgbToXyz(const QColor& color);

    static QVector3D xyzToLab(const QVector3D& xyz);

    static QVector3D rgbToLab(const QColor& color);

    static double deltaE2000(const QVector3D& lab1, const QVector3D& lab2);

    static double deltaE94(const QVector3D& lab1, const QVector3D& lab2);

    static double perceptualRgbDistance(const QColor& c1, const QColor& c2);
};

#endif // IMAGEUTIL_H
