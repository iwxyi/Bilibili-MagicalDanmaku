#ifndef STRINGDISTANCEUTIL_H
#define STRINGDISTANCEUTIL_H

#include <QCoreApplication>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 按需更换相关的类型与一些接口，QString/wstring/char[] 都可以用
typedef QString SDStringType;

/**
 * 计算两个字符串的编辑距离与相似程度
 * 采用动态规划的思想
 * 由于用到了递归，所以不宜比较太长的字符串
 * 参考源码：https://blog.csdn.net/jfkidear/article/details/52928471
 */
class StringDistanceUtil
{
public:
    /**
     * 获取两个字符串之间的距离
     * 即变成另一个字符串的最少修改次数
     */
    static int getDistance(const SDStringType &a, const SDStringType &b)
    {
        int	  len_a = a.length();
        int	  len_b = b.length();
        int **diss	= (int **)malloc(sizeof(int *) * (len_a + 1));
        for (int i = 0; i < len_a + 1; i++)
        {
            diss[i] = (int *)malloc(sizeof(int) * (len_b + 1));
            memset(diss[i], 0, sizeof(int) * (len_b + 1));
        }
        diss[0][1] = 0;
        // memset(temp, 0, sizeof(temp)); // 仅char*可用
        int distance = StringDistanceUtil().compute_distance(
            a, 0, len_a - 1, b, 0, len_b - 1, diss);
        return distance;
    }

    /**
     * 获取两个字符串之间的相似度百分比
     * 距离/字符串长度，向下取整
     */
    static double getSimilarity(const SDStringType &a, const SDStringType &b, int scale = 100)
    {
        int len_a  = a.length();
        int len_b  = b.length();
        int maxLen = len_a > len_b ? len_a : len_b;
        if (maxLen == 0)
            return 0;
        int dis = getDistance(a, b);
        return ((double)(maxLen - dis) / maxLen) * scale;
    }

private:
    int min(int a, int b, int c)
    {
        if (a < b)
        {
            if (a < c)
                return a;
            else
                return c;
        }
        else
        {
            if (b < c)
                return b;
            else
                return c;
        }
    }

    int compute_distance(const SDStringType &strA, int pABegin, int pAEnd,
                         const SDStringType &strB, int pBBegin, int pBEnd,
                         int **diss)
    {
        int a, b, c;
        if (pABegin > pAEnd)
        {
            if (pBBegin > pBEnd)
            {
                return 0;
            }
            else
            {
                return pBEnd - pBBegin + 1;
            }
        }

        if (pBBegin > pBEnd)
        {
            if (pABegin > pAEnd)
            {
                return 0;
            }
            else
            {
                return pAEnd - pABegin + 1;
            }
        }

        if (strA[pABegin] == strB[pBBegin])
        {
            if (diss[pABegin + 1][pBBegin + 1] != 0)
            {
                a = diss[pABegin + 1][pBBegin + 1];
            }
            else
            {
                a = compute_distance(strA, pABegin + 1, pAEnd, strB,
                                     pBBegin + 1, pBEnd, diss);
            }
            return a;
        }
        else
        {
            if (diss[pABegin + 1][pBBegin + 1] != 0)
            {
                a = diss[pABegin + 1][pBBegin + 1];
            }
            else
            {
                a = compute_distance(strA, pABegin + 1, pAEnd, strB,
                                     pBBegin + 1, pBEnd, diss);
                diss[pABegin + 1][pBBegin + 1] = a;
            }

            if (diss[pABegin + 1][pBBegin] != 0)
            {
                b = diss[pABegin + 1][pBBegin];
            }
            else
            {
                b = compute_distance(strA, pABegin + 1, pAEnd, strB, pBBegin,
                                     pBEnd, diss);
                diss[pABegin + 1][pBBegin] = b;
            }

            if (diss[pABegin][pBBegin + 1] != 0)
            {
                c = diss[pABegin][pBBegin + 1];
            }
            else
            {
                c = compute_distance(strA, pABegin, pAEnd, strB, pBBegin + 1,
                                     pBEnd, diss);
                diss[pABegin][pBBegin + 1] = c;
            }

            return min(a, b, c) + 1;
        }
    }
};

#endif // STRINGDISTANCEUTIL_H
