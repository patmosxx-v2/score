#pragma once
#include <QObject>
#include <Inspector/InspectorWidgetFactoryInterface.hpp>


class AutomationStateInspectorFactory final : public InspectorWidgetFactory
{
    public:
        AutomationStateInspectorFactory();

        InspectorWidgetBase* makeWidget(
                const QObject& sourceElement,
                iscore::Document& doc,
                QWidget* parent) override;

        const QList<QString>& key_impl() const override;
};
