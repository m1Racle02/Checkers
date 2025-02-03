#pragma once

#include <stdlib.h>

// ќпределение типа POS_T как int8_t дл€ представлени€ координат на доске
typedef int8_t POS_T;

// —труктура move_pos дл€ представлени€ хода в игре
struct move_pos {
    POS_T x, y;             //  оординаты начальной позиции фигуры (откуда перемещаетс€)
    POS_T x2, y2;           //  оординаты конечной позиции фигуры (куда перемещаетс€)
    POS_T xb = -1, yb = -1; //  оординаты побитой фигуры (если есть удар)

    //  онструктор дл€ создани€ хода без побитой фигуры
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2)
        : x(x), y(y), x2(x2), y2(y2) {
        // »нициализаци€ координат начала и конца хода
    }

    //  онструктор дл€ создани€ хода с побитой фигурой
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb) {
        // »нициализаци€ координат начала, конца и побитой фигуры
    }

    // ѕерегрузка оператора == дл€ сравнени€ двух ходов
    bool operator==(const move_pos& other) const {
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
        // ¬озвращаем true, если все соответствующие координаты равны
    }

    // ѕерегрузка оператора != дл€ сравнени€ двух ходов
    bool operator!=(const move_pos& other) const {
        return !(*this == other);
        // ¬озвращаем true, если ходы не равны (используем уже определенный оператор ==)
    }
};