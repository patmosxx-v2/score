#include <API/Headers/Editor/TimeEvent.h>
#include <API/Headers/Editor/TimeNode.h>
#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <Scenario/Document/Constraint/ConstraintModel.hpp>
#include <core/document/Document.hpp>
#include <iscore/document/DocumentInterface.hpp>
#include <OSSIA/iscore2OSSIA.hpp>
#include <algorithm>
#include <vector>

#include "Editor/Loop.h"
#include "Editor/TimeValue.h"
#include "Editor/State.h"
#include "Loop/LoopProcessModel.hpp"
#include "LoopElement.hpp"
#include <OSSIA/Executor/ConstraintElement.hpp>
#include <OSSIA/Executor/EventElement.hpp>
#include <OSSIA/Executor/ProcessElement.hpp>
#include <OSSIA/Executor/TimeNodeElement.hpp>
#include <OSSIA/Executor/StateElement.hpp>
#include <Scenario/Document/Constraint/ConstraintDurations.hpp>

#include <OSSIA/Executor/ExecutorContext.hpp>
#include <OSSIA/Executor/DocumentPlugin.hpp>

namespace Process { class ProcessModel; }
class QObject;
namespace OSSIA {
class StateElement;
class TimeProcess;
}  // namespace OSSIA
#include <iscore/tools/SettableIdentifier.hpp>

RecreateOnPlay::LoopElement::LoopElement(
        RecreateOnPlay::ConstraintElement& parentConstraint,
        Loop::ProcessModel& element,
        const Context& ctx,
        const Id<iscore::Component>& id,
        QObject* parent):
    ProcessComponent{parentConstraint, element, id, "LoopComponent", parent},
    m_ctx{ctx},
    m_iscore_loop{element}
{
    OSSIA::TimeValue main_duration(iscore::convert::time(element.constraint().duration.defaultDuration()));

    m_ossia_loop = OSSIA::Loop::create(main_duration,
                                       [] (const OSSIA::TimeValue&, const OSSIA::TimeValue&, std::shared_ptr<OSSIA::StateElement>) {},
    [] (OSSIA::TimeEvent::Status) {},
    [] (OSSIA::TimeEvent::Status) {}
    );

    // TODO also states in BasEelement
    // TODO put graphical settings somewhere.

    auto startTN = m_ossia_loop->getPatternStartTimeNode();
    auto endTN = m_ossia_loop->getPatternEndTimeNode();
    auto startEV = *startTN->timeEvents().begin();
    auto endEV = *endTN->timeEvents().begin();


    m_ossia_startTimeNode = new TimeNodeElement{startTN, element.startTimeNode(),  m_ctx.devices, this};
    m_ossia_endTimeNode = new TimeNodeElement{endTN, element.endTimeNode(), m_ctx.devices, this};

    m_ossia_startEvent = new EventElement{startEV, element.startEvent(), m_ctx.devices, this};
    m_ossia_endEvent = new EventElement{endEV, element.endEvent(), m_ctx.devices, this};

    m_ossia_startState = new StateElement{
            element.startState(),
            iscore::convert::state(element.startState(), m_ctx.devices),
            m_ctx.devices,
            this};
    m_ossia_endState = new StateElement{
            element.endState(),
            iscore::convert::state(element.endState(), m_ctx.devices),
            m_ctx.devices,
            this};

    startEV->getState()->stateElements().push_back(m_ossia_startState->OSSIAState());
    endEV->getState()->stateElements().push_back(m_ossia_endState->OSSIAState());
    m_ossia_constraint = new ConstraintElement{m_ossia_loop->getPatternTimeConstraint(), element.constraint(), m_ctx, this};
}

RecreateOnPlay::LoopElement::~LoopElement()
{
}

std::shared_ptr<OSSIA::TimeProcess> RecreateOnPlay::LoopElement::OSSIAProcess() const
{ return m_ossia_loop; }

std::shared_ptr<OSSIA::Loop> RecreateOnPlay::LoopElement::scenario() const
{ return m_ossia_loop; }

Process::ProcessModel&RecreateOnPlay::LoopElement::iscoreProcess() const
{ return m_iscore_loop; }

void RecreateOnPlay::LoopElement::stop()
{
    ProcessComponent::stop();
    m_iscore_loop.constraint().duration.setPlayPercentage(0);
}

void RecreateOnPlay::LoopElement::startConstraintExecution(const Id<ConstraintModel>&)
{
    m_ossia_constraint->executionStarted();
}

void RecreateOnPlay::LoopElement::stopConstraintExecution(const Id<ConstraintModel>&)
{
    m_ossia_constraint->executionStopped();
}

const iscore::Component::Key&RecreateOnPlay::LoopElement::key() const
{
    static iscore::Component::Key k("OSSIALoopElement");
    return k;
}

namespace RecreateOnPlay
{
LoopComponentFactory::~LoopComponentFactory()
{

}

ProcessComponent* LoopComponentFactory::make(
        ConstraintElement& cst,
        Process::ProcessModel& proc,
        const Context& ctx,
        const Id<iscore::Component>& id,
        QObject* parent) const
{

    return new LoopElement{
                cst,
                static_cast<Loop::ProcessModel&>(proc),
                ctx, id, parent};

}

const LoopComponentFactory::factory_key_type&
LoopComponentFactory::key_impl() const
{
    static LoopComponentFactory::factory_key_type k("OSSIALoopElement");
    return k;
}

bool LoopComponentFactory::matches(
        Process::ProcessModel& proc,
        const DocumentPlugin&,
        const iscore::DocumentContext&) const
{
    return dynamic_cast<Loop::ProcessModel*>(&proc);
}
}
