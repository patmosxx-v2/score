#include <Process/ProcessFactory.hpp>
#include "SimpleProcessFactory.hpp"
#include <iscore/plugins/customfactory/StringFactoryKey.hpp>
#include "iscore_plugin_ossia_simpleprocess.hpp"
#include <iscore/plugins/customfactory/FactoryFamily.hpp>

iscore_plugin_ossia_simpleprocess::iscore_plugin_ossia_simpleprocess() :
    QObject {}
{

}

iscore_plugin_ossia_simpleprocess::~iscore_plugin_ossia_simpleprocess()
{

}


std::vector<std::unique_ptr<iscore::FactoryInterfaceBase>> iscore_plugin_ossia_simpleprocess::factories(
        const iscore::ApplicationContext&,
        const iscore::FactoryBaseKey& factoryName) const
{
    if(factoryName == ProcessFactory::staticFactoryKey())
    {
        return make_ptr_vector<iscore::FactoryInterfaceBase,
                SimpleProcessFactory>();
    }

    return {};
}

