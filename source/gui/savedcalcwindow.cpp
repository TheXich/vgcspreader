#include "savedcalcwindow.hpp"

#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QListWidget>
#include <QVBoxLayout>
#include <QPixmap>
#include <QIcon>
#include <QPainter>

#include "mainwindow.hpp"

static constexpr int SPRITE_ICON_W = 80;
static constexpr int SPRITE_ICON_H = 60;

SavedCalcWindow::SavedCalcWindow(QWidget* parent, Qt::WindowFlags f) : QDialog(parent, f) {
    setObjectName("SavedCalcWindow");
    setWindowTitle("Saved Calculations");
    setMinimumSize(420, 340);

    QVBoxLayout* main_layout = new QVBoxLayout;
    setLayout(main_layout);

    QVBoxLayout* layout = new QVBoxLayout;
    group_box = new QGroupBox(tr("Saved Calculations"));
    group_box->setLayout(layout);

    calc_list = new QListWidget;
    calc_list->setObjectName("calc_list");
    calc_list->setIconSize(QSize(SPRITE_ICON_W, SPRITE_ICON_H));
    calc_list->setSpacing(4);
    calc_list->setUniformItemSizes(false);
    layout->addWidget(calc_list);

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
    connect(calc_list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) {
        loadCalc(true);
    });

    layout->addWidget(bottom_buttons);
    main_layout->addWidget(group_box);
}

void SavedCalcWindow::loadList(const std::vector<CalcEntry>& entries) {
    calc_list->clear();

    for (const auto& entry : entries) {
        // Always produce a canvas of exactly (SPRITE_ICON_W x SPRITE_ICON_H) so Qt
        // never leaves uncleared pixels from the previous list item in the icon area.
        QPixmap canvas(SPRITE_ICON_W, SPRITE_ICON_H);
        canvas.fill(Qt::transparent);

        // calc.species is stored as (dex_number - 1), same indexing as species_names[]
        const int dex = entry.species + 1;
        QString sprite_path;
        if (entry.form == 0)
            sprite_path = ":/db/sprites/" + QString::number(dex) + ".png";
        else
            sprite_path = ":/db/sprites/" + QString::number(dex) + "-" + QString::number(entry.form) + ".png";

        QPixmap pix;
        pix.load(sprite_path);
        if (pix.isNull() && entry.form != 0)
            pix.load(":/db/sprites/" + QString::number(dex) + ".png");

        if (!pix.isNull()) {
            QPixmap scaled = pix.scaled(SPRITE_ICON_W, SPRITE_ICON_H, Qt::KeepAspectRatio, Qt::FastTransformation);
            QPainter painter(&canvas);
            painter.drawPixmap((SPRITE_ICON_W - scaled.width()) / 2,
                               (SPRITE_ICON_H - scaled.height()) / 2,
                               scaled);
        }

        QListWidgetItem* item = new QListWidgetItem(QIcon(canvas), entry.name);
        item->setSizeHint(QSize(0, SPRITE_ICON_H + 8));
        calc_list->addItem(item);
    }

    bool has_items = calc_list->count() > 0;
    calc_list->setVisible(has_items);
    group_box->findChild<QLabel*>("no_saves_label")->setVisible(!has_items);
    group_box->findChild<QDialogButtonBox*>("bottom_buttons")->setVisible(has_items);

    if (has_items) calc_list->setCurrentRow(0);
}

void SavedCalcWindow::deleteCalc(bool clicked) {
    int idx = calc_list->currentRow();
    if (idx < 0) return;

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
    delete calc_list->takeItem(idx);

    if (calc_list->count() == 0) close();
}

void SavedCalcWindow::loadCalc(bool clicked) {
    int idx = calc_list->currentRow();
    if (idx < 0) return;
    ((MainWindow*)parentWidget())->restoreFromSavedCalc(idx);
    close();
}
