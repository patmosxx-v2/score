#pragma once
#include <Scenario/Document/DisplayedElements/DisplayedElementsToolPalette/DisplayedElementsToolPaletteFactory.hpp>
#include <iscore/plugins/customfactory/FactoryFamily.hpp>

class DisplayedElementsToolPaletteFactoryList final : public iscore::FactoryListInterface
{
    public:
      static const iscore::FactoryBaseKey& staticFactoryKey() {
          return DisplayedElementsToolPaletteFactory::staticFactoryKey();
      }

      iscore::FactoryBaseKey name() const final override {
          return DisplayedElementsToolPaletteFactory::staticFactoryKey();
      }

      void insert(iscore::FactoryInterfaceBase* e) final override
      {
          if(auto pf = dynamic_cast<DisplayedElementsToolPaletteFactory*>(e))
              m_list.push_back(pf);
      }

      const auto& list() const
      {
          return m_list;
      }

      auto make(
              ScenarioDocumentPresenter& pres,
              const ConstraintModel& constraint) const
      {
          auto it = find_if(m_list, [&] (auto elt) { return elt->matches(constraint); });
          return (it != m_list.end())
                  ? (*it)->make(pres, constraint)
                  : nullptr;
      }

    private:
      std::vector<DisplayedElementsToolPaletteFactory*> m_list;
};
