#pragma once

#include <QString>
#include <QVariantList>

// Builds keyboard rows for skvirt's QML panel from the system xkb symbol
// files (/usr/share/X11/xkb/symbols/<layout>), so any fcitx keyboard-XX
// input method gets an on-screen layout matching what the physical
// keyboard would produce — without hardcoding every language.
class LayoutGenerator
{
public:
    // Parse the given xkb layout and return 3 letter rows (AD/AC/AB) in the
    // existing QML row format: each row is a QVariantList of
    // [keyName, label, shiftLabel, widthFactor(double)] entries, where
    // keyName is the US-position name ("q".."p", "[", "]", "a".."l", ";",
    // "'", "z".."m", ",", ".", "/") so positional committing works.
    // The digit row (AE) and TLDE/LSGT/BKSL are intentionally skipped.
    // Returns {} on any failure (missing file, unparseable, too few keys).
    QVariantList generate(const QString &layoutName);
};
