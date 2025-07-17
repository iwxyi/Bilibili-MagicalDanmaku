#ifndef NANOPBUTIL_H
#define NANOPBUTIL_H

#include <QJsonObject>
#include <QJsonArray>
#include "nanopb/pb.h"

class NanopbUtil
{
    void messageToJson(const pb_msgdesc_t* fields, const void* msg, QJsonObject& json) {
        const pb_field_t* field;
        for (field = fields->fields; field->tag != 0; field++) {
            const char* name = field->name;
            const void* ptr = (const char*)msg + field->data_offset;

            switch (field->type) {
            case PB_FTYPE_BOOL:
                json[name] = *(bool*)ptr;
                break;

            case PB_FTYPE_INT32:
            case PB_FTYPE_UINT32:
                json[name] = *(int*)ptr;
                break;

            case PB_FTYPE_INT64:
            case PB_FTYPE_UINT64:
                json[name] = static_cast<qint64>(*(int64_t*)ptr);
                break;

            case PB_FTYPE_FLOAT:
                json[name] = *(float*)ptr;
                break;

            case PB_FTYPE_DOUBLE:
                json[name] = *(double*)ptr;
                break;

            case PB_FTYPE_STRING:
                json[name] = QString::fromUtf8((char*)ptr);
                break;

            case PB_FTYPE_SUBMESSAGE: {
                QJsonObject subObj;
                messageToJson(field->submsg, ptr, subObj);
                json[name] = subObj;
                break;
            }

            case PB_FTYPE_REPEATED: {
                QJsonArray arr;
                const pb_array_t* array = (pb_array_t*)ptr;
                for (size_t i = 0; i < array->count; i++) {
                    const void* elem = (const char*)array->data + i * field->data_size;
                    // 处理数组元素...
                }
                json[name] = arr;
                break;
            }
            }
        }
    }

    // 使用方式
    QJsonObject convertToJson(const InteractWordV2& msg) {
        QJsonObject result;
        messageToJson(InteractWordV2_fields, &msg, result);
        return result;
    }
};

#endif // NANOPBUTIL_H
