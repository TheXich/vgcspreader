/****************************************************************************
** Meta object code from reading C++ file 'defensemovewindow.hpp'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../include/gui/defensemovewindow.hpp"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'defensemovewindow.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_DefenseMoveWindow_t {
    QByteArrayData data[17];
    char stringdata0[177];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_DefenseMoveWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_DefenseMoveWindow_t qt_meta_stringdata_DefenseMoveWindow = {
    {
QT_MOC_LITERAL(0, 0, 17), // "DefenseMoveWindow"
QT_MOC_LITERAL(1, 18, 11), // "setSpecies1"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 5), // "index"
QT_MOC_LITERAL(4, 37, 8), // "setForm1"
QT_MOC_LITERAL(5, 46, 8), // "setForm2"
QT_MOC_LITERAL(6, 55, 8), // "setMove1"
QT_MOC_LITERAL(7, 64, 8), // "setMove2"
QT_MOC_LITERAL(8, 73, 16), // "setMoveCategory1"
QT_MOC_LITERAL(9, 90, 16), // "setMoveCategory2"
QT_MOC_LITERAL(10, 107, 11), // "setSpecies2"
QT_MOC_LITERAL(11, 119, 12), // "activateAtk2"
QT_MOC_LITERAL(12, 132, 5), // "state"
QT_MOC_LITERAL(13, 138, 9), // "addPreset"
QT_MOC_LITERAL(14, 148, 9), // "solveMove"
QT_MOC_LITERAL(15, 158, 6), // "preset"
QT_MOC_LITERAL(16, 165, 11) // "preset_name"

    },
    "DefenseMoveWindow\0setSpecies1\0\0index\0"
    "setForm1\0setForm2\0setMove1\0setMove2\0"
    "setMoveCategory1\0setMoveCategory2\0"
    "setSpecies2\0activateAtk2\0state\0addPreset\0"
    "solveMove\0preset\0preset_name"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DefenseMoveWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   79,    2, 0x08 /* Private */,
       4,    1,   82,    2, 0x08 /* Private */,
       5,    1,   85,    2, 0x08 /* Private */,
       6,    1,   88,    2, 0x08 /* Private */,
       7,    1,   91,    2, 0x08 /* Private */,
       8,    1,   94,    2, 0x08 /* Private */,
       9,    1,   97,    2, 0x08 /* Private */,
      10,    1,  100,    2, 0x08 /* Private */,
      11,    1,  103,    2, 0x08 /* Private */,
      13,    1,  106,    2, 0x08 /* Private */,
      14,    2,  109,    2, 0x0a /* Public */,
      14,    1,  114,    2, 0x2a /* Public | MethodCloned */,
      14,    0,  117,    2, 0x2a /* Public | MethodCloned */,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,   12,
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   15,   16,
    QMetaType::Void, QMetaType::Bool,   15,
    QMetaType::Void,

       0        // eod
};

void DefenseMoveWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<DefenseMoveWindow *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->setSpecies1((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->setForm1((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->setForm2((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->setMove1((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->setMove2((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->setMoveCategory1((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->setMoveCategory2((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 7: _t->setSpecies2((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 8: _t->activateAtk2((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 9: _t->addPreset((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 10: _t->solveMove((*reinterpret_cast< const bool(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 11: _t->solveMove((*reinterpret_cast< const bool(*)>(_a[1]))); break;
        case 12: _t->solveMove(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject DefenseMoveWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_DefenseMoveWindow.data,
    qt_meta_data_DefenseMoveWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *DefenseMoveWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DefenseMoveWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_DefenseMoveWindow.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int DefenseMoveWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 13;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
