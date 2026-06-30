//Copyright © 2025 - 2026, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

#include "NodeDataSerializer.hpp"
#include "SyncData.hpp"
#include "DebugLogging.hpp"
#include <QDebug>

// ============================================================================
// Main Serialization Entry Point
// ============================================================================

std::vector<uint8_t> NodeDataSerializer::serialize(std::shared_ptr<QtNodes::NodeData> data,
                                                   ImageEncoding imageEncoding)
{
    if (!data) {
        return std::vector<uint8_t>();
    }

    // Determine data type and call appropriate serializer
    if (auto cvImage = std::dynamic_pointer_cast<CVImageData>(data)) {
        return serializeCVImage(cvImage.get(), imageEncoding);
    }
    else if (auto cvPoint = std::dynamic_pointer_cast<CVPointData>(data)) {
        return serializeCVPoint(cvPoint.get());
    }
    else if (auto cvRect = std::dynamic_pointer_cast<CVRectData>(data)) {
        return serializeCVRect(cvRect.get());
    }
    else if (auto cvScalar = std::dynamic_pointer_cast<CVScalarData>(data)) {
        return serializeCVScalar(cvScalar.get());
    }
    else if (auto cvSize = std::dynamic_pointer_cast<CVSizeData>(data)) {
        return serializeCVSize(cvSize.get());
    }
    else if (auto boolData = std::dynamic_pointer_cast<BoolData>(data)) {
        return serializeBool(boolData.get());
    }
    else if (auto intData = std::dynamic_pointer_cast<IntegerData>(data)) {
        return serializeInteger(intData.get());
    }
    else if (auto dblData = std::dynamic_pointer_cast<DoubleData>(data)) {
        return serializeDouble(dblData.get());
    }
    else if (auto fltData = std::dynamic_pointer_cast<FloatData>(data)) {
        return serializeFloat(fltData.get());
    }
    else if (auto strData = std::dynamic_pointer_cast<StdStringData>(data)) {
        return serializeStdString(strData.get());
    }
    else if (auto syncData = std::dynamic_pointer_cast<SyncData>(data)) {
        return serializeSyncData(syncData.get());
    }
    else if (auto infoData = std::dynamic_pointer_cast<InformationData>(data)) {
        return serializeInformation(infoData.get());
    }
    else if (auto vecIntData = std::dynamic_pointer_cast<StdVectorIntData>(data)) {
        return serializeStdVector(vecIntData.get(), TYPE_STDVECTOR_INT);
    }
    else if (auto vecFltData = std::dynamic_pointer_cast<StdVectorFloatData>(data)) {
        return serializeStdVector(vecFltData.get(), TYPE_STDVECTOR_FLT);
    }
    else if (auto vecDblData = std::dynamic_pointer_cast<StdVectorDoubleData>(data)) {
        return serializeStdVector(vecDblData.get(), TYPE_STDVECTOR_DBL);
    }

    const NodeDataType nodeType = data->type();
    DEBUG_LOG_WARNING() << "[NodeDataSerializer] Unsupported data type for serialization"
                        << "id:" << nodeType.id
                        << "name:" << nodeType.name;
    return std::vector<uint8_t>();
}

// ============================================================================
// Main Deserialization Entry Point
// ============================================================================

