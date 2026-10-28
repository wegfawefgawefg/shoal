#include "rules/units.hpp"

namespace shoal {

bool UnitTypeFromShortName(std::string_view name, UnitType* out) {
    for (int i = 0; i < kUnitTypeCount; i++) {
        if (kUnitSpecs[i].short_name == name) {
            *out = static_cast<UnitType>(i);
            return true;
        }
    }
    return false;
}

} // namespace shoal
