// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <algorithm>
#include <core/document/Document.hpp>
#include <QQmlEngine>
#include <QQmlComponent>
#include <vector>
#include <JS/Qml/QmlObjects.hpp>
#include <Process/Dataflow/Port.hpp>

#include "JS/JSProcessMetadata.hpp"
#include "JSProcessModel.hpp"
#include <score/document/DocumentInterface.hpp>
#include <score/serialization/VisitorCommon.hpp>

#include <score/model/Identifier.hpp>

namespace JS
{
ProcessModel::ProcessModel(
    const TimeVal& duration,
    const Id<Process::ProcessModel>& id,
    QObject* parent)
    : Process::ProcessModel{duration, id,
                            Metadata<ObjectKey_k, ProcessModel>::get(), parent}
{
  setScript(
R"_(import QtQuick 2.0
import Score 1.0
Item {
  ValueInlet { id: in1 }
  ValueOutlet { id: out1 }

  function onTick(oldtime, time, position, offset) {
    out1.value = in1.value + 10 * Math.random();
  }
}
)_");
  metadata().setInstanceName(*this);
}

ProcessModel::ProcessModel(
    const ProcessModel& source,
    const Id<Process::ProcessModel>& id,
    QObject* parent)
    : Process::ProcessModel{source, id,
                            Metadata<ObjectKey_k, ProcessModel>::get(), parent}
{
  setScript(source.m_script);
  metadata().setInstanceName(*this);
}

ProcessModel::~ProcessModel()
{
}

void ProcessModel::setScript(const QString& script)
{
  qDeleteAll(m_inlets);
  m_inlets.clear();
  qDeleteAll(m_outlets);
  m_outlets.clear();
  delete m_dummyObject;
  m_dummyObject = nullptr;

  m_script = script;

  if(script.trimmed().startsWith("import"))
  {
    m_dummyComponent.setData(script.trimmed().toUtf8(), QUrl());
    const auto& errs = m_dummyComponent.errors();
    if(!errs.empty())
    {
      const auto& err = errs.first();
      qDebug() << err.line() << err.toString();
      emit scriptError(err.line(), err.toString());
    }
    else
    {
      m_dummyObject = m_dummyComponent.create();

      {
        auto cld_inlet = m_dummyObject->findChildren<Inlet*>();
        int i = 0;
        for(auto n : cld_inlet) {
          auto port = n->make(Id<Process::Port>(i++), this);
          port->setCustomData(n->objectName());
          m_inlets.push_back(port);
        }
      }

      {
        auto cld_outlet = m_dummyObject->findChildren<Outlet*>();
        int i = 0;
        for(auto n : cld_outlet) {
          auto port = n->make(Id<Process::Port>(i++), this);
          port->setCustomData(n->objectName());
          m_outlets.push_back(port);
        }
      }
      emit scriptOk();
    }
  }

  emit scriptChanged(script);
  emit inletsChanged();
  emit outletsChanged();
}
}