std::shared_ptr<QtNodes::NodeData> NodeDataSerializer::deserialize(const std::vector<uint8_t>& payload)
{
    // Minimum payload: version(1) + type(1) + size(4) = 6 bytes
    if (payload.size() < 6) {
        DEBUG_LOG_WARNING() << "[NodeDataSerializer] Payload too small";
        return nullptr;
    }

    // Check protocol version
    uint8_t version = payload[0];
    if (version != PROTOCOL_VERSION) {
        DEBUG_LOG_WARNING() << "[NodeDataSerializer] Unsupported protocol version:" << version;
        return nullptr;
    }

    // Extract type and data size
    DataTypeID typeId = static_cast<DataTypeID>(payload[1]);
    uint32_t dataSize = readUInt32(&payload[2]);

    // Validate payload size
    if (payload.size() < 6 + dataSize) {
        DEBUG_LOG_WARNING() << "[NodeDataSerializer] Payload size mismatch";
        return nullptr;
    }

    const uint8_t* dataPtr = &payload[6];

    // Dispatch to type-specific deserializer
    switch (typeId) {
        case TYPE_CVIMAGE:       return deserializeCVImage(dataPtr, dataSize);
        case TYPE_CVPOINT:       return deserializeCVPoint(dataPtr, dataSize);
        case TYPE_CVRECT:        return deserializeCVRect(dataPtr, dataSize);
        case TYPE_CVSCALAR:      return deserializeCVScalar(dataPtr, dataSize);
        case TYPE_CVSIZE:        return deserializeCVSize(dataPtr, dataSize);
        case TYPE_BOOL:          return deserializeBool(dataPtr, dataSize);
        case TYPE_INTEGER:       return deserializeInteger(dataPtr, dataSize);
        case TYPE_DOUBLE:        return deserializeDouble(dataPtr, dataSize);
        case TYPE_FLOAT:         return deserializeFloat(dataPtr, dataSize);
        case TYPE_STDSTRING:     return deserializeStdString(dataPtr, dataSize);
        case TYPE_STDVECTOR_INT: return deserializeStdVector<int>(dataPtr, dataSize);
        case TYPE_STDVECTOR_FLT: return deserializeStdVector<float>(dataPtr, dataSize);
        case TYPE_STDVECTOR_DBL: return deserializeStdVector<double>(dataPtr, dataSize);
        case TYPE_SYNCDATA:      return deserializeSyncData(dataPtr, dataSize);
        case TYPE_INFORMATION:   return deserializeInformation(dataPtr, dataSize);
        default:
            DEBUG_LOG_WARNING() << "[NodeDataSerializer] Unknown type ID:" << typeId;
            return nullptr;
    }
}

std::shared_ptr<QtNodes::NodeData> NodeDataSerializer::deserialize(const QByteArray& payload)
{
    // Convert QByteArray to std::vector and call main deserialize
    std::vector<uint8_t> vec(reinterpret_cast<const uint8_t*>(payload.data()),
                             reinterpret_cast<const uint8_t*>(payload.data()) + payload.size());
    return deserialize(vec);
}

// ============================================================================
// CVImageData Serialization (with JPEG compression for efficiency)
// ============================================================================

std::vector<uint8_t> NodeDataSerializer::serializeCVImage(const CVImageData* data,
                                                          ImageEncoding imageEncoding)
{
    std::vector<uint8_t> result;
    
    // Write header
    result.push_back(PROTOCOL_VERSION);
    result.push_back(TYPE_CVIMAGE);
    
    // Get cv::Mat
    cv::Mat mat = const_cast<CVImageData*>(data)->data();
    
    if (mat.empty()) {
        writeUInt32(result, 0);  // Empty image
        return result;
    }

    std::vector<uint8_t> encodedBody;
    encodedBody.push_back(static_cast<uint8_t>(imageEncoding));

    if (imageEncoding == ImageEncoding::JPEG) {
        std::vector<uint8_t> compressed;
        std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 95};
        if (!cv::imencode(".jpg", mat, compressed, params)) {
            DEBUG_LOG_WARNING() << "[NodeDataSerializer] Failed to JPEG-encode image";
            return std::vector<uint8_t>();
        }
        encodedBody.insert(encodedBody.end(), compressed.begin(), compressed.end());
    }
    else if (imageEncoding == ImageEncoding::PNG) {
        std::vector<uint8_t> compressed;
        if (!cv::imencode(".png", mat, compressed)) {
            DEBUG_LOG_WARNING() << "[NodeDataSerializer] Failed to PNG-encode image";
            return std::vector<uint8_t>();
        }
        encodedBody.insert(encodedBody.end(), compressed.begin(), compressed.end());
    }
    else if (imageEncoding == ImageEncoding::RAW) {
        writeUInt32(encodedBody, static_cast<uint32_t>(mat.cols));
        writeUInt32(encodedBody, static_cast<uint32_t>(mat.rows));
        writeInt32(encodedBody, mat.type());
        writeUInt32(encodedBody, static_cast<uint32_t>(mat.step));

        const size_t rawSize = mat.step * static_cast<size_t>(mat.rows);
        encodedBody.insert(encodedBody.end(), mat.data, mat.data + rawSize);
    }
    else {
        DEBUG_LOG_WARNING() << "[NodeDataSerializer] Unknown image encoding";
        return std::vector<uint8_t>();
    }

    // Write data size
    writeUInt32(result, static_cast<uint32_t>(encodedBody.size()));

    // Append encoded image body
    result.insert(result.end(), encodedBody.begin(), encodedBody.end());
    
    return result;
}

