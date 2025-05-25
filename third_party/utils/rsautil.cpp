#include "rsautil.h"
#include <QRandomGenerator>
#include <QString>
#include <QVector>
#include <QDebug>

QByteArray RSAUtil::rsaEncrypt(const QByteArray& data, const QByteArray& publicKeyPEM)
{
    qDebug() << "Input data:" << data.toHex();
    qDebug() << "Public key:" << publicKeyPEM;

    // 1. 解析公钥
    QByteArray n, e;
    if (parsePublicKey(publicKeyPEM, n, e).isEmpty()) {
        qDebug() << "Failed to parse public key";
        return QByteArray();
    }
    qDebug() << "Parsed n:" << n.toHex();
    qDebug() << "Parsed e:" << e.toHex();

    // 2. OAEP 填充
    QByteArray paddedData = oaepPad(data, n.length());
    if (paddedData.isEmpty()) {
        qDebug() << "Failed to pad data";
        return QByteArray();
    }
    qDebug() << "Padded data:" << paddedData.toHex();

    // 3. RSA 加密
    QByteArray result = modPow(paddedData, e, n);
    qDebug() << "Encrypted result:" << result.toHex();
    return result;
}

QByteArray RSAUtil::parsePublicKey(const QByteArray& publicKeyPEM, QByteArray& n, QByteArray& e)
{
    // 1. 解析 PEM 格式的公钥
    QString pem = QString::fromUtf8(publicKeyPEM);
    pem = pem.replace("-----BEGIN PUBLIC KEY-----", "")
              .replace("-----END PUBLIC KEY-----", "")
              .replace("\n", "")
              .trimmed();
    
    qDebug() << "Cleaned PEM:" << pem;

    QByteArray der = QByteArray::fromBase64(pem.toUtf8());
    qDebug() << "DER length:" << der.length();
    qDebug() << "DER hex:" << der.toHex();

    // 2. 解析 DER 格式的公钥
    int pos = 0;

    // 跳过外层 SEQUENCE
    if (static_cast<unsigned char>(der[pos++]) != 0x30) {
        qDebug() << "Invalid outer SEQUENCE tag at pos" << pos-1;
        return QByteArray();
    }
    int seqLen = static_cast<unsigned char>(der[pos++]);
    qDebug() << "Outer SEQUENCE length:" << seqLen;
    if (seqLen > 0x80) {
        pos += (seqLen & 0x0F);
    }

    // 跳过内层 SEQUENCE
    if (static_cast<unsigned char>(der[pos++]) != 0x30) {
        qDebug() << "Invalid inner SEQUENCE tag at pos" << pos-1;
        return QByteArray();
    }
    seqLen = static_cast<unsigned char>(der[pos++]);
    qDebug() << "Inner SEQUENCE length:" << seqLen;
    if (seqLen > 0x80) {
        pos += (seqLen & 0x0F);
    }

    // 跳过 OID
    if (static_cast<unsigned char>(der[pos++]) != 0x06) {
        qDebug() << "Invalid OID tag at pos" << pos-1;
        return QByteArray();
    }
    seqLen = static_cast<unsigned char>(der[pos++]);
    qDebug() << "OID length:" << seqLen;
    pos += seqLen;

    // 跳过 NULL
    if (static_cast<unsigned char>(der[pos++]) != 0x05) {
        qDebug() << "Invalid NULL tag at pos" << pos-1;
        return QByteArray();
    }
    if (static_cast<unsigned char>(der[pos++]) != 0x00) {
        qDebug() << "Invalid NULL value at pos" << pos-1;
        return QByteArray();
    }

    // 获取 BITSTRING
    if (static_cast<unsigned char>(der[pos++]) != 0x03) {
        qDebug() << "Invalid BITSTRING tag at pos" << pos-1;
        return QByteArray();
    }
    seqLen = static_cast<unsigned char>(der[pos++]);
    qDebug() << "BITSTRING length:" << seqLen;
    if (seqLen > 0x80) {
        pos += (seqLen & 0x0F);
    }

    // 跳过 BITSTRING 的填充位
    pos++;

    // 获取 n 和 e
    if (static_cast<unsigned char>(der[pos++]) != 0x30) {
        qDebug() << "Invalid n/e SEQUENCE tag at pos" << pos-1;
        return QByteArray();
    }
    seqLen = static_cast<unsigned char>(der[pos++]);
    qDebug() << "n/e SEQUENCE length:" << seqLen;
    if (seqLen > 0x80) {
        pos += (seqLen & 0x0F);
    }

    // 获取 n
    if (static_cast<unsigned char>(der[pos++]) != 0x02) {
        qDebug() << "Invalid n INTEGER tag at pos" << pos-1;
        return QByteArray();
    }
    int nLen = static_cast<unsigned char>(der[pos++]);
    qDebug() << "n length:" << nLen;
    if (nLen > 0x80) {
        nLen = (nLen & 0x0F);
        pos += nLen;
    }
    n = der.mid(pos, nLen);
    pos += nLen;

    // 获取 e
    if (static_cast<unsigned char>(der[pos++]) != 0x02) {
        qDebug() << "Invalid e INTEGER tag at pos" << pos-1;
        return QByteArray();
    }
    int eLen = static_cast<unsigned char>(der[pos++]);
    qDebug() << "e length:" << eLen;
    if (eLen > 0x80) {
        eLen = (eLen & 0x0F);
        pos += eLen;
    }
    e = der.mid(pos, eLen);

    // 打印解析结果
    qDebug() << "Successfully parsed public key:";
    qDebug() << "n:" << n.toHex();
    qDebug() << "e:" << e.toHex();

    return QByteArray(1, 1); // 成功返回非空
}

