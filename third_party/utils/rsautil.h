#ifndef RSAUTIL_H
#define RSAUTIL_H

#include <QByteArray>
#include <QCryptographicHash>

class RSAUtil
{
public:
    // 主要的加密函数
    static QByteArray rsaEncrypt(const QByteArray& data, const QByteArray& publicKeyPEM);

private:
    // OAEP 相关函数
    static QByteArray mgf1(const QByteArray& seed, int length);
    static QByteArray xorBytes(const QByteArray& a, const QByteArray& b);

    // 大整数运算函数
    static QByteArray modPow(const QByteArray& base, const QByteArray& exponent, const QByteArray& modulus);
    static QByteArray modMul(const QByteArray& a, const QByteArray& b, const QByteArray& modulus);
    static QByteArray mod(const QByteArray& a, const QByteArray& modulus);
    static QByteArray rightShift(const QByteArray& a);
    static int compare(const QByteArray& a, const QByteArray& b);
    static QByteArray subtract(const QByteArray& a, const QByteArray& b);

    // 辅助函数
    static QByteArray parsePublicKey(const QByteArray& publicKeyPEM, QByteArray& n, QByteArray& e);
    static QByteArray oaepPad(const QByteArray& data, int keyLength);
};

#endif // RSAUTIL_H
