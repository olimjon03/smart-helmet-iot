#pragma once

// Keep private settings in secrets.h (excluded from Git).
#if __has_include("secrets.h")
#include "secrets.h"
#endif

// The public firmware builds with SMS disabled until configured locally.
#ifndef EMERGENCY_PHONE_NUMBER
#define EMERGENCY_PHONE_NUMBER ""
#endif
