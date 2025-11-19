//Copyright © 2025, NECTEC, all rights reserved

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

#include "CVDevLibrary.hpp"
#include <QtNodes/NodeData>
#include <QDateTime>

using QtNodes::NodeData;
using QtNodes::NodeDataType;

/**
 * @class InformationData
 * @brief Base class for displayable node data with optional timestamping.
 *
 * Stores a QString for human-readable information and an optional timestamp.
 * Derived classes override set_information() to format specific data types.
 *
 * **Core Functionality:**
 * - **Information Storage:** QString for formatted display
 * - **Timestamp Tracking:** Optional long int timestamp (milliseconds since epoch)
 * - **Virtual Formatting:** Derived classes implement custom formatting
 * - **Type Identification:** NodeDataType {"Information", "Inf"}
 *
 * **Construction Patterns:**
 * @code
 * // Default construction
 * auto info = std::make_shared<InformationData>();
 * info->set_information("Ready");
 * 
 * // Direct initialization
 * auto info = std::make_shared<InformationData>("Processing...");
 * 
 * // With timestamp
 * auto info = std::make_shared<InformationData>("Complete");
 * info->set_timestamp();  // Current time
 * @endcode
 *
 * **Derivation Pattern:**
 * @code
 * class MyData : public InformationData {
 * public:
 *     void set_information() override {
 *         // Format member data into mQSData
 *         mQSData = QString("Value: %1").arg(mValue);
 *         set_timestamp();  // Optional
 *     }
 *     
 * private:
 *     double mValue{0.0};
 * };
 * @endcode
 *
 * @note Timestamps are in milliseconds since Unix epoch
 * @see CVSizeData for example derived class
 */
class CVDEVSHAREDLIB_EXPORT InformationData : public NodeData
{
public:
    /**
     * @brief Default constructor - creates empty information.
     *
     * Initializes with empty QString and zero timestamp.
     *
     * **Example:**
     * @code
     * auto info = std::make_shared<InformationData>();
     * info->set_information("Initialized");
     * @endcode
     */
    InformationData();

    /**
     * @brief Virtual destructor for proper polymorphic deletion.
     */
    virtual ~InformationData() = default;

    /**
     * @brief Constructs with initial information text.
     *
     * Sets the information string without timestamp.
     *
     * @param text Initial information text
     *
     * **Example:**
     * @code
     * auto info = std::make_shared<InformationData>("Ready");
     * QString msg = info->info();  // "Ready"
     * @endcode
     */
    InformationData(QString const &text);

    /**
     * @brief Returns the node data type identifier.
     *
     * @return NodeDataType with id="Information", name="Inf"
     *
     * **Example:**
     * @code
     * auto info = std::make_shared<InformationData>();
     * NodeDataType type = info->type();
     * // type.id == "Information"
     * // type.name == "Inf"
     * @endcode
     *
     * @note Derived classes may override with specific types
     */
    NodeDataType type() const override;

    /**
     * @brief Virtual method to update information from derived class data.
     *
     * Called by derived classes to format their specific data into mQSData.
     * Base implementation does nothing; derived classes override.
     *
     * **Derivation Example:**
     * @code
     * class SizeData : public InformationData {
     *     void set_information() override {
     *         mQSData = QString("[%1 px x %2 px]")
     *             .arg(mSize.height())
     *             .arg(mSize.width());
     *     }
     * private:
     *     cv::Size mSize;
     * };
     * @endcode
     *
     * @see CVSizeData::set_information() for example
     * @see CVRectData::set_information() for example
     */
    virtual void set_information();

    /**
     * @brief Sets the information text directly.
     *
     * Allows external setting of information without timestamp.
     *
     * @param text Information text to store
     *
     * **Example:**
     * @code
     * auto info = std::make_shared<InformationData>();
     * info->set_information("Processing frame 42");
     * @endcode
     */
    void set_information(QString const &text);

    /**
     * @brief Sets the timestamp to a specific value.
     *
     * @param time Timestamp in milliseconds since Unix epoch
     *
     * **Example:**
     * @code
     * long int captureTime = 1704067200000;  // 2024-01-01 00:00:00 UTC
     * info->set_timestamp(captureTime);
     * @endcode
     */
    void set_timestamp(long int const &time);

    /**
     * @brief Sets the timestamp to current system time.
     *
     * Uses system clock to get current milliseconds since epoch.
     *
     * **Example:**
     * @code
     * auto info = std::make_shared<InformationData>("Frame captured");
     * info->set_timestamp();  // Current time
     * 
     * long int when = info->timestamp();
     * qDebug() << "Captured at:" << when;
     * @endcode
     *
     * @note Automatically called by some derived classes in set_information()
     */
    void set_timestamp();

    /**
     * @brief Returns the stored information text.
     *
     * @return const QString& Reference to information string
     *
     * **Example:**
     * @code
     * auto info = std::make_shared<InformationData>("Ready");
     * QString text = info->info();  // "Ready"
     * 
     * // Display in UI
     * label->setText(info->info());
     * @endcode
     */
    QString const & info() const;

    /**
     * @brief Returns the stored timestamp.
     *
     * @return const long int& Timestamp in milliseconds since epoch
     *
     * **Example:**
     * @code
     * long int t = info->timestamp();
     * if (t > 0) {
     *     QDateTime dt = QDateTime::fromMSecsSinceEpoch(t);
     *     qDebug() << "Created at:" << dt.toString();
     * }
     * @endcode
     */
    long int const & timestamp() const;

protected:
    /**
     * @brief Information text storage.
     *
     * Formatted string for display, set by set_information().
     */
    QString mQSData{""};
    
    /**
     * @brief Timestamp in milliseconds since Unix epoch.
     *
     * Zero if not set. Use set_timestamp() to populate.
     */
    long int mlTimeStamp{0};
};