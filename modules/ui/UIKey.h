#ifndef _EOKAS_UI_KEY_H_
#define _EOKAS_UI_KEY_H_

namespace eokas
{
    enum class UIKey
    {
        Backspace,
        Delete,
        Left,
        Right,
        Up,
        Down,
        Home,
        End,
        Enter,
        Escape,
        A,
        C,
        X,
        V
    };

    struct UIKeyMods
    {
        bool ctrl = false;
        bool shift = false;
    };
}

#endif//_EOKAS_UI_KEY_H_
