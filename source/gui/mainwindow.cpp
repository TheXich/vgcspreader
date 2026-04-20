#include "mainwindow.hpp"

#include <algorithm>
#include <QFile>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTextStream>
#include <QSpacerItem>
#include <QHeaderView>
#include <QFormLayout>
#include <QDebug>
#include <QThread>
#include <QProgressBar>
#include <QTabWidget>
#include <qtconcurrentrun.h>

#include "pokemon.hpp"

MainWindow::MainWindow() {
    setObjectName("MainWindow");
    setWindowTitle("VGCSpreader");
    selected_pokemon = nullptr;

    LoadPresetsFromFile();

    createDefendingPokemonGroupBox();
    createMovesGroupBox();

    QVBoxLayout* main_layout = new QVBoxLayout;
    main_layout->addWidget(defending_groupbox);
    main_layout->addWidget(moves_groupbox);

    setLayout(main_layout);

    QFile moves_input(":/db/moves.txt");
    moves_input.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream in_moves(&moves_input);
    while (!in_moves.atEnd()) {
        QString line = in_moves.readLine();
        moves_names.push_back(line);
    }

    defense_move_window = new DefenseMoveWindow(this);
    defense_move_window->setObjectName("DefenseMoveWindow");
    defense_move_window->setWindowTitle("VGCSpreader");

    attack_move_window = new AttackMoveWindow(this);
    attack_move_window->setObjectName("AttackMoveWindow");
    attack_move_window->setWindowTitle("VGCSpreader");

    result_window = new ResultWindow(this);
    result_window->setObjectName("ResultWindow");
    result_window->setWindowTitle("VGCSpreader");

    alert_window = new AlertWindow(this);
    alert_window->setObjectName("AlertWindow");
    alert_window->setWindowTitle("VGCSpreader");

    preset_window = new PresetWindow(this);
    preset_window->setObjectName("PresetWindow");
    preset_window->setWindowTitle("VGCSpreader");

    //bottom buttons
    bottom_buttons = new QDialogButtonBox;
    QPushButton* calculate_button = new QPushButton(tr("Calculate"));
    calculate_button->setObjectName("calculate_button");
    QPushButton* clear_button = new QPushButton(tr("Clear"));
    clear_button->setObjectName("clear_button");
    QPushButton* stop_button = new QPushButton(tr("Stop"));
    stop_button->setObjectName("stop_button");

    bottom_buttons->addButton(clear_button, QDialogButtonBox::ButtonRole::ResetRole);
    bottom_buttons->addButton(calculate_button, QDialogButtonBox::ButtonRole::AcceptRole);
    bottom_buttons->addButton(stop_button, QDialogButtonBox::ButtonRole::ApplyRole);
    stop_button->setVisible(false);

    main_layout->addWidget(bottom_buttons, Qt::AlignRight);

    //SIGNAL
    connect(defense_move_window->bottom_button_box, SIGNAL(accepted()), this, SLOT(solveMoveDefense()));
    connect(attack_move_window->bottom_button_box, SIGNAL(accepted()), this, SLOT(solveMoveAttack()));
    connect(alert_window->bottom_buttons, SIGNAL(accepted()), this, SLOT(calculate()));
    connect(bottom_buttons, SIGNAL(clicked(QAbstractButton*)), this, SLOT(clear(QAbstractButton*)));
    connect(bottom_buttons, SIGNAL(accepted()), this, SLOT(calculateStart()));
    connect(&this->future_watcher, SIGNAL (finished()), this, SLOT (calculateFinished()));

    layout()->setSizeConstraint( QLayout::SetFixedSize );
}

/*static*/ void MainWindow::populateSortedComboBox(QComboBox* combo, const std::vector<QString>& names) {
    std::vector<std::pair<QString, int>> sorted;
    sorted.reserve(names.size());
    for (int i = 0; i < (int)names.size(); i++)
        sorted.push_back({names[i], i});
    std::sort(sorted.begin(), sorted.end(), [](const std::pair<QString,int>& a, const std::pair<QString,int>& b){
        return a.first.toLower() < b.first.toLower();
    });
    for (const auto& p : sorted) {
        combo->addItem(p.first);
        combo->setItemData(combo->count() - 1, p.second, Qt::UserRole);
    }
}

/*static*/ void MainWindow::setComboByOriginalIdx(QComboBox* combo, int originalIdx) {
    for (int i = 0; i < combo->count(); i++) {
        if (combo->itemData(i, Qt::UserRole).toInt() == originalIdx) {
            combo->setCurrentIndex(i);
            return;
        }
    }
}

void MainWindow::moveTabChanged(int index) {
    moves_groupbox->findChild<QPushButton*>("moves_edit_button")->setEnabled(false);
    moves_groupbox->findChild<QPushButton*>("moves_delete_button")->setEnabled(false);

    moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->setCurrentItem(nullptr);
    moves_groupbox->findChild<QTableWidget*>("moves_attack_view")->setCurrentItem(nullptr);

    if( index == 0 ) moves_groupbox->findChild<QPushButton*>("moves_preset_button")->setEnabled(true);
    else moves_groupbox->findChild<QPushButton*>("moves_preset_button")->setEnabled(false);

}

void MainWindow::setButtonClickable(int row, int column) {
    moves_groupbox->findChild<QPushButton*>("moves_edit_button")->setEnabled(true);
    moves_groupbox->findChild<QPushButton*>("moves_delete_button")->setEnabled(true);
}

void MainWindow::setDefendingPokemonSpecies(int index) {
    int orig = defending_groupbox->findChild<QComboBox*>("defending_species_combobox")->currentData(Qt::UserRole).toInt();
    Pokemon selected_pokemon(orig + 1);

    //setting correct sprite
    QPixmap sprite_pixmap;
    QString sprite_path = ":/db/sprites/" + QString::number(selected_pokemon.getPokedexNumber()) + ".png";
    sprite_pixmap.load(sprite_path);
    const int SPRITE_SCALE_FACTOR = 2;
    sprite_pixmap = sprite_pixmap.scaled(sprite_pixmap.width() * SPRITE_SCALE_FACTOR, sprite_pixmap.height() * SPRITE_SCALE_FACTOR);

    QLabel* sprite = defending_groupbox->findChild<QLabel*>("defending_sprite");
    sprite->setPixmap(sprite_pixmap);

    //setting ability
    setComboByOriginalIdx(defending_groupbox->findChild<QComboBox*>("defending_abilities_combobox"), selected_pokemon.getPossibleAbilities()[0][0]);

    //setting correct types
    setComboByOriginalIdx(defending_groupbox->findChild<QComboBox*>("defending_type1_combobox"), selected_pokemon.getTypes()[0][0]);
    setComboByOriginalIdx(defending_groupbox->findChild<QComboBox*>("defending_type2_combobox"), selected_pokemon.getTypes()[0][1]);

    if( selected_pokemon.getTypes()[0][0] == selected_pokemon.getTypes()[0][1] ) defending_groupbox->findChild<QComboBox*>("defending_type2_combobox")->setVisible(false);
    else defending_groupbox->findChild<QComboBox*>("defending_type2_combobox")->setVisible(true);

    //setting correct form
    {
        QComboBox* forms_combo = defending_groupbox->findChild<QComboBox*>("defending_forms_combobox");
        int dex = orig + 1;
        populateFormCombo(forms_combo, dex, selected_pokemon.getFormesNumber());
        forms_combo->setCurrentIndex(0);
        // hide if only "Base" remains (all alt forms were G-Max)
        forms_combo->setVisible(forms_combo->count() > 1);
    }
}

void MainWindow::setDefendingPokemonForm(int index) {
    int orig = defending_groupbox->findChild<QComboBox*>("defending_species_combobox")->currentData(Qt::UserRole).toInt();
    Pokemon selected_pokemon(orig + 1);

    //setting correct sprite
    QPixmap sprite_pixmap;
    QString sprite_path;
    bool load_result;

    int form_idx = defending_groupbox->findChild<QComboBox*>("defending_forms_combobox")->currentData(Qt::UserRole).toInt();
    if( form_idx == 0 ) sprite_path = ":/db/sprites/" + QString::number(selected_pokemon.getPokedexNumber()) + ".png";
    else sprite_path = ":/db/sprites/" + QString::number(selected_pokemon.getPokedexNumber()) + "-" + QString::number(form_idx) + ".png";

    sprite_pixmap.load(sprite_path);
    const int SPRITE_SCALE_FACTOR = 2;
    sprite_pixmap = sprite_pixmap.scaled(sprite_pixmap.width() * SPRITE_SCALE_FACTOR, sprite_pixmap.height() * SPRITE_SCALE_FACTOR);

    QLabel* sprite = defending_groupbox->findChild<QLabel*>("defending_sprite");
    sprite->setPixmap(sprite_pixmap);

    //setting ability
    setComboByOriginalIdx(defending_groupbox->findChild<QComboBox*>("defending_abilities_combobox"), selected_pokemon.getPossibleAbilities()[form_idx][0]);

    //setting correct types
    setComboByOriginalIdx(defending_groupbox->findChild<QComboBox*>("defending_type1_combobox"), selected_pokemon.getTypes()[form_idx][0]);
    setComboByOriginalIdx(defending_groupbox->findChild<QComboBox*>("defending_type2_combobox"), selected_pokemon.getTypes()[form_idx][1]);

    if( selected_pokemon.getTypes()[form_idx][0] == selected_pokemon.getTypes()[form_idx][1] ) defending_groupbox->findChild<QComboBox*>("defending_type2_combobox")->setVisible(false);
    else defending_groupbox->findChild<QComboBox*>("defending_type2_combobox")->setVisible(true);
}