std::shared_ptr<QtNodes::NodeData> NodeDataSerializer::deserializeCVImage(const uint8_t* data, uint32_t size)
{
    constexpr uint8_t LEGACY_JPEG_SOI_MARKER = 0xFF;

    if (size == 0) {
        return std::make_shared<CVImageData>();  // Empty image
    }

    const uint8_t encodingPrefix = data[0];

    // Backward compatibility: legacy payload has no prefix and starts with JPEG SOI (0xFF 0xD8)
    if (encodingPrefix == LEGACY_JPEG_SOI_MARKER) {
        std::vector<uint8_t> compressed(data, data + size);
        cv::Mat mat = cv::imdecode(compressed, cv::IMREAD_UNCHANGED);

        if (mat.empty()) {
            DEBUG_LOG_WARNING() << "[NodeDataSerializer] Failed to decode legacy JPEG image";
            return nullptr;
        }

        return std::make_shared<CVImageData>(std::move(mat));
    }

    if (size < 1) {
        DEBUG_LOG_WARNING() << "[NodeDataSerializer] Invalid CVImage payload size";
        return nullptr;
    }

    const uint8_t* encodedData = data + 1;
    const uint32_t encodedSize = size - 1;

    cv::Mat mat;
    if (encodingPrefix == static_cast<uint8_t>(ImageEncoding::JPEG) ||
        encodingPrefix == static_cast<uint8_t>(ImageEncoding::PNG)) {
        std::vector<uint8_t> compressed(encodedData, encodedData + encodedSize);
        mat = cv::imdecode(compressed, cv::IMREAD_UNCHANGED);
    }
    else if (encodingPrefix == static_cast<uint8_t>(ImageEncoding::RAW)) {
        if (encodedSize < 16) {
            DEBUG_LOG_WARNING() << "[NodeDataSerializer] RAW image payload too small";
            return nullptr;
        }

        const uint32_t cols = readUInt32(&encodedData[0]);
        const uint32_t rows = readUInt32(&encodedData[4]);
        const int32_t cvType = readInt32(&encodedData[8]);
        const uint32_t step = readUInt32(&encodedData[12]);

        const uint8_t* rawPtr = &encodedData[16];
        const uint32_t rawSize = encodedSize - 16;
        const size_t expectedRawSize = static_cast<size_t>(step) * static_cast<size_t>(rows);
        const int depth = CV_MAT_DEPTH(cvType);
        const int channels = CV_MAT_CN(cvType);

        if (depth < CV_8U || depth > CV_16F || channels <= 0 || channels > CV_CN_MAX) {
            DEBUG_LOG_WARNING() << "[NodeDataSerializer] Invalid RAW image cv::Mat type";
            return nullptr;
        }

        const size_t minStep = static_cast<size_t>(cols) * CV_ELEM_SIZE(cvType);

        if (cols == 0 || rows == 0 || step < minStep || rawSize != expectedRawSize) {
            DEBUG_LOG_WARNING() << "[NodeDataSerializer] Invalid RAW image metadata or size";
            return nullptr;
        }

        // Wrap source bytes then clone to own storage beyond payload lifetime.
        cv::Mat wrapped(static_cast<int>(rows),
                        static_cast<int>(cols),
                        cvType,
                        const_cast<uint8_t*>(rawPtr),
                        static_cast<size_t>(step));
        mat = wrapped.clone();
    }
    else {
        DEBUG_LOG_WARNING() << "[NodeDataSerializer] Unknown image encoding prefix:" << encodingPrefix;
        return nullptr;
    }

    if (mat.empty()) {
        DEBUG_LOG_WARNING() << "[NodeDataSerializer] Failed to decode image";
        return nullptr;
    }

    // Move the decoded Mat into CVImageData to avoid an extra deep copy
    return std::make_shared<CVImageData>(std::move(mat));
}

// ============================================================================
// CVPointData Serialization (2 x int32)
// ============================================================================

std::vector<uint8_t> NodeDataSerializer::serializeCVPoint(const CVPointData* data)
{
    std::vector<uint8_t> result;
    result.push_back(PROTOCOL_VERSION);
    result.push_back(TYPE_CVPOINT);
    writeUInt32(result, 8);  // 2 x int32 = 8 bytes

    cv::Point pt = const_cast<CVPointData*>(data)->data();
    writeInt32(result, pt.x);
    writeInt32(result, pt.y);

    return result;
}

