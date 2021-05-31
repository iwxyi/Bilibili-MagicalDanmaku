#include <math.h>
#include "imageutil.h"

/**
 * 图片转换为颜色集
 */
QColor ImageUtil::getImageAverageColor(QImage image, int maxSize)
{
    int hCount = image.width();
    int vCount = image.height();

    if (hCount > maxSize || vCount > maxSize) // 数量过大，按比例缩减
    {
        double prop = (double)maxSize / qMax(hCount, vCount);
        image.scaledToWidth(image.width() * prop); // 缩放到最大大小
    }

    long long sumr = 0, sumg = 0, sumb = 0;

    int w = image.width(), h = image.height();
    int sumSize = w * h;
    for (int y = 0; y < h; y++)
    {
        QRgb *line = (QRgb *)image.scanLine(y);
        for (int x = 0; x < w; x++)
        {
            int r = qRed(line[x]), g = qGreen(line[x]), b = qBlue(line[x]);
            sumr += r;
            sumg += g;
            sumb += b;
        }
    }

    sumr /= sumSize;
    sumg /= sumSize;
    sumb /= sumSize;
    return QColor(sumr, sumg, sumb);
}

/** 
 * 获取图片中所有的颜色
 */
QList<ColorOctree::ColorCount> ImageUtil::extractImageThemeColors(QImage image, int count)
{
    auto octree = ColorOctree(image, IMAGE_CALC_PIXEL_MAX_SIZE, count);
    auto result = octree.result();

    if (!result.size() || result.first().count <= 0)
        return result;

    // 可以过滤太少的颜色，看情况开关
    int maxCount = result.first().count;
    int minCount = maxCount / 1000; // 小于最大项1‰的都去掉
    while (result.last().count < minCount || result.size() > count)
        result.removeLast();

    return result;
}

/**
 * 获取图片中的所有主题色
 * 使用最小差值法一一求差
 * 然后和调色盘中颜色对比
 * 取出每种最是相近的颜色
 */
QList<QColor> ImageUtil::extractImageThemeColorsInPalette(QImage image, QList<QColor> paletteColors, int needCount)
{
    auto octree = ColorOctree(image, IMAGE_CALC_PIXEL_MAX_SIZE, paletteColors.size());
    auto result = octree.result();

    QList<QColor> colors;
    for (int i = 0; i < result.size(); i++)
    {
        auto cc = result.at(i);
        QColor nestColor;
        int minVariance = 255*255*3+1;
        for (int j = 0; j < paletteColors.size(); j++)
        {
            auto qc = paletteColors.at(j);

            // 差值计算：直接计算方差吧
            int dr = cc.red - qc.red();
            int dg = cc.green - qc.green();
            int db = cc.blue - qc.blue();
            int variance = dr * dr + dg * dg + db * db;

            if (minVariance > variance)
            {
                minVariance = variance;
                nestColor = qc;
            }
        }
        // 找到方差最小的颜色
        colors.append(nestColor);
    }

    return colors;
}

/**
 * 获取反色
 */
QColor ImageUtil::getInvertColor(QColor color)
{
    auto getInvert = [=](int c) -> int{
        if (c < 96 || c > 160)
            return 255 - c;
        else if (c < 128)
            return 255;
        else
            return 0;
    };

    color.setRed(getInvert(color.red()));
    color.setGreen(getInvert(color.green()));
    color.setBlue(getInvert(color.blue()));
    return color;
}

/**
 * 获取一张图片中应有的背景色、前景色
 * 背景色是颜色占比最高的
 * 前景色是与其余颜色的(色差*数量)方差和最大的
 * @return 颜色提取是否成功
 */
bool ImageUtil::getBgFgColor(QList<ColorOctree::ColorCount> colors, QColor *bg, QColor *fg)
{
    Q_ASSERT(bg && fg);
    if (!colors.size())
    {
        *bg = Qt::white;
        *fg = Qt::black;
        return false;
    }

    if (colors.size() == 1)
    {
        *bg = colors.first().toColor();
        *fg = getInvertColor(*bg);
        return false;
    }

    int maxIndex = -1;
    qint64 maxVariance = 0;
    int size = colors.size();
    for (int i = 0; i < size; i++)
    {
        ColorOctree::ColorCount c = colors.at(i);
        int r = c.red, g = c.green, b = c.blue, n = c.count;

        qint64 sumVariance = 0;
        for (int j = 0; j < size; j++)
        {
            if (j == i)
                continue;
            ColorOctree::ColorCount c2 = colors.at(j);
            qint64 variant = 3 * (r - c2.red) * (r - c2.red)
                    + 4 * (g - c2.green) * (g - c2.green)
                    + 2 * (b - c2.blue) * (b - c2.blue);
            variant *= c2.count;
            sumVariance += variant;
        }

        if (sumVariance > maxVariance)
        {
            maxVariance = sumVariance;
            maxIndex = i;
        }
    }

    *bg = colors.first().toColor();
    *fg = colors.at(maxIndex).toColor();
    return true;
}

/**
 * 同上，外加一个辅助色
 */
