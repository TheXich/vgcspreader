#ifndef SAVEDCALCWINDOW_HPP
#define SAVEDCALCWINDOW_HPP

#include <QDialog>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <vector>
#include <QString>

class SavedCalcWindow : public QDialog {
    Q_OBJECT

    private slots:
        void deleteCalc(bool clicked);
        void loadCalc(bool clicked);

    private:
        QGroupBox* group_box;

    public:
        QDialogButtonBox* bottom_buttons;

        SavedCalcWindow(QWidget* parent = nullptr, Qt::WindowFlags f = nullptr);
        void loadComboBox(const std::vector<QString>& theNames);
};

#endif // SAVEDCALCWINDOW_HPP
