#include "PropertyChangeCommand.hpp"
#include "PBNodeDelegateModel.hpp"

#include <QtNodes/BasicGraphicsScene>
#include <QtNodes/internal/NodeGraphicsObject.hpp>

PropertyChangeCommand::PropertyChangeCommand(QtNodes::BasicGraphicsScene *scene,
                                             QtNodes::NodeId nodeId,
                                             PBNodeDelegateModel *delegateModel,
                                             const QString &propertyId,
                                             const QVariant &oldValue,
                                             const QVariant &newValue)
    : _scene(scene)
    , _nodeId(nodeId)
    , _delegateModel(delegateModel)
    , _propertyId(propertyId)
    , _oldValue(oldValue)
    , _newValue(newValue)
{
    setText(QString("Change %1").arg(propertyId));
    DEBUG_LOG_INFO() << "[constructor] NodeId:" << nodeId 
            << "propertyId:" << propertyId 
            << "oldValue:" << oldValue 
            << "newValue:" << newValue;
}

void PropertyChangeCommand::undo()
{
    DEBUG_LOG_INFO() << "[undo] Reverting" << _propertyId 
            << "from" << _newValue << "to" << _oldValue;
    applyValue(_oldValue);
}

void PropertyChangeCommand::redo()
{
    DEBUG_LOG_INFO() << "[redo] Applying" << _propertyId 
            << "from" << _oldValue << "to" << _newValue;
    applyValue(_newValue);
}

void PropertyChangeCommand::applyValue(const QVariant &value)
{
    DEBUG_LOG_INFO() << "[applyValue] propertyId:" << _propertyId 
            << "value:" << value;
    
    if (!_delegateModel || !_scene)
    {
        DEBUG_LOG_INFO() << "[applyValue] No delegate model or scene, returning";
        return;
    }

    // Special handling for minimize property
    if (_propertyId == "minimize") {
        // Check if the node can be minimized
        if (!_delegateModel->canMinimize()) {
            DEBUG_LOG_INFO() << "[applyValue] Node cannot be minimized, ignoring";
            return;
        }
        
        auto *widget = _scene->graphModel().nodeData<QWidget*>(_nodeId, QtNodes::NodeRole::Widget);
        
        if (widget) {
            DEBUG_LOG_INFO() << "[applyValue] Minimize property, hiding/showing widget";
            // Just hide/show the widget - PBNodeGeometry will handle the size
            if (value.toBool()) {
                widget->hide();
            } else {
                widget->show();
            }
        }
    }

    // Apply the property change
    DEBUG_LOG_INFO() << "[applyValue] Calling setModelProperty";
    QString propId = _propertyId;  // Create non-const copy for setModelProperty
    _delegateModel->setModelProperty(propId, value);

    // Trigger visual update for the node
    auto *ngo = _scene->nodeGraphicsObject(_nodeId);
    if (ngo) {
        DEBUG_LOG_INFO() << "[applyValue] Updating node graphics";
        // Always recompute geometry when minimize state changes
        if (_propertyId == "minimize") {
            ngo->nodeScene()->nodeGeometry().recomputeSize(_nodeId);
            // Update connection positions after geometry change
            ngo->moveConnections();
        }
        ngo->setGeometryChanged();
        ngo->update();
    }
    
    // If this node is currently selected, emit the signal to update Property Browser
    // This ensures the UI stays in sync even when undo/redo is called
    if (ngo && ngo->isSelected()) {
        DEBUG_LOG_INFO() << "[applyValue] Node is selected, emitting property_changed_signal for UI sync";
        // The property_changed_signal will be caught by MainWindow::nodePropertyChanged
        // which updates the Property Browser UI
        auto prop = _delegateModel->getProperty();
        for (auto& p : prop) {
            if (p->getID() == _propertyId) {
                Q_EMIT _delegateModel->property_changed_signal(p);
                break;
            }
        }
    }
    else
    {
        DEBUG_LOG_INFO() << "[applyValue] Node not selected, skipping UI sync signal";
    }
}

int PropertyChangeCommand::id() const
{
    return PROPERTY_CHANGE_COMMAND_ID;
}

bool PropertyChangeCommand::mergeWith(const QUndoCommand *other)
{
    if (other->id() != id()) {
        return false;
    }

    const PropertyChangeCommand *otherCmd = static_cast<const PropertyChangeCommand *>(other);
    
    // Only merge if it's the same property of the same node
    if (otherCmd->_nodeId != _nodeId || otherCmd->_propertyId != _propertyId) {
        return false;
    }

    // Update the new value to the latest change
    _newValue = otherCmd->_newValue;
    return true;
}
