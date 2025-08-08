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


/// 计算明度
/// <40为暗色，>120为亮色
double ImageUtil::calculateLuminance(QColor c)
{
    return (0.2126 * c.redF() + 0.7152 * c.greenF() + 0.0722 * c.blueF()) * 255;
}

/// 从一组色彩中获取最相似的颜色
QColor ImageUtil::getCloestColorByRGB(QColor color, const QList<QColor> &colors)
{
    double min_diff = std::numeric_limits<double>::max();
    QColor nearest_color = color;
    foreach (auto c, colors)
    {
        int diff = perceptualRgbDistance(color, c);
        if (diff < min_diff)
        {
            min_diff = diff;
            nearest_color = c;
        }
    }
    return nearest_color;
}

/* 查找视觉上最接近的颜色，CIELAB 颜色空间
颜色空间转换：
- RGB → XYZ → CIELAB（使用 D65 白点，标准照明条件）
- 包含 sRGB 非线性校正
色差计算：
- 使用 CIEDE2000 色差公式（ΔE00）
- 考虑了明度、色度和色调的感知差异
- 包含了补偿因子（如色调旋转、彩度权重）
优势：
- 更符合人眼对颜色的感知
- 对高饱和度和中等明度的颜色特别准确
- 正确处理了 CIELAB 空间中的非线性问题
*/
QColor ImageUtil::getVisuallyClosestColorByCIELAB(const QColor& target, const QList<QColor>& colors, const QList<QVector3D>& colorLabs) {
    QVector3D targetLab = rgbToLab(target);
    QColor closestColor;
    double minDeltaE = std::numeric_limits<double>::max();

    for (int i = 0; i < colors.size(); i++) {
        QVector3D colorLab = colorLabs[i];
        double deltaE = deltaE94(targetLab, colorLab);

        if (deltaE < minDeltaE) {
            minDeltaE = deltaE;
            closestColor = colors[i];
        }
    }

    return closestColor;
}

// RGB 转 XYZ (D65 白点)
QVector3D ImageUtil::rgbToXyz(const QColor& color) {
    float r = color.redF();
    float g = color.greenF();
    float b = color.blueF();

    // sRGB 到线性 RGB 的转换
    auto linearize = [](float c) {
        return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
    };

    r = linearize(r);
    g = linearize(g);
    b = linearize(b);

    // 线性 RGB 到 XYZ 的转换 (D65 白点)
    float X = r * 0.4124564 + g * 0.3575761 + b * 0.1804375;
    float Y = r * 0.2126729 + g * 0.7151522 + b * 0.0721750;
    float Z = r * 0.0193339 + g * 0.1191920 + b * 0.9503041;

    return {X, Y, Z};
}

// XYZ 转 CIELAB (D65 白点)
QVector3D ImageUtil::xyzToLab(const QVector3D& xyz) {
    float X = xyz.x() / 0.95047; // D65 白点 X
    float Y = xyz.y();
    float Z = xyz.z() / 1.08883; // D65 白点 Z

    auto f = [](float t) {
        return t > 0.008856 ? std::cbrt(t) : (7.787 * t + 16.0 / 116.0);
    };

    float L = 116.0 * f(Y) - 16.0;
    float a = 500.0 * (f(X) - f(Y));
    float b = 200.0 * (f(Y) - f(Z));

    return {L, a, b};
}

// RGB 转 CIELAB
QVector3D ImageUtil::rgbToLab(const QColor& color) {
    return xyzToLab(rgbToXyz(color));
}

