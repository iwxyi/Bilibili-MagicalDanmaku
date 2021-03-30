#ifndef COLOROCTREE_H
#define COLOROCTREE_H

#include <QColor>
#include <QList>
#include <QImage>
#include <QDebug>

typedef QList<QColor> ColorList;

class ColorOctree
{
public:
    ColorOctree();
    ColorOctree(QImage image, int maxPool = 250000, int maxCount = 20);
    ~ColorOctree();

    struct RGB
    {
        int red, green, blue;
    };

    struct OctreeNode
    {
        ~OctreeNode()
        {
            for (int i = 0; i < 8; i++)
                if (children[i])
                    delete children[i];
        }

        long long red = 0, green = 0, blue = 0; // 许多点累加的结果，会非常大
        bool isLeaf = false;
        int pixelCount = 0;
        OctreeNode *children[8] = {};
    };

    struct ColorCount
    {
        int count = 0;
        char color[7] = {}; // 16进制字符串
        int red = 0, green = 0, blue = 0;
        int colorValue = 0; // 对应int值

        QColor toColor() const
        {
            return QColor(red, green, blue);
        }
    };

    void buildTree(QImage image, int maxCount = 20);
    QList<ColorCount> result();

private:
    void addColor(OctreeNode *node, RGB* color, int level);
    bool reduceTree();
    void colorStats(OctreeNode *node, QList<ColorCount> *colors);

private:
    OctreeNode *root = nullptr;
    QList<OctreeNode *> reducible[7];
    int leafCount = 0; // 叶子数量
    int maxCount = 20; // 最终得到结果的最大值
};

#endif // COLOROCTREE_H