QByteArray RSAUtil::oaepPad(const QByteArray& data, int keyLength)
{
    // 检查输入数据长度
    if (data.length() > keyLength - 2 * 32 - 2) {
        qDebug() << "Input data too long:" << data.length() << "bytes, max allowed:" << (keyLength - 2 * 32 - 2) << "bytes";
        return QByteArray();
    }

    // 使用 SHA-256 作为哈希函数
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    qDebug() << "Hash length:" << hash.length();

    // 生成随机种子
    QByteArray seed(32, 0);
    for (int i = 0; i < 32; i++) {
        seed[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));
    }
    qDebug() << "Seed length:" << seed.length();

    // 计算 db = lHash || PS || 0x01 || M
    QByteArray lHash = QCryptographicHash::hash(QByteArray(), QCryptographicHash::Sha256);
    QByteArray PS(keyLength - data.length() - 2 * 32 - 2, 0);
    QByteArray db = lHash + PS + QByteArray(1, 0x01) + data;
    qDebug() << "DB length:" << db.length();

    // 计算 dbMask = MGF(seed, nLen - 32 - 1)
    QByteArray dbMask = mgf1(seed, keyLength - 32 - 1);
    qDebug() << "DBMask length:" << dbMask.length();

    // 计算 maskedDB = db ⊕ dbMask
    QByteArray maskedDB = xorBytes(db, dbMask);
    qDebug() << "MaskedDB length:" << maskedDB.length();

    // 计算 seedMask = MGF(maskedDB, 32)
    QByteArray seedMask = mgf1(maskedDB, 32);
    qDebug() << "SeedMask length:" << seedMask.length();

    // 计算 maskedSeed = seed ⊕ seedMask
    QByteArray maskedSeed = xorBytes(seed, seedMask);
    qDebug() << "MaskedSeed length:" << maskedSeed.length();

    // 组合 EM = 0x00 || maskedSeed || maskedDB
    QByteArray result = QByteArray(1, 0x00) + maskedSeed + maskedDB;
    qDebug() << "Final padded data length:" << result.length();

    return result;
}

