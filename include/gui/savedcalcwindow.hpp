#ifndef SAVEDCALCWINDOW_HPP
#define SAVEDCALCWINDOW_HPP

#include <QDialog>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QListWidget>
#include <vector>
#include <QString>

class SavedCalcWindow : public QDialog {
    Q_OBJECT

    private slots:
        void deleteCalc(bool clicked);
        void loadCalc(bool clicked);

    private:
        QGroupBox* group_box;
        QListWidget* calc_list;

    public:
        struct CalcEntry {
            QString name;
            int species;
            int form;
        };

        QDialogButtonBox* bottom_buttons;

        SavedCalcWindow(QWidget* parent = nullptr, Qt::WindowFlags f = nullptr);
        void loadList(const std::vector<CalcEntry>& entries);
};

#endif // SAVEDCALCWINDOW_HPP
