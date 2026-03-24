/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.hpp'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../include/gui/mainwindow.hpp"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MainWindow_t {
    QByteArrayData data[23];
    char stringdata0[299];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
QT_MOC_LITERAL(0, 0, 10), // "MainWindow"
QT_MOC_LITERAL(1, 11, 26), // "setDefendingPokemonSpecies"
QT_MOC_LITERAL(2, 38, 0), // ""
QT_MOC_LITERAL(3, 39, 5), // "index"
QT_MOC_LITERAL(4, 45, 23), // "setDefendingPokemonForm"
QT_MOC_LITERAL(5, 69, 18), // "setButtonClickable"
QT_MOC_LITERAL(6, 88, 3), // "row"
QT_MOC_LITERAL(7, 92, 6), // "column"
QT_MOC_LITERAL(8, 99, 9), // "eraseMove"
QT_MOC_LITERAL(9, 109, 7), // "checked"
QT_MOC_LITERAL(10, 117, 9), // "addPreset"
QT_MOC_LITERAL(11, 127, 16), // "solveMoveDefense"
QT_MOC_LITERAL(12, 144, 15), // "solveMoveAttack"
QT_MOC_LITERAL(13, 160, 14), // "openMoveWindow"
QT_MOC_LITERAL(14, 175, 18), // "openMoveWindowEdit"
QT_MOC_LITERAL(15, 194, 14), // "moveTabChanged"
QT_MOC_LITERAL(16, 209, 5), // "clear"
QT_MOC_LITERAL(17, 215, 16), // "QAbstractButton*"
QT_MOC_LITERAL(18, 232, 9), // "theButton"
QT_MOC_LITERAL(19, 242, 9), // "calculate"
QT_MOC_LITERAL(20, 252, 14), // "calculateStart"
QT_MOC_LITERAL(21, 267, 13), // "calculateStop"
QT_MOC_LITERAL(22, 281, 17) // "calculateFinished"

    },
    "MainWindow\0setDefendingPokemonSpecies\0"
    "\0index\0setDefendingPokemonForm\0"
    "setButtonClickable\0row\0column\0eraseMove\0"
    "checked\0addPreset\0solveMoveDefense\0"
    "solveMoveAttack\0openMoveWindow\0"
    "openMoveWindowEdit\0moveTabChanged\0"
    "clear\0QAbstractButton*\0theButton\0"
    "calculate\0calculateStart\0calculateStop\0"
    "calculateFinished"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   89,    2, 0x08 /* Private */,
       4,    1,   92,    2, 0x08 /* Private */,
       5,    2,   95,    2, 0x08 /* Private */,
       8,    1,  100,    2, 0x08 /* Private */,
      10,    1,  103,    2, 0x08 /* Private */,
      11,    0,  106,    2, 0x08 /* Private */,
      12,    0,  107,    2, 0x08 /* Private */,
      13,    1,  108,    2, 0x08 /* Private */,
      14,    1,  111,    2, 0x08 /* Private */,
      15,    1,  114,    2, 0x08 /* Private */,
      16,    1,  117,    2, 0x08 /* Private */,
      19,    0,  120,    2, 0x08 /* Private */,
      20,    0,  121,    2, 0x08 /* Private */,
      21,    0,  122,    2, 0x08 /* Private */,
      22,    0,  123,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    6,    7,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, 0x80000000 | 17,   18,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->setDefendingPokemonSpecies((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->setDefendingPokemonForm((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->setButtonClickable((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 3: _t->eraseMove((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 4: _t->addPreset((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 5: _t->solveMoveDefense(); break;
        case 6: _t->solveMoveAttack(); break;
        case 7: _t->openMoveWindow((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 8: _t->openMoveWindowEdit((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 9: _t->moveTabChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 10: _t->clear((*reinterpret_cast< QAbstractButton*(*)>(_a[1]))); break;
        case 11: _t->calculate(); break;
        case 12: _t->calculateStart(); break;
        case 13: _t->calculateStop(); break;
        case 14: _t->calculateFinished(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 10:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QAbstractButton* >(); break;
            }
            break;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.data,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
