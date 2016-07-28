#pragma once
#include <ossia/editor/scenario/time_value.hpp>
#include <ossia/editor/value/value.hpp>
#include <ossia/editor/state/state_element.hpp>

#include <Process/State/MessageNode.hpp>
#include <Process/TimeValue.hpp>

#include <State/Expression.hpp>
#include <QStringList>
#include <memory>

#include <State/Value.hpp>
#include <iscore_plugin_ossia_export.h>
namespace RecreateOnPlay
{
struct Context;
}
namespace Scenario
{
class StateModel;
}
namespace Device
{
class DeviceList;
}
namespace OSSIA {
namespace net
{
class Address;
class Node;
class Device;
}
class Expression;
struct Message;
class State;
}  // namespace OSSIA
namespace Device
{
struct FullAddressSettings;
}
namespace State
{
struct Message;
}  // namespace iscore

namespace iscore
{
namespace convert
{
// Gets a node from an address in a device.
// Creates it if necessary.
//// Device-related functions
// OSSIA::net::Node* might be null.
ISCORE_PLUGIN_OSSIA_EXPORT OSSIA::net::Node* findNodeFromPath(
        const QStringList& path,
        OSSIA::net::Device& dev);

// OSSIA::net::Node* won't be null.
ISCORE_PLUGIN_OSSIA_EXPORT OSSIA::net::Node* getNodeFromPath(
        const QStringList& path,
        OSSIA::net::Device& dev);
ISCORE_PLUGIN_OSSIA_EXPORT OSSIA::net::Node* createNodeFromPath(
        const QStringList& path,
        OSSIA::net::Device& dev);

ISCORE_PLUGIN_OSSIA_EXPORT void createOSSIAAddress(
        const Device::FullAddressSettings& settings,
        OSSIA::net::Node& node);
ISCORE_PLUGIN_OSSIA_EXPORT void updateOSSIAAddress(
        const Device::FullAddressSettings& settings,
        OSSIA::net::Address& addr);
ISCORE_PLUGIN_OSSIA_EXPORT void removeOSSIAAddress(
        OSSIA::net::Node&); // Keeps the Node.
ISCORE_PLUGIN_OSSIA_EXPORT void updateOSSIAValue(
        const State::ValueImpl& data,
        OSSIA::Value& val);

ISCORE_PLUGIN_OSSIA_EXPORT OSSIA::Value toOSSIAValue(
        const State::Value&);

//// Other conversions
ISCORE_PLUGIN_OSSIA_EXPORT inline OSSIA::TimeValue time(const TimeValue& t)
{
    return t.isInfinite()
            ? OSSIA::Infinite
            : OSSIA::TimeValue{t.msec()};
}

ISCORE_PLUGIN_OSSIA_EXPORT void state(
        OSSIA::State& ossia_state,
        const Scenario::StateModel& iscore_state,
        const RecreateOnPlay::Context& ctx);
ISCORE_PLUGIN_OSSIA_EXPORT OSSIA::State state(
        const Scenario::StateModel& iscore_state,
        const RecreateOnPlay::Context& ctx);


ISCORE_PLUGIN_OSSIA_EXPORT optional<OSSIA::Message> message(
        const State::Message& mess,
        const Device::DeviceList&);

ISCORE_PLUGIN_OSSIA_EXPORT std::shared_ptr<OSSIA::Expression> expression(
        const State::Expression& expr,
        const Device::DeviceList&);


}
}