void MainWindow::eraseMove(bool checked) {
    if( moves_groupbox->findChild<QTabWidget*>("moves_tabs")->currentIndex() == 0 ) {
        turns_def.erase(turns_def.begin() +  moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->currentRow());
        modifiers_def.erase(modifiers_def.begin() + moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->currentRow());

        moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->removeRow(moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->currentRow());

        if( turns_def.empty() ) {
            moves_groupbox->findChild<QPushButton*>("moves_edit_button")->setEnabled(false);
            moves_groupbox->findChild<QPushButton*>("moves_delete_button")->setEnabled(false);
        }
    }

    else {
        turns_atk.erase(turns_atk.begin() +  moves_groupbox->findChild<QTableWidget*>("moves_attack_view")->currentRow());
        modifiers_atk.erase(modifiers_atk.begin() + moves_groupbox->findChild<QTableWidget*>("moves_attack_view")->currentRow());
        defending_pokemons_in_attack.erase(defending_pokemons_in_attack.begin() + moves_groupbox->findChild<QTableWidget*>("moves_attack_view")->currentRow());

        moves_groupbox->findChild<QTableWidget*>("moves_attack_view")->removeRow(moves_groupbox->findChild<QTableWidget*>("moves_attack_view")->currentRow());

        if( turns_atk.empty() ) {
            moves_groupbox->findChild<QPushButton*>("moves_edit_button")->setEnabled(false);
            moves_groupbox->findChild<QPushButton*>("moves_delete_button")->setEnabled(false);
        }
    }
}

void MainWindow::createDefendingPokemonGroupBox() {
    defending_groupbox = new QGroupBox(tr("Pokemon:"));

    //the main horizontal layout for this window
    QHBoxLayout* defending_layout = new QHBoxLayout;

    //the groupbox in which we encapsulate this window
    defending_groupbox->setLayout(defending_layout);

    //---SPECIES & TYPES---//

    //---species---//
    //the main layout for this entire section
    QVBoxLayout* species_types_layout = new QVBoxLayout;
    species_types_layout->setSpacing(1);
    species_types_layout->setAlignment(Qt::AlignVCenter);

    //---sprite---
    QHBoxLayout* sprite_layout = new QHBoxLayout;

    QLabel* sprite = new QLabel;
    sprite->setObjectName("defending_sprite");
    sprite->setAlignment(Qt::AlignCenter);
    sprite_layout->addWidget(sprite);

    species_types_layout->addLayout(sprite_layout);

    //some spacing
    species_types_layout->insertSpacing(1, 10);

    //---species---//
    //the layout just for the species
    QHBoxLayout* species_layout = new QHBoxLayout;
    species_types_layout->addLayout(species_layout);

    //the combobox for all the species
    QComboBox* species = new QComboBox;
    species->setObjectName("defending_species_combobox");

    //loading species names
    QFile species_input(":/db/species.txt");
    species_input.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream in_species(&species_input);
    bool is_egg = true; //WE DO NOT WANT TO ADD THE EGG
    while (!in_species.atEnd()) {
        QString line = in_species.readLine();
        if( !is_egg ) species_names.push_back(line);
        else is_egg = false;
    }

    populateSortedComboBox(species, species_names);

    //some resizing
    int species_width = species->minimumSizeHint().width();
    species->setMaximumWidth(species_width);
    //adding it to the layout
    species_layout->addWidget(species);

    //---forms---//
    //the layout just for the forms
    QHBoxLayout* forms_layout = new QHBoxLayout;
    species_types_layout->addLayout(forms_layout);

    //the combobox for all the species
    QComboBox* forms = new QComboBox;
    forms->setObjectName("defending_forms_combobox");

    //some resizing
    int forms_width = forms->minimumSizeHint().width();
    forms->setMaximumWidth(forms_width);
    //adding it to the layout
    forms_layout->addWidget(forms);

    //---types---//
    //the layout for the types
    QHBoxLayout* types_layout = new QHBoxLayout;

    //the comboboxes for the types
    QComboBox* types1 = new QComboBox;
    QComboBox* types2 = new QComboBox;
    types1->setObjectName("defending_type1_combobox");
    types2->setObjectName("defending_type2_combobox");

    //loading type names
    QFile types_input(":/db/types.txt");
    types_input.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream in_types(&types_input);
    while (!in_types.atEnd()) {
        QString line = in_types.readLine();
        types_names.push_back(line);
    }

    populateSortedComboBox(types1, types_names);
    populateSortedComboBox(types2, types_names);

    //resizing them
    int types_width = species->minimumSizeHint().width();
    types1->setMaximumWidth(types_width);
    types2->setMaximumWidth(types_width);

    //adding them to the layout
    types_layout->addWidget(types1);
    types_layout->addWidget(types2);
    species_types_layout->addLayout(types_layout);

    //some spacing
    species_types_layout->insertSpacing(5, 20);

    //adding everything to the window
    defending_layout->addLayout(species_types_layout);

    //and then some spacing
    defending_layout->insertSpacing(1, 35);

    //---MAIN FORM---//
    QVBoxLayout* main_form_layout = new QVBoxLayout;
    main_form_layout->setAlignment(Qt::AlignVCenter);

    //---information section---///
    QFormLayout* form_layout = new QFormLayout;
    main_form_layout->addLayout(form_layout);

    //NATURE
    //the combobox for the nature
    QComboBox* natures = new QComboBox;
    natures->setObjectName("defending_nature_combobox");

    //loading nature names
    QFile natures_input(":/db/natures.txt");
    natures_input.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream in_natures(&natures_input);
    while (!in_natures.atEnd()) {
        QString line = in_natures.readLine();
        natures_names.push_back(line);
    }

    populateSortedComboBox(natures, natures_names);
    natures->addItem(tr("Auto"));
    natures->setItemData(natures->count() - 1, (int)natures_names.size(), Qt::UserRole);
    setComboByOriginalIdx(natures, 0); // Default: Hardy (neutral)

    form_layout->addRow(tr("Nature:"), natures);

    //ABILITY
    //the combobox for the ability
    QComboBox* abilities = new QComboBox;
    abilities->setObjectName("defending_abilities_combobox");

    //loading abilities names
    QFile abilities_input(":/db/abilities.txt");
    abilities_input.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream in_abilities(&abilities_input);
    while (!in_abilities.atEnd()) {
        QString line = in_abilities.readLine();
        abilities_names.push_back(line);
    }

    populateSortedComboBox(abilities, abilities_names);

    //getting the abilities width (to be used later)
    int abilities_width = abilities->minimumSizeHint().width();

    form_layout->addRow(tr("Ability:"), abilities);

    //ITEM
    //the combobox for the item
    QComboBox* items = new QComboBox;
    items->setObjectName("defending_items_combobox");

    //loading item names
    QFile items_input(":/db/items.txt");
    items_input.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream in_items(&items_input);
    while (!in_items.atEnd()) {
        QString line = in_items.readLine();
        items_names.push_back(line);
    }

    populateSortedComboBox(items, items_names);
    setComboByOriginalIdx(items, 0); // Default: None

    form_layout->addRow(tr("Item:"), items);

    natures->setMaximumWidth(abilities_width);
    items->setMaximumWidth(abilities_width);
    abilities->setMaximumWidth(abilities_width);

    //adding everything to the layout
    defending_layout->addLayout(main_form_layout);

    defending_layout->addSpacing(20);

    //---iv modifiers---//
    QGroupBox* iv_groupbox = new QGroupBox(tr("IV:"));

    QFormLayout* modifiers_layout = new QFormLayout;
    iv_groupbox->setLayout(modifiers_layout);

    //hp iv
    QSpinBox* hpiv = new QSpinBox;
    hpiv->setObjectName("defending_hpiv_spinbox");
    hpiv->setRange(0, 31);
    hpiv->setValue(31);
    modifiers_layout->addRow(tr("HP IV:"), hpiv);

    //atk iv
    QSpinBox* atkiv = new QSpinBox;
    atkiv->setObjectName("defending_atkiv_spinbox");
    atkiv->setRange(0, 31);
    atkiv->setValue(31);
    modifiers_layout->addRow(tr("Atk IV:"), atkiv);

    //def iv
    QSpinBox* defiv = new QSpinBox;
    defiv->setRange(0, 31);
    defiv->setValue(31);
    defiv->setObjectName("defending_defiv_spinbox");
    modifiers_layout->addRow(tr("Def IV:"), defiv);

    //spatk iv
    QSpinBox* spatkiv = new QSpinBox;
    spatkiv->setRange(0, 31);
    spatkiv->setValue(31);
    spatkiv->setObjectName("defending_spatkiv_spinbox");
    modifiers_layout->addRow(tr("Sp. Atk IV:"), spatkiv);

    //spdef iv
    QSpinBox* spdefiv = new QSpinBox;
    spdefiv->setObjectName("defending_spdefiv_spinbox");
    spdefiv->setRange(0, 31);
    spdefiv->setValue(31);
    modifiers_layout->addRow(tr("Sp. Def IV:"), spdefiv);

    //spe iv
    QSpinBox* speiv = new QSpinBox;
    speiv->setObjectName("defending_speiv_spinbox");
    speiv->setRange(0, 31);
    speiv->setValue(31);
    modifiers_layout->addRow(tr("Spe IV:"), speiv);

    defending_layout->addWidget(iv_groupbox);

    //already assigned evs
    QGroupBox* assigned_groupbox = new QGroupBox(tr("Already assigned SPs:"));

    QFormLayout* assigned_layout = new QFormLayout;
    assigned_groupbox->setLayout(assigned_layout);

    //hp evs
    QSpinBox* assigned_hp = new QSpinBox;
    assigned_hp->setObjectName("defending_hpev_spinbox");
    assigned_hp->setRange(0, 32);
    assigned_layout->addRow(tr("HP SPs:"), assigned_hp);

    //atk sps
    QSpinBox* assigned_atk = new QSpinBox;
    assigned_atk->setObjectName("defending_atkev_spinbox");
    assigned_atk->setRange(0, 32);
    assigned_layout->addRow(tr("Atk SPs:"), assigned_atk);

    //def sps
    QSpinBox* assigned_def = new QSpinBox;
    assigned_def->setObjectName("defending_defev_spinbox");
    assigned_def->setRange(0, 32);
    assigned_layout->addRow(tr("Def SPs:"), assigned_def);

    //spatk sps
    QSpinBox* assigned_spatk = new QSpinBox;
    assigned_spatk->setObjectName("defending_spatkev_spinbox");
    assigned_spatk->setRange(0, 32);
    assigned_layout->addRow(tr("Sp. Atk SPs:"), assigned_spatk);

    //spdef sps
    QSpinBox* assigned_spdef = new QSpinBox;
    assigned_spdef->setObjectName("defending_spdefev_spinbox");
    assigned_spdef->setRange(0, 32);
    assigned_layout->addRow(tr("Sp. Def SPs:"), assigned_spdef);

    //spe sps
    QSpinBox* assigned_spe = new QSpinBox;
    assigned_spe->setObjectName("defending_speev_spinbox");
    assigned_spe->setRange(0, 32);
    assigned_layout->addRow(tr("Spe SPs:"), assigned_spe);

    defending_layout->addWidget(assigned_groupbox);

    //CONNECTING SIGNALS
    connect(species, SIGNAL(currentIndexChanged(int)), this, SLOT(setDefendingPokemonSpecies(int)));
    connect(forms, SIGNAL(currentIndexChanged(int)), this, SLOT(setDefendingPokemonForm(int)));

    //setting index 0
    species->setCurrentIndex(1); //setting it to 1 because the signal is currentindexCHANGED, i am so stupid lol
    species->setCurrentIndex(0);

}