std::shared_ptr<QtNodes::NodeData> NodeDataSerializer::deserializeCVPoint(const uint8_t* data, uint32_t size)
{
    if (size != 8) {
        DEBUG_LOG_WARNING() << "[NodeDataSerializer] Invalid CVPoint size";
        return nullptr;
    }

    int32_t x = readInt32(&data[0]);
    int32_t y = readInt32(&data[4]);

    return std::make_shared<CVPointData>(cv::Point(x, y));
}

// ============================================================================
// CVRectData Serialization (4 x int32)
// ============================================================================

std::vector<uint8_t> NodeDataSerializer::serializeCVRect(const CVRectData* data)
{
    std::vector<uint8_t> result;
    result.push_back(PROTOCOL_VERSION);
    result.push_back(TYPE_CVRECT);
    writeUInt32(result, 16);  // 4 x int32 = 16 bytes

    cv::Rect rect = const_cast<CVRectData*>(data)->data();
    writeInt32(result, rect.x);
    writeInt32(result, rect.y);
    writeInt32(result, rect.width);
    writeInt32(result, rect.height);

    return result;
}

std::shared_ptr<QtNodes::NodeData> NodeDataSerializer::deserializeCVRect(const uint8_t* data, uint32_t size)
{
    if (size != 16) {
        DEBUG_LOG_WARNING() << "[NodeDataSerializer] Invalid CVRect size";
        return nullptr;
    }

    int32_t x = readInt32(&data[0]);
    int32_t y = readInt32(&data[4]);
    int32_t width = readInt32(&data[8]);
    int32_t height = readInt32(&data[12]);

    return std::make_shared<CVRectData>(cv::Rect(x, y, width, height));
}

// ============================================================================
// CVScalarData Serialization (4 x double)
// ============================================================================

std::vector<uint8_t> NodeDataSerializer::serializeCVScalar(const CVScalarData* data)
{
    std::vector<uint8_t> result;
    result.push_back(PROTOCOL_VERSION);
    result.push_back(TYPE_CVSCALAR);
    writeUInt32(result, 32);  // 4 x double = 32 bytes

    cv::Scalar scalar = const_cast<CVScalarData*>(data)->scalar();
    writeDouble(result, scalar[0]);
    writeDouble(result, scalar[1]);
    writeDouble(result, scalar[2]);
    writeDouble(result, scalar[3]);

    return result;
}

std::shared_ptr<QtNodes::NodeData> NodeDataSerializer::deserializeCVScalar(const uint8_t* data, uint32_t size)
{
    if (size != 32) {
        DEBUG_LOG_WARNING() << "[NodeDataSerializer] Invalid CVScalar size";
        return nullptr;
    }

    double v0 = readDouble(&data[0]);
    double v1 = readDouble(&data[8]);
    double v2 = readDouble(&data[16]);
    double v3 = readDouble(&data[24]);

    return std::make_shared<CVScalarData>(cv::Scalar(v0, v1, v2, v3));
}

// ============================================================================
// CVSizeData Serialization (2 x int32)
// ============================================================================

std::vector<uint8_t> NodeDataSerializer::serializeCVSize(const CVSizeData* data)
{
    std::vector<uint8_t> result;
    result.push_back(PROTOCOL_VERSION);
    result.push_back(TYPE_CVSIZE);
    writeUInt32(result, 8);  // 2 x int32 = 8 bytes

    cv::Size size = const_cast<CVSizeData*>(data)->data();
    writeInt32(result, size.width);
    writeInt32(result, size.height);

    return result;
}

std::shared_ptr<QtNodes::NodeData> NodeDataSerializer::deserializeCVSize(const uint8_t* data, uint32_t size)
{
    if (size != 8) {
        DEBUG_LOG_WARNING() << "[NodeDataSerializer] Invalid CVSize size";
        return nullptr;
    }

    int32_t width = readInt32(&data[0]);
    int32_t height = readInt32(&data[4]);

    return std::make_shared<CVSizeData>(cv::Size(width, height));
}

// ============================================================================
// BoolData Serialization (1 byte)
// ============================================================================

