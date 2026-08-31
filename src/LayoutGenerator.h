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
    // Parse the given xkb layout and return the 4 typing rows (number row,
    // AD/AC/AB) in the QML row format: each row is a QVariantList of
    // [keyName, label, shiftLabel, widthFactor(double)] entries, where
    // keyName is the US-position name ("`", "1".."=", "q".."p", "[", "]",
    // "\\", "a".."l", ";", "'", "z".."m", ",", ".", "/") so positional
    // committing works. The function keys framing the rows (tab, shift,
    // return, …) are fixed and added in QML; LSGT is skipped.
    // Returns {} on any failure (missing file, unparseable, too few keys).
    QVariantList generate(const QString &layoutName);
};
