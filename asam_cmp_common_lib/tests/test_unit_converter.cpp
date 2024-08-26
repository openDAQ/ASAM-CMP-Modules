#include <gmock/gmock.h>
#include <asam_cmp_common_lib/unit_converter.h>

using namespace daq::asam_cmp_common_lib::Units;

TEST(UnitConverterTest, UnitConverterTest)
{
    for (uint8_t i = 0; i < std::numeric_limits<uint8_t>::max(); ++i) {
        auto sym = getSymbolById(i);
        ASSERT_EQ((sym.empty() ? (uint8_t)0 : i), getIdBySymbol(sym));
    }
}
