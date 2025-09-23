#pragma once

#include <tim_enum.h>
#include <tim_flags.h>

namespace tim
{

// You can use only one of the horizontal flags at a time.
enum class align
{
    Left    = 1 << 0, ///< Aligns with the left edge.
    Right   = 1 << 1, ///< Aligns with the right edge.
    HCenter = 1 << 2, ///< Centers horizontally in the available space.
    Justify = 1 << 3, ///< Justifies the text in the available space.

    Top      = 1 << 4, ///< Aligns with the top.
    Bottom   = 1 << 5, ///< Aligns with the bottom.
    VCenter  = 1 << 6, ///< Centers vertically in the available space.
    Baseline = 1 << 7, ///< Aligns with the baseline.

    Center = VCenter | HCenter, ///< Centers in both dimensions.

    Leading = Left, ///< Synonym for \c Left.
    Trailing = Right, ///< Synonym for \c Right.

    // Masks
    HorizontalMask = Left | Right | HCenter | Justify,
    VerticalMask = Top | Bottom | VCenter | Baseline
};

using alignment = tim::flags<tim::align>;

}

TIM_DECLARE_META_ENUM(tim::align,
    TIM_ENUM_ITEM(tim::align::Left, TIM_TR("Left"_en, "Влево"_ru)),
    TIM_ENUM_ITEM(tim::align::Right, TIM_TR("Right"_en, "Вправо"_ru)),
    TIM_ENUM_ITEM(tim::align::HCenter, TIM_TR("Center horizontaly"_en, "По центру горизонтально"_ru)),
    TIM_ENUM_ITEM(tim::align::Justify, TIM_TR("Justify"_en, "По ширине"_ru)),
    TIM_ENUM_ITEM(tim::align::Top, TIM_TR("Top"_en, "Вверх"_ru)),
    TIM_ENUM_ITEM(tim::align::Bottom, TIM_TR("Bottom"_en, "Вниз"_ru)),
    TIM_ENUM_ITEM(tim::align::VCenter, TIM_TR("Center vertically"_en, "По центру вертикально"_ru)),
    TIM_ENUM_ITEM(tim::align::Baseline, TIM_TR("Baseline"_en, "По базовой линии"_ru)),
    TIM_ENUM_ITEM(tim::align::Center, TIM_TR("Center"_en, "По центру"_ru))
);
