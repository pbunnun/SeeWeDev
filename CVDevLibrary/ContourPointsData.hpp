//Copyright © 2024, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

/**
 * @file ContourPointsData.hpp
 * @brief Data type for passing contour information between nodes
 * 
 * This file defines a data structure that encapsulates contours (connected sequences
 * of points) for transmission through the data flow graph. Contours are fundamental
 * in computer vision for shape analysis, object detection, and boundary representation.
 */

#pragma once

#include "InformationData.hpp"
#include <opencv2/core/types.hpp>

using QtNodes::NodeData;
using QtNodes::NodeDataType;

/**
 * @class ContourPointsData
 * @brief Node data type for representing image contours
 * 
 * This class wraps a collection of contours for transmission between nodes in the
 * data flow graph. Each contour is represented as a vector of cv::Point, and multiple
 * contours can be stored (e.g., from cv::findContours).
 * 
 * Data structure hierarchy:
 * ```
 * ContourPointsData
 *   └─ std::vector<std::vector<cv::Point>>  (collection of contours)
 *        └─ std::vector<cv::Point>           (single contour)
 *             └─ cv::Point                    (x, y coordinate)
 * ```
 * 
 * Typical sources of contour data:
 * - cv::findContours() - Extract contours from binary images
 * - cv::convexHull() - Convex hull around point sets
 * - Shape approximation algorithms (cv::approxPolyDP)
 * - Hand-drawn or synthetic shapes
 * 
 * Common operations on contours:
 * - Drawing: cv::drawContours() to visualize
 * - Analysis: Area, perimeter, moments, bounding boxes
 * - Approximation: Simplify contours to fewer points
 * - Matching: Compare shapes using contour descriptors
 * - Fitting: Fit ellipses, lines, or circles to contours
 * 
 * Use cases:
 * - Object detection and recognition
 * - Shape analysis and classification
 * - Measurement of objects (size, orientation)
 * - Path planning around obstacles
 * - Optical character recognition (character boundaries)
 * 
 * Design Note: Inherits from InformationData to provide base functionality
 * for metadata and timing information that may accompany contour data.
 * 
 * @note Contours are typically extracted from binary images (thresholded or edge-detected)
 * @see cv::findContours for contour extraction
 * @see cv::drawContours for visualization
 * @see InformationData for base class functionality
 */
class ContourPointsData : public InformationData
{
public:
    /**
     * @brief Default constructor creating empty contour data
     * 
     * Initializes with no contours. Contours can be added later via data().
     */
    ContourPointsData()
        : mvvPoints()
    {}

    /**
     * @brief Constructs contour data from a vector of contours
     * 
     * Creates ContourPointsData initialized with the provided contours.
     * Each contour is a sequence of points representing a closed or open curve.
     * 
     * @param data Vector of contours, where each contour is a vector of cv::Point
     * @note Typical usage: Pass output directly from cv::findContours()
     */
    ContourPointsData(const std::vector< std::vector<cv::Point> > & data )
        : mvvPoints( data )
    {}

    /**
     * @brief Returns the data type identifier for the node editor
     * 
     * Provides type information for the node editor's connection validation.
     * Only ports with matching types can be connected.
     * 
     * @return NodeDataType with name "Contours" and abbreviation "Cnt"
     */
    NodeDataType
    type() const override
    {
        return { "Contours", "Cnt" };
    }

    /**
     * @brief Provides access to the contour data
     * 
     * Returns a reference to the internal contour storage, allowing both
     * reading and modification of contour points.
     * 
     * @return Reference to vector of contours (each contour is a vector of points)
     * @note Modifying the returned reference directly changes the internal data
     */
    std::vector< std::vector<cv::Point> > &
    data()
    {
        return mvvPoints;
    }

private:
    /** 
     * @brief Collection of contours
     * 
     * Outer vector: Multiple contours (e.g., multiple objects)
     * Inner vector: Points in a single contour (boundary of one object)
     * 
     * Example:
     * - mvvPoints[0] = first contour (e.g., outer boundary of object 1)
     * - mvvPoints[0][0] = first point of first contour
     * - mvvPoints[1] = second contour (e.g., object 2 or hole in object 1)
     */
    std::vector< std::vector<cv::Point> > mvvPoints;
};


