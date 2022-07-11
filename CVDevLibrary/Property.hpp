//Copyright © 2022, NECTEC, all rights reserved

//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.

#ifndef PROPERTY_HPP
#define PROPERTY_HPP
#include <vector>
#include <memory>
#include <QString>
#include <QVariant>

#pragma once

#if									\
   defined( linux )				||	\
   defined( __linux )			||	\
   defined( __linux__ )
       #include <memory>
#endif

typedef struct EnumPropertyType {
    int miCurrentIndex{0};
    QStringList mslEnumNames;
} EnumPropertyType;

typedef struct DoublePropertyType {
    double mdValue{0};
    double mdMax{100};
    double mdMin{0};
} DoublePropertyType;

typedef struct IntPropertyType {
    int miValue{0};
    int miMax{100};
    int miMin{0};
} IntPropertyType;

typedef struct UcharPropertyType {
    int mucValue{0}; //UcharPropertyType is always treated as
    int mucMax{255}; //IntPropertyType with QVariant::Int
    int mucMin{0};   //Only cast its member to uchar right before each
} UcharPropertyType; //implementation using static_cast<uchar>(mucVar)

typedef struct FilePathPropertyType {
    QString msFilename;
    QString msFilter;
    QString msMode{"open"};
} FilePathPropertyType;

typedef struct PathPropertyType {
    QString msPath;
} PathPropertyType;

typedef struct SizePropertyType {
    int miWidth{0};
    int miHeight{0};
} SizePropertyType;

typedef struct RectPropertyType {
    int miXPosition{0};
    int miYPosition{0};
    int miWidth{0};
    int miHeight{0};
} RectPropertyType;

typedef struct PointPropertyType {
    int miXPosition{0};
    int miYPosition{0};
} PointPropertyType;

typedef struct SizeFPropertyType {
    float mfWidth{0.};
    float mfHeight{0.};
} SizeFPropertyType;

typedef struct PointFPropertyType {
    float mfXPosition{0.};
    float mfYPosition{0.};
} PointFPropertyType;

class Property
{
public:
    Property(const QString & name, const QString & id, int type ) : msName(name), msID(id), miType(type) {}
    virtual ~Property() {}
    QString getName() { return msName; };
    QString getID() { return msID; };
    int getType() { return miType; };
private:
    QString msName;
    QString msID;
    int miType;
};

template < typename T >
class TypedProperty : public Property
{
public:
    TypedProperty( const QString & name, const QString & id, int type, const T & data, QString sSubPropertyText = "", bool bReadOnly = false )
        : Property(name, id, type), mData(data), msSubPropertyText(sSubPropertyText), mbReadOnly(bReadOnly) {};
    T & getData() { return mData; };
    QString getSubPropertyText() { return msSubPropertyText; };
    bool isReadOnly() { return mbReadOnly; };
private:
    T mData;
    QString msSubPropertyText;
    bool mbReadOnly;
};

typedef std::vector< std::shared_ptr<Property> > PropertyVector;

#endif // PROPERTY_HPP
