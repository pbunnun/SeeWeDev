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

#pragma once

/**
 * @file NodeDataSerializer.hpp
 * @brief Serialization framework for CVDev data types to transport payloads.
 *
 * This file provides serialization and deserialization functions for all CVDev
 * NodeData types, enabling them to be transmitted over transport channels.
 *
 * **Key Features:**
 * - **Binary Serialization:** Efficient binary encoding for performance
 * - **Type Safety:** Type identification in payload for runtime validation
 * - **cv::Mat Optimization:** Efficient encoding of OpenCV matrices
 * - **Version Control:** Payload versioning for future compatibility
 *
 * **Supported Data Types:**
 * - CVImageData (cv::Mat with compression support)
 * - CVPointData (cv::Point as 2 ints)
 * - CVRectData (cv::Rect as 4 ints)
 * - CVScalarData (cv::Scalar as 4 doubles)
 * - CVSizeData (cv::Size as 2 ints)
 * - BoolData (single byte)
 * - IntegerData (4 bytes)
 * - DoubleData (8 bytes)
 * - FloatData (4 bytes)
 * - StdStringData (length + UTF-8 bytes)
 * - StdVectorNumberData (count + elements)
 * - SyncData (single byte boolean)
 *
 * **Payload Format:**
 * @code
 * [Version:1] [Type:1] [DataSize:4] [Data:N]
 * 
 * Version: Protocol version (0x01)
 * Type: Data type identifier (0x01-0x0F)
 * DataSize: Size of data section in bytes
 * Data: Type-specific binary data
 * @endcode
 *
 * **Usage Example:**
 * @code
 * // Serialize
 * auto imageData = std::make_shared<CVImageData>(cvMat);
 * std::vector<uint8_t> payload = NodeDataSerializer::serialize(imageData);
 * 
 * // Send via Zenoh
 * zenoh_session.put(key, payload);
 * 
 * // Receive and deserialize
 * std::shared_ptr<NodeData> data = NodeDataSerializer::deserialize(payload);
 * auto image = std::dynamic_pointer_cast<CVImageData>(data);
 * @endcode
 *
 * **Performance Considerations:**
 * - cv::Mat serialization includes optional JPEG compression
 * - Small data types (<100 bytes) serialize in microseconds
 * - Large images (1920x1080) serialize in 5-20ms with compression
 * - Binary format is more efficient than JSON/XML
 *
 * @see ZenohBridge, CycloneDDSBridge for pub/sub integration
 * @see PBNodeDelegateModel for node-level usage
 */

#include <vector>
#include <memory>
#include <cstdint>
#include <cstring>
#include <QtNodes/NodeData>
#include <opencv2/opencv.hpp>

#include "CVImageData.hpp"
#include "CVPointData.hpp"
#include "CVRectData.hpp"
#include "CVScalarData.hpp"
#include "CVSizeData.hpp"
#include "BoolData.hpp"
#include "IntegerData.hpp"
#include "DoubleData.hpp"
#include "FloatData.hpp"
#include "InformationData.hpp"
#include "StdStringData.hpp"
#include "StdVectorNumberData.hpp"
#include "SyncData.hpp"

/**
 * @class NodeDataSerializer
 * @brief Static utility class for serializing/deserializing CVDev data types.
 *
 * Provides binary serialization for all CVDev NodeData types, enabling
 * transmission over Zenoh pub/sub channels with type safety and versioning.
 */
class NodeDataSerializer
{
public:
    enum class ImageEncoding : uint8_t {
        JPEG = 0x00,
        PNG  = 0x01,
        RAW  = 0x02
    };

    /**
     * @enum DataTypeID
     * @brief Type identifiers for serialized data.
     *
     * Single-byte type codes used in serialized payloads to identify
     * the data type and guide deserialization.
     */
    enum DataTypeID : uint8_t {
        TYPE_UNKNOWN       = 0x00,
        TYPE_CVIMAGE       = 0x01,  ///< CVImageData (cv::Mat)
        TYPE_CVPOINT       = 0x02,  ///< CVPointData (cv::Point)
        TYPE_CVRECT        = 0x03,  ///< CVRectData (cv::Rect)
        TYPE_CVSCALAR      = 0x04,  ///< CVScalarData (cv::Scalar)
        TYPE_CVSIZE        = 0x05,  ///< CVSizeData (cv::Size)
        TYPE_BOOL          = 0x06,  ///< BoolData
        TYPE_INTEGER       = 0x07,  ///< IntegerData
        TYPE_DOUBLE        = 0x08,  ///< DoubleData
        TYPE_FLOAT         = 0x09,  ///< FloatData
        TYPE_STDSTRING     = 0x0A,  ///< StdStringData
        TYPE_STDVECTOR_INT = 0x0B,  ///< StdVectorIntData
        TYPE_STDVECTOR_FLT = 0x0C,  ///< StdVectorFloatData
        TYPE_STDVECTOR_DBL = 0x0D,  ///< StdVectorDoubleData
        TYPE_SYNCDATA      = 0x0E,  ///< SyncData
        TYPE_INFORMATION   = 0x0F   ///< InformationData
    };

