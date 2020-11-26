#include "coloroctree.h"

ColorOctree::ColorOctree()
{
}

ColorOctree::ColorOctree(QImage image, int maxSize, int maxCount)
{
    // 缩减尺寸
    long long hCount = image.width();
    long long vCount = image.height();
    if (hCount > maxSize || vCount > maxSize) // 数量过大，按比例缩减
    {
        double prop = (double)maxSize / qMax(hCount, vCount);
        image = image.scaledToWidth(image.width() * prop); // 缩放到最大大小
    }

    // 开始建树
    buildTree(image, maxCount);
}

ColorOctree::~ColorOctree()
{
    if (root)
        delete root;
}

/**
 * 构建八叉树
 */
void ColorOctree::buildTree(QImage image, int maxCount)
{
    this->maxCount = maxCount;
    if (root)
        delete root;
    root = new OctreeNode();
    memset(root, 0, sizeof(OctreeNode));
    leafCount = 0;

    // 先转换为颜色，再添加到八叉树节点
    // 据说使用按行读取的方式能加快效率
    int w = image.width(), h = image.height();
    for (int y = 0; y < h; y++)
    {
        QRgb *line = (QRgb *)image.scanLine(y);
        for (int x = 0; x < w; x++)
        {
            int r = qRed(line[x]), g = qGreen(line[x]), b = qBlue(line[x]);
            RGB rgb{r, g, b};
            // 添加颜色到八叉树
            addColor(root, &rgb, 0);

            // 合并颜色
            // 有两个选择：addColor完就减（快），全部add再减（慢）
            // 但是这结果一模一样……
            while (leafCount > maxCount)
            {
                // 加返回值会影响性能，但是避免了结果颜色少时死循环的问题
                // 个人猜测，可能是因为0层全都是叶子节点
                // 例如7缩减到6时，八叉树第0层有7个叶子节点，无法缩减
                if (!reduceTree()) // 当 maxCount <= 7 时，很容易触发
                    break;
            }
        }
    }
}

/**
 * 返回结果
 */
QList<ColorOctree::ColorCount> ColorOctree::result()
{
    QList<ColorCount> counts;
    colorStats(root, &counts);
    std::sort(counts.begin(), counts.end(), [=](ColorCount a, ColorCount b) {
        if (a.count > b.count)
            return true;
        if (a.count < b.count)
            return false;
        return strcmp(a.color, b.color) < 0;
    });
    return counts;
}

void ColorOctree::addColor(ColorOctree::OctreeNode *node, RGB *color, int level)
{
    if (node->isLeaf) // 加到叶子节点
    {
        node->pixelCount++;
        node->red += color->red;
        node->green += color->green;
        node->blue += color->blue;
    }
    else // 加到下几层的叶子节点
    {
        /**
         * eg.
         *   R: 10101101
         *   G: 00101101
         *   B: 10010010
         *
         * idx: 50616616
         */
        unsigned char r = (color->red >> (7 - level)) & 1;
        unsigned char g = (color->green >> (7 - level)) & 1;
        unsigned char b = (color->blue >> (7 - level)) & 1;
        int idx = (r << 2) + (g << 1) + b;

        if (!node->children[idx])
        {
            // 创建下一层
            OctreeNode *tmp = node->children[idx] = new OctreeNode;
            memset(tmp, 0, sizeof(OctreeNode));
            if (level == 7) // 最后一层
            {
                tmp->isLeaf = true;
                leafCount++;
            }
            else // 不是最后一层
            {
                reducible[level].push_front(tmp); // 放入缩减的列表中
            }
        }

        addColor(node->children[idx], color, level + 1);
    }
}

bool ColorOctree::reduceTree()
{
    // 找到最深的叶子
    int lv = 6; // 从最后第2层（第7层）开始合并自己的叶子
    while (reducible[lv].empty() && lv >= 0)
        lv--;
    if (lv < 0)
        return false;

    // 移除该节点
    OctreeNode *node = reducible[lv].front();
    reducible[lv].pop_front();

    // 合并该节点的子节点
    long long r = 0, g = 0, b = 0;
    int count = 0;
    for (int i = 0; i < 8; i++)
    {
        if (!node->children[i])
            continue;
        r += node->children[i]->red;
        g += node->children[i]->green;
        b += node->children[i]->blue;
        count += node->children[i]->pixelCount;
        leafCount--;

        delete node->children[i];
        node->children[i] = nullptr;
    }

    node->isLeaf = true;
    node->red = r;
    node->green = g;
    node->blue = b;
    node->pixelCount = count;
    leafCount++;
    return true;
}

/**
 * 获取颜色结果
 */
void ColorOctree::colorStats(ColorOctree::OctreeNode *node, QList<ColorOctree::ColorCount> *colors)
{
    if (node->isLeaf)
    {
        int r = node->red / node->pixelCount;
        int g = node->green / node->pixelCount;
        int b = node->blue / node->pixelCount;

        ColorCount cnt;
        sprintf(cnt.color, "%.2X%.2X%.2X", r, g, b);
        cnt.count = node->pixelCount;
        cnt.colorValue = (r << 16) + (g << 8) + b;
        cnt.red = r;
        cnt.green = g;
        cnt.blue = b;

        colors->push_back(cnt);
        return;
    }

    for (int i = 0; i < 8; i++)
    {
        if (node->children[i])
        {
            colorStats(node->children[i], colors);
        }
    }
}

