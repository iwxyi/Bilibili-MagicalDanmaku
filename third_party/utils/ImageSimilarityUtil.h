#ifndef IMAGESIMILARITYUTIL_H
#define IMAGESIMILARITYUTIL_H

#include <QImage>
#include <QPixmap>
#include <QDebug>

typedef QVector<unsigned short> HashSeq;

class ImageSimilarityUtil
{
public:
    /**
     * 根据像素严格地比较
     */
    static double compareImageByPixel(const QImage& image1, const QImage& image2)
    {
        Q_ASSERT(image1.size() == image2.size());
        int sum = 0;
        for (int i = 0; i < image1.width(); i++)
        {
            for (int j = 0; j < image1.height(); j++)
            {
                if (image1.pixel(i, j) == image2.pixel(i, j))
                    sum++;
            }
        }
        return (double)sum / (image1.width() * image1.height());
    }

    /**
     * 根据像素严格地比较
     */
    static double compareImageByPixelThreshold(const QImage& image1, const QImage& image2, int threshold)
    {
        Q_ASSERT(image1.size() == image2.size());
        int sum = 0;
        for (int i = 0; i < image1.width(); i++)
        {
            for (int j = 0; j < image1.height(); j++)
            {
                QColor c1 = image1.pixelColor(i, j);
                QColor c2 = image2.pixelColor(i, j);
                if (qAbs(c1.red() - c2.red()) < threshold
                        && qAbs(c1.green() - c2.green()) < threshold
                        && qAbs(c1.blue() - c2.blue()) < threshold)
                    sum++;
            }
        }
        return (double)sum / (image1.width() * image1.height());
    }

    /**
     * 进行一定的缩放
     */
    static double compareImageByPixel(QImage image1, QImage image2, int side, int threshold)
    {
        Q_ASSERT(image1.size() == image2.size());
        if (image1.width() > side && image1.height() > side)
        {
            if (threshold == 0)
                return compareImageByPixel(image1.scaled(side, side, Qt::KeepAspectRatioByExpanding), image2.scaled(side, side, Qt::KeepAspectRatioByExpanding));
            else
                return compareImageByPixelThreshold(image1.scaled(side, side, Qt::KeepAspectRatioByExpanding), image2.scaled(side, side, Qt::KeepAspectRatioByExpanding), threshold);
        }
        else
        {
            if (threshold == 0)
                return compareImageByPixel(image1, image2);
            else
                return compareImageByPixelThreshold(image1, image2, threshold);
        }
    }

    /**
     * 平均哈希
     * 返回两张图哈希的汉明距离，一般认为 <5 的为相同，>10 的为不同
     */
    static int aHash(QImage image1, QImage image2)
    {
        Q_ASSERT(image1.size() == image2.size());

        auto toHashSeq = [](const QImage& image, int ave) {
            HashSeq hash;
            for (int i = 0; i < image.width(); i++)
            {
                for (int j = 0; j < image.height(); j++)
                    hash.append(image.pixelColor(i, j).red() > ave ? 1 : 0);
            }
            return hash;
        };

        auto toAHash = [toHashSeq](const QImage& image) {
            QImage img = image.scaled(8, 8, Qt::KeepAspectRatio);
            img = convertToGray64(img);
            int ave1 = getGrayAverage(img);
            return toHashSeq(img, ave1);
        };

        return hanmingDistance(toAHash(image1), toAHash(image2));
    }

    /**
     * 差值哈希
     */
    static double dHash(QImage image1, QImage image2)
    {
        Q_ASSERT(image1.size() == image2.size());

    }

    /**
     * 感知哈希
     */
    static double pHash(QImage image1, QImage image2)
    {
        Q_ASSERT(image1.size() == image2.size());

    }

    /**
     * 获取灰度图的平均值
     */
    static int getGrayAverage(QImage grayImage)
    {
        int average = 0;
        for (int i = 0; i < grayImage.width(); i++)
        {
            for (int j = 0; j < grayImage.height(); j++)
            {
                average += grayImage.pixelColor(i, j).red();
            }
        }
        return average / (grayImage.width() * grayImage.height());
    }

    /**
     * 转换为灰度图
     */
    static QImage convertToGray(QImage image)
    {
        for (int i = 0; i < image.width(); i++)
        {
            for (int j = 0; j < image.height(); j++)
            {
                QColor c = image.pixelColor(i, j);
                int ave = (c.red() + c.green() + c.blue()) / 3;
                image.setPixelColor(i, j, QColor(ave, ave, ave));
            }
        }
        return image;
    }

    /**
     * 转换为64级灰度图
     */
    static QImage convertToGray64(QImage image)
    {
        for (int i = 0; i < image.width(); i++)
        {
            for (int j = 0; j < image.height(); j++)
            {
                QColor c = image.pixelColor(i, j);
                int ave = (c.red() + c.green() + c.blue()) / 3 / 4;
                image.setPixelColor(i, j, QColor(ave, ave, ave));
            }
        }
        return image;
    }

    /**
     * 调整图像亮度
     */
    static QImage adjustBright(QImage image, int bright)
    {
        for (int i = 0; i < image.width(); i++)
        {
            for (int j = 0; j < image.height(); j++)
            {
                QColor c = image.pixelColor(i, j);
                int r = qBound(0, c.red() + bright, 255);
                int g = qBound(0, c.green() + bright, 255);
                int b = qBound(0, c.blue() + bright, 255);
                image.setPixelColor(i, j, QColor(r, g, b));
            }
        }
        return image;
    }

    /**
     * 计算两个hash的汉明距离
     */
    static int hanmingDistance(HashSeq hash1, HashSeq hash2)
    {
        Q_ASSERT(hash1.size() == hash2.size());
        qDebug() << hash1;
        qDebug() << hash2;
        int distance = 0;
        for (int i = 0; i < hash1.size(); i++)
        {
            distance += (hash1[i] == hash2[i] ? 1 : 0);
        }
        return distance;
    }

private:

};

#endif // IMAGESIMILARITYUTIL_H