    /**
     * @brief Serializes a NodeData object to binary payload.
     *
     * Converts any supported CVDev data type to a binary representation
     * suitable for transmission over Zenoh.
     *
     * @param data Shared pointer to NodeData (must be a supported type)
     * @return std::vector<uint8_t> Binary payload, empty if type unsupported
     *
     * **Example:**
     * @code
     * auto imageData = std::make_shared<CVImageData>(mat);
     * std::vector<uint8_t> payload = NodeDataSerializer::serialize(imageData);
     * 
     * if (!payload.empty()) {
     *     zenohSession.put(key, payload);
     * }
     * @endcode
     *
     * **Payload Structure:**
     * @code
     * Byte 0:    Protocol version (0x01)
     * Byte 1:    Type ID (DataTypeID enum)
     * Bytes 2-5: Data size (uint32_t, little-endian)
     * Bytes 6+:  Type-specific data
     * @endcode
     */
    static std::vector<uint8_t> serialize(std::shared_ptr<QtNodes::NodeData> data,
                                          ImageEncoding imageEncoding = ImageEncoding::RAW);

    /**
     * @brief Deserializes a binary payload to NodeData object.
     *
     * Reconstructs a CVDev data type from a Zenoh payload, using the
     * embedded type identifier to instantiate the correct class.
     *
     * @param payload Binary payload from Zenoh (must be valid format)
     * @return std::shared_ptr<QtNodes::NodeData> Reconstructed data, or nullptr if invalid
     *
     * **Example:**
     * @code
     * // Zenoh callback
     * void onData(std::vector<uint8_t> payload) {
     *     auto data = NodeDataSerializer::deserialize(payload);
     *     
     *     if (auto image = std::dynamic_pointer_cast<CVImageData>(data)) {
     *         processImage(image->data());
     *     }
     * }
     * @endcode
     *
     * **Error Handling:**
     * @code
     * auto data = NodeDataSerializer::deserialize(payload);
     * if (!data) {
     *     qWarning() << "Deserialization failed: invalid payload";
     *     return;
     * }
     * @endcode
     */
    static std::shared_ptr<QtNodes::NodeData> deserialize(const std::vector<uint8_t>& payload);

    /**
     * @brief Deserializes a QByteArray payload to NodeData object.
     *
     * Convenience overload for Qt-based code.
     *
     * @param payload Binary data from Zenoh
     * @return Reconstructed NodeData object, or nullptr if deserialization fails
     */
    static std::shared_ptr<QtNodes::NodeData> deserialize(const QByteArray& payload);

private:
    static constexpr uint8_t PROTOCOL_VERSION = 0x01;

    // Serialize individual types
    static std::vector<uint8_t> serializeCVImage(const CVImageData* data, ImageEncoding imageEncoding);
    static std::vector<uint8_t> serializeCVPoint(const CVPointData* data);
    static std::vector<uint8_t> serializeCVRect(const CVRectData* data);
    static std::vector<uint8_t> serializeCVScalar(const CVScalarData* data);
    static std::vector<uint8_t> serializeCVSize(const CVSizeData* data);
    static std::vector<uint8_t> serializeBool(const BoolData* data);
    static std::vector<uint8_t> serializeInteger(const IntegerData* data);
    static std::vector<uint8_t> serializeDouble(const DoubleData* data);
    static std::vector<uint8_t> serializeFloat(const FloatData* data);
    static std::vector<uint8_t> serializeStdString(const StdStringData* data);
    static std::vector<uint8_t> serializeSyncData(const SyncData* data);
    static std::vector<uint8_t> serializeInformation(const InformationData* data);
    
    template<typename T>
    static std::vector<uint8_t> serializeStdVector(const StdVectorNumberData<T>* data, DataTypeID typeId);

    // Deserialize individual types
    static std::shared_ptr<QtNodes::NodeData> deserializeCVImage(const uint8_t* data, uint32_t size);
    static std::shared_ptr<QtNodes::NodeData> deserializeCVPoint(const uint8_t* data, uint32_t size);
    static std::shared_ptr<QtNodes::NodeData> deserializeCVRect(const uint8_t* data, uint32_t size);
    static std::shared_ptr<QtNodes::NodeData> deserializeCVScalar(const uint8_t* data, uint32_t size);
    static std::shared_ptr<QtNodes::NodeData> deserializeCVSize(const uint8_t* data, uint32_t size);
    static std::shared_ptr<QtNodes::NodeData> deserializeBool(const uint8_t* data, uint32_t size);
    static std::shared_ptr<QtNodes::NodeData> deserializeInteger(const uint8_t* data, uint32_t size);
    static std::shared_ptr<QtNodes::NodeData> deserializeDouble(const uint8_t* data, uint32_t size);
    static std::shared_ptr<QtNodes::NodeData> deserializeFloat(const uint8_t* data, uint32_t size);
    static std::shared_ptr<QtNodes::NodeData> deserializeStdString(const uint8_t* data, uint32_t size);
    static std::shared_ptr<QtNodes::NodeData> deserializeSyncData(const uint8_t* data, uint32_t size);
    static std::shared_ptr<QtNodes::NodeData> deserializeInformation(const uint8_t* data, uint32_t size);
    
    template<typename T>
    static std::shared_ptr<QtNodes::NodeData> deserializeStdVector(const uint8_t* data, uint32_t size);

    // Helper functions
    static void writeUInt32(std::vector<uint8_t>& buffer, uint32_t value);
    static uint32_t readUInt32(const uint8_t* data);
    static void writeInt32(std::vector<uint8_t>& buffer, int32_t value);
    static int32_t readInt32(const uint8_t* data);
    static void writeDouble(std::vector<uint8_t>& buffer, double value);
    static double readDouble(const uint8_t* data);
    static void writeFloat(std::vector<uint8_t>& buffer, float value);
    static float readFloat(const uint8_t* data);
    static void writeInt64(std::vector<uint8_t>& buffer, int64_t value);
    static int64_t readInt64(const uint8_t* data);
};
