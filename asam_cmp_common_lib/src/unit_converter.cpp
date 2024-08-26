#include <asam_cmp_common_lib/unit_converter.h>

BEGIN_NAMESPACE_ASAM_CMP_COMMON

namespace Units {
    const std::unordered_map<std::string, uint8_t> symbolToUnitId = {
        {"m", 0x02},          // Length
        {"kg", 0x03},         // Mass
        {"s", 0x01},          // Time
        {"A", 0x04},          // Electric Current
        {"K", 0x05},          // Thermodynamic Temperature
        {"mol", 0x06},        // Amount of Substance
        {"cd", 0x07},         // Luminous Intensity
        {"J", 0x0D},          // Energy, Work, Heat
        {"N", 0x0B},          // Force
        {"C", 0x0F},          // Electric Charge
        {"kat", 0x1D},        // Catalytic Activity
        {"lx", 0x19},         // Illuminance
        {"W", 0x0E},          // Power, Radiant Flux
        {"Pa", 0x0C},         // Pressure, Stress
        {"V", 0x10},          // Electric Potential Difference
        {"Wb", 0x14},         // Magnetic Flux
        {"T", 0x15},          // Magnetic Flux Density
        {"lm", 0x18},         // Luminous Flux
        {"Hz", 0x08},         // Frequency
        {"Bq", 0x1A},         // Activity (of a Radionuclide)
        {"Gy", 0x1B},         // Absorbed Dose
        {"Ohm", 0x12},        // Electric Resistance
        {"H", 0x16},          // Inductance
        {"F", 0x11},          // Capacitance
        {"sr", 0x0A},         // Solid Angle
        {"Sv", 0x1C},         // Dose Equivalent
        {"S", 0x13},          // Electric Conductance
        {"rad", 0x09},        // Plane Angle
        {"°C", 0x17},         // Celsius Temperature
        {"m/s", 0x1E},        // Speed / Velocity
        {"m/s²", 0x1F},       // Acceleration
        {"m/s³", 0x20},       // Jerk
        {"m/s⁴", 0x21},       // Jounce
        {"rad/s", 0x22},      // Angular Velocity
        {"rad/s²", 0x23},     // Angular Acceleration
        {"Hz/s", 0x24},       // Frequency Drift
        {"m³/s", 0x25},       // Volumetric Flow Rate
        {"m²", 0x26},         // Area
        {"m³", 0x27},         // Volume
        {"N s", 0x28},        // Momentum
        {"N m s", 0x29},      // Angular Momentum
        {"N m", 0x2A},        // Moment of Force (Torque)
        {"kg/m²", 0x2B},      // Area Density
        {"kg/m³", 0x2C},      // Mass Density
        {"m³/kg", 0x2D},      // Specific Volume
        {"J s", 0x2E},        // Action
        {"J/kg", 0x2F},       // Specific Energy
        {"J/m³", 0x30},       // Energy Density
        {"N/m", 0x31},        // Surface Tension
        {"W/m²", 0x32},       // Heat Flux Density
        {"m²/s", 0x33},       // Kinematic Viscosity
        {"Pa s", 0x34},       // Dynamic Viscosity
        {"kg/s", 0x35},       // Mass Flow Rate
        {"W/(sr m²)", 0x36},  // Radiance
        {"Gy/s", 0x37},       // Absorbed Dose Rate
        {"m/m³", 0x38},       // Fuel Efficiency
        {"W/m³", 0x39},       // Power Density
        {"J/(m² s)", 0x3A},   // Surface Power Density
        {"kg m²", 0x3B},      // Moment of Inertia
        {"W/sr", 0x3C},       // Radiant Intensity
        {"mol/m³", 0x3D},     // Amount of Substance Concentration
        {"m³/mol", 0x3E},     // Molar Volume
        {"J/(mol K)", 0x3F},  // Molar Heat Capacity
        {"J/mol", 0x40},      // Molar Enthalpy (Energy)
        {"mol/kg", 0x41},     // Molality
        {"kg/mol", 0x42},     // Molar Mass
        {"C/m", 0x43},        // Linear Charge Density
        {"C/m²", 0x44},       // Electric Flux Density
        {"C/m³", 0x45},       // Electric Charge Density
        {"A/m²", 0x46},       // Electric Current Density
        {"S/m", 0x47},        // Electrical Conductivity
        {"F/m", 0x48},        // Permittivity
        {"H/m", 0x49},        // Magnetic Permeability
        {"V/m", 0x4A},        // Electric Field Strength
        {"A/m", 0x4B},        // Magnetic Field Strength
        {"C/kg", 0x4C},       // Radiation Exposure
        {"J/T", 0x4D},        // Magnetic Moment
        {"lm s", 0x4E},       // Luminous Energy
        {"lx s", 0x4F},       // Luminous Exposure
        {"cd/m²", 0x50},      // Luminance
        {"lm/W", 0x51},       // Luminous Efficacy
        {"J/K", 0x52},        // Heat Capacity
        {"J/(K kg)", 0x53},   // Specific Heat Capacity
        {"W/(m K)", 0x54}     // Thermal Conductivity
    };

    const std::array<const char*, 0x55> unitIdToSymbol = []()
    {
        std::array<const char*, 0x55> arr = {};
        for (const auto& [key, value] : symbolToUnitId)
        {
            arr[value] = key.c_str();
        }
        return arr;
    }();

    uint8_t getIdBySymbol(const std::string& symbol) noexcept
    {
        auto it = symbolToUnitId.find(symbol);
        if (it != symbolToUnitId.end())
        {
            return it->second;
        }
        return 0;
    }

    std::string getSymbolById(uint8_t id) noexcept
    {
        if (id < unitIdToSymbol.size() && unitIdToSymbol[id] != nullptr)
        {
            return unitIdToSymbol[id];
        }
        return "";
    }
}

END_NAMESPACE_ASAM_CMP_COMMON