bool ImageUtil::getBgFgSgColor(QList<ColorOctree::ColorCount> colors, QColor *bg, QColor *fg, QColor *sg)
{
    // 调用上面先获取背景色、前景色
    if (!getBgFgColor(colors, bg, fg))
        return false;

    // 再获取合适的辅助色
    if (colors.size() == 2)
    {
        *sg = getInvertColor(*fg); // 文字取反
        return false;
    }

    int maxIndex = -1;
    qint64 maxVariance = 0;
    for (int i = 0; i < colors.size(); i++)
    {
        ColorOctree::ColorCount c = colors.at(i);
        if (c.toColor() == *bg || c.toColor() == *fg)
            continue;

        int r = c.red, g = c.green, b = c.blue, n = c.count;
        qint64 variantBg = 3 * (r - bg->red()) * (r - bg->red())
                + 4 * (g - bg->green()) * (g - bg->green())
                + 2 * (b - bg->blue()) * (b - bg->blue());
        qint64 variantFg = 3 * (r - fg->red()) * (r - fg->red())
                + 4 * (g - fg->green()) * (g - fg->green())
                + 2 * (b - fg->blue()) * (b - fg->blue());
        qint64 sum = variantBg + variantFg * 2; // 文字占比比较大
        if (sum > maxVariance)
        {
            maxVariance = sum;
            maxIndex = i;
        }
    }

    *sg = colors.at(maxIndex).toColor();
    return true;
}

/**
 * 同上
 * 获取四种颜色：背景色、前景色（文字）、辅助背景色（与文字色差大）、辅助前景色（与背景色差大）
 */
bool ImageUtil::getBgFgSgColor(QList<ColorOctree::ColorCount> colors, QColor *bg, QColor *fg, QColor *sbg, QColor *sfg)
{
    // 调用上面先获取背景色、前景色
    if (!getBgFgColor(colors, bg, fg))
        return false;

    // 再获取合适的辅助色
    if (colors.size() == 2)
    {
        *sbg = getInvertColor(*fg); // 文字取反
        *sfg = getInvertColor(*bg); // 背景取反
        return false;
    }

    // 计算辅助背景色
    int maxIndex = -1;
    qint64 maxVariance = 0;
    for (int i = 0; i < colors.size(); i++)
    {
        ColorOctree::ColorCount c = colors.at(i);
        if (c.toColor() == *bg || c.toColor() == *fg)
            continue;

        int r = c.red, g = c.green, b = c.blue, n = c.count;
        qint64 variantBg = 3 * (r - bg->red()) * (r - bg->red())
                + 4 * (g - bg->green()) * (g - bg->green())
                + 2 * (b - bg->blue()) * (b - bg->blue());
        qint64 variantFg = 3 * (r - fg->red()) * (r - fg->red())
                + 4 * (g - fg->green()) * (g - fg->green())
                + 2 * (b - fg->blue()) * (b - fg->blue());

        qint64 variant = variantBg + variantFg * 2; // 文字占比比较大
        if (variant > maxVariance)
        {
            maxVariance = variant;
            maxIndex = i;
        }
    }
    *sbg = colors.at(maxIndex).toColor();

    // 根据辅助背景色计算辅助前景色
    maxIndex = -1;
    maxVariance = 0;
    for (int i = 0; i < colors.size(); i++)
    {
        ColorOctree::ColorCount c = colors.at(i);
        if (c.toColor() == *sbg)
            continue;

        int r = c.red, g = c.green, b = c.blue, n = c.count;
        qint64 variant = 3 * (r - sbg->red()) * (r - sbg->red())
                + 4 * (g - sbg->green()) * (g - sbg->green())
                + 2 * (b - sbg->blue()) * (b - sbg->blue());

        if (variant > maxVariance)
        {
            maxVariance = variant;
            maxIndex = i;
        }
    }
    *sfg = colors.at(maxIndex).toColor();
    return true;
}

/// 获取色差最大的一项
/// 由于RGB颜色空间不是均匀颜色空间,按照空间距离得到的色差并不完全符合人的视觉,
/// 在实际应用时经常采取给各颜色分量加上一定权值的办法，一般加权取值(3,4,2)
QColor ImageUtil::getFastestColor(QColor bg, QList<QColor> palette)
{
    qint64 maxi = -1;
    QColor maxiColor;
    int rr = bg.red(), gg = bg.green(), bb = bg.blue();
    foreach (auto c, palette)
    {
        int r = c.red(), g = c.green(), b = c.blue();
        qint64 delta = 3 * (r - rr) * (r - rr)
                + 4 * (g - gg) * (g - gg)
                + 2 * (b - bb) * (b - bb);
        if (delta > maxi)
        {
            maxi = delta;
            maxiColor = c;
        }
    }
    return maxiColor;
}

/// 获取色差最大的一项
/// 但是也会参考数量，数量越多权重越高
QColor ImageUtil::getFastestColor(QColor bg, QList<ColorOctree::ColorCount> palette, int enableCount)
{
    qint64 maxi = -1;
    QColor maxiColor = QColor::Invalid;
    int rr = bg.red(), gg = bg.green(), bb = bg.blue();
    foreach (auto c, palette)
    {
        int r = c.red, g = c.green, b = c.blue;
        qint64 delta = 3 * (r - rr) * (r - rr)
                + 4 * (g - gg) * (g - gg)
                + 2 * (b - bb) * (b - bb);
        if (enableCount == 1)
            delta *= c.count;
        else if (enableCount == 2)
            delta *= qint64(sqrt(c.count + 1));
        if (delta > maxi)
        {
            maxi = delta;
            maxiColor = c.toColor();
        }
    }
    return maxiColor;
}

/// 返回随机深色调
QColor ImageUtil::randomColor()
{
    return QColor::fromHsl(rand()%360,rand()%256,rand()%200);
}