void MainWindow::createMovesGroupBox() {
    moves_groupbox = new QGroupBox(tr("Moves:"));

    QHBoxLayout* moves_layout = new QHBoxLayout;

    moves_groupbox->setLayout(moves_layout);

    //Buttons
    QVBoxLayout* button_layout = new QVBoxLayout;
    button_layout->setSpacing(5);
    button_layout->setAlignment(Qt::AlignTop);
    //MOVES ADD BUTTON
    QPushButton* moves_add_button = new QPushButton(tr("Add"));
    moves_add_button->setObjectName("moves_add_button");
    button_layout->addWidget(moves_add_button);

    //MOVES ADD PRESET BUTTON
    QPushButton* moves_preset_button = new QPushButton(tr("Add a Preset"));
    moves_preset_button->setObjectName("moves_preset_button");
    button_layout->addWidget(moves_preset_button);

    //MOVES EDIT BUTTON
    QPushButton* moves_edit_button = new QPushButton(tr("Edit"));
    moves_edit_button->setObjectName("moves_edit_button");
    moves_edit_button->setEnabled(false);
    button_layout->addWidget(moves_edit_button);

    //MOVES DELETE BUTTON
    QPushButton* moves_delete_button = new QPushButton(tr("Delete"));
    moves_delete_button->setObjectName("moves_delete_button");
    moves_delete_button->setEnabled(false);
    button_layout->addWidget(moves_delete_button);

    moves_layout->addLayout(button_layout);

    //creating the two tabs for attack and defense
    QVBoxLayout* tab_layout = new QVBoxLayout;

    QTabWidget* tabs = new QTabWidget;
    tabs->setObjectName("moves_tabs");
    tab_layout->addWidget(tabs);

    //MOVES DEFENSE VIEW
    QTableWidget* moves_defense_view = new QTableWidget(0, 3);
    moves_defense_view->setObjectName("moves_defense_view");
    tabs->addTab(moves_defense_view, tr("Defense"));
    moves_defense_view->horizontalHeader()->setVisible(false);
    moves_defense_view->verticalHeader()->setVisible(false);
    moves_defense_view->setShowGrid(false);
    moves_defense_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    moves_defense_view->setColumnWidth(1, 20);

    //MOVES ATTACK VIEW
    QTableWidget* moves_attack_view = new QTableWidget(0, 3);
    moves_attack_view->setObjectName("moves_attack_view");
    tabs->addTab(moves_attack_view, tr("Attack"));
    moves_attack_view->horizontalHeader()->setVisible(false);
    moves_attack_view->verticalHeader()->setVisible(false);
    moves_attack_view->setShowGrid(false);
    moves_attack_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    moves_attack_view->setColumnWidth(1, 20);

    //PRIORITIZE COMBOBOX
    QComboBox* prioritize_combobox = new QComboBox;
    prioritize_combobox->setObjectName("prioritize_combobox");
    prioritize_combobox->addItem(tr("Prioritize Defense"));
    prioritize_combobox->addItem(tr("Prioritize Attack"));
    tab_layout->addWidget(prioritize_combobox);

    int prioritize_width = prioritize_combobox->minimumSizeHint().width();
    prioritize_combobox->setMaximumWidth(prioritize_width);

    //adding the calculate spread label
    QHBoxLayout* progress_bar_layout = new QHBoxLayout;
    tab_layout->addLayout(progress_bar_layout);

    QLabel* calculating_spread_label = new QLabel(tr("Calculating spread..."));
    calculating_spread_label->setObjectName("calculating_spread_label");
    calculating_spread_label->setVisible(false);
    calculating_spread_label->setAlignment(Qt::AlignRight);
    progress_bar_layout->addWidget(calculating_spread_label);

    QProgressBar* progress_bar = new QProgressBar;
    progress_bar->setObjectName("progress_bar");
    progress_bar->setMinimum(0);
    progress_bar->setMaximum(0);
    progress_bar->setMaximumWidth(60);
    progress_bar_layout->addWidget(progress_bar);
    progress_bar->setVisible(false);

    moves_layout->addLayout(tab_layout);

    //CONNECTING SIGNALS
    connect(moves_defense_view, SIGNAL(cellClicked(int,int)), this, SLOT(setButtonClickable(int,int)));
    connect(moves_attack_view, SIGNAL(cellClicked(int,int)), this, SLOT(setButtonClickable(int,int)));
    connect(moves_delete_button, SIGNAL(clicked(bool)), this, SLOT(eraseMove(bool)));
    connect(moves_preset_button, SIGNAL(clicked(bool)), this, SLOT(addPreset(bool)));
    connect(moves_add_button, SIGNAL(clicked(bool)), this, SLOT(openMoveWindow(bool)));
    connect(moves_edit_button, SIGNAL(clicked(bool)), this, SLOT(openMoveWindowEdit(bool)));
    connect(tabs, SIGNAL(currentChanged(int)), this, SLOT(moveTabChanged(int)));
}