std::vector<uint8_t> NodeDataSerializer::serializeBool(const BoolData* data)
{
    std::vector<uint8_t> result;
    result.push_back(PROTOCOL_VERSION);
    result.push_back(TYPE_BOOL);
    writeUInt32(result, 1);  // 1 byte

    bool value = const_cast<BoolData*>(data)->data();
    result.push_back(value ? 0x01 : 0x00);

    return result;
}

std::shared_ptr<QtNodes::NodeData> NodeDataSerializer::deserializeBool(const uint8_t* data, uint32_t size)
{
    if (size != 1) {
        DEBUG_LOG_WARNING() << "[NodeDataSerializer] Invalid Bool size";
        return nullptr;
    }

    bool value = (data[0] != 0x00);
    return std::make_shared<BoolData>(value);
}

// ============================================================================
// IntegerData Serialization (int32)
// ============================================================================

std::vector<uint8_t> NodeDataSerializer::serializeInteger(const IntegerData* data)
{
    std::vector<uint8_t> result;
    result.push_back(PROTOCOL_VERSION);
    result.push_back(TYPE_INTEGER);
    writeUInt32(result, 4);  // int32 = 4 bytes

    int value = const_cast<IntegerData*>(data)->data();
    writeInt32(result, value);

    return result;
}

std::shared_ptr<QtNodes::NodeData> NodeDataSerializer::deserializeInteger(const uint8_t* data, uint32_t size)
{
    if (size != 4) {
        DEBUG_LOG_WARNING() << "[NodeDataSerializer] Invalid Integer size";
        return nullptr;
    }

    int32_t value = readInt32(&data[0]);
    return std::make_shared<IntegerData>(value);
}

// ============================================================================
// DoubleData Serialization (double)
// ============================================================================

std::vector<uint8_t> NodeDataSerializer::serializeDouble(const DoubleData* data)
{
    std::vector<uint8_t> result;
    result.push_back(PROTOCOL_VERSION);
    result.push_back(TYPE_DOUBLE);
    writeUInt32(result, 8);  // double = 8 bytes

    double value = const_cast<DoubleData*>(data)->data();
    writeDouble(result, value);

    return result;
}

std::shared_ptr<QtNodes::NodeData> NodeDataSerializer::deserializeDouble(const uint8_t* data, uint32_t size)
{
    if (size != 8) {
        DEBUG_LOG_WARNING() << "[NodeDataSerializer] Invalid Double size";
        return nullptr;
    }

    double value = readDouble(&data[0]);
    return std::make_shared<DoubleData>(value);
}

// ============================================================================
// FloatData Serialization (float)
// ============================================================================

std::vector<uint8_t> NodeDataSerializer::serializeFloat(const FloatData* data)
{
    std::vector<uint8_t> result;
    result.push_back(PROTOCOL_VERSION);
    result.push_back(TYPE_FLOAT);
    writeUInt32(result, 4);  // float = 4 bytes

    float value = const_cast<FloatData*>(data)->data();
    writeFloat(result, value);

    return result;
}

std::shared_ptr<QtNodes::NodeData> NodeDataSerializer::deserializeFloat(const uint8_t* data, uint32_t size)
{
    if (size != 4) {
        DEBUG_LOG_WARNING() << "[NodeDataSerializer] Invalid Float size";
        return nullptr;
    }

    float value = readFloat(&data[0]);
    return std::make_shared<FloatData>(value);
}

// ============================================================================
// StdStringData Serialization (length + UTF-8 bytes)
// ============================================================================

std::vector<uint8_t> NodeDataSerializer::serializeStdString(const StdStringData* data)
{
    std::vector<uint8_t> result;
    result.push_back(PROTOCOL_VERSION);
    result.push_back(TYPE_STDSTRING);

    std::string str = const_cast<StdStringData*>(data)->data();
    uint32_t length = static_cast<uint32_t>(str.size());

    writeUInt32(result, 4 + length);  // size = length(4) + string bytes
    writeUInt32(result, length);
    result.insert(result.end(), str.begin(), str.end());

    return result;
}

std::shared_ptr<QtNodes::NodeData> NodeDataSerializer::deserializeStdString(const uint8_t* data, uint32_t size)
{
    if (size < 4) {
        DEBUG_LOG_WARNING() << "[NodeDataSerializer] Invalid StdString size";
        return nullptr;
    }

    uint32_t length = readUInt32(&data[0]);
    if (size != 4 + length) {
        DEBUG_LOG_WARNING() << "[NodeDataSerializer] StdString size mismatch";
        return nullptr;
    }

    std::string str(reinterpret_cast<const char*>(&data[4]), length);
    return std::make_shared<StdStringData>(str);
}

