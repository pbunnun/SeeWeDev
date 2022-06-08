#ifndef PBNODEDATAMODEL_HPP
#define PBNODEDATAMODEL_HPP

#pragma once

#include "CVDevLibrary.hpp"
#include "Property.hpp"
#include "NodeDataModel"

using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeStyle;

class CVDEVSHAREDLIB_EXPORT PBNodeDataModel : public QtNodes::NodeDataModel
{
    Q_OBJECT
public:
    explicit PBNodeDataModel(QString modelName, bool bSource=false, bool bEnable=true);

    QJsonObject
    save() const override;

    void
    restore(QJsonObject const & p) override;

    QString
    caption() const override { return msCaptionName; };

    void
    setCaption(QString caption) { msCaptionName = caption; };

    QString
    name() const override { return msModelName; };

    QString
    modelName() const { return msModelName; };

    PropertyVector
    getProperty() { return mvProperty; };

    virtual
    std::shared_ptr<NodeData>
    outData(PortIndex) override { return nullptr; };

    virtual
    void
    setModelProperty(QString & , const QVariant & );

    void
    setEnable( bool ) override;

    void
    setMinimize( bool ) override;

    void
    setLockPosition( bool ) override;

    void
    setDrawEntries( bool ) override;

    void
    updateAllOutputPorts();

    virtual
    void
    setSelected( bool selected ) { mbSelected = selected; }

    bool
    isSelected( ) const { return mbSelected; }

    bool
    isSource( ) const { return mbSource; }

Q_SIGNALS:
    void
    property_changed_signal( std::shared_ptr<Property> );

    void
    enable_changed_signal( bool );

    void
    minimize_changed_signal( bool );

    void
    lock_position_changed_signal( bool );

    void
    draw_entries_changed_signal( bool );

    void
    property_structure_changed_signal( );

protected Q_SLOTS:
    virtual
    void enable_changed( bool );

    virtual
    void draw_entries_changed( bool ) {};

    virtual
    void minimize_changed( bool ) {};

    virtual
    void lock_position_changed( bool );

protected:
    PropertyVector mvProperty;
    QMap<QString, std::shared_ptr<Property>> mMapIdToProperty;
    bool mbSelected{true};

private:
    QString msCaptionName;
    QString msModelName;

    NodeStyle mOrgNodeStyle;

    bool mbSource;

    void enabled( bool );
    void minimized( bool );
    void locked_position( bool );
    void draw_entries( bool );
};

#endif // PBNODEDATAMODEL_HPP