void MainWindow::solveMoveDefense() {
    if( !defense_move_window->isEditMode() ) {
        moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->setRowCount(moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->rowCount()+1);

        auto buffer = turns_def.back().getMoves();

        QString move_name_1;
        if( buffer[0].second.isZ() ) move_name_1 = tr("Z-") + moves_names[buffer[0].second.getMoveIndex()];
        else move_name_1 = moves_names[buffer[0].second.getMoveIndex()];
        QString move1(species_names[buffer[0].first.getPokedexNumber()-1] + " " + move_name_1);

       moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->setItem(turns_def.size()-1, 0, new QTableWidgetItem(move1));

        moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->resizeColumnToContents(0);

        if( turns_def.back().getMoveNum() > 1 ) {
            QString move_name_2;
            if( buffer[1].second.isZ() ) move_name_2 = tr("Z-") + moves_names[buffer[1].second.getMoveIndex()];
            else move_name_2 = moves_names[buffer[1].second.getMoveIndex()];

            QString move2(species_names[buffer[1].first.getPokedexNumber()-1] + " " + move_name_2);

            QTableWidgetItem* plus_sign = new QTableWidgetItem("+");
            plus_sign->setTextAlignment(Qt::AlignCenter);
            moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->setItem(turns_def.size()-1, 1, plus_sign);
            moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->setItem(turns_def.size()-1, 2, new QTableWidgetItem(move2));
            moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->resizeColumnToContents(2);
        }
    }

    else {
        auto buffer = turns_def[moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->currentRow()].getMoves();

        QString move_name_1;
        if( buffer[0].second.isZ() ) move_name_1 = tr("Z-") + moves_names[buffer[0].second.getMoveIndex()];
        else move_name_1 = moves_names[buffer[0].second.getMoveIndex()];
        QString move1(species_names[buffer[0].first.getPokedexNumber()-1] + " " + move_name_1);

        moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->setItem(moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->currentRow(), 0, new QTableWidgetItem(move1));

        moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->resizeColumnToContents(0);

        if( turns_def[moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->currentRow()].getMoveNum() > 1 ) {
            QString move_name_2;
            if( buffer[1].second.isZ() ) move_name_2 = tr("Z-") + moves_names[buffer[1].second.getMoveIndex()];
            else move_name_2 = moves_names[buffer[1].second.getMoveIndex()];

            QString move2(species_names[buffer[1].first.getPokedexNumber()-1] + " " + move_name_2);

            QTableWidgetItem* plus_sign = new QTableWidgetItem("+");
            plus_sign->setTextAlignment(Qt::AlignCenter);
            moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->setItem(moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->currentRow(), 1, plus_sign);
            moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->setItem(moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->currentRow(), 2, new QTableWidgetItem(move2));
            moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->resizeColumnToContents(2);
        }

        else {
            moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->takeItem(moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->currentRow(), 1);
            moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->takeItem(moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->currentRow(), 2);
        }
    }
}

void MainWindow::openMoveWindow(bool checked) {
    //if the defensive window is requested
    if( moves_groupbox->findChild<QTabWidget*>("moves_tabs")->currentIndex() == 0 ) openMoveWindowDefense();

    //if the attacking window is requested
    if( moves_groupbox->findChild<QTabWidget*>("moves_tabs")->currentIndex() == 1 ) openMoveWindowAttack();
}

void MainWindow::openMoveWindowEdit(bool checked) {
    //if the defensive window is requested
    if( moves_groupbox->findChild<QTabWidget*>("moves_tabs")->currentIndex() == 0 ) openMoveWindowEditDefense();

    //if the attacking window is requested
    if( moves_groupbox->findChild<QTabWidget*>("moves_tabs")->currentIndex() == 1 ) openMoveWindowEditAttack();
}

void MainWindow::openMoveWindowDefense() {
    defense_move_window->setAsBlank();
    defense_move_window->setEditMode(false);
    defense_move_window->setModal(true);
    defense_move_window->setVisible(true);
}

void MainWindow::openMoveWindowAttack() {
    attack_move_window->setAsBlank();
    attack_move_window->setEditMode(false);
    attack_move_window->setModal(true);
    attack_move_window->setVisible(true);
}

void MainWindow::openMoveWindowEditDefense() {
    defense_move_window->setAsBlank();
    defense_move_window->setAsTurn(turns_def[moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->currentRow()], modifiers_def[moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->currentRow()]);
    defense_move_window->setEditMode(true);
    defense_move_window->setModal(true);
    defense_move_window->setVisible(true);
}

void MainWindow::openMoveWindowEditAttack() {
    attack_move_window->setAsBlank();
    attack_move_window->setAsTurn(turns_atk[moves_groupbox->findChild<QTableWidget*>("moves_attack_view")->currentRow()], defending_pokemons_in_attack[moves_groupbox->findChild<QTableWidget*>("moves_attack_view")->currentRow()], modifiers_atk[moves_groupbox->findChild<QTableWidget*>("moves_attack_view")->currentRow()]);
    attack_move_window->setEditMode(true);
    attack_move_window->setModal(true);
    attack_move_window->setVisible(true);
}

void MainWindow::addDefenseTurn(const Turn& theTurn, const defense_modifier& theModifier) {
    if( !defense_move_window->isEditMode() ) {
        turns_def.push_back(theTurn);
        modifiers_def.push_back(theModifier);
    }

    else {
        turns_def[ moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->currentRow()] = theTurn;
        modifiers_def[ moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->currentRow()] = theModifier;
    }
}

void MainWindow::clear(QAbstractButton* theButton) {
    if( theButton->objectName() == "clear_button" ) {
        defending_groupbox->findChild<QComboBox*>("defending_species_combobox")->setCurrentIndex(0);
        defending_groupbox->findChild<QComboBox*>("defending_forms_combobox")->setCurrentIndex(0);
        setComboByOriginalIdx(defending_groupbox->findChild<QComboBox*>("defending_nature_combobox"), 0); // Hardy
        setComboByOriginalIdx(defending_groupbox->findChild<QComboBox*>("defending_items_combobox"), 0); // None

        defending_groupbox->findChild<QSpinBox*>("defending_hpiv_spinbox")->setValue(31);
        defending_groupbox->findChild<QSpinBox*>("defending_atkiv_spinbox")->setValue(31);
        defending_groupbox->findChild<QSpinBox*>("defending_defiv_spinbox")->setValue(31);
        defending_groupbox->findChild<QSpinBox*>("defending_spatkiv_spinbox")->setValue(31);
        defending_groupbox->findChild<QSpinBox*>("defending_spdefiv_spinbox")->setValue(31);
        defending_groupbox->findChild<QSpinBox*>("defending_speiv_spinbox")->setValue(31);

        defending_groupbox->findChild<QSpinBox*>("defending_hpev_spinbox")->setValue(0);
        defending_groupbox->findChild<QSpinBox*>("defending_atkev_spinbox")->setValue(0);
        defending_groupbox->findChild<QSpinBox*>("defending_defev_spinbox")->setValue(0);
        defending_groupbox->findChild<QSpinBox*>("defending_spatkev_spinbox")->setValue(0);
        defending_groupbox->findChild<QSpinBox*>("defending_spdefev_spinbox")->setValue(0);
        defending_groupbox->findChild<QSpinBox*>("defending_speev_spinbox")->setValue(0);

        moves_groupbox->findChild<QPushButton*>("moves_edit_button")->setEnabled(false);
        moves_groupbox->findChild<QPushButton*>("moves_delete_button")->setEnabled(false);

        moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->clear();
        moves_groupbox->findChild<QTableWidget*>("moves_attack_view")->clear();
        turns_def.clear();
        modifiers_def.clear();
        turns_atk.clear();
        modifiers_atk.clear();
        defending_pokemons_in_attack.clear();
    }

    else if( theButton->objectName() == "stop_button" ) calculateStop();
}

