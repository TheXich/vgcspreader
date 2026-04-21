#include "savedcalcwindow.hpp"

#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QComboBox>
#include <QVBoxLayout>

#include "mainwindow.hpp"

SavedCalcWindow::SavedCalcWindow(QWidget* parent, Qt::WindowFlags f) : QDialog(parent, f) {
    setObjectName("SavedCalcWindow");
    setWindowTitle("VGCSpreader");

    QVBoxLayout* main_layout = new QVBoxLayout;
    setLayout(main_layout);

    QVBoxLayout* layout = new QVBoxLayout;
    group_box = new QGroupBox;
    group_box->setLayout(layout);

    QComboBox* calc_combobox = new QComboBox;
    calc_combobox->setObjectName("calc_combobox");
    layout->addWidget(calc_combobox);

    QLabel* no_saves_label = new QLabel(tr("No saved calculations yet!"));
    no_saves_label->setAlignment(Qt::AlignCenter);
    no_saves_label->setObjectName("no_saves_label");
    layout->addWidget(no_saves_label);

    bottom_buttons = new QDialogButtonBox;
    bottom_buttons->setObjectName("bottom_buttons");

    QPushButton* load_button = new QPushButton(tr("Load"));
    QPushButton* cancel_button = new QPushButton(tr("Cancel"));
    QPushButton* delete_button = new QPushButton(tr("Delete"));

    bottom_buttons->addButton(delete_button, QDialogButtonBox::ButtonRole::DestructiveRole);
    bottom_buttons->addButton(load_button, QDialogButtonBox::ButtonRole::AcceptRole);
    bottom_buttons->addButton(cancel_button, QDialogButtonBox::ButtonRole::RejectRole);

    connect(bottom_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bottom_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(delete_button, SIGNAL(clicked(bool)), this, SLOT(deleteCalc(bool)));
    connect(load_button, SIGNAL(clicked(bool)), this, SLOT(loadCalc(bool)));

    layout->addWidget(bottom_buttons);
    main_layout->addWidget(group_box);
}

void SavedCalcWindow::loadComboBox(const std::vector<QString>& theNames) {
    QComboBox* combo = group_box->findChild<QComboBox*>("calc_combobox");
    combo->clear();
    for (const auto& name : theNames) combo->addItem(name);

    bool has_items = combo->count() > 0;
    combo->setVisible(has_items);
    group_box->findChild<QLabel*>("no_saves_label")->setVisible(!has_items);
    group_box->findChild<QDialogButtonBox*>("bottom_buttons")->setVisible(has_items);
}

void SavedCalcWindow::deleteCalc(bool clicked) {
    QComboBox* combo = group_box->findChild<QComboBox*>("calc_combobox");
    int idx = combo->currentIndex();

    MainWindow* mw = (MainWindow*)parentWidget();

    auto* root = mw->xml_saves.RootElement();
    if (root) {
        auto* node = root->FirstChildElement("Calculation");
        for (int i = 0; i < idx && node; i++) node = node->NextSiblingElement("Calculation");
        if (node) {
            root->DeleteChild(node);
            mw->xml_saves.SaveFile("saves.xml");
        }
    }

    mw->saved_calculations.erase(mw->saved_calculations.begin() + idx);
    combo->removeItem(idx);

    if (combo->count() == 0) close();
}

void SavedCalcWindow::loadCalc(bool clicked) {
    int idx = group_box->findChild<QComboBox*>("calc_combobox")->currentIndex();
    ((MainWindow*)parentWidget())->restoreFromSavedCalc(idx);
    close();
}
