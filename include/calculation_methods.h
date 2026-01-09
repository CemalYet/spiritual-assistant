#pragma once
#include <cstdint>

/*
 * This header contains only the method ID and name mapping.
 * The full MethodSpec with Adhan-specific parameters is in prayer_calculator.cpp
 */

struct CalculationMethodInfo
{
    int id;
    const char *name;
};

// All supported calculation methods
// Must stay in sync with kMethodSpecs in prayer_calculator.cpp
inline constexpr CalculationMethodInfo CALCULATION_METHODS[] = {
    {1, "Karachi"},
    {2, "ISNA"},
    {3, "MWL"},
    {4, "Umm al-Qura"},
    {5, "Egyptian"},
    {6, "Gulf"},
    {7, "Tehran"},
    {8, "Dubai"},
    {9, "Kuwait"},
    {10, "Qatar"},
    {11, "Singapore"},
    {12, "France UOIF"},
    {13, "Turkey Diyanet"},
    {14, "Russia"},
    {15, "Moonsighting"}};

inline constexpr int CALCULATION_METHOD_COUNT = sizeof(CALCULATION_METHODS) / sizeof(CALCULATION_METHODS[0]);

// Find method by ID, returns nullptr if not found
inline const CalculationMethodInfo *findCalculationMethod(int id)
{
    for (const auto &method : CALCULATION_METHODS)
    {
        if (method.id == id)
            return &method;
    }
    return nullptr;
}

// Get method name by ID, returns nullptr if not found
inline const char *getCalculationMethodName(int id)
{
    const auto *method = findCalculationMethod(id);
    return method ? method->name : nullptr;
}