void MainWindow::calculate() {
    selected_pokemon = new Pokemon(defending_groupbox->findChild<QComboBox*>("defending_species_combobox")->currentData(Qt::UserRole).toInt()+1);
    selected_pokemon->setForm(defending_groupbox->findChild<QComboBox*>("defending_forms_combobox")->currentData(Qt::UserRole).toInt());
    selected_pokemon->setType(0, (Type)defending_groupbox->findChild<QComboBox*>("defending_type1_combobox")->currentData(Qt::UserRole).toInt());
    selected_pokemon->setType(1, (Type)defending_groupbox->findChild<QComboBox*>("defending_type2_combobox")->currentData(Qt::UserRole).toInt());
    selected_pokemon->setNature((Stats::Nature)defending_groupbox->findChild<QComboBox*>("defending_nature_combobox")->currentData(Qt::UserRole).toInt());
    selected_pokemon->setAbility((Ability)defending_groupbox->findChild<QComboBox*>("defending_abilities_combobox")->currentData(Qt::UserRole).toInt());
    selected_pokemon->setItem(Item(defending_groupbox->findChild<QComboBox*>("defending_items_combobox")->currentData(Qt::UserRole).toInt()));

    selected_pokemon->setIV(Stats::HP, defending_groupbox->findChild<QSpinBox*>("defending_hpiv_spinbox")->value());
    selected_pokemon->setIV(Stats::ATK, defending_groupbox->findChild<QSpinBox*>("defending_atkiv_spinbox")->value());
    selected_pokemon->setIV(Stats::DEF, defending_groupbox->findChild<QSpinBox*>("defending_defiv_spinbox")->value());
    selected_pokemon->setIV(Stats::SPATK, defending_groupbox->findChild<QSpinBox*>("defending_spatkiv_spinbox")->value());
    selected_pokemon->setIV(Stats::SPDEF, defending_groupbox->findChild<QSpinBox*>("defending_spdefiv_spinbox")->value());
    selected_pokemon->setIV(Stats::SPE, defending_groupbox->findChild<QSpinBox*>("defending_speiv_spinbox")->value());

    selected_pokemon->setEV(Stats::HP, defending_groupbox->findChild<QSpinBox*>("defending_hpev_spinbox")->value());
    selected_pokemon->setEV(Stats::ATK, defending_groupbox->findChild<QSpinBox*>("defending_atkev_spinbox")->value());
    selected_pokemon->setEV(Stats::DEF, defending_groupbox->findChild<QSpinBox*>("defending_defev_spinbox")->value());
    selected_pokemon->setEV(Stats::SPATK, defending_groupbox->findChild<QSpinBox*>("defending_spatkev_spinbox")->value());
    selected_pokemon->setEV(Stats::SPDEF, defending_groupbox->findChild<QSpinBox*>("defending_spdefev_spinbox")->value());
    selected_pokemon->setEV(Stats::SPE, defending_groupbox->findChild<QSpinBox*>("defending_speev_spinbox")->value());

    //enabling the stop button
    bottom_buttons->findChild<QPushButton*>("stop_button")->setVisible(true);
    //disabling other buttons
    bottom_buttons->findChild<QPushButton*>("calculate_button")->setEnabled(false);
    bottom_buttons->findChild<QPushButton*>("clear_button")->setEnabled(false);
    //enabling the progress bar
    moves_groupbox->findChild<QLabel*>("calculating_spread_label")->setVisible(true);
    moves_groupbox->findChild<QProgressBar*>("progress_bar")->setVisible(true);

    //encapsulating the input into a wrapper
    EVCalculationInput input;
    input.def_turn = turns_def;
    input.def_modifier = modifiers_def;
    input.atk_turn = turns_atk;
    input.atk_modifier = modifiers_atk;
    input.defending_pokemon = defending_pokemons_in_attack;
    input.priority = (Priority)moves_groupbox->findChild<QComboBox*>("prioritize_combobox")->currentIndex();
    future = QtConcurrent::run(selected_pokemon, &Pokemon::calculateEVSDistrisbution, input);
    future_watcher.setFuture(future);
}

/*static*/ bool MainWindow::isGMaxForm(int dex, int form) {
    static const QSet<QPair<int,int>> gmax_forms = {
        {3,2}, {6,3}, {9,2}, {12,1}, {68,1}, {94,2}, {99,1}, {131,1}, {133,2}, {143,1},
        {448,2}, {569,1}, {807,1}, {809,1}, {812,1}, {815,1}, {818,1}, {823,1}, {826,1},
        {834,1}, {839,1}, {841,1}, {842,1}, {844,1}, {849,2}, {849,3}, {851,1}, {858,1},
        {861,1}, {869,1}, {879,1}, {884,1}, {890,1}, {892,2}, {892,3}
    };
    return gmax_forms.contains({dex, form});
}

/*static*/ void MainWindow::populateFormCombo(QComboBox* combo, int dex, int form_count) {
    combo->clear();
    for (int i = 0; i < form_count; i++) {
        if (isGMaxForm(dex, i)) continue;
        QString name = (i == 0) ? "Base" : retrieveFormName(dex, i);
        combo->addItem(name);
        combo->setItemData(combo->count() - 1, i, Qt::UserRole);
    }
}

/*static*/ void MainWindow::setFormComboByFormIdx(QComboBox* combo, int form_idx) {
    for (int i = 0; i < combo->count(); i++) {
        if (combo->itemData(i, Qt::UserRole).toInt() == form_idx) {
            combo->setCurrentIndex(i);
            return;
        }
    }
    combo->setCurrentIndex(0);
}

/*static*/ QString MainWindow::retrieveFormName(const int species, const int form) {
    if (form == 0) return "Base";

    using FKey = QPair<int,int>;
    static const QHash<FKey, QString> form_names = {
        // ── GEN 1 ─────────────────────────────────────────────────────────
        {{3,1},"Mega"},         {{3,2},"G-Max"},
        {{6,1},"Mega X"},       {{6,2},"Mega Y"},       {{6,3},"G-Max"},
        {{9,1},"Mega"},         {{9,2},"G-Max"},
        {{12,1},"G-Max"},
        {{15,1},"Mega"},
        {{18,1},"Mega"},
        {{19,1},"Alola"},
        {{20,1},"Alola"},       {{20,2},"Alola Totem"},
        {{26,1},"Alola"},
        {{27,1},"Alola"},
        {{28,1},"Alola"},
        {{36,1},"Mega"},
        {{37,1},"Alola"},
        {{38,1},"Alola"},
        {{50,1},"Alola"},
        {{51,1},"Alola"},
        {{52,1},"Alola"},       {{52,2},"Galar"},       {{52,3},"G-Max"},
        {{53,1},"Alola"},
        {{58,1},"Hisui"},
        {{59,1},"Hisui"},
        {{65,1},"Mega"},
        {{68,1},"G-Max"},
        {{71,1},"Mega"},
        {{74,1},"Alola"},
        {{75,1},"Alola"},
        {{76,1},"Alola"},
        {{77,1},"Galar"},
        {{78,1},"Galar"},
        {{79,1},"Galar"},
        {{80,1},"Galar"},       {{80,2},"Mega"},
        {{83,1},"Galar"},
        {{88,1},"Alola"},
        {{89,1},"Alola"},
        {{94,1},"Mega"},        {{94,2},"G-Max"},
        {{99,1},"G-Max"},
        {{100,1},"Hisui"},
        {{101,1},"Hisui"},
        {{103,1},"Alola"},
        {{105,1},"Alola"},      {{105,2},"Alola Totem"},
        {{110,1},"Galar"},
        {{115,1},"Mega"},
        {{121,1},"Mega"},
        {{122,1},"Galar"},
        {{127,1},"Mega"},
        {{128,1},"Paldea (Combat)"},{{128,2},"Paldea (Blaze)"},{{128,3},"Paldea (Aqua)"},
        {{130,1},"Mega"},
        {{131,1},"G-Max"},
        {{133,1},"Partner"},    {{133,2},"G-Max"},
        {{142,1},"Mega"},
        {{143,1},"G-Max"},
        {{144,1},"Galar"},
        {{145,1},"Galar"},
        {{146,1},"Galar"},
        {{149,1},"Mega"},
        {{150,1},"Mega X"},     {{150,2},"Mega Y"},
        // ── GEN 2 ─────────────────────────────────────────────────────────
        {{154,1},"Mega"},
        {{157,1},"Hisui"},
        {{160,1},"Mega"},
        {{172,1},"Spiky"},
        {{181,1},"Mega"},
        {{194,1},"Paldea"},
        {{199,1},"Galar"},
        {{208,1},"Mega"},
        {{211,1},"Hisui"},
        {{212,1},"Mega"},
        {{214,1},"Mega"},
        {{215,1},"Hisui"},
        {{222,1},"Galar"},
        {{229,1},"Mega"},
        {{248,1},"Mega"},
        // ── GEN 3 ─────────────────────────────────────────────────────────
        {{254,1},"Mega"},
        {{257,1},"Mega"},
        {{260,1},"Mega"},
        {{263,1},"Galar"},
        {{264,1},"Galar"},
        {{282,1},"Mega"},
        {{302,1},"Mega"},
        {{303,1},"Mega"},
        {{306,1},"Mega"},
        {{308,1},"Mega"},
        {{310,1},"Mega"},
        {{319,1},"Mega"},
        {{323,1},"Mega"},
        {{334,1},"Mega"},
        {{351,1},"Rainy"},      {{351,2},"Snowy"},      {{351,3},"Sunny"},
        {{354,1},"Mega"},
        {{358,1},"Mega"},
        {{359,1},"Mega"},       {{359,2},"Mega Z"},
        {{362,1},"Mega"},
        {{373,1},"Mega"},
        {{376,1},"Mega"},
        {{380,1},"Mega"},
        {{381,1},"Mega"},
        {{382,1},"Primal"},
        {{383,1},"Primal"},
        {{386,1},"Attack"},     {{386,2},"Defense"},    {{386,3},"Speed"},
        // ── GEN 4 ─────────────────────────────────────────────────────────
        {{413,1},"Sandy"},      {{413,2},"Trash"},
        {{421,1},"Sunshine"},
        {{428,1},"Mega"},
        {{445,1},"Mega"},       {{445,2},"Mega B"},
        {{448,1},"Mega"},       {{448,2},"G-Max"},
        {{460,1},"Mega"},
        {{475,1},"Mega"},
        {{479,1},"Fan"},        {{479,2},"Frost"},      {{479,3},"Heat"},
        {{479,4},"Mow"},        {{479,5},"Wash"},
        {{483,1},"Origin"},
        {{484,1},"Origin"},
        {{487,1},"Origin"},
        {{492,1},"Sky"},
        {{493,1},"Bug"},    {{493,2},"Dark"},   {{493,3},"Dragon"},
        {{493,4},"Electric"},{{493,5},"Fairy"},  {{493,6},"Fighting"},
        {{493,7},"Fire"},   {{493,8},"Flying"}, {{493,9},"Ghost"},
        {{493,10},"Grass"}, {{493,11},"Ground"},{{493,12},"Ice"},
        {{493,13},"Poison"},{{493,14},"Psychic"},{{493,15},"Rock"},
        {{493,16},"Steel"}, {{493,17},"Water"},
        // ── GEN 5 ─────────────────────────────────────────────────────────
        {{503,1},"Hisui"},
        {{549,1},"Hisui"},
        {{550,1},"Blue-Striped"},{{550,2},"White-Striped"},
        {{554,1},"Galar"},
        {{555,1},"Galar"},      {{555,2},"Galar Zen"},  {{555,3},"Zen"},
        {{562,1},"Galar"},
        {{569,1},"G-Max"},
        {{570,1},"Hisui"},
        // Mega Eelektross (Champions)
        {{604,1},"Mega"},
        {{571,1},"Hisui"},
        {{618,1},"Galar"},
        {{628,1},"Hisui"},
        {{641,1},"Therian"},
        {{642,1},"Therian"},
        {{645,1},"Therian"},
        {{646,1},"Black"},      {{646,2},"White"},
        {{647,1},"Resolute"},
        {{648,1},"Pirouette"},
        {{649,1},"Burn"},       {{649,2},"Chill"},      {{649,3},"Douse"},
        {{649,4},"Shock"},
        // ── GEN 6 ─────────────────────────────────────────────────────────
        {{658,1},"Battle Bond"},{{658,2},"Mega"},       {{658,3},"Ash"},
        {{668,1},"Female"},
        {{670,1},"Eternal Flower"},{{670,2},"Mega"},
        {{678,1},"Female"},
        {{713,1},"Hisui"},
        {{716,1},"Active"},
        {{718,1},"10%"},        {{718,2},"Complete"},
        {{718,3},"10% (Power)"},{{718,4},"Complete (Power)"},
        {{724,1},"Hisui"},
        {{745,1},"Midnight"},   {{745,2},"Dusk"},
        {{746,1},"School"},
        {{773,1},"Fighting"},   {{773,2},"Flying"},     {{773,3},"Poison"},
        {{773,4},"Ground"},     {{773,5},"Rock"},        {{773,6},"Bug"},
        {{773,7},"Ghost"},      {{773,8},"Steel"},       {{773,9},"Fire"},
        {{773,10},"Water"},     {{773,11},"Grass"},      {{773,12},"Electric"},
        {{773,13},"Psychic"},   {{773,14},"Ice"},        {{773,15},"Dragon"},
        {{773,16},"Dark"},      {{773,17},"Fairy"},
        {{807,1},"G-Max"},
        // ── GEN 8 ─────────────────────────────────────────────────────────
        {{809,1},"G-Max"},
        {{812,1},"G-Max"},
        {{815,1},"G-Max"},
        {{818,1},"G-Max"},
        {{823,1},"G-Max"},
        {{826,1},"G-Max"},
        {{834,1},"G-Max"},
        {{839,1},"G-Max"},
        {{841,1},"G-Max"},
        {{842,1},"G-Max"},
        {{844,1},"G-Max"},
        {{845,1},"Gulping"},    {{845,2},"Gorging"},
        {{849,1},"Low-Key"},    {{849,2},"G-Max"},      {{849,3},"Low-Key G-Max"},
        {{851,1},"G-Max"},
        {{854,1},"Antique"},
        {{855,1},"Antique"},
        {{858,1},"G-Max"},
        {{861,1},"G-Max"},
        {{869,1},"G-Max"},
        {{870,1},"Mega"},
        {{875,1},"No-Ice Face"},
        {{876,1},"Female"},
        {{877,1},"Hangry"},
        {{879,1},"G-Max"},
        {{884,1},"G-Max"},
        {{888,1},"Crowned"},
        {{889,1},"Crowned"},
        {{890,1},"Eternamax"},
        {{892,1},"Rapid Strike"},{{892,2},"G-Max"},    {{892,3},"Rapid Strike G-Max"},
        {{893,1},"Dada"},
        {{898,1},"Ice Rider"},  {{898,2},"Shadow Rider"},
        // ── GEN 9 ─────────────────────────────────────────────────────────
        {{901,1},"Blood Moon"},
        {{902,1},"Female"},
        {{905,1},"Therian"},
        {{916,1},"Female"},
        {{925,1},"Three"},
        {{931,1},"Blue"},       {{931,2},"Yellow"},     {{931,3},"White"},
        {{952,1},"Female"},
        {{964,1},"Hero"},
        {{970,1},"Female"},
        {{978,1},"Droopy"},     {{978,2},"Stretchy"},
        {{982,1},"Three-Segment"},
        {{999,1},"Roaming"},
        {{1017,1},"Wellspring"},{{1017,2},"Hearthflame"},{{1017,3},"Cornerstone"},
        {{1017,4},"Teal Tera"}, {{1017,5},"Wellspring Tera"},
        {{1017,6},"Hearthflame Tera"},{{1017,7},"Cornerstone Tera"},
        {{1024,1},"Terastal"},  {{1024,2},"Stellar"},
    };

    auto it = form_names.find({species, form});
    if (it != form_names.end()) return *it;

    return "Mega";  // Most unknown alt forms are Champions Megas
}