// 计算 CIELAB 颜色空间中的 Delta E 2000 距离
double ImageUtil::deltaE2000(const QVector3D& lab1, const QVector3D& lab2) {
    double L1 = lab1.x();
    double a1 = lab1.y();
    double b1 = lab1.z();
    double L2 = lab2.x();
    double a2 = lab2.y();
    double b2 = lab2.z();

    // 计算 C1, C2, h1, h2
    double C1 = std::sqrt(a1 * a1 + b1 * b1);
    double C2 = std::sqrt(a2 * a2 + b2 * b2);
    double C_bar = (C1 + C2) / 2.0;

    // 计算 G
    double G = 0.5 * (1 - std::sqrt(std::pow(C_bar, 7) / (std::pow(C_bar, 7) + std::pow(25.0, 7))));

    // 计算 a'
    double a1_prime = a1 * (1 + G);
    double a2_prime = a2 * (1 + G);

    // 计算 C', h'
    double C1_prime = std::sqrt(a1_prime * a1_prime + b1 * b1);
    double C2_prime = std::sqrt(a2_prime * a2_prime + b2 * b2);

    double h1_prime = std::atan2(b1, a1_prime);
    double h2_prime = std::atan2(b2, a2_prime);

    // 确保 h 在 0 到 2π 之间
    if (h1_prime < 0) h1_prime += 2 * M_PI;
    if (h2_prime < 0) h2_prime += 2 * M_PI;

    // 计算 ΔL', ΔC', Δh'
    double delta_L_prime = L2 - L1;
    double delta_C_prime = C2_prime - C1_prime;

    double delta_h_prime;
    if (C1_prime * C2_prime == 0) {
        delta_h_prime = 0;
    } else {
        double dh = h2_prime - h1_prime;
        if (std::abs(dh) <= M_PI) {
            delta_h_prime = dh;
        } else if (dh > M_PI) {
            delta_h_prime = dh - 2 * M_PI;
        } else {
            delta_h_prime = dh + 2 * M_PI;
        }
    }

    double delta_H_prime = 2 * std::sqrt(C1_prime * C2_prime) * std::sin(delta_h_prime / 2);

    // 计算 C' 平均值和 h' 平均值
    double C_prime_bar = (C1_prime + C2_prime) / 2.0;

    double h_prime_bar;
    if (C1_prime * C2_prime == 0) {
        h_prime_bar = h1_prime + h2_prime;
    } else {
        double dh = h2_prime - h1_prime;
        if (std::abs(dh) <= M_PI) {
            h_prime_bar = (h1_prime + h2_prime) / 2.0;
        } else {
            if (h1_prime + h2_prime < 2 * M_PI) {
                h_prime_bar = (h1_prime + h2_prime + 2 * M_PI) / 2.0;
            } else {
                h_prime_bar = (h1_prime + h2_prime - 2 * M_PI) / 2.0;
            }
        }
    }

    // 计算 T
    double T = 1 - 0.17 * std::cos(h_prime_bar - M_PI / 6) +
               0.24 * std::cos(2 * h_prime_bar) +
               0.32 * std::cos(3 * h_prime_bar + M_PI / 30) -
               0.20 * std::cos(4 * h_prime_bar - 63 * M_PI / 180);

    // 计算 Δθ
    double delta_theta = 30 * M_PI / 180 * std::exp(-std::pow((h_prime_bar * 180 / M_PI - 275) / 25, 2));

    // 计算 RC, SL, SC, SH
    double R_C = 2 * std::sqrt(std::pow(C_prime_bar, 7) / (std::pow(C_prime_bar, 7) + std::pow(25.0, 7)));
    double S_L = 1 + (0.015 * std::pow(L2 + L1 - 100, 2)) / std::sqrt(20 + std::pow(L2 + L1 - 100, 2));
    double S_C = 1 + 0.045 * C_prime_bar;
    double S_H = 1 + 0.015 * C_prime_bar * T;

    // 计算 RT
    double R_T = -std::sin(2 * delta_theta) * R_C;

    // 计算最终的 ΔE00
    double delta_E = std::sqrt(
        std::pow(delta_L_prime / S_L, 2) +
        std::pow(delta_C_prime / S_C, 2) +
        std::pow(delta_H_prime / S_H, 2) +
        R_T * (delta_C_prime / S_C) * (delta_H_prime / S_H)
        );

    return delta_E;
}

/// CIELAB 空间下的 CIE94 色差公式（比 CIEDE2000 快 3-5 倍）
double ImageUtil::deltaE94(const QVector3D& lab1, const QVector3D& lab2) {
    double L1 = lab1.x(), a1 = lab1.y(), b1 = lab1.z();
    double L2 = lab2.x(), a2 = lab2.y(), b2 = lab2.z();

    double deltaL = L1 - L2;
    double C1 = std::sqrt(a1*a1 + b1*b1);
    double C2 = std::sqrt(a2*a2 + b2*b2);
    double deltaC = C1 - C2;
    double deltaA = a1 - a2;
    double deltaB = b1 - b2;
    double deltaH_squared = deltaA*deltaA + deltaB*deltaB - deltaC*deltaC;

    // 纺织品应用的参数（可根据需求调整）
    double kL = 1, kC = 1, kH = 1;
    double SL = 1, SC = 1 + 0.045 * C1;
    double SH = 1 + 0.015 * C1;

    return std::sqrt(
        std::pow(deltaL / (kL * SL), 2) +
        std::pow(deltaC / (kC * SC), 2) +
        std::pow(deltaH_squared > 0 ? std::sqrt(deltaH_squared) / (kH * SH) : 0, 2)
        );
}

/// 感知加权的 RGB 距离（比 CIELAB 快 10 倍以上）
double ImageUtil::perceptualRgbDistance(const QColor& c1, const QColor& c2) {
    int r1 = c1.red(), g1 = c1.green(), b1 = c1.blue();
    int r2 = c2.red(), g2 = c2.green(), b2 = c2.blue();

    // 中点位置
    int r_mean = (r1 + r2) / 2;

    // 加权欧几里得距离（更接近人眼感知）
    int dr = r1 - r2;
    int dg = g1 - g2;
    int db = b1 - b2;

    return std::sqrt(
        (2 + r_mean/256.0) * dr*dr +
        4 * dg*dg +
        (2 + (255 - r_mean)/256.0) * db*db
        );
}
