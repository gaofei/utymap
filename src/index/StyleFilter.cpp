#include "../entities/ElementVisitor.hpp"
#include "StyleFilter.hpp"
#include "StringTable.hpp"

#include <cstdint>
#include <vector>

using namespace utymap::entities;
using namespace utymap::index;
using namespace utymap::mapcss;

// Contains operation types supported by mapcss parser.
enum OpType
{
    Exists,
    Equals,
    NotEquals
};

struct Condition
{
    uint32_t key;
    uint32_t value;
    OpType type;
};

struct Filter
{
    int zoomStart;
    int zoomEnd;
    // list of conditions which should be satisfied
    std::vector<::Condition> conditions;
};


struct FilterCollection
{
    std::vector<Filter> nodes;
    std::vector<Filter> ways;
    std::vector<Filter> areas;
};

class StyleElementVisitor : public ElementVisitor
{
public:

    StyleElementVisitor(const FilterCollection& filters, int levelOfDetails) :
        filters_(filters),
        levelOfDetails_(levelOfDetails),
        isApplicable_(false)
    {
    }

    void visitNode(const Node&)
    {
    }

    void visitWay(const Way&)
    {

    }

    void visitRelation(const Relation&)
    {
    }

    inline bool isApplicable() { return isApplicable_; }

private:
    const FilterCollection& filters_;
    int levelOfDetails_;
    bool isApplicable_;
};

class StyleFilter::StyleFilterImpl
{
public:

    StyleFilterImpl(const StyleSheet& stylesheet, StringTable& stringTable) :
        stringTable_(stringTable),
        filters_()
    {
        initFilterCollection(stylesheet);
    }

    bool isApplicable(const Element& element, int levelOfDetails) const
    {
        StyleElementVisitor visitor = { filters_, levelOfDetails };
        element.accept(visitor);
        return visitor.isApplicable();
    }

private:
    
    // convert mapcss stylesheet to index optimized representation.
    void initFilterCollection(const StyleSheet& stylesheet)
    {
        filters_.nodes.reserve(24);
        filters_.ways.reserve(24);
        filters_.areas.reserve(24);

        for (const Rule& rule : stylesheet.rules) {
            for (const Selector& selector : rule.selectors) {
                std::vector<Filter>* filtersPtr;
                if (selector.name == "node")
                    filtersPtr = &filters_.nodes;
                else if (selector.name == "way")
                    filtersPtr = &filters_.ways;
                else if (selector.name == "area")
                    filtersPtr = &filters_.areas;
                else
                    continue;

                Filter filter;
                filter.zoomStart = selector.zoom.start;
                filter.zoomEnd = selector.zoom.end;
                filter.conditions.reserve(selector.conditions.size());
                for (const utymap::mapcss::Condition& condition : selector.conditions) {
                    ::Condition c;
                    if (condition.operation == "")
                        c.type = OpType::Exists;
                    else if (condition.operation == "=")
                        c.type = OpType::Equals;
                    else if (condition.operation == "!=")
                        c.type = OpType::NotEquals;
                    else
                        continue;
                    c.key = stringTable_.getId(condition.key);
                    c.value = stringTable_.getId(condition.value);
                    filter.conditions.push_back(c);
                }
                filtersPtr->push_back(filter);
            }
        }
    }
    
    StringTable& stringTable_;
    FilterCollection filters_;
};

StyleFilter::StyleFilter(const StyleSheet& stylesheet, StringTable& stringTable) :
    pimpl_(std::unique_ptr<StyleFilter::StyleFilterImpl>(
        new StyleFilter::StyleFilterImpl(stylesheet, stringTable)))
{
}

bool utymap::index::StyleFilter::isApplicable(const Element& element, int levelOfDetails) const
{
    return pimpl_->isApplicable(element, levelOfDetails);
}