void MainWindow::calculateFinished() {
    std::pair<DefenseResult, AttackResult> result = future.result();

    if( !result.first.isAborted() && !result.second.isAborted()  ) { //if an abort has been requested we don't show the result window
        result_window->setModal(true);

        result_window->setResult(*selected_pokemon, modifiers_def, modifiers_atk, turns_def, turns_atk, defending_pokemons_in_attack, result.first, result.second);
        result_window->show();
    }
    delete selected_pokemon;

    //enabling/disabling buttons back
    bottom_buttons->findChild<QPushButton*>("calculate_button")->setEnabled(true);
    bottom_buttons->findChild<QPushButton*>("clear_button")->setEnabled(true);
    bottom_buttons->findChild<QPushButton*>("stop_button")->setVisible(false);
    moves_groupbox->findChild<QLabel*>("calculating_spread_label")->setVisible(false);
    moves_groupbox->findChild<QProgressBar*>("progress_bar")->setVisible(false);

}

void MainWindow::calculateStop() {
    if( selected_pokemon != nullptr ) selected_pokemon->abortCalculation();
}

void MainWindow::reject() {
    calculateStop(); //just in case a calculation is still in process
    QDialog::reject();
}

void MainWindow::calculateStart() {
    bool prompt = false;

    //checking if we have to show the prompt for the long calculation
    for( auto it = turns_def.begin(); it < turns_def.end(); it++ )
        if( it->getMoveNum() > 1 && it->getHits() > 2 )
            prompt = true;

    if( prompt ) {
        alert_window->setModal(true);
        alert_window->setVisible(true);
    }

    else calculate();
}

void MainWindow::addAttackTurn(const Turn& theTurn, const Pokemon& theDefendingPokemon, const attack_modifier& theModifier) {
    if( !attack_move_window->isEditMode() ) {
        turns_atk.push_back(theTurn);
        modifiers_atk.push_back(theModifier);
        defending_pokemons_in_attack.push_back(theDefendingPokemon);
    }

    else {
        turns_atk[ moves_groupbox->findChild<QTableWidget*>("moves_attack_view")->currentRow()] = theTurn;
        modifiers_atk[ moves_groupbox->findChild<QTableWidget*>("moves_attack_view")->currentRow()] = theModifier;
        defending_pokemons_in_attack[moves_groupbox->findChild<QTableWidget*>("moves_attack_view")->currentRow()] = theDefendingPokemon;
    }
}

void MainWindow::solveMoveAttack() {
    if( !attack_move_window->isEditMode() ) {
        moves_groupbox->findChild<QTableWidget*>("moves_attack_view")->setRowCount(moves_groupbox->findChild<QTableWidget*>("moves_attack_view")->rowCount()+1);

        auto buffer = turns_atk.back().getMoves();
        auto buffer_pokemon = defending_pokemons_in_attack.back();

        QString move_name_1;
        if( buffer[0].second.isZ() ) move_name_1 = tr("Z-") + moves_names[buffer[0].second.getMoveIndex()];
        else move_name_1 = moves_names[buffer[0].second.getMoveIndex()];
        QString move1(move_name_1 + " vs " + species_names[buffer_pokemon.getPokedexNumber()-1]);

        moves_groupbox->findChild<QTableWidget*>("moves_attack_view")->setItem(turns_atk.size()-1, 0, new QTableWidgetItem(move1));
        moves_groupbox->findChild<QTableWidget*>("moves_attack_view")->resizeColumnToContents(0);
    }

    else {
        auto buffer = turns_atk[moves_groupbox->findChild<QTableWidget*>("moves_attack_view")->currentRow()].getMoves();
        auto pokemon_buffer = defending_pokemons_in_attack[moves_groupbox->findChild<QTableWidget*>("moves_attack_view")->currentRow()];

        QString move_name_1;
        if( buffer[0].second.isZ() ) move_name_1 = tr("Z-") + moves_names[buffer[0].second.getMoveIndex()];
        else move_name_1 = moves_names[buffer[0].second.getMoveIndex()];
        QString move1(move_name_1 + " vs " + species_names[pokemon_buffer.getPokedexNumber()-1]);

        moves_groupbox->findChild<QTableWidget*>("moves_attack_view")->setItem(moves_groupbox->findChild<QTableWidget*>("moves_attack_view")->currentRow(), 0, new QTableWidgetItem(move1));

        moves_groupbox->findChild<QTableWidget*>("moves_attack_view")->resizeColumnToContents(0);
    }
}

