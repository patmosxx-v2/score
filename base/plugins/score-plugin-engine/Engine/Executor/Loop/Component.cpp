// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <ossia/editor/scenario/time_event.hpp>
#include <ossia/editor/scenario/time_sync.hpp>
#include <ossia/network/base/device.hpp>
#include <Engine/score2OSSIA.hpp>
#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/Event/EventModel.hpp>
#include <Scenario/Document/State/StateModel.hpp>
#include <Scenario/Document/TimeSync/TimeSyncModel.hpp>
#include <algorithm>
#include <core/document/Document.hpp>
#include <score/document/DocumentInterface.hpp>
#include <ossia/dataflow/graph/graph_interface.hpp>
#include <vector>

#include "Component.hpp"
#include <ossia/editor/loop/loop.hpp>
#include <ossia/editor/scenario/time_value.hpp>
#include <ossia/editor/state/state.hpp>
#include <Engine/Executor/IntervalRawPtrComponent.hpp>
#include <Engine/Executor/EventComponent.hpp>
#include <Engine/Executor/ProcessComponent.hpp>
#include <Engine/Executor/StateComponent.hpp>
#include <Engine/Executor/TimeSyncRawPtrComponent.hpp>
#include <Scenario/Document/Interval/IntervalDurations.hpp>
#include <score/tools/IdentifierGeneration.hpp>
#include <Engine/Executor/DocumentPlugin.hpp>
#include <Engine/Executor/ExecutorContext.hpp>

namespace Process
{
class ProcessModel;
}
class QObject;
namespace ossia
{
class time_process;
} // namespace OSSIA
#include <score/model/Identifier.hpp>

namespace Loop
{
namespace RecreateOnPlay
{
Component::Component(
    ::Loop::ProcessModel& element,
    const ::Engine::Execution::Context& ctx,
    const Id<score::Component>& id,
    QObject* parent)
    : ::Engine::Execution::ProcessComponent_T<Loop::ProcessModel, ossia::loop>{
          element, ctx, id, "LoopComponent", parent}
{
  ossia::time_value main_duration(ctx.time(
      element.interval().duration.defaultDuration()));

  std::shared_ptr<ossia::loop> loop = std::make_shared<ossia::loop>(
      main_duration,
      ossia::time_interval::exec_callback{},
      [this, &element](ossia::time_event::status newStatus) {

        element.startEvent().setStatus(
            static_cast<Scenario::ExecutionStatus>(newStatus), process());
        switch (newStatus)
        {
          case ossia::time_event::status::NONE:
            break;
          case ossia::time_event::status::PENDING:
            break;
          case ossia::time_event::status::HAPPENED:
            startIntervalExecution(
                m_ossia_interval->scoreInterval().id());
            break;
          case ossia::time_event::status::DISPOSED:
            break;
          default:
            SCORE_TODO;
            break;
        }
      },
      [this, &element](ossia::time_event::status newStatus) {

        element.endEvent().setStatus(
            static_cast<Scenario::ExecutionStatus>(newStatus), process());
        switch (newStatus)
        {
          case ossia::time_event::status::NONE:
            break;
          case ossia::time_event::status::PENDING:
            break;
          case ossia::time_event::status::HAPPENED:
            stopIntervalExecution(
                m_ossia_interval->scoreInterval().id());
            break;
          case ossia::time_event::status::DISPOSED:
            break;
          default:
            SCORE_TODO;
            break;
        }
      });

  m_ossia_process = loop;
  node = loop->node;

  // TODO also states in BasEelement
  // TODO put graphical settings somewhere.
  auto& main_start_node = loop->get_start_timesync();
  auto& main_end_node = loop->get_end_timesync();
  auto main_start_event = *main_start_node.get_time_events().begin();
  auto main_end_event = *main_end_node.get_time_events().begin();

  using namespace Engine::Execution;
  m_ossia_startTimeSync = std::make_shared<TimeSyncRawPtrComponent>(element.startTimeSync(),
                                              system(), score::newId(element.startTimeSync()), this);
  m_ossia_endTimeSync = std::make_shared<TimeSyncRawPtrComponent>(element.endTimeSync(),
                                            system(), score::newId(element.endTimeSync()), this);

  m_ossia_startEvent = std::make_shared<EventComponent>(element.startEvent(),
                                        system(), score::newId(element.startEvent()), this);
  m_ossia_endEvent =std::make_shared<EventComponent>(element.endEvent(),
                                      system(), score::newId(element.endEvent()), this);

  m_ossia_startState
      = std::make_shared<StateComponent>(element.startState(), system(), score::newId(element.startState()), this);
  m_ossia_endState
      = std::make_shared<StateComponent>(element.endState(), system(), score::newId(element.endState()), this);


  m_ossia_interval = std::make_shared<IntervalRawPtrComponent>(element.interval(), system(), score::newId(element.interval()), this);

  m_ossia_startTimeSync->onSetup(&main_start_node, m_ossia_startTimeSync->makeTrigger());
  m_ossia_endTimeSync->onSetup(&main_end_node, m_ossia_endTimeSync->makeTrigger());
  m_ossia_startEvent->onSetup(main_start_event, m_ossia_startEvent->makeExpression(), (ossia::time_event::offset_behavior)element.startEvent().offsetBehavior());
  m_ossia_endEvent->onSetup(main_end_event, m_ossia_endEvent->makeExpression(), (ossia::time_event::offset_behavior)element.endEvent().offsetBehavior());
  m_ossia_startState->onSetup(main_start_event);
  m_ossia_endState->onSetup(main_end_event);
  m_ossia_interval->onSetup(m_ossia_interval, &loop->get_time_interval(), m_ossia_interval->makeDurations(), false);

  auto cable = ossia::make_edge(
                 ossia::immediate_glutton_connection{}
                 , m_ossia_interval->OSSIAInterval()->node->outputs()[0]
                 , loop->node->inputs()[0]
                 , m_ossia_interval->OSSIAInterval()->node
                 , loop->node);

  in_exec(
        [g=system().plugin.execGraph, cable] {
    g->connect(cable);
  });
}

Component::~Component()
{
}

void Component::cleanup()
{
  if(m_ossia_interval)
  {
    m_ossia_interval->cleanup(m_ossia_interval);
  }
  if(m_ossia_startState)
  {
    m_ossia_startState->cleanup();
  }
  if(m_ossia_endState)
  {
    m_ossia_endState->cleanup();
  }
  if(m_ossia_startEvent)
  {
    m_ossia_startEvent->cleanup();
  }
  if(m_ossia_endEvent)
  {
    m_ossia_endEvent->cleanup();
  }
  if(m_ossia_startTimeSync)
  {
    m_ossia_startTimeSync->cleanup();
  }
  if(m_ossia_endTimeSync)
  {
    m_ossia_endTimeSync->cleanup();
  }

  m_ossia_interval = nullptr;
  m_ossia_startState = nullptr;
  m_ossia_endState = nullptr;
  m_ossia_startEvent = nullptr;
  m_ossia_endEvent = nullptr;
  m_ossia_startTimeSync = nullptr;
  m_ossia_endTimeSync = nullptr;

  ProcessComponent::cleanup();
}

void Component::stop()
{
  ProcessComponent::stop();
  process().interval().duration.setPlayPercentage(0);
}

void Component::startIntervalExecution(const Id<Scenario::IntervalModel>&)
{
  m_ossia_interval->executionStarted();
}

void Component::stopIntervalExecution(const Id<Scenario::IntervalModel>&)
{
  m_ossia_interval->executionStopped();
}
}
}