// ============================================================================
// SyncData Serialization (1 byte: bool state)
// ============================================================================

std::vector<uint8_t> NodeDataSerializer::serializeSyncData(const SyncData* data)
{
    std::vector<uint8_t> result;
    result.push_back(PROTOCOL_VERSION);
    result.push_back(TYPE_SYNCDATA);
    writeUInt32(result, 1);  // 1 byte

    bool value = const_cast<SyncData*>(data)->data();
    result.push_back(value ? 0x01 : 0x00);

    return result;
}

std::shared_ptr<QtNodes::NodeData> NodeDataSerializer::deserializeSyncData(const uint8_t* data, uint32_t size)
{
    if (size != 1) {
        DEBUG_LOG_WARNING() << "[NodeDataSerializer] Invalid SyncData size";
        return nullptr;
    }

    auto syncData = std::make_shared<SyncData>();
    syncData->set_data(data[0] != 0x00);
    return syncData;
}

// ============================================================================
// InformationData Serialization (timestamp + UTF-8 text)
// ============================================================================

std::vector<uint8_t> NodeDataSerializer::serializeInformation(const InformationData* data)
{
    std::vector<uint8_t> result;
    result.push_back(PROTOCOL_VERSION);
    result.push_back(TYPE_INFORMATION);

    const QString infoText = const_cast<InformationData*>(data)->info();
    const QByteArray utf8 = infoText.toUtf8();
    const uint32_t length = static_cast<uint32_t>(utf8.size());
    const int64_t timestamp = static_cast<int64_t>(const_cast<InformationData*>(data)->timestamp());

    // size = timestamp(8) + string length(4) + UTF-8 bytes
    writeUInt32(result, 12 + length);
    writeInt64(result, timestamp);
    writeUInt32(result, length);
    result.insert(result.end(), utf8.begin(), utf8.end());

    return result;
}

std::shared_ptr<QtNodes::NodeData> NodeDataSerializer::deserializeInformation(const uint8_t* data, uint32_t size)
{
    // Minimum = timestamp(8) + string length(4)
    if (size < 12) {
        DEBUG_LOG_WARNING() << "[NodeDataSerializer] Invalid InformationData size";
        return nullptr;
    }

    const int64_t timestamp = readInt64(&data[0]);
    const uint32_t length = readUInt32(&data[8]);

    if (size != 12 + length) {
        DEBUG_LOG_WARNING() << "[NodeDataSerializer] InformationData size mismatch";
        return nullptr;
    }

    const QString text = QString::fromUtf8(reinterpret_cast<const char*>(&data[12]), static_cast<int>(length));
    auto infoData = std::make_shared<InformationData>(text);
    infoData->set_timestamp(static_cast<long int>(timestamp));
    return infoData;
}

// ============================================================================
// StdVectorNumberData Serialization (count + elements)
// ============================================================================

template<typename T>
std::vector<uint8_t> NodeDataSerializer::serializeStdVector(const StdVectorNumberData<T>* data, DataTypeID typeId)
{
    std::vector<uint8_t> result;
    result.push_back(PROTOCOL_VERSION);
    result.push_back(typeId);

    std::vector<T> vec = const_cast<StdVectorNumberData<T>*>(data)->data();
    uint32_t count = static_cast<uint32_t>(vec.size());
    uint32_t dataSize = 4 + count * sizeof(T);  // count(4) + elements

    writeUInt32(result, dataSize);
    writeUInt32(result, count);

    // Serialize each element
    for (const T& value : vec) {
        if constexpr (std::is_same_v<T, int>) {
            writeInt32(result, value);
        } else if constexpr (std::is_same_v<T, float>) {
            writeFloat(result, value);
        } else if constexpr (std::is_same_v<T, double>) {
            writeDouble(result, value);
        }
    }

    return result;
}

