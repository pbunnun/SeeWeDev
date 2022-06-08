#ifndef PBFLOWSCENE_HPP
#define PBFLOWSCENE_HPP

#pragma once

#include "CVDevLibrary.hpp"

#include "nodes/internal/Export.hpp"
#include <nodes/FlowScene>

class CVDEVSHAREDLIB_EXPORT PBFlowScene : public QtNodes::FlowScene
{
    Q_OBJECT
public:
    explicit PBFlowScene(QWidget *parent = nullptr);

    bool save(QString & sFilename) const;

    bool load(QString & sFilename);
};

#endif // PBFLOWSCENE_HPP