void MainWindow::addPreset(bool checked) {
    preset_window->setVisible(true);

    std::vector<QString> buffer;
    for(auto it = presets.begin(); it < presets.end(); it++) buffer.push_back(std::get<0>(*it));

    preset_window->loadComboBox(buffer);
}

void MainWindow::addAsPreset(const QString& theName, const Turn& theTurn, const defense_modifier& theDefModifier) {
    presets.push_back(std::make_tuple(theName, theTurn, theDefModifier));

    //create a root node if it doesn't exists
    if( xml_preset.RootElement() == nullptr ) {
        tinyxml2::XMLNode* root = xml_preset.NewElement("Presets");
        xml_preset.InsertFirstChild(root);
    }

    //title
    tinyxml2::XMLElement* title_node = xml_preset.NewElement("Title");
    title_node->SetText(theName.toStdString().c_str());

    auto move_count = std::get<1>(presets.back()).getMoveNum();

    for(auto i = 0; i < move_count; i++) {
        //first of all we find the move category
        auto move_category = std::get<1>(presets.back()).getMoves()[i].second.getMoveCategory();

        //pokemon 1
        std::string pokemon_string;
        if( i == 0 ) pokemon_string = "Pokemon1";
        else pokemon_string = "Pokemon2";
        tinyxml2::XMLElement* pokemon_node = xml_preset.NewElement(pokemon_string.c_str());
        title_node->InsertEndChild(pokemon_node);

        //species
        tinyxml2::XMLElement* species_node = xml_preset.NewElement("Species");
        species_node->SetText(std::get<1>(presets.back()).getMoves()[i].first.getPokedexNumber());
        pokemon_node->InsertEndChild(species_node);

        //form
        tinyxml2::XMLElement* form_node = xml_preset.NewElement("Form");
        unsigned int form = std::get<1>(presets.back()).getMoves()[i].first.getForm();
        form_node->SetText(form);
        pokemon_node->InsertEndChild(form_node);

        //type1
        tinyxml2::XMLElement* type1_node = xml_preset.NewElement("Type1");
        type1_node->SetText(std::get<1>(presets.back()).getMoves()[i].first.getTypes()[form][0]);
        pokemon_node->InsertEndChild(type1_node);

        //type2
        tinyxml2::XMLElement* type2_node = xml_preset.NewElement("Type2");
        type2_node->SetText(std::get<1>(presets.back()).getMoves()[i].first.getTypes()[form][1]);
        pokemon_node->InsertEndChild(type2_node);

        //nature
        tinyxml2::XMLElement* nature_node = xml_preset.NewElement("Nature");
        nature_node->SetText(std::get<1>(presets.back()).getMoves()[i].first.getNature());
        pokemon_node->InsertEndChild(nature_node);

        //ability
        tinyxml2::XMLElement* ability_node = xml_preset.NewElement("Ability");
        ability_node->SetText(std::get<1>(presets.back()).getMoves()[i].first.getAbility());
        pokemon_node->InsertEndChild(ability_node);

        //item
        tinyxml2::XMLElement* item_node = xml_preset.NewElement("Item");
        item_node->SetText(std::get<1>(presets.back()).getMoves()[i].first.getItem().getIndex());
        pokemon_node->InsertEndChild(item_node);

        //iv
        tinyxml2::XMLElement* iv_node = xml_preset.NewElement("IV");
        if( move_category == Move::Category::PHYSICAL ) iv_node->SetText(std::get<1>(presets.back()).getMoves()[i].first.getIV(Stats::ATK));
        else iv_node->SetText(std::get<1>(presets.back()).getMoves()[i].first.getIV(Stats::SPATK));
        pokemon_node->InsertEndChild(iv_node);

        //ev
        tinyxml2::XMLElement* ev_node = xml_preset.NewElement("EV");
        if( move_category == Move::Category::PHYSICAL ) ev_node->SetText(std::get<1>(presets.back()).getMoves()[i].first.getEV(Stats::ATK));
        else ev_node->SetText(std::get<1>(presets.back()).getMoves()[i].first.getEV(Stats::SPATK));
        pokemon_node->InsertEndChild(ev_node);

        //modifier
        tinyxml2::XMLElement* modifier_node = xml_preset.NewElement("Modifier");
        if( move_category == Move::Category::PHYSICAL ) modifier_node->SetText(std::get<1>(presets.back()).getMoves()[i].first.getModifier(Stats::ATK));
        else modifier_node->SetText(std::get<1>(presets.back()).getMoves()[i].first.getModifier(Stats::SPATK));
        pokemon_node->InsertEndChild(modifier_node);

        //move
        tinyxml2::XMLElement* move_node = xml_preset.NewElement("Move");
        move_node->SetText(std::get<1>(presets.back()).getMoves()[i].second.getMoveIndex());
        pokemon_node->InsertEndChild(move_node);

        //movetype
        tinyxml2::XMLElement* movetype_node = xml_preset.NewElement("Movetype");
        movetype_node->SetText(std::get<1>(presets.back()).getMoves()[i].second.getMoveType());
        pokemon_node->InsertEndChild(movetype_node);

        //movetarget
        tinyxml2::XMLElement* movetarget_node = xml_preset.NewElement("Movetarget");
        movetarget_node->SetText(std::get<1>(presets.back()).getMoves()[i].second.getTarget());
        pokemon_node->InsertEndChild(movetarget_node);

        //movecategory
        tinyxml2::XMLElement* movecategory_node = xml_preset.NewElement("Movecategory");
        movecategory_node->SetText(move_category);
        pokemon_node->InsertEndChild(movecategory_node);

        //movebp
        tinyxml2::XMLElement* movebp_node = xml_preset.NewElement("Movebp");
        movebp_node->SetText(std::get<1>(presets.back()).getMoves()[i].second.getBasePower());
        pokemon_node->InsertEndChild(movebp_node);

        //movecrit
        tinyxml2::XMLElement* movecrit_node = xml_preset.NewElement("Movecrit");
        movecrit_node->SetText(std::get<1>(presets.back()).getMoves()[i].second.isCrit());
        pokemon_node->InsertEndChild(movecrit_node);

        //movez
        tinyxml2::XMLElement* movez_node = xml_preset.NewElement("Movez");
        movez_node->SetText(std::get<1>(presets.back()).getMoves()[i].second.isZ());
        pokemon_node->InsertEndChild(movez_node);

        //terrain
        tinyxml2::XMLElement* terrain_node = xml_preset.NewElement("Terrain");
        terrain_node->SetText(std::get<1>(presets.back()).getMoves()[i].second.getTerrain());
        pokemon_node->InsertEndChild(terrain_node);

        //weather
        tinyxml2::XMLElement* weather_node = xml_preset.NewElement("Weather");
        weather_node->SetText(std::get<1>(presets.back()).getMoves()[i].second.getWeather());
        pokemon_node->InsertEndChild(weather_node);
    }

    //defense modifier
    tinyxml2::XMLElement* defmod_node = xml_preset.NewElement("Defmodifier");
    defmod_node->SetText(std::get<1>(std::get<2>(presets.back())));
    title_node->InsertEndChild(defmod_node);

    //spdefense modifier
    tinyxml2::XMLElement* spdefmod_node = xml_preset.NewElement("Spdefmodifier");
    spdefmod_node->SetText(std::get<2>(std::get<2>(presets.back())));
    title_node->InsertEndChild(spdefmod_node);

    //hppercentage modifier
    tinyxml2::XMLElement* hpmod_node = xml_preset.NewElement("HPmodifier");
    hpmod_node->SetText(std::get<0>(std::get<2>(presets.back())));
    title_node->InsertEndChild(hpmod_node);

    //hits
    tinyxml2::XMLElement* hits_node = xml_preset.NewElement("Hits");
    hits_node->SetText(std::get<1>(presets.back()).getHits());
    title_node->InsertEndChild(hits_node);

    //tera type
    tinyxml2::XMLElement* teratype_node = xml_preset.NewElement("TeraType");
    teratype_node->SetText(std::get<3>(std::get<2>(presets.back())));
    title_node->InsertEndChild(teratype_node);

    //terastallized
    tinyxml2::XMLElement* terastallized_node = xml_preset.NewElement("Terastallized");
    terastallized_node->SetText(std::get<4>(std::get<2>(presets.back())));
    title_node->InsertEndChild(terastallized_node);

    //sword of ruin
    tinyxml2::XMLElement* sword_node = xml_preset.NewElement("SwordOfRuin");
    sword_node->SetText(std::get<5>(std::get<2>(presets.back())));
    title_node->InsertEndChild(sword_node);

    //beads of ruin
    tinyxml2::XMLElement* beads_node = xml_preset.NewElement("BeadsOfRuin");
    beads_node->SetText(std::get<6>(std::get<2>(presets.back())));
    title_node->InsertEndChild(beads_node);

    //tablets of ruin
    tinyxml2::XMLElement* tablets_node = xml_preset.NewElement("TabletsOfRuin");
    tablets_node->SetText(std::get<7>(std::get<2>(presets.back())));
    title_node->InsertEndChild(tablets_node);

    //vessel of ruin
    tinyxml2::XMLElement* vessel_node = xml_preset.NewElement("VesselOfRuin");
    vessel_node->SetText(std::get<8>(std::get<2>(presets.back())));
    title_node->InsertEndChild(vessel_node);

    //helping hand
    tinyxml2::XMLElement* helping_hand_node = xml_preset.NewElement("HelpingHand");
    helping_hand_node->SetText(std::get<9>(std::get<2>(presets.back())));
    title_node->InsertEndChild(helping_hand_node);

    //friend guard
    tinyxml2::XMLElement* friend_guard_node = xml_preset.NewElement("FriendGuard");
    friend_guard_node->SetText(std::get<10>(std::get<2>(presets.back())));
    title_node->InsertEndChild(friend_guard_node);

    xml_preset.LastChild()->InsertEndChild(title_node);

    xml_preset.SaveFile("presets.xml");
}

