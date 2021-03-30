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
    while (result.last().count < minCount)
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
            qint64 variant = (r - c2.red) * (r - c2.red)
                    + (g - c2.green) * (g - c2.green)
                    + (b - c2.blue) * (b - c2.blue);
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
        qint64 variantBg = (r - bg->red()) * (r - bg->red())
                + (g - bg->green()) * (g - bg->green())
                + (b - bg->blue()) * (b - bg->blue());
        qint64 variantFg = (r - fg->red()) * (r - fg->red())
                + (g - fg->green()) * (g - fg->green())
                + (b - fg->blue()) * (b - fg->blue());
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

    int maxBIndex = -1, maxFIndex = -1;
    qint64 maxBVariance = 0, maxFVariance = 0;
    for (int i = 0; i < colors.size(); i++)
    {
        ColorOctree::ColorCount c = colors.at(i);
        if (c.toColor() == *bg || c.toColor() == *fg)
            continue;

        int r = c.red, g = c.green, b = c.blue, n = c.count;
        qint64 variantBg = (r - bg->red()) * (r - bg->red())
                + (g - bg->green()) * (g - bg->green())
                + (b - bg->blue()) * (b - bg->blue());
        qint64 variantFg = (r - fg->red()) * (r - fg->red())
                + (g - fg->green()) * (g - fg->green())
                + (b - fg->blue()) * (b - fg->blue());

        qint64 sum = variantBg + variantFg * 2; // 文字占比比较大
        if (sum > maxBVariance)
        {
            maxBVariance = sum;
            maxBIndex = i;
        }

        sum = variantBg * 2 + variantFg; // 背景占比比较大
        if (sum > maxFVariance)
        {
            maxFVariance = sum;
            maxFIndex = i;
        }
    }

    *sbg = colors.at(maxBIndex).toColor();
    *sfg = colors.at(maxFIndex).toColor();
    return true;
}