template<typename T>
std::shared_ptr<QtNodes::NodeData> NodeDataSerializer::deserializeStdVector(const uint8_t* data, uint32_t size)
{
    if (size < 4) {
        DEBUG_LOG_WARNING() << "[NodeDataSerializer] Invalid StdVector size";
        return nullptr;
    }

    uint32_t count = readUInt32(&data[0]);
    uint32_t expectedSize = 4 + count * sizeof(T);

    if (size != expectedSize) {
        DEBUG_LOG_WARNING() << "[NodeDataSerializer] StdVector size mismatch";
        return nullptr;
    }

    std::vector<T> vec;
    vec.reserve(count);

    const uint8_t* ptr = &data[4];
    for (uint32_t i = 0; i < count; ++i) {
        if constexpr (std::is_same_v<T, int>) {
            vec.push_back(readInt32(ptr));
            ptr += 4;
        } else if constexpr (std::is_same_v<T, float>) {
            vec.push_back(readFloat(ptr));
            ptr += 4;
        } else if constexpr (std::is_same_v<T, double>) {
            vec.push_back(readDouble(ptr));
            ptr += 8;
        }
    }

    return std::make_shared<StdVectorNumberData<T>>(vec);
}

// ============================================================================
// Helper Functions (Little-Endian Binary I/O)
// ============================================================================

void NodeDataSerializer::writeUInt32(std::vector<uint8_t>& buffer, uint32_t value)
{
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

uint32_t NodeDataSerializer::readUInt32(const uint8_t* data)
{
    return static_cast<uint32_t>(data[0]) |
           (static_cast<uint32_t>(data[1]) << 8) |
           (static_cast<uint32_t>(data[2]) << 16) |
           (static_cast<uint32_t>(data[3]) << 24);
}

void NodeDataSerializer::writeInt32(std::vector<uint8_t>& buffer, int32_t value)
{
    writeUInt32(buffer, static_cast<uint32_t>(value));
}

int32_t NodeDataSerializer::readInt32(const uint8_t* data)
{
    return static_cast<int32_t>(readUInt32(data));
}

void NodeDataSerializer::writeDouble(std::vector<uint8_t>& buffer, double value)
{
    uint64_t bits;
    std::memcpy(&bits, &value, sizeof(double));
    
    for (int i = 0; i < 8; ++i) {
        buffer.push_back(static_cast<uint8_t>((bits >> (i * 8)) & 0xFF));
    }
}

double NodeDataSerializer::readDouble(const uint8_t* data)
{
    uint64_t bits = 0;
    for (int i = 0; i < 8; ++i) {
        bits |= (static_cast<uint64_t>(data[i]) << (i * 8));
    }
    
    double value;
    std::memcpy(&value, &bits, sizeof(double));
    return value;
}

void NodeDataSerializer::writeFloat(std::vector<uint8_t>& buffer, float value)
{
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(float));
    writeUInt32(buffer, bits);
}

float NodeDataSerializer::readFloat(const uint8_t* data)
{
    uint32_t bits = readUInt32(data);
    float value;
    std::memcpy(&value, &bits, sizeof(float));
    return value;
}

void NodeDataSerializer::writeInt64(std::vector<uint8_t>& buffer, int64_t value)
{
    const uint64_t bits = static_cast<uint64_t>(value);
    for (int i = 0; i < 8; ++i) {
        buffer.push_back(static_cast<uint8_t>((bits >> (i * 8)) & 0xFF));
    }
}

int64_t NodeDataSerializer::readInt64(const uint8_t* data)
{
    uint64_t bits = 0;
    for (int i = 0; i < 8; ++i) {
        bits |= (static_cast<uint64_t>(data[i]) << (i * 8));
    }
    return static_cast<int64_t>(bits);
}

// Explicit template instantiations
template std::vector<uint8_t> NodeDataSerializer::serializeStdVector<int>(const StdVectorNumberData<int>*, DataTypeID);
template std::vector<uint8_t> NodeDataSerializer::serializeStdVector<float>(const StdVectorNumberData<float>*, DataTypeID);
template std::vector<uint8_t> NodeDataSerializer::serializeStdVector<double>(const StdVectorNumberData<double>*, DataTypeID);

template std::shared_ptr<QtNodes::NodeData> NodeDataSerializer::deserializeStdVector<int>(const uint8_t*, uint32_t);
template std::shared_ptr<QtNodes::NodeData> NodeDataSerializer::deserializeStdVector<float>(const uint8_t*, uint32_t);
template std::shared_ptr<QtNodes::NodeData> NodeDataSerializer::deserializeStdVector<double>(const uint8_t*, uint32_t);