QByteArray RSAUtil::mgf1(const QByteArray& seed, int length)
{
    QByteArray result;
    int counter = 0;

    while (result.length() < length) {
        QByteArray counterBytes(4, 0);
        counterBytes[0] = static_cast<char>((counter >> 24) & 0xFF);
        counterBytes[1] = static_cast<char>((counter >> 16) & 0xFF);
        counterBytes[2] = static_cast<char>((counter >> 8) & 0xFF);
        counterBytes[3] = static_cast<char>(counter & 0xFF);

        QByteArray hash = QCryptographicHash::hash(seed + counterBytes, QCryptographicHash::Sha256);
        result.append(hash);
        counter++;
    }

    return result.left(length);
}

QByteArray RSAUtil::xorBytes(const QByteArray& a, const QByteArray& b)
{
    QByteArray result;
    int len = qMin(a.length(), b.length());

    for (int i = 0; i < len; i++) {
        result.append(static_cast<char>(static_cast<unsigned char>(a[i]) ^ static_cast<unsigned char>(b[i])));
    }

    return result;
}

QByteArray RSAUtil::modPow(const QByteArray& base, const QByteArray& exponent, const QByteArray& modulus)
{
    QByteArray result(1, 1);
    QByteArray b = base;
    QByteArray e = exponent;

    while (!e.isEmpty() && static_cast<unsigned char>(e[0]) != 0) {
        if (static_cast<unsigned char>(e[0]) & 1) {
            result = modMul(result, b, modulus);
        }
        b = modMul(b, b, modulus);
        e = rightShift(e);
    }

    return result;
}

QByteArray RSAUtil::modMul(const QByteArray& a, const QByteArray& b, const QByteArray& modulus)
{
    QByteArray result(a.length() + b.length(), 0);
    QVector<unsigned int> temp(result.length(), 0);  // 使用临时数组存储中间结果

    // 先进行乘法运算，结果存储在临时数组中
    for (int i = 0; i < a.length(); i++) {
        for (int j = 0; j < b.length(); j++) {
            temp[i + j] += static_cast<unsigned char>(a[i]) * static_cast<unsigned char>(b[j]);
        }
    }

    // 处理进位
    for (int i = 0; i < temp.length() - 1; i++) {
        temp[i + 1] += temp[i] >> 8;
        temp[i] &= 0xFF;
    }

    // 将结果转换回 QByteArray
    for (int i = 0; i < result.length(); i++) {
        result[i] = static_cast<char>(temp[i] & 0xFF);
    }

    return mod(result, modulus);
}

QByteArray RSAUtil::mod(const QByteArray& a, const QByteArray& modulus)
{
    QByteArray result = a;

    while (compare(result, modulus) >= 0) {
        result = subtract(result, modulus);
    }

    return result;
}

QByteArray RSAUtil::rightShift(const QByteArray& a)
{
    QByteArray result = a;

    for (int i = 0; i < result.length() - 1; i++) {
        result[i] = static_cast<char>((static_cast<unsigned char>(result[i]) >> 1) | 
                                    ((static_cast<unsigned char>(result[i + 1]) & 1) << 7));
    }
    result[result.length() - 1] = static_cast<char>(static_cast<unsigned char>(result[result.length() - 1]) >> 1);

    return result;
}

int RSAUtil::compare(const QByteArray& a, const QByteArray& b)
{
    if (a.length() != b.length()) {
        return a.length() - b.length();
    }

    for (int i = 0; i < a.length(); i++) {
        if (static_cast<unsigned char>(a[i]) != static_cast<unsigned char>(b[i])) {
            return static_cast<unsigned char>(a[i]) - static_cast<unsigned char>(b[i]);
        }
    }

    return 0;
}

QByteArray RSAUtil::subtract(const QByteArray& a, const QByteArray& b)
{
    QByteArray result = a;
    int borrow = 0;

    for (int i = 0; i < result.length(); i++) {
        int diff = static_cast<unsigned char>(result[i]) - 
                  (i < b.length() ? static_cast<unsigned char>(b[i]) : 0) - borrow;
        if (diff < 0) {
            diff += 256;
            borrow = 1;
        } else {
            borrow = 0;
        }
        result[i] = static_cast<char>(diff);
    }

    return result;
}