void MainWindow::solveMovePreset(const int index) {
    turns_def.push_back(std::get<1>(presets[index]));
    modifiers_def.push_back(std::get<2>(presets[index]));

    moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->setRowCount(moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->rowCount()+1);

    auto buffer = turns_def.back().getMoves();

    QString move_name_1;
    if( buffer[0].second.isZ() ) move_name_1 = tr("Z-") + moves_names[buffer[0].second.getMoveIndex()];
    else move_name_1 = moves_names[buffer[0].second.getMoveIndex()];
    QString move1(species_names[buffer[0].first.getPokedexNumber()-1] + " " + move_name_1);

   moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->setItem(turns_def.size()-1, 0, new QTableWidgetItem(move1));

    moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->resizeColumnToContents(0);

    if( turns_def.back().getMoveNum() > 1 ) {
        QString move_name_2;
        if( buffer[1].second.isZ() ) move_name_2 = tr("Z-") + moves_names[buffer[1].second.getMoveIndex()];
        else move_name_2 = moves_names[buffer[1].second.getMoveIndex()];

        QString move2(species_names[buffer[1].first.getPokedexNumber()-1] + " " + move_name_2);

        QTableWidgetItem* plus_sign = new QTableWidgetItem("+");
        plus_sign->setTextAlignment(Qt::AlignCenter);
        moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->setItem(turns_def.size()-1, 1, plus_sign);
        moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->setItem(turns_def.size()-1, 2, new QTableWidgetItem(move2));
        moves_groupbox->findChild<QTableWidget*>("moves_defense_view")->resizeColumnToContents(2);
    }
}

void MainWindow::LoadPresetsFromFile() {
    xml_preset.LoadFile("presets.xml");

    tinyxml2::XMLElement* root;

    //create a root node if it doesn't exists
    if( (root = xml_preset.RootElement()) == nullptr ) {
        tinyxml2::XMLNode* root = xml_preset.NewElement("Presets");
        xml_preset.InsertFirstChild(root);
    }

    else {
        auto element = root->FirstChildElement();

        while( element != nullptr ) {
            Preset buffer;

            //name
            std::get<0>(buffer) = element->GetText();

            auto move_element_temp = element->FirstChildElement();
            auto move_element = element->FirstChildElement();

            while( move_element != nullptr ) {
                Pokemon pkmn_buffer(std::atoi(move_element->FirstChildElement("Species")->GetText()));

                Move move_buffer(Moves(std::atoi(move_element->FirstChildElement("Move")->GetText())));
                move_buffer.setMoveCategory(Move::Category(std::atoi(move_element->FirstChildElement("Movecategory")->GetText())));

                pkmn_buffer.setForm(std::atoi(move_element->FirstChildElement("Form")->GetText()));
                pkmn_buffer.setType(0, Type(std::atoi(move_element->FirstChildElement("Type1")->GetText())));
                pkmn_buffer.setType(1, Type(std::atoi(move_element->FirstChildElement("Type2")->GetText())));
                pkmn_buffer.setNature(Stats::Nature(std::atoi(move_element->FirstChildElement("Nature")->GetText())));
                pkmn_buffer.setAbility(Ability(std::atoi(move_element->FirstChildElement("Ability")->GetText())));
                pkmn_buffer.setItem(Item(std::atoi(move_element->FirstChildElement("Item")->GetText())));

                if( move_buffer.getMoveCategory() == Move::Category::PHYSICAL ) {
                    pkmn_buffer.setIV(Stats::ATK, std::atoi(move_element->FirstChildElement("IV")->GetText()));
                    pkmn_buffer.setEV(Stats::ATK, std::atoi(move_element->FirstChildElement("EV")->GetText()));
                    pkmn_buffer.setModifier(Stats::ATK, std::atoi(move_element->FirstChildElement("Modifier")->GetText()));
                }

                else {
                    pkmn_buffer.setIV(Stats::SPATK, std::atoi(move_element->FirstChildElement("IV")->GetText()));
                    pkmn_buffer.setEV(Stats::SPATK, std::atoi(move_element->FirstChildElement("EV")->GetText()));
                    pkmn_buffer.setModifier(Stats::SPATK, std::atoi(move_element->FirstChildElement("Modifier")->GetText()));
                }

                move_buffer.setMoveType(Type(std::atoi(move_element->FirstChildElement("Movetype")->GetText())));
                move_buffer.setTarget(Move::Target(std::atoi(move_element->FirstChildElement("Movetarget")->GetText())));
                move_buffer.setBasePower(std::atoi(move_element->FirstChildElement("Movebp")->GetText()));
                move_buffer.setCrit(move_element->FirstChildElement("Movetype")->BoolText());
                move_buffer.setZ(move_element->FirstChildElement("Movez")->BoolText());
                move_buffer.setTerrain(Move::Terrain(std::atoi(move_element->FirstChildElement("Terrain")->GetText())));
                move_buffer.setWeather(Move::Weather(std::atoi(move_element->FirstChildElement("Weather")->GetText())));

                std::get<1>(buffer).addMove(pkmn_buffer, move_buffer);
                move_element = move_element->NextSiblingElement("Pokemon2");
            }

            std::get<1>(buffer).setHits(std::atoi(move_element_temp->NextSiblingElement("Hits")->GetText()));
            std::get<0>(std::get<2>(buffer)) = std::atoi(move_element_temp->NextSiblingElement("HPmodifier")->GetText());
            std::get<1>(std::get<2>(buffer)) = std::atoi(move_element_temp->NextSiblingElement("Defmodifier")->GetText());
            std::get<2>(std::get<2>(buffer)) = std::atoi(move_element_temp->NextSiblingElement("Spdefmodifier")->GetText());
            auto* tera_elem = move_element_temp->NextSiblingElement("TeraType");
            std::get<3>(std::get<2>(buffer)) = tera_elem ? (Type)std::atoi(tera_elem->GetText()) : Type::Typeless;
            auto* tera_bool_elem = move_element_temp->NextSiblingElement("Terastallized");
            std::get<4>(std::get<2>(buffer)) = tera_bool_elem ? (bool)std::atoi(tera_bool_elem->GetText()) : false;
            auto* sword_elem = move_element_temp->NextSiblingElement("SwordOfRuin");
            std::get<5>(std::get<2>(buffer)) = sword_elem ? (bool)std::atoi(sword_elem->GetText()) : false;
            auto* beads_elem = move_element_temp->NextSiblingElement("BeadsOfRuin");
            std::get<6>(std::get<2>(buffer)) = beads_elem ? (bool)std::atoi(beads_elem->GetText()) : false;
            auto* tablets_elem = move_element_temp->NextSiblingElement("TabletsOfRuin");
            std::get<7>(std::get<2>(buffer)) = tablets_elem ? (bool)std::atoi(tablets_elem->GetText()) : false;
            auto* vessel_elem = move_element_temp->NextSiblingElement("VesselOfRuin");
            std::get<8>(std::get<2>(buffer)) = vessel_elem ? (bool)std::atoi(vessel_elem->GetText()) : false;
            auto* hh_elem = move_element_temp->NextSiblingElement("HelpingHand");
            std::get<9>(std::get<2>(buffer)) = hh_elem ? (bool)std::atoi(hh_elem->GetText()) : false;
            auto* fg_elem = move_element_temp->NextSiblingElement("FriendGuard");
            std::get<10>(std::get<2>(buffer)) = fg_elem ? (bool)std::atoi(fg_elem->GetText()) : false;

            presets.push_back(buffer);
            element = element->NextSiblingElement();
        }
    }
}
